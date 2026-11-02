
#define BATTLE_CHANGER_MAX_EFFECTS 4
#define BATTLE_CHANGER_MAX_GROUPS 2
#define BATTLE_CHANGER_MAX_UNITS 2
#define BATTLE_CHANGER_TEXT_SIZE 256

typedef struct {
    char groups[BATTLE_CHANGER_MAX_GROUPS][24];
    int group_count;
    char units[BATTLE_CHANGER_MAX_UNITS][48];
    int unit_count;
    unsigned faction_mask;
    int health_pct, power_pct;
} BattleChangerEffect;

typedef struct {
    char text_key[128];
    char text_ru[BATTLE_CHANGER_TEXT_SIZE];
    BattleChangerEffect effects[BATTLE_CHANGER_MAX_EFFECTS];
    int effect_count;
} BattleGameChanger;

typedef struct {
    Character chars[SQUAD_SLOT_COUNT];
    Ability abilities[SQUAD_SLOT_COUNT];
    bool present[SQUAD_SLOT_COUNT];
    const BattleGameChanger *game_changer;
    int rune_defs[RUNE_SLOT_COUNT];
    int rune_def_count;
} BattleRoster;

static void
BattleRosterLoad(BattleRoster *r, const char *rosters_path, const char *catalog_path, const char *map_id, int step) {
    memset(r, 0, sizeof(*r));

    cJSON *root = JsonLoadFile(rosters_path);
    cJSON *catalog_root = JsonLoadFile(catalog_path);
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    if (!cJSON_IsArray(catalog)) TraceLog(LOG_FATAL, "Battle: no character catalog in %s", catalog_path);

    cJSON *node = NULL;
    cJSON *campaign = NULL;
    cJSON_ArrayForEach(campaign, cJSON_GetObjectItemCaseSensitive(root, "campaigns")) {
        if (strcmp(JsonStr(campaign, "campaign"), map_id) != 0) continue;
        cJSON *n = NULL;
        cJSON_ArrayForEach(n, cJSON_GetObjectItemCaseSensitive(campaign, "nodes")) {
            if (JsonInt(n, "step") == step) node = n;
        }
    }
    if (!node) TraceLog(LOG_FATAL, "Battle: no roster for %s step %d in %s", map_id, step, rosters_path);

    int slot = 0;
    cJSON *cell = NULL;
    cJSON_ArrayForEach(cell, cJSON_GetObjectItemCaseSensitive(node, "roster")) {
        if (slot >= SQUAD_SLOT_COUNT) TraceLog(LOG_FATAL, "Battle: %s step %d roster has too many cells", map_id, step);
        int s = slot++;
        if (!cJSON_IsObject(cell)) continue;

        const char *id = JsonStr(cell, "id");
        cJSON *def = CatalogFindCharacter(catalog, id);
        if (!def) TraceLog(LOG_FATAL, "Battle: enemy '%s' not found in %s", id, catalog_path);
        CharacterParse(def, &r->chars[s], &r->abilities[s]);

        Character *ch = &r->chars[s];
        ch->tier = TierFromString(JsonStr(cell, "tier"));
        if (!ch->has_tier[ch->tier])
            TraceLog(LOG_FATAL, "Battle: %s has no tier %s in the catalog", id, TIER_NAMES[ch->tier]);
        cJSON *level = cJSON_GetObjectItemCaseSensitive(cell, "level");
        ch->level = cJSON_IsNumber(level) ? level->valueint : 1;

        if (ch->giant != (s == SQUAD_SLOT_LARGE))
            TraceLog(
                LOG_WARNING,
                "Battle: %s step %d slot %d: %s is %s",
                map_id,
                step,
                s,
                id,
                ch->giant ? "a giant outside the giant slot" : "not a giant in the giant slot"
            );
        r->present[s] = true;
    }

    cJSON_Delete(catalog_root);
    cJSON_Delete(root);
}

typedef struct {
    float x, z;
} BattleSpot;
#define BATTLE_MARKER_Y 0.9706f
#define BATTLE_PLAYER_YAW (DEG2RAD * -93.12f)
#define BATTLE_ENEMY_YAW (DEG2RAD * 86.87f)
static const BattleSpot BATTLE_PLAYER_SPOTS[SQUAD_SLOT_COUNT] = {
    {4.1946f, 15.2986f},
    {1.9865f, 15.1952f},
    {3.7227f, 13.2035f},
    {1.5157f, 13.1160f},
    {2.7584f, 17.9087f},
};
static const BattleSpot BATTLE_ENEMY_SPOTS[SQUAD_SLOT_COUNT] = {
    {-4.5202f, 12.8568f},
    {-2.3028f, 12.9389f},
    {-5.1595f, 14.8794f},
    {-2.9654f, 14.9985f},
    {-4.0209f, 17.5386f},
};

