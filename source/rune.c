#include <limits.h>

#define RUNE_SLOT_COUNT 3
#define RUNE_NORMAL_SLOT_COUNT 2
#define RUNE_PRESTIGE_SLOT 2
#define RUNE_PRESTIGE_RARITY 7
#define RUNE_INVENTORY_CAPACITY 512
#define RUNE_CATALOG_CAPACITY 160
#define RUNE_SECONDS_PER_MINUTE 60LL
#define RUNE_INVENTORY_LIFETIME_SECONDS (30LL * NEXUS_SECONDS_PER_DAY)
#define RUNE_PRESTIGE_UNLOCK_GEMS 100
#define RUNE_RESIST_MAX_PCT 100
#define RUNE_OVERCHARGED_ID_MARK "-6-"
#define RUNE_SPECIAL_ID_MARK "Special"
#define RUNE_ACTIVATION_FX "fx_runeonunit_pre"
#define RUNE_ACTIVATION_FX_LIFT ((Vector3){0.0f, 0.93f, 0.0f})
#define RUNE_CATALOG_PATH "data/runes.json"
#define RUNE_RESOURCE_CACHE_PREFIX "cache:"

typedef enum {
    RUNE_BUFF_POWER,
    RUNE_BUFF_HEALTH,
    RUNE_BUFF_INITIATIVE,
    RUNE_BUFF_DAMAGE_SHIELD,
    RUNE_BUFF_DAMAGE_RESIST,
    RUNE_BUFF_RETALIATION,
    RUNE_BUFF_STUN_RESIST,
    RUNE_BUFF_SLOW_RESIST,
    RUNE_BUFF_DRAIN_RESIST,
    RUNE_BUFF_CURSE_POWER_IMMUNE,
    RUNE_BUFF_STEALTH,
    RUNE_BUFF_PERCEPTIVE,
    RUNE_BUFF_GOLD,
    RUNE_BUFF_XP,
    RUNE_BUFF_TARGET_HIGHEST_POWER,
    RUNE_BUFF_TARGET_HIGHEST_SPEED,
    RUNE_BUFF_TARGET_LOWEST_SPEED,
    RUNE_BUFF_TARGET_LOWEST_HEALTH,
    RUNE_BUFF_COUNT
} RuneBuff;

typedef enum {
    RUNE_TARGET_HIGHEST_POWER,
    RUNE_TARGET_HIGHEST_SPEED,
    RUNE_TARGET_LOWEST_SPEED,
    RUNE_TARGET_LOWEST_HEALTH,
    RUNE_TARGET_COUNT
} RuneTargetPreference;

typedef struct {
    char id[40];
    char family[32];
    char name_ru[64];
    char desc_ru[192];
    char buff_ru[64];
    char value_ru[24];
    char icon[96];
    char buff_icon[96];
    char craft_to_id[40];
    RuneBuff buff;
    int value;
    int duration_minutes;
    int rarity;
    int craft_to;
} RuneDef;

typedef struct {
    RuneDef defs[RUNE_CATALOG_CAPACITY];
    int count;
    bool loaded;
} RuneCatalog;

typedef struct {
    uint64_t uuid;
    int def;
    long long expire_at;
    long long inventory_expire_at;
} PlayerRune;

typedef struct {
    PlayerRune items[RUNE_INVENTORY_CAPACITY];
    int count;
    int seen;
    uint64_t next_uuid;
    uint64_t squad_slots[PLAYER_MAX_SQUADS][RUNE_SLOT_COUNT];
    bool prestige_unlocked[PLAYER_MAX_SQUADS];
} PlayerRunes;

typedef struct {
    int power_pct, health_pct;
    int initiative_steps;
    int damage_shield, damage_resist_pct, retaliation_pct;
    int stun_resist_pct, slow_resist_pct, drain_resist_pct;
    int gold_pct, xp_pct;
    int target_level[RUNE_TARGET_COUNT];
    bool curse_power_immune, stealth, perceptive;
} RuneBonus;

typedef enum {
    RUNE_ASSIGN_OK,
    RUNE_ASSIGN_NOT_AVAILABLE,
    RUNE_ASSIGN_SLOT_LOCKED,
    RUNE_ASSIGN_NEEDS_PRESTIGE_RUNE,
    RUNE_ASSIGN_NEEDS_NORMAL_SLOT
} RuneAssignResult;

