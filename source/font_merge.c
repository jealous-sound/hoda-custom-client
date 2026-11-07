#define FONT_MERGE_FALLBACK_CAP 16
#define FONT_MERGE_GLYPH_PADDING 4
#define FONT_MERGE_TABLE_RECORD_BYTES 16
#define FONT_MERGE_OFFSET_TABLE_BYTES 12
#define FONT_MERGE_CMAP_RECORD_BYTES 8
#define FONT_MERGE_GROUP_BYTES 12
#define FONT_MERGE_PLATFORM_UNICODE 0
#define FONT_MERGE_PLATFORM_WINDOWS 3
#define FONT_MERGE_ENCODING_UNICODE_BMP 1
#define FONT_MERGE_ENCODING_UNICODE_FULL 10
#define FONT_MERGE_SEGMENT_FORMAT 4
#define FONT_MERGE_GROUP_FORMAT 12
#define FONT_MERGE_COLLECTION_TAG "ttcf"
#define FONT_MERGE_PRIMARY_OWNER 0
#define FONT_MERGE_HANGUL_FIRST 0xAC00
#define FONT_MERGE_HANGUL_LAST 0xD7AF

typedef struct {
    unsigned char *data;
    int size;
    unsigned int cmap;
    int format;
} FontFile;

static unsigned int FontRead16(const FontFile *f, unsigned int at) {
    if (at + 2 > (unsigned int)f->size) return 0;
    return (unsigned int)f->data[at] << 8 | f->data[at + 1];
}

static unsigned int FontRead32(const FontFile *f, unsigned int at) {
    if (at + 4 > (unsigned int)f->size) return 0;
    return (unsigned int)f->data[at] << 24 | (unsigned int)f->data[at + 1] << 16 | (unsigned int)f->data[at + 2] << 8 |
           f->data[at + 3];
}

static unsigned int FontTableOffset(const FontFile *f, const char tag[4]) {
    unsigned int tables = FontRead16(f, 4);
    for (unsigned int i = 0; i < tables; i++) {
        unsigned int record = FONT_MERGE_OFFSET_TABLE_BYTES + i * FONT_MERGE_TABLE_RECORD_BYTES;
        if (record + FONT_MERGE_TABLE_RECORD_BYTES > (unsigned int)f->size) return 0;
        if (!memcmp(f->data + record, tag, 4)) return FontRead32(f, record + 8);
    }
    return 0;
}

static bool FontExtractFirstFace(FontFile *f) {
    if (f->size < FONT_MERGE_OFFSET_TABLE_BYTES || memcmp(f->data, FONT_MERGE_COLLECTION_TAG, 4)) return true;
    unsigned int face = FontRead32(f, FONT_MERGE_OFFSET_TABLE_BYTES);
    unsigned int tables = FontRead16(f, face + 4);
    unsigned int header = FONT_MERGE_OFFSET_TABLE_BYTES + tables * FONT_MERGE_TABLE_RECORD_BYTES;
    unsigned int total = header;
    for (unsigned int i = 0; i < tables; i++)
        total +=
            (FontRead32(f, face + FONT_MERGE_OFFSET_TABLE_BYTES + i * FONT_MERGE_TABLE_RECORD_BYTES + 12) + 3) & ~3u;
    if (tables == 0 || face + header > (unsigned int)f->size) return false;
    unsigned char *face_data = MemAlloc(total);
    memcpy(face_data, f->data + face, header);
    unsigned int written = header;
    for (unsigned int i = 0; i < tables; i++) {
        unsigned int record = face + FONT_MERGE_OFFSET_TABLE_BYTES + i * FONT_MERGE_TABLE_RECORD_BYTES;
        unsigned int offset = FontRead32(f, record + 8), length = FontRead32(f, record + 12);
        if (offset + length > (unsigned int)f->size) {
            MemFree(face_data);
            return false;
        }
        memcpy(face_data + written, f->data + offset, length);
        unsigned char *slot = face_data + FONT_MERGE_OFFSET_TABLE_BYTES + i * FONT_MERGE_TABLE_RECORD_BYTES + 8;
        slot[0] = (unsigned char)(written >> 24);
        slot[1] = (unsigned char)(written >> 16);
        slot[2] = (unsigned char)(written >> 8);
        slot[3] = (unsigned char)written;
        written += (length + 3) & ~3u;
    }
    MemFree(f->data);
    f->data = face_data;
    f->size = (int)total;
    return true;
}

