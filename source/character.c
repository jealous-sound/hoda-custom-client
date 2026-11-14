#include <stdint.h>

#ifndef TIER_COUNT
#define TIER_COUNT 4
#endif

#ifndef MAX_GROUPS
#define MAX_GROUPS 8
#endif

#ifndef MAX_SPECIALTIES
#define MAX_SPECIALTIES 16
#endif

typedef enum { TIER_I, TIER_II, TIER_III, TIER_IV } Tier;

typedef enum {
    RARITY_COMMON = 1,
    RARITY_UNCOMMON = 2,
    RARITY_RARE = 3,
    RARITY_EPIC = 4,
    RARITY_LEGENDARY = 5,
    RARITY_MYTHIC = 6
} Rarity;

typedef enum { SPEED_SLOW, SPEED_NORMAL, SPEED_QUICK } Speed;

typedef enum {
    FACTION_LARGE,
    FACTION_WHITE,
    FACTION_RED,
    FACTION_BLUE,
    FACTION_BLACK,
    FACTION_WHITE_RED,
    FACTION_WHITE_BLUE,
    FACTION_RED_BLUE,
    FACTION_WHITE_BLACK,
    FACTION_RED_BLACK,
    FACTION_BLUE_BLACK,
    FACTION_ALL
} Faction;

typedef enum { TARGET_ONE, TARGET_ROW, TARGET_COLUMN, TARGET_ALL } TargetPattern;

typedef enum {
    ABILITY_ATTACK = 1 << 0,
    ABILITY_ATTACK_NONE = 1 << 1,
    ABILITY_POWER_DRAIN = 1 << 2,
    ABILITY_STUN = 1 << 3,
    ABILITY_SLOW = 1 << 4,
    ABILITY_CURSE_POWER = 1 << 5,
    ABILITY_CURSE_HEALTH = 1 << 6,
    ABILITY_HEAL_ALLIES = 1 << 7,
    ABILITY_HEAL_SELF = 1 << 8,
    ABILITY_GAIN_POWER_ALLIES = 1 << 9,
    ABILITY_GAIN_POWER_SELF = 1 << 10,
    ABILITY_MULTIATTACK = 1 << 11,
    ABILITY_MARK = 1 << 12,
    ABILITY_LEVY = 1 << 13,
    ABILITY_ASSASSINATE = 1 << 14,
    ABILITY_ATTACK_FRIENDLY = 1 << 15,
    ABILITY_DAMAGE_SELF = 1 << 16,
    ABILITY_LOSE_POWER_SELF = 1 << 17,
    ABILITY_SLOW_ALLIES = 1 << 18,
    ABILITY_GAIN_CRIT_ALLIES = 1 << 19,
    ABILITY_CURSE_TWO_ROUNDS = 1 << 20,
    ABILITY_GAIN_DAMAGE_ALLIES = 1 << 21,
    ABILITY_GAIN_DAMAGE_SELF = 1 << 22,
} AbilityMechanic;

typedef enum {
    SPEC_POWERGAIN,
    SPEC_ATTACKSINGLE,
    SPEC_HEAL,
    SPEC_ATTACKALL,
    SPEC_STUN,
    SPEC_POWERDRAIN,
    SPEC_ATTACKROW,
    SPEC_MULTIATTACK,
    SPEC_SLOW,
    SPEC_ATTACKCOLUMN,
    SPEC_CURSEPOWER,
    SPEC_TAUNT,
    SPEC_DEFENDER,
    SPEC_ELUSIVE,
    SPEC_CURSEHEALTH,
    SPEC_LEVY,
    SPEC_FLANK,
    SPEC_RETALIATE,
    SPEC_ATTACKFRIENDLY,
    SPEC_STEALTH,
    SPEC_ASSASSIN,
    SPEC_MARK,
    SPEC_ATTACKNONE,
    SPEC_PERCEPTIVE,
    SPECIALTY_COUNT
} Specialty;

