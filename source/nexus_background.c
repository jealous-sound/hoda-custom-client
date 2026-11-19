#define NEXUS_SCENES_PATH "data/nexus_scenes.json"

#define NEXUS_BG_MAX_LAYERS 48
#define NEXUS_BG_MAX_TEXTURES 8

#define NEXUS_NODE_TABLE_Y 15.09556f

#define NEXUS_COVER_ROWS 24
#define NEXUS_COVER_EDGE_NDC 0.998f
#define NEXUS_COVER_TOP_NDC (1.0f - 2.0f * HUD_TOP_HEIGHT / UiHeight())
#define NEXUS_COVER_STEP 0.25f
#define NEXUS_COVER_REFINE_STEPS 12
#define NEXUS_COVER_ALPHA 0.5f
#define NEXUS_COVER_MASK_SIZE 256
#define NEXUS_RAY_EPSILON 1.0e-6f
#define NEXUS_HIT_BOUNDS_PAD 0.01f

#define NEXUS_SEAM_SAMPLES 16
#define NEXUS_SEAM_INSET 0.01f
#define NEXUS_SEAM_OCCLUSION_GAP 0.05f
#define NEXUS_SEAM_MIN_EXPOSED 2
#define NEXUS_SEAM_STRIP_SHARE 0.4f

#define NEXUS_MIN_VIEW_SCALE 0.8f
#define NEXUS_VIEW_FIT_STEPS 7

#define NEXUS_OPAQUE_QUEUE 0
#define NEXUS_MAX_DRAWS (NEXUS_BG_MAX_LAYERS + MAP_SCENE_MAX_EMITTERS)

static const char *const NEXUS_FS_LAYER = "#version 330\n"
                                          "in vec2 fragTexCoord;\n"
                                          "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
                                          "uniform float opaque; uniform float cutoff;\n"
                                          "out vec4 finalColor;\n"
                                          "void main(){\n"
                                          "  vec4 c = texture(texture0, fragTexCoord)*colDiffuse;\n"
                                          "  if (c.a < cutoff) discard;\n"
                                          "  finalColor = vec4(c.rgb, mix(c.a, 1.0, opaque));\n"
                                          "}\n";

typedef enum {
    NEXUS_BLEND_OPAQUE,
    NEXUS_BLEND_CUTOUT,
    NEXUS_BLEND_ALPHA,
    NEXUS_BLEND_ADDITIVE,
} NexusBlend;

typedef struct {
    char ref[128];
    Texture2D texture;
    Image mask;
} NexusSceneTexture;

typedef struct {
    Mesh mesh;
    int texture;
    Color color;
    NexusBlend blend;
    float cutoff;
    Vector3 centre;
    BoundingBox bounds;
    Vector3 seams[NEXUS_SEAM_SAMPLES * 2];
    unsigned char seam_end[NEXUS_SEAM_SAMPLES * 2];
    int seam_count;
    bool mirror;
    int mirror_plane_end;
    bool end_mirrored[2];
} NexusLayer;

typedef struct {
    Vector3 world;
    float half_width;
} NexusViewTarget;

typedef struct {
    Shader layer_sh;
    int opaque_loc, cutoff_loc;
    Material material;
    Texture2D white;
    NexusSceneTexture textures[NEXUS_BG_MAX_TEXTURES];
    int texture_count;
    NexusLayer layers[NEXUS_BG_MAX_LAYERS];
    int layer_count;
    MapSceneParticles particles;

    Camera3D cam;
    float cam_home_x;
    float base_fovy;
    bool mirrors_on;

    Matrix node_frame;
    Vector2 node_unit;

    float scroll_lo, scroll_hi;
    char prefab[64];
} NexusBackground;

static Vector3 NexusJsonVec3(const cJSON *node, const char *key) {
    cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    Vector3 v = {0};
    if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) < 3) return v;
    v.x = (float)cJSON_GetArrayItem(array, 0)->valuedouble;
    v.y = (float)cJSON_GetArrayItem(array, 1)->valuedouble;
    v.z = (float)cJSON_GetArrayItem(array, 2)->valuedouble;
    return v;
}

static Matrix NexusJsonMatrix(const cJSON *json) {
    float v[16] = {0};
    for (int i = 0; i < 16; i++) v[i] = (float)cJSON_GetArrayItem((cJSON *)json, i)->valuedouble;
    return (Matrix){
        v[0], v[4], v[8], v[12], v[1], v[5], v[9], v[13], v[2], v[6], v[10], v[14], v[3], v[7], v[11], v[15]
    };
}

