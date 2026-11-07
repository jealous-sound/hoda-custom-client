#define EXPEDITIONS_PATH "data/expeditions.json"
#define EXPEDITIONS_VERSION 1
#define EXPEDITION_CAPACITY 24
#define EXPEDITION_SLOT_CAPACITY 5
#define EXPEDITION_PARTY_CAPACITY 5
#define EXPEDITION_REQUIREMENT_CAPACITY 2
#define EXPEDITION_DROP_CAPACITY 4
#define EXPEDITION_ID_CAPACITY 32
#define EXPEDITION_KEY_CAPACITY 64
#define EXPEDITION_ART_CAPACITY 96
#define EXPEDITION_GROUP_CAPACITY 24
#define EXPEDITION_NONE (-1)
#define EXPEDITION_SECONDS_PER_MINUTE 60
#define EXPEDITION_PERCENT 100
#define EXPEDITION_TEXTURE_CACHE_CAPACITY 24
#define EXPEDITION_AWAY_BADGE_ART "gui/expeditions/away_badge.png"
#define EXPEDITIONS_HOME_ART "gui/expeditions/button.png"
#define EXPEDITIONS_HOME_ART_ON "gui/expeditions/button_on.png"

typedef enum {
    EXPEDITION_TRAIT_NONE,
    EXPEDITION_TRAIT_GROUP,
    EXPEDITION_TRAIT_FACTION,
    EXPEDITION_TRAIT_RARITY,
    EXPEDITION_TRAIT_GIANT,
} ExpeditionTraitKind;

typedef struct {
    ExpeditionTraitKind kind;
    char group[EXPEDITION_GROUP_CAPACITY];
    Faction faction;
    Rarity rarity;
    int count;
} ExpeditionTrait;

typedef enum {
    EXPEDITION_DROP_GEAR,
    EXPEDITION_DROP_RUNE,
    EXPEDITION_DROP_HERO,
} ExpeditionDropKind;

typedef struct {
    ExpeditionDropKind kind;
    int chance;
    int min_rarity;
    int max_rarity;
    int hero_weights[RARITY_MYTHIC + 1];
} ExpeditionDrop;

typedef struct {
    char id[EXPEDITION_ID_CAPACITY];
    char name_key[EXPEDITION_KEY_CAPACITY];
    char description_key[EXPEDITION_KEY_CAPACITY];
    char art[EXPEDITION_ART_CAPACITY];
    int level;
    int minutes;
    int party;
    ExpeditionTrait requirements[EXPEDITION_REQUIREMENT_CAPACITY];
    int requirement_count;
    ExpeditionTrait bonus;
    int gold_min, gold_max;
    int gems_min, gems_max;
    int hero_xp;
    ExpeditionDrop drops[EXPEDITION_DROP_CAPACITY];
    int drop_count;
} ExpeditionDef;

typedef struct {
    bool loaded;
    int slot_levels[EXPEDITION_SLOT_CAPACITY];
    int slot_count;
    int great_success_pct;
    ExpeditionDef defs[EXPEDITION_CAPACITY];
    int count;
} ExpeditionCatalog;

typedef struct {
    int def;
    uint64_t heroes[EXPEDITION_PARTY_CAPACITY];
    int hero_count;
    long long started_at;
    long long ends_at;
} PlayerExpedition;

typedef struct {
    PlayerExpedition active[EXPEDITION_SLOT_CAPACITY];
    int count;
} PlayerExpeditions;

static ExpeditionCatalog expedition_catalog;

static struct {
    char ref[EXPEDITION_ART_CAPACITY];
    Texture2D texture;
} expedition_textures[EXPEDITION_TEXTURE_CACHE_CAPACITY];
static int expedition_texture_count;

static const char *const EXPEDITION_DROP_KIND_NAMES[] = {
    [EXPEDITION_DROP_GEAR] = "gear",
    [EXPEDITION_DROP_RUNE] = "rune",
    [EXPEDITION_DROP_HERO] = "hero",
};

static bool ExpeditionRange(const cJSON *json, const char *key, int *low, int *high) {
    const cJSON *range = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_GetArraySize(range) != 2) return false;
    *low = cJSON_GetArrayItem(range, 0)->valueint;
    *high = cJSON_GetArrayItem(range, 1)->valueint;
    return *low >= 0 && *high >= *low;
}

