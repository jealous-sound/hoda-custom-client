#define ORIGINAL_GAME_DIR HOD_CACHE "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row"
#define ORIGINAL_BUNDLE_DIR ORIGINAL_GAME_DIR "/files/UnityCache/Shared"
#define ORIGINAL_LOCALIZATION_DIR ORIGINAL_GAME_DIR "/cache/Localization"
#define ORIGINAL_APK_PATH HOD_CACHE "Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk"
#define ORIGINAL_APK_DATA_PREFIX "assets/bin/Data/"
#define BUNDLE_FILE_PREFIX "CAB-"
#define EXTRA_ASSET_DIR HOD_CACHE "extra/"

#define CACHE_RESOURCE_PREFIX "cache:"
#define APK_RESOURCE_PREFIX "apk:"
#define CACHE_RESOURCE(name) CACHE_RESOURCE_PREFIX name
#define CACHE_BUNDLE_RESOURCE(bundle, name) CACHE_RESOURCE_PREFIX bundle "/" name
#define APK_RESOURCE(name) APK_RESOURCE_PREFIX name

#define RESOURCE_BUCKET_COUNT 16384
#define RESOURCE_METADATA_MAX (16 * 1024 * 1024)
#define RESOURCE_MUSIC_MAX 32
#define RESOURCE_WORKERS_MAX 16
#define RESOURCE_REF_MAX 256
#define RESOURCE_WORKER_STACK (8 * 1024 * 1024)

typedef enum {
    RESOURCE_FILE,
    RESOURCE_CACHE,
    RESOURCE_APK,
} ResourceOrigin;

typedef struct {
    ResourceOrigin origin;
    char bundle[UNITY_NAME_MAX];
    char name[UNITY_NAME_MAX];
    const char *path;
} ResourceRef;

typedef struct {
    char name[UNITY_NAME_MAX];
    int container;
    UnityObject object;
    int next;
} ResourceEntry;

typedef struct {
    ResourceEntry *entries;
    int count;
    int capacity;
    int buckets[RESOURCE_BUCKET_COUNT];
    bool indexed;
} ResourceCatalog;

typedef struct {
    void *stream;
    unsigned char *data;
} ResourceMusicData;

typedef struct {
    char ref[RESOURCE_REF_MAX];
    Image image;
} ResourcePrefetchedImage;

typedef struct {
    ResourcePrefetchedImage *entries;
    int count;
} ResourcePrefetch;

typedef struct {
    char (*refs)[RESOURCE_REF_MAX];
    int count;
    int capacity;
} ResourceRecording;

static ResourcePrefetch resource_prefetched;
static ResourceRecording resource_recorded;
static bool resource_recording;
static ResourceCatalog resource_cache;
static ResourceCatalog resource_apk;
static FilePathList resource_bundle_files;
static ApkArchive resource_apk_archive;
static ResourceMusicData resource_music[RESOURCE_MUSIC_MAX];

static int ResourceThreadStart(pthread_t *thread, void *(*routine)(void *), void *argument) {
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, RESOURCE_WORKER_STACK);
    int result = pthread_create(thread, &attributes, routine, argument);
    pthread_attr_destroy(&attributes);
    return result;
}

static unsigned int ResourceHash(const char *name) {
    unsigned int hash = 2166136261u;
    for (; *name; name++) hash = (hash ^ (unsigned char)*name) * 16777619u;
    return hash % RESOURCE_BUCKET_COUNT;
}

static void ResourceCatalogReset(ResourceCatalog *catalog) {
    MemFree(catalog->entries);
    *catalog = (ResourceCatalog){0};
    for (int i = 0; i < RESOURCE_BUCKET_COUNT; i++) catalog->buckets[i] = -1;
}

static void ResourceCatalogAdd(ResourceCatalog *catalog, const char *name, int container, UnityObject object) {
    if (catalog->count == catalog->capacity) {
        catalog->capacity = catalog->capacity ? catalog->capacity * 2 : 1024;
        catalog->entries = MemRealloc(catalog->entries, sizeof(*catalog->entries) * catalog->capacity);
    }
    unsigned int bucket = ResourceHash(name);
    ResourceEntry *entry = &catalog->entries[catalog->count];
    *entry = (ResourceEntry){.container = container, .object = object, .next = catalog->buckets[bucket]};
    snprintf(entry->name, sizeof(entry->name), "%s", name);
    catalog->buckets[bucket] = catalog->count++;
}

