#define PVP_RULES_PATH "data/pvp_rules.json"
#define PVP_CATALOG_PATH HOD_CACHE "characters.json"
#define PVP_BOT_CAPACITY 256
#define PVP_SQUAD_CAPACITY 1024
#define PVP_NAME_CAP 32
#define PVP_BOT_NAME_FORMAT "[BOT] %s"
#define PVP_UNIT_ID_CAP 48
#define PVP_LOCAL_MAX_LEVEL 100
#define PVP_STAMINA_COST 1
#define PVP_LEVEL_SALT 0x51ED27u
#define PVP_BOT_SALT 0x7A3C11u
#define PVP_SQUAD_SALT 0x2F90D3u
#define PVP_DRIFT_SALT 0x1C8A77u
#define PVP_DEFENSE_SALT 0x3D6F02u
#define PVP_DEFENSE_SQUAD 0

#define PVP_EVENT_CAPACITY 16
#define PVP_GUILD_EVENT_CAPACITY 8
#define PVP_DUEL_ID_CAP 48
#define PVP_EVENT_REWARD_CAPACITY 16
#define PVP_EVENT_KEY_CAP 96
#define PVP_EVENT_SKILL_JITTER_PCT 25
#define PVP_EVENT_SALT 0x4E11A9u
#define PVP_REWARD_PRIZE_CAPACITY 6
#define PVP_PAYOUT_PRIZE_CAPACITY 16
#define PVP_PRIZE_TEXTURE_CAPACITY 256
#define PVP_RULE_SIDE_CAPACITY 4
#define PVP_RULE_NAME_CAPACITY 8
#define PVP_RULE_NAME_CAP 24
#define PVP_RULE_ANY_SIDE (-1)
#define PVP_RULE_NO_SLOT (-1)

typedef struct {
    char names[PVP_RULE_NAME_CAPACITY][PVP_RULE_NAME_CAP];
    bool faction[PVP_RULE_NAME_CAPACITY];
    int count;
    unsigned faction_mask;
} PvpHeroFilter;

typedef struct {
    bool active;
    Rarity max_rarity;
    unsigned rarity_mask;
    bool unique_heroes;
    PvpHeroFilter members;
    PvpHeroFilter sides[PVP_RULE_SIDE_CAPACITY];
    int side_count;
} PvpSquadRules;

typedef enum {
    PVP_SQUAD_ALLOWED,
    PVP_SQUAD_EMPTY,
    PVP_SQUAD_RARITY,
    PVP_SQUAD_MEMBER,
    PVP_SQUAD_DUPLICATE,
    PVP_SQUAD_SIDE,
} PvpSquadVerdictKind;

typedef struct {
    PvpSquadVerdictKind kind;
    int slot;
    int side;
} PvpSquadVerdict;

typedef struct {
    int squads[PVP_SQUAD_CAPACITY];
    int count;
} PvpSquadList;

typedef enum {
    PVP_PRIZE_UNIT,
    PVP_PRIZE_GEAR,
    PVP_PRIZE_VOUCHER,
    PVP_PRIZE_TOME,
} PvpPrizeKind;

typedef struct {
    PvpPrizeKind kind;
    char id[48];
    char icon[72];
    int count;
    int index;
} PvpPrize;

typedef struct {
    int gold;
    int gems;
    int prize_count;
    PvpPrize prizes[PVP_REWARD_PRIZE_CAPACITY];
} PvpReward;

typedef struct {
    int gold;
    int gems;
    int prize_count;
    const PvpPrize *prizes[PVP_PAYOUT_PRIZE_CAPACITY];
} PvpPayout;

typedef struct {
    int banners;
    PvpReward reward;
} PvpEventMilestone;

typedef struct {
    int first;
    int last;
    PvpReward reward;
} PvpEventRankReward;

typedef struct {
    char title_key[PVP_EVENT_KEY_CAP];
    BattleGameChanger changer;
    PvpSquadRules rules;
    PvpSquadList pools[PVP_RULE_SIDE_CAPACITY];
    PvpEventMilestone milestones[PVP_EVENT_REWARD_CAPACITY];
    int milestone_count;
    PvpEventRankReward rank_rewards[PVP_EVENT_REWARD_CAPACITY];
    int rank_reward_count;
    bool guild;
    int guild_minimum_banners;
    PvpEventRankReward guild_rank_rewards[PVP_EVENT_REWARD_CAPACITY];
    int guild_rank_reward_count;
} PvpEvent;

typedef struct {
    int duration_seconds;
    int win_banners_start, win_banners_peak_bonus, win_banners_steps_to_peak;
    int win_banners_decline_slowdown, win_banners_floor_pct;
    int matchmaking_banner_window;
    int loss_banners, top_bot_banners;
    int ladder_banners;
    PvpEvent catalog[PVP_EVENT_CAPACITY];
    int catalog_count;
    long long guild_every_cycles;
    PvpEvent guild_catalog[PVP_GUILD_EVENT_CAPACITY];
    int guild_catalog_count;
} PvpEventRules;

typedef struct {
    int base_gain, base_loss, points_per_step;
    int min_gain, max_gain, min_loss, max_loss;
    int window_below, window_above, daily_nexts, bot_daily_drift, level_spread;
    int squad_candidates;
    int win_gold_base, win_gold_per_level, loss_gold_pct;
    int win_xp_base, win_xp_per_level, loss_xp_pct;
    int attack_interval_seconds, max_attacks_per_login, defense_loss_share_pct;
    bool stat_budget;
} PvpRules;

typedef struct {
    char name[PVP_NAME_CAP];
    char avatar[PVP_UNIT_ID_CAP];
    int trophies;
} PvpBot;

typedef struct {
    char id[PVP_UNIT_ID_CAP];
    Tier tier;
    int level;
    bool present;
    bool ascended;
    short gear_defs[GEAR_SLOT_CAPACITY];
} PvpSquadCell;

typedef struct {
    int strength;
    bool events_only;
    bool elite;
    PvpSquadCell cells[SQUAD_SLOT_COUNT];
    int rune_defs[RUNE_SLOT_COUNT];
    int rune_count;
} PvpSquad;

typedef struct {
    PvpRules rules;
    PvpEventRules events;
    PvpBot bots[PVP_BOT_CAPACITY];
    int bot_count;
    PvpSquad squads[PVP_SQUAD_CAPACITY];
    PvpSquadList ladder;
    int squad_count;
    bool loaded;
} PvpTable;

typedef struct {
    int bot;
    int squad;
    int level;
    int trophies;
    int gain;
    int loss;
    unsigned seed;
    int rune_defs[RUNE_SLOT_COUNT];
    int rune_count;
    RenownBattleBonus renown;
    bool duel;
    char duel_id[PVP_DUEL_ID_CAP];
    char duel_name[PLAYER_NAME_CAPACITY];
    int duel_renown_rank;
    int duel_vault_tier;
    PvpSquadCell duel_cells[SQUAD_SLOT_COUNT];
    bool duel_hero_builds;
    BattleHeroBuild duel_builds[SQUAD_SLOT_COUNT];
} PvpOpponent;

#define PVP_RANK_PLAYER (-1)
#define PVP_RANK_RIVAL (-2)

typedef struct {
    int bot;
    int trophies;
    int rival;
} PvpRankEntry;

static PvpTable pvp_table;

static int PvpSectionInt(const cJSON *root, const char *section, const char *key) {
    const cJSON *object = cJSON_GetObjectItemCaseSensitive((cJSON *)root, section);
    const cJSON *value = cJSON_GetObjectItemCaseSensitive((cJSON *)object, key);
    if (!cJSON_IsNumber(value)) TraceLog(LOG_FATAL, "Pvp: rule %s.%s is missing", section, key);
    return value->valueint;
}

