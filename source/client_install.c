#define CLIENT_INSTALL_DIRECTORY "client"
#define CLIENT_INSTALL_STATE_FILE "state.json"
#define CLIENT_INSTALL_DOWNLOADS "downloads"
#define CLIENT_INSTALL_MANIFEST_FILE "client-update.json"
#define CLIENT_INSTALL_SIGNATURE_FILE CLIENT_INSTALL_MANIFEST_FILE ".sig"
#define CLIENT_INSTALL_ARCHIVE_EXTENSION ".zip"
#define CLIENT_INSTALL_UNFINISHED_SUFFIX ".unfinished"
#define CLIENT_INSTALL_MANIFEST_FORMAT 1
#define CLIENT_INSTALL_STATE_FORMAT 1
#define CLIENT_INSTALL_LAUNCH_ATTEMPTS 2
#define CLIENT_INSTALL_DOWNLOAD_ATTEMPTS 3
#define CLIENT_INSTALL_MAX_SIGNED_FILE_BYTES 65536LL
#define CLIENT_INSTALL_MAX_ARCHIVE_BYTES (256LL * 1024 * 1024)
#define CLIENT_INSTALL_MAX_UNPACKED_BYTES (512LL * 1024 * 1024)
#define CLIENT_INSTALL_FILE_CAP 128
#define CLIENT_INSTALL_FILE_PUNCTUATION "-_."
#define CLIENT_INSTALL_PLATFORM_CAP 16
#define CLIENT_INSTALL_KEY_HEX_CAP (RELEASE_PUBLIC_KEY_BYTES * 2 + 1)
#define CLIENT_INSTALL_HEX_BASE 16
#define CLIENT_INSTALL_APPLE_DOUBLE_PREFIX "._"
#define CLIENT_INSTALL_APPLE_DOUBLE_DIRECTORY "__MACOSX/"
#define CLIENT_INSTALL_OPT_OUT_ENV "HOD_NO_SELF_UPDATE"
#define CLIENT_INSTALL_RELEASE_URL_ENV "HOD_RELEASE_URL"
#ifndef HOD_RELEASE_URL
#define HOD_RELEASE_URL ""
#endif
#ifndef HOD_RELEASE_PUBLIC_KEY
#define HOD_RELEASE_PUBLIC_KEY ""
#endif

#if defined(_WIN32) && defined(__x86_64__)
#define CLIENT_INSTALL_PLATFORM "windows"
#elif defined(__APPLE__)
#define CLIENT_INSTALL_PLATFORM "macos"
#elif defined(__linux__) && defined(__x86_64__)
#define CLIENT_INSTALL_PLATFORM "linux"
#else
#define CLIENT_INSTALL_PLATFORM ""
#endif

typedef enum {
    CLIENT_INSTALL_IDLE,
    CLIENT_INSTALL_CHECKING,
    CLIENT_INSTALL_DOWNLOADING,
    CLIENT_INSTALL_UNPACKING,
    CLIENT_INSTALL_READY,
    CLIENT_INSTALL_FAILED,
} ClientInstallPhase;

typedef enum {
    CLIENT_INSTALL_ERROR_NONE,
    CLIENT_INSTALL_ERROR_DOWNLOAD,
    CLIENT_INSTALL_ERROR_SIGNATURE,
    CLIENT_INSTALL_ERROR_MANIFEST,
    CLIENT_INSTALL_ERROR_CORRUPT,
    CLIENT_INSTALL_ERROR_DISK,
    CLIENT_INSTALL_ERROR_CANCELLED,
} ClientInstallError;

typedef enum {
    CLIENT_LAUNCH_SELF,
    CLIENT_LAUNCH_INSTALLED,
    CLIENT_LAUNCH_GIVE_UP_ON_INSTALLED,
} ClientLaunchChoice;

typedef struct {
    ClientInstallPhase phase;
    ClientInstallError error;
    long long total_bytes;
    long long received_bytes;
    char version[CLIENT_UPDATE_VERSION_CAP];
} ClientInstallProgress;

typedef struct {
    char file[CLIENT_INSTALL_FILE_CAP];
    char sha256[SHA256_HEX_CAP];
    long long size;
    char executable[CONTENT_PATH_CAP];
} ClientBuild;

typedef struct {
    char version[CLIENT_UPDATE_VERSION_CAP];
    char executable[CONTENT_PATH_CAP];
    int attempts;
    char rejected[CLIENT_UPDATE_VERSION_CAP];
} ClientInstallState;

