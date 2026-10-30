typedef enum {
    SCREEN_HOME,
    SCREEN_STORE,
    SCREEN_HEROES,
    SCREEN_QUESTS,
    SCREEN_MAP,
    SCREEN_NEXUS,
    SCREEN_BATTLE
} Screen;

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
    Store store;
    Screen store_return_screen;
    Quests quests;
    Campaign map;
    Nexus nexus;
    EnergyDialog energy_dialog;
    EnergyDialog stamina_dialog;
    bool stamina_dialog_top;

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
    if (g->battle_is_nexus) {
        PlayerNexusRun *run = &g->player.nexus_run;
        int from_step = g->battle_node_index + 1;
        g->battle_reward =
            NexusResolveBattle(&g->player, &g->nexus, from_step, won, BattlePlayCriticalGold(&g->battle_play));
        run->pending_animation = won;
        run->pending_from_step = from_step;
        run->pending_to_step = run->current_node_step;
        ProgressionSave(&g->player);
        return;
    }
    g->battle_reward = CampaignResolveBattle(
        &g->player, &g->map, g->battle_map_index, g->battle_node_index, won, BattlePlayCriticalGold(&g->battle_play)
    );
    g->player.campaign_progress.pending_animation = GamePendingCampaignAnimation(&g->map, &g->battle_reward);
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

static void GameClearBattleOutcome(Game *g) {
    g->battle_outcome_committed = false;
    g->battle_reward = (BattleRewardResult){0};
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
}

static void GameReturnToBattleMatchmaking(Game *g) {
    BattlePlayStop(&g->battle_play);
    BattleResultsFlowReset(&g->battle_results);
    BattleStopAmbience(&g->battle);

    g->battle_energy_spent = false;
    GameClearBattleOutcome(g);
    GameEnterSelectedBattle(g);
}