static void PvpRulesParse(PvpRules *r, const cJSON *root) {
    r->base_gain = PvpSectionInt(root, "trophies", "base_gain");
    r->base_loss = PvpSectionInt(root, "trophies", "base_loss");
    r->points_per_step = PvpSectionInt(root, "trophies", "points_per_step");
    r->min_gain = PvpSectionInt(root, "trophies", "min_gain");
    r->max_gain = PvpSectionInt(root, "trophies", "max_gain");
    r->min_loss = PvpSectionInt(root, "trophies", "min_loss");
    r->max_loss = PvpSectionInt(root, "trophies", "max_loss");
    r->window_below = PvpSectionInt(root, "matchmaking", "window_below");
    r->window_above = PvpSectionInt(root, "matchmaking", "window_above");
    r->daily_nexts = PvpSectionInt(root, "matchmaking", "daily_nexts");
    r->bot_daily_drift = PvpSectionInt(root, "matchmaking", "bot_daily_drift");
    r->level_spread = PvpSectionInt(root, "matchmaking", "level_spread");
    r->squad_candidates = PvpSectionInt(root, "squads", "candidates");
    r->win_gold_base = PvpSectionInt(root, "rewards", "win_gold_base");
    r->win_gold_per_level = PvpSectionInt(root, "rewards", "win_gold_per_level");
    r->loss_gold_pct = PvpSectionInt(root, "rewards", "loss_gold_pct");
    r->win_xp_base = PvpSectionInt(root, "rewards", "win_xp_base");
    r->win_xp_per_level = PvpSectionInt(root, "rewards", "win_xp_per_level");
    r->loss_xp_pct = PvpSectionInt(root, "rewards", "loss_xp_pct");
    r->attack_interval_seconds = PvpSectionInt(root, "defense", "attack_interval_seconds");
    r->max_attacks_per_login = PvpSectionInt(root, "defense", "max_attacks_per_login");
    r->defense_loss_share_pct = PvpSectionInt(root, "defense", "loss_share_pct");
    r->stat_budget = JsonBool(cJSON_GetObjectItemCaseSensitive((cJSON *)root, "battle"), "stat_budget");
}

static void BattleChangerEffectLoad(BattleChangerEffect *effect, const cJSON *json) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(json, "groups")) {
        if (effect->group_count == BATTLE_CHANGER_MAX_GROUPS) TraceLog(LOG_FATAL, "Battle: too many changer groups");
        StrCopy(effect->groups[effect->group_count++], sizeof(effect->groups[0]), item->valuestring);
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(json, "units")) {
        if (effect->unit_count == BATTLE_CHANGER_MAX_UNITS) TraceLog(LOG_FATAL, "Battle: too many changer units");
        StrCopy(effect->units[effect->unit_count++], sizeof(effect->units[0]), item->valuestring);
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(json, "factions")) {
        effect->faction_mask |= SimFactionMask(FactionFromString(item->valuestring));
    }
    effect->health_pct = JsonInt(json, "health_pct");
    effect->power_pct = JsonInt(json, "power_pct");
}

static PvpPrizeKind PvpPrizeKindParse(const char *kind) {
    if (!strcmp(kind, "unit")) return PVP_PRIZE_UNIT;
    if (!strcmp(kind, "gear")) return PVP_PRIZE_GEAR;
    if (!strcmp(kind, "voucher")) return PVP_PRIZE_VOUCHER;
    if (!strcmp(kind, "tome")) return PVP_PRIZE_TOME;
    TraceLog(LOG_FATAL, "Pvp: unknown prize kind %s", kind);
    return PVP_PRIZE_UNIT;
}

static PvpPrize PvpPrizeParse(const cJSON *json) {
    PvpPrize prize = {.kind = PvpPrizeKindParse(JsonStr(json, "kind")), .count = JsonInt(json, "count")};
    StrCopy(prize.id, sizeof(prize.id), JsonStr(json, "id"));
    StrCopy(prize.icon, sizeof(prize.icon), JsonStr(json, "icon"));
    prize.index = prize.kind == PVP_PRIZE_GEAR ? GearDefIndex(prize.id) : JsonInt(json, "index");
    if (prize.index < 0) TraceLog(LOG_FATAL, "Pvp: unknown prize %s", prize.id);
    return prize;
}

static PvpReward PvpRewardParse(const cJSON *json) {
    PvpReward reward = {.gold = JsonInt(json, "gold"), .gems = JsonInt(json, "gems")};
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "prizes")) {
        if (reward.prize_count == PVP_REWARD_PRIZE_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many prizes");
        reward.prizes[reward.prize_count++] = PvpPrizeParse(item);
    }
    return reward;
}

static void PvpRankRewardsParse(PvpEventRankReward *out, int *count, const cJSON *json) {
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        if (*count == PVP_EVENT_REWARD_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many rank rewards");
        out[(*count)++] = (PvpEventRankReward){JsonInt(item, "first"), JsonInt(item, "last"), PvpRewardParse(item)};
    }
}

static void PvpEventLadderParse(PvpEvent *event, const cJSON *json) {
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "milestones")) {
        if (event->milestone_count == PVP_EVENT_REWARD_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many milestones");
        event->milestones[event->milestone_count++] =
            (PvpEventMilestone){JsonInt(item, "banners"), PvpRewardParse(item)};
    }
    PvpRankRewardsParse(
        event->rank_rewards, &event->rank_reward_count, cJSON_GetObjectItemCaseSensitive(json, "rank_rewards")
    );
    PvpRankRewardsParse(
        event->guild_rank_rewards,
        &event->guild_rank_reward_count,
        cJSON_GetObjectItemCaseSensitive(json, "guild_rank_rewards")
    );
    event->guild_minimum_banners = JsonInt(json, "guild_minimum_banners");
}

static void PvpHeroFilterAdd(PvpHeroFilter *filter, const char *name, bool faction) {
    if (filter->count == PVP_RULE_NAME_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many names in a squad rule");
    StrCopy(filter->names[filter->count], PVP_RULE_NAME_CAP, name);
    filter->faction[filter->count++] = faction;
    if (faction) filter->faction_mask |= SimFactionMask(FactionFromString(name));
}

static void PvpHeroFilterParse(PvpHeroFilter *filter, const cJSON *json) {
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "factions")) {
        PvpHeroFilterAdd(filter, item->valuestring, true);
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "groups")) {
        PvpHeroFilterAdd(filter, item->valuestring, false);
    }
}

static unsigned PvpRarityBit(Rarity rarity) {
    return 1u << rarity;
}

static void PvpSquadRulesParse(PvpSquadRules *rules, const cJSON *json) {
    if (!cJSON_IsObject(json)) return;
    rules->active = true;
    const char *max_rarity = JsonStr(json, "max_rarity");
    rules->max_rarity = max_rarity[0] ? RarityFromString(max_rarity) : RARITY_MYTHIC;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "rarities")) {
        rules->rarity_mask |= PvpRarityBit(RarityFromString(item->valuestring));
    }
    rules->unique_heroes = JsonBool(json, "unique_heroes");
    PvpHeroFilterParse(&rules->members, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "members"));
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "sides")) {
        if (rules->side_count == PVP_RULE_SIDE_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many sides in a squad rule");
        PvpHeroFilterParse(&rules->sides[rules->side_count++], item);
    }
}

static bool PvpHeroFilterPasses(const PvpHeroFilter *filter, const Character *ch) {
    if (filter->count == 0) return true;
    if (filter->faction_mask && (ch->giant || (SimFactionMask(ch->faction) & filter->faction_mask))) return true;
    for (int i = 0; i < filter->count; i++)
        if (!filter->faction[i] && SimPassiveHasGroup(ch, filter->names[i])) return true;
    return false;
}

static bool PvpRarityAllowed(const PvpSquadRules *rules, const Character *ch) {
    Rarity shown = CharacterShownRarity(ch);
    if (shown > rules->max_rarity) return false;
    return !rules->rarity_mask || (rules->rarity_mask & PvpRarityBit(shown));
}

static int PvpSquadSideMisfit(const PvpSquadRules *rules, const Character *const heroes[SQUAD_SLOT_COUNT], int side) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (heroes[slot] && !PvpHeroFilterPasses(&rules->sides[side], heroes[slot])) return slot;
    return PVP_RULE_NO_SLOT;
}

