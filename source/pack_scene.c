
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <stdbool.h>
#include <string.h>

#ifndef HOD_CACHE
#define HOD_CACHE "heroes-of-dragons-cache/"
#endif

#define PACK_SCENE_EXPECTED_MESH_COUNT 38
#define PACK_SCENE_PARTS_MATERIAL 7
#define PACK_SCENE_MATERIAL_MAP_COUNT (MATERIAL_MAP_BRDF + 1)

typedef enum {
    PACK_MAT_UNUSED,
    PACK_MAT_PROP,
    PACK_MAT_PARTS,
    PACK_MAT_GROUND,
    PACK_MAT_FOLIAGE,
    PACK_MAT_PLAIN,
} PackMaterialKind;

static const PackMaterialKind PACK_SCENE_MATERIAL_KIND[] = {
    [0] = PACK_MAT_UNUSED,
    [1] = PACK_MAT_PROP,
    [2] = PACK_MAT_UNUSED,
    [3] = PACK_MAT_PROP,
    [4] = PACK_MAT_PROP,
    [5] = PACK_MAT_FOLIAGE,
    [6] = PACK_MAT_FOLIAGE,
    [7] = PACK_MAT_PARTS,
    [8] = PACK_MAT_PLAIN,
    [9] = PACK_MAT_PROP,
    [10] = PACK_MAT_GROUND,
};
#define PACK_SCENE_MATERIAL_KIND_COUNT ((int)(sizeof(PACK_SCENE_MATERIAL_KIND) / sizeof(PACK_SCENE_MATERIAL_KIND[0])))

#define PACK_SCENE_FERN_CUTOFF 0.679f
#define PACK_SCENE_GRASS_CUTOFF 0.276f
#define PACK_SCENE_FERN_TINT ((Vector3){1.0f, 1.0f, 1.0f})
#define PACK_SCENE_GRASS_TINT ((Vector3){0.8811f, 0.9412f, 0.7543f})

typedef struct {
    Rarity rarity;
    Faction faction;
    int stars;
    float base_scale;
    bool tall;
} PackReveal;

typedef struct PackScene {
    Model model;
    Texture2D atlas;
    Texture2D *embedded_textures;
    int embedded_texture_count;
    int embedded_texture_capacity;
    Shader prop_sh, parts_sh, ground_sh, foliage_sh, plain_sh;
    int foliage_cutoff_loc, foliage_vcscale_loc, foliage_vcshift_loc, foliage_tint_loc;
    Texture2D ground_detail, ground_mask;

    SquadPadAssets pads;
    Texture2D base_tex[RARITY_MYTHIC + 1];
    Texture2D base_default;
    Texture2D fac_texture;
    Faction fac_loaded;

    bool shader_loaded;
    bool model_loaded;
    bool atlas_loaded;
    bool pads_loaded;
} PackScene;

// clang-format off
static const int PACK_SCENE_KEEP_MESHES[] = {
    1, 5, 6, 7, 19, 23, 26, 27, 30, 31, 32, 33, 34, 36, 37,
    9, 13,
};
// clang-format on

static const Camera3D PACK_SCENE_CAMERA_START = {
    .position = {220.0f, 2.2886f, -173.8590f},
    .target = {220.0f, 2.2886f, -172.8590f},
    .up = {0.0f, 1.0f, 0.0f},
    .fovy = 45.0f,
    .projection = CAMERA_PERSPECTIVE,
};

static const Camera3D PACK_SCENE_CAMERA_END = {
    .position = {222.7021f, 1.3849f, -175.4241f},
    .target = {222.38191f, 1.316483f, -174.479216f},
    .up = {-0.005423f, 0.997505f, 0.070390f},
    .fovy = 45.0f,
    .projection = CAMERA_PERSPECTIVE,
};

static const Camera3D PACK_SCENE_CAMERA_END_TALL = {
    .position = {222.8783f, 1.0819f, -175.9783f},
    .target = {222.560108f, 1.102098f, -175.030489f},
    .up = {0.022972f, 0.999644f, -0.013591f},
    .fovy = 45.0f,
    .projection = CAMERA_PERSPECTIVE,
};