typedef struct {
    char root[CONTENT_PATH_CAP];
    char release_url[CONTENT_URL_CAP];
    char public_key[CLIENT_INSTALL_KEY_HEX_CAP];
    char platform[CLIENT_INSTALL_PLATFORM_CAP];
    char running[CLIENT_UPDATE_VERSION_CAP];
    char version[CLIENT_UPDATE_VERSION_CAP];
} ClientInstallPlan;

typedef struct {
    pthread_mutex_t lock;
    bool working;
    bool cancel;
    ClientInstallPlan plan;
    ClientInstallProgress progress;
    char failed[CLIENT_UPDATE_VERSION_CAP];
} ClientInstallJob;

typedef struct {
    ClientInstallJob *job;
    long long limit;
    bool oversized;
} ClientInstallTransfer;

typedef struct {
    bool supported;
    char root[CONTENT_PATH_CAP];
    char rejected[CLIENT_UPDATE_VERSION_CAP];
} ClientInstallSite;

static ClientInstallJob client_install = {.lock = PTHREAD_MUTEX_INITIALIZER};
static ClientInstallSite client_install_site;

static int ClientInstallHexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool ClientInstallReadHex(const char *hex, unsigned char *out, size_t count) {
    for (size_t i = 0; i < count; i++) {
        int high = ClientInstallHexDigit(hex[i * 2]);
        int low = high < 0 ? -1 : ClientInstallHexDigit(hex[i * 2 + 1]);
        if (low < 0) return false;
        out[i] = (unsigned char)(high * CLIENT_INSTALL_HEX_BASE + low);
    }
    const char *rest = hex + count * 2;
    while (isspace((unsigned char)*rest)) rest++;
    return *rest == '\0';
}

static bool ClientInstallKeyConfigured(const char *public_key_hex) {
    unsigned char key[RELEASE_PUBLIC_KEY_BYTES];
    return ClientInstallReadHex(public_key_hex, key, sizeof(key));
}

static bool ClientInstallManifestSigned(
    const unsigned char *manifest, size_t length, const char *signature_hex, const char *public_key_hex
) {
    unsigned char signature[RELEASE_SIGNATURE_BYTES], key[RELEASE_PUBLIC_KEY_BYTES];
    return ClientInstallReadHex(signature_hex, signature, sizeof(signature)) &&
           ClientInstallReadHex(public_key_hex, key, sizeof(key)) &&
           ReleaseSignatureValid(manifest, length, signature, key);
}

static bool ClientInstallFileNameSafe(const char *name) {
    if (!name[0] || name[0] == '.' || strstr(name, "..")) return false;
    for (const char *c = name; *c; c++)
        if (!isalnum((unsigned char)*c) && !strchr(CLIENT_INSTALL_FILE_PUNCTUATION, *c)) return false;
    return true;
}

static bool ClientInstallReleaseBuild(const char *version) {
    ClientVersion parsed;
    return ClientVersionParse(version, &parsed) && parsed.ahead == 0 && !strstr(version, CLIENT_VERSION_DIRTY_SUFFIX);
}

static bool ClientInstallReadBuild(const cJSON *item, ClientBuild *build) {
    *build = (ClientBuild){0};
    const cJSON *size = cJSON_GetObjectItemCaseSensitive(item, "size");
    bool read = ContentCopyField(item, "file", build->file, sizeof(build->file)) &&
                ContentCopyField(item, "sha256", build->sha256, sizeof(build->sha256)) &&
                ContentCopyField(item, "executable", build->executable, sizeof(build->executable)) &&
                cJSON_IsNumber(size) && size->valuedouble > 0 &&
                size->valuedouble <= (double)CLIENT_INSTALL_MAX_ARCHIVE_BYTES;
    if (!read) return false;
    build->size = (long long)size->valuedouble;
    return ClientInstallFileNameSafe(build->file) && ContentDigestValid(build->sha256) &&
           ContentRelativePathSafe(build->executable);
}

static bool ClientInstallManifestBuild(const char *manifest, const ClientInstallPlan *plan, ClientBuild *build) {
    cJSON *root = cJSON_Parse(manifest);
    bool wanted = cJSON_IsObject(root) && JsonInt(root, "format") == CLIENT_INSTALL_MANIFEST_FORMAT &&
                  strcmp(JsonStr(root, "version"), plan->version) == 0 && ClientInstallReleaseBuild(plan->version) &&
                  ClientVersionOlder(plan->running, plan->version);
    const cJSON *builds = wanted ? cJSON_GetObjectItemCaseSensitive(root, "builds") : NULL;
    bool found = false;
    const cJSON *item;
    cJSON_ArrayForEach(item, builds) {
        if (strcmp(JsonStr(item, "platform"), plan->platform) != 0) continue;
        found = ClientInstallReadBuild(item, build);
        break;
    }
    cJSON_Delete(root);
    return found;
}