static float *NexusJsonFloats(const cJSON *json, const char *key, int *count) {
    cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)json, key);
    *count = cJSON_IsArray(array) ? cJSON_GetArraySize(array) : 0;
    float *values = MemAlloc(sizeof(float) * (size_t)(*count > 0 ? *count : 1));
    int i = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, array) values[i++] = (float)item->valuedouble;
    return values;
}

static NexusBlend NexusBlendNamed(const char *name) {
    if (strcmp(name, "opaque") == 0) return NEXUS_BLEND_OPAQUE;
    if (strcmp(name, "cutout") == 0) return NEXUS_BLEND_CUTOUT;
    if (strcmp(name, "additive") == 0) return NEXUS_BLEND_ADDITIVE;
    if (strcmp(name, "alpha") != 0) TraceLog(LOG_WARNING, "Nexus: unknown blend %s", name);
    return NEXUS_BLEND_ALPHA;
}

static int NexusSceneTextureLoad(NexusBackground *bg, const char *ref) {
    if (!ref) return -1;
    for (int i = 0; i < bg->texture_count; i++)
        if (strcmp(bg->textures[i].ref, ref) == 0) return i;
    if (bg->texture_count >= NEXUS_BG_MAX_TEXTURES) {
        TraceLog(LOG_WARNING, "Nexus: %s has more than %d textures", bg->prefab, NEXUS_BG_MAX_TEXTURES);
        return -1;
    }
    Image image = ResourceLoadImage(ref);
    if (!image.data) return -1;
    NexusSceneTexture *entry = &bg->textures[bg->texture_count];
    StrCopy(entry->ref, sizeof(entry->ref), ref);
    entry->texture = LoadTextureFromImage(image);
    SetTextureWrap(entry->texture, TEXTURE_WRAP_REPEAT);
    GenTextureMipmaps(&entry->texture);
    SetTextureFilter(entry->texture, TEXTURE_FILTER_TRILINEAR);
    ImageResize(&image, NEXUS_COVER_MASK_SIZE, NEXUS_COVER_MASK_SIZE);
    entry->mask = image;
    return bg->texture_count++;
}

static Mesh NexusLayerMesh(const cJSON *json) {
    int position_count = 0, uv_count = 0, index_count = 0;
    float *positions = NexusJsonFloats(json, "positions", &position_count);
    float *uvs = NexusJsonFloats(json, "uvs", &uv_count);
    float *indices = NexusJsonFloats(json, "indices", &index_count);
    Mesh mesh = {0};
    mesh.vertexCount = position_count / 3;
    mesh.triangleCount = index_count / 3;
    mesh.vertices = positions;
    mesh.texcoords = uvs;
    if (uv_count != mesh.vertexCount * 2) TraceLog(LOG_FATAL, "Nexus: layer uv count does not match its vertices");
    mesh.indices = MemAlloc(sizeof(unsigned short) * (size_t)(index_count > 0 ? index_count : 1));
    for (int i = 0; i < index_count; i++) mesh.indices[i] = (unsigned short)indices[i];
    MemFree(indices);
    UploadMesh(&mesh, false);
    return mesh;
}

static bool NexusLayerLoad(NexusBackground *bg, NexusLayer *layer, const cJSON *json) {
    cJSON *texture = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "texture");
    layer->texture = NexusSceneTextureLoad(bg, cJSON_IsString(texture) ? texture->valuestring : NULL);
    if (cJSON_IsString(texture) && layer->texture < 0) return false;
    layer->mesh = NexusLayerMesh(json);
    cJSON *color = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "color");
    float rgba[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    for (int i = 0; i < 4 && i < cJSON_GetArraySize(color); i++)
        rgba[i] = (float)cJSON_GetArrayItem(color, i)->valuedouble;
    layer->color = ColorFromNormalized((Vector4){rgba[0], rgba[1], rgba[2], rgba[3]});
    layer->blend = NexusBlendNamed(JsonStr(json, "blend"));
    layer->cutoff = layer->blend == NEXUS_BLEND_CUTOUT ? JsonFloat(json, "cutoff") : 0.0f;
    layer->bounds = GetMeshBoundingBox(layer->mesh);
    layer->centre = Vector3Scale(Vector3Add(layer->bounds.min, layer->bounds.max), 0.5f);
    return true;
}

static bool NexusLayerIsFrame(const NexusLayer *layer) {
    return layer->texture < 0;
}

static bool NexusLayerActive(const NexusBackground *bg, const NexusLayer *layer) {
    return !layer->mirror || bg->mirrors_on;
}

static bool NexusSeamCounts(const NexusBackground *bg, const NexusLayer *layer, int end) {
    if (layer->mirror) return end != layer->mirror_plane_end;
    return !(bg->mirrors_on && layer->end_mirrored[end]);
}