#define PACK_SCENE_NEAR 0.3f
#define PACK_SCENE_FAR 150.0f

#define PACK_UNIT_X 220.0f
#define PACK_UNIT_Z -171.1642f

#define PACK_BASE_APPEAR 5.181f
#define PACK_BASE_LAND_START 7.0157f
#define PACK_BASE_LAND_END 7.9334f
#define PACK_BASE_REVEAL 8.0503f
#define PACK_BASE_VISIBLE_SCALE 0.7f
#define PACK_BASE_LANDED_SCALE 0.8415f
#define PACK_BASE_GROW_TIME 0.10f
#define PACK_BASE_UNIT_SCALE (PACK_BASE_LANDED_SCALE * PACK_BASE_MESH_SCALE)
#define PACK_UNIT_FEET (PACK_BASE_LANDED_Y + HOME_PAD_HALF_THICK * PACK_BASE_UNIT_SCALE)
#define PACK_BASE_MESH_SCALE 1.2714f
#define PACK_BASE_LANDED_Y (-0.3962f)
#define PACK_BASE_UPRIGHT_DEG -168.770f
#define PACK_BASE_LOOK_Y (-0.2077f)
#define PACK_BASE_LOOK_Z (-176.5607f)
#define PACK_BASE_FLIP_TIME 0.07f

float PackSceneBaseScale(void) {
    return PACK_BASE_UNIT_SCALE;
}

static bool PackSceneTextureIdIsTracked(const PackScene *scene, unsigned int id) {
    for (int i = 0; i < scene->embedded_texture_count; i++) {
        if (scene->embedded_textures[i].id == id) return true;
    }
    return false;
}

static void PackSceneTrackEmbeddedTextures(PackScene *scene) {
    if (!scene->model.materials || scene->model.materialCount <= 0) return;

    int capacity = scene->model.materialCount * PACK_SCENE_MATERIAL_MAP_COUNT;
    if (capacity <= 0) return;
    scene->embedded_textures = MemAlloc((unsigned int)(capacity * (int)sizeof(Texture2D)));
    if (!scene->embedded_textures) {
        TraceLog(LOG_WARNING, "PackScene: cannot allocate embedded-texture ownership table");
        return;
    }
    scene->embedded_texture_capacity = capacity;

    unsigned int default_id = rlGetTextureIdDefault();
    for (int material_index = 0; material_index < scene->model.materialCount; material_index++) {
        Material *material = &scene->model.materials[material_index];
        if (!material->maps) continue;
        for (int map_index = 0; map_index < PACK_SCENE_MATERIAL_MAP_COUNT; map_index++) {
            Texture2D texture = material->maps[map_index].texture;
            if (texture.id == 0 || texture.id == default_id) continue;
            if (PackSceneTextureIdIsTracked(scene, texture.id)) continue;
            if (scene->embedded_texture_count >= scene->embedded_texture_capacity) return;
            scene->embedded_textures[scene->embedded_texture_count++] = texture;
        }
    }
}

static PackMaterialKind PackSceneMaterialKind(int material_index) {
    if (material_index < 0 || material_index >= PACK_SCENE_MATERIAL_KIND_COUNT) return PACK_MAT_PROP;
    return PACK_SCENE_MATERIAL_KIND[material_index];
}

