typedef struct {
    const char *output;
    const char *screen;
    int frames_drawn;
    int capture_frame;
    bool flow_started;
    bool resource_opened;
} DebugShot;

#define DEBUG_SHOT_DEFAULT_FRAMES 90
#define DEBUG_BFX_DEFAULT_AGE 30
#define DEBUG_PACK_FUNDS 999999
#define DEBUG_WINS_TO_MASTER 3
#define DEBUG_RESULTS_XP 20
#define DEBUG_RESULTS_GOLD 140

static const Vector3 DEBUG_BFX_ORIGIN = {-0.526f, 1.87f, 15.886f};

static DebugShot DebugShotFromEnvironment(void) {
    return (DebugShot){
        .output = getenv("HOD_SHOT"),
        .screen = getenv("HOD_SHOT_SCREEN"),
        .capture_frame = EnvInt("HOD_SHOT_FRAMES", DEBUG_SHOT_DEFAULT_FRAMES),
    };
}

static bool DebugShotIs(const DebugShot *shot, const char *name) {
    return shot->screen && strcmp(shot->screen, name) == 0;
}

static bool DebugShotIsAny(const DebugShot *shot, const char *const *names, int count) {
    for (int i = 0; i < count; i++)
        if (DebugShotIs(shot, names[i])) return true;
    return false;
}

static bool DebugMousePosition(float *x, float *y) {
    const char *mouse = getenv("HOD_MOUSE");
    return mouse && sscanf(mouse, "%f,%f", x, y) == 2;
}

static void DebugRunModelLoadTest(const char *list_path) {
    bool quiet = EnvFlag("HOD_LOADTEST_QUIET");
    char *text = LoadFileText(list_path);
    int pass = 0, fail = 0;
    for (char *line = text ? strtok(text, "\r\n") : NULL; line; line = strtok(NULL, "\r\n")) {
        if (!line[0] || line[0] == '#') continue;
        Model model = LoadModel(line);
        int clip_count = 0, frames = 0;
        ModelAnimation *clips = LoadModelAnimations(line, &clip_count);
        if (clips) {
            for (int i = 0; i < clip_count; i++) {
                if (frames == 0 && clips[i].frameCount > 0 && IsModelAnimationValid(model, clips[i]))
                    frames = clips[i].frameCount;
                UnloadModelAnimation(clips[i]);
            }
            MemFree(clips);
        }
        bool ok = model.meshCount > 0 && frames > 0;
        const char *why = "";
        if (model.meshCount <= 0)
            why = "no mesh";
        else if (frames == 0)
            why = "no usable idle clip";
        if (!quiet || !ok)
            printf("LOADTEST %-4s meshes=%d frames=%d %-20s | %s\n", ok ? "OK" : "FAIL", model.meshCount, frames, why, line);
        if (model.meshCount > 0) UnloadModel(model);
        ok ? pass++ : fail++;
    }
    if (text) UnloadFileText(text);
    printf("LOADTEST DONE pass=%d fail=%d\n", pass, fail);
}

static void DebugRenderNexusScene(const char *scene_name) {
    static NexusBackground bg;
    NexusBackgroundLoad(&bg, HOD_CACHE "meshes", scene_name);
    float art_lo = 0.0f, art_hi = 0.0f;
    NexusBackgroundArtScrollLimits(&bg, &art_lo, &art_hi);
    NexusBackgroundSetScroll(&bg, Clamp(EnvFloat("HOD_NEXUS_SCROLL", NexusBackgroundHomeX(&bg)), art_lo, art_hi));
    TraceLog(LOG_INFO, "Nexus: %s art scroll limits [%.2f, %.2f]", scene_name, art_lo, art_hi);
    for (int frame = 0; frame < 4; frame++) {
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(bg.cam);
        NexusBackgroundDraw(&bg);
        EndMode3D();
        EndDrawing();
    }
    const char *output = getenv("HOD_SHOT");
    if (output) TakeScreenshot(GetFileName(output));
    NexusBackgroundUnload(&bg);
}

