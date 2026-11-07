#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>
#include <stdarg.h>
#include <unistd.h>

#define TEST_PATH_CAP 1024
#define TEST_MAX_ENTRIES 8
#define TEST_ZIP_VERSION 20
#define TEST_TEMPORARY_TEMPLATE "hod-content-XXXXXX"
#define TEST_EMPTY_SHA256 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
#define TEST_ABC_SHA256 "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
#define TEST_TWO_BLOCK_MESSAGE "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
#define TEST_TWO_BLOCK_SHA256 "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"
#define TEST_MILLION_A_BYTES 1000000
#define TEST_MILLION_A_SHA256 "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"
#define TEST_ODD_CHUNK_BYTES 7
#define TEST_WRONG_SHA256 "0000000000000000000000000000000000000000000000000000000000000000"

typedef struct {
    const char *name;
    const char *text;
    bool deflate;
} TestEntry;

typedef struct {
    char root[TEST_PATH_CAP];
    char server[TEST_PATH_CAP];
    char install[TEST_PATH_CAP];
    char url[TEST_PATH_CAP];
} TestSite;

static char test_base[TEST_PATH_CAP];
static int test_site_count;

static void TestFormat(char *out, size_t capacity, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(out, capacity, format, arguments);
    va_end(arguments);
    assert(length >= 0 && (size_t)length < capacity);
}

static void TestPutU16(FILE *file, unsigned int value) {
    fputc((int)(value & 0xffu), file);
    fputc((int)((value >> 8) & 0xffu), file);
}

static void TestPutU32(FILE *file, unsigned int value) {
    TestPutU16(file, value & 0xffffu);
    TestPutU16(file, value >> 16);
}

static void TestWriteZip(const char *path, const TestEntry *entries, int count) {
    FILE *file = fopen(path, "wb");
    assert(file);
    unsigned int offsets[TEST_MAX_ENTRIES], stored_sizes[TEST_MAX_ENTRIES], crcs[TEST_MAX_ENTRIES];
    unsigned int methods[TEST_MAX_ENTRIES];
    for (int i = 0; i < count; i++) {
        unsigned char *raw = (unsigned char *)entries[i].text;
        int raw_size = (int)strlen(entries[i].text);
        int stored_size = raw_size;
        unsigned char *stored = entries[i].deflate ? CompressData(raw, raw_size, &stored_size) : raw;
        methods[i] = entries[i].deflate ? ZIP_METHOD_DEFLATE : ZIP_METHOD_STORED;
        offsets[i] = (unsigned int)ftell(file);
        stored_sizes[i] = (unsigned int)stored_size;
        crcs[i] = ComputeCRC32(raw, raw_size);
        TestPutU32(file, ZIP_LOCAL_HEADER_SIGNATURE);
        TestPutU16(file, TEST_ZIP_VERSION);
        TestPutU16(file, 0);
        TestPutU16(file, methods[i]);
        TestPutU32(file, 0);
        TestPutU32(file, crcs[i]);
        TestPutU32(file, stored_sizes[i]);
        TestPutU32(file, (unsigned int)raw_size);
        TestPutU16(file, (unsigned int)strlen(entries[i].name));
        TestPutU16(file, 0);
        fwrite(entries[i].name, 1, strlen(entries[i].name), file);
        fwrite(stored, 1, (size_t)stored_size, file);
        if (entries[i].deflate) MemFree(stored);
    }
    unsigned int directory_start = (unsigned int)ftell(file);
    for (int i = 0; i < count; i++) {
        TestPutU32(file, ZIP_DIRECTORY_ENTRY_SIGNATURE);
        TestPutU16(file, TEST_ZIP_VERSION);
        TestPutU16(file, TEST_ZIP_VERSION);
        TestPutU16(file, 0);
        TestPutU16(file, methods[i]);
        TestPutU32(file, 0);
        TestPutU32(file, crcs[i]);
        TestPutU32(file, stored_sizes[i]);
        TestPutU32(file, (unsigned int)strlen(entries[i].text));
        TestPutU16(file, (unsigned int)strlen(entries[i].name));
        TestPutU16(file, 0);
        TestPutU16(file, 0);
        TestPutU16(file, 0);
        TestPutU16(file, 0);
        TestPutU32(file, 0);
        TestPutU32(file, offsets[i]);
        fwrite(entries[i].name, 1, strlen(entries[i].name), file);
    }
    unsigned int directory_size = (unsigned int)ftell(file) - directory_start;
    TestPutU32(file, ZIP_END_OF_DIRECTORY_SIGNATURE);
    TestPutU16(file, 0);
    TestPutU16(file, 0);
    TestPutU16(file, (unsigned int)count);
    TestPutU16(file, (unsigned int)count);
    TestPutU32(file, directory_size);
    TestPutU32(file, directory_start);
    TestPutU16(file, 0);
    fclose(file);
}