static const char *ResourceBundleName(int container) {
    return GetFileName(resource_bundle_files.paths[container]) + strlen(BUNDLE_FILE_PREFIX);
}

static bool ResourceReadFileRange(FILE *file, unsigned int offset, unsigned char *out, unsigned int size) {
    return fseek(file, (long)offset, SEEK_SET) == 0 && fread(out, 1, size, file) == size;
}

static void ResourceIndexBundle(ResourceCatalog *catalog, int container) {
    FILE *file = fopen(resource_bundle_files.paths[container], "rb");
    if (!file) return;
    unsigned char header[UNITY_HEADER_SIZE];
    unsigned char *metadata = NULL;
    UnityObject *objects = NULL;
    int count = -1;
    if (ResourceReadFileRange(file, 0, header, UNITY_HEADER_SIZE)) {
        unsigned int metadata_end = UnityMetadataEnd(header);
        if (metadata_end <= RESOURCE_METADATA_MAX) {
            metadata = MemAlloc(metadata_end);
            if (ResourceReadFileRange(file, 0, metadata, metadata_end))
                count = UnityReadObjectTable(metadata, metadata_end, &objects);
        }
    }
    unsigned char name_field[4 + UNITY_NAME_MAX];
    for (int i = 0; i < count; i++) {
        if (!UnityIsNamedClass(objects[i].class_id)) continue;
        unsigned int length = objects[i].size < sizeof(name_field) ? objects[i].size : sizeof(name_field);
        if (!ResourceReadFileRange(file, objects[i].offset, name_field, length)) continue;
        char name[UNITY_NAME_MAX];
        UnityObjectName(name_field, length, name);
        ResourceCatalogAdd(catalog, name, container, objects[i]);
    }
    if (count < 0) TraceLog(LOG_WARNING, "Resource: unreadable bundle %s", resource_bundle_files.paths[container]);
    MemFree(objects);
    MemFree(metadata);
    fclose(file);
}

typedef struct {
    ResourceCatalog catalog;
    int *next;
} ResourceBundleJob;

static int ResourceWorkerCount(int tasks) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    int count = cores < RESOURCE_WORKERS_MAX ? (int)cores : RESOURCE_WORKERS_MAX;
    return count < tasks ? count : tasks;
}

static void *ResourceBundleWorker(void *argument) {
    ResourceBundleJob *job = argument;
    ResourceCatalogReset(&job->catalog);
    for (int i = __atomic_fetch_add(job->next, 1, __ATOMIC_RELAXED); i < (int)resource_bundle_files.count;
         i = __atomic_fetch_add(job->next, 1, __ATOMIC_RELAXED))
        ResourceIndexBundle(&job->catalog, i);
    return NULL;
}

static void ResourceIndexBundles(void) {
    int worker_count = ResourceWorkerCount((int)resource_bundle_files.count);
    int next = 0;
    ResourceBundleJob jobs[RESOURCE_WORKERS_MAX] = {0};
    pthread_t threads[RESOURCE_WORKERS_MAX];
    bool threaded[RESOURCE_WORKERS_MAX] = {0};
    for (int i = 0; i < worker_count; i++) jobs[i].next = &next;
    for (int i = 1; i < worker_count; i++) threaded[i] = ResourceThreadStart(&threads[i], ResourceBundleWorker, &jobs[i]) == 0;
    ResourceBundleWorker(&jobs[0]);
    for (int i = 0; i < worker_count; i++) {
        if (i > 0 && threaded[i]) pthread_join(threads[i], NULL);
        for (int e = 0; e < jobs[i].catalog.count; e++) {
            const ResourceEntry *entry = &jobs[i].catalog.entries[e];
            ResourceCatalogAdd(&resource_cache, entry->name, entry->container, entry->object);
        }
        MemFree(jobs[i].catalog.entries);
    }
}

