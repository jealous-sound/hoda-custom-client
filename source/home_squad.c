#define HOME_PAD_SINK 0.054f
#define HOME_PAD_HALF_THICK 0.0739f
#define HOME_FAC_HEIGHT 0.115f
#define HOME_FAC_DIAMETER 0.85f
#define HOME_CLIP_FPS (1000.0f / 17.0f)

#define SQUAD_SHADOW_SX 1.8892f
#define SQUAD_SHADOW_SZ 1.8434f
#define SQUAD_SHADOW_OX 0.0147f
#define SQUAD_SHADOW_OZ 0.0048f
#define SQUAD_SHADOW_DROP 0.0684f
#define SQUAD_SHADOW_LIFT 0.006f

typedef struct {
    float x, z, scale, yaw;
} HomePadSpot;
static const HomePadSpot HOME_PAD_SPOTS[SQUAD_SLOT_COUNT] = {
    {-201.6132f, 15.2162f, 1.00f, +3.0165f},
    {-197.0611f, 13.3045f, 1.00f, -2.4989f},
    {-199.7968f, 15.4758f, 1.00f, -2.8741f},
    {-198.1274f, 14.8212f, 1.00f, -2.6686f},
    {-204.5314f, 18.1156f, 1.55f, +2.1512f},
};

typedef struct {
    bool active;
    bool anim_override;
    Model model;
    ModelAnimation idle;
    float frame_time;
    int phase;
    int last_frame;
    Matrix xform;
    float fade;
} HomeSquadUnit;

typedef struct {
    Model pad_model;
    Texture2D pad_tex;
    Material pad_mat;
    Mesh star_row[3][2];
    float star_row_pivot[3][2];
    Texture2D star_tex;
    Material star_mat;
    Model fac_model;
    Material fac_mat;
} SquadPadAssets;

typedef struct {
    HomeSquadUnit units[SQUAD_SLOT_COUNT];
    SquadPadAssets pads;
    Matrix pad_xform[SQUAD_SLOT_COUNT];
    Texture2D pad_tex[SQUAD_SLOT_COUNT];
    unsigned char stars[SQUAD_SLOT_COUNT];
    Texture2D fac_tex[SQUAD_SLOT_COUNT];
    Matrix fac_xform[SQUAD_SLOT_COUNT];
    Mesh shadow_mesh;
    Texture2D shadow_tex;
    Material shadow_mat;
    Matrix shadow_xform[SQUAD_SLOT_COUNT];
} HomeSquad;

#define SQUAD_STAR_TILT_DEG 45.0f

static SquadPadAssets SquadPadAssetsLoad(const char *mesh_dir, Shader pad_shader) {
    SquadPadAssets a = {0};
    if (!IsShaderValid(pad_shader)) pad_shader = (Shader){rlGetShaderIdDefault(), rlGetShaderLocsDefault()};

    a.pad_model = LoadModel(TextFormat("%s/charbase_t1_pre/char_base.obj", mesh_dir));
    a.pad_tex = LoadSprite(CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "base_T1"));
    a.pad_mat = LoadMaterialDefault();
    a.pad_mat.shader = pad_shader;
    for (int i = 0; i < 3; i++) {
        Model stars = LoadModel(TextFormat("%s/legendary_base/stars_%d.obj", mesh_dir, i + 1));
        if (stars.meshCount > 0) {
            for (int row = 0; row < 2; row++)
                a.star_row[i][row] = MeshSplitHalf(&stars.meshes[0], 1, row ? -1.0f : 1.0f, &a.star_row_pivot[i][row]);
        }
        UnloadModel(stars);
    }
    a.star_tex = LoadSprite(CACHE_RESOURCE("base_gold_star"));
    a.star_mat = LoadMaterialDefault();
    a.star_mat.shader = pad_shader;
    a.star_mat.maps[MATERIAL_MAP_DIFFUSE].texture = a.star_tex;
    a.fac_model = LoadModel(TextFormat("%s/charbase_t1_pre/plane_primitive.obj", mesh_dir));
    a.fac_mat = LoadMaterialDefault();
    a.fac_mat.shader = pad_shader;
    return a;
}

static int RarityBaseTier(Rarity r) {
    return (r >= RARITY_COMMON && r <= RARITY_MYTHIC) ? (int)r : 1;
}

static int LegendaryStarRating(const Character *ch) {
    if (!ch || ch->rarity != RARITY_LEGENDARY) return 0;
    return ch->stars >= 1 && ch->stars <= 3 ? ch->stars : 0;
}