static void DebugPrepareStore(Game *game, DebugShot *shot) {
    static const char *const names[] = {"store", "store_heroes", "store_gems", "pack", "pack_reveal", "pack_summary"};
    if (!DebugShotIsAny(shot, names, 6)) return;
    game->screen = SCREEN_STORE;
    if (!shot->flow_started) {
        StoreOpen(&game->store);
        if (DebugShotIs(shot, "store_heroes")) {
            game->store.view = STORE_VIEW_HEROES;
        } else if (DebugShotIs(shot, "store_gems")) {
            game->store.view = STORE_VIEW_GEMS;
        } else if (!DebugShotIs(shot, "store")) {
            int pack = Clamp(EnvInt("HOD_PACK", STORE_PACK_CHAMPION), 0, STORE_PACK_COUNT - 1);
            game->player.gold = DEBUG_PACK_FUNDS;
            game->player.gems = DEBUG_PACK_FUNDS;
            bool purchased = StoreTryPurchase(&game->store, &game->player, &game->heroes, pack);
            if (purchased && (DebugShotIs(shot, "pack_reveal") || DebugShotIs(shot, "pack_summary"))) {
                if (EnvFlag("HOD_PACK_INDEX")) {
                    game->store.reveal_index = Clamp(EnvInt("HOD_PACK_INDEX", 0), 0, game->store.result_count - 1);
                    game->store.loaded_reveal_index = -1;
                }
                game->store.view = STORE_VIEW_RESULT;
                game->store.phase_time = STORE_OPEN_COMPLETE;
                game->store.result_time = 1.0f;
            }
        }
        shot->flow_started = true;
    }
    if (EnvFlag("HOD_PACK_TIME") && game->store.view == STORE_VIEW_OPENING)
        game->store.phase_time = EnvFloat("HOD_PACK_TIME", 0.0f);
}

static int DebugNexusEvent(const Game *game) {
    const char *want = getenv("HOD_NEXUS");
    int event = want ? NexusFindEvent(&game->nexus, want) : 0;
    return event < 0 ? 0 : event;
}

static void DebugPrepareNexus(Game *game, DebugShot *shot) {
    if (!DebugShotIs(shot, "nexus")) return;
    game->screen = SCREEN_NEXUS;
    if (shot->flow_started) return;
    int event = DebugNexusEvent(game);
    int difficulty = EnvInt("HOD_NEXUS_DIFF", 0);
    NexusEnterMap(&game->nexus, &game->player, event, difficulty);
    const NexusDifficulty *d = NexusActiveDifficulty(&game->nexus);
    int step = Clamp(EnvInt("HOD_NEXUS_STEP", 1), 1, d ? d->node_count : 1);
    game->player.nexus_run.current_node_step = step;
    game->player.nexus_run.committed = step > 1;
    for (int i = 0; i < step - 1; i++) {
        game->player.nexus_run.nodes[i].wins = 1;
        game->player.nexus_run.escrow_gold += d->nodes[i].gold;
        game->player.nexus_run.escrow_xp += d->nodes[i].xp;
    }
    NexusEnterMap(&game->nexus, &game->player, event, difficulty);
    shot->flow_started = true;
}

static void DebugPrepareCampaignMap(Game *game, DebugShot *shot) {
    if (!DebugShotIs(shot, "map")) return;
    game->screen = SCREEN_MAP;
    const char *map_number = getenv("HOD_MAP");
    if (!map_number || shot->flow_started) return;
    int map_index = Clamp(atoi(map_number) - 1, 0, game->map.map_count - 1);
    for (int i = 0; i < map_index; i++) {
        int boss = CampaignBossNodeIndex(&game->map, i);
        game->player.campaign_progress.node_completions[i][boss] = 1;
    }
    CampaignSelectMap(&game->map, &game->player.campaign_progress, map_index);
    shot->flow_started = true;
}

