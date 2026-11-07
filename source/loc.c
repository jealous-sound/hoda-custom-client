#define LOC_LANGUAGE_ENV "HOD_LANGUAGE"
#define LOC_PROJECT_DIR "data/localization"
#define LOC_LOCALE_CAP 64
#define LOC_PATH_CAP 512
#define LOC_SEEN_CODEPOINTS 0x10000
#define LOC_LAYER_CAP 32

typedef struct {
    const char *code;
    const char *file;
    const char *native_name;
    const char *name_key;
    bool ideographic;
} LocLanguage;

typedef struct {
    const char *tag;
    const char *code;
} LocLocaleTag;

static const LocLanguage LOC_LANGUAGES[] = {
    {"en", "Loc_ENG_US.txt", "English", "LANGUAGE_ENGLISH", false},
    {"fr", "Loc_FRE_FR.txt", "Fran\u00e7ais", "LANGUAGE_FRENCH", false},
    {"de", "Loc_GER_DE.txt", "Deutsch", "LANGUAGE_GERMAN", false},
    {"it", "Loc_ITA_IT.txt", "Italiano", "LANGUAGE_ITALIAN", false},
    {"es", "Loc_SPA_XM.txt", "Espa\u00f1ol", "LANGUAGE_SPANISH", false},
    {"pt", "Loc_POR_BR.txt", "Portugu\u00eas", "LANGUAGE_PORTUGUESE", false},
    {"nl", "Loc_DUT_NL.txt", "Nederlands", "LANGUAGE_DUTCH", false},
    {"ru", "Loc_RUS_RU.txt", "\u0420\u0443\u0441\u0441\u043a\u0438\u0439", "LANGUAGE_RUSSIAN", false},
    {"ja", "Loc_JPN_JP.txt", "\u65e5\u672c\u8a9e", "LANGUAGE_JAPANESE", true},
    {"ko", "Loc_KOR_KR.txt", "\ud55c\uad6d\uc5b4", "LANGUAGE_KOREAN", true},
    {"zh-hans", "Loc_CHS_CN.txt", "\u7b80\u4f53\u4e2d\u6587", "LANGUAGE_CHINESE_SIMPLIFIED", true},
    {"zh-hant", "Loc_CHT_CN.txt", "\u7e41\u9ad4\u4e2d\u6587", "LANGUAGE_CHINESE_TRADITIONAL", true},
};

static const LocLocaleTag LOC_LOCALE_TAGS[] = {
    {"en", "en"},
    {"fr", "fr"},
    {"de", "de"},
    {"it", "it"},
    {"es", "es"},
    {"pt", "pt"},
    {"nl", "nl"},
    {"ru", "ru"},
    {"ja", "ja"},
    {"ko", "ko"},
    {"zh-hant", "zh-hant"},
    {"zh-tw", "zh-hant"},
    {"zh-hk", "zh-hant"},
    {"zh-mo", "zh-hant"},
    {"zh", "zh-hans"},
};

#define LOC_LANGUAGE_COUNT ((int)(sizeof(LOC_LANGUAGES) / sizeof(LOC_LANGUAGES[0])))
#define LOC_LOCALE_TAG_COUNT ((int)(sizeof(LOC_LOCALE_TAGS) / sizeof(LOC_LOCALE_TAGS[0])))
#define LOC_ENGLISH_CODE "en"
#define LOC_RUSSIAN_CODE "ru"

typedef struct {
    const char *key;
    const char *value;
    int order;
} LocEntry;

typedef struct {
    char *layers[LOC_LAYER_CAP];
    int layer_count;
    LocEntry *entries;
    int count;
    int capacity;
} Loc;

static Loc loc;
static const LocLanguage *loc_language;

static int LocEntryCompare(const void *a, const void *b) {
    const LocEntry *left = a, *right = b;
    int by_key = strcasecmp(left->key, right->key);
    return by_key ? by_key : right->order - left->order;
}

static void LocAddEntry(const char *key, const char *value) {
    if (loc.count == loc.capacity) {
        loc.capacity = loc.capacity ? loc.capacity * 2 : 4096;
        loc.entries = MemRealloc(loc.entries, (unsigned int)(loc.capacity * sizeof(*loc.entries)));
    }
    loc.entries[loc.count] = (LocEntry){key, value, loc.count};
    loc.count++;
}