static const char *RarityBaseRef(Rarity r) {
    static const char *const refs[] = {
        [RARITY_COMMON] = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "base_T1"),
        [RARITY_UNCOMMON] = CACHE_RESOURCE("base_T2"),
        [RARITY_RARE] = CACHE_RESOURCE("base_T3"),
        [RARITY_EPIC] = CACHE_RESOURCE("base_T4"),
        [RARITY_LEGENDARY] = CACHE_RESOURCE("base_T5"),
        [RARITY_MYTHIC] = CACHE_RESOURCE("base_T6"),
    };
    return refs[RarityBaseTier(r)];
}

static void SquadPadRarityLoad(const Character *ch, Texture2D *pad_tex, unsigned char *stars) {
    *pad_tex = LoadSprite(RarityBaseRef(ch->rarity));
    *stars = (unsigned char)LegendaryStarRating(ch);
}

static void SquadPadRarityUnload(Texture2D *pad_tex, unsigned char *stars) {
    SceneTexUnload(*pad_tex);
    *pad_tex = (Texture2D){0};
    *stars = 0;
}

static void SquadPadPlaceAtFeet(Vector3 feet, float k, Matrix *pad, Matrix *fac, Matrix *shadow) {
    float pad_y = feet.y - HOME_PAD_HALF_THICK * k;
    if (pad)
        *pad = MatrixMultiply(
            MatrixMultiply(MatrixScale(k, k, k), MatrixRotateX(-PI / 2.0f)), MatrixTranslate(feet.x, pad_y, feet.z)
        );
    if (fac) {
        float d = HOME_FAC_DIAMETER * k;
        float fac_y = feet.y + (HOME_FAC_HEIGHT - HOME_PAD_SINK - HOME_PAD_HALF_THICK) * k;
        *fac = MatrixMultiply(MatrixScale(d, 1.0f, d), MatrixTranslate(feet.x, fac_y, feet.z));
    }
    if (shadow) {
        float sh_y = pad_y - SQUAD_SHADOW_DROP * k + SQUAD_SHADOW_LIFT;
        *shadow = MatrixMultiply(
            MatrixScale(SQUAD_SHADOW_SX * k, 1.0f, SQUAD_SHADOW_SZ * k),
            MatrixTranslate(feet.x + SQUAD_SHADOW_OX * k, sh_y, feet.z + SQUAD_SHADOW_OZ * k)
        );
    }
}

static void
SquadPadPlaceOnGround(Vector3 ground, float k, float yaw, float unit_scale, Matrix *pad, Matrix *fac, Matrix *unit) {
    if (pad)
        *pad = MatrixMultiply(
            MatrixMultiply(MatrixScale(k, k, k), MatrixRotateX(-PI / 2.0f)),
            MatrixTranslate(ground.x, ground.y + HOME_PAD_SINK * k, ground.z)
        );
    if (fac) {
        float d = HOME_FAC_DIAMETER * k;
        *fac = MatrixMultiply(
            MatrixScale(d, 1.0f, d), MatrixTranslate(ground.x, ground.y + HOME_FAC_HEIGHT * k, ground.z)
        );
    }
    if (unit) {
        float top_y = ground.y + (HOME_PAD_SINK + HOME_PAD_HALF_THICK) * k;
        *unit = MatrixMultiply(
            MatrixMultiply(MatrixScale(unit_scale, unit_scale, unit_scale), MatrixRotateY(yaw)),
            MatrixTranslate(ground.x, top_y, ground.z)
        );
    }
}

static Material SquadPadMaterial(SquadPadAssets *a, Texture2D tex) {
    a->pad_mat.maps[MATERIAL_MAP_DIFFUSE].texture = tex.id ? tex : a->pad_tex;
    return a->pad_mat;
}

static void SquadStarsDraw(SquadPadAssets *a, int stars, Matrix pad_xform) {
    if (stars < 1 || stars > 3) return;
    for (int row = 0; row < 2; row++) {
        Mesh mesh = a->star_row[stars - 1][row];
        if (mesh.vertexCount <= 0) continue;
        float pivot = a->star_row_pivot[stars - 1][row];
        float tilt = (row ? SQUAD_STAR_TILT_DEG : -SQUAD_STAR_TILT_DEG) * DEG2RAD;
        Matrix seat = MatrixMultiply(
            MatrixMultiply(MatrixTranslate(0.0f, -pivot, -HOME_PAD_HALF_THICK), MatrixRotateX(tilt)),
            MatrixTranslate(0.0f, pivot, HOME_PAD_HALF_THICK)
        );
        DrawMesh(mesh, a->star_mat, MatrixMultiply(seat, pad_xform));
    }
}

