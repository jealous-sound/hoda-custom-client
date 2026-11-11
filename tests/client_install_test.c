#undef HOD_VERSION
#define HOD_VERSION "v1.0.0"
#undef HOD_RELEASE_PUBLIC_KEY
#define HOD_RELEASE_PUBLIC_KEY "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"

#define main hod_game_main
#include "../source/main.c"
#undef main

#include <sys/wait.h>
#include <unistd.h>

#include "../source/vendors/tweetnacl.h"
#include "zip_fixture.h"

#define TEST_TEMPORARY_TEMPLATE "hod-install-XXXXXX"
#define TEST_NEW_VERSION "v1.1.0"
#define TEST_OLDER_RELEASE "v0.9.0"
#define TEST_LATER_RELEASE "v1.2.0"
#define TEST_PLATFORM "macos"
#define TEST_OTHER_PLATFORM "windows"
#define TEST_ARCHIVE "Game-build.zip"
#define TEST_EXECUTABLE "Game.app/Contents/MacOS/Game"
#define TEST_STARTED_MARKER "Game.app/Contents/MacOS/started.txt"
#define TEST_MANIFEST_CAP 1024
#define TEST_SIGNATURE_HEX_CAP (RELEASE_SIGNATURE_BYTES * 2 + 1)
#define TEST_RFC8032_SECRET_KEY_1                                                                                      \
    "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60" HOD_RELEASE_PUBLIC_KEY
#define TEST_RFC8032_SIGNATURE_1                                                                                       \
    "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24" \
    "655141438e7a100b"
#define TEST_RFC8032_PUBLIC_KEY_2 "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c"
#define TEST_RFC8032_SECRET_KEY_2                                                                                      \
    "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb" TEST_RFC8032_PUBLIC_KEY_2
#define TEST_RFC8032_MESSAGE_2 "\x72"
#define TEST_RFC8032_SIGNATURE_2                                                                                       \
    "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aee" \
    "b00d291612bb0c00"
#define TEST_HAND_OVER_DECLINED 77
#define TEST_WAIT_STEP_MICROSECONDS 10000
#define TEST_WAIT_STEPS 1000

typedef struct {
    char server[TEST_PATH_CAP];
    char data[TEST_PATH_CAP];
    ClientInstallPlan plan;
} TestSite;

static char test_base[TEST_PATH_CAP];
static int test_site_count;

static const TestEntry startable_build[] = {
    {"Game.app/", "", false},
    {TEST_EXECUTABLE, "#!/bin/sh\necho started > started.txt\n", true},
    {"Game.app/Contents/Info.plist", "<plist/>", false},
    {"Game.app/Contents/MacOS/._Game", "resource fork", false},
    {"__MACOSX/Game.app/._Info.plist", "resource fork", false},
};
static const int startable_build_count = (int)(sizeof(startable_build) / sizeof(startable_build[0]));

static TestSite TestSiteCreate(void) {
    TestSite site = {0};
    char root[TEST_PATH_CAP];
    TestFormat(root, sizeof(root), "%s/site%d", test_base, test_site_count++);
    TestFormat(site.server, sizeof(site.server), "%s/server", root);
    TestFormat(site.data, sizeof(site.data), "%s/data", root);
    assert(ContentMakeDirectories(site.server));
    assert(ContentMakeDirectories(site.data));
    TestFormat(site.plan.root, sizeof(site.plan.root), "%s/%s", site.data, CLIENT_INSTALL_DIRECTORY);
    TestFormat(site.plan.release_url, sizeof(site.plan.release_url), "file://%s", site.server);
    StrCopy(site.plan.public_key, sizeof(site.plan.public_key), HOD_RELEASE_PUBLIC_KEY);
    StrCopy(site.plan.platform, sizeof(site.plan.platform), TEST_PLATFORM);
    StrCopy(site.plan.running, sizeof(site.plan.running), HOD_VERSION);
    StrCopy(site.plan.version, sizeof(site.plan.version), TEST_NEW_VERSION);
    return site;
}

static void TestSign(const char *secret_key_hex, const char *message, size_t length, char *signature_hex) {
    unsigned char secret_key[crypto_sign_SECRETKEYBYTES];
    assert(ClientInstallReadHex(secret_key_hex, secret_key, sizeof(secret_key)));
    unsigned char *signed_message = malloc(length + crypto_sign_BYTES);
    assert(signed_message);
    unsigned long long signed_length = 0;
    assert(crypto_sign(signed_message, &signed_length, (const unsigned char *)message, length, secret_key) == 0);
    for (int i = 0; i < crypto_sign_BYTES; i++) snprintf(signature_hex + i * 2, 3, "%02x", signed_message[i]);
    free(signed_message);
}

static void TestReleasePath(const TestSite *site, const char *version, const char *file, char *out, size_t capacity) {
    char directory[TEST_PATH_CAP];
    TestFormat(directory, sizeof(directory), "%s/%s", site->server, version);
    assert(ContentMakeDirectories(directory));
    TestFormat(out, capacity, "%s/%s", directory, file);
}

