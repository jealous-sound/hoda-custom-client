#define GALLERY_MAX_GROUPS 64
#define GALLERY_RECORD_ID_CAP 64
#define GALLERY_RECORD_INITIAL_CAPACITY 128
#define GALLERY_KEY_CAP 96

typedef enum {
    GALLERY_CATEGORY_ALL,
    GALLERY_CATEGORY_GROUP,
    GALLERY_CATEGORY_RARITY,
    GALLERY_CATEGORY_FACTION,
    GALLERY_CATEGORY_SPECIALTY,
    GALLERY_CATEGORY_COUNT
} GalleryCategory;

typedef enum {
    GALLERY_FILTER_NONE,
    GALLERY_FILTER_OWNED,
    GALLERY_FILTER_FOUGHT,
    GALLERY_FILTER_DEFEATED
} GalleryFilter;

typedef enum {
    GALLERY_FACTION_BLACK,
    GALLERY_FACTION_BLUE,
    GALLERY_FACTION_RED,
    GALLERY_FACTION_WHITE,
    GALLERY_FACTION_LARGE,
    GALLERY_FACTION_DUAL,
    GALLERY_FACTION_COUNT
} GalleryFaction;

typedef struct {
    char id[GALLERY_RECORD_ID_CAP];
    unsigned char owned_tiers;
    bool fought, defeated;
    bool new_owned, new_fought, new_defeated;
} GalleryRecord;

typedef struct {
    GalleryRecord *records;
    int count, capacity;
    int badge;
    int synced_roster_count;
    bool seeded;
} PlayerGallery;

typedef struct {
    Character *units;
    Ability *abilities;
    int count;
    char groups[GALLERY_MAX_GROUPS][24];
    int group_count;
    Specialty specialties[SPECIALTY_COUNT];
    int specialty_count;
} GalleryCatalog;

typedef struct {
    GalleryCategory category;
    Rarity rarity;
    GalleryFaction faction;
    int group;
    int specialty;
} GallerySelection;

typedef struct {
    int total, owned, fought, defeated;
    bool new_owned, new_fought, new_defeated;
} GalleryProgress;

typedef struct {
    int total, owned;
    int rarity_total[RARITY_MYTHIC], rarity_owned[RARITY_MYTHIC];
    int tier_total[TIER_COUNT], tier_owned[TIER_COUNT];
} GalleryTotals;

static unsigned char GalleryTierBit(Tier tier) {
    return (unsigned char)(1u << tier);
}

const GalleryRecord *GalleryRecordFind(const PlayerGallery *gallery, const char *id) {
    for (int i = 0; i < gallery->count; i++)
        if (!strcmp(gallery->records[i].id, id)) return &gallery->records[i];
    return NULL;
}

static GalleryRecord *GalleryRecordObtain(PlayerGallery *gallery, const char *id) {
    GalleryRecord *found = (GalleryRecord *)GalleryRecordFind(gallery, id);
    if (found) return found;
    if (gallery->count == gallery->capacity) {
        int capacity = gallery->capacity ? gallery->capacity * 2 : GALLERY_RECORD_INITIAL_CAPACITY;
        GalleryRecord *grown = (GalleryRecord *)realloc(gallery->records, (size_t)capacity * sizeof(GalleryRecord));
        if (!grown) return NULL;
        gallery->records = grown;
        gallery->capacity = capacity;
    }
    GalleryRecord *record = &gallery->records[gallery->count++];
    memset(record, 0, sizeof(*record));
    StrCopy(record->id, sizeof(record->id), id);
    return record;
}

void GalleryFree(PlayerGallery *gallery) {
    free(gallery->records);
    *gallery = (PlayerGallery){0};
}

void GalleryRecordOwned(PlayerGallery *gallery, const char *id, Tier tier, bool initial) {
    GalleryRecord *record = GalleryRecordObtain(gallery, id);
    if (!record) return;
    unsigned char bit = GalleryTierBit(tier);
    bool newly = !(record->owned_tiers & bit);
    record->owned_tiers |= bit;
    if (newly && !initial) {
        record->new_owned = true;
        gallery->badge++;
    }
}

