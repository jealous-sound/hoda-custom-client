#define RENOWN_PATH "data/renown.json"
#define RENOWN_VERSION 1
#define RENOWN_NONE (-1)

typedef enum {
    RENOWN_PERK_POWER,
    RENOWN_PERK_HEALTH,
    RENOWN_PERK_CRIT,
    RENOWN_PERK_GOLD,
    RENOWN_PERK_VOUCHER,
    RENOWN_PERK_KEY,
    RENOWN_PERK_HERO_XP,
    RENOWN_PERK_ENERGY,
    RENOWN_PERK_STAMINA,
    RENOWN_PERK_COUNT,
} RenownPerk;

typedef enum {
    RENOWN_TIER_NONE,
    RENOWN_TIER_BRONZE,
    RENOWN_TIER_SILVER,
    RENOWN_TIER_GOLD,
    RENOWN_TIER_CRIMSON,
    RENOWN_TIER_ROYAL,
    RENOWN_TIER_COUNT,
} RenownTier;

typedef struct {
    bool loaded;
    int levels_per_rank;
    int rank_gems;
    int perk_max_rank;
    int tier_ranks[RENOWN_TIER_COUNT];
    int perk_values[RENOWN_PERK_COUNT];
} RenownCatalog;

typedef struct {
    int perks[RENOWN_PERK_COUNT];
} PlayerRenown;

typedef struct {
    int power_pct;
    int health_pct;
    int crit_permille;
} RenownBattleBonus;

static RenownCatalog renown_catalog;

static const char *const RENOWN_PERK_IDS[RENOWN_PERK_COUNT] = {
    [RENOWN_PERK_POWER] = "power_pct",
    [RENOWN_PERK_HEALTH] = "health_pct",
    [RENOWN_PERK_CRIT] = "crit_permille",
    [RENOWN_PERK_GOLD] = "gold_pct",
    [RENOWN_PERK_VOUCHER] = "voucher_pct",
    [RENOWN_PERK_KEY] = "key_pct",
    [RENOWN_PERK_HERO_XP] = "hero_xp_pct",
    [RENOWN_PERK_ENERGY] = "energy_max",
    [RENOWN_PERK_STAMINA] = "stamina_max",
};

static const char *const RENOWN_TIER_IDS[RENOWN_TIER_COUNT] = {
    [RENOWN_TIER_NONE] = "",
    [RENOWN_TIER_BRONZE] = "bronze",
    [RENOWN_TIER_SILVER] = "silver",
    [RENOWN_TIER_GOLD] = "gold",
    [RENOWN_TIER_CRIMSON] = "crimson",
    [RENOWN_TIER_ROYAL] = "royal",
};

int ProgressionVirtualLevel(int experience);

static bool RenownJsonPositive(const cJSON *json, const char *key, int *out) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < 1.0 || item->valuedouble > INT_MAX ||
        item->valuedouble != (double)item->valueint)
        return false;
    *out = item->valueint;
    return true;
}

bool RenownCatalogLoad(const char *path) {
    RenownCatalog *c = &renown_catalog;
    memset(c, 0, sizeof(*c));
    char *text = FileExists(path) ? LoadFileText(path) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    if (text) UnloadFileText(text);
    bool ok = root && JsonInt(root, "version") == RENOWN_VERSION &&
              RenownJsonPositive(root, "levels_per_rank", &c->levels_per_rank) &&
              RenownJsonPositive(root, "rank_gems", &c->rank_gems) &&
              RenownJsonPositive(root, "perk_max_rank", &c->perk_max_rank);
    const cJSON *perks = cJSON_GetObjectItemCaseSensitive(root, "perks");
    for (int perk = 0; ok && perk < RENOWN_PERK_COUNT; perk++)
        ok = RenownJsonPositive(perks, RENOWN_PERK_IDS[perk], &c->perk_values[perk]);
    const cJSON *tiers = cJSON_GetObjectItemCaseSensitive(root, "tier_ranks");
    for (int tier = RENOWN_TIER_BRONZE; ok && tier < RENOWN_TIER_COUNT; tier++)
        ok = RenownJsonPositive(tiers, RENOWN_TIER_IDS[tier], &c->tier_ranks[tier]) &&
             c->tier_ranks[tier] > c->tier_ranks[tier - 1];
    cJSON_Delete(root);
    if (!ok) {
        TraceLog(LOG_WARNING, "Renown: %s is missing or invalid, renown is off", path);
        memset(c, 0, sizeof(*c));
    }
    c->loaded = ok;
    return ok;
}

bool RenownAvailable(void) {
    return renown_catalog.loaded;
}

int RenownLevelsPerRank(void) {
    return renown_catalog.levels_per_rank;
}

int RenownRankGems(void) {
    return renown_catalog.rank_gems;
}

int RenownPerkMaxRank(void) {
    return renown_catalog.perk_max_rank;
}

const char *RenownPerkId(RenownPerk perk) {
    return perk >= 0 && perk < RENOWN_PERK_COUNT ? RENOWN_PERK_IDS[perk] : "";
}

int RenownPerkFromId(const char *id) {
    for (int perk = 0; id && perk < RENOWN_PERK_COUNT; perk++)
        if (!strcmp(id, RENOWN_PERK_IDS[perk])) return perk;
    return RENOWN_NONE;
}

