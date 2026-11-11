#define CONTENT_URL_ENV "HOD_CONTENT_URL"
#define CONTENT_DIR_ENV "HOD_CONTENT_DIR"
#ifndef HOD_CONTENT_URL
#define HOD_CONTENT_URL ""
#endif
#define CONTENT_MANIFEST_FILE "manifest.json"
#define CONTENT_MANIFEST_FORMAT 2
#define CONTENT_STATE_PATH "content-state.json"
#define CONTENT_DOWNLOAD_DIR "downloads"
#define CONTENT_MANIFEST_DOWNLOAD CONTENT_DOWNLOAD_DIR "/" CONTENT_MANIFEST_FILE
#define CONTENT_PACK_EXTENSION ".zip"
#define CONTENT_PARTIAL_SUFFIX ".partial"
#define CONTENT_RULES_ROOT "data/"
#define CONTENT_PACK_ATTEMPTS 3
#define CONTENT_URL_CAP 1024
#define CONTENT_PATH_CAP 1024
#define CONTENT_NAME_CAP 64
#define CONTENT_HASH_CHUNK_BYTES 65536

typedef enum {
    CONTENT_PHASE_CHECKING,
    CONTENT_PHASE_DOWNLOADING,
    CONTENT_PHASE_INSTALLING,
    CONTENT_PHASE_READY,
    CONTENT_PHASE_FAILED,
} ContentPhase;

typedef enum {
    CONTENT_ERROR_NONE,
    CONTENT_ERROR_NOT_CONFIGURED,
    CONTENT_ERROR_DOWNLOAD_UNAVAILABLE,
    CONTENT_ERROR_OFFLINE,
    CONTENT_ERROR_SERVER,
    CONTENT_ERROR_CLIENT_TOO_OLD,
    CONTENT_ERROR_CORRUPT,
    CONTENT_ERROR_DISK,
    CONTENT_ERROR_CANCELLED,
} ContentError;

typedef struct {
    ContentPhase phase;
    ContentError error;
    long long total_bytes;
    long long received_bytes;
    int packs_total;
    int packs_done;
    bool first_install;
} ContentProgress;

typedef struct {
    char name[CONTENT_NAME_CAP];
    char file[CONTENT_PATH_CAP];
    char sha256[SHA256_HEX_CAP];
    long long size;
} ContentPack;

typedef struct {
    int format;
    char version[CONTENT_NAME_CAP];
    ContentPack *packs;
    int count;
} ContentManifest;

typedef struct {
    char base_url[CONTENT_URL_CAP];
    pthread_t thread;
    bool started;
    pthread_mutex_t lock;
    ContentProgress progress;
    bool cancel;
} ContentUpdate;

typedef struct {
    ContentUpdate *update;
    ContentProgress *progress;
    long long finished_bytes;
} ContentTransfer;

static const char *ContentBaseUrl(void) {
    const char *configured = getenv(CONTENT_URL_ENV);
    return configured && configured[0] ? configured : HOD_CONTENT_URL;
}

static bool ContentUsesLocalFiles(void) {
    return !getenv(CONTENT_DIR_ENV) && DirectoryExists(HOD_CACHE);
}

static bool ContentIsSeparator(char c) {
    return c == '/' || c == '\\';
}

static bool ContentMakeParents(const char *path) {
    char partial[CONTENT_PATH_CAP];
    size_t length = strlen(path);
    if (length >= sizeof(partial)) return false;
    for (size_t i = 1; i < length; i++) {
        if (!ContentIsSeparator(path[i]) || path[i - 1] == ':' || ContentIsSeparator(path[i - 1])) continue;
        memcpy(partial, path, i);
        partial[i] = '\0';
        if (!PlatformMakeDirectory(partial)) return false;
    }
    return true;
}

static bool ContentMakeDirectories(const char *path) {
    return ContentMakeParents(path) && PlatformMakeDirectory(path);
}

static bool ContentInstallDirectory(char *directory, size_t capacity) {
    const char *forced = getenv(CONTENT_DIR_ENV);
    return forced && forced[0] ? (size_t)snprintf(directory, capacity, "%s", forced) < capacity
                               : PlatformUserDataDirectory(APP_NAME, directory, capacity);
}

static bool ContentEnterInstallDirectory(char *directory, size_t capacity) {
    return ContentInstallDirectory(directory, capacity) && ContentMakeDirectories(directory) &&
           ChangeDirectory(directory);
}

