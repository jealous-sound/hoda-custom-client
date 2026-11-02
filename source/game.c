typedef enum {
    SCREEN_HOME,
    SCREEN_STORE,
    SCREEN_HEROES,
    SCREEN_RUNES,
    SCREEN_CONSUME,
    SCREEN_GALLERY,
    SCREEN_QUESTS,
    SCREEN_MISSIONS,
    SCREEN_PVP_HUB,
    SCREEN_PVP_EVENT,
    SCREEN_MAP,
    SCREEN_NEXUS,
    SCREEN_BATTLE
} Screen;

#define GAME_WISP_LOOP_CAPACITY 64

typedef struct {
    int index;
    unsigned serial;
    unsigned frame;
} GameWispLoop;

typedef struct {
    Screen screen;
    Player player;
    Hud hud;
    HomeUI home_ui;
    HomeBackground scene;
    HeroesBackground heroes_scene;
    Battle battle;
    BattleUi battle_ui;
    BattlePlay battle_play;
    MusicPlayer music;
    Heroes heroes;
    RunesScreen runes;
    MissionsScreen missions;
    Screen runes_return_screen;
    ConsumeScreen consume;
    Character consume_shown_target;
    ConsumeAttackClip consume_attack_pending;
    GameWispLoop wisp_loops[GAME_WISP_LOOP_CAPACITY];
    int wisp_loop_count;
    unsigned wisp_frame;
    Gallery gallery;
    Store store;
    Screen store_return_screen;
    Quests quests;
    News news;
    Campaign map;
    Nexus nexus;
    NexusRecap nexus_recap;
    EnergyDialog energy_dialog;
    EnergyDialog stamina_dialog;
    bool stamina_dialog_top;

    PvpHub pvp_hub;
    PvpEventView pvp_event_view;
    bool battle_is_pvp;
    bool pvp_event;
    bool pvp_can_next;
    int pvp_revenge_index;
    PvpOpponent pvp_opponent;
    Screen pvp_return_screen;

    bool battle_is_nexus;
    int battle_map_index;
    int battle_node_index;
    int battle_squad_index;
    int battle_energy_cost;
    bool battle_energy_spent;
    bool battle_outcome_committed;
    BattleRewardResult battle_reward;
    BattleResultsFlow battle_results;
} Game;

static bool GameBattleSelectionValid(const Game *g) {
    if (g->battle_map_index < 0 || g->battle_map_index >= g->map.map_count) return false;
    const CampaignMap *map = &g->map.maps[g->battle_map_index];
    return g->battle_node_index >= 0 && g->battle_node_index < map->node_count;
}

static void GameClearBattleOutcome(Game *g) {
    g->battle_outcome_committed = false;
    g->battle_reward = (BattleRewardResult){0};
}

static void GameEnterSelectedBattle(Game *g) {
    if (!GameBattleSelectionValid(g)) {
        TraceLog(
            LOG_ERROR, "Battle: invalid saved selection map=%d node=%d", g->battle_map_index, g->battle_node_index
        );
        return;
    }

    g->battle_is_nexus = false;
    CampaignMap *map = &g->map.maps[g->battle_map_index];
    CampaignNode *node = &map->nodes[g->battle_node_index];
    int step = g->battle_node_index + 1;
    BattleEnterNode(&g->battle, &g->player, map->id, step);
    BattleUiEnterNode(&g->battle_ui, &g->player, &g->battle.enemy_roster, node, map->id, step);

    int wins = g->player.campaign_progress.node_completions[g->battle_map_index][g->battle_node_index];
    bool boss = g->battle_node_index == CampaignBossNodeIndex(&g->map, g->battle_map_index);
    BattleUiSetNodeWins(&g->battle_ui, wins);
    BattleUiSetNewMapOnWin(&g->battle_ui, boss && wins == 0 && g->battle_map_index + 1 < g->map.map_count);
    g->battle_squad_index = g->player.current_squad;
    g->battle_energy_cost = node->energy_cost;
}

static void GameEnterSelectedNexusBattle(Game *g) {
    const NexusEvent *event = NexusEventAt(&g->nexus, g->nexus.active_event);
    const NexusDifficulty *difficulty = NexusActiveDifficulty(&g->nexus);
    if (!event || !difficulty) {
        TraceLog(LOG_ERROR, "Nexus: no map selected for battle");
        return;
    }
    g->battle_is_nexus = true;
    int step = g->battle_node_index + 1;
    BattleEnterNodeFrom(&g->battle, &g->player, "data/nexus_rosters.json", difficulty->id, step);
    g->battle.enemy_roster.game_changer = NexusEventGameChanger(&g->nexus, g->nexus.active_event);
    BattleUiEnterNexusNode(
        &g->battle_ui,
        &g->player,
        &g->battle.enemy_roster,
        event,
        difficulty,
        step,
        NexusSelectedEnergyCost(&g->nexus, &g->player)
    );
    BattleUiSetNewMapOnWin(&g->battle_ui, false);
    g->battle_squad_index = g->player.current_squad;
}

static void GameEnterPvpOpponent(Game *g) {
    long long now = (long long)time(NULL);
    BattleEnterPvp(&g->battle, &g->player, &g->pvp_opponent);
    if (g->pvp_event) g->battle.enemy_roster.game_changer = &PvpEventForCycle(PvpEventCycle(now))->changer;
    int nexts = g->pvp_can_next ? PvpNextsRemaining(&g->player.pvp, now) : -1;
    BattleUiEnterPvp(
        &g->battle_ui, &g->player, &g->battle.enemy_roster, &g->pvp_opponent, PVP_STAMINA_COST, nexts, g->pvp_event
    );
    g->battle_is_pvp = true;
    g->battle_is_nexus = false;
    g->battle_squad_index = g->player.current_squad;
    g->battle_energy_cost = PVP_STAMINA_COST;
    g->battle_energy_spent = false;
    GameClearBattleOutcome(g);
    g->screen = SCREEN_BATTLE;
}

static PvpOpponent GameLadderOpponent(const Game *g) {
    long long now = (long long)time(NULL);
    unsigned seed = (unsigned)GetRandomValue(1, 0x7FFFFFFF);
    const PlayerPvp *pvp = &g->player.pvp;
    if (g->pvp_event) return PvpEventOpponentGenerate(g->player.level, pvp->trophies, pvp->event.banners, now, seed);
    return PvpOpponentGenerate(g->player.level, pvp->trophies, PvpDay(now), seed);
}

static void GameEnterLadderBattle(Game *g, Screen return_screen) {
    g->pvp_opponent = GameLadderOpponent(g);
    g->pvp_can_next = true;
    g->pvp_revenge_index = -1;
    g->pvp_return_screen = return_screen;
    GameEnterPvpOpponent(g);
}

static void GameEnterFixedPvpBattle(Game *g, PvpOpponent opponent, int revenge_index, Screen return_screen) {
    g->pvp_event = false;
    g->pvp_opponent = opponent;
    g->pvp_can_next = false;
    g->pvp_revenge_index = revenge_index;
    g->pvp_return_screen = return_screen;
    GameEnterPvpOpponent(g);
}