static const char *const SPECIALTY_NAMES[SPECIALTY_COUNT] = {
    "POWERGAIN", "ATTACKSINGLE", "HEAL",           "ATTACKALL", "STUN",     "POWERDRAIN", "ATTACKROW",   "MULTIATTACK",
    "SLOW",      "ATTACKCOLUMN", "CURSEPOWER",     "TAUNT",     "DEFENDER", "ELUSIVE",    "CURSEHEALTH", "LEVY",
    "FLANK",     "RETALIATE",    "ATTACKFRIENDLY", "STEALTH",   "ASSASSIN", "MARK",       "ATTACKNONE",  "PERCEPTIVE"
};

typedef struct {
    char key[160];
    char text[224];
    TargetPattern target;
    unsigned mechanics;
} Ability;

typedef struct {
    int base_power;
    int base_health;
    int max_level;
    int power_per_level;
    int health_per_level;
} TierStats;

#define GEAR_SLOT_CAPACITY 5
#define FORTIFY_SLOT_COUNT 10

typedef struct {
    char id[64];
    Tier tier;
    bool ascended;
} FortifyChild;

typedef struct {
    short defs[GEAR_SLOT_CAPACITY];
    bool prestige_unlocked;
} GearLoadout;

typedef struct {
    uint64_t instance_id;
    char id[64];
    char based_on[64];
    char name[64];
    Rarity rarity;
    Rarity original_rarity;
    int stars;
    bool giant;

    Speed speed;
    Faction faction;

    char groups[MAX_GROUPS][24];
    int group_count;

    Specialty specialties[MAX_SPECIALTIES];
    int specialty_count;

    const Ability *ability;
    char equipped_rune[160];
    char target_weight[32];

    int gear_slots;
    bool has_prestige_slot;
    bool ascended;
    GearLoadout gear;

    char icon_prefab[TIER_COUNT][64];
    char model_prefab[TIER_COUNT][64];

    TierStats stats[TIER_COUNT];
    bool has_tier[TIER_COUNT];
    Tier max_tier;

    Tier tier;
    int level;
    int xp;
    int consume_xp;
    int consume_level;
    int crit_chance;
    FortifyChild fortify[FORTIFY_SLOT_COUNT];
} Character;

static const char *TIER_NAMES[TIER_COUNT] = {"I", "II", "III", "IV"};

static Rarity RarityFromString(const char *s) {
    if (!strcmp(s, "Uncommon")) return RARITY_UNCOMMON;
    if (!strcmp(s, "Rare")) return RARITY_RARE;
    if (!strcmp(s, "Epic")) return RARITY_EPIC;
    if (!strcmp(s, "Legendary")) return RARITY_LEGENDARY;
    if (!strcmp(s, "Mythic")) return RARITY_MYTHIC;
    return RARITY_COMMON;
}

static Speed SpeedFromString(const char *s) {
    if (!strcmp(s, "Slow")) return SPEED_SLOW;
    if (!strcmp(s, "Quick")) return SPEED_QUICK;
    return SPEED_NORMAL;
}

static Faction FactionFromString(const char *s) {
    if (!strcmp(s, "White")) return FACTION_WHITE;
    if (!strcmp(s, "Red")) return FACTION_RED;
    if (!strcmp(s, "Blue")) return FACTION_BLUE;
    if (!strcmp(s, "Black")) return FACTION_BLACK;
    if (!strcmp(s, "White/Red")) return FACTION_WHITE_RED;
    if (!strcmp(s, "White/Blue")) return FACTION_WHITE_BLUE;
    if (!strcmp(s, "Red/Blue")) return FACTION_RED_BLUE;
    if (!strcmp(s, "White/Black")) return FACTION_WHITE_BLACK;
    if (!strcmp(s, "Red/Black")) return FACTION_RED_BLACK;
    if (!strcmp(s, "Blue/Black")) return FACTION_BLUE_BLACK;
    if (!strcmp(s, "All")) return FACTION_ALL;
    return FACTION_LARGE;
}