typedef enum {
    RUNE_CRAFT_SUCCESS,
    RUNE_CRAFT_EXPIRED,
    RUNE_CRAFT_RUNES_NOT_FOUND,
    RUNE_CRAFT_MISMATCH,
    RUNE_CRAFT_NO_UPGRADE,
    RUNE_CRAFT_ALREADY_ASSIGNED,
    RUNE_CRAFT_INVENTORY_FULL
} RuneCraftResult;

typedef enum { RUNE_CRAFT_MODE_TYPE, RUNE_CRAFT_MODE_RARITY } RuneCraftMode;

static RuneCatalog rune_catalog;

long long RuneNow(void) {
    return (long long)time(NULL);
}

static RuneBuff RuneBuffFromType(const char *type) {
    static const struct {
        const char *type;
        RuneBuff buff;
    } table[RUNE_BUFF_COUNT] = {
        {"power", RUNE_BUFF_POWER},
        {"health", RUNE_BUFF_HEALTH},
        {"initiative", RUNE_BUFF_INITIATIVE},
        {"damage", RUNE_BUFF_DAMAGE_SHIELD},
        {"damage-resist-percent", RUNE_BUFF_DAMAGE_RESIST},
        {"damage-retaliation-percent", RUNE_BUFF_RETALIATION},
        {"stunned", RUNE_BUFF_STUN_RESIST},
        {"slow-resist", RUNE_BUFF_SLOW_RESIST},
        {"power-drain-resist", RUNE_BUFF_DRAIN_RESIST},
        {"power-cursed", RUNE_BUFF_CURSE_POWER_IMMUNE},
        {"stealth", RUNE_BUFF_STEALTH},
        {"perceptive", RUNE_BUFF_PERCEPTIVE},
        {"gold", RUNE_BUFF_GOLD},
        {"xp", RUNE_BUFF_XP},
        {"highest-power-unit", RUNE_BUFF_TARGET_HIGHEST_POWER},
        {"highest-speed-unit", RUNE_BUFF_TARGET_HIGHEST_SPEED},
        {"lowest-speed-unit", RUNE_BUFF_TARGET_LOWEST_SPEED},
        {"lowest-percent-health", RUNE_BUFF_TARGET_LOWEST_HEALTH},
    };
    for (int i = 0; i < RUNE_BUFF_COUNT; i++)
        if (!strcmp(table[i].type, type)) return table[i].buff;
    TraceLog(LOG_FATAL, "Runes: unknown buff type %s", type);
    return RUNE_BUFF_POWER;
}

int RuneDefIndex(const char *id) {
    for (int i = 0; i < rune_catalog.count; i++)
        if (!strcmp(rune_catalog.defs[i].id, id)) return i;
    return -1;
}

void RuneCatalogEnsure(void) {
    if (rune_catalog.loaded) return;
    rune_catalog.loaded = true;
    cJSON *root = JsonLoadFile(RUNE_CATALOG_PATH);
    cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "runes");
    int count = cJSON_IsArray(rows) ? cJSON_GetArraySize(rows) : 0;
    if (count <= 0 || count > RUNE_CATALOG_CAPACITY)
        TraceLog(LOG_FATAL, "Runes: invalid catalog %s", RUNE_CATALOG_PATH);
    int index = 0;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, rows) {
        RuneDef *def = &rune_catalog.defs[index++];
        StrCopy(def->id, sizeof(def->id), JsonStr(row, "id"));
        StrCopy(def->family, sizeof(def->family), JsonStr(row, "family"));
        StrCopy(def->name_ru, sizeof(def->name_ru), JsonStr(row, "name_ru"));
        StrCopy(def->desc_ru, sizeof(def->desc_ru), JsonStr(row, "desc_ru"));
        StrCopy(def->buff_ru, sizeof(def->buff_ru), JsonStr(row, "buff_ru"));
        StrCopy(def->value_ru, sizeof(def->value_ru), JsonStr(row, "value_ru"));
        StrCopy(def->icon, sizeof(def->icon), JsonStr(row, "icon"));
        StrCopy(def->buff_icon, sizeof(def->buff_icon), JsonStr(row, "buff_icon"));
        StrCopy(def->craft_to_id, sizeof(def->craft_to_id), JsonStr(row, "craft_to"));
        def->buff = RuneBuffFromType(JsonStr(row, "buff_type"));
        def->value = JsonInt(row, "buff_value");
        def->duration_minutes = JsonInt(row, "duration_minutes");
        def->rarity = JsonInt(row, "rarity");
    }
    rune_catalog.count = count;
    for (int i = 0; i < count; i++) {
        RuneDef *def = &rune_catalog.defs[i];
        def->craft_to = def->craft_to_id[0] ? RuneDefIndex(def->craft_to_id) : -1;
        if (def->craft_to_id[0] && def->craft_to < 0) TraceLog(LOG_FATAL, "Runes: %s crafts to nothing", def->id);
    }
    cJSON_Delete(root);
}

