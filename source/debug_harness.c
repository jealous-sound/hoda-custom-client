typedef struct {
    const char *output;
    const char *screen;
    int frames_drawn;
    int capture_frame;
    bool flow_started;
    bool resource_opened;
    bool runes_seeded;
    bool consume_opened;
    bool gear_store;
} DebugShot;

#define DEBUG_SHOT_DEFAULT_FRAMES 90
#define DEBUG_TUTORIAL_SELECT_FRAME 5
#define DEBUG_PVP_TROPHIES 640
#define DEBUG_PVP_FIGHT_SEED 7
#define DEBUG_PVP_BANNERS 320
#define DEBUG_PVP_MILESTONES 2
#define DEBUG_PVP_BATTLES 5
#define DEBUG_PVP_BATTLE_SPACING 5400
#define DEBUG_PVP_DEFENSE_DELAY 90000
#define DEBUG_BFX_DEFAULT_AGE 30
#define DEBUG_PACK_FUNDS 999999
#define DEBUG_REVEAL_STEPS_PER_SECOND 60.0f
#define DEBUG_RUNE_FX_START_FRAME 30
#define DEBUG_WINS_TO_MASTER 3
#define DEBUG_RESULTS_XP 20
#define DEBUG_RESULTS_GOLD 140
#define DEBUG_MISSION_SUMMONS 600
#define DEBUG_MISSION_CONSUMES 60
#define DEBUG_MISSION_GOLD 45000
#define DEBUG_MISSION_SUMMON_SERIES 4
#define DEBUG_MISSION_CLAIM_FRAME 30
#define DEBUG_MISSION_CLAIMED_TIERS 3
#define DEBUG_MISSION_ROW_X 234.0f
#define DEBUG_MISSION_ROW_Y 141.0f
#define DEBUG_MISSION_ROW_WIDTH 816.0f

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
            printf(
                "LOADTEST %-4s meshes=%d frames=%d %-20s | %s\n", ok ? "OK" : "FAIL", model.meshCount, frames, why, line
            );
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

static void DebugAdvanceItemReveal(Store *store) {
    int dismissals = EnvInt("HOD_REVEAL_DISMISSALS", 0);
    int steps = (int)(EnvFloat("HOD_REVEAL_TIME", 2.0f) * DEBUG_REVEAL_STEPS_PER_SECOND);
    for (int step = 0; step < steps; step++) {
        StoreAdvanceItemReveal(store, 1.0f / DEBUG_REVEAL_STEPS_PER_SECOND);
        if (store->item_details >= 0 && dismissals-- > 0) StoreDismissItemDetails(store);
    }
    TraceLog(
        LOG_WARNING,
        "Reveal: started %d of %d, details %d, pending %d",
        store->item_reveal_next,
        store->item_result_count,
        store->item_details,
        store->item_details_pending
    );
}

