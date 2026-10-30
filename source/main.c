#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef HOD_CACHE
#define HOD_CACHE "heroes-of-dragons-cache/"
#endif

#include "config.c"
#include "vendors/cJSON.c"
#include "utils.c"
#include "ui.c"
#include "assets.c"
#include "loc.c"
#include "character.c"
#include "passive.c"
#include "player.c"
#include "button.c"
#include "energy_dialog.c"
#include "hud.c"
#include "home_ui.c"
#include "home_squad.c"
#include "home_background.c"
#include "heroes_background.c"
#include "battle_squad.c"
#include "battle_sim.c"
#include "battle.c"
#include "music_player.c"
#include "heroes.c"
#include "pack_scene.c"
#include "pack_fx.c"
#include "pack_hero.c"
#include "store.c"
#include "campaign.c"
#include "nexus_background.c"
#include "nexus.c"
#include "quests.c"
#include "progression.c"
#include "battle_play.c"
#include "battle_ui.c"
#include "battle_results.c"
#include "game.c"

int main(void) {
    if (getenv("HOD_SHOT")) SetTraceLogLevel(LOG_WARNING);
    int win_w = config.game_width, win_h = config.game_height;
    const char *win = getenv("HOD_WINDOW");
    if (win) sscanf(win, "%dx%d", &win_w, &win_h);

    unsigned int flags = FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI;
    if (getenv("HOD_SHOT")) flags |= FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNFOCUSED;
    SetConfigFlags(flags);
    InitWindow(win_w, win_h, "Heroes of Dragons");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(640, 360);
    InitAudioDevice();
    if (getenv("HOD_SHOT") || getenv("HOD_MUTE")) SetMasterVolume(0.0f);
    SetTargetFPS(144);

    const char *loadtest = getenv("HOD_LOADTEST");
    if (loadtest) {
        bool quiet = getenv("HOD_LOADTEST_QUIET") != NULL;
        char *text = LoadFileText(loadtest);
        int pass = 0, fail = 0;
        for (char *line = text ? strtok(text, "\r\n") : NULL; line; line = strtok(NULL, "\r\n")) {
            if (!line[0] || line[0] == '#') continue;
            const char *why = "";
            Model m = LoadModel(line);
            int cc = 0, frames = 0;
            ModelAnimation *cl = LoadModelAnimations(line, &cc);
            if (cl) {
                for (int a = 0; a < cc; a++) {
                    if (frames == 0 && cl[a].frameCount > 0 && IsModelAnimationValid(m, cl[a]))
                        frames = cl[a].frameCount;
                    UnloadModelAnimation(cl[a]);
                }
                MemFree(cl);
            }
            bool ok = m.meshCount > 0 && frames > 0;
            if (m.meshCount <= 0)
                why = "no mesh";
            else if (frames == 0)
                why = "no usable idle clip";
            if (!quiet || !ok)
                printf(
                    "LOADTEST %-4s meshes=%d frames=%d %-20s | %s\n", ok ? "OK" : "FAIL", m.meshCount, frames, why, line
                );
            if (m.meshCount > 0) UnloadModel(m);
            ok ? pass++ : fail++;
        }
        if (text) UnloadFileText(text);
        printf("LOADTEST DONE pass=%d fail=%d\n", pass, fail);
        CloseAudioDevice();
        CloseWindow();
        return 0;
    }

    const char *nexus_scene = getenv("HOD_NEXUS_SCENE");
    if (nexus_scene) {
        static NexusBackground bg;
        NexusBackgroundLoad(&bg, HOD_CACHE "meshes", nexus_scene);
        float art_lo = 0.0f, art_hi = 0.0f;
        NexusBackgroundArtScrollLimits(&bg, &art_lo, &art_hi);
        float scroll = getenv("HOD_NEXUS_SCROLL") ? (float)atof(getenv("HOD_NEXUS_SCROLL")) : NexusBackgroundHomeX(&bg);
        NexusBackgroundSetScroll(&bg, Clamp(scroll, art_lo, art_hi));
        TraceLog(LOG_INFO, "Nexus: %s art scroll limits [%.2f, %.2f]", nexus_scene, art_lo, art_hi);
        for (int frame = 0; frame < 4; frame++) {
            BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(bg.cam);
            NexusBackgroundDraw(&bg);
            EndMode3D();
            EndDrawing();
        }
        const char *out = getenv("HOD_SHOT");
        if (out) TakeScreenshot(GetFileName(out));
        NexusBackgroundUnload(&bg);
        CloseAudioDevice();
        CloseWindow();
        return 0;
    }

    Game game = {0};
    GameInitialize(&game);

    const char *shot = getenv("HOD_SHOT");
    const char *shot_screen = getenv("HOD_SHOT_SCREEN");
    int shot_frames = 0;
    bool shot_flow_started = false;
    bool shot_resource_opened = false;
    int shot_after = getenv("HOD_SHOT_FRAMES") ? atoi(getenv("HOD_SHOT_FRAMES")) : 90;

    const char *mouse = getenv("HOD_MOUSE");
    float mouse_x = 0, mouse_y = 0;
    if (mouse && sscanf(mouse, "%f,%f", &mouse_x, &mouse_y) != 2) mouse = NULL;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE) && !GameHandlePopupBack(&game)) break;
        if (mouse) SetMousePosition((int)(mouse_x * UiScale()), (int)(mouse_y * UiScale()));

        if (shot) {
            game.screen = SCREEN_HEROES;
            if (shot_screen && strcmp(shot_screen, "home") == 0) game.screen = SCREEN_HOME;
            if (shot_screen && strcmp(shot_screen, "quests") == 0) game.screen = SCREEN_QUESTS;
            bool shot_store =
                shot_screen && (!strcmp(shot_screen, "store") || !strcmp(shot_screen, "store_heroes") ||
                                !strcmp(shot_screen, "store_gems") || !strcmp(shot_screen, "pack") ||
                                !strcmp(shot_screen, "pack_reveal") || !strcmp(shot_screen, "pack_summary"));
            if (shot_store) {
                game.screen = SCREEN_STORE;
                if (!shot_flow_started) {
                    StoreOpen(&game.store);
                    if (!strcmp(shot_screen, "store_heroes")) {
                        game.store.view = STORE_VIEW_HEROES;
                    } else if (!strcmp(shot_screen, "store_gems")) {
                        game.store.view = STORE_VIEW_GEMS;
                    } else if (strcmp(shot_screen, "store")) {
                        int pack = getenv("HOD_PACK") ? atoi(getenv("HOD_PACK")) : STORE_PACK_CHAMPION;
                        pack = Clamp(pack, 0, STORE_PACK_COUNT - 1);
                        game.player.gold = 999999;
                        game.player.gems = 999999;
                        if (StoreTryPurchase(&game.store, &game.player, &game.heroes, pack)) {
                            if (!strcmp(shot_screen, "pack_reveal") || !strcmp(shot_screen, "pack_summary")) {
                                if (getenv("HOD_PACK_INDEX")) {
                                    int index = atoi(getenv("HOD_PACK_INDEX"));
                                    game.store.reveal_index = Clamp(index, 0, game.store.result_count - 1);
                                    game.store.loaded_reveal_index = -1;
                                }
                                game.store.view = STORE_VIEW_RESULT;
                                game.store.phase_time = STORE_OPEN_COMPLETE;
                                game.store.result_time = 1.0f;
                            }
                        }
                    }
                    shot_flow_started = true;
                }
                if (getenv("HOD_PACK_TIME") && game.store.view == STORE_VIEW_OPENING)
                    game.store.phase_time = (float)atof(getenv("HOD_PACK_TIME"));
            }
            if (shot_screen && strcmp(shot_screen, "nexus") == 0) {
                game.screen = SCREEN_NEXUS;
                if (!shot_flow_started) {
                    const char *want = getenv("HOD_NEXUS");
                    int event = want ? NexusFindEvent(&game.nexus, want) : 0;
                    if (event < 0) event = 0;
                    int diff = getenv("HOD_NEXUS_DIFF") ? atoi(getenv("HOD_NEXUS_DIFF")) : 0;
                    NexusEnterMap(&game.nexus, &game.player, event, diff);
                    int step = getenv("HOD_NEXUS_STEP") ? atoi(getenv("HOD_NEXUS_STEP")) : 1;
                    const NexusDifficulty *d = NexusActiveDifficulty(&game.nexus);
                    step = Clamp(step, 1, d ? d->node_count : 1);
                    game.player.nexus_run.current_node_step = step;
                    game.player.nexus_run.committed = step > 1;
                    for (int i = 0; i < step - 1; i++) {
                        game.player.nexus_run.nodes[i].wins = 1;
                        game.player.nexus_run.escrow_gold += d->nodes[i].gold;
                        game.player.nexus_run.escrow_xp += d->nodes[i].xp;
                    }
                    NexusEnterMap(&game.nexus, &game.player, event, diff);
                    shot_flow_started = true;
                }
            }
            if (shot_screen && strcmp(shot_screen, "map") == 0) {
                game.screen = SCREEN_MAP;
                const char *map_number = getenv("HOD_MAP");
                if (map_number && !shot_flow_started) {
                    int map_index = Clamp(atoi(map_number) - 1, 0, game.map.map_count - 1);
                    for (int i = 0; i < map_index; i++) {
                        int boss = CampaignBossNodeIndex(&game.map, i);
                        game.player.campaign_progress.node_completions[i][boss] = 1;
                    }
                    CampaignSelectMap(&game.map, &game.player.campaign_progress, map_index);
                    shot_flow_started = true;
                }
            }
            if (shot_screen && strcmp(shot_screen, "nexus_battle") == 0) {
                game.screen = SCREEN_BATTLE;
                if (!game.battle.squads_loaded) {
                    const char *want = getenv("HOD_NEXUS");
                    int event = want ? NexusFindEvent(&game.nexus, want) : 0;
                    if (event < 0) event = 0;
                    int diff = getenv("HOD_NEXUS_DIFF") ? atoi(getenv("HOD_NEXUS_DIFF")) : 0;
                    NexusEnterMap(&game.nexus, &game.player, event, diff);
                    const NexusDifficulty *d = NexusActiveDifficulty(&game.nexus);
                    if (!d) TraceLog(LOG_FATAL, "HOD_NEXUS_DIFF=%d is out of range for this event", diff);
                    int step = getenv("HOD_NEXUS_STEP") ? atoi(getenv("HOD_NEXUS_STEP")) : 1;
                    game.nexus.selected_node = (int)Clamp((float)(step - 1), 0.0f, (float)(d->node_count - 1));
                    game.battle_is_nexus = true;
                    game.battle_node_index = game.nexus.selected_node;
                    GameEnterSelectedNexusBattle(&game);
                }
            }
            bool shot_battle =
                shot_screen && (!strcmp(shot_screen, "battle") || !strcmp(shot_screen, "levelup") ||
                                !strcmp(shot_screen, "completion") || !strcmp(shot_screen, "results") ||
                                !strcmp(shot_screen, "completion_mastery") || !strcmp(shot_screen, "results_repeat") ||
                                !strcmp(shot_screen, "results_loss"));
            if (shot_battle) {
                game.screen = SCREEN_BATTLE;
                if (!game.battle.squads_loaded) {
                    char map_id[32] = "PVE_MAP_01";
                    int step = 1;
                    const char *node = getenv("HOD_NODE");
                    if (node) sscanf(node, "%31[^:]:%d", map_id, &step);
                    BattleEnterNode(&game.battle, &game.player, map_id, step);
                    BattleUiEnterNode(
                        &game.battle_ui,
                        &game.player,
                        &game.battle.enemy_roster,
                        CampaignFindNode(&game.map, map_id, step),
                        map_id,
                        step
                    );
                    const char *sim_seed = getenv("HOD_SIM");
                    if (sim_seed) {
                        static BattleSim sim;
                        BattleSimRun(&sim, &game.player, &game.battle.enemy_roster, (unsigned)atoi(sim_seed));
                        BattleSimDump(&sim);
                    }
                    const char *fight_seed = getenv("HOD_FIGHT");
                    if (fight_seed && !BattlePlayActive(&game.battle_play)) {
                        BattlePlayStart(&game.battle_play, &game.battle, &game.player, (unsigned)atoi(fight_seed));
                        if (getenv("HOD_FAST")) BattlePlayToggleSpeed(&game.battle_play);
                        const char *splash = getenv("HOD_SPLASH");
                        if (splash) {
                            game.battle_play.won = strncmp(splash, "win", 3) == 0;
                            game.battle_play.state = strstr(splash, "summary") ? BP_SUMMARY : BP_END_SPLASH;
                        }
                    }
                    if (getenv("HOD_TACTICAL")) BattleUiSetTactical(&game.battle_ui, &game.battle, true);
                    const char *plate = getenv("HOD_PLATE");
                    int pside = 0, pslot = 0;
                    if (plate && sscanf(plate, "%d,%d", &pside, &pslot) == 2 && pside >= 0 && pside < 2 && pslot >= 0 &&
                        pslot < SQUAD_SLOT_COUNT && game.battle_ui.plates[pside][pslot].ch) {
                        const BattlePlate *bp = &game.battle_ui.plates[pside][pslot];
                        BattleUiOpenPlateDetails(&game.battle_ui, bp);
                    }
                }
                if (!shot_flow_started && strcmp(shot_screen, "battle")) {
                    const int debug_wins_to_master = 3;
                    BattleResultsPresentation presentation = {
                        .victory = strcmp(shot_screen, "results_loss") != 0,
                        .xp = strcmp(shot_screen, "results_loss") ? 20 : 0,
                        .gold = strcmp(shot_screen, "results_loss") ? 140 : 0,
                        .completion_gold = strcmp(shot_screen, "results_loss") ? 140 : 0,
                        .completion_mastery_gems = 0,
                        .gems = 0,
                        .completions_pre = 1,
                        .completions_post = 2,
                        .wins_to_master = debug_wins_to_master,
                    };
                    if (!strcmp(shot_screen, "levelup")) {
                        presentation.player_level_pre = 1;
                        presentation.player_level_post = 2;
                        presentation.energy_max_delta = 1;
                    } else if (!strcmp(shot_screen, "completion")) {
                        presentation.first_win = true;
                        presentation.completions_pre = 0;
                        presentation.completions_post = 1;
                    } else if (!strcmp(shot_screen, "completion_mastery")) {
                        presentation.mastered = true;
                        presentation.gems = CAMPAIGN_OFFLINE_MASTERY_GEMS;
                        presentation.completion_mastery_gems = CAMPAIGN_OFFLINE_MASTERY_GEMS;
                        presentation.completions_pre = debug_wins_to_master - 1;
                        presentation.completions_post = debug_wins_to_master;
                    } else if (!strcmp(shot_screen, "results_repeat")) {
                        presentation.completions_pre = debug_wins_to_master;
                        presentation.completions_post = debug_wins_to_master + 1;
                    }
                    BattleResultsFlowStart(&game.battle_results, presentation);
                    shot_flow_started = true;
                }
                const char *splash_freeze = getenv("HOD_SPLASH");
                if (splash_freeze && !strstr(splash_freeze, "summary") && BattlePlayActive(&game.battle_play)) {
                    game.battle_play.state = BP_END_SPLASH;
                    game.battle_play.t = 1.0f;
                }
            }
            if (shot_screen && (!strcmp(shot_screen, "map_progress") || !strcmp(shot_screen, "map_progress_new"))) {
                game.screen = SCREEN_MAP;
                if (!shot_flow_started) {
                    if (!strcmp(shot_screen, "map_progress_new")) {
                        int boss = CampaignBossNodeIndex(&game.map, 0);
                        for (int i = 0; i <= boss; i++) game.player.campaign_progress.node_completions[0][i] = 1;
                        game.player.campaign_progress.campaign_index = 1;
                        game.player.campaign_progress.node_index = 0;
                        CampaignBeginProgressionAnimation(&game.map, 0, boss, 1, 0);
                    } else {
                        game.player.campaign_progress.node_completions[0][0] = 1;
                        game.player.campaign_progress.campaign_index = 0;
                        game.player.campaign_progress.node_index = 1;
                        CampaignBeginProgressionAnimation(&game.map, 0, 0, 0, 1);
                    }
                    shot_flow_started = true;
                }
            }
            const char *sel = getenv("HOD_SELECT");
            if (sel) game.heroes.selected = atoi(sel);

            const char *resource = getenv("HOD_RESOURCE");
            if (resource && !shot_resource_opened) {
                bool stamina = strcmp(resource, "stamina") == 0;
                EnergyDialogOpen(stamina ? &game.stamina_dialog : &game.energy_dialog);
                game.stamina_dialog_top = stamina;
                shot_resource_opened = true;
            }

            const char *bfx = getenv("HOD_BFX");
            int bfx_age = getenv("HOD_BFX_AGE") ? atoi(getenv("HOD_BFX_AGE")) : 30;
            if (bfx && game.screen == SCREEN_BATTLE && shot_frames == shot_after - bfx_age) {
                Vector3 origin = {-0.526f, 1.87f, 15.886f};
                Vector3 target;
                const char *target_text = getenv("HOD_BFX_TARGET");
                if (target_text && sscanf(target_text, "%f,%f,%f", &target.x, &target.y, &target.z) == 3)
                    BattleFxBurstDirected(&game.battle, bfx, origin, target);
                else
                    BattleFxBurst(&game.battle, bfx, origin);
            }
        }

        BeginDrawing();
        GameDraw(&game);
        bool take_shot = shot && ++shot_frames >= shot_after;
        if (take_shot) {
            rlDrawRenderBatchActive();
            TakeScreenshot(shot);
        }
        EndDrawing();

        if (take_shot) break;
    }

    GameDestroy(&game);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
