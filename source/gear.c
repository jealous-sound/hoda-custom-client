#define GEAR_CATALOG_PATH "data/gear.json"
#define GEAR_CATALOG_CAPACITY 288
#define GEAR_RARITY_COUNT 6
#define GEAR_RARITY_LEGENDARY 5
#define GEAR_PACK_CAPACITY 6
#define GEAR_ROLL_TOME (-1)
#define GEAR_ROLL_NONE (-2)
#define GEAR_STORE_CATALOG_PATH "data/store_packs.json"
#define GEAR_ASCENSION_BONUS_LEVELS 5
#define GEAR_ASCENSION_BONUS_SLOTS 1
#define GEAR_PACK_MAX_ITEMS 5
#define GEAR_PACK_MAX_ROLLS (GEAR_PACK_MAX_ITEMS + 1)
#define GEAR_CATEGORY_CAPACITY 8
#define GEAR_NORMAL_SLOT_MAX 3
#define GEAR_PRESTIGE_MIN_SLOTS 4
#define GEAR_EMPTY_SLOT 0
#define GEAR_TEXT_KEY_CAPACITY 80
#define GEAR_AURA_KEY_CAPACITY 8
#define GEAR_AURA_SEPARATOR " "
#define GEAR_UI_TEXTURE_PATH "gui/gear/%s.png"
#define GEAR_TOME_ICON "gui/gear/tex.tomeofascension.png"
#define GEAR_ICON_CACHE_CAPACITY (GEAR_CATALOG_CAPACITY + 16)

typedef enum {
    GEAR_STAT_NONE,
    GEAR_STAT_POWER,
    GEAR_STAT_HEALTH,
    GEAR_STAT_CRIT,
    GEAR_STAT_BONUS_DAMAGE,
    GEAR_STAT_DAMAGE_RESIST,
    GEAR_STAT_RAW_DAMAGE_RESIST,
    GEAR_STAT_STUN_RESIST,
    GEAR_STAT_SLOW_RESIST,
    GEAR_STAT_DRAIN_RESIST,
    GEAR_STAT_CURSE_POWER_RESIST,
    GEAR_STAT_CURSE_HEALTH_RESIST,
    GEAR_STAT_DEFENDER,
    GEAR_STAT_STEALTH,
    GEAR_STAT_PERCEPTIVE,
    GEAR_STAT_HASTE,
    GEAR_STAT_FLANK_REDUCTION,
    GEAR_STAT_TARGET_HIGHEST_POWER,
    GEAR_STAT_TARGET_LOWEST_HEALTH,
    GEAR_STAT_TARGET_BACK_ROW,
    GEAR_STAT_TARGET_QUICK,
    GEAR_STAT_TARGET_SLOWEST,
    GEAR_STAT_TARGET_MATCH,
    GEAR_STAT_DAMAGE,
    GEAR_STAT_DRAIN,
    GEAR_STAT_COUNT
} GearStat;

typedef enum {
    GEAR_ON_BATTLE,
    GEAR_ON_SELF_HIT,
    GEAR_ON_ALLY_HIT,
    GEAR_ON_ENEMY_HIT,
    GEAR_ON_SELF_ATTACK,
    GEAR_ON_ALLY_ATTACK,
    GEAR_ON_ENEMY_DEATH,
    GEAR_ON_ALLY_DEATH,
    GEAR_ON_SELF_DEATH
} GearTrigger;

typedef enum { GEAR_BASIS_BASE, GEAR_BASIS_DAMAGE } GearBasis;
typedef enum {
    GEAR_CATEGORY_FACTION,
    GEAR_CATEGORY_GROUP,
    GEAR_CATEGORY_SPECIALTY,
    GEAR_CATEGORY_SPEED
} GearCategoryKind;

typedef struct {
    GearStat stat;
    GearTrigger on;
    GearBasis basis;
    int value;
    PassiveFilter recipient;
    PassiveFilter wearer;
    PassiveFilter versus;
} GearEffect;

typedef struct {
    GearCategoryKind kind;
    char value[24];
} GearCategory;

