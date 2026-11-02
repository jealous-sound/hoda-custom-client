#define HOME_PAD_SINK 0.054f
#define HOME_PAD_HALF_THICK 0.0739f
#define HOME_FAC_HEIGHT 0.115f
#define HOME_FAC_DIAMETER 0.85f
#define HOME_CLIP_FPS (1000.0f / 17.0f)
#define CLIP_ENTER_BLEND_TIME 0.1f
#define CLIP_EXIT_BLEND_TIME 0.25f
#define ATTACK_EXIT_TIME 0.6f
#define ATTACK_EXIT_BLEND 0.23f
#define HOME_UNIT_PATH_MAX 512
#define UNIT_SOLE_SAMPLES 8
#define UNIT_SOLE_SINK 0.01f
#define UNIT_POOL_CAPACITY 12
#define HOME_UI_SOUND_REF_MAX 128
#define HOME_UNIT_AUDIO_MANIFEST "data/unit_audio.json"

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
    {-204.5314f, 18.1156f, 2.00f, +2.1512f},
};

typedef struct {
    char path[HOME_UNIT_PATH_MAX];
    bool active;
    bool anim_override;
    bool melee_playing;
    bool projectile_playing;
    bool melee_exiting;
    Model model;
    ModelAnimation idle;
    ModelAnimation melee;
    ModelAnimation projectile;
    float frame_time;
    float melee_frame;
    int phase;
    int last_frame;
    int pose_bone_count;
    int *bone_order;
    Transform *pose;
    Transform *pose_from;
    bool pose_ready;
    float pose_blend_time;
    float pose_blend_left;
    ModelAnimation pose_source;
    float pose_source_frame;
    Matrix xform;
    float sole_lift;
    float fade;
    const char *wisp_fx;
    int wisp_bone;
    int wisp_loop;
    unsigned wisp_loop_serial;
} HomeSquadUnit;

#define HOME_WISP_QUEUE_CAPACITY 16
#define HOME_WISP_FX_LIFT 0.93f
#define HOME_WISP_ANCHOR_BONE "default_Chest"

typedef struct {
    HomeSquadUnit *unit;
    Vector3 pos;
} HomeWispRequest;

static HomeWispRequest home_wisp_queue[HOME_WISP_QUEUE_CAPACITY];
static int home_wisp_queue_count;

static const char *UnitWispFxFor(const char *path) {
    static const struct {
        const char *rarity, *fx;
    } tiers[] = {
        {"_common_", "fx_wispchar_t1_pre"},
        {"_uncommon_", "fx_wispchar_t2_pre"},
        {"_rare_", "fx_wispchar_t3_pre"},
        {"_epic_", "fx_wispchar_t4_pre"},
        {"_legendary_", "fx_wispchar_t5_pre"},
        {"_mythic_", "fx_wispchar_t5_pre"},
    };
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    if (strncmp(base, "c_wisp", 6) != 0) return NULL;
    for (int i = 0; i < (int)(sizeof(tiers) / sizeof(tiers[0])); i++)
        if (strstr(base, tiers[i].rarity)) return tiers[i].fx;
    return NULL;
}

static HomeSquadUnit unit_pool[UNIT_POOL_CAPACITY];
static int unit_pool_count;

typedef struct {
    Model pad_model;
    Texture2D pad_tex;
    Material pad_mat;
    Model stars[3];
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
    char ui_sound_ref[SQUAD_SLOT_COUNT][HOME_UI_SOUND_REF_MAX];
    float ui_sound_volume[SQUAD_SLOT_COUNT];
    Sound ui_sound[SQUAD_SLOT_COUNT];
} HomeSquad;

static SquadPadAssets SquadPadAssetsLoad(const char *mesh_dir, Shader pad_shader) {
    SquadPadAssets a = {0};
    if (!IsShaderValid(pad_shader)) pad_shader = (Shader){rlGetShaderIdDefault(), rlGetShaderLocsDefault()};

    a.pad_model = LoadModel(TextFormat("%s/charbase_t1_pre/char_base.obj", mesh_dir));
    a.pad_tex = LoadSprite(CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "base_T1"));
    a.pad_mat = LoadMaterialDefault();
    a.pad_mat.shader = pad_shader;
    for (int i = 0; i < 3; i++) a.stars[i] = LoadModel(TextFormat("%s/legendary_base/stars_%d.obj", mesh_dir, i + 1));
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
    Model *model = &a->stars[stars - 1];
    if (model->meshCount > 0) DrawMesh(model->meshes[0], a->star_mat, pad_xform);
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
    for (int i = 0; i < 3; i++) UnloadModel(a->stars[i]);
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

