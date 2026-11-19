#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>
#include <unistd.h>

#define TEST_TEMPORARY_TEMPLATE "/tmp/hoda-settings-test-XXXXXX"
#define TEST_PATH_CAP 1024

static const Rectangle TEST_TROUGH = {100.0f, 0.0f, 200.0f, 10.0f};

static void TestReloadFrom(const char *json) {
    if (json)
        assert(SaveFileText(SETTINGS_PATH, (char *)json));
    else
        unlink(SETTINGS_PATH);
    settings = (Settings){0};
}

static void TestFreshInstallPlaysEverythingAtFullVolume(void) {
    TestReloadFrom(NULL);
    for (int i = 0; i < SETTINGS_VOLUME_COUNT; i++) assert(SettingsVolumeLevel(i) == 1.0f);
    for (int i = 0; i < SETTINGS_SWITCH_COUNT; i++) assert(!SettingsSwitchOn(i));
    assert(SettingsFrameRateLimit() == SETTINGS_DEFAULT_FRAME_RATE);
    assert(SettingsUiScale() == SETTINGS_FULL_UI_SCALE);
}

static void TestMusicSwitchedOffBeforeVolumesStaysSilent(void) {
    TestReloadFrom("{\"language\":\"de\",\"music\":false,\"sound\":true}");
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_MUSIC) == 0.0f);
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_SOUND) == 1.0f);
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_MASTER) == 1.0f);
    assert(!strcmp(SettingsLanguage(), "de"));
}

static void TestAVolumeLevelWinsOverTheOldSwitch(void) {
    TestReloadFrom("{\"music\":false,\"music_volume\":0.4}");
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_MUSIC) == 0.4f);
}

static void TestOutOfRangeValuesAreClamped(void) {
    TestReloadFrom("{\"master_volume\":1.7,\"sound_volume\":-2,\"frame_rate_limit\":5}");
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_MASTER) == 1.0f);
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_SOUND) == 0.0f);
    assert(SettingsFrameRateLimit() == SETTINGS_MINIMUM_FRAME_RATE);
    TestReloadFrom("{\"frame_rate_limit\":0}");
    assert(SettingsFrameRateLimit() == SETTINGS_UNLIMITED_FRAME_RATE);
    TestReloadFrom("{\"ui_scale\":0.2}");
    assert(SettingsUiScale() == SETTINGS_MINIMUM_UI_SCALE);
    TestReloadFrom("{\"ui_scale\":1.7}");
    assert(SettingsUiScale() == SETTINGS_FULL_UI_SCALE);
    TestReloadFrom("{\"ui_scale\":0.83}");
    assert(SettingsUiScale() == 0.85f);
}

static void TestAVolumeAdjustmentWaitsForTheSave(void) {
    TestReloadFrom(NULL);
    SettingsAdjustVolume(SETTINGS_VOLUME_SOUND, 0.333f);
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_SOUND) == 0.33f);
    assert(!FileExists(SETTINGS_PATH));
    SettingsSaveAdjustments();
    assert(FileExists(SETTINGS_PATH));
    settings = (Settings){0};
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_SOUND) == 0.33f);
}

static void TestEverySettingSurvivesARestart(void) {
    TestReloadFrom(NULL);
    SettingsSetLanguage("ja");
    SettingsAdjustVolume(SETTINGS_VOLUME_MASTER, 0.5f);
    SettingsAdjustVolume(SETTINGS_VOLUME_MUSIC, 0.25f);
    SettingsSaveAdjustments();
    SettingsSetSwitch(SETTINGS_SWITCH_MUTE_IN_BACKGROUND, true);
    SettingsSetSwitch(SETTINGS_SWITCH_FULLSCREEN, true);
    SettingsSetSwitch(SETTINGS_SWITCH_VSYNC, true);
    SettingsSetSwitch(SETTINGS_SWITCH_SHOW_FPS, true);
    SettingsSetFrameRateLimit(60);
    SettingsSetUiScale(0.75f);
    SettingsSetSkippedUpdate("v9.9.9");
    settings = (Settings){0};
    assert(!strcmp(SettingsLanguage(), "ja"));
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_MASTER) == 0.5f);
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_MUSIC) == 0.25f);
    assert(SettingsVolumeLevel(SETTINGS_VOLUME_SOUND) == 1.0f);
    for (int i = 0; i < SETTINGS_SWITCH_COUNT; i++) assert(SettingsSwitchOn(i));
    assert(SettingsFrameRateLimit() == 60);
    assert(SettingsUiScale() == 0.75f);
    assert(!strcmp(SettingsSkippedUpdate(), "v9.9.9"));
}