static bool NexusRayTriangle(Ray ray, Vector3 a, Vector3 b, Vector3 c, float *distance, float *bary_u, float *bary_v) {
    Vector3 ab = Vector3Subtract(b, a), ac = Vector3Subtract(c, a);
    Vector3 p = Vector3CrossProduct(ray.direction, ac);
    float det = Vector3DotProduct(ab, p);
    if (fabsf(det) < NEXUS_RAY_EPSILON) return false;
    float inv = 1.0f / det;
    Vector3 s = Vector3Subtract(ray.position, a);
    float u = Vector3DotProduct(s, p) * inv;
    if (u < 0.0f || u > 1.0f) return false;
    Vector3 q = Vector3CrossProduct(s, ab);
    float v = Vector3DotProduct(ray.direction, q) * inv;
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = Vector3DotProduct(ac, q) * inv;
    if (t <= 0.0f) return false;
    *distance = t;
    *bary_u = u;
    *bary_v = v;
    return true;
}

static float NexusMaskAlpha(const Image *mask, float u, float v) {
    int x = (int)floorf((u - floorf(u)) * (float)mask->width);
    int y = (int)floorf((v - floorf(v)) * (float)mask->height);
    x = x < 0 ? 0 : x >= mask->width ? mask->width - 1 : x;
    y = y < 0 ? 0 : y >= mask->height ? mask->height - 1 : y;
    return (float)((const Color *)mask->data)[y * mask->width + x].a / 255.0f;
}

static bool NexusLayerSolidAt(const NexusBackground *bg, const NexusLayer *layer, Ray ray, float *distance) {
    if (layer->blend == NEXUS_BLEND_ADDITIVE) return false;
    BoundingBox padded = {
        Vector3SubtractValue(layer->bounds.min, NEXUS_HIT_BOUNDS_PAD),
        Vector3AddValue(layer->bounds.max, NEXUS_HIT_BOUNDS_PAD),
    };
    if (!GetRayCollisionBox(ray, padded).hit) return false;
    const Mesh *mesh = &layer->mesh;
    bool found = false;
    float best = 1.0e9f;
    for (int tri = 0; tri < mesh->triangleCount; tri++) {
        int ia = mesh->indices[tri * 3], ib = mesh->indices[tri * 3 + 1], ic = mesh->indices[tri * 3 + 2];
        Vector3 a = {mesh->vertices[ia * 3], mesh->vertices[ia * 3 + 1], mesh->vertices[ia * 3 + 2]};
        Vector3 b = {mesh->vertices[ib * 3], mesh->vertices[ib * 3 + 1], mesh->vertices[ib * 3 + 2]};
        Vector3 c = {mesh->vertices[ic * 3], mesh->vertices[ic * 3 + 1], mesh->vertices[ic * 3 + 2]};
        float t = 0.0f, bu = 0.0f, bv = 0.0f;
        if (!NexusRayTriangle(ray, a, b, c, &t, &bu, &bv) || t >= best) continue;
        if (!NexusLayerIsFrame(layer) && layer->blend != NEXUS_BLEND_OPAQUE) {
            float bw = 1.0f - bu - bv;
            float u = bw * mesh->texcoords[ia * 2] + bu * mesh->texcoords[ib * 2] + bv * mesh->texcoords[ic * 2];
            float v =
                bw * mesh->texcoords[ia * 2 + 1] + bu * mesh->texcoords[ib * 2 + 1] + bv * mesh->texcoords[ic * 2 + 1];
            float alpha = NexusMaskAlpha(&bg->textures[layer->texture].mask, u, v) * (float)layer->color.a / 255.0f;
            float needed = layer->blend == NEXUS_BLEND_CUTOUT ? layer->cutoff : NEXUS_COVER_ALPHA;
            if (alpha < needed) continue;
        }
        best = t;
        found = true;
    }
    *distance = best;
    return found;
}

static void NexusArtEnvelope(const NexusBackground *bg, float *out_lo, float *out_hi) {
    *out_lo = *out_hi = bg->cam_home_x;
    for (int i = 0; i < bg->layer_count; i++) {
        if (NexusLayerIsFrame(&bg->layers[i]) || !NexusLayerActive(bg, &bg->layers[i])) continue;
        *out_lo = fminf(*out_lo, bg->layers[i].bounds.min.x);
        *out_hi = fmaxf(*out_hi, bg->layers[i].bounds.max.x);
    }
}

