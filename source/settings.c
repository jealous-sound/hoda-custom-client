#define SETTINGS_PATH "settings.json"
#define SETTINGS_TEMPORARY_PATH SETTINGS_PATH ".tmp"
#define SETTINGS_LANGUAGE_CAP 16

typedef struct {
    bool loaded;
    char language[SETTINGS_LANGUAGE_CAP];
    bool music;
    bool sound;
} Settings;

static Settings settings;

static bool SettingsFlag(const cJSON *root, const char *name, bool fallback) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    return cJSON_IsBool(item) ? cJSON_IsTrue(item) : fallback;
}

static void SettingsLoad(void) {
    if (settings.loaded) return;
    settings = (Settings){.loaded = true, .music = true, .sound = true};
    char *text = FileExists(SETTINGS_PATH) ? LoadFileText(SETTINGS_PATH) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    UnloadFileText(text);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return;
    }
    StrCopy(settings.language, sizeof(settings.language), JsonStr(root, "language"));
    settings.music = SettingsFlag(root, "music", true);
    settings.sound = SettingsFlag(root, "sound", true);
    cJSON_Delete(root);
}

static bool SettingsSave(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "language", settings.language);
    cJSON_AddBoolToObject(root, "music", settings.music);
    cJSON_AddBoolToObject(root, "sound", settings.sound);
    char *text = cJSON_Print(root);
    cJSON_Delete(root);
    bool saved = text && SaveFileText(SETTINGS_TEMPORARY_PATH, text) &&
                 PlatformReplaceFile(SETTINGS_TEMPORARY_PATH, SETTINGS_PATH);
    cJSON_free(text);
    if (!saved) TraceLog(LOG_WARNING, "Settings: could not save %s", SETTINGS_PATH);
    return saved;
}

const char *SettingsLanguage(void) {
    SettingsLoad();
    return settings.language;
}

bool SettingsMusicEnabled(void) {
    SettingsLoad();
    return settings.music;
}

bool SettingsSoundEnabled(void) {
    SettingsLoad();
    return settings.sound;
}

void SettingsSetLanguage(const char *code) {
    SettingsLoad();
    StrCopy(settings.language, sizeof(settings.language), code ? code : "");
    SettingsSave();
}

void SettingsSetMusic(bool enabled) {
    SettingsLoad();
    settings.music = enabled;
    SettingsSave();
}

void SettingsSetSound(bool enabled) {
    SettingsLoad();
    settings.sound = enabled;
    SettingsSave();
}
