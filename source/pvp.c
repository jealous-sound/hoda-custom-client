#define PVP_RULES_PATH "data/pvp_rules.json"
#define PVP_CATALOG_PATH HOD_CACHE "characters.json"
#define PVP_BOT_CAPACITY 256
#define PVP_SQUAD_CAPACITY 640
#define PVP_NAME_CAP 32
#define PVP_UNIT_ID_CAP 48
#define PVP_LOCAL_MAX_LEVEL 100
#define PVP_STAMINA_COST 1
#define PVP_LEVEL_SALT 0x51ED27u
#define PVP_BOT_SALT 0x7A3C11u
#define PVP_SQUAD_SALT 0x2F90D3u
#define PVP_RUNE_SALT 0x64B8E1u
#define PVP_DRIFT_SALT 0x1C8A77u
#define PVP_DEFENSE_SALT 0x3D6F02u

#define PVP_EVENT_CAPACITY 16
#define PVP_EVENT_REWARD_CAPACITY 16
#define PVP_EVENT_KEY_CAP 96
#define PVP_EVENT_SKILL_JITTER_PCT 25
#define PVP_EVENT_SALT 0x4E11A9u
#define PVP_REWARD_PRIZE_CAPACITY 6
#define PVP_PAYOUT_PRIZE_CAPACITY 16
#define PVP_PRIZE_TEXTURE_CAPACITY 256

typedef enum {
    PVP_PRIZE_UNIT,
    PVP_PRIZE_GEAR,
    PVP_PRIZE_VOUCHER,
    PVP_PRIZE_TOME,
} PvpPrizeKind;

typedef struct {
    PvpPrizeKind kind;
    char id[48];
    char name[96];
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
    PvpEventMilestone milestones[PVP_EVENT_REWARD_CAPACITY];
    int milestone_count;
    PvpEventRankReward rank_rewards[PVP_EVENT_REWARD_CAPACITY];
    int rank_reward_count;
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
} PvpEventRules;

typedef struct {
    int base_gain, base_loss, points_per_step;
    int min_gain, max_gain, min_loss, max_loss;
    int window_below, window_above, daily_nexts, bot_daily_drift, level_spread;
    int rating_base, rating_per_level, rating_per_level_squared;
    int rating_max_shift_pct, trophies_per_shift_pct, squad_candidates;
    int first_rune_level, levels_per_rune, levels_per_rarity, max_rune_rarity;
    int win_gold_base, win_gold_per_level, loss_gold_pct;
    int win_xp_base, win_xp_per_level, loss_xp_pct;
    int attack_interval_seconds, max_attacks_per_login, defense_win_pct, defense_loss_share_pct;
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
} PvpSquadCell;

typedef struct {
    int rating;
    PvpSquadCell cells[SQUAD_SLOT_COUNT];
} PvpSquad;

typedef struct {
    PvpRules rules;
    PvpEventRules events;
    PvpBot bots[PVP_BOT_CAPACITY];
    int bot_count;
    PvpSquad squads[PVP_SQUAD_CAPACITY];
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
} PvpOpponent;

#define PVP_RANK_PLAYER (-1)