static void
SquadPadDraw(SquadPadAssets *a, Texture2D pad_tex, Matrix pad_xform, int stars, Texture2D fac_tex, Matrix fac_xform) {
    if (a->pad_model.meshCount <= 0) return;
    DrawMesh(a->pad_model.meshes[0], SquadPadMaterial(a, pad_tex), pad_xform);
    SquadStarsDraw(a, stars, pad_xform);
    if (fac_tex.id > 0 && a->fac_model.meshCount > 0) {
        a->fac_mat.maps[MATERIAL_MAP_DIFFUSE].texture = fac_tex;
        DrawMesh(a->fac_model.meshes[0], a->fac_mat, fac_xform);
    }
}

static void SquadPadAssetsDestroy(SquadPadAssets *a) {
    SceneMaterialUnload(a->fac_mat);
    UnloadModel(a->fac_model);
    SceneMaterialUnload(a->star_mat);
    SceneTexUnload(a->star_tex);
    for (int i = 0; i < 3; i++)
        for (int row = 0; row < 2; row++)
            if (a->star_row[i][row].vertexCount > 0) UnloadMesh(a->star_row[i][row]);
    SceneTexUnload(a->pad_tex);
    SceneMaterialUnload(a->pad_mat);
    UnloadModel(a->pad_model);
}

static const char *HomeFactionTexName(Faction f) {
    static const char *const names[] = {
        [FACTION_LARGE] = "tex.faction_large",
        [FACTION_WHITE] = "tex.faction_white",
        [FACTION_RED] = "tex.faction_red",
        [FACTION_BLUE] = "tex.faction_blue",
        [FACTION_BLACK] = "tex.faction_black",
        [FACTION_WHITE_RED] = "tex.faction_redwhite",
        [FACTION_WHITE_BLUE] = "tex.faction_bluewhite",
        [FACTION_RED_BLUE] = "tex.faction_redblue",
        [FACTION_WHITE_BLACK] = "tex.faction_blackwhite",
        [FACTION_RED_BLACK] = "tex.faction_blackred",
        [FACTION_BLUE_BLACK] = "tex.faction_blackblue",
    };
    if ((int)f < 0 || (int)f >= (int)(sizeof(names) / sizeof(names[0]))) return NULL;
    return names[f];
}

static Texture2D SquadFacTexLoad(Faction faction) {
    const char *name = HomeFactionTexName(faction);
    if (!name) return (Texture2D){0};
    return LoadSprite(TextFormat(CACHE_RESOURCE("%s"), name));
}

static void HomeSquadBaseName(char *base, int cap, const char *prefab) {
    if (strncmp(prefab, "unit.", 5) == 0) prefab += 5;
    StrCopy(base, (size_t)cap, prefab);
    size_t len = strlen(base);
    if (len > 4 && strcmp(base + len - 4, "_pre") == 0) base[len - 4] = '\0';
}

typedef struct {
    char base[64];
    float scale;
} UnitScaleEntry;
static UnitScaleEntry *unit_scales;
static int unit_scale_count = -1;

static float UnitDisplayScale(const char *prefab) {
    if (unit_scale_count < 0) {
        unit_scale_count = 0;
        if (!FileExists(HOD_CACHE "anim_tools/display_scales.json")) {
            TraceLog(LOG_WARNING, "Squad: no display_scales.json yet, all units at 1.2885");
            return 1.2885f;
        }
        cJSON *root = JsonLoadFile(HOD_CACHE "anim_tools/display_scales.json");
        int capacity = cJSON_GetArraySize(root);
        if (capacity > 0) unit_scales = MemAlloc(sizeof(*unit_scales) * capacity);
        cJSON *e = NULL;
        cJSON_ArrayForEach(e, root) {
            cJSON *s = cJSON_GetObjectItemCaseSensitive(e, "scale");
            if (!e->string || !cJSON_IsNumber(s)) continue;
            StrCopy(unit_scales[unit_scale_count].base, sizeof(unit_scales[0].base), e->string);
            unit_scales[unit_scale_count].scale = (float)s->valuedouble;
            unit_scale_count++;
        }
        cJSON_Delete(root);
        TraceLog(LOG_INFO, "Squad: %d display scales loaded", unit_scale_count);
    }
    char base[128];
    HomeSquadBaseName(base, sizeof(base), prefab);
    for (int i = 0; i < unit_scale_count; i++)
        if (!strcmp(unit_scales[i].base, base)) return unit_scales[i].scale;
    return 1.2885f;
}