static const char *ClientInstallErrorName(ClientInstallError error) {
    switch (error) {
        case CLIENT_INSTALL_ERROR_NONE:
            return "none";
        case CLIENT_INSTALL_ERROR_DOWNLOAD:
            return "download failed";
        case CLIENT_INSTALL_ERROR_SIGNATURE:
            return "manifest is not signed by the release key";
        case CLIENT_INSTALL_ERROR_MANIFEST:
            return "manifest does not offer this build";
        case CLIENT_INSTALL_ERROR_CORRUPT:
            return "damaged build";
        case CLIENT_INSTALL_ERROR_DISK:
            return "disk write failed";
        case CLIENT_INSTALL_ERROR_CANCELLED:
            return "cancelled";
    }
    return "unknown";
}

static ClientInstallError ClientInstallDownloadError(PlatformDownloadResult result) {
    switch (result) {
        case PLATFORM_DOWNLOAD_OK:
            return CLIENT_INSTALL_ERROR_NONE;
        case PLATFORM_DOWNLOAD_FILE_ERROR:
            return CLIENT_INSTALL_ERROR_DISK;
        case PLATFORM_DOWNLOAD_CANCELLED:
            return CLIENT_INSTALL_ERROR_CANCELLED;
        case PLATFORM_DOWNLOAD_UNAVAILABLE:
        case PLATFORM_DOWNLOAD_NETWORK_ERROR:
        case PLATFORM_DOWNLOAD_SERVER_ERROR:
            break;
    }
    return CLIENT_INSTALL_ERROR_DOWNLOAD;
}

static bool ClientInstallCancelled(ClientInstallJob *job) {
    pthread_mutex_lock(&job->lock);
    bool cancelled = job->cancel;
    pthread_mutex_unlock(&job->lock);
    return cancelled;
}

static bool ClientInstallTransferProgress(void *context, long long file_bytes) {
    ClientInstallTransfer *transfer = context;
    if (file_bytes > transfer->limit) {
        transfer->oversized = true;
        return false;
    }
    pthread_mutex_lock(&transfer->job->lock);
    transfer->job->progress.received_bytes = file_bytes;
    bool cancelled = transfer->job->cancel;
    pthread_mutex_unlock(&transfer->job->lock);
    return !cancelled;
}

static ClientInstallError
ClientInstallDownload(ClientInstallJob *job, const char *url, const char *path, long long limit, bool resume) {
    ClientInstallTransfer transfer = {.job = job, .limit = limit};
    ClientInstallError error =
        ClientInstallDownloadError(PlatformDownload(url, path, resume, ClientInstallTransferProgress, &transfer));
    if (!transfer.oversized && error == CLIENT_INSTALL_ERROR_NONE && GetFileLength(path) > limit)
        transfer.oversized = true;
    if (!transfer.oversized) return error;
    remove(path);
    return CLIENT_INSTALL_ERROR_CORRUPT;
}

static bool ClientInstallJoin(char *out, size_t capacity, const char *directory, const char *name) {
    return (size_t)snprintf(out, capacity, "%s/%s", directory, name) < capacity;
}

static ClientInstallError ClientInstallReadSignedBuild(
    const ClientInstallPlan *plan, const char *manifest_path, const char *signature_path, ClientBuild *build
) {
    int length = 0;
    unsigned char *manifest = FileExists(manifest_path) ? LoadFileData(manifest_path, &length) : NULL;
    char *signature = FileExists(signature_path) ? LoadFileText(signature_path) : NULL;
    char *text = manifest ? calloc((size_t)length + 1, 1) : NULL;
    ClientInstallError error = CLIENT_INSTALL_ERROR_NONE;
    if (!text || !signature || !ClientInstallManifestSigned(manifest, (size_t)length, signature, plan->public_key)) {
        error = CLIENT_INSTALL_ERROR_SIGNATURE;
    } else {
        memcpy(text, manifest, (size_t)length);
        if (!ClientInstallManifestBuild(text, plan, build)) error = CLIENT_INSTALL_ERROR_MANIFEST;
    }
    free(text);
    if (signature) UnloadFileText(signature);
    if (manifest) UnloadFileData(manifest);
    return error;
}

