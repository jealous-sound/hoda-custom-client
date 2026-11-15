#define SETTINGS_PATH "settings.json"
#define SETTINGS_TEMPORARY_PATH SETTINGS_PATH ".tmp"
#define SETTINGS_LANGUAGE_CAP 16
#define SETTINGS_VERSION_CAP 64
#define SETTINGS_FULL_VOLUME 1.0f
#define SETTINGS_VOLUME_STEPS 100.0
#define SETTINGS_UNLIMITED_FRAME_RATE 0
#define SETTINGS_DEFAULT_FRAME_RATE 144
#define SETTINGS_MINIMUM_FRAME_RATE 15
#define SETTINGS_MAXIMUM_FRAME_RATE 1000

typedef enum {
    SETTINGS_VOLUME_MASTER,
    SETTINGS_VOLUME_MUSIC,
    SETTINGS_VOLUME_SOUND,
    SETTINGS_VOLUME_COUNT,
} SettingsVolume;

typedef enum {
    SETTINGS_SWITCH_MUTE_IN_BACKGROUND,
    SETTINGS_SWITCH_FULLSCREEN,
    SETTINGS_SWITCH_VSYNC,
    SETTINGS_SWITCH_SHOW_FPS,
    SETTINGS_SWITCH_COUNT,
} SettingsSwitch;

typedef struct {
    const char *key;
    const char *on_off_key_before_volumes;
} SettingsVolumeField;

static const SettingsVolumeField SETTINGS_VOLUME_FIELDS[SETTINGS_VOLUME_COUNT] = {
    [SETTINGS_VOLUME_MASTER] = {"master_volume", NULL},
    [SETTINGS_VOLUME_MUSIC] = {"music_volume", "music"},
    [SETTINGS_VOLUME_SOUND] = {"sound_volume", "sound"},
};

static const char *const SETTINGS_SWITCH_KEYS[SETTINGS_SWITCH_COUNT] = {
    [SETTINGS_SWITCH_MUTE_IN_BACKGROUND] = "mute_in_background",
    [SETTINGS_SWITCH_FULLSCREEN] = "fullscreen",
    [SETTINGS_SWITCH_VSYNC] = "vsync",
    [SETTINGS_SWITCH_SHOW_FPS] = "show_fps",
};

typedef struct {
    bool loaded;
    bool unsaved;
    char language[SETTINGS_LANGUAGE_CAP];
    float volume[SETTINGS_VOLUME_COUNT];
    bool switches[SETTINGS_SWITCH_COUNT];
    int frame_rate_limit;
    char skipped_update[SETTINGS_VERSION_CAP];
} Settings;

static Settings settings;

static Settings SettingsDefaults(void) {
    Settings defaults = {.loaded = true, .frame_rate_limit = SETTINGS_DEFAULT_FRAME_RATE};
    for (int i = 0; i < SETTINGS_VOLUME_COUNT; i++) defaults.volume[i] = SETTINGS_FULL_VOLUME;
    return defaults;
}

static double SettingsVolumeStep(double level) {
    return round(level * SETTINGS_VOLUME_STEPS) / SETTINGS_VOLUME_STEPS;
}

static float SettingsValidVolume(double level) {
    return (float)SettingsVolumeStep(Clamp((float)level, 0.0f, SETTINGS_FULL_VOLUME));
}

static float SettingsVolumeFrom(const cJSON *root, SettingsVolumeField field) {
    const cJSON *level = cJSON_GetObjectItemCaseSensitive(root, field.key);
    if (cJSON_IsNumber(level)) return SettingsValidVolume(level->valuedouble);
    if (!field.on_off_key_before_volumes) return SETTINGS_FULL_VOLUME;
    const cJSON *enabled = cJSON_GetObjectItemCaseSensitive(root, field.on_off_key_before_volumes);
    return cJSON_IsFalse(enabled) ? 0.0f : SETTINGS_FULL_VOLUME;
}

static int SettingsValidFrameRate(int frame_rate) {
    if (frame_rate == SETTINGS_UNLIMITED_FRAME_RATE) return frame_rate;
    if (frame_rate < SETTINGS_MINIMUM_FRAME_RATE) return SETTINGS_MINIMUM_FRAME_RATE;
    return frame_rate > SETTINGS_MAXIMUM_FRAME_RATE ? SETTINGS_MAXIMUM_FRAME_RATE : frame_rate;
}