static ClientBuild TestPublishArchive(const TestSite *site, const char *version, const TestEntry *entries, int count) {
    ClientBuild build = {0};
    StrCopy(build.file, sizeof(build.file), TEST_ARCHIVE);
    StrCopy(build.executable, sizeof(build.executable), TEST_EXECUTABLE);
    char path[TEST_PATH_CAP];
    TestReleasePath(site, version, build.file, path, sizeof(path));
    TestWriteZip(path, entries, count);
    assert(ContentFileDigest(path, &build.size, build.sha256));
    return build;
}

static void TestManifest(
    char *out, size_t capacity, int format, const char *version, const char *platform, const ClientBuild *build
) {
    TestFormat(
        out,
        capacity,
        "{\"format\":%d,\"version\":\"%s\",\"builds\":[{\"platform\":\"%s\",\"file\":\"%s\",\"size\":%lld,"
        "\"sha256\":\"%s\",\"executable\":\"%s\"}]}",
        format,
        version,
        platform,
        build->file,
        build->size,
        build->sha256,
        build->executable
    );
}

static void
TestPublishSigned(const TestSite *site, const char *version, const char *manifest, const char *secret_key_hex) {
    char path[TEST_PATH_CAP], signature[TEST_SIGNATURE_HEX_CAP + 1];
    TestReleasePath(site, version, CLIENT_INSTALL_MANIFEST_FILE, path, sizeof(path));
    assert(ContentWriteFile(path, manifest, strlen(manifest)));
    TestSign(secret_key_hex, manifest, strlen(manifest), signature);
    strcat(signature, "\n");
    TestReleasePath(site, version, CLIENT_INSTALL_SIGNATURE_FILE, path, sizeof(path));
    assert(ContentWriteFile(path, signature, strlen(signature)));
}

static ClientBuild TestPublishRelease(
    const TestSite *site, const char *version, const char *platform, const TestEntry *entries, int count
) {
    ClientBuild build = TestPublishArchive(site, version, entries, count);
    char manifest[TEST_MANIFEST_CAP];
    TestManifest(manifest, sizeof(manifest), CLIENT_INSTALL_MANIFEST_FORMAT, version, platform, &build);
    TestPublishSigned(site, version, manifest, TEST_RFC8032_SECRET_KEY_1);
    return build;
}

static ClientInstallError TestInstall(const TestSite *site) {
    ClientInstallJob job = {.lock = PTHREAD_MUTEX_INITIALIZER};
    return ClientInstallApply(&job, &site->plan);
}

static void TestInstalledPath(const TestSite *site, const char *relative, char *out, size_t capacity) {
    TestFormat(out, capacity, "%s/%s", site->plan.root, relative);
}

static bool TestInstalledFileExists(const TestSite *site, const char *relative) {
    char path[TEST_PATH_CAP];
    TestInstalledPath(site, relative, path, sizeof(path));
    return FileExists(path);
}

static bool TestInstalledFolderExists(const TestSite *site, const char *relative) {
    char path[TEST_PATH_CAP];
    TestInstalledPath(site, relative, path, sizeof(path));
    return DirectoryExists(path);
}

static bool TestInstalledFileHolds(const TestSite *site, const char *relative, const char *text) {
    char path[TEST_PATH_CAP];
    TestInstalledPath(site, relative, path, sizeof(path));
    if (!FileExists(path)) return false;
    char *loaded = LoadFileText(path);
    bool same = loaded && strcmp(loaded, text) == 0;
    UnloadFileText(loaded);
    return same;
}

static bool TestNothingWasInstalled(const TestSite *site) {
    return !TestInstalledFolderExists(site, site->plan.version) &&
           !TestInstalledFolderExists(site, TextFormat("%s%s", site->plan.version, CLIENT_INSTALL_UNFINISHED_SUFFIX)) &&
           !TestInstalledFileExists(site, CLIENT_INSTALL_STATE_FILE);
}

static ClientInstallState TestState(const TestSite *site) {
    ClientInstallState state;
    ClientInstallStateLoad(site->plan.root, &state);
    return state;
}

static bool TestSignatureAccepted(const char *message, size_t length, const char *signature, const char *public_key) {
    return ClientInstallManifestSigned((const unsigned char *)message, length, signature, public_key);
}

static void TestStandardEd25519SignaturesVerify(void) {
    assert(TestSignatureAccepted("", 0, TEST_RFC8032_SIGNATURE_1, HOD_RELEASE_PUBLIC_KEY));
    assert(TestSignatureAccepted(TEST_RFC8032_MESSAGE_2, 1, TEST_RFC8032_SIGNATURE_2, TEST_RFC8032_PUBLIC_KEY_2));
    assert(!TestSignatureAccepted(TEST_RFC8032_MESSAGE_2, 1, TEST_RFC8032_SIGNATURE_2, HOD_RELEASE_PUBLIC_KEY));
    assert(!TestSignatureAccepted("s", 1, TEST_RFC8032_SIGNATURE_2, TEST_RFC8032_PUBLIC_KEY_2));
    assert(!TestSignatureAccepted(TEST_RFC8032_MESSAGE_2, 1, TEST_RFC8032_SIGNATURE_1, TEST_RFC8032_PUBLIC_KEY_2));

    char signature[TEST_SIGNATURE_HEX_CAP];
    TestSign(TEST_RFC8032_SECRET_KEY_2, TEST_RFC8032_MESSAGE_2, 1, signature);
    assert(strcmp(signature, TEST_RFC8032_SIGNATURE_2) == 0);
}