static ClientInstallError ClientInstallFetchBuild(
    ClientInstallJob *job, const ClientInstallPlan *plan, const char *release, const char *downloads, ClientBuild *build
) {
    char manifest_url[CONTENT_URL_CAP], signature_url[CONTENT_URL_CAP];
    char manifest_path[CONTENT_PATH_CAP], signature_path[CONTENT_PATH_CAP];
    ContentJoinUrl(release, CLIENT_INSTALL_MANIFEST_FILE, manifest_url, sizeof(manifest_url));
    ContentJoinUrl(release, CLIENT_INSTALL_SIGNATURE_FILE, signature_url, sizeof(signature_url));
    if (!ClientInstallJoin(manifest_path, sizeof(manifest_path), downloads, CLIENT_INSTALL_MANIFEST_FILE) ||
        !ClientInstallJoin(signature_path, sizeof(signature_path), downloads, CLIENT_INSTALL_SIGNATURE_FILE))
        return CLIENT_INSTALL_ERROR_DISK;
    long long limit = CLIENT_INSTALL_MAX_SIGNED_FILE_BYTES;
    ClientInstallError error = ClientInstallDownload(job, manifest_url, manifest_path, limit, false);
    if (error == CLIENT_INSTALL_ERROR_NONE)
        error = ClientInstallDownload(job, signature_url, signature_path, limit, false);
    if (error == CLIENT_INSTALL_ERROR_NONE)
        error = ClientInstallReadSignedBuild(plan, manifest_path, signature_path, build);
    remove(manifest_path);
    remove(signature_path);
    return error;
}

static bool ClientInstallArchiveMatches(const char *path, const ClientBuild *build) {
    long long size = 0;
    char hex[SHA256_HEX_CAP];
    return ContentFileDigest(path, &size, hex) && size == build->size && strcmp(hex, build->sha256) == 0;
}

static ClientInstallError
ClientInstallFetchArchive(ClientInstallJob *job, const char *release, const ClientBuild *build, const char *path) {
    char url[CONTENT_URL_CAP];
    ContentJoinUrl(release, build->file, url, sizeof(url));
    ClientInstallError error = CLIENT_INSTALL_ERROR_NONE;
    for (int attempt = 0; attempt < CLIENT_INSTALL_DOWNLOAD_ATTEMPTS; attempt++) {
        error = ClientInstallDownload(job, url, path, build->size, true);
        if (error == CLIENT_INSTALL_ERROR_NONE) {
            if (ClientInstallArchiveMatches(path, build)) return CLIENT_INSTALL_ERROR_NONE;
            remove(path);
            error = CLIENT_INSTALL_ERROR_CORRUPT;
        }
        bool retryable = error == CLIENT_INSTALL_ERROR_DOWNLOAD || error == CLIENT_INSTALL_ERROR_CORRUPT;
        if (!retryable || ClientInstallCancelled(job)) break;
    }
    return error;
}

static bool ClientInstallEntrySkipped(const char *name) {
    size_t length = strlen(name);
    if (length == 0 || name[length - 1] == '/') return true;
    if (strncmp(name, CLIENT_INSTALL_APPLE_DOUBLE_DIRECTORY, strlen(CLIENT_INSTALL_APPLE_DOUBLE_DIRECTORY)) == 0)
        return true;
    const char *separator = strrchr(name, '/');
    const char *base = separator ? separator + 1 : name;
    return strncmp(base, CLIENT_INSTALL_APPLE_DOUBLE_PREFIX, strlen(CLIENT_INSTALL_APPLE_DOUBLE_PREFIX)) == 0;
}