void GalleryRecordBattleUnit(PlayerGallery *gallery, const char *id, bool defeated) {
    GalleryRecord *record = GalleryRecordObtain(gallery, id);
    if (!record) return;
    bool newly_fought = !record->fought;
    bool newly_defeated = defeated && !record->defeated;
    record->fought = true;
    record->defeated |= defeated;
    record->new_fought |= newly_fought;
    record->new_defeated |= newly_defeated;
    if (newly_fought || newly_defeated) gallery->badge++;
}

bool GalleryRecordClearNew(PlayerGallery *gallery, const char *id) {
    GalleryRecord *record = (GalleryRecord *)GalleryRecordFind(gallery, id);
    if (!record || !(record->new_owned || record->new_fought || record->new_defeated)) return false;
    record->new_owned = record->new_fought = record->new_defeated = false;
    return true;
}

Tier GalleryLowestTier(const Character *ch) {
    for (int tier = 0; tier < TIER_COUNT; tier++)
        if (ch->has_tier[tier]) return (Tier)tier;
    return TIER_I;
}

static int GalleryCompareUnits(const void *left, const void *right) {
    const Character *a = (const Character *)left, *b = (const Character *)right;
    if (a->original_rarity != b->original_rarity) return (int)b->original_rarity - (int)a->original_rarity;
    int power_a = a->stats[GalleryLowestTier(a)].base_power, power_b = b->stats[GalleryLowestTier(b)].base_power;
    if (power_a != power_b) return power_b - power_a;
    return strcmp(a->id, b->id);
}

static int GalleryCompareGroups(const void *left, const void *right) {
    return strcasecmp((const char *)left, (const char *)right);
}

static int GalleryCompareSpecialties(const void *left, const void *right) {
    return strcmp(SPECIALTY_NAMES[*(const Specialty *)left], SPECIALTY_NAMES[*(const Specialty *)right]);
}

static void GalleryCatalogAddGroup(GalleryCatalog *catalog, const char *group) {
    for (int g = 0; g < catalog->group_count; g++)
        if (!strcmp(catalog->groups[g], group)) return;
    if (catalog->group_count < GALLERY_MAX_GROUPS)
        StrCopy(catalog->groups[catalog->group_count++], sizeof(catalog->groups[0]), group);
}

static void GalleryCatalogAddSpecialty(GalleryCatalog *catalog, Specialty specialty) {
    for (int s = 0; s < catalog->specialty_count; s++)
        if (catalog->specialties[s] == specialty) return;
    catalog->specialties[catalog->specialty_count++] = specialty;
}

static void GalleryCatalogRebind(GalleryCatalog *catalog) {
    for (int i = 0; i < catalog->count; i++) catalog->units[i].ability = &catalog->abilities[i];
}

static void GalleryCatalogFinish(GalleryCatalog *catalog) {
    qsort(catalog->units, (size_t)catalog->count, sizeof(Character), GalleryCompareUnits);
    for (int i = 0; i < catalog->count; i++) {
        const Character *unit = &catalog->units[i];
        catalog->abilities[i] = *unit->ability;
    }
    GalleryCatalogRebind(catalog);
    for (int i = 0; i < catalog->count; i++) {
        const Character *unit = &catalog->units[i];
        for (int g = 0; g < unit->group_count; g++) GalleryCatalogAddGroup(catalog, unit->groups[g]);
        for (int s = 0; s < unit->specialty_count; s++) GalleryCatalogAddSpecialty(catalog, unit->specialties[s]);
    }
    qsort(catalog->groups, (size_t)catalog->group_count, sizeof(catalog->groups[0]), GalleryCompareGroups);
    qsort(catalog->specialties, (size_t)catalog->specialty_count, sizeof(Specialty), GalleryCompareSpecialties);
}

void GalleryCatalogLoad(GalleryCatalog *catalog, const char *path) {
    memset(catalog, 0, sizeof(*catalog));
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(path));
    cJSON *characters = cJSON_GetObjectItemCaseSensitive(root, "characters");
    int capacity = cJSON_GetArraySize(characters);
    catalog->units = (Character *)MemAlloc((unsigned int)capacity * sizeof(Character));
    catalog->abilities = (Ability *)MemAlloc((unsigned int)capacity * sizeof(Ability));
    Ability *parsed = (Ability *)MemAlloc((unsigned int)capacity * sizeof(Ability));

    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, characters) {
        if (!JsonBool(entry, "obtainable")) continue;
        Character *unit = &catalog->units[catalog->count];
        CharacterParse(entry, unit, &parsed[catalog->count]);
        if (unit->rarity == RARITY_MYTHIC) continue;
        unit->tier = GalleryLowestTier(unit);
        unit->level = 1;
        catalog->count++;
    }
    GalleryCatalogFinish(catalog);
    MemFree(parsed);
    cJSON_Delete(root);
}