#define UNIT_PREFAB_SCALE 1.28853f
static const char *const UNIT_DOUBLED_RIG_SCALE_MODELS[] = {
    "em_bw_sentinelarcher_t1",
    "em_bw_sentinelarcher_t4",
    "em_mw_apostatesolas_t1",
    "em_mw_apostatesolas_t4",
    "em_mw_fenharel_t4",
    "em_mw_solas_t4",
    "hm_bw_venatorimarksman_t1",
    "hm_bw_venatorimarksman_t4",
    "hm_mw_macrinus_t4",
    "hm_mw_rion_t1",
    "hm_mw_rion_t4",
    "hm_tw_gurdharofsen_t1",
    "hm_tw_gurdharofsen_t4",
};

static bool UnitRigScaleDoubled(const char *base) {
    int count = sizeof(UNIT_DOUBLED_RIG_SCALE_MODELS) / sizeof(UNIT_DOUBLED_RIG_SCALE_MODELS[0]);
    for (int i = 0; i < count; i++)
        if (!strcmp(UNIT_DOUBLED_RIG_SCALE_MODELS[i], base)) return true;
    return false;
}

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
    if (UnitRigScaleDoubled(base)) return UNIT_PREFAB_SCALE;
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

static void SquadClipKey(char out[64], const char *name) {
    int length = 0;
    if (!name) name = "";
    for (; *name && length < 63; name++) {
        char c = *name;
        if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) out[length++] = c;
    }
    out[length] = '\0';
}

static int SquadMeleeClipRank(const char *name) {
    char key[64];
    SquadClipKey(key, name);
    if (!key[0]) return -1;
    if (!strcmp(key, "melee") || !strcmp(key, "meleeattack")) return 100;
    if (strstr(key, "melee") && strstr(key, "attack")) return 95;
    if (strstr(key, "attack")) return 90;
    if (strstr(key, "attk") || strstr(key, "atk")) return 80;
    if (strstr(key, "meleeability")) return 75;
    if (strstr(key, "projectileability") || strstr(key, "fireball")) return 60;
    return -1;
}

static bool SquadProjectileClip(const char *name) {
    char key[64];
    SquadClipKey(key, name);
    return strstr(key, "attack2") || strstr(key, "projectile") || strstr(key, "attackv2");
}

static void HomeSquadUnitFree(HomeSquadUnit *unit) {
    if (unit->idle.frameCount > 0) UnloadModelAnimation(unit->idle);
    if (unit->melee.frameCount > 0) UnloadModelAnimation(unit->melee);
    if (unit->projectile.frameCount > 0) UnloadModelAnimation(unit->projectile);
    for (int m = 0; m < unit->model.materialCount; m++) {
        Texture2D tex = unit->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (tex.id > 0 && tex.id != rlGetTextureIdDefault()) UnloadTexture(tex);
    }
    UnloadModel(unit->model);
    MemFree(unit->bone_order);
    MemFree(unit->pose);
    MemFree(unit->pose_from);
}

static void UnitPoolRemoveAt(int index) {
    unit_pool_count--;
    memmove(&unit_pool[index], &unit_pool[index + 1], sizeof(unit_pool[0]) * (unit_pool_count - index));
}

static void UnitPoolPut(const HomeSquadUnit *unit) {
    if (unit_pool_count == UNIT_POOL_CAPACITY) {
        HomeSquadUnitFree(&unit_pool[0]);
        UnitPoolRemoveAt(0);
    }
    unit_pool[unit_pool_count++] = *unit;
}

static bool UnitPoolTake(HomeSquadUnit *unit, const char *path) {
    for (int i = unit_pool_count - 1; i >= 0; i--) {
        if (strcmp(unit_pool[i].path, path) != 0) continue;
        *unit = unit_pool[i];
        UnitPoolRemoveAt(i);
        return true;
    }
    return false;
}

static void UnitPoolClear(void) {
    for (int i = 0; i < unit_pool_count; i++) HomeSquadUnitFree(&unit_pool[i]);
    unit_pool_count = 0;
}

static void HomeSquadUnitBindShader(HomeSquadUnit *unit, Shader unit_shader) {
    for (int m = 0; m < unit->model.materialCount; m++) unit->model.materials[m].shader = unit_shader;
}

