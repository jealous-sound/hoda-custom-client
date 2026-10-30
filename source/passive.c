
typedef enum {
    PASSIVE_NONE,
    PASSIVE_STAT_PERCENT,
    PASSIVE_STAT_PERCENT_PER_MATCHING_ALLY,
    PASSIVE_INCOMING_TARGET_WEIGHT,
    PASSIVE_OUTGOING_TARGET_PREFERENCE,
    PASSIVE_HEAL_PERCENT_MAX_HEALTH,
    PASSIVE_BONUS_DAMAGE_PERCENT
} PassiveKind;

typedef enum {
    PASSIVE_TRIGGER_NONE,
    PASSIVE_TRIGGER_BATTLE_START,
    PASSIVE_TRIGGER_TARGET_SELECTION,
    PASSIVE_TRIGGER_ENEMY_DEATH,
    PASSIVE_TRIGGER_BEFORE_DAMAGE
} PassiveTrigger;

typedef enum { PASSIVE_SCOPE_SELF, PASSIVE_SCOPE_ALLIES, PASSIVE_SCOPE_ENEMIES } PassiveScope;
typedef enum { PASSIVE_MATCH_ANY, PASSIVE_MATCH_EACH } PassiveMatchMode;
typedef enum {
    PASSIVE_TERM_GROUP,
    PASSIVE_TERM_FACTION,
    PASSIVE_TERM_CHARACTER_ID,
    PASSIVE_TERM_SPECIALTY,
    PASSIVE_TERM_RARITY,
    PASSIVE_TERM_SPEED,
    PASSIVE_TERM_TAG,
    PASSIVE_TERM_UNKNOWN
} PassiveTermKind;
typedef enum { PASSIVE_SELECTOR_NONE, PASSIVE_SELECTOR_LOWEST_HEALTH_PERCENT } PassiveSelector;

enum { PASSIVE_STAT_HEALTH = 1 << 0, PASSIVE_STAT_POWER = 1 << 1 };

typedef struct {
    PassiveTermKind kind;
    char value[64];
} PassiveMatchTerm;

typedef struct {
    PassiveScope scope;
    bool include_source;
    bool has_match;
    PassiveMatchMode mode;
    PassiveMatchTerm *terms;
    int term_count;
} PassiveFilter;

typedef struct {
    char id[64];
    PassiveKind kind;
    PassiveTrigger trigger;
    unsigned stats;
    float value_pct;
    float value_multiplier;
    PassiveSelector selector;
    PassiveFilter recipient;
    PassiveFilter count;
    bool has_count;
    PassiveFilter target;
    bool has_target;
    bool additive;
} PassiveEffect;

typedef struct {
    char id[64];
    PassiveEffect *effects;
    int effect_count;
} PassiveCharacter;

typedef struct {
    PassiveCharacter *characters;
    int character_count;
    bool loaded;
} PassiveManifest;

static PassiveManifest passive_manifest;

static PassiveKind PassiveKindFromString(const char *s) {
    if (!strcmp(s, "stat_percent")) return PASSIVE_STAT_PERCENT;
    if (!strcmp(s, "stat_percent_per_matching_ally")) return PASSIVE_STAT_PERCENT_PER_MATCHING_ALLY;
    if (!strcmp(s, "incoming_target_weight")) return PASSIVE_INCOMING_TARGET_WEIGHT;
    if (!strcmp(s, "outgoing_target_preference")) return PASSIVE_OUTGOING_TARGET_PREFERENCE;
    if (!strcmp(s, "heal_percent_max_health")) return PASSIVE_HEAL_PERCENT_MAX_HEALTH;
    if (!strcmp(s, "bonus_damage_percent")) return PASSIVE_BONUS_DAMAGE_PERCENT;
    return PASSIVE_NONE;
}

static PassiveTrigger PassiveTriggerFromString(const char *s) {
    if (!strcmp(s, "battle_start")) return PASSIVE_TRIGGER_BATTLE_START;
    if (!strcmp(s, "target_selection")) return PASSIVE_TRIGGER_TARGET_SELECTION;
    if (!strcmp(s, "enemy_death")) return PASSIVE_TRIGGER_ENEMY_DEATH;
    if (!strcmp(s, "before_damage")) return PASSIVE_TRIGGER_BEFORE_DAMAGE;
    return PASSIVE_TRIGGER_NONE;
}

