#define CLIENT_UPDATE_PATH "/v1/client"
#define CLIENT_UPDATE_TIMEOUT_SECONDS 5
#define CLIENT_UPDATE_DEFAULT_DOWNLOAD_URL "https://hodareborn.com/#download"
#define CLIENT_UPDATE_SECURE_SCHEME "https://"
#define CLIENT_UPDATE_URL_SAFE_PUNCTUATION "-._~/?#=&:"
#define CLIENT_UPDATE_HTTP_OK 200
#define CLIENT_UPDATE_HTTP_UPGRADE_REQUIRED 426
#define CLIENT_UPDATE_VERSION_CAP 64
#define CLIENT_UPDATE_URL_CAP 256
#define CLIENT_UPDATE_REQUEST_URL_CAP 512
#define CLIENT_VERSION_DIRTY_SUFFIX "-dirty"
#define CLIENT_VERSION_DESCRIBE_MARK "-g"
#define CLIENT_VERSION_PARTS 3
#define CLIENT_VERSION_NUMBER_LIMIT 1000000000L
#define CLIENT_UPDATE_RECHECK_SECONDS 60.0
#define CLIENT_MAINTENANCE_NOTICE_SECONDS (30 * 60)
#define CLIENT_MAINTENANCE_BATTLE_LEAD_SECONDS (3 * 60)
#define CLIENT_MAINTENANCE_OVERRUN_SECONDS (10 * 60)

typedef struct {
    int core[CLIENT_VERSION_PARTS];
    char prerelease[CLIENT_UPDATE_VERSION_CAP];
    int ahead;
} ClientVersion;

typedef struct {
    char latest[CLIENT_UPDATE_VERSION_CAP];
    char minimum[CLIENT_UPDATE_VERSION_CAP];
    char download_url[CLIENT_UPDATE_URL_CAP];
    long long server_time;
    long long maintenance_at;
} ClientRelease;

typedef enum {
    CLIENT_UPDATE_CURRENT,
    CLIENT_UPDATE_AVAILABLE,
    CLIENT_UPDATE_REQUIRED,
} ClientUpdateStatus;

typedef enum {
    CLIENT_MAINTENANCE_NONE,
    CLIENT_MAINTENANCE_ANNOUNCED,
    CLIENT_MAINTENANCE_HOLDING_BATTLES,
} ClientMaintenancePhase;

typedef struct {
    ClientMaintenancePhase phase;
    long long at;
    long long seconds_left;
} ClientMaintenance;

typedef struct {
    pthread_mutex_t lock;
    bool configured;
    bool checking;
    double next_check_at;
    bool answered;
    bool refused;
    char request_url[CLIENT_UPDATE_REQUEST_URL_CAP];
    ClientRelease release;
    long long received_at;
} ClientUpdateState;

static ClientUpdateState client_update = {.lock = PTHREAD_MUTEX_INITIALIZER};

static bool ClientVersionReadNumber(const char **cursor, int *out) {
    const char *c = *cursor;
    if (!isdigit((unsigned char)*c)) return false;
    long value = 0;
    while (isdigit((unsigned char)*c)) {
        value = value * 10 + (*c - '0');
        if (value > CLIENT_VERSION_NUMBER_LIMIT) return false;
        c++;
    }
    *out = (int)value;
    *cursor = c;
    return true;
}

static bool ClientVersionAllDigits(const char *begin, const char *end) {
    if (begin >= end) return false;
    for (const char *c = begin; c < end; c++)
        if (!isdigit((unsigned char)*c)) return false;
    return true;
}

static bool ClientVersionAllHex(const char *begin, const char *end) {
    if (begin >= end) return false;
    for (const char *c = begin; c < end; c++)
        if (!isdigit((unsigned char)*c) && (*c < 'a' || *c > 'f')) return false;
    return true;
}

static bool ClientVersionValidPrerelease(const char *text) {
    if (!text[0]) return false;
    bool identifier_open = false;
    for (const char *c = text; *c; c++) {
        if (*c == '.') {
            if (!identifier_open) return false;
            identifier_open = false;
        } else if (isalnum((unsigned char)*c)) {
            identifier_open = true;
        } else {
            return false;
        }
    }
    return identifier_open;
}

