#define MISSION_CAPACITY 160
#define MISSION_SERIES_CAPACITY 64
#define MISSION_SERIES_MAX_TIERS 5
#define MISSION_ID_CAP 8
#define MISSION_ICON_CAP 40
#define MISSION_NONE (-1)

typedef enum {
    MISSION_STAT_FIRST_WIN,
    MISSION_STAT_FIRST_BOSS,
    MISSION_STAT_NODE_MASTERED,
    MISSION_STAT_MAP_MASTERED,
    MISSION_STAT_MAPS_UNLOCKED,
    MISSION_STAT_PLAYER_LEVEL,
    MISSION_STAT_HERO_LEVEL,
    MISSION_STAT_TIER_IV_LEVEL,
    MISSION_STAT_COLLECTED,
    MISSION_STAT_SQUAD_FACTION,
    MISSION_STAT_ORIGINS_SQUAD,
    MISSION_STAT_SUMMONED,
    MISSION_STAT_GOLD_EARNED,
    MISSION_STAT_CONSUMED,
    MISSION_STAT_COMBINED,
    MISSION_STAT_RUNES_USED,
    MISSION_STAT_CRITS,
    MISSION_STAT_BATTLE_DAMAGE,
    MISSION_STAT_CHALLENGE_ATTEMPTS,
    MISSION_STAT_BATTLE_DAYS,
    MISSION_STAT_FACTION_HITS,
    MISSION_STAT_COUNT
} MissionStat;

static const char *const MISSION_STAT_NAMES[MISSION_STAT_COUNT] = {
    [MISSION_STAT_FIRST_WIN] = "first_win",
    [MISSION_STAT_FIRST_BOSS] = "first_boss",
    [MISSION_STAT_NODE_MASTERED] = "node_mastered",
    [MISSION_STAT_MAP_MASTERED] = "map_mastered",
    [MISSION_STAT_MAPS_UNLOCKED] = "maps_unlocked",
    [MISSION_STAT_PLAYER_LEVEL] = "player_level",
    [MISSION_STAT_HERO_LEVEL] = "hero_level",
    [MISSION_STAT_TIER_IV_LEVEL] = "tier_iv_level",
    [MISSION_STAT_COLLECTED] = "collected",
    [MISSION_STAT_SQUAD_FACTION] = "squad_faction",
    [MISSION_STAT_ORIGINS_SQUAD] = "origins_squad",
    [MISSION_STAT_SUMMONED] = "summoned",
    [MISSION_STAT_GOLD_EARNED] = "gold_earned",
    [MISSION_STAT_CONSUMED] = "consumed",
    [MISSION_STAT_COMBINED] = "combined",
    [MISSION_STAT_RUNES_USED] = "runes_used",
    [MISSION_STAT_CRITS] = "crits",
    [MISSION_STAT_BATTLE_DAMAGE] = "battle_damage",
    [MISSION_STAT_CHALLENGE_ATTEMPTS] = "challenge_attempts",
    [MISSION_STAT_BATTLE_DAYS] = "battle_days",
    [MISSION_STAT_FACTION_HITS] = "faction_hits",
};

typedef struct {
    char id[MISSION_ID_CAP];
    char icon[MISSION_ICON_CAP];
    int series;
    int tier;
    MissionStat stat;
    long long target;
    int gold;
    int gems;
    int map;
    int rarity;
    int hero_tier;
    int faction;
} MissionDef;

typedef struct {
    MissionDef defs[MISSION_CAPACITY];
    int count;
    int series_tiers[MISSION_SERIES_CAPACITY][MISSION_SERIES_MAX_TIERS];
    int series_tier_count[MISSION_SERIES_CAPACITY];
    int series_count;
} MissionCatalog;

typedef struct {
    long long counters[MISSION_STAT_COUNT];
    long long progress[MISSION_CAPACITY];
    bool claimed[MISSION_CAPACITY];
    bool seen[MISSION_CAPACITY];
    bool gold_baseline_set;
    int gold_baseline;
    int last_battle_day;
} PlayerMissions;

typedef struct {
    int claimed;
    int total;
    int gold_claimed;
    int gold_total;
    int gems_claimed;
    int gems_total;
} MissionTotals;

static MissionCatalog mission_catalog;

static MissionStat MissionStatFromName(const char *name) {
    for (int stat = 0; stat < MISSION_STAT_COUNT; stat++) {
        if (!strcmp(name, MISSION_STAT_NAMES[stat])) return (MissionStat)stat;
    }
    return MISSION_STAT_COUNT;
}

static int MissionJsonIntOr(const cJSON *json, const char *key, int fallback) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    return cJSON_IsNumber(item) ? item->valueint : fallback;
}