static int PvpSquadSideFitCount(const PvpSquadRules *rules, const Character *const heroes[SQUAD_SLOT_COUNT], int side) {
    int fitting = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        fitting += heroes[slot] && PvpHeroFilterPasses(&rules->sides[side], heroes[slot]);
    return fitting;
}

static PvpSquadVerdict
PvpSquadJudgeSide(const PvpSquadRules *rules, const Character *const heroes[SQUAD_SLOT_COUNT], int side) {
    if (side != PVP_RULE_ANY_SIDE) {
        int misfit = side >= 0 && side < rules->side_count ? PvpSquadSideMisfit(rules, heroes, side) : 0;
        return (PvpSquadVerdict){misfit == PVP_RULE_NO_SLOT ? PVP_SQUAD_ALLOWED : PVP_SQUAD_SIDE, misfit, side};
    }
    int closest = 0;
    for (int candidate = 0; candidate < rules->side_count; candidate++) {
        if (PvpSquadSideMisfit(rules, heroes, candidate) == PVP_RULE_NO_SLOT)
            return (PvpSquadVerdict){PVP_SQUAD_ALLOWED, PVP_RULE_NO_SLOT, candidate};
        if (PvpSquadSideFitCount(rules, heroes, candidate) > PvpSquadSideFitCount(rules, heroes, closest))
            closest = candidate;
    }
    return (PvpSquadVerdict){PVP_SQUAD_SIDE, PvpSquadSideMisfit(rules, heroes, closest), closest};
}

PvpSquadVerdict
PvpSquadRulesJudge(const PvpSquadRules *rules, const Character *const heroes[SQUAD_SLOT_COUNT], int side) {
    int fielded = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const Character *ch = heroes[slot];
        if (!ch) continue;
        fielded++;
        if (!rules->active) continue;
        if (!PvpRarityAllowed(rules, ch)) return (PvpSquadVerdict){PVP_SQUAD_RARITY, slot, 0};
        if (!PvpHeroFilterPasses(&rules->members, ch)) return (PvpSquadVerdict){PVP_SQUAD_MEMBER, slot, 0};
        for (int earlier = 0; rules->unique_heroes && earlier < slot; earlier++)
            if (heroes[earlier] && !strcmp(heroes[earlier]->id, ch->id))
                return (PvpSquadVerdict){PVP_SQUAD_DUPLICATE, slot, 0};
    }
    if (fielded == 0) return (PvpSquadVerdict){PVP_SQUAD_EMPTY, PVP_RULE_NO_SLOT, 0};
    if (!rules->active || rules->side_count == 0) return (PvpSquadVerdict){PVP_SQUAD_ALLOWED, PVP_RULE_NO_SLOT, 0};
    return PvpSquadJudgeSide(rules, heroes, side);
}

static void PvpEventParse(PvpEvent *event, const cJSON *item, bool guild) {
    StrCopy(event->title_key, sizeof(event->title_key), JsonStr(item, "title_key"));
    PvpEventLadderParse(event, item);
    PvpSquadRulesParse(&event->rules, cJSON_GetObjectItemCaseSensitive((cJSON *)item, "squad_rules"));
    event->guild = guild;
    StrCopy(event->changer.text_key, sizeof(event->changer.text_key), JsonStr(item, "text_key"));
    const cJSON *effect = NULL;
    cJSON_ArrayForEach(effect, cJSON_GetObjectItemCaseSensitive((cJSON *)item, "effects")) {
        if (event->changer.effect_count == BATTLE_CHANGER_MAX_EFFECTS)
            TraceLog(LOG_FATAL, "Pvp: too many event effects");
        BattleChangerEffectLoad(&event->changer.effects[event->changer.effect_count++], effect);
    }
}

static void PvpEventsParse(PvpEventRules *e, const cJSON *root) {
    const cJSON *json = cJSON_GetObjectItemCaseSensitive((cJSON *)root, "events");
    e->duration_seconds = JsonInt(json, "duration_seconds");
    e->win_banners_start = JsonInt(json, "win_banners_start");
    e->win_banners_peak_bonus = JsonInt(json, "win_banners_peak_bonus");
    e->win_banners_steps_to_peak = JsonInt(json, "win_banners_steps_to_peak");
    e->win_banners_decline_slowdown = JsonInt(json, "win_banners_decline_slowdown");
    e->win_banners_floor_pct = JsonInt(json, "win_banners_floor_pct");
    e->matchmaking_banner_window = JsonInt(json, "matchmaking_banner_window");
    e->loss_banners = JsonInt(json, "loss_banners");
    e->top_bot_banners = JsonInt(json, "top_bot_banners");
    e->ladder_banners = JsonInt(json, "ladder_banners");
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "catalog")) {
        if (e->catalog_count == PVP_EVENT_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many events");
        PvpEventParse(&e->catalog[e->catalog_count++], item, false);
    }
    e->guild_every_cycles = JsonInt(json, "guild_every_cycles");
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "guild_catalog")) {
        if (e->guild_catalog_count == PVP_GUILD_EVENT_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many guild events");
        PvpEventParse(&e->guild_catalog[e->guild_catalog_count++], item, true);
    }
    if (e->catalog_count == 0 || e->duration_seconds <= 0) TraceLog(LOG_FATAL, "Pvp: no events");
    if (e->guild_catalog_count > 0 && e->guild_every_cycles <= 0) TraceLog(LOG_FATAL, "Pvp: no guild event cadence");
}

static void PvpBotsParse(PvpTable *t, const cJSON *root) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive((cJSON *)root, "bots")) {
        if (t->bot_count == PVP_BOT_CAPACITY) TraceLog(LOG_FATAL, "Pvp: more than %d bots", PVP_BOT_CAPACITY);
        PvpBot *bot = &t->bots[t->bot_count++];
        snprintf(bot->name, sizeof(bot->name), PVP_BOT_NAME_FORMAT, JsonStr(entry, "name"));
        StrCopy(bot->avatar, sizeof(bot->avatar), JsonStr(entry, "avatar"));
        bot->trophies = JsonInt(entry, "trophies");
    }
}

static void PvpSquadsParse(PvpTable *t, const cJSON *root) {
    RuneCatalogEnsure();
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive((cJSON *)root, "squad_pool")) {
        if (t->squad_count == PVP_SQUAD_CAPACITY) TraceLog(LOG_FATAL, "Pvp: more than %d squads", PVP_SQUAD_CAPACITY);
        PvpSquad *squad = &t->squads[t->squad_count++];
        squad->strength = JsonInt(entry, "strength");
        squad->events_only = JsonBool(entry, "events_only");
        squad->elite = JsonBool(entry, "elite");
        int slot = 0;
        const cJSON *cell = NULL;
        cJSON_ArrayForEach(cell, cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "cells")) {
            if (slot == SQUAD_SLOT_COUNT) TraceLog(LOG_FATAL, "Pvp: squad with too many cells");
            PvpSquadCell *target = &squad->cells[slot++];
            if (!cJSON_IsObject(cell)) continue;
            StrCopy(target->id, sizeof(target->id), JsonStr(cell, "id"));
            target->tier = TierFromString(JsonStr(cell, "tier"));
            target->level = JsonInt(cell, "level");
            target->present = true;
            target->ascended = JsonBool(cell, "ascended");
            if (!BattleGearDefsLoad(target->gear_defs, cJSON_GetObjectItemCaseSensitive((cJSON *)cell, "gear")))
                TraceLog(LOG_FATAL, "Pvp: squad %d wears unknown gear", t->squad_count - 1);
        }
        const cJSON *rune = NULL;
        cJSON_ArrayForEach(rune, cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "runes")) {
            int def = cJSON_IsString(rune) ? RuneDefIndex(rune->valuestring) : -1;
            if (def < 0 || squad->rune_count == RUNE_SLOT_COUNT)
                TraceLog(LOG_FATAL, "Pvp: squad %d has an unknown rune or too many", t->squad_count - 1);
            squad->rune_defs[squad->rune_count++] = def;
        }
    }
}