static ClientInstallError
ClientInstallUnpack(const char *archive_path, const char *directory, const ClientBuild *build) {
    ApkArchive archive;
    if (!ApkOpen(&archive, archive_path)) return CLIENT_INSTALL_ERROR_CORRUPT;
    ClientInstallError error = CLIENT_INSTALL_ERROR_NONE;
    long long unpacked = 0;
    bool executable_found = false;
    for (int i = 0; i < archive.count && error == CLIENT_INSTALL_ERROR_NONE; i++) {
        const ApkEntry *entry = &archive.entries[i];
        if (ClientInstallEntrySkipped(entry->name)) continue;
        unpacked += entry->size;
        char path[CONTENT_PATH_CAP];
        bool placed = ContentRelativePathSafe(entry->name) && unpacked <= CLIENT_INSTALL_MAX_UNPACKED_BYTES &&
                      ClientInstallJoin(path, sizeof(path) - strlen(CONTENT_PARTIAL_SUFFIX), directory, entry->name);
        unsigned int size = 0;
        unsigned char *data = placed ? ApkReadEntry(&archive, entry, &size) : NULL;
        if (!data) {
            error = CLIENT_INSTALL_ERROR_CORRUPT;
        } else if (!ContentWriteFile(path, data, size)) {
            error = CLIENT_INSTALL_ERROR_DISK;
        }
        MemFree(data);
        executable_found = executable_found || strcmp(entry->name, build->executable) == 0;
    }
    ApkClose(&archive);
    if (error != CLIENT_INSTALL_ERROR_NONE) return error;
    char executable[CONTENT_PATH_CAP];
    if (!executable_found || !ClientInstallJoin(executable, sizeof(executable), directory, build->executable))
        return CLIENT_INSTALL_ERROR_CORRUPT;
    return PlatformMakeExecutable(executable) ? CLIENT_INSTALL_ERROR_NONE : CLIENT_INSTALL_ERROR_DISK;
}

static ClientInstallError
ClientInstallPlace(const ClientInstallPlan *plan, const ClientBuild *build, const char *archive_path) {
    char placed[CONTENT_PATH_CAP], unfinished[CONTENT_PATH_CAP];
    if (!ClientInstallJoin(placed, sizeof(placed), plan->root, plan->version) ||
        (size_t)snprintf(unfinished, sizeof(unfinished), "%s%s", placed, CLIENT_INSTALL_UNFINISHED_SUFFIX) >=
            sizeof(unfinished))
        return CLIENT_INSTALL_ERROR_DISK;
    if (!PlatformRemoveTree(unfinished) || !ContentMakeDirectories(unfinished)) return CLIENT_INSTALL_ERROR_DISK;
    ClientInstallError error = ClientInstallUnpack(archive_path, unfinished, build);
    if (error == CLIENT_INSTALL_ERROR_NONE && !(PlatformRemoveTree(placed) && rename(unfinished, placed) == 0))
        error = CLIENT_INSTALL_ERROR_DISK;
    if (error != CLIENT_INSTALL_ERROR_NONE) PlatformRemoveTree(unfinished);
    return error;
}

static bool ClientInstallStateLoad(const char *root, ClientInstallState *state) {
    *state = (ClientInstallState){0};
    char path[CONTENT_PATH_CAP];
    if (!ClientInstallJoin(path, sizeof(path), root, CLIENT_INSTALL_STATE_FILE)) return false;
    cJSON *json = ContentLoadJson(path);
    bool readable = cJSON_IsObject(json) && JsonInt(json, "format") == CLIENT_INSTALL_STATE_FORMAT;
    if (readable) {
        const char *executable = JsonStr(json, "executable");
        ClientReleaseCopyVersion(state->rejected, sizeof(state->rejected), JsonStr(json, "rejected"));
        if (ContentRelativePathSafe(executable)) {
            ClientReleaseCopyVersion(state->version, sizeof(state->version), JsonStr(json, "version"));
            StrCopy(state->executable, sizeof(state->executable), executable);
            state->attempts = JsonInt(json, "attempts");
        }
    }
    cJSON_Delete(json);
    return readable;
}

static bool ClientInstallStateSave(const char *root, const ClientInstallState *state) {
    char path[CONTENT_PATH_CAP];
    if (!ClientInstallJoin(path, sizeof(path), root, CLIENT_INSTALL_STATE_FILE)) return false;
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "format", CLIENT_INSTALL_STATE_FORMAT);
    cJSON_AddStringToObject(json, "version", state->version);
    cJSON_AddStringToObject(json, "executable", state->executable);
    cJSON_AddNumberToObject(json, "attempts", state->attempts);
    cJSON_AddStringToObject(json, "rejected", state->rejected);
    char *text = cJSON_PrintUnformatted(json);
    bool saved = text && ContentWriteFile(path, text, strlen(text));
    cJSON_free(text);
    cJSON_Delete(json);
    return saved;
}

static bool ClientInstallRecord(const ClientInstallPlan *plan, const ClientBuild *build) {
    ClientInstallState state;
    ClientInstallStateLoad(plan->root, &state);
    StrCopy(state.version, sizeof(state.version), plan->version);
    state.attempts = 0;
    return ClientInstallJoin(state.executable, sizeof(state.executable), plan->version, build->executable) &&
           ClientInstallStateSave(plan->root, &state);
}