static void ClientVersionTakeDescribe(char *rest, int *ahead) {
    char *mark = NULL;
    for (char *found = strstr(rest, CLIENT_VERSION_DESCRIBE_MARK); found;
         found = strstr(found + 1, CLIENT_VERSION_DESCRIBE_MARK))
        mark = found;
    if (!mark || !ClientVersionAllHex(mark + strlen(CLIENT_VERSION_DESCRIBE_MARK), rest + strlen(rest))) return;
    char *count = mark;
    while (count > rest && isdigit((unsigned char)count[-1])) count--;
    if (count == mark || count == rest || count[-1] != '-' || !ClientVersionAllDigits(count, mark)) return;
    *ahead = atoi(count);
    count[-1] = '\0';
}

static bool ClientVersionParse(const char *text, ClientVersion *out) {
    *out = (ClientVersion){0};
    if (!text) return false;
    char copy[CLIENT_UPDATE_VERSION_CAP];
    size_t length = strlen(text);
    if (length == 0 || length >= sizeof(copy)) return false;
    memcpy(copy, text, length + 1);
    size_t dirty = strlen(CLIENT_VERSION_DIRTY_SUFFIX);
    if (length > dirty && strcmp(copy + length - dirty, CLIENT_VERSION_DIRTY_SUFFIX) == 0) copy[length - dirty] = '\0';
    const char *cursor = copy[0] == 'v' ? copy + 1 : copy;
    for (int i = 0; i < CLIENT_VERSION_PARTS; i++) {
        if (i > 0 && *cursor++ != '.') return false;
        if (!ClientVersionReadNumber(&cursor, &out->core[i])) return false;
    }
    char rest[CLIENT_UPDATE_VERSION_CAP];
    StrCopy(rest, sizeof(rest), cursor);
    ClientVersionTakeDescribe(rest, &out->ahead);
    if (!rest[0]) return true;
    if (rest[0] != '-' || !ClientVersionValidPrerelease(rest + 1)) return false;
    StrCopy(out->prerelease, sizeof(out->prerelease), rest + 1);
    return true;
}

static int ClientVersionCompareInts(long long a, long long b) {
    return (a > b) - (a < b);
}

static int ClientVersionCompareIdentifiers(const char *a, size_t a_length, const char *b, size_t b_length) {
    bool a_number = ClientVersionAllDigits(a, a + a_length);
    bool b_number = ClientVersionAllDigits(b, b + b_length);
    if (a_number && b_number) return ClientVersionCompareInts(atoll(a), atoll(b));
    if (a_number != b_number) return a_number ? -1 : 1;
    int order = strncmp(a, b, a_length < b_length ? a_length : b_length);
    if (order != 0) return order < 0 ? -1 : 1;
    return ClientVersionCompareInts((long long)a_length, (long long)b_length);
}

static int ClientVersionComparePrerelease(const char *a, const char *b) {
    if (!a[0] || !b[0]) return ClientVersionCompareInts(!a[0], !b[0]);
    while (*a && *b) {
        size_t a_length = strcspn(a, ".");
        size_t b_length = strcspn(b, ".");
        int order = ClientVersionCompareIdentifiers(a, a_length, b, b_length);
        if (order != 0) return order;
        a += a_length + (a[a_length] == '.');
        b += b_length + (b[b_length] == '.');
    }
    return ClientVersionCompareInts(*a != '\0', *b != '\0');
}

static int ClientVersionCompare(const ClientVersion *a, const ClientVersion *b) {
    for (int i = 0; i < CLIENT_VERSION_PARTS; i++) {
        int order = ClientVersionCompareInts(a->core[i], b->core[i]);
        if (order != 0) return order;
    }
    int order = ClientVersionComparePrerelease(a->prerelease, b->prerelease);
    return order != 0 ? order : ClientVersionCompareInts(a->ahead, b->ahead);
}

static bool ClientVersionOlder(const char *running, const char *reference) {
    ClientVersion mine, theirs;
    return ClientVersionParse(running, &mine) && ClientVersionParse(reference, &theirs) &&
           ClientVersionCompare(&mine, &theirs) < 0;
}

static bool ClientUpdateSafeUrl(const char *url) {
    size_t scheme = strlen(CLIENT_UPDATE_SECURE_SCHEME);
    size_t length = url ? strlen(url) : 0;
    if (length <= scheme || length >= CLIENT_UPDATE_URL_CAP || strncmp(url, CLIENT_UPDATE_SECURE_SCHEME, scheme) != 0)
        return false;
    for (const char *c = url; *c; c++)
        if (!isalnum((unsigned char)*c) && !strchr(CLIENT_UPDATE_URL_SAFE_PUNCTUATION, *c)) return false;
    return true;
}