bool FactionIsWhite(Faction f) {
    return f == FACTION_WHITE || f == FACTION_WHITE_RED || f == FACTION_WHITE_BLUE || f == FACTION_WHITE_BLACK ||
           f == FACTION_ALL;
}

bool FactionIsRed(Faction f) {
    return f == FACTION_RED || f == FACTION_WHITE_RED || f == FACTION_RED_BLUE || f == FACTION_RED_BLACK ||
           f == FACTION_ALL;
}

bool FactionIsBlue(Faction f) {
    return f == FACTION_BLUE || f == FACTION_WHITE_BLUE || f == FACTION_RED_BLUE || f == FACTION_BLUE_BLACK ||
           f == FACTION_ALL;
}

bool FactionIsBlack(Faction f) {
    return f == FACTION_BLACK || f == FACTION_WHITE_BLACK || f == FACTION_RED_BLACK || f == FACTION_BLUE_BLACK ||
           f == FACTION_ALL;
}

bool FactionsOverlap(Faction a, Faction b) {
    return (FactionIsWhite(a) && FactionIsWhite(b)) || (FactionIsRed(a) && FactionIsRed(b)) ||
           (FactionIsBlue(a) && FactionIsBlue(b)) || (FactionIsBlack(a) && FactionIsBlack(b));
}

static TargetPattern TargetFromString(const char *s) {
    if (!strcmp(s, "Row")) return TARGET_ROW;
    if (!strcmp(s, "Column")) return TARGET_COLUMN;
    if (!strcmp(s, "All")) return TARGET_ALL;
    return TARGET_ONE;
}

static Tier TierFromString(const char *s) {
    for (int t = 0; t < TIER_COUNT; t++) {
        if (!strcmp(s, TIER_NAMES[t])) return (Tier)t;
    }
    return TIER_I;
}

static int SpecialtyFromString(const char *s) {
    for (int i = 0; i < SPECIALTY_COUNT; i++) {
        if (!strcmp(s, SPECIALTY_NAMES[i])) return i;
    }
    return -1;
}

static unsigned AbilityMechanicFromString(const char *s) {
    static const struct {
        const char *name;
        unsigned mechanic;
    } mechanics[] = {
        {"ATTACK", ABILITY_ATTACK},
        {"ATTACK_NONE", ABILITY_ATTACK_NONE},
        {"POWER_DRAIN", ABILITY_POWER_DRAIN},
        {"STUN", ABILITY_STUN},
        {"SLOW", ABILITY_SLOW},
        {"CURSE_POWER", ABILITY_CURSE_POWER},
        {"CURSE_HEALTH", ABILITY_CURSE_HEALTH},
        {"HEAL_ALLIES", ABILITY_HEAL_ALLIES},
        {"HEAL_SELF", ABILITY_HEAL_SELF},
        {"GAIN_POWER_ALLIES", ABILITY_GAIN_POWER_ALLIES},
        {"GAIN_POWER_SELF", ABILITY_GAIN_POWER_SELF},
        {"MULTIATTACK", ABILITY_MULTIATTACK},
        {"MARK", ABILITY_MARK},
        {"LEVY", ABILITY_LEVY},
        {"ASSASSINATE", ABILITY_ASSASSINATE},
        {"ATTACK_FRIENDLY", ABILITY_ATTACK_FRIENDLY},
        {"DAMAGE_SELF", ABILITY_DAMAGE_SELF},
        {"LOSE_POWER_SELF", ABILITY_LOSE_POWER_SELF},
        {"SLOW_ALLIES", ABILITY_SLOW_ALLIES},
        {"GAIN_CRIT_ALLIES", ABILITY_GAIN_CRIT_ALLIES},
        {"CURSE_TWO_ROUNDS", ABILITY_CURSE_TWO_ROUNDS},
        {"GAIN_DAMAGE_ALLIES", ABILITY_GAIN_DAMAGE_ALLIES},
        {"GAIN_DAMAGE_SELF", ABILITY_GAIN_DAMAGE_SELF},
    };
    for (int i = 0; i < (int)(sizeof(mechanics) / sizeof(mechanics[0])); i++) {
        if (!strcmp(s, mechanics[i].name)) return mechanics[i].mechanic;
    }
    return 0;
}

