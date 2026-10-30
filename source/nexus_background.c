
#define NEXUS_BG_MAX_LAYERS 48

#define NEXUS_BG_LIVE_ROOT_PREFIX "map."

#define NEXUS_NODE_PLANE_Y 15.09556f

static const char *const NEXUS_FS_PLATE = "#version 330\n"
                                          "in vec2 fragTexCoord;\n"
                                          "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
                                          "out vec4 finalColor;\n"
                                          "void main(){ finalColor = texture(texture0, fragTexCoord)*colDiffuse; }\n";

typedef struct {
    Model model;
    Matrix xform;
    float depth;
    float min_x, max_x;
} NexusLayer;

typedef struct {
    Shader plate_sh;
    NexusLayer layers[NEXUS_BG_MAX_LAYERS];
    Texture2D textures[NEXUS_BG_MAX_LAYERS];
    int layer_count;

    Camera3D cam;
    float cam_home_x;
    float scroll_x;

    Vector3 node_origin;
    float node_scale;
    Color clear;
    char prefab[64];
} NexusBackground;

static const struct {
    const char *material;
    const char *texture;
} NEXUS_TEX_OVERRIDES[] = {
    {"SnexusAV_BG_lava", "SnexusAV_BG_tex.png"},
    {"SnexusB_RockGlow_tex", "SnexusB_midground_tex.png"},
    {"SnexusDP_map1_lava", "SnexusDP_map1_tex.png"},
    {"DemonParty_Back", "SnexusDEMON_BG.png"},
    {"DemonParty_Main", "SnexusDEMON_MAIN.png"},
    {"Border_Black", NULL},
    {"Default-Diffuse", NULL},
};

static bool NexusResolveTexture(const char *mesh_dir, const char *prefab, const char *material, char *out, int cap) {
    out[0] = '\0';
    if (!material || !material[0]) return false;
    for (int i = 0; i < (int)(sizeof(NEXUS_TEX_OVERRIDES) / sizeof(NEXUS_TEX_OVERRIDES[0])); i++) {
        if (strcmp(NEXUS_TEX_OVERRIDES[i].material, material) != 0) continue;
        if (!NEXUS_TEX_OVERRIDES[i].texture) return false;
        snprintf(out, (size_t)cap, "%s/%s/%s", mesh_dir, prefab, NEXUS_TEX_OVERRIDES[i].texture);
        return true;
    }

    char base[96];
    StrCopy(base, sizeof(base), material);
    static const char *const trims[] = {" 1", "_mat1", "_mat"};
    for (int t = 0; t < 3; t++) {
        int len = (int)strlen(base), tlen = (int)strlen(trims[t]);
        if (len > tlen && strcmp(base + len - tlen, trims[t]) == 0) {
            base[len - tlen] = '\0';
            break;
        }
    }

    static const char *const suffixes[] = {"", "_tex", "_text"};
    const char *stems[2] = {material, base};
    for (int s = 0; s < 2; s++) {
        for (int f = 0; f < 3; f++) {
            snprintf(out, (size_t)cap, "%s/%s/%s%s.png", mesh_dir, prefab, stems[s], suffixes[f]);
            if (FileExists(out)) return true;
        }
    }
    out[0] = '\0';
    return false;
}

static Vector3 NexusJsonVec3(const cJSON *node, const char *key) {
    cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    Vector3 v = {0};
    if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) < 3) return v;
    v.x = (float)cJSON_GetArrayItem(array, 0)->valuedouble;
    v.y = (float)cJSON_GetArrayItem(array, 1)->valuedouble;
    v.z = (float)cJSON_GetArrayItem(array, 2)->valuedouble;
    return v;
}

static Quaternion NexusJsonQuat(const cJSON *node, const char *key) {
    cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    Quaternion q = QuaternionIdentity();
    if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) < 4) return q;
    q.x = (float)cJSON_GetArrayItem(array, 0)->valuedouble;
    q.y = (float)cJSON_GetArrayItem(array, 1)->valuedouble;
    q.z = (float)cJSON_GetArrayItem(array, 2)->valuedouble;
    q.w = (float)cJSON_GetArrayItem(array, 3)->valuedouble;
    return q;
}

static cJSON *NexusFindByTransform(cJSON *nodes, int transform_id) {
    cJSON *n = NULL;
    cJSON_ArrayForEach(n, nodes) {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(n, "transform_id");
        if (cJSON_IsNumber(id) && id->valueint == transform_id) return n;
    }
    return NULL;
}

static cJSON *NexusRootOf(cJSON *nodes, cJSON *node) {
    for (int guard = 0; guard < NEXUS_BG_MAX_LAYERS * 8 && node; guard++) {
        cJSON *parent_id = cJSON_GetObjectItemCaseSensitive(node, "parent_id");
        if (!cJSON_IsNumber(parent_id)) return node;
        cJSON *parent = NexusFindByTransform(nodes, parent_id->valueint);
        if (!parent) return node;
        node = parent;
    }
    return node;
}

