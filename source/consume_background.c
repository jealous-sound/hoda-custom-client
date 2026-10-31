#define CONSUME_SCENE_PATH "data/consume_scene.json"
#define CONSUME_ENV_DIR "TrainingAndPromotionsContainer"
#define CONSUME_ENV_MAX_NODES 64
#define CONSUME_ENV_MAX_MESHES 48
#define CONSUME_ENV_LIGHT_SCALE 10.0f
#define CONSUME_ENV_LIGHT_SHIFT 0.1f
#define CONSUME_CAMERA_TRANSITION_TIME 1.0f
#define CONSUME_HELIX_SCROLL_RATE 0.6f

typedef enum {
    CONSUME_MATERIAL_GROUND,
    CONSUME_MATERIAL_ROCK,
    CONSUME_MATERIAL_PEDESTAL,
    CONSUME_MATERIAL_POLE,
    CONSUME_MATERIAL_GRASS,
    CONSUME_MATERIAL_FLAG,
    CONSUME_MATERIAL_SKY,
    CONSUME_MATERIAL_CASTLE,
    CONSUME_MATERIAL_CIRCLE_OFF,
    CONSUME_MATERIAL_CIRCLE_ON,
    CONSUME_MATERIAL_HELIX,
    CONSUME_MATERIAL_COUNT
} ConsumeMaterialKind;

typedef enum {
    CONSUME_CAM_BOARD,
    CONSUME_CAM_ALL,
    CONSUME_CAM_COMPLETE,
    CONSUME_CAM_COMPLETE_TALL,
    CONSUME_CAM_COUNT
} ConsumeCamKey;

typedef struct {
    ConsumeMaterialKind kind;
    int mesh;
    int seed;
    Matrix matrix;
} ConsumeEnvNode;

typedef struct {
    Vector3 position, forward, up;
    float fov;
} ConsumeCamPose;

typedef struct {
    bool loaded;
    Mesh meshes[CONSUME_ENV_MAX_MESHES];
    char mesh_names[CONSUME_ENV_MAX_MESHES][48];
    int mesh_count;
    ConsumeEnvNode nodes[CONSUME_ENV_MAX_NODES];
    int node_count;
    Material materials[CONSUME_MATERIAL_COUNT];
    Shader lit_sh, cutout_sh, mix_sh, unlit_sh, helix_sh;
    int helix_scroll_loc;
    Vector3 spots[CONSUME_MAX_SEEDS + 1];
    float target_root_y;
    ConsumeCamPose cameras[CONSUME_CAM_COUNT];
    ConsumeCamPose from;
    ConsumeCamKey camera_key;
    float camera_time;
    float time;
    Camera3D camera;
    bool seed_bound[CONSUME_MAX_SEEDS];
} ConsumeEnv;

static const char *const FS_CONSUME_LIT = "#version 330\n"
                                          "in vec2 fragTexCoord; in vec4 fragColor;\n"
                                          "uniform sampler2D texture0;\n"
                                          "out vec4 finalColor;\n"
                                          "void main(){\n"
                                          "  vec3 col = texture(texture0, fragTexCoord).rgb * fragColor.rgb * 2.0;\n"
                                          "  finalColor = vec4(col, 1.0);\n"
                                          "}\n";
static const char *const FS_CONSUME_CUTOUT = "#version 330\n"
                                             "in vec2 fragTexCoord; in vec4 fragColor;\n"
                                             "uniform sampler2D texture0;\n"
                                             "out vec4 finalColor;\n"
                                             "void main(){\n"
                                             "  vec4 tex = texture(texture0, fragTexCoord);\n"
                                             "  if (tex.a < 0.4) discard;\n"
                                             "  finalColor = vec4(tex.rgb * fragColor.rgb * 2.0, 1.0);\n"
                                             "}\n";
static const char *const FS_CONSUME_MIX = "#version 330\n"
                                          "in vec2 fragTexCoord; in vec4 fragColor;\n"
                                          "uniform sampler2D texture0;\n"
                                          "uniform sampler2D texture1;\n"
                                          "uniform sampler2D texture2;\n"
                                          "out vec4 finalColor;\n"
                                          "void main(){\n"
                                          "  vec3 stone = texture(texture0, fragTexCoord*5.0).rgb;\n"
                                          "  vec3 grass = texture(texture1, fragTexCoord*7.0).rgb;\n"
                                          "  float mask = texture(texture2, fragTexCoord).r;\n"
                                          "  finalColor = vec4(mix(stone, grass, mask) * fragColor.rgb * 2.0, 1.0);\n"
                                          "}\n";
static const char *const FS_CONSUME_UNLIT = "#version 330\n"
                                            "in vec2 fragTexCoord;\n"
                                            "uniform sampler2D texture0;\n"
                                            "out vec4 finalColor;\n"
                                            "void main(){ finalColor = texture(texture0, fragTexCoord); }\n";
