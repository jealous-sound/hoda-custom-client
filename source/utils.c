static bool EnvFlag(const char *name) {
    return getenv(name) != NULL;
}

static int EnvInt(const char *name, int fallback) {
    const char *value = getenv(name);
    return value ? atoi(value) : fallback;
}

static float EnvFloat(const char *name, float fallback) {
    const char *value = getenv(name);
    return value ? (float)atof(value) : fallback;
}

static bool FlycamEnabled(void) {
    static int cached = -1;
    if (cached < 0) cached = EnvFlag("HOD_FLYCAM");
    return cached == 1;
}

static void StrCopy(char *dst, int cap, const char *src) {
    if (!src) src = "";
    int n = (int)strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static const char *JsonStr(const cJSON *obj, const char *key) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    return (item && cJSON_IsString(item)) ? item->valuestring : "";
}

static int JsonInt(const cJSON *obj, const char *key) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    return (item && cJSON_IsNumber(item)) ? item->valueint : 0;
}

static float JsonFloat(const cJSON *obj, const char *key) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    return (item && cJSON_IsNumber(item)) ? (float)item->valuedouble : 0.0f;
}

static bool JsonBool(const cJSON *obj, const char *key) {
    return cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key));
}

static cJSON *JsonLoadFile(const char *path) {
    char *text = LoadFileText(path);
    if (!text) TraceLog(LOG_FATAL, "JSON: could not read %s", path);
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    if (!root) TraceLog(LOG_FATAL, "JSON: failed to parse %s", path);
    return root;
}

typedef struct {
    char path[256];
    bool tiled;
    int refs;
    Texture2D tex;
} SceneTexEntry;
#define SCENE_TEX_CACHE_MAX 64
static SceneTexEntry scene_tex_cache[SCENE_TEX_CACHE_MAX];

static Texture2D SceneTexLoad(const char *path, bool tiled) {
    for (int i = 0; i < SCENE_TEX_CACHE_MAX; i++) {
        SceneTexEntry *e = &scene_tex_cache[i];
        if (e->refs > 0 && e->tiled == tiled && strcmp(e->path, path) == 0) {
            e->refs++;
            return e->tex;
        }
    }
    Texture2D tex = LoadTexture(path);
    if (tex.id == 0) return tex;
    if (tiled) {
        SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
        GenTextureMipmaps(&tex);
        SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    } else {
        SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    }
    for (int i = 0; i < SCENE_TEX_CACHE_MAX; i++) {
        SceneTexEntry *e = &scene_tex_cache[i];
        if (e->refs == 0) {
            snprintf(e->path, sizeof(e->path), "%s", path);
            e->tiled = tiled;
            e->refs = 1;
            e->tex = tex;
            return tex;
        }
    }
    TraceLog(LOG_WARNING, "SceneTexLoad: cache full, %s not shared", path);
    return tex;
}

static void SceneTexUnload(Texture2D tex) {
    if (tex.id == 0) return;
    for (int i = 0; i < SCENE_TEX_CACHE_MAX; i++) {
        SceneTexEntry *e = &scene_tex_cache[i];
        if (e->refs > 0 && e->tex.id == tex.id) {
            if (--e->refs == 0) UnloadTexture(e->tex);
            return;
        }
    }
    UnloadTexture(tex);
}

static Texture2D LoadTiled(const char *path) {
    return SceneTexLoad(path, true);
}
static Texture2D LoadSprite(const char *path) {
    return SceneTexLoad(path, false);
}

#ifndef MAX_MATERIAL_MAPS
#define MAX_MATERIAL_MAPS 12
#endif

static void SceneMaterialUnload(Material mat) {
    mat.shader = (Shader){0};
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++) mat.maps[i].texture = (Texture2D){0};
    UnloadMaterial(mat);
}