void GameInitialize(Game *g) {
    g->screen = SCREEN_HOME;
    ProgressionInitializePlayerDefaults(&g->player);

    AssetsLoad();
    LocLoad(
        HOD_CACHE
        "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization/Loc_RUS_RU.txt"
    );

    PlayerLoadRoster(&g->player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    ProgressionLoad(&g->player);

    HudInitialize(&g->hud);
    HomeUIInitialize(&g->home_ui);
    HomeBackgroundInitialize(&g->scene, HOD_CACHE "meshes", HOD_CACHE "char_pack", &g->player);
    HeroesBackgroundInitialize(&g->heroes_scene, HOD_CACHE "meshes", HOD_CACHE "char_pack", &g->player);
    BattleInitialize(&g->battle, HOD_CACHE "meshes");
    BattleUiInitialize(&g->battle_ui);
    memset(&g->battle_play, 0, sizeof(g->battle_play));
    MusicPlayerInitialize(&g->music);
    HeroesInitialize(&g->heroes, &g->player);
    StoreInitialize(&g->store, "data/store_packs.json", HOD_CACHE "characters.json");
    QuestsInitialize(&g->quests);
    CampaignInitialize(&g->map, "data/campaign_maps.json");
    CampaignProgressSync(&g->map, &g->player.campaign_progress);
    NexusInitialize(&g->nexus, "data/nexus_maps.json");
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
    if (g->screen == SCREEN_STORE) {
        if (StoreHandleBack(&g->store, &g->heroes) == STORE_ACTION_CLOSE) g->screen = SCREEN_HOME;
        return true;
    }
    return false;
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

void GameDraw(Game *g) {
    float dt = GetFrameTime();
    Screen screen_at_frame_start = g->screen;
    if (g->screen != SCREEN_BATTLE) MusicPlayerUpdate(&g->music);
    PlayerUpdate(&g->player, dt);

    ClearBackground(RAYWHITE);

    bool resource_modal = GameResourceDialogOpen(g);
    UiSetInputBlocked(resource_modal);

    switch (g->screen) {
        case SCREEN_HOME: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, true));

            HomeButton pressed = HomeUIDraw(&g->home_ui, true);
            UiFrameEnd();
            if (pressed == HOME_HEROES) {
                g->screen = SCREEN_HEROES;
            } else if (pressed == HOME_STORE) {
                g->store_return_screen = SCREEN_HOME;
                StoreOpen(&g->store);
                g->screen = SCREEN_STORE;
            } else if (pressed == HOME_QUESTS) {
                g->screen = SCREEN_QUESTS;
            } else if (pressed != HOME_NONE) {
                TraceLog(LOG_INFO, "Main screen button pressed: %d", pressed);
            }
            break;
        }
        case SCREEN_STORE: {
            if (StoreUsesHomeBackground(&g->store)) {
                HomeBackgroundUpdate(&g->scene, resource_modal ? 0.0f : dt);
                HomeBackgroundDraw(&g->scene);
            } else {
                StoreDrawWorld(&g->store, &g->heroes);
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
            }
            break;
        }
        case SCREEN_HEROES: {
            HeroesBackgroundUpdate(&g->heroes_scene, dt);
            HeroesBackgroundDraw(&g->heroes_scene);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            int prev_squad = g->player.current_squad;
            HeroesButton heroes_action = HeroesDraw(&g->heroes, &g->player);
            bool squad_tab_changed = g->player.current_squad != prev_squad;
            if (squad_tab_changed || g->screen != SCREEN_HEROES || heroes_action == HEROES_QUEST)
                HeroesBackgroundClearRowBonus(&g->heroes_scene);
            HeroesBackgroundDrawRowBonus(&g->heroes_scene);
            UiFrameEnd();

            if (heroes_action == HEROES_QUEST) {
                g->screen = SCREEN_QUESTS;
            } else if (heroes_action != HEROES_NONE && heroes_action != HEROES_SQUAD) {
                TraceLog(LOG_INFO, "Heroes action %d has no local target screen yet", heroes_action);
            }

            bool squad_changed = false;
            int req = HeroesTakeDragRequest(&g->heroes);
            if (req >= 0 && HeroesBackgroundDragStart(&g->heroes_scene, &g->player, req, -1)) g->heroes.drag_icon = req;
            if (!resource_modal && !HeroesBackgroundDragActive(&g->heroes_scene) &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && UiMouse().x > 256.0f) {
                int slot = HeroesBackgroundSlotAtMouse(&g->heroes_scene, &g->player);
                if (slot >= 0) {
                    int unit = PlayerSquadUnitAt(&g->player, g->player.current_squad, slot);
                    HeroesBackgroundDragStart(&g->heroes_scene, &g->player, unit, slot);
                }
            }
            if (!resource_modal && HeroesBackgroundDragActive(&g->heroes_scene)) {
                bool over_tray = UiMouse().x < 256.0f;
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
            if (squad_changed) HomeBackgroundReloadSquad(&g->scene, &g->player);
            break;
        }
        case SCREEN_QUESTS: {
            HomeBackgroundUpdate(&g->scene, dt);
            HomeBackgroundDraw(&g->scene);

            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            HomeUIDraw(&g->home_ui, false);

            QuestButton pressed = QuestsDraw(&g->quests, &g->nexus, &g->player);
            UiFrameEnd();
            if (pressed == QUESTS_MAP) {
                g->screen = SCREEN_MAP;
                GameStartPendingCampaignAnimation(g);
            } else if (pressed == QUESTS_NEXUS) {
                int event = QuestsSelectedNexusEvent(&g->quests);
                int difficulty = QuestsSelectedNexusDifficulty(&g->quests);
                bool ready =
                    NexusEventUnlocked(&g->nexus, &g->player, event) || NexusUnlockEvent(&g->player, &g->nexus, event);
                if (ready && NexusEnterMap(&g->nexus, &g->player, event, difficulty)) {
                    ProgressionSave(&g->player);
                    g->screen = SCREEN_NEXUS;
                }
            } else if (pressed == QUESTS_CLOSE) {
                g->screen = SCREEN_HOME;
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
            UiFrameBegin();
            if (!resource_modal) GameHandleHudAction(g, HudDraw(&g->hud, &g->player, true, false));
            UiFrameEnd();
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
            } else if (act == NEXUS_COMPLETE || act == NEXUS_QUIT || act == NEXUS_LEAVE) {
                if (act == NEXUS_QUIT && !NexusRunComplete(&g->nexus, &g->player.nexus_run)) NexusResetRun(&g->player);
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
                    if (result_action == BATTLE_RESULTS_ACTION_CONTINUE && !completion_route && !g->battle_is_nexus) {
                        GameReturnToBattleMatchmaking(g);
                    } else {
                        Screen map_screen = g->battle_is_nexus ? SCREEN_NEXUS : SCREEN_MAP;
                        Screen destination = result_action == BATTLE_RESULTS_ACTION_HEROES ? SCREEN_HEROES : map_screen;
                        GameExitBattleTo(g, destination);
                    }
                }
            } else {
                if (ba == BATTLE_UI_FIGHT && !combat) {
                    if (g->battle_energy_spent || PlayerSpendEnergy(&g->player, g->battle_energy_cost)) {
                        g->battle_energy_spent = true;
                        GameClearBattleOutcome(g);
                        ProgressionSave(&g->player);
                        BattlePlayStart(
                            &g->battle_play, &g->battle, &g->player, (unsigned)GetRandomValue(1, 0x7FFFFFFF)
                        );
                        GameCommitBattleOutcome(g, g->battle_play.sim.player_won);
                    } else {
                        GameOpenResourceDialogSilently(g, false);
                    }
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
                GameExitBattleTo(g, g->battle_is_nexus ? SCREEN_NEXUS : SCREEN_MAP);
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
    if (resource_dialog_action == ENERGY_DIALOG_ACTION_QUESTS) {
        EnergyDialogClose(&g->energy_dialog);
        EnergyDialogClose(&g->stamina_dialog);
        if (g->screen == SCREEN_BATTLE)
            GameExitBattleTo(g, SCREEN_QUESTS);
        else
            g->screen = SCREEN_QUESTS;
    }
    GameHandleHudAction(g, popup_hud_action);
    if (screen_at_frame_start == SCREEN_HEROES && g->screen != SCREEN_HEROES)
        HeroesBackgroundClearRowBonus(&g->heroes_scene);
    if (screen_at_frame_start == SCREEN_QUESTS && g->screen != SCREEN_QUESTS) QuestsClose(&g->quests);
    if (screen_at_frame_start == SCREEN_NEXUS && g->screen != SCREEN_NEXUS && g->screen != SCREEN_BATTLE &&
        NexusRunComplete(&g->nexus, &g->player.nexus_run)) {
        NexusAwardEscrow(&g->player);
        ProgressionSave(&g->player);
    }
}

void GameDestroy(Game *g) {
    StoreDestroy(&g->store);
    HeroesDestroy(&g->heroes);
    LocUnload();
    ProgressionSave(&g->player);
    PlayerUnloadRoster(&g->player);
    HomeBackgroundDestroy(&g->scene);
    HeroesBackgroundDestroy(&g->heroes_scene);
    NexusUnload(&g->nexus);
    BattleDestroy(&g->battle);
    PassiveManifestUnload();
    AssetsUnload();
}