static TestSite TestSiteCreate(void) {
    TestSite site;
    TestFormat(site.root, sizeof(site.root), "%s/site%d", test_base, test_site_count++);
    TestFormat(site.server, sizeof(site.server), "%s/server", site.root);
    TestFormat(site.install, sizeof(site.install), "%s/install", site.root);
    TestFormat(site.url, sizeof(site.url), "file://%s", site.server);
    char packs[TEST_PATH_CAP];
    TestFormat(packs, sizeof(packs), "%s/packs", site.server);
    assert(ContentMakeDirectories(packs));
    assert(ContentMakeDirectories(site.install));
    assert(ChangeDirectory(site.install));
    return site;
}

static ContentPack TestPublishPack(const TestSite *site, const char *name, const TestEntry *entries, int count) {
    ContentPack pack = {0};
    StrCopy(pack.name, sizeof(pack.name), name);
    TestFormat(pack.file, sizeof(pack.file), "packs/%s.zip", name);
    char path[TEST_PATH_CAP];
    TestFormat(path, sizeof(path), "%s/%s", site->server, pack.file);
    TestWriteZip(path, entries, count);
    assert(ContentFileDigest(path, &pack.size, pack.sha256));
    return pack;
}

static void
TestPublishManifest(const TestSite *site, int format, const char *version, const ContentPack *packs, int count) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "format", format);
    cJSON_AddStringToObject(root, "version", version);
    cJSON *list = cJSON_AddArrayToObject(root, "packs");
    for (int i = 0; i < count; i++) {
        cJSON *pack = cJSON_CreateObject();
        cJSON_AddStringToObject(pack, "name", packs[i].name);
        cJSON_AddStringToObject(pack, "file", packs[i].file);
        cJSON_AddStringToObject(pack, "sha256", packs[i].sha256);
        cJSON_AddNumberToObject(pack, "size", (double)packs[i].size);
        cJSON_AddItemToArray(list, pack);
    }
    char *text = cJSON_Print(root);
    char path[TEST_PATH_CAP];
    TestFormat(path, sizeof(path), "%s/%s", site->server, CONTENT_MANIFEST_FILE);
    assert(SaveFileText(path, text));
    cJSON_free(text);
    cJSON_Delete(root);
}

static void TestRemoveServerPacks(const TestSite *site, const ContentPack *packs, int count) {
    for (int i = 0; i < count; i++) {
        char path[TEST_PATH_CAP];
        TestFormat(path, sizeof(path), "%s/%s", site->server, packs[i].file);
        assert(remove(path) == 0);
    }
}

static ContentProgress TestRunUpdate(const char *url) {
    ContentUpdate update;
    ContentUpdateBegin(&update, url);
    ContentUpdateExecute(&update);
    ContentProgress progress = ContentUpdatePoll(&update);
    ContentUpdateEnd(&update);
    return progress;
}

static bool TestFileHolds(const char *path, const char *text) {
    if (!FileExists(path)) return false;
    char *loaded = LoadFileText(path);
    bool same = loaded && strcmp(loaded, text) == 0;
    UnloadFileText(loaded);
    return same;
}