static void DebugPrepareMapProgress(Game *game, DebugShot *shot) {
    bool is_new = DebugShotIs(shot, "map_progress_new");
    if (!is_new && !DebugShotIs(shot, "map_progress")) return;
    game->screen = SCREEN_MAP;
    if (shot->flow_started) return;
    PlayerCampaignProgress *progress = &game->player.campaign_progress;
    if (is_new) {
        int boss = CampaignBossNodeIndex(&game->map, 0);
        for (int i = 0; i <= boss; i++) progress->node_completions[0][i] = 1;
        progress->campaign_index = 1;
        progress->node_index = 0;
        CampaignBeginProgressionAnimation(&game->map, 0, boss, 1, 0);
    } else {
        progress->node_completions[0][0] = 1;
        progress->campaign_index = 0;
        progress->node_index = 1;
        CampaignBeginProgressionAnimation(&game->map, 0, 0, 0, 1);
    }
    shot->flow_started = true;
}

static void DebugPrepareNexusBattle(Game *game, const DebugShot *shot) {
    if (!DebugShotIs(shot, "nexus_battle")) return;
    game->screen = SCREEN_BATTLE;
    if (game->battle.squads_loaded) return;
    int difficulty = EnvInt("HOD_NEXUS_DIFF", 0);
    NexusEnterMap(&game->nexus, &game->player, DebugNexusEvent(game), difficulty);
    const NexusDifficulty *d = NexusActiveDifficulty(&game->nexus);
    if (!d) TraceLog(LOG_FATAL, "HOD_NEXUS_DIFF=%d is out of range for this event", difficulty);
    int step = EnvInt("HOD_NEXUS_STEP", 1);
    game->nexus.selected_node = Clamp(step - 1, 0, d->node_count - 1);
    game->battle_is_nexus = true;
    game->battle_node_index = game->nexus.selected_node;
    GameEnterSelectedNexusBattle(game);
}

static void DebugEnterCampaignBattle(Game *game) {
    char map_id[32] = "PVE_MAP_01";
    int step = 1;
    const char *node = getenv("HOD_NODE");
    if (node) sscanf(node, "%31[^:]:%d", map_id, &step);
    BattleEnterNode(&game->battle, &game->player, map_id, step);
    BattleUiEnterNode(
        &game->battle_ui,
        &game->player,
        &game->battle.enemy_roster,
        CampaignFindNode(&game->map, map_id, step),
        map_id,
        step
    );
    const char *sim_seed = getenv("HOD_SIM");
    if (sim_seed) {
        static BattleSim sim;
        BattleSimRun(&sim, &game->player, &game->battle.enemy_roster, (unsigned)atoi(sim_seed));
        BattleSimDump(&sim);
    }
    const char *fight_seed = getenv("HOD_FIGHT");
    if (fight_seed && !BattlePlayActive(&game->battle_play)) {
        BattlePlayStart(&game->battle_play, &game->battle, &game->player, (unsigned)atoi(fight_seed));
        if (EnvFlag("HOD_FAST")) BattlePlayToggleSpeed(&game->battle_play);
        const char *splash = getenv("HOD_SPLASH");
        if (splash) {
            game->battle_play.won = strncmp(splash, "win", 3) == 0;
            game->battle_play.state = strstr(splash, "summary") ? BP_SUMMARY : BP_END_SPLASH;
        }
    }
    if (EnvFlag("HOD_TACTICAL")) BattleUiSetTactical(&game->battle_ui, &game->battle, true);
    int plate_side = 0, plate_slot = 0;
    const char *plate = getenv("HOD_PLATE");
    if (plate && sscanf(plate, "%d,%d", &plate_side, &plate_slot) == 2 && plate_side >= 0 && plate_side < 2 &&
        plate_slot >= 0 && plate_slot < SQUAD_SLOT_COUNT && game->battle_ui.plates[plate_side][plate_slot].ch)
        BattleUiOpenPlateDetails(&game->battle_ui, &game->battle_ui.plates[plate_side][plate_slot]);
}