static void HomeSquadUnitBuildBoneOrder(HomeSquadUnit *unit) {
    int count = unit->idle.boneCount;
    const BoneInfo *bones = unit->idle.bones;
    bool *placed = MemAlloc(count * sizeof(bool));
    int ordered = 0;
    while (ordered < count) {
        int before = ordered;
        for (int b = 0; b < count; b++) {
            int parent = bones[b].parent;
            if (placed[b] || (parent >= 0 && !placed[parent])) continue;
            placed[b] = true;
            unit->bone_order[ordered++] = b;
        }
        if (ordered == before) TraceLog(LOG_FATAL, "Squad: cyclic skeleton in %s", unit->path);
    }
    MemFree(placed);
}

static void HomeSquadUnitAllocPose(HomeSquadUnit *unit) {
    unit->pose_bone_count = unit->idle.boneCount;
    unit->bone_order = MemAlloc(unit->pose_bone_count * sizeof(int));
    unit->pose = MemAlloc(unit->pose_bone_count * sizeof(Transform));
    unit->pose_from = MemAlloc(unit->pose_bone_count * sizeof(Transform));
    HomeSquadUnitBuildBoneOrder(unit);
}

static Transform BoneLocalTransform(const Transform *pose, const BoneInfo *bones, int bone) {
    int parent = bones[bone].parent;
    if (parent < 0) return pose[bone];
    Quaternion to_parent = QuaternionInvert(pose[parent].rotation);
    return (Transform){
        Vector3RotateByQuaternion(Vector3Subtract(pose[bone].translation, pose[parent].translation), to_parent),
        QuaternionMultiply(to_parent, pose[bone].rotation),
        pose[bone].scale,
    };
}

static void HomeSquadUnitCloseIdleLoop(HomeSquadUnit *unit) {
    ModelAnimation idle = unit->idle;
    int last = idle.frameCount - 1;
    if (last < 1) return;
    size_t pose_size = idle.boneCount * sizeof(Transform);
    Transform *end = MemAlloc(pose_size);
    Transform *source = MemAlloc(pose_size);
    memcpy(end, idle.framePoses[last], pose_size);
    for (int frame = 1; frame <= last; frame++) {
        float weight = (float)frame / (float)last;
        Transform *pose = idle.framePoses[frame];
        memcpy(source, pose, pose_size);
        for (int i = 0; i < idle.boneCount; i++) {
            int bone = unit->bone_order[i];
            int parent = idle.bones[bone].parent;
            Transform start = BoneLocalTransform(idle.framePoses[0], idle.bones, bone);
            Transform stop = BoneLocalTransform(end, idle.bones, bone);
            Transform local = BoneLocalTransform(source, idle.bones, bone);
            Quaternion seam = QuaternionNormalize(QuaternionMultiply(start.rotation, QuaternionInvert(stop.rotation)));
            Quaternion rotation = QuaternionMultiply(QuaternionSlerp(QuaternionIdentity(), seam, weight), local.rotation);
            Vector3 drift = Vector3Scale(Vector3Subtract(start.translation, stop.translation), weight);
            Vector3 translation = Vector3Add(local.translation, drift);
            if (parent < 0) {
                pose[bone].translation = translation;
                pose[bone].rotation = rotation;
                continue;
            }
            const Transform *up = &pose[parent];
            pose[bone].translation = Vector3Add(up->translation, Vector3RotateByQuaternion(translation, up->rotation));
            pose[bone].rotation = QuaternionMultiply(up->rotation, rotation);
        }
    }
    MemFree(end);
    MemFree(source);
}

static void UnitBoneLowerName(char *lower, const char *name) {
    int n = 0;
    for (; name[n] && n < (int)sizeof(((BoneInfo *)0)->name) - 1; n++) lower[n] = (char)tolower((unsigned char)name[n]);
    lower[n] = '\0';
}

static bool UnitBoneIsFoot(const char *name) {
    char lower[sizeof(((BoneInfo *)0)->name)];
    UnitBoneLowerName(lower, name);
    return strstr(lower, "foot") || strstr(lower, "toe");
}

static bool UnitBoneIsBody(const char *name) {
    char lower[sizeof(((BoneInfo *)0)->name)];
    UnitBoneLowerName(lower, name);
    return !strstr(lower, "tail");
}