static void NexusLayerFindSeams(const NexusBackground *bg, NexusLayer *layer, float envelope_width) {
    layer->seam_count = 0;
    if (NexusLayerIsFrame(layer) || layer->blend == NEXUS_BLEND_ADDITIVE) return;
    float width = layer->bounds.max.x - layer->bounds.min.x;
    if (width < envelope_width * NEXUS_SEAM_STRIP_SHARE) return;
    float rows = layer->bounds.max.z - layer->bounds.min.z;
    for (int end = 0; end < 2; end++) {
        float x =
            end == 0 ? layer->bounds.min.x + width * NEXUS_SEAM_INSET : layer->bounds.max.x - width * NEXUS_SEAM_INSET;
        for (int i = 0; i < NEXUS_SEAM_SAMPLES; i++) {
            float z = layer->bounds.min.z + rows * ((float)i + 0.5f) / (float)NEXUS_SEAM_SAMPLES;
            Ray down = {{x, layer->bounds.max.y + 1.0f, z}, {0.0f, -1.0f, 0.0f}};
            float distance = 0.0f;
            if (!NexusLayerSolidAt(bg, layer, down, &distance)) continue;
            layer->seam_end[layer->seam_count] = (unsigned char)end;
            layer->seams[layer->seam_count++] = Vector3Add(down.position, Vector3Scale(down.direction, distance));
        }
    }
}

static int NexusSeamsAtEnd(const NexusLayer *layer, int end) {
    int count = 0;
    for (int i = 0; i < layer->seam_count; i++) count += layer->seam_end[i] == end;
    return count;
}

static void NexusAddMirror(NexusBackground *bg, int source, int end, float envelope_width) {
    if (bg->layer_count >= NEXUS_BG_MAX_LAYERS) return;
    NexusLayer *from = &bg->layers[source];
    NexusLayer *to = &bg->layers[bg->layer_count];
    float plane = end == 0 ? from->bounds.min.x : from->bounds.max.x;
    Mesh mesh = {0};
    mesh.vertexCount = from->mesh.vertexCount;
    mesh.triangleCount = from->mesh.triangleCount;
    mesh.vertices = MemAlloc(sizeof(float) * 3 * (size_t)mesh.vertexCount);
    mesh.texcoords = MemAlloc(sizeof(float) * 2 * (size_t)mesh.vertexCount);
    mesh.indices = MemAlloc(sizeof(unsigned short) * 3 * (size_t)mesh.triangleCount);
    for (int i = 0; i < mesh.vertexCount; i++) {
        mesh.vertices[i * 3] = 2.0f * plane - from->mesh.vertices[i * 3];
        mesh.vertices[i * 3 + 1] = from->mesh.vertices[i * 3 + 1];
        mesh.vertices[i * 3 + 2] = from->mesh.vertices[i * 3 + 2];
    }
    memcpy(mesh.texcoords, from->mesh.texcoords, sizeof(float) * 2 * (size_t)mesh.vertexCount);
    memcpy(mesh.indices, from->mesh.indices, sizeof(unsigned short) * 3 * (size_t)mesh.triangleCount);
    UploadMesh(&mesh, false);
    *to = *from;
    to->mesh = mesh;
    to->bounds = GetMeshBoundingBox(mesh);
    to->centre = Vector3Scale(Vector3Add(to->bounds.min, to->bounds.max), 0.5f);
    to->mirror = true;
    to->mirror_plane_end = 1 - end;
    to->end_mirrored[0] = to->end_mirrored[1] = false;
    from->end_mirrored[end] = true;
    NexusLayerFindSeams(bg, to, envelope_width);
    bg->layer_count++;
}

static void NexusAddMirrors(NexusBackground *bg, float envelope_width) {
    int originals = bg->layer_count;
    for (int i = 0; i < originals; i++)
        for (int end = 0; end < 2; end++)
            if (NexusLayerIsFrame(&bg->layers[i]) || NexusSeamsAtEnd(&bg->layers[i], end) >= NEXUS_SEAM_MIN_EXPOSED)
                NexusAddMirror(bg, i, end, envelope_width);
}

static const cJSON *NexusSceneJson(const cJSON *root, const char *prefab) {
    cJSON *scene = NULL;
    cJSON_ArrayForEach(scene, cJSON_GetObjectItemCaseSensitive((cJSON *)root, "scenes")) {
        if (strcmp(JsonStr(scene, "prefab"), prefab) == 0) return scene;
    }
    return NULL;
}