static bool HomeSquadModelPath(char *out, int cap, const char *char_dir, const char *prefab) {
    char base[128];
    HomeSquadBaseName(base, sizeof(base), prefab);
    size_t len = strlen(base);

    char dir[128];
    StrCopy(dir, sizeof(dir), base);
    bool tiered = len > 3 && dir[len - 3] == '_' && dir[len - 2] == 't' && dir[len - 1] >= '1' && dir[len - 1] <= '9';
    if (tiered) dir[len - 3] = '\0';

    snprintf(out, (size_t)cap, "%s/%s/%s_rl.glb", char_dir, dir, base);
    if (FileExists(out)) return true;

    if (!tiered) {
        snprintf(out, (size_t)cap, "%s/%s/%s_t0_rl.glb", char_dir, dir, base);
        if (FileExists(out)) return true;
    }
    return false;
}

static const char *HomeSquadResolvePrefab(const char *char_dir, const Character *ch, char *path, int cap) {
    if (!ch || !ch->model_prefab[ch->tier][0]) return NULL;
    if (HomeSquadModelPath(path, cap, char_dir, ch->model_prefab[ch->tier])) return ch->model_prefab[ch->tier];
    for (int distance = 1; distance < TIER_COUNT; distance++) {
        int candidates[2] = {(int)ch->tier + distance, (int)ch->tier - distance};
        for (int c = 0; c < 2; c++) {
            int tier = candidates[c];
            if (tier < 0 || tier >= TIER_COUNT || !ch->model_prefab[tier][0]) continue;
            if (!HomeSquadModelPath(path, cap, char_dir, ch->model_prefab[tier])) continue;
            TraceLog(
                LOG_WARNING,
                "Squad: %s has no tier-%s bake, using the nearest tier-%s skin",
                ch->id,
                TIER_NAMES[ch->tier],
                TIER_NAMES[tier]
            );
            return ch->model_prefab[tier];
        }
    }
    return NULL;
}

static void HomeSquadUnitLoad(HomeSquadUnit *unit, const char *path, Shader unit_shader, Matrix xform, int phase_seed) {
    *unit = (HomeSquadUnit){0};
    unit->model = LoadModel(path);
    if (unit->model.meshCount <= 0) TraceLog(LOG_FATAL, "Squad: failed to load %s", path);
    for (int m = 0; m < unit->model.materialCount; m++) {
        unit->model.materials[m].shader = unit_shader;
        Texture2D tex = unit->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (tex.id > 0) SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    }

    int clip_count = 0;
    ModelAnimation *clips = LoadModelAnimations(path, &clip_count);
    if (clips) {
        for (int a = 0; a < clip_count; a++) {
            if (unit->idle.frameCount == 0 && clips[a].frameCount > 0 && IsModelAnimationValid(unit->model, clips[a])) {
                unit->idle = clips[a];
            } else {
                UnloadModelAnimation(clips[a]);
            }
        }
        MemFree(clips);
    }
    if (unit->idle.frameCount == 0) TraceLog(LOG_FATAL, "Squad: no usable idle clip in %s", path);
    unit->phase = (phase_seed * 47) % unit->idle.frameCount;
    unit->last_frame = -1;
    unit->frame_time = 0.0f;
    unit->xform = xform;
    unit->fade = 1.0f;
    unit->active = true;
}

static void HomeSquadUnitUpdate(HomeSquadUnit *unit, float dt) {
    if (!unit->active || unit->anim_override || unit->idle.frameCount <= 0) return;
    unit->frame_time = fmodf(unit->frame_time + dt * HOME_CLIP_FPS, (float)unit->idle.frameCount);
    int frame = ((int)unit->frame_time + unit->phase) % unit->idle.frameCount;
    if (frame == unit->last_frame) return;
    unit->last_frame = frame;
    UpdateModelAnimation(unit->model, unit->idle, frame);
}

static void HomeSquadUnitDraw(HomeSquadUnit *unit) {
    if (!unit->active) return;
    unsigned char a = (unsigned char)(255.0f * fminf(fmaxf(unit->fade, 0.0f), 1.0f));
    for (int m = 0; m < unit->model.materialCount; m++)
        unit->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 255, a};
    for (int m = 0; m < unit->model.meshCount; m++)
        DrawMesh(unit->model.meshes[m], unit->model.materials[unit->model.meshMaterial[m]], unit->xform);
}

static void HomeSquadUnitDestroy(HomeSquadUnit *unit) {
    if (!unit->active) return;
    if (unit->idle.frameCount > 0) UnloadModelAnimation(unit->idle);
    for (int m = 0; m < unit->model.materialCount; m++) {
        Texture2D tex = unit->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (tex.id > 0 && tex.id != rlGetTextureIdDefault()) UnloadTexture(tex);
    }
    UnloadModel(unit->model);
    unit->active = false;
}

