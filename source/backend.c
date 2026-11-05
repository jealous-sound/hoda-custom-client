#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BACKEND_URL_ENV "HOD_BACKEND_URL"
#define BACKEND_PLAYER_ENV "HOD_PLAYER_ID"
#define BACKEND_ACCOUNT_ENV "HOD_ACCOUNT_PATH"
#define BACKEND_DEFAULT_ACCOUNT_PATH "account.dat"
#define BACKEND_PLAYER_ID_CAP 65
#define BACKEND_SECRET_CAP 129
#define BACKEND_TIMEOUT_SECONDS 5
#define BACKEND_HOST_CAP 256
#define BACKEND_PATH_CAP 512
#define BACKEND_TOKEN_CAP 128
#define BACKEND_REVISION_HEADER "X-Profile-Revision"
#define BACKEND_HTTP_OK 200
#define BACKEND_HTTP_CREATED 201
#define BACKEND_HTTP_UNAUTHORIZED 401
#define BACKEND_HTTP_NOT_FOUND 404
#define BACKEND_HTTP_CONFLICT 409
#define BACKEND_HTTP_UNPROCESSABLE 422
#define BACKEND_REQUEST_ID_CAP 33
#define BACKEND_NO_TICKET 0
#define BACKEND_SESSION_RETRY_SECONDS 10
#define BACKEND_REVISION_UNKNOWN -1
#define BACKEND_UPLOAD_RETRY_SECONDS BACKEND_SESSION_RETRY_SECONDS
#define BACKEND_SHUTDOWN_FLUSH_SECONDS 8

typedef struct {
    bool resolved;
    bool enabled;
    char host[BACKEND_HOST_CAP];
    char port[16];
    bool session_open;
    long long session_failed_at;
    char token[BACKEND_TOKEN_CAP];
    bool account_ready;
    bool account_fresh;
    char player_id[BACKEND_PLAYER_ID_CAP];
    char secret[BACKEND_SECRET_CAP];
    long long clock_offset;
} BackendState;

static BackendState backend_state;

typedef struct {
    int status;
    char *body;
    long long revision;
} BackendResponse;

static bool BackendParseUrl(const char *url, BackendState *state) {
    const char *scheme = "http://";
    if (strncmp(url, scheme, strlen(scheme)) != 0) return false;
    const char *authority = url + strlen(scheme);
    size_t length = strcspn(authority, "/");
    const char *colon = memchr(authority, ':', length);
    size_t host_length = colon ? (size_t)(colon - authority) : length;
    if (host_length == 0 || host_length >= sizeof(state->host)) return false;
    memcpy(state->host, authority, host_length);
    state->host[host_length] = '\0';
    size_t port_length = colon ? length - host_length - 1 : 2;
    if (port_length == 0 || port_length >= sizeof(state->port)) return false;
    if (colon) {
        memcpy(state->port, colon + 1, port_length);
        state->port[port_length] = '\0';
    } else {
        StrCopy(state->port, sizeof(state->port), "80");
    }
    return true;
}

static BackendState *BackendResolve(void) {
    if (backend_state.resolved) return &backend_state;
    backend_state.resolved = true;
    const char *url = getenv(BACKEND_URL_ENV);
    if (!url || !url[0]) return &backend_state;
    backend_state.enabled = BackendParseUrl(url, &backend_state);
    if (!backend_state.enabled) TraceLog(LOG_WARNING, "Backend: unsupported %s=%s", BACKEND_URL_ENV, url);
    return &backend_state;
}

bool BackendEnabled(void) {
    return BackendResolve()->enabled;
}