static int UnitVertexMainBone(const Mesh *mesh, int vertex) {
    int best = 0;
    for (int w = 1; w < 4; w++)
        if (mesh->boneWeights[vertex * 4 + w] > mesh->boneWeights[vertex * 4 + best]) best = w;
    return mesh->boneIds[vertex * 4 + best];
}

static float UnitLowestSkinnedY(Model model, bool (*bone_counts)(const char *)) {
    float lowest = INFINITY;
    for (int m = 0; m < model.meshCount; m++) {
        const Mesh *mesh = &model.meshes[m];
        if (!mesh->animVertices || !mesh->boneIds || !mesh->boneWeights) continue;
        for (int v = 0; v < mesh->vertexCount; v++) {
            if (!bone_counts(model.bones[UnitVertexMainBone(mesh, v)].name)) continue;
            lowest = fminf(lowest, mesh->animVertices[v * 3 + 1]);
        }
    }
    return lowest;
}

static float UnitGroundContactY(Model model) {
    float feet = UnitLowestSkinnedY(model, UnitBoneIsFoot);
    if (feet <= 0.0f) return feet;
    return fmaxf(0.0f, fminf(feet, UnitLowestSkinnedY(model, UnitBoneIsBody)));
}

static float UnitSoleLift(Model model, ModelAnimation idle) {
    float sole = INFINITY;
    for (int sample = 0; sample < UNIT_SOLE_SAMPLES; sample++) {
        UpdateModelAnimation(model, idle, sample * idle.frameCount / UNIT_SOLE_SAMPLES);
        sole = fminf(sole, UnitGroundContactY(model));
    }
    return isinf(sole) ? 0.0f : -sole - UNIT_SOLE_SINK;
}

static bool UnitClipsHoldGround(const char *path) {
    static const char *const prefixes[] = {"c_", "mnt_", "qm_"};
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    if (strncmp(base, "c_wisp", 6) == 0) return false;
    for (int i = 0; i < (int)(sizeof(prefixes) / sizeof(prefixes[0])); i++)
        if (strncmp(base, prefixes[i], strlen(prefixes[i])) == 0) return true;
    return false;
}

static float ClipLowestBodyBoneY(ModelAnimation clip, int frame) {
    float lowest = INFINITY;
    for (int bone = 0; bone < clip.boneCount; bone++)
        if (UnitBoneIsBody(clip.bones[bone].name)) lowest = fminf(lowest, clip.framePoses[frame][bone].translation.y);
    return lowest;
}

static void ClipHoldGroundOfIdle(ModelAnimation clip, ModelAnimation idle) {
    float idle_ground = 0.0f;
    for (int frame = 0; frame < idle.frameCount; frame++) idle_ground += ClipLowestBodyBoneY(idle, frame);
    idle_ground /= (float)idle.frameCount;
    for (int frame = 0; frame < clip.frameCount; frame++) {
        float correction = idle_ground - ClipLowestBodyBoneY(clip, frame);
        for (int bone = 0; bone < clip.boneCount; bone++) clip.framePoses[frame][bone].translation.y += correction;
    }
}

static void HomeSquadUnitDecode(HomeSquadUnit *unit, const char *path, int phase_seed) {
    *unit = (HomeSquadUnit){0};
    StrCopy(unit->path, sizeof(unit->path), path);
    unit->wisp_fx = UnitWispFxFor(path);
    unit->model = LoadModel(path);
    if (unit->model.meshCount <= 0) TraceLog(LOG_FATAL, "Squad: failed to load %s", path);
    for (int m = 0; m < unit->model.materialCount; m++) {
        Texture2D tex = unit->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (tex.id > 0) SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    }

    int clip_count = 0;
    ModelAnimation *clips = LoadModelAnimations(path, &clip_count);
    if (clips) {
        int idle_clip = -1, melee_clip = -1, melee_rank = -1;
        for (int a = 0; a < clip_count; a++) {
            if (clips[a].frameCount <= 0 || !IsModelAnimationValid(unit->model, clips[a])) continue;
            if (idle_clip < 0) {
                idle_clip = a;
                continue;
            }
            int rank = SquadMeleeClipRank(clips[a].name);
            if (rank > melee_rank) {
                melee_clip = a;
                melee_rank = rank;
            }
        }
        for (int a = 0; a < clip_count; a++) {
            if (a == idle_clip)
                unit->idle = clips[a];
            else if (a == melee_clip)
                unit->melee = clips[a];
            else if (
                !unit->projectile.frameCount && SquadProjectileClip(clips[a].name) &&
                IsModelAnimationValid(unit->model, clips[a])
            )
                unit->projectile = clips[a];
            else
                UnloadModelAnimation(clips[a]);
        }
        MemFree(clips);
    }
    if (unit->idle.frameCount == 0) TraceLog(LOG_FATAL, "Squad: no usable idle clip in %s", path);
    HomeSquadUnitAllocPose(unit);
    HomeSquadUnitCloseIdleLoop(unit);
    if (UnitClipsHoldGround(path)) {
        if (unit->melee.frameCount) ClipHoldGroundOfIdle(unit->melee, unit->idle);
        if (unit->projectile.frameCount) ClipHoldGroundOfIdle(unit->projectile, unit->idle);
    }
    unit->phase = (phase_seed * 47) % unit->idle.frameCount;
    unit->sole_lift = unit->wisp_fx ? 0.0f : UnitSoleLift(unit->model, unit->idle);
    unit->wisp_bone = -1;
    unit->wisp_loop = -1;
    if (unit->wisp_fx)
        for (int bone = 0; bone < unit->model.boneCount; bone++)
            if (!strcmp(unit->model.bones[bone].name, HOME_WISP_ANCHOR_BONE)) unit->wisp_bone = bone;
    unit->active = true;
}