void NexusBackgroundLoad(NexusBackground *bg, const char *prefab) {
    memset(bg, 0, sizeof(*bg));
    StrCopy(bg->prefab, sizeof(bg->prefab), prefab);
    bg->layer_sh = LoadShaderFromMemory(VS_UNLIT, NEXUS_FS_LAYER);
    bg->opaque_loc = GetShaderLocation(bg->layer_sh, "opaque");
    bg->cutoff_loc = GetShaderLocation(bg->layer_sh, "cutoff");
    bg->material = LoadMaterialDefault();
    bg->material.shader = bg->layer_sh;
    bg->white = bg->material.maps[MATERIAL_MAP_DIFFUSE].texture;

    cJSON *root = JsonLoadFile(NEXUS_SCENES_PATH);
    const cJSON *scene = NexusSceneJson(root, prefab);
    if (!scene) TraceLog(LOG_FATAL, "Nexus: %s is not in %s", prefab, NEXUS_SCENES_PATH);

    const cJSON *camera = cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "camera");
    Vector3 position = NexusJsonVec3(camera, "position");
    bg->cam = (Camera3D){
        .position = position,
        .target = Vector3Add(position, NexusJsonVec3(camera, "forward")),
        .up = NexusJsonVec3(camera, "up"),
        .fovy = JsonFloat(camera, "fov"),
        .projection = CAMERA_PERSPECTIVE,
    };
    bg->cam_home_x = position.x;
    bg->base_fovy = bg->cam.fovy;
    bg->scroll_lo = bg->scroll_hi = position.x;

    const cJSON *nodes = cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "nodes");
    bg->node_frame = NexusJsonMatrix(cJSON_GetObjectItemCaseSensitive((cJSON *)nodes, "matrix"));
    cJSON *unit = cJSON_GetObjectItemCaseSensitive((cJSON *)nodes, "unit");
    bg->node_unit =
        (Vector2){(float)cJSON_GetArrayItem(unit, 0)->valuedouble, (float)cJSON_GetArrayItem(unit, 1)->valuedouble};

    cJSON *layer_json = NULL;
    cJSON_ArrayForEach(layer_json, cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "layers")) {
        if (bg->layer_count >= NEXUS_BG_MAX_LAYERS) {
            TraceLog(LOG_WARNING, "Nexus: %s has more than %d layers", prefab, NEXUS_BG_MAX_LAYERS);
            break;
        }
        if (NexusLayerLoad(bg, &bg->layers[bg->layer_count], layer_json)) bg->layer_count++;
    }
    MapSceneParticlesLoad(&bg->particles, prefab, cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "emitters"));
    cJSON_Delete(root);
    float envelope_lo = 0.0f, envelope_hi = 0.0f;
    NexusArtEnvelope(bg, &envelope_lo, &envelope_hi);
    for (int i = 0; i < bg->layer_count; i++) NexusLayerFindSeams(bg, &bg->layers[i], envelope_hi - envelope_lo);
    NexusAddMirrors(bg, envelope_hi - envelope_lo);
    TraceLog(LOG_INFO, "Nexus: %s loaded %d layers, %d emitters", prefab, bg->layer_count, bg->particles.count);
}

void NexusBackgroundUnload(NexusBackground *bg) {
    for (int i = 0; i < bg->layer_count; i++) UnloadMesh(bg->layers[i].mesh);
    MapSceneParticlesUnload(&bg->particles);
    for (int i = 0; i < bg->texture_count; i++) {
        UnloadTexture(bg->textures[i].texture);
        UnloadImage(bg->textures[i].mask);
    }
    bg->material.shader = (Shader){0};
    bg->material.maps[MATERIAL_MAP_DIFFUSE].texture = (Texture2D){0};
    UnloadMaterial(bg->material);
    UnloadShader(bg->layer_sh);
    memset(bg, 0, sizeof(*bg));
}

void NexusBackgroundUpdate(NexusBackground *bg, float dt) {
    MapSceneParticlesUpdate(&bg->particles, dt);
}

void NexusBackgroundSetScroll(NexusBackground *bg, float x) {
    bg->cam.position.x = x;
    bg->cam.target.x = x;
}

float NexusBackgroundHomeX(const NexusBackground *bg) {
    return bg->cam_home_x;
}

Vector3 NexusBackgroundNodeWorld(const NexusBackground *bg, float table_x, float table_z) {
    return Vector3Transform((Vector3){table_x, NEXUS_NODE_TABLE_Y, table_z}, bg->node_frame);
}

Vector2 NexusBackgroundNodeUnit(const NexusBackground *bg) {
    return bg->node_unit;
}

float NexusBackgroundNodePlaneY(const NexusBackground *bg) {
    return NexusBackgroundNodeWorld(bg, 0.0f, 0.0f).y;
}

Vector2 NexusBackgroundNodeScreen(const NexusBackground *bg, Vector3 world) {
    Vector2 pixels = GetWorldToScreen(world, bg->cam);
    float scale = UiScale();
    return (Vector2){pixels.x / scale, pixels.y / scale};
}

