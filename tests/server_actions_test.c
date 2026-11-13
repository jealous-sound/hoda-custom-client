#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>

#define TEST_UNREACHABLE_SERVER "http://127.0.0.1:1"
#define TEST_LOADED_REVISION 1LL
#define TEST_SQUAD_BEFORE_EDIT "before the edit"
#define TEST_SQUAD_AFTER_EDIT "after the edit"
#define TEST_PROFILE_BEFORE_EDIT "{\"squad\":\"" TEST_SQUAD_BEFORE_EDIT "\"}"
#define TEST_PROFILE_AFTER_EDIT "{\"squad\":\"" TEST_SQUAD_AFTER_EDIT "\"}"
#define TEST_REPLY_BEFORE_EDIT "{\"profile\":" TEST_PROFILE_BEFORE_EDIT "}"
#define TEST_REPLY_AFTER_EDIT "{\"profile\":" TEST_PROFILE_AFTER_EDIT "}"

static void TestStartOnlineWithoutANetworkWorker(void) {
    unsetenv("HOD_SHOT");
    unsetenv("HOD_NO_SAVE");
    assert(BackendConnect(TEST_UNREACHABLE_SERVER));
    BackendWorker *w = &backend_worker;
    free(w->upload_text);
    free(w->fetch_text);
    free(w->action_text);
    free(w->action_body);
    w->upload_text = w->fetch_text = w->action_text = w->action_body = NULL;
    w->started = true;
    w->revision = TEST_LOADED_REVISION;
    w->fetch_wanted = w->fetch_ready = w->action_wanted = false;
    w->action_state = BACKEND_ACTION_IDLE;
    ServerActions *s = &server_actions;
    while (s->count > 0) ServerActionsPop(s);
    cJSON_Delete(s->adopt);
    cJSON_Delete(s->reply);
    *s = (ServerActions){.ticket = BACKEND_NO_TICKET};
}

static void TestSendAction(void) {
    ServerActionSubmit(ServerActionCreate("pvp_match"));
    ServerActionsPump();
    assert(backend_worker.action_wanted);
}

static void TestServerAnswersAction(const char *reply) {
    BackendWorker *w = &backend_worker;
    assert(w->action_wanted);
    w->action_wanted = false;
    free(w->action_body);
    w->action_body = NULL;
    w->action_text = BackendCopyText(reply);
    w->action_state = BACKEND_ACTION_APPLIED;
    w->revision++;
    ServerActionsPump();
}

static void TestSaveLocally(void) {
    assert(ServerActionsStoreProfile(TEST_PROFILE_AFTER_EDIT));
    assert(BackendUploadPending());
}

static void TestServerStoresUpload(void) {
    BackendWorker *w = &backend_worker;
    assert(w->upload_text);
    free(w->upload_text);
    w->upload_text = NULL;
    w->revision++;
}

static void TestServerAnswersFetch(const char *profile, long long revision) {
    BackendWorker *w = &backend_worker;
    assert(w->fetch_wanted);
    w->fetch_wanted = false;
    w->fetch_text = BackendCopyText(profile);
    w->fetch_revision = revision;
    w->fetch_result = BACKEND_PROFILE_FOUND;
    w->fetch_ready = true;
}

static void TestNothingIsAdoptedYet(void) {
    cJSON *adopted = ServerActionsTakeAdoption();
    assert(!adopted);
}

static void TestAdopts(const char *squad) {
    cJSON *adopted = ServerActionsTakeAdoption();
    assert(adopted);
    const cJSON *profile = cJSON_GetObjectItemCaseSensitive(adopted, "profile");
    assert(strcmp(JsonStr(profile, "squad"), squad) == 0);
    cJSON_Delete(adopted);
    TestNothingIsAdoptedYet();
    assert(!backend_worker.fetch_wanted);
}