static void ResourceIndexCache(void) {
    if (resource_cache.indexed) return;
    ResourceCatalogReset(&resource_cache);
    resource_cache.indexed = true;
    FilePathList bundle_dirs = LoadDirectoryFiles(ORIGINAL_BUNDLE_DIR);
    resource_bundle_files = (FilePathList){
        .capacity = bundle_dirs.count,
        .paths = MemAlloc(sizeof(char *) * (bundle_dirs.count > 0 ? bundle_dirs.count : 1)),
    };
    for (unsigned int d = 0; d < bundle_dirs.count; d++) {
        if (!DirectoryExists(bundle_dirs.paths[d])) continue;
        FilePathList files = LoadDirectoryFiles(bundle_dirs.paths[d]);
        for (unsigned int i = 0; i < files.count && resource_bundle_files.count < resource_bundle_files.capacity; i++) {
            if (strncmp(GetFileName(files.paths[i]), BUNDLE_FILE_PREFIX, strlen(BUNDLE_FILE_PREFIX)) != 0) continue;
            size_t length = strlen(files.paths[i]) + 1;
            char *path = MemAlloc((unsigned int)length);
            memcpy(path, files.paths[i], length);
            resource_bundle_files.paths[resource_bundle_files.count++] = path;
        }
        UnloadDirectoryFiles(files);
    }
    UnloadDirectoryFiles(bundle_dirs);
    if (resource_bundle_files.count == 0) TraceLog(LOG_FATAL, "Resource: no bundles in %s", ORIGINAL_BUNDLE_DIR);
    ResourceIndexBundles();
    TraceLog(
        LOG_INFO, "Resource: indexed %d objects in %u cache bundles", resource_cache.count, resource_bundle_files.count
    );
}

static bool ResourceIsApkData(const ApkEntry *entry) {
    size_t prefix = strlen(ORIGINAL_APK_DATA_PREFIX);
    return strncmp(entry->name, ORIGINAL_APK_DATA_PREFIX, prefix) == 0 && entry->name[strlen(entry->name) - 1] != '/';
}

static void ResourceIndexApk(void) {
    if (resource_apk.indexed) return;
    ResourceCatalogReset(&resource_apk);
    resource_apk.indexed = true;
    if (!ApkOpen(&resource_apk_archive, ORIGINAL_APK_PATH))
        TraceLog(LOG_FATAL, "Resource: could not open %s", ORIGINAL_APK_PATH);
    for (int e = 0; e < resource_apk_archive.count; e++) {
        if (!ResourceIsApkData(&resource_apk_archive.entries[e])) continue;
        unsigned int size = 0;
        unsigned char *data = ApkReadEntry(&resource_apk_archive, &resource_apk_archive.entries[e], &size);
        UnityObject *objects = NULL;
        int count = data ? UnityReadObjectTable(data, size, &objects) : -1;
        for (int i = 0; i < count; i++) {
            if (!UnityIsNamedClass(objects[i].class_id) || objects[i].offset + objects[i].size > size) continue;
            char name[UNITY_NAME_MAX];
            UnityObjectName(data + objects[i].offset, objects[i].size, name);
            ResourceCatalogAdd(&resource_apk, name, e, objects[i]);
        }
        MemFree(objects);
        MemFree(data);
    }
    TraceLog(LOG_INFO, "Resource: indexed %d objects in %s", resource_apk.count, ORIGINAL_APK_PATH);
}

static ResourceRef ResourceParse(const char *ref) {
    ResourceRef parsed = {.origin = RESOURCE_FILE, .path = ref};
    const char *rest = NULL;
    if (strncmp(ref, CACHE_RESOURCE_PREFIX, strlen(CACHE_RESOURCE_PREFIX)) == 0) {
        parsed.origin = RESOURCE_CACHE;
        rest = ref + strlen(CACHE_RESOURCE_PREFIX);
    } else if (strncmp(ref, APK_RESOURCE_PREFIX, strlen(APK_RESOURCE_PREFIX)) == 0) {
        parsed.origin = RESOURCE_APK;
        rest = ref + strlen(APK_RESOURCE_PREFIX);
    }
    if (!rest) return parsed;
    const char *slash = strchr(rest, '/');
    if (slash) {
        snprintf(parsed.bundle, sizeof(parsed.bundle), "%.*s", (int)(slash - rest), rest);
        rest = slash + 1;
    }
    snprintf(parsed.name, sizeof(parsed.name), "%s", rest);
    return parsed;
}