static bool PvpSquadRanksBelow(const PvpSquad *squad, const PvpSquad *other) {
    if (squad->elite != other->elite) return other->elite;
    return squad->strength < other->strength;
}

static void PvpSquadListAddByStrength(PvpSquadList *list, const PvpTable *t, int squad) {
    int rank = list->count++;
    for (; rank > 0 && PvpSquadRanksBelow(&t->squads[squad], &t->squads[list->squads[rank - 1]]); rank--)
        list->squads[rank] = list->squads[rank - 1];
    list->squads[rank] = squad;
}

static void PvpSquadsRankByStrength(PvpTable *t) {
    for (int squad = 0; squad < t->squad_count; squad++)
        if (!t->squads[squad].events_only) PvpSquadListAddByStrength(&t->ladder, t, squad);
}

static Character PvpRuleHero(const cJSON *def, bool ascended) {
    Character ch = {0};
    StrCopy(ch.id, sizeof(ch.id), JsonStr(def, "id"));
    ch.rarity = RarityFromString(JsonStr(def, "rarity"));
    const char *original_rarity = JsonStr(def, "original_rarity");
    ch.original_rarity = original_rarity[0] ? RarityFromString(original_rarity) : ch.rarity;
    ch.ascended = ascended;
    ch.giant = JsonBool(def, "giant");
    ch.faction = FactionFromString(JsonStr(def, "faction"));
    const cJSON *group = NULL;
    cJSON_ArrayForEach(group, cJSON_GetObjectItemCaseSensitive((cJSON *)def, "groups")) {
        if (ch.group_count == MAX_GROUPS) break;
        StrCopy(ch.groups[ch.group_count++], sizeof(ch.groups[0]), group->valuestring);
    }
    return ch;
}

static bool PvpPooledSquadFits(
    const PvpSquadRules *rules, const Character heroes[SQUAD_SLOT_COUNT], const PvpSquad *squad, int player_side
) {
    const Character *fielded[SQUAD_SLOT_COUNT] = {0};
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (squad->cells[slot].present) fielded[slot] = &heroes[slot];
    if (rules->side_count == 0) return PvpSquadRulesJudge(rules, fielded, PVP_RULE_ANY_SIDE).kind == PVP_SQUAD_ALLOWED;
    if (PvpSquadRulesJudge(rules, fielded, player_side).kind == PVP_SQUAD_ALLOWED) return false;
    for (int side = 0; side < rules->side_count; side++)
        if (side != player_side && PvpSquadRulesJudge(rules, fielded, side).kind == PVP_SQUAD_ALLOWED) return true;
    return false;
}

static int PvpEventPoolCount(const PvpEvent *event) {
    return event->rules.side_count > 0 ? event->rules.side_count : 1;
}

static void PvpEventPoolsBuild(PvpTable *t, PvpEvent *events, int event_count, const cJSON *catalog) {
    static Character heroes[SQUAD_SLOT_COUNT];
    for (int squad = 0; squad < t->squad_count; squad++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            const PvpSquadCell *cell = &t->squads[squad].cells[slot];
            const cJSON *def = cell->present ? CatalogFindCharacter(catalog, cell->id) : NULL;
            if (cell->present && !def) TraceLog(LOG_FATAL, "Pvp: unit %s is not in the catalog", cell->id);
            heroes[slot] = def ? PvpRuleHero(def, cell->ascended) : (Character){0};
        }
        for (int e = 0; e < event_count; e++) {
            PvpEvent *event = &events[e];
            for (int side = 0; event->rules.active && side < PvpEventPoolCount(event); side++)
                if (PvpPooledSquadFits(&event->rules, heroes, &t->squads[squad], side))
                    PvpSquadListAddByStrength(&event->pools[side], t, squad);
        }
    }
    for (int e = 0; e < event_count; e++)
        for (int side = 0; events[e].rules.active && side < PvpEventPoolCount(&events[e]); side++)
            if (events[e].pools[side].count == 0)
                TraceLog(LOG_FATAL, "Pvp: %s has no squad to field", events[e].title_key);
}

static bool PvpEventsHaveSquadRules(const PvpEventRules *e) {
    for (int i = 0; i < e->catalog_count; i++)
        if (e->catalog[i].rules.active) return true;
    for (int i = 0; i < e->guild_catalog_count; i++)
        if (e->guild_catalog[i].rules.active) return true;
    return false;
}

static void PvpEventsPoolSquads(PvpTable *t) {
    if (!PvpEventsHaveSquadRules(&t->events)) return;
    cJSON *catalog_root = CharacterCatalogAddCustomHeroes(JsonLoadFile(PVP_CATALOG_PATH));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    PvpEventPoolsBuild(t, t->events.catalog, t->events.catalog_count, catalog);
    PvpEventPoolsBuild(t, t->events.guild_catalog, t->events.guild_catalog_count, catalog);
    cJSON_Delete(catalog_root);
}

const PvpTable *PvpTableEnsure(void) {
    if (pvp_table.loaded) return &pvp_table;
    cJSON *root = JsonLoadFile(PVP_RULES_PATH);
    PvpRulesParse(&pvp_table.rules, root);
    PvpEventsParse(&pvp_table.events, root);
    PvpBotsParse(&pvp_table, root);
    PvpSquadsParse(&pvp_table, root);
    PvpSquadsRankByStrength(&pvp_table);
    cJSON_Delete(root);
    if (pvp_table.bot_count == 0 || pvp_table.ladder.count == 0) TraceLog(LOG_FATAL, "Pvp: empty ladder");
    PvpEventsPoolSquads(&pvp_table);
    pvp_table.loaded = true;
    return &pvp_table;
}

static unsigned PvpMix(unsigned a, unsigned b) {
    unsigned h = a ^ (b * 0x9E3779B1u);
    h ^= h >> 15;
    h *= 0x85EBCA77u;
    h ^= h >> 13;
    h *= 0xC2B2AE3Du;
    return h ^ (h >> 16);
}

static int PvpRoll(unsigned seed, unsigned salt, int count) {
    return count > 0 ? (int)(PvpMix(seed, salt) % (unsigned)count) : 0;
}

long long PvpDay(long long now) {
    return now / NEXUS_SECONDS_PER_DAY;
}

int PvpTrophyGain(int trophies, int opponent_trophies) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    int gain = r->base_gain + (opponent_trophies - trophies) / r->points_per_step;
    return Clamp(gain, r->min_gain, r->max_gain);
}

int PvpTrophyLoss(int trophies, int opponent_trophies) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    int loss = r->base_loss + (trophies - opponent_trophies) / r->points_per_step;
    return Clamp(loss, r->min_loss, r->max_loss);
}

int PvpBotTrophies(int bot, long long day) {
    const PvpTable *t = PvpTableEnsure();
    int drift = t->rules.bot_daily_drift;
    unsigned seed = PvpMix((unsigned)bot, (unsigned)day);
    int trophies = t->bots[bot].trophies + PvpRoll(seed, PVP_DRIFT_SALT, drift * 2 + 1) - drift;
    return trophies > 0 ? trophies : 0;
}

static int PvpPickBot(int trophies, long long day, unsigned seed) {
    const PvpTable *t = PvpTableEnsure();
    int in_window = 0, closest = 0, closest_distance = INT_MAX;
    for (int bot = 0; bot < t->bot_count; bot++) {
        int delta = PvpBotTrophies(bot, day) - trophies;
        if (delta >= -t->rules.window_below && delta <= t->rules.window_above) in_window++;
        if (abs(delta) < closest_distance) {
            closest_distance = abs(delta);
            closest = bot;
        }
    }
    if (in_window == 0) return closest;
    int pick = PvpRoll(seed, PVP_BOT_SALT, in_window);
    for (int bot = 0; bot < t->bot_count; bot++) {
        int delta = PvpBotTrophies(bot, day) - trophies;
        if (delta >= -t->rules.window_below && delta <= t->rules.window_above && pick-- == 0) return bot;
    }
    return closest;
}