typedef struct {
    int bot;
    int trophies;
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
    r->rating_base = PvpSectionInt(root, "squads", "rating_base");
    r->rating_per_level = PvpSectionInt(root, "squads", "rating_per_level");
    r->rating_per_level_squared = PvpSectionInt(root, "squads", "rating_per_level_squared");
    r->rating_max_shift_pct = PvpSectionInt(root, "squads", "rating_max_shift_pct");
    r->trophies_per_shift_pct = PvpSectionInt(root, "squads", "trophies_per_shift_pct");
    r->squad_candidates = PvpSectionInt(root, "squads", "candidates");
    r->first_rune_level = PvpSectionInt(root, "runes", "first_rune_level");
    r->levels_per_rune = PvpSectionInt(root, "runes", "levels_per_rune");
    r->levels_per_rarity = PvpSectionInt(root, "runes", "levels_per_rarity");
    r->max_rune_rarity = PvpSectionInt(root, "runes", "max_rarity");
    r->win_gold_base = PvpSectionInt(root, "rewards", "win_gold_base");
    r->win_gold_per_level = PvpSectionInt(root, "rewards", "win_gold_per_level");
    r->loss_gold_pct = PvpSectionInt(root, "rewards", "loss_gold_pct");
    r->win_xp_base = PvpSectionInt(root, "rewards", "win_xp_base");
    r->win_xp_per_level = PvpSectionInt(root, "rewards", "win_xp_per_level");
    r->loss_xp_pct = PvpSectionInt(root, "rewards", "loss_xp_pct");
    r->attack_interval_seconds = PvpSectionInt(root, "defense", "attack_interval_seconds");
    r->max_attacks_per_login = PvpSectionInt(root, "defense", "max_attacks_per_login");
    r->defense_win_pct = PvpSectionInt(root, "defense", "win_pct");
    r->defense_loss_share_pct = PvpSectionInt(root, "defense", "loss_share_pct");
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
    StrCopy(prize.name, sizeof(prize.name), JsonStr(json, "name"));
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

static void PvpEventLadderParse(PvpEvent *event, const cJSON *json) {
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "milestones")) {
        if (event->milestone_count == PVP_EVENT_REWARD_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many milestones");
        event->milestones[event->milestone_count++] =
            (PvpEventMilestone){JsonInt(item, "banners"), PvpRewardParse(item)};
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "rank_rewards")) {
        if (event->rank_reward_count == PVP_EVENT_REWARD_CAPACITY) TraceLog(LOG_FATAL, "Pvp: too many rank rewards");
        event->rank_rewards[event->rank_reward_count++] =
            (PvpEventRankReward){JsonInt(item, "first"), JsonInt(item, "last"), PvpRewardParse(item)};
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
        PvpEvent *event = &e->catalog[e->catalog_count++];
        StrCopy(event->title_key, sizeof(event->title_key), JsonStr(item, "title_key"));
        PvpEventLadderParse(event, item);
        StrCopy(event->changer.text_key, sizeof(event->changer.text_key), JsonStr(item, "text_key"));
        const cJSON *effect = NULL;
        cJSON_ArrayForEach(effect, cJSON_GetObjectItemCaseSensitive((cJSON *)item, "effects")) {
            if (event->changer.effect_count == BATTLE_CHANGER_MAX_EFFECTS)
                TraceLog(LOG_FATAL, "Pvp: too many event effects");
            BattleChangerEffectLoad(&event->changer.effects[event->changer.effect_count++], effect);
        }
    }
    if (e->catalog_count == 0 || e->duration_seconds <= 0) TraceLog(LOG_FATAL, "Pvp: no events");
}

static void PvpBotsParse(PvpTable *t, const cJSON *root) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive((cJSON *)root, "bots")) {
        if (t->bot_count == PVP_BOT_CAPACITY) TraceLog(LOG_FATAL, "Pvp: more than %d bots", PVP_BOT_CAPACITY);
        PvpBot *bot = &t->bots[t->bot_count++];
        StrCopy(bot->name, sizeof(bot->name), JsonStr(entry, "name"));
        StrCopy(bot->avatar, sizeof(bot->avatar), JsonStr(entry, "avatar"));
        bot->trophies = JsonInt(entry, "trophies");
    }
}

static void PvpSquadsParse(PvpTable *t, const cJSON *root) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive((cJSON *)root, "squad_pool")) {
        if (t->squad_count == PVP_SQUAD_CAPACITY) TraceLog(LOG_FATAL, "Pvp: more than %d squads", PVP_SQUAD_CAPACITY);
        PvpSquad *squad = &t->squads[t->squad_count++];
        squad->rating = JsonInt(entry, "rating");
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
        }
    }
}