static const ResourceEntry *ResourceFind(const ResourceRef *ref, int class_id) {
    ResourceCatalog *catalog = ref->origin == RESOURCE_CACHE ? &resource_cache : &resource_apk;
    if (ref->origin == RESOURCE_CACHE)
        ResourceIndexCache();
    else
        ResourceIndexApk();
    const ResourceEntry *found = NULL;
    for (int i = catalog->buckets[ResourceHash(ref->name)]; i >= 0; i = catalog->entries[i].next) {
        const ResourceEntry *entry = &catalog->entries[i];
        if (entry->object.class_id != class_id || strcmp(entry->name, ref->name) != 0) continue;
        if (ref->origin != RESOURCE_CACHE) return entry;
        const char *bundle = ResourceBundleName(entry->container);
        if (ref->bundle[0] && strcmp(bundle, ref->bundle) != 0) continue;
        if (!found || strcmp(bundle, ResourceBundleName(found->container)) < 0) found = entry;
    }
    return found;
}

static unsigned char *ResourceReadObject(const ResourceRef *ref, const ResourceEntry *entry) {
    unsigned char *object = MemAlloc(entry->object.size > 0 ? entry->object.size : 1);
    bool ok = false;
    if (ref->origin == RESOURCE_CACHE) {
        FILE *file = fopen(resource_bundle_files.paths[entry->container], "rb");
        ok = file && ResourceReadFileRange(file, entry->object.offset, object, entry->object.size);
        if (file) fclose(file);
    } else {
        unsigned int size = 0;
        unsigned char *data =
            ApkReadEntry(&resource_apk_archive, &resource_apk_archive.entries[entry->container], &size);
        ok = data && entry->object.offset + entry->object.size <= size;
        if (ok) memcpy(object, data + entry->object.offset, entry->object.size);
        MemFree(data);
    }
    if (ok) return object;
    MemFree(object);
    return NULL;
}

static unsigned char *ResourceLoadObject(const char *ref, int class_id, unsigned int *size) {
    ResourceRef parsed = ResourceParse(ref);
    const ResourceEntry *entry = ResourceFind(&parsed, class_id);
    unsigned char *object = entry ? ResourceReadObject(&parsed, entry) : NULL;
    *size = object ? entry->object.size : 0;
    if (!object) TraceLog(LOG_WARNING, "Resource: %s not found", ref);
    return object;
}

bool ResourceExists(const char *ref) {
    ResourceRef parsed = ResourceParse(ref);
    if (parsed.origin == RESOURCE_FILE) return FileExists(ref);
    return ResourceFind(&parsed, UNITY_CLASS_TEXTURE_2D) || ResourceFind(&parsed, UNITY_CLASS_AUDIO_CLIP) ||
           ResourceFind(&parsed, UNITY_CLASS_FONT);
}

static void *ResourceApkIndexWorker(void *argument) {
    (void)argument;
    ResourceIndexApk();
    return NULL;
}

static void ResourceIndexSources(const char *const *refs, int count) {
    bool needs_cache = false, needs_apk = false;
    for (int i = 0; i < count; i++) {
        ResourceOrigin origin = ResourceParse(refs[i]).origin;
        needs_cache |= origin == RESOURCE_CACHE;
        needs_apk |= origin == RESOURCE_APK;
    }
    pthread_t apk_thread;
    bool apk_threaded = needs_apk && ResourceThreadStart(&apk_thread, ResourceApkIndexWorker, NULL) == 0;
    if (needs_apk && !apk_threaded) ResourceIndexApk();
    if (needs_cache) ResourceIndexCache();
    if (apk_threaded) pthread_join(apk_thread, NULL);
}

static Image ResourceDecodeImage(const char *ref) {
    unsigned int object_size = 0;
    if (ResourceParse(ref).origin == RESOURCE_FILE) return LoadImage(ref);
    unsigned char *object = ResourceLoadObject(ref, UNITY_CLASS_TEXTURE_2D, &object_size);
    if (!object) return (Image){0};
    Image image = UnityDecodeTexture(object, object_size);
    MemFree(object);
    return image;
}

typedef struct {
    const char *const *refs;
    Image *images;
    int count;
    int next;
} ResourceImageJob;

static void *ResourceImageWorker(void *argument) {
    ResourceImageJob *job = argument;
    for (int i = __atomic_fetch_add(&job->next, 1, __ATOMIC_RELAXED); i < job->count;
         i = __atomic_fetch_add(&job->next, 1, __ATOMIC_RELAXED))
        job->images[i] = ResourceDecodeImage(job->refs[i]);
    return NULL;
}