static void PackSceneApplyMaterials(PackScene *scene) {
    scene->prop_sh = LoadShaderFromMemory(VS_UNLIT, FS_HOME_DIFF);
    scene->parts_sh = LoadShaderFromMemory(VS_UNLIT, FS_HOME_DIFF);
    scene->ground_sh = LoadShaderFromMemory(VS_UNLIT, FS_HOME_MIX);
    scene->foliage_sh = LoadShaderFromMemory(VS_UNLIT, FS_HOME_WIND);
    scene->plain_sh = LoadShaderFromMemory(VS_UNLIT, FS_PLAIN);
    scene->shader_loaded = IsShaderValid(scene->prop_sh) && IsShaderValid(scene->parts_sh) &&
                           IsShaderValid(scene->ground_sh) && IsShaderValid(scene->foliage_sh) &&
                           IsShaderValid(scene->plain_sh);
    if (!scene->shader_loaded) {
        TraceLog(LOG_WARNING, "PackScene: diorama shaders unavailable; falling back to raylib's default");
        return;
    }

    float gain = 0.178f, one = 1.0f;
    SetShaderValue(scene->prop_sh, GetShaderLocation(scene->prop_sh, "uBright"), &gain, SHADER_UNIFORM_FLOAT);
    SetShaderValue(scene->ground_sh, GetShaderLocation(scene->ground_sh, "uBright"), &gain, SHADER_UNIFORM_FLOAT);
    float parts_gain = gain * 0.32f;
    SetShaderValue(scene->parts_sh, GetShaderLocation(scene->parts_sh, "uBright"), &parts_gain, SHADER_UNIFORM_FLOAT);
    SetShaderValue(scene->plain_sh, GetShaderLocation(scene->plain_sh, "brightness"), &one, SHADER_UNIFORM_FLOAT);
    scene->foliage_tint_loc = GetShaderLocation(scene->foliage_sh, "uTint");
    scene->foliage_cutoff_loc = GetShaderLocation(scene->foliage_sh, "uCutoff");
    scene->foliage_vcscale_loc = GetShaderLocation(scene->foliage_sh, "uVcScale");
    scene->foliage_vcshift_loc = GetShaderLocation(scene->foliage_sh, "uVcShift");

    scene->ground_detail = LoadTiled(HOD_CACHE "meshes/PackOpening_pre/background_grass_01.png");
    scene->ground_mask = LoadTiled(HOD_CACHE "meshes/_shared_textures/background_platform_01_mask.png");

    for (int i = 0; i < scene->model.materialCount; i++) {
        Material *material = &scene->model.materials[i];
        switch (PackSceneMaterialKind(i)) {
            case PACK_MAT_GROUND:
                material->shader = scene->ground_sh;
                if (material->maps) {
                    material->maps[MATERIAL_MAP_SPECULAR].texture = scene->ground_detail;
                    material->maps[MATERIAL_MAP_NORMAL].texture = scene->ground_mask;
                }
                break;
            case PACK_MAT_PARTS:
                material->shader = scene->parts_sh;
                break;
            case PACK_MAT_FOLIAGE:
                material->shader = scene->foliage_sh;
                break;
            case PACK_MAT_PLAIN:
                material->shader = scene->plain_sh;
                break;
            default:
                material->shader = scene->prop_sh;
                break;
        }
    }
}

static bool PackSceneMeshIndexIsKept(int index) {
    int count = (int)(sizeof(PACK_SCENE_KEEP_MESHES) / sizeof(PACK_SCENE_KEEP_MESHES[0]));
    for (int i = 0; i < count; i++) {
        if (PACK_SCENE_KEEP_MESHES[i] == index) return true;
    }
    return false;
}

static void PackSceneReleaseModel(PackScene *scene) {
    if (!scene) return;

    if (scene->model.materials) {
        for (int material_index = 0; material_index < scene->model.materialCount; material_index++) {
            Material *material = &scene->model.materials[material_index];
            if (!material->maps) continue;
            for (int map_index = 0; map_index < PACK_SCENE_MATERIAL_MAP_COUNT; map_index++) {
                material->maps[map_index].texture = (Texture2D){0};
            }
        }
    }

    for (int i = 0; i < scene->embedded_texture_count; i++) {
        Texture2D texture = scene->embedded_textures[i];
        if (IsTextureValid(texture)) UnloadTexture(texture);
    }
    if (scene->atlas_loaded && IsTextureValid(scene->atlas) && !PackSceneTextureIdIsTracked(scene, scene->atlas.id)) {
        UnloadTexture(scene->atlas);
    }
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC; rarity++) {
        SceneTexUnload(scene->base_tex[rarity]);
    }
    SceneTexUnload(scene->base_default);
    if (scene->pads_loaded) {
        SceneTexUnload(scene->fac_texture);
        SquadPadAssetsDestroy(&scene->pads);
    }
    if (scene->embedded_textures) MemFree(scene->embedded_textures);
    if (scene->shader_loaded) {
        for (int i = 0; i < scene->model.materialCount; i++) scene->model.materials[i].shader = (Shader){0};
        UnloadShader(scene->prop_sh);
        UnloadShader(scene->parts_sh);
        UnloadShader(scene->ground_sh);
        UnloadShader(scene->foliage_sh);
        UnloadShader(scene->plain_sh);
        SceneTexUnload(scene->ground_detail);
        SceneTexUnload(scene->ground_mask);
    }
    if (scene->model.meshes || scene->model.materials || scene->model.meshMaterial) {
        UnloadModel(scene->model);
    }
    memset(scene, 0, sizeof(*scene));
}