void GalleryCatalogUnload(GalleryCatalog *catalog) {
    MemFree(catalog->units);
    MemFree(catalog->abilities);
    *catalog = (GalleryCatalog){0};
}

GalleryCategory GalleryCategoryStep(GalleryCategory category, int delta) {
    return (GalleryCategory)((category + delta + GALLERY_CATEGORY_COUNT) % GALLERY_CATEGORY_COUNT);
}

Rarity GalleryRarityStep(Rarity rarity, int delta) {
    int span = RARITY_LEGENDARY - RARITY_COMMON + 1;
    return (Rarity)(RARITY_COMMON + (rarity - RARITY_COMMON + delta + span) % span);
}

GalleryFaction GalleryFactionStep(GalleryFaction faction, int delta) {
    return (GalleryFaction)((faction + delta + GALLERY_FACTION_COUNT) % GALLERY_FACTION_COUNT);
}

void GallerySelectionStepSubcategory(GallerySelection *selection, const GalleryCatalog *catalog, int delta) {
    switch (selection->category) {
        case GALLERY_CATEGORY_GROUP:
            selection->group = (selection->group + delta + catalog->group_count) % catalog->group_count;
            break;
        case GALLERY_CATEGORY_RARITY:
            selection->rarity = GalleryRarityStep(selection->rarity, delta);
            break;
        case GALLERY_CATEGORY_FACTION:
            selection->faction = GalleryFactionStep(selection->faction, delta);
            break;
        case GALLERY_CATEGORY_SPECIALTY:
            selection->specialty = (selection->specialty + delta + catalog->specialty_count) % catalog->specialty_count;
            break;
        default:
            break;
    }
}

static bool GalleryUnitHasGroup(const Character *unit, const char *group) {
    for (int g = 0; g < unit->group_count; g++)
        if (!strcmp(unit->groups[g], group)) return true;
    return false;
}

static bool GalleryUnitHasSpecialty(const Character *unit, Specialty specialty) {
    for (int s = 0; s < unit->specialty_count; s++)
        if (unit->specialties[s] == specialty) return true;
    return false;
}

static bool GalleryUnitInFaction(const Character *unit, GalleryFaction faction) {
    switch (faction) {
        case GALLERY_FACTION_BLACK:
            return FactionIsBlack(unit->faction);
        case GALLERY_FACTION_BLUE:
            return FactionIsBlue(unit->faction);
        case GALLERY_FACTION_RED:
            return FactionIsRed(unit->faction);
        case GALLERY_FACTION_WHITE:
            return FactionIsWhite(unit->faction);
        case GALLERY_FACTION_LARGE:
            return unit->faction == FACTION_LARGE;
        default:
            return unit->faction >= FACTION_WHITE_RED;
    }
}

bool GalleryUnitMatches(const GalleryCatalog *catalog, const GallerySelection *selection, const Character *unit) {
    switch (selection->category) {
        case GALLERY_CATEGORY_GROUP:
            return GalleryUnitHasGroup(unit, catalog->groups[selection->group]);
        case GALLERY_CATEGORY_RARITY:
            return unit->original_rarity == selection->rarity;
        case GALLERY_CATEGORY_FACTION:
            return GalleryUnitInFaction(unit, selection->faction);
        case GALLERY_CATEGORY_SPECIALTY:
            return GalleryUnitHasSpecialty(unit, catalog->specialties[selection->specialty]);
        default:
            return false;
    }
}

static bool GalleryRecordPassesFilter(const GalleryRecord *record, GalleryFilter filter) {
    switch (filter) {
        case GALLERY_FILTER_OWNED:
            return record && record->owned_tiers;
        case GALLERY_FILTER_FOUGHT:
            return record && record->fought;
        case GALLERY_FILTER_DEFEATED:
            return record && record->defeated;
        default:
            return true;
    }
}