static bool ContentRelativePathSafe(const char *path) {
    if (strchr(path, '\\') || strchr(path, ':')) return false;
    if (strlen(path) + strlen(CONTENT_PARTIAL_SUFFIX) >= CONTENT_PATH_CAP) return false;
    for (const char *segment = path;;) {
        size_t length = strcspn(segment, "/");
        bool dots = (length == 1 && segment[0] == '.') || (length == 2 && segment[0] == '.' && segment[1] == '.');
        if (length == 0 || dots) return false;
        if (segment[length] == '\0') return true;
        segment += length + 1;
    }
}

static bool ContentEntryPathSafe(const char *path) {
    bool rooted = strncmp(path, CONTENT_RULES_ROOT, strlen(CONTENT_RULES_ROOT)) == 0 ||
                  strncmp(path, HOD_CACHE, strlen(HOD_CACHE)) == 0;
    return rooted && ContentRelativePathSafe(path);
}

static bool ContentPackNameSafe(const char *name) {
    if (!name[0]) return false;
    for (const char *c = name; *c; c++)
        if (!isalnum((unsigned char)*c) && *c != '-' && *c != '_') return false;
    return true;
}

static bool ContentDigestValid(const char *hex) {
    if (strlen(hex) != SHA256_HEX_CAP - 1) return false;
    for (const char *c = hex; *c; c++)
        if (!isdigit((unsigned char)*c) && !(*c >= 'a' && *c <= 'f')) return false;
    return true;
}

static bool ContentFileDigest(const char *path, long long *size, char *hex) {
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    unsigned char *buffer = malloc(CONTENT_HASH_CHUNK_BYTES);
    Sha256 hash;
    Sha256Begin(&hash);
    *size = 0;
    size_t read = 0;
    while (buffer && (read = fread(buffer, 1, CONTENT_HASH_CHUNK_BYTES, file)) > 0) {
        Sha256Update(&hash, buffer, read);
        *size += (long long)read;
    }
    bool complete = buffer && !ferror(file);
    free(buffer);
    fclose(file);
    Sha256FinishHex(&hash, hex);
    return complete;
}

static bool ContentFileMatches(const char *path, const ContentPack *pack) {
    long long size = 0;
    char hex[SHA256_HEX_CAP];
    return ContentFileDigest(path, &size, hex) && size == pack->size && strcmp(hex, pack->sha256) == 0;
}

static bool ContentWriteFile(const char *path, const void *data, size_t size) {
    char partial[CONTENT_PATH_CAP];
    if ((size_t)snprintf(partial, sizeof(partial), "%s%s", path, CONTENT_PARTIAL_SUFFIX) >= sizeof(partial))
        return false;
    if (!ContentMakeParents(path)) return false;
    FILE *file = fopen(partial, "wb");
    if (!file) return false;
    bool written = fwrite(data, 1, size, file) == size;
    if (fclose(file) != 0) written = false;
    if (written && PlatformReplaceFile(partial, path)) return true;
    remove(partial);
    return false;
}

static cJSON *ContentLoadJson(const char *path) {
    if (!FileExists(path)) return NULL;
    char *text = LoadFileText(path);
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    UnloadFileText(text);
    return root;
}

static bool ContentCopyField(const cJSON *object, const char *key, char *out, size_t capacity) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsString(item) || strlen(item->valuestring) >= capacity) return false;
    memcpy(out, item->valuestring, strlen(item->valuestring) + 1);
    return true;
}

static void ContentManifestFree(ContentManifest *manifest) {
    free(manifest->packs);
    *manifest = (ContentManifest){0};
}

static bool ContentManifestParse(const cJSON *root, ContentManifest *manifest) {
    *manifest = (ContentManifest){0};
    const cJSON *format = cJSON_GetObjectItemCaseSensitive(root, "format");
    if (!cJSON_IsNumber(format)) return false;
    manifest->format = format->valueint;
    if (manifest->format != CONTENT_MANIFEST_FORMAT) return true;
    const cJSON *packs = cJSON_GetObjectItemCaseSensitive(root, "packs");
    if (!ContentCopyField(root, "version", manifest->version, sizeof(manifest->version)) || !cJSON_IsArray(packs))
        return false;
    int count = cJSON_GetArraySize(packs);
    manifest->packs = calloc(count > 0 ? (size_t)count : 1, sizeof(*manifest->packs));
    const cJSON *item;
    cJSON_ArrayForEach(item, packs) {
        ContentPack *pack = &manifest->packs[manifest->count];
        const cJSON *size = cJSON_GetObjectItemCaseSensitive(item, "size");
        bool valid = ContentCopyField(item, "name", pack->name, sizeof(pack->name)) &&
                     ContentCopyField(item, "file", pack->file, sizeof(pack->file)) &&
                     ContentCopyField(item, "sha256", pack->sha256, sizeof(pack->sha256)) && cJSON_IsNumber(size) &&
                     size->valuedouble > 0 && ContentPackNameSafe(pack->name) && ContentDigestValid(pack->sha256) &&
                     !strstr(pack->file, "..");
        if (!valid) {
            ContentManifestFree(manifest);
            return false;
        }
        pack->size = (long long)size->valuedouble;
        manifest->count++;
    }
    return true;
}