static void FontChooseCmap(FontFile *f) {
    unsigned int cmap = FontTableOffset(f, "cmap");
    unsigned int records = cmap ? FontRead16(f, cmap + 2) : 0;
    int best_rank = 0;
    for (unsigned int i = 0; i < records; i++) {
        unsigned int record = cmap + 4 + i * FONT_MERGE_CMAP_RECORD_BYTES;
        unsigned int platform = FontRead16(f, record), encoding = FontRead16(f, record + 2);
        unsigned int table = cmap + FontRead32(f, record + 4);
        int format = (int)FontRead16(f, table);
        bool windows_full = platform == FONT_MERGE_PLATFORM_WINDOWS && encoding == FONT_MERGE_ENCODING_UNICODE_FULL;
        bool windows_bmp = platform == FONT_MERGE_PLATFORM_WINDOWS && encoding == FONT_MERGE_ENCODING_UNICODE_BMP;
        bool unicode = platform == FONT_MERGE_PLATFORM_UNICODE;
        int rank = format == FONT_MERGE_GROUP_FORMAT && (windows_full || unicode) ? 3
                   : format == FONT_MERGE_SEGMENT_FORMAT && windows_bmp           ? 2
                   : format == FONT_MERGE_SEGMENT_FORMAT && unicode               ? 1
                                                                                  : 0;
        if (rank <= best_rank) continue;
        best_rank = rank;
        f->cmap = table;
        f->format = format;
    }
}

static bool FontSegmentHas(const FontFile *f, int codepoint) {
    if (codepoint > 0xFFFF) return false;
    unsigned int segments = FontRead16(f, f->cmap + 6) / 2;
    unsigned int ends = f->cmap + 14, starts = ends + segments * 2 + 2;
    unsigned int deltas = starts + segments * 2, ranges = deltas + segments * 2;
    for (unsigned int i = 0; i < segments; i++) {
        if (FontRead16(f, ends + i * 2) < (unsigned int)codepoint) continue;
        unsigned int start = FontRead16(f, starts + i * 2);
        if (start > (unsigned int)codepoint) return false;
        unsigned int delta = FontRead16(f, deltas + i * 2), range = FontRead16(f, ranges + i * 2);
        if (range == 0) return ((codepoint + delta) & 0xFFFF) != 0;
        unsigned int glyph = FontRead16(f, ranges + i * 2 + range + ((unsigned int)codepoint - start) * 2);
        return glyph != 0;
    }
    return false;
}

static bool FontGroupHas(const FontFile *f, int codepoint) {
    unsigned int groups = FontRead32(f, f->cmap + 12);
    for (unsigned int i = 0; i < groups; i++) {
        unsigned int group = f->cmap + 16 + i * FONT_MERGE_GROUP_BYTES;
        unsigned int first = FontRead32(f, group), last = FontRead32(f, group + 4);
        if ((unsigned int)codepoint < first || (unsigned int)codepoint > last) continue;
        return FontRead32(f, group + 8) + ((unsigned int)codepoint - first) != 0;
    }
    return false;
}

static bool FontHasCodepoint(const FontFile *f, int codepoint) {
    if (!f->cmap) return false;
    return f->format == FONT_MERGE_GROUP_FORMAT ? FontGroupHas(f, codepoint) : FontSegmentHas(f, codepoint);
}

static bool FontFileOpen(FontFile *f, unsigned char *data, int size) {
    *f = (FontFile){.data = data, .size = size};
    if (!data || !FontExtractFirstFace(f)) return false;
    FontChooseCmap(f);
    return f->cmap != 0;
}

static void FontFileClose(FontFile *f) {
    MemFree(f->data);
    *f = (FontFile){0};
}

static bool FontWantsFallback(int codepoint) {
    return UiIdeographic(codepoint) || (codepoint >= FONT_MERGE_HANGUL_FIRST && codepoint <= FONT_MERGE_HANGUL_LAST);
}