static int SettingsFrameRateFrom(const cJSON *root) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "frame_rate_limit");
    return cJSON_IsNumber(item) ? SettingsValidFrameRate(item->valueint) : SETTINGS_DEFAULT_FRAME_RATE;
}

static void SettingsLoad(void) {
    if (settings.loaded) return;
    settings = SettingsDefaults();
    char *text = FileExists(SETTINGS_PATH) ? LoadFileText(SETTINGS_PATH) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    UnloadFileText(text);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return;
    }
    StrCopy(settings.language, sizeof(settings.language), JsonStr(root, "language"));
    for (int i = 0; i < SETTINGS_VOLUME_COUNT; i++)
        settings.volume[i] = SettingsVolumeFrom(root, SETTINGS_VOLUME_FIELDS[i]);
    for (int i = 0; i < SETTINGS_SWITCH_COUNT; i++) settings.switches[i] = JsonBool(root, SETTINGS_SWITCH_KEYS[i]);
    settings.frame_rate_limit = SettingsFrameRateFrom(root);
    StrCopy(settings.skipped_update, sizeof(settings.skipped_update), JsonStr(root, "skipped_update"));
    cJSON_Delete(root);
}

static bool SettingsSave(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "language", settings.language);
    for (int i = 0; i < SETTINGS_VOLUME_COUNT; i++)
        cJSON_AddNumberToObject(root, SETTINGS_VOLUME_FIELDS[i].key, SettingsVolumeStep(settings.volume[i]));
    for (int i = 0; i < SETTINGS_SWITCH_COUNT; i++)
        cJSON_AddBoolToObject(root, SETTINGS_SWITCH_KEYS[i], settings.switches[i]);
    cJSON_AddNumberToObject(root, "frame_rate_limit", settings.frame_rate_limit);
    if (settings.skipped_update[0]) cJSON_AddStringToObject(root, "skipped_update", settings.skipped_update);
    char *text = cJSON_Print(root);
    cJSON_Delete(root);
    bool saved = text && SaveFileText(SETTINGS_TEMPORARY_PATH, text) &&
                 PlatformReplaceFile(SETTINGS_TEMPORARY_PATH, SETTINGS_PATH);
    cJSON_free(text);
    if (!saved) TraceLog(LOG_WARNING, "Settings: could not save %s", SETTINGS_PATH);
    settings.unsaved = !saved;
    return saved;
}

const char *SettingsLanguage(void) {
    SettingsLoad();
    return settings.language;
}

void SettingsSetLanguage(const char *code) {
    SettingsLoad();
    StrCopy(settings.language, sizeof(settings.language), code ? code : "");
    SettingsSave();
}

float SettingsVolumeLevel(SettingsVolume volume) {
    SettingsLoad();
    return settings.volume[volume];
}

void SettingsAdjustVolume(SettingsVolume volume, float level) {
    SettingsLoad();
    float valid = SettingsValidVolume(level);
    if (settings.volume[volume] == valid) return;
    settings.volume[volume] = valid;
    settings.unsaved = true;
}

void SettingsSaveAdjustments(void) {
    if (settings.loaded && settings.unsaved) SettingsSave();
}

bool SettingsSwitchOn(SettingsSwitch setting) {
    SettingsLoad();
    return settings.switches[setting];
}

void SettingsSetSwitch(SettingsSwitch setting, bool on) {
    SettingsLoad();
    settings.switches[setting] = on;
    SettingsSave();
}

int SettingsFrameRateLimit(void) {
    SettingsLoad();
    return settings.frame_rate_limit;
}

void SettingsSetFrameRateLimit(int frame_rate) {
    SettingsLoad();
    settings.frame_rate_limit = SettingsValidFrameRate(frame_rate);
    SettingsSave();
}

const char *SettingsSkippedUpdate(void) {
    SettingsLoad();
    return settings.skipped_update;
}

void SettingsSetSkippedUpdate(const char *version) {
    SettingsLoad();
    StrCopy(settings.skipped_update, sizeof(settings.skipped_update), version ? version : "");
    SettingsSave();
}