static int BackendOpenSocket(const BackendState *state) {
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    struct addrinfo *addresses = NULL;
    if (getaddrinfo(state->host, state->port, &hints, &addresses) != 0) return -1;
    int fd = -1;
    struct timeval timeout = {.tv_sec = BACKEND_TIMEOUT_SECONDS};
    for (struct addrinfo *address = addresses; address; address = address->ai_next) {
        fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) continue;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        if (connect(fd, address->ai_addr, address->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(addresses);
    return fd;
}

static bool BackendSendAll(int fd, const char *data, size_t length) {
    while (length > 0) {
        ssize_t sent = send(fd, data, length, 0);
        if (sent <= 0) return false;
        data += sent;
        length -= (size_t)sent;
    }
    return true;
}

static char *BackendReceiveAll(int fd, size_t *out_length) {
    size_t capacity = 16384, length = 0;
    char *buffer = malloc(capacity + 1);
    if (!buffer) return NULL;
    for (;;) {
        if (length == capacity) {
            char *grown = realloc(buffer, capacity * 2 + 1);
            if (!grown) {
                free(buffer);
                return NULL;
            }
            buffer = grown;
            capacity *= 2;
        }
        ssize_t received = recv(fd, buffer + length, capacity - length, 0);
        if (received < 0) {
            free(buffer);
            return NULL;
        }
        if (received == 0) break;
        length += (size_t)received;
    }
    buffer[length] = '\0';
    *out_length = length;
    return buffer;
}

static long long BackendHeaderInt(const char *headers, const char *headers_end, const char *name) {
    size_t name_length = strlen(name);
    for (const char *line = headers; line && line < headers_end;) {
        const char *next = strstr(line, "\r\n");
        if (!next || next > headers_end) break;
        if (strncasecmp(line, name, name_length) == 0 && line[name_length] == ':')
            return strtoll(line + name_length + 1, NULL, 10);
        line = next + 2;
    }
    return 0;
}

static bool
BackendRequest(const char *method, const char *path, const char *body, long long if_match, BackendResponse *response) {
    BackendState *state = BackendResolve();
    memset(response, 0, sizeof(*response));
    if (!state->enabled) return false;
    int fd = BackendOpenSocket(state);
    if (fd < 0) {
        TraceLog(LOG_WARNING, "Backend: cannot reach %s:%s", state->host, state->port);
        return false;
    }

    size_t body_length = body ? strlen(body) : 0;
    char head[1024];
    int head_length = snprintf(
        head,
        sizeof(head),
        "%s %s HTTP/1.1\r\nHost: %s:%s\r\nConnection: close\r\n"
        "Content-Type: application/json\r\nContent-Length: %zu\r\n",
        method,
        path,
        state->host,
        state->port,
        body_length
    );
    if (state->session_open && head_length > 0 && head_length < (int)sizeof(head))
        head_length += snprintf(
            head + head_length, sizeof(head) - (size_t)head_length, "Authorization: Bearer %s\r\n", state->token
        );
    if (if_match >= 0 && head_length > 0 && head_length < (int)sizeof(head))
        head_length +=
            snprintf(head + head_length, sizeof(head) - (size_t)head_length, "If-Match: \"%lld\"\r\n", if_match);
    if (head_length > 0 && head_length < (int)sizeof(head))
        head_length += snprintf(head + head_length, sizeof(head) - (size_t)head_length, "\r\n");
    bool sent = head_length > 0 && head_length < (int)sizeof(head) && BackendSendAll(fd, head, (size_t)head_length) &&
                (!body || BackendSendAll(fd, body, body_length));
    size_t raw_length = 0;
    char *raw = sent ? BackendReceiveAll(fd, &raw_length) : NULL;
    close(fd);
    if (!raw) {
        TraceLog(LOG_WARNING, "Backend: %s %s failed", method, path);
        return false;
    }

    char *headers_end = strstr(raw, "\r\n\r\n");
    if (!headers_end || sscanf(raw, "HTTP/%*s %d", &response->status) != 1) {
        free(raw);
        TraceLog(LOG_WARNING, "Backend: malformed response to %s %s", method, path);
        return false;
    }
    response->revision = BackendHeaderInt(raw, headers_end, BACKEND_REVISION_HEADER);
    const char *payload = headers_end + 4;
    size_t payload_length = raw_length - (size_t)(payload - raw);
    long long declared_length = BackendHeaderInt(raw, headers_end, "Content-Length");
    if (declared_length != (long long)payload_length) {
        free(raw);
        TraceLog(
            LOG_WARNING, "Backend: %s %s returned %zu of %lld bytes", method, path, payload_length, declared_length
        );
        return false;
    }
    response->body = malloc(payload_length + 1);
    if (response->body) {
        memcpy(response->body, payload, payload_length);
        response->body[payload_length] = '\0';
    }
    free(raw);
    return response->body != NULL;
}

static const char *BackendAccountPath(void) {
    const char *path = getenv(BACKEND_ACCOUNT_ENV);
    return path && path[0] ? path : BACKEND_DEFAULT_ACCOUNT_PATH;
}

static bool BackendAdoptCredentials(BackendState *state, const cJSON *json) {
    const char *player_id = JsonStr(json, "player_id");
    const char *secret = JsonStr(json, "secret");
    if (!player_id[0] || !secret[0] || strlen(player_id) >= sizeof(state->player_id) ||
        strlen(secret) >= sizeof(state->secret))
        return false;
    StrCopy(state->player_id, sizeof(state->player_id), player_id);
    StrCopy(state->secret, sizeof(state->secret), secret);
    return true;
}

static bool BackendLoadAccount(BackendState *state, const char *path) {
    if (!FileExists(path)) return false;
    char *text = LoadFileText(path);
    cJSON *json = text ? cJSON_Parse(text) : NULL;
    bool ok = BackendAdoptCredentials(state, json);
    cJSON_Delete(json);
    if (text) UnloadFileText(text);
    if (!ok) TraceLog(LOG_WARNING, "Backend: ignoring unreadable account file %s", path);
    return ok;
}

static bool BackendWriteAccount(const BackendState *state, const char *path) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "player_id", state->player_id);
    cJSON_AddStringToObject(json, "secret", state->secret);
    char *text = cJSON_Print(json);
    cJSON_Delete(json);
    char temporary[1024];
    int needed = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    bool ok = text && needed > 0 && needed < (int)sizeof(temporary) && SaveFileText(temporary, text);
    cJSON_free(text);
    if (ok) ok = rename(temporary, path) == 0;
    if (!ok) TraceLog(LOG_WARNING, "Backend: could not write account file %s", path);
    return ok;
}