static void ResourceDecodeImages(const char *const *refs, int count, Image *images) {
    ResourceImageJob job = {.refs = refs, .images = images, .count = count};
    ResourceIndexSources(refs, count);
    int worker_count = ResourceWorkerCount(count);
    pthread_t workers[RESOURCE_WORKERS_MAX];
    int started = 0;
    for (int i = 1; i < worker_count; i++)
        if (ResourceThreadStart(&workers[started], ResourceImageWorker, &job) == 0) started++;
    ResourceImageWorker(&job);
    for (int i = 0; i < started; i++) pthread_join(workers[i], NULL);
}

static bool ResourceIsRecordable(const char *ref) {
    return ResourceParse(ref).origin != RESOURCE_FILE && strlen(ref) < RESOURCE_REF_MAX;
}

static void ResourceRecordImage(const char *ref) {
    if (!resource_recording || !ResourceIsRecordable(ref)) return;
    if (resource_recorded.count == resource_recorded.capacity) {
        resource_recorded.capacity = resource_recorded.capacity ? resource_recorded.capacity * 2 : 256;
        resource_recorded.refs =
            MemRealloc(resource_recorded.refs, sizeof(*resource_recorded.refs) * resource_recorded.capacity);
    }
    snprintf(resource_recorded.refs[resource_recorded.count++], RESOURCE_REF_MAX, "%s", ref);
}

static Image ResourceTakePrefetched(const char *ref) {
    for (int i = 0; i < resource_prefetched.count; i++) {
        ResourcePrefetchedImage *entry = &resource_prefetched.entries[i];
        if (!entry->image.data || strcmp(entry->ref, ref) != 0) continue;
        Image image = entry->image;
        entry->image = (Image){0};
        return image;
    }
    return (Image){0};
}

void ResourcePrefetchImages(const char *manifest_path) {
    char *text = LoadFileText(manifest_path);
    if (!text) return;
    int line_count = 1;
    for (const char *c = text; *c; c++) line_count += *c == '\n';
    const char **refs = MemAlloc(sizeof(*refs) * line_count);
    int count = 0;
    for (char *line = text; line && *line;) {
        char *end = strchr(line, '\n');
        if (end) *end = '\0';
        if (ResourceIsRecordable(line)) refs[count++] = line;
        line = end ? end + 1 : NULL;
    }
    Image *images = MemAlloc(sizeof(*images) * (count > 0 ? count : 1));
    ResourceDecodeImages(refs, count, images);
    resource_prefetched.entries = MemAlloc(sizeof(*resource_prefetched.entries) * (count > 0 ? count : 1));
    for (int i = 0; i < count; i++) {
        snprintf(resource_prefetched.entries[i].ref, RESOURCE_REF_MAX, "%s", refs[i]);
        resource_prefetched.entries[i].image = images[i];
    }
    resource_prefetched.count = count;
    MemFree(images);
    MemFree(refs);
    UnloadFileText(text);
}

void ResourceRecordImagesBegin(void) {
    resource_recording = true;
}

void ResourceRecordImagesEnd(const char *manifest_path) {
    resource_recording = false;
    for (int i = 0; i < resource_prefetched.count; i++) UnloadImage(resource_prefetched.entries[i].image);
    MemFree(resource_prefetched.entries);
    resource_prefetched = (ResourcePrefetch){0};
    FILE *file = fopen(manifest_path, "w");
    if (!file) return;
    for (int i = 0; i < resource_recorded.count; i++) fprintf(file, "%s\n", resource_recorded.refs[i]);
    fclose(file);
    MemFree(resource_recorded.refs);
    resource_recorded = (ResourceRecording){0};
}

Image ResourceLoadImage(const char *ref) {
    ResourceRecordImage(ref);
    Image image = ResourceTakePrefetched(ref);
    return image.data ? image : ResourceDecodeImage(ref);
}

void ResourceLoadImages(const char *const *refs, int count, Image *images) {
    const char **missing_refs = MemAlloc(sizeof(*missing_refs) * (count > 0 ? count : 1));
    int *missing_slots = MemAlloc(sizeof(*missing_slots) * (count > 0 ? count : 1));
    int missing = 0;
    for (int i = 0; i < count; i++) {
        ResourceRecordImage(refs[i]);
        images[i] = ResourceTakePrefetched(refs[i]);
        if (images[i].data) continue;
        missing_refs[missing] = refs[i];
        missing_slots[missing++] = i;
    }
    Image *decoded = MemAlloc(sizeof(*decoded) * (missing > 0 ? missing : 1));
    ResourceDecodeImages(missing_refs, missing, decoded);
    for (int i = 0; i < missing; i++) images[missing_slots[i]] = decoded[i];
    MemFree(decoded);
    MemFree(missing_slots);
    MemFree(missing_refs);
}