static void DebugPrepareStore(Game *game, DebugShot *shot) {
    static const char *const names[] = {
        "store",
        "store_heroes",
        "store_gems",
        "store_items",
        "store_featured",
        "store_vault",
        "store_voucher",
        "pack",
        "pack_reveal",
        "pack_summary",
    };
    if (!DebugShotIsAny(shot, names, sizeof(names) / sizeof(names[0]))) return;
    game->screen = SCREEN_STORE;
    if (!shot->flow_started) {
        StoreOpen(&game->store);
        game->store.clock_override = EnvInt("HOD_STORE_NOW", 0);
        game->store.scroll = EnvFloat("HOD_STORE_SCROLL", 0.0f);
        for (int voucher = 0; voucher < game->store.voucher_count; voucher++)
            game->player.pack_vouchers[voucher] = EnvInt("HOD_VOUCHERS", 0);
        if (DebugShotIs(shot, "store_heroes")) {
            game->store.view = STORE_VIEW_HEROES;
        } else if (DebugShotIs(shot, "store_gems")) {
            game->store.view = STORE_VIEW_GEMS;
        } else if (DebugShotIs(shot, "store_items")) {
            game->store.view = STORE_VIEW_ITEMS;
        } else if (DebugShotIs(shot, "store_featured")) {
            game->store.view = STORE_VIEW_FEATURED;
        } else if (DebugShotIs(shot, "store_vault")) {
            game->store.view = STORE_VIEW_VAULT;
        } else if (DebugShotIs(shot, "store_voucher")) {
            game->store.view = STORE_VIEW_HEROES;
            game->store.pending_pack = Clamp(EnvInt("HOD_PACK", 0), 0, game->store.pack_count - 1);
            game->store.dialog = STORE_DIALOG_VOUCHER;
        } else if (!DebugShotIs(shot, "store")) {
            int pack = Clamp(EnvInt("HOD_PACK", STORE_PACK_CHAMPION), 0, game->store.pack_count - 1);
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
    if (EnvFlag("HOD_NEXUS_COMPLETE")) {
        game->player.nexus_run.committed = true;
        game->player.nexus_run.nodes[step - 1].wins = 1;
        game->player.nexus_run.escrow_gold += d->nodes[step - 1].gold;
        game->player.nexus_run.escrow_xp += d->nodes[step - 1].xp;
        game->player.nexus_run.escrow_potions = EnvInt("HOD_NEXUS_POTIONS", 2);
        game->player.nexus_run.escrow_units[EnvInt("HOD_NEXUS_RARITY", RARITY_RARE)] = EnvInt("HOD_NEXUS_UNITS", 3);
    }
    NexusEnterMap(&game->nexus, &game->player, event, difficulty);
    game->nexus.quit_open = EnvFlag("HOD_NEXUS_QUIT");
    shot->flow_started = true;
}

static void DebugPrepareNews(Game *game, DebugShot *shot) {
    bool event_popup = DebugShotIs(shot, "news_event");
    if (!event_popup && !DebugShotIs(shot, "news")) return;
    game->screen = SCREEN_HOME;
    if (EnvFlag("HOD_NEWS_ZOOM")) {
        game->news.zoom = NEWS_ZOOM_OPENING;
        game->news.zoom_progress = EnvFloat("HOD_NEWS_ZOOM", 0.0f);
    }
    if (shot->flow_started) return;
    if (event_popup) {
        NewsOpenEvent(&game->news, &game->nexus, &game->player, DebugNexusEvent(game));
    } else {
        NewsOpen(&game->news, &game->player);
    }
    shot->flow_started = true;
}

static void DebugPrepareQuestLadder(Game *game, const DebugShot *shot) {
    if (!DebugShotIs(shot, "quests_ladder")) return;
    game->screen = SCREEN_QUESTS;
    QuestsOpenLadder(&game->quests, DebugNexusEvent(game));
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
    if (game->battle.squads_loaded) {
        const char *fight_seed = getenv("HOD_FIGHT");
        if (fight_seed && !BattlePlayActive(&game->battle_play))
            BattlePlayStart(&game->battle_play, &game->battle, &game->player, (unsigned)atoi(fight_seed));
        return;
    }
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

static void DebugPreparePvp(Game *game, DebugShot *shot) {
    if (DebugShotIs(shot, "pvp_hub")) {
        if (shot->frames_drawn == 0) GameOpenPvpHub(game);
        game->screen = SCREEN_PVP_HUB;
        return;
    }
    static const struct {
        const char *screen;
        PvpEventTab tab;
    } event_shots[] = {
        {"pvp_event", PVP_EVENT_TAB_EVENT},
        {"pvp_event_banners", PVP_EVENT_TAB_BANNERS},
        {"pvp_event_rewards", PVP_EVENT_TAB_REWARDS},
        {"pvp_event_battles", PVP_EVENT_TAB_BATTLES},
    };
    const PvpEventTab *event_tab = NULL;
    for (size_t i = 0; i < sizeof(event_shots) / sizeof(event_shots[0]); i++)
        if (DebugShotIs(shot, event_shots[i].screen)) event_tab = &event_shots[i].tab;
    if (event_tab) {
        if (shot->frames_drawn == 0) {
            PvpEventSync(&game->player.pvp, (long long)time(NULL));
            game->player.pvp.event.banners = DEBUG_PVP_BANNERS;
            game->player.pvp.event.milestones_paid = DEBUG_PVP_MILESTONES;
            GameOpenPvpEvent(game);
            game->pvp_event_view.tab = *event_tab;
            if (*event_tab == PVP_EVENT_TAB_BATTLES) {
                long long now = (long long)time(NULL);
                PlayerPvp *pvp = &game->player.pvp;
                PvpOpponent opponent =
                    PvpEventOpponentGenerate(game->player.level, pvp->trophies, pvp->event.banners, now, 1);
                PvpRecordEventAttack(pvp, &opponent, true, now);
            }
        }
        game->screen = SCREEN_PVP_EVENT;
        return;
    }
    if (!DebugShotIsAny(
            shot,
            (const char *const[]){"pvp_match", "pvp_fight", "pvp_results", "pvp_event_match", "pvp_event_results"},
            5
        ))
        return;
    game->screen = SCREEN_BATTLE;
    if (!game->battle.squads_loaded) {
        game->player.pvp.trophies = DEBUG_PVP_TROPHIES;
        game->pvp_event = DebugShotIsAny(shot, (const char *const[]){"pvp_event_match", "pvp_event_results"}, 2);
        GameEnterLadderBattle(game, SCREEN_PVP_HUB);
        return;
    }
    const char *fight_seed = getenv("HOD_FIGHT");
    if (DebugShotIs(shot, "pvp_fight") && fight_seed && !BattlePlayActive(&game->battle_play))
        BattlePlayStart(&game->battle_play, &game->battle, &game->player, (unsigned)atoi(fight_seed));
    if (DebugShotIsAny(shot, (const char *const[]){"pvp_results", "pvp_event_results"}, 2) && !shot->flow_started) {
        BattlePlayStart(&game->battle_play, &game->battle, &game->player, DEBUG_PVP_FIGHT_SEED);
        GameCommitBattleOutcome(game, game->battle_play.sim.player_won);
        BattleResultsFlowStart(&game->battle_results, GameBattleResultsSnapshot(game));
        shot->flow_started = true;
    }
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

static void DebugPlayHeroMelee(Game *game, const DebugShot *shot) {
    if (!EnvFlag("HOD_HERO_MELEE") || game->screen != SCREEN_HEROES) return;
    if (shot->frames_drawn != shot->capture_frame - EnvInt("HOD_HERO_MELEE", 0)) return;
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitPlayMelee(&game->heroes_scene.units[s]);
}

static void DebugSeedRunes(Player *player) {
    static const char *const ids[] = {
        "rune-power-5",
        "rune-power-5",
        "rune-health-6",
        "rune-knowledge-3",
        "rune-greed-2",
        "rune-guard-7",
        "rune-stealth-7",
        "rune-stun-6",
        "rune-slow-4",
        "rune-brave-targeting-6",
        "rune-initiative-5",
    };
    long long now = RuneNow();
    player->runes = (PlayerRunes){0};
    for (int i = 0; i < (int)(sizeof(ids) / sizeof(ids[0])); i++) RuneGrantById(&player->runes, ids[i], now);
}

static void DebugActivateBattleRunes(Game *game, DebugShot *shot) {
    if (!EnvFlag("HOD_RUNES") || shot->runes_seeded) return;
    shot->runes_seeded = true;
    Player *player = &game->player;
    DebugSeedRunes(player);
    uint64_t power = RuneGrantById(&player->runes, "rune-power-5", RuneNow());
    uint64_t health = RuneGrantById(&player->runes, "rune-health-6", RuneNow());
    RuneActivate(&player->runes, player->current_squad, 0, power, RuneNow());
    RuneActivate(&player->runes, player->current_squad, 1, health, RuneNow());
}

static void DebugPrepareRunes(Game *game, DebugShot *shot) {
    if (!shot->screen || strncmp(shot->screen, "runes", 5) != 0) return;
    if (shot->flow_started) {
        game->screen = DebugShotIs(shot, "runes_store") ? SCREEN_STORE : SCREEN_RUNES;
        if (DebugShotIs(shot, "runes_fx") && shot->frames_drawn == DEBUG_RUNE_FX_START_FRAME)
            RunesBeginActivation(&game->runes, &game->player);
        return;
    }
    shot->flow_started = true;
    Player *player = &game->player;
    int squad = player->current_squad;
    DebugSeedRunes(player);
    RunesOpen(&game->runes, player);
    game->runes_return_screen = SCREEN_HEROES;
    game->screen = SCREEN_RUNES;
    uint64_t sorted[RUNE_INVENTORY_CAPACITY];
    int count = RuneAvailableSorted(&player->runes, RuneNow(), true, sorted, RUNE_INVENTORY_CAPACITY);
    if (count == 0) return;
    if (DebugShotIs(shot, "runes_active")) {
        player->runes.prestige_unlocked[squad] = true;
        RuneActivate(&player->runes, squad, RUNE_PRESTIGE_SLOT, sorted[0], RuneNow());
        RuneActivate(&player->runes, squad, 0, sorted[2], RuneNow());
        game->runes.pending[1] = sorted[3];
    } else if (DebugShotIs(shot, "runes_fx")) {
        game->runes.pending[0] = sorted[2];
    } else if (DebugShotIs(shot, "runes_message")) {
        StrCopy(game->runes.message, sizeof(game->runes.message), LocGet("RUNE_NOT_CRAFTABLE"));
        game->runes.dialog = RUNES_DIALOG_MESSAGE;
    } else if (DebugShotIs(shot, "runes_replace")) {
        game->runes.dialog = RUNES_DIALOG_REPLACE;
    } else if (DebugShotIs(shot, "runes_prestige")) {
        game->runes.dialog = RUNES_DIALOG_PRESTIGE;
    } else if (DebugShotIs(shot, "runes_details")) {
        game->runes.details_uuid = sorted[0];
    } else if (DebugShotIs(shot, "runes_craft")) {
        game->runes.mode = RUNES_MODE_CRAFT;
        const PlayerRune *twin = NULL;
        for (int i = 0; i < player->runes.count && !twin; i++)
            if (!strcmp(RuneDefFor(&player->runes.items[i])->id, "rune-power-5")) twin = &player->runes.items[i];
        RunesDropCraft(&game->runes, &player->runes, 0, twin->uuid, RuneNow());
    } else if (DebugShotIs(shot, "runes_dialog")) {
        game->runes.mode = RUNES_MODE_CRAFT;
        const PlayerRune *twin = NULL;
        for (int i = 0; i < player->runes.count && !twin; i++)
            if (!strcmp(RuneDefFor(&player->runes.items[i])->id, "rune-power-5")) twin = &player->runes.items[i];
        RunesDropCraft(&game->runes, &player->runes, 0, twin->uuid, RuneNow());
        game->runes.dialog = RUNES_DIALOG_CRAFT_SINGLE;
    } else if (DebugShotIs(shot, "runes_store")) {
        StoreOpen(&game->store);
        StoreShowItems(&game->store);
        player->gems = DEBUG_PACK_FUNDS;
        game->store_return_screen = SCREEN_HEROES;
        game->screen = SCREEN_STORE;
    }
}

static void DebugSeedGoals(Game *game, DebugShot *shot) {
    if (shot->frames_drawn != 0) return;
    static const struct {
        const char *name;
        GoalsTab tab;
    } shots[] = {
        {"goals_trophies", GOALS_TAB_TROPHIES},
        {"goals_battles", GOALS_TAB_BATTLES},
        {"goals_events", GOALS_TAB_EVENTS},
    };
    for (int i = 0; i < (int)(sizeof(shots) / sizeof(shots[0])); i++) {
        if (!DebugShotIs(shot, shots[i].name)) continue;
        PlayerPvp *pvp = &game->player.pvp;
        long long now = (long long)time(NULL);
        pvp->trophies = DEBUG_PVP_TROPHIES;
        for (unsigned seed = 1; seed <= DEBUG_PVP_BATTLES; seed++) {
            PvpOpponent o = PvpOpponentGenerate(game->player.level, pvp->trophies, PvpDay(now), seed);
            PvpRecordAttack(pvp, &o, seed % 3 != 0, now - seed * DEBUG_PVP_BATTLE_SPACING);
        }
        PvpSimulateDefense(pvp, game->player.level, now + DEBUG_PVP_DEFENSE_DELAY);
        pvp->event = (PlayerPvpEvent){.cycle = PvpEventCycle(now) - 1, .banners = DEBUG_PVP_BANNERS};
        MissionsOpen(&game->missions, &game->player, shots[i].tab);
    }
}

static void DebugSeedMissions(Game *game, DebugShot *shot) {
    if (!DebugShotIsAny(shot, (const char *const[]){"missions", "missions_claim"}, 2)) return;
    if (shot->frames_drawn == 0) {
        PlayerMissions *m = &game->player.missions;
        m->counters[MISSION_STAT_SUMMONED] = DEBUG_MISSION_SUMMONS;
        m->counters[MISSION_STAT_CONSUMED] = DEBUG_MISSION_CONSUMES;
        m->counters[MISSION_STAT_GOLD_EARNED] = DEBUG_MISSION_GOLD;
        MissionsSync(&game->player, &game->map);
        for (int tier = 0; tier < DEBUG_MISSION_CLAIMED_TIERS; tier++)
            m->claimed[MissionSeriesTierAt(DEBUG_MISSION_SUMMON_SERIES, tier)] = true;
        MissionsOpen(&game->missions, &game->player, GOALS_TAB_MISSIONS);
        game->screen = SCREEN_MISSIONS;
    }
    if (DebugShotIs(shot, "missions_claim") && shot->frames_drawn == DEBUG_MISSION_CLAIM_FRAME) {
        Rectangle row = {DEBUG_MISSION_ROW_X, DEBUG_MISSION_ROW_Y, DEBUG_MISSION_ROW_WIDTH, MISSIONS_ROW_HEIGHT};
        MissionsBeginClaim(&game->missions, &game->hud, &game->player, game->missions.order[0], row);
    }
}

static void DebugPrepareGallery(Game *game, DebugShot *shot) {
    bool show_unit = DebugShotIs(shot, "gallery_unit");
    if (!show_unit && !DebugShotIs(shot, "gallery")) return;
    game->screen = SCREEN_HEROES;
    if (shot->flow_started) return;
    Gallery *gallery = &game->gallery;
    gallery->selection.category =
        (GalleryCategory)Clamp(EnvInt("HOD_GALLERY_CATEGORY", GALLERY_CATEGORY_ALL), 0, GALLERY_CATEGORY_COUNT - 1);
    if (show_unit && gallery->selection.category == GALLERY_CATEGORY_ALL)
        gallery->selection.category = GALLERY_CATEGORY_RARITY;
    gallery->selection.rarity =
        (Rarity)Clamp(EnvInt("HOD_GALLERY_RARITY", RARITY_COMMON), RARITY_COMMON, RARITY_LEGENDARY);
    GalleryOpenPopup(gallery, &game->player);
    gallery->page = EnvInt("HOD_GALLERY_PAGE", 0);
    GalleryRefresh(gallery, &game->player, false);
    if (show_unit && gallery->match_count > 0) {
        GalleryPrepareUnitList(
            gallery, gallery->matches[(int)Clamp(EnvInt("HOD_GALLERY_UNIT", 0), 0, gallery->match_count - 1)]
        );
        GalleryClosePopup(gallery);
        GalleryScreenEnter(gallery, &game->player, &game->scene);
        gallery->selected_level = (Tier)Clamp(EnvInt("HOD_GALLERY_TIER", 0), 0, TIER_COUNT - 1);
        GalleryShowCurrentUnit(gallery, &game->player, &game->scene);
    }
    shot->flow_started = true;
}

static const Character *DebugConsumeDefinition(Game *game, const char *id, int *index) {
    cJSON *catalog_root = ProgressionLoadCharacterCatalog();
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    const cJSON *definition = CatalogFindCharacter(catalog, id);
    bool added = definition && PlayerRosterAppendDefinitions(&game->player, &definition, 1, index);
    cJSON_Delete(catalog_root);
    if (!added) TraceLog(LOG_FATAL, "Debug: cannot add %s", id);
    return &game->player.roster[*index];
}

static int DebugGearHero(const Game *game) {
    for (int i = 0; i < game->player.roster_count; i++)
        if (game->player.roster[i].gear_slots >= GEAR_PRESTIGE_MIN_SLOTS) return i;
    return 0;
}

static void DebugPrepareGear(Game *game, DebugShot *shot) {
    if (!shot->screen || strncmp(shot->screen, "gear", 4) != 0) return;
    game->screen = shot->gear_store ? SCREEN_STORE : SCREEN_HEROES;
    if (shot->flow_started) return;
    shot->flow_started = true;
    Player *player = &game->player;
    int hero = DebugGearHero(game);
    for (int i = 0; i < GearCatalogCount(); i += 3) GearGrant(&player->gear, i);
    player->gold = player->gems = DEBUG_PACK_FUNDS;
    GearGrant(&player->gear, GearDefIndex("red_iron_maul"));
    GearEquip(&player->gear, &player->roster[hero], 0, GearDefIndex("red_iron_maul"));
    game->heroes.selected = hero;
    if (DebugShotIs(shot, "gear_ascend") || DebugShotIs(shot, "gear_ascended")) {
        Character *ch = &player->roster[hero];
        ch->gear.defs[1] = (short)(GearDefIndex("lotherings_lament") + 1);
        ch->gear.defs[2] = (short)(GearDefIndex("defender_coat") + 1);
        player->gear.tomes = 1;
        gear_view.hero = hero;
        if (DebugShotIs(shot, "gear_ascend")) gear_view.dialog = GEAR_VIEW_DIALOG_ASCEND;
        if (DebugShotIs(shot, "gear_ascended")) {
            GearAscend(&player->gear, ch);
            HeroesRebuildCard(&game->heroes, player, hero);
        }
        return;
    }
    if (DebugShotIs(shot, "gear_slots")) return;
    if (DebugShotIs(shot, "gear_prestige")) {
        gear_view.hero = hero;
        gear_view.slot = GearSlotCount(&player->roster[hero]) - 1;
        gear_view.dialog = GEAR_VIEW_DIALOG_PRESTIGE;
        return;
    }
    if (DebugShotIs(shot, "gear_store") || DebugShotIs(shot, "gear_reveal")) {
        game->screen = SCREEN_STORE;
        StoreOpen(&game->store);
        StoreShowItems(&game->store);
        shot->gear_store = true;
        if (DebugShotIs(shot, "gear_reveal")) {
            player->gems = INT_MAX / 2;
            player->gold = INT_MAX / 2;
            StoreTryPurchase(&game->store, player, &game->heroes, EnvInt("HOD_PACK", game->store.pack_count - 1));
            DebugAdvanceItemReveal(&game->store);
        }
        return;
    }
    if (DebugShotIs(shot, "gear_inventory")) {
        game->heroes.selected = -1;
        GearViewOpenInventory();
        return;
    }
    GearViewShow(hero, 1);
    int items[GEAR_CATALOG_CAPACITY];
    if (DebugShotIs(shot, "gear_list")) return;
    if (GearViewFilteredItems(player, gear_view.filter, false, items) > 0) gear_view.selected = items[0];
    if (DebugShotIs(shot, "gear_filter")) gear_view.filter_open = true;
    if (DebugShotIs(shot, "gear_confirm")) GearViewRequestEquip(player);
}

static void DebugPrepareConsume(Game *game, DebugShot *shot) {
    if (!shot->screen || strncmp(shot->screen, "consume", 7) != 0) return;
    if (shot->consume_opened) {
        if (game->screen == SCREEN_HEROES) game->screen = SCREEN_CONSUME;
        return;
    }
    shot->consume_opened = true;
    ConsumeScreen *c = &game->consume;
    int target = -1, duplicate = -1, seed_a = -1, seed_b = -1;
    if (!DebugShotIs(shot, "consume")) {
        const char *override = getenv("HOD_CONSUME_HERO");
        const char *hero = override ? override : game->player.roster[0].id;
        DebugConsumeDefinition(game, hero, &target);
        DebugConsumeDefinition(game, hero, &duplicate);
        DebugConsumeDefinition(game, "UNIT_ELVENMAGE", &seed_a);
        DebugConsumeDefinition(game, game->player.roster[0].id, &seed_b);
    }
    HeroesSyncRoster(&game->heroes, &game->player);
    GameOpenConsume(game);
    if (DebugShotIs(shot, "consume")) return;
    memset(c->warnings_disabled, 1, sizeof(c->warnings_disabled));
    ConsumePlaceUnit(c, &game->heroes_scene, &game->player, target);
    ConsumePlaceUnit(c, &game->heroes_scene, &game->player, duplicate);
    ConsumePlaceUnit(c, &game->heroes_scene, &game->player, seed_a);
    ConsumePlaceUnit(c, &game->heroes_scene, &game->player, seed_b);
    if (DebugShotIs(shot, "consume_squad"))
        ConsumePlaceUnit(c, &game->heroes_scene, &game->player, PlayerSquadUnitAt(&game->player, 0, 0));
    if (DebugShotIs(shot, "consume_ladder")) c->ladder_open = true;
    if (DebugShotIs(shot, "consume_multi")) c->dialog = CONSUME_DIALOG_MULTI_TYPE;
    if (DebugShotIs(shot, "consume_multi_confirm")) ConsumeMultiSelect(c, &game->player, CONSUME_MULTI_RARITY);
    if (DebugShotIs(shot, "consume_warning")) {
        memset(c->warnings_disabled, 0, sizeof(c->warnings_disabled));
        c->warning_disable = false;
        c->pending_seed = seed_a;
        c->dialog = CONSUME_DIALOG_RARITY_WARNING;
    }
    if (DebugShotIs(shot, "consume_results")) ConsumeBeginResults(c, &game->heroes, &game->player);
}

static bool DebugPrepareTutorial(Game *game, DebugShot *shot) {
    if (!DebugShotIs(shot, "tutorial")) return false;
    int selected = EnvInt("HOD_SELECT", -1);
    if (shot->frames_drawn == DEBUG_TUTORIAL_SELECT_FRAME && selected >= 0 && selected < game->heroes.card_count) {
        game->heroes.selected = selected;
        TutorialRaise(TUTORIAL_EVENT_UNIT_DETAILS_SHOWING);
    }
    if (shot->frames_drawn > 0) return true;
    const char *step = getenv("HOD_TUTORIAL_STEP");
    const char *screen = getenv("HOD_TUTORIAL_SCREEN");
    if (step && !TutorialJumpTo(&game->tutorial, strtof(step, NULL)))
        TraceLog(LOG_WARNING, "Tutorial: no step %s", step);
    if (!step) TutorialResumeMain(&game->tutorial, &(PlayerTutorial){0});
    if (screen && !strcmp(screen, "map")) game->screen = SCREEN_MAP;
    if (screen && !strcmp(screen, "heroes")) game->screen = SCREEN_HEROES;
    if (screen && !strcmp(screen, "store")) GameOpenStore(game, SCREEN_HOME);
    bool fight = screen && !strcmp(screen, "fight");
    if (fight || (screen && !strcmp(screen, "battle"))) {
        game->battle_map_index = EnvInt("HOD_MAP", 1) - 1;
        game->battle_node_index = EnvInt("HOD_NODE", 1) - 1;
        GameEnterSelectedBattle(game);
        game->screen = SCREEN_BATTLE;
    }
    if (fight) GameStartBattlePlay(game, &game->player);
    if (screen && !strcmp(screen, "intro")) GameStartIntroBattle(game);
    return true;
}

static bool DebugPrepareNamePrompt(Game *game, DebugShot *shot) {
    if (!DebugShotIs(shot, "name_prompt")) return false;
    game->screen = SCREEN_HOME;
    if (shot->frames_drawn == 0) NamePromptOpen(&game->name_prompt);
    return true;
}

static void DebugShotPrepare(Game *game, DebugShot *shot) {
    if (DebugPrepareTutorial(game, shot) || DebugPrepareNamePrompt(game, shot)) return;
    game->screen = shot->consume_opened ? SCREEN_CONSUME : SCREEN_HEROES;
    if (DebugShotIs(shot, "home")) game->screen = SCREEN_HOME;
    if (DebugShotIs(shot, "quests")) game->screen = SCREEN_QUESTS;
    if (DebugShotIsAny(
            shot,
            (const char *const[]){"missions", "missions_claim", "goals_trophies", "goals_battles", "goals_events"},
            5
        ))
        game->screen = SCREEN_MISSIONS;
    DebugActivateBattleRunes(game, shot);
    DebugPrepareRunes(game, shot);
    DebugPrepareConsume(game, shot);
    DebugPrepareGear(game, shot);
    DebugPrepareGallery(game, shot);
    DebugSeedGoals(game, shot);
    DebugSeedMissions(game, shot);
    if (DebugShotIs(shot, "gallery_unit")) game->screen = SCREEN_GALLERY;
    DebugPrepareStore(game, shot);
    DebugPrepareNexus(game, shot);
    DebugPrepareNews(game, shot);
    DebugPrepareQuestLadder(game, shot);
    DebugPrepareCampaignMap(game, shot);
    DebugPrepareNexusBattle(game, shot);
    DebugPrepareBattle(game, shot);
    DebugPreparePvp(game, shot);
    DebugPrepareMapProgress(game, shot);
    int selected_hero = EnvInt("HOD_SELECT", -1);
    if (selected_hero >= 0 && selected_hero < game->heroes.card_count) game->heroes.selected = selected_hero;
    DebugOpenResourceDialog(game, shot);
    DebugBurstBattleFx(game, shot);
    DebugPlayHeroMelee(game, shot);
}

static bool DebugShotCapture(DebugShot *shot) {
    if (++shot->frames_drawn < shot->capture_frame) return false;
    rlDrawRenderBatchActive();
    TakeScreenshot(shot->output);
    return true;
}