static bool BackendCreateGuest(BackendState *state, const char *path) {
    BackendResponse response;
    if (!BackendRequest("POST", "/v1/players", NULL, -1, &response)) return false;
    cJSON *json = response.status == BACKEND_HTTP_CREATED ? cJSON_Parse(response.body) : NULL;
    bool ok = BackendAdoptCredentials(state, json) && BackendWriteAccount(state, path);
    state->account_fresh = ok;
    if (ok)
        TraceLog(LOG_INFO, "Backend: created guest account %s", state->player_id);
    else
        TraceLog(LOG_WARNING, "Backend: guest account creation failed (HTTP %d)", response.status);
    cJSON_Delete(json);
    free(response.body);
    return ok;
}

static bool BackendResolveAccount(BackendState *state) {
    if (state->account_ready) return true;
    const char *forced = getenv(BACKEND_PLAYER_ENV);
    if (forced && forced[0]) {
        if (strlen(forced) >= sizeof(state->player_id)) return false;
        StrCopy(state->player_id, sizeof(state->player_id), forced);
        state->secret[0] = '\0';
        state->account_ready = true;
        return true;
    }
    const char *path = BackendAccountPath();
    state->account_ready = BackendLoadAccount(state, path) || BackendCreateGuest(state, path);
    return state->account_ready;
}

static const char *BackendPlayerId(void) {
    return backend_state.player_id;
}

static bool BackendOpenSession(void) {
    BackendState *state = BackendResolve();
    if (state->session_open) return true;
    long long sent_at = (long long)time(NULL);
    if (state->session_failed_at && sent_at - state->session_failed_at < BACKEND_SESSION_RETRY_SECONDS) return false;
    if (!BackendResolveAccount(state)) {
        state->session_failed_at = sent_at;
        return false;
    }
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "player_id", BackendPlayerId());
    if (state->secret[0]) cJSON_AddStringToObject(request, "secret", state->secret);
    char *body = cJSON_PrintUnformatted(request);
    cJSON_Delete(request);
    if (!body) return false;
    BackendResponse response;
    bool ok = BackendRequest("POST", "/v1/sessions", body, -1, &response);
    cJSON_free(body);
    if (!ok) {
        state->session_failed_at = sent_at;
        return false;
    }
    cJSON *json = response.status == BACKEND_HTTP_OK ? cJSON_Parse(response.body) : NULL;
    const char *token = JsonStr(json, "token");
    cJSON *server_time = cJSON_GetObjectItemCaseSensitive(json, "server_time");
    if (token[0] && strlen(token) < sizeof(state->token) && cJSON_IsNumber(server_time)) {
        StrCopy(state->token, sizeof(state->token), token);
        state->clock_offset = (long long)server_time->valuedouble - sent_at;
        state->session_open = true;
        TraceLog(LOG_INFO, "Backend: session open for %s (clock offset %llds)", BackendPlayerId(), state->clock_offset);
    } else {
        state->session_failed_at = sent_at;
        TraceLog(LOG_WARNING, "Backend: session refused (HTTP %d)", response.status);
    }
    cJSON_Delete(json);
    free(response.body);
    return state->session_open;
}

static void BackendProfilePath(char *path, int cap) {
    snprintf(path, cap, "/v1/players/%s/profile", BackendPlayerId());
}

typedef enum {
    BACKEND_PROFILE_FOUND,
    BACKEND_PROFILE_MISSING,
    BACKEND_PROFILE_UNAVAILABLE,
} BackendProfileFetch;