static int TestInstalledPackCount(void) {
    cJSON *state = ContentStateLoad();
    int count = cJSON_GetArraySize(ContentStatePacks(state));
    cJSON_Delete(state);
    return count;
}

static const TestEntry rules_v1[] = {
    {"data/a.json", "{\"version\": 1, \"padding\": \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}", true},
    {"data/b.json", "{\"b\": true}", false},
};
static const TestEntry rules_v2[] = {
    {"data/a.json", "{\"version\": 2}", true},
    {"data/c.json", "{\"c\": true}", true},
};
static const TestEntry core_entries[] = {
    {HOD_CACHE "characters.json", "[]", false},
    {HOD_CACHE "extra/gui/button.txt", "button", true},
};

static void TestHashText(const char *text, char *hex) {
    Sha256 hash;
    Sha256Begin(&hash);
    Sha256Update(&hash, text, strlen(text));
    Sha256FinishHex(&hash, hex);
}

static void TestSha256MatchesTheStandardVectors(void) {
    char hex[SHA256_HEX_CAP];
    TestHashText("", hex);
    assert(strcmp(hex, TEST_EMPTY_SHA256) == 0);
    TestHashText("abc", hex);
    assert(strcmp(hex, TEST_ABC_SHA256) == 0);
    TestHashText(TEST_TWO_BLOCK_MESSAGE, hex);
    assert(strcmp(hex, TEST_TWO_BLOCK_SHA256) == 0);

    Sha256 hash;
    Sha256Begin(&hash);
    char chunk[TEST_ODD_CHUNK_BYTES];
    memset(chunk, 'a', sizeof(chunk));
    for (int fed = 0; fed < TEST_MILLION_A_BYTES; fed += TEST_ODD_CHUNK_BYTES) {
        int left = TEST_MILLION_A_BYTES - fed;
        Sha256Update(&hash, chunk, left < TEST_ODD_CHUNK_BYTES ? (size_t)left : sizeof(chunk));
    }
    Sha256FinishHex(&hash, hex);
    assert(strcmp(hex, TEST_MILLION_A_SHA256) == 0);
}

static void TestFileDigestReadsLargeFilesInChunks(void) {
    TestSiteCreate();
    FILE *file = fopen("million.bin", "wb");
    assert(file);
    for (int i = 0; i < TEST_MILLION_A_BYTES; i++) fputc('a', file);
    fclose(file);
    long long size = 0;
    char hex[SHA256_HEX_CAP];
    assert(ContentFileDigest("million.bin", &size, hex));
    assert(size == TEST_MILLION_A_BYTES && strcmp(hex, TEST_MILLION_A_SHA256) == 0);
}

static void TestEntryPathsStayInsideTheGameData(void) {
    assert(ContentEntryPathSafe("data/gear.json"));
    assert(ContentEntryPathSafe(HOD_CACHE "char_pack/Hero/skin.png"));
    assert(!ContentEntryPathSafe("account.dat"));
    assert(!ContentEntryPathSafe("data/../account.dat"));
    assert(!ContentEntryPathSafe("data/./gear.json"));
    assert(!ContentEntryPathSafe("data//gear.json"));
    assert(!ContentEntryPathSafe("data/"));
    assert(!ContentEntryPathSafe("data\\gear.json"));
    assert(!ContentEntryPathSafe("/data/gear.json"));
    assert(!ContentEntryPathSafe("C:data/gear.json"));
}

static void TestManifestRejectsUnsafePackFields(void) {
    char text[TEST_PATH_CAP];
    TestFormat(
        text,
        sizeof(text),
        "{\"format\":%d,\"version\":\"v\",\"packs\":[{\"name\":\"../x\",\"file\":\"packs/x.zip\","
        "\"sha256\":\"" TEST_ABC_SHA256 "\",\"size\":3}]}",
        CONTENT_MANIFEST_FORMAT
    );
    cJSON *root = cJSON_Parse(text);
    ContentManifest manifest;
    assert(!ContentManifestParse(root, &manifest));
    cJSON_Delete(root);
}

