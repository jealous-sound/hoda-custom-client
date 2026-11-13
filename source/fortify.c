#define FORTIFY_TABLES_PATH "data/fortify_tables.json"
#define FORTIFY_TABLES_VERSION 1
#define FORTIFY_HERO_CATALOG_PATH HOD_CACHE "characters.json"
#define FORTIFY_MAX_ARCHETYPES 8
#define FORTIFY_REMOVE_ALL -1
#define FORTIFY_SLOTS_PER_ROW 5

typedef enum { FORTIFY_STAT_HEALTH, FORTIFY_STAT_POWER } FortifyStat;

typedef struct {
    FortifyStat stat;
    float cap;
} FortifySlotDef;

typedef struct {
    char name[16];
    FortifySlotDef slots[FORTIFY_SLOT_COUNT];
} FortifyArchetype;

typedef struct {
    char id[64];
    Rarity rarity;
    Faction faction;
    bool giant;
    bool has_tier[TIER_COUNT];
    Tier max_tier;
    char icon_prefab[TIER_COUNT][64];
} FortifyHero;

typedef struct {
    bool loaded;
    float faction_bonus;
    float rarity_tier_bonus[RARITY_MYTHIC][TIER_COUNT];
    int remove_gem_cost[RARITY_MYTHIC];
    FortifyArchetype archetypes[FORTIFY_MAX_ARCHETYPES];
    int archetype_count;
    FortifyHero *heroes;
    int hero_count;
} FortifyCatalog;

typedef struct {
    int rarity;
    int faction;
    int tier;
} FortifyBonus;

typedef struct {
    int slot;
    int unit;
} FortifyPlacement;

static FortifyCatalog fortify_catalog;

static void FortifyCatalogLoadTables(const cJSON *root) {
    if (JsonInt(root, "version") != FORTIFY_TABLES_VERSION)
        TraceLog(LOG_FATAL, "Fortify: %s has version %d", FORTIFY_TABLES_PATH, JsonInt(root, "version"));
    fortify_catalog.faction_bonus = (float)cJSON_GetObjectItemCaseSensitive(root, "faction_bonus")->valuedouble;
    const cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "rarity_tier_bonus");
    const cJSON *costs = cJSON_GetObjectItemCaseSensitive(root, "remove_gem_cost");
    if (cJSON_GetArraySize(rows) != RARITY_MYTHIC || cJSON_GetArraySize(costs) != RARITY_MYTHIC)
        TraceLog(LOG_FATAL, "Fortify: %s needs one bonus row and cost per rarity", FORTIFY_TABLES_PATH);
    for (int rarity = 0; rarity < RARITY_MYTHIC; rarity++) {
        const cJSON *row = cJSON_GetArrayItem(rows, rarity);
        for (int tier = 0; tier < TIER_COUNT; tier++)
            fortify_catalog.rarity_tier_bonus[rarity][tier] = (float)cJSON_GetArrayItem(row, tier)->valuedouble;
        fortify_catalog.remove_gem_cost[rarity] = cJSON_GetArrayItem(costs, rarity)->valueint;
    }
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(root, "archetypes")) {
        if (fortify_catalog.archetype_count >= FORTIFY_MAX_ARCHETYPES)
            TraceLog(LOG_FATAL, "Fortify: too many archetypes");
        if (cJSON_GetArraySize(entry) != FORTIFY_SLOT_COUNT)
            TraceLog(LOG_FATAL, "Fortify: archetype %s needs %d slots", entry->string, FORTIFY_SLOT_COUNT);
        FortifyArchetype *archetype = &fortify_catalog.archetypes[fortify_catalog.archetype_count++];
        StrCopy(archetype->name, sizeof(archetype->name), entry->string);
        for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) {
            const cJSON *def = cJSON_GetArrayItem(entry, slot);
            archetype->slots[slot].stat =
                !strcmp(JsonStr(def, "stat"), "health") ? FORTIFY_STAT_HEALTH : FORTIFY_STAT_POWER;
            archetype->slots[slot].cap = (float)cJSON_GetObjectItemCaseSensitive(def, "cap")->valuedouble;
        }
    }
}