static void TestSignaturesAndKeysMustBeWholeHexStrings(void) {
    assert(ClientInstallKeyConfigured(HOD_RELEASE_PUBLIC_KEY));
    assert(ClientInstallKeyConfigured(HOD_RELEASE_PUBLIC_KEY "\n"));
    assert(!ClientInstallKeyConfigured(""));
    assert(!ClientInstallKeyConfigured("d75a98"));
    assert(!ClientInstallKeyConfigured(HOD_RELEASE_PUBLIC_KEY "00"));
    assert(!ClientInstallKeyConfigured("D75A980182B10AB7D54BFED3C964073A0EE172F3DAA62325AF021A68F707511A"));
    assert(!TestSignatureAccepted("", 0, "", HOD_RELEASE_PUBLIC_KEY));
    assert(!TestSignatureAccepted("", 0, TEST_RFC8032_SIGNATURE_1 "00", HOD_RELEASE_PUBLIC_KEY));
    assert(!TestSignatureAccepted("", 0, TEST_RFC8032_SIGNATURE_1, ""));
}

static void TestOnlyTaggedReleasesAreInstalled(void) {
    assert(ClientInstallReleaseBuild("v1.1.0"));
    assert(ClientInstallReleaseBuild("v1.1.0-rc.1"));
    assert(!ClientInstallReleaseBuild("v1.1.0-3-gabc1234"));
    assert(!ClientInstallReleaseBuild("v1.1.0-dirty"));
    assert(!ClientInstallReleaseBuild("dev"));
    assert(!ClientInstallReleaseBuild(""));
}

static bool TestManifestOffers(const TestSite *site, const char *manifest) {
    ClientBuild build;
    return ClientInstallManifestBuild(manifest, &site->plan, &build);
}

static bool TestManifestOffersBuild(const TestSite *site, int format, const char *platform, const ClientBuild *build) {
    char manifest[TEST_MANIFEST_CAP];
    TestManifest(manifest, sizeof(manifest), format, site->plan.version, platform, build);
    return TestManifestOffers(site, manifest);
}

static void TestManifestOffersOnlyASafeBuildForThisPlatform(void) {
    TestSite site = TestSiteCreate();
    ClientBuild good = TestPublishArchive(&site, TEST_NEW_VERSION, startable_build, startable_build_count);
    char manifest[TEST_MANIFEST_CAP];
    TestManifest(manifest, sizeof(manifest), CLIENT_INSTALL_MANIFEST_FORMAT, TEST_NEW_VERSION, TEST_PLATFORM, &good);
    ClientBuild read;
    assert(ClientInstallManifestBuild(manifest, &site.plan, &read));
    assert(strcmp(read.file, good.file) == 0 && strcmp(read.sha256, good.sha256) == 0);
    assert(read.size == good.size && strcmp(read.executable, good.executable) == 0);

    assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT + 1, TEST_PLATFORM, &good));
    assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT, TEST_OTHER_PLATFORM, &good));
    assert(!TestManifestOffers(&site, "{\"format\":1,\"version\":\"" TEST_NEW_VERSION "\"}"));
    assert(!TestManifestOffers(&site, "not json"));

    const char *unsafe_files[] = {
        "../Game-build.zip", "builds/Game-build.zip", ".Game-build.zip", "Game build.zip", ""
    };
    for (size_t i = 0; i < sizeof(unsafe_files) / sizeof(unsafe_files[0]); i++) {
        ClientBuild unsafe = good;
        StrCopy(unsafe.file, sizeof(unsafe.file), unsafe_files[i]);
        assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT, TEST_PLATFORM, &unsafe));
    }
    const char *unsafe_executables[] = {"../Game", "/usr/bin/env", "Game.app/../../Game", "C:/Game.exe", ""};
    for (size_t i = 0; i < sizeof(unsafe_executables) / sizeof(unsafe_executables[0]); i++) {
        ClientBuild unsafe = good;
        StrCopy(unsafe.executable, sizeof(unsafe.executable), unsafe_executables[i]);
        assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT, TEST_PLATFORM, &unsafe));
    }
    ClientBuild empty = good, huge = good, undigested = good;
    empty.size = 0;
    huge.size = CLIENT_INSTALL_MAX_ARCHIVE_BYTES + 1;
    StrCopy(undigested.sha256, sizeof(undigested.sha256), "abc123");
    assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT, TEST_PLATFORM, &empty));
    assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT, TEST_PLATFORM, &huge));
    assert(!TestManifestOffersBuild(&site, CLIENT_INSTALL_MANIFEST_FORMAT, TEST_PLATFORM, &undigested));
}

