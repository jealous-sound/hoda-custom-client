#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef HOD_CACHE
#define HOD_CACHE "client-additional-data/"
#endif

#include "config.c"
#include "vendors/cJSON.c"
#include "unity_texture.c"
#include "unity_asset.c"
#include "apk_archive.c"
#include "resource.c"
#include "utils.c"
#include "ui.c"
#include "assets.c"
#include "loc.c"
#include "character.c"
#include "consume.c"
#include "passive.c"
#include "rune.c"
#include "gallery.c"
#include "mission.c"
#include "player.c"
#include "button.c"
#include "energy_dialog.c"
#include "hud.c"
#include "home_ui.c"
#include "home_squad.c"
#include "home_background.c"
#include "consume_background.c"
#include "heroes_background.c"
#include "battle_squad.c"
#include "battle_sim.c"
#include "battle.c"
#include "music_player.c"
#include "heroes.c"
#include "message_popup.c"
#include "runes.c"
#include "consume_view.c"
#include "gallery_view.c"
#include "pack_scene.c"
#include "pack_fx.c"
#include "pack_hero.c"
#include "store.c"
#include "campaign.c"
#include "nexus_background.c"
#include "nexus.c"
#include "quests.c"
#include "progression.c"
#include "mission_progress.c"
#include "battle_play.c"
#include "battle_ui.c"
#include "battle_results.c"
#include "mission_view.c"
#include "game.c"
#include "debug_harness.c"

static void RunGame(void) {
    Game game = {0};
    GameInitialize(&game);

    DebugShot shot = DebugShotFromEnvironment();
    float mouse_x = 0.0f, mouse_y = 0.0f;
    bool mouse_override = DebugMousePosition(&mouse_x, &mouse_y);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE) && !GameHandlePopupBack(&game)) break;
        if (mouse_override) SetMousePosition((int)(mouse_x * UiScale()), (int)(mouse_y * UiScale()));
        if (shot.output) DebugShotPrepare(&game, &shot);

        BeginDrawing();
        GameDraw(&game);
        bool captured = shot.output && DebugShotCapture(&shot);
        EndDrawing();

        if (captured) break;
    }

    GameDestroy(&game);
}

int main(void) {
    bool screenshot_mode = EnvFlag("HOD_SHOT");
    if (screenshot_mode) SetTraceLogLevel(LOG_WARNING);
    int win_w = config.game_width, win_h = config.game_height;
    const char *win = getenv("HOD_WINDOW");
    if (win) sscanf(win, "%dx%d", &win_w, &win_h);

    unsigned int flags = FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI;
    if (screenshot_mode) flags |= FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNFOCUSED;
    SetConfigFlags(flags);
    InitWindow(win_w, win_h, "Heroes of Dragon Age");
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    if (screenshot_mode || EnvFlag("HOD_MUTE")) SetMasterVolume(0.0f);
    SetTargetFPS(144);

    if (EnvFlag("HOD_LOADTEST")) {
        DebugRunModelLoadTest(getenv("HOD_LOADTEST"));
    } else if (EnvFlag("HOD_NEXUS_SCENE")) {
        DebugRenderNexusScene(getenv("HOD_NEXUS_SCENE"));
    } else {
        RunGame();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