static int PvpListStrengthRank(const PvpSquadList *list, int bot) {
    const PvpTable *t = PvpTableEnsure();
    int top_trophies = t->bots[t->bot_count - 1].trophies;
    if (top_trophies <= 0) return 0;
    return (int)((long long)t->bots[bot].trophies * (list->count - 1) / top_trophies);
}

int PvpBotStrengthRank(int bot) {
    return PvpListStrengthRank(&PvpTableEnsure()->ladder, bot);
}

static int PvpPickSquadFrom(const PvpSquadList *list, int bot, unsigned seed) {
    int candidates = PvpTableEnsure()->rules.squad_candidates;
    int weakest = PvpListStrengthRank(list, bot) - candidates / 2;
    weakest = Clamp(weakest, 0, list->count > candidates ? list->count - candidates : 0);
    int span = candidates < list->count ? candidates : list->count;
    return list->squads[weakest + PvpRoll(seed, PVP_SQUAD_SALT, span)];
}

const PvpSquadList *PvpEventSquadPool(const PvpEvent *event, int side) {
    if (!event->rules.active) return &PvpTableEnsure()->ladder;
    return &event->pools[side >= 0 && side < PvpEventPoolCount(event) ? side : 0];
}

PvpSquadVerdict PvpEventJudgeSquad(const PvpEvent *event, const Player *p, int squad, int side) {
    const Character *heroes[SQUAD_SLOT_COUNT] = {0};
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        int unit = PlayerSquadUnitAt(p, squad, slot);
        if (unit >= 0 && unit < p->roster_count) heroes[slot] = &p->roster[unit];
    }
    return PvpSquadRulesJudge(&event->rules, heroes, side);
}

static void PvpTakeSquadRunes(PvpOpponent *o) {
    const PvpSquad *squad = &PvpTableEnsure()->squads[o->squad];
    o->rune_count = squad->rune_count;
    memcpy(o->rune_defs, squad->rune_defs, sizeof(o->rune_defs));
}

static void PvpOpponentDerive(PvpOpponent *o, int player_trophies) {
    o->gain = PvpTrophyGain(player_trophies, o->trophies);
    o->loss = PvpTrophyLoss(player_trophies, o->trophies);
    PvpTakeSquadRunes(o);
}

PvpOpponent
PvpOpponentFromBotIn(const PvpSquadList *pool, int bot, int level, int trophies, long long day, unsigned seed) {
    const PvpTable *t = PvpTableEnsure();
    PvpOpponent o = {.bot = bot, .seed = seed ? seed : 1u};
    o.trophies = PvpBotTrophies(bot, day);
    int spread = t->rules.level_spread;
    o.level = Clamp(level + PvpRoll(o.seed, PVP_LEVEL_SALT, spread * 2 + 1) - spread, 1, PVP_LOCAL_MAX_LEVEL);
    o.squad = PvpPickSquadFrom(pool, bot, o.seed);
    PvpOpponentDerive(&o, trophies);
    return o;
}

PvpOpponent PvpOpponentFromBot(int bot, int level, int trophies, long long day, unsigned seed) {
    return PvpOpponentFromBotIn(&PvpTableEnsure()->ladder, bot, level, trophies, day, seed);
}

PvpOpponent PvpOpponentGenerate(int level, int trophies, long long day, unsigned seed) {
    return PvpOpponentFromBot(PvpPickBot(trophies, day, seed ? seed : 1u), level, trophies, day, seed);
}

PvpOpponent PvpOpponentFromBattle(const PlayerPvpBattle *battle, int trophies) {
    PvpOpponent o = {
        .bot = battle->bot,
        .squad = battle->squad,
        .level = battle->level,
        .trophies = battle->opponent_trophies,
        .seed = battle->seed,
    };
    PvpOpponentDerive(&o, trophies);
    return o;
}

bool PvpOpponentFromServerMatch(const cJSON *match, int trophies, PvpOpponent *out) {
    const PvpTable *t = PvpTableEnsure();
    const cJSON *seed = cJSON_GetObjectItemCaseSensitive(match, "seed");
    int bot = JsonInt(match, "bot");
    int squad = JsonInt(match, "squad");
    int level = JsonInt(match, "level");
    if (!cJSON_IsObject(match) || !cJSON_IsNumber(seed) || bot < 0 || bot >= t->bot_count || squad < 0 ||
        squad >= t->squad_count || level < 1 || level > PVP_LOCAL_MAX_LEVEL)
        return false;
    PlayerPvpBattle matched = {
        .bot = bot,
        .squad = squad,
        .level = level,
        .opponent_trophies = JsonInt(match, "trophies"),
        .seed = (unsigned)seed->valuedouble,
    };
    *out = PvpOpponentFromBattle(&matched, trophies);
    return true;
}

PvpOpponent
PvpOpponentFromDuel(const char *id, const char *name, int level, int opponent_trophies, int trophies, unsigned seed) {
    PvpOpponent o = {.bot = -1, .squad = -1, .duel = true, .seed = seed ? seed : 1u};
    StrCopy(o.duel_id, sizeof(o.duel_id), id);
    StrCopy(o.duel_name, sizeof(o.duel_name), name);
    o.level = Clamp(level, 1, PVP_LOCAL_MAX_LEVEL);
    o.trophies = opponent_trophies;
    o.gain = PvpTrophyGain(trophies, o.trophies);
    o.loss = PvpTrophyLoss(trophies, o.trophies);
    return o;
}

bool PvpOpponentAddDuelUnit(
    PvpOpponent *o, int slot, const char *id, Tier tier, int level, const BattleHeroBuild *build
) {
    if (slot < 0 || slot >= SQUAD_SLOT_COUNT || !id[0]) return false;
    PvpSquadCell *cell = &o->duel_cells[slot];
    StrCopy(cell->id, sizeof(cell->id), id);
    cell->tier = tier;
    cell->level = level > 0 ? level : 1;
    cell->present = true;
    if (build) {
        o->duel_builds[slot] = *build;
        o->duel_hero_builds = true;
    }
    return true;
}

void PvpOpponentAddDuelRune(PvpOpponent *o, const char *rune_id) {
    int def = RuneDefIndex(rune_id);
    if (def >= 0 && o->rune_count < RUNE_SLOT_COUNT) o->rune_defs[o->rune_count++] = def;
}

NameBadges PvpOpponentBadges(const PvpOpponent *o) {
    return o->duel ? (NameBadges){o->duel_renown_rank, o->duel_vault_tier} : (NameBadges){0};
}

const char *PvpOpponentName(const PvpOpponent *o) {
    if (o->duel) return o->duel_name;
    return PvpTableEnsure()->bots[o->bot].name;
}

void PvpOpponentRoster(BattleRoster *roster, const PvpOpponent *o) {
    memset(roster, 0, sizeof(*roster));
    cJSON *catalog_root = CharacterCatalogAddCustomHeroes(JsonLoadFile(PVP_CATALOG_PATH));
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    const PvpSquadCell *cells = o->duel ? o->duel_cells : PvpTableEnsure()->squads[o->squad].cells;
    roster->hero_builds = o->duel && o->duel_hero_builds;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const PvpSquadCell *cell = &cells[slot];
        if (!cell->present) continue;
        cJSON *def = CatalogFindCharacter(catalog, cell->id);
        if (!def && o->duel) continue;
        if (!def) TraceLog(LOG_FATAL, "Pvp: unit %s is not in the catalog", cell->id);
        Character *ch = &roster->chars[slot];
        CharacterParse(def, ch, &roster->abilities[slot]);
        ch->tier = cell->tier;
        ch->level = cell->level;
        if (roster->hero_builds && !BattleHeroBuildApply(ch, &o->duel_builds[slot])) continue;
        if (!o->duel) BattleWornGearApply(ch, cell->ascended, cell->gear_defs);
        roster->present[slot] = true;
    }
    cJSON_Delete(catalog_root);
    for (int i = 0; i < o->rune_count; i++) roster->rune_defs[roster->rune_def_count++] = o->rune_defs[i];
    roster->renown = o->renown;
    roster->stat_budget = PvpTableEnsure()->rules.stat_budget ? StatBudgetEnsure() : NULL;
}