static void TestManifestMustNameTheRequestedNewerRelease(void) {
    TestSite site = TestSiteCreate();
    ClientBuild build = TestPublishArchive(&site, TEST_NEW_VERSION, startable_build, startable_build_count);
    char manifest[TEST_MANIFEST_CAP];
    TestManifest(manifest, sizeof(manifest), CLIENT_INSTALL_MANIFEST_FORMAT, TEST_NEW_VERSION, TEST_PLATFORM, &build);
    assert(TestManifestOffers(&site, manifest));

    TestSite asked_for_later = site;
    StrCopy(asked_for_later.plan.version, sizeof(asked_for_later.plan.version), TEST_LATER_RELEASE);
    assert(!TestManifestOffers(&asked_for_later, manifest));

    TestSite already_running = site;
    StrCopy(already_running.plan.running, sizeof(already_running.plan.running), TEST_NEW_VERSION);
    assert(!TestManifestOffers(&already_running, manifest));

    TestSite running_later = site;
    StrCopy(running_later.plan.running, sizeof(running_later.plan.running), TEST_LATER_RELEASE);
    assert(!TestManifestOffers(&running_later, manifest));
}

static void TestSignedReleaseInstallsBesideTheRunningBuild(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);

    assert(TestInstalledFileHolds(&site, TEST_NEW_VERSION "/" TEST_EXECUTABLE, startable_build[1].text));
    assert(TestInstalledFileHolds(&site, TEST_NEW_VERSION "/Game.app/Contents/Info.plist", startable_build[2].text));
    assert(!TestInstalledFileExists(&site, TEST_NEW_VERSION "/Game.app/Contents/MacOS/._Game"));
    assert(!TestInstalledFolderExists(&site, TEST_NEW_VERSION "/__MACOSX"));
    assert(!TestInstalledFolderExists(&site, TEST_NEW_VERSION CLIENT_INSTALL_UNFINISHED_SUFFIX));
    char executable[TEST_PATH_CAP];
    TestInstalledPath(&site, TEST_NEW_VERSION "/" TEST_EXECUTABLE, executable, sizeof(executable));
    assert(access(executable, X_OK) == 0);

    char downloads[TEST_PATH_CAP];
    TestInstalledPath(&site, CLIENT_INSTALL_DOWNLOADS, downloads, sizeof(downloads));
    FilePathList leftovers = LoadDirectoryFiles(downloads);
    assert(leftovers.count == 0);
    UnloadDirectoryFiles(leftovers);

    ClientInstallState state = TestState(&site);
    assert(strcmp(state.version, TEST_NEW_VERSION) == 0 && state.attempts == 0 && !state.rejected[0]);
    assert(strcmp(state.executable, TEST_NEW_VERSION "/" TEST_EXECUTABLE) == 0);
    assert(ClientInstallLaunchChoice(&state, HOD_VERSION) == CLIENT_LAUNCH_INSTALLED);
}

static void TestInstallingAgainReplacesTheEarlierCopy(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    const TestEntry rebuilt[] = {{TEST_EXECUTABLE, "rebuilt", false}};
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, rebuilt, 1);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    assert(TestInstalledFileHolds(&site, TEST_NEW_VERSION "/" TEST_EXECUTABLE, rebuilt[0].text));
    assert(!TestInstalledFileExists(&site, TEST_NEW_VERSION "/Game.app/Contents/Info.plist"));
}

static void TestManifestChangedAfterSigningIsRefused(void) {
    TestSite site = TestSiteCreate();
    ClientBuild build =
        TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    const TestEntry other_build[] = {{TEST_EXECUTABLE, "someone else's build", false}};
    char archive[TEST_PATH_CAP], manifest_path[TEST_PATH_CAP], manifest[TEST_MANIFEST_CAP];
    TestReleasePath(&site, TEST_NEW_VERSION, TEST_ARCHIVE, archive, sizeof(archive));
    TestWriteZip(archive, other_build, 1);
    assert(ContentFileDigest(archive, &build.size, build.sha256));
    TestManifest(manifest, sizeof(manifest), CLIENT_INSTALL_MANIFEST_FORMAT, TEST_NEW_VERSION, TEST_PLATFORM, &build);
    TestReleasePath(&site, TEST_NEW_VERSION, CLIENT_INSTALL_MANIFEST_FILE, manifest_path, sizeof(manifest_path));
    assert(ContentWriteFile(manifest_path, manifest, strlen(manifest)));

    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_SIGNATURE);
    assert(TestNothingWasInstalled(&site));
}

static void TestManifestSignedWithAnotherKeyIsRefused(void) {
    TestSite site = TestSiteCreate();
    ClientBuild build = TestPublishArchive(&site, TEST_NEW_VERSION, startable_build, startable_build_count);
    char manifest[TEST_MANIFEST_CAP];
    TestManifest(manifest, sizeof(manifest), CLIENT_INSTALL_MANIFEST_FORMAT, TEST_NEW_VERSION, TEST_PLATFORM, &build);
    TestPublishSigned(&site, TEST_NEW_VERSION, manifest, TEST_RFC8032_SECRET_KEY_2);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_SIGNATURE);
    assert(TestNothingWasInstalled(&site));
}

static void TestReleaseWithoutASignatureIsRefused(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    char signature[TEST_PATH_CAP];
    TestReleasePath(&site, TEST_NEW_VERSION, CLIENT_INSTALL_SIGNATURE_FILE, signature, sizeof(signature));
    assert(remove(signature) == 0);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_DOWNLOAD);
    assert(TestNothingWasInstalled(&site));

    assert(ContentWriteFile(signature, "", 0));
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_SIGNATURE);
    assert(TestNothingWasInstalled(&site));
}

