#define CONSUME_TABLES_PATH "data/consume_tables.json"
#define CONSUME_MAX_XP_TABLES 16
#define CONSUME_MAX_XP_ROWS 104
#define CONSUME_ASCENDED_LEVEL_CAP 100
#define CONSUME_HERO_LEVEL_TABLE "unit-level-Default"
#define CONSUME_MAX_PROGRESSIONS 4
#define CONSUME_MAX_PROGRESSION_ROWS 48
#define CONSUME_MAX_ARCHETYPES 8
#define CONSUME_FORTIFY_SLOT_COUNT 10
#define CONSUME_MAX_PROFILES 800
#define CONSUME_MAX_SEEDS 5
#define CONSUME_MAX_RARITY RARITY_MYTHIC
#define CONSUME_BASE_CRIT_CHANCE 1
#define CONSUME_FORTIFY_HEALTH_ATTRIBUTE 1

static const int CONSUME_XP_BASE_STANDARD[CONSUME_MAX_RARITY] = {100, 200, 600, 2000, 5000, 10000};
static const int CONSUME_XP_BASE_GIANT[CONSUME_MAX_RARITY] = {150, 300, 900, 3000, 7500, 15000};
static const float CONSUME_SAME_PROMOTE_GROUP_BONUS = 0.0f;
static const float CONSUME_SAME_FACTION_BONUS = 0.2f;
static const float CONSUME_SAME_TIER_BONUS = 0.0f;
static const float CONSUME_HERO_XP_SHARE_OF_WORTH = 0.1f;

typedef enum {
    CONSUME_UNLOCK_NONE,
    CONSUME_UNLOCK_PROMOTION,
    CONSUME_UNLOCK_FORTIFY,
    CONSUME_UNLOCK_CRIT
} ConsumeUnlock;

typedef struct {
    char name[32];
    int xp[CONSUME_MAX_XP_ROWS];
    int count;
} ConsumeXpTable;

typedef struct {
    char name[16];
    int totals[CONSUME_MAX_PROGRESSION_ROWS][3];
    int count;
} ConsumeProgression;

typedef struct {
    char name[16];
    int attributes[CONSUME_FORTIFY_SLOT_COUNT];
} ConsumeArchetype;

typedef struct {
    char id[64];
    const ConsumeXpTable *level_table;
    const ConsumeXpTable *consume_table;
    const ConsumeProgression *progression;
    const ConsumeArchetype *archetype;
    float worth[TIER_COUNT];
} ConsumeProfile;

typedef struct {
    bool loaded;
    ConsumeXpTable xp_tables[CONSUME_MAX_XP_TABLES];
    int xp_table_count;
    ConsumeProgression progressions[CONSUME_MAX_PROGRESSIONS];
    int progression_count;
    ConsumeArchetype archetypes[CONSUME_MAX_ARCHETYPES];
    int archetype_count;
    ConsumeProfile *profiles;
    int profile_count;
} ConsumeCatalog;

typedef struct {
    int xp, level;
    int consume_xp, consume_level;
    Tier tier;
    int crit_chance;
} ConsumeOutcome;

typedef struct {
    int xp_gain;
    int tier_bonus_pct;
    int faction_bonus_pct;
    int combine_bonus_pct;
    bool shares_faction;
    bool same_group;
} ConsumeSeedPreview;

static ConsumeCatalog consume_catalog;

static const ConsumeXpTable *ConsumeFindXpTable(const char *name) {
    for (int i = 0; i < consume_catalog.xp_table_count; i++)
        if (!strcmp(consume_catalog.xp_tables[i].name, name)) return &consume_catalog.xp_tables[i];
    TraceLog(LOG_FATAL, "Consume: unknown XP table %s", name);
    return NULL;
}

static const ConsumeProgression *ConsumeFindProgression(const char *name) {
    for (int i = 0; i < consume_catalog.progression_count; i++)
        if (!strcmp(consume_catalog.progressions[i].name, name)) return &consume_catalog.progressions[i];
    TraceLog(LOG_FATAL, "Consume: unknown progression %s", name);
    return NULL;
}

