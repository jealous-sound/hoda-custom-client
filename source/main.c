#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "platform.h"
#include "client_version.h"

#define APP_NAME "Heroes of Dragon Age"

#ifndef HOD_CACHE
#define HOD_CACHE "client-additional-data/"
#endif

#include "config.c"
#include "vendors/cJSON.c"
#include "unity_texture.c"
#include "unity_asset.c"
#include "apk_archive.c"
#include "resource.c"
#include "app_bundle.c"
#include "utils.c"
#include "sha256.c"
#include "settings.c"
#include "content_update.c"
#include "client_update.c"
#include "backend.c"
#include "server_actions.c"
#include "ui.c"
#include "loc.c"
#include "font_merge.c"
#include "assets.c"
#include "loading_screen.c"
#include "character.c"
#include "consume.c"
#include "passive.c"
#include "rune.c"
#include "gear.c"
#include "gallery.c"
#include "mission.c"
#include "expedition.c"
#include "timed_quest.c"
#include "pvp_state.c"
#include "player.c"
#include "fortify.c"
#include "tutorial_hooks.c"
#include "button.c"
#include "energy_dialog.c"
#include "options_popup.c"
#include "hud.c"
#include "home_ui.c"
#include "home_squad.c"
#include "home_background.c"
#include "consume_background.c"
#include "heroes_background.c"
#include "stat_budget.c"
#include "battle_squad.c"
#include "battle_sim.c"
#include "pvp.c"
#include "battle.c"
#include "music_player.c"
#include "heroes.c"
#include "message_popup.c"
#include "content_screen.c"
#include "play_mode_screen.c"
#include "name_prompt.c"
#include "maintenance_notice.c"
#include "runes.c"
#include "gear_view.c"
#include "consume_view.c"
#include "fortify_view.c"
#include "gallery_view.c"
#include "pack_scene.c"
#include "pack_fx.c"
#include "pack_hero.c"
#include "store.c"
#include "campaign.c"
#include "nexus_background.c"
#include "nexus.c"
#include "nexus_recap.c"
#include "quests.c"
#include "news.c"
#include "progression.c"
#include "expedition_rules.c"
#include "timed_quest_rules.c"
#include "mission_progress.c"
#include "battle_play.c"
#include "battle_replay.c"
#include "battle_ui.c"
#include "battle_results.c"
#include "goals_tabs.c"
#include "timed_quest_view.c"
#include "mission_view.c"
#include "pvp_hub.c"
#include "pvp_event_view.c"
#include "text_field.c"
#include "guild.c"
#include "guild_view.c"
#include "expedition_view.c"
#include "tutorial.c"
#include "game.c"
#include "debug_harness.c"

static bool RunGameSession(void) {
    Game game = {0};
    if (!GameInitialize(&game)) return false;

    DebugShot shot = DebugShotFromEnvironment();
    float mouse_x = 0.0f, mouse_y = 0.0f;
    bool mouse_override = DebugMousePosition(&mouse_x, &mouse_y);

    while (!WindowShouldClose()) {
        bool server_wait = GameBusyOverlayUpdate(&game, GetFrameTime());
        if (!server_wait && IsKeyPressed(KEY_ESCAPE) && !GameHandlePopupBack(&game)) break;
        if (mouse_override) SetMousePosition((int)(mouse_x * UiScale()), (int)(mouse_y * UiScale()));
        if (shot.output) DebugShotPrepare(&game, &shot);

        BeginDrawing();
        GameDraw(&game);
        GameBusyOverlayDraw();
        bool captured = shot.output && DebugShotCapture(&shot);
        EndDrawing();

        if (captured || game.restart_requested || game.quit_requested) break;
    }

    bool restart = game.restart_requested;
    GameDestroy(&game);
    return restart;
}

static void RunGame(void) {
    while (RunGameSession()) {}
}

static void RunSelectedMode(void) {
    if (EnvFlag("HOD_LOADTEST")) {
        DebugRunModelLoadTest(getenv("HOD_LOADTEST"));
    } else if (EnvFlag("HOD_NEXUS_SCENE")) {
        DebugRenderNexusScene(getenv("HOD_NEXUS_SCENE"));
    } else if (DebugRenderPlayMode(getenv("HOD_SHOT_SCREEN"), getenv("HOD_SHOT"))) {
        return;
    } else if (PlayModeScreenRun()) {
        RunGame();
    }
}

int main(void) {
    AppBundleEnterParentDirectory();
    bool screenshot_mode = EnvFlag("HOD_SHOT");
    if (screenshot_mode) SetTraceLogLevel(LOG_WARNING);
    int win_w = config.game_width, win_h = config.game_height;
    const char *win = getenv("HOD_WINDOW");
    if (win) sscanf(win, "%dx%d", &win_w, &win_h);

    unsigned int flags = FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI;
    if (screenshot_mode) flags |= FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNFOCUSED;
    SetConfigFlags(flags);
    InitWindow(win_w, win_h, APP_NAME);
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    if (screenshot_mode || EnvFlag("HOD_MUTE")) SetMasterVolume(0.0f);
    SetTargetFPS(144);

    if (!screenshot_mode) ClientUpdateCheckStart(BackendUrlOverride() ? BackendUrlOverride() : BackendDefaultUrl());
    if (ContentScreenRun()) RunSelectedMode();

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
