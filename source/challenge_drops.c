#define CHALLENGE_DROPS_PATH "data/challenge_drops.json"
#define CHALLENGE_DROPS_CHARACTERS_PATH HOD_CACHE "characters.json"
#define CHALLENGE_DROPS_VERSION 1
#define CHALLENGE_DROPS_PER_MAP 16
#define CHALLENGE_DROPS_PREVIEW_CAPACITY (RARITY_MYTHIC + RUNE_PRESTIGE_RARITY)
#define CHALLENGE_DROP_ID_CAPACITY 48
#define CHALLENGE_DROP_ICON_CAPACITY 96
#define CHALLENGE_DROP_KIND_UNIT "unit"
#define CHALLENGE_DROP_KIND_RUNE "rune"
#define CHALLENGE_DROP_PREVIEW_FADE_SECONDS 0.25f
#define CHALLENGE_DROP_PREVIEW_FADE_OUT_AT 1.75f
#define CHALLENGE_DROP_PREVIEW_SECONDS 2.25f

typedef enum {
    CHALLENGE_DROP_UNIT,
    CHALLENGE_DROP_RUNE,
} ChallengeDropKind;

typedef struct {
    ChallengeDropKind kind;
    char id[CHALLENGE_DROP_ID_CAPACITY];
    char icon[CHALLENGE_DROP_ICON_CAPACITY];
    int weight;
    int rarity;
    Texture2D texture;
} ChallengeDrop;

typedef struct {
    ChallengeDropKind kind;
    int rarity;
} ChallengeDropPreview;

typedef struct {
    char map_id[32];
    int drop_count;
    ChallengeDrop drops[CHALLENGE_DROPS_PER_MAP];
    int preview_count;
    ChallengeDropPreview previews[CHALLENGE_DROPS_PREVIEW_CAPACITY];
} ChallengeDropTable;

typedef struct {
    bool attempted;
    bool loaded;
    int roll_total;
    int table_count;
    ChallengeDropTable tables[CAMPAIGN_MAX_MAPS];
} ChallengeDropCatalog;

typedef struct {
    bool pending;
    const ChallengeDrop *drop;
} ChallengeDropAward;

static ChallengeDropCatalog challenge_drops;

static const cJSON *ChallengeDropsFindCharacter(const cJSON *characters, const char *id) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)characters) {
        if (!strcmp(JsonStr(entry, "id"), id)) return entry;
    }
    return NULL;
}

static const char *ChallengeDropsGrantedIcon(const cJSON *character) {
    const cJSON *tiers = cJSON_GetObjectItemCaseSensitive(character, "tiers");
    const cJSON *icons = cJSON_GetObjectItemCaseSensitive(character, "icon");
    for (int tier = 0; tier < TIER_COUNT; tier++)
        if (cJSON_GetObjectItemCaseSensitive(tiers, TIER_NAMES[tier])) return JsonStr(icons, TIER_NAMES[tier]);
    return "";
}

static bool ChallengeDropsResolveUnit(ChallengeDrop *drop, const cJSON *characters) {
    const cJSON *character = ChallengeDropsFindCharacter(characters, drop->id);
    if (!character || !JsonBool(character, "obtainable")) return false;
    drop->rarity = RarityFromString(JsonStr(character, "rarity"));
    StrCopy(drop->icon, sizeof(drop->icon), ChallengeDropsGrantedIcon(character));
    return drop->rarity >= RARITY_COMMON && drop->rarity <= RARITY_LEGENDARY && drop->icon[0];
}

static bool ChallengeDropsResolveRune(ChallengeDrop *drop) {
    const RuneDef *def = RuneDefAt(RuneDefIndex(drop->id));
    if (!def) return false;
    drop->rarity = def->rarity;
    StrCopy(drop->icon, sizeof(drop->icon), def->icon);
    return true;
}

static bool ChallengeDropsParseDrop(const cJSON *json, ChallengeDrop *drop, const cJSON *characters) {
    const char *kind = JsonStr(json, "kind");
    const cJSON *weight = cJSON_GetObjectItemCaseSensitive(json, "weight");
    if (!cJSON_IsNumber(weight) || weight->valueint <= 0 || weight->valuedouble != (double)weight->valueint)
        return false;
    *drop = (ChallengeDrop){.weight = weight->valueint};
    StrCopy(drop->id, sizeof(drop->id), JsonStr(json, "id"));
    if (!strcmp(kind, CHALLENGE_DROP_KIND_UNIT)) {
        drop->kind = CHALLENGE_DROP_UNIT;
        return ChallengeDropsResolveUnit(drop, characters);
    }
    if (!strcmp(kind, CHALLENGE_DROP_KIND_RUNE)) {
        drop->kind = CHALLENGE_DROP_RUNE;
        return ChallengeDropsResolveRune(drop);
    }
    return false;
}