static bool LocAddLayer(const char *path) {
    if (loc.layer_count == LOC_LAYER_CAP || !FileExists(path)) return false;
    char *data = LoadFileText(path);
    if (!data) return false;
    loc.layers[loc.layer_count++] = data;
    int before = loc.count;
    for (char *line = data; line && *line;) {
        char *next = strchr(line, '\n');
        if (next) *next++ = '\0';
        size_t length = strlen(line);
        if (length && line[length - 1] == '\r') line[length - 1] = '\0';
        char *separator = strchr(line, '|');
        if (separator) {
            *separator = '\0';
            LocAddEntry(line, separator + 1);
        }
        line = next;
    }
    TraceLog(LOG_INFO, "Loc: %d strings from %s", loc.count - before, path);
    return true;
}

static void LocAddProjectLayers(const char *file) {
    if (!DirectoryExists(LOC_PROJECT_DIR)) return;
    FilePathList areas = LoadDirectoryFiles(LOC_PROJECT_DIR);
    qsort(areas.paths, areas.count, sizeof(*areas.paths), (int (*)(const void *, const void *))strcmp);
    char path[LOC_PATH_CAP];
    for (unsigned int i = 0; i < areas.count; i++) {
        if (!DirectoryExists(areas.paths[i])) continue;
        snprintf(path, sizeof(path), "%s/%s", areas.paths[i], file);
        LocAddLayer(path);
    }
    UnloadDirectoryFiles(areas);
}

void LocLoad(const char *path) {
    if (!LocAddLayer(path)) TraceLog(LOG_FATAL, "Loc: failed to load %s", path);
    LocAddProjectLayers(GetFileName(path));
    qsort(loc.entries, loc.count, sizeof(*loc.entries), LocEntryCompare);
}

static const LocEntry *LocFind(const char *key) {
    int low = 0, high = loc.count - 1;
    const LocEntry *found = NULL;
    while (low <= high) {
        int middle = (low + high) / 2;
        int order = strcasecmp(loc.entries[middle].key, key);
        if (order < 0) {
            low = middle + 1;
        } else {
            if (order == 0) found = &loc.entries[middle];
            high = middle - 1;
        }
    }
    return found;
}

const char *LocGet(const char *key) {
    const LocEntry *entry = key ? LocFind(key) : NULL;
    return entry ? entry->value : "";
}

const char *LocGetOr(const char *key, const char *fallback) {
    const char *value = LocGet(key);
    return value[0] ? value : fallback;
}

const char *LocGetAbility(const char *ability_key) {
    char with_suffix[224];
    snprintf(with_suffix, sizeof(with_suffix), "%s_DESCRIPTION", ability_key);
    const char *text = LocGet(with_suffix);
    if (!text[0]) text = LocGet(ability_key);
    return text;
}

static int LocPlaceholderLength(const char *c, int *slot) {
    if (c[0] != '{' || c[1] < '0' || c[1] > '9') return 0;
    *slot = c[1] - '0';
    if (c[2] == '}') return 3;
    if (c[2] != ':') return 0;
    const char *close = strchr(c + 3, '}');
    return close ? (int)(close - c) + 1 : 0;
}

void LocFormatArgs(char *out, size_t size, const char *key, const char *const *args, int arg_count) {
    const char *format = LocGet(key);
    size_t written = 0;
    for (const char *c = format; *c && written + 1 < size;) {
        int slot = -1;
        int length = LocPlaceholderLength(c, &slot);
        if (!length || slot >= arg_count) {
            out[written++] = *c++;
            continue;
        }
        for (const char *insert = args[slot]; *insert && written + 1 < size; insert++) out[written++] = *insert;
        c += length;
    }
    out[written] = '\0';
}

void LocFormat1(char *out, size_t size, const char *key, const char *first) {
    const char *args[] = {first};
    LocFormatArgs(out, size, key, args, 1);
}

void LocFormat2(char *out, size_t size, const char *key, const char *first, const char *second) {
    const char *args[] = {first, second};
    LocFormatArgs(out, size, key, args, 2);
}

void LocFormatInt(char *out, size_t size, const char *key, int value) {
    char number[32];
    snprintf(number, sizeof(number), "%d", value);
    LocFormat1(out, size, key, number);
}

void LocFormatInt2(char *out, size_t size, const char *key, int first, int second) {
    char left[32], right[32];
    snprintf(left, sizeof(left), "%d", first);
    snprintf(right, sizeof(right), "%d", second);
    LocFormat2(out, size, key, left, right);
}

void LocUnload(void) {
    for (int i = 0; i < loc.layer_count; i++) UnloadFileText(loc.layers[i]);
    MemFree(loc.entries);
    loc = (Loc){0};
}

int LocLanguageCount(void) {
    return LOC_LANGUAGE_COUNT;
}

const LocLanguage *LocLanguageAt(int index) {
    return index >= 0 && index < LOC_LANGUAGE_COUNT ? &LOC_LANGUAGES[index] : NULL;
}

