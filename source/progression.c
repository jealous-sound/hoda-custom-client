#include <limits.h>

#define PROGRESSION_SAVE_VERSION 6
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
    p->champion_since_legendary = 0;
    p->campaign_progress = (PlayerCampaignProgress){0};
    p->potions = 3;
    p->potion_max = 10;
    memset(p->keys, 0, sizeof(p->keys));
    p->nexus_run = (PlayerNexusRun){0};
    p->nexus_events = (PlayerNexusEvents){0};
}

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

static void BattleRewardSnapshotPre(BattleRewardResult *result, const Player *player) {
    result->player_level_pre = player->level;
    result->player_xp_pre = player->experience;
    result->energy_max_pre = player->energy_max;
}

static void BattleRewardSnapshotPost(BattleRewardResult *result, const Player *player) {
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

static void ProgressionAwardBattleXp(Player *player, int xp) {
    int level_pre = player->level;
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
        BattleRewardSnapshotPost(&result, player);
        return result;
    }

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
    return root;
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

static void NexusGrantEscrowUnits(Player *player, const int *escrow_units) {
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
    if (count > 0 && !PlayerRosterAppendDefinitions(player, definitions, count, NULL))
        TraceLog(LOG_WARNING, "Nexus: could not grant %d escrowed unit(s)", count);
    cJSON_Delete(catalog_root);
}

void NexusAwardEscrow(Player *player) {
    if (!player || !player->nexus_run.committed) return;
    PlayerNexusRun *run = &player->nexus_run;
    player->gold = ProgressionSaturatingAdd(player->gold, run->escrow_gold);
    PlayerAddPotions(player, run->escrow_potions);
    NexusGrantEscrowUnits(player, run->escrow_units);

    int event = run->event_index, difficulty = run->difficulty_index;
    if (event >= 0 && event < NEXUS_MAX_EVENTS && difficulty >= 0 && difficulty < NEXUS_MAX_DIFFICULTIES &&
        player->nexus_events.completions[event][difficulty] < INT_MAX)
        player->nexus_events.completions[event][difficulty]++;
    NexusResetRun(player);
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
    player->nexus_events.unlocked_at[event] = (long long)time(NULL);
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

bool ProgressionSaveToPath(const Player *player, const char *path) {
    if (!player || !path || !path[0]) return false;
    bool managed_roster = player->roster && player->roster_abilities && player->roster_capacity > 0;
    if (!ProgressionRosterStateValid(player)) return false;
    if (player->store_opened_total < 0 || player->champion_since_legendary < 0) return false;
    int opened_sum = 0;
    for (int pack = 0; pack < PLAYER_STORE_PACK_COUNT; pack++) {
        if (player->store_opened_by_pack[pack] < 0 || opened_sum > INT_MAX - player->store_opened_by_pack[pack])
            return false;
        opened_sum += player->store_opened_by_pack[pack];
    }
    if (opened_sum != player->store_opened_total || player->champion_since_legendary > opened_sum) return false;

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

    PROGRESSION_SAVE_NUMBER(store, "rng_state", player->store_rng_state);
    PROGRESSION_SAVE_NUMBER(store, "opened_total", player->store_opened_total);
    for (int pack = 0; pack < PLAYER_STORE_PACK_COUNT; pack++)
        if (!ProgressionJsonAddArrayNumber(opened, player->store_opened_by_pack[pack])) goto save_json_failed;
    PROGRESSION_SAVE_NUMBER(store, "champion_since_legendary", player->champion_since_legendary);

    int starter_count = managed_roster ? player->starter_roster_count : 0;
    int roster_count = managed_roster ? player->roster_count : 0;
    uint64_t next_instance_id = managed_roster ? player->next_instance_id : 1;
    PROGRESSION_SAVE_NUMBER(roster, "starter_count", starter_count);
    PROGRESSION_SAVE_NUMBER(roster, "next_instance_id", (double)next_instance_id);
    for (int i = 0; i < starter_count; i++) {
        cJSON *entry = cJSON_CreateObject();
        if (!entry || !cJSON_AddStringToObject(entry, "id", player->roster[i].id) ||
            !cJSON_AddNumberToObject(entry, "instance_id", (double)player->roster[i].instance_id)) {
            cJSON_Delete(entry);
            goto save_json_failed;
        }
        if (!ProgressionJsonAddArrayItem(starters, entry)) goto save_json_failed;
    }
    for (int i = starter_count; i < roster_count; i++) {
        const Character *ch = &player->roster[i];
        cJSON *entry = cJSON_CreateObject();
        if (!entry || !cJSON_AddStringToObject(entry, "id", ch->id) ||
            !cJSON_AddNumberToObject(entry, "instance_id", (double)ch->instance_id) ||
            !cJSON_AddStringToObject(entry, "tier", TIER_NAMES[ch->tier]) ||
            !cJSON_AddNumberToObject(entry, "level", ch->level)) {
            cJSON_Delete(entry);
            goto save_json_failed;
        }
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

    const PlayerNexusRun *nexus_run = &player->nexus_run;
    PROGRESSION_SAVE_NUMBER(nexus, "potions", player->potions);
    PROGRESSION_SAVE_NUMBER(nexus, "potion_max", player->potion_max);
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

    cJSON *runes = ProgressionSaveRunes(&player->runes);
    if (!runes || !cJSON_AddItemToObject(root, "runes", runes)) {
        cJSON_Delete(runes);
        goto save_json_failed;
    }

    cJSON *gallery = ProgressionSaveGallery(&player->gallery);
    if (!gallery || !cJSON_AddItemToObject(root, "gallery", gallery)) {
        cJSON_Delete(gallery);
        goto save_json_failed;
    }

    serialized = cJSON_Print(root);
    cJSON_Delete(root);
    root = NULL;
    if (!serialized) goto save_json_failed;

#undef PROGRESSION_SAVE_NUMBER

    char temporary[1024];
    int needed = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    bool ok = needed > 0 && needed < (int)sizeof(temporary) && SaveFileText(temporary, serialized);
    cJSON_free(serialized);
    if (ok) ok = rename(temporary, path) == 0;
    if (!ok && needed > 0 && needed < (int)sizeof(temporary)) remove(temporary);
    if (!ok) TraceLog(LOG_WARNING, "Progression: could not save %s", path);
    return ok;

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
    return false;
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

bool ProgressionLoadFromPath(Player *player, const char *path) {
    if (!player || !path || !path[0] || !FileExists(path)) return false;
    char *text = LoadFileText(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        TraceLog(LOG_WARNING, "Progression: invalid save %s", path);
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
            valid = ProgressionRosterStateValid(player) && saved_starter_count == player->starter_roster_count &&
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
            int level_value = 0;
            uint64_t instance_id = 0;
            valid = valid && tier_name[0] && strcmp(tier_name, TIER_NAMES[tier]) == 0 && ch->has_tier[tier] &&
                    ProgressionJsonInt(entry, "level", 1, ch->stats[tier].max_level, &level_value) &&
                    ProgressionJsonU64(entry, "instance_id", 1, PROGRESSION_JSON_MAX_SAFE_INTEGER, &instance_id);
            for (int starter = 0; starter < saved_starter_count && valid; starter++)
                valid = player->roster[starter].instance_id != instance_id;
            for (int previous = 0; previous < n && valid; previous++)
                valid = loaded_additions[previous].character.instance_id != instance_id;
            if (valid) {
                ch->tier = tier;
                ch->level = level_value;
                ch->instance_id = instance_id;
                ch->ability = ability;
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

    PlayerGallery loaded_gallery = {0};
    if (valid && version >= 6)
        valid = ProgressionLoadGallery(cJSON_GetObjectItemCaseSensitive(root, "gallery"), &loaded_gallery);

    cJSON_Delete(root);
    if (!valid) {
        GalleryFree(&loaded_gallery);
        free(loaded_runes);
        free(loaded_additions);
        TraceLog(LOG_WARNING, "Progression: rejected incompatible or corrupt save %s", path);
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
    player->campaign_progress = loaded_progress;
    if (version >= 4) {
        player->potions = loaded_potions;
        player->potion_max = loaded_potion_max;
        memcpy(player->keys, loaded_keys, sizeof(loaded_keys));
        player->nexus_run = loaded_run;
        player->nexus_events = loaded_events;
    }
    if (version >= 3) {
        player->store_rng_state = loaded_rng;
        player->store_opened_total = loaded_opened_total;
        memcpy(player->store_opened_by_pack, loaded_opened, sizeof(loaded_opened));
        player->champion_since_legendary = loaded_champion_since;
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
            player->next_instance_id = loaded_next_instance_id;
            memcpy(player->squads, loaded_squads, sizeof(loaded_squads));
            player->current_squad = loaded_current_squad;
        }
    }
    if (loaded_runes) player->runes = *loaded_runes;
    GalleryFree(&player->gallery);
    player->gallery = loaded_gallery;
    free(loaded_runes);
    free(loaded_additions);
    return true;
}

bool ProgressionLoad(Player *player) {
    if (!ProgressionPersistenceEnabled()) return false;
    return ProgressionLoadFromPath(player, ProgressionSavePath());
}

bool ProgressionSave(const Player *player) {
    if (!ProgressionPersistenceEnabled()) return true;
    return ProgressionSaveToPath(player, ProgressionSavePath());
}