int PvpNextsRemaining(const PlayerPvp *pvp, long long now) {
    int limit = PvpTableEnsure()->rules.daily_nexts;
    return pvp->nexts_day == PvpDay(now) ? limit - pvp->nexts_used : limit;
}

bool PvpSpendNext(PlayerPvp *pvp, long long now) {
    if (PvpNextsRemaining(pvp, now) <= 0) return false;
    long long day = PvpDay(now);
    if (pvp->nexts_day != day) pvp->nexts_used = 0;
    pvp->nexts_day = day;
    pvp->nexts_used++;
    return true;
}

static void PvpHistoryPush(PlayerPvp *pvp, const PlayerPvpBattle *battle) {
    int keep = pvp->history_count < PVP_HISTORY_CAPACITY ? pvp->history_count : PVP_HISTORY_CAPACITY - 1;
    memmove(&pvp->history[1], &pvp->history[0], (size_t)keep * sizeof(pvp->history[0]));
    pvp->history[0] = *battle;
    pvp->history_count = keep + 1;
}

static void PvpHoldUpdate(PlayerPvp *pvp, long long now) {
    for (int i = 0; i < PVP_HOLD_THRESHOLD_COUNT; i++) {
        if (pvp->trophies <= PVP_HOLD_THRESHOLDS[i])
            pvp->held_since[i] = 0;
        else if (pvp->held_since[i] == 0)
            pvp->held_since[i] = now;
    }
}

int PvpTrophiesHeld(const PlayerPvp *pvp, long long now) {
    int held = 0;
    for (int i = 0; i < PVP_HOLD_THRESHOLD_COUNT; i++)
        if (pvp->held_since[i] > 0 && now - pvp->held_since[i] >= PVP_HOLD_SECONDS) held = PVP_HOLD_THRESHOLDS[i];
    return held;
}

static int PvpApplyTrophies(PlayerPvp *pvp, int delta, long long now) {
    int before = pvp->trophies;
    pvp->trophies = before + delta > 0 ? before + delta : 0;
    if (pvp->trophies > pvp->best_trophies) pvp->best_trophies = pvp->trophies;
    PvpHoldUpdate(pvp, now);
    return pvp->trophies - before;
}

int PvpGoldReward(const PvpOpponent *o, bool won) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    int gold = r->win_gold_base + r->win_gold_per_level * o->level;
    return won ? gold : gold * r->loss_gold_pct / 100;
}

int PvpXpReward(const PvpOpponent *o, bool won) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    int xp = r->win_xp_base + r->win_xp_per_level * o->level;
    return won ? xp : xp * r->loss_xp_pct / 100;
}

void PvpNoteAttack(PlayerPvp *pvp, const char *id, long long at) {
    PlayerPvpAttack kept[PVP_ATTACKED_CAPACITY] = {0};
    int count = 0, oldest = 0;
    for (int i = 0; i < pvp->attacked_count; i++) {
        const PlayerPvpAttack *attack = &pvp->attacked[i];
        if (!strcmp(attack->id, id) || attack->at + PVP_REPEAT_ATTACK_SECONDS <= at) continue;
        if (count == 0 || attack->at < kept[oldest].at) oldest = count;
        kept[count++] = *attack;
    }
    if (count == PVP_ATTACKED_CAPACITY) {
        kept[oldest] = kept[--count];
        kept[count] = (PlayerPvpAttack){0};
    }
    kept[count].at = at;
    StrCopy(kept[count].id, sizeof(kept[count].id), id);
    memcpy(pvp->attacked, kept, sizeof(kept));
    pvp->attacked_count = count + 1;
}

long long PvpAttackCooldownLeft(const PlayerPvp *pvp, const char *id, long long now) {
    for (int i = 0; i < pvp->attacked_count; i++) {
        long long left = pvp->attacked[i].at + PVP_REPEAT_ATTACK_SECONDS - now;
        if (!strcmp(pvp->attacked[i].id, id)) return left > 0 ? left : 0;
    }
    return 0;
}

static void PvpNameDuelOpponent(PlayerPvp *pvp, PlayerPvpBattle *battle, const PvpOpponent *o, long long now) {
    if (!o->duel) return;
    StrCopy(battle->opponent, sizeof(battle->opponent), o->duel_name);
    StrCopy(battle->opponent_id, sizeof(battle->opponent_id), o->duel_id);
    PvpNoteAttack(pvp, o->duel_id, now);
}

PlayerPvpBattle PvpRecordAttack(PlayerPvp *pvp, const PvpOpponent *o, bool won, long long now) {
    int delta = PvpApplyTrophies(pvp, won ? o->gain : -o->loss, now);
    if (pvp->wins + pvp->losses == 0) pvp->defense_checked_at = now;
    pvp->wins += won;
    pvp->losses += !won;
    pvp->win_streak = won ? pvp->win_streak + 1 : 0;
    if (pvp->win_streak > pvp->best_win_streak) pvp->best_win_streak = pvp->win_streak;
    if (delta > 0) pvp->trophies_won += delta;
    PlayerPvpBattle battle = {
        .at = now,
        .bot = o->duel ? 0 : o->bot,
        .squad = o->duel ? 0 : o->squad,
        .level = o->level,
        .opponent_trophies = o->trophies,
        .trophy_delta = delta,
        .gold = PvpGoldReward(o, won),
        .xp = PvpXpReward(o, won),
        .seed = o->seed,
        .won = won,
    };
    PvpNameDuelOpponent(pvp, &battle, o, now);
    PvpHistoryPush(pvp, &battle);
    return battle;
}

int PvpBotDefenseTrophies(int bot_trophies, int defender_trophies, bool bot_won) {
    int share = PvpTableEnsure()->rules.defense_loss_share_pct;
    if (bot_won) return -PvpTrophyGain(bot_trophies, defender_trophies) * share / 100;
    return PvpTrophyLoss(bot_trophies, defender_trophies) * share / 100;
}

static void PvpRecordDefense(PlayerPvp *pvp, const PvpOpponent *o, bool won, long long at) {
    int delta = PvpApplyTrophies(pvp, PvpBotDefenseTrophies(o->trophies, pvp->trophies, !won), at);
    pvp->defense_wins += won;
    pvp->defense_losses += !won;
    PlayerPvpBattle battle = {
        .at = at,
        .bot = o->bot,
        .squad = o->squad,
        .level = o->level,
        .opponent_trophies = o->trophies,
        .trophy_delta = delta,
        .seed = o->seed,
        .defense = true,
        .won = won,
    };
    PvpHistoryPush(pvp, &battle);
}

static bool PvpFirstSquadHolds(const Player *player, const PvpOpponent *attacker) {
    if (PlayerSquadCount(player, PVP_DEFENSE_SQUAD) == 0) return false;
    static BattleRoster roster;
    static BattleSim sim;
    PvpOpponentRoster(&roster, attacker);
    BattleSimRunSquad(&sim, player, PVP_DEFENSE_SQUAD, &roster, attacker->seed);
    return sim.player_won;
}

int PvpSimulateDefense(Player *player, long long now) {
    PlayerPvp *pvp = &player->pvp;
    if (pvp->wins + pvp->losses == 0) return 0;
    const PvpRules *r = &PvpTableEnsure()->rules;
    if (pvp->defense_checked_at <= 0 || pvp->defense_checked_at > now) {
        pvp->defense_checked_at = now;
        return 0;
    }
    long long due = (now - pvp->defense_checked_at) / r->attack_interval_seconds;
    if (due <= 0) return 0;
    int attacks = due < r->max_attacks_per_login ? (int)due : r->max_attacks_per_login;
    for (int i = 0; i < attacks; i++) {
        long long at = now - (long long)(attacks - 1 - i) * r->attack_interval_seconds;
        unsigned seed = PvpMix((unsigned)at, PVP_DEFENSE_SALT + (unsigned)i);
        PvpOpponent o = PvpOpponentGenerate(player->level, pvp->trophies, PvpDay(at), seed);
        PvpRecordDefense(pvp, &o, PvpFirstSquadHolds(player, &o), at);
    }
    pvp->defense_checked_at += due * r->attack_interval_seconds;
    return attacks;
}