static const char *const FS_CONSUME_HELIX =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0;\n"
    "uniform float uScroll;\n"
    "out vec4 finalColor;\n"
    "void main(){ finalColor = texture(texture0, fragTexCoord + vec2(uScroll, 0.0)) * 0.45; }\n";

static ConsumeMaterialKind ConsumeMaterialKindFor(const char *name) {
    static const struct {
        const char *prefix;
        ConsumeMaterialKind kind;
    } kinds[] = {
        {"background_layer_stone_03", CONSUME_MATERIAL_ROCK},
        {"Stone_Snow", CONSUME_MATERIAL_ROCK},
        {"background_layer_stone", CONSUME_MATERIAL_GROUND},
        {"basesTexture", CONSUME_MATERIAL_PEDESTAL},
        {"flag_pole", CONSUME_MATERIAL_POLE},
        {"flag_Txture", CONSUME_MATERIAL_FLAG},
        {"grass", CONSUME_MATERIAL_GRASS},
        {"bgScreen", CONSUME_MATERIAL_SKY},
        {"Castle", CONSUME_MATERIAL_CASTLE},
        {"unit_circle_magi_lit", CONSUME_MATERIAL_CIRCLE_ON},
        {"unit_circle_magi", CONSUME_MATERIAL_CIRCLE_OFF},
        {"power_strip", CONSUME_MATERIAL_HELIX},
    };
    for (int i = 0; i < (int)(sizeof(kinds) / sizeof(kinds[0])); i++)
        if (!strncmp(name, kinds[i].prefix, strlen(kinds[i].prefix))) return kinds[i].kind;
    TraceLog(LOG_FATAL, "Consume scene: unknown material %s", name);
    return CONSUME_MATERIAL_ROCK;
}

static Mesh ConsumeEnvLoadFlatMesh(const char *path) {
    Model model = LoadModel(path);
    if (model.meshCount < 1) TraceLog(LOG_FATAL, "Consume scene: no mesh in %s", path);
    Mesh mesh = model.meshes[0];
    for (int i = 0; i < mesh.vertexCount; i++) mesh.vertices[i * 3 + 2] = -mesh.vertices[i * 3 + 2];
    UpdateMeshBuffer(mesh, 0, mesh.vertices, mesh.vertexCount * 3 * (int)sizeof(float), 0);
    model.meshCount = 0;
    model.meshes = NULL;
    MemFree(model.meshMaterial);
    model.meshMaterial = NULL;
    for (int i = 0; i < model.materialCount; i++) MemFree(model.materials[i].maps);
    MemFree(model.materials);
    return mesh;
}

static int ConsumeEnvMesh(ConsumeEnv *env, const char *mesh_dir, const char *name) {
    for (int i = 0; i < env->mesh_count; i++)
        if (!strcmp(env->mesh_names[i], name)) return i;
    if (env->mesh_count >= CONSUME_ENV_MAX_MESHES) TraceLog(LOG_FATAL, "Consume scene: too many meshes");
    int index = env->mesh_count++;
    StrCopy(env->mesh_names[index], sizeof(env->mesh_names[index]), name);
    const char *ply = TextFormat("%s/%s/ply_vertexcolor/%s.ply", mesh_dir, CONSUME_ENV_DIR, name);
    if (FileExists(ply)) {
        env->meshes[index] = LoadPlyMesh(ply);
    } else {
        const char *obj = TextFormat("%s/%s/%s.obj", mesh_dir, CONSUME_ENV_DIR, name);
        env->meshes[index] = ConsumeEnvLoadFlatMesh(obj);
    }
    return index;
}

static Matrix ConsumeJsonMatrix(const cJSON *json) {
    float v[16];
    for (int i = 0; i < 16; i++) v[i] = (float)cJSON_GetArrayItem((cJSON *)json, i)->valuedouble;
    return (Matrix){
        v[0], v[4], v[8], v[12], v[1], v[5], v[9], v[13], v[2], v[6], v[10], v[14], v[3], v[7], v[11], v[15]
    };
}

static Vector3 ConsumeJsonVector(const cJSON *json) {
    return (Vector3){
        (float)cJSON_GetArrayItem((cJSON *)json, 0)->valuedouble,
        (float)cJSON_GetArrayItem((cJSON *)json, 1)->valuedouble,
        (float)cJSON_GetArrayItem((cJSON *)json, 2)->valuedouble
    };
}

static int ConsumeNodeSeed(const char *name) {
    static const char *const prefixes[] = {"arrow_direction_o", "power_helix_0"};
    for (int i = 0; i < 2; i++) {
        size_t length = strlen(prefixes[i]);
        if (strncmp(name, prefixes[i], length) != 0) continue;
        const char *digit = strpbrk(name + length, "12345");
        return digit ? *digit - '1' : -1;
    }
    return -1;
}