int RenownRankForVirtualLevel(int virtual_level) {
    return renown_catalog.loaded ? virtual_level / renown_catalog.levels_per_rank : 0;
}

int RenownRank(int experience) {
    return RenownRankForVirtualLevel(ProgressionVirtualLevel(experience));
}

int RenownRankFirstLevel(int rank) {
    if (!renown_catalog.loaded || rank <= 0) return 1;
    return rank > INT_MAX / renown_catalog.levels_per_rank ? INT_MAX : rank * renown_catalog.levels_per_rank;
}

RenownTier RenownTierForRank(int rank) {
    RenownTier tier = RENOWN_TIER_NONE;
    for (int next = RENOWN_TIER_BRONZE; next < RENOWN_TIER_COUNT; next++)
        if (renown_catalog.loaded && rank >= renown_catalog.tier_ranks[next]) tier = (RenownTier)next;
    return tier;
}

int RenownTierFirstRank(RenownTier tier) {
    return tier > RENOWN_TIER_NONE && tier < RENOWN_TIER_COUNT ? renown_catalog.tier_ranks[tier] : 0;
}

int RenownPerkRank(const PlayerRenown *renown, RenownPerk perk) {
    return perk >= 0 && perk < RENOWN_PERK_COUNT ? renown->perks[perk] : 0;
}

int RenownPerkValueAtRank(RenownPerk perk, int perk_rank) {
    if (perk < 0 || perk >= RENOWN_PERK_COUNT || perk_rank <= 0) return 0;
    long long value = (long long)perk_rank * renown_catalog.perk_values[perk];
    return value > INT_MAX ? INT_MAX : (int)value;
}

int RenownPerkValue(const PlayerRenown *renown, RenownPerk perk) {
    return RenownPerkValueAtRank(perk, RenownPerkRank(renown, perk));
}

int RenownPointsSpent(const PlayerRenown *renown) {
    long long spent = 0;
    for (int perk = 0; perk < RENOWN_PERK_COUNT; perk++) spent += renown->perks[perk];
    return spent > INT_MAX ? INT_MAX : (int)spent;
}

int RenownPointsFree(const PlayerRenown *renown, int experience) {
    int free_points = RenownRank(experience) - RenownPointsSpent(renown);
    return free_points > 0 ? free_points : 0;
}

bool RenownOverspent(const PlayerRenown *renown, int experience) {
    return renown_catalog.loaded && RenownPointsSpent(renown) > RenownRank(experience);
}

bool RenownPerkMaxed(const PlayerRenown *renown, RenownPerk perk) {
    return RenownPerkRank(renown, perk) >= renown_catalog.perk_max_rank;
}

bool RenownCanUpgrade(const PlayerRenown *renown, int experience, RenownPerk perk) {
    return renown_catalog.loaded && perk >= 0 && perk < RENOWN_PERK_COUNT && !RenownPerkMaxed(renown, perk) &&
           RenownPointsFree(renown, experience) > 0;
}

RenownBattleBonus RenownBattleBonusOf(const PlayerRenown *renown) {
    return (RenownBattleBonus){
        .power_pct = RenownPerkValue(renown, RENOWN_PERK_POWER),
        .health_pct = RenownPerkValue(renown, RENOWN_PERK_HEALTH),
        .crit_permille = RenownPerkValue(renown, RENOWN_PERK_CRIT),
    };
}

bool RenownBattleBonusActive(const RenownBattleBonus *bonus) {
    return bonus->power_pct > 0 || bonus->health_pct > 0 || bonus->crit_permille > 0;
}

cJSON *RenownBattleBonusJson(const RenownBattleBonus *bonus) {
    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;
    cJSON_AddNumberToObject(json, "power_pct", bonus->power_pct);
    cJSON_AddNumberToObject(json, "health_pct", bonus->health_pct);
    cJSON_AddNumberToObject(json, "crit_permille", bonus->crit_permille);
    return json;
}

bool RenownBattleBonusLoad(const cJSON *json, RenownBattleBonus *bonus) {
    *bonus = (RenownBattleBonus){
        .power_pct = JsonInt(json, "power_pct"),
        .health_pct = JsonInt(json, "health_pct"),
        .crit_permille = JsonInt(json, "crit_permille"),
    };
    return bonus->power_pct >= 0 && bonus->health_pct >= 0 && bonus->crit_permille >= 0;
}

cJSON *RenownSave(const PlayerRenown *renown) {
    cJSON *root = cJSON_CreateObject();
    cJSON *perks = root ? cJSON_AddObjectToObject(root, "perks") : NULL;
    bool ok = perks != NULL;
    for (int perk = 0; ok && perk < RENOWN_PERK_COUNT; perk++)
        if (renown->perks[perk] > 0)
            ok = cJSON_AddNumberToObject(perks, RENOWN_PERK_IDS[perk], renown->perks[perk]) != NULL;
    if (ok) return root;
    cJSON_Delete(root);
    return NULL;
}

void RenownLoad(const cJSON *json, PlayerRenown *renown) {
    memset(renown, 0, sizeof(*renown));
    int limit = renown_catalog.loaded ? renown_catalog.perk_max_rank : INT_MAX;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "perks")) {
        int perk = RenownPerkFromId(entry->string);
        if (perk == RENOWN_NONE || !cJSON_IsNumber(entry) || entry->valuedouble < 1.0) continue;
        renown->perks[perk] = entry->valuedouble > limit ? limit : entry->valueint;
    }
}