static void ChallengeDropsAddPreview(ChallengeDropTable *table, ChallengeDropKind kind, int rarity) {
    for (int i = 0; i < table->preview_count; i++)
        if (table->previews[i].kind == kind && table->previews[i].rarity == rarity) return;
    if (table->preview_count < CHALLENGE_DROPS_PREVIEW_CAPACITY)
        table->previews[table->preview_count++] = (ChallengeDropPreview){kind, rarity};
}

static void ChallengeDropsBuildPreviews(ChallengeDropTable *table) {
    table->preview_count = 0;
    for (int kind = CHALLENGE_DROP_UNIT; kind <= CHALLENGE_DROP_RUNE; kind++)
        for (int rarity = RARITY_COMMON; rarity <= RUNE_PRESTIGE_RARITY; rarity++)
            for (int i = 0; i < table->drop_count; i++)
                if ((int)table->drops[i].kind == kind && table->drops[i].rarity == rarity)
                    ChallengeDropsAddPreview(table, (ChallengeDropKind)kind, rarity);
}

static bool ChallengeDropsParseTable(const cJSON *json, ChallengeDropTable *table, const cJSON *characters) {
    *table = (ChallengeDropTable){0};
    StrCopy(table->map_id, sizeof(table->map_id), JsonStr(json, "map"));
    long long total = 0;
    const cJSON *drop = NULL;
    cJSON_ArrayForEach(drop, cJSON_GetObjectItemCaseSensitive(json, "drops")) {
        if (table->drop_count == CHALLENGE_DROPS_PER_MAP) return false;
        ChallengeDrop *parsed = &table->drops[table->drop_count++];
        if (!ChallengeDropsParseDrop(drop, parsed, characters)) {
            TraceLog(LOG_WARNING, "ChallengeDrops: %s has an unknown drop %s", table->map_id, parsed->id);
            return false;
        }
        total += parsed->weight;
    }
    ChallengeDropsBuildPreviews(table);
    return table->map_id[0] && table->drop_count > 0 && total < challenge_drops.roll_total;
}

bool ChallengeDropsLoad(const char *path, const char *characters_path) {
    ChallengeDropCatalog *c = &challenge_drops;
    *c = (ChallengeDropCatalog){.attempted = true};
    char *text = FileExists(path) ? LoadFileText(path) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    if (text) UnloadFileText(text);
    RuneCatalogEnsure();
    cJSON *catalog = root ? CharacterCatalogAddCustomHeroes(JsonLoadFile(characters_path)) : NULL;
    const cJSON *characters = cJSON_GetObjectItemCaseSensitive(catalog, "characters");
    c->roll_total = JsonInt(root, "roll_total");
    bool ok = root && JsonInt(root, "version") == CHALLENGE_DROPS_VERSION && c->roll_total > 0;
    const cJSON *maps = ok ? cJSON_GetObjectItemCaseSensitive(root, "maps") : NULL;
    const cJSON *map = NULL;
    cJSON_ArrayForEach(map, maps) {
        if (c->table_count == CAMPAIGN_MAX_MAPS ||
            !ChallengeDropsParseTable(map, &c->tables[c->table_count++], characters)) {
            ok = false;
            break;
        }
    }
    cJSON_Delete(catalog);
    cJSON_Delete(root);
    if (!ok) {
        TraceLog(LOG_WARNING, "ChallengeDrops: %s is missing or invalid, challenge drops are off", path);
        *c = (ChallengeDropCatalog){.attempted = true};
        return false;
    }
    c->loaded = true;
    TraceLog(LOG_INFO, "ChallengeDrops: loaded %d maps from %s", c->table_count, path);
    return true;
}

static void ChallengeDropsEnsure(void) {
    if (!challenge_drops.attempted) ChallengeDropsLoad(CHALLENGE_DROPS_PATH, CHALLENGE_DROPS_CHARACTERS_PATH);
}