static BackendProfileFetch BackendDownloadProfile(char **out_text, long long *out_revision) {
    *out_text = NULL;
    *out_revision = BACKEND_REVISION_UNKNOWN;
    if (!BackendOpenSession()) return BACKEND_PROFILE_UNAVAILABLE;
    if (backend_state.account_fresh) {
        backend_state.account_fresh = false;
        *out_revision = 0;
        TraceLog(LOG_INFO, "Backend: new guest %s has no profile yet, starting fresh", BackendPlayerId());
        return BACKEND_PROFILE_MISSING;
    }
    char path[BACKEND_PATH_CAP];
    BackendProfilePath(path, sizeof(path));
    BackendResponse response;
    if (!BackendRequest("GET", path, NULL, -1, &response)) return BACKEND_PROFILE_UNAVAILABLE;
    if (response.status == BACKEND_HTTP_OK) {
        *out_text = response.body;
        *out_revision = response.revision;
        return BACKEND_PROFILE_FOUND;
    }
    BackendProfileFetch result = BACKEND_PROFILE_UNAVAILABLE;
    if (response.status == BACKEND_HTTP_NOT_FOUND) {
        *out_revision = 0;
        result = BACKEND_PROFILE_MISSING;
        TraceLog(LOG_INFO, "Backend: no profile for %s yet, starting fresh", BackendPlayerId());
    } else {
        TraceLog(LOG_WARNING, "Backend: profile fetch failed (HTTP %d)", response.status);
    }
    free(response.body);
    return result;
}

typedef enum {
    BACKEND_UPLOAD_STORED,
    BACKEND_UPLOAD_REJECTED,
    BACKEND_UPLOAD_UNREACHABLE,
} BackendUpload;

static BackendUpload BackendUploadProfile(const char *text, long long revision, long long *stored_revision) {
    if (!BackendOpenSession()) return BACKEND_UPLOAD_UNREACHABLE;
    char path[BACKEND_PATH_CAP];
    BackendProfilePath(path, sizeof(path));
    BackendResponse response;
    if (!BackendRequest("PUT", path, text, revision, &response)) return BACKEND_UPLOAD_UNREACHABLE;
    BackendUpload result = BACKEND_UPLOAD_REJECTED;
    if (response.status == BACKEND_HTTP_OK) {
        *stored_revision = response.revision;
        result = BACKEND_UPLOAD_STORED;
    } else if (response.status == BACKEND_HTTP_UNAUTHORIZED) {
        backend_state.session_open = false;
        result = BACKEND_UPLOAD_UNREACHABLE;
        TraceLog(LOG_WARNING, "Backend: session expired, reopening before the next save");
    } else if (response.status == BACKEND_HTTP_CONFLICT) {
        TraceLog(LOG_WARNING, "Backend: profile changed elsewhere, save rejected (revision %lld)", revision);
    } else {
        TraceLog(LOG_WARNING, "Backend: profile save failed (HTTP %d): %s", response.status, response.body);
    }
    free(response.body);
    return result;
}

typedef enum {
    BACKEND_ACTION_IDLE,
    BACKEND_ACTION_PENDING,
    BACKEND_ACTION_APPLIED,
    BACKEND_ACTION_REFUSED,
    BACKEND_ACTION_CONFLICT,
    BACKEND_ACTION_UNREACHABLE,
} BackendActionState;

static BackendActionState
BackendSendAction(const char *route, const char *body, long long revision, char **out_text, long long *out_revision) {
    *out_text = NULL;
    if (!BackendOpenSession()) return BACKEND_ACTION_UNREACHABLE;
    char path[BACKEND_PATH_CAP];
    snprintf(path, sizeof(path), "/v1/players/%s/%s", BackendPlayerId(), route);
    BackendResponse response;
    if (!BackendRequest("POST", path, body, revision, &response)) return BACKEND_ACTION_UNREACHABLE;
    BackendActionState state = BACKEND_ACTION_UNREACHABLE;
    if (response.status == BACKEND_HTTP_OK) {
        state = BACKEND_ACTION_APPLIED;
        *out_revision = response.revision;
    } else if (response.status == BACKEND_HTTP_UNPROCESSABLE) {
        state = BACKEND_ACTION_REFUSED;
    } else if (response.status == BACKEND_HTTP_CONFLICT) {
        state = BACKEND_ACTION_CONFLICT;
        TraceLog(LOG_WARNING, "Backend: %s refused, profile changed since revision %lld", route, revision);
    } else if (response.status == BACKEND_HTTP_UNAUTHORIZED) {
        backend_state.session_open = false;
    } else {
        TraceLog(LOG_WARNING, "Backend: %s failed (HTTP %d): %s", route, response.status, response.body);
    }
    *out_text = response.body;
    return state;
}