static void TestFetchesAFreshProfileInstead(void) {
    TestNothingIsAdoptedYet();
    assert(backend_worker.fetch_wanted);
    long long revision = backend_worker.revision;
    TestServerAnswersFetch(TEST_PROFILE_AFTER_EDIT, revision);
    TestAdopts(TEST_SQUAD_AFTER_EDIT);
    assert(backend_worker.revision == revision);
}

static void TestAReplyIsAdoptedWhenNothingWasSavedSince(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSendAction();
    TestServerAnswersAction(TEST_REPLY_BEFORE_EDIT);
    TestAdopts(TEST_SQUAD_BEFORE_EDIT);
}

static void TestASaveMadeBeforeTheActionIsSentKeepsItsReply(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSaveLocally();
    TestSendAction();
    TestServerStoresUpload();
    TestServerAnswersAction(TEST_REPLY_AFTER_EDIT);
    TestAdopts(TEST_SQUAD_AFTER_EDIT);
}

static void TestAReplyToAnActionSentBeforeALocalSaveIsNotAdopted(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSendAction();
    TestSaveLocally();
    TestServerAnswersAction(TEST_REPLY_BEFORE_EDIT);
    TestServerStoresUpload();
    TestFetchesAFreshProfileInstead();
}

static void TestAReplyHeldBackIsDroppedByALaterLocalSave(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSendAction();
    TestServerAnswersAction(TEST_REPLY_BEFORE_EDIT);
    TestSaveLocally();
    TestServerStoresUpload();
    TestFetchesAFreshProfileInstead();
}

static void TestNothingIsAdoptedWhileTheSaveIsStillUploading(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSendAction();
    TestSaveLocally();
    TestServerAnswersAction(TEST_REPLY_BEFORE_EDIT);
    TestNothingIsAdoptedYet();
    assert(!backend_worker.fetch_wanted);
    TestServerStoresUpload();
    TestFetchesAFreshProfileInstead();
}

static void TestAFetchBegunBeforeALocalSaveIsFetchedAgain(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSendAction();
    TestServerAnswersAction(TEST_REPLY_BEFORE_EDIT);
    TestSaveLocally();
    TestServerStoresUpload();
    TestNothingIsAdoptedYet();
    assert(backend_worker.fetch_wanted);

    long long stale_revision = backend_worker.revision;
    TestSaveLocally();
    TestServerAnswersFetch(TEST_PROFILE_BEFORE_EDIT, stale_revision);
    TestServerStoresUpload();
    assert(backend_worker.revision == stale_revision + 1);
    TestNothingIsAdoptedYet();
    assert(backend_worker.revision == stale_revision + 1);
    TestFetchesAFreshProfileInstead();
}

static void TestAFetchOvertakenByANewerReplyIsDiscarded(void) {
    TestStartOnlineWithoutANetworkWorker();
    TestSendAction();
    TestServerAnswersAction(TEST_REPLY_BEFORE_EDIT);
    TestSaveLocally();
    TestServerStoresUpload();
    TestNothingIsAdoptedYet();
    assert(backend_worker.fetch_wanted);

    long long fetched_revision = backend_worker.revision;
    TestSendAction();
    TestServerAnswersFetch(TEST_PROFILE_BEFORE_EDIT, fetched_revision);
    TestServerAnswersAction(TEST_REPLY_AFTER_EDIT);
    TestNothingIsAdoptedYet();
    assert(backend_worker.revision == fetched_revision + 1);
    TestAdopts(TEST_SQUAD_AFTER_EDIT);
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    TestAReplyIsAdoptedWhenNothingWasSavedSince();
    TestASaveMadeBeforeTheActionIsSentKeepsItsReply();
    TestAReplyToAnActionSentBeforeALocalSaveIsNotAdopted();
    TestAReplyHeldBackIsDroppedByALaterLocalSave();
    TestNothingIsAdoptedWhileTheSaveIsStillUploading();
    TestAFetchBegunBeforeALocalSaveIsFetchedAgain();
    TestAFetchOvertakenByANewerReplyIsDiscarded();
    backend_state.enabled = false;
    printf("server actions: ok\n");
    return 0;
}
