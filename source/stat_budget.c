#define STAT_BUDGET_PATH "data/stat_budget.json"
#define STAT_BUDGET_VERSION 5
#define STAT_BUDGET_PATTERN_COUNT (TARGET_ALL + 1)
#define STAT_BUDGET_RARITY_COUNT (RARITY_MYTHIC + 1)
#define STAT_BUDGET_NEUTRAL_RARITY_PCT 100
#define STAT_BUDGET_AURA_STAT_COUNT 2
#define STAT_BUDGET_HEROES_PATH "data/stat_budget_heroes.json"
#define STAT_BUDGET_HEROES_VERSION 1
#define STAT_BUDGET_HERO_CAPACITY 1024
#define STAT_BUDGET_NEUTRAL_HERO_PCT 100
#define STAT_BUDGET_HERO_MIN_PCT 33
#define STAT_BUDGET_HERO_MAX_PCT 300
#define STAT_BUDGET_FULL_SHARE_PCT 100
#define STAT_BUDGET_WORTH_PER_PCT 100
#define STAT_BUDGET_WORTH_OF_A_HERO (100 * STAT_BUDGET_WORTH_PER_PCT)

static const char *const STAT_BUDGET_PATTERN_NAMES[STAT_BUDGET_PATTERN_COUNT] = {
    [TARGET_ONE] = "One",
    [TARGET_ROW] = "Row",
    [TARGET_COLUMN] = "Column",
    [TARGET_ALL] = "All",
};

static const char *const STAT_BUDGET_RARITY_NAMES[STAT_BUDGET_RARITY_COUNT] = {
    [RARITY_COMMON] = "Common",
    [RARITY_UNCOMMON] = "Uncommon",
    [RARITY_RARE] = "Rare",
    [RARITY_EPIC] = "Epic",
    [RARITY_LEGENDARY] = "Legendary",
    [RARITY_MYTHIC] = "Mythic",
};

typedef struct {
    char id[64];
    int budget_pct;
} StatBudgetHero;

typedef struct {
    bool giant;
    TargetPattern pattern;
    int aura_points;
    int hero_pct;
    int rarity_pct;
} StatBudgetKit;

typedef struct {
    int power, health;
} StatBudgetStats;

typedef struct {
    int budget;
    int health_per_power;
    int giant_budget_pct;
    int pattern_power_cost_pct[STAT_BUDGET_PATTERN_COUNT];
    int aura_cost_pct;
    int self_aura_share_pct;
    int aura_matching_allies;
    int aura_cap_pct;
    int gear_cap_pct;
    int formation_bonus_pct_per_hero;
    int changer_cap_pct;
    int rune_stat_share_pct;
    int rune_initiative_share_pct;
    int flat_resist_per_pct;
    int gear_item_cap_pct;
    int gear_trigger_cap_pct;
    int rarity_budget_pct[STAT_BUDGET_RARITY_COUNT];
    StatBudgetHero heroes[STAT_BUDGET_HERO_CAPACITY];
    int hero_count;
    bool loaded;
} StatBudget;

static StatBudget stat_budget_table;

static void StatBudgetLoadHeroes(StatBudget *b) {
    cJSON *root = JsonLoadFile(STAT_BUDGET_HEROES_PATH);
    if (JsonInt(root, "version") != STAT_BUDGET_HEROES_VERSION)
        TraceLog(LOG_FATAL, "StatBudget: %s has version %d", STAT_BUDGET_HEROES_PATH, JsonInt(root, "version"));
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(root, "heroes")) {
        if (b->hero_count == STAT_BUDGET_HERO_CAPACITY)
            TraceLog(LOG_FATAL, "StatBudget: more than %d heroes in %s", b->hero_count, STAT_BUDGET_HEROES_PATH);
        StatBudgetHero *hero = &b->heroes[b->hero_count++];
        StrCopy(hero->id, sizeof(hero->id), entry->string);
        hero->budget_pct = entry->valueint;
        bool sorted = b->hero_count == 1 || strcmp(b->heroes[b->hero_count - 2].id, hero->id) < 0;
        bool bounded = hero->budget_pct >= STAT_BUDGET_HERO_MIN_PCT && hero->budget_pct <= STAT_BUDGET_HERO_MAX_PCT;
        if (!sorted || !bounded)
            TraceLog(
                LOG_FATAL,
                "StatBudget: %s needs sorted ids and percents from %d to %d",
                STAT_BUDGET_HEROES_PATH,
                STAT_BUDGET_HERO_MIN_PCT,
                STAT_BUDGET_HERO_MAX_PCT
            );
    }
    cJSON_Delete(root);
}