const ChallengeDropTable *ChallengeDropsForMap(const char *map_id) {
    ChallengeDropsEnsure();
    for (int i = 0; map_id && i < challenge_drops.table_count; i++)
        if (!strcmp(challenge_drops.tables[i].map_id, map_id)) return &challenge_drops.tables[i];
    return NULL;
}

const ChallengeDrop *ChallengeDropRoll(const ChallengeDropTable *table, uint32_t *rng) {
    if (!table || table->drop_count <= 0 || challenge_drops.roll_total <= 0) return NULL;
    int roll = StoreRngRange(rng, challenge_drops.roll_total);
    for (int i = 0; i < table->drop_count; i++) {
        if (roll < table->drops[i].weight) return &table->drops[i];
        roll -= table->drops[i].weight;
    }
    return NULL;
}

const ChallengeDrop *ChallengeDropFind(const ChallengeDropTable *table, ChallengeDropKind kind, const char *id) {
    for (int i = 0; table && id && i < table->drop_count; i++)
        if (table->drops[i].kind == kind && !strcmp(table->drops[i].id, id)) return &table->drops[i];
    return NULL;
}

static const char *ChallengeDropsReplyFirstString(const cJSON *list, const char *field) {
    const cJSON *first = cJSON_GetArrayItem(list, 0);
    if (cJSON_IsString(first)) return first->valuestring;
    return field ? JsonStr(first, field) : NULL;
}

ChallengeDropAward ChallengeDropAwardFromReply(const ChallengeDropTable *table, const cJSON *reply) {
    const cJSON *result = cJSON_GetObjectItemCaseSensitive(reply, "granted");
    const cJSON *granted = cJSON_GetObjectItemCaseSensitive(result, "granted");
    const char *hero = ChallengeDropsReplyFirstString(cJSON_GetObjectItemCaseSensitive(granted, "heroes"), "id");
    const char *rune = ChallengeDropsReplyFirstString(cJSON_GetObjectItemCaseSensitive(granted, "runes"), NULL);
    const ChallengeDrop *drop = ChallengeDropFind(table, CHALLENGE_DROP_UNIT, hero);
    if (!drop) drop = ChallengeDropFind(table, CHALLENGE_DROP_RUNE, rune);
    return (ChallengeDropAward){.drop = drop};
}

Texture2D ChallengeDropTexture(const ChallengeDrop *drop) {
    if (!drop) return (Texture2D){0};
    ChallengeDrop *cached = (ChallengeDrop *)drop;
    if (cached->texture.id) return cached->texture;
    cached->texture = drop->kind == CHALLENGE_DROP_UNIT ? GuiPortraitLoad(drop->icon) : RuneIconLoad(drop->icon);
    return cached->texture;
}

static Texture2D ChallengeDropPreviewTexture(ChallengeDropPreview preview) {
    return StoreRarityBackdrop(preview.kind == CHALLENGE_DROP_RUNE ? STORE_KIND_RUNE : STORE_KIND_HERO, preview.rarity);
}

float ChallengeDropPreviewAlpha(float phase, int count) {
    if (phase < CHALLENGE_DROP_PREVIEW_FADE_SECONDS) return phase / CHALLENGE_DROP_PREVIEW_FADE_SECONDS;
    if (count <= 1 || phase < CHALLENGE_DROP_PREVIEW_FADE_OUT_AT) return 1.0f;
    return 1.0f - fminf((phase - CHALLENGE_DROP_PREVIEW_FADE_OUT_AT) / CHALLENGE_DROP_PREVIEW_FADE_SECONDS, 1.0f);
}

int ChallengeDropPreviewIndex(float elapsed, int count) {
    if (count <= 1 || elapsed < 0.0f) return 0;
    return (int)(elapsed / CHALLENGE_DROP_PREVIEW_SECONDS) % count;
}

void ChallengeDropsDrawPreview(const ChallengeDropTable *table, Rectangle rect, float elapsed) {
    if (!table || table->preview_count <= 0) return;
    if (elapsed < 0.0f) elapsed = 0.0f;
    int count = table->preview_count;
    float phase = count > 1 ? fmodf(elapsed, CHALLENGE_DROP_PREVIEW_SECONDS) : elapsed;
    float alpha = Clamp(ChallengeDropPreviewAlpha(phase, count), 0.0f, 1.0f);
    ChallengeDropPreview preview = table->previews[ChallengeDropPreviewIndex(elapsed, count)];
    UiDrawTexture(ChallengeDropPreviewTexture(preview), rect, Fade(WHITE, alpha));
}