typedef struct {
    char id[64];
    char title_key[GEAR_TEXT_KEY_CAPACITY];
    char description_key[GEAR_TEXT_KEY_CAPACITY];
    char aura_keys[GEAR_AURA_KEY_CAPACITY][GEAR_TEXT_KEY_CAPACITY];
    int aura_key_count;
    char icon[72];
    int rarity;
    GearCategory categories[GEAR_CATEGORY_CAPACITY];
    int category_count;
    bool categories_all;
    bool absorbs_damage;
    GearEffect *effects;
    int effect_count;
} GearDef;

typedef struct {
    char id[32];
    bool gems;
    int price;
    int count;
    int min_rarity, max_rarity;
    int tome_chance_pct;
} GearPackDef;

typedef struct {
    GearDef defs[GEAR_CATALOG_CAPACITY];
    int count;
    GearPackDef packs[GEAR_PACK_CAPACITY];
    int pack_count;
    int rarity_gold_values[GEAR_RARITY_COUNT];
    int prestige_slot_gem_cost;
    bool loaded;
} GearCatalog;

typedef struct {
    int counts[GEAR_CATALOG_CAPACITY];
    int tomes;
} PlayerGear;

typedef enum {
    GEAR_EQUIP_OK,
    GEAR_EQUIP_NOT_OWNED,
    GEAR_EQUIP_INVALID_SLOT,
    GEAR_EQUIP_CATEGORY,
    GEAR_EQUIP_DUPLICATE,
    GEAR_EQUIP_AURAS_CANNOT_STACK,
    GEAR_EQUIP_INVALID_PRESTIGE,
    GEAR_EQUIP_PRESTIGE_LOCKED
} GearEquipResult;

static GearCatalog gear_catalog;

static GearStat GearStatFromString(const char *s) {
    static const struct {
        const char *name;
        GearStat stat;
    } names[] = {
        {"power", GEAR_STAT_POWER},
        {"health", GEAR_STAT_HEALTH},
        {"crit", GEAR_STAT_CRIT},
        {"bonus_damage", GEAR_STAT_BONUS_DAMAGE},
        {"damage_resist", GEAR_STAT_DAMAGE_RESIST},
        {"raw_damage_resist", GEAR_STAT_RAW_DAMAGE_RESIST},
        {"stun_resist", GEAR_STAT_STUN_RESIST},
        {"slow_resist", GEAR_STAT_SLOW_RESIST},
        {"drain_resist", GEAR_STAT_DRAIN_RESIST},
        {"curse_power_resist", GEAR_STAT_CURSE_POWER_RESIST},
        {"curse_health_resist", GEAR_STAT_CURSE_HEALTH_RESIST},
        {"defender", GEAR_STAT_DEFENDER},
        {"stealth", GEAR_STAT_STEALTH},
        {"perceptive", GEAR_STAT_PERCEPTIVE},
        {"haste", GEAR_STAT_HASTE},
        {"flank_reduction", GEAR_STAT_FLANK_REDUCTION},
        {"target_highest_power", GEAR_STAT_TARGET_HIGHEST_POWER},
        {"target_lowest_health", GEAR_STAT_TARGET_LOWEST_HEALTH},
        {"target_back_row", GEAR_STAT_TARGET_BACK_ROW},
        {"target_quick", GEAR_STAT_TARGET_QUICK},
        {"target_slowest", GEAR_STAT_TARGET_SLOWEST},
        {"target_match", GEAR_STAT_TARGET_MATCH},
        {"damage", GEAR_STAT_DAMAGE},
        {"drain", GEAR_STAT_DRAIN},
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (!strcmp(names[i].name, s)) return names[i].stat;
    return GEAR_STAT_NONE;
}

static bool GearTriggerFromString(const char *s, GearTrigger *out) {
    static const char *const names[] = {
        "battle",
        "self_hit",
        "ally_hit",
        "enemy_hit",
        "self_attack",
        "ally_attack",
        "enemy_death",
        "ally_death",
        "self_death",
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (strcmp(names[i], s)) continue;
        *out = (GearTrigger)i;
        return true;
    }
    return false;
}

static void GearFilterParseTerms(PassiveFilter *filter, const cJSON *terms, bool each) {
    filter->term_count = cJSON_GetArraySize(terms);
    if (filter->term_count <= 0) return;
    filter->has_match = true;
    filter->mode = each ? PASSIVE_MATCH_EACH : PASSIVE_MATCH_ANY;
    filter->terms = calloc((size_t)filter->term_count, sizeof(*filter->terms));
    if (!filter->terms) TraceLog(LOG_FATAL, "Gear: out of memory parsing match terms");
    int index = 0;
    const cJSON *term = NULL;
    cJSON_ArrayForEach(term, terms) {
        PassiveMatchTerm *dst = &filter->terms[index++];
        dst->kind = PassiveTermKindFromString(cJSON_GetStringValue(cJSON_GetArrayItem(term, 0)));
        StrCopy(dst->value, sizeof(dst->value), cJSON_GetStringValue(cJSON_GetArrayItem(term, 1)));
    }
}

static PassiveScope GearScopeFromString(const char *s) {
    return !strcmp(s, "enemies")  ? PASSIVE_SCOPE_ENEMIES
           : !strcmp(s, "allies") ? PASSIVE_SCOPE_ALLIES
                                  : PASSIVE_SCOPE_SELF;
}

static void GearEffectParse(GearEffect *effect, const cJSON *row, const char *gear_id) {
    effect->stat = GearStatFromString(JsonStr(row, "stat"));
    if (effect->stat == GEAR_STAT_NONE || !GearTriggerFromString(JsonStr(row, "on"), &effect->on))
        TraceLog(LOG_FATAL, "Gear: %s has an unknown effect", gear_id);
    effect->value = JsonInt(row, "value");
    effect->basis = !strcmp(JsonStr(row, "basis"), "damage") ? GEAR_BASIS_DAMAGE : GEAR_BASIS_BASE;
    bool each = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive((cJSON *)row, "each"));
    effect->recipient.scope = GearScopeFromString(JsonStr(row, "scope"));
    effect->recipient.include_source = true;
    GearFilterParseTerms(&effect->recipient, cJSON_GetObjectItemCaseSensitive((cJSON *)row, "who"), each);
    effect->wearer.scope = PASSIVE_SCOPE_SELF;
    GearFilterParseTerms(&effect->wearer, cJSON_GetObjectItemCaseSensitive((cJSON *)row, "wearer"), false);
    GearFilterParseTerms(&effect->versus, cJSON_GetObjectItemCaseSensitive((cJSON *)row, "vs"), each);
}