static void ContentJoinUrl(const char *base, const char *file, char *out, size_t capacity) {
    size_t base_length = strlen(base);
    while (base_length > 0 && base[base_length - 1] == '/') base_length--;
    snprintf(out, capacity, "%.*s/%s", (int)base_length, base, file);
}

static void ContentPackDownloadPath(const ContentPack *pack, char *out, size_t capacity) {
    snprintf(out, capacity, "%s/%s%s", CONTENT_DOWNLOAD_DIR, pack->sha256, CONTENT_PACK_EXTENSION);
}

static void ContentPublish(ContentUpdate *update, const ContentProgress *progress) {
    pthread_mutex_lock(&update->lock);
    update->progress = *progress;
    pthread_mutex_unlock(&update->lock);
}

static bool ContentCancelled(ContentUpdate *update) {
    pthread_mutex_lock(&update->lock);
    bool cancelled = update->cancel;
    pthread_mutex_unlock(&update->lock);
    return cancelled;
}

static bool ContentTransferProgress(void *context, long long file_bytes) {
    ContentTransfer *transfer = context;
    transfer->progress->received_bytes = transfer->finished_bytes + file_bytes;
    ContentPublish(transfer->update, transfer->progress);
    return !ContentCancelled(transfer->update);
}

static ContentError ContentDownloadError(PlatformDownloadResult result) {
    switch (result) {
        case PLATFORM_DOWNLOAD_OK:
            return CONTENT_ERROR_NONE;
        case PLATFORM_DOWNLOAD_UNAVAILABLE:
            return CONTENT_ERROR_DOWNLOAD_UNAVAILABLE;
        case PLATFORM_DOWNLOAD_SERVER_ERROR:
            return CONTENT_ERROR_SERVER;
        case PLATFORM_DOWNLOAD_FILE_ERROR:
            return CONTENT_ERROR_DISK;
        case PLATFORM_DOWNLOAD_CANCELLED:
            return CONTENT_ERROR_CANCELLED;
        case PLATFORM_DOWNLOAD_NETWORK_ERROR:
            break;
    }
    return CONTENT_ERROR_OFFLINE;
}

static ContentError ContentFetchManifest(ContentUpdate *update, ContentManifest *manifest) {
    *manifest = (ContentManifest){0};
    if (!update->base_url[0]) return CONTENT_ERROR_NOT_CONFIGURED;
    if (!ContentMakeDirectories(CONTENT_DOWNLOAD_DIR)) return CONTENT_ERROR_DISK;
    char url[CONTENT_URL_CAP];
    ContentJoinUrl(update->base_url, CONTENT_MANIFEST_FILE, url, sizeof(url));
    ContentError error = ContentDownloadError(PlatformDownload(url, CONTENT_MANIFEST_DOWNLOAD, false, NULL, NULL));
    if (error != CONTENT_ERROR_NONE) return error;
    cJSON *root = ContentLoadJson(CONTENT_MANIFEST_DOWNLOAD);
    bool parsed = root && ContentManifestParse(root, manifest);
    cJSON_Delete(root);
    remove(CONTENT_MANIFEST_DOWNLOAD);
    if (!parsed || manifest->format < CONTENT_MANIFEST_FORMAT) return CONTENT_ERROR_SERVER;
    return manifest->format == CONTENT_MANIFEST_FORMAT ? CONTENT_ERROR_NONE : CONTENT_ERROR_CLIENT_TOO_OLD;
}