static void HomeSquadLoad(
    HomeSquad *squad,
    const Player *player,
    const char *mesh_dir,
    const char *char_dir,
    const Mesh *terrain,
    int terrain_count,
    Shader pad_shader,
    Shader unit_shader,
    Shader mult_shader
) {
    memset(squad, 0, sizeof(*squad));
    squad->pads = SquadPadAssetsLoad(mesh_dir, pad_shader);
    squad->shadow_mesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
    squad->shadow_tex = LoadSprite(CACHE_RESOURCE("base shadow2"));
    squad->shadow_mat = LoadMaterialDefault();
    squad->shadow_mat.shader = mult_shader;
    squad->shadow_mat.maps[MATERIAL_MAP_DIFFUSE].texture = squad->shadow_tex;

    const Character *pad_char[SQUAD_SLOT_COUNT] = {0};
    int filled = 0;
    for (int slot = 0; slot < SQUAD_SLOT_LARGE; slot++) {
        const Character *ch = PlayerSquadCharacter(player, slot);
        if (ch) pad_char[filled++] = ch;
    }
    pad_char[SQUAD_SLOT_LARGE] = PlayerSquadCharacter(player, SQUAD_SLOT_LARGE);

    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        const Character *ch = pad_char[s];
        if (!ch) continue;
        const HomePadSpot *spot = &HOME_PAD_SPOTS[s];
        float ground = TerrainHeightAt(spot->x, spot->z, terrain, terrain_count, 0.0f);
        Vector3 stand = {spot->x, ground, spot->z};

        char path[512];
        const char *prefab = HomeSquadResolvePrefab(char_dir, ch, path, sizeof(path));
        if (!prefab) {
            TraceLog(LOG_WARNING, "Squad: %s has no baked model; slot hidden", ch->id);
            continue;
        }

        float char_scale = UnitDisplayScale(prefab) * (ch->giant ? 2.0f : 1.0f);
        Matrix xform;
        SquadPadPlaceOnGround(
            stand, spot->scale, spot->yaw, char_scale, &squad->pad_xform[s], &squad->fac_xform[s], &xform
        );
        HomeSquadUnitLoad(&squad->units[s], path, unit_shader, xform, s);
        TraceLog(LOG_INFO, "Squad: slot %d %s (%s)", s, ch->name, path);

        squad->shadow_xform[s] = MatrixMultiply(
            MatrixScale(SQUAD_SHADOW_SX * spot->scale, 1.0f, SQUAD_SHADOW_SZ * spot->scale),
            MatrixTranslate(
                spot->x + SQUAD_SHADOW_OX * spot->scale,
                ground + SQUAD_SHADOW_LIFT,
                spot->z + SQUAD_SHADOW_OZ * spot->scale
            )
        );

        SquadPadRarityLoad(ch, &squad->pad_tex[s], &squad->stars[s]);
        squad->fac_tex[s] = SquadFacTexLoad(ch->faction);
    }
}

static void HomeSquadUpdate(HomeSquad *squad, float dt) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitUpdate(&squad->units[s], dt);
}

static void HomeSquadDrawPads(HomeSquad *squad) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        if (!squad->units[s].active) continue;
        SquadPadDraw(
            &squad->pads,
            squad->pad_tex[s],
            squad->pad_xform[s],
            squad->stars[s],
            squad->fac_tex[s],
            squad->fac_xform[s]
        );
    }
}

static void HomeSquadDrawUnits(HomeSquad *squad) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitDraw(&squad->units[s]);
}

static void HomeSquadDrawShadows(HomeSquad *squad) {
    rlDrawRenderBatchActive();
    rlSetBlendFactors(RL_ZERO, RL_SRC_COLOR, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    rlDisableDepthMask();
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++)
        if (squad->units[s].active) DrawMesh(squad->shadow_mesh, squad->shadow_mat, squad->shadow_xform[s]);
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}

static void HomeSquadDestroy(HomeSquad *squad) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitDestroy(&squad->units[s]);
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) SceneTexUnload(squad->fac_tex[s]);
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) SquadPadRarityUnload(&squad->pad_tex[s], &squad->stars[s]);
    SceneTexUnload(squad->shadow_tex);
    SceneMaterialUnload(squad->shadow_mat);
    UnloadMesh(squad->shadow_mesh);
    SquadPadAssetsDestroy(&squad->pads);
}