static void HomeSquadUnitLoad(HomeSquadUnit *unit, const char *path, Shader unit_shader, Matrix xform, int phase_seed) {
    if (!UnitPoolTake(unit, path)) HomeSquadUnitDecode(unit, path, phase_seed);
    HomeSquadUnitBindShader(unit, unit_shader);
    unit->anim_override = false;
    unit->melee_playing = false;
    unit->projectile_playing = false;
    unit->melee_exiting = false;
    unit->last_frame = -1;
    unit->pose_ready = false;
    unit->pose_blend_left = 0.0f;
    unit->pose_source = (ModelAnimation){0};
    unit->xform = xform;
    unit->fade = 1.0f;
}

static void HomeSquadUnitBeginBlend(HomeSquadUnit *unit, float seconds) {
    unit->last_frame = -1;
    unit->pose_source = (ModelAnimation){0};
    if (!unit->pose_ready) return;
    memcpy(unit->pose_from, unit->pose, unit->pose_bone_count * sizeof(Transform));
    unit->pose_blend_time = seconds;
    unit->pose_blend_left = seconds;
}

static void HomeSquadUnitBlendTick(HomeSquadUnit *unit, float dt) {
    if (unit->pose_blend_left <= 0.0f) return;
    unit->pose_blend_left -= dt;
    unit->pose_source_frame += dt * HOME_CLIP_FPS;
    if (unit->pose_blend_left <= 0.0f) unit->last_frame = -1;
}

static void HomeSquadUnitBlendPose(HomeSquadUnit *unit, const BoneInfo *bones, const Transform *target, float weight) {
    for (int i = 0; i < unit->pose_bone_count; i++) {
        int bone = unit->bone_order[i];
        int parent = bones[bone].parent;
        Transform from = BoneLocalTransform(unit->pose_from, bones, bone);
        Transform to = BoneLocalTransform(target, bones, bone);
        Vector3 translation = Vector3Lerp(from.translation, to.translation, weight);
        Quaternion rotation = QuaternionSlerp(from.rotation, to.rotation, weight);
        Transform *out = &unit->pose[bone];
        out->scale = Vector3Lerp(from.scale, to.scale, weight);
        if (parent < 0) {
            out->translation = translation;
            out->rotation = rotation;
            continue;
        }
        const Transform *up = &unit->pose[parent];
        out->translation = Vector3Add(up->translation, Vector3RotateByQuaternion(translation, up->rotation));
        out->rotation = QuaternionMultiply(up->rotation, rotation);
    }
}