int StatBudgetHeroPct(const StatBudget *b, const char *id) {
    int lo = 0, hi = b->hero_count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(id, b->heroes[mid].id);
        if (cmp == 0) return b->heroes[mid].budget_pct;
        if (cmp < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return STAT_BUDGET_NEUTRAL_HERO_PCT;
}

const StatBudget *StatBudgetEnsure(void) {
    StatBudget *b = &stat_budget_table;
    if (b->loaded) return b;
    cJSON *root = JsonLoadFile(STAT_BUDGET_PATH);
    if (JsonInt(root, "version") != STAT_BUDGET_VERSION)
        TraceLog(LOG_FATAL, "StatBudget: %s has version %d", STAT_BUDGET_PATH, JsonInt(root, "version"));
    b->budget = JsonInt(root, "budget");
    b->health_per_power = JsonInt(root, "health_per_power");
    b->giant_budget_pct = JsonInt(root, "giant_budget_pct");
    b->aura_cost_pct = JsonInt(root, "aura_cost_pct");
    b->self_aura_share_pct = JsonInt(root, "self_aura_share_pct");
    b->aura_matching_allies = JsonInt(root, "aura_matching_allies");
    b->aura_cap_pct = JsonInt(root, "aura_cap_pct");
    b->gear_cap_pct = JsonInt(root, "gear_cap_pct");
    b->formation_bonus_pct_per_hero = JsonInt(root, "formation_bonus_pct_per_hero");
    b->changer_cap_pct = JsonInt(root, "changer_cap_pct");
    b->rune_stat_share_pct = JsonInt(root, "rune_stat_share_pct");
    b->rune_initiative_share_pct = JsonInt(root, "rune_initiative_share_pct");
    b->flat_resist_per_pct = JsonInt(root, "flat_resist_per_pct");
    b->gear_item_cap_pct = JsonInt(root, "gear_item_cap_pct");
    b->gear_trigger_cap_pct = JsonInt(root, "gear_trigger_cap_pct");
    const cJSON *patterns = cJSON_GetObjectItemCaseSensitive(root, "pattern_power_cost_pct");
    const cJSON *rarities = cJSON_GetObjectItemCaseSensitive(root, "rarity_budget_pct");
    bool valid = b->budget > 0 && b->health_per_power > 0 && b->giant_budget_pct > 0 && b->aura_cost_pct >= 0 &&
                 b->self_aura_share_pct >= 0 && b->aura_matching_allies >= 0 && b->aura_cap_pct > 0 &&
                 b->gear_cap_pct >= 0 && b->formation_bonus_pct_per_hero >= 0 && b->changer_cap_pct >= 0 &&
                 b->rune_stat_share_pct >= 0 && b->rune_initiative_share_pct >= 0 && b->flat_resist_per_pct > 0 &&
                 b->gear_item_cap_pct >= 0 && b->gear_trigger_cap_pct >= 0;
    for (int pattern = 0; pattern < STAT_BUDGET_PATTERN_COUNT; pattern++) {
        b->pattern_power_cost_pct[pattern] = JsonInt(patterns, STAT_BUDGET_PATTERN_NAMES[pattern]);
        valid = valid && b->pattern_power_cost_pct[pattern] > 0;
    }
    for (int rarity = RARITY_COMMON; rarity < STAT_BUDGET_RARITY_COUNT; rarity++) {
        b->rarity_budget_pct[rarity] = JsonInt(rarities, STAT_BUDGET_RARITY_NAMES[rarity]);
        valid = valid && b->rarity_budget_pct[rarity] > 0;
    }
    if (!valid) TraceLog(LOG_FATAL, "StatBudget: %s holds a missing or non-positive rule", STAT_BUDGET_PATH);
    cJSON_Delete(root);
    StatBudgetLoadHeroes(b);
    b->loaded = true;
    return b;
}

static int StatBudgetEffectStatCount(const PassiveEffect *effect) {
    if (effect->kind == PASSIVE_BONUS_DAMAGE_PERCENT) return 1;
    return ((effect->stats & PASSIVE_STAT_HEALTH) != 0) + ((effect->stats & PASSIVE_STAT_POWER) != 0);
}

static int StatBudgetEffectPoints(const StatBudget *b, const PassiveEffect *effect) {
    bool per_ally = effect->kind == PASSIVE_STAT_PERCENT_PER_MATCHING_ALLY;
    if (effect->kind != PASSIVE_STAT_PERCENT && effect->kind != PASSIVE_BONUS_DAMAGE_PERCENT && !per_ally) return 0;
    int points = (int)lroundf(effect->value_pct) * StatBudgetEffectStatCount(effect);
    if (per_ally) points *= b->aura_matching_allies;
    if (effect->recipient.scope == PASSIVE_SCOPE_ENEMIES) return -points;
    if (effect->recipient.scope == PASSIVE_SCOPE_SELF) return points * b->self_aura_share_pct / 100;
    return points;
}

int StatBudgetAuraPoints(const StatBudget *b, const PassiveCharacter *passives) {
    if (!passives) return 0;
    int points = 0;
    for (int i = 0; i < passives->effect_count; i++) points += StatBudgetEffectPoints(b, &passives->effects[i]);
    points /= STAT_BUDGET_AURA_STAT_COUNT;
    if (points > b->aura_cap_pct) return b->aura_cap_pct;
    return points > 0 ? points : 0;
}

static long long StatBudgetCostPct(const StatBudget *b, TargetPattern pattern, int power, int health) {
    return (long long)power * b->health_per_power * b->pattern_power_cost_pct[pattern] + (long long)health * 100;
}

static int StatBudgetAuraChargePct(const StatBudget *b, int aura_points) {
    return 100 + aura_points * b->aura_cost_pct / 100;
}

int StatBudgetPotencyCutPct(const StatBudget *b, const Character *ch) {
    int hero_pct = StatBudgetHeroPct(b, ch->id);
    return hero_pct < STAT_BUDGET_NEUTRAL_HERO_PCT ? STAT_BUDGET_NEUTRAL_HERO_PCT - hero_pct : 0;
}

float StatBudgetCappedAuraPct(const StatBudget *b, float aura_pct) {
    return aura_pct > (float)b->aura_cap_pct ? (float)b->aura_cap_pct : aura_pct;
}

float StatBudgetCappedGearPct(const StatBudget *b, float gear_pct) {
    float cap = (float)b->gear_cap_pct;
    if (gear_pct < -cap) return -cap;
    return gear_pct > cap ? cap : gear_pct;
}

int StatBudgetGearItemCap(const StatBudget *b) {
    return b->gear_item_cap_pct;
}

int StatBudgetGearTriggerAllowance(const StatBudget *b) {
    return b->gear_trigger_cap_pct * STAT_BUDGET_WORTH_PER_PCT;
}

long long StatBudgetShareOfStat(long long amount, int stat) {
    if (stat <= 0) return 0;
    return (amount < 0 ? -amount : amount) * STAT_BUDGET_WORTH_OF_A_HERO / stat;
}

long long StatBudgetWorthOfPoints(long long points) {
    return (points < 0 ? -points : points) * STAT_BUDGET_WORTH_PER_PCT;
}

int StatBudgetFlatResistPct(const StatBudget *b, int flat_resist) {
    return flat_resist / b->flat_resist_per_pct;
}

float StatBudgetFormationBonus(const StatBudget *b) {
    return (float)b->formation_bonus_pct_per_hero / 100.0f;
}

int StatBudgetCappedChangerPct(const StatBudget *b, int changer_pct) {
    return changer_pct > b->changer_cap_pct ? b->changer_cap_pct : changer_pct;
}

static int StatBudgetRuneShare(int rune_value, int share_pct) {
    return (rune_value * share_pct + STAT_BUDGET_FULL_SHARE_PCT / 2) / STAT_BUDGET_FULL_SHARE_PCT;
}

void StatBudgetLimitRuneBonus(const StatBudget *b, RuneBonus *bonus) {
    bonus->damage_shield = 0;
    bonus->damage_resist_pct = StatBudgetRuneShare(bonus->damage_resist_pct, b->rune_stat_share_pct);
}

int StatBudgetRuneStat(const StatBudget *b, int stat, int rune_pct) {
    if (stat <= 0 || rune_pct <= 0) return stat;
    long long whole = 100LL * STAT_BUDGET_FULL_SHARE_PCT;
    return (int)((stat * (whole + (long long)rune_pct * b->rune_stat_share_pct) + whole / 2) / whole);
}

int StatBudgetRuneInitiative(const StatBudget *b, int initiative) {
    return StatBudgetRuneShare(initiative, b->rune_initiative_share_pct);
}

int StatBudgetRarityPct(const StatBudget *b, Rarity rarity) {
    bool listed = rarity >= RARITY_COMMON && rarity < STAT_BUDGET_RARITY_COUNT;
    return listed ? b->rarity_budget_pct[rarity] : STAT_BUDGET_NEUTRAL_RARITY_PCT;
}

static Tier StatBudgetGrownTier(const Character *ch) {
    return ch->has_tier[ch->max_tier] ? ch->max_tier : ch->tier;
}

static int StatBudgetGrownLevel(const Character *ch, Tier tier) {
    int level = ch->stats[tier].max_level - (ch->ascended ? GEAR_ASCENSION_BONUS_LEVELS : 0);
    return level > 1 ? level : 1;
}

StatBudgetStats StatBudgetGrownStats(const Character *ch) {
    Tier tier = StatBudgetGrownTier(ch);
    const TierStats *st = &ch->stats[tier];
    int steps = StatBudgetGrownLevel(ch, tier) - 1;
    return (StatBudgetStats){
        .power = st->base_power + steps * st->power_per_level,
        .health = st->base_health + steps * st->health_per_level,
    };
}

void StatBudgetApplyKit(const StatBudget *b, const StatBudgetKit *kit, StatBudgetStats grown, int *power, int *health) {
    long long cost_pct = StatBudgetCostPct(b, kit->pattern, grown.power, grown.health);
    if (cost_pct <= 0) return;
    long long numerator =
        (long long)b->budget * (kit->giant ? b->giant_budget_pct : 100) * kit->hero_pct * kit->rarity_pct;
    long long denominator = cost_pct * StatBudgetAuraChargePct(b, kit->aura_points) * STAT_BUDGET_NEUTRAL_RARITY_PCT;
    *power = (int)((*power * numerator + denominator / 2) / denominator);
    *health = (int)((*health * numerator + denominator / 2) / denominator);
}

void StatBudgetApply(const StatBudget *b, const Character *ch, int *power, int *health) {
    PassiveManifestEnsure();
    StatBudgetKit kit = {
        .giant = ch->giant,
        .pattern = ch->ability ? ch->ability->target : TARGET_ONE,
        .aura_points = StatBudgetAuraPoints(b, PassiveManifestFind(CharacterDataId(ch))),
        .hero_pct = StatBudgetHeroPct(b, ch->id),
        .rarity_pct = StatBudgetRarityPct(b, ch->rarity),
    };
    StatBudgetApplyKit(b, &kit, StatBudgetGrownStats(ch), power, health);
}