static bool MissionCatalogAdd(const cJSON *json) {
    MissionCatalog *c = &mission_catalog;
    const char *id = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "id"));
    const char *stat = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "stat"));
    const char *icon = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "icon"));
    int series = MissionJsonIntOr(json, "series", -1);
    int tier = MissionJsonIntOr(json, "tier", -1);
    if (!id || !stat || c->count >= MISSION_CAPACITY || series < 0 || series >= MISSION_SERIES_CAPACITY || tier < 0 ||
        tier >= MISSION_SERIES_MAX_TIERS)
        return false;
    MissionDef *def = &c->defs[c->count];
    *def = (MissionDef){
        .series = series,
        .tier = tier,
        .stat = MissionStatFromName(stat),
        .target = (long long)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(json, "target")),
        .gold = MissionJsonIntOr(json, "gold", 0),
        .gems = MissionJsonIntOr(json, "gems", 0),
        .map = MissionJsonIntOr(json, "map", 0),
        .rarity = MissionJsonIntOr(json, "rarity", 0),
        .hero_tier = MissionJsonIntOr(json, "hero_tier", 0),
        .faction = MissionJsonIntOr(json, "faction", 0),
    };
    if (def->stat == MISSION_STAT_COUNT || def->target <= 0) return false;
    StrCopy(def->id, sizeof(def->id), id);
    StrCopy(def->icon, sizeof(def->icon), icon ? icon : id);
    c->series_tiers[series][tier] = c->count;
    if (tier >= c->series_tier_count[series]) c->series_tier_count[series] = tier + 1;
    if (series >= c->series_count) c->series_count = series + 1;
    c->count++;
    return true;
}

bool MissionCatalogLoad(const char *path) {
    memset(&mission_catalog, 0, sizeof(mission_catalog));
    char *text = LoadFileText(path);
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    if (text) UnloadFileText(text);
    const cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "missions");
    bool ok = cJSON_IsArray(rows);
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, rows) {
        ok = MissionCatalogAdd(row) && ok;
    }
    cJSON_Delete(root);
    if (!ok) TraceLog(LOG_WARNING, "Missions: invalid table %s", path);
    return ok;
}

int MissionCount(void) {
    return mission_catalog.count;
}

int MissionSeriesCount(void) {
    return mission_catalog.series_count;
}

const MissionDef *MissionAt(int index) {
    return index >= 0 && index < mission_catalog.count ? &mission_catalog.defs[index] : NULL;
}

int MissionIndexById(const char *id) {
    for (int i = 0; i < mission_catalog.count; i++) {
        if (!strcmp(mission_catalog.defs[i].id, id)) return i;
    }
    return MISSION_NONE;
}

int MissionSeriesTierCount(int series) {
    return mission_catalog.series_tier_count[series];
}

int MissionSeriesTierAt(int series, int tier) {
    return mission_catalog.series_tiers[series][tier];
}

bool MissionCompleted(const PlayerMissions *m, int index) {
    return m->progress[index] >= mission_catalog.defs[index].target;
}

bool MissionClaimable(const PlayerMissions *m, int index) {
    return MissionCompleted(m, index) && !m->claimed[index];
}

void MissionRaiseProgress(PlayerMissions *m, int index, long long value) {
    if (value > m->progress[index]) m->progress[index] = value;
}

bool MissionSeriesNeedsCollection(const PlayerMissions *m, int series) {
    for (int tier = 0; tier < MissionSeriesTierCount(series); tier++) {
        if (MissionClaimable(m, MissionSeriesTierAt(series, tier))) return true;
    }
    return false;
}

int MissionSeriesCurrent(const PlayerMissions *m, int series) {
    int tiers = MissionSeriesTierCount(series);
    int first_unclaimed = MISSION_NONE;
    for (int tier = 0; tier < tiers; tier++) {
        int index = MissionSeriesTierAt(series, tier);
        if (MissionClaimable(m, index)) return index;
        if (first_unclaimed == MISSION_NONE && !m->claimed[index]) first_unclaimed = index;
    }
    return first_unclaimed != MISSION_NONE ? first_unclaimed : MissionSeriesTierAt(series, tiers - 1);
}

int MissionUnseenClaimableCount(const PlayerMissions *m) {
    int count = 0;
    for (int i = 0; i < MissionCount(); i++) {
        if (MissionClaimable(m, i) && !m->seen[i]) count++;
    }
    return count;
}

void MissionMarkAllSeen(PlayerMissions *m) {
    for (int i = 0; i < MissionCount(); i++) m->seen[i] = true;
}

MissionTotals MissionComputeTotals(const PlayerMissions *m) {
    MissionTotals totals = {.total = MissionCount()};
    for (int i = 0; i < MissionCount(); i++) {
        const MissionDef *def = MissionAt(i);
        totals.gold_total += def->gold;
        totals.gems_total += def->gems;
        if (!m->claimed[i]) continue;
        totals.claimed++;
        totals.gold_claimed += def->gold;
        totals.gems_claimed += def->gems;
    }
    return totals;
}
