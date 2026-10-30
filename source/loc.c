
typedef struct {
    char *data;
    const char **keys;
    const char **values;
    int count;
} Loc;

static Loc loc;

void LocLoad(const char *path) {
    loc.data = LoadFileText(path);
    if (!loc.data) TraceLog(LOG_FATAL, "Loc: failed to load %s", path);

    int lines = 0;
    for (char *c = loc.data; *c; c++) {
        if (*c == '\n') lines++;
    }
    lines++;
    loc.keys = (const char **)MemAlloc(lines * sizeof(char *));
    loc.values = (const char **)MemAlloc(lines * sizeof(char *));

    loc.count = 0;
    char *line = loc.data;
    while (line && *line) {
        char *next = strchr(line, '\n');
        if (next) *next++ = '\0';
        char *sep = strchr(line, '|');
        if (sep) {
            *sep = '\0';
            loc.keys[loc.count] = line;
            loc.values[loc.count] = sep + 1;
            loc.count++;
        }
        line = next;
    }
    TraceLog(LOG_INFO, "Loc: %d strings from %s", loc.count, path);
}

const char *LocGet(const char *key) {
    for (int i = 0; i < loc.count; i++) {
        if (!strcasecmp(loc.keys[i], key)) return loc.values[i];
    }
    return "";
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

void LocFormat2(char *out, size_t size, const char *key, const char *first, const char *second) {
    const char *format = LocGet(key);
    size_t written = 0;
    for (const char *c = format; *c && written + 1 < size;) {
        const char *insert = NULL;
        if (c[0] == '{' && c[2] == '}' && (c[1] == '0' || c[1] == '1')) insert = c[1] == '0' ? first : second;
        if (!insert) {
            out[written++] = *c++;
            continue;
        }
        for (; *insert && written + 1 < size; insert++) out[written++] = *insert;
        c += 3;
    }
    out[written] = '\0';
}

void LocUnload(void) {
    MemFree((void *)loc.keys);
    MemFree((void *)loc.values);
    UnloadFileText(loc.data);
    loc = (Loc){0};
}