static void ConsumeEnvLoadData(ConsumeEnv *env, const char *mesh_dir) {
    cJSON *root = JsonLoadFile(CONSUME_SCENE_PATH);
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(root, "nodes")) {
        if (env->node_count >= CONSUME_ENV_MAX_NODES) TraceLog(LOG_FATAL, "Consume scene: too many nodes");
        ConsumeEnvNode *node = &env->nodes[env->node_count++];
        node->kind = ConsumeMaterialKindFor(JsonStr(entry, "material"));
        node->mesh = ConsumeEnvMesh(env, mesh_dir, JsonStr(entry, "mesh"));
        node->seed = ConsumeNodeSeed(JsonStr(entry, "name"));
        node->matrix = ConsumeJsonMatrix(cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "matrix"));
    }
    env->spots[0] = ConsumeJsonVector(cJSON_GetObjectItemCaseSensitive(root, "target_spot"));
    env->target_root_y = JsonFloat(root, "target_root_y");
    const cJSON *seeds = cJSON_GetObjectItemCaseSensitive(root, "seed_spots");
    for (int i = 0; i < CONSUME_MAX_SEEDS; i++)
        env->spots[i + 1] = ConsumeJsonVector(cJSON_GetArrayItem((cJSON *)seeds, i));

    static const char *const keys[CONSUME_CAM_COUNT] = {"board", "all", "complete", "complete_tall"};
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(root, "cameras")) {
        for (int key = 0; key < CONSUME_CAM_COUNT; key++) {
            if (strcmp(JsonStr(entry, "key"), keys[key]) != 0) continue;
            env->cameras[key] = (ConsumeCamPose){
                .position = ConsumeJsonVector(cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "position")),
                .forward = ConsumeJsonVector(cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "forward")),
                .up = ConsumeJsonVector(cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "up")),
                .fov = JsonFloat(entry, "fov"),
            };
        }
    }
    cJSON_Delete(root);
}

static Material ConsumeEnvMaterial(Shader shader, Texture2D texture) {
    Material material = LoadMaterialDefault();
    material.shader = shader;
    material.maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    return material;
}

void ConsumeEnvEnsure(ConsumeEnv *env, const char *mesh_dir) {
    if (env->loaded) return;
    env->loaded = true;
    ConsumeEnvLoadData(env, mesh_dir);

    env->lit_sh = LoadShaderFromMemory(VS_UNLIT, FS_CONSUME_LIT);
    env->cutout_sh = LoadShaderFromMemory(VS_UNLIT, FS_CONSUME_CUTOUT);
    env->mix_sh = LoadShaderFromMemory(VS_UNLIT, FS_CONSUME_MIX);
    env->unlit_sh = LoadShaderFromMemory(VS_UNLIT, FS_CONSUME_UNLIT);
    env->helix_sh = LoadShaderFromMemory(VS_UNLIT, FS_CONSUME_HELIX);
    env->helix_scroll_loc = GetShaderLocation(env->helix_sh, "uScroll");

    Material *m = env->materials;
    m[CONSUME_MATERIAL_GROUND] = ConsumeEnvMaterial(env->mix_sh, LoadTiled(CACHE_RESOURCE("background_layer_stone")));
    m[CONSUME_MATERIAL_GROUND].maps[MATERIAL_MAP_SPECULAR].texture = LoadTiled(CACHE_RESOURCE("background_grass_01"));
    m[CONSUME_MATERIAL_GROUND].maps[MATERIAL_MAP_NORMAL].texture =
        LoadTiled(CACHE_RESOURCE("background_platform_01_mask"));
    m[CONSUME_MATERIAL_ROCK] = ConsumeEnvMaterial(env->lit_sh, LoadTiled(CACHE_RESOURCE("background_layer_stone_03")));
    m[CONSUME_MATERIAL_PEDESTAL] = ConsumeEnvMaterial(env->lit_sh, LoadTiled(CACHE_RESOURCE("basesTexture")));
    m[CONSUME_MATERIAL_POLE] = ConsumeEnvMaterial(env->lit_sh, LoadTiled(CACHE_RESOURCE("flag_pole")));
    m[CONSUME_MATERIAL_GRASS] = ConsumeEnvMaterial(env->cutout_sh, LoadTiled(CACHE_RESOURCE("grass_02_winter")));
    m[CONSUME_MATERIAL_FLAG] = ConsumeEnvMaterial(env->cutout_sh, LoadTiled(CACHE_RESOURCE("flag_Txture")));
    m[CONSUME_MATERIAL_SKY] = ConsumeEnvMaterial(env->unlit_sh, LoadSprite(CACHE_RESOURCE("woodfort_background")));
    m[CONSUME_MATERIAL_CASTLE] = ConsumeEnvMaterial(env->unlit_sh, LoadSprite(CACHE_RESOURCE("Castle3")));
    m[CONSUME_MATERIAL_CIRCLE_OFF] = ConsumeEnvMaterial(env->unlit_sh, LoadSprite(CACHE_RESOURCE("unit_circle_magi")));
    m[CONSUME_MATERIAL_CIRCLE_ON] =
        ConsumeEnvMaterial(env->unlit_sh, LoadSprite(CACHE_RESOURCE("unit_circle_magi_lit")));
    m[CONSUME_MATERIAL_HELIX] = ConsumeEnvMaterial(env->helix_sh, LoadTiled(CACHE_RESOURCE("power_strip_01")));

    env->camera_key = CONSUME_CAM_BOARD;
    env->from = env->cameras[CONSUME_CAM_BOARD];
    env->camera_time = CONSUME_CAMERA_TRANSITION_TIME;
}