static PassiveTermKind PassiveTermKindFromString(const char *s) {
    if (!strcmp(s, "group")) return PASSIVE_TERM_GROUP;
    if (!strcmp(s, "faction")) return PASSIVE_TERM_FACTION;
    if (!strcmp(s, "character_id")) return PASSIVE_TERM_CHARACTER_ID;
    if (!strcmp(s, "specialty")) return PASSIVE_TERM_SPECIALTY;
    if (!strcmp(s, "rarity")) return PASSIVE_TERM_RARITY;
    if (!strcmp(s, "speed")) return PASSIVE_TERM_SPEED;
    if (!strcmp(s, "tag")) return PASSIVE_TERM_TAG;
    return PASSIVE_TERM_UNKNOWN;
}

static PassiveFilter PassiveFilterParse(const cJSON *json) {
    PassiveFilter filter = {0};
    const char *scope = JsonStr(json, "scope");
    filter.scope = !strcmp(scope, "enemies")  ? PASSIVE_SCOPE_ENEMIES
                   : !strcmp(scope, "allies") ? PASSIVE_SCOPE_ALLIES
                                              : PASSIVE_SCOPE_SELF;
    cJSON *include = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "include_source");
    filter.include_source = cJSON_IsBool(include) ? cJSON_IsTrue(include) : true;

    cJSON *match = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "match");
    if (!cJSON_IsObject(match)) return filter;
    filter.has_match = true;
    filter.mode = !strcmp(JsonStr(match, "mode"), "each") ? PASSIVE_MATCH_EACH : PASSIVE_MATCH_ANY;
    cJSON *terms = cJSON_GetObjectItemCaseSensitive(match, "terms");
    filter.term_count = cJSON_GetArraySize(terms);
    if (filter.term_count > 0) {
        filter.terms = calloc((size_t)filter.term_count, sizeof(*filter.terms));
        if (!filter.terms) {
            TraceLog(LOG_FATAL, "Passives: out of memory parsing match terms");
        }
    }
    cJSON *term = NULL;
    int term_index = 0;
    cJSON_ArrayForEach(term, terms) {
        PassiveMatchTerm *dst = &filter.terms[term_index++];
        dst->kind = PassiveTermKindFromString(JsonStr(term, "kind"));
        StrCopy(dst->value, sizeof(dst->value), JsonStr(term, "value"));
    }
    return filter;
}

static PassiveEffect PassiveEffectParse(const cJSON *json) {
    PassiveEffect effect = {0};
    StrCopy(effect.id, sizeof(effect.id), JsonStr(json, "id"));
    effect.kind = PassiveKindFromString(JsonStr(json, "kind"));
    effect.trigger = PassiveTriggerFromString(JsonStr(json, "trigger"));
    effect.value_pct = JsonFloat(json, "value_pct");
    effect.value_multiplier = JsonFloat(json, "value_multiplier");
    effect.additive = !strcmp(JsonStr(json, "stacking"), "additive");
    if (!strcmp(JsonStr(json, "selector"), "lowest_health_percent"))
        effect.selector = PASSIVE_SELECTOR_LOWEST_HEALTH_PERCENT;

    cJSON *stat = NULL;
    cJSON_ArrayForEach(stat, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "stats")) {
        const char *name = cJSON_GetStringValue(stat);
        if (name && !strcmp(name, "health")) effect.stats |= PASSIVE_STAT_HEALTH;
        if (name && !strcmp(name, "power")) effect.stats |= PASSIVE_STAT_POWER;
    }

    cJSON *recipient = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "recipient");
    if (cJSON_IsObject(recipient)) effect.recipient = PassiveFilterParse(recipient);
    cJSON *count = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "count");
    if (cJSON_IsObject(count)) {
        effect.count = PassiveFilterParse(count);
        effect.has_count = true;
    }
    cJSON *target = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "target");
    if (cJSON_IsObject(target)) {
        effect.target = PassiveFilterParse(target);
        effect.has_target = true;
    }
    return effect;
}

static bool PassiveFilterValid(const PassiveFilter *filter) {
    if (!filter->has_match) return true;
    if (filter->term_count <= 0 || !filter->terms) return false;
    for (int i = 0; i < filter->term_count; i++)
        if (filter->terms[i].kind == PASSIVE_TERM_UNKNOWN || filter->terms[i].value[0] == '\0') return false;
    return true;
}