static PlayerCampaignAnimation
GamePendingCampaignAnimation(const Campaign *campaign, const BattleRewardResult *reward) {
    PlayerCampaignAnimation animation = {0};
    if (!campaign || !reward || !reward->resolved || !reward->won || reward->grind ||
        (!reward->first_win && !reward->mastered) || reward->map_index < 0 ||
        reward->map_index >= campaign->map_count || reward->node_index < 0 ||
        reward->node_index >= campaign->maps[reward->map_index].node_count - 1)
        return animation;

    animation.active = true;
    animation.unlock_next =
        reward->first_win && !(reward->wins_to_master > 0 && reward->completions_post >= reward->wins_to_master);
    animation.from_map = reward->map_index;
    animation.from_node = reward->node_index;
    animation.to_map = reward->next_map_index;
    animation.to_node = reward->mastered ? reward->node_index + 1 : reward->next_node_index;
    return animation;
}

static void GameCommitBattleOutcome(Game *g, bool won) {
    if (g->battle_outcome_committed) return;

    g->battle_outcome_committed = true;
    GalleryRecordBattle(&g->player.gallery, &g->battle.enemy_roster, &g->battle_play.sim);
    int crits = BattlePlayCriticalGold(&g->battle_play) / BP_CRIT_GOLD_REWARD;
    bool challenge = !g->battle_is_nexus && !g->battle_is_pvp && g->map.maps[g->battle_map_index].nodes[g->battle_node_index].grind;
    MissionsRecordBattle(&g->player, &g->battle_play.sim, crits, challenge, (long long)time(NULL));
    if (g->battle_is_pvp) {
        g->battle_reward = PvpResolveBattle(
            &g->player,
            &g->pvp_opponent,
            won,
            g->pvp_revenge_index,
            BattlePlayCriticalGold(&g->battle_play),
            (long long)time(NULL),
            g->pvp_event
        );
        g->pvp_revenge_index = -1;
        if (g->pvp_event) MissionsRecordEventBattle(&g->player, &g->battle_play.sim, g->battle_reward.banner_delta);
        MissionsSync(&g->player, &g->map);
        ProgressionSave(&g->player);
        return;
    }
    if (g->battle_is_nexus) {
        PlayerNexusRun *run = &g->player.nexus_run;
        int from_step = g->battle_node_index + 1;
        g->battle_reward =
            NexusResolveBattle(&g->player, &g->nexus, from_step, won, BattlePlayCriticalGold(&g->battle_play));
        run->pending_animation = won;
        run->pending_from_step = from_step;
        run->pending_to_step = run->current_node_step;
        MissionsSync(&g->player, &g->map);
        ProgressionSave(&g->player);
        return;
    }
    g->battle_reward = CampaignResolveBattle(
        &g->player, &g->map, g->battle_map_index, g->battle_node_index, won, BattlePlayCriticalGold(&g->battle_play)
    );
    g->player.campaign_progress.pending_animation = GamePendingCampaignAnimation(&g->map, &g->battle_reward);
    MissionsSync(&g->player, &g->map);
    ProgressionSave(&g->player);
}

static BattleResultsPresentation GameBattleResultsSnapshot(const Game *g) {
    return (BattleResultsPresentation){
        .victory = g->battle_reward.won,
        .first_win = g->battle_reward.first_win,
        .mastered = g->battle_reward.mastered,
        .grind = g->battle_reward.grind,
        .completions_pre = g->battle_reward.completions_pre,
        .completions_post = g->battle_reward.completions_post,
        .wins_to_master = g->battle_reward.wins_to_master,
        .player_level_pre = g->battle_reward.player_level_pre,
        .player_level_post = g->battle_reward.player_level_post,
        .energy_max_delta = g->battle_reward.energy_max_post - g->battle_reward.energy_max_pre,
        .xp = g->battle_reward.xp_reward,
        .gold = g->battle_reward.gold_reward,
        .completion_gold = g->battle_reward.default_gold_reward,
        .completion_mastery_gems = g->battle_reward.mastery_gems_reward,
        .gems = g->battle_reward.gems_reward,
        .escrowed = g->battle_is_nexus,
        .pvp = g->battle_is_pvp,
        .trophy_delta = g->battle_reward.trophy_delta,
        .banner_delta = g->battle_reward.banner_delta,
    };
}

static void GameStartPendingNexusAnimation(Game *g) {
    PlayerNexusRun *run = &g->player.nexus_run;
    if (!run->pending_animation) return;
    run->pending_animation = false;
    int from = run->pending_from_step, to = run->pending_to_step;
    ProgressionSave(&g->player);

    const NexusDifficulty *d = NexusActiveDifficulty(&g->nexus);
    if (!d || from < 1 || from > d->node_count || to < 1 || to > d->node_count) return;
    if (from == to && run->nodes[from - 1].wins > 0 && from == d->node_count)
        NexusBeginCompletion(&g->nexus);
    else
        NexusBeginProgressionAnimation(&g->nexus, from, to);
}

static void GameOpenNexusRecap(Game *g) {
    HudCurrencyHold(&g->hud, false);
    NexusAward award = NexusAwardEscrow(&g->player);
    if (!HeroesSyncRoster(&g->heroes, &g->player)) TraceLog(LOG_FATAL, "Nexus: granted units cannot sync to Heroes");
    ProgressionSave(&g->player);
    NexusRecapOpen(&g->nexus_recap, &award);
}

static void GameReplayNexusMap(Game *g) {
    int event = g->nexus.active_event, difficulty = g->nexus.active_difficulty;
    if (NexusEventUnlocked(&g->nexus, &g->player, event) && NexusEnterMap(&g->nexus, &g->player, event, difficulty))
        ProgressionSave(&g->player);
    else
        g->screen = SCREEN_QUESTS;
}

static void GameStartPendingCampaignAnimation(Game *g) {
    PlayerCampaignAnimation animation = g->player.campaign_progress.pending_animation;
    if (!animation.active) return;
    g->player.campaign_progress.pending_animation = (PlayerCampaignAnimation){0};
    ProgressionSave(&g->player);

    bool valid = animation.from_map >= 0 && animation.from_map < g->map.map_count && animation.to_map >= 0 &&
                 animation.to_map < g->map.map_count && animation.from_node >= 0 &&
                 animation.from_node < g->map.maps[animation.from_map].node_count && animation.to_node >= 0 &&
                 animation.to_node < g->map.maps[animation.to_map].node_count;
    if (!valid) {
        TraceLog(LOG_WARNING, "Campaign: discarded invalid pending progression animation");
        return;
    }

    if (animation.unlock_next)
        CampaignBeginProgressionAnimation(
            &g->map, animation.from_map, animation.from_node, animation.to_map, animation.to_node
        );
    else
        CampaignBeginMasteryAnimation(&g->map, animation.from_map, animation.from_node, animation.to_node);
}


static void GameExitBattleTo(Game *g, Screen destination) {
    BattlePlayStop(&g->battle_play);
    BattleResultsFlowReset(&g->battle_results);
    BattleStopAmbience(&g->battle);
    BattleExitNode(&g->battle);
    MusicPlayerPlayRandom(&g->music);
    g->battle_energy_spent = false;
    GameClearBattleOutcome(g);
    g->screen = destination;
    if (destination == SCREEN_MAP) GameStartPendingCampaignAnimation(g);
    if (destination == SCREEN_NEXUS) GameStartPendingNexusAnimation(g);
    if (destination != SCREEN_NEXUS && destination != SCREEN_BATTLE) g->battle_is_nexus = false;
    if (destination != SCREEN_BATTLE) g->battle_is_pvp = false;
    if (destination != SCREEN_BATTLE) g->pvp_event = false;
    if (destination == SCREEN_PVP_HUB) PvpHubOpen(&g->pvp_hub, &g->player, (long long)time(NULL));
    if (destination == SCREEN_MISSIONS) MissionsSelectTab(&g->missions, &g->player, g->missions.tab);
}