static void GearDefParse(GearDef *def, const cJSON *row) {
    StrCopy(def->id, sizeof(def->id), JsonStr(row, "id"));
    StrCopy(def->title_key, sizeof(def->title_key), JsonStr(row, "title_key"));
    StrCopy(def->description_key, sizeof(def->description_key), JsonStr(row, "description_key"));
    const cJSON *aura_key = NULL;
    cJSON_ArrayForEach(aura_key, cJSON_GetObjectItemCaseSensitive((cJSON *)row, "aura_keys")) {
        if (def->aura_key_count >= GEAR_AURA_KEY_CAPACITY || !cJSON_IsString(aura_key))
            TraceLog(LOG_FATAL, "Gear: %s has invalid aura keys", def->id);
        StrCopy(def->aura_keys[def->aura_key_count++], GEAR_TEXT_KEY_CAPACITY, aura_key->valuestring);
    }
    StrCopy(def->icon, sizeof(def->icon), JsonStr(row, "icon"));
    def->rarity = JsonInt(row, "rarity");
    def->categories_all = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive((cJSON *)row, "categories_all"));
    def->absorbs_damage = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive((cJSON *)row, "absorbs_damage"));
    const cJSON *category = NULL;
    cJSON_ArrayForEach(category, cJSON_GetObjectItemCaseSensitive((cJSON *)row, "categories")) {
        if (def->category_count >= GEAR_CATEGORY_CAPACITY)
            TraceLog(LOG_FATAL, "Gear: %s has too many categories", def->id);
        GearCategory *dst = &def->categories[def->category_count++];
        const char *kind = JsonStr(category, "kind");
        dst->kind = !strcmp(kind, "faction")      ? GEAR_CATEGORY_FACTION
                    : !strcmp(kind, "specialty")  ? GEAR_CATEGORY_SPECIALTY
                    : !strcmp(kind, "initiative") ? GEAR_CATEGORY_SPEED
                                                  : GEAR_CATEGORY_GROUP;
        StrCopy(dst->value, sizeof(dst->value), JsonStr(category, "value"));
    }
    const cJSON *effects = cJSON_GetObjectItemCaseSensitive((cJSON *)row, "effects");
    def->effect_count = cJSON_GetArraySize(effects);
    if (def->effect_count <= 0) return;
    def->effects = calloc((size_t)def->effect_count, sizeof(*def->effects));
    if (!def->effects) TraceLog(LOG_FATAL, "Gear: out of memory parsing effects");
    int index = 0;
    const cJSON *effect = NULL;
    cJSON_ArrayForEach(effect, effects) GearEffectParse(&def->effects[index++], effect, def->id);
}