Texture2D ResourceLoadTexture(const char *ref) {
    if (ResourceParse(ref).origin == RESOURCE_FILE) return LoadTexture(ref);
    Image image = ResourceLoadImage(ref);
    if (!image.data) return (Texture2D){0};
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

static Wave ResourceLoadWave(const char *ref) {
    unsigned int object_size = 0;
    if (ResourceParse(ref).origin == RESOURCE_FILE) return LoadWave(ref);
    unsigned char *object = ResourceLoadObject(ref, UNITY_CLASS_AUDIO_CLIP, &object_size);
    if (!object) return (Wave){0};
    int size = 0;
    const char *file_type = NULL;
    const unsigned char *data = UnityAudioClipData(object, object_size, &size, &file_type);
    Wave wave = data ? LoadWaveFromMemory(file_type, data, size) : (Wave){0};
    MemFree(object);
    return wave;
}

Sound ResourceLoadSound(const char *ref) {
    Wave wave = ResourceLoadWave(ref);
    if (!IsWaveValid(wave)) return (Sound){0};
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

Music ResourceLoadMusic(const char *ref) {
    unsigned int object_size = 0;
    if (ResourceParse(ref).origin == RESOURCE_FILE) return LoadMusicStream(ref);
    unsigned char *object = ResourceLoadObject(ref, UNITY_CLASS_AUDIO_CLIP, &object_size);
    if (!object) return (Music){0};
    int size = 0;
    const char *file_type = NULL;
    const unsigned char *data = UnityAudioClipData(object, object_size, &size, &file_type);
    int slot = 0;
    while (slot < RESOURCE_MUSIC_MAX && resource_music[slot].stream) slot++;
    if (!data || slot == RESOURCE_MUSIC_MAX) {
        if (slot == RESOURCE_MUSIC_MAX) TraceLog(LOG_WARNING, "Resource: too many music streams for %s", ref);
        MemFree(object);
        return (Music){0};
    }
    unsigned char *stream_data = MemAlloc(size);
    memcpy(stream_data, data, size);
    MemFree(object);
    Music music = LoadMusicStreamFromMemory(file_type, stream_data, size);
    if (!music.ctxData) {
        MemFree(stream_data);
        return music;
    }
    resource_music[slot] = (ResourceMusicData){music.ctxData, stream_data};
    return music;
}

void ResourceUnloadMusic(Music music) {
    void *stream = music.ctxData;
    UnloadMusicStream(music);
    for (int i = 0; stream && i < RESOURCE_MUSIC_MAX; i++) {
        if (resource_music[i].stream != stream) continue;
        MemFree(resource_music[i].data);
        resource_music[i] = (ResourceMusicData){0};
    }
}

Font ResourceLoadFont(const char *ref, int size, int *codepoints, int codepoint_count) {
    unsigned int object_size = 0;
    if (ResourceParse(ref).origin == RESOURCE_FILE) return LoadFontEx(ref, size, codepoints, codepoint_count);
    unsigned char *object = ResourceLoadObject(ref, UNITY_CLASS_FONT, &object_size);
    if (!object) return (Font){0};
    int data_size = 0;
    const unsigned char *data = UnityFontData(object, object_size, &data_size);
    Font font = data ? LoadFontFromMemory(".ttf", data, data_size, size, codepoints, codepoint_count) : (Font){0};
    MemFree(object);
    return font;
}

void ResourceShutdown(void) {
    for (unsigned int i = 0; i < resource_bundle_files.count; i++) MemFree(resource_bundle_files.paths[i]);
    MemFree(resource_bundle_files.paths);
    resource_bundle_files = (FilePathList){0};
    ApkClose(&resource_apk_archive);
    MemFree(resource_cache.entries);
    MemFree(resource_apk.entries);
    resource_cache = (ResourceCatalog){0};
    resource_apk = (ResourceCatalog){0};
}
