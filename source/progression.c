#include <limits.h>

#define PROGRESSION_SAVE_VERSION 8
#define PROGRESSION_FIRST_PVP_VERSION 8
#define PROGRESSION_FIRST_FULL_ROSTER_VERSION 7
#define PROGRESSION_LOCAL_MAX_LEVEL 100
#define PROGRESSION_START_ENERGY_MAX 12
#define PROGRESSION_START_STAMINA_MAX 8
#define PROGRESSION_JSON_MAX_SAFE_INTEGER PLAYER_INSTANCE_ID_MAX

static int ProgressionCumulativeXpThroughLevel(int level) {
    if (level <= 0) return 0;
    return 5 * level * level + 15 * level;
}

int ProgressionXpForCurrentLevel(int level) {
    level = Clamp(level, 1, PROGRESSION_LOCAL_MAX_LEVEL);
    return ProgressionCumulativeXpThroughLevel(level - 1);
}

int ProgressionXpForNextLevel(int level) {
    level = Clamp(level, 1, PROGRESSION_LOCAL_MAX_LEVEL);
    if (level == PROGRESSION_LOCAL_MAX_LEVEL) return ProgressionXpForCurrentLevel(level);
    return ProgressionCumulativeXpThroughLevel(level);
}

int ProgressionEnergyMaxForLevel(int level) {
    level = Clamp(level, 1, PROGRESSION_LOCAL_MAX_LEVEL);
    return PROGRESSION_START_ENERGY_MAX + level - 1;
}

void ProgressionInitializePlayerDefaults(Player *p) {
    p->level = 1;
    p->experience = 0;
    p->experience_min = ProgressionXpForCurrentLevel(1);
    p->experience_max = ProgressionXpForNextLevel(1);
    p->energy = PROGRESSION_START_ENERGY_MAX;
    p->energy_max = PROGRESSION_START_ENERGY_MAX;
    p->stamina = PROGRESSION_START_STAMINA_MAX;
    p->stamina_max = PROGRESSION_START_STAMINA_MAX;
    p->energy_regen_timer = 0.0f;
    p->stamina_regen_timer = 0.0f;
    p->gold = 1725;
    p->gems = 41;
    p->store_rng_state = 0x9e3779b9u;
    p->store_opened_total = 0;
    memset(p->store_opened_by_pack, 0, sizeof(p->store_opened_by_pack));
    memset(p->pack_vouchers, 0, sizeof(p->pack_vouchers));
    p->champion_since_legendary = 0;
    p->campaign_progress = (PlayerCampaignProgress){0};
    p->potions = 3;
    p->potion_max = 10;
    memset(p->keys, 0, sizeof(p->keys));
    p->nexus_run = (PlayerNexusRun){0};
    p->nexus_events = (PlayerNexusEvents){0};
    p->pvp = (PlayerPvp){0};
    p->name[0] = '\0';
    p->tutorial = (PlayerTutorial){.main_complete = true, .consume_complete = true};
    p->fast_battles = false;
}

typedef struct {
    int level, xp, power, health;
    int band_low, band_high;
    bool maxed;
} BattleHeroStats;

typedef struct {
    int slot;
    int roster_index;
    BattleHeroStats pre, post;
} BattleHeroLeveling;

typedef struct {
    BattleHeroLeveling heroes[SQUAD_SLOT_COUNT];
    int count;
} BattleSquadLeveling;

typedef struct {
    bool resolved;
    bool won;
    bool first_win;
    bool mastered;
    bool grind;
    bool advanced;
    bool unlocked_map;

    int map_index;
    int node_index;
    int next_map_index;
    int next_node_index;
    int completions_pre;
    int completions_post;

    int default_gold_reward;
    int crit_gold_reward;
    int rune_gold_reward;
    int rune_xp_reward;
    int gold_reward;
    int xp_reward;
    int first_win_gold_reward;
    int first_win_xp_reward;
    int mastery_gold_reward;
    int mastery_xp_reward;
    int default_gems_reward;
    int challenge_gold_reward;
    int challenge_xp_reward;
    int challenge_gems_reward;
    int grind_gold_reward;
    int grind_xp_reward;
    int grind_gems_reward;
    int first_win_gems_reward;
    int mastery_gems_reward;
    int gems_reward;
    PvpPayout event_payout;
    int wins_to_master;

    int player_level_pre;
    int player_level_post;
    int player_xp_pre;
    int player_xp_post;
    int player_xp_min_post;
    int player_xp_max_post;
    int levels_gained;

    int energy_post;
    int energy_max_pre;
    int energy_max_post;
    int stamina_post;
    int trophy_delta;
    int banner_delta;
    BattleSquadLeveling squad_leveling;
} BattleRewardResult;

static int ProgressionSaturatingAdd(int value, int amount) {
    if (amount <= 0) return value;
    if (value > INT_MAX - amount) return INT_MAX;
    return value + amount;
}

void CampaignProgressSync(const Campaign *campaign, PlayerCampaignProgress *progress) {
    if (!campaign || !progress || campaign->map_count <= 0) return;
    if (progress->campaign_index < 0 || progress->campaign_index >= campaign->map_count ||
        !CampaignMapUnlocked(campaign, progress, progress->campaign_index)) {
        progress->campaign_index = 0;
        for (int map = 1; map < campaign->map_count; map++) {
            if (!CampaignMapUnlocked(campaign, progress, map)) break;
            progress->campaign_index = map;
        }
    }
    progress->node_index = CampaignFrontierForMap(campaign, progress, progress->campaign_index);
}

static BattleHeroStats BattleHeroStatsOf(const Character *ch) {
    bool maxed = ConsumeHeroAtLevelCap(ch, ch->level);
    int band_low = ConsumeHeroLevelXp(ch, ch->level);
    return (BattleHeroStats){
        .level = ch->level,
        .xp = ch->xp,
        .power = CharacterStatPower(ch),
        .health = CharacterStatHealth(ch),
        .band_low = band_low,
        .band_high = maxed ? band_low : ConsumeHeroLevelXp(ch, ch->level + 1),
        .maxed = maxed,
    };
}

static void BattleSquadLevelingSnapshotPre(BattleSquadLeveling *leveling, const Player *player) {
    *leveling = (BattleSquadLeveling){0};
    for (int giant_pass = 0; giant_pass < 2; giant_pass++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            int roster_index = PlayerSquadUnitAt(player, player->current_squad, slot);
            if (roster_index < 0 || roster_index >= player->roster_count) continue;
            const Character *ch = &player->roster[roster_index];
            if ((int)ch->giant != giant_pass) continue;
            leveling->heroes[leveling->count++] = (BattleHeroLeveling){
                .slot = slot,
                .roster_index = roster_index,
                .pre = BattleHeroStatsOf(ch),
                .post = BattleHeroStatsOf(ch),
            };
        }
    }
}

static void BattleSquadLevelingSnapshotPost(BattleSquadLeveling *leveling, const Player *player) {
    for (int i = 0; i < leveling->count; i++)
        leveling->heroes[i].post = BattleHeroStatsOf(&player->roster[leveling->heroes[i].roster_index]);
}

static void BattleRewardSnapshotPre(BattleRewardResult *result, const Player *player) {
    result->player_level_pre = player->level;
    result->player_xp_pre = player->experience;
    result->energy_max_pre = player->energy_max;
    BattleSquadLevelingSnapshotPre(&result->squad_leveling, player);
}

static void BattleRewardSnapshotPost(BattleRewardResult *result, const Player *player) {
    BattleSquadLevelingSnapshotPost(&result->squad_leveling, player);
    result->player_level_post = player->level;
    result->player_xp_post = player->experience;
    result->player_xp_min_post = player->experience_min;
    result->player_xp_max_post = player->experience_max;
    result->energy_post = player->energy;
    result->energy_max_post = player->energy_max;
    result->stamina_post = player->stamina;
    result->levels_gained = result->player_level_post - result->player_level_pre;
}

static void ProgressionAwardAccountXp(Player *player, int xp) {
    if (xp <= 0) return;
    player->experience_min = ProgressionXpForCurrentLevel(player->level);
    player->experience_max = ProgressionXpForNextLevel(player->level);
    player->experience = ProgressionSaturatingAdd(player->experience, xp);

    while (player->level < PROGRESSION_LOCAL_MAX_LEVEL && player->experience >= player->experience_max) {
        player->level++;
        player->experience_min = ProgressionXpForCurrentLevel(player->level);
        player->experience_max = ProgressionXpForNextLevel(player->level);
    }
    if (player->level == PROGRESSION_LOCAL_MAX_LEVEL && player->experience > player->experience_max)
        player->experience = player->experience_max;
}

static void ProgressionAwardSquadXp(Player *player, int xp) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        int roster_index = PlayerSquadUnitAt(player, player->current_squad, slot);
        if (roster_index >= 0 && roster_index < player->roster_count)
            ConsumeAwardHeroXp(&player->roster[roster_index], xp);
    }
}

static void ProgressionAwardBattleXp(Player *player, int xp) {
    int level_pre = player->level;
    ProgressionAwardSquadXp(player, xp);
    ProgressionAwardAccountXp(player, xp);
    if (player->level == level_pre) return;
    player->energy_max = ProgressionEnergyMaxForLevel(player->level);
    player->energy = player->energy_max;
    player->stamina = player->stamina_max;
    player->energy_regen_timer = 0.0f;
    player->stamina_regen_timer = 0.0f;
}

static void ProgressionApplyRuneRewards(const Player *player, BattleRewardResult *result) {
    RuneBonus bonus = RuneSquadBonus(&player->runes, player->current_squad, RuneNow());
    int gold = RuneApplyPct(result->gold_reward, bonus.gold_pct);
    int xp = RuneApplyPct(result->xp_reward, bonus.xp_pct);
    result->rune_gold_reward = gold - result->gold_reward;
    result->rune_xp_reward = xp - result->xp_reward;
    result->gold_reward = gold;
    result->xp_reward = xp;
}