static void GearLoadPacks(void);

void GearCatalogEnsure(void) {
    if (gear_catalog.loaded) return;
    gear_catalog.loaded = true;
    GearLoadPacks();
    cJSON *root = JsonLoadFile(GEAR_CATALOG_PATH);
    cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "items");
    int count = cJSON_IsArray(rows) ? cJSON_GetArraySize(rows) : 0;
    if (count <= 0 || count > GEAR_CATALOG_CAPACITY) TraceLog(LOG_FATAL, "Gear: invalid catalog %s", GEAR_CATALOG_PATH);
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, rows) GearDefParse(&gear_catalog.defs[gear_catalog.count++], row);
    int rarity = 0;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(root, "rarity_gold_values")) {
        if (rarity < GEAR_RARITY_COUNT) gear_catalog.rarity_gold_values[rarity++] = row->valueint;
    }
    gear_catalog.prestige_slot_gem_cost = JsonInt(root, "prestige_slot_gem_cost");
    cJSON_Delete(root);
}

const char *GearTitle(const GearDef *def) {
    return LocGet(def->title_key);
}

const char *GearDescription(const GearDef *def) {
    return LocGet(def->description_key);
}

void GearFormatAura(const GearDef *def, char *dst, int capacity) {
    int used = 0;
    dst[0] = '\0';
    for (int i = 0; i < def->aura_key_count && used < capacity - 1; i++)
        used += snprintf(
            dst + used, (size_t)(capacity - used), "%s%s", i ? GEAR_AURA_SEPARATOR : "", LocGet(def->aura_keys[i])
        );
}

void GearFormatLimits(const GearDef *def, char *dst, int capacity) {
    int used = snprintf(dst, (size_t)capacity, "%s", LocGet("HODA_GEAR_LIMITED_TITLE"));
    if (def->category_count == 0) {
        snprintf(dst + used, (size_t)(capacity - used), "%s", LocGet("HODA_GEAR_NO_LIMITATIONS"));
        return;
    }
    for (int i = 0; i < def->category_count && used < capacity - 1; i++) {
        const char *key = TextFormat("GALLERY_%s_TITLE", def->categories[i].value);
        const char *name = LocGet(TextToUpper(key));
        used += snprintf(dst + used, (size_t)(capacity - used), "%s%s", i ? ", " : "", name);
    }
}

int GearCatalogCount(void) {
    GearCatalogEnsure();
    return gear_catalog.count;
}

const GearDef *GearDefAt(int index) {
    GearCatalogEnsure();
    return index >= 0 && index < gear_catalog.count ? &gear_catalog.defs[index] : NULL;
}

int GearDefIndex(const char *id) {
    GearCatalogEnsure();
    for (int i = 0; i < gear_catalog.count; i++)
        if (!strcmp(gear_catalog.defs[i].id, id)) return i;
    return -1;
}