static void TestBuildWithoutAReleaseKeyInstallsNothing(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    site.plan.public_key[0] = '\0';
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_SIGNATURE);
    assert(TestNothingWasInstalled(&site));
}

static void TestOlderSignedReleaseCannotBeServedAsTheNewOne(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_OLDER_RELEASE, TEST_PLATFORM, startable_build, startable_build_count);
    const char *files[] = {TEST_ARCHIVE, CLIENT_INSTALL_MANIFEST_FILE, CLIENT_INSTALL_SIGNATURE_FILE};
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char source[TEST_PATH_CAP], target[TEST_PATH_CAP];
        TestReleasePath(&site, TEST_OLDER_RELEASE, files[i], source, sizeof(source));
        TestReleasePath(&site, TEST_NEW_VERSION, files[i], target, sizeof(target));
        int size = 0;
        unsigned char *data = LoadFileData(source, &size);
        assert(data && ContentWriteFile(target, data, (size_t)size));
        UnloadFileData(data);
    }
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_MANIFEST);
    assert(TestNothingWasInstalled(&site));

    StrCopy(site.plan.version, sizeof(site.plan.version), TEST_OLDER_RELEASE);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_MANIFEST);
    assert(TestNothingWasInstalled(&site));
}

static void TestOverwriteArchive(const TestSite *site, long long bytes) {
    char archive[TEST_PATH_CAP];
    TestReleasePath(site, TEST_NEW_VERSION, TEST_ARCHIVE, archive, sizeof(archive));
    FILE *file = fopen(archive, "wb");
    assert(file);
    for (long long i = 0; i < bytes; i++) fputc('x', file);
    fclose(file);
}

static void TestArchiveThatDoesNotMatchTheManifestIsDiscarded(void) {
    TestSite site = TestSiteCreate();
    const TestEntry signed_build[] = {{TEST_EXECUTABLE, "signed build", false}};
    const TestEntry forged_build[] = {{TEST_EXECUTABLE, "forged build", false}};
    ClientBuild build = TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, signed_build, 1);
    char archive[TEST_PATH_CAP];
    TestReleasePath(&site, TEST_NEW_VERSION, TEST_ARCHIVE, archive, sizeof(archive));
    TestWriteZip(archive, forged_build, 1);
    assert(GetFileLength(archive) == build.size);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_CORRUPT);
    assert(TestNothingWasInstalled(&site));

    TestOverwriteArchive(&site, build.size);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_CORRUPT);
    assert(TestNothingWasInstalled(&site));

    TestOverwriteArchive(&site, build.size * 2);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_CORRUPT);
    assert(TestNothingWasInstalled(&site));
    assert(!TestInstalledFileExists(&site, TextFormat("%s/%s.zip", CLIENT_INSTALL_DOWNLOADS, build.sha256)));
}

static void TestArchiveWritingOutsideItsFolderIsRefused(void) {
    TestSite site = TestSiteCreate();
    const TestEntry escaping[] = {{TEST_EXECUTABLE, "build", false}, {"../escaped.txt", "outside", false}};
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, escaping, 2);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_CORRUPT);
    assert(TestNothingWasInstalled(&site));
    assert(!TestInstalledFileExists(&site, "escaped.txt"));
}

static void TestArchiveWithoutItsExecutableIsRefused(void) {
    TestSite site = TestSiteCreate();
    const TestEntry incomplete[] = {{"Game.app/Contents/Info.plist", "<plist/>", false}};
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, incomplete, 1);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_CORRUPT);
    assert(TestNothingWasInstalled(&site));
}

static void TestInterruptedArchiveDownloadResumes(void) {
    TestSite site = TestSiteCreate();
    ClientBuild build =
        TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    char archive[TEST_PATH_CAP], partial[TEST_PATH_CAP], downloads[TEST_PATH_CAP];
    TestReleasePath(&site, TEST_NEW_VERSION, TEST_ARCHIVE, archive, sizeof(archive));
    TestInstalledPath(&site, CLIENT_INSTALL_DOWNLOADS, downloads, sizeof(downloads));
    assert(ContentMakeDirectories(downloads));
    TestFormat(partial, sizeof(partial), "%s/%s%s", downloads, build.sha256, CLIENT_INSTALL_ARCHIVE_EXTENSION);
    int size = 0;
    unsigned char *data = LoadFileData(archive, &size);
    assert(data && ContentWriteFile(partial, data, (size_t)size / 2));
    UnloadFileData(data);

    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    assert(TestInstalledFileHolds(&site, TEST_NEW_VERSION "/" TEST_EXECUTABLE, startable_build[1].text));
}

static void TestCancelledInstallRecordsNothing(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    ClientInstallJob job = {.lock = PTHREAD_MUTEX_INITIALIZER, .cancel = true};
    assert(ClientInstallApply(&job, &site.plan) == CLIENT_INSTALL_ERROR_CANCELLED);
    assert(!TestInstalledFileExists(&site, CLIENT_INSTALL_STATE_FILE));
    assert(ClientInstallPhaseAfter(CLIENT_INSTALL_ERROR_CANCELLED) == CLIENT_INSTALL_IDLE);
    assert(ClientInstallPhaseAfter(CLIENT_INSTALL_ERROR_CORRUPT) == CLIENT_INSTALL_FAILED);
    assert(ClientInstallPhaseAfter(CLIENT_INSTALL_ERROR_NONE) == CLIENT_INSTALL_READY);
}