static const ConsumeArchetype *ConsumeFindArchetype(const char *name) {
    for (int i = 0; i < consume_catalog.archetype_count; i++)
        if (!strcmp(consume_catalog.archetypes[i].name, name)) return &consume_catalog.archetypes[i];
    TraceLog(LOG_FATAL, "Consume: unknown archetype %s", name);
    return NULL;
}

static void ConsumeExtendXpTable(ConsumeXpTable *table, int rows) {
    int step = table->xp[table->count - 1] - table->xp[table->count - 2];
    while (table->count < rows) {
        table->xp[table->count] = table->xp[table->count - 1] + step;
        table->count++;
    }
}

static void ConsumeCatalogLoadXpTables(const cJSON *json) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, json) {
        if (consume_catalog.xp_table_count >= CONSUME_MAX_XP_TABLES) TraceLog(LOG_FATAL, "Consume: too many XP tables");
        ConsumeXpTable *table = &consume_catalog.xp_tables[consume_catalog.xp_table_count++];
        StrCopy(table->name, sizeof(table->name), entry->string);
        const cJSON *row = NULL;
        cJSON_ArrayForEach(row, entry) {
            if (table->count >= CONSUME_MAX_XP_ROWS) TraceLog(LOG_FATAL, "Consume: XP table %s too long", table->name);
            table->xp[table->count++] = row->valueint;
        }
        if (!strcmp(table->name, CONSUME_HERO_LEVEL_TABLE)) ConsumeExtendXpTable(table, CONSUME_ASCENDED_LEVEL_CAP);
    }
}

static void ConsumeCatalogLoadProgressions(const cJSON *json) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, json) {
        if (consume_catalog.progression_count >= CONSUME_MAX_PROGRESSIONS)
            TraceLog(LOG_FATAL, "Consume: too many progressions");
        ConsumeProgression *progression = &consume_catalog.progressions[consume_catalog.progression_count++];
        StrCopy(progression->name, sizeof(progression->name), entry->string);
        const cJSON *row = NULL;
        cJSON_ArrayForEach(row, entry) {
            if (progression->count >= CONSUME_MAX_PROGRESSION_ROWS)
                TraceLog(LOG_FATAL, "Consume: progression %s too long", progression->name);
            for (int kind = 0; kind < 3; kind++)
                progression->totals[progression->count][kind] = cJSON_GetArrayItem(row, kind)->valueint;
            progression->count++;
        }
    }
}

static void ConsumeCatalogLoadArchetypes(const cJSON *json) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, json) {
        if (consume_catalog.archetype_count >= CONSUME_MAX_ARCHETYPES)
            TraceLog(LOG_FATAL, "Consume: too many archetypes");
        ConsumeArchetype *archetype = &consume_catalog.archetypes[consume_catalog.archetype_count++];
        StrCopy(archetype->name, sizeof(archetype->name), entry->string);
        for (int slot = 0; slot < CONSUME_FORTIFY_SLOT_COUNT; slot++)
            archetype->attributes[slot] = cJSON_GetArrayItem(entry, slot)->valueint;
    }
}

static void ConsumeCatalogLoadProfiles(const cJSON *json) {
    consume_catalog.profiles = (ConsumeProfile *)MemAlloc(CONSUME_MAX_PROFILES * sizeof(ConsumeProfile));
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, json) {
        if (consume_catalog.profile_count >= CONSUME_MAX_PROFILES) TraceLog(LOG_FATAL, "Consume: too many profiles");
        ConsumeProfile *profile = &consume_catalog.profiles[consume_catalog.profile_count++];
        StrCopy(profile->id, sizeof(profile->id), entry->string);
        profile->level_table = ConsumeFindXpTable(JsonStr(entry, "level_table"));
        profile->consume_table = ConsumeFindXpTable(JsonStr(entry, "consume_table"));
        profile->progression = ConsumeFindProgression(JsonStr(entry, "progression"));
        profile->archetype = ConsumeFindArchetype(JsonStr(entry, "archetype"));
        const cJSON *worth = cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "worth");
        for (int tier = 0; tier < TIER_COUNT; tier++)
            profile->worth[tier] = (float)cJSON_GetArrayItem(worth, tier)->valuedouble;
    }
}