static void GearLoadPacks(void) {
    cJSON *root = JsonLoadFile(GEAR_STORE_CATALOG_PATH);
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(root, "packs")) {
        if (strcmp(JsonStr(row, "kind"), "gear")) continue;
        if (gear_catalog.pack_count >= GEAR_PACK_CAPACITY) TraceLog(LOG_FATAL, "Gear: too many packs");
        GearPackDef *pack = &gear_catalog.packs[gear_catalog.pack_count++];
        const cJSON *offer = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive((cJSON *)row, "offers"), 0);
        StrCopy(pack->id, sizeof(pack->id), JsonStr(row, "id"));
        pack->gems = !strcmp(JsonStr(offer, "currency"), "gems");
        pack->price = JsonInt(offer, "price");
        pack->count = JsonInt(row, "count");
        pack->min_rarity = JsonInt(row, "min_rarity");
        pack->max_rarity = JsonInt(row, "max_rarity");
        pack->tome_chance_pct = JsonInt(row, "tome_chance_pct");
        if (pack->count <= 0 || pack->count > GEAR_PACK_MAX_ITEMS)
            TraceLog(LOG_FATAL, "Gear: invalid pack %s", pack->id);
    }
    cJSON_Delete(root);
}

int GearPackCount(void) {
    GearCatalogEnsure();
    return gear_catalog.pack_count;
}

const GearPackDef *GearPackAt(int index) {
    GearCatalogEnsure();
    return index >= 0 && index < gear_catalog.pack_count ? &gear_catalog.packs[index] : NULL;
}

int GearPrestigeSlotGemCost(void) {
    GearCatalogEnsure();
    return gear_catalog.prestige_slot_gem_cost;
}

const GearDef *GearInSlot(const Character *ch, int slot) {
    if (!ch || slot < 0 || slot >= GEAR_SLOT_CAPACITY || ch->gear.defs[slot] == GEAR_EMPTY_SLOT) return NULL;
    return GearDefAt(ch->gear.defs[slot] - 1);
}

int GearSlotCount(const Character *ch) {
    return ch->gear_slots < GEAR_SLOT_CAPACITY ? ch->gear_slots : GEAR_SLOT_CAPACITY;
}

bool GearHasPrestigeSlot(const Character *ch) {
    return ch->has_prestige_slot && GearSlotCount(ch) >= GEAR_PRESTIGE_MIN_SLOTS;
}

bool GearSlotIsPrestige(const Character *ch, int slot) {
    return GearHasPrestigeSlot(ch) && slot == GearSlotCount(ch) - 1;
}

bool GearHasEquipped(const Character *ch) {
    for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++)
        if (ch->gear.defs[slot] != GEAR_EMPTY_SLOT) return true;
    return false;
}

static bool GearCategoryMatches(const GearCategory *category, const Character *ch) {
    switch (category->kind) {
        case GEAR_CATEGORY_FACTION:
            return (!strcmp(category->value, "White") && FactionIsWhite(ch->faction)) ||
                   (!strcmp(category->value, "Black") && FactionIsBlack(ch->faction)) ||
                   (!strcmp(category->value, "Red") && FactionIsRed(ch->faction)) ||
                   (!strcmp(category->value, "Blue") && FactionIsBlue(ch->faction));
        case GEAR_CATEGORY_GROUP:
            for (int i = 0; i < ch->group_count; i++)
                if (!strcasecmp(ch->groups[i], category->value)) return true;
            return false;
        case GEAR_CATEGORY_SPECIALTY: {
            int specialty = SpecialtyFromString(category->value);
            for (int i = 0; i < ch->specialty_count; i++)
                if ((int)ch->specialties[i] == specialty) return true;
            return false;
        }
        case GEAR_CATEGORY_SPEED:
            return (!strcmp(category->value, "Quick") && ch->speed == SPEED_QUICK) ||
                   (!strcmp(category->value, "Normal") && ch->speed == SPEED_NORMAL) ||
                   (!strcmp(category->value, "Slow") && ch->speed == SPEED_SLOW);
    }
    return false;
}

bool GearUnitCanWear(const Character *ch, const GearDef *def) {
    if (def->category_count == 0) return true;
    int matched = 0;
    for (int i = 0; i < def->category_count; i++)
        if (GearCategoryMatches(&def->categories[i], ch)) matched++;
    return def->categories_all ? matched == def->category_count : matched > 0;
}

bool GearUnitIsWearing(const Character *ch, int def_index, int ignored_slot) {
    for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++)
        if (slot != ignored_slot && ch->gear.defs[slot] == def_index + 1) return true;
    return false;
}