BattleRewardResult CampaignResolveBattle(
    Player *player, const Campaign *campaign, int map_index, int node_index, bool won, int crit_gold
) {
    BattleRewardResult result = {
        .won = won,
        .map_index = map_index,
        .node_index = node_index,
        .next_map_index = player ? player->campaign_progress.campaign_index : 0,
        .next_node_index = player ? player->campaign_progress.node_index : 0,
    };
    if (!player || !campaign || map_index < 0 || map_index >= campaign->map_count) return result;
    if (node_index < 0 || node_index >= campaign->maps[map_index].node_count) return result;

    PlayerCampaignProgress *progress = &player->campaign_progress;
    if (!CampaignMapUnlocked(campaign, progress, map_index)) return result;
    int frontier_pre = CampaignFrontierForMap(campaign, progress, map_index);
    if (node_index > frontier_pre) return result;

    const CampaignNode *node = &campaign->maps[map_index].nodes[node_index];
    BattleRewardSnapshotPre(&result, player);
    result.resolved = true;
    result.grind = node->grind;
    result.wins_to_master = node->wins_to_master;
    result.completions_pre = progress->node_completions[map_index][node_index];
    result.completions_post = result.completions_pre;
    result.next_map_index = map_index;
    result.next_node_index = frontier_pre;

    if (!won) {
        if (progress->node_losses[map_index][node_index] < INT_MAX) progress->node_losses[map_index][node_index]++;
        BattleRewardSnapshotPost(&result, player);
        return result;
    }

    progress->node_losses[map_index][node_index] = 0;
    result.first_win = result.completions_pre == 0;
    result.mastered = CampaignNodeRewardBucket(node, result.completions_pre) == CAMPAIGN_REWARD_MASTERY;
    if (progress->node_completions[map_index][node_index] < INT_MAX)
        progress->node_completions[map_index][node_index]++;
    result.completions_post = progress->node_completions[map_index][node_index];

    result.first_win_gold_reward =
        result.first_win && !node->grind && node->first_win_gold > 0 ? node->first_win_gold : 0;
    result.first_win_xp_reward = result.first_win && !node->grind && node->first_win_xp > 0 ? node->first_win_xp : 0;
    result.mastery_gold_reward = result.mastered && node->mastery_gold > 0 ? node->mastery_gold : 0;
    result.mastery_xp_reward = result.mastered && node->mastery_xp > 0 ? node->mastery_xp : 0;
    result.challenge_gold_reward =
        result.first_win && node->grind && node->challenge_gold > 0 ? node->challenge_gold : 0;
    result.challenge_xp_reward = result.first_win && node->grind && node->challenge_xp > 0 ? node->challenge_xp : 0;
    result.challenge_gems_reward =
        result.first_win && node->grind && node->challenge_gems > 0 ? node->challenge_gems : 0;
    result.grind_gold_reward = !result.first_win && node->grind && node->grind_gold > 0 ? node->grind_gold : 0;
    result.grind_xp_reward = !result.first_win && node->grind && node->grind_xp > 0 ? node->grind_xp : 0;
    result.grind_gems_reward = !result.first_win && node->grind && node->grind_gems > 0 ? node->grind_gems : 0;
    result.default_gold_reward = node->gold > 0 ? node->gold : 0;
    result.crit_gold_reward = crit_gold > 0 ? crit_gold : 0;
    result.gold_reward = ProgressionSaturatingAdd(result.default_gold_reward, result.first_win_gold_reward);
    result.gold_reward = ProgressionSaturatingAdd(result.gold_reward, result.mastery_gold_reward);
    result.gold_reward = ProgressionSaturatingAdd(result.gold_reward, result.challenge_gold_reward);
    result.gold_reward = ProgressionSaturatingAdd(result.gold_reward, result.grind_gold_reward);
    result.gold_reward = ProgressionSaturatingAdd(result.gold_reward, result.crit_gold_reward);
    result.xp_reward = ProgressionSaturatingAdd(node->xp > 0 ? node->xp : 0, result.first_win_xp_reward);
    result.xp_reward = ProgressionSaturatingAdd(result.xp_reward, result.mastery_xp_reward);
    result.xp_reward = ProgressionSaturatingAdd(result.xp_reward, result.challenge_xp_reward);
    result.xp_reward = ProgressionSaturatingAdd(result.xp_reward, result.grind_xp_reward);
    ProgressionApplyRuneRewards(player, &result);
    StoreAwardVoucherForWin(player);
    result.default_gems_reward = node->default_gems > 0 ? node->default_gems : 0;
    result.first_win_gems_reward =
        result.first_win && !node->grind && node->first_win_gems > 0 ? node->first_win_gems : 0;
    result.mastery_gems_reward = result.mastered && node->mastery_gems > 0 ? node->mastery_gems : 0;
    result.gems_reward = ProgressionSaturatingAdd(result.default_gems_reward, result.first_win_gems_reward);
    result.gems_reward = ProgressionSaturatingAdd(result.gems_reward, result.mastery_gems_reward);
    result.gems_reward = ProgressionSaturatingAdd(result.gems_reward, result.challenge_gems_reward);
    result.gems_reward = ProgressionSaturatingAdd(result.gems_reward, result.grind_gems_reward);
    player->gold = ProgressionSaturatingAdd(player->gold, result.gold_reward);
    player->gems = ProgressionSaturatingAdd(player->gems, result.gems_reward);

    ProgressionAwardBattleXp(player, result.xp_reward);

    if (result.first_win && node_index == frontier_pre) {
        int boss = CampaignBossNodeIndex(campaign, map_index);
        int last = campaign->maps[map_index].node_count - 1;
        if (node_index == boss && last > boss && campaign->maps[map_index].nodes[last].grind) {
            result.next_node_index = last;
            result.advanced = true;
            result.unlocked_map = map_index + 1 < campaign->map_count;
        } else if (node_index == boss && map_index + 1 < campaign->map_count) {
            result.next_map_index = map_index + 1;
            result.next_node_index = 0;
            result.advanced = true;
            result.unlocked_map = true;
        } else if (node->grind && map_index + 1 < campaign->map_count) {
            result.next_map_index = map_index + 1;
            result.next_node_index = 0;
            result.advanced = true;
        } else if (!node->grind && node_index < boss) {
            result.next_node_index = node_index + 1;
            result.advanced = true;
        }
    } else if (result.mastered) {
        if (node_index + 1 < campaign->maps[map_index].node_count) {
            result.next_node_index = node_index + 1;
        } else if (map_index + 1 < campaign->map_count) {
            result.next_map_index = map_index + 1;
            result.next_node_index = 0;
        }
    }
    progress->campaign_index = result.next_map_index;
    progress->node_index = result.next_node_index;

    BattleRewardSnapshotPost(&result, player);
    return result;
}

static cJSON *ProgressionLoadCharacterCatalog(void) {
    char *text = LoadFileText(HOD_CACHE "characters.json");
    if (!text) return NULL;
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    if (!cJSON_IsObject(root) || !cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(root, "characters"))) {
        cJSON_Delete(root);
        return NULL;
    }
    return CharacterCatalogAddCustomHeroes(root);
}

void NexusResetRun(Player *player) {
    if (!player) return;
    PlayerNexusRun *run = &player->nexus_run;
    run->current_node_step = 1;
    run->selected_node_step = 1;
    run->committed = false;
    run->reset = false;
    run->escrow_gold = 0;
    run->escrow_xp = 0;
    run->escrow_potions = 0;
    memset(run->escrow_units, 0, sizeof(run->escrow_units));
    for (int index = 0; index < NEXUS_MAX_NODES; index++) {
        PlayerNexusNode *node = &run->nodes[index];
        if (node->wins > node->previous_wins) node->previous_wins = node->wins;
        node->wins = 0;
        node->losses = 0;
    }
}

static bool PvpGrantUnits(Player *player, const PvpPayout *payout) {
    int wanted = PvpPayoutUnitCount(payout);
    if (wanted == 0) return true;
    cJSON *root = ProgressionLoadCharacterCatalog();
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    const cJSON **definitions = (const cJSON **)MemAlloc((unsigned int)wanted * sizeof(*definitions));
    int count = 0;
    for (int i = 0; i < payout->prize_count; i++) {
        const PvpPrize *prize = payout->prizes[i];
        if (prize->kind != PVP_PRIZE_UNIT) continue;
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, (cJSON *)catalog) {
            if (!strcmp(JsonStr(entry, "id"), prize->id)) break;
        }
        for (int n = 0; entry && n < prize->count; n++) definitions[count++] = entry;
    }
    bool granted = count == wanted && PlayerRosterAppendDefinitions(player, definitions, count, NULL);
    MemFree((void *)definitions);
    cJSON_Delete(root);
    return granted;
}

static void PvpGrantItems(Player *player, const PvpPayout *payout) {
    for (int i = 0; i < payout->prize_count; i++) {
        const PvpPrize *prize = payout->prizes[i];
        if (prize->kind == PVP_PRIZE_GEAR)
            for (int n = 0; n < prize->count; n++) GearGrant(&player->gear, prize->index);
        if (prize->kind == PVP_PRIZE_TOME) player->gear.tomes += prize->count;
        if (prize->kind == PVP_PRIZE_VOUCHER)
            player->pack_vouchers[prize->index] =
                ProgressionSaturatingAdd(player->pack_vouchers[prize->index], prize->count);
    }
}

static int PvpRosterRoom(const Player *player) {
    return PlayerRosterHasCapacity(player, 0) ? player->roster_capacity - player->roster_count : 0;
}

bool PvpEventClaim(Player *player, int index) {
    PlayerPvp *pvp = &player->pvp;
    if (index < 0 || index >= pvp->event_result_count || pvp->event_results[index].claimed) return false;
    PlayerPvpEventResult *result = &pvp->event_results[index];
    const PvpEventRankReward *reward = PvpEventRewardForRank(PvpEventForCycle(result->cycle), result->rank);
    PvpPayout payout = {0};
    if (reward) PvpPayoutAdd(&payout, &reward->reward);
    if (!PvpGrantUnits(player, &payout)) return false;
    PvpGrantItems(player, &payout);
    result->claimed = true;
    player->gold = ProgressionSaturatingAdd(player->gold, result->gold);
    player->gems = ProgressionSaturatingAdd(player->gems, result->gems);
    PvpEventSortResults(pvp);
    return true;
}

BattleRewardResult PvpResolveBattle(
    Player *player, const PvpOpponent *opponent, bool won, int revenge_index, int crit_gold, long long now, bool event
) {
    BattleRewardResult result = {.won = won, .resolved = true};
    BattleRewardSnapshotPre(&result, player);
    PlayerPvp *pvp = &player->pvp;
    if (revenge_index >= 0 && revenge_index < pvp->history_count) pvp->history[revenge_index].revenged = true;
    PlayerPvpBattle battle =
        event ? PvpRecordEventAttack(pvp, opponent, won, now) : PvpRecordAttack(pvp, opponent, won, now);
    result.trophy_delta = battle.trophy_delta;
    result.banner_delta = battle.banners;
    PvpPayout milestones = event ? PvpEventTakeMilestones(pvp, PvpRosterRoom(player)) : (PvpPayout){0};
    if (!PvpGrantUnits(player, &milestones)) TraceLog(LOG_WARNING, "Pvp: could not grant milestone units");
    PvpGrantItems(player, &milestones);
    result.event_payout = milestones;
    result.gems_reward = milestones.gems;
    player->gems = ProgressionSaturatingAdd(player->gems, milestones.gems);
    result.crit_gold_reward = crit_gold > 0 ? crit_gold : 0;
    result.gold_reward = ProgressionSaturatingAdd(battle.gold, result.crit_gold_reward);
    result.gold_reward = ProgressionSaturatingAdd(result.gold_reward, milestones.gold);
    result.xp_reward = battle.xp;
    if (won) StoreAwardVoucherForWin(player);
    ProgressionApplyRuneRewards(player, &result);
    player->gold = ProgressionSaturatingAdd(player->gold, result.gold_reward);
    ProgressionAwardBattleXp(player, result.xp_reward);
    BattleRewardSnapshotPost(&result, player);
    return result;
}

BattleRewardResult NexusResolveBattle(Player *player, const Nexus *nexus, int step, bool won, int crit_gold) {
    BattleRewardResult result = {.won = won, .node_index = step - 1};
    if (!player || !nexus) return result;

    PlayerNexusRun *run = &player->nexus_run;
    const NexusEvent *event = NexusEventAt(nexus, run->event_index);
    const NexusDifficulty *difficulty = NexusDifficultyAt(nexus, run->event_index, run->difficulty_index);
    if (!event || !difficulty) return result;
    if (step < 1 || step > difficulty->node_count || step != run->current_node_step) return result;
    if (step == difficulty->node_count && run->nodes[step - 1].wins > 0) return result;

    const NexusNode *node = &difficulty->nodes[step - 1];
    PlayerNexusNode *progress = &run->nodes[step - 1];
    BattleRewardSnapshotPre(&result, player);
    result.resolved = true;
    result.wins_to_master = 1;
    result.map_index = run->event_index;
    result.next_map_index = run->event_index;
    result.next_node_index = run->current_node_step - 1;
    result.completions_pre = progress->wins;
    result.completions_post = progress->wins;

    if (!run->committed) {
        run->committed = true;
        int index = run->event_index;
        if (event->max_runs > 0 && index >= 0 && index < NEXUS_MAX_EVENTS &&
            player->nexus_events.remaining_runs[index] > 0)
            player->nexus_events.remaining_runs[index]--;
    }

    if (!won) {
        if (progress->losses < INT_MAX) progress->losses++;
        run->reset = difficulty->potion_cost > 0;
        BattleRewardSnapshotPost(&result, player);
        return result;
    }

    if (progress->wins < INT_MAX) progress->wins++;
    result.completions_post = progress->wins;
    result.default_gold_reward = node->gold > 0 ? node->gold : 0;
    result.crit_gold_reward = crit_gold > 0 ? crit_gold : 0;
    result.gold_reward = ProgressionSaturatingAdd(result.default_gold_reward, result.crit_gold_reward);
    result.xp_reward = node->xp > 0 ? node->xp : 0;
    ProgressionApplyRuneRewards(player, &result);
    StoreAwardVoucherForWin(player);
    run->escrow_gold = ProgressionSaturatingAdd(run->escrow_gold, result.gold_reward);
    run->escrow_xp = ProgressionSaturatingAdd(run->escrow_xp, result.xp_reward);
    run->escrow_potions = ProgressionSaturatingAdd(run->escrow_potions, node->potion_reward);
    if (node->reward_rarity_count > 0) {
        int rarity = node->reward_rarity[StoreRngRange(&player->store_rng_state, node->reward_rarity_count)];
        if (rarity >= RARITY_COMMON && rarity <= RARITY_MYTHIC)
            run->escrow_units[rarity] = ProgressionSaturatingAdd(run->escrow_units[rarity], 1);
    }

    ProgressionAwardBattleXp(player, result.xp_reward);

    if (step == run->current_node_step && run->current_node_step < difficulty->node_count) {
        run->current_node_step++;
        result.advanced = true;
    }
    result.next_node_index = run->current_node_step - 1;

    BattleRewardSnapshotPost(&result, player);
    return result;
}

static void NexusGrantEscrowUnits(Player *player, const int *escrow_units, NexusAward *award) {
    int wanted = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC; rarity++) wanted += escrow_units[rarity];
    if (wanted <= 0) return;

    cJSON *catalog_root = ProgressionLoadCharacterCatalog();
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    const cJSON *definitions[NEXUS_MAX_NODES];
    int count = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC && count < NEXUS_MAX_NODES; rarity++) {
        int pool = 0;
        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, catalog) pool +=
            StoreHeroDefinitionEligible(entry) && RarityFromString(JsonStr(entry, "rarity")) == rarity;
        for (int n = 0; n < escrow_units[rarity] && pool > 0 && count < NEXUS_MAX_NODES; n++) {
            int pick = StoreRngRange(&player->store_rng_state, pool);
            cJSON_ArrayForEach(entry, catalog) {
                if (!StoreHeroDefinitionEligible(entry) || RarityFromString(JsonStr(entry, "rarity")) != rarity)
                    continue;
                if (pick-- == 0) break;
            }
            if (entry) definitions[count++] = entry;
        }
    }
    int first = -1;
    if (count > 0 && !PlayerRosterAppendDefinitions(player, definitions, count, &first))
        TraceLog(LOG_WARNING, "Nexus: could not grant %d escrowed unit(s)", count);
    for (int n = 0; first >= 0 && n < count; n++) {
        int index = first + n;
        bool owned = false;
        for (int other = 0; other < index && !owned; other++)
            owned = strcmp(player->roster[other].id, player->roster[index].id) == 0;
        award->unit_roster[n] = index;
        award->unit_newly_owned[n] = !owned;
        award->unit_count = n + 1;
    }
    cJSON_Delete(catalog_root);
}