static void TestFreshInstallUnpacksEveryPack(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {
        TestPublishPack(&site, "rules", rules_v1, 2), TestPublishPack(&site, "core", core_entries, 2)
    };
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 2);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_READY);
    assert(progress.packs_total == 2 && progress.packs_done == 2);
    assert(progress.received_bytes == packs[0].size + packs[1].size);
    for (int i = 0; i < 2; i++) {
        assert(TestFileHolds(rules_v1[i].name, rules_v1[i].text));
        assert(TestFileHolds(core_entries[i].name, core_entries[i].text));
    }
    assert(TestInstalledPackCount() == 2);
    char download[TEST_PATH_CAP];
    ContentPackDownloadPath(&packs[0], download, sizeof(download));
    assert(!FileExists(download));
    assert(!FileExists(CONTENT_MANIFEST_DOWNLOAD));
}

static void TestCurrentInstallDownloadsNothing(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    TestRemoveServerPacks(&site, packs, 1);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_READY);
    assert(progress.packs_total == 0);
}

static void TestUpdateReplacesChangedPacksAndRetiresOldFiles(void) {
    TestSite site = TestSiteCreate();
    ContentPack first[] = {
        TestPublishPack(&site, "rules", rules_v1, 2), TestPublishPack(&site, "core", core_entries, 2)
    };
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", first, 2);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);

    ContentPack second[] = {TestPublishPack(&site, "rules", rules_v2, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v2", second, 1);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_READY);
    assert(progress.packs_total == 1);
    assert(TestFileHolds("data/a.json", rules_v2[0].text));
    assert(TestFileHolds("data/c.json", rules_v2[1].text));
    assert(!FileExists("data/b.json"));
    assert(!FileExists(core_entries[0].name));
    assert(!FileExists(core_entries[1].name));
    assert(TestInstalledPackCount() == 1);
}

static void TestMissingFileIsDownloadedAgain(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    assert(remove("data/b.json") == 0);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_READY && progress.packs_total == 1);
    assert(TestFileHolds("data/b.json", rules_v1[1].text));
}

static void TestUnreachableServerFallsBackToInstalledData(void) {
    TestSite site = TestSiteCreate();
    char missing[TEST_PATH_CAP];
    TestFormat(missing, sizeof(missing), "%s/nowhere", site.url);
    ContentProgress offline = TestRunUpdate(missing);
    assert(offline.phase == CONTENT_PHASE_FAILED && offline.error == CONTENT_ERROR_OFFLINE);

    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    assert(TestRunUpdate(missing).phase == CONTENT_PHASE_READY);
}

static void TestDamagedPackIsDiscarded(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    StrCopy(packs[0].sha256, sizeof(packs[0].sha256), TEST_WRONG_SHA256);
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_FAILED && progress.error == CONTENT_ERROR_CORRUPT);
    assert(!FileExists("data/a.json"));
    char download[TEST_PATH_CAP];
    ContentPackDownloadPath(&packs[0], download, sizeof(download));
    assert(!FileExists(download));
    assert(TestInstalledPackCount() == 0);
}

static void TestPackWritingOutsideTheGameDataIsRejected(void) {
    TestSite site = TestSiteCreate();
    const TestEntry hostile[] = {{"data/a.json", "{}", false}, {BACKEND_DEFAULT_ACCOUNT_PATH, "stolen", false}};
    ContentPack packs[] = {TestPublishPack(&site, "rules", hostile, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_FAILED && progress.error == CONTENT_ERROR_CORRUPT);
    assert(!FileExists(BACKEND_DEFAULT_ACCOUNT_PATH));
    assert(!FileExists("data/a.json"));
}

static void TestNewerManifestFormatAsksForANewClient(void) {
    TestSite site = TestSiteCreate();
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT + 1, "v9", NULL, 0);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_FAILED && progress.error == CONTENT_ERROR_CLIENT_TOO_OLD);
    ContentAlert alert = ContentAlertFor(CONTENT_ERROR_CLIENT_TOO_OLD);
    assert(alert.quits && strcmp(alert.title_key, "HODA_CONTENT_CLIENT_TOO_OLD_TITLE") == 0);
}