static bool ExpeditionTraitParse(const cJSON *json, ExpeditionTrait *trait) {
    *trait = (ExpeditionTrait){.count = JsonInt(json, "count")};
    if (cJSON_IsString(cJSON_GetObjectItemCaseSensitive(json, "group"))) {
        trait->kind = EXPEDITION_TRAIT_GROUP;
        StrCopy(trait->group, sizeof(trait->group), JsonStr(json, "group"));
    } else if (cJSON_IsString(cJSON_GetObjectItemCaseSensitive(json, "faction"))) {
        trait->kind = EXPEDITION_TRAIT_FACTION;
        trait->faction = FactionFromString(JsonStr(json, "faction"));
    } else if (cJSON_IsString(cJSON_GetObjectItemCaseSensitive(json, "rarity"))) {
        trait->kind = EXPEDITION_TRAIT_RARITY;
        trait->rarity = RarityFromString(JsonStr(json, "rarity"));
    } else if (JsonBool(json, "giant")) {
        trait->kind = EXPEDITION_TRAIT_GIANT;
    }
    return trait->kind != EXPEDITION_TRAIT_NONE && trait->count >= 1 && trait->count <= EXPEDITION_PARTY_CAPACITY;
}

static int ExpeditionDropKindFromName(const char *name) {
    for (int kind = 0; kind < (int)(sizeof(EXPEDITION_DROP_KIND_NAMES) / sizeof(EXPEDITION_DROP_KIND_NAMES[0])); kind++)
        if (!strcmp(name, EXPEDITION_DROP_KIND_NAMES[kind])) return kind;
    return EXPEDITION_NONE;
}

static bool ExpeditionDropParse(const cJSON *json, ExpeditionDrop *drop) {
    int kind = ExpeditionDropKindFromName(JsonStr(json, "kind"));
    *drop = (ExpeditionDrop){
        .kind = (ExpeditionDropKind)kind,
        .chance = JsonInt(json, "chance"),
        .min_rarity = JsonInt(json, "min_rarity"),
        .max_rarity = JsonInt(json, "max_rarity"),
    };
    if (kind == EXPEDITION_NONE || drop->chance <= 0 || drop->chance > EXPEDITION_PERCENT) return false;
    if (kind != EXPEDITION_DROP_HERO) return drop->min_rarity >= 1 && drop->max_rarity >= drop->min_rarity;
    int total = 0;
    const cJSON *weight = NULL;
    cJSON_ArrayForEach(weight, cJSON_GetObjectItemCaseSensitive(json, "weights")) {
        Rarity rarity = RarityFromString(weight->string);
        if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY || !cJSON_IsNumber(weight) || weight->valueint < 0)
            return false;
        drop->hero_weights[rarity] = weight->valueint;
        total += weight->valueint;
    }
    return total > 0;
}

static bool ExpeditionDefParse(const cJSON *json, ExpeditionDef *def) {
    *def = (ExpeditionDef){
        .level = JsonInt(json, "level"),
        .minutes = JsonInt(json, "minutes"),
        .party = JsonInt(json, "party"),
        .hero_xp = JsonInt(json, "hero_xp"),
    };
    StrCopy(def->id, sizeof(def->id), JsonStr(json, "id"));
    StrCopy(def->name_key, sizeof(def->name_key), JsonStr(json, "name_key"));
    StrCopy(def->description_key, sizeof(def->description_key), JsonStr(json, "description_key"));
    StrCopy(def->art, sizeof(def->art), JsonStr(json, "art"));
    bool ok = def->id[0] && def->name_key[0] && def->minutes > 0 && def->party >= 1 &&
              def->party <= EXPEDITION_PARTY_CAPACITY && def->hero_xp >= 0 &&
              ExpeditionRange(json, "gold", &def->gold_min, &def->gold_max) &&
              ExpeditionRange(json, "gems", &def->gems_min, &def->gems_max) &&
              ExpeditionTraitParse(cJSON_GetObjectItemCaseSensitive(json, "bonus"), &def->bonus);
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(json, "requirements")) {
        if (def->requirement_count == EXPEDITION_REQUIREMENT_CAPACITY) return false;
        ok = ExpeditionTraitParse(row, &def->requirements[def->requirement_count++]) && ok;
    }
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(json, "drops")) {
        if (def->drop_count == EXPEDITION_DROP_CAPACITY) return false;
        ok = ExpeditionDropParse(row, &def->drops[def->drop_count++]) && ok;
    }
    return ok;
}

bool ExpeditionCatalogLoad(const char *path) {
    memset(&expedition_catalog, 0, sizeof(expedition_catalog));
    char *text = FileExists(path) ? LoadFileText(path) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    if (text) UnloadFileText(text);
    ExpeditionCatalog *c = &expedition_catalog;
    bool ok = root && JsonInt(root, "version") == EXPEDITIONS_VERSION;
    c->great_success_pct = JsonInt(root, "great_success_pct");
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(root, "slot_levels")) {
        if (!ok || c->slot_count == EXPEDITION_SLOT_CAPACITY || !cJSON_IsNumber(row)) {
            ok = false;
            break;
        }
        c->slot_levels[c->slot_count++] = row->valueint;
    }
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(root, "expeditions")) {
        if (!ok || c->count == EXPEDITION_CAPACITY || !ExpeditionDefParse(row, &c->defs[c->count])) {
            ok = false;
            break;
        }
        c->count++;
    }
    cJSON_Delete(root);
    ok = ok && c->slot_count > 0 && c->count > 0 && c->great_success_pct >= 0;
    if (!ok) {
        TraceLog(LOG_WARNING, "Expeditions: %s is missing or invalid, expeditions are off", path);
        memset(c, 0, sizeof(*c));
    }
    c->loaded = ok;
    return ok;
}