static void ClientInstallEnter(ClientInstallJob *job, ClientInstallPhase phase, long long total_bytes) {
    pthread_mutex_lock(&job->lock);
    job->progress.phase = phase;
    job->progress.total_bytes = total_bytes;
    job->progress.received_bytes = 0;
    pthread_mutex_unlock(&job->lock);
}

static ClientInstallError ClientInstallApply(ClientInstallJob *job, const ClientInstallPlan *plan) {
    char release[CONTENT_URL_CAP], downloads[CONTENT_PATH_CAP], archive[CONTENT_PATH_CAP];
    ContentJoinUrl(plan->release_url, plan->version, release, sizeof(release));
    if (!ClientInstallJoin(downloads, sizeof(downloads), plan->root, CLIENT_INSTALL_DOWNLOADS) ||
        !ContentMakeDirectories(downloads))
        return CLIENT_INSTALL_ERROR_DISK;
    ClientBuild build;
    ClientInstallError error = ClientInstallFetchBuild(job, plan, release, downloads, &build);
    if (error != CLIENT_INSTALL_ERROR_NONE) return error;
    if ((size_t)snprintf(
            archive, sizeof(archive), "%s/%s%s", downloads, build.sha256, CLIENT_INSTALL_ARCHIVE_EXTENSION
        ) >= sizeof(archive))
        return CLIENT_INSTALL_ERROR_DISK;
    ClientInstallEnter(job, CLIENT_INSTALL_DOWNLOADING, build.size);
    error = ClientInstallFetchArchive(job, release, &build, archive);
    if (error != CLIENT_INSTALL_ERROR_NONE) return error;
    ClientInstallEnter(job, CLIENT_INSTALL_UNPACKING, 0);
    error = ClientInstallPlace(plan, &build, archive);
    remove(archive);
    if (error != CLIENT_INSTALL_ERROR_NONE) return error;
    if (ClientInstallCancelled(job)) return CLIENT_INSTALL_ERROR_CANCELLED;
    return ClientInstallRecord(plan, &build) ? CLIENT_INSTALL_ERROR_NONE : CLIENT_INSTALL_ERROR_DISK;
}

static ClientInstallPhase ClientInstallPhaseAfter(ClientInstallError error) {
    if (error == CLIENT_INSTALL_ERROR_NONE) return CLIENT_INSTALL_READY;
    return error == CLIENT_INSTALL_ERROR_CANCELLED ? CLIENT_INSTALL_IDLE : CLIENT_INSTALL_FAILED;
}

static void ClientInstallExecute(ClientInstallJob *job) {
    pthread_mutex_lock(&job->lock);
    ClientInstallPlan plan = job->plan;
    pthread_mutex_unlock(&job->lock);
    ClientInstallError error = ClientInstallApply(job, &plan);
    if (error == CLIENT_INSTALL_ERROR_NONE)
        TraceLog(LOG_INFO, "Update: %s is installed", plan.version);
    else
        TraceLog(LOG_WARNING, "Update: installing %s failed (%s)", plan.version, ClientInstallErrorName(error));
    pthread_mutex_lock(&job->lock);
    job->progress.error = error;
    job->progress.phase = ClientInstallPhaseAfter(error);
    if (job->progress.phase == CLIENT_INSTALL_FAILED) StrCopy(job->failed, sizeof(job->failed), plan.version);
    job->working = false;
    job->cancel = false;
    pthread_mutex_unlock(&job->lock);
}

static void *ClientInstallThread(void *argument) {
    ClientInstallExecute(argument);
    return NULL;
}

static ClientLaunchChoice ClientInstallLaunchChoice(const ClientInstallState *state, const char *running) {
    if (!state->version[0] || !ClientVersionOlder(running, state->version)) return CLIENT_LAUNCH_SELF;
    return state->attempts >= CLIENT_INSTALL_LAUNCH_ATTEMPTS ? CLIENT_LAUNCH_GIVE_UP_ON_INSTALLED
                                                             : CLIENT_LAUNCH_INSTALLED;
}

static void ClientInstallForget(const char *root, ClientInstallState *state) {
    state->version[0] = '\0';
    state->executable[0] = '\0';
    state->attempts = 0;
    ClientInstallStateSave(root, state);
}

static void ClientInstallGiveUpOn(const char *root, ClientInstallState *state) {
    StrCopy(state->rejected, sizeof(state->rejected), state->version);
    ClientInstallForget(root, state);
}