int GalleryQuery(
    const GalleryCatalog *catalog,
    const PlayerGallery *gallery,
    const GallerySelection *selection,
    GalleryFilter filter,
    int *matches,
    GalleryProgress *progress
) {
    *progress = (GalleryProgress){0};
    int count = 0;
    for (int i = 0; i < catalog->count; i++) {
        const Character *unit = &catalog->units[i];
        if (!GalleryUnitMatches(catalog, selection, unit)) continue;
        const GalleryRecord *record = GalleryRecordFind(gallery, unit->id);
        progress->total++;
        if (record) {
            progress->owned += record->owned_tiers != 0;
            progress->fought += record->fought;
            progress->defeated += record->defeated;
            progress->new_owned |= record->new_owned;
            progress->new_fought |= record->new_fought;
            progress->new_defeated |= record->new_defeated;
        }
        if (GalleryRecordPassesFilter(record, filter)) matches[count++] = i;
    }
    return count;
}

GalleryTotals GalleryTotalsCompute(const GalleryCatalog *catalog, const PlayerGallery *gallery) {
    GalleryTotals totals = {0};
    for (int i = 0; i < catalog->count; i++) {
        const Character *unit = &catalog->units[i];
        const GalleryRecord *record = GalleryRecordFind(gallery, unit->id);
        unsigned char owned_tiers = record ? record->owned_tiers : 0;
        bool lowest_owned = owned_tiers & GalleryTierBit(GalleryLowestTier(unit));
        totals.total++;
        totals.owned += lowest_owned;
        totals.rarity_total[unit->original_rarity - RARITY_COMMON]++;
        totals.rarity_owned[unit->original_rarity - RARITY_COMMON] += lowest_owned;
        for (int tier = 0; tier < TIER_COUNT; tier++) {
            if (!unit->has_tier[tier]) continue;
            totals.tier_total[tier]++;
            totals.tier_owned[tier] += (owned_tiers & GalleryTierBit((Tier)tier)) != 0;
        }
    }
    return totals;
}

static void GalleryUpperKey(char *out, int capacity, const char *format, const char *name) {
    snprintf(out, (size_t)capacity, format, name);
    for (char *c = out; *c; c++) *c = (char)toupper((unsigned char)*c);
}

static const char *GalleryCategoryName(GalleryCategory category) {
    static const char *const names[GALLERY_CATEGORY_COUNT] = {"ALL", "GROUP", "RARITY", "FACTION", "SPECIALTY"};
    return names[category];
}

static const char *GallerySubcategoryName(const GalleryCatalog *catalog, const GallerySelection *selection) {
    static const char *const rarities[] = {"", "COMMON", "UNCOMMON", "RARE", "EPIC", "LEGENDARY"};
    static const char *const factions[GALLERY_FACTION_COUNT] = {"BLACK", "BLUE", "RED", "WHITE", "NONE", "DUAL"};
    switch (selection->category) {
        case GALLERY_CATEGORY_GROUP:
            return catalog->groups[selection->group];
        case GALLERY_CATEGORY_RARITY:
            return rarities[selection->rarity];
        case GALLERY_CATEGORY_FACTION:
            return factions[selection->faction];
        case GALLERY_CATEGORY_SPECIALTY:
            return SPECIALTY_NAMES[catalog->specialties[selection->specialty]];
        default:
            return "ALL";
    }
}

void GalleryCategoryTitleKey(char *out, int capacity, GalleryCategory category) {
    GalleryUpperKey(out, capacity, "GALLERY_%s_TITLE", GalleryCategoryName(category));
}

void GallerySubcategoryTitleKey(
    char *out, int capacity, const GalleryCatalog *catalog, const GallerySelection *selection
) {
    GalleryUpperKey(out, capacity, "GALLERY_%s_TITLE", GallerySubcategoryName(catalog, selection));
}

void GallerySubcategoryLoreKey(
    char *out, int capacity, const GalleryCatalog *catalog, const GallerySelection *selection
) {
    GalleryUpperKey(out, capacity, "GALLERY_%s_LORE", GallerySubcategoryName(catalog, selection));
}
