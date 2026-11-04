#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BACKEND_URL_ENV "HOD_BACKEND_URL"
#define BACKEND_PLAYER_ENV "HOD_PLAYER_ID"
#define BACKEND_DEFAULT_PLAYER "local"
#define BACKEND_TIMEOUT_SECONDS 5
#define BACKEND_HOST_CAP 256
#define BACKEND_PATH_CAP 512
#define BACKEND_TOKEN_CAP 128
#define BACKEND_REVISION_HEADER "X-Profile-Revision"
#define BACKEND_HTTP_OK 200
#define BACKEND_HTTP_NOT_FOUND 404
#define BACKEND_HTTP_CONFLICT 409
#define BACKEND_SESSION_RETRY_SECONDS 10
#define BACKEND_REVISION_UNKNOWN -1

typedef struct {
    bool resolved;
    bool enabled;
    char host[BACKEND_HOST_CAP];
    char port[16];
    bool session_open;
    long long session_failed_at;
    char token[BACKEND_TOKEN_CAP];
    long long clock_offset;
    long long profile_revision;
} BackendState;

static BackendState backend_state;

typedef struct {
    int status;
    char *body;
    long long revision;
} BackendResponse;

static long long BackendProfileRevision(void);

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

static const char *BackendPlayerId(void) {
    const char *player = getenv(BACKEND_PLAYER_ENV);
    return player && player[0] ? player : BACKEND_DEFAULT_PLAYER;
}

static bool BackendOpenSession(void) {
    BackendState *state = BackendResolve();
    if (state->session_open) return true;
    long long sent_at = (long long)time(NULL);
    if (state->session_failed_at && sent_at - state->session_failed_at < BACKEND_SESSION_RETRY_SECONDS) return false;
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "player_id", BackendPlayerId());
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

long long ServerNow(void) {
    long long now = (long long)time(NULL);
    if (!BackendEnabled() || !BackendOpenSession()) return now;
    return now + backend_state.clock_offset;
}

static void BackendProfilePath(char *path, int cap) {
    snprintf(path, cap, "/v1/players/%s/profile", BackendPlayerId());
}

static long long BackendProfileRevision(void) {
    return BackendResolve()->profile_revision;
}

void BackendAcceptProfile(long long revision) {
    backend_state.profile_revision = revision;
}

bool BackendFetchProfile(char **out_text, long long *out_revision) {
    *out_text = NULL;
    *out_revision = BACKEND_REVISION_UNKNOWN;
    if (!BackendOpenSession()) return false;
    char path[BACKEND_PATH_CAP];
    BackendProfilePath(path, sizeof(path));
    BackendResponse response;
    if (!BackendRequest("GET", path, NULL, -1, &response)) return false;
    if (response.status == BACKEND_HTTP_OK) {
        *out_text = response.body;
        *out_revision = response.revision;
        return true;
    }
    if (response.status == BACKEND_HTTP_NOT_FOUND) {
        BackendAcceptProfile(0);
        TraceLog(LOG_INFO, "Backend: no profile for %s yet, starting fresh", BackendPlayerId());
    } else {
        TraceLog(LOG_WARNING, "Backend: profile fetch failed (HTTP %d)", response.status);
    }
    free(response.body);
    return false;
}

bool BackendStoreProfile(const char *text) {
    if (BackendProfileRevision() == BACKEND_REVISION_UNKNOWN) {
        TraceLog(LOG_WARNING, "Backend: profile was never loaded, save refused to protect server progress");
        return false;
    }
    if (!BackendOpenSession()) return false;
    char path[BACKEND_PATH_CAP];
    BackendProfilePath(path, sizeof(path));
    BackendResponse response;
    if (!BackendRequest("PUT", path, text, backend_state.profile_revision, &response)) return false;
    bool ok = response.status == BACKEND_HTTP_OK;
    if (ok)
        backend_state.profile_revision = response.revision;
    else if (response.status == BACKEND_HTTP_CONFLICT)
        TraceLog(
            LOG_WARNING,
            "Backend: profile changed elsewhere, save rejected (revision %lld)",
            backend_state.profile_revision
        );
    else
        TraceLog(LOG_WARNING, "Backend: profile save failed (HTTP %d): %s", response.status, response.body);
    free(response.body);
    return ok;
}