static long long ClientReleaseWhole(const cJSON *json, const char *key) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    return cJSON_IsNumber(item) && item->valuedouble > 0 ? (long long)item->valuedouble : 0;
}

static void ClientReleaseCopyVersion(char *out, size_t capacity, const char *text) {
    ClientVersion parsed;
    StrCopy(out, capacity, ClientVersionParse(text, &parsed) ? text : "");
}

static bool ClientReleaseParse(const char *json_text, ClientRelease *out) {
    *out = (ClientRelease){0};
    StrCopy(out->download_url, sizeof(out->download_url), CLIENT_UPDATE_DEFAULT_DOWNLOAD_URL);
    cJSON *json = json_text ? cJSON_Parse(json_text) : NULL;
    bool parsed = cJSON_IsObject(json);
    if (parsed) {
        ClientReleaseCopyVersion(out->latest, sizeof(out->latest), JsonStr(json, "latest"));
        ClientReleaseCopyVersion(out->minimum, sizeof(out->minimum), JsonStr(json, "minimum"));
        const char *url = JsonStr(json, "download_url");
        if (ClientUpdateSafeUrl(url)) StrCopy(out->download_url, sizeof(out->download_url), url);
        if (!out->latest[0] && out->minimum[0]) StrCopy(out->latest, sizeof(out->latest), out->minimum);
        out->server_time = ClientReleaseWhole(json, "server_time");
        out->maintenance_at = ClientReleaseWhole(json, "maintenance_at");
    }
    cJSON_Delete(json);
    return parsed;
}

static ClientUpdateStatus ClientUpdateStatusFor(const char *running, const ClientRelease *release) {
    if (ClientVersionOlder(running, release->minimum)) return CLIENT_UPDATE_REQUIRED;
    if (ClientVersionOlder(running, release->latest)) return CLIENT_UPDATE_AVAILABLE;
    return CLIENT_UPDATE_CURRENT;
}

static ClientRelease ClientUpdateCurrentRelease(void) {
    pthread_mutex_lock(&client_update.lock);
    ClientRelease release = client_update.release;
    pthread_mutex_unlock(&client_update.lock);
    return release;
}

static ClientMaintenance
ClientMaintenanceFor(const ClientRelease *release, long long received_at, long long local_now) {
    ClientMaintenance none = {.phase = CLIENT_MAINTENANCE_NONE};
    if (release->maintenance_at <= 0 || release->server_time <= 0) return none;
    long long server_now = release->server_time + (local_now - received_at);
    long long left = release->maintenance_at - server_now;
    if (left > CLIENT_MAINTENANCE_NOTICE_SECONDS || left <= -CLIENT_MAINTENANCE_OVERRUN_SECONDS) return none;
    bool holding = left <= CLIENT_MAINTENANCE_BATTLE_LEAD_SECONDS;
    return (ClientMaintenance){
        .phase = holding ? CLIENT_MAINTENANCE_HOLDING_BATTLES : CLIENT_MAINTENANCE_ANNOUNCED,
        .at = release->maintenance_at,
        .seconds_left = left > 0 ? left : 0,
    };
}

static void ClientUpdateAdopt(const ClientRelease *release, bool refused) {
    pthread_mutex_lock(&client_update.lock);
    client_update.release = *release;
    client_update.received_at = (long long)time(NULL);
    client_update.answered = true;
    client_update.refused = client_update.refused || refused;
    pthread_mutex_unlock(&client_update.lock);
}

static void ClientUpdateNoteRefusal(int status, const char *body) {
    if (status != CLIENT_UPDATE_HTTP_UPGRADE_REQUIRED) return;
    ClientRelease release;
    ClientReleaseParse(body, &release);
    ClientUpdateAdopt(&release, true);
    TraceLog(LOG_WARNING, "Update: the server refused client %s, %s is required", HOD_VERSION, release.minimum);
}

static bool ClientReleaseDiffers(const ClientRelease *a, const ClientRelease *b) {
    return strcmp(a->latest, b->latest) != 0 || strcmp(a->minimum, b->minimum) != 0 ||
           a->maintenance_at != b->maintenance_at;
}