const PvpTable *PvpTableEnsure(void) {
    if (pvp_table.loaded) return &pvp_table;
    cJSON *root = JsonLoadFile(PVP_RULES_PATH);
    PvpRulesParse(&pvp_table.rules, root);
    PvpEventsParse(&pvp_table.events, root);
    PvpBotsParse(&pvp_table, root);
    PvpSquadsParse(&pvp_table, root);
    cJSON_Delete(root);
    if (pvp_table.bot_count == 0 || pvp_table.squad_count == 0) TraceLog(LOG_FATAL, "Pvp: empty ladder");
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

int PvpTargetRating(int level, int trophy_lead) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    long long base = r->rating_base + (long long)r->rating_per_level * level +
                     (long long)r->rating_per_level_squared * level * level;
    int shift = trophy_lead / r->trophies_per_shift_pct;
    shift = Clamp(shift, -r->rating_max_shift_pct, r->rating_max_shift_pct);
    return (int)(base * (100 + shift) / 100);
}

static int PvpPickSquad(int rating, unsigned seed) {
    const PvpTable *t = PvpTableEnsure();
    int nearest = 0;
    for (int i = 1; i < t->squad_count; i++)
        if (abs(t->squads[i].rating - rating) < abs(t->squads[nearest].rating - rating)) nearest = i;
    int candidates = t->rules.squad_candidates;
    int first = Clamp(nearest - candidates / 2, 0, t->squad_count > candidates ? t->squad_count - candidates : 0);
    int span = candidates < t->squad_count ? candidates : t->squad_count;
    return first + PvpRoll(seed, PVP_SQUAD_SALT, span);
}

static void PvpPickRunes(PvpOpponent *o) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    o->rune_count = 0;
    if (o->level < r->first_rune_level) return;
    int wanted = 1 + (o->level - r->first_rune_level) / r->levels_per_rune;
    if (wanted > RUNE_SLOT_COUNT) wanted = RUNE_SLOT_COUNT;
    int rarity_cap = 1 + o->level / r->levels_per_rarity;
    if (rarity_cap > r->max_rune_rarity) rarity_cap = r->max_rune_rarity;

    int pool[RUNE_CATALOG_CAPACITY], pool_count = 0;
    for (int def = 0; def < RuneCatalogCount(); def++) {
        const RuneDef *rune = RuneDefAt(def);
        if (rune->rarity <= rarity_cap && RuneDefRollable(rune)) pool[pool_count++] = def;
    }
    for (int i = 0; i < wanted && pool_count > 0; i++) {
        int pick = PvpRoll(o->seed, PVP_RUNE_SALT + (unsigned)i, pool_count);
        o->rune_defs[o->rune_count++] = pool[pick];
        pool[pick] = pool[--pool_count];
    }
}

static void PvpOpponentDerive(PvpOpponent *o, int player_trophies) {
    o->gain = PvpTrophyGain(player_trophies, o->trophies);
    o->loss = PvpTrophyLoss(player_trophies, o->trophies);
    PvpPickRunes(o);
}

PvpOpponent PvpOpponentFromBot(int bot, int level, int trophies, long long day, unsigned seed) {
    const PvpTable *t = PvpTableEnsure();
    PvpOpponent o = {.bot = bot, .seed = seed ? seed : 1u};
    o.trophies = PvpBotTrophies(bot, day);
    int spread = t->rules.level_spread;
    o.level = Clamp(level + PvpRoll(o.seed, PVP_LEVEL_SALT, spread * 2 + 1) - spread, 1, PVP_LOCAL_MAX_LEVEL);
    o.squad = PvpPickSquad(PvpTargetRating(level, o.trophies - trophies), o.seed);
    PvpOpponentDerive(&o, trophies);
    return o;
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

const char *PvpOpponentName(const PvpOpponent *o) {
    return PvpTableEnsure()->bots[o->bot].name;
}

void PvpOpponentRoster(BattleRoster *roster, const PvpOpponent *o) {
    memset(roster, 0, sizeof(*roster));
    cJSON *catalog_root = JsonLoadFile(PVP_CATALOG_PATH);
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    const PvpSquad *squad = &PvpTableEnsure()->squads[o->squad];
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const PvpSquadCell *cell = &squad->cells[slot];
        if (!cell->present) continue;
        cJSON *def = CatalogFindCharacter(catalog, cell->id);
        if (!def) TraceLog(LOG_FATAL, "Pvp: unit %s is not in the catalog", cell->id);
        CharacterParse(def, &roster->chars[slot], &roster->abilities[slot]);
        roster->chars[slot].tier = cell->tier;
        roster->chars[slot].level = cell->level;
        roster->present[slot] = true;
    }
    cJSON_Delete(catalog_root);
    for (int i = 0; i < o->rune_count; i++) roster->rune_defs[roster->rune_def_count++] = o->rune_defs[i];
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
        .bot = o->bot,
        .squad = o->squad,
        .level = o->level,
        .opponent_trophies = o->trophies,
        .trophy_delta = delta,
        .gold = PvpGoldReward(o, won),
        .xp = PvpXpReward(o, won),
        .seed = o->seed,
        .won = won,
    };
    PvpHistoryPush(pvp, &battle);
    return battle;
}