bool PvpCanRevenge(const PlayerPvpBattle *battle) {
    return battle->defense && !battle->opponent_id[0] && !battle->revenged;
}

bool PvpRevengesAPlayer(const PlayerPvpBattle *battle) {
    return battle->defense && battle->opponent_id[0];
}

bool PvpBattleInEvent(const PlayerPvpBattle *battle) {
    return battle->event || battle->banners > 0;
}

long long PvpEventCycle(long long now) {
    return now / PvpTableEnsure()->events.duration_seconds;
}

bool PvpEventIsGuildCycle(long long cycle) {
    const PvpEventRules *e = &PvpTableEnsure()->events;
    return e->guild_catalog_count > 0 && cycle >= 0 && cycle % e->guild_every_cycles == 0;
}

const PvpEvent *PvpEventForCycle(long long cycle) {
    const PvpEventRules *e = &PvpTableEnsure()->events;
    if (PvpEventIsGuildCycle(cycle)) return &e->guild_catalog[(cycle / e->guild_every_cycles) % e->guild_catalog_count];
    return &e->catalog[cycle % e->catalog_count];
}

long long PvpEventCycleStart(long long cycle) {
    return cycle * PvpTableEnsure()->events.duration_seconds;
}

long long PvpGuildCycleBefore(long long cycle) {
    const PvpEventRules *e = &PvpTableEnsure()->events;
    if (e->guild_catalog_count == 0 || cycle <= 0) return -1;
    return (cycle - 1) / e->guild_every_cycles * e->guild_every_cycles;
}

long long PvpGuildCycleAfter(long long cycle) {
    const PvpEventRules *e = &PvpTableEnsure()->events;
    if (e->guild_catalog_count == 0) return -1;
    return (cycle / e->guild_every_cycles + 1) * e->guild_every_cycles;
}

const PvpEventRankReward *PvpGuildRewardForRank(const PvpEvent *event, int rank) {
    for (int i = 0; i < event->guild_rank_reward_count; i++) {
        const PvpEventRankReward *tier = &event->guild_rank_rewards[i];
        if (rank >= tier->first && rank <= tier->last) return tier;
    }
    return NULL;
}

long long PvpEventRemaining(long long now) {
    long long duration = PvpTableEnsure()->events.duration_seconds;
    return duration - now % duration;
}

static int PvpBotBanners(int bot, long long cycle, long long elapsed) {
    const PvpTable *t = PvpTableEnsure();
    long long standing = (long long)(bot + 1) * (bot + 1);
    long long full = t->events.top_bot_banners * standing / ((long long)t->bot_count * t->bot_count);
    unsigned seed = PvpMix((unsigned)bot, (unsigned)cycle);
    int jitter = PVP_EVENT_SKILL_JITTER_PCT;
    full = full * (100 - jitter + PvpRoll(seed, PVP_EVENT_SALT, jitter * 2 + 1)) / 100;
    return (int)(full * elapsed / t->events.duration_seconds);
}

int PvpEventMaxBannersPerWin(int banners) {
    const PvpEventRules *e = &PvpTableEnsure()->events;
    int ladder = e->ladder_banners;
    int step = ladder / e->win_banners_steps_to_peak;
    int midpoint = ladder / 2;
    int peak = e->win_banners_start + e->win_banners_peak_bonus;
    int position = banners < ladder ? banners : ladder;
    if (position <= midpoint) return e->win_banners_start + position / step;
    return peak - (position - midpoint) / (step * e->win_banners_decline_slowdown);
}

int PvpEventBannerReward(const PvpOpponent *o, int banners, bool won) {
    const PvpTable *t = PvpTableEnsure();
    if (!won) return t->events.loss_banners;
    int span = t->rules.max_gain - t->rules.min_gain;
    int floor_pct = t->events.win_banners_floor_pct;
    int pct = floor_pct + (100 - floor_pct) * (o->gain - t->rules.min_gain) / span;
    return PvpEventMaxBannersPerWin(banners) * pct / 100;
}

static int PvpPickEventBot(int banners, long long cycle, long long elapsed, unsigned seed) {
    const PvpTable *t = PvpTableEnsure();
    int window = t->events.matchmaking_banner_window;
    int in_window = 0, closest = 0, closest_distance = INT_MAX;
    for (int bot = 0; bot < t->bot_count; bot++) {
        int distance = abs(PvpBotBanners(bot, cycle, elapsed) - banners);
        in_window += distance <= window;
        if (distance < closest_distance) {
            closest_distance = distance;
            closest = bot;
        }
    }
    if (in_window == 0) return closest;
    int pick = PvpRoll(seed, PVP_BOT_SALT, in_window);
    for (int bot = 0; bot < t->bot_count; bot++)
        if (abs(PvpBotBanners(bot, cycle, elapsed) - banners) <= window && pick-- == 0) return bot;
    return closest;
}

PvpOpponent PvpEventOpponentFromBot(int bot, int level, int trophies, long long now, int side, unsigned seed) {
    const PvpSquadList *pool = PvpEventSquadPool(PvpEventForCycle(PvpEventCycle(now)), side);
    return PvpOpponentFromBotIn(pool, bot, level, trophies, PvpDay(now), seed);
}

PvpOpponent PvpEventOpponentGenerate(int level, int trophies, int banners, long long now, int side, unsigned seed) {
    long long elapsed = now % PvpTableEnsure()->events.duration_seconds;
    int bot = PvpPickEventBot(banners, PvpEventCycle(now), elapsed, seed ? seed : 1u);
    return PvpEventOpponentFromBot(bot, level, trophies, now, side, seed);
}

int PvpEventRivalsAhead(long long cycle, int banners);

int PvpEventRankAt(int banners, long long cycle, long long elapsed) {
    const PvpTable *t = PvpTableEnsure();
    int rank = 1 + PvpEventRivalsAhead(cycle, banners);
    for (int bot = 0; bot < t->bot_count; bot++) rank += PvpBotBanners(bot, cycle, elapsed) > banners;
    return rank;
}

const PvpEventRankReward *PvpEventRewardForRank(const PvpEvent *event, int rank) {
    for (int i = 0; i < event->rank_reward_count; i++)
        if (rank >= event->rank_rewards[i].first && rank <= event->rank_rewards[i].last) return &event->rank_rewards[i];
    if (event->rank_reward_count == 0) return NULL;
    const PvpEventRankReward *last_tier = &event->rank_rewards[event->rank_reward_count - 1];
    return rank > last_tier->last ? last_tier : NULL;
}

int PvpEventRankTier(const PvpEvent *event, int rank) {
    const PvpEventRankReward *reward = PvpEventRewardForRank(event, rank);
    return reward ? (int)(reward - event->rank_rewards) : event->rank_reward_count - 1;
}

void PvpPayoutAdd(PvpPayout *payout, const PvpReward *reward) {
    payout->gold += reward->gold;
    payout->gems += reward->gems;
    for (int i = 0; i < reward->prize_count && payout->prize_count < PVP_PAYOUT_PRIZE_CAPACITY; i++)
        payout->prizes[payout->prize_count++] = &reward->prizes[i];
}

int PvpRewardUnitCount(const PvpReward *reward) {
    int count = 0;
    for (int i = 0; i < reward->prize_count; i++)
        if (reward->prizes[i].kind == PVP_PRIZE_UNIT) count += reward->prizes[i].count;
    return count;
}

int PvpPayoutUnitCount(const PvpPayout *payout) {
    int count = 0;
    for (int i = 0; i < payout->prize_count; i++)
        if (payout->prizes[i]->kind == PVP_PRIZE_UNIT) count += payout->prizes[i]->count;
    return count;
}

static Texture2D PvpPrizeTextureLoad(const PvpPrize *prize) {
    if (prize->kind == PVP_PRIZE_UNIT) return GuiPortraitLoad(prize->icon);
    Texture2D texture = ResourceLoadTexture(prize->icon);
    if (texture.id == 0) TraceLog(LOG_WARNING, "Pvp: missing prize icon %s", prize->icon);
    return texture;
}