bool ExpeditionsAvailable(void) {
    return expedition_catalog.loaded;
}

int ExpeditionCount(void) {
    return expedition_catalog.count;
}

const ExpeditionDef *ExpeditionAt(int index) {
    return index >= 0 && index < expedition_catalog.count ? &expedition_catalog.defs[index] : NULL;
}

int ExpeditionIndexById(const char *id) {
    for (int i = 0; i < expedition_catalog.count; i++)
        if (!strcmp(expedition_catalog.defs[i].id, id)) return i;
    return EXPEDITION_NONE;
}

int ExpeditionUnlockLevel(void) {
    return expedition_catalog.slot_count > 0 ? expedition_catalog.slot_levels[0] : INT_MAX;
}

int ExpeditionSlotsUnlocked(int player_level) {
    int slots = 0;
    for (int i = 0; i < expedition_catalog.slot_count; i++) slots += player_level >= expedition_catalog.slot_levels[i];
    return slots;
}

int ExpeditionNextSlotLevel(int player_level) {
    for (int i = 0; i < expedition_catalog.slot_count; i++)
        if (player_level < expedition_catalog.slot_levels[i]) return expedition_catalog.slot_levels[i];
    return 0;
}

bool ExpeditionUnlocked(const ExpeditionDef *def, int player_level) {
    return def && player_level >= def->level && player_level >= ExpeditionUnlockLevel();
}

long long ExpeditionDurationSeconds(const ExpeditionDef *def) {
    return (long long)def->minutes * EXPEDITION_SECONDS_PER_MINUTE;
}

bool ExpeditionTraitMatches(const ExpeditionTrait *trait, const Character *ch) {
    switch (trait->kind) {
        case EXPEDITION_TRAIT_GROUP:
            for (int i = 0; i < ch->group_count; i++)
                if (!strcmp(ch->groups[i], trait->group)) return true;
            return false;
        case EXPEDITION_TRAIT_FACTION:
            return !ch->giant && FactionsOverlap(ch->faction, trait->faction);
        case EXPEDITION_TRAIT_RARITY:
            return ch->rarity >= trait->rarity;
        case EXPEDITION_TRAIT_GIANT:
            return ch->giant;
        default:
            return false;
    }
}

int ExpeditionTraitMatchCount(const ExpeditionTrait *trait, const Character *const *party, int count) {
    int matches = 0;
    for (int i = 0; i < count; i++) matches += ExpeditionTraitMatches(trait, party[i]);
    return matches;
}

bool ExpeditionTraitMet(const ExpeditionTrait *trait, const Character *const *party, int count) {
    return ExpeditionTraitMatchCount(trait, party, count) >= trait->count;
}

bool ExpeditionRequirementsMet(const ExpeditionDef *def, const Character *const *party, int count) {
    for (int i = 0; i < def->requirement_count; i++)
        if (!ExpeditionTraitMet(&def->requirements[i], party, count)) return false;
    return true;
}

bool ExpeditionGreatSuccess(const ExpeditionDef *def, const Character *const *party, int count) {
    return ExpeditionTraitMet(&def->bonus, party, count);
}

int ExpeditionGreatSuccessAmount(int amount, bool great_success) {
    if (!great_success || amount <= 0) return amount;
    long long boosted = (long long)amount * (EXPEDITION_PERCENT + expedition_catalog.great_success_pct);
    boosted /= EXPEDITION_PERCENT;
    return boosted > INT_MAX ? INT_MAX : (int)boosted;
}

int ExpeditionHeroDropLimit(const ExpeditionDef *def) {
    int heroes = 0;
    for (int i = 0; i < def->drop_count; i++) heroes += def->drops[i].kind == EXPEDITION_DROP_HERO;
    return heroes;
}

int ExpeditionsFind(const PlayerExpeditions *expeditions, int def) {
    for (int i = 0; i < expeditions->count; i++)
        if (expeditions->active[i].def == def) return i;
    return EXPEDITION_NONE;
}

bool ExpeditionsHoldInstance(const PlayerExpeditions *expeditions, uint64_t instance) {
    for (int i = 0; i < expeditions->count; i++)
        for (int hero = 0; hero < expeditions->active[i].hero_count; hero++)
            if (expeditions->active[i].heroes[hero] == instance) return true;
    return false;
}