typedef struct {
    HomeSquadUnit units[2][SQUAD_SLOT_COUNT];
    bool present[2][SQUAD_SLOT_COUNT];
    ModelAnimation *clips[2][SQUAD_SLOT_COUNT];
    int clip_counts[2][SQUAD_SLOT_COUNT];
    SquadPadAssets pads;
    Mesh shadow_mesh;
    Material shadow_mat;
    Texture2D shadow_tex;
    Texture2D pad_tex[2][SQUAD_SLOT_COUNT];
    unsigned char stars[2][SQUAD_SLOT_COUNT];
    Texture2D fac_tex[2][SQUAD_SLOT_COUNT];
    Matrix pad_xform[2][SQUAD_SLOT_COUNT];
    Matrix fac_xform[2][SQUAD_SLOT_COUNT];
    Matrix shadow_xform[2][SQUAD_SLOT_COUNT];
    float unit_scale[2][SQUAD_SLOT_COUNT];
    Shader unit_sh, mult_sh;
} BattleSquads;

static void BattleSquadsLoad(
    BattleSquads *sq, const Player *player, const BattleRoster *enemy, const char *mesh_dir, const char *char_dir
) {
    memset(sq, 0, sizeof(*sq));

    sq->unit_sh = LoadShaderFromMemory(VS_UNLIT, FS_PLAIN);
    float bright = 1.0f;
    SetShaderValue(sq->unit_sh, GetShaderLocation(sq->unit_sh, "brightness"), &bright, SHADER_UNIFORM_FLOAT);
    sq->mult_sh = LoadShaderFromMemory(VS_UNLIT, FS_MULTIPLY);

    sq->pads = SquadPadAssetsLoad(mesh_dir, sq->unit_sh);
    sq->shadow_mesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
    sq->shadow_tex = LoadSprite(CACHE_RESOURCE("base shadow2"));

    sq->shadow_mat = LoadMaterialDefault();
    sq->shadow_mat.shader = sq->mult_sh;
    sq->shadow_mat.maps[MATERIAL_MAP_DIFFUSE].texture = sq->shadow_tex;

    for (int side = 0; side < 2; side++) {
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
            const Character *ch =
                side == 0 ? PlayerSquadCharacter(player, s) : (enemy->present[s] ? &enemy->chars[s] : NULL);
            if (!ch) continue;
            if (!ch->model_prefab[ch->tier][0]) {
                TraceLog(LOG_WARNING, "Battle: %s has no tier-%s model prefab", ch->id, TIER_NAMES[ch->tier]);
                continue;
            }
            const BattleSpot *spot = side == 0 ? &BATTLE_PLAYER_SPOTS[s] : &BATTLE_ENEMY_SPOTS[s];
            float k = ch->giant ? 2.0f : 1.0f;

            char path[512];
            const char *prefab = HomeSquadResolvePrefab(char_dir, ch, path, sizeof(path));
            if (prefab) {
                float ds = UnitDisplayScale(prefab) * k;
                sq->unit_scale[side][s] = ds;
                Matrix xform = MatrixMultiply(
                    MatrixMultiply(
                        MatrixScale(ds, ds, ds), MatrixRotateY(side == 0 ? BATTLE_PLAYER_YAW : BATTLE_ENEMY_YAW)
                    ),
                    MatrixTranslate(spot->x, BATTLE_MARKER_Y, spot->z)
                );
                HomeSquadUnitLoad(&sq->units[side][s], path, sq->unit_sh, xform, side * SQUAD_SLOT_COUNT + s);
                sq->clips[side][s] = LoadModelAnimations(path, &sq->clip_counts[side][s]);
            } else {
                TraceLog(LOG_WARNING, "Battle: %s missing (bake %s), pedestal only", path, ch->id);
            }
            sq->present[side][s] = true;

            SquadPadRarityLoad(ch, &sq->pad_tex[side][s], &sq->stars[side][s]);
            sq->fac_tex[side][s] = SquadFacTexLoad(ch->faction);
            SquadPadPlaceAtFeet(
                (Vector3){spot->x, BATTLE_MARKER_Y, spot->z},
                k,
                &sq->pad_xform[side][s],
                &sq->fac_xform[side][s],
                &sq->shadow_xform[side][s]
            );

            TraceLog(
                LOG_INFO,
                "Battle: %s slot %d %s (t%d base)",
                side == 0 ? "player" : "enemy",
                s,
                ch->id,
                RarityBaseTier(ch->rarity)
            );
        }
    }
}

