#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>
#include <unistd.h>

#define TEST_PATH_CAP 1024
#define TEST_TEMPORARY_TEMPLATE "hod-update-XXXXXX"
#define TEST_FAR_FUTURE_VERSION "v999.0.0"
#define TEST_SERVER_TIME 1800000000LL
#define TEST_RECEIVED_AT 5000LL
#define TEST_UNREACHABLE_SERVER "http://127.0.0.1:1"

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

static void TestARefusedClientBeginsNoPlayerRequests(void) {
    assert(ClientUpdateOnlineRefused());
    assert(BackendConnect(TEST_UNREACHABLE_SERVER));
    cJSON *fields = cJSON_CreateObject();
    assert(BackendBeginQuery("GET", "guild", NULL) == BACKEND_NO_TICKET);
    assert(BackendBeginAction(SERVER_ACTION_ROUTE, fields) == BACKEND_NO_TICKET);
    assert(!backend_worker.started);
    cJSON_Delete(fields);
    backend_state.enabled = false;
}

static void TestReadsTheAnnouncedRestart(void) {
    ClientRelease release;
    assert(
        ClientReleaseParse("{\"latest\":\"v0.5.1\",\"server_time\":1800000000,\"maintenance_at\":1800001800}", &release)
    );
    assert(release.server_time == TEST_SERVER_TIME && release.maintenance_at == TEST_SERVER_TIME + 1800);

    assert(ClientReleaseParse("{\"latest\":\"v0.5.1\",\"server_time\":1800000000}", &release));
    assert(release.server_time == TEST_SERVER_TIME && release.maintenance_at == 0);

    assert(ClientReleaseParse("{\"server_time\":\"soon\",\"maintenance_at\":-5}", &release));
    assert(release.server_time == 0 && release.maintenance_at == 0);
}

static ClientMaintenance TestMaintenance(long long restart_in, long long seconds_since_reply) {
    ClientRelease release = {.server_time = TEST_SERVER_TIME, .maintenance_at = TEST_SERVER_TIME + restart_in};
    return ClientMaintenanceFor(&release, TEST_RECEIVED_AT, TEST_RECEIVED_AT + seconds_since_reply);
}

static void TestRestartIsAnnouncedOnlyInItsLastHalfHour(void) {
    assert(TestMaintenance(CLIENT_MAINTENANCE_NOTICE_SECONDS + 1, 0).phase == CLIENT_MAINTENANCE_NONE);
    ClientMaintenance announced = TestMaintenance(CLIENT_MAINTENANCE_NOTICE_SECONDS, 0);
    assert(announced.phase == CLIENT_MAINTENANCE_ANNOUNCED);
    assert(announced.seconds_left == CLIENT_MAINTENANCE_NOTICE_SECONDS);
    assert(announced.at == TEST_SERVER_TIME + CLIENT_MAINTENANCE_NOTICE_SECONDS);

    ClientRelease quiet = {.server_time = TEST_SERVER_TIME};
    assert(ClientMaintenanceFor(&quiet, TEST_RECEIVED_AT, TEST_RECEIVED_AT).phase == CLIENT_MAINTENANCE_NONE);
    ClientRelease clockless = {.maintenance_at = TEST_SERVER_TIME + 60};
    assert(ClientMaintenanceFor(&clockless, TEST_RECEIVED_AT, TEST_RECEIVED_AT).phase == CLIENT_MAINTENANCE_NONE);
}

static void TestCountdownFollowsTheTimeSinceTheReply(void) {
    long long restart_in = 2 * 60 * 60;
    assert(TestMaintenance(restart_in, 0).phase == CLIENT_MAINTENANCE_NONE);
    long long waited = restart_in - 600;
    ClientMaintenance later = TestMaintenance(restart_in, waited);
    assert(later.phase == CLIENT_MAINTENANCE_ANNOUNCED && later.seconds_left == 600);
    assert(TestMaintenance(restart_in, waited + 1).seconds_left == 599);
}

static void TestBattlesAreHeldFromTheLeadUntilTheOverrunEnds(void) {
    long long lead = CLIENT_MAINTENANCE_BATTLE_LEAD_SECONDS;
    assert(TestMaintenance(lead + 1, 0).phase == CLIENT_MAINTENANCE_ANNOUNCED);
    ClientMaintenance holding = TestMaintenance(lead, 0);
    assert(holding.phase == CLIENT_MAINTENANCE_HOLDING_BATTLES && holding.seconds_left == lead);

    ClientMaintenance overdue = TestMaintenance(lead, lead + 30);
    assert(overdue.phase == CLIENT_MAINTENANCE_HOLDING_BATTLES && overdue.seconds_left == 0);
    long long overrun = CLIENT_MAINTENANCE_OVERRUN_SECONDS;
    assert(TestMaintenance(lead, lead + overrun - 1).phase == CLIENT_MAINTENANCE_HOLDING_BATTLES);
    assert(TestMaintenance(lead, lead + overrun).phase == CLIENT_MAINTENANCE_NONE);
}

static void TestTheLatestReplyDecidesTheAnnouncement(void) {
    ClientRelease release = {.server_time = TEST_SERVER_TIME, .maintenance_at = TEST_SERVER_TIME + 600};
    ClientUpdateAdopt(&release, false);
    ClientMaintenance announced = ClientMaintenanceNow();
    assert(announced.phase == CLIENT_MAINTENANCE_ANNOUNCED && announced.at == release.maintenance_at);

    ClientRelease cancelled = {.server_time = TEST_SERVER_TIME};
    ClientUpdateAdopt(&cancelled, false);
    assert(ClientMaintenanceNow().phase == CLIENT_MAINTENANCE_NONE);
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
    TestReadsTheAnnouncedRestart();
    TestRestartIsAnnouncedOnlyInItsLastHalfHour();
    TestCountdownFollowsTheTimeSinceTheReply();
    TestBattlesAreHeldFromTheLeadUntilTheOverrunEnds();
    TestTheLatestReplyDecidesTheAnnouncement();
    TestUpgradeRequiredBlocksOnlineAndKeepsTheServerRelease();
    TestARefusedClientBeginsNoPlayerRequests();
    printf("client update: ok\n");
    return 0;
}