static ContentError ContentFetchPack(
    ContentUpdate *update,
    ContentProgress *progress,
    long long finished_bytes,
    const ContentPack *pack,
    const char *path
) {
    char url[CONTENT_URL_CAP];
    ContentJoinUrl(update->base_url, pack->file, url, sizeof(url));
    ContentError error = CONTENT_ERROR_NONE;
    for (int attempt = 0; attempt < CONTENT_PACK_ATTEMPTS; attempt++) {
        ContentTransfer transfer = {.update = update, .progress = progress, .finished_bytes = finished_bytes};
        error = ContentDownloadError(PlatformDownload(url, path, true, ContentTransferProgress, &transfer));
        if (error == CONTENT_ERROR_NONE) {
            if (ContentFileMatches(path, pack)) return CONTENT_ERROR_NONE;
            remove(path);
            error = CONTENT_ERROR_CORRUPT;
        }
        bool retryable =
            error == CONTENT_ERROR_OFFLINE || error == CONTENT_ERROR_SERVER || error == CONTENT_ERROR_CORRUPT;
        if (!retryable || ContentCancelled(update)) break;
    }
    return error;
}

static ContentError ContentExtractPack(const char *path, cJSON **installed) {
    ApkArchive archive;
    if (!ApkOpen(&archive, path)) return CONTENT_ERROR_CORRUPT;
    for (int i = 0; i < archive.count; i++) {
        if (!ContentEntryPathSafe(archive.entries[i].name)) {
            TraceLog(LOG_WARNING, "Content: rejected pack entry %s", archive.entries[i].name);
            ApkClose(&archive);
            return CONTENT_ERROR_CORRUPT;
        }
    }
    cJSON *files = cJSON_CreateArray();
    ContentError error = CONTENT_ERROR_NONE;
    for (int i = 0; i < archive.count && error == CONTENT_ERROR_NONE; i++) {
        const ApkEntry *entry = &archive.entries[i];
        unsigned int size = 0;
        unsigned char *data = ApkReadEntry(&archive, entry, &size);
        if (!data) {
            error = CONTENT_ERROR_CORRUPT;
            break;
        }
        if (!ContentWriteFile(entry->name, data, size)) error = CONTENT_ERROR_DISK;
        MemFree(data);
        cJSON_AddItemToArray(files, cJSON_CreateString(entry->name));
    }
    ApkClose(&archive);
    if (error != CONTENT_ERROR_NONE) {
        cJSON_Delete(files);
        return error;
    }
    *installed = files;
    return CONTENT_ERROR_NONE;
}

static cJSON *ContentStatePacks(cJSON *state) {
    return cJSON_GetObjectItemCaseSensitive(state, "packs");
}

static cJSON *ContentStateLoad(void) {
    cJSON *state = ContentLoadJson(CONTENT_STATE_PATH);
    if (cJSON_IsObject(state) && cJSON_IsArray(ContentStatePacks(state))) return state;
    cJSON_Delete(state);
    state = cJSON_CreateObject();
    cJSON_AddArrayToObject(state, "packs");
    return state;
}

static bool ContentStateSave(cJSON *state) {
    char *text = cJSON_PrintUnformatted(state);
    if (!text) return false;
    bool saved = ContentWriteFile(CONTENT_STATE_PATH, text, strlen(text));
    cJSON_free(text);
    return saved;
}

static cJSON *ContentStateFind(cJSON *state, const char *name) {
    cJSON *pack;
    cJSON_ArrayForEach(pack, ContentStatePacks(state)) {
        if (strcmp(JsonStr(pack, "name"), name) == 0) return pack;
    }
    return NULL;
}

static bool ContentStatePackIntact(const cJSON *pack) {
    const cJSON *file;
    cJSON_ArrayForEach(file, cJSON_GetObjectItemCaseSensitive(pack, "files")) {
        if (!cJSON_IsString(file) || !FileExists(file->valuestring)) return false;
    }
    return true;
}

static bool ContentStateUsable(cJSON *state) {
    cJSON *packs = ContentStatePacks(state);
    if (JsonInt(state, "format") != CONTENT_MANIFEST_FORMAT || cJSON_GetArraySize(packs) == 0) return false;
    cJSON *pack;
    cJSON_ArrayForEach(pack, packs) {
        if (!ContentStatePackIntact(pack)) return false;
    }
    return true;
}

static bool ContentListHas(const cJSON *list, const char *path) {
    const cJSON *item;
    cJSON_ArrayForEach(item, list) {
        if (cJSON_IsString(item) && strcmp(item->valuestring, path) == 0) return true;
    }
    return false;
}