NexusAward NexusAwardEscrow(Player *player) {
    NexusAward award = {0};
    if (!player || !player->nexus_run.committed) return award;
    PlayerNexusRun *run = &player->nexus_run;
    award.gold = run->escrow_gold;
    award.potions = run->escrow_potions;
    player->gold = ProgressionSaturatingAdd(player->gold, run->escrow_gold);
    PlayerAddPotions(player, run->escrow_potions);
    NexusGrantEscrowUnits(player, run->escrow_units, &award);

    int event = run->event_index, difficulty = run->difficulty_index;
    if (event >= 0 && event < NEXUS_MAX_EVENTS && difficulty >= 0 && difficulty < NEXUS_MAX_DIFFICULTIES &&
        player->nexus_events.completions[event][difficulty] < INT_MAX)
        player->nexus_events.completions[event][difficulty]++;
    NexusResetRun(player);
    return award;
}

bool NexusReviveRun(Player *player) {
    if (!player || !PlayerSpendPotion(player, 1)) return false;
    player->nexus_run.reset = false;
    return true;
}

bool NexusUnlockEvent(Player *player, const Nexus *nexus, int event) {
    const NexusEvent *definition = NexusEventAt(nexus, event);
    if (!player || !definition || event < 0 || event >= NEXUS_MAX_EVENTS) return false;
    if (definition->unlock_key != 0 && !PlayerSpendKeys(player, definition->unlock_key, definition->unlock_key_qty))
        return false;
    player->nexus_events.unlocked_at[event] = ServerNow();
    player->nexus_events.remaining_runs[event] = definition->max_runs;
    return true;
}

static bool ProgressionPersistenceEnabled(void) {
    return getenv("HOD_SHOT") == NULL && getenv("HOD_NO_SAVE") == NULL;
}

const char *ProgressionSavePath(void) {
    const char *path = getenv("HOD_SAVE_PATH");
    return path && path[0] ? path : "player_save.dat";
}

static bool ProgressionJsonInt(const cJSON *obj, const char *key, int min, int max, int *out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < min || item->valuedouble > max ||
        floor(item->valuedouble) != item->valuedouble)
        return false;
    *out = (int)item->valuedouble;
    return true;
}

static bool ProgressionJsonTimer(const cJSON *obj, const char *key, float *out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) || item->valuedouble < 0.0 ||
        item->valuedouble >= PLAYER_REGEN_PERIOD)
        return false;
    *out = (float)item->valuedouble;
    return true;
}

static bool ProgressionJsonU64(const cJSON *obj, const char *key, uint64_t min, uint64_t max, uint64_t *out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) || item->valuedouble < (double)min ||
        item->valuedouble > (double)max || floor(item->valuedouble) != item->valuedouble)
        return false;
    *out = (uint64_t)item->valuedouble;
    return true;
}

static bool ProgressionRosterStateValid(const Player *player) {
    if (!player) return false;
    if (!player->roster && !player->roster_abilities && player->roster_count == 0) return true;
    if (!player->roster || !player->roster_abilities || player->roster_capacity <= 0) return false;
    if (player->starter_roster_count < 0 || player->starter_roster_count > player->roster_count ||
        player->roster_count > player->roster_capacity)
        return false;

    uint64_t max_id = 0;
    for (int i = 0; i < player->roster_count; i++) {
        const Character *ch = &player->roster[i];
        if (!ch->instance_id || !ch->id[0] || ch->tier < TIER_I || ch->tier > TIER_IV || !ch->has_tier[ch->tier] ||
            ch->level < 1 || ch->level > ch->stats[ch->tier].max_level)
            return false;
        if (ch->instance_id > PROGRESSION_JSON_MAX_SAFE_INTEGER) return false;
        for (int previous = 0; previous < i; previous++)
            if (player->roster[previous].instance_id == ch->instance_id) return false;
        if (ch->instance_id > max_id) max_id = ch->instance_id;
    }
    return player->next_instance_id > max_id && player->next_instance_id <= PROGRESSION_JSON_MAX_SAFE_INTEGER;
}

static bool ProgressionJsonTakeObject(cJSON *parent, const char *key, cJSON **item) {
    if (!parent || !item || !*item || !cJSON_AddItemToObject(parent, key, *item)) return false;
    *item = NULL;
    return true;
}

static bool ProgressionJsonAddArrayItem(cJSON *array, cJSON *item) {
    if (!item) return false;
    if (cJSON_AddItemToArray(array, item)) return true;
    cJSON_Delete(item);
    return false;
}

static bool ProgressionJsonAddArrayNumber(cJSON *array, double value) {
    return ProgressionJsonAddArrayItem(array, cJSON_CreateNumber(value));
}

static bool ProgressionJsonAddArrayNull(cJSON *array) {
    return ProgressionJsonAddArrayItem(array, cJSON_CreateNull());
}

static cJSON *ProgressionSaveGear(const Player *player) {
    cJSON *root = cJSON_CreateObject();
    cJSON *inventory = cJSON_AddObjectToObject(root, "inventory");
    cJSON *heroes = cJSON_AddArrayToObject(root, "heroes");
    bool ok = root && inventory && heroes && cJSON_AddNumberToObject(root, "tomes", player->gear.tomes);
    for (int i = 0; ok && i < GearCatalogCount(); i++)
        if (player->gear.counts[i] > 0)
            ok = cJSON_AddNumberToObject(inventory, GearDefAt(i)->id, player->gear.counts[i]) != NULL;
    for (int r = 0; ok && r < player->roster_count; r++) {
        const Character *ch = &player->roster[r];
        if (!GearHasEquipped(ch) && !ch->gear.prestige_unlocked && !ch->ascended) continue;
        cJSON *entry = cJSON_CreateObject();
        cJSON *slots = cJSON_AddArrayToObject(entry, "slots");
        ok = entry && slots && cJSON_AddNumberToObject(entry, "instance_id", (double)ch->instance_id) &&
             cJSON_AddBoolToObject(entry, "ascended", ch->ascended) &&
             cJSON_AddBoolToObject(entry, "prestige_unlocked", ch->gear.prestige_unlocked);
        for (int slot = 0; ok && slot < GEAR_SLOT_CAPACITY; slot++) {
            const GearDef *def = GearInSlot(ch, slot);
            ok = cJSON_AddItemToArray(slots, cJSON_CreateString(def ? def->id : ""));
        }
        ok = cJSON_AddItemToArray(heroes, entry) && ok;
    }
    if (!ok) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

static void ProgressionLoadGear(const cJSON *json, Player *player) {
    memset(&player->gear, 0, sizeof(player->gear));
    for (int r = 0; r < player->roster_count; r++) memset(&player->roster[r].gear, 0, sizeof(GearLoadout));
    if (!cJSON_IsObject(json)) return;
    player->gear.tomes = JsonInt(json, "tomes");
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "inventory")) {
        int def = GearDefIndex(item->string);
        if (def < 0 || !cJSON_IsNumber(item) || item->valueint <= 0) {
            TraceLog(LOG_WARNING, "Progression: dropped unknown gear %s", item->string);
            continue;
        }
        player->gear.counts[def] = item->valueint;
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "heroes")) {
        uint64_t instance_id = (uint64_t)JsonFloat(item, "instance_id");
        Character *ch = NULL;
        for (int r = 0; r < player->roster_count && !ch; r++)
            if (player->roster[r].instance_id == instance_id) ch = &player->roster[r];
        if (!ch) continue;
        if (cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive((cJSON *)item, "ascended"))) GearApplyAscension(ch);
        ch->gear.prestige_unlocked = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive((cJSON *)item, "prestige_unlocked"));
        int slot = 0;
        const cJSON *id = NULL;
        cJSON_ArrayForEach(id, cJSON_GetObjectItemCaseSensitive((cJSON *)item, "slots")) {
            if (slot >= GEAR_SLOT_CAPACITY) break;
            int def = cJSON_IsString(id) && id->valuestring[0] ? GearDefIndex(id->valuestring) : -1;
            ch->gear.defs[slot++] = (short)(def + 1);
        }
    }
}

static cJSON *ProgressionSaveRunes(const PlayerRunes *runes) {
    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_AddArrayToObject(root, "items");
    cJSON *squads = cJSON_AddArrayToObject(root, "squads");
    bool ok = root && items && squads && cJSON_AddNumberToObject(root, "next_uuid", (double)runes->next_uuid) &&
              cJSON_AddNumberToObject(root, "seen", runes->seen);
    for (int i = 0; ok && i < runes->count; i++) {
        const PlayerRune *rune = &runes->items[i];
        cJSON *entry = cJSON_CreateObject();
        ok = entry && cJSON_AddItemToArray(items, entry) &&
             cJSON_AddNumberToObject(entry, "uuid", (double)rune->uuid) &&
             cJSON_AddStringToObject(entry, "def", RuneDefFor(rune)->id) &&
             cJSON_AddNumberToObject(entry, "expire_at", (double)rune->expire_at) &&
             cJSON_AddNumberToObject(entry, "inventory_expire_at", (double)rune->inventory_expire_at);
    }
    for (int squad = 0; ok && squad < PLAYER_MAX_SQUADS; squad++) {
        cJSON *entry = cJSON_CreateObject();
        cJSON *slots = cJSON_AddArrayToObject(entry, "slots");
        ok = entry && slots && cJSON_AddItemToArray(squads, entry) &&
             cJSON_AddBoolToObject(entry, "prestige_unlocked", runes->prestige_unlocked[squad]);
        for (int slot = 0; ok && slot < RUNE_SLOT_COUNT; slot++)
            ok = ProgressionJsonAddArrayNumber(slots, (double)runes->squad_slots[squad][slot]);
    }
    if (ok) return root;
    cJSON_Delete(root);
    return NULL;
}

static bool ProgressionPvpAddBattle(cJSON *history, const PlayerPvpBattle *battle) {
    cJSON *entry = cJSON_CreateObject();
    if (!entry || !cJSON_AddNumberToObject(entry, "at", (double)battle->at) ||
        !cJSON_AddNumberToObject(entry, "bot", battle->bot) ||
        !cJSON_AddNumberToObject(entry, "squad", battle->squad) ||
        !cJSON_AddNumberToObject(entry, "level", battle->level) ||
        !cJSON_AddNumberToObject(entry, "opponent_trophies", battle->opponent_trophies) ||
        !cJSON_AddNumberToObject(entry, "trophy_delta", battle->trophy_delta) ||
        !cJSON_AddNumberToObject(entry, "gold", battle->gold) || !cJSON_AddNumberToObject(entry, "xp", battle->xp) ||
        !cJSON_AddNumberToObject(entry, "banners", battle->banners) ||
        !cJSON_AddNumberToObject(entry, "seed", battle->seed) ||
        !cJSON_AddBoolToObject(entry, "defense", battle->defense) ||
        !cJSON_AddBoolToObject(entry, "won", battle->won) ||
        !cJSON_AddBoolToObject(entry, "revenged", battle->revenged) ||
        (battle->opponent[0] && !cJSON_AddStringToObject(entry, "opponent", battle->opponent))) {
        cJSON_Delete(entry);
        return false;
    }
    return ProgressionJsonAddArrayItem(history, entry);
}

static bool ProgressionPvpAddEvents(cJSON *json, const PlayerPvp *pvp) {
    cJSON *event = cJSON_AddObjectToObject(json, "event");
    cJSON *results = cJSON_AddArrayToObject(json, "event_results");
    if (!event || !results || !cJSON_AddNumberToObject(event, "cycle", (double)pvp->event.cycle) ||
        !cJSON_AddNumberToObject(event, "banners", pvp->event.banners) ||
        !cJSON_AddNumberToObject(event, "milestones_paid", pvp->event.milestones_paid) ||
        !cJSON_AddBoolToObject(event, "buffed", pvp->event.buffed))
        return false;
    for (int i = 0; i < pvp->event_result_count; i++) {
        const PlayerPvpEventResult *result = &pvp->event_results[i];
        cJSON *entry = cJSON_CreateObject();
        if (!entry || !cJSON_AddNumberToObject(entry, "cycle", (double)result->cycle) ||
            !cJSON_AddNumberToObject(entry, "banners", result->banners) ||
            !cJSON_AddNumberToObject(entry, "rank", result->rank) ||
            !cJSON_AddNumberToObject(entry, "gold", result->gold) ||
            !cJSON_AddNumberToObject(entry, "gems", result->gems) ||
            !cJSON_AddBoolToObject(entry, "claimed", result->claimed)) {
            cJSON_Delete(entry);
            return false;
        }
        if (!ProgressionJsonAddArrayItem(results, entry)) return false;
    }
    return true;
}

