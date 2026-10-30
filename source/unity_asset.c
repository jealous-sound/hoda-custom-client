#define UNITY_SERIALIZED_VERSION 9
#define UNITY_HEADER_SIZE 20
#define UNITY_NAME_MAX 64

typedef enum {
    UNITY_CLASS_TEXTURE_2D = 28,
    UNITY_CLASS_AUDIO_CLIP = 83,
    UNITY_CLASS_FONT = 128,
} UnityClassId;

typedef enum {
    UNITY_AUDIO_MPEG = 13,
    UNITY_AUDIO_WAV = 20,
} UnityAudioType;

typedef struct {
    unsigned int offset;
    unsigned int size;
    int class_id;
} UnityObject;

typedef struct {
    const unsigned char *data;
    unsigned int size;
    unsigned int pos;
    bool failed;
} UnityReader;

static UnityReader UnityReaderOver(const unsigned char *data, unsigned int size) {
    return (UnityReader){.data = data, .size = size};
}

static bool UnityReaderHas(UnityReader *r, unsigned int bytes) {
    if (r->failed || bytes > r->size - r->pos) r->failed = true;
    return !r->failed;
}

static unsigned int UnityReadBigU32(UnityReader *r) {
    if (!UnityReaderHas(r, 4)) return 0;
    const unsigned char *p = r->data + r->pos;
    r->pos += 4;
    return (unsigned int)p[0] << 24 | (unsigned int)p[1] << 16 | (unsigned int)p[2] << 8 | p[3];
}

static int UnityReadI32(UnityReader *r) {
    if (!UnityReaderHas(r, 4)) return 0;
    unsigned int v = ReadU32(r->data + r->pos);
    r->pos += 4;
    return (int)v;
}

static int UnityReadI16(UnityReader *r) {
    if (!UnityReaderHas(r, 2)) return 0;
    unsigned int v = ReadU16(r->data + r->pos);
    r->pos += 2;
    return (short)v;
}

static void UnitySkip(UnityReader *r, unsigned int bytes) {
    if (UnityReaderHas(r, bytes)) r->pos += bytes;
}

static void UnityAlign(UnityReader *r) {
    UnitySkip(r, (4 - r->pos % 4) % 4);
}

static void UnitySkipCString(UnityReader *r) {
    while (UnityReaderHas(r, 1) && r->data[r->pos++] != '\0') {}
}

static const unsigned char *UnityReadBytes(UnityReader *r, int *size) {
    *size = UnityReadI32(r);
    if (*size < 0 || !UnityReaderHas(r, (unsigned int)*size)) return NULL;
    const unsigned char *bytes = r->data + r->pos;
    r->pos += (unsigned int)*size;
    UnityAlign(r);
    return bytes;
}

static void UnityReadName(UnityReader *r, char name[UNITY_NAME_MAX]) {
    int size = 0;
    const unsigned char *bytes = UnityReadBytes(r, &size);
    int n = bytes ? (size < UNITY_NAME_MAX - 1 ? size : UNITY_NAME_MAX - 1) : 0;
    if (n > 0) memcpy(name, bytes, n);
    name[n] = '\0';
}

static unsigned int UnityMetadataEnd(const unsigned char header[UNITY_HEADER_SIZE]) {
    UnityReader r = UnityReaderOver(header, UNITY_HEADER_SIZE);
    return UNITY_HEADER_SIZE + UnityReadBigU32(&r);
}