static void CharacterParse(const cJSON *c, Character *ch, Ability *ab) {
    memset(ch, 0, sizeof(*ch));
    memset(ab, 0, sizeof(*ab));

    StrCopy(ch->id, sizeof(ch->id), JsonStr(c, "id"));
    StrCopy(ch->based_on, sizeof(ch->based_on), JsonStr(c, "based_on"));
    StrCopy(ch->name, sizeof(ch->name), JsonStr(c, "name"));
    ch->rarity = RarityFromString(JsonStr(c, "rarity"));
    const char *original_rarity = JsonStr(c, "original_rarity");
    ch->original_rarity = original_rarity[0] ? RarityFromString(original_rarity) : ch->rarity;
    ch->stars = JsonInt(c, "stars");
    ch->giant = JsonBool(c, "giant");
    ch->speed = SpeedFromString(JsonStr(c, "speed"));
    ch->faction = FactionFromString(JsonStr(c, "faction"));
    StrCopy(ch->equipped_rune, sizeof(ch->equipped_rune), JsonStr(c, "equipped_rune"));
    StrCopy(ch->target_weight, sizeof(ch->target_weight), JsonStr(c, "target_weight"));
    ch->gear_slots = JsonInt(c, "gear_slots");
    ch->has_prestige_slot = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(c, "has_prestige_slot"));

    cJSON *item = NULL;

    cJSON *groups = cJSON_GetObjectItemCaseSensitive((cJSON *)c, "groups");
    cJSON_ArrayForEach(item, groups) {
        if (ch->group_count >= MAX_GROUPS) break;
        StrCopy(ch->groups[ch->group_count++], 24, item->valuestring);
    }

    cJSON *specialties = cJSON_GetObjectItemCaseSensitive((cJSON *)c, "specialties");
    cJSON_ArrayForEach(item, specialties) {
        int spec = SpecialtyFromString(item->valuestring);
        if (spec >= 0 && ch->specialty_count < MAX_SPECIALTIES) {
            ch->specialties[ch->specialty_count++] = (Specialty)spec;
        }
    }

    cJSON *ability = cJSON_GetObjectItemCaseSensitive((cJSON *)c, "ability");
    StrCopy(ab->key, sizeof(ab->key), JsonStr(ability, "key"));
    StrCopy(ab->text, sizeof(ab->text), JsonStr(ability, "text"));
    ab->target = TargetFromString(JsonStr(ability, "target"));
    cJSON *mechanics = cJSON_GetObjectItemCaseSensitive(ability, "mechanics");
    cJSON_ArrayForEach(item, mechanics) {
        if (cJSON_IsString(item)) ab->mechanics |= AbilityMechanicFromString(item->valuestring);
    }
    ch->ability = ab;

    cJSON *icon = cJSON_GetObjectItemCaseSensitive((cJSON *)c, "icon");
    cJSON *models = cJSON_GetObjectItemCaseSensitive((cJSON *)c, "models");
    for (int t = 0; t < TIER_COUNT; t++) {
        StrCopy(ch->icon_prefab[t], 64, JsonStr(icon, TIER_NAMES[t]));
        StrCopy(ch->model_prefab[t], 64, JsonStr(models, TIER_NAMES[t]));
    }

    cJSON *tiers = cJSON_GetObjectItemCaseSensitive((cJSON *)c, "tiers");
    cJSON *tier = NULL;
    cJSON_ArrayForEach(tier, tiers) {
        Tier t = TierFromString(tier->string);
        ch->has_tier[t] = true;
        TierStats *s = &ch->stats[t];
        s->base_power = JsonInt(tier, "base_power");
        s->base_health = JsonInt(tier, "base_health");
        s->max_level = JsonInt(tier, "max_level");
        s->power_per_level = JsonInt(tier, "power_per_level");
        s->health_per_level = JsonInt(tier, "health_per_level");
    }
    ch->max_tier = TierFromString(JsonStr(c, "max_tier"));
}

