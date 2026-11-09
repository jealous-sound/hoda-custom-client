#define STAT_BUDGET_PATH "data/stat_budget.json"
#define STAT_BUDGET_VERSION 2
#define STAT_BUDGET_PATTERN_COUNT (TARGET_ALL + 1)
#define STAT_BUDGET_AURA_STAT_COUNT 2

static const char *const STAT_BUDGET_PATTERN_NAMES[STAT_BUDGET_PATTERN_COUNT] = {
    [TARGET_ONE] = "One",
    [TARGET_ROW] = "Row",
    [TARGET_COLUMN] = "Column",
    [TARGET_ALL] = "All",
};

typedef struct {
    int budget;
    int health_per_power;
    int giant_budget_pct;
    int pattern_power_cost_pct[STAT_BUDGET_PATTERN_COUNT];
    int aura_cost_pct;
    int self_aura_share_pct;
    int aura_matching_allies;
    bool loaded;
} StatBudget;

static StatBudget stat_budget_table;

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
    const cJSON *patterns = cJSON_GetObjectItemCaseSensitive(root, "pattern_power_cost_pct");
    bool valid = b->budget > 0 && b->health_per_power > 0 && b->giant_budget_pct > 0 && b->aura_cost_pct >= 0 &&
                 b->self_aura_share_pct >= 0 && b->aura_matching_allies >= 0;
    for (int pattern = 0; pattern < STAT_BUDGET_PATTERN_COUNT; pattern++) {
        b->pattern_power_cost_pct[pattern] = JsonInt(patterns, STAT_BUDGET_PATTERN_NAMES[pattern]);
        valid = valid && b->pattern_power_cost_pct[pattern] > 0;
    }
    if (!valid) TraceLog(LOG_FATAL, "StatBudget: %s holds a missing or non-positive rule", STAT_BUDGET_PATH);
    cJSON_Delete(root);
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
    return points > 0 ? points : 0;
}

static long long StatBudgetCostPct(const StatBudget *b, TargetPattern pattern, int power, int health) {
    return (long long)power * b->health_per_power * b->pattern_power_cost_pct[pattern] + (long long)health * 100;
}

static int StatBudgetAuraChargePct(const StatBudget *b, int aura_points) {
    return 100 + aura_points * b->aura_cost_pct / 100;
}

void StatBudgetApplyKit(
    const StatBudget *b, bool giant, TargetPattern pattern, int aura_points, int *power, int *health
) {
    long long cost_pct = StatBudgetCostPct(b, pattern, *power, *health);
    if (cost_pct <= 0) return;
    long long numerator = (long long)b->budget * (giant ? b->giant_budget_pct : 100) * 100;
    long long denominator = cost_pct * StatBudgetAuraChargePct(b, aura_points);
    *power = (int)((*power * numerator + denominator / 2) / denominator);
    *health = (int)((*health * numerator + denominator / 2) / denominator);
}

void StatBudgetApply(const StatBudget *b, const Character *ch, int *power, int *health) {
    PassiveManifestEnsure();
    TargetPattern pattern = ch->ability ? ch->ability->target : TARGET_ONE;
    int aura_points = StatBudgetAuraPoints(b, PassiveManifestFind(CharacterDataId(ch)));
    StatBudgetApplyKit(b, ch->giant, pattern, aura_points, power, health);
}