static void TestTheUiScaleIsStoredAsAWholePercent(void) {
    TestReloadFrom(NULL);
    SettingsSetUiScale(0.85f);
    char *text = LoadFileText(SETTINGS_PATH);
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    assert(cJSON_GetObjectItemCaseSensitive(root, "ui_scale")->valuedouble == 0.85);
    cJSON_Delete(root);
}

static void TestUiScaleArrowsStepFivePercentAndStopAtTheEnds(void) {
    TestReloadFrom(NULL);
    OptionsStepUiScale(OPTIONS_STEP_FORWARD);
    assert(SettingsUiScale() == SETTINGS_FULL_UI_SCALE);
    OptionsStepUiScale(OPTIONS_STEP_BACK);
    assert(SettingsUiScale() == 0.95f);
    assert(ui_chosen_scale == 0.95f);
    for (int step = 0; step < 20; step++) OptionsStepUiScale(OPTIONS_STEP_BACK);
    assert(SettingsUiScale() == SETTINGS_MINIMUM_UI_SCALE);
    OptionsStepUiScale(OPTIONS_STEP_FORWARD);
    assert(SettingsUiScale() == 0.65f);
}

static void TestFrameRateArrowsWalkTheListAndWrap(void) {
    assert(OptionsSteppedFrameRate(144, OPTIONS_STEP_FORWARD) == 240);
    assert(OptionsSteppedFrameRate(144, OPTIONS_STEP_BACK) == 120);
    assert(OptionsSteppedFrameRate(240, OPTIONS_STEP_FORWARD) == SETTINGS_UNLIMITED_FRAME_RATE);
    assert(OptionsSteppedFrameRate(SETTINGS_UNLIMITED_FRAME_RATE, OPTIONS_STEP_FORWARD) == 30);
    assert(OptionsSteppedFrameRate(30, OPTIONS_STEP_BACK) == SETTINGS_UNLIMITED_FRAME_RATE);
}

static void TestAnUnlistedFrameRateStepsToItsNeighbours(void) {
    assert(OptionsSteppedFrameRate(75, OPTIONS_STEP_FORWARD) == 120);
    assert(OptionsSteppedFrameRate(75, OPTIONS_STEP_BACK) == 60);
    assert(OptionsSteppedFrameRate(500, OPTIONS_STEP_FORWARD) == SETTINGS_UNLIMITED_FRAME_RATE);
}

static void TestTheSliderFollowsThePointerAndStopsAtItsEnds(void) {
    assert(OptionsSliderLevelAt(TEST_TROUGH, 150.0f) == 0.25f);
    assert(OptionsSliderLevelAt(TEST_TROUGH, 40.0f) == 0.0f);
    assert(OptionsSliderLevelAt(TEST_TROUGH, 900.0f) == 1.0f);
}

int main(void) {
    char directory[] = TEST_TEMPORARY_TEMPLATE;
    assert(mkdtemp(directory));
    char previous[TEST_PATH_CAP];
    assert(getcwd(previous, sizeof(previous)));
    assert(chdir(directory) == 0);
    SetTraceLogLevel(LOG_WARNING);

    TestFreshInstallPlaysEverythingAtFullVolume();
    TestMusicSwitchedOffBeforeVolumesStaysSilent();
    TestAVolumeLevelWinsOverTheOldSwitch();
    TestOutOfRangeValuesAreClamped();
    TestAVolumeAdjustmentWaitsForTheSave();
    TestEverySettingSurvivesARestart();
    TestTheUiScaleIsStoredAsAWholePercent();
    TestUiScaleArrowsStepFivePercentAndStopAtTheEnds();
    TestFrameRateArrowsWalkTheListAndWrap();
    TestAnUnlistedFrameRateStepsToItsNeighbours();
    TestTheSliderFollowsThePointerAndStopsAtItsEnds();

    unlink(SETTINGS_PATH);
    assert(chdir(previous) == 0);
    rmdir(directory);
    printf("settings tests passed\n");
    return 0;
}