static void FortifyCatalogLoadHeroes(void) {
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(FORTIFY_HERO_CATALOG_PATH));
    const cJSON *characters = cJSON_GetObjectItemCaseSensitive(root, "characters");
    fortify_catalog.heroes =
        (FortifyHero *)MemAlloc((unsigned int)(cJSON_GetArraySize(characters) * sizeof(FortifyHero)));
    static Character parsed;
    static Ability ability;
    const cJSON *def = NULL;
    cJSON_ArrayForEach(def, characters) {
        CharacterParse(def, &parsed, &ability);
        FortifyHero *hero = &fortify_catalog.heroes[fortify_catalog.hero_count++];
        StrCopy(hero->id, sizeof(hero->id), parsed.id);
        hero->rarity = parsed.rarity;
        hero->faction = parsed.faction;
        hero->giant = parsed.giant;
        hero->max_tier = parsed.max_tier;
        memcpy(hero->has_tier, parsed.has_tier, sizeof(hero->has_tier));
        memcpy(hero->icon_prefab, parsed.icon_prefab, sizeof(hero->icon_prefab));
    }
    cJSON_Delete(root);
}

static void FortifyCatalogEnsure(void) {
    if (fortify_catalog.loaded) return;
    fortify_catalog.loaded = true;
    cJSON *root = JsonLoadFile(FORTIFY_TABLES_PATH);
    FortifyCatalogLoadTables(root);
    cJSON_Delete(root);
    FortifyCatalogLoadHeroes();
}

void FortifyCatalogUnload(void) {
    MemFree(fortify_catalog.heroes);
    memset(&fortify_catalog, 0, sizeof(fortify_catalog));
}

const FortifyHero *FortifyHeroFind(const char *id) {
    FortifyCatalogEnsure();
    for (int i = 0; i < fortify_catalog.hero_count; i++)
        if (!strcmp(fortify_catalog.heroes[i].id, id)) return &fortify_catalog.heroes[i];
    return NULL;
}

static const FortifyArchetype *FortifyArchetypeOf(const Character *ch) {
    FortifyCatalogEnsure();
    const char *name = ConsumeProfileFor(ch)->archetype->name;
    for (int i = 0; i < fortify_catalog.archetype_count; i++)
        if (!strcmp(fortify_catalog.archetypes[i].name, name)) return &fortify_catalog.archetypes[i];
    TraceLog(LOG_FATAL, "Fortify: %s has unknown archetype %s", ch->id, name);
    return NULL;
}

#define FORTIFY_DEFAULT_ARCHETYPE "default"

const FortifySlotDef *FortifyDefaultSlotDef(int slot) {
    FortifyCatalogEnsure();
    for (int i = 0; i < fortify_catalog.archetype_count; i++)
        if (!strcmp(fortify_catalog.archetypes[i].name, FORTIFY_DEFAULT_ARCHETYPE))
            return &fortify_catalog.archetypes[i].slots[slot];
    TraceLog(LOG_FATAL, "Fortify: %s has no %s archetype", FORTIFY_TABLES_PATH, FORTIFY_DEFAULT_ARCHETYPE);
    return NULL;
}

const FortifySlotDef *FortifySlotDefAt(const Character *parent, int slot) {
    if (slot < 0 || slot >= FORTIFY_SLOT_COUNT) return NULL;
    return &FortifyArchetypeOf(parent)->slots[slot];
}

int FortifySlotsUnlocked(const Character *ch) {
    int unlocked = ConsumeUnlockedTotal(ConsumeProfileFor(ch), ch->consume_level, CONSUME_UNLOCK_FORTIFY);
    return unlocked < FORTIFY_SLOT_COUNT ? unlocked : FORTIFY_SLOT_COUNT;
}

bool FortifySlotFilled(const Character *ch, int slot) {
    return slot >= 0 && slot < FORTIFY_SLOT_COUNT && ch->fortify[slot].id[0] != '\0';
}

bool FortifyHasChildren(const Character *ch) {
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++)
        if (FortifySlotFilled(ch, slot)) return true;
    return false;
}

int FortifyFilledCount(const Character *ch) {
    int count = 0;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) count += FortifySlotFilled(ch, slot);
    return count;
}

FortifyChild FortifyChildOf(const Character *unit) {
    FortifyChild child = {.tier = unit->tier, .ascended = unit->ascended};
    StrCopy(child.id, sizeof(child.id), unit->id);
    return child;
}

