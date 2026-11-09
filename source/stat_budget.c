#define STAT_BUDGET_PATH "data/stat_budget.json"
#define STAT_BUDGET_VERSION 1

typedef struct {
    int budget;
    int health_per_power;
    int giant_budget_pct;
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
    if (b->budget <= 0 || b->health_per_power <= 0 || b->giant_budget_pct <= 0)
        TraceLog(
            LOG_FATAL, "StatBudget: %s needs a positive budget, health_per_power and giant_budget_pct", STAT_BUDGET_PATH
        );
    cJSON_Delete(root);
    b->loaded = true;
    return b;
}

static long long StatBudgetCost(const StatBudget *b, int power, int health) {
    return (long long)power * b->health_per_power + health;
}

static int StatBudgetScale(int stat, long long target, long long cost) {
    return (int)(((long long)stat * target + cost / 2) / cost);
}

void StatBudgetApply(const StatBudget *b, bool giant, int *power, int *health) {
    long long cost = StatBudgetCost(b, *power, *health);
    if (cost <= 0) return;
    long long target = giant ? (long long)b->budget * b->giant_budget_pct / 100 : b->budget;
    *power = StatBudgetScale(*power, target, cost);
    *health = StatBudgetScale(*health, target, cost);
}