static void ClientUpdateCheckOnce(void) {
    PlatformHttpRequest request = {
        .method = "GET", .url = client_update.request_url, .timeout_seconds = CLIENT_UPDATE_TIMEOUT_SECONDS
    };
    PlatformHttpResponse reply = {0};
    if (!PlatformHttpSend(&request, &reply)) return;
    ClientRelease release;
    if (reply.status == CLIENT_UPDATE_HTTP_OK && ClientReleaseParse(reply.body, &release)) {
        ClientRelease previous = ClientUpdateCurrentRelease();
        ClientUpdateAdopt(&release, false);
        if (ClientReleaseDiffers(&previous, &release))
            TraceLog(
                LOG_INFO,
                "Update: running %s, latest %s, minimum %s, restart in %llds",
                HOD_VERSION,
                release.latest,
                release.minimum,
                release.maintenance_at > 0 ? release.maintenance_at - release.server_time : 0
            );
    }
    ClientUpdateNoteRefusal(reply.status, reply.body);
    free(reply.body);
}

static void ClientUpdateCheckFinished(void) {
    pthread_mutex_lock(&client_update.lock);
    client_update.checking = false;
    pthread_mutex_unlock(&client_update.lock);
}

static void *ClientUpdateCheckRun(void *argument) {
    (void)argument;
    ClientUpdateCheckOnce();
    ClientUpdateCheckFinished();
    return NULL;
}

static void ClientUpdateCheckBegin(void) {
    pthread_mutex_lock(&client_update.lock);
    bool idle = client_update.configured && !client_update.checking;
    if (idle) client_update.checking = true;
    pthread_mutex_unlock(&client_update.lock);
    if (!idle) return;
    pthread_t thread;
    if (pthread_create(&thread, NULL, ClientUpdateCheckRun, NULL) == 0)
        pthread_detach(thread);
    else
        ClientUpdateCheckFinished();
}

static void ClientUpdateCheckStart(const char *server_url) {
    if (client_update.configured || !server_url || !server_url[0]) return;
    size_t length = strlen(server_url);
    while (length > 0 && server_url[length - 1] == '/') length--;
    int written = snprintf(
        client_update.request_url,
        sizeof(client_update.request_url),
        "%.*s%s",
        (int)length,
        server_url,
        CLIENT_UPDATE_PATH
    );
    if (written <= 0 || written >= (int)sizeof(client_update.request_url)) return;
    client_update.configured = true;
    client_update.next_check_at = GetTime() + CLIENT_UPDATE_RECHECK_SECONDS;
    ClientUpdateCheckBegin();
}

static void ClientUpdateKeepFresh(void) {
    if (!client_update.configured || GetTime() < client_update.next_check_at) return;
    client_update.next_check_at = GetTime() + CLIENT_UPDATE_RECHECK_SECONDS;
    ClientUpdateCheckBegin();
}

static ClientMaintenance ClientMaintenanceNow(void) {
    pthread_mutex_lock(&client_update.lock);
    bool answered = client_update.answered;
    ClientRelease release = client_update.release;
    long long received_at = client_update.received_at;
    pthread_mutex_unlock(&client_update.lock);
    if (!answered) return (ClientMaintenance){.phase = CLIENT_MAINTENANCE_NONE};
    return ClientMaintenanceFor(&release, received_at, (long long)time(NULL));
}

static ClientUpdateStatus ClientUpdateStatusNow(void) {
    pthread_mutex_lock(&client_update.lock);
    bool answered = client_update.answered;
    bool refused = client_update.refused;
    ClientRelease release = client_update.release;
    pthread_mutex_unlock(&client_update.lock);
    if (refused) return CLIENT_UPDATE_REQUIRED;
    return answered ? ClientUpdateStatusFor(HOD_VERSION, &release) : CLIENT_UPDATE_CURRENT;
}

static bool ClientUpdateOnlineRefused(void) {
    pthread_mutex_lock(&client_update.lock);
    bool refused = client_update.refused;
    pthread_mutex_unlock(&client_update.lock);
    return refused;
}

static void ClientUpdateOpenDownload(void) {
    ClientRelease release = ClientUpdateCurrentRelease();
    const char *url =
        ClientUpdateSafeUrl(release.download_url) ? release.download_url : CLIENT_UPDATE_DEFAULT_DOWNLOAD_URL;
    TraceLog(LOG_INFO, "Update: opening %s", url);
    OpenURL(url);
}

static bool ClientUpdateOffered(void) {
    if (ClientUpdateStatusNow() != CLIENT_UPDATE_AVAILABLE) return false;
    ClientRelease release = ClientUpdateCurrentRelease();
    return strcmp(release.latest, SettingsSkippedUpdate()) != 0;
}

static void ClientUpdateSkipOffered(void) {
    SettingsSetSkippedUpdate(ClientUpdateCurrentRelease().latest);
}