static void GameContinuePvp(Game *g) {
    if (!g->pvp_can_next) {
        GameExitBattleTo(g, g->pvp_return_screen);
        return;
    }
    BattlePlayStop(&g->battle_play);
    BattleResultsFlowReset(&g->battle_results);
    BattleStopAmbience(&g->battle);
    GameEnterLadderBattle(g, g->pvp_return_screen);
}

static void GameOpenPvpEvent(Game *g) {
    PvpEventSync(&g->player.pvp, (long long)time(NULL));
    g->screen = SCREEN_PVP_EVENT;
}

static void GameOpenPvpHub(Game *g) {
    PvpHubOpen(&g->pvp_hub, &g->player, (long long)time(NULL));
    g->screen = SCREEN_PVP_HUB;
}

static void GameReturnToBattleMatchmaking(Game *g) {
    BattlePlayStop(&g->battle_play);
    BattleResultsFlowReset(&g->battle_results);
    BattleStopAmbience(&g->battle);

    g->battle_energy_spent = false;
    GameClearBattleOutcome(g);
    GameEnterSelectedBattle(g);
}

#define STARTUP_IMAGE_MANIFEST HOD_CACHE "startup_images.txt"

void GameInitialize(Game *g) {
    g->screen = SCREEN_HOME;
    ProgressionInitializePlayerDefaults(&g->player);

    ResourcePrefetchImages(STARTUP_IMAGE_MANIFEST);
    ResourceRecordImagesBegin();
    AssetsLoad();
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    MissionCatalogLoad("data/missions.json");

    PlayerLoadRoster(&g->player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    ProgressionLoad(&g->player);
    PvpEventSync(&g->player.pvp, (long long)time(NULL));
    if (PvpSimulateDefense(&g->player.pvp, g->player.level, (long long)time(NULL)) > 0) ProgressionSave(&g->player);
    PlayerGallerySync(&g->player);

    HudInitialize(&g->hud);
    HomeUIInitialize(&g->home_ui);
    HomeBackgroundInitialize(&g->scene, HOD_CACHE "meshes", HOD_CACHE "char_pack", &g->player);
    HeroesBackgroundInitialize(&g->heroes_scene, HOD_CACHE "meshes", HOD_CACHE "char_pack", &g->player);
    BattleInitialize(&g->battle, HOD_CACHE "meshes");
    BattleUiInitialize(&g->battle_ui);
    memset(&g->battle_play, 0, sizeof(g->battle_play));
    MusicPlayerInitialize(&g->music);
    HeroesInitialize(&g->heroes, &g->player);
    RunesInitialize(&g->runes);
    ConsumeInitialize(&g->consume, &g->player);
    GalleryInitialize(&g->gallery, HOD_CACHE "characters.json");
    RuneGrantStarterSet(&g->player.runes, RuneNow());
    StoreInitialize(&g->store, "data/store_packs.json", HOD_CACHE "characters.json");
    QuestsInitialize(&g->quests);
    CampaignInitialize(&g->map, "data/campaign_maps.json");
    MissionsInitialize(&g->missions);
    PvpHubInitialize(&g->pvp_hub);
    PvpEventViewInitialize(&g->pvp_event_view);
    MissionsSync(&g->player, &g->map);
    CampaignProgressSync(&g->map, &g->player.campaign_progress);
    NexusInitialize(&g->nexus, "data/nexus_maps.json");
    NewsInitialize(&g->news, &g->nexus, "data/news_feed.json");
    NexusRecapInitialize(&g->nexus_recap);
    EnergyDialogInitialize(&g->energy_dialog);
    StaminaDialogInitialize(&g->stamina_dialog);
    g->stamina_dialog_top = false;
    g->battle_map_index = 0;
    g->battle_node_index = 0;
    g->battle_squad_index = 0;
    g->battle_energy_cost = 0;
    g->battle_energy_spent = false;
    GameClearBattleOutcome(g);
    BattleResultsFlowInitialize(&g->battle_results);
    ResourceRecordImagesEnd(STARTUP_IMAGE_MANIFEST);
}

static bool GameResourceDialogOpen(const Game *g) {
    return EnergyDialogIsOpen(&g->energy_dialog) || EnergyDialogIsOpen(&g->stamina_dialog);
}

static EnergyDialog *GameTopResourceDialog(Game *g) {
    bool energy_open = EnergyDialogIsOpen(&g->energy_dialog);
    bool stamina_open = EnergyDialogIsOpen(&g->stamina_dialog);
    if (!energy_open && !stamina_open) return NULL;
    if (energy_open && stamina_open) return g->stamina_dialog_top ? &g->stamina_dialog : &g->energy_dialog;
    g->stamina_dialog_top = stamina_open;
    return stamina_open ? &g->stamina_dialog : &g->energy_dialog;
}

bool GameHandlePopupBack(Game *g) {
    EnergyDialog *top = GameTopResourceDialog(g);
    if (top) {
        EnergyDialogClose(top);
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (GalleryHandleBack(&g->gallery)) return true;
    if (g->screen == SCREEN_GALLERY) {
        g->screen = SCREEN_HEROES;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (g->screen == SCREEN_HEROES && GearViewIsOpen()) {
        GearViewClose();
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (g->screen == SCREEN_RUNES) return RunesHandleBack(&g->runes);
    if (g->screen == SCREEN_CONSUME) return ConsumeHandleBack(&g->consume);
    if (g->screen == SCREEN_HOME && NewsHandleBack(&g->news)) return true;
    if (g->screen == SCREEN_QUESTS && QuestsHandleBack(&g->quests)) return true;
    if (g->screen == SCREEN_MISSIONS || g->screen == SCREEN_PVP_HUB || g->screen == SCREEN_PVP_EVENT) {
        g->screen = SCREEN_HOME;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (g->screen == SCREEN_MAP && CampaignHandleBack(&g->map)) return true;
    if (g->screen == SCREEN_NEXUS && NexusRecapHandleBack(&g->nexus_recap, &g->hud, &g->player)) {
        if (!NexusRecapOpened(&g->nexus_recap)) g->screen = SCREEN_QUESTS;
        return true;
    }
    if (g->screen == SCREEN_NEXUS && NexusHandleBack(&g->nexus)) return true;
    if (g->screen == SCREEN_STORE) {
        if (StoreHandleBack(&g->store, &g->heroes) == STORE_ACTION_CLOSE) g->screen = SCREEN_HOME;
        return true;
    }
    return false;
}

static void GamePlayRuneActivationFx(Game *g) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (PlayerSquadUnitAt(&g->player, g->player.current_squad, slot) >= 0)
            BattleFxBurst(
                &g->battle, RUNE_ACTIVATION_FX, Vector3Add(HeroesBackgroundSlotPosition(slot), RUNE_ACTIVATION_FX_LIFT)
            );
}

static void GameOpenRunes(Game *g, Screen return_screen) {
    g->runes_return_screen = return_screen;
    RunesOpen(&g->runes, &g->player);
    g->screen = SCREEN_RUNES;
}

static void GameReturnFromRunes(Game *g) {
    HomeBackgroundMarkSquadStale(&g->scene);
    if (g->runes_return_screen != SCREEN_BATTLE) {
        g->screen = g->runes_return_screen;
        return;
    }
    g->screen = SCREEN_BATTLE;
    if (g->battle_is_nexus)
        GameEnterSelectedNexusBattle(g);
    else
        GameEnterSelectedBattle(g);
}

static void GameEmitWispFx(Game *g) {
    for (int i = 0; i < home_wisp_queue_count; i++) {
        HomeWispRequest *request = &home_wisp_queue[i];
        HomeSquadUnit *unit = request->unit;
        if (!BattleFxLoopMove(&g->battle, unit->wisp_loop, unit->wisp_loop_serial, request->pos))
            unit->wisp_loop = BattleFxLoopStart(&g->battle, unit->wisp_fx, request->pos, &unit->wisp_loop_serial);
        if (unit->wisp_loop < 0) continue;
        g->wisp_loops[g->wisp_loop_count % GAME_WISP_LOOP_CAPACITY] =
            (GameWispLoop){unit->wisp_loop, unit->wisp_loop_serial, g->wisp_frame};
        g->wisp_loop_count = (g->wisp_loop_count + 1) % GAME_WISP_LOOP_CAPACITY;
    }
    home_wisp_queue_count = 0;
}

static void GameStopUnseenWispLoops(Game *g) {
    home_wisp_queue_count = 0;
    for (int i = 0; i < GAME_WISP_LOOP_CAPACITY; i++) {
        GameWispLoop *loop = &g->wisp_loops[i];
        if (loop->serial == 0) continue;
        bool drawn_later = false;
        for (int other = 0; other < GAME_WISP_LOOP_CAPACITY; other++)
            drawn_later |= g->wisp_loops[other].serial == loop->serial && g->wisp_loops[other].frame == g->wisp_frame;
        if (drawn_later) continue;
        BattleFxLoopStop(&g->battle, loop->index, loop->serial);
        *loop = (GameWispLoop){0};
    }
    g->wisp_frame++;
}

static void GameDrawSceneFx(Game *g, Camera3D camera, float dt) {
    GameEmitWispFx(g);
    BattleFxPlayOnCamera(&g->battle, camera, dt);
}

static void GameOpenConsume(Game *g) {
    ConsumeOpen(&g->consume, &g->player);
    g->consume_attack_pending = CONSUME_ATTACK_NONE;
    const Character *empty[CONSUME_MAX_SEEDS + 1] = {0};
    HeroesBackgroundShowConsumeBoard(&g->heroes_scene, empty);
    ConsumeEnvSnapCamera(&g->heroes_scene.consume_env, CONSUME_CAM_BOARD);
    ConsumeEnvUpdate(&g->heroes_scene.consume_env, 0.0f);
    g->screen = SCREEN_CONSUME;
}

static void GameHandleConsumeLeave(Game *g, ConsumeLeave leave) {
    switch (leave) {
        case CONSUME_LEAVE_SQUAD:
            g->screen = SCREEN_HEROES;
            break;
        case CONSUME_LEAVE_HOME:
            g->screen = SCREEN_HOME;
            break;
        case CONSUME_LEAVE_QUESTS:
            g->screen = SCREEN_QUESTS;
            break;
        case CONSUME_LEAVE_PVP:
            GameOpenPvpHub(g);
            break;
        case CONSUME_LEAVE_RUNES:
            GameOpenRunes(g, SCREEN_HEROES);
            break;
        case CONSUME_LEAVE_STORE:
            g->screen = SCREEN_HEROES;
            g->store_return_screen = SCREEN_HEROES;
            StoreOpen(&g->store);
            g->screen = SCREEN_STORE;
            break;
        case CONSUME_LEAVE_GALLERY:
            g->screen = SCREEN_HEROES;
            GalleryOpenPopup(&g->gallery, &g->player);
            break;
        default:
            break;
    }
}

static void GameUpdateConsumeBoard(Game *g) {
    const Character *units[CONSUME_MAX_SEEDS + 1];
    if (ConsumeTakeBoardUpdate(&g->consume, &g->player, units, &g->consume_shown_target))
        HeroesBackgroundShowConsumeBoard(&g->heroes_scene, units);
    int placed_spot = ConsumeTakePlacedSpot(&g->consume);
    if (placed_spot >= 0) HeroesBackgroundPlayMelee(&g->heroes_scene, placed_spot);
    ConsumeEnvSetCamera(&g->heroes_scene.consume_env, ConsumeWantedCamera(&g->consume, &g->player));
    Vector3 target_root = HeroesBackgroundConsumeTargetRoot(&g->heroes_scene);
    const char *fx = ConsumeTakeFxRequest(&g->consume);
    if (fx) BattleFxBurst(&g->battle, fx, target_root);
    ConsumeAttackClip attack = ConsumeTakeAttackRequest(&g->consume);
    if (attack == CONSUME_ATTACK_LEVEL) BattleFxBurst(&g->battle, CONSUME_LEVEL_BEAM_FX, target_root);
    if (attack > g->consume_attack_pending) g->consume_attack_pending = attack;
    if (g->consume_attack_pending == CONSUME_ATTACK_NONE || HeroesBackgroundAttackPlaying(&g->heroes_scene, 0)) return;
    if (g->consume_attack_pending == CONSUME_ATTACK_TIER)
        HeroesBackgroundPlayMelee(&g->heroes_scene, 0);
    else
        HeroesBackgroundPlayProjectile(&g->heroes_scene, 0);
    g->consume_attack_pending = CONSUME_ATTACK_NONE;
}

static void GameLeaveRunesTo(Game *g, Screen destination) {
    if (g->runes_return_screen == SCREEN_BATTLE) GameExitBattleTo(g, destination);
    g->screen = destination;
}

static void GameHandleRunesLeave(Game *g, RunesLeave leave) {
    switch (leave) {
        case RUNES_LEAVE_SQUAD:
            GameLeaveRunesTo(g, SCREEN_HEROES);
            break;
        case RUNES_LEAVE_RETURN:
            GameReturnFromRunes(g);
            break;
        case RUNES_LEAVE_HOME:
            GameLeaveRunesTo(g, SCREEN_HOME);
            break;
        case RUNES_LEAVE_STORE:
            GameLeaveRunesTo(g, SCREEN_HEROES);
            g->store_return_screen = SCREEN_HEROES;
            StoreOpen(&g->store);
            StoreShowItems(&g->store);
            g->screen = SCREEN_STORE;
            break;
        case RUNES_LEAVE_GALLERY:
            GameLeaveRunesTo(g, SCREEN_HEROES);
            GalleryOpenPopup(&g->gallery, &g->player);
            break;
        case RUNES_LEAVE_CONSUME:
            GameLeaveRunesTo(g, SCREEN_HEROES);
            GameOpenConsume(g);
            break;
        case RUNES_LEAVE_SELECT_SQUAD:
            PlayerSquadSelect(&g->player, g->runes.leave_squad);
            HeroesBackgroundReloadSquad(&g->heroes_scene, &g->player);
            HomeBackgroundMarkSquadStale(&g->scene);
            break;
        default:
            break;
    }
}

static void GameToggleResourceDialog(Game *g, bool stamina) {
    EnergyDialog *target = stamina ? &g->stamina_dialog : &g->energy_dialog;
    EnergyDialog *other = stamina ? &g->energy_dialog : &g->stamina_dialog;
    bool target_open = EnergyDialogIsOpen(target);
    bool other_open = EnergyDialogIsOpen(other);
    bool target_top = target_open && (!other_open || g->stamina_dialog_top == stamina);
    if (target_top) {
        EnergyDialogClose(target);
        if (other_open) g->stamina_dialog_top = !stamina;
        if (stamina) GuiSoundPlay(SND_UI_POPUP_CLOSE);
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return;
    }

    if (!target_open) EnergyDialogOpen(target);
    g->stamina_dialog_top = stamina;
    GuiSoundPlay(other_open ? SND_UI_TAB : SND_UI_HOME);
}

static void GameOpenResourceDialogSilently(Game *g, bool stamina) {
    EnergyDialog *target = stamina ? &g->stamina_dialog : &g->energy_dialog;
    if (!EnergyDialogIsOpen(target)) EnergyDialogOpen(target);
    g->stamina_dialog_top = stamina;
}

static void GameHandleHudAction(Game *g, HudAction a) {
    switch (a) {
        case HUD_HOME:
            if (EnergyDialogIsOpen(&g->stamina_dialog)) GuiSoundPlay(SND_UI_POPUP_CLOSE);
            EnergyDialogClose(&g->energy_dialog);
            EnergyDialogClose(&g->stamina_dialog);
            if (g->screen == SCREEN_BATTLE)
                GameExitBattleTo(g, SCREEN_HOME);
            else
                g->screen = SCREEN_HOME;
            break;
        case HUD_ENERGY:
            GameToggleResourceDialog(g, false);
            break;
        case HUD_STAMINA:
            GameToggleResourceDialog(g, true);
            break;
        case HUD_BANNERS:
            if (g->screen != SCREEN_BATTLE) GameOpenPvpEvent(g);
            break;
        case HUD_TROPHIES:
            if (g->screen == SCREEN_BATTLE) break;
            MissionsOpen(&g->missions, &g->player, GOALS_TAB_TROPHIES);
            g->screen = SCREEN_MISSIONS;
            break;
        case HUD_GOLD:
        case HUD_GEMS:
            if (g->screen == SCREEN_BATTLE) break;
            EnergyDialogClose(&g->energy_dialog);
            EnergyDialogClose(&g->stamina_dialog);
            if (g->screen != SCREEN_STORE) {
                g->store_return_screen = g->screen;
                StoreOpen(&g->store);
                g->screen = SCREEN_STORE;
            }
            StoreShowPage(&g->store, a == HUD_GEMS);
            break;
        case HUD_NONE:
            break;
        default:
            TraceLog(LOG_INFO, "HUD action %d (no target screen yet)", a);
            break;
    }
}

static void GameDrawGalleryPopup(Game *g, bool resource_modal) {
    UiSetInputBlocked(resource_modal);
    if (GalleryPopupDraw(&g->gallery, &g->player) != GALLERY_ACTION_SHOW_UNIT) return;
    GalleryScreenEnter(&g->gallery, &g->player, &g->scene);
    g->screen = SCREEN_GALLERY;
}

void GameDraw(Game *g) {
    float dt = GetFrameTime();
    Screen screen_at_frame_start = g->screen;
    bool pvp_battle = g->screen == SCREEN_BATTLE && g->battle_is_pvp;
    g->hud.show_banners = g->screen == SCREEN_PVP_EVENT || (pvp_battle && g->pvp_event);
    g->hud.show_trophies = g->screen == SCREEN_PVP_HUB || (pvp_battle && !g->pvp_event);
    if (g->screen != SCREEN_BATTLE) MusicPlayerUpdate(&g->music);
    PlayerUpdate(&g->player, dt);
    if (g->player.roster_count != g->player.gallery.synced_roster_count) PlayerGallerySync(&g->player);

    ClearBackground(RAYWHITE);

    bool resource_modal = GameResourceDialogOpen(g);
    UiSetInputBlocked(resource_modal);

    switch (g->screen) {
        case SCREEN_HOME: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);
            GameDrawSceneFx(g, g->scene.cam, dt);

            UiFrameBegin();
            if (NewsAutoShow(&g->news, &g->player)) ProgressionSave(&g->player);
            bool news_open = NewsIsOpen(&g->news);
            bool home_interactive = !news_open;
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, home_interactive, true));

            int missions_badge = MissionUnseenClaimableCount(&g->player.missions);
            missions_badge += PvpEventUnclaimedCount(&g->player.pvp);
            if (missions_badge == 0) missions_badge = abs(g->player.pvp.trophies - g->player.pvp.trophies_seen);
            HomeButton pressed = HomeUIDraw(&g->home_ui, home_interactive, g->player.gallery.badge, missions_badge);
            NewsDrawHome(&g->news, &g->quests, &g->nexus, &g->player, home_interactive && !resource_modal, dt);
            if (NewsTakePvpEventRequest(&g->news)) GameOpenPvpEvent(g);
            if (!news_open && NewsIsOpen(&g->news)) ProgressionSave(&g->player);
            if (NewsDrawOverlay(&g->news, &g->quests, &g->nexus, &g->player) == NEWS_OPEN_EVENT) {
                QuestsOpenEvent(&g->quests, &g->nexus, &g->player, NewsTargetEvent(&g->news));
                g->screen = SCREEN_QUESTS;
            }
            UiFrameEnd();
            if (pressed == HOME_NONE && home_interactive && !NewsIsOpen(&g->news) && !resource_modal &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                HomeBackgroundSquadClick(&g->scene, GetMousePosition());
            if (pressed == HOME_HEROES) {
                g->screen = SCREEN_HEROES;
            } else if (pressed == HOME_STORE) {
                g->store_return_screen = SCREEN_HOME;
                StoreOpen(&g->store);
                g->screen = SCREEN_STORE;
            } else if (pressed == HOME_QUESTS) {
                g->screen = SCREEN_QUESTS;
            } else if (pressed == HOME_HISTORY) {
                MissionsSync(&g->player, &g->map);
                MissionsOpen(&g->missions, &g->player, GOALS_TAB_MISSIONS);
                g->screen = SCREEN_MISSIONS;
            } else if (pressed == HOME_PVP) {
                GameOpenPvpHub(g);
            }
            break;
        }
        case SCREEN_STORE: {
            if (StoreUsesHomeBackground(&g->store)) {
                HomeBackgroundUpdate(&g->scene, resource_modal ? 0.0f : dt);
                HomeBackgroundDraw(&g->scene);
                GameDrawSceneFx(g, g->scene.cam, dt);
            } else {
                StoreDrawWorld(&g->store, &g->heroes);
                GameDrawSceneFx(g, g->store.opening_camera, dt);
            }

            UiFrameBegin();
            StoreAction store_action = StoreDraw(&g->store, &g->player, &g->heroes, resource_modal ? 0.0f : dt);
            if (!resource_modal && StoreTopHudVisible(&g->store)) {
                if (StoreUsesHomeBackground(&g->store))
                    HudSetStoreSelection(&g->hud, StoreIsOnGemsPage(&g->store) ? HUD_GEMS : HUD_GOLD);
                GameHandleHudAction(
                    g, HudDraw(&g->hud, &g->player, !StoreDialogOpen(&g->store), !StoreTopHudHomeActive(&g->store))
                );
                HudSetStoreSelection(&g->hud, HUD_NONE);
            }
            UiFrameEnd();

            if (store_action == STORE_ACTION_CLOSE) {
                g->screen = g->store_return_screen;
            } else if (store_action == STORE_ACTION_GOTO_MAP) {
                g->screen = SCREEN_MAP;
                GameStartPendingCampaignAnimation(g);
            } else if (store_action == STORE_ACTION_GOTO_SQUAD) {
                g->screen = SCREEN_HEROES;
            } else if (store_action == STORE_ACTION_GOTO_CONSUME) {
                GameOpenConsume(g);
            }
            break;
        }
        case SCREEN_HEROES: {
            HeroesBackgroundUpdate(&g->heroes_scene, dt);
            HeroesBackgroundDraw(&g->heroes_scene);
            GameDrawSceneFx(g, g->heroes_scene.cam, dt);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            bool gallery_modal = GalleryPopupOpen(&g->gallery);
            bool heroes_modal = resource_modal || gallery_modal;
            UiSetInputBlocked(heroes_modal);
            int prev_squad = g->player.current_squad;
            HeroesButton heroes_action = HeroesDraw(&g->heroes, &g->player);
            bool squad_tab_changed = g->player.current_squad != prev_squad;
            if (squad_tab_changed || g->screen != SCREEN_HEROES || heroes_action == HEROES_QUEST)
                HeroesBackgroundClearRowBonus(&g->heroes_scene);
            HeroesBackgroundDrawRowBonus(&g->heroes_scene);
            if (gallery_modal) GameDrawGalleryPopup(g, resource_modal);
            UiFrameEnd();

            if (heroes_action == HEROES_QUEST) {
                g->screen = SCREEN_QUESTS;
            } else if (GearViewTakeStoreRequest()) {
                GearViewClose();
                StoreOpen(&g->store);
                StoreShowItems(&g->store);
                g->screen = SCREEN_STORE;
            } else if (heroes_action == HEROES_RUNES) {
                GameOpenRunes(g, SCREEN_HEROES);
            } else if (heroes_action == HEROES_GALLERY) {
                GalleryOpenPopup(&g->gallery, &g->player);
            } else if (heroes_action == HEROES_CONSUME) {
                GameOpenConsume(g);
            } else if (heroes_action != HEROES_NONE && heroes_action != HEROES_SQUAD) {
                TraceLog(LOG_INFO, "Heroes action %d has no local target screen yet", heroes_action);
            }

            bool squad_changed = false;
            int req = HeroesTakeDragRequest(&g->heroes);
            if (req >= 0 && HeroesBackgroundDragStart(&g->heroes_scene, &g->player, req, -1)) g->heroes.drag_icon = req;
            if (!heroes_modal && !HeroesBoardInputBlocked(&g->heroes) &&
                !HeroesBackgroundDragActive(&g->heroes_scene) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                UiMouse().x > ROSTER_PANEL_WIDTH) {
                int slot = HeroesBackgroundSlotAtMouse(&g->heroes_scene, &g->player);
                if (slot >= 0) {
                    int unit = PlayerSquadUnitAt(&g->player, g->player.current_squad, slot);
                    HeroesBackgroundDragStart(&g->heroes_scene, &g->player, unit, slot);
                }
            }
            if (!heroes_modal && HeroesBackgroundDragActive(&g->heroes_scene)) {
                bool over_tray = UiMouse().x < ROSTER_PANEL_WIDTH;
                g->heroes.drag_icon = over_tray && !HeroesBackgroundDragFromBoard(&g->heroes_scene)
                                          ? HeroesBackgroundDragRosterIndex(&g->heroes_scene)
                                          : -1;
                HeroesBackgroundDragMove(&g->heroes_scene, over_tray);
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    squad_changed = HeroesBackgroundDragDrop(&g->heroes_scene, &g->player, over_tray);
                    g->heroes.drag_icon = -1;
                }
            }

            if (squad_tab_changed) {
                HeroesBackgroundReloadSquad(&g->heroes_scene, &g->player);
                squad_changed = true;
            }
            if (squad_changed) HomeBackgroundMarkSquadStale(&g->scene);
            break;
        }
        case SCREEN_RUNES: {
            HeroesBackgroundUpdate(&g->heroes_scene, dt);
            HeroesBackgroundDraw(&g->heroes_scene);
            while (RunesTakeActivationFx(&g->runes)) GamePlayRuneActivationFx(g);
            GameDrawSceneFx(g, g->heroes_scene.cam, dt);

            UiFrameBegin();
            HudAction hud_action = resource_modal ? HUD_NONE : HudDraw(&g->hud, &g->player, true, false);
            if (hud_action == HUD_HOME) {
                RunesRequestLeave(&g->runes, RUNES_LEAVE_HOME, 0);
                hud_action = HUD_NONE;
            }
            RunesLeave leave = RunesDraw(&g->runes, &g->heroes, &g->player, dt);
            UiFrameEnd();
            GameHandleRunesLeave(g, leave);
            GameHandleHudAction(g, hud_action);
            break;
        }
        case SCREEN_CONSUME: {
            GameUpdateConsumeBoard(g);
            HeroesBackgroundUpdate(&g->heroes_scene, dt);
            HeroesBackgroundDraw(&g->heroes_scene);
            GameDrawSceneFx(g, HeroesBackgroundConsumeCamera(&g->heroes_scene), dt);

            UiFrameBegin();
            HudAction hud_action = resource_modal ? HUD_NONE : HudDraw(&g->hud, &g->player, true, false);
            if (hud_action == HUD_HOME) {
                ConsumeRequestLeave(&g->consume, CONSUME_LEAVE_HOME);
                hud_action = HUD_NONE;
            }
            ConsumeLeave leave = ConsumeDraw(&g->consume, &g->heroes_scene, &g->heroes, &g->player, dt);
            UiFrameEnd();
            GameHandleConsumeLeave(g, leave);
            GameHandleHudAction(g, hud_action);
            break;
        }
        case SCREEN_GALLERY: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundSetGalleryView(&g->scene, true);
            HomeBackgroundDraw(&g->scene);
            HomeBackgroundSetGalleryView(&g->scene, false);
            GameDrawSceneFx(g, g->scene.cam, dt);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            bool gallery_modal = GalleryPopupOpen(&g->gallery);
            UiSetInputBlocked(resource_modal || gallery_modal);
            GalleryScreenAction gallery_action = GalleryScreenDraw(&g->gallery, &g->heroes, &g->player, &g->scene);
            if (gallery_modal) GameDrawGalleryPopup(g, resource_modal);
            UiFrameEnd();

            if (g->screen == SCREEN_GALLERY) {
                if (gallery_action == GALLERY_SCREEN_ACTION_SQUAD) {
                    g->screen = SCREEN_HEROES;
                } else if (gallery_action == GALLERY_SCREEN_ACTION_RUNES) {
                    GameOpenRunes(g, SCREEN_HEROES);
                } else if (gallery_action == GALLERY_SCREEN_ACTION_CONSUME) {
                    GameOpenConsume(g);
                } else if (gallery_action == GALLERY_SCREEN_ACTION_POPUP) {
                    GalleryOpenPopup(&g->gallery, &g->player);
                }
            }
            break;
        }
        case SCREEN_QUESTS: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);
            GameDrawSceneFx(g, g->scene.cam, dt);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            HomeUIDraw(&g->home_ui, false, g->player.gallery.badge, 0);

            QuestButton pressed = QuestsDraw(&g->quests, &g->nexus, &g->player, &g->map);
            UiFrameEnd();
            if (pressed == QUESTS_MAP) {
                g->screen = SCREEN_MAP;
                GameStartPendingCampaignAnimation(g);
            } else if (pressed == QUESTS_NEXUS) {
                int event = QuestsSelectedNexusEvent(&g->quests);
                int difficulty = QuestsSelectedNexusDifficulty(&g->quests);
                if (NexusEventUnlocked(&g->nexus, &g->player, event) &&
                    NexusEnterMap(&g->nexus, &g->player, event, difficulty)) {
                    ProgressionSave(&g->player);
                    g->screen = SCREEN_NEXUS;
                }
            } else if (pressed == QUESTS_UNLOCK) {
                if (NexusUnlockEvent(&g->player, &g->nexus, QuestsPendingNexusEvent(&g->quests)))
                    ProgressionSave(&g->player);
            } else if (pressed == QUESTS_FORFEIT) {
                NexusResetRun(&g->player);
                ProgressionSave(&g->player);
                QuestsOpenLadder(&g->quests, QuestsPendingNexusEvent(&g->quests));
            } else if (pressed == QUESTS_NEWS) {
                NewsOpen(&g->news, &g->player);
                ProgressionSave(&g->player);
                g->screen = SCREEN_HOME;
            } else if (pressed == QUESTS_CLOSE) {
                g->screen = SCREEN_HOME;
            }
            break;
        }
        case SCREEN_MISSIONS: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);
            GameDrawSceneFx(g, g->scene.cam, dt);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            HomeUIDraw(&g->home_ui, false, g->player.gallery.badge, 0);
            MissionsAction pressed = MissionsDraw(&g->missions, &g->hud, &g->player, dt);
            UiFrameEnd();
            long long now = (long long)time(NULL);
            if (pressed == MISSIONS_CLOSE) {
                g->screen = SCREEN_HOME;
            } else if (pressed == MISSIONS_FIGHT_BOT) {
                unsigned seed = (unsigned)GetRandomValue(1, 0x7FFFFFFF);
                GameEnterFixedPvpBattle(
                    g,
                    PvpOpponentFromBot(
                        g->missions.tabs.selected, g->player.level, g->player.pvp.trophies, PvpDay(now), seed
                    ),
                    -1,
                    SCREEN_MISSIONS
                );
            } else if (pressed == MISSIONS_REVENGE) {
                int index = g->missions.tabs.selected;
                PvpOpponent opponent = PvpOpponentFromBattle(&g->player.pvp.history[index], g->player.pvp.trophies);
                GameEnterFixedPvpBattle(g, opponent, index, SCREEN_MISSIONS);
            }
            break;
        }
        case SCREEN_PVP_HUB: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);
            GameDrawSceneFx(g, g->scene.cam, dt);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            HomeUIDraw(&g->home_ui, false, g->player.gallery.badge, 0);
            PvpHubAction pressed = PvpHubDraw(&g->pvp_hub, &g->player);
            UiFrameEnd();
            if (pressed == PVP_HUB_CLOSE) g->screen = SCREEN_HOME;
            if (pressed == PVP_HUB_EVENT) GameOpenPvpEvent(g);
            if (pressed == PVP_HUB_LADDER) {
                g->pvp_event = false;
                GameEnterLadderBattle(g, SCREEN_PVP_HUB);
            }
            break;
        }
        case SCREEN_PVP_EVENT: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);
            GameDrawSceneFx(g, g->scene.cam, dt);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            HomeUIDraw(&g->home_ui, false, g->player.gallery.badge, 0);
            PvpEventViewAction pressed = PvpEventViewDraw(&g->pvp_event_view, &g->player);
            UiFrameEnd();
            if (pressed == PVP_EVENT_VIEW_CLOSE) g->screen = SCREEN_HOME;
            if (pressed == PVP_EVENT_VIEW_ENTER) {
                g->pvp_event = true;
                GameEnterLadderBattle(g, SCREEN_PVP_EVENT);
            }
            break;
        }
        case SCREEN_MAP: {
            UiFrameBegin();
            CampaignAction act = CampaignDraw(&g->map, &g->player.campaign_progress);
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            UiFrameEnd();
            if (act == CAMPAIGN_ENTER) {
                int cost = CampaignSelectedEnergyCost(&g->map, &g->player.campaign_progress);
                if (g->player.energy >= cost) {
                    g->battle_map_index = g->player.campaign_progress.campaign_index;
                    g->battle_node_index = g->map.selected_node;
                    g->battle_squad_index = g->player.current_squad;
                    g->battle_energy_cost = cost;
                    g->battle_energy_spent = false;
                    GameClearBattleOutcome(g);
                    GameEnterSelectedBattle(g);
                    g->screen = SCREEN_BATTLE;
                } else {
                    GameOpenResourceDialogSilently(g, false);
                }
            }
            break;
        }
        case SCREEN_NEXUS: {
            if (!NexusMapOpen(&g->nexus)) {
                g->screen = SCREEN_QUESTS;
                break;
            }
            NexusAction act = NexusDraw(&g->nexus, &g->player, &g->player.nexus_run);
            if (act == NEXUS_SHOW_RECAP) GameOpenNexusRecap(g);
            UiFrameBegin();
            NexusRecapAction recap_action = NexusRecapDraw(&g->nexus_recap, &g->heroes, &g->hud, &g->player);
            bool hud_interactive = !NexusCompletionActive(&g->nexus);
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, hud_interactive, false));
            UiFrameEnd();
            if (recap_action == NEXUS_RECAP_REPLAY) GameReplayNexusMap(g);
            if (recap_action == NEXUS_RECAP_RETURN) g->screen = SCREEN_QUESTS;
            if (act == NEXUS_ENTER || act == NEXUS_REVIVE) {
                int cost = NexusSelectedEnergyCost(&g->nexus, &g->player);
                bool paid = act != NEXUS_REVIVE || NexusReviveRun(&g->player);
                if (paid && g->player.energy >= cost) {
                    g->battle_is_nexus = true;
                    g->battle_node_index = g->nexus.selected_node;
                    g->battle_squad_index = g->player.current_squad;
                    g->battle_energy_cost = cost;
                    g->battle_energy_spent = false;
                    GameClearBattleOutcome(g);
                    GameEnterSelectedNexusBattle(g);
                    g->screen = SCREEN_BATTLE;
                } else if (paid) {
                    GameOpenResourceDialogSilently(g, false);
                }
            } else if (act == NEXUS_QUIT || act == NEXUS_LEAVE) {
                if (act == NEXUS_QUIT && !NexusRunComplete(&g->nexus, &g->player.nexus_run)) NexusResetRun(&g->player);
                if (act == NEXUS_LEAVE) QuestsOpenLadder(&g->quests, g->nexus.active_event);
                g->screen = SCREEN_QUESTS;
            }
            break;
        }
        case SCREEN_BATTLE: {
            bool showing_results = BattleResultsFlowActive(&g->battle_results);
            if (!showing_results) {
                BattlePlayUpdate(&g->battle_play, &g->battle, dt);
                if (!g->battle_outcome_committed &&
                    (g->battle_play.state == BP_END_SPLASH || g->battle_play.state == BP_SUMMARY))
                    GameCommitBattleOutcome(g, g->battle_play.won);
            }
            BattleUpdate(&g->battle, showing_results ? dt : dt * BattlePlayTimeScale(&g->battle_play));
            BattleDraw(&g->battle);
            GameEmitWispFx(g);

            UiFrameBegin();
            bool combat = BattlePlayActive(&g->battle_play);
            HudAction a = HUD_NONE;
            BattleUiAction ba = BATTLE_UI_NONE;
            if (showing_results) {
                if (!resource_modal && BattleResultsFlowTopUiVisible(&g->battle_results)) {
                    Player hud_player = BattleResultsFlowHudPlayer(&g->battle_results, &g->player);
                    a = HudDraw(&g->hud, &hud_player, true, false);
                }
                BattleResultsFlowDraw(&g->battle_results, &g->player, &g->heroes, g->battle_squad_index);
            } else {
                if (!resource_modal && !combat) a = HudDraw(&g->hud, &g->player, true, false);
                ba = BattleUiDraw(&g->battle_ui, &g->player, &g->battle, &g->battle_play);
            }
            UiFrameEnd();

            bool leave = false;
            if (showing_results) {
                BattleResultsAction result_action = BattleResultsFlowUpdate(&g->battle_results, dt, !resource_modal);
                if (result_action != BATTLE_RESULTS_ACTION_NONE) {
                    bool completion_route = g->battle_reward.first_win || g->battle_reward.mastered;
                    if (g->battle_is_pvp) {
                        if (result_action == BATTLE_RESULTS_ACTION_CONTINUE)
                            GameContinuePvp(g);
                        else
                            GameExitBattleTo(
                                g,
                                result_action == BATTLE_RESULTS_ACTION_HEROES ? SCREEN_HEROES : g->pvp_return_screen
                            );
                    } else if (result_action == BATTLE_RESULTS_ACTION_CONTINUE && !completion_route &&
                               !g->battle_is_nexus) {
                        GameReturnToBattleMatchmaking(g);
                    } else {
                        Screen map_screen = g->battle_is_nexus ? SCREEN_NEXUS : SCREEN_MAP;
                        Screen destination = result_action == BATTLE_RESULTS_ACTION_HEROES ? SCREEN_HEROES : map_screen;
                        GameExitBattleTo(g, destination);
                    }
                }
            } else {
                if (ba == BATTLE_UI_FIGHT && !combat) {
                    bool paid = g->battle_energy_spent ||
                                (g->battle_is_pvp ? PlayerSpendStamina(&g->player, g->battle_energy_cost)
                                                  : PlayerSpendEnergy(&g->player, g->battle_energy_cost));
                    if (paid) {
                        g->battle_energy_spent = true;
                        GameClearBattleOutcome(g);
                        ProgressionSave(&g->player);
                        BattlePlayStart(
                            &g->battle_play, &g->battle, &g->player, (unsigned)GetRandomValue(1, 0x7FFFFFFF)
                        );
                        GameCommitBattleOutcome(g, g->battle_play.sim.player_won);
                    } else {
                        GameOpenResourceDialogSilently(g, g->battle_is_pvp);
                    }
                }
                if (ba == BATTLE_UI_NEXT && !combat && PvpSpendNext(&g->player.pvp, (long long)time(NULL))) {
                    ProgressionSave(&g->player);
                    g->pvp_opponent = GameLadderOpponent(g);
                    GameEnterPvpOpponent(g);
                }
                leave = ba == BATTLE_UI_RETREAT;
                bool battle_continue = !resource_modal && (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ||
                                                           IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));
                if (g->battle_play.state == BP_END_SPLASH && battle_continue) {
                    g->battle_play.state = BP_SUMMARY;
                    g->battle_play.t = 0.0f;
                } else if (BattlePlayFinished(&g->battle_play) && battle_continue) {
                    BattleResultsFlowStart(&g->battle_results, GameBattleResultsSnapshot(g));
                }
            }
            if (leave) {
                Screen retreat_screen = g->battle_is_pvp      ? g->pvp_return_screen
                                        : g->battle_is_nexus ? SCREEN_NEXUS
                                                             : SCREEN_MAP;
                GameExitBattleTo(g, retreat_screen);
            } else if (ba == BATTLE_UI_RUNES && !combat) {
                BattleStopAmbience(&g->battle);
                GameOpenRunes(g, SCREEN_BATTLE);
            }
            if (a == HUD_HOME) {
                GameExitBattleTo(g, SCREEN_HOME);
                a = HUD_NONE;
            }
            GameHandleHudAction(g, a);
            break;
        }
    }

    UiSetInputBlocked(false);
    HudAction popup_hud_action = HUD_NONE;
    EnergyDialogAction resource_dialog_action = ENERGY_DIALOG_ACTION_NONE;
    EnergyDialog *top_resource_dialog = GameTopResourceDialog(g);
    if (top_resource_dialog) {
        UiFrameBegin();
        resource_dialog_action = EnergyDialogDraw(top_resource_dialog, &g->player, true);
        Player popup_hud_player = g->player;
        if (g->screen == SCREEN_BATTLE && BattleResultsFlowActive(&g->battle_results) &&
            BattleResultsFlowTopUiVisible(&g->battle_results))
            popup_hud_player = BattleResultsFlowHudPlayer(&g->battle_results, &g->player);
        popup_hud_action = HudDraw(&g->hud, &popup_hud_player, resource_modal, false);
        UiFrameEnd();
    }
    if (resource_dialog_action == ENERGY_DIALOG_ACTION_PVP) {
        EnergyDialogClose(&g->energy_dialog);
        EnergyDialogClose(&g->stamina_dialog);
        if (g->screen == SCREEN_BATTLE)
            GameExitBattleTo(g, SCREEN_PVP_HUB);
        else
            GameOpenPvpHub(g);
    }
    if (resource_dialog_action == ENERGY_DIALOG_ACTION_QUESTS) {
        EnergyDialogClose(&g->energy_dialog);
        EnergyDialogClose(&g->stamina_dialog);
        if (g->screen == SCREEN_BATTLE)
            GameExitBattleTo(g, SCREEN_QUESTS);
        else
            g->screen = SCREEN_QUESTS;
    }
    GameHandleHudAction(g, popup_hud_action);
    if (screen_at_frame_start == SCREEN_HEROES && g->screen != SCREEN_HEROES && g->screen != SCREEN_RUNES)
        HeroesBackgroundClearRowBonus(&g->heroes_scene);
    if (screen_at_frame_start == SCREEN_CONSUME && g->screen != SCREEN_CONSUME) {
        HeroesBackgroundReloadSquad(&g->heroes_scene, &g->player);
        HomeBackgroundMarkSquadStale(&g->scene);
    }
    if (screen_at_frame_start != g->screen) MissionsSync(&g->player, &g->map);
    if (screen_at_frame_start == SCREEN_QUESTS && g->screen != SCREEN_QUESTS) QuestsClose(&g->quests);
    if (screen_at_frame_start == SCREEN_MISSIONS && g->screen != SCREEN_MISSIONS)
        MissionsClose(&g->missions, &g->hud, &g->player);
    if (g->screen != SCREEN_HEROES && g->screen != SCREEN_GALLERY && GalleryPopupOpen(&g->gallery))
        GalleryClosePopup(&g->gallery);
    if (screen_at_frame_start == SCREEN_GALLERY && g->screen != SCREEN_GALLERY && g->screen != SCREEN_STORE)
        HomeBackgroundGalleryClear(&g->scene);
    if (screen_at_frame_start == SCREEN_NEXUS && g->screen != SCREEN_NEXUS && g->screen != SCREEN_BATTLE &&
        NexusRunComplete(&g->nexus, &g->player.nexus_run)) {
        NexusAwardEscrow(&g->player);
        ProgressionSave(&g->player);
    }
    GameStopUnseenWispLoops(g);
}

void GameDestroy(Game *g) {
    ConsumeCatalogUnload();
    StoreDestroy(&g->store);
    GalleryDestroy(&g->gallery);
    RuneIconsUnload();
    HeroesDestroy(&g->heroes);
    LocUnload();
    ProgressionSave(&g->player);
    PlayerUnloadRoster(&g->player);
    HomeBackgroundDestroy(&g->scene);
    HeroesBackgroundDestroy(&g->heroes_scene);
    NexusUnload(&g->nexus);
    BattleDestroy(&g->battle);
    UnitPoolClear();
    PassiveManifestUnload();
    AssetsUnload();
    ResourceShutdown();
}