static cJSON *ProgressionSavePvp(const PlayerPvp *pvp) {
    cJSON *json = cJSON_CreateObject();
    cJSON *history = json ? cJSON_AddArrayToObject(json, "history") : NULL;
    bool ok = history && cJSON_AddNumberToObject(json, "trophies", pvp->trophies) &&
              cJSON_AddNumberToObject(json, "best_trophies", pvp->best_trophies) &&
              cJSON_AddNumberToObject(json, "trophies_seen", pvp->trophies_seen) &&
              cJSON_AddNumberToObject(json, "wins", pvp->wins) &&
              cJSON_AddNumberToObject(json, "losses", pvp->losses) &&
              cJSON_AddNumberToObject(json, "win_streak", pvp->win_streak) &&
              cJSON_AddNumberToObject(json, "best_win_streak", pvp->best_win_streak) &&
              cJSON_AddNumberToObject(json, "trophies_won", pvp->trophies_won) &&
              cJSON_AddNumberToObject(json, "defense_wins", pvp->defense_wins) &&
              cJSON_AddNumberToObject(json, "defense_losses", pvp->defense_losses) &&
              cJSON_AddNumberToObject(json, "nexts_used", pvp->nexts_used) &&
              cJSON_AddNumberToObject(json, "nexts_day", (double)pvp->nexts_day) &&
              cJSON_AddNumberToObject(json, "defense_checked_at", (double)pvp->defense_checked_at);
    cJSON *held_since = ok ? cJSON_AddArrayToObject(json, "held_since") : NULL;
    ok = ok && held_since;
    for (int i = 0; ok && i < PVP_HOLD_THRESHOLD_COUNT; i++)
        ok = ProgressionJsonAddArrayNumber(held_since, (double)pvp->held_since[i]);
    for (int i = 0; ok && i < pvp->history_count; i++) ok = ProgressionPvpAddBattle(history, &pvp->history[i]);
    ok = ok && ProgressionPvpAddEvents(json, pvp);
    if (!ok) {
        cJSON_Delete(json);
        return NULL;
    }
    return json;
}

static cJSON *ProgressionSaveMissions(const PlayerMissions *missions) {
    cJSON *root = cJSON_CreateObject();
    cJSON *counters = cJSON_AddObjectToObject(root, "counters");
    cJSON *progress = cJSON_AddObjectToObject(root, "progress");
    cJSON *claimed = cJSON_AddArrayToObject(root, "claimed");
    bool ok =
        counters && progress && claimed && cJSON_AddNumberToObject(root, "last_battle_day", missions->last_battle_day);
    for (int stat = 0; ok && stat < MISSION_STAT_COUNT; stat++) {
        ok = cJSON_AddNumberToObject(counters, MISSION_STAT_NAMES[stat], (double)missions->counters[stat]);
    }
    for (int i = 0; ok && i < MissionCount(); i++) {
        const char *id = MissionAt(i)->id;
        ok = cJSON_AddNumberToObject(progress, id, (double)missions->progress[i]);
        if (ok && missions->claimed[i]) ok = cJSON_AddItemToArray(claimed, cJSON_CreateString(id));
    }
    if (!ok) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

static cJSON *ProgressionSaveGallery(const PlayerGallery *gallery) {
    cJSON *root = cJSON_CreateObject();
    cJSON *units = cJSON_AddArrayToObject(root, "units");
    bool ok = root && units && cJSON_AddNumberToObject(root, "badge", gallery->badge);
    for (int i = 0; ok && i < gallery->count; i++) {
        const GalleryRecord *record = &gallery->records[i];
        cJSON *entry = cJSON_CreateObject();
        ok = entry && cJSON_AddItemToArray(units, entry) && cJSON_AddStringToObject(entry, "id", record->id) &&
             cJSON_AddNumberToObject(entry, "owned_tiers", record->owned_tiers) &&
             cJSON_AddBoolToObject(entry, "fought", record->fought) &&
             cJSON_AddBoolToObject(entry, "defeated", record->defeated) &&
             cJSON_AddBoolToObject(entry, "new_owned", record->new_owned) &&
             cJSON_AddBoolToObject(entry, "new_fought", record->new_fought) &&
             cJSON_AddBoolToObject(entry, "new_defeated", record->new_defeated);
    }
    if (ok) return root;
    cJSON_Delete(root);
    return NULL;
}

char *ProgressionSerialize(const Player *player) {
    if (!player) return NULL;
    bool managed_roster = player->roster && player->roster_abilities && player->roster_capacity > 0;
    if (!ProgressionRosterStateValid(player)) return NULL;
    if (player->store_opened_total < 0 || player->champion_since_legendary < 0) return NULL;
    int opened_sum = 0;
    for (int pack = 0; pack < PLAYER_STORE_PACK_COUNT; pack++) {
        if (player->store_opened_by_pack[pack] < 0 || opened_sum > INT_MAX - player->store_opened_by_pack[pack])
            return NULL;
        opened_sum += player->store_opened_by_pack[pack];
    }
    if (opened_sum != player->store_opened_total || player->champion_since_legendary > opened_sum) return NULL;

    cJSON *root = cJSON_CreateObject();
    cJSON *account_owner = cJSON_CreateObject();
    cJSON *campaign_owner = cJSON_CreateObject();
    cJSON *completions_owner = cJSON_CreateArray();
    cJSON *pending_owner = cJSON_CreateObject();
    cJSON *store_owner = cJSON_CreateObject();
    cJSON *opened_owner = cJSON_CreateArray();
    cJSON *roster_owner = cJSON_CreateObject();
    cJSON *starters_owner = cJSON_CreateArray();
    cJSON *additions_owner = cJSON_CreateArray();
    cJSON *squads_owner = cJSON_CreateArray();
    cJSON *nexus_owner = cJSON_CreateObject();
    cJSON *nexus_keys_owner = cJSON_CreateArray();
    cJSON *run_owner = cJSON_CreateObject();
    cJSON *run_nodes_owner = cJSON_CreateArray();
    cJSON *escrow_units_owner = cJSON_CreateArray();
    cJSON *events_owner = cJSON_CreateObject();
    cJSON *event_completions_owner = cJSON_CreateArray();
    cJSON *unlocked_at_owner = cJSON_CreateArray();
    cJSON *remaining_runs_owner = cJSON_CreateArray();
    cJSON *account = account_owner;
    cJSON *campaign = campaign_owner;
    cJSON *completions = completions_owner;
    cJSON *pending = pending_owner;
    cJSON *store = store_owner;
    cJSON *opened = opened_owner;
    cJSON *roster = roster_owner;
    cJSON *starters = starters_owner;
    cJSON *additions = additions_owner;
    cJSON *squads = squads_owner;
    cJSON *nexus = nexus_owner;
    cJSON *nexus_keys = nexus_keys_owner;
    cJSON *run = run_owner;
    cJSON *run_nodes = run_nodes_owner;
    cJSON *escrow_units = escrow_units_owner;
    cJSON *events = events_owner;
    cJSON *event_completions = event_completions_owner;
    cJSON *unlocked_at = unlocked_at_owner;
    cJSON *remaining_runs = remaining_runs_owner;
    char *serialized = NULL;
    if (!root || !account || !campaign || !completions || !pending || !store || !opened || !roster || !starters ||
        !additions || !squads || !nexus || !nexus_keys || !run || !run_nodes || !escrow_units || !events ||
        !event_completions || !unlocked_at || !remaining_runs)
        goto save_json_failed;

    if (!ProgressionJsonTakeObject(root, "account", &account_owner) ||
        !ProgressionJsonTakeObject(root, "campaign", &campaign_owner) ||
        !ProgressionJsonTakeObject(campaign, "completions", &completions_owner) ||
        !ProgressionJsonTakeObject(campaign, "pending_animation", &pending_owner) ||
        !ProgressionJsonTakeObject(root, "store", &store_owner) ||
        !ProgressionJsonTakeObject(store, "opened_by_pack", &opened_owner) ||
        !ProgressionJsonTakeObject(root, "roster", &roster_owner) ||
        !ProgressionJsonTakeObject(roster, "starters", &starters_owner) ||
        !ProgressionJsonTakeObject(roster, "additions", &additions_owner) ||
        !ProgressionJsonTakeObject(root, "squads", &squads_owner) ||
        !ProgressionJsonTakeObject(root, "nexus", &nexus_owner) ||
        !ProgressionJsonTakeObject(nexus, "keys", &nexus_keys_owner) ||
        !ProgressionJsonTakeObject(nexus, "run", &run_owner) ||
        !ProgressionJsonTakeObject(run, "nodes", &run_nodes_owner) ||
        !ProgressionJsonTakeObject(run, "escrow_units", &escrow_units_owner) ||
        !ProgressionJsonTakeObject(nexus, "events", &events_owner) ||
        !ProgressionJsonTakeObject(events, "completions", &event_completions_owner) ||
        !ProgressionJsonTakeObject(events, "unlocked_at", &unlocked_at_owner) ||
        !ProgressionJsonTakeObject(events, "remaining_runs", &remaining_runs_owner))
        goto save_json_failed;

#define PROGRESSION_SAVE_NUMBER(object, key, value)                                                                    \
    do {                                                                                                               \
        if (!cJSON_AddNumberToObject((object), (key), (value))) goto save_json_failed;                                 \
    } while (0)

    PROGRESSION_SAVE_NUMBER(root, "version", PROGRESSION_SAVE_VERSION);
    PROGRESSION_SAVE_NUMBER(account, "level", player->level);
    PROGRESSION_SAVE_NUMBER(account, "experience", player->experience);
    PROGRESSION_SAVE_NUMBER(account, "experience_min", player->experience_min);
    PROGRESSION_SAVE_NUMBER(account, "experience_max", player->experience_max);
    PROGRESSION_SAVE_NUMBER(account, "energy", player->energy);
    PROGRESSION_SAVE_NUMBER(account, "energy_max", player->energy_max);
    PROGRESSION_SAVE_NUMBER(account, "stamina", player->stamina);
    PROGRESSION_SAVE_NUMBER(account, "stamina_max", player->stamina_max);
    PROGRESSION_SAVE_NUMBER(account, "energy_regen_timer", player->energy_regen_timer);
    PROGRESSION_SAVE_NUMBER(account, "stamina_regen_timer", player->stamina_regen_timer);
    PROGRESSION_SAVE_NUMBER(account, "gold", player->gold);
    PROGRESSION_SAVE_NUMBER(account, "gems", player->gems);
    if (player->name[0] && !cJSON_AddStringToObject(account, "name", player->name)) goto save_json_failed;

    PROGRESSION_SAVE_NUMBER(store, "rng_state", player->store_rng_state);
    PROGRESSION_SAVE_NUMBER(store, "opened_total", player->store_opened_total);
    for (int pack = 0; pack < PLAYER_STORE_PACK_COUNT; pack++)
        if (!ProgressionJsonAddArrayNumber(opened, player->store_opened_by_pack[pack])) goto save_json_failed;
    PROGRESSION_SAVE_NUMBER(store, "champion_since_legendary", player->champion_since_legendary);
    cJSON *vouchers = cJSON_AddArrayToObject(store, "vouchers");
    if (!vouchers) goto save_json_failed;
    for (int voucher = 0; voucher < PLAYER_VOUCHER_CAPACITY; voucher++)
        if (!ProgressionJsonAddArrayNumber(vouchers, player->pack_vouchers[voucher])) goto save_json_failed;

    int roster_count = managed_roster ? player->roster_count : 0;
    uint64_t next_instance_id = managed_roster ? player->next_instance_id : 1;
    PROGRESSION_SAVE_NUMBER(roster, "starter_count", 0);
    PROGRESSION_SAVE_NUMBER(roster, "next_instance_id", (double)next_instance_id);
    for (int i = 0; i < roster_count; i++) {
        const Character *ch = &player->roster[i];
        cJSON *entry = cJSON_CreateObject();
        if (!entry || !cJSON_AddStringToObject(entry, "id", ch->id) ||
            !cJSON_AddNumberToObject(entry, "instance_id", (double)ch->instance_id) ||
            !cJSON_AddStringToObject(entry, "tier", TIER_NAMES[ch->tier]) ||
            !cJSON_AddNumberToObject(entry, "level", ch->level) || !cJSON_AddNumberToObject(entry, "xp", ch->xp) ||
            !cJSON_AddBoolToObject(entry, "ascended", ch->ascended) ||
            !cJSON_AddNumberToObject(entry, "consume_xp", ch->consume_xp)) {
            cJSON_Delete(entry);
            goto save_json_failed;
        }
        cJSON *fortify = FortifyChildrenJson(ch);
        if (fortify) cJSON_AddItemToObject(entry, "fortify", fortify);
        if (!ProgressionJsonAddArrayItem(additions, entry)) goto save_json_failed;
    }

    int current_squad = managed_roster ? player->current_squad : 0;
    if (current_squad < 0 || current_squad >= PLAYER_MAX_SQUADS) goto save_json_failed;
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++) {
        cJSON *slots = cJSON_CreateArray();
        if (!slots) goto save_json_failed;
        bool slots_valid = true;
        for (int slot = 0; slot < SQUAD_SLOT_COUNT && slots_valid; slot++) {
            int index = managed_roster ? player->squads[squad][slot] : -1;
            if (index < 0) {
                slots_valid = ProgressionJsonAddArrayNull(slots);
                continue;
            }
            slots_valid = index < roster_count && player->roster[index].giant == (slot == SQUAD_SLOT_LARGE);
            for (int previous = 0; previous < slot && slots_valid; previous++)
                slots_valid = player->squads[squad][previous] != index;
            if (slots_valid)
                slots_valid = ProgressionJsonAddArrayNumber(slots, (double)player->roster[index].instance_id);
        }
        if (!slots_valid || !ProgressionJsonAddArrayItem(squads, slots)) {
            if (!slots_valid) cJSON_Delete(slots);
            goto save_json_failed;
        }
    }
    PROGRESSION_SAVE_NUMBER(root, "current_squad", current_squad);

    PROGRESSION_SAVE_NUMBER(campaign, "map_index", player->campaign_progress.campaign_index);
    PROGRESSION_SAVE_NUMBER(campaign, "node_index", player->campaign_progress.node_index);
    const PlayerCampaignAnimation *animation = &player->campaign_progress.pending_animation;
    if (!cJSON_AddBoolToObject(pending, "active", animation->active) ||
        !cJSON_AddBoolToObject(pending, "unlock_next", animation->unlock_next))
        goto save_json_failed;
    PROGRESSION_SAVE_NUMBER(pending, "from_map", animation->from_map);
    PROGRESSION_SAVE_NUMBER(pending, "from_node", animation->from_node);
    PROGRESSION_SAVE_NUMBER(pending, "to_map", animation->to_map);
    PROGRESSION_SAVE_NUMBER(pending, "to_node", animation->to_node);
    for (int map = 0; map < CAMPAIGN_MAX_MAPS; map++) {
        cJSON *nodes = cJSON_CreateArray();
        if (!nodes) goto save_json_failed;
        bool nodes_valid = true;
        for (int node = 0; node < CAMPAIGN_MAX_NODES; node++)
            nodes_valid = nodes_valid &&
                          ProgressionJsonAddArrayNumber(nodes, player->campaign_progress.node_completions[map][node]);
        if (!nodes_valid || !ProgressionJsonAddArrayItem(completions, nodes)) {
            if (!nodes_valid) cJSON_Delete(nodes);
            goto save_json_failed;
        }
    }

    cJSON *node_losses = cJSON_AddArrayToObject(campaign, "node_losses");
    if (!node_losses) goto save_json_failed;
    for (int map = 0; map < CAMPAIGN_MAX_MAPS; map++) {
        for (int node = 0; node < CAMPAIGN_MAX_NODES; node++) {
            if (!ProgressionJsonAddArrayNumber(node_losses, player->campaign_progress.node_losses[map][node]))
                goto save_json_failed;
        }
    }

    const PlayerNexusRun *nexus_run = &player->nexus_run;
    PROGRESSION_SAVE_NUMBER(nexus, "potions", player->potions);
    PROGRESSION_SAVE_NUMBER(nexus, "potion_max", player->potion_max);
    PROGRESSION_SAVE_NUMBER(nexus, "news_seen_at", (double)player->news_seen_at);
    for (int key = 0; key < NEXUS_KEY_COUNT; key++)
        if (!ProgressionJsonAddArrayNumber(nexus_keys, player->keys[key])) goto save_json_failed;
    if (!cJSON_AddStringToObject(run, "map_id", nexus_run->map_id) ||
        !cJSON_AddBoolToObject(run, "committed", nexus_run->committed) ||
        !cJSON_AddBoolToObject(run, "reset", nexus_run->reset) ||
        !cJSON_AddBoolToObject(run, "pending_animation", nexus_run->pending_animation))
        goto save_json_failed;
    PROGRESSION_SAVE_NUMBER(run, "event_index", nexus_run->event_index);
    PROGRESSION_SAVE_NUMBER(run, "difficulty_index", nexus_run->difficulty_index);
    PROGRESSION_SAVE_NUMBER(run, "current_node_step", nexus_run->current_node_step);
    PROGRESSION_SAVE_NUMBER(run, "selected_node_step", nexus_run->selected_node_step);
    PROGRESSION_SAVE_NUMBER(run, "escrow_gold", nexus_run->escrow_gold);
    PROGRESSION_SAVE_NUMBER(run, "escrow_xp", nexus_run->escrow_xp);
    PROGRESSION_SAVE_NUMBER(run, "escrow_potions", nexus_run->escrow_potions);
    PROGRESSION_SAVE_NUMBER(run, "pending_from_step", nexus_run->pending_from_step);
    PROGRESSION_SAVE_NUMBER(run, "pending_to_step", nexus_run->pending_to_step);
    for (int rarity = 0; rarity <= RARITY_MYTHIC; rarity++)
        if (!ProgressionJsonAddArrayNumber(escrow_units, nexus_run->escrow_units[rarity])) goto save_json_failed;
    for (int index = 0; index < NEXUS_MAX_NODES; index++) {
        const PlayerNexusNode *node = &nexus_run->nodes[index];
        cJSON *entry = cJSON_CreateObject();
        if (!entry || !cJSON_AddNumberToObject(entry, "wins", node->wins) ||
            !cJSON_AddNumberToObject(entry, "losses", node->losses) ||
            !cJSON_AddNumberToObject(entry, "previous_wins", node->previous_wins)) {
            cJSON_Delete(entry);
            goto save_json_failed;
        }
        if (!ProgressionJsonAddArrayItem(run_nodes, entry)) goto save_json_failed;
    }
    for (int event = 0; event < NEXUS_MAX_EVENTS; event++) {
        cJSON *tiers = cJSON_CreateArray();
        if (!tiers) goto save_json_failed;
        bool tiers_valid = true;
        for (int difficulty = 0; difficulty < NEXUS_MAX_DIFFICULTIES; difficulty++)
            tiers_valid = tiers_valid &&
                          ProgressionJsonAddArrayNumber(tiers, player->nexus_events.completions[event][difficulty]);
        if (!tiers_valid || !ProgressionJsonAddArrayItem(event_completions, tiers)) {
            if (!tiers_valid) cJSON_Delete(tiers);
            goto save_json_failed;
        }
        if (!ProgressionJsonAddArrayNumber(unlocked_at, (double)player->nexus_events.unlocked_at[event]) ||
            !ProgressionJsonAddArrayNumber(remaining_runs, player->nexus_events.remaining_runs[event]))
            goto save_json_failed;
    }

    cJSON *gear = ProgressionSaveGear(player);
    if (!gear || !cJSON_AddItemToObject(root, "gear", gear)) {
        cJSON_Delete(gear);
        goto save_json_failed;
    }
    cJSON *runes = ProgressionSaveRunes(&player->runes);
    if (!runes || !cJSON_AddItemToObject(root, "runes", runes)) {
        cJSON_Delete(runes);
        goto save_json_failed;
    }

    cJSON *missions = ProgressionSaveMissions(&player->missions);
    if (!missions || !cJSON_AddItemToObject(root, "missions", missions)) {
        cJSON_Delete(missions);
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *pvp = ProgressionSavePvp(&player->pvp);
    if (!pvp || !cJSON_AddItemToObject(root, "pvp", pvp)) {
        cJSON_Delete(pvp);
        goto save_json_failed;
    }

    cJSON *gallery = ProgressionSaveGallery(&player->gallery);
    if (!gallery || !cJSON_AddItemToObject(root, "gallery", gallery)) {
        cJSON_Delete(gallery);
        goto save_json_failed;
    }

    cJSON *expeditions = ExpeditionsSave(&player->expeditions);
    if (!expeditions || !cJSON_AddItemToObject(root, "expeditions", expeditions)) {
        cJSON_Delete(expeditions);
        goto save_json_failed;
    }

    cJSON *timed_quests = TimedQuestsSave(&player->timed_quests);
    if (!timed_quests || !cJSON_AddItemToObject(root, "timed_quests", timed_quests)) {
        cJSON_Delete(timed_quests);
        goto save_json_failed;
    }

    cJSON *tutorial = cJSON_AddObjectToObject(root, "tutorial");
    if (!tutorial || !cJSON_AddBoolToObject(tutorial, "complete", player->tutorial.main_complete) ||
        !cJSON_AddBoolToObject(tutorial, "consume_complete", player->tutorial.consume_complete))
        goto save_json_failed;
    PROGRESSION_SAVE_NUMBER(tutorial, "restore_step", player->tutorial.restore_step);

    cJSON *settings = cJSON_AddObjectToObject(root, "settings");
    if (!settings || !cJSON_AddBoolToObject(settings, "fast_battles", player->fast_battles)) goto save_json_failed;

    serialized = cJSON_Print(root);
    cJSON_Delete(root);
    root = NULL;
    if (!serialized) goto save_json_failed;

#undef PROGRESSION_SAVE_NUMBER

    return serialized;

save_json_failed:
    cJSON_Delete(root);
    cJSON_Delete(account_owner);
    cJSON_Delete(campaign_owner);
    cJSON_Delete(completions_owner);
    cJSON_Delete(pending_owner);
    cJSON_Delete(store_owner);
    cJSON_Delete(opened_owner);
    cJSON_Delete(roster_owner);
    cJSON_Delete(starters_owner);
    cJSON_Delete(additions_owner);
    cJSON_Delete(squads_owner);
    cJSON_Delete(nexus_owner);
    cJSON_Delete(nexus_keys_owner);
    cJSON_Delete(run_owner);
    cJSON_Delete(run_nodes_owner);
    cJSON_Delete(escrow_units_owner);
    cJSON_Delete(events_owner);
    cJSON_Delete(event_completions_owner);
    cJSON_Delete(unlocked_at_owner);
    cJSON_Delete(remaining_runs_owner);
    if (serialized) cJSON_free(serialized);
    return NULL;
}

bool ProgressionSaveToPath(const Player *player, const char *path) {
    if (!player || !path || !path[0]) return false;
    char *serialized = ProgressionSerialize(player);
    if (!serialized) return false;
    char temporary[1024];
    int needed = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    bool ok = needed > 0 && needed < (int)sizeof(temporary) && SaveFileText(temporary, serialized);
    cJSON_free(serialized);
    if (ok) ok = PlatformReplaceFile(temporary, path);
    if (!ok && needed > 0 && needed < (int)sizeof(temporary)) remove(temporary);
    if (!ok) TraceLog(LOG_WARNING, "Progression: could not save %s", path);
    return ok;
}

typedef struct {
    Character character;
    Ability ability;
} ProgressionRosterAddition;

static bool ProgressionJsonArrayInt(const cJSON *array, int index, int min, int max, int *out) {
    cJSON *item = cJSON_GetArrayItem((cJSON *)array, index);
    if (!cJSON_IsNumber(item) || item->valuedouble < min || item->valuedouble > max ||
        floor(item->valuedouble) != item->valuedouble)
        return false;
    *out = (int)item->valuedouble;
    return true;
}

static bool ProgressionJsonItemU64(const cJSON *item, uint64_t min, uint64_t max, uint64_t *out) {
    if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) || item->valuedouble < (double)min ||
        item->valuedouble > (double)max || floor(item->valuedouble) != item->valuedouble)
        return false;
    *out = (uint64_t)item->valuedouble;
    return true;
}