int RuneCatalogCount(void) {
    RuneCatalogEnsure();
    return rune_catalog.count;
}

const RuneDef *RuneDefAt(int index) {
    RuneCatalogEnsure();
    return index >= 0 && index < rune_catalog.count ? &rune_catalog.defs[index] : NULL;
}

const RuneDef *RuneDefFor(const PlayerRune *rune) {
    return rune ? RuneDefAt(rune->def) : NULL;
}

bool RuneIsActivated(const PlayerRune *rune) {
    return rune->expire_at > 0;
}

bool RuneIsExpired(const PlayerRune *rune, long long now) {
    return RuneIsActivated(rune) && rune->expire_at < now;
}

bool RuneHasExpiredFromInventory(const PlayerRune *rune, long long now) {
    return !RuneIsActivated(rune) && rune->inventory_expire_at < now;
}

bool RuneIsAvailable(const PlayerRune *rune, long long now) {
    return !RuneIsActivated(rune) && !RuneHasExpiredFromInventory(rune, now);
}

PlayerRune *RuneByUuid(PlayerRunes *runes, uint64_t uuid) {
    if (!uuid) return NULL;
    for (int i = 0; i < runes->count; i++)
        if (runes->items[i].uuid == uuid) return &runes->items[i];
    return NULL;
}

const PlayerRune *RuneByUuidConst(const PlayerRunes *runes, uint64_t uuid) {
    return RuneByUuid((PlayerRunes *)runes, uuid);
}

const PlayerRune *RuneInSlot(const PlayerRunes *runes, int squad, int slot) {
    if (squad < 0 || squad >= PLAYER_MAX_SQUADS || slot < 0 || slot >= RUNE_SLOT_COUNT) return NULL;
    return RuneByUuidConst(runes, runes->squad_slots[squad][slot]);
}

uint64_t RuneGrant(PlayerRunes *runes, int def, long long now) {
    if (!RuneDefAt(def) || runes->count >= RUNE_INVENTORY_CAPACITY) return 0;
    if (!runes->next_uuid) runes->next_uuid = 1;
    uint64_t uuid = runes->next_uuid++;
    runes->items[runes->count++] = (PlayerRune){
        .uuid = uuid,
        .def = def,
        .inventory_expire_at = now + RUNE_INVENTORY_LIFETIME_SECONDS,
    };
    return uuid;
}

uint64_t RuneGrantById(PlayerRunes *runes, const char *id, long long now) {
    RuneCatalogEnsure();
    return RuneGrant(runes, RuneDefIndex(id), now);
}

void RuneGrantStarterSet(PlayerRunes *runes, long long now) {
    if (runes->next_uuid) return;
    RuneGrantById(runes, "rune-power-1", now);
    RuneGrantById(runes, "rune-health-1", now);
    RuneGrantById(runes, "rune-knowledge-1", now);
    RuneGrantById(runes, "rune-greed-1", now);
}

static void RuneRemoveAt(PlayerRunes *runes, int index) {
    uint64_t uuid = runes->items[index].uuid;
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++)
        for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++)
            if (runes->squad_slots[squad][slot] == uuid) runes->squad_slots[squad][slot] = 0;
    runes->items[index] = runes->items[--runes->count];
}

static void RuneRemove(PlayerRunes *runes, uint64_t uuid) {
    PlayerRune *rune = RuneByUuid(runes, uuid);
    if (rune) RuneRemoveAt(runes, (int)(rune - runes->items));
}

int RuneExpire(PlayerRunes *runes, long long now) {
    int removed = 0;
    for (int i = runes->count - 1; i >= 0; i--) {
        const PlayerRune *rune = &runes->items[i];
        if (!RuneIsExpired(rune, now) && !RuneHasExpiredFromInventory(rune, now)) continue;
        RuneRemoveAt(runes, i);
        removed++;
    }
    return removed;
}

int RuneAvailableCount(const PlayerRunes *runes, long long now) {
    int count = 0;
    for (int i = 0; i < runes->count; i++) count += RuneIsAvailable(&runes->items[i], now);
    return count;
}