Texture2D PvpPrizeTexture(const PvpPrize *prize) {
    if (prize->kind == PVP_PRIZE_GEAR) return GearIconLoad(GearDefAt(prize->index));
    if (prize->kind == PVP_PRIZE_TOME) return GearTomeIconLoad();
    static struct {
        const char *icon;
        Texture2D texture;
    } cache[PVP_PRIZE_TEXTURE_CAPACITY];
    static int count = 0;
    for (int i = 0; i < count; i++)
        if (!strcmp(cache[i].icon, prize->icon)) return cache[i].texture;
    if (count == PVP_PRIZE_TEXTURE_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many prize textures");
    cache[count].icon = prize->icon;
    cache[count].texture = PvpPrizeTextureLoad(prize);
    return cache[count++].texture;
}

Texture2D PvpRewardTexture(const PvpReward *reward) {
    if (reward->prize_count > 0) return PvpPrizeTexture(&reward->prizes[0]);
    return GuiTex(reward->gems > 0 ? GUI_MISSION_GEM : GUI_MISSION_GOLD);
}

static int PvpEventResultCompare(const void *a, const void *b) {
    const PlayerPvpEventResult *left = a, *right = b;
    if (left->claimed != right->claimed) return left->claimed ? 1 : -1;
    if (left->cycle == right->cycle) return 0;
    return left->cycle > right->cycle ? -1 : 1;
}

void PvpEventSortResults(PlayerPvp *pvp) {
    qsort(pvp->event_results, (size_t)pvp->event_result_count, sizeof(pvp->event_results[0]), PvpEventResultCompare);
}

static void PvpEventDropOldestResult(PlayerPvp *pvp) {
    int oldest = 0;
    for (int i = 1; i < pvp->event_result_count; i++)
        if (pvp->event_results[i].cycle < pvp->event_results[oldest].cycle) oldest = i;
    pvp->event_results[oldest] = pvp->event_results[--pvp->event_result_count];
}

void PvpEventResultRank(PlayerPvpEventResult *result, int rank) {
    const PvpEventRankReward *reward = PvpEventRewardForRank(PvpEventForCycle(result->cycle), rank);
    result->rank = rank;
    result->gold = reward ? reward->reward.gold : 0;
    result->gems = reward ? reward->reward.gems : 0;
}

static void PvpEventFinish(PlayerPvp *pvp) {
    const PlayerPvpEvent *event = &pvp->event;
    if (event->banners <= 0) return;
    int duration = PvpTableEnsure()->events.duration_seconds;
    PlayerPvpEventResult result = {.cycle = event->cycle, .banners = event->banners};
    PvpEventResultRank(&result, PvpEventRankAt(event->banners, event->cycle, duration));
    if (pvp->event_result_count == PVP_EVENT_RESULT_CAPACITY) PvpEventDropOldestResult(pvp);
    pvp->event_results[pvp->event_result_count++] = result;
    PvpEventSortResults(pvp);
}

void PvpEventSync(PlayerPvp *pvp, long long now) {
    long long cycle = PvpEventCycle(now);
    if (pvp->event.cycle == cycle) return;
    PvpEventFinish(pvp);
    pvp->event = (PlayerPvpEvent){.cycle = cycle};
}

int PvpEventUnclaimedCount(const PlayerPvp *pvp) {
    int count = 0;
    for (int i = 0; i < pvp->event_result_count; i++) count += !pvp->event_results[i].claimed;
    return count;
}

int PvpEventCurrentRank(const PlayerPvp *pvp, long long now) {
    long long duration = PvpTableEnsure()->events.duration_seconds;
    return PvpEventRankAt(pvp->event.banners, pvp->event.cycle, now % duration);
}

PlayerPvpBattle PvpRecordEventAttack(PlayerPvp *pvp, const PvpOpponent *o, bool won, long long now) {
    PvpEventSync(pvp, now);
    if (pvp->wins + pvp->losses == 0) pvp->defense_checked_at = now;
    pvp->wins += won;
    pvp->losses += !won;
    pvp->win_streak = won ? pvp->win_streak + 1 : 0;
    if (pvp->win_streak > pvp->best_win_streak) pvp->best_win_streak = pvp->win_streak;
    PlayerPvpBattle battle = {
        .at = now,
        .bot = o->duel ? 0 : o->bot,
        .squad = o->duel ? 0 : o->squad,
        .level = o->level,
        .opponent_trophies = o->trophies,
        .gold = PvpGoldReward(o, won),
        .xp = PvpXpReward(o, won),
        .banners = PvpEventBannerReward(o, pvp->event.banners, won),
        .seed = o->seed,
        .won = won,
        .event = true,
    };
    PvpNameDuelOpponent(pvp, &battle, o, now);
    pvp->event.banners += battle.banners;
    PvpHistoryPush(pvp, &battle);
    return battle;
}

PvpPayout PvpEventTakeMilestones(PlayerPvp *pvp, int roster_room) {
    const PvpEvent *event = PvpEventForCycle(pvp->event.cycle);
    PvpPayout total = {0};
    int units = 0;
    while (pvp->event.milestones_paid < event->milestone_count) {
        const PvpEventMilestone *next = &event->milestones[pvp->event.milestones_paid];
        if (pvp->event.banners < next->banners) break;
        units += PvpRewardUnitCount(&next->reward);
        if (units > roster_room) break;
        PvpPayoutAdd(&total, &next->reward);
        pvp->event.milestones_paid++;
    }
    return total;
}

static int PvpRankTieOrder(const PvpRankEntry *entry) {
    if (entry->bot == PVP_RANK_PLAYER) return 0;
    return entry->bot == PVP_RANK_RIVAL ? 1 : 2;
}

static int PvpRankCompare(const void *a, const void *b) {
    const PvpRankEntry *left = a, *right = b;
    if (left->trophies != right->trophies) return right->trophies - left->trophies;
    if (PvpRankTieOrder(left) != PvpRankTieOrder(right)) return PvpRankTieOrder(left) - PvpRankTieOrder(right);
    if (left->bot != right->bot) return left->bot - right->bot;
    return left->rival - right->rival;
}

int PvpLeaderboardBuild(PvpRankEntry *out, int trophies, long long day) {
    const PvpTable *t = PvpTableEnsure();
    for (int bot = 0; bot < t->bot_count; bot++) out[bot] = (PvpRankEntry){bot, PvpBotTrophies(bot, day)};
    out[t->bot_count] = (PvpRankEntry){PVP_RANK_PLAYER, trophies};
    qsort(out, (size_t)t->bot_count + 1, sizeof(out[0]), PvpRankCompare);
    return t->bot_count + 1;
}

int PvpEventLeaderboardBuild(PvpRankEntry *out, int banners, long long cycle, long long elapsed) {
    const PvpTable *t = PvpTableEnsure();
    for (int bot = 0; bot < t->bot_count; bot++) out[bot] = (PvpRankEntry){bot, PvpBotBanners(bot, cycle, elapsed)};
    out[t->bot_count] = (PvpRankEntry){PVP_RANK_PLAYER, banners};
    qsort(out, (size_t)t->bot_count + 1, sizeof(out[0]), PvpRankCompare);
    return t->bot_count + 1;
}

int PvpLeaderboardRank(const PvpRankEntry *entries, int count) {
    for (int i = 0; i < count; i++)
        if (entries[i].bot == PVP_RANK_PLAYER) return i + 1;
    return count;
}

void PvpHistorySanitize(PlayerPvp *pvp) {
    const PvpTable *t = PvpTableEnsure();
    int kept = 0;
    for (int i = 0; i < pvp->history_count; i++) {
        const PlayerPvpBattle *battle = &pvp->history[i];
        if (battle->bot >= t->bot_count || battle->squad >= t->squad_count) continue;
        pvp->history[kept++] = *battle;
    }
    pvp->history_count = kept;
}