static void ConsumeCatalogEnsure(void) {
    if (consume_catalog.loaded) return;
    consume_catalog.loaded = true;
    cJSON *root = JsonLoadFile(CONSUME_TABLES_PATH);
    ConsumeCatalogLoadXpTables(cJSON_GetObjectItemCaseSensitive(root, "xp_tables"));
    ConsumeCatalogLoadProgressions(cJSON_GetObjectItemCaseSensitive(root, "progressions"));
    ConsumeCatalogLoadArchetypes(cJSON_GetObjectItemCaseSensitive(root, "archetypes"));
    ConsumeCatalogLoadProfiles(cJSON_GetObjectItemCaseSensitive(root, "characters"));
    cJSON_Delete(root);
}

void ConsumeCatalogUnload(void) {
    MemFree(consume_catalog.profiles);
    memset(&consume_catalog, 0, sizeof(consume_catalog));
}

static const ConsumeProfile *ConsumeProfileFor(const Character *ch) {
    ConsumeCatalogEnsure();
    for (int i = 0; i < consume_catalog.profile_count; i++)
        if (!strcmp(consume_catalog.profiles[i].id, CharacterDataId(ch))) return &consume_catalog.profiles[i];
    TraceLog(LOG_FATAL, "Consume: %s has no profile; rerun tools/build_consume_tables.py", ch->id);
    return NULL;
}

int ConsumeXpForLevel(const ConsumeXpTable *table, int level) {
    if (level < 1) level = 1;
    if (level > table->count) level = table->count;
    return table->xp[level - 1];
}

int ConsumeLevelForXp(const ConsumeXpTable *table, int xp) {
    for (int i = 0; i < table->count; i++)
        if (xp < table->xp[i]) return i;
    return table->count;
}

static int ConsumeHeroLevelCap(const Character *ch, const ConsumeProfile *profile, Tier tier) {
    int cap = ch->stats[tier].max_level;
    return cap < profile->level_table->count ? cap : profile->level_table->count;
}

int ConsumeUnlockValueAt(const ConsumeProfile *profile, int level, ConsumeUnlock kind) {
    if (level < 0 || level >= profile->progression->count || kind == CONSUME_UNLOCK_NONE) return 0;
    return profile->progression->totals[level][kind - 1];
}

ConsumeUnlock ConsumeUnlockKindAt(const ConsumeProfile *profile, int level) {
    if (level < 0 || level >= profile->progression->count) return CONSUME_UNLOCK_NONE;
    for (int kind = CONSUME_UNLOCK_PROMOTION; kind <= CONSUME_UNLOCK_CRIT; kind++)
        if (ConsumeUnlockValueAt(profile, level, (ConsumeUnlock)kind)) return (ConsumeUnlock)kind;
    return CONSUME_UNLOCK_NONE;
}

bool ConsumeFortifySlotIsHealth(const ConsumeProfile *profile, int slot_number) {
    int slot = Clamp(slot_number - 1, 0, CONSUME_FORTIFY_SLOT_COUNT - 1);
    return profile->archetype->attributes[slot] == CONSUME_FORTIFY_HEALTH_ATTRIBUTE;
}

static int ConsumeUnlockedTotal(const ConsumeProfile *profile, int level, ConsumeUnlock kind) {
    int total = 0;
    for (int key = 0; key < level && key < profile->progression->count; key++) {
        int value = ConsumeUnlockValueAt(profile, key, kind);
        if (value > total) total = value;
    }
    return total;
}

static int ConsumeProgressionLevelOfTier(const ConsumeProfile *profile, int tier) {
    for (int key = 0; key < profile->progression->count; key++)
        if (ConsumeUnlockValueAt(profile, key, CONSUME_UNLOCK_PROMOTION) == tier + 1) return key;
    return -1;
}