int RuneUnseenCount(const PlayerRunes *runes, long long now) {
    int unseen = RuneAvailableCount(runes, now) - runes->seen;
    return unseen > 0 ? unseen : 0;
}

void RuneMarkSeen(PlayerRunes *runes, long long now) {
    runes->seen = RuneAvailableCount(runes, now);
}

static int RuneSortCompare(const PlayerRune *a, const PlayerRune *b) {
    const RuneDef *da = RuneDefFor(a), *db = RuneDefFor(b);
    if (da->rarity != db->rarity) return db->rarity - da->rarity;
    int family = strcmp(db->family, da->family);
    if (family) return family;
    if (da->value != db->value) return db->value - da->value;
    if (da->duration_minutes != db->duration_minutes) return db->duration_minutes - da->duration_minutes;
    if (a->inventory_expire_at != b->inventory_expire_at)
        return a->inventory_expire_at < b->inventory_expire_at ? -1 : 1;
    return a->uuid < b->uuid ? -1 : a->uuid > b->uuid;
}

int RuneAvailableSorted(const PlayerRunes *runes, long long now, bool descending, uint64_t *out, int capacity) {
    const PlayerRune *sorted[RUNE_INVENTORY_CAPACITY];
    int count = 0;
    for (int i = 0; i < runes->count; i++) {
        const PlayerRune *rune = &runes->items[i];
        if (!RuneIsAvailable(rune, now)) continue;
        int at = count++;
        while (at > 0 && RuneSortCompare(rune, sorted[at - 1]) < 0) {
            sorted[at] = sorted[at - 1];
            at--;
        }
        sorted[at] = rune;
    }
    if (count > capacity) count = capacity;
    for (int i = 0; i < count; i++) out[i] = sorted[descending ? i : count - 1 - i]->uuid;
    return count;
}

RuneAssignResult RuneCheckSlot(const PlayerRunes *runes, int squad, int slot, const PlayerRune *rune, long long now) {
    if (!rune || !RuneIsAvailable(rune, now) || squad < 0 || squad >= PLAYER_MAX_SQUADS || slot < 0 ||
        slot >= RUNE_SLOT_COUNT)
        return RUNE_ASSIGN_NOT_AVAILABLE;
    bool prestige_rune = RuneDefFor(rune)->rarity == RUNE_PRESTIGE_RARITY;
    if (slot == RUNE_PRESTIGE_SLOT) {
        if (!runes->prestige_unlocked[squad]) return RUNE_ASSIGN_SLOT_LOCKED;
        if (!prestige_rune) return RUNE_ASSIGN_NEEDS_PRESTIGE_RUNE;
    } else if (prestige_rune) {
        return RUNE_ASSIGN_NEEDS_NORMAL_SLOT;
    }
    return RUNE_ASSIGN_OK;
}

RuneAssignResult RuneActivate(PlayerRunes *runes, int squad, int slot, uint64_t uuid, long long now) {
    PlayerRune *rune = RuneByUuid(runes, uuid);
    RuneAssignResult check = RuneCheckSlot(runes, squad, slot, rune, now);
    if (check != RUNE_ASSIGN_OK) return check;
    rune->expire_at = now + RuneDefFor(rune)->duration_minutes * RUNE_SECONDS_PER_MINUTE;
    uint64_t replaced = runes->squad_slots[squad][slot];
    runes->squad_slots[squad][slot] = uuid;
    if (replaced) RuneRemove(runes, replaced);
    return RUNE_ASSIGN_OK;
}

bool RuneCanCraft(const RuneDef *def) {
    return def && def->craft_to >= 0;
}

RuneCraftResult RuneCraft(PlayerRunes *runes, uint64_t first, uint64_t second, long long now, int *out_def) {
    const PlayerRune *a = RuneByUuidConst(runes, first), *b = RuneByUuidConst(runes, second);
    if (!a || !b || a == b) return RUNE_CRAFT_RUNES_NOT_FOUND;
    if (RuneIsActivated(a) || RuneIsActivated(b)) return RUNE_CRAFT_ALREADY_ASSIGNED;
    if (RuneHasExpiredFromInventory(a, now) || RuneHasExpiredFromInventory(b, now)) return RUNE_CRAFT_EXPIRED;
    if (a->def != b->def) return RUNE_CRAFT_MISMATCH;
    int target = RuneDefFor(a)->craft_to;
    if (target < 0) return RUNE_CRAFT_NO_UPGRADE;
    RuneRemove(runes, first);
    RuneRemove(runes, second);
    if (!RuneGrant(runes, target, now)) return RUNE_CRAFT_INVENTORY_FULL;
    if (out_def) *out_def = target;
    return RUNE_CRAFT_SUCCESS;
}