static BattleResultsPresentation DebugResultsPresentation(const DebugShot *shot) {
    bool loss = DebugShotIs(shot, "results_loss");
    BattleResultsPresentation presentation = {
        .victory = !loss,
        .xp = loss ? 0 : DEBUG_RESULTS_XP,
        .gold = loss ? 0 : DEBUG_RESULTS_GOLD,
        .completion_gold = loss ? 0 : DEBUG_RESULTS_GOLD,
        .completions_pre = 1,
        .completions_post = 2,
        .wins_to_master = DEBUG_WINS_TO_MASTER,
    };
    if (DebugShotIs(shot, "levelup")) {
        presentation.player_level_pre = 1;
        presentation.player_level_post = 2;
        presentation.energy_max_delta = 1;
    } else if (DebugShotIs(shot, "completion")) {
        presentation.first_win = true;
        presentation.completions_pre = 0;
        presentation.completions_post = 1;
    } else if (DebugShotIs(shot, "completion_mastery")) {
        presentation.mastered = true;
        presentation.gems = CAMPAIGN_OFFLINE_MASTERY_GEMS;
        presentation.completion_mastery_gems = CAMPAIGN_OFFLINE_MASTERY_GEMS;
        presentation.completions_pre = DEBUG_WINS_TO_MASTER - 1;
        presentation.completions_post = DEBUG_WINS_TO_MASTER;
    } else if (DebugShotIs(shot, "results_repeat")) {
        presentation.completions_pre = DEBUG_WINS_TO_MASTER;
        presentation.completions_post = DEBUG_WINS_TO_MASTER + 1;
    }
    return presentation;
}

static void DebugPrepareBattle(Game *game, DebugShot *shot) {
    static const char *const names[] = {
        "battle", "levelup", "completion", "results", "completion_mastery", "results_repeat", "results_loss"
    };
    if (!DebugShotIsAny(shot, names, 7)) return;
    game->screen = SCREEN_BATTLE;
    if (!game->battle.squads_loaded) DebugEnterCampaignBattle(game);
    if (!shot->flow_started && !DebugShotIs(shot, "battle")) {
        BattleResultsFlowStart(&game->battle_results, DebugResultsPresentation(shot));
        shot->flow_started = true;
    }
    const char *splash = getenv("HOD_SPLASH");
    if (splash && !strstr(splash, "summary") && BattlePlayActive(&game->battle_play)) {
        game->battle_play.state = BP_END_SPLASH;
        game->battle_play.t = 1.0f;
    }
}

static void DebugOpenResourceDialog(Game *game, DebugShot *shot) {
    const char *resource = getenv("HOD_RESOURCE");
    if (!resource || shot->resource_opened) return;
    bool stamina = strcmp(resource, "stamina") == 0;
    EnergyDialogOpen(stamina ? &game->stamina_dialog : &game->energy_dialog);
    game->stamina_dialog_top = stamina;
    shot->resource_opened = true;
}

static void DebugBurstBattleFx(Game *game, const DebugShot *shot) {
    const char *effect = getenv("HOD_BFX");
    if (!effect || game->screen != SCREEN_BATTLE) return;
    if (shot->frames_drawn != shot->capture_frame - EnvInt("HOD_BFX_AGE", DEBUG_BFX_DEFAULT_AGE)) return;
    Vector3 target;
    const char *target_text = getenv("HOD_BFX_TARGET");
    if (target_text && sscanf(target_text, "%f,%f,%f", &target.x, &target.y, &target.z) == 3)
        BattleFxBurstDirected(&game->battle, effect, DEBUG_BFX_ORIGIN, target);
    else
        BattleFxBurst(&game->battle, effect, DEBUG_BFX_ORIGIN);
}

static void DebugShotPrepare(Game *game, DebugShot *shot) {
    game->screen = SCREEN_HEROES;
    if (DebugShotIs(shot, "home")) game->screen = SCREEN_HOME;
    if (DebugShotIs(shot, "quests")) game->screen = SCREEN_QUESTS;
    DebugPrepareStore(game, shot);
    DebugPrepareNexus(game, shot);
    DebugPrepareCampaignMap(game, shot);
    DebugPrepareNexusBattle(game, shot);
    DebugPrepareBattle(game, shot);
    DebugPrepareMapProgress(game, shot);
    const char *selected_hero = getenv("HOD_SELECT");
    if (selected_hero) game->heroes.selected = atoi(selected_hero);
    DebugOpenResourceDialog(game, shot);
    DebugBurstBattleFx(game, shot);
}

static bool DebugShotCapture(DebugShot *shot) {
    if (++shot->frames_drawn < shot->capture_frame) return false;
    rlDrawRenderBatchActive();
    TakeScreenshot(shot->output);
    return true;
}