static void PvpRecordDefense(PlayerPvp *pvp, const PvpOpponent *o, bool won, long long at) {
    const PvpRules *r = &PvpTableEnsure()->rules;
    int stake = o->loss * r->defense_loss_share_pct / 100;
    int delta = PvpApplyTrophies(pvp, won ? stake : -stake, at);
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

int PvpSimulateDefense(PlayerPvp *pvp, int level, long long now) {
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
        PvpOpponent o = PvpOpponentGenerate(level, pvp->trophies, PvpDay(at), seed);
        PvpRecordDefense(pvp, &o, PvpRoll(seed, PVP_DEFENSE_SALT, 100) < r->defense_win_pct, at);
    }
    pvp->defense_checked_at += due * r->attack_interval_seconds;
    return attacks;
}

bool PvpCanRevenge(const PlayerPvpBattle *battle) {
    return battle->defense && !battle->won && !battle->revenged;
}

long long PvpEventCycle(long long now) {
    return now / PvpTableEnsure()->events.duration_seconds;
}

const PvpEvent *PvpEventForCycle(long long cycle) {
    const PvpEventRules *e = &PvpTableEnsure()->events;
    return &e->catalog[cycle % e->catalog_count];
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

PvpOpponent PvpEventOpponentGenerate(int level, int trophies, int banners, long long now, unsigned seed) {
    long long elapsed = now % PvpTableEnsure()->events.duration_seconds;
    int bot = PvpPickEventBot(banners, PvpEventCycle(now), elapsed, seed ? seed : 1u);
    return PvpOpponentFromBot(bot, level, trophies, PvpDay(now), seed);
}

int PvpEventRankAt(int banners, long long cycle, long long elapsed) {
    const PvpTable *t = PvpTableEnsure();
    int rank = 1;
    for (int bot = 0; bot < t->bot_count; bot++) rank += PvpBotBanners(bot, cycle, elapsed) > banners;
    return rank;
}

const PvpEventRankReward *PvpEventRewardForRank(const PvpEvent *event, int rank) {
    for (int i = 0; i < event->rank_reward_count; i++)
        if (rank >= event->rank_rewards[i].first && rank <= event->rank_rewards[i].last) return &event->rank_rewards[i];
    return NULL;
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

static void PvpEventFinish(PlayerPvp *pvp) {
    const PlayerPvpEvent *event = &pvp->event;
    if (event->banners <= 0) return;
    int duration = PvpTableEnsure()->events.duration_seconds;
    PlayerPvpEventResult result = {.cycle = event->cycle, .banners = event->banners};
    result.rank = PvpEventRankAt(event->banners, event->cycle, duration);
    const PvpEventRankReward *reward = PvpEventRewardForRank(PvpEventForCycle(event->cycle), result.rank);
    if (reward) {
        result.gold = reward->reward.gold;
        result.gems = reward->reward.gems;
    }
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
        .bot = o->bot,
        .squad = o->squad,
        .level = o->level,
        .opponent_trophies = o->trophies,
        .gold = PvpGoldReward(o, won),
        .xp = PvpXpReward(o, won),
        .banners = PvpEventBannerReward(o, pvp->event.banners, won),
        .seed = o->seed,
        .won = won,
    };
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

static int PvpRankCompare(const void *a, const void *b) {
    const PvpRankEntry *left = a, *right = b;
    if (left->trophies != right->trophies) return right->trophies - left->trophies;
    return left->bot - right->bot;
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