static ClientLaunchChoice TestLaunchChoice(const char *installed, int attempts, const char *running) {
    ClientInstallState state = {.attempts = attempts};
    StrCopy(state.version, sizeof(state.version), installed);
    return ClientInstallLaunchChoice(&state, running);
}

static void TestLaunchPrefersANewerInstalledBuildUntilItFailsTwice(void) {
    assert(TestLaunchChoice("", 0, "v1.0.0") == CLIENT_LAUNCH_SELF);
    assert(TestLaunchChoice("v1.1.0", 0, "v1.0.0") == CLIENT_LAUNCH_INSTALLED);
    assert(TestLaunchChoice("v1.1.0", CLIENT_INSTALL_LAUNCH_ATTEMPTS - 1, "v1.0.0") == CLIENT_LAUNCH_INSTALLED);
    assert(TestLaunchChoice("v1.1.0", CLIENT_INSTALL_LAUNCH_ATTEMPTS, "v1.0.0") == CLIENT_LAUNCH_GIVE_UP_ON_INSTALLED);
    assert(TestLaunchChoice("v1.1.0", 0, "v1.1.0") == CLIENT_LAUNCH_SELF);
    assert(TestLaunchChoice("v1.1.0", 0, "v1.2.0") == CLIENT_LAUNCH_SELF);
    assert(TestLaunchChoice("v1.1.0", CLIENT_INSTALL_LAUNCH_ATTEMPTS, "v1.2.0") == CLIENT_LAUNCH_SELF);
    assert(TestLaunchChoice("v1.1.0", 0, "v1.1.0-rc.1") == CLIENT_LAUNCH_INSTALLED);
}

static void TestStateSurvivesARoundTripAndDropsUnsafeExecutables(void) {
    TestSite site = TestSiteCreate();
    ClientInstallState saved = {.attempts = 1};
    StrCopy(saved.version, sizeof(saved.version), TEST_NEW_VERSION);
    StrCopy(saved.executable, sizeof(saved.executable), TEST_NEW_VERSION "/" TEST_EXECUTABLE);
    StrCopy(saved.rejected, sizeof(saved.rejected), TEST_OLDER_RELEASE);
    assert(ContentMakeDirectories(site.plan.root));
    assert(ClientInstallStateSave(site.plan.root, &saved));
    ClientInstallState loaded;
    assert(ClientInstallStateLoad(site.plan.root, &loaded));
    assert(strcmp(loaded.version, saved.version) == 0 && strcmp(loaded.executable, saved.executable) == 0);
    assert(loaded.attempts == saved.attempts && strcmp(loaded.rejected, saved.rejected) == 0);

    StrCopy(saved.executable, sizeof(saved.executable), "../../../../bin/sh");
    assert(ClientInstallStateSave(site.plan.root, &saved));
    assert(ClientInstallStateLoad(site.plan.root, &loaded));
    assert(!loaded.version[0] && !loaded.executable[0] && strcmp(loaded.rejected, TEST_OLDER_RELEASE) == 0);
    assert(ClientInstallLaunchChoice(&loaded, HOD_VERSION) == CLIENT_LAUNCH_SELF);

    char path[TEST_PATH_CAP];
    TestInstalledPath(&site, CLIENT_INSTALL_STATE_FILE, path, sizeof(path));
    assert(ContentWriteFile(path, "{", 1));
    assert(!ClientInstallStateLoad(site.plan.root, &loaded) && !loaded.version[0]);
}

static void TestBuildThatKeepsFailingIsRejectedAndForgotten(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    ClientInstallState state = TestState(&site);
    ClientInstallGiveUpOn(site.plan.root, &state);
    ClientInstallState stored = TestState(&site);
    assert(!stored.version[0] && !stored.executable[0] && stored.attempts == 0);
    assert(strcmp(stored.rejected, TEST_NEW_VERSION) == 0);
    assert(ClientInstallLaunchChoice(&stored, HOD_VERSION) == CLIENT_LAUNCH_SELF);
}

static void TestConfirmedLaunchClearsAttemptsAndRemovesEverythingElse(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    char stale_build[TEST_PATH_CAP], stale_download[TEST_PATH_CAP];
    TestInstalledPath(&site, TEST_OLDER_RELEASE "/Game.app/Contents/MacOS/Game", stale_build, sizeof(stale_build));
    TestInstalledPath(&site, CLIENT_INSTALL_DOWNLOADS "/half.zip", stale_download, sizeof(stale_download));
    assert(ContentWriteFile(stale_build, "old", 3) && ContentWriteFile(stale_download, "half", 4));
    ClientInstallState state = TestState(&site);
    state.attempts = 1;
    assert(ClientInstallStateSave(site.plan.root, &state));

    ClientInstallSettle(site.plan.root, &state, TEST_NEW_VERSION);
    ClientInstallRemoveStale(site.plan.root, &state);
    assert(TestState(&site).attempts == 0 && strcmp(TestState(&site).version, TEST_NEW_VERSION) == 0);
    assert(TestInstalledFileExists(&site, TEST_NEW_VERSION "/" TEST_EXECUTABLE));
    assert(!TestInstalledFolderExists(&site, TEST_OLDER_RELEASE));
    assert(!TestInstalledFolderExists(&site, CLIENT_INSTALL_DOWNLOADS));
    assert(TestInstalledFileExists(&site, CLIENT_INSTALL_STATE_FILE));
}