static void ClientInstallSettle(const char *root, ClientInstallState *state, const char *running) {
    if (!state->version[0]) return;
    bool running_installed = strcmp(state->version, running) == 0;
    if (running_installed && state->attempts > 0) {
        state->attempts = 0;
        ClientInstallStateSave(root, state);
    }
    if (!running_installed && !ClientVersionOlder(running, state->version)) ClientInstallForget(root, state);
}

static void ClientInstallRemoveStale(const char *root, const ClientInstallState *state) {
    if (!DirectoryExists(root)) return;
    FilePathList entries = LoadDirectoryFiles(root);
    for (unsigned int i = 0; i < entries.count; i++) {
        const char *name = GetFileName(entries.paths[i]);
        bool kept =
            strcmp(name, CLIENT_INSTALL_STATE_FILE) == 0 || (state->version[0] && strcmp(name, state->version) == 0);
        if (!kept) PlatformRemoveTree(entries.paths[i]);
    }
    UnloadDirectoryFiles(entries);
}

static bool ClientInstallExecutablePath(const char *root, const ClientInstallState *state, char *out, size_t capacity) {
    return state->version[0] && ClientInstallJoin(out, capacity, root, state->executable) && FileExists(out);
}

static bool ClientInstallSupportedHere(void) {
    return strlen(CLIENT_INSTALL_PLATFORM) > 0 && !EnvFlag(CLIENT_INSTALL_OPT_OUT_ENV) && !ContentUsesLocalFiles() &&
           ClientInstallKeyConfigured(HOD_RELEASE_PUBLIC_KEY) && ClientInstallReleaseBuild(HOD_VERSION);
}

static bool ClientInstallPathAbsolute(const char *path) {
    return path[0] == '/' || path[0] == '\\' || (isalpha((unsigned char)path[0]) && path[1] == ':');
}

static void ClientInstallLocate(ClientInstallSite *site) {
    *site = (ClientInstallSite){0};
    char data[CONTENT_PATH_CAP];
    if (!ClientInstallSupportedHere() || !ContentInstallDirectory(data, sizeof(data))) return;
    int length =
        ClientInstallPathAbsolute(data)
            ? snprintf(site->root, sizeof(site->root), "%s/%s", data, CLIENT_INSTALL_DIRECTORY)
            : snprintf(
                  site->root, sizeof(site->root), "%s/%s/%s", GetWorkingDirectory(), data, CLIENT_INSTALL_DIRECTORY
              );
    site->supported = length > 0 && (size_t)length < sizeof(site->root);
}

static bool ClientInstallContinueAs(const char *executable, bool (*start)(const char *)) {
    char previous[CONTENT_PATH_CAP], directory[CONTENT_PATH_CAP];
    StrCopy(previous, sizeof(previous), GetWorkingDirectory());
    StrCopy(directory, sizeof(directory), GetDirectoryPath(executable));
    if (!ChangeDirectory(directory)) return false;
    if (start(executable)) return true;
    ChangeDirectory(previous);
    return false;
}

static bool ClientInstallHandOver(void) {
    ClientInstallSite *site = &client_install_site;
    ClientInstallLocate(site);
    ClientInstallState state;
    if (!site->supported || !ClientInstallStateLoad(site->root, &state)) return false;
    StrCopy(site->rejected, sizeof(site->rejected), state.rejected);
    ClientLaunchChoice choice = ClientInstallLaunchChoice(&state, HOD_VERSION);
    if (choice == CLIENT_LAUNCH_GIVE_UP_ON_INSTALLED) {
        TraceLog(LOG_WARNING, "Update: %s did not start; staying on %s", state.version, HOD_VERSION);
        ClientInstallGiveUpOn(site->root, &state);
        StrCopy(site->rejected, sizeof(site->rejected), state.rejected);
    }
    if (choice != CLIENT_LAUNCH_INSTALLED) return false;
    char executable[CONTENT_PATH_CAP];
    if (!ClientInstallExecutablePath(site->root, &state, executable, sizeof(executable))) {
        ClientInstallForget(site->root, &state);
        return false;
    }
    state.attempts++;
    ClientInstallStateSave(site->root, &state);
    TraceLog(LOG_INFO, "Update: %s continues as %s", HOD_VERSION, state.version);
    return ClientInstallContinueAs(executable, PlatformContinueAs);
}

static void ClientInstallConfirmLaunch(void) {
    const ClientInstallSite *site = &client_install_site;
    if (!site->supported) return;
    ClientInstallState state;
    if (ClientInstallStateLoad(site->root, &state)) ClientInstallSettle(site->root, &state, HOD_VERSION);
    ClientInstallRemoveStale(site->root, &state);
}