bool ExpeditionReady(const PlayerExpedition *expedition, long long now) {
    return now >= expedition->ends_at;
}

long long ExpeditionRemaining(const PlayerExpedition *expedition, long long now) {
    return expedition->ends_at > now ? expedition->ends_at - now : 0;
}

float ExpeditionProgress(const PlayerExpedition *expedition, long long now) {
    long long span = expedition->ends_at - expedition->started_at;
    if (span <= 0 || now >= expedition->ends_at) return 1.0f;
    if (now <= expedition->started_at) return 0.0f;
    return (float)(now - expedition->started_at) / (float)span;
}

int ExpeditionsReadyCount(const PlayerExpeditions *expeditions, long long now) {
    int ready = 0;
    for (int i = 0; i < expeditions->count; i++) ready += ExpeditionReady(&expeditions->active[i], now);
    return ready;
}

void ExpeditionsRemoveAt(PlayerExpeditions *expeditions, int index) {
    if (index < 0 || index >= expeditions->count) return;
    for (int i = index; i + 1 < expeditions->count; i++) expeditions->active[i] = expeditions->active[i + 1];
    expeditions->count--;
}

cJSON *ExpeditionsSave(const PlayerExpeditions *expeditions) {
    cJSON *root = cJSON_CreateObject();
    cJSON *active = cJSON_AddArrayToObject(root, "active");
    bool ok = active != NULL;
    for (int i = 0; ok && i < expeditions->count; i++) {
        const PlayerExpedition *expedition = &expeditions->active[i];
        const ExpeditionDef *def = ExpeditionAt(expedition->def);
        cJSON *entry = cJSON_CreateObject();
        cJSON *heroes = entry ? cJSON_AddArrayToObject(entry, "heroes") : NULL;
        ok = def && heroes && cJSON_AddStringToObject(entry, "id", def->id) &&
             cJSON_AddNumberToObject(entry, "started_at", (double)expedition->started_at) &&
             cJSON_AddNumberToObject(entry, "ends_at", (double)expedition->ends_at);
        for (int hero = 0; ok && hero < expedition->hero_count; hero++)
            ok = cJSON_AddItemToArray(heroes, cJSON_CreateNumber((double)expedition->heroes[hero]));
        if (ok) ok = cJSON_AddItemToArray(active, entry);
        if (!ok) cJSON_Delete(entry);
    }
    if (!ok) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

static bool ExpeditionLoadEntry(const cJSON *entry, PlayerExpedition *expedition) {
    *expedition = (PlayerExpedition){
        .def = ExpeditionIndexById(JsonStr(entry, "id")),
        .started_at = (long long)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(entry, "started_at")),
        .ends_at = (long long)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(entry, "ends_at")),
    };
    const ExpeditionDef *def = ExpeditionAt(expedition->def);
    const cJSON *hero = NULL;
    cJSON_ArrayForEach(hero, cJSON_GetObjectItemCaseSensitive(entry, "heroes")) {
        if (!cJSON_IsNumber(hero) || hero->valuedouble < 1.0 || expedition->hero_count == EXPEDITION_PARTY_CAPACITY)
            return false;
        expedition->heroes[expedition->hero_count++] = (uint64_t)hero->valuedouble;
    }
    return def && expedition->hero_count == def->party && expedition->ends_at >= expedition->started_at;
}

void ExpeditionsLoad(const cJSON *json, PlayerExpeditions *expeditions) {
    memset(expeditions, 0, sizeof(*expeditions));
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "active")) {
        if (expeditions->count == EXPEDITION_SLOT_CAPACITY) break;
        PlayerExpedition expedition;
        if (!ExpeditionLoadEntry(entry, &expedition) || ExpeditionsFind(expeditions, expedition.def) >= 0) continue;
        bool shared = false;
        for (int hero = 0; hero < expedition.hero_count && !shared; hero++)
            shared = ExpeditionsHoldInstance(expeditions, expedition.heroes[hero]);
        if (!shared) expeditions->active[expeditions->count++] = expedition;
    }
}

Texture2D ExpeditionTexture(const char *art) {
    for (int i = 0; i < expedition_texture_count; i++)
        if (!strcmp(expedition_textures[i].ref, art)) return expedition_textures[i].texture;
    if (expedition_texture_count == EXPEDITION_TEXTURE_CACHE_CAPACITY) return (Texture2D){0};
    const char *path = TextFormat(EXTRA_ASSET_DIR "%s", art);
    Texture2D texture = ResourceExists(path) ? ResourceLoadTexture(path) : (Texture2D){0};
    if (texture.id) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    } else {
        TraceLog(LOG_WARNING, "Expeditions: missing art %s", art);
    }
    StrCopy(expedition_textures[expedition_texture_count].ref, EXPEDITION_ART_CAPACITY, art);
    expedition_textures[expedition_texture_count++].texture = texture;
    return texture;
}