static Ray NexusCameraRay(const Camera3D *cam, float aspect, float ndc_x, float ndc_y) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(cam->target, cam->position));
    Vector3 up = Vector3Normalize(cam->up);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));
    float tan_half = tanf(cam->fovy * 0.5f * DEG2RAD);
    Vector3 direction = Vector3Add(
        forward, Vector3Add(Vector3Scale(right, ndc_x * tan_half * aspect), Vector3Scale(up, ndc_y * tan_half))
    );
    return (Ray){cam->position, Vector3Normalize(direction)};
}

static Vector2 NexusViewNdc(const Camera3D *cam, float aspect, Vector3 world) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(cam->target, cam->position));
    Vector3 up = Vector3Normalize(cam->up);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));
    Vector3 offset = Vector3Subtract(world, cam->position);
    float tan_half = tanf(cam->fovy * 0.5f * DEG2RAD);
    float depth = Vector3DotProduct(offset, forward);
    return (Vector2){
        Vector3DotProduct(offset, right) / (depth * tan_half * aspect),
        Vector3DotProduct(offset, up) / (depth * tan_half),
    };
}

static Camera3D NexusViewCamera(const NexusBackground *bg, float camera_x, float view_scale) {
    Camera3D cam = bg->cam;
    cam.position.x = camera_x;
    cam.target.x = camera_x;
    cam.fovy = 2.0f * atanf(view_scale * tanf(bg->base_fovy * 0.5f * DEG2RAD)) * RAD2DEG;
    return cam;
}

static bool NexusRayShowsArt(const NexusBackground *bg, Ray ray) {
    for (int i = 0; i < bg->layer_count; i++) {
        float distance = 0.0f;
        if (NexusLayerActive(bg, &bg->layers[i]) && NexusLayerSolidAt(bg, &bg->layers[i], ray, &distance)) return true;
    }
    return false;
}

static bool NexusEdgesShowArt(const NexusBackground *bg, const Camera3D *cam, float aspect) {
    for (int side = -1; side <= 1; side += 2) {
        for (int row = 0; row < NEXUS_COVER_ROWS; row++) {
            float ndc_y = -1.0f + (NEXUS_COVER_TOP_NDC + 1.0f) * ((float)row + 0.5f) / (float)NEXUS_COVER_ROWS;
            if (!NexusRayShowsArt(bg, NexusCameraRay(cam, aspect, side * NEXUS_COVER_EDGE_NDC, ndc_y))) return false;
        }
    }
    return true;
}

static bool NexusSeamOccluded(const NexusBackground *bg, int owner, const Camera3D *cam, Vector3 seam) {
    Vector3 offset = Vector3Subtract(seam, cam->position);
    float reach = Vector3Length(offset) - NEXUS_SEAM_OCCLUSION_GAP;
    Ray ray = {cam->position, Vector3Normalize(offset)};
    for (int i = 0; i < bg->layer_count; i++) {
        float distance = 0.0f;
        if (i == owner || !NexusLayerActive(bg, &bg->layers[i])) continue;
        if (NexusLayerSolidAt(bg, &bg->layers[i], ray, &distance) && distance < reach) return true;
    }
    return false;
}

static bool NexusSeamsHidden(const NexusBackground *bg, const Camera3D *cam, float aspect) {
    int exposed = 0;
    for (int i = 0; i < bg->layer_count; i++) {
        const NexusLayer *layer = &bg->layers[i];
        if (!NexusLayerActive(bg, layer)) continue;
        for (int s = 0; s < layer->seam_count; s++) {
            if (!NexusSeamCounts(bg, layer, layer->seam_end[s])) continue;
            Vector2 ndc = NexusViewNdc(cam, aspect, layer->seams[s]);
            if (fabsf(ndc.x) > 1.0f || ndc.y < -1.0f || ndc.y > NEXUS_COVER_TOP_NDC) continue;
            if (NexusSeamOccluded(bg, i, cam, layer->seams[s])) continue;
            if (++exposed >= NEXUS_SEAM_MIN_EXPOSED) return false;
        }
    }
    return true;
}

static bool NexusViewClean(const NexusBackground *bg, float camera_x, float view_scale, float aspect) {
    Camera3D cam = NexusViewCamera(bg, camera_x, view_scale);
    return NexusEdgesShowArt(bg, &cam, aspect) && NexusSeamsHidden(bg, &cam, aspect);
}

