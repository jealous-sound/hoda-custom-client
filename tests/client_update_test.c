#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>
#include <unistd.h>

#define TEST_PATH_CAP 1024
#define TEST_TEMPORARY_TEMPLATE "hod-update-XXXXXX"
#define TEST_FAR_FUTURE_VERSION "v999.0.0"

static ClientVersion TestVersion(const char *text) {
    ClientVersion version;
    if (!ClientVersionParse(text, &version)) {
        fprintf(stderr, "could not parse %s\n", text);
        assert(false);
    }
    return version;
}

static void TestParsesTagsAndDescribeOutput(void) {
    const char *accepted[] = {
        "v0.4.0",
        "0.4.0",
        "v1.12.3",
        "v0.5.0-rc.1",
        "v0.4.0-3-gabc1234",
        "v0.5.0-rc.1-2-g0f3a9c1",
        "v0.4.0-dirty",
        "v0.4.0-3-gabc1234-dirty",
        "v0.4.0-g12",
    };
    for (size_t i = 0; i < sizeof(accepted) / sizeof(accepted[0]); i++) TestVersion(accepted[i]);
    ClientVersion describe = TestVersion("v0.5.0-rc.1-2-g0f3a9c1");
    assert(describe.core[1] == 5 && strcmp(describe.prerelease, "rc.1") == 0 && describe.ahead == 2);
    assert(strcmp(TestVersion("v0.4.0-g12").prerelease, "g12") == 0);
}

static void TestRejectsBuildsWithoutAVersionTag(void) {
    const char *rejected[] = {
        "",
        "dev",
        "abc1234",
        "abc1234-dirty",
        "v0.4",
        "v0.4.0.1",
        "version",
        "v0.4.0-",
        "v0.4.0-rc..1",
        "v0.4.0-rc_1",
    };
    ClientVersion version;
    for (size_t i = 0; i < sizeof(rejected) / sizeof(rejected[0]); i++)
        assert(!ClientVersionParse(rejected[i], &version));
    assert(!ClientVersionParse(NULL, &version));
}

static void TestOrdersReleasesCandidatesAndLaterCommits(void) {
    const char *ordered[] = {
        "v0.3.9",
        "v0.4.0-rc.1",
        "v0.4.0-rc.1-4-gabc1234",
        "v0.4.0-rc.2",
        "v0.4.0-rc.10",
        "v0.4.0",
        "v0.4.0-1-gabc1234",
        "v0.4.0-12-gabc1234",
        "v0.4.1",
        "v0.10.0",
        "v1.0.0",
    };
    for (size_t i = 0; i + 1 < sizeof(ordered) / sizeof(ordered[0]); i++) {
        ClientVersion lower = TestVersion(ordered[i]);
        ClientVersion higher = TestVersion(ordered[i + 1]);
        assert(ClientVersionCompare(&lower, &higher) < 0);
        assert(ClientVersionCompare(&higher, &lower) > 0);
        assert(ClientVersionCompare(&lower, &lower) == 0);
    }
    ClientVersion dirty = TestVersion("v0.4.0-dirty");
    ClientVersion clean = TestVersion("v0.4.0");
    assert(ClientVersionCompare(&dirty, &clean) == 0);
}

static void TestAcceptsOnlyPlainHttpsDownloadPages(void) {
    assert(ClientUpdateSafeUrl("https://hodareborn.com/#download"));
    assert(ClientUpdateSafeUrl("https://github.com/jealous-sound/hoda-custom-client/releases/latest"));
    const char *unsafe[] = {
        "http://hodareborn.com/#download",
        "https://",
        "https://hodareborn.com/'; rm -rf ~; '",
        "https://hodareborn.com/\" & calc & \"",
        "https://hodareborn.com/%APPDATA%",
        "https://hodareborn.com/$(id)",
        "https://hodareborn.com/`id`",
        "https://hodareborn.com/a b",
        "file:///etc/passwd",
        "",
    };
    for (size_t i = 0; i < sizeof(unsafe) / sizeof(unsafe[0]); i++) assert(!ClientUpdateSafeUrl(unsafe[i]));
    assert(!ClientUpdateSafeUrl(NULL));
}

static void TestReadsTheServerRelease(void) {
    ClientRelease release;
    assert(ClientReleaseParse(
        "{\"latest\":\"v0.5.1\",\"minimum\":\"v0.5.0\",\"download_url\":\"https://example.org/get\"}", &release
    ));
    assert(strcmp(release.latest, "v0.5.1") == 0);
    assert(strcmp(release.minimum, "v0.5.0") == 0);
    assert(strcmp(release.download_url, "https://example.org/get") == 0);

    assert(ClientReleaseParse("{\"latest\":\"\",\"minimum\":\"v0.6.0\",\"download_url\":\"javascript:x\"}", &release));
    assert(strcmp(release.latest, "v0.6.0") == 0);
    assert(strcmp(release.download_url, CLIENT_UPDATE_DEFAULT_DOWNLOAD_URL) == 0);

    assert(ClientReleaseParse("{\"latest\":\"nonsense\",\"minimum\":42}", &release));
    assert(!release.latest[0] && !release.minimum[0]);

    assert(!ClientReleaseParse("not json", &release));
    assert(!ClientReleaseParse(NULL, &release));
    assert(strcmp(release.download_url, CLIENT_UPDATE_DEFAULT_DOWNLOAD_URL) == 0);
}