static bool ContentStateOwns(cJSON *state, const char *path, const cJSON *except) {
    cJSON *pack;
    cJSON_ArrayForEach(pack, ContentStatePacks(state)) {
        if (pack != except && ContentListHas(cJSON_GetObjectItemCaseSensitive(pack, "files"), path)) return true;
    }
    return false;
}

static void ContentRemoveOrphans(cJSON *state, const cJSON *previous, const cJSON *kept, const cJSON *except) {
    const cJSON *file;
    cJSON_ArrayForEach(file, previous) {
        if (!cJSON_IsString(file) || !ContentEntryPathSafe(file->valuestring)) continue;
        if (ContentListHas(kept, file->valuestring) || ContentStateOwns(state, file->valuestring, except)) continue;
        remove(file->valuestring);
    }
}

static void ContentStateRecord(cJSON *state, const ContentPack *pack, cJSON *files) {
    cJSON *record = cJSON_CreateObject();
    cJSON_AddStringToObject(record, "name", pack->name);
    cJSON_AddStringToObject(record, "sha256", pack->sha256);
    cJSON_AddItemToObject(record, "files", files);
    cJSON *previous = ContentStateFind(state, pack->name);
    if (!previous) {
        cJSON_AddItemToArray(ContentStatePacks(state), record);
        return;
    }
    ContentRemoveOrphans(state, cJSON_GetObjectItemCaseSensitive(previous, "files"), files, previous);
    cJSON_ReplaceItemViaPointer(ContentStatePacks(state), previous, record);
}

static bool ContentManifestHas(const ContentManifest *manifest, const char *name) {
    for (int i = 0; i < manifest->count; i++)
        if (strcmp(manifest->packs[i].name, name) == 0) return true;
    return false;
}

static void ContentStateDropRetired(cJSON *state, const ContentManifest *manifest) {
    cJSON *packs = ContentStatePacks(state);
    for (int i = cJSON_GetArraySize(packs) - 1; i >= 0; i--) {
        cJSON *pack = cJSON_GetArrayItem(packs, i);
        if (ContentManifestHas(manifest, JsonStr(pack, "name"))) continue;
        ContentRemoveOrphans(state, cJSON_GetObjectItemCaseSensitive(pack, "files"), NULL, pack);
        cJSON_DeleteItemFromArray(packs, i);
    }
}

static bool ContentPackCurrent(cJSON *state, const ContentPack *pack) {
    cJSON *installed = ContentStateFind(state, pack->name);
    return installed && strcmp(JsonStr(installed, "sha256"), pack->sha256) == 0 && ContentStatePackIntact(installed);
}

static ContentError ContentInstallPack(
    ContentUpdate *update, ContentProgress *progress, cJSON *state, const ContentPack *pack, long long finished_bytes
) {
    char path[CONTENT_PATH_CAP];
    ContentPackDownloadPath(pack, path, sizeof(path));
    progress->phase = CONTENT_PHASE_DOWNLOADING;
    ContentPublish(update, progress);
    ContentError error = ContentFetchPack(update, progress, finished_bytes, pack, path);
    if (error != CONTENT_ERROR_NONE) return error;
    progress->phase = CONTENT_PHASE_INSTALLING;
    ContentPublish(update, progress);
    cJSON *files = NULL;
    error = ContentExtractPack(path, &files);
    remove(path);
    if (error != CONTENT_ERROR_NONE) return error;
    ContentStateRecord(state, pack, files);
    return ContentStateSave(state) ? CONTENT_ERROR_NONE : CONTENT_ERROR_DISK;
}

static ContentError
ContentApplyManifest(ContentUpdate *update, ContentProgress *progress, cJSON *state, const ContentManifest *manifest) {
    bool *needed = calloc(manifest->count > 0 ? (size_t)manifest->count : 1, sizeof(*needed));
    for (int i = 0; i < manifest->count; i++) {
        needed[i] = !ContentPackCurrent(state, &manifest->packs[i]);
        if (!needed[i]) continue;
        progress->total_bytes += manifest->packs[i].size;
        progress->packs_total++;
    }
    ContentError error = CONTENT_ERROR_NONE;
    long long finished_bytes = 0;
    for (int i = 0; i < manifest->count && error == CONTENT_ERROR_NONE; i++) {
        if (!needed[i]) continue;
        if (ContentCancelled(update)) {
            error = CONTENT_ERROR_CANCELLED;
            break;
        }
        error = ContentInstallPack(update, progress, state, &manifest->packs[i], finished_bytes);
        if (error != CONTENT_ERROR_NONE) break;
        finished_bytes += manifest->packs[i].size;
        progress->received_bytes = finished_bytes;
        progress->packs_done++;
        ContentPublish(update, progress);
    }
    free(needed);
    if (error != CONTENT_ERROR_NONE) return error;
    ContentStateDropRetired(state, manifest);
    cJSON_DeleteItemFromObjectCaseSensitive(state, "version");
    cJSON_AddStringToObject(state, "version", manifest->version);
    cJSON_DeleteItemFromObjectCaseSensitive(state, "format");
    cJSON_AddNumberToObject(state, "format", manifest->format);
    return ContentStateSave(state) ? CONTENT_ERROR_NONE : CONTENT_ERROR_DISK;
}