Rarity FortifyChildRarity(const FortifyChild *child) {
    const FortifyHero *hero = FortifyHeroFind(child->id);
    return hero ? hero->rarity : RARITY_COMMON;
}

static int FortifyRound(float value) {
    return (int)rintf(value);
}

static float FortifyParentMaxStat(const Character *parent, FortifyStat stat) {
    const TierStats *st = &parent->stats[parent->tier];
    int value = stat == FORTIFY_STAT_HEALTH ? st->base_health + (st->max_level - 1) * st->health_per_level
                                            : st->base_power + (st->max_level - 1) * st->power_per_level;
    return (float)value;
}

static bool FortifySharesFaction(const Character *parent, const FortifyHero *child) {
    if (parent->giant && child->giant) return true;
    return FactionsOverlap(parent->faction, child->faction);
}

FortifyBonus FortifySlotBonus(const Character *parent, int slot, const FortifyChild *child) {
    FortifyBonus bonus = {0};
    const FortifySlotDef *def = FortifySlotDefAt(parent, slot);
    const FortifyHero *hero = child && child->id[0] ? FortifyHeroFind(child->id) : NULL;
    if (!def || !hero) return bonus;
    float base = FortifyParentMaxStat(parent, def->stat) * def->cap;
    const float *row = fortify_catalog.rarity_tier_bonus[FortifyChildRarity(child) - 1];
    bonus.rarity = FortifyRound(base * row[0]);
    bonus.tier = FortifyRound(base * (row[child->tier] - row[0]));
    if (FortifySharesFaction(parent, hero)) {
        float faction_multiplier = (1.0f + fortify_catalog.faction_bonus) - 1.0f;
        bonus.faction = FortifyRound((float)(bonus.rarity + bonus.tier) * faction_multiplier);
    }
    return bonus;
}

int FortifyBonusTotal(FortifyBonus bonus) {
    return bonus.rarity + bonus.faction + bonus.tier;
}

static int FortifyStatBonus(const Character *ch, FortifyStat stat) {
    int total = 0;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) {
        if (!FortifySlotFilled(ch, slot)) continue;
        if (FortifySlotDefAt(ch, slot)->stat == stat)
            total += FortifyBonusTotal(FortifySlotBonus(ch, slot, &ch->fortify[slot]));
    }
    return total;
}

int FortifyBonusPower(const Character *ch) {
    return FortifyStatBonus(ch, FORTIFY_STAT_POWER);
}

int FortifyBonusHealth(const Character *ch) {
    return FortifyStatBonus(ch, FORTIFY_STAT_HEALTH);
}

int FortifySlotMaxBonus(const Character *parent, int slot) {
    const FortifySlotDef *def = FortifySlotDefAt(parent, slot);
    float top = fortify_catalog.rarity_tier_bonus[RARITY_MYTHIC - 1][TIER_IV];
    return FortifyRound(
        FortifyParentMaxStat(parent, def->stat) * def->cap * top * (1.0f + fortify_catalog.faction_bonus)
    );
}

int FortifyMaxBonus(const Character *ch, FortifyStat stat) {
    int total = 0;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++)
        if (FortifySlotDefAt(ch, slot)->stat == stat) total += FortifySlotMaxBonus(ch, slot);
    return total;
}

static Tier FortifyNextPresentTier(const Character *unit, Tier tier) {
    for (int next = (int)tier + 1; next < TIER_COUNT; next++)
        if (unit->has_tier[next]) return (Tier)next;
    return tier;
}

FortifyChild FortifyMergedChild(const Character *parent, int slot, const Character *unit) {
    FortifyChild incoming = FortifyChildOf(unit);
    const FortifyChild *existing = &parent->fortify[slot];
    if (!existing->id[0] || strcmp(existing->id, unit->id) != 0) return incoming;
    int combined_tiers = (int)unit->tier + (int)existing->tier + 1;
    int target = combined_tiers < 1 ? 1 : combined_tiers > TIER_COUNT ? TIER_COUNT : combined_tiers;
    FortifyChild merged = *existing;
    merged.ascended = existing->ascended || incoming.ascended;
    for (int promotion = (int)merged.tier; promotion < target; promotion++)
        merged.tier = FortifyNextPresentTier(unit, merged.tier);
    return merged;
}