typedef enum {
    BACKEND_QUERY_IDLE,
    BACKEND_QUERY_PENDING,
    BACKEND_QUERY_ANSWERED,
    BACKEND_QUERY_UNREACHABLE,
} BackendQueryState;

#define BACKEND_QUERY_METHOD_CAP 8

static BackendQueryState
BackendSendQuery(const char *method, const char *route, const char *body, char **out_text, int *out_status) {
    *out_text = NULL;
    *out_status = 0;
    if (!BackendOpenSession()) return BACKEND_QUERY_UNREACHABLE;
    char path[BACKEND_PATH_CAP];
    snprintf(path, sizeof(path), "/v1/players/%s/%s", BackendPlayerId(), route);
    BackendResponse response;
    if (!BackendRequest(method, path, body, BACKEND_REVISION_UNKNOWN, &response)) return BACKEND_QUERY_UNREACHABLE;
    if (response.status == BACKEND_HTTP_UNAUTHORIZED) {
        backend_state.session_open = false;
        free(response.body);
        return BACKEND_QUERY_UNREACHABLE;
    }
    *out_status = response.status;
    *out_text = response.body;
    return BACKEND_QUERY_ANSWERED;
}

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t wake;
    pthread_cond_t settled;
    pthread_t thread;
    bool started;
    bool stopping;
    bool retry_wanted;
    bool session_wanted;
    bool fetch_wanted;
    bool fetch_running;
    bool fetch_ready;
    BackendProfileFetch fetch_result;
    char *fetch_text;
    long long fetch_revision;
    char *upload_text;
    bool upload_running;
    long long upload_due_at;
    long long revision;
    bool clock_known;
    long long clock_offset;
    char action_route[BACKEND_PATH_CAP];
    char *action_body;
    bool action_wanted;
    bool action_running;
    BackendActionState action_state;
    int action_ticket;
    int next_ticket;
    char *action_text;
    char *retry_route_body;
    char retry_request_id[BACKEND_REQUEST_ID_CAP];
    char query_method[BACKEND_QUERY_METHOD_CAP];
    char query_route[BACKEND_PATH_CAP];
    char *query_body;
    bool query_wanted;
    bool query_running;
    BackendQueryState query_state;
    int query_status;
    int query_ticket;
    int next_query_ticket;
    char *query_text;
} BackendWorker;

static BackendWorker backend_worker = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .wake = PTHREAD_COND_INITIALIZER,
    .settled = PTHREAD_COND_INITIALIZER,
    .revision = BACKEND_REVISION_UNKNOWN,
};

static void BackendWorkerPublishClock(BackendWorker *w) {
    if (!backend_state.session_open) return;
    w->clock_known = true;
    w->clock_offset = backend_state.clock_offset;
}

static void BackendWorkerFetch(BackendWorker *w) {
    w->fetch_wanted = false;
    w->fetch_running = true;
    pthread_mutex_unlock(&w->lock);
    char *text = NULL;
    long long revision = BACKEND_REVISION_UNKNOWN;
    BackendProfileFetch result = BackendDownloadProfile(&text, &revision);
    pthread_mutex_lock(&w->lock);
    free(w->fetch_text);
    w->fetch_text = text;
    w->fetch_revision = revision;
    w->fetch_result = result;
    w->fetch_running = false;
    w->fetch_ready = true;
}

static void BackendWorkerUpload(BackendWorker *w) {
    char *text = w->upload_text;
    long long revision = w->revision;
    w->upload_text = NULL;
    w->upload_running = true;
    pthread_mutex_unlock(&w->lock);
    long long stored_revision = revision;
    BackendUpload result = BackendUploadProfile(text, revision, &stored_revision);
    pthread_mutex_lock(&w->lock);
    w->upload_running = false;
    if (result == BACKEND_UPLOAD_STORED) w->revision = stored_revision;
    if (result == BACKEND_UPLOAD_UNREACHABLE && !w->upload_text) {
        w->upload_text = text;
        w->upload_due_at = (long long)time(NULL) + BACKEND_UPLOAD_RETRY_SECONDS;
    } else {
        free(text);
    }
}