static void BattleSquadsUpdate(BattleSquads *sq, float dt) {
    for (int side = 0; side < 2; side++)
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitUpdate(&sq->units[side][s], dt);
}

static void BattleSquadsDraw(BattleSquads *sq) {
    for (int side = 0; side < 2; side++) {
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
            if (!sq->present[side][s]) continue;
            SquadPadDraw(
                &sq->pads,
                sq->pad_tex[side][s],
                sq->pad_xform[side][s],
                sq->stars[side][s],
                sq->fac_tex[side][s],
                sq->fac_xform[side][s]
            );
        }
    }

    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    for (int side = 0; side < 2; side++)
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitDraw(&sq->units[side][s]);
    rlDisableBackfaceCulling();

    rlDrawRenderBatchActive();
    rlSetBlendFactors(RL_ZERO, RL_SRC_COLOR, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    rlDisableDepthMask();
    for (int side = 0; side < 2; side++)
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++)
            if (sq->present[side][s]) DrawMesh(sq->shadow_mesh, sq->shadow_mat, sq->shadow_xform[side][s]);
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}

static void BattleClipKey(char out[64], const char *name) {
    int n = 0;
    for (; *name && n < 63; name++) {
        char c = *name;
        if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) out[n++] = c;
    }
    out[n] = '\0';
}

static bool BattleClipKeyEnds(const char *key, const char *suffix) {
    size_t key_len = strlen(key), suffix_len = strlen(suffix);
    return key_len >= suffix_len && strcmp(key + key_len - suffix_len, suffix) == 0;
}

static int BattleClipAliasRank(const char *actual, const char *role) {
    char key[64], wanted[64];
    BattleClipKey(key, actual);
    BattleClipKey(wanted, role);
    if (!key[0] || !wanted[0]) return -1;
    if (strcmp(key, wanted) == 0) return 100;

    if (strcmp(wanted, "hit") == 0) {
        if (strstr(key, "gethit") || strstr(key, "gettinghit")) return 90;
        if (strstr(key, "hit")) return 80;
        if (strstr(key, "damaged") || strstr(key, "flinch") || strstr(key, "hurt")) return 70;
    } else if (strcmp(wanted, "death") == 0) {
        if (strstr(key, "death") || strstr(key, "dying")) return 90;
        if (BattleClipKeyEnds(key, "die")) return 80;
    } else if (strcmp(wanted, "attack") == 0) {
        if (strstr(key, "attack")) return 90;
        if (strstr(key, "attk") || strstr(key, "atk")) return 80;
        if (strstr(key, "projectileability") || strstr(key, "meleeability") || strstr(key, "fireball")) return 70;
    } else if (strcmp(wanted, "idle") == 0) {
        if (strstr(key, "idle") || strstr(key, "ready") || strstr(key, "rdy")) return 80;
    } else if (strcmp(wanted, "cheer") == 0) {
        if (strstr(key, "cheer") || strstr(key, "roar") || strstr(key, "victory")) return 80;
    }
    return -1;
}

static const ModelAnimation *BattleSquadClip(const BattleSquads *sq, int side, int slot, const char *name) {
    const ModelAnimation *best = NULL;
    int best_rank = -1;
    for (int i = 0; i < sq->clip_counts[side][slot]; i++) {
        const ModelAnimation *a = &sq->clips[side][slot][i];
        int rank = a->frameCount > 0 ? BattleClipAliasRank(a->name, name) : -1;
        if (rank > best_rank) {
            best = a;
            best_rank = rank;
        }
    }
    return best;
}

static void BattleSquadsDestroy(BattleSquads *sq) {
    for (int side = 0; side < 2; side++) {
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
            HomeSquadUnitDestroy(&sq->units[side][s]);
            if (sq->clips[side][s]) {
                for (int i = 0; i < sq->clip_counts[side][s]; i++) UnloadModelAnimation(sq->clips[side][s][i]);
                MemFree(sq->clips[side][s]);
            }
            SquadPadRarityUnload(&sq->pad_tex[side][s], &sq->stars[side][s]);
            SceneTexUnload(sq->fac_tex[side][s]);
        }
    }
    SceneTexUnload(sq->shadow_tex);
    SceneMaterialUnload(sq->shadow_mat);
    UnloadMesh(sq->shadow_mesh);
    SquadPadAssetsDestroy(&sq->pads);
    UnloadShader(sq->unit_sh);
    UnloadShader(sq->mult_sh);
}