static bool GearUnitHasDamageAbsorb(const Character *ch, int ignored_slot) {
    for (int i = 0; i < ch->specialty_count; i++)
        if (ch->specialties[i] == SPEC_DEFENDER) return true;
    for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++) {
        const GearDef *worn = slot == ignored_slot ? NULL : GearInSlot(ch, slot);
        if (worn && worn->absorbs_damage) return true;
    }
    return false;
}

GearEquipResult GearCheckEquip(const PlayerGear *gear, const Character *ch, int slot, int def_index) {
    const GearDef *def = GearDefAt(def_index);
    if (!def || gear->counts[def_index] <= 0) return GEAR_EQUIP_NOT_OWNED;
    if (slot < 0 || slot >= GearSlotCount(ch)) return GEAR_EQUIP_INVALID_SLOT;
    if (!GearUnitCanWear(ch, def)) return GEAR_EQUIP_CATEGORY;
    if (GearUnitIsWearing(ch, def_index, slot)) return GEAR_EQUIP_DUPLICATE;
    if (def->absorbs_damage && GearUnitHasDamageAbsorb(ch, slot)) return GEAR_EQUIP_AURAS_CANNOT_STACK;
    if (GearSlotIsPrestige(ch, slot)) {
        if (!ch->gear.prestige_unlocked) return GEAR_EQUIP_PRESTIGE_LOCKED;
        if (def->rarity < GEAR_RARITY_LEGENDARY) return GEAR_EQUIP_INVALID_PRESTIGE;
    }
    return GEAR_EQUIP_OK;
}

GearEquipResult GearEquip(PlayerGear *gear, Character *ch, int slot, int def_index) {
    GearEquipResult result = GearCheckEquip(gear, ch, slot, def_index);
    if (result != GEAR_EQUIP_OK) return result;
    gear->counts[def_index]--;
    ch->gear.defs[slot] = (short)(def_index + 1);
    return GEAR_EQUIP_OK;
}

int GearSellValue(const GearDef *def) {
    GearCatalogEnsure();
    return def->rarity >= 1 && def->rarity <= GEAR_RARITY_COUNT ? gear_catalog.rarity_gold_values[def->rarity - 1] : 0;
}

bool GearGrant(PlayerGear *gear, int def_index) {
    if (def_index < 0 || def_index >= GearCatalogCount()) return false;
    gear->counts[def_index]++;
    return true;
}

int GearInventoryTotal(const PlayerGear *gear) {
    int total = 0;
    for (int i = 0; i < GearCatalogCount(); i++) total += gear->counts[i];
    return total;
}

static int GearInventoryCompare(const void *a, const void *b) {
    const GearDef *first = GearDefAt(*(const int *)a), *second = GearDefAt(*(const int *)b);
    if (first->rarity != second->rarity) return second->rarity - first->rarity;
    return strcmp(second->id, first->id);
}

int GearInventorySorted(const PlayerGear *gear, int *out, int capacity) {
    int count = 0;
    for (int i = 0; i < GearCatalogCount() && count < capacity; i++)
        if (gear->counts[i] > 0) out[count++] = i;
    qsort(out, (size_t)count, sizeof(*out), GearInventoryCompare);
    return count;
}

static uint32_t GearNextRandom(uint32_t *rng) {
    *rng = *rng * 1664525u + 1013904223u;
    return *rng >> 8;
}

static int GearRollRarity(const GearPackDef *pack, uint32_t *rng) {
    int total = 0;
    for (int rarity = pack->min_rarity; rarity <= pack->max_rarity; rarity++) total += 1 << (pack->max_rarity - rarity);
    int roll = (int)(GearNextRandom(rng) % (uint32_t)total);
    for (int rarity = pack->min_rarity; rarity <= pack->max_rarity; rarity++) {
        roll -= 1 << (pack->max_rarity - rarity);
        if (roll < 0) return rarity;
    }
    return pack->max_rarity;
}

bool GearRollTome(const GearPackDef *pack, uint32_t *rng) {
    return pack->tome_chance_pct > 0 && (int)(GearNextRandom(rng) % 100u) < pack->tome_chance_pct;
}

