#define SERVER_ACTION_ROUTE "actions"
#define SERVER_ACTION_QUEUE_CAPACITY 32
#define SERVER_ACTION_RETRY_SECONDS 5.0

bool ProgressionNeedsServer(void);

typedef enum {
    SERVER_REPLY_NONE,
    SERVER_REPLY_NEXUS_AWARD,
    SERVER_REPLY_TUTORIAL_HEROES,
} ServerReplyTag;

typedef struct {
    cJSON *fields;
    ServerReplyTag tag;
} ServerActionEntry;

typedef struct {
    ServerActionEntry queue[SERVER_ACTION_QUEUE_CAPACITY];
    int count;
    int ticket;
    double retry_at;
    cJSON *adopt;
    bool refetch;
    bool fetching;
    ServerReplyTag reply_tag;
    cJSON *reply;
} ServerActions;

static ServerActions server_actions = {.ticket = BACKEND_NO_TICKET};

bool ServerActionsActive(void) {
    return ProgressionNeedsServer();
}

cJSON *ServerActionCreate(const char *action) {
    cJSON *fields = cJSON_CreateObject();
    if (fields) cJSON_AddStringToObject(fields, "action", action);
    return fields;
}

void ServerActionSubmitTagged(cJSON *fields, ServerReplyTag tag) {
    ServerActions *s = &server_actions;
    if (!fields) return;
    if (!ServerActionsActive()) {
        cJSON_Delete(fields);
        return;
    }
    if (s->count == SERVER_ACTION_QUEUE_CAPACITY) {
        TraceLog(LOG_WARNING, "Server: action queue is full, the server will not see this change");
        cJSON_Delete(fields);
        return;
    }
    s->queue[s->count++] = (ServerActionEntry){.fields = fields, .tag = tag};
}

void ServerActionSubmit(cJSON *fields) {
    ServerActionSubmitTagged(fields, SERVER_REPLY_NONE);
}

static void ServerActionsPop(ServerActions *s) {
    cJSON_Delete(s->queue[0].fields);
    memmove(&s->queue[0], &s->queue[1], (size_t)(s->count - 1) * sizeof(s->queue[0]));
    s->count--;
}

static void ServerActionsKeepProfile(ServerActions *s, cJSON *response) {
    if (!cJSON_IsObject(cJSON_GetObjectItemCaseSensitive(response, "profile"))) {
        s->refetch = true;
        cJSON_Delete(response);
        return;
    }
    cJSON_Delete(s->adopt);
    s->adopt = response;
    s->refetch = false;
}

static void ServerActionsSettle(ServerActions *s, BackendActionState state, cJSON *response) {
    ServerReplyTag tag = s->queue[0].tag;
    if (state == BACKEND_ACTION_UNREACHABLE) {
        s->retry_at = GetTime() + SERVER_ACTION_RETRY_SECONDS;
        cJSON_Delete(response);
        return;
    }
    ServerActionsPop(s);
    if (state == BACKEND_ACTION_REFUSED) {
        const cJSON *error = cJSON_GetObjectItemCaseSensitive(response, "error");
        TraceLog(LOG_WARNING, "Server: action refused: %s", cJSON_IsString(error) ? error->valuestring : "?");
    }
    if (tag != SERVER_REPLY_NONE) {
        cJSON_Delete(s->reply);
        s->reply = state == BACKEND_ACTION_APPLIED ? cJSON_Duplicate(response, true) : NULL;
        s->reply_tag = tag;
    }
    if (state == BACKEND_ACTION_CONFLICT) {
        s->refetch = true;
        cJSON_Delete(response);
        return;
    }
    ServerActionsKeepProfile(s, response);
}

void ServerActionsPump(void) {
    ServerActions *s = &server_actions;
    if (s->ticket != BACKEND_NO_TICKET) {
        cJSON *response = NULL;
        BackendActionState state = BackendPollAction(s->ticket, &response);
        if (state == BACKEND_ACTION_PENDING) return;
        s->ticket = BACKEND_NO_TICKET;
        if (state == BACKEND_ACTION_IDLE) {
            cJSON_Delete(response);
            return;
        }
        ServerActionsSettle(s, state, response);
    }
    if (s->count == 0 || GetTime() < s->retry_at) return;
    cJSON *body = cJSON_Duplicate(s->queue[0].fields, true);
    s->ticket = BackendBeginAction(SERVER_ACTION_ROUTE, body);
    cJSON_Delete(body);
}

bool ServerActionsDrained(void) {
    const ServerActions *s = &server_actions;
    return s->count == 0 && s->ticket == BACKEND_NO_TICKET;
}

static cJSON *ServerActionsFinishRefetch(ServerActions *s) {
    if (!BackendProfileFetchSettled()) return NULL;
    s->fetching = false;
    char *text = NULL;
    long long revision = 0;
    if (BackendFetchProfile(&text, &revision) != BACKEND_PROFILE_FOUND) {
        free(text);
        return NULL;
    }
    cJSON *profile = cJSON_Parse(text);
    free(text);
    if (!cJSON_IsObject(profile)) {
        cJSON_Delete(profile);
        return NULL;
    }
    BackendAcceptProfile(revision);
    cJSON *wrapped = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapped, "profile", profile);
    return wrapped;
}

cJSON *ServerActionsTakeAdoption(void) {
    ServerActions *s = &server_actions;
    if (!ServerActionsDrained()) return NULL;
    if (s->fetching) return ServerActionsFinishRefetch(s);
    if (BackendUploadPending()) {
        if (s->adopt) {
            cJSON_Delete(s->adopt);
            s->adopt = NULL;
            s->refetch = true;
        }
        return NULL;
    }
    if (s->refetch) {
        s->refetch = false;
        s->fetching = true;
        BackendBeginProfileFetch();
        return NULL;
    }
    cJSON *adopt = s->adopt;
    s->adopt = NULL;
    return adopt;
}

bool ServerActionsReplyWaiting(ServerReplyTag tag) {
    const ServerActions *s = &server_actions;
    for (int i = 0; i < s->count; i++)
        if (s->queue[i].tag == tag) return true;
    return false;
}

bool ServerActionsTakeReply(ServerReplyTag tag, cJSON **out_reply) {
    ServerActions *s = &server_actions;
    *out_reply = NULL;
    if (s->reply_tag != tag || !ServerActionsDrained() || s->adopt || s->refetch || s->fetching) return false;
    *out_reply = s->reply;
    s->reply = NULL;
    s->reply_tag = SERVER_REPLY_NONE;
    return true;
}