static void TestStatusComparesTheRunningBuild(void) {
    ClientRelease release = {.latest = "v0.5.1", .minimum = "v0.5.0"};
    assert(ClientUpdateStatusFor("v0.4.9", &release) == CLIENT_UPDATE_REQUIRED);
    assert(ClientUpdateStatusFor("v0.5.0-rc.3", &release) == CLIENT_UPDATE_REQUIRED);
    assert(ClientUpdateStatusFor("v0.5.0", &release) == CLIENT_UPDATE_AVAILABLE);
    assert(ClientUpdateStatusFor("v0.5.0-7-gabc1234", &release) == CLIENT_UPDATE_AVAILABLE);
    assert(ClientUpdateStatusFor("v0.5.1", &release) == CLIENT_UPDATE_CURRENT);
    assert(ClientUpdateStatusFor("v0.5.1-2-gabc1234-dirty", &release) == CLIENT_UPDATE_CURRENT);
    assert(ClientUpdateStatusFor("v0.6.0-rc.1", &release) == CLIENT_UPDATE_CURRENT);
    assert(ClientUpdateStatusFor("dev", &release) == CLIENT_UPDATE_CURRENT);
    ClientRelease unknown = {0};
    assert(ClientUpdateStatusFor("v0.1.0", &unknown) == CLIENT_UPDATE_CURRENT);
}

static void TestLaterHidesOnlyTheSkippedRelease(void) {
    ClientVersion running;
    if (!ClientVersionParse(HOD_VERSION, &running)) return;
    char directory[] = TEST_TEMPORARY_TEMPLATE;
    assert(mkdtemp(directory));
    char previous[TEST_PATH_CAP];
    assert(getcwd(previous, sizeof(previous)));
    assert(chdir(directory) == 0);

    ClientRelease release = {.latest = TEST_FAR_FUTURE_VERSION};
    StrCopy(release.download_url, sizeof(release.download_url), CLIENT_UPDATE_DEFAULT_DOWNLOAD_URL);
    ClientUpdateAdopt(&release, false);
    assert(ClientUpdateStatusNow() == CLIENT_UPDATE_AVAILABLE);
    assert(ClientUpdateOffered());
    ClientUpdateSkipOffered();
    assert(!ClientUpdateOffered());
    settings = (Settings){0};
    assert(strcmp(SettingsSkippedUpdate(), TEST_FAR_FUTURE_VERSION) == 0);
    assert(!ClientUpdateOffered());

    StrCopy(release.latest, sizeof(release.latest), "v1000.0.0");
    ClientUpdateAdopt(&release, false);
    assert(ClientUpdateOffered());

    unlink(SETTINGS_PATH);
    assert(chdir(previous) == 0);
    rmdir(directory);
}

static void TestUpgradeRequiredBlocksOnlineAndKeepsTheServerRelease(void) {
    ClientUpdateNoteRefusal(409, "{\"error\":\"conflict\"}");
    assert(!ClientUpdateOnlineRefused());
    ClientUpdateNoteRefusal(
        CLIENT_UPDATE_HTTP_UPGRADE_REQUIRED,
        "{\"error\":\"too old\",\"latest\":\"v0.9.2\",\"minimum\":\"v0.9.0\","
        "\"download_url\":\"https://hodareborn.com/#download\"}"
    );
    assert(ClientUpdateOnlineRefused());
    assert(ClientUpdateStatusNow() == CLIENT_UPDATE_REQUIRED);
    ClientRelease release = ClientUpdateCurrentRelease();
    assert(strcmp(release.latest, "v0.9.2") == 0 && strcmp(release.minimum, "v0.9.0") == 0);
    ClientRelease later = {.latest = "v0.9.3"};
    ClientUpdateAdopt(&later, false);
    assert(ClientUpdateOnlineRefused());
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    TestParsesTagsAndDescribeOutput();
    TestRejectsBuildsWithoutAVersionTag();
    TestOrdersReleasesCandidatesAndLaterCommits();
    TestAcceptsOnlyPlainHttpsDownloadPages();
    TestReadsTheServerRelease();
    TestStatusComparesTheRunningBuild();
    TestLaterHidesOnlyTheSkippedRelease();
    TestUpgradeRequiredBlocksOnlineAndKeepsTheServerRelease();
    printf("client update: ok\n");
    return 0;
}