bool PackSceneInitialize(PackScene *scene) {
    if (!scene) return false;
    memset(scene, 0, sizeof(*scene));

    const char *model_path = HOD_CACHE "meshes/PackOpening_pre/PackOpening_pre_vertexcolor.glb";
    scene->model = LoadModel(model_path);
    if (!IsModelValid(scene->model) || scene->model.meshCount <= 0) {
        TraceLog(LOG_WARNING, "PackScene: failed to load %s", model_path);
        PackSceneReleaseModel(scene);
        return false;
    }
    scene->model_loaded = true;
    PackSceneTrackEmbeddedTextures(scene);

    PackSceneApplyMaterials(scene);

    scene->pads = SquadPadAssetsLoad(HOD_CACHE "meshes", scene->plain_sh);
    scene->pads_loaded = true;
    scene->fac_loaded = FACTION_ALL;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC; rarity++)
        scene->base_tex[rarity] = LoadSprite(TextFormat(HOD_CACHE "meshes/_shared_textures/base_T%d.png", rarity));
    scene->base_default = LoadSprite(HOD_CACHE "meshes/PackOpening_pre/base_T1.png");
    if (scene->pads.pad_model.meshCount <= 0)
        TraceLog(LOG_WARNING, "PackScene: the charbase pedestal is unavailable; the reveal lands on nothing");

    if (scene->model.meshCount != PACK_SCENE_EXPECTED_MESH_COUNT) {
        TraceLog(
            LOG_WARNING,
            "PackScene: expected %d meshes in audited GLB, got %d; drawing only valid audited indices",
            PACK_SCENE_EXPECTED_MESH_COUNT,
            scene->model.meshCount
        );
    }

    if (scene->model.materials && scene->model.materialCount > PACK_SCENE_PARTS_MATERIAL) {
        Material *parts = &scene->model.materials[PACK_SCENE_PARTS_MATERIAL];
        Texture2D diffuse = {0};
        if (parts->maps) diffuse = parts->maps[MATERIAL_MAP_DIFFUSE].texture;
        bool missing = diffuse.id == 0 || diffuse.id == rlGetTextureIdDefault();
        if (missing && parts->maps) {
            const char *atlas_path = HOD_CACHE "meshes/PackOpening_pre/PackOpenPartsAtlas.png";
            scene->atlas = LoadTexture(atlas_path);
            scene->atlas_loaded = IsTextureValid(scene->atlas);
            if (scene->atlas_loaded) {
                parts->maps[MATERIAL_MAP_DIFFUSE].texture = scene->atlas;
                parts->maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            } else {
                TraceLog(LOG_WARNING, "PackScene: failed to load %s", atlas_path);
            }
        }
    } else {
        TraceLog(
            LOG_WARNING,
            "PackScene: parts material %d unavailable (materialCount=%d)",
            PACK_SCENE_PARTS_MATERIAL,
            scene->model.materialCount
        );
    }

    int usable_meshes = 0;
    int keep_count = (int)(sizeof(PACK_SCENE_KEEP_MESHES) / sizeof(PACK_SCENE_KEEP_MESHES[0]));
    for (int i = 0; i < keep_count; i++) {
        int mesh_index = PACK_SCENE_KEEP_MESHES[i];
        if (mesh_index < scene->model.meshCount && scene->model.meshes[mesh_index].vertexCount > 0) usable_meshes++;
    }
    if (usable_meshes == 0) {
        TraceLog(LOG_WARNING, "PackScene: audited meshes are unavailable; using the fallback background");
        PackSceneReleaseModel(scene);
        return false;
    }

    if (scene->model.meshCount == PACK_SCENE_EXPECTED_MESH_COUNT) {
        for (int mesh_index = 0; mesh_index < scene->model.meshCount; mesh_index++) {
            if (PackSceneMeshIndexIsKept(mesh_index)) continue;
            UnloadMesh(scene->model.meshes[mesh_index]);
            scene->model.meshes[mesh_index] = (Mesh){0};
        }
    }
    return true;
}