static const char *ContentErrorName(ContentError error) {
    switch (error) {
        case CONTENT_ERROR_NONE:
            return "none";
        case CONTENT_ERROR_NOT_CONFIGURED:
            return "no content URL";
        case CONTENT_ERROR_DOWNLOAD_UNAVAILABLE:
            return "downloads unavailable";
        case CONTENT_ERROR_OFFLINE:
            return "server unreachable";
        case CONTENT_ERROR_SERVER:
            return "bad server response";
        case CONTENT_ERROR_CLIENT_TOO_OLD:
            return "client too old";
        case CONTENT_ERROR_CORRUPT:
            return "damaged pack";
        case CONTENT_ERROR_DISK:
            return "disk write failed";
        case CONTENT_ERROR_CANCELLED:
            return "cancelled";
    }
    return "unknown";
}

static ContentError ContentUpdateApply(ContentUpdate *update, ContentProgress *progress) {
    cJSON *state = ContentStateLoad();
    progress->first_install = cJSON_GetArraySize(ContentStatePacks(state)) == 0;
    ContentPublish(update, progress);
    ContentManifest manifest;
    ContentError error = ContentFetchManifest(update, &manifest);
    if (error == CONTENT_ERROR_NONE) {
        error = ContentApplyManifest(update, progress, state, &manifest);
    } else if (error != CONTENT_ERROR_CANCELLED && ContentStateUsable(state)) {
        TraceLog(LOG_WARNING, "Content: update skipped (%s); using installed data", ContentErrorName(error));
        error = CONTENT_ERROR_NONE;
    }
    ContentManifestFree(&manifest);
    cJSON_Delete(state);
    return error;
}

static void ContentUpdateExecute(ContentUpdate *update) {
    ContentProgress progress = {.phase = CONTENT_PHASE_CHECKING};
    ContentPublish(update, &progress);
    progress.error = ContentUpdateApply(update, &progress);
    progress.phase = progress.error == CONTENT_ERROR_NONE ? CONTENT_PHASE_READY : CONTENT_PHASE_FAILED;
    if (progress.error != CONTENT_ERROR_NONE)
        TraceLog(LOG_WARNING, "Content: update failed (%s)", ContentErrorName(progress.error));
    ContentPublish(update, &progress);
}

static void *ContentUpdateThread(void *argument) {
    ContentUpdateExecute(argument);
    return NULL;
}

static void ContentUpdateBegin(ContentUpdate *update, const char *base_url) {
    *update = (ContentUpdate){.progress = {.phase = CONTENT_PHASE_CHECKING}};
    StrCopy(update->base_url, sizeof(update->base_url), base_url);
    pthread_mutex_init(&update->lock, NULL);
}

static void ContentUpdateEnd(ContentUpdate *update) {
    pthread_mutex_destroy(&update->lock);
}

static void ContentUpdateStart(ContentUpdate *update, const char *base_url) {
    ContentUpdateBegin(update, base_url);
    update->started = pthread_create(&update->thread, NULL, ContentUpdateThread, update) == 0;
    if (update->started) return;
    update->progress = (ContentProgress){.phase = CONTENT_PHASE_FAILED, .error = CONTENT_ERROR_DISK};
}

static ContentProgress ContentUpdatePoll(ContentUpdate *update) {
    pthread_mutex_lock(&update->lock);
    ContentProgress progress = update->progress;
    pthread_mutex_unlock(&update->lock);
    return progress;
}

static void ContentUpdateStop(ContentUpdate *update, bool cancel) {
    if (cancel) {
        pthread_mutex_lock(&update->lock);
        update->cancel = true;
        pthread_mutex_unlock(&update->lock);
    }
    if (update->started) pthread_join(update->thread, NULL);
    update->started = false;
    ContentUpdateEnd(update);
}