static Matrix NexusComposeXform(cJSON *nodes, cJSON *node, cJSON *root) {
    Matrix local = MatrixIdentity();
    for (cJSON *n = node; n && n != root;) {
        Vector3 p = NexusJsonVec3(n, "local_position");
        Vector3 s = NexusJsonVec3(n, "local_scale");
        Quaternion q = NexusJsonQuat(n, "local_rotation_quat");
        Matrix trs = MatrixMultiply(
            MatrixMultiply(MatrixScale(s.x, s.y, s.z), QuaternionToMatrix(q)), MatrixTranslate(p.x, p.y, p.z)
        );
        local = MatrixMultiply(local, trs);
        cJSON *parent_id = cJSON_GetObjectItemCaseSensitive(n, "parent_id");
        n = cJSON_IsNumber(parent_id) ? NexusFindByTransform(nodes, parent_id->valueint) : NULL;
    }
    return MatrixMultiply(local, MatrixScale(1.0f, 1.0f, -1.0f));
}

static int NexusLayerCompare(const void *a, const void *b) {
    float da = ((const NexusLayer *)a)->depth, db = ((const NexusLayer *)b)->depth;
    return (da > db) - (da < db);
}

void NexusBackgroundLoad(NexusBackground *bg, const char *mesh_dir, const char *prefab) {
    memset(bg, 0, sizeof(*bg));
    StrCopy(bg->prefab, sizeof(bg->prefab), prefab);
    bg->plate_sh = LoadShaderFromMemory(VS_UNLIT, NEXUS_FS_PLATE);

    cJSON *root_json = JsonLoadFile(TextFormat("%s/%s/scene_config.json", mesh_dir, prefab));
    cJSON *nodes = cJSON_GetObjectItemCaseSensitive(root_json, "nodes");
    if (!cJSON_IsArray(nodes)) TraceLog(LOG_FATAL, "Nexus: %s scene_config has no nodes", prefab);

    cJSON *live_root = NULL;
    cJSON *n = NULL;
    cJSON_ArrayForEach(n, nodes) {
        const char *name = JsonStr(n, "name");
        if (strncmp(name, NEXUS_BG_LIVE_ROOT_PREFIX, strlen(NEXUS_BG_LIVE_ROOT_PREFIX)) != 0) continue;
        cJSON *parent_id = cJSON_GetObjectItemCaseSensitive(n, "parent_id");
        if (cJSON_IsNumber(parent_id) && NexusFindByTransform(nodes, parent_id->valueint)) continue;
        live_root = n;
    }
    if (!live_root) TraceLog(LOG_FATAL, "Nexus: %s has no map.* root", prefab);

    Vector3 cam_local = {0};
    bg->node_scale = 1.0f;
    cJSON_ArrayForEach(n, nodes) {
        const char *name = JsonStr(n, "name");
        if (strcmp(name, "MapCamera") == 0) {
            cam_local = NexusJsonVec3(n, "local_position");
        } else if (strcmp(name, "NodeMarkers") == 0) {
            bg->node_origin = NexusJsonVec3(n, "local_position");
            bg->node_scale = NexusJsonVec3(n, "local_scale").x;
        }
        if (!cJSON_IsString(cJSON_GetObjectItemCaseSensitive(n, "mesh"))) continue;
        if (NexusRootOf(nodes, n) != live_root) continue;
        if (bg->layer_count >= NEXUS_BG_MAX_LAYERS) {
            TraceLog(LOG_WARNING, "Nexus: %s has more than %d layers", prefab, NEXUS_BG_MAX_LAYERS);
            break;
        }

        cJSON *materials = cJSON_GetObjectItemCaseSensitive(n, "materials");
        const char *material = cJSON_IsArray(materials) && cJSON_GetArraySize(materials) > 0
                                   ? cJSON_GetArrayItem(materials, 0)->valuestring
                                   : "";
        char tex_path[512];
        if (!NexusResolveTexture(mesh_dir, prefab, material, tex_path, sizeof(tex_path))) continue;

        const char *obj = TextFormat("%s/%s/%s.obj", mesh_dir, prefab, JsonStr(n, "mesh"));
        if (!FileExists(obj)) continue;
        Model model = LoadModel(obj);
        if (model.meshCount == 0) {
            UnloadModel(model);
            continue;
        }
        Texture2D tex = LoadTiled(tex_path);
        model.materials[0].shader = bg->plate_sh;
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;

        NexusLayer *layer = &bg->layers[bg->layer_count];
        layer->model = model;
        layer->xform = NexusComposeXform(nodes, n, live_root);
        BoundingBox box = GetMeshBoundingBox(model.meshes[0]);
        float sum_y = 0.0f;
        for (int corner = 0; corner < 8; corner++) {
            Vector3 v = {
                (corner & 1) ? box.max.x : box.min.x,
                (corner & 2) ? box.max.y : box.min.y,
                (corner & 4) ? box.max.z : box.min.z,
            };
            Vector3 world = Vector3Transform(v, layer->xform);
            layer->min_x = corner == 0 ? world.x : fminf(layer->min_x, world.x);
            layer->max_x = corner == 0 ? world.x : fmaxf(layer->max_x, world.x);
            sum_y += world.y;
        }
        layer->depth = sum_y / 8.0f;
        bg->textures[bg->layer_count] = tex;
        bg->layer_count++;
    }
    cJSON_Delete(root_json);

    qsort(bg->layers, (size_t)bg->layer_count, sizeof(bg->layers[0]), NexusLayerCompare);

    bg->clear = BLACK;
    if (bg->layer_count > 0) {
        Image deepest = LoadImageFromTexture(bg->layers[0].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        if (deepest.data) {
            bg->clear = GetImageColor(deepest, deepest.width / 2, deepest.height / 2);
            bg->clear.a = 255;
            UnloadImage(deepest);
        }
    }

    Quaternion cam_rot = {0.0f, 0.70711f, -0.70711f, 0.0f};
    Vector3 forward = Vector3RotateByQuaternion((Vector3){0.0f, 0.0f, 1.0f}, cam_rot);
    Vector3 up = Vector3RotateByQuaternion((Vector3){0.0f, 1.0f, 0.0f}, cam_rot);
    Vector3 position = cam_local;
    Vector3 target = Vector3Add(position, forward);
    position.z = -position.z;
    target.z = -target.z;
    up.z = -up.z;
    bg->cam_home_x = cam_local.x;
    bg->cam = (Camera3D){0};
    bg->cam.position = position;
    bg->cam.target = target;
    bg->cam.up = up;
    bg->cam.fovy = 26.0f;
    bg->cam.projection = CAMERA_PERSPECTIVE;

    TraceLog(LOG_INFO, "Nexus: %s loaded %d layers", prefab, bg->layer_count);
}

void NexusBackgroundUnload(NexusBackground *bg) {
    for (int i = 0; i < bg->layer_count; i++) {
        SceneTexUnload(bg->textures[i]);
        bg->layers[i].model.materials[0].shader = (Shader){0};
        bg->layers[i].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = (Texture2D){0};
        UnloadModel(bg->layers[i].model);
    }
    UnloadShader(bg->plate_sh);
    memset(bg, 0, sizeof(*bg));
}

void NexusBackgroundSetScroll(NexusBackground *bg, float x) {
    bg->scroll_x = x;
    bg->cam.position.x = x;
    bg->cam.target.x = x;
}

float NexusBackgroundHomeX(const NexusBackground *bg) {
    return bg->cam_home_x;
}

Vector3 NexusBackgroundNodeWorld(const NexusBackground *bg, float table_x, float table_z) {
    return (Vector3){
        bg->node_origin.x + bg->node_scale * table_x,
        bg->node_origin.y + bg->node_scale * NEXUS_NODE_PLANE_Y,
        -(bg->node_origin.z + bg->node_scale * table_z),
    };
}

float NexusBackgroundScrollForNode(const NexusBackground *bg, float table_x) {
    return bg->node_origin.x + bg->node_scale * table_x;
}

void NexusBackgroundArtScrollLimits(const NexusBackground *bg, float *out_lo, float *out_hi) {
    if (bg->layer_count == 0) {
        *out_lo = *out_hi = bg->cam_home_x;
        return;
    }
    float aspect = UiWidth() / UiHeight();
    float tan_half = tanf(bg->cam.fovy * 0.5f * DEG2RAD);
    float best_lo = 0.0f, best_hi = 0.0f, best_span = -1.0e9f;
    float envelope_lo = bg->layers[0].min_x, envelope_hi = bg->layers[0].max_x;
    for (int i = 0; i < bg->layer_count; i++) {
        const NexusLayer *layer = &bg->layers[i];
        envelope_lo = fminf(envelope_lo, layer->min_x);
        envelope_hi = fmaxf(envelope_hi, layer->max_x);
        float half_w = (bg->cam.position.y - layer->depth) * tan_half * aspect;
        float lo = layer->min_x + half_w, hi = layer->max_x - half_w;
        if (hi - lo <= best_span) continue;
        best_span = hi - lo;
        best_lo = lo;
        best_hi = hi;
    }
    if (best_span < 0.0f) {
        best_lo = envelope_lo;
        best_hi = envelope_hi;
    }
    *out_lo = best_lo;
    *out_hi = best_hi;
}

Vector2 NexusBackgroundNodeScreen(const NexusBackground *bg, Vector3 world) {
    Vector2 pixels = GetWorldToScreen(world, bg->cam);
    float scale = UiScale();
    return (Vector2){pixels.x / scale, pixels.y / scale};
}

float NexusBackgroundNodePlaneY(const NexusBackground *bg) {
    return bg->node_origin.y + bg->node_scale * NEXUS_NODE_PLANE_Y;
}

void NexusBackgroundDrawRange(const NexusBackground *bg, float min_y, float max_y) {
    rlDisableBackfaceCulling();
    for (int i = 0; i < bg->layer_count; i++) {
        if (bg->layers[i].depth < min_y || bg->layers[i].depth >= max_y) continue;
        DrawMesh(bg->layers[i].model.meshes[0], bg->layers[i].model.materials[0], bg->layers[i].xform);
    }
    rlEnableBackfaceCulling();
}

void NexusBackgroundDraw(const NexusBackground *bg) {
    NexusBackgroundDrawRange(bg, -1.0e9f, 1.0e9f);
}