static int ProgressionFutureRosterIndex(
    const Player *player,
    int starter_count,
    const ProgressionRosterAddition *additions,
    int addition_count,
    uint64_t instance_id
) {
    for (int i = 0; i < starter_count; i++)
        if (player->roster[i].instance_id == instance_id) return i;
    for (int i = 0; i < addition_count; i++)
        if (additions[i].character.instance_id == instance_id) return starter_count + i;
    return -1;
}

static bool ProgressionLoadRunes(const cJSON *json, PlayerRunes *runes) {
    RuneCatalogEnsure();
    memset(runes, 0, sizeof(*runes));
    const cJSON *items = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "items");
    const cJSON *squads = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "squads");
    if (!cJSON_IsArray(items) || !cJSON_IsArray(squads) || cJSON_GetArraySize(squads) != PLAYER_MAX_SQUADS ||
        !ProgressionJsonU64(json, "next_uuid", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &runes->next_uuid) ||
        !ProgressionJsonInt(json, "seen", 0, RUNE_INVENTORY_CAPACITY, &runes->seen))
        return false;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, (cJSON *)items) {
        if (runes->count >= RUNE_INVENTORY_CAPACITY) return false;
        PlayerRune rune = {.def = RuneDefIndex(JsonStr(item, "def"))};
        uint64_t expire = 0, inventory_expire = 0;
        if (!ProgressionJsonU64(item, "uuid", 1, PROGRESSION_JSON_MAX_SAFE_INTEGER, &rune.uuid) ||
            !ProgressionJsonU64(item, "expire_at", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &expire) ||
            !ProgressionJsonU64(item, "inventory_expire_at", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &inventory_expire) ||
            rune.uuid >= runes->next_uuid || RuneByUuid(runes, rune.uuid))
            return false;
        if (rune.def < 0) {
            TraceLog(LOG_WARNING, "Progression: dropped unknown rune %s", JsonStr(item, "def"));
            continue;
        }
        rune.expire_at = (long long)expire;
        rune.inventory_expire_at = (long long)inventory_expire;
        runes->items[runes->count++] = rune;
    }
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++) {
        const cJSON *entry = cJSON_GetArrayItem(squads, squad);
        const cJSON *slots = cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "slots");
        const cJSON *unlocked = cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "prestige_unlocked");
        if (!cJSON_IsArray(slots) || cJSON_GetArraySize(slots) != RUNE_SLOT_COUNT || !cJSON_IsBool(unlocked))
            return false;
        runes->prestige_unlocked[squad] = cJSON_IsTrue(unlocked);
        for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++) {
            uint64_t uuid = 0;
            if (!ProgressionJsonItemU64(cJSON_GetArrayItem(slots, slot), 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &uuid))
                return false;
            runes->squad_slots[squad][slot] = RuneByUuid(runes, uuid) ? uuid : 0;
        }
    }
    return true;
}