static void HomeSquadUnitShowFrame(HomeSquadUnit *unit, ModelAnimation clip, int frame) {
    bool blending = unit->pose_blend_left > 0.0f;
    if (frame == unit->last_frame && !blending) return;
    unit->last_frame = frame;
    if (clip.boneCount != unit->pose_bone_count) {
        UpdateModelAnimation(unit->model, clip, frame);
        return;
    }
    size_t pose_size = unit->pose_bone_count * sizeof(Transform);
    if (blending) {
        ModelAnimation source = unit->pose_source;
        if (source.frameCount > 0) {
            int source_frame = (int)unit->pose_source_frame;
            if (source_frame >= source.frameCount) source_frame = source.frameCount - 1;
            memcpy(unit->pose_from, source.framePoses[source_frame], pose_size);
        }
        float weight = 1.0f - unit->pose_blend_left / unit->pose_blend_time;
        HomeSquadUnitBlendPose(unit, clip.bones, clip.framePoses[frame], weight);
    } else {
        memcpy(unit->pose, clip.framePoses[frame], pose_size);
    }
    unit->pose_ready = true;
    Transform *shown_frames[1] = {unit->pose};
    ModelAnimation shown = clip;
    shown.frameCount = 1;
    shown.framePoses = shown_frames;
    UpdateModelAnimation(unit->model, shown, 0);
}

static bool HomeSquadUnitPlayMelee(HomeSquadUnit *unit) {
    if (!unit->active || unit->melee.frameCount <= 0) return false;
    HomeSquadUnitBeginBlend(unit, CLIP_ENTER_BLEND_TIME);
    unit->melee_playing = true;
    unit->melee_exiting = false;
    unit->projectile_playing = false;
    unit->melee_frame = 0.0f;
    HomeSquadUnitShowFrame(unit, unit->melee, 0);
    return true;
}

static bool HomeSquadUnitPlayProjectile(HomeSquadUnit *unit) {
    if (!HomeSquadUnitPlayMelee(unit)) return false;
    if (unit->projectile.frameCount <= 0) return true;
    unit->projectile_playing = true;
    unit->last_frame = -1;
    HomeSquadUnitShowFrame(unit, unit->projectile, 0);
    return true;
}

static void HomeSquadUnitBeginAttackExit(HomeSquadUnit *unit, ModelAnimation clip) {
    HomeSquadUnitBeginBlend(unit, ATTACK_EXIT_BLEND * (float)clip.frameCount / HOME_CLIP_FPS);
    unit->pose_source = clip;
    unit->pose_source_frame = unit->melee_frame;
    unit->melee_exiting = true;
}

static void HomeSquadUnitUpdate(HomeSquadUnit *unit, float dt) {
    if (!unit->active) return;
    HomeSquadUnitBlendTick(unit, dt);
    if (unit->anim_override) return;
    if (unit->melee_playing) {
        ModelAnimation clip = unit->projectile_playing ? unit->projectile : unit->melee;
        if (!unit->melee_exiting) {
            unit->melee_frame += dt * HOME_CLIP_FPS;
            if (unit->melee_frame < ATTACK_EXIT_TIME * (float)clip.frameCount) {
                HomeSquadUnitShowFrame(unit, clip, (int)unit->melee_frame);
                return;
            }
            HomeSquadUnitBeginAttackExit(unit, clip);
        } else if (unit->pose_blend_left <= 0.0f) {
            unit->melee_playing = false;
            unit->melee_exiting = false;
        }
    }
    if (unit->idle.frameCount <= 0) return;
    unit->frame_time = fmodf(unit->frame_time + dt * HOME_CLIP_FPS, (float)unit->idle.frameCount);
    HomeSquadUnitShowFrame(unit, unit->idle, ((int)unit->frame_time + unit->phase) % unit->idle.frameCount);
}

static Vector3 HomeSquadWispPosition(const HomeSquadUnit *unit) {
    Matrix m = unit->xform;
    if (unit->wisp_bone < 0 || unit->idle.frameCount <= 0) return (Vector3){m.m12, m.m13 + HOME_WISP_FX_LIFT, m.m14};
    int frame = unit->last_frame >= 0 ? unit->last_frame % unit->idle.frameCount : 0;
    return Vector3Transform(unit->idle.framePoses[frame][unit->wisp_bone].translation, m);
}

static Matrix HomeSquadUnitDrawXform(const HomeSquadUnit *unit) {
    return MatrixMultiply(MatrixTranslate(0.0f, unit->sole_lift, 0.0f), unit->xform);
}

static bool HomeSquadUnitBoneAnchor(
    const HomeSquadUnit *unit, const char *bone_name, Vector3 offset, Vector3 forward, Vector3 *position, Vector3 *aim
) {
    if (!unit->active || !unit->pose_ready || !unit->pose) return false;
    for (int bone = 0; bone < unit->pose_bone_count; bone++) {
        if (strcmp(unit->model.bones[bone].name, bone_name) != 0) continue;
        Transform pose = unit->pose[bone];
        Vector3 local = Vector3Add(pose.translation, Vector3RotateByQuaternion(offset, pose.rotation));
        Vector3 ahead = Vector3Add(local, Vector3RotateByQuaternion(forward, pose.rotation));
        Matrix xform = HomeSquadUnitDrawXform(unit);
        *position = Vector3Transform(local, xform);
        *aim = Vector3Transform(ahead, xform);
        return true;
    }
    return false;
}