static int UnityReadObjectTable(const unsigned char *file, unsigned int size, UnityObject **objects) {
    *objects = NULL;
    UnityReader r = UnityReaderOver(file, size);
    UnitySkip(&r, 4);
    UnityReadBigU32(&r);
    unsigned int version = UnityReadBigU32(&r);
    unsigned int data_offset = UnityReadBigU32(&r);
    UnitySkip(&r, 4);
    if (r.failed || version != UNITY_SERIALIZED_VERSION) return -1;
    UnitySkipCString(&r);
    UnityReadI32(&r);
    int type_count = UnityReadI32(&r);
    int big_ids = UnityReadI32(&r);
    int count = UnityReadI32(&r);
    if (r.failed || type_count != 0 || big_ids != 0 || count < 0) return -1;
    *objects = MemAlloc(sizeof(**objects) * (count > 0 ? count : 1));
    for (int i = 0; i < count; i++) {
        UnityReadI32(&r);
        unsigned int start = (unsigned int)UnityReadI32(&r);
        unsigned int length = (unsigned int)UnityReadI32(&r);
        UnityReadI32(&r);
        int class_id = UnityReadI16(&r);
        UnityReadI16(&r);
        (*objects)[i] = (UnityObject){data_offset + start, length, class_id};
    }
    if (r.failed) {
        MemFree(*objects);
        *objects = NULL;
        return -1;
    }
    return count;
}

static bool UnityIsNamedClass(int class_id) {
    return class_id == UNITY_CLASS_TEXTURE_2D || class_id == UNITY_CLASS_AUDIO_CLIP || class_id == UNITY_CLASS_FONT;
}

static void UnityObjectName(const unsigned char *object, unsigned int size, char name[UNITY_NAME_MAX]) {
    UnityReader r = UnityReaderOver(object, size);
    UnityReadName(&r, name);
}

static Image UnityDecodeTexture(const unsigned char *object, unsigned int size) {
    char name[UNITY_NAME_MAX];
    UnityReader r = UnityReaderOver(object, size);
    UnityReadName(&r, name);
    int width = UnityReadI32(&r);
    int height = UnityReadI32(&r);
    UnityReadI32(&r);
    UnityTextureFormat format = (UnityTextureFormat)UnityReadI32(&r);
    UnitySkip(&r, 4 + 4 + 4 + 16 + 4 + 4);
    int data_size = 0;
    const unsigned char *data = UnityReadBytes(&r, &data_size);
    if (!data || width <= 0 || height <= 0) return (Image){0};
    int level_size = UnityTextureDataSize(format, width, height);
    if (level_size <= 0 || level_size > data_size) {
        TraceLog(LOG_WARNING, "Unity: texture %s has unsupported format %d", name, format);
        return (Image){0};
    }
    Image image = UnityDecodeTextureData(format, width, height, data);
    if (!image.data) TraceLog(LOG_WARNING, "Unity: could not decode texture %s (format %d)", name, format);
    return image;
}

static const unsigned char *
UnityAudioClipData(const unsigned char *object, unsigned int size, int *data_size, const char **file_type) {
    char name[UNITY_NAME_MAX];
    UnityReader r = UnityReaderOver(object, size);
    UnityReadName(&r, name);
    UnityReadI32(&r);
    UnityAudioType type = (UnityAudioType)UnityReadI32(&r);
    UnitySkip(&r, 4 + 4);
    const unsigned char *data = UnityReadBytes(&r, data_size);
    *file_type = type == UNITY_AUDIO_MPEG ? ".mp3" : type == UNITY_AUDIO_WAV ? ".wav" : NULL;
    if (!*file_type) TraceLog(LOG_WARNING, "Unity: audio clip %s has unsupported type %d", name, type);
    return *file_type ? data : NULL;
}

static const unsigned char *UnityFontData(const unsigned char *object, unsigned int size, int *data_size) {
    char name[UNITY_NAME_MAX];
    UnityReader r = UnityReaderOver(object, size);
    UnityReadName(&r, name);
    UnitySkip(&r, 6 * 4 + 8);
    int glyph_rects = UnityReadI32(&r);
    if (glyph_rects != 0) {
        TraceLog(LOG_WARNING, "Unity: font %s has baked glyphs, only dynamic fonts are supported", name);
        return NULL;
    }
    UnitySkip(&r, 8);
    int kerning_pairs = UnityReadI32(&r);
    UnitySkip(&r, (unsigned int)(kerning_pairs > 0 ? kerning_pairs : 0) * 8 + 4);
    return UnityReadBytes(&r, data_size);
}