static void ProgressionLoadMissions(const cJSON *json, PlayerMissions *missions) {
    memset(missions, 0, sizeof(*missions));
    if (!cJSON_IsObject(json)) return;
    const cJSON *counters = cJSON_GetObjectItemCaseSensitive(json, "counters");
    const cJSON *progress = cJSON_GetObjectItemCaseSensitive(json, "progress");
    const cJSON *claimed = cJSON_GetObjectItemCaseSensitive(json, "claimed");
    ProgressionJsonInt(json, "last_battle_day", 0, INT_MAX, &missions->last_battle_day);
    for (int stat = 0; stat < MISSION_STAT_COUNT; stat++) {
        const cJSON *value = cJSON_GetObjectItemCaseSensitive(counters, MISSION_STAT_NAMES[stat]);
        if (cJSON_IsNumber(value) && value->valuedouble > 0.0) missions->counters[stat] = (long long)value->valuedouble;
    }
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, progress) {
        int index = MissionIndexById(entry->string);
        if (index != MISSION_NONE && cJSON_IsNumber(entry) && entry->valuedouble > 0.0)
            missions->progress[index] = (long long)entry->valuedouble;
    }
    cJSON_ArrayForEach(entry, claimed) {
        const char *id = cJSON_GetStringValue(entry);
        int index = id ? MissionIndexById(id) : MISSION_NONE;
        if (index != MISSION_NONE && MissionCompleted(missions, index)) missions->claimed[index] = true;
    }
}

static bool ProgressionLoadPvpBattle(const cJSON *entry, PlayerPvpBattle *battle) {
    uint64_t at = 0, seed = 0;
    bool ok = ProgressionJsonU64(entry, "at", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &at) &&
              ProgressionJsonU64(entry, "seed", 0, UINT32_MAX, &seed) &&
              ProgressionJsonInt(entry, "bot", 0, INT_MAX, &battle->bot) &&
              ProgressionJsonInt(entry, "squad", 0, INT_MAX, &battle->squad) &&
              ProgressionJsonInt(entry, "level", 1, PROGRESSION_LOCAL_MAX_LEVEL, &battle->level) &&
              ProgressionJsonInt(entry, "opponent_trophies", 0, INT_MAX, &battle->opponent_trophies) &&
              ProgressionJsonInt(entry, "trophy_delta", INT_MIN, INT_MAX, &battle->trophy_delta) &&
              ProgressionJsonInt(entry, "gold", 0, INT_MAX, &battle->gold) &&
              ProgressionJsonInt(entry, "xp", 0, INT_MAX, &battle->xp) &&
              ProgressionJsonInt(entry, "banners", 0, INT_MAX, &battle->banners);
    battle->at = (long long)at;
    battle->seed = (unsigned)seed;
    battle->defense = JsonBool(entry, "defense");
    battle->won = JsonBool(entry, "won");
    battle->revenged = JsonBool(entry, "revenged");
    StrCopy(battle->opponent, sizeof(battle->opponent), JsonStr(entry, "opponent"));
    return ok;
}

static bool ProgressionLoadPvpEvents(const cJSON *json, PlayerPvp *pvp) {
    const cJSON *event = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "event");
    const cJSON *results = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "event_results");
    uint64_t cycle = 0;
    bool ok = cJSON_IsObject(event) && cJSON_IsArray(results) &&
              ProgressionJsonU64(event, "cycle", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &cycle) &&
              ProgressionJsonInt(event, "banners", 0, INT_MAX, &pvp->event.banners) &&
              ProgressionJsonInt(event, "milestones_paid", 0, INT_MAX, &pvp->event.milestones_paid);
    pvp->event.cycle = (long long)cycle;
    pvp->event.buffed = JsonBool(event, "buffed");
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)results) {
        if (!ok || pvp->event_result_count == PVP_EVENT_RESULT_CAPACITY) break;
        PlayerPvpEventResult *result = &pvp->event_results[pvp->event_result_count++];
        ok = ProgressionJsonU64(entry, "cycle", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &cycle) &&
             ProgressionJsonInt(entry, "banners", 0, INT_MAX, &result->banners) &&
             ProgressionJsonInt(entry, "rank", 1, INT_MAX, &result->rank) &&
             ProgressionJsonInt(entry, "gold", 0, INT_MAX, &result->gold) &&
             ProgressionJsonInt(entry, "gems", 0, INT_MAX, &result->gems);
        result->cycle = (long long)cycle;
        result->claimed = JsonBool(entry, "claimed");
    }
    return ok;
}

static bool ProgressionLoadPvp(const cJSON *json, PlayerPvp *pvp) {
    uint64_t nexts_day = 0, checked_at = 0;
    const cJSON *history = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "history");
    bool ok = cJSON_IsObject(json) && cJSON_IsArray(history) &&
              ProgressionJsonInt(json, "trophies", 0, INT_MAX, &pvp->trophies) &&
              ProgressionJsonInt(json, "best_trophies", 0, INT_MAX, &pvp->best_trophies) &&
              ProgressionJsonInt(json, "trophies_seen", 0, INT_MAX, &pvp->trophies_seen) &&
              ProgressionJsonInt(json, "wins", 0, INT_MAX, &pvp->wins) &&
              ProgressionJsonInt(json, "losses", 0, INT_MAX, &pvp->losses) &&
              ProgressionJsonInt(json, "win_streak", 0, INT_MAX, &pvp->win_streak) &&
              ProgressionJsonInt(json, "best_win_streak", 0, INT_MAX, &pvp->best_win_streak) &&
              ProgressionJsonInt(json, "trophies_won", 0, INT_MAX, &pvp->trophies_won) &&
              ProgressionJsonInt(json, "defense_wins", 0, INT_MAX, &pvp->defense_wins) &&
              ProgressionJsonInt(json, "defense_losses", 0, INT_MAX, &pvp->defense_losses) &&
              ProgressionJsonInt(json, "nexts_used", 0, INT_MAX, &pvp->nexts_used) &&
              ProgressionJsonU64(json, "nexts_day", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &nexts_day) &&
              ProgressionJsonU64(json, "defense_checked_at", 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &checked_at);
    pvp->nexts_day = (long long)nexts_day;
    pvp->defense_checked_at = (long long)checked_at;
    const cJSON *held_since = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "held_since");
    ok = ok && cJSON_IsArray(held_since) && cJSON_GetArraySize(held_since) == PVP_HOLD_THRESHOLD_COUNT;
    for (int i = 0; ok && i < PVP_HOLD_THRESHOLD_COUNT; i++) {
        uint64_t stamp = 0;
        ok = ProgressionJsonItemU64(cJSON_GetArrayItem(held_since, i), 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &stamp);
        pvp->held_since[i] = (long long)stamp;
    }
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)history) {
        if (!ok || pvp->history_count == PVP_HISTORY_CAPACITY) break;
        ok = ProgressionLoadPvpBattle(entry, &pvp->history[pvp->history_count++]);
    }
    return ok && ProgressionLoadPvpEvents(json, pvp);
}

static bool ProgressionLoadGallery(const cJSON *json, PlayerGallery *gallery) {
    const cJSON *units = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "units");
    if (!cJSON_IsArray(units) || !ProgressionJsonInt(json, "badge", 0, INT_MAX, &gallery->badge)) return false;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)units) {
        int owned_tiers = 0;
        const char *id = JsonStr(entry, "id");
        if (!id[0] || strlen(id) >= GALLERY_RECORD_ID_CAP ||
            !ProgressionJsonInt(entry, "owned_tiers", 0, (1 << TIER_COUNT) - 1, &owned_tiers))
            return false;
        GalleryRecord *record = GalleryRecordObtain(gallery, id);
        if (!record) return false;
        record->owned_tiers = (unsigned char)owned_tiers;
        record->fought = JsonBool(entry, "fought");
        record->defeated = JsonBool(entry, "defeated");
        record->new_owned = JsonBool(entry, "new_owned");
        record->new_fought = JsonBool(entry, "new_fought");
        record->new_defeated = JsonBool(entry, "new_defeated");
    }
    gallery->seeded = true;
    return true;
}