static void HomeSquadUnitDraw(HomeSquadUnit *unit) {
    if (unit->active && unit->wisp_fx) {
        if (home_wisp_queue_count < HOME_WISP_QUEUE_CAPACITY) {
            home_wisp_queue[home_wisp_queue_count++] = (HomeWispRequest){unit, HomeSquadWispPosition(unit)};
        }
        return;
    }
    if (!unit->active) return;
    unsigned char a = (unsigned char)(255.0f * fminf(fmaxf(unit->fade, 0.0f), 1.0f));
    for (int m = 0; m < unit->model.materialCount; m++)
        unit->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 255, a};
    Matrix xform = HomeSquadUnitDrawXform(unit);
    for (int m = 0; m < unit->model.meshCount; m++)
        DrawMesh(unit->model.meshes[m], unit->model.materials[unit->model.meshMaterial[m]], xform);
}

static void HomeSquadUnitDestroy(HomeSquadUnit *unit) {
    if (!unit->active) return;
    UnitPoolPut(unit);
    unit->active = false;
}

static void HomeSquadUiSoundBind(HomeSquad *squad, int slot, const Character *ch, const cJSON *manifest) {
    squad->ui_sound_ref[slot][0] = '\0';
    cJSON *unit = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(manifest, "characters"), ch->id);
    cJSON *tier = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(unit, "tiers"), TIER_NAMES[ch->tier]);
    cJSON *profile_name = cJSON_GetObjectItemCaseSensitive(tier, "profile");
    if (!cJSON_IsString(profile_name)) return;
    cJSON *profile = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(manifest, "profiles"), profile_name->valuestring
    );
    cJSON *ui = cJSON_GetObjectItemCaseSensitive(profile, "ui");
    cJSON *clip = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(ui, "clips"), 0);
    if (!cJSON_IsString(clip) || !clip->valuestring[0]) return;
    cJSON *volume = cJSON_GetObjectItemCaseSensitive(ui, "volume");
    squad->ui_sound_volume[slot] = cJSON_IsNumber(volume) ? (float)volume->valuedouble : 1.0f;
    cJSON *source = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(manifest, "audio_sources"), clip->valuestring
    );
    StrCopy(
        squad->ui_sound_ref[slot],
        HOME_UI_SOUND_REF_MAX,
        cJSON_IsString(source) ? source->valuestring : TextFormat(CACHE_RESOURCE("%s"), clip->valuestring)
    );
}

static void HomeSquadUnitsLoad(
    HomeSquad *squad,
    const Player *player,
    const char *char_dir,
    const Mesh *terrain,
    int terrain_count,
    Shader unit_shader
) {
    const Character *pad_char[SQUAD_SLOT_COUNT] = {0};
    int filled = 0;
    for (int slot = 0; slot < SQUAD_SLOT_LARGE; slot++) {
        const Character *ch = PlayerSquadCharacter(player, slot);
        if (ch) pad_char[filled++] = ch;
    }
    pad_char[SQUAD_SLOT_LARGE] = PlayerSquadCharacter(player, SQUAD_SLOT_LARGE);

    cJSON *audio_manifest = JsonLoadFile(HOME_UNIT_AUDIO_MANIFEST);
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
        HomeSquadUiSoundBind(squad, s, ch, audio_manifest);
    }
    cJSON_Delete(audio_manifest);
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
    HomeSquadUnitsLoad(squad, player, char_dir, terrain, terrain_count, unit_shader);
}

static Vector3 MeshVertexAt(const float *vertices, int index) {
    return (Vector3){vertices[index * 3], vertices[index * 3 + 1], vertices[index * 3 + 2]};
}