static void TestOlderManifestFormatIsTheServersFault(void) {
    TestSite site = TestSiteCreate();
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT - 1, "v0", NULL, 0);
    ContentProgress progress = TestRunUpdate(site.url);
    assert(progress.phase == CONTENT_PHASE_FAILED && progress.error == CONTENT_ERROR_SERVER);
    assert(!ContentAlertFor(CONTENT_ERROR_SERVER).quits);
}

static void TestInstalledDataOfAnotherFormatIsNotPlayedOffline(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    cJSON *state = ContentStateLoad();
    cJSON_ReplaceItemInObjectCaseSensitive(state, "format", cJSON_CreateNumber(CONTENT_MANIFEST_FORMAT - 1));
    assert(ContentStateSave(state));
    cJSON_Delete(state);
    char missing[TEST_PATH_CAP];
    TestFormat(missing, sizeof(missing), "%s/nowhere", site.url);
    ContentProgress offline = TestRunUpdate(missing);
    assert(offline.phase == CONTENT_PHASE_FAILED && offline.error == CONTENT_ERROR_OFFLINE);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    assert(TestRunUpdate(missing).phase == CONTENT_PHASE_READY);
}

static void TestBuildWithoutContentUrlReportsIt(void) {
    TestSiteCreate();
    ContentProgress progress = TestRunUpdate("");
    assert(progress.phase == CONTENT_PHASE_FAILED && progress.error == CONTENT_ERROR_NOT_CONFIGURED);
}

static void TestWritePartialDownload(const TestSite *site, const ContentPack *pack, long long bytes, bool garbage) {
    char source[TEST_PATH_CAP], target[TEST_PATH_CAP];
    TestFormat(source, sizeof(source), "%s/%s", site->server, pack->file);
    ContentPackDownloadPath(pack, target, sizeof(target));
    assert(ContentMakeDirectories(CONTENT_DOWNLOAD_DIR));
    int size = 0;
    unsigned char *data = LoadFileData(source, &size);
    assert(data && bytes <= size * 2);
    FILE *file = fopen(target, "wb");
    assert(file);
    for (long long i = 0; i < bytes; i++) fputc(garbage ? 'x' : data[i % size], file);
    fclose(file);
    UnloadFileData(data);
}

static void TestInterruptedDownloadResumes(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    TestWritePartialDownload(&site, &packs[0], packs[0].size / 2, false);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    assert(TestFileHolds("data/a.json", rules_v1[0].text));
}

static void TestOversizedPartialDownloadStartsOver(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    TestWritePartialDownload(&site, &packs[0], packs[0].size + packs[0].size / 2, true);
    assert(TestRunUpdate(site.url).phase == CONTENT_PHASE_READY);
    assert(TestFileHolds("data/b.json", rules_v1[1].text));
}

static void TestBackgroundUpdateFinishes(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    ContentUpdate update;
    ContentUpdateStart(&update, site.url);
    ContentProgress progress;
    do {
        progress = ContentUpdatePoll(&update);
    } while (progress.phase != CONTENT_PHASE_READY && progress.phase != CONTENT_PHASE_FAILED);
    ContentUpdateStop(&update, false);
    assert(progress.phase == CONTENT_PHASE_READY);
}

static void TestOriginalLoaderShowsFlooredPercentAndHidesTheBarAtTheEnds(void) {
    ContentProgress progress = {.total_bytes = 1000, .received_bytes = 999};
    assert(ContentPercent(&progress) == 99);
    progress.received_bytes = 5;
    assert(ContentPercent(&progress) == 0);
    assert(!ContentBarVisible(0) && ContentBarVisible(1) && ContentBarVisible(99) && !ContentBarVisible(100));
    progress.total_bytes = 0;
    assert(ContentPercent(&progress) == 0);
}

static void TestMegabytesUseTheLocalizedDecimalSeparator(void) {
    char text[CONTENT_LABEL_CAP];
    ContentFormatMegabytes(1572864, ",", text, sizeof(text));
    assert(strcmp(text, "1,5") == 0);
    ContentFormatMegabytes(0, ".", text, sizeof(text));
    assert(strcmp(text, "0.0") == 0);
}