static void BackendWorkerAction(BackendWorker *w) {
    char *body = w->action_body;
    long long revision = w->revision;
    char route[BACKEND_PATH_CAP];
    StrCopy(route, sizeof(route), w->action_route);
    bool upload_stuck = w->upload_text != NULL;
    w->action_body = NULL;
    w->action_wanted = false;
    w->action_running = true;
    pthread_mutex_unlock(&w->lock);
    char *text = NULL;
    long long stored_revision = revision;
    BackendActionState state =
        upload_stuck ? BACKEND_ACTION_UNREACHABLE : BackendSendAction(route, body, revision, &text, &stored_revision);
    free(body);
    pthread_mutex_lock(&w->lock);
    w->action_running = false;
    if (state == BACKEND_ACTION_APPLIED) w->revision = stored_revision;
    free(w->action_text);
    w->action_text = text;
    w->action_state = state;
}

static void BackendWorkerQuery(BackendWorker *w) {
    char *body = w->query_body;
    char method[BACKEND_QUERY_METHOD_CAP];
    char route[BACKEND_PATH_CAP];
    StrCopy(method, sizeof(method), w->query_method);
    StrCopy(route, sizeof(route), w->query_route);
    w->query_body = NULL;
    w->query_wanted = false;
    w->query_running = true;
    pthread_mutex_unlock(&w->lock);
    char *text = NULL;
    int status = 0;
    BackendQueryState state = BackendSendQuery(method, route, body, &text, &status);
    free(body);
    pthread_mutex_lock(&w->lock);
    w->query_running = false;
    free(w->query_text);
    w->query_text = text;
    w->query_status = status;
    w->query_state = state;
}

static void BackendWorkerOpenSession(BackendWorker *w) {
    w->session_wanted = false;
    pthread_mutex_unlock(&w->lock);
    BackendOpenSession();
    pthread_mutex_lock(&w->lock);
}

static void BackendWorkerSleep(BackendWorker *w) {
    if (!w->upload_text) {
        pthread_cond_wait(&w->wake, &w->lock);
        return;
    }
    struct timespec due = {.tv_sec = (time_t)w->upload_due_at};
    pthread_cond_timedwait(&w->wake, &w->lock, &due);
}

static void *BackendWorkerRun(void *argument) {
    BackendWorker *w = argument;
    pthread_mutex_lock(&w->lock);
    for (;;) {
        bool upload_due = w->upload_text && (long long)time(NULL) >= w->upload_due_at;
        if (w->retry_wanted) {
            w->retry_wanted = false;
            backend_state.session_failed_at = 0;
        } else if (w->fetch_wanted) {
            BackendWorkerFetch(w);
        } else if (upload_due) {
            BackendWorkerUpload(w);
        } else if (w->action_wanted) {
            BackendWorkerAction(w);
        } else if (w->query_wanted) {
            BackendWorkerQuery(w);
        } else if (w->session_wanted) {
            BackendWorkerOpenSession(w);
        } else if (w->stopping) {
            break;
        } else {
            BackendWorkerSleep(w);
            continue;
        }
        BackendWorkerPublishClock(w);
        pthread_cond_broadcast(&w->settled);
    }
    pthread_mutex_unlock(&w->lock);
    return NULL;
}

static BackendWorker *BackendWorkerStart(void) {
    BackendWorker *w = &backend_worker;
    BackendResolve();
    if (w->started) return w;
    if (pthread_create(&w->thread, NULL, BackendWorkerRun, w) != 0)
        TraceLog(LOG_FATAL, "Backend: could not start the network worker");
    w->started = true;
    return w;
}

static void BackendWorkerSignal(BackendWorker *w) {
    pthread_cond_signal(&w->wake);
}

void BackendRetryNow(void) {
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    w->retry_wanted = true;
    BackendWorkerSignal(w);
    pthread_mutex_unlock(&w->lock);
}

static long long server_now_shift;

void ServerNowShift(long long seconds) {
    server_now_shift = seconds;
}

long long ServerNow(void) {
    long long now = (long long)time(NULL) + server_now_shift;
    if (!BackendEnabled()) return now;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    bool known = w->clock_known;
    long long offset = w->clock_offset;
    if (!known && !w->session_wanted) {
        w->session_wanted = true;
        BackendWorkerSignal(w);
    }
    pthread_mutex_unlock(&w->lock);
    return known ? now + offset : now;
}

static void BackendRequestFetchLocked(BackendWorker *w) {
    if (w->fetch_ready || w->fetch_running || w->fetch_wanted) return;
    w->fetch_wanted = true;
    BackendWorkerSignal(w);
}

void BackendBeginProfileFetch(void) {
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    BackendRequestFetchLocked(w);
    pthread_mutex_unlock(&w->lock);
}