static Tier ConsumeReachableTier(const Character *ch, int wanted) {
    int capped = wanted < (int)ch->max_tier ? wanted : (int)ch->max_tier;
    for (int tier = capped; tier > (int)ch->tier; tier--)
        if (ch->has_tier[tier]) return (Tier)tier;
    return ch->tier;
}

void ConsumeNormalizeCharacter(Character *ch) {
    const ConsumeProfile *profile = ConsumeProfileFor(ch);
    int tier_key = ConsumeProgressionLevelOfTier(profile, (int)ch->tier);
    if (tier_key >= 0) {
        int floor_xp = ConsumeXpForLevel(profile->consume_table, tier_key + 1);
        if (ch->consume_xp < floor_xp) ch->consume_xp = floor_xp;
    }
    int consume_cap = ConsumeXpForLevel(profile->consume_table, profile->consume_table->count);
    if (ch->consume_xp > consume_cap) ch->consume_xp = consume_cap;
    ch->consume_level = ConsumeLevelForXp(profile->consume_table, ch->consume_xp);
    ch->crit_chance = ConsumeUnlockedTotal(profile, ch->consume_level, CONSUME_UNLOCK_CRIT);

    int cap = ConsumeHeroLevelCap(ch, profile, ch->tier);
    if (ch->level > cap) ch->level = cap;
    int low = ConsumeXpForLevel(profile->level_table, ch->level);
    int high = ch->level >= cap ? low : ConsumeXpForLevel(profile->level_table, ch->level + 1) - 1;
    ch->xp = (int)Clamp((float)ch->xp, (float)low, (float)high);
}

bool ConsumeCharactersShareFaction(const Character *a, const Character *b) {
    if (a->giant && b->giant) return true;
    return (FactionIsWhite(a->faction) && FactionIsWhite(b->faction)) ||
           (FactionIsRed(a->faction) && FactionIsRed(b->faction)) ||
           (FactionIsBlue(a->faction) && FactionIsBlue(b->faction)) ||
           (FactionIsBlack(a->faction) && FactionIsBlack(b->faction));
}

static int ConsumeRarityIndex(const Character *ch) {
    return Clamp((int)ch->rarity - 1, 0, CONSUME_MAX_RARITY - 1);
}

static float ConsumeSeedBaseWorth(const Character *seed) {
    const int *base = seed->giant ? CONSUME_XP_BASE_GIANT : CONSUME_XP_BASE_STANDARD;
    return (float)rint(base[ConsumeRarityIndex(seed)] * ConsumeProfileFor(seed)->worth[seed->tier]);
}

static int ConsumeHeroXpGain(const Character *seed) {
    return seed->xp + (int)rint(ConsumeSeedBaseWorth(seed) * CONSUME_HERO_XP_SHARE_OF_WORTH);
}

static float ConsumeSeedBonus(const Character *target, Tier target_tier, const Character *seed) {
    float bonus = 0.0f;
    if (seed->tier == target_tier) bonus += CONSUME_SAME_TIER_BONUS * (float)((int)seed->tier + 1);
    if (ConsumeCharactersShareFaction(seed, target)) bonus += CONSUME_SAME_FACTION_BONUS;
    if (!strcmp(seed->id, target->id)) bonus += CONSUME_SAME_PROMOTE_GROUP_BONUS;
    return bonus;
}

static int ConsumeSeedWorth(const Character *target, Tier target_tier, const Character *seed) {
    float worth = ConsumeSeedBaseWorth(seed);
    return (int)rint(worth + worth * ConsumeSeedBonus(target, target_tier, seed));
}