static const LocLanguage *LocLanguageForCode(const char *code) {
    for (int i = 0; code && i < LOC_LANGUAGE_COUNT; i++)
        if (!strcmp(LOC_LANGUAGES[i].code, code)) return &LOC_LANGUAGES[i];
    return NULL;
}

static bool LocTagMatches(const char *locale, const char *tag) {
    size_t length = strlen(tag);
    return strncmp(locale, tag, length) == 0 && !isalnum((unsigned char)locale[length]);
}

static const LocLanguage *LocLanguageForLocale(const char *locale) {
    char normalized[LOC_LOCALE_CAP];
    size_t n = 0;
    for (; locale[n] && n + 1 < sizeof(normalized); n++)
        normalized[n] = locale[n] == '_' ? '-' : (char)tolower((unsigned char)locale[n]);
    normalized[n] = '\0';
    for (int i = 0; i < LOC_LOCALE_TAG_COUNT; i++)
        if (LocTagMatches(normalized, LOC_LOCALE_TAGS[i].tag)) return LocLanguageForCode(LOC_LOCALE_TAGS[i].code);
    return NULL;
}

static const LocLanguage *LocSystemLanguage(void) {
    char locale[LOC_LOCALE_CAP];
    return PlatformPreferredLanguage(locale, sizeof(locale)) ? LocLanguageForLocale(locale) : NULL;
}

static const LocLanguage *LocRequestedLanguage(void) {
    const char *forced = getenv(LOC_LANGUAGE_ENV);
    if (forced && forced[0]) return LocLanguageForLocale(forced);
    const LocLanguage *chosen = LocLanguageForCode(SettingsLanguage());
    return chosen ? chosen : LocSystemLanguage();
}

static void LocLanguagePath(const LocLanguage *language, char *out, size_t capacity) {
    snprintf(out, capacity, "%s/%s", ORIGINAL_LOCALIZATION_DIR, language->file);
}

bool LocLanguageInstalled(const LocLanguage *language) {
    char path[LOC_PATH_CAP];
    LocLanguagePath(language, path, sizeof(path));
    return FileExists(path);
}

static const LocLanguage *LocInstalledLanguage(void) {
    const LocLanguage *candidates[] = {
        LocRequestedLanguage(), LocLanguageForCode(LOC_ENGLISH_CODE), LocLanguageForCode(LOC_RUSSIAN_CODE)
    };
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
        if (candidates[i] && LocLanguageInstalled(candidates[i])) return candidates[i];
    return NULL;
}

bool LocPreferredInstalled(void) {
    return LocInstalledLanguage() != NULL;
}

void LocChooseLanguage(void) {
    const LocLanguage *language = LocInstalledLanguage();
    loc_language = language ? language : LocLanguageForCode(LOC_RUSSIAN_CODE);
}

void LocLoadPreferred(void) {
    LocChooseLanguage();
    char path[LOC_PATH_CAP];
    LocLanguagePath(loc_language, path, sizeof(path));
    LocLoad(path);
}

const char *LocLanguageCode(void) {
    return loc_language ? loc_language->code : LOC_RUSSIAN_CODE;
}

bool LocIdeographic(void) {
    return loc_language && loc_language->ideographic;
}

static bool LocCodepointTracked(int codepoint) {
    return codepoint >= ' ' && codepoint < LOC_SEEN_CODEPOINTS;
}

static bool LocMarkCodepoint(unsigned char *seen, int codepoint) {
    unsigned char bit = (unsigned char)(1u << (codepoint % 8));
    bool fresh = !(seen[codepoint / 8] & bit);
    seen[codepoint / 8] |= bit;
    return fresh;
}

static int LocCollectText(const char *text, unsigned char *seen, int *codepoints, int count, int capacity) {
    for (const char *c = text; *c && count < capacity;) {
        int size = 0;
        int codepoint = GetCodepointNext(c, &size);
        c += size > 0 ? size : 1;
        if (LocCodepointTracked(codepoint) && LocMarkCodepoint(seen, codepoint)) codepoints[count++] = codepoint;
    }
    return count;
}

int LocCollectCodepoints(int *codepoints, int count, int capacity, bool native_names) {
    static unsigned char seen[LOC_SEEN_CODEPOINTS / 8];
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < count; i++)
        if (LocCodepointTracked(codepoints[i])) LocMarkCodepoint(seen, codepoints[i]);
    for (int i = 0; native_names && i < LOC_LANGUAGE_COUNT; i++)
        count = LocCollectText(LOC_LANGUAGES[i].native_name, seen, codepoints, count, capacity);
    for (int i = 0; i < loc.count; i++) count = LocCollectText(loc.entries[i].value, seen, codepoints, count, capacity);
    return count;
}