bool BackendProfileFetchSettled(void) {
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    bool settled = w->fetch_ready;
    pthread_mutex_unlock(&w->lock);
    return settled;
}

BackendProfileFetch BackendFetchProfile(char **out_text, long long *out_revision) {
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    BackendRequestFetchLocked(w);
    while (!w->fetch_ready) pthread_cond_wait(&w->settled, &w->lock);
    w->fetch_ready = false;
    *out_text = w->fetch_text;
    *out_revision = w->fetch_revision;
    w->fetch_text = NULL;
    BackendProfileFetch result = w->fetch_result;
    if (result == BACKEND_PROFILE_MISSING) w->revision = w->fetch_revision;
    pthread_mutex_unlock(&w->lock);
    return result;
}

void BackendAcceptProfile(long long revision) {
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    w->revision = revision;
    pthread_mutex_unlock(&w->lock);
}

static char *BackendCopyText(const char *text) {
    size_t length = strlen(text);
    char *copy = malloc(length + 1);
    if (copy) memcpy(copy, text, length + 1);
    return copy;
}

bool BackendStoreProfile(const char *text) {
    BackendWorker *w = BackendWorkerStart();
    char *copy = BackendCopyText(text);
    if (!copy) return false;
    pthread_mutex_lock(&w->lock);
    bool loaded = w->revision != BACKEND_REVISION_UNKNOWN;
    if (loaded) {
        free(w->upload_text);
        w->upload_text = copy;
        w->upload_due_at = 0;
        BackendWorkerSignal(w);
    }
    pthread_mutex_unlock(&w->lock);
    if (!loaded) {
        free(copy);
        TraceLog(LOG_WARNING, "Backend: profile was never loaded, save refused to protect server progress");
    }
    return loaded;
}

static bool BackendWorkerInFlight(const BackendWorker *w) {
    bool upload_due = w->upload_text && (long long)time(NULL) >= w->upload_due_at;
    return w->fetch_wanted || w->fetch_running || w->upload_running || upload_due || w->action_wanted ||
           w->action_running;
}

static char *BackendJoinRouteBody(const char *route, const char *body) {
    size_t length = strlen(route) + strlen(body) + 2;
    char *joined = malloc(length);
    if (joined) snprintf(joined, length, "%s\n%s", route, body);
    return joined;
}

static void BackendChooseRequestId(BackendWorker *w, const char *route, const char *body) {
    char *signature = BackendJoinRouteBody(route, body);
    bool retrying = signature && w->retry_route_body && !strcmp(signature, w->retry_route_body);
    if (!retrying) {
        snprintf(
            w->retry_request_id,
            sizeof(w->retry_request_id),
            "%08x%08x%08llx",
            (unsigned)GetRandomValue(0, INT_MAX),
            (unsigned)GetRandomValue(0, INT_MAX),
            (unsigned long long)time(NULL)
        );
    }
    free(w->retry_route_body);
    w->retry_route_body = signature;
}

int BackendBeginAction(const char *route, cJSON *fields) {
    if (!BackendEnabled() || !fields) return BACKEND_NO_TICKET;
    char *unsigned_body = cJSON_PrintUnformatted(fields);
    if (!unsigned_body) return BACKEND_NO_TICKET;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    bool busy = w->action_wanted || w->action_running || w->action_state == BACKEND_ACTION_PENDING;
    int ticket = BACKEND_NO_TICKET;
    if (!busy) {
        BackendChooseRequestId(w, route, unsigned_body);
        cJSON_DeleteItemFromObjectCaseSensitive(fields, "request_id");
        cJSON_AddStringToObject(fields, "request_id", w->retry_request_id);
        free(w->action_body);
        w->action_body = cJSON_PrintUnformatted(fields);
        StrCopy(w->action_route, sizeof(w->action_route), route);
        w->action_wanted = w->action_body != NULL;
        w->action_state = w->action_wanted ? BACKEND_ACTION_PENDING : BACKEND_ACTION_IDLE;
        if (w->upload_text) w->upload_due_at = 0;
        if (w->action_wanted) {
            w->next_ticket = w->next_ticket == INT_MAX ? BACKEND_NO_TICKET + 1 : w->next_ticket + 1;
            ticket = w->action_ticket = w->next_ticket;
        }
        BackendWorkerSignal(w);
    }
    pthread_mutex_unlock(&w->lock);
    cJSON_free(unsigned_body);
    return ticket;
}