static Camera3D PackSceneCamera(float reveal_mix, bool tall) {
    float t = Clamp(reveal_mix, 0.0f, 1.0f);
    t = t * t * (3.0f - 2.0f * t);

    const Camera3D *end = tall ? &PACK_SCENE_CAMERA_END_TALL : &PACK_SCENE_CAMERA_END;
    Camera3D camera = PACK_SCENE_CAMERA_START;
    camera.position = Vector3Lerp(PACK_SCENE_CAMERA_START.position, end->position, t);
    camera.target = Vector3Lerp(PACK_SCENE_CAMERA_START.target, end->target, t);
    camera.up = Vector3Normalize(Vector3Lerp(PACK_SCENE_CAMERA_START.up, end->up, t));
    return camera;
}

static void PackSceneDrawFallback(void) {
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    DrawRectangleGradientV(0, 0, width, height, (Color){8, 20, 23, 255}, (Color){2, 5, 7, 255});
    DrawCircleGradient(
        width / 2, (int)((float)height * 0.64f), (float)height * 0.38f, (Color){24, 45, 35, 170}, (Color){2, 5, 7, 0}
    );
}

static Texture2D PackSceneBaseTexture(const PackScene *scene, Rarity rarity) {
    if (rarity < RARITY_COMMON || rarity > RARITY_MYTHIC) rarity = RARITY_COMMON;
    Texture2D texture = scene->base_tex[rarity];
    return IsTextureValid(texture) ? texture : scene->base_tex[RARITY_COMMON];
}

static Texture2D PackSceneFactionTexture(PackScene *scene, Faction faction) {
    if (faction != scene->fac_loaded) {
        SceneTexUnload(scene->fac_texture);
        scene->fac_texture = (Texture2D){0};
        const char *name = HomeFactionTexName(faction);
        if (name) scene->fac_texture = LoadSprite(TextFormat(HOD_CACHE "meshes/_shared_textures/%s.png", name));
        scene->fac_loaded = faction;
    }
    return scene->fac_texture;
}

static void PackSceneDrawResultPad(PackScene *scene, float time, PackReveal reveal) {
    if (!scene->pads_loaded || time < PACK_BASE_REVEAL) return;
    Texture2D base = PackSceneBaseTexture(scene, reveal.rarity);
    if (!IsTextureValid(base)) return;

    float scale = reveal.base_scale > 0.0f ? reveal.base_scale : 1.0f;
    Matrix pad, emblem;
    SquadPadPlaceAtFeet((Vector3){PACK_UNIT_X, PACK_UNIT_FEET, PACK_UNIT_Z}, scale, &pad, &emblem, NULL);
    SquadPadDraw(&scene->pads, base, pad, reveal.stars, PackSceneFactionTexture(scene, reveal.faction), emblem);
}