bool FortifyReplacementBlocked(const Character *parent, int slot, const Character *unit) {
    const FortifyChild *existing = &parent->fortify[slot];
    return existing->id[0] && !strcmp(existing->id, unit->id) && existing->tier == unit->max_tier;
}

int FortifyRemoveCostOf(const FortifyChild *child) {
    FortifyCatalogEnsure();
    return fortify_catalog.remove_gem_cost[FortifyChildRarity(child) - 1];
}

int FortifyRemoveCost(const Character *parent, int slot) {
    int cost = 0;
    for (int index = 0; index < FORTIFY_SLOT_COUNT; index++)
        if ((slot == FORTIFY_REMOVE_ALL || slot == index) && FortifySlotFilled(parent, index))
            cost += FortifyRemoveCostOf(&parent->fortify[index]);
    return cost;
}

bool FortifyPlacementAllowed(const Player *p, int parent, int slot, int unit) {
    if (parent < 0 || parent >= p->roster_count || unit < 0 || unit >= p->roster_count || unit == parent) return false;
    const Character *ch = &p->roster[parent];
    return slot >= 0 && slot < FortifySlotsUnlocked(ch) && !PlayerSquadsContain(p, unit) && !PlayerHeroAway(p, unit) &&
           !PlayerHeroAway(p, parent) && !FortifyReplacementBlocked(ch, slot, &p->roster[unit]);
}

static bool FortifyPlacementsValid(const Player *p, int parent, const FortifyPlacement *placements, int count) {
    if (count <= 0 || count > FORTIFY_SLOT_COUNT) return false;
    for (int i = 0; i < count; i++) {
        if (!FortifyPlacementAllowed(p, parent, placements[i].slot, placements[i].unit)) return false;
        for (int previous = 0; previous < i; previous++)
            if (placements[previous].slot == placements[i].slot || placements[previous].unit == placements[i].unit)
                return false;
    }
    return true;
}

int PlayerFortify(Player *p, int parent, const FortifyPlacement *placements, int count) {
    if (!p || !placements || !FortifyPlacementsValid(p, parent, placements, count)) return -1;
    Character *ch = &p->roster[parent];
    FortifyChild merged[FORTIFY_SLOT_COUNT];
    for (int i = 0; i < count; i++)
        merged[i] = FortifyMergedChild(ch, placements[i].slot, &p->roster[placements[i].unit]);
    bool *remove = (bool *)MemAlloc((unsigned int)p->roster_count);
    for (int i = 0; i < count; i++) {
        ch->fortify[placements[i].slot] = merged[i];
        remove[placements[i].unit] = true;
    }
    int result = PlayerRosterCompact(p, remove, parent);
    MemFree(remove);
    return result;
}

static const cJSON *FortifyCatalogDefinition(const cJSON *catalog, const char *id) {
    const cJSON *def = NULL;
    cJSON_ArrayForEach(def, (cJSON *)catalog) {
        if (!strcmp(JsonStr(def, "id"), id)) return def;
    }
    return NULL;
}

int PlayerRemoveFortify(Player *p, int parent, int slot, int *out_first) {
    if (out_first) *out_first = -1;
    if (!p || parent < 0 || parent >= p->roster_count) return 0;
    if (slot != FORTIFY_REMOVE_ALL && !FortifySlotFilled(&p->roster[parent], slot)) return 0;
    int slots[FORTIFY_SLOT_COUNT], count = 0;
    for (int index = 0; index < FORTIFY_SLOT_COUNT; index++)
        if ((slot == FORTIFY_REMOVE_ALL || slot == index) && FortifySlotFilled(&p->roster[parent], index))
            slots[count++] = index;
    int cost = FortifyRemoveCost(&p->roster[parent], slot);
    if (count == 0 || !PlayerRosterHasCapacity(p, count) || p->gems < cost) return 0;

    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(FORTIFY_HERO_CATALOG_PATH));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    const cJSON *defs[FORTIFY_SLOT_COUNT];
    bool found = true;
    for (int i = 0; i < count && found; i++)
        found = (defs[i] = FortifyCatalogDefinition(catalog, p->roster[parent].fortify[slots[i]].id)) != NULL;
    int first = -1;
    bool added = found && PlayerRosterAppendDefinitions(p, defs, count, &first);
    cJSON_Delete(root);
    if (!added) return 0;

    PlayerSpendGems(p, cost);
    Character *ch = &p->roster[parent];
    for (int i = 0; i < count; i++) {
        const FortifyChild *child = &ch->fortify[slots[i]];
        Character *returned = &p->roster[first + i];
        returned->tier = child->tier;
        if (child->ascended) GearApplyAscension(returned);
        ConsumeNormalizeCharacter(returned);
        memset(&ch->fortify[slots[i]], 0, sizeof(ch->fortify[slots[i]]));
    }
    if (out_first) *out_first = first;
    return count;
}