static float NexusRefineCleanEdge(const NexusBackground *bg, float clean, float dirty, float view_scale, float aspect) {
    for (int step = 0; step < NEXUS_COVER_REFINE_STEPS; step++) {
        float middle = (clean + dirty) * 0.5f;
        if (NexusViewClean(bg, middle, view_scale, aspect))
            clean = middle;
        else
            dirty = middle;
    }
    return clean;
}
static bool
NexusFindCleanFrom(const NexusBackground *bg, float from, float to, float view_scale, float aspect, float *out) {
    float step = to > from ? NEXUS_COVER_STEP : -NEXUS_COVER_STEP;
    int steps = (int)ceilf(fabsf(to - from) / NEXUS_COVER_STEP);
    for (int i = 0; i <= steps; i++) {
        float x = from + (float)i * step;
        if (!NexusViewClean(bg, x, view_scale, aspect)) continue;
        *out = i == 0 ? x : NexusRefineCleanEdge(bg, x, x - step, view_scale, aspect);
        return true;
    }
    return false;
}

static bool NexusCleanRange(const NexusBackground *bg, float view_scale, float aspect, float *out_lo, float *out_hi) {
    float envelope_lo = 0.0f, envelope_hi = 0.0f;
    NexusArtEnvelope(bg, &envelope_lo, &envelope_hi);
    if (NexusFindCleanFrom(bg, envelope_lo, envelope_hi, view_scale, aspect, out_lo) &&
        NexusFindCleanFrom(bg, envelope_hi, envelope_lo, view_scale, aspect, out_hi))
        return true;
    *out_lo = *out_hi = (envelope_lo + envelope_hi) * 0.5f;
    return false;
}

static void NexusTargetReach(
    const NexusBackground *bg,
    const NexusViewTarget *target,
    float view_scale,
    float aspect,
    float view_lo,
    float view_hi,
    float *out_lo,
    float *out_hi
) {
    Camera3D here = NexusViewCamera(bg, target->world.x, view_scale);
    Camera3D there = NexusViewCamera(bg, target->world.x + 1.0f, view_scale);
    float centre = (NexusViewNdc(&here, aspect, target->world).x + 1.0f) * 0.5f;
    float slope = (NexusViewNdc(&there, aspect, target->world).x + 1.0f) * 0.5f - centre;
    Vector3 side = Vector3Add(target->world, (Vector3){target->half_width, 0.0f, 0.0f});
    float margin = fabsf((NexusViewNdc(&here, aspect, side).x + 1.0f) * 0.5f - centre);
    float a = target->world.x + (view_lo + margin - centre) / slope;
    float b = target->world.x + (view_hi - margin - centre) / slope;
    *out_lo = fminf(a, b);
    *out_hi = fmaxf(a, b);
}

static bool NexusTargetsFit(
    const NexusBackground *bg,
    const NexusViewTarget *targets,
    int count,
    float view_scale,
    float aspect,
    float view_lo,
    float view_hi,
    float *out_lo,
    float *out_hi
) {
    bool fits = NexusCleanRange(bg, view_scale, aspect, out_lo, out_hi);
    for (int i = 0; i < count; i++) {
        float reach_lo = 0.0f, reach_hi = 0.0f;
        NexusTargetReach(bg, &targets[i], view_scale, aspect, view_lo, view_hi, &reach_lo, &reach_hi);
        if (*out_hi < reach_lo) {
            *out_hi = reach_lo;
            fits = false;
        }
        if (*out_lo > reach_hi) {
            *out_lo = reach_hi;
            fits = false;
        }
    }
    return fits;
}

static bool NexusFitScale(
    const NexusBackground *bg,
    const NexusViewTarget *targets,
    int count,
    float aspect,
    float view_lo,
    float view_hi,
    float *out_scale,
    float *out_lo,
    float *out_hi
) {
    *out_scale = 1.0f;
    if (NexusTargetsFit(bg, targets, count, 1.0f, aspect, view_lo, view_hi, out_lo, out_hi)) return true;
    *out_scale = NEXUS_MIN_VIEW_SCALE;
    if (!NexusTargetsFit(bg, targets, count, NEXUS_MIN_VIEW_SCALE, aspect, view_lo, view_hi, out_lo, out_hi))
        return false;
    float fits = NEXUS_MIN_VIEW_SCALE, fails = 1.0f;
    for (int step = 0; step < NEXUS_VIEW_FIT_STEPS; step++) {
        float middle = (fits + fails) * 0.5f;
        if (NexusTargetsFit(bg, targets, count, middle, aspect, view_lo, view_hi, out_lo, out_hi))
            fits = middle;
        else
            fails = middle;
    }
    *out_scale = fits;
    NexusTargetsFit(bg, targets, count, fits, aspect, view_lo, view_hi, out_lo, out_hi);
    return true;
}