bool ProgressionLoadFromText(Player *player, const char *text, const char *origin) {
    if (!player || !text) return false;
    cJSON *root = cJSON_Parse(text);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        TraceLog(LOG_WARNING, "Progression: invalid save %s", origin);
        return false;
    }

    cJSON *account = cJSON_GetObjectItemCaseSensitive(root, "account");
    cJSON *campaign = cJSON_GetObjectItemCaseSensitive(root, "campaign");
    cJSON *completions = cJSON_GetObjectItemCaseSensitive(campaign, "completions");
    cJSON *pending = cJSON_GetObjectItemCaseSensitive(campaign, "pending_animation");
    PlayerCampaignProgress loaded_progress = {0};
    int version = 0, level = 0, experience = 0, experience_min = 0, experience_max = 0;
    int energy = 0, energy_max = 0, stamina = 0, stamina_max = 0, gold = 0, gems = 0;
    float energy_timer = 0.0f, stamina_timer = 0.0f;
    bool valid = cJSON_IsObject(account) && cJSON_IsObject(campaign) &&
                 ProgressionJsonInt(root, "version", 2, PROGRESSION_SAVE_VERSION, &version) &&
                 ProgressionJsonInt(account, "level", 1, PROGRESSION_LOCAL_MAX_LEVEL, &level) &&
                 ProgressionJsonInt(account, "experience", 0, INT_MAX, &experience) &&
                 ProgressionJsonInt(account, "experience_min", 0, INT_MAX, &experience_min) &&
                 ProgressionJsonInt(account, "experience_max", 1, INT_MAX, &experience_max) &&
                 ProgressionJsonInt(account, "energy", 0, INT_MAX, &energy) &&
                 ProgressionJsonInt(account, "energy_max", 1, INT_MAX, &energy_max) &&
                 ProgressionJsonInt(account, "stamina", 0, INT_MAX, &stamina) &&
                 ProgressionJsonInt(account, "stamina_max", 1, INT_MAX, &stamina_max) &&
                 ProgressionJsonTimer(account, "energy_regen_timer", &energy_timer) &&
                 ProgressionJsonTimer(account, "stamina_regen_timer", &stamina_timer) &&
                 ProgressionJsonInt(account, "gold", 0, INT_MAX, &gold) &&
                 ProgressionJsonInt(account, "gems", 0, INT_MAX, &gems) &&
                 ProgressionJsonInt(campaign, "map_index", 0, CAMPAIGN_MAX_MAPS - 1, &loaded_progress.campaign_index) &&
                 ProgressionJsonInt(campaign, "node_index", 0, CAMPAIGN_MAX_NODES - 1, &loaded_progress.node_index) &&
                 cJSON_IsArray(completions) && cJSON_GetArraySize(completions) == CAMPAIGN_MAX_MAPS;

    if (valid && pending) {
        cJSON *active = cJSON_GetObjectItemCaseSensitive(pending, "active");
        valid = cJSON_IsObject(pending) && cJSON_IsBool(active);
        loaded_progress.pending_animation.active = valid && cJSON_IsTrue(active);
        if (valid && loaded_progress.pending_animation.active) {
            cJSON *unlock_next = cJSON_GetObjectItemCaseSensitive(pending, "unlock_next");
            valid = cJSON_IsBool(unlock_next) &&
                    ProgressionJsonInt(
                        pending, "from_map", 0, CAMPAIGN_MAX_MAPS - 1, &loaded_progress.pending_animation.from_map
                    ) &&
                    ProgressionJsonInt(
                        pending, "from_node", 0, CAMPAIGN_MAX_NODES - 1, &loaded_progress.pending_animation.from_node
                    ) &&
                    ProgressionJsonInt(
                        pending, "to_map", 0, CAMPAIGN_MAX_MAPS - 1, &loaded_progress.pending_animation.to_map
                    ) &&
                    ProgressionJsonInt(
                        pending, "to_node", 0, CAMPAIGN_MAX_NODES - 1, &loaded_progress.pending_animation.to_node
                    );
            loaded_progress.pending_animation.unlock_next = valid && cJSON_IsTrue(unlock_next);
        }
    }
    if (valid) {
        for (int map = 0; map < CAMPAIGN_MAX_MAPS && valid; map++) {
            cJSON *nodes = cJSON_GetArrayItem(completions, map);
            valid = cJSON_IsArray(nodes) && cJSON_GetArraySize(nodes) == CAMPAIGN_MAX_NODES;
            for (int node = 0; node < CAMPAIGN_MAX_NODES && valid; node++)
                valid = ProgressionJsonArrayInt(nodes, node, 0, INT_MAX, &loaded_progress.node_completions[map][node]);
        }
        cJSON *node_losses = cJSON_GetObjectItemCaseSensitive(campaign, "node_losses");
        if (cJSON_IsArray(node_losses)) {
            valid = cJSON_GetArraySize(node_losses) == CAMPAIGN_MAX_MAPS * CAMPAIGN_MAX_NODES;
            for (int map = 0; map < CAMPAIGN_MAX_MAPS && valid; map++) {
                for (int node = 0; node < CAMPAIGN_MAX_NODES && valid; node++)
                    valid = ProgressionJsonArrayInt(
                        node_losses,
                        map * CAMPAIGN_MAX_NODES + node,
                        0,
                        INT_MAX,
                        &loaded_progress.node_losses[map][node]
                    );
            }
        }
    }
    valid = valid && experience_min == ProgressionXpForCurrentLevel(level) &&
            experience_max == ProgressionXpForNextLevel(level) &&
            (level == PROGRESSION_LOCAL_MAX_LEVEL ? experience == experience_max
                                                  : experience >= experience_min && experience < experience_max) &&
            energy_max == ProgressionEnergyMaxForLevel(level) && energy <= energy_max && stamina <= stamina_max;

    uint32_t loaded_rng = player->store_rng_state;
    int loaded_opened_total = player->store_opened_total;
    int loaded_opened[PLAYER_STORE_PACK_COUNT];
    memcpy(loaded_opened, player->store_opened_by_pack, sizeof(loaded_opened));
    int loaded_champion_since = player->champion_since_legendary;
    int loaded_vouchers[PLAYER_VOUCHER_CAPACITY];
    memcpy(loaded_vouchers, player->pack_vouchers, sizeof(loaded_vouchers));
    int saved_starter_count = 0;
    int addition_count = 0;
    uint64_t loaded_next_instance_id = player->next_instance_id;
    ProgressionRosterAddition *loaded_additions = NULL;
    int loaded_squads[PLAYER_MAX_SQUADS][SQUAD_SLOT_COUNT];
    int loaded_current_squad = player->current_squad;
    bool commit_roster = false;
    PlayerNexusRun loaded_run = {0};
    PlayerNexusEvents loaded_events = {0};
    int loaded_potions = player->potions;
    int loaded_potion_max = player->potion_max;
    long long loaded_news_seen_at = 0;
    int loaded_keys[NEXUS_KEY_COUNT];
    memcpy(loaded_keys, player->keys, sizeof(loaded_keys));

    if (valid && version >= 3) {
        cJSON *store = cJSON_GetObjectItemCaseSensitive(root, "store");
        cJSON *opened = cJSON_GetObjectItemCaseSensitive(store, "opened_by_pack");
        uint64_t rng_value = 0;
        valid = cJSON_IsObject(store) && ProgressionJsonU64(store, "rng_state", 0, UINT32_MAX, &rng_value) &&
                ProgressionJsonInt(store, "opened_total", 0, INT_MAX, &loaded_opened_total) &&
                ProgressionJsonInt(store, "champion_since_legendary", 0, INT_MAX, &loaded_champion_since) &&
                cJSON_IsArray(opened) && cJSON_GetArraySize(opened) == PLAYER_STORE_PACK_COUNT;
        loaded_rng = (uint32_t)rng_value;
        int opened_sum = 0;
        for (int pack = 0; pack < PLAYER_STORE_PACK_COUNT && valid; pack++) {
            valid = ProgressionJsonArrayInt(opened, pack, 0, INT_MAX, &loaded_opened[pack]) &&
                    opened_sum <= INT_MAX - loaded_opened[pack];
            if (valid) opened_sum += loaded_opened[pack];
        }
        valid = valid && opened_sum == loaded_opened_total && loaded_champion_since <= opened_sum;
        int voucher = 0;
        const cJSON *wallet = NULL;
        cJSON_ArrayForEach(wallet, cJSON_GetObjectItemCaseSensitive(store, "vouchers")) {
            if (voucher < PLAYER_VOUCHER_CAPACITY && cJSON_IsNumber(wallet) && wallet->valueint >= 0)
                loaded_vouchers[voucher] = wallet->valueint;
            voucher++;
        }

        cJSON *roster = cJSON_GetObjectItemCaseSensitive(root, "roster");
        cJSON *starters = cJSON_GetObjectItemCaseSensitive(roster, "starters");
        cJSON *additions = cJSON_GetObjectItemCaseSensitive(roster, "additions");
        valid = valid && cJSON_IsObject(roster) &&
                ProgressionJsonInt(roster, "starter_count", 0, PLAYER_ROSTER_CAPACITY, &saved_starter_count) &&
                ProgressionJsonU64(
                    roster, "next_instance_id", 1, PROGRESSION_JSON_MAX_SAFE_INTEGER, &loaded_next_instance_id
                ) &&
                cJSON_IsArray(starters) && cJSON_IsArray(additions);
        addition_count = valid ? cJSON_GetArraySize(additions) : 0;
        bool target_managed = player->roster && player->roster_abilities && player->roster_capacity > 0;
        commit_roster = target_managed;
        if (valid && target_managed) {
            int expected_starters = version >= PROGRESSION_FIRST_FULL_ROSTER_VERSION ? 0 : player->starter_roster_count;
            valid = ProgressionRosterStateValid(player) && saved_starter_count == expected_starters &&
                    cJSON_GetArraySize(starters) == saved_starter_count && addition_count >= 0 &&
                    addition_count <= player->roster_capacity - saved_starter_count;
        } else if (valid) {
            valid = saved_starter_count == 0 && cJSON_GetArraySize(starters) == 0 && addition_count == 0;
        }

        if (valid && addition_count > 0) {
            loaded_additions = (ProgressionRosterAddition *)calloc((size_t)addition_count, sizeof(*loaded_additions));
            valid = loaded_additions != NULL;
        }
        cJSON *catalog_root = NULL;
        cJSON *catalog = NULL;
        if (valid && addition_count > 0) {
            catalog_root = ProgressionLoadCharacterCatalog();
            catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
            valid = cJSON_IsArray(catalog);
        }

        uint64_t max_instance_id = 0;
        if (valid && target_managed) {
            for (int i = 0; i < saved_starter_count && valid; i++) {
                cJSON *entry = cJSON_GetArrayItem(starters, i);
                const char *id = JsonStr(entry, "id");
                uint64_t instance_id = player->roster[i].instance_id;
                uint64_t saved_instance_id = 0;
                valid = cJSON_IsObject(entry) && id[0] && strcmp(id, player->roster[i].id) == 0 &&
                        ProgressionJsonU64(
                            entry, "instance_id", 1, PROGRESSION_JSON_MAX_SAFE_INTEGER, &saved_instance_id
                        ) &&
                        saved_instance_id == instance_id && instance_id <= PROGRESSION_JSON_MAX_SAFE_INTEGER;
                for (int previous = 0; previous < i && valid; previous++)
                    valid = player->roster[previous].instance_id != instance_id;
                if (valid && instance_id > max_instance_id) max_instance_id = instance_id;
            }
        }
        for (int n = 0; n < addition_count && valid; n++) {
            cJSON *entry = cJSON_GetArrayItem(additions, n);
            const char *id = JsonStr(entry, "id");
            cJSON *def = cJSON_IsObject(entry) && id[0] ? CatalogFindCharacter(catalog, id) : NULL;
            Character *ch = &loaded_additions[n].character;
            Ability *ability = &loaded_additions[n].ability;
            valid = def && PlayerRosterParseDefinition(def, ch, ability);
            const char *tier_name = JsonStr(entry, "tier");
            Tier tier = TierFromString(tier_name);
            int level_value = 0, xp_value = 0, consume_xp_value = 0;
            uint64_t instance_id = 0;
            if (valid && cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(entry, "ascended"))) GearApplyAscension(ch);
            valid = valid && tier_name[0] && strcmp(tier_name, TIER_NAMES[tier]) == 0 && ch->has_tier[tier] &&
                    ProgressionJsonInt(entry, "level", 1, ch->stats[tier].max_level, &level_value) &&
                    ProgressionJsonU64(entry, "instance_id", 1, PROGRESSION_JSON_MAX_SAFE_INTEGER, &instance_id);
            if (valid && version >= PROGRESSION_FIRST_FULL_ROSTER_VERSION)
                valid = ProgressionJsonInt(entry, "xp", 0, INT_MAX, &xp_value) &&
                        ProgressionJsonInt(entry, "consume_xp", 0, INT_MAX, &consume_xp_value);
            for (int starter = 0; starter < saved_starter_count && valid; starter++)
                valid = player->roster[starter].instance_id != instance_id;
            for (int previous = 0; previous < n && valid; previous++)
                valid = loaded_additions[previous].character.instance_id != instance_id;
            if (valid) {
                ch->tier = tier;
                ch->level = level_value;
                ch->xp = xp_value;
                ch->consume_xp = consume_xp_value;
                ch->instance_id = instance_id;
                ch->ability = ability;
                if (!FortifyChildrenLoad(ch, cJSON_GetObjectItemCaseSensitive(entry, "fortify")))
                    TraceLog(
                        LOG_WARNING,
                        "Progression: dropped unreadable fortify slots of hero %llu",
                        (unsigned long long)instance_id
                    );
                if (instance_id > max_instance_id) max_instance_id = instance_id;
            }
        }
        valid = valid && loaded_next_instance_id > max_instance_id;
        cJSON_Delete(catalog_root);

        cJSON *squads = cJSON_GetObjectItemCaseSensitive(root, "squads");
        valid = valid && cJSON_IsArray(squads) && cJSON_GetArraySize(squads) == PLAYER_MAX_SQUADS &&
                ProgressionJsonInt(root, "current_squad", 0, PLAYER_MAX_SQUADS - 1, &loaded_current_squad);
        for (int squad = 0; squad < PLAYER_MAX_SQUADS && valid; squad++) {
            cJSON *slots = cJSON_GetArrayItem(squads, squad);
            valid = cJSON_IsArray(slots) && cJSON_GetArraySize(slots) == SQUAD_SLOT_COUNT;
            for (int slot = 0; slot < SQUAD_SLOT_COUNT && valid; slot++) {
                cJSON *cell = cJSON_GetArrayItem(slots, slot);
                loaded_squads[squad][slot] = -1;
                if (cJSON_IsNull(cell)) continue;
                uint64_t instance_id = 0;
                valid = ProgressionJsonItemU64(cell, 1, PROGRESSION_JSON_MAX_SAFE_INTEGER, &instance_id);
                int index = valid && target_managed
                                ? ProgressionFutureRosterIndex(
                                      player, saved_starter_count, loaded_additions, addition_count, instance_id
                                  )
                                : -1;
                valid = valid && index >= 0;
                if (valid) {
                    const Character *ch = index < saved_starter_count
                                              ? &player->roster[index]
                                              : &loaded_additions[index - saved_starter_count].character;
                    valid = ch->giant == (slot == SQUAD_SLOT_LARGE);
                }
                for (int previous = 0; previous < slot && valid; previous++)
                    valid = loaded_squads[squad][previous] != index;
                if (valid) loaded_squads[squad][slot] = index;
            }
        }
    }

    if (valid && version >= 4) {
        cJSON *nexus = cJSON_GetObjectItemCaseSensitive(root, "nexus");
        cJSON *nexus_keys = cJSON_GetObjectItemCaseSensitive(nexus, "keys");
        cJSON *run = cJSON_GetObjectItemCaseSensitive(nexus, "run");
        cJSON *run_nodes = cJSON_GetObjectItemCaseSensitive(run, "nodes");
        cJSON *escrow_units = cJSON_GetObjectItemCaseSensitive(run, "escrow_units");
        cJSON *events = cJSON_GetObjectItemCaseSensitive(nexus, "events");
        cJSON *event_completions = cJSON_GetObjectItemCaseSensitive(events, "completions");
        cJSON *unlocked_at = cJSON_GetObjectItemCaseSensitive(events, "unlocked_at");
        cJSON *remaining_runs = cJSON_GetObjectItemCaseSensitive(events, "remaining_runs");
        cJSON *map_id = cJSON_GetObjectItemCaseSensitive(run, "map_id");
        cJSON *committed = cJSON_GetObjectItemCaseSensitive(run, "committed");
        cJSON *reset = cJSON_GetObjectItemCaseSensitive(run, "reset");
        cJSON *run_pending = cJSON_GetObjectItemCaseSensitive(run, "pending_animation");
        cJSON *news_seen_at = cJSON_GetObjectItemCaseSensitive(nexus, "news_seen_at");
        if (cJSON_IsNumber(news_seen_at)) loaded_news_seen_at = (long long)news_seen_at->valuedouble;
        valid =
            cJSON_IsObject(nexus) && cJSON_IsObject(run) && cJSON_IsObject(events) && cJSON_IsString(map_id) &&
            strlen(map_id->valuestring) < sizeof(loaded_run.map_id) && cJSON_IsBool(committed) && cJSON_IsBool(reset) &&
            cJSON_IsBool(run_pending) && ProgressionJsonInt(nexus, "potions", 0, INT_MAX, &loaded_potions) &&
            ProgressionJsonInt(nexus, "potion_max", 0, INT_MAX, &loaded_potion_max) &&
            ProgressionJsonInt(run, "event_index", 0, NEXUS_MAX_EVENTS - 1, &loaded_run.event_index) &&
            ProgressionJsonInt(run, "difficulty_index", 0, NEXUS_MAX_DIFFICULTIES - 1, &loaded_run.difficulty_index) &&
            ProgressionJsonInt(run, "current_node_step", 0, NEXUS_MAX_NODES, &loaded_run.current_node_step) &&
            ProgressionJsonInt(run, "selected_node_step", 0, NEXUS_MAX_NODES, &loaded_run.selected_node_step) &&
            ProgressionJsonInt(run, "escrow_gold", 0, INT_MAX, &loaded_run.escrow_gold) &&
            ProgressionJsonInt(run, "escrow_xp", 0, INT_MAX, &loaded_run.escrow_xp) &&
            ProgressionJsonInt(run, "escrow_potions", 0, INT_MAX, &loaded_run.escrow_potions) &&
            ProgressionJsonInt(run, "pending_from_step", 0, NEXUS_MAX_NODES, &loaded_run.pending_from_step) &&
            ProgressionJsonInt(run, "pending_to_step", 0, NEXUS_MAX_NODES, &loaded_run.pending_to_step) &&
            cJSON_IsArray(nexus_keys) && cJSON_GetArraySize(nexus_keys) == NEXUS_KEY_COUNT &&
            cJSON_IsArray(escrow_units) && cJSON_GetArraySize(escrow_units) == RARITY_MYTHIC + 1 &&
            cJSON_IsArray(run_nodes) && cJSON_GetArraySize(run_nodes) == NEXUS_MAX_NODES &&
            cJSON_IsArray(event_completions) && cJSON_GetArraySize(event_completions) == NEXUS_MAX_EVENTS &&
            cJSON_IsArray(unlocked_at) && cJSON_GetArraySize(unlocked_at) == NEXUS_MAX_EVENTS &&
            cJSON_IsArray(remaining_runs) && cJSON_GetArraySize(remaining_runs) == NEXUS_MAX_EVENTS;
        if (valid) {
            StrCopy(loaded_run.map_id, sizeof(loaded_run.map_id), map_id->valuestring);
            loaded_run.committed = cJSON_IsTrue(committed);
            loaded_run.reset = cJSON_IsTrue(reset);
            loaded_run.pending_animation = cJSON_IsTrue(run_pending);
        }
        for (int key = 0; key < NEXUS_KEY_COUNT && valid; key++)
            valid = ProgressionJsonArrayInt(nexus_keys, key, 0, NEXUS_KEY_MAX, &loaded_keys[key]);
        for (int rarity = 0; rarity <= RARITY_MYTHIC && valid; rarity++)
            valid = ProgressionJsonArrayInt(escrow_units, rarity, 0, INT_MAX, &loaded_run.escrow_units[rarity]);
        for (int index = 0; index < NEXUS_MAX_NODES && valid; index++) {
            cJSON *entry = cJSON_GetArrayItem(run_nodes, index);
            PlayerNexusNode *node = &loaded_run.nodes[index];
            valid = cJSON_IsObject(entry) && ProgressionJsonInt(entry, "wins", 0, INT_MAX, &node->wins) &&
                    ProgressionJsonInt(entry, "losses", 0, INT_MAX, &node->losses) &&
                    ProgressionJsonInt(entry, "previous_wins", 0, INT_MAX, &node->previous_wins);
        }
        for (int event = 0; event < NEXUS_MAX_EVENTS && valid; event++) {
            cJSON *tiers = cJSON_GetArrayItem(event_completions, event);
            valid = cJSON_IsArray(tiers) && cJSON_GetArraySize(tiers) == NEXUS_MAX_DIFFICULTIES;
            for (int difficulty = 0; difficulty < NEXUS_MAX_DIFFICULTIES && valid; difficulty++)
                valid = ProgressionJsonArrayInt(
                    tiers, difficulty, 0, INT_MAX, &loaded_events.completions[event][difficulty]
                );
            uint64_t stamp = 0;
            valid = valid &&
                    ProgressionJsonItemU64(
                        cJSON_GetArrayItem(unlocked_at, event), 0, PROGRESSION_JSON_MAX_SAFE_INTEGER, &stamp
                    ) &&
                    ProgressionJsonArrayInt(remaining_runs, event, 0, INT_MAX, &loaded_events.remaining_runs[event]);
            loaded_events.unlocked_at[event] = (long long)stamp;
        }
    }

    PlayerRunes *loaded_runes = NULL;
    if (valid && version >= 5) {
        loaded_runes = (PlayerRunes *)malloc(sizeof(PlayerRunes));
        valid = loaded_runes && ProgressionLoadRunes(cJSON_GetObjectItemCaseSensitive(root, "runes"), loaded_runes);
    }

    static PlayerMissions loaded_missions;
    ProgressionLoadMissions(cJSON_GetObjectItemCaseSensitive(root, "missions"), &loaded_missions);

    PlayerExpeditions loaded_expeditions;
    ExpeditionsLoad(cJSON_GetObjectItemCaseSensitive(root, "expeditions"), &loaded_expeditions);

    static PlayerTimedQuests loaded_timed_quests;
    TimedQuestsLoad(cJSON_GetObjectItemCaseSensitive(root, "timed_quests"), &loaded_timed_quests);

    PlayerGallery loaded_gallery = {0};
    if (valid && version >= 6)
        valid = ProgressionLoadGallery(cJSON_GetObjectItemCaseSensitive(root, "gallery"), &loaded_gallery);

    static PlayerPvp loaded_pvp;
    memset(&loaded_pvp, 0, sizeof(loaded_pvp));
    if (valid && version >= PROGRESSION_FIRST_PVP_VERSION)
        valid = ProgressionLoadPvp(cJSON_GetObjectItemCaseSensitive(root, "pvp"), &loaded_pvp);

    char loaded_name[PLAYER_NAME_CAPACITY] = "";
    cJSON *name = cJSON_GetObjectItemCaseSensitive(account, "name");
    if (valid && name) {
        valid = cJSON_IsString(name) && PlayerNameValid(name->valuestring);
        if (valid) StrCopy(loaded_name, sizeof(loaded_name), name->valuestring);
    }
    PlayerTutorial loaded_tutorial = {.main_complete = true, .consume_complete = true};
    cJSON *tutorial = cJSON_GetObjectItemCaseSensitive(root, "tutorial");
    if (valid && tutorial) {
        cJSON *complete = cJSON_GetObjectItemCaseSensitive(tutorial, "complete");
        cJSON *consume_complete = cJSON_GetObjectItemCaseSensitive(tutorial, "consume_complete");
        valid = cJSON_IsBool(complete) && cJSON_IsBool(consume_complete) &&
                ProgressionJsonInt(tutorial, "restore_step", 0, INT_MAX, &loaded_tutorial.restore_step);
        loaded_tutorial.main_complete = cJSON_IsTrue(complete);
        loaded_tutorial.consume_complete = cJSON_IsTrue(consume_complete);
    }

    bool loaded_fast_battles = JsonBool(cJSON_GetObjectItemCaseSensitive(root, "settings"), "fast_battles");

    cJSON *loaded_gear = cJSON_DetachItemFromObjectCaseSensitive(root, "gear");
    cJSON_Delete(root);
    if (!valid) {
        cJSON_Delete(loaded_gear);
        GalleryFree(&loaded_gallery);
        free(loaded_runes);
        free(loaded_additions);
        TraceLog(LOG_WARNING, "Progression: rejected incompatible or corrupt save %s", origin);
        return false;
    }

    player->level = level;
    player->experience = experience;
    player->experience_min = experience_min;
    player->experience_max = experience_max;
    player->energy = energy;
    player->energy_max = energy_max;
    player->stamina = stamina;
    player->stamina_max = stamina_max;
    player->energy_regen_timer = energy == energy_max ? 0.0f : energy_timer;
    player->stamina_regen_timer = stamina == stamina_max ? 0.0f : stamina_timer;
    player->gold = gold;
    player->gems = gems;
    StrCopy(player->name, sizeof(player->name), loaded_name);
    player->tutorial = loaded_tutorial;
    player->fast_battles = loaded_fast_battles;
    player->campaign_progress = loaded_progress;
    if (version >= 4) {
        player->potions = loaded_potions;
        player->potion_max = loaded_potion_max;
        player->news_seen_at = loaded_news_seen_at;
        memcpy(player->keys, loaded_keys, sizeof(loaded_keys));
        player->nexus_run = loaded_run;
        player->nexus_events = loaded_events;
    }
    if (version >= 3) {
        player->store_rng_state = loaded_rng;
        player->store_opened_total = loaded_opened_total;
        memcpy(player->store_opened_by_pack, loaded_opened, sizeof(loaded_opened));
        player->champion_since_legendary = loaded_champion_since;
        memcpy(player->pack_vouchers, loaded_vouchers, sizeof(loaded_vouchers));
        if (commit_roster) {
            int old_count = player->roster_count;
            int new_count = saved_starter_count + addition_count;
            PlayerRosterClearRange(player, saved_starter_count, old_count);
            for (int n = 0; n < addition_count; n++) {
                int i = saved_starter_count + n;
                player->roster[i] = loaded_additions[n].character;
                player->roster_abilities[i] = loaded_additions[n].ability;
                player->roster[i].ability = &player->roster_abilities[i];
            }
            player->roster_count = new_count;
            player->starter_roster_count = saved_starter_count;
            for (int i = 0; i < new_count; i++) ConsumeNormalizeCharacter(&player->roster[i]);
            player->next_instance_id = loaded_next_instance_id;
            memcpy(player->squads, loaded_squads, sizeof(loaded_squads));
            player->current_squad = loaded_current_squad;
        }
    }
    if (player->roster) ProgressionLoadGear(loaded_gear, player);
    cJSON_Delete(loaded_gear);
    if (loaded_runes) player->runes = *loaded_runes;
    GalleryFree(&player->gallery);
    player->gallery = loaded_gallery;
    player->missions = loaded_missions;
    player->expeditions = loaded_expeditions;
    player->timed_quests = loaded_timed_quests;
    player->pvp = loaded_pvp;
    PvpHistorySanitize(&player->pvp);
    free(loaded_runes);
    free(loaded_additions);
    return true;
}