static ClientInstallProgress ClientInstallPoll(void) {
    pthread_mutex_lock(&client_install.lock);
    ClientInstallProgress progress = client_install.progress;
    pthread_mutex_unlock(&client_install.lock);
    return progress;
}

static bool ClientInstallBusy(void) {
    ClientInstallPhase phase = ClientInstallPoll().phase;
    return phase != CLIENT_INSTALL_IDLE && phase != CLIENT_INSTALL_FAILED;
}

static bool ClientInstallOffered(void) {
    const ClientInstallSite *site = &client_install_site;
    if (!site->supported || ClientUpdateStatusNow() == CLIENT_UPDATE_CURRENT) return false;
    ClientRelease release = ClientUpdateCurrentRelease();
    pthread_mutex_lock(&client_install.lock);
    bool failed = strcmp(client_install.failed, release.latest) == 0;
    pthread_mutex_unlock(&client_install.lock);
    return ClientInstallReleaseBuild(release.latest) && !failed && strcmp(site->rejected, release.latest) != 0;
}

static const char *ClientInstallReleaseUrl(void) {
    const char *configured = getenv(CLIENT_INSTALL_RELEASE_URL_ENV);
    return configured && configured[0] ? configured : HOD_RELEASE_URL;
}

static void ClientInstallFail(ClientInstallJob *job) {
    pthread_mutex_lock(&job->lock);
    job->working = false;
    job->progress.phase = CLIENT_INSTALL_FAILED;
    job->progress.error = CLIENT_INSTALL_ERROR_DISK;
    StrCopy(job->failed, sizeof(job->failed), job->plan.version);
    pthread_mutex_unlock(&job->lock);
}

static void ClientInstallStart(void) {
    if (!ClientInstallOffered()) return;
    ClientRelease release = ClientUpdateCurrentRelease();
    ClientInstallJob *job = &client_install;
    pthread_mutex_lock(&job->lock);
    bool startable = !job->working && job->progress.phase != CLIENT_INSTALL_READY;
    if (startable) {
        job->working = true;
        job->cancel = false;
        job->progress = (ClientInstallProgress){.phase = CLIENT_INSTALL_CHECKING};
        StrCopy(job->progress.version, sizeof(job->progress.version), release.latest);
        job->plan = (ClientInstallPlan){0};
        StrCopy(job->plan.root, sizeof(job->plan.root), client_install_site.root);
        StrCopy(job->plan.release_url, sizeof(job->plan.release_url), ClientInstallReleaseUrl());
        StrCopy(job->plan.public_key, sizeof(job->plan.public_key), HOD_RELEASE_PUBLIC_KEY);
        StrCopy(job->plan.platform, sizeof(job->plan.platform), CLIENT_INSTALL_PLATFORM);
        StrCopy(job->plan.running, sizeof(job->plan.running), HOD_VERSION);
        StrCopy(job->plan.version, sizeof(job->plan.version), release.latest);
    }
    pthread_mutex_unlock(&job->lock);
    if (!startable) return;
    TraceLog(LOG_INFO, "Update: installing %s", release.latest);
    pthread_t thread;
    if (pthread_create(&thread, NULL, ClientInstallThread, job) == 0)
        pthread_detach(thread);
    else
        ClientInstallFail(job);
}

static void ClientInstallCancel(void) {
    pthread_mutex_lock(&client_install.lock);
    if (client_install.working) client_install.cancel = true;
    pthread_mutex_unlock(&client_install.lock);
}

static void ClientInstallAcknowledgeFailure(void) {
    pthread_mutex_lock(&client_install.lock);
    if (client_install.progress.phase == CLIENT_INSTALL_FAILED) client_install.progress.phase = CLIENT_INSTALL_IDLE;
    pthread_mutex_unlock(&client_install.lock);
}

static bool ClientInstallRestartPending(void) {
    return ClientInstallPoll().phase == CLIENT_INSTALL_READY;
}

static void ClientInstallRestart(void) {
    if (!ClientInstallRestartPending()) return;
    const ClientInstallSite *site = &client_install_site;
    ClientInstallState state;
    char executable[CONTENT_PATH_CAP];
    bool started = ClientInstallStateLoad(site->root, &state) &&
                   ClientInstallExecutablePath(site->root, &state, executable, sizeof(executable)) &&
                   ClientInstallContinueAs(executable, PlatformStartProgram);
    if (!started) TraceLog(LOG_WARNING, "Update: could not restart into the installed build");
}