BackendActionState BackendPollAction(int ticket, cJSON **out_json) {
    *out_json = NULL;
    if (!BackendEnabled() || ticket == BACKEND_NO_TICKET) return BACKEND_ACTION_IDLE;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    if (w->action_ticket != ticket) {
        pthread_mutex_unlock(&w->lock);
        return BACKEND_ACTION_IDLE;
    }
    BackendActionState state = w->action_state;
    bool settled = state != BACKEND_ACTION_IDLE && state != BACKEND_ACTION_PENDING;
    if (settled) {
        *out_json = w->action_text ? cJSON_Parse(w->action_text) : NULL;
        free(w->action_text);
        w->action_text = NULL;
        w->action_state = BACKEND_ACTION_IDLE;
        if (state != BACKEND_ACTION_UNREACHABLE) {
            free(w->retry_route_body);
            w->retry_route_body = NULL;
        }
    }
    pthread_mutex_unlock(&w->lock);
    return state;
}

int BackendBeginQuery(const char *method, const char *route, const cJSON *fields) {
    if (!BackendEnabled()) return BACKEND_NO_TICKET;
    char *body = fields ? cJSON_PrintUnformatted(fields) : NULL;
    if (fields && !body) return BACKEND_NO_TICKET;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    bool busy = w->query_wanted || w->query_running || w->query_state == BACKEND_QUERY_PENDING;
    int ticket = BACKEND_NO_TICKET;
    if (!busy) {
        StrCopy(w->query_method, sizeof(w->query_method), method);
        StrCopy(w->query_route, sizeof(w->query_route), route);
        free(w->query_body);
        w->query_body = NULL;
        if (body) w->query_body = BackendCopyText(body);
        w->query_wanted = !body || w->query_body;
        w->query_state = w->query_wanted ? BACKEND_QUERY_PENDING : BACKEND_QUERY_IDLE;
        if (w->query_wanted) {
            w->next_query_ticket = w->next_query_ticket == INT_MAX ? BACKEND_NO_TICKET + 1 : w->next_query_ticket + 1;
            ticket = w->query_ticket = w->next_query_ticket;
        }
        BackendWorkerSignal(w);
    }
    pthread_mutex_unlock(&w->lock);
    if (body) cJSON_free(body);
    return ticket;
}

BackendQueryState BackendPollQuery(int ticket, cJSON **out_json, int *out_status) {
    *out_json = NULL;
    *out_status = 0;
    if (!BackendEnabled() || ticket == BACKEND_NO_TICKET) return BACKEND_QUERY_IDLE;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    if (w->query_ticket != ticket) {
        pthread_mutex_unlock(&w->lock);
        return BACKEND_QUERY_IDLE;
    }
    BackendQueryState state = w->query_state;
    if (state == BACKEND_QUERY_ANSWERED || state == BACKEND_QUERY_UNREACHABLE) {
        *out_json = w->query_text ? cJSON_Parse(w->query_text) : NULL;
        *out_status = w->query_status;
        free(w->query_text);
        w->query_text = NULL;
        w->query_state = BACKEND_QUERY_IDLE;
    }
    pthread_mutex_unlock(&w->lock);
    return state;
}

bool BackendUploadPending(void) {
    if (!BackendEnabled()) return false;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    bool pending = w->upload_text != NULL || w->upload_running;
    pthread_mutex_unlock(&w->lock);
    return pending;
}

bool BackendBusy(void) {
    if (!BackendEnabled()) return false;
    BackendWorker *w = BackendWorkerStart();
    pthread_mutex_lock(&w->lock);
    bool busy = BackendWorkerInFlight(w);
    pthread_mutex_unlock(&w->lock);
    return busy;
}

void BackendShutdown(void) {
    BackendWorker *w = &backend_worker;
    if (!w->started) return;
    pthread_mutex_lock(&w->lock);
    if (w->upload_text) w->upload_due_at = 0;
    BackendWorkerSignal(w);
    struct timespec deadline = {.tv_sec = time(NULL) + BACKEND_SHUTDOWN_FLUSH_SECONDS};
    while ((w->upload_text || w->upload_running) &&
           pthread_cond_timedwait(&w->settled, &w->lock, &deadline) != ETIMEDOUT) {}
    bool unsaved = w->upload_text || w->upload_running;
    bool idle = !w->upload_running && !w->fetch_running;
    w->stopping = true;
    BackendWorkerSignal(w);
    pthread_mutex_unlock(&w->lock);
    if (unsaved) TraceLog(LOG_WARNING, "Backend: the last save did not reach the server before exit");
    if (idle)
        pthread_join(w->thread, NULL);
    else
        pthread_detach(w->thread);
    w->started = false;
}