int GearRollFromPack(const GearPackDef *pack, uint32_t *rng) {
    int rarity = GearRollRarity(pack, rng);
    int count = 0;
    for (int i = 0; i < GearCatalogCount(); i++)
        if (GearDefAt(i)->rarity == rarity) count++;
    if (count == 0) return -1;
    int pick = (int)(GearNextRandom(rng) % (uint32_t)count);
    for (int i = 0; i < GearCatalogCount(); i++)
        if (GearDefAt(i)->rarity == rarity && pick-- == 0) return i;
    return -1;
}

static struct {
    char ref[72];
    Texture2D texture;
} gear_icons[GEAR_ICON_CACHE_CAPACITY];
static int gear_icon_count;

static Texture2D GearTextureLoad(const char *ref) {
    for (int i = 0; i < gear_icon_count; i++)
        if (!strcmp(gear_icons[i].ref, ref)) return gear_icons[i].texture;
    const char *path = TextFormat(EXTRA_ASSET_DIR "%s", ref);
    Texture2D texture = ResourceExists(path) ? ResourceLoadTexture(path) : (Texture2D){0};
    if (texture.id == 0) TraceLog(LOG_WARNING, "Gear: missing icon %s", ref);
    if (texture.id != 0) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    }
    if (gear_icon_count < GEAR_ICON_CACHE_CAPACITY) {
        StrCopy(gear_icons[gear_icon_count].ref, sizeof(gear_icons[gear_icon_count].ref), ref);
        gear_icons[gear_icon_count++].texture = texture;
    }
    return texture;
}

Texture2D GearIconLoad(const GearDef *def) {
    return GearTextureLoad(def->icon);
}

Texture2D GearUiTexture(const char *name) {
    char ref[72];
    snprintf(ref, sizeof(ref), GEAR_UI_TEXTURE_PATH, name);
    return GearTextureLoad(ref);
}

Texture2D GearRarityFrame(int rarity) {
    static const char *const frames[GEAR_RARITY_COUNT] = {
        "tex.gear_frame_common",
        "tex.gear_frame_uncommon",
        "tex.gear_frame_rare",
        "tex.gear_frame_epic",
        "tex.gear_frame_legendary",
        "tex.gear_frame_mythic",
    };
    return GearUiTexture(frames[rarity >= 1 && rarity <= GEAR_RARITY_COUNT ? rarity - 1 : 0]);
}

Texture2D GearTomeIconLoad(void) {
    return GearTextureLoad(GEAR_TOME_ICON);
}

bool GearHasTomeSlot(const Character *ch) {
    return ch->rarity < RARITY_MYTHIC && GearHasPrestigeSlot(ch);
}

int GearNormalSlotsFilled(const Character *ch) {
    int filled = 0;
    for (int slot = 0; slot < GEAR_NORMAL_SLOT_MAX; slot++)
        if (ch->gear.defs[slot] != GEAR_EMPTY_SLOT) filled++;
    return filled;
}

bool GearCanAscend(const Character *ch) {
    return GearHasTomeSlot(ch) && GearNormalSlotsFilled(ch) >= GEAR_NORMAL_SLOT_MAX;
}

void GearApplyAscension(Character *ch) {
    if (ch->ascended) return;
    ch->ascended = true;
    ch->rarity = RARITY_MYTHIC;
    ch->gear_slots += GEAR_ASCENSION_BONUS_SLOTS;
    for (int tier = 0; tier < TIER_COUNT; tier++) ch->stats[tier].max_level += GEAR_ASCENSION_BONUS_LEVELS;
}

bool GearAscend(PlayerGear *gear, Character *ch) {
    if (gear->tomes <= 0 || !GearCanAscend(ch)) return false;
    int old_prestige_slot = GearSlotCount(ch) - 1;
    gear->tomes--;
    GearApplyAscension(ch);
    ch->gear.defs[GearSlotCount(ch) - 1] = ch->gear.defs[old_prestige_slot];
    ch->gear.defs[old_prestige_slot] = GEAR_EMPTY_SLOT;
    return true;
}

void GearIconsUnload(void) {
    for (int i = 0; i < gear_icon_count; i++)
        if (gear_icons[i].texture.id != 0) UnloadTexture(gear_icons[i].texture);
    gear_icon_count = 0;
}