static bool HomeSquadMeshHit(Mesh mesh, Ray local, Vector3 *nearest) {
    const float *vertices = mesh.animVertices ? mesh.animVertices : mesh.vertices;
    if (!vertices) return false;
    bool found = false;
    float nearest_distance = 0.0f;
    for (int t = 0; t < mesh.triangleCount; t++) {
        int a = mesh.indices ? mesh.indices[t * 3] : t * 3;
        int b = mesh.indices ? mesh.indices[t * 3 + 1] : t * 3 + 1;
        int c = mesh.indices ? mesh.indices[t * 3 + 2] : t * 3 + 2;
        RayCollision hit = GetRayCollisionTriangle(
            local, MeshVertexAt(vertices, a), MeshVertexAt(vertices, b), MeshVertexAt(vertices, c)
        );
        if (!hit.hit || (found && hit.distance >= nearest_distance)) continue;
        found = true;
        nearest_distance = hit.distance;
        *nearest = hit.point;
    }
    return found;
}

static bool HomeSquadUnitHit(const HomeSquadUnit *unit, Ray ray, float *distance) {
    if (!unit->active || unit->wisp_fx) return false;
    Matrix xform = HomeSquadUnitDrawXform(unit);
    Matrix inverse = MatrixInvert(xform);
    Vector3 origin = Vector3Transform(ray.position, inverse);
    Vector3 target = Vector3Transform(Vector3Add(ray.position, ray.direction), inverse);
    Ray local = {origin, Vector3Normalize(Vector3Subtract(target, origin))};
    bool found = false;
    for (int m = 0; m < unit->model.meshCount; m++) {
        Vector3 point;
        if (!HomeSquadMeshHit(unit->model.meshes[m], local, &point)) continue;
        float world_distance = Vector3Distance(ray.position, Vector3Transform(point, xform));
        if (found && world_distance >= *distance) continue;
        found = true;
        *distance = world_distance;
    }
    return found;
}

static int HomeSquadSlotAt(const HomeSquad *squad, Ray ray) {
    int nearest = -1;
    float nearest_distance = 0.0f;
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        float distance;
        if (!HomeSquadUnitHit(&squad->units[s], ray, &distance)) continue;
        if (nearest < 0 || distance < nearest_distance) {
            nearest = s;
            nearest_distance = distance;
        }
    }
    return nearest;
}

static void HomeSquadPlayUiSound(HomeSquad *squad, int slot) {
    if (!squad->ui_sound_ref[slot][0]) return;
    if (!IsSoundValid(squad->ui_sound[slot])) {
        if (!ResourceExists(squad->ui_sound_ref[slot])) {
            TraceLog(LOG_WARNING, "Squad: no UI sound %s", squad->ui_sound_ref[slot]);
            squad->ui_sound_ref[slot][0] = '\0';
            return;
        }
        squad->ui_sound[slot] = ResourceLoadSound(squad->ui_sound_ref[slot]);
    }
    if (!IsSoundValid(squad->ui_sound[slot])) return;
    SetSoundVolume(squad->ui_sound[slot], squad->ui_sound_volume[slot]);
    PlaySound(squad->ui_sound[slot]);
}

static void HomeSquadUnitPlayRandomAttack(HomeSquadUnit *unit) {
    if (unit->projectile.frameCount > 0 && GetRandomValue(0, 1)) {
        HomeSquadUnitPlayProjectile(unit);
        return;
    }
    HomeSquadUnitPlayMelee(unit);
}

static bool HomeSquadPlayAttackAt(HomeSquad *squad, Ray ray) {
    int slot = HomeSquadSlotAt(squad, ray);
    if (slot < 0) return false;
    if (squad->units[slot].melee_playing) return true;
    HomeSquadPlayUiSound(squad, slot);
    HomeSquadUnitPlayRandomAttack(&squad->units[slot]);
    return true;
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

static void HomeSquadUnitsClear(HomeSquad *squad) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        HomeSquadUnitDestroy(&squad->units[s]);
        SceneTexUnload(squad->fac_tex[s]);
        squad->fac_tex[s] = (Texture2D){0};
        SquadPadRarityUnload(&squad->pad_tex[s], &squad->stars[s]);
        if (IsSoundValid(squad->ui_sound[s])) UnloadSound(squad->ui_sound[s]);
        squad->ui_sound[s] = (Sound){0};
        squad->ui_sound_ref[s][0] = '\0';
    }
}

static void HomeSquadDestroy(HomeSquad *squad) {
    HomeSquadUnitsClear(squad);
    SceneTexUnload(squad->shadow_tex);
    SceneMaterialUnload(squad->shadow_mat);
    UnloadMesh(squad->shadow_mesh);
    SquadPadAssetsDestroy(&squad->pads);
}