static bool RuneCraftAllMatches(const RuneDef *def, const RuneDef *pattern, RuneCraftMode mode) {
    return RuneCanCraft(def) && (mode == RUNE_CRAFT_MODE_TYPE ? def == pattern : def->rarity == pattern->rarity);
}

int RuneCraftAll(PlayerRunes *runes, RuneCraftMode mode, int pattern_def, long long now) {
    const RuneDef *pattern = RuneDefAt(pattern_def);
    if (!RuneCanCraft(pattern)) return 0;
    int crafted = 0;
    int defs = RuneCatalogCount();
    bool eligible[RUNE_CATALOG_CAPACITY];
    for (int def = 0; def < defs; def++) eligible[def] = RuneCraftAllMatches(RuneDefAt(def), pattern, mode);
    for (int def = 0; def < defs; def++) {
        if (!eligible[def]) continue;
        uint64_t pair[RUNE_INVENTORY_CAPACITY];
        int count = 0;
        for (int i = 0; i < runes->count; i++)
            if (runes->items[i].def == def && RuneIsAvailable(&runes->items[i], now))
                pair[count++] = runes->items[i].uuid;
        for (int i = 0; i + 1 < count; i += 2)
            if (RuneCraft(runes, pair[i], pair[i + 1], now, NULL) == RUNE_CRAFT_SUCCESS) crafted++;
    }
    return crafted;
}

static void RuneBonusAdd(RuneBonus *bonus, const RuneDef *def) {
    switch (def->buff) {
        case RUNE_BUFF_POWER:
            bonus->power_pct += def->value;
            break;
        case RUNE_BUFF_HEALTH:
            bonus->health_pct += def->value;
            break;
        case RUNE_BUFF_INITIATIVE:
            bonus->initiative_steps += def->value;
            break;
        case RUNE_BUFF_DAMAGE_SHIELD:
            bonus->damage_shield += def->value;
            break;
        case RUNE_BUFF_DAMAGE_RESIST:
            bonus->damage_resist_pct += def->value;
            break;
        case RUNE_BUFF_RETALIATION:
            bonus->retaliation_pct += def->value;
            break;
        case RUNE_BUFF_STUN_RESIST:
            bonus->stun_resist_pct += def->value;
            break;
        case RUNE_BUFF_SLOW_RESIST:
            bonus->slow_resist_pct += def->value;
            break;
        case RUNE_BUFF_DRAIN_RESIST:
            bonus->drain_resist_pct += def->value;
            break;
        case RUNE_BUFF_CURSE_POWER_IMMUNE:
            bonus->curse_power_immune = true;
            break;
        case RUNE_BUFF_STEALTH:
            bonus->stealth = true;
            break;
        case RUNE_BUFF_PERCEPTIVE:
            bonus->perceptive = true;
            break;
        case RUNE_BUFF_GOLD:
            bonus->gold_pct += def->value;
            break;
        case RUNE_BUFF_XP:
            bonus->xp_pct += def->value;
            break;
        case RUNE_BUFF_TARGET_HIGHEST_POWER:
            bonus->target_level[RUNE_TARGET_HIGHEST_POWER] += def->value;
            break;
        case RUNE_BUFF_TARGET_HIGHEST_SPEED:
            bonus->target_level[RUNE_TARGET_HIGHEST_SPEED] += def->value;
            break;
        case RUNE_BUFF_TARGET_LOWEST_SPEED:
            bonus->target_level[RUNE_TARGET_LOWEST_SPEED] += def->value;
            break;
        case RUNE_BUFF_TARGET_LOWEST_HEALTH:
            bonus->target_level[RUNE_TARGET_LOWEST_HEALTH] += def->value;
            break;
        default:
            break;
    }
}

RuneBonus RuneSquadBonus(const PlayerRunes *runes, int squad, long long now) {
    RuneBonus bonus = {0};
    for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++) {
        const PlayerRune *rune = RuneInSlot(runes, squad, slot);
        if (rune && !RuneIsExpired(rune, now)) RuneBonusAdd(&bonus, RuneDefFor(rune));
    }
    return bonus;
}