void ConsumeEnvDestroy(ConsumeEnv *env) {
    if (!env->loaded) return;
    for (int i = 0; i < env->mesh_count; i++) UnloadMesh(env->meshes[i]);
    for (int i = 0; i < CONSUME_MATERIAL_COUNT; i++) {
        SceneTexUnload(env->materials[i].maps[MATERIAL_MAP_DIFFUSE].texture);
        if (i == CONSUME_MATERIAL_GROUND) {
            SceneTexUnload(env->materials[i].maps[MATERIAL_MAP_SPECULAR].texture);
            SceneTexUnload(env->materials[i].maps[MATERIAL_MAP_NORMAL].texture);
        }
        SceneMaterialUnload(env->materials[i]);
    }
    UnloadShader(env->lit_sh);
    UnloadShader(env->cutout_sh);
    UnloadShader(env->mix_sh);
    UnloadShader(env->unlit_sh);
    UnloadShader(env->helix_sh);
    env->loaded = false;
}

void ConsumeEnvSetCamera(ConsumeEnv *env, ConsumeCamKey key) {
    if (env->camera_key == key) return;
    env->from = (ConsumeCamPose){
        env->camera.position,
        Vector3Normalize(Vector3Subtract(env->camera.target, env->camera.position)),
        env->camera.up,
        env->camera.fovy
    };
    env->camera_key = key;
    env->camera_time = 0.0f;
}

void ConsumeEnvSnapCamera(ConsumeEnv *env, ConsumeCamKey key) {
    env->camera_key = key;
    env->camera_time = CONSUME_CAMERA_TRANSITION_TIME;
}

void ConsumeEnvUpdate(ConsumeEnv *env, float dt) {
    env->time += dt;
    env->camera_time = fminf(env->camera_time + dt, CONSUME_CAMERA_TRANSITION_TIME);
    float t = env->camera_time / CONSUME_CAMERA_TRANSITION_TIME;
    t = t * t * (3.0f - 2.0f * t);
    const ConsumeCamPose *to = &env->cameras[env->camera_key];
    Vector3 forward = Vector3Normalize(Vector3Lerp(env->from.forward, to->forward, t));
    Vector3 position = Vector3Lerp(env->from.position, to->position, t);
    env->camera = (Camera3D){
        .position = position,
        .target = Vector3Add(position, forward),
        .up = Vector3Normalize(Vector3Lerp(env->from.up, to->up, t)),
        .fovy = Lerp(env->from.fov, to->fov, t),
        .projection = CAMERA_PERSPECTIVE,
    };
}

static void ConsumeEnvDrawKind(const ConsumeEnv *env, ConsumeMaterialKind kind) {
    for (int i = 0; i < env->node_count; i++) {
        const ConsumeEnvNode *node = &env->nodes[i];
        if (node->kind != kind) continue;
        if (node->seed >= 0 && env->seed_bound[node->seed] != (kind != CONSUME_MATERIAL_CIRCLE_OFF)) continue;
        DrawMesh(env->meshes[node->mesh], env->materials[kind], node->matrix);
    }
}

void ConsumeEnvDrawBackground(ConsumeEnv *env) {
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_SKY);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_CASTLE);
    EndBlendMode();
    rlEnableDepthMask();
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_GROUND);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_ROCK);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_PEDESTAL);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_POLE);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_GRASS);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_FLAG);

    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_CIRCLE_OFF);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_CIRCLE_ON);
    EndBlendMode();
    rlEnableDepthMask();
}

void ConsumeEnvDrawEffects(ConsumeEnv *env) {
    float scroll = fmodf(env->time * CONSUME_HELIX_SCROLL_RATE, 1.0f);
    SetShaderValue(env->helix_sh, env->helix_scroll_loc, &scroll, SHADER_UNIFORM_FLOAT);
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    ConsumeEnvDrawKind(env, CONSUME_MATERIAL_HELIX);
    EndBlendMode();
    rlEnableDepthMask();
}