#define CUSTOM_HERO_CATALOG_PATH "data/custom_heroes.json"

static cJSON *CharacterCatalogAddCustomHeroes(cJSON *catalog_root) {
    cJSON *characters = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    if (!cJSON_IsArray(characters) || !FileExists(CUSTOM_HERO_CATALOG_PATH)) return catalog_root;
    cJSON *custom_root = JsonLoadFile(CUSTOM_HERO_CATALOG_PATH);
    cJSON *custom = cJSON_GetObjectItemCaseSensitive(custom_root, "characters");
    for (cJSON *hero = cJSON_DetachItemFromArray(custom, 0); hero; hero = cJSON_DetachItemFromArray(custom, 0))
        cJSON_AddItemToArray(characters, hero);
    cJSON_Delete(custom_root);
    return catalog_root;
}

static const char *CharacterDataId(const Character *ch) {
    return ch->based_on[0] ? ch->based_on : ch->id;
}

int FortifyBonusPower(const Character *ch);
int FortifyBonusHealth(const Character *ch);

int CharacterLevelPower(const Character *ch) {
    const TierStats *st = &ch->stats[ch->tier];
    return st->base_power + (ch->level - 1) * st->power_per_level;
}

int CharacterLevelHealth(const Character *ch) {
    const TierStats *st = &ch->stats[ch->tier];
    return st->base_health + (ch->level - 1) * st->health_per_level;
}

int CharacterStatPower(const Character *ch) {
    return CharacterLevelPower(ch) + FortifyBonusPower(ch);
}

int CharacterStatHealth(const Character *ch) {
    return CharacterLevelHealth(ch) + FortifyBonusHealth(ch);
}

#define CHARACTER_PROJECT_KEY_PREFIX "HODA_"
#define CHARACTER_LOC_KEY_CAPACITY 224

static const char *CharacterProjectText(const char *original_key) {
    if (!original_key || !original_key[0]) return "";
    char key[CHARACTER_LOC_KEY_CAPACITY];
    snprintf(key, sizeof(key), CHARACTER_PROJECT_KEY_PREFIX "%s", original_key);
    return LocGet(key);
}

const char *CharacterLocText(const char *original_key) {
    const char *text = LocGet(original_key);
    return text[0] ? text : CharacterProjectText(original_key);
}

const char *CharacterTierName(const Character *ch, Tier tier) {
    char key[CHARACTER_LOC_KEY_CAPACITY];
    snprintf(key, sizeof(key), "%s%s", ch->id, TIER_NAMES[tier]);
    const char *name = CharacterLocText(key);
    if (!name[0]) name = LocGet(ch->id);
    return name[0] ? name : ch->name;
}

const char *CharacterName(const Character *ch) {
    return CharacterTierName(ch, ch->tier);
}

const char *CharacterAbilityText(const Ability *ability) {
    const char *text = LocGetAbility(ability->key);
    if (!text[0]) text = CharacterProjectText(ability->key);
    return text[0] ? text : ability->text;
}

static void CharacterGroupTitleKey(char *out, int capacity, const char *group) {
    snprintf(out, (size_t)capacity, "GALLERY_%s_TITLE", group);
    for (char *c = out; *c; c++) *c = (char)toupper((unsigned char)*c);
}

const char *CharacterGroupTitle(const char *group) {
    char key[CHARACTER_LOC_KEY_CAPACITY];
    CharacterGroupTitleKey(key, sizeof(key), group);
    const char *title = CharacterLocText(key);
    return title[0] ? title : group;
}