RuneBonus RuneDefsBonus(const int *defs, int count) {
    RuneBonus bonus = {0};
    for (int i = 0; i < count; i++) RuneBonusAdd(&bonus, RuneDefAt(defs[i]));
    return bonus;
}

int RuneActiveDefs(const PlayerRunes *runes, int squad, long long now, int out[RUNE_SLOT_COUNT]) {
    int count = 0;
    for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++) {
        const PlayerRune *rune = RuneInSlot(runes, squad, slot);
        if (rune && !RuneIsExpired(rune, now)) out[count++] = rune->def;
    }
    return count;
}

int RuneApplyPct(int value, int pct) {
    if (value <= 0 || pct <= 0) return value;
    double scaled = (double)value * (100.0 + pct) / 100.0;
    return scaled >= INT_MAX ? INT_MAX : (int)llround(scaled);
}

bool RuneDefRollable(const RuneDef *def) {
    return def->duration_minutes > 0 && !strstr(def->id, RUNE_OVERCHARGED_ID_MARK) &&
           !strstr(def->id, RUNE_SPECIAL_ID_MARK);
}

const char *RuneRarityLabel(int rarity) {
    static const char *const keys[] = {
        "", "RARITY_COMMON", "RARITY_UNCOMMON", "RARITY_RARE", "RARITY_EPIC", "RARITY_LEGENDARY"
    };
    if (rarity >= 1 && rarity <= 5) return LocGet(keys[rarity]);
    return rarity == RUNE_PRESTIGE_RARITY ? "Престижный" : "Мифический";
}

Color RuneRarityColor(int rarity) {
    static const Color colors[] = {
        {255, 255, 255, 255},
        {195, 141, 77, 255},
        {173, 171, 163, 255},
        {255, 242, 154, 255},
        {236, 78, 68, 255},
        {182, 252, 240, 255},
        {255, 0, 255, 255},
        {255, 200, 60, 255},
    };
    return colors[rarity >= 1 && rarity <= RUNE_PRESTIGE_RARITY ? rarity : 0];
}

void RuneFormatClock(long long seconds, char *dst, int cap) {
    if (seconds < 0) seconds = 0;
    snprintf(dst, cap, "%02d:%02d:%02d", (int)(seconds / 3600), (int)(seconds / 60 % 60), (int)(seconds % 60));
}

void RuneFormatExpiration(long long seconds, char *dst, int cap) {
    if (seconds < 0) seconds = 0;
    long long days = seconds / NEXUS_SECONDS_PER_DAY;
    if (days > 0)
        snprintf(dst, cap, "%d дн.", (int)days);
    else
        RuneFormatClock(seconds, dst, cap);
}

#define RUNE_ICON_STORE_ART_CAPACITY 96
#define RUNE_ICON_CACHE_CAPACITY (RUNE_CATALOG_CAPACITY + RUNE_ICON_STORE_ART_CAPACITY)

typedef struct {
    char ref[96];
    Texture2D texture;
} RuneIconEntry;

static RuneIconEntry rune_icons[RUNE_ICON_CACHE_CAPACITY];
static int rune_icon_count;

Texture2D RuneIconLoad(const char *ref) {
    for (int i = 0; i < rune_icon_count; i++)
        if (!strcmp(rune_icons[i].ref, ref)) return rune_icons[i].texture;
    const char *path = strncmp(ref, RUNE_RESOURCE_CACHE_PREFIX, strlen(RUNE_RESOURCE_CACHE_PREFIX)) == 0
                           ? ref
                           : TextFormat(EXTRA_ASSET_DIR "%s", ref);
    Texture2D texture = ResourceExists(path) ? ResourceLoadTexture(path) : (Texture2D){0};
    if (texture.id == 0) TraceLog(LOG_WARNING, "Runes: missing icon %s", ref);
    if (texture.id != 0) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    }
    if (rune_icon_count < RUNE_ICON_CACHE_CAPACITY) {
        StrCopy(rune_icons[rune_icon_count].ref, sizeof(rune_icons[rune_icon_count].ref), ref);
        rune_icons[rune_icon_count++].texture = texture;
    }
    return texture;
}

void RuneIconsUnload(void) {
    for (int i = 0; i < rune_icon_count; i++)
        if (rune_icons[i].texture.id != 0) UnloadTexture(rune_icons[i].texture);
    rune_icon_count = 0;
}