static int FontAssignOwners(const FontFile *primary, const int *codepoints, int count, int *owners) {
    int missing = 0;
    for (int i = 0; i < count; i++) {
        bool covered = !FontWantsFallback(codepoints[i]) || FontHasCodepoint(primary, codepoints[i]);
        owners[i] = covered ? FONT_MERGE_PRIMARY_OWNER : -1;
        if (!covered) missing++;
    }
    return missing;
}

static int FontLoadFallbacks(FontFile *fallbacks, const int *codepoints, int count, int *owners, int missing) {
    static char paths[FONT_MERGE_FALLBACK_CAP][PLATFORM_FONT_PATH_CAP];
    int path_count = PlatformFallbackFonts(paths, FONT_MERGE_FALLBACK_CAP);
    int loaded = 0;
    for (int p = 0; p < path_count && missing > 0; p++) {
        if (!FileExists(paths[p])) continue;
        int size = 0;
        unsigned char *data = LoadFileData(paths[p], &size);
        FontFile *candidate = &fallbacks[loaded];
        if (!FontFileOpen(candidate, data, size)) {
            if (candidate->data) FontFileClose(candidate);
            continue;
        }
        int claimed = 0;
        for (int i = 0; i < count; i++) {
            if (owners[i] >= 0 || !FontHasCodepoint(candidate, codepoints[i])) continue;
            owners[i] = loaded + 1;
            claimed++;
        }
        if (!claimed) {
            FontFileClose(candidate);
            continue;
        }
        TraceLog(LOG_INFO, "Font: %d glyphs from %s", claimed, paths[p]);
        missing -= claimed;
        loaded++;
    }
    if (missing > 0) TraceLog(LOG_WARNING, "Font: %d glyphs have no font", missing);
    for (int i = 0; i < count; i++)
        if (owners[i] < 0) owners[i] = FONT_MERGE_PRIMARY_OWNER;
    return loaded;
}

static Font
FontBuild(const FontFile *files, int file_count, const int *codepoints, int count, const int *owners, int size) {
    GlyphInfo *glyphs = MemAlloc((unsigned int)(count * sizeof(*glyphs)));
    int *subset = MemAlloc((unsigned int)(count * sizeof(*subset)));
    for (int owner = 0; owner < file_count; owner++) {
        int subset_count = 0;
        for (int i = 0; i < count; i++)
            if (owners[i] == owner) subset[subset_count++] = codepoints[i];
        if (!subset_count) continue;
        GlyphInfo *loaded =
            LoadFontData(files[owner].data, files[owner].size, size, subset, subset_count, FONT_DEFAULT);
        for (int i = 0, next = 0; i < count && loaded; i++)
            if (owners[i] == owner) glyphs[i] = loaded[next++];
        MemFree(loaded);
    }
    MemFree(subset);
    Font font = {.baseSize = size, .glyphCount = count, .glyphPadding = FONT_MERGE_GLYPH_PADDING, .glyphs = glyphs};
    Image atlas = GenImageFontAtlas(glyphs, &font.recs, count, size, font.glyphPadding, 0);
    font.texture = LoadTextureFromImage(atlas);
    for (int i = 0; i < count; i++) {
        UnloadImage(glyphs[i].image);
        glyphs[i].image = ImageFromImage(atlas, font.recs[i]);
    }
    UnloadImage(atlas);
    return font;
}

Font FontLoadWithFallbacks(const char *ref, int size, int *codepoints, int count) {
    int data_size = 0;
    FontFile files[FONT_MERGE_FALLBACK_CAP + 1];
    if (!FontFileOpen(&files[0], ResourceLoadFontData(ref, &data_size), data_size)) {
        if (files[0].data) FontFileClose(&files[0]);
        return ResourceLoadFont(ref, size, codepoints, count);
    }
    int *owners = MemAlloc((unsigned int)(count * sizeof(*owners)));
    int missing = FontAssignOwners(&files[0], codepoints, count, owners);
    int file_count = 1 + (missing ? FontLoadFallbacks(files + 1, codepoints, count, owners, missing) : 0);
    Font font = FontBuild(files, file_count, codepoints, count, owners, size);
    MemFree(owners);
    for (int i = 0; i < file_count; i++) FontFileClose(&files[i]);
    return font;
}