void NexusBackgroundFitView(
    NexusBackground *bg, const NexusViewTarget *targets, int count, float view_lo, float view_hi
) {
    float aspect = UiWidth() / UiHeight();
    float view_scale = 1.0f, lo = 0.0f, hi = 0.0f;
    bg->mirrors_on = false;
    if (!NexusFitScale(bg, targets, count, aspect, view_lo, view_hi, &view_scale, &lo, &hi)) {
        bg->mirrors_on = true;
        if (!NexusFitScale(bg, targets, count, aspect, view_lo, view_hi, &view_scale, &lo, &hi))
            TraceLog(LOG_WARNING, "Nexus: %s cannot frame its nodes cleanly at %.2f:1", bg->prefab, aspect);
    }
    bg->cam.fovy = NexusViewCamera(bg, bg->cam.position.x, view_scale).fovy;
    bg->scroll_lo = lo;
    bg->scroll_hi = hi;
}

void NexusBackgroundScrollLimits(const NexusBackground *bg, float *out_lo, float *out_hi) {
    *out_lo = bg->scroll_lo;
    *out_hi = bg->scroll_hi;
}

static int NexusLayerQueue(const NexusLayer *layer) {
    bool opaque = layer->blend == NEXUS_BLEND_OPAQUE || layer->blend == NEXUS_BLEND_CUTOUT;
    return opaque ? NEXUS_OPAQUE_QUEUE : MAP_SCENE_TRANSPARENT_QUEUE;
}

static int NexusDrawOrderCompare(const void *a, const void *b) {
    const MapSceneDrawItem *da = a, *db = b;
    if (da->queue != db->queue || da->queue != NEXUS_OPAQUE_QUEUE) return MapSceneDrawCompare(a, b);
    return (da->distance > db->distance) - (da->distance < db->distance);
}

static void NexusLayerDraw(const NexusBackground *bg, const NexusLayer *layer) {
    Material material = bg->material;
    material.maps[MATERIAL_MAP_DIFFUSE].texture =
        NexusLayerIsFrame(layer) ? bg->white : bg->textures[layer->texture].texture;
    material.maps[MATERIAL_MAP_DIFFUSE].color = layer->color;
    float opaque = NexusLayerQueue(layer) == NEXUS_OPAQUE_QUEUE ? 1.0f : 0.0f;
    SetShaderValue(bg->layer_sh, bg->opaque_loc, &opaque, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bg->layer_sh, bg->cutoff_loc, &layer->cutoff, SHADER_UNIFORM_FLOAT);
    if (opaque > 0.0f)
        rlEnableDepthMask();
    else
        rlDisableDepthMask();
    BeginBlendMode(layer->blend == NEXUS_BLEND_ADDITIVE ? BLEND_ADDITIVE : BLEND_ALPHA);
    DrawMesh(layer->mesh, material, MatrixIdentity());
    EndBlendMode();
}

void NexusBackgroundDrawRange(const NexusBackground *bg, float min_y, float max_y) {
    MapSceneDrawItem draws[NEXUS_MAX_DRAWS];
    int count = 0;
    Vector3 eye = bg->cam.position;
    for (int i = 0; i < bg->layer_count; i++) {
        const NexusLayer *layer = &bg->layers[i];
        if (!NexusLayerActive(bg, layer) || layer->centre.y < min_y || layer->centre.y >= max_y) continue;
        MapSceneAddDraw(draws, &count, eye, MAP_SCENE_DRAW_LAYER, i, NexusLayerQueue(layer), layer->centre);
    }
    for (int i = 0; i < bg->particles.count; i++) {
        Vector3 origin = bg->particles.emitters[i].origin;
        if (origin.y < min_y || origin.y >= max_y) continue;
        MapSceneAddDraw(draws, &count, eye, MAP_SCENE_DRAW_EMITTER, i, MAP_SCENE_TRANSPARENT_QUEUE, origin);
    }
    qsort(draws, (size_t)count, sizeof(draws[0]), NexusDrawOrderCompare);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlEnableDepthTest();
    for (int i = 0; i < count; i++) {
        if (draws[i].kind == MAP_SCENE_DRAW_EMITTER)
            MapSceneParticlesDraw(&bg->particles, draws[i].index, bg->cam);
        else
            NexusLayerDraw(bg, &bg->layers[draws[i].index]);
    }
    rlEnableDepthMask();
    rlDisableDepthTest();
    rlEnableBackfaceCulling();
}

void NexusBackgroundDraw(const NexusBackground *bg) {
    NexusBackgroundDrawRange(bg, -1.0e9f, 1.0e9f);
}