static int ConsumeCombineGain(const Character *target, const Character *seed, int *tier_cursor) {
    if (strcmp(seed->id, target->id) != 0) return 0;
    const ConsumeProfile *profile = ConsumeProfileFor(target);
    int tiers = (int)seed->tier + 1;
    int headroom = (int)target->max_tier - *tier_cursor;
    if (tiers > headroom) tiers = headroom;
    if (tiers <= 0) return 0;
    int from = ConsumeProgressionLevelOfTier(profile, *tier_cursor);
    int to = ConsumeProgressionLevelOfTier(profile, *tier_cursor + tiers);
    *tier_cursor += tiers;
    if (from < 0 || to < 0) return 0;
    return ConsumeXpForLevel(profile->consume_table, to + 1) - ConsumeXpForLevel(profile->consume_table, from + 1);
}

ConsumeOutcome ConsumeCompute(const Character *target, const Character *const *seeds, int count) {
    const ConsumeProfile *profile = ConsumeProfileFor(target);
    int consume_gain = 0, xp_gain = 0, tier_cursor = (int)target->tier;
    for (int i = 0; i < count; i++) {
        consume_gain += ConsumeCombineGain(target, seeds[i], &tier_cursor);
        consume_gain += ConsumeSeedWorth(target, target->tier, seeds[i]);
        xp_gain += ConsumeHeroXpGain(seeds[i]);
    }

    int consume_cap = ConsumeXpForLevel(profile->consume_table, profile->consume_table->count);
    ConsumeOutcome outcome = {0};
    outcome.consume_xp =
        target->consume_xp > consume_cap - consume_gain ? consume_cap : target->consume_xp + consume_gain;
    outcome.consume_level = ConsumeLevelForXp(profile->consume_table, outcome.consume_xp);
    int unlocked_tier = ConsumeUnlockedTotal(profile, outcome.consume_level, CONSUME_UNLOCK_PROMOTION) - 1;
    outcome.tier = ConsumeReachableTier(target, unlocked_tier);
    int unlocked_crit = ConsumeUnlockedTotal(profile, outcome.consume_level, CONSUME_UNLOCK_CRIT);
    outcome.crit_chance = unlocked_crit > target->crit_chance ? unlocked_crit : target->crit_chance;

    int level_cap = ConsumeHeroLevelCap(target, profile, outcome.tier);
    int xp_cap = ConsumeXpForLevel(profile->level_table, level_cap);
    outcome.xp = target->xp > xp_cap - xp_gain ? xp_cap : target->xp + xp_gain;
    int level = ConsumeLevelForXp(profile->level_table, outcome.xp);
    outcome.level = level > level_cap ? level_cap : level;
    return outcome;
}

ConsumeSeedPreview ConsumePreviewSeed(const Character *target, const Character *seed) {
    bool same_tier = seed->tier == target->tier;
    ConsumeSeedPreview preview = {
        .xp_gain = ConsumeHeroXpGain(seed),
        .tier_bonus_pct = same_tier ? (int)lroundf(CONSUME_SAME_TIER_BONUS * 100.0f) * ((int)seed->tier + 1) : 0,
        .shares_faction = ConsumeCharactersShareFaction(seed, target),
        .same_group = !strcmp(seed->id, target->id),
    };
    preview.faction_bonus_pct = preview.shares_faction ? (int)lroundf(CONSUME_SAME_FACTION_BONUS * 100.0f) : 0;
    preview.combine_bonus_pct = preview.same_group ? (int)lroundf(CONSUME_SAME_PROMOTE_GROUP_BONUS * 100.0f) : 0;
    return preview;
}

bool ConsumeCharacterMaxed(const Character *ch) {
    const ConsumeProfile *profile = ConsumeProfileFor(ch);
    bool hero_maxed = ch->level >= ConsumeHeroLevelCap(ch, profile, ch->tier);
    return hero_maxed && ch->consume_level >= profile->consume_table->count;
}

float ConsumeBandFraction(const ConsumeXpTable *table, int level, int xp) {
    if (level >= table->count) return 1.0f;
    int low = ConsumeXpForLevel(table, level), high = ConsumeXpForLevel(table, level + 1);
    return high > low ? Clamp((float)(xp - low) / (float)(high - low), 0.0f, 1.0f) : 1.0f;
}