static void TestNewerInstalledAppRetiresTheDownloadedBuild(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    ClientInstallState state = TestState(&site);
    ClientInstallSettle(site.plan.root, &state, TEST_LATER_RELEASE);
    ClientInstallRemoveStale(site.plan.root, &state);
    assert(!TestState(&site).version[0]);
    assert(!TestInstalledFolderExists(&site, TEST_NEW_VERSION));
}

static void TestOlderRunningBuildLeavesTheDownloadedBuildAlone(void) {
    TestSite site = TestSiteCreate();
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    ClientInstallState state = TestState(&site);
    state.attempts = 1;
    assert(ClientInstallStateSave(site.plan.root, &state));
    ClientInstallSettle(site.plan.root, &state, HOD_VERSION);
    ClientInstallRemoveStale(site.plan.root, &state);
    assert(TestState(&site).attempts == 1 && strcmp(TestState(&site).version, TEST_NEW_VERSION) == 0);
    assert(TestInstalledFileExists(&site, TEST_NEW_VERSION "/" TEST_EXECUTABLE));
}

static bool TestWaitForInstalledFile(const TestSite *site, const char *relative) {
    for (int step = 0; step < TEST_WAIT_STEPS; step++) {
        if (TestInstalledFileExists(site, relative)) return true;
        usleep(TEST_WAIT_STEP_MICROSECONDS);
    }
    return false;
}

static int TestHandOverInAnotherProcess(void) {
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        ClientInstallHandOver();
        _exit(TEST_HAND_OVER_DECLINED);
    }
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status));
    return WEXITSTATUS(status);
}

static bool TestPlatformHasReleaseBuilds(void) {
    return strlen(CLIENT_INSTALL_PLATFORM) > 0;
}

static void TestUseAsGameData(const TestSite *site) {
    assert(setenv(CONTENT_DIR_ENV, site->data, 1) == 0);
    assert(setenv(CLIENT_INSTALL_RELEASE_URL_ENV, site->plan.release_url, 1) == 0);
}

static void TestLaunchHandsOverUntilTheInstalledBuildFailsTwice(void) {
    TestSite site = TestSiteCreate();
    TestUseAsGameData(&site);
    assert(!ClientInstallHandOver());
    assert(client_install_site.supported && strcmp(client_install_site.root, site.plan.root) == 0);

    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    const char *marker = TEST_NEW_VERSION "/" TEST_STARTED_MARKER;
    assert(TestHandOverInAnotherProcess() == 0);
    assert(TestInstalledFileExists(&site, marker));
    assert(TestState(&site).attempts == 1);

    ClientInstallState confirmed = TestState(&site);
    ClientInstallSettle(site.plan.root, &confirmed, TEST_NEW_VERSION);
    assert(TestState(&site).attempts == 0);

    for (int attempt = 1; attempt <= CLIENT_INSTALL_LAUNCH_ATTEMPTS; attempt++) {
        assert(TestHandOverInAnotherProcess() == 0);
        assert(TestState(&site).attempts == attempt);
    }
    assert(TestHandOverInAnotherProcess() == TEST_HAND_OVER_DECLINED);
    assert(!TestState(&site).version[0] && strcmp(TestState(&site).rejected, TEST_NEW_VERSION) == 0);

    assert(!ClientInstallHandOver());
    assert(strcmp(client_install_site.rejected, TEST_NEW_VERSION) == 0);
    ClientRelease release = {.latest = TEST_NEW_VERSION};
    ClientUpdateAdopt(&release, false);
    assert(!ClientInstallOffered());
}

static void TestMissingBuildFolderIsForgottenInsteadOfLaunched(void) {
    TestSite site = TestSiteCreate();
    TestUseAsGameData(&site);
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    char build[TEST_PATH_CAP];
    TestInstalledPath(&site, TEST_NEW_VERSION, build, sizeof(build));
    assert(PlatformRemoveTree(build));
    assert(!ClientInstallHandOver());
    assert(!TestState(&site).version[0] && !TestState(&site).rejected[0]);
}

static void TestOptOutKeepsTheInstalledAppInCharge(void) {
    TestSite site = TestSiteCreate();
    TestUseAsGameData(&site);
    TestPublishRelease(&site, TEST_NEW_VERSION, TEST_PLATFORM, startable_build, startable_build_count);
    assert(TestInstall(&site) == CLIENT_INSTALL_ERROR_NONE);
    assert(setenv(CLIENT_INSTALL_OPT_OUT_ENV, "1", 1) == 0);
    assert(TestHandOverInAnotherProcess() == TEST_HAND_OVER_DECLINED);
    assert(!ClientInstallHandOver() && !client_install_site.supported);
    ClientRelease release = {.latest = TEST_NEW_VERSION};
    ClientUpdateAdopt(&release, false);
    assert(!ClientInstallOffered());
    assert(unsetenv(CLIENT_INSTALL_OPT_OUT_ENV) == 0);
}