cJSON *FortifyServerAction(const Player *p, int parent, const FortifyPlacement *placements, int count) {
    cJSON *fields = ServerActionCreate("fortify");
    cJSON *slots = fields ? cJSON_AddArrayToObject(fields, "slots") : NULL;
    if (!slots) {
        cJSON_Delete(fields);
        return NULL;
    }
    cJSON_AddNumberToObject(fields, "parent", (double)p->roster[parent].instance_id);
    for (int i = 0; i < count; i++) {
        cJSON *slot = cJSON_CreateObject();
        cJSON_AddNumberToObject(slot, "index", placements[i].slot);
        cJSON_AddNumberToObject(slot, "unit", (double)p->roster[placements[i].unit].instance_id);
        cJSON_AddItemToArray(slots, slot);
    }
    return fields;
}

cJSON *FortifyRemoveServerAction(const Player *p, int parent, int slot) {
    cJSON *fields = ServerActionCreate("remove_fortify");
    if (!fields) return NULL;
    cJSON_AddNumberToObject(fields, "parent", (double)p->roster[parent].instance_id);
    cJSON_AddNumberToObject(fields, "index", slot);
    return fields;
}

cJSON *FortifyChildrenJson(const Character *ch) {
    if (!FortifyHasChildren(ch)) return NULL;
    cJSON *array = cJSON_CreateArray();
    for (int slot = 0; array && slot < FORTIFY_SLOT_COUNT; slot++) {
        if (!FortifySlotFilled(ch, slot)) continue;
        const FortifyChild *child = &ch->fortify[slot];
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(entry, "slot", slot);
        cJSON_AddStringToObject(entry, "id", child->id);
        cJSON_AddStringToObject(entry, "tier", TIER_NAMES[child->tier]);
        if (child->ascended) cJSON_AddBoolToObject(entry, "ascended", true);
        cJSON_AddItemToArray(array, entry);
    }
    return array;
}

static bool FortifyChildLoad(const cJSON *entry, int *slot, FortifyChild *child) {
    const cJSON *slot_value = cJSON_GetObjectItemCaseSensitive(entry, "slot");
    const cJSON *ascended = cJSON_GetObjectItemCaseSensitive(entry, "ascended");
    const char *id = JsonStr(entry, "id");
    const char *tier_name = JsonStr(entry, "tier");
    if (!cJSON_IsObject(entry) || !cJSON_IsNumber(slot_value) || (ascended && !cJSON_IsBool(ascended))) return false;
    *slot = slot_value->valueint;
    Tier tier = TierFromString(tier_name);
    const FortifyHero *hero = FortifyHeroFind(id);
    if (*slot < 0 || *slot >= FORTIFY_SLOT_COUNT || !hero || strcmp(tier_name, TIER_NAMES[tier]) != 0 ||
        !hero->has_tier[tier])
        return false;
    *child = (FortifyChild){.tier = tier, .ascended = cJSON_IsTrue(ascended)};
    StrCopy(child->id, sizeof(child->id), id);
    return true;
}

bool FortifyChildrenParse(FortifyChild children[FORTIFY_SLOT_COUNT], const cJSON *array) {
    size_t size = FORTIFY_SLOT_COUNT * sizeof(children[0]);
    memset(children, 0, size);
    if (!array) return true;
    if (!cJSON_IsArray(array)) return false;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)array) {
        int slot = -1;
        FortifyChild child;
        if (!FortifyChildLoad(entry, &slot, &child) || children[slot].id[0] != '\0') {
            memset(children, 0, size);
            return false;
        }
        children[slot] = child;
    }
    return true;
}

bool FortifyChildrenLoad(Character *ch, const cJSON *array) {
    return FortifyChildrenParse(ch->fortify, array);
}