static Mesh MeshSplitHalf(const Mesh *src, int axis, float sign, float *inner_edge) {
    Mesh half = {0};
    if (inner_edge) *inner_edge = 0.0f;
    if (!src || src->triangleCount <= 0 || !src->vertices) return half;

    float *vertices = MemAlloc((unsigned int)(src->triangleCount * 9 * (int)sizeof(float)));
    float *texcoords = MemAlloc((unsigned int)(src->triangleCount * 6 * (int)sizeof(float)));
    if (!vertices || !texcoords) {
        MemFree(vertices);
        MemFree(texcoords);
        return half;
    }

    int kept = 0;
    float inner = 0.0f;
    for (int triangle = 0; triangle < src->triangleCount; triangle++) {
        int index[3];
        for (int corner = 0; corner < 3; corner++)
            index[corner] = src->indices ? (int)src->indices[triangle * 3 + corner] : triangle * 3 + corner;
        float centre = (src->vertices[index[0] * 3 + axis] + src->vertices[index[1] * 3 + axis] +
                        src->vertices[index[2] * 3 + axis]) /
                       3.0f;
        if (centre * sign < 0.0f) continue;
        for (int corner = 0; corner < 3; corner++) {
            for (int component = 0; component < 3; component++)
                vertices[kept * 9 + corner * 3 + component] = src->vertices[index[corner] * 3 + component];
            texcoords[kept * 6 + corner * 2 + 0] = src->texcoords ? src->texcoords[index[corner] * 2 + 0] : 0.0f;
            texcoords[kept * 6 + corner * 2 + 1] = src->texcoords ? src->texcoords[index[corner] * 2 + 1] : 0.0f;
            float coordinate = src->vertices[index[corner] * 3 + axis];
            if (kept == 0 || coordinate * sign < inner * sign) inner = coordinate;
        }
        kept++;
    }
    if (kept == 0) {
        MemFree(vertices);
        MemFree(texcoords);
        return half;
    }

    half.triangleCount = kept;
    half.vertexCount = kept * 3;
    half.vertices = vertices;
    half.texcoords = texcoords;
    UploadMesh(&half, false);
    if (inner_edge) *inner_edge = inner;
    return half;
}

typedef struct {
    Vector3 c;
    float rx, rz, rot;
    unsigned char a;
} SceneShadowDecal;

static float TerrainHeightAt(float x, float z, const Mesh *terrain, int terrain_count, float fallback) {
    Ray ray = {(Vector3){x, 100.0f, z}, (Vector3){0.0f, -1.0f, 0.0f}};
    float best_y = fallback;
    bool hit = false;
    for (int i = 0; i < terrain_count; i++) {
        RayCollision hit_info = GetRayCollisionMesh(ray, terrain[i], MatrixIdentity());
        if (hit_info.hit && (!hit || hit_info.point.y > best_y)) {
            best_y = hit_info.point.y;
            hit = true;
        }
    }
    return best_y;
}

static void DrawGroundDecal(Texture2D tex, SceneShadowDecal decal, Color tint) {
    float cos_r = cosf(decal.rot), sin_r = sinf(decal.rot);
    Vector3 axis_x = {cos_r * decal.rx, 0.0f, sin_r * decal.rx};
    Vector3 axis_z = {-sin_r * decal.rz, 0.0f, cos_r * decal.rz};
    Vector3 p0 = {decal.c.x - axis_x.x - axis_z.x, decal.c.y, decal.c.z - axis_x.z - axis_z.z};
    Vector3 p1 = {decal.c.x + axis_x.x - axis_z.x, decal.c.y, decal.c.z + axis_x.z - axis_z.z};
    Vector3 p2 = {decal.c.x + axis_x.x + axis_z.x, decal.c.y, decal.c.z + axis_x.z + axis_z.z};
    Vector3 p3 = {decal.c.x - axis_x.x + axis_z.x, decal.c.y, decal.c.z - axis_x.z + axis_z.z};
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, decal.a);
    rlTexCoord2f(0, 0);
    rlVertex3f(p0.x, p0.y, p0.z);
    rlTexCoord2f(1, 0);
    rlVertex3f(p1.x, p1.y, p1.z);
    rlTexCoord2f(1, 1);
    rlVertex3f(p2.x, p2.y, p2.z);
    rlTexCoord2f(0, 1);
    rlVertex3f(p3.x, p3.y, p3.z);
    rlEnd();
    rlSetTexture(0);
}