bool ProgressionLoadFromPath(Player *player, const char *path) {
    if (!player || !path || !path[0] || !FileExists(path)) return false;
    char *text = LoadFileText(path);
    if (!text) return false;
    bool ok = ProgressionLoadFromText(player, text, path);
    UnloadFileText(text);
    return ok;
}

typedef enum {
    PROGRESSION_LOAD_RESTORED,
    PROGRESSION_LOAD_NEW_ACCOUNT,
    PROGRESSION_LOAD_UNAVAILABLE,
} ProgressionLoadResult;

static ProgressionLoadResult ProgressionLoadFromBackend(Player *player) {
    char *text = NULL;
    long long revision = 0;
    BackendProfileFetch fetched = BackendFetchProfile(&text, &revision);
    if (fetched == BACKEND_PROFILE_MISSING) return PROGRESSION_LOAD_NEW_ACCOUNT;
    if (fetched != BACKEND_PROFILE_FOUND) return PROGRESSION_LOAD_UNAVAILABLE;
    bool ok = ProgressionLoadFromText(player, text, "from backend");
    free(text);
    if (ok) BackendAcceptProfile(revision);
    return ok ? PROGRESSION_LOAD_RESTORED : PROGRESSION_LOAD_UNAVAILABLE;
}

static bool ProgressionSaveToBackend(const Player *player) {
    char *serialized = ProgressionSerialize(player);
    if (!serialized) return false;
    bool ok = ServerActionsStoreProfile(serialized);
    cJSON_free(serialized);
    return ok;
}

ProgressionLoadResult ProgressionLoad(Player *player) {
    if (!ProgressionPersistenceEnabled()) return PROGRESSION_LOAD_UNAVAILABLE;
    if (BackendEnabled()) return ProgressionLoadFromBackend(player);
    const char *path = ProgressionSavePath();
    if (!FileExists(path)) return PROGRESSION_LOAD_NEW_ACCOUNT;
    return ProgressionLoadFromPath(player, path) ? PROGRESSION_LOAD_RESTORED : PROGRESSION_LOAD_UNAVAILABLE;
}

bool ProgressionAdoptServerProfile(Player *player, const cJSON *response) {
    const cJSON *profile = cJSON_GetObjectItemCaseSensitive(response, "profile");
    char *text = cJSON_IsObject(profile) ? cJSON_PrintUnformatted(profile) : NULL;
    bool ok = text && ProgressionLoadFromText(player, text, "from a server action");
    cJSON_free(text);
    if (!ok) TraceLog(LOG_WARNING, "Progression: the server applied an action but its profile did not load");
    return ok;
}

bool ProgressionNeedsServer(void) {
    return ProgressionPersistenceEnabled() && BackendEnabled();
}

bool ProgressionSave(const Player *player) {
    if (!ProgressionPersistenceEnabled()) return true;
    if (BackendEnabled()) return ProgressionSaveToBackend(player);
    return ProgressionSaveToPath(player, ProgressionSavePath());
}