static bool PassiveEffectValid(const PassiveEffect *effect) {
    if (effect->id[0] == '\0' || effect->kind == PASSIVE_NONE || effect->trigger == PASSIVE_TRIGGER_NONE) return false;
    if (!PassiveFilterValid(&effect->recipient) || (effect->has_count && !PassiveFilterValid(&effect->count)) ||
        (effect->has_target && !PassiveFilterValid(&effect->target)))
        return false;
    if (effect->kind == PASSIVE_STAT_PERCENT)
        return effect->trigger == PASSIVE_TRIGGER_BATTLE_START && effect->stats != 0 && effect->additive;
    if (effect->kind == PASSIVE_STAT_PERCENT_PER_MATCHING_ALLY)
        return effect->trigger == PASSIVE_TRIGGER_BATTLE_START && effect->stats != 0 && effect->value_pct != 0.0f &&
               effect->recipient.scope == PASSIVE_SCOPE_SELF && effect->has_count &&
               effect->count.scope == PASSIVE_SCOPE_ALLIES && effect->count.has_match && effect->additive;
    if (effect->kind == PASSIVE_INCOMING_TARGET_WEIGHT || effect->kind == PASSIVE_OUTGOING_TARGET_PREFERENCE)
        return effect->trigger == PASSIVE_TRIGGER_TARGET_SELECTION && effect->value_multiplier > 0.0f;
    if (effect->kind == PASSIVE_HEAL_PERCENT_MAX_HEALTH)
        return effect->trigger == PASSIVE_TRIGGER_ENEMY_DEATH && effect->value_pct != 0.0f;
    return effect->kind == PASSIVE_BONUS_DAMAGE_PERCENT && effect->trigger == PASSIVE_TRIGGER_BEFORE_DAMAGE &&
           effect->value_pct != 0.0f;
}

static void PassiveManifestUnload(void) {
    for (int i = 0; i < passive_manifest.character_count; i++) {
        PassiveCharacter *character = &passive_manifest.characters[i];
        for (int e = 0; e < character->effect_count; e++) {
            free(character->effects[e].recipient.terms);
            free(character->effects[e].count.terms);
            free(character->effects[e].target.terms);
        }
        free(character->effects);
    }
    free(passive_manifest.characters);
    memset(&passive_manifest, 0, sizeof(passive_manifest));
}

static bool PassiveManifestLoad(const char *path) {
    PassiveManifestUnload();
    cJSON *root = JsonLoadFile(path);
    if (!root) return false;
    if (JsonInt(root, "version") != 1) {
        TraceLog(LOG_WARNING, "Passives: unsupported manifest version in %s", path);
        cJSON_Delete(root);
        return false;
    }

    cJSON *characters = cJSON_GetObjectItemCaseSensitive(root, "characters");
    if (!cJSON_IsArray(characters)) {
        cJSON_Delete(root);
        return false;
    }
    int count = cJSON_GetArraySize(characters);
    passive_manifest.characters = calloc((size_t)count, sizeof(*passive_manifest.characters));
    if (!passive_manifest.characters) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *row = NULL;
    cJSON_ArrayForEach(row, characters) {
        PassiveCharacter *character = &passive_manifest.characters[passive_manifest.character_count++];
        StrCopy(character->id, sizeof(character->id), JsonStr(row, "id"));
        if (character->id[0] == '\0' ||
            (passive_manifest.character_count > 1 &&
             strcmp(passive_manifest.characters[passive_manifest.character_count - 2].id, character->id) >= 0)) {
            TraceLog(LOG_WARNING, "Passives: character ids must be nonempty, unique, and sorted");
            cJSON_Delete(root);
            PassiveManifestUnload();
            return false;
        }
        cJSON *effects = cJSON_GetObjectItemCaseSensitive(row, "effects");
        character->effect_count = cJSON_GetArraySize(effects);
        if (character->effect_count == 0) continue;
        character->effects = calloc((size_t)character->effect_count, sizeof(*character->effects));
        if (!character->effects) {
            cJSON_Delete(root);
            PassiveManifestUnload();
            return false;
        }
        for (int i = 0; i < character->effect_count; i++) {
            character->effects[i] = PassiveEffectParse(cJSON_GetArrayItem(effects, i));
            if (!PassiveEffectValid(&character->effects[i])) {
                TraceLog(LOG_WARNING, "Passives: invalid effect %s on %s", character->effects[i].id, character->id);
                cJSON_Delete(root);
                PassiveManifestUnload();
                return false;
            }
        }
    }
    cJSON_Delete(root);
    passive_manifest.loaded = true;
    TraceLog(LOG_INFO, "Passives: loaded %d character records", passive_manifest.character_count);
    return true;
}

static void PassiveManifestEnsure(void) {
    if (!passive_manifest.loaded && !PassiveManifestLoad("data/intrinsic_passives.json")) {
        TraceLog(LOG_FATAL, "Passives: required manifest could not be loaded");
    }
}

static const PassiveCharacter *PassiveManifestFind(const char *id) {
    int lo = 0, hi = passive_manifest.character_count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(id, passive_manifest.characters[mid].id);
        if (cmp == 0) return &passive_manifest.characters[mid];
        if (cmp < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return NULL;
}