static ClientInstallProgress TestWaitForTheInstallToSettle(void) {
    for (int step = 0; step < TEST_WAIT_STEPS && ClientInstallBusy() && !ClientInstallRestartPending(); step++)
        usleep(TEST_WAIT_STEP_MICROSECONDS);
    return ClientInstallPoll();
}

static void TestUpdateButtonInstallsInTheBackgroundAndRestarts(void) {
    TestSite site = TestSiteCreate();
    TestUseAsGameData(&site);
    TestPublishRelease(&site, TEST_LATER_RELEASE, CLIENT_INSTALL_PLATFORM, startable_build, startable_build_count);
    assert(!ClientInstallHandOver());
    ClientRelease release = {.latest = TEST_LATER_RELEASE};
    ClientUpdateAdopt(&release, false);
    assert(ClientInstallOffered() && !ClientInstallBusy());

    ClientInstallStart();
    ClientInstallProgress progress = TestWaitForTheInstallToSettle();
    assert(progress.phase == CLIENT_INSTALL_READY && progress.error == CLIENT_INSTALL_ERROR_NONE);
    assert(strcmp(progress.version, TEST_LATER_RELEASE) == 0);
    assert(ClientInstallRestartPending());

    ClientInstallRestart();
    assert(TestWaitForInstalledFile(&site, TEST_LATER_RELEASE "/" TEST_STARTED_MARKER));
    assert(TestState(&site).attempts == 0);
    client_install.progress = (ClientInstallProgress){0};
}

static void TestFailedInstallOffersTheWebsiteInstead(void) {
    TestSite site = TestSiteCreate();
    TestUseAsGameData(&site);
    assert(!ClientInstallHandOver());
    ClientRelease release = {.latest = TEST_NEW_VERSION};
    ClientUpdateAdopt(&release, false);
    assert(ClientInstallOffered());

    ClientInstallStart();
    ClientInstallProgress progress = TestWaitForTheInstallToSettle();
    assert(progress.phase == CLIENT_INSTALL_FAILED && progress.error == CLIENT_INSTALL_ERROR_DOWNLOAD);
    assert(!ClientInstallOffered() && !ClientInstallRestartPending());
    ClientInstallAcknowledgeFailure();
    assert(ClientInstallPoll().phase == CLIENT_INSTALL_IDLE && !ClientInstallOffered());
}

int main(void) {
    SetTraceLogLevel(LOG_ERROR);
    const char *temporary = getenv("TMPDIR");
    TestFormat(
        test_base, sizeof(test_base), "%s/%s", temporary && temporary[0] ? temporary : "/tmp", TEST_TEMPORARY_TEMPLATE
    );
    assert(mkdtemp(test_base));

    TestStandardEd25519SignaturesVerify();
    TestSignaturesAndKeysMustBeWholeHexStrings();
    TestOnlyTaggedReleasesAreInstalled();
    TestManifestOffersOnlyASafeBuildForThisPlatform();
    TestManifestMustNameTheRequestedNewerRelease();
    TestSignedReleaseInstallsBesideTheRunningBuild();
    TestInstallingAgainReplacesTheEarlierCopy();
    TestManifestChangedAfterSigningIsRefused();
    TestManifestSignedWithAnotherKeyIsRefused();
    TestReleaseWithoutASignatureIsRefused();
    TestBuildWithoutAReleaseKeyInstallsNothing();
    TestOlderSignedReleaseCannotBeServedAsTheNewOne();
    TestArchiveThatDoesNotMatchTheManifestIsDiscarded();
    TestArchiveWritingOutsideItsFolderIsRefused();
    TestArchiveWithoutItsExecutableIsRefused();
    TestInterruptedArchiveDownloadResumes();
    TestCancelledInstallRecordsNothing();
    TestLaunchPrefersANewerInstalledBuildUntilItFailsTwice();
    TestStateSurvivesARoundTripAndDropsUnsafeExecutables();
    TestBuildThatKeepsFailingIsRejectedAndForgotten();
    TestConfirmedLaunchClearsAttemptsAndRemovesEverythingElse();
    TestNewerInstalledAppRetiresTheDownloadedBuild();
    TestOlderRunningBuildLeavesTheDownloadedBuildAlone();
    if (TestPlatformHasReleaseBuilds()) {
        TestLaunchHandsOverUntilTheInstalledBuildFailsTwice();
        TestMissingBuildFolderIsForgottenInsteadOfLaunched();
        TestOptOutKeepsTheInstalledAppInCharge();
        TestFailedInstallOffersTheWebsiteInstead();
        TestUpdateButtonInstallsInTheBackgroundAndRestarts();
    }

    assert(ChangeDirectory(test_base));
    char cleanup[TEST_PATH_CAP + 16];
    TestFormat(cleanup, sizeof(cleanup), "rm -rf '%s'", test_base);
    assert(system(cleanup) == 0);
    printf("client install: ok\n");
    return 0;
}