static void TestLoaderMessageFollowsTheOriginalModes(void) {
    ContentProgress checking = {.phase = CONTENT_PHASE_CHECKING};
    assert(strcmp(ContentLoadingMessageKey(&checking), "LOADING_MESSAGE_CHECKING_UPDATES") == 0);
    ContentProgress welcome = {.phase = CONTENT_PHASE_DOWNLOADING, .packs_total = 3, .first_install = true};
    assert(strcmp(ContentLoadingMessageKey(&welcome), "LOADING_MESSAGE_WELCOME") == 0);
    ContentProgress update = {.phase = CONTENT_PHASE_DOWNLOADING, .packs_total = 1};
    assert(strcmp(ContentLoadingMessageKey(&update), "LOADING_MESSAGE_DOWNLOADING_UPDATES") == 0);
}

static void TestErrorsUseTheOriginalPopups(void) {
    assert(strcmp(ContentAlertFor(CONTENT_ERROR_OFFLINE).title_key, "CONNECTION_ERROR_TITLE") == 0);
    assert(strcmp(ContentAlertFor(CONTENT_ERROR_CORRUPT).title_key, "ASSET_DOWNLOAD_ERROR_TITLE_TU5") == 0);
    assert(strcmp(ContentAlertFor(CONTENT_ERROR_DISK).message_key, "ERROR_INSUFFICIENT_DEVICE_STORAGE") == 0);
}

static void TestFirstInstallIsReportedOnlyBeforeAnythingIsInstalled(void) {
    TestSite site = TestSiteCreate();
    ContentPack packs[] = {TestPublishPack(&site, "rules", rules_v1, 2)};
    TestPublishManifest(&site, CONTENT_MANIFEST_FORMAT, "v1", packs, 1);
    assert(TestRunUpdate(site.url).first_install);
    assert(remove("data/a.json") == 0);
    assert(!TestRunUpdate(site.url).first_install);
}

int main(void) {
    SetTraceLogLevel(LOG_ERROR);
    const char *temporary = getenv("TMPDIR");
    TestFormat(
        test_base, sizeof(test_base), "%s/%s", temporary && temporary[0] ? temporary : "/tmp", TEST_TEMPORARY_TEMPLATE
    );
    assert(mkdtemp(test_base));

    TestSha256MatchesTheStandardVectors();
    TestFileDigestReadsLargeFilesInChunks();
    TestEntryPathsStayInsideTheGameData();
    TestManifestRejectsUnsafePackFields();
    TestFreshInstallUnpacksEveryPack();
    TestCurrentInstallDownloadsNothing();
    TestUpdateReplacesChangedPacksAndRetiresOldFiles();
    TestMissingFileIsDownloadedAgain();
    TestUnreachableServerFallsBackToInstalledData();
    TestDamagedPackIsDiscarded();
    TestPackWritingOutsideTheGameDataIsRejected();
    TestNewerManifestFormatAsksForANewClient();
    TestOlderManifestFormatIsTheServersFault();
    TestInstalledDataOfAnotherFormatIsNotPlayedOffline();
    TestBuildWithoutContentUrlReportsIt();
    TestInterruptedDownloadResumes();
    TestOversizedPartialDownloadStartsOver();
    TestBackgroundUpdateFinishes();
    TestOriginalLoaderShowsFlooredPercentAndHidesTheBarAtTheEnds();
    TestMegabytesUseTheLocalizedDecimalSeparator();
    TestLoaderMessageFollowsTheOriginalModes();
    TestErrorsUseTheOriginalPopups();
    TestFirstInstallIsReportedOnlyBeforeAnythingIsInstalled();

    char cleanup[TEST_PATH_CAP + 16];
    TestFormat(cleanup, sizeof(cleanup), "rm -rf '%s'", test_base);
    assert(system(cleanup) == 0);
    printf("content_update_test: all tests passed\n");
    return 0;
}