static float PackSceneEaseOutQuad(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static void PackSceneDrawMockBase(PackScene *scene, float time, Rarity rarity) {
    if (scene->pads.pad_model.meshCount <= 0) return;
    if (time < PACK_BASE_APPEAR || time >= PACK_BASE_REVEAL) return;

    bool landing_begun = time >= PACK_BASE_LAND_START;
    Texture2D plate = landing_begun ? PackSceneBaseTexture(scene, rarity) : scene->base_default;
    if (!IsTextureValid(plate)) return;

    float landing = PackSceneEaseOutQuad((time - PACK_BASE_LAND_START) / (PACK_BASE_LAND_END - PACK_BASE_LAND_START));
    if (!landing_begun) landing = 0.0f;
    float grown = Clamp((time - PACK_BASE_LAND_START) / PACK_BASE_GROW_TIME, 0.0f, 1.0f);
    float scale =
        (PACK_BASE_VISIBLE_SCALE + grown * (PACK_BASE_LANDED_SCALE - PACK_BASE_VISIBLE_SCALE)) * PACK_BASE_MESH_SCALE;

    const Vector3 born = {PACK_UNIT_X, 1.5806f, -170.0f};
    const Vector3 landed = {PACK_UNIT_X, PACK_BASE_LANDED_Y, PACK_UNIT_Z};
    Vector3 position = Vector3Lerp(born, landed, landing);

    float aim = -90.0f + atan2f(PACK_BASE_LOOK_Y - position.y, fabsf(PACK_BASE_LOOK_Z - position.z)) * RAD2DEG;
    float flip = Clamp((time - PACK_BASE_LAND_START) / PACK_BASE_FLIP_TIME, 0.0f, 1.0f);
    flip = 1.0f - (1.0f - flip) * (1.0f - flip) * (1.0f - flip);
    float tilt = (PACK_BASE_UPRIGHT_DEG + (aim - PACK_BASE_UPRIGHT_DEG) * flip) * DEG2RAD;

    Matrix transform = MatrixMultiply(
        MatrixScale(scale, scale, scale),
        MatrixMultiply(MatrixRotateX(tilt), MatrixTranslate(position.x, position.y, position.z))
    );
    DrawMesh(scene->pads.pad_model.meshes[0], SquadPadMaterial(&scene->pads, plate), transform);
}

void PackSceneDraw(PackScene *scene, float reveal_mix, float time, PackReveal reveal) {
    ClearBackground(BLACK);
    if (!scene || !scene->model_loaded || !IsModelValid(scene->model)) {
        PackSceneDrawFallback();
        return;
    }

    Camera3D camera = PackSceneCamera(reveal_mix, reveal.tall);
    Matrix transform = MatrixScale(1.0f, 1.0f, -1.0f);
    int keep_count = (int)(sizeof(PACK_SCENE_KEEP_MESHES) / sizeof(PACK_SCENE_KEEP_MESHES[0]));

    BeginMode3D(camera);
    rlSetClipPlanes(PACK_SCENE_NEAR, PACK_SCENE_FAR);
    rlDisableBackfaceCulling();
    for (int i = 0; i < keep_count; i++) {
        int mesh_index = PACK_SCENE_KEEP_MESHES[i];
        if (mesh_index < 0 || mesh_index >= scene->model.meshCount) continue;
        Mesh mesh = scene->model.meshes[mesh_index];
        if (mesh.vertexCount <= 0) continue;

        int material_index = 0;
        if (scene->model.meshMaterial) material_index = scene->model.meshMaterial[mesh_index];
        if (material_index < 0 || material_index >= scene->model.materialCount) material_index = 0;
        if (!scene->model.materials || scene->model.materialCount <= 0) continue;
        if (scene->shader_loaded && PackSceneMaterialKind(material_index) == PACK_MAT_FOLIAGE) {
            bool fern = material_index == 5;
            float cutoff = fern ? PACK_SCENE_FERN_CUTOFF : PACK_SCENE_GRASS_CUTOFF;
            float vc_scale = fern ? 3.0f : 1.4f;
            float vc_shift = fern ? 0.1f : 0.2f;
            Vector3 tint = fern ? PACK_SCENE_FERN_TINT : PACK_SCENE_GRASS_TINT;
            SetShaderValue(scene->foliage_sh, scene->foliage_tint_loc, &tint, SHADER_UNIFORM_VEC3);
            SetShaderValue(scene->foliage_sh, scene->foliage_cutoff_loc, &cutoff, SHADER_UNIFORM_FLOAT);
            SetShaderValue(scene->foliage_sh, scene->foliage_vcscale_loc, &vc_scale, SHADER_UNIFORM_FLOAT);
            SetShaderValue(scene->foliage_sh, scene->foliage_vcshift_loc, &vc_shift, SHADER_UNIFORM_FLOAT);
        }
        DrawMesh(mesh, scene->model.materials[material_index], transform);
    }
    PackSceneDrawMockBase(scene, time, reveal.rarity);
    PackSceneDrawResultPad(scene, time, reveal);
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    EndMode3D();
}

void PackSceneDestroy(PackScene *scene) {
    PackSceneReleaseModel(scene);
}
