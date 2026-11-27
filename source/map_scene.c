#define MAP_SCENE_MAX_TEXTURES 16
#define MAP_SCENE_MAX_LAYERS 32
#define MAP_SCENE_MAX_WINDS 4
#define MAP_SCENE_MAX_EMITTERS 64
#define MAP_SCENE_MAX_NODES 12
#define MAP_SCENE_MAX_CURVE_KEYS 17
#define MAP_SCENE_MAX_GRADIENT_KEYS 8
#define MAP_SCENE_MAX_EMITTER_PARTICLES 512
#define MAP_SCENE_MAX_DRAWS (MAP_SCENE_MAX_LAYERS + MAP_SCENE_MAX_WINDS + MAP_SCENE_MAX_EMITTERS)
#define MAP_SCENE_TRANSPARENT_QUEUE 3000
#define MAP_SCENE_STEP (1.0f / 30.0f)
#define MAP_SCENE_MAX_FRAME_TIME 0.1f
#define MAP_SCENE_QUAD_CORNERS 4
#define MAP_SCENE_RANDOM_ROW (-1)
#define MAP_SCENE_WHOLE_SHEET (-2)
#define MAP_SCENE_RANDOM_SEED 0x9e3779b9u

static const char *const MAP_SCENE_FS_LAYER = SHADER_GLSL_HEADER
                                              "in vec2 fragTexCoord;\n"
                                              "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
                                              "uniform float opaque; uniform float cutoff;\n"
                                              "out vec4 finalColor;\n"
                                              "void main(){\n"
                                              "  vec4 c = texture(texture0, fragTexCoord)*colDiffuse;\n"
                                              "  if (c.a < cutoff) discard;\n"
                                              "  finalColor = vec4(c.rgb, mix(c.a, 1.0, opaque));\n"
                                              "}\n";

static const char *const MAP_SCENE_FS_PARTICLE =
    SHADER_GLSL_HEADER
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 tint; uniform float premultiply;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec4 c = clamp(fragColor*tint*texture(texture0, fragTexCoord), 0.0, 1.0);\n"
    "  finalColor = vec4(c.rgb*mix(1.0, c.a, premultiply), c.a);\n"
    "}\n";

static const char *const MAP_SCENE_VS_WIND =
    SHADER_GLSL_HEADER
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform float time; uniform vec4 wind; uniform float phase;\n"
    "uniform float flutter; uniform float flutterFrequency; uniform float colorScale; uniform float colorShift;\n"
    "out vec2 fragTexCoord; out vec3 fragTint;\n"
    "void main(){\n"
    "  float bending = vertexColor.a;\n"
    "  vec4 w = vec4(wind.xyz, wind.w*bending);\n"
    "  vec2 waveIn = vec2(time*flutterFrequency) + vec2(dot(vertexPosition, vec3(flutter + phase)), phase);\n"
    "  vec4 waves = abs(fract((fract(waveIn.xxyy*vec4(1.975, 0.793, 0.375, 0.193))*2.0 - 1.0) + 0.5)*2.0 - 1.0);\n"
    "  waves = waves*waves*(3.0 - 2.0*waves);\n"
    "  vec2 sum = waves.xz + waves.yw;\n"
    "  vec3 normal = normalize(vertexNormal);\n"
    "  vec3 bend = vec3(flutter*0.1*normal.x, bending*0.3, flutter*0.1*normal.z);\n"
    "  vec3 p = vertexPosition + (sum.xyx*bend + w.xyz*sum.y*bending)*w.w + bending*w.xyz;\n"
    "  fragTexCoord = vertexTexCoord;\n"
    "  fragTint = vertexColor.rgb*colorScale + colorShift;\n"
    "  gl_Position = mvp*vec4(p, 1.0);\n"
    "}\n";

static const char *const MAP_SCENE_FS_WIND = SHADER_GLSL_HEADER
                                             "in vec2 fragTexCoord; in vec3 fragTint;\n"
                                             "uniform sampler2D texture0;\n"
                                             "out vec4 finalColor;\n"
                                             "void main(){\n"
                                             "  vec4 c = texture(texture0, fragTexCoord);\n"
                                             "  finalColor = vec4(c.rgb*fragTint, c.a);\n"
                                             "}\n";

typedef enum {
    MAP_SCENE_BLEND_OPAQUE,
    MAP_SCENE_BLEND_CUTOUT,
    MAP_SCENE_BLEND_ALPHA,
    MAP_SCENE_BLEND_ADDITIVE,
    MAP_SCENE_BLEND_SOFT_ADDITIVE,
} MapSceneBlend;

typedef enum {
    MAP_SCENE_RENDER_BILLBOARD,
    MAP_SCENE_RENDER_STRETCHED,
    MAP_SCENE_RENDER_HORIZONTAL,
} MapSceneRender;

typedef enum {
    MAP_SCENE_SHAPE_POINT,
    MAP_SCENE_SHAPE_BOX,
    MAP_SCENE_SHAPE_CONE,
    MAP_SCENE_SHAPE_SPHERE,
    MAP_SCENE_SHAPE_POINTS,
} MapSceneShapeKind;

typedef enum {
    MAP_SCENE_DRAW_LAYER,
    MAP_SCENE_DRAW_WIND,
    MAP_SCENE_DRAW_EMITTER,
} MapSceneDrawKind;

typedef struct {
    int count;
    float time[MAP_SCENE_MAX_CURVE_KEYS];
    float low[MAP_SCENE_MAX_CURVE_KEYS];
    float high[MAP_SCENE_MAX_CURVE_KEYS];
} MapSceneCurve;

typedef struct {
    int color_count, alpha_count;
    float color_time[MAP_SCENE_MAX_GRADIENT_KEYS];
    Vector3 color[MAP_SCENE_MAX_GRADIENT_KEYS];
    float alpha_time[MAP_SCENE_MAX_GRADIENT_KEYS];
    float alpha[MAP_SCENE_MAX_GRADIENT_KEYS];
} MapSceneGradient;

typedef struct {
    char ref[128];
    Texture2D texture;
} MapSceneTexture;

typedef struct {
    Mesh mesh;
    int texture;
    Color color;
    MapSceneBlend blend;
    float cutoff;
    int queue;
    Vector3 centre;
} MapSceneLayer;

typedef struct {
    Mesh mesh;
    int texture;
    Matrix transform;
    Vector4 wind;
    float phase, flutter, flutter_frequency, color_scale, color_shift;
    Vector3 centre;
} MapSceneWind;

typedef struct {
    Vector3 position, velocity;
    float age, life, size, rotation;
    Vector4 color;
    float spin_random, size_random, velocity_random, frame_random, gradient_random;
    int row;
} MapSceneParticle;

typedef struct {
    int texture;
    MapSceneBlend blend;
    Vector4 tint;
    MapSceneRender render;
    Vector2 stretch;
    float max_size;
    Vector3 origin;
    Vector3 shape_axes[3];
    Vector3 axes[3];
    MapSceneShapeKind shape;
    Vector3 box;
    float angle, radius, length;
    bool random_direction, hemisphere, shell;
    float *points, *normals;
    int point_count;
    float duration;
    bool prewarm;
    float rate;
    Vector2 lifetime, speed, size, rotation;
    Vector4 colors[2];
    float gravity;
    bool has_spin, has_velocity, velocity_world, has_force, force_world, has_size_curve, has_sheet;
    bool has_gradient, has_gradient_low, has_start_gradient;
    MapSceneCurve spin, velocity[3], force[3], size_curve, frame;
    MapSceneGradient gradient, gradient_low, start_gradient;
    int tiles_x, tiles_y, row;
    float cycles;
    float time;
    float emit_accumulator;
    MapSceneParticle *particles;
    int particle_count, capacity;
} MapSceneEmitter;

typedef struct {
    MapSceneTexture entries[MAP_SCENE_MAX_TEXTURES];
    int count;
} MapSceneTextures;

typedef struct {
    char owner[64];
    Shader shader;
    int tint_loc, premultiply_loc;
    uint32_t random_state;
    MapSceneTextures textures;
    MapSceneEmitter emitters[MAP_SCENE_MAX_EMITTERS];
    int count;
} MapSceneParticles;

typedef struct {
    Vector3 position;
    Vector3 icon[MAP_SCENE_QUAD_CORNERS];
    Vector3 banner[MAP_SCENE_QUAD_CORNERS];
    Vector3 flask[MAP_SCENE_QUAD_CORNERS];
    Vector3 text;
} MapSceneNode;

typedef struct {
    MapSceneDrawKind kind;
    int index;
    int queue;
    float distance;
} MapSceneDrawItem;

typedef struct {
    bool loaded;
    char map_id[32];
    Camera3D camera;
    float time;
    Shader layer_shader, wind_shader;
    int opaque_loc, cutoff_loc;
    int wind_time_loc, wind_loc, wind_phase_loc, wind_flutter_loc, wind_frequency_loc, wind_scale_loc, wind_shift_loc;
    Material material;
    Texture2D white;
    MapSceneTextures textures;
    MapSceneLayer layers[MAP_SCENE_MAX_LAYERS];
    int layer_count;
    MapSceneWind winds[MAP_SCENE_MAX_WINDS];
    int wind_count;
    MapSceneParticles particles;
    MapSceneNode nodes[MAP_SCENE_MAX_NODES];
    int node_count;
} MapScene;

static int MapSceneClampInt(int value, int low, int high) {
    return value < low ? low : value > high ? high : value;
}

static float MapSceneRandom(MapSceneParticles *fx) {
    uint32_t x = fx->random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    fx->random_state = x;
    return (float)(x & 0x00ffffffu) / 16777216.0f;
}

static float MapSceneRandomRange(MapSceneParticles *fx, Vector2 range) {
    return range.x + (range.y - range.x) * MapSceneRandom(fx);
}

static float MapSceneJsonNumber(const cJSON *array, int index) {
    return (float)cJSON_GetArrayItem((cJSON *)array, index)->valuedouble;
}

static Vector2 MapSceneJsonVec2(const cJSON *node, const char *key) {
    const cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    if (cJSON_GetArraySize(array) < 2) return (Vector2){0.0f, 0.0f};
    return (Vector2){MapSceneJsonNumber(array, 0), MapSceneJsonNumber(array, 1)};
}

static Vector3 MapSceneJsonVec3At(const cJSON *array, int index) {
    return (Vector3){
        MapSceneJsonNumber(array, index), MapSceneJsonNumber(array, index + 1), MapSceneJsonNumber(array, index + 2)
    };
}

static Vector3 MapSceneJsonVec3(const cJSON *node, const char *key) {
    const cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    if (cJSON_GetArraySize(array) < 3) return (Vector3){0.0f, 0.0f, 0.0f};
    return MapSceneJsonVec3At(array, 0);
}

static Vector4 MapSceneJsonVec4(const cJSON *array) {
    if (cJSON_GetArraySize(array) < 4) return (Vector4){1.0f, 1.0f, 1.0f, 1.0f};
    return (Vector4){
        MapSceneJsonNumber(array, 0),
        MapSceneJsonNumber(array, 1),
        MapSceneJsonNumber(array, 2),
        MapSceneJsonNumber(array, 3),
    };
}

static float *MapSceneJsonFloats(const cJSON *node, const char *key, int *count) {
    const cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    *count = cJSON_IsArray(array) ? cJSON_GetArraySize(array) : 0;
    float *values = MemAlloc(sizeof(float) * (size_t)(*count > 0 ? *count : 1));
    int i = 0;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, array) values[i++] = (float)item->valuedouble;
    return values;
}

static void MapSceneJsonAxes(const cJSON *node, const char *key, Vector3 axes[3]) {
    const cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)node, key);
    if (cJSON_GetArraySize(array) != 9) TraceLog(LOG_FATAL, "MapScene: %s is not a 3x3 matrix", key);
    for (int column = 0; column < 3; column++) axes[column] = MapSceneJsonVec3At(array, column * 3);
}

static Matrix MapSceneJsonMatrix(const cJSON *array) {
    float v[16] = {0};
    for (int i = 0; i < 16; i++) v[i] = MapSceneJsonNumber(array, i);
    return (Matrix){
        v[0], v[4], v[8], v[12], v[1], v[5], v[9], v[13], v[2], v[6], v[10], v[14], v[3], v[7], v[11], v[15]
    };
}

static MapSceneCurve MapSceneJsonCurve(const cJSON *keys) {
    MapSceneCurve curve = {0};
    const cJSON *key = NULL;
    cJSON_ArrayForEach(key, keys) {
        if (curve.count >= MAP_SCENE_MAX_CURVE_KEYS) TraceLog(LOG_FATAL, "MapScene: curve has too many keys");
        curve.time[curve.count] = MapSceneJsonNumber(key, 0);
        curve.low[curve.count] = MapSceneJsonNumber(key, 1);
        curve.high[curve.count] = MapSceneJsonNumber(key, 2);
        curve.count++;
    }
    if (curve.count == 0) TraceLog(LOG_FATAL, "MapScene: curve has no keys");
    return curve;
}

static MapSceneGradient MapSceneJsonGradient(const cJSON *gradient) {
    MapSceneGradient g = {0};
    const cJSON *key = NULL;
    cJSON_ArrayForEach(key, cJSON_GetObjectItemCaseSensitive((cJSON *)gradient, "color")) {
        if (g.color_count >= MAP_SCENE_MAX_GRADIENT_KEYS) TraceLog(LOG_FATAL, "MapScene: gradient has too many keys");
        g.color_time[g.color_count] = MapSceneJsonNumber(key, 0);
        g.color[g.color_count++] = MapSceneJsonVec3At(key, 1);
    }
    cJSON_ArrayForEach(key, cJSON_GetObjectItemCaseSensitive((cJSON *)gradient, "alpha")) {
        if (g.alpha_count >= MAP_SCENE_MAX_GRADIENT_KEYS) TraceLog(LOG_FATAL, "MapScene: gradient has too many keys");
        g.alpha_time[g.alpha_count] = MapSceneJsonNumber(key, 0);
        g.alpha[g.alpha_count++] = MapSceneJsonNumber(key, 1);
    }
    return g;
}

static float MapSceneCurveSample(const MapSceneCurve *curve, float time, float random) {
    int last = curve->count - 1;
    int right = 0;
    while (right <= last && curve->time[right] < time) right++;
    if (right == 0 || right > last) {
        int key = right == 0 ? 0 : last;
        return Lerp(curve->low[key], curve->high[key], random);
    }
    int left = right - 1;
    float span = curve->time[right] - curve->time[left];
    float t = span > 0.0f ? (time - curve->time[left]) / span : 0.0f;
    float low = Lerp(curve->low[left], curve->low[right], t);
    float high = Lerp(curve->high[left], curve->high[right], t);
    return Lerp(low, high, random);
}

static float MapSceneKeysSample(const float *times, const float *values, int count, float time) {
    if (count == 0) return 1.0f;
    if (time <= times[0]) return values[0];
    for (int i = 1; i < count; i++) {
        if (time > times[i]) continue;
        float span = times[i] - times[i - 1];
        return Lerp(values[i - 1], values[i], span > 0.0f ? (time - times[i - 1]) / span : 0.0f);
    }
    return values[count - 1];
}

static Vector4 MapSceneGradientSample(const MapSceneGradient *g, float time) {
    Vector3 color = {1.0f, 1.0f, 1.0f};
    if (g->color_count > 0) {
        float r[MAP_SCENE_MAX_GRADIENT_KEYS], gr[MAP_SCENE_MAX_GRADIENT_KEYS], b[MAP_SCENE_MAX_GRADIENT_KEYS];
        for (int i = 0; i < g->color_count; i++) {
            r[i] = g->color[i].x;
            gr[i] = g->color[i].y;
            b[i] = g->color[i].z;
        }
        color = (Vector3){
            MapSceneKeysSample(g->color_time, r, g->color_count, time),
            MapSceneKeysSample(g->color_time, gr, g->color_count, time),
            MapSceneKeysSample(g->color_time, b, g->color_count, time),
        };
    }
    float alpha = MapSceneKeysSample(g->alpha_time, g->alpha, g->alpha_count, time);
    return (Vector4){color.x, color.y, color.z, alpha};
}

static MapSceneBlend MapSceneBlendNamed(const char *name) {
    if (strcmp(name, "opaque") == 0) return MAP_SCENE_BLEND_OPAQUE;
    if (strcmp(name, "cutout") == 0) return MAP_SCENE_BLEND_CUTOUT;
    if (strcmp(name, "additive") == 0) return MAP_SCENE_BLEND_ADDITIVE;
    if (strcmp(name, "soft_additive") == 0) return MAP_SCENE_BLEND_SOFT_ADDITIVE;
    if (strcmp(name, "alpha") != 0) TraceLog(LOG_FATAL, "MapScene: unknown blend %s", name);
    return MAP_SCENE_BLEND_ALPHA;
}

static MapSceneRender MapSceneRenderNamed(const char *name) {
    if (strcmp(name, "stretched") == 0) return MAP_SCENE_RENDER_STRETCHED;
    if (strcmp(name, "horizontal") == 0) return MAP_SCENE_RENDER_HORIZONTAL;
    if (strcmp(name, "billboard") != 0) TraceLog(LOG_FATAL, "MapScene: unknown render mode %s", name);
    return MAP_SCENE_RENDER_BILLBOARD;
}

static int MapSceneTexturesLoad(MapSceneTextures *t, const char *owner, const char *ref) {
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->entries[i].ref, ref) == 0) return i;
    if (t->count >= MAP_SCENE_MAX_TEXTURES)
        TraceLog(LOG_FATAL, "MapScene: %s has more than %d textures", owner, MAP_SCENE_MAX_TEXTURES);
    Image image = ResourceLoadImage(ref);
    if (!image.data) TraceLog(LOG_FATAL, "MapScene: %s has no texture %s", owner, ref);
    MapSceneTexture *entry = &t->entries[t->count];
    StrCopy(entry->ref, sizeof(entry->ref), ref);
    entry->texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureWrap(entry->texture, TEXTURE_WRAP_REPEAT);
    GenTextureMipmaps(&entry->texture);
    SetTextureFilter(entry->texture, TEXTURE_FILTER_TRILINEAR);
    return t->count++;
}

static void MapSceneTexturesUnload(MapSceneTextures *t) {
    for (int i = 0; i < t->count; i++) UnloadTexture(t->entries[i].texture);
    t->count = 0;
}

static int MapSceneOptionalTexture(MapScene *s, const cJSON *node) {
    const cJSON *texture = cJSON_GetObjectItemCaseSensitive((cJSON *)node, "texture");
    return cJSON_IsString(texture) ? MapSceneTexturesLoad(&s->textures, s->map_id, texture->valuestring) : -1;
}

static Mesh MapSceneMesh(const cJSON *node, bool with_normals, bool with_colors) {
    int position_count = 0, uv_count = 0, index_count = 0;
    Mesh mesh = {0};
    mesh.vertices = MapSceneJsonFloats(node, "positions", &position_count);
    mesh.texcoords = MapSceneJsonFloats(node, "uvs", &uv_count);
    float *indices = MapSceneJsonFloats(node, "indices", &index_count);
    mesh.vertexCount = position_count / 3;
    mesh.triangleCount = index_count / 3;
    if (uv_count != mesh.vertexCount * 2) TraceLog(LOG_FATAL, "MapScene: mesh uv count does not match its vertices");
    mesh.indices = MemAlloc(sizeof(unsigned short) * (size_t)(index_count > 0 ? index_count : 1));
    for (int i = 0; i < index_count; i++) mesh.indices[i] = (unsigned short)indices[i];
    MemFree(indices);
    if (with_normals) {
        int normal_count = 0;
        mesh.normals = MapSceneJsonFloats(node, "normals", &normal_count);
        if (normal_count != position_count) TraceLog(LOG_FATAL, "MapScene: mesh normals do not match its vertices");
    }
    if (with_colors) {
        int color_count = 0;
        float *colors = MapSceneJsonFloats(node, "colors", &color_count);
        if (color_count != mesh.vertexCount * 4)
            TraceLog(LOG_FATAL, "MapScene: mesh colours do not match its vertices");
        mesh.colors = MemAlloc((size_t)color_count);
        for (int i = 0; i < color_count; i++) mesh.colors[i] = (unsigned char)(Clamp(colors[i], 0.0f, 1.0f) * 255.0f);
        MemFree(colors);
    }
    UploadMesh(&mesh, false);
    return mesh;
}

static Vector3 MapSceneMeshCentre(Mesh mesh, Matrix transform) {
    BoundingBox bounds = GetMeshBoundingBox(mesh);
    return Vector3Transform(Vector3Scale(Vector3Add(bounds.min, bounds.max), 0.5f), transform);
}

static void MapSceneLoadLayer(MapScene *s, const cJSON *json) {
    if (s->layer_count >= MAP_SCENE_MAX_LAYERS)
        TraceLog(LOG_FATAL, "MapScene: %s has more than %d layers", s->map_id, MAP_SCENE_MAX_LAYERS);
    MapSceneLayer *layer = &s->layers[s->layer_count++];
    layer->texture = MapSceneOptionalTexture(s, json);
    layer->mesh = MapSceneMesh(json, false, false);
    Vector4 color = MapSceneJsonVec4(cJSON_GetObjectItemCaseSensitive((cJSON *)json, "color"));
    layer->color = ColorFromNormalized(color);
    layer->blend = MapSceneBlendNamed(JsonStr(json, "blend"));
    layer->cutoff = layer->blend == MAP_SCENE_BLEND_CUTOUT ? JsonFloat(json, "cutoff") : 0.0f;
    const cJSON *queue = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "queue");
    layer->queue = cJSON_IsNumber(queue) ? queue->valueint : MAP_SCENE_TRANSPARENT_QUEUE;
    layer->centre = MapSceneMeshCentre(layer->mesh, MatrixIdentity());
}

static void MapSceneLoadWind(MapScene *s, const cJSON *json) {
    if (s->wind_count >= MAP_SCENE_MAX_WINDS)
        TraceLog(LOG_FATAL, "MapScene: %s has more than %d wind meshes", s->map_id, MAP_SCENE_MAX_WINDS);
    MapSceneWind *wind = &s->winds[s->wind_count++];
    wind->texture = MapSceneOptionalTexture(s, json);
    wind->mesh = MapSceneMesh(json, true, true);
    wind->transform = MapSceneJsonMatrix(cJSON_GetObjectItemCaseSensitive((cJSON *)json, "matrix"));
    wind->wind = MapSceneJsonVec4(cJSON_GetObjectItemCaseSensitive((cJSON *)json, "wind"));
    wind->phase = JsonFloat(json, "phase");
    wind->flutter = JsonFloat(json, "flutter");
    wind->flutter_frequency = JsonFloat(json, "flutter_frequency");
    wind->color_scale = JsonFloat(json, "color_scale");
    wind->color_shift = JsonFloat(json, "color_shift");
    wind->centre = MapSceneMeshCentre(wind->mesh, wind->transform);
}

static void MapSceneLoadShape(MapSceneEmitter *e, const cJSON *shape) {
    const char *type = JsonStr(shape, "type");
    if (strcmp(type, "box") == 0) {
        e->shape = MAP_SCENE_SHAPE_BOX;
        e->box = MapSceneJsonVec3(shape, "size");
        e->random_direction = JsonBool(shape, "random_direction");
    } else if (strcmp(type, "cone") == 0) {
        e->shape = MAP_SCENE_SHAPE_CONE;
        e->angle = JsonFloat(shape, "angle") * DEG2RAD;
        e->radius = JsonFloat(shape, "radius");
        e->length = JsonFloat(shape, "length");
        e->random_direction = JsonBool(shape, "random_direction");
    } else if (strcmp(type, "sphere") == 0) {
        e->shape = MAP_SCENE_SHAPE_SPHERE;
        e->radius = JsonFloat(shape, "radius");
        e->hemisphere = JsonBool(shape, "hemisphere");
        e->shell = JsonBool(shape, "shell");
    } else if (strcmp(type, "points") == 0) {
        int point_count = 0, normal_count = 0;
        e->shape = MAP_SCENE_SHAPE_POINTS;
        e->points = MapSceneJsonFloats(shape, "points", &point_count);
        e->normals = MapSceneJsonFloats(shape, "normals", &normal_count);
        if (point_count == 0 || point_count != normal_count || point_count % 3 != 0)
            TraceLog(LOG_FATAL, "MapScene: emitter points do not match their normals");
        e->point_count = point_count / 3;
    } else {
        if (strcmp(type, "point") != 0) TraceLog(LOG_FATAL, "MapScene: unknown emitter shape %s", type);
        e->shape = MAP_SCENE_SHAPE_POINT;
    }
}

static bool MapSceneJsonCurves(const cJSON *json, const char *key, MapSceneCurve *curves, int count) {
    const cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)json, key);
    if (!cJSON_IsArray(array)) return false;
    if (count == 1) {
        curves[0] = MapSceneJsonCurve(array);
        return true;
    }
    if (cJSON_GetArraySize(array) != count) TraceLog(LOG_FATAL, "MapScene: %s needs %d curves", key, count);
    for (int i = 0; i < count; i++) curves[i] = MapSceneJsonCurve(cJSON_GetArrayItem((cJSON *)array, i));
    return true;
}

static bool MapSceneJsonOptionalGradient(const cJSON *json, const char *key, MapSceneGradient *gradient) {
    const cJSON *keys = cJSON_GetObjectItemCaseSensitive((cJSON *)json, key);
    if (!cJSON_IsObject(keys)) return false;
    *gradient = MapSceneJsonGradient(keys);
    return true;
}

static void MapSceneParticlesAdd(MapSceneParticles *fx, const cJSON *json) {
    if (fx->count >= MAP_SCENE_MAX_EMITTERS)
        TraceLog(LOG_FATAL, "MapScene: %s has more than %d emitters", fx->owner, MAP_SCENE_MAX_EMITTERS);
    MapSceneEmitter *e = &fx->emitters[fx->count++];
    *e = (MapSceneEmitter){0};
    e->texture = MapSceneTexturesLoad(&fx->textures, fx->owner, JsonStr(json, "texture"));
    e->blend = MapSceneBlendNamed(JsonStr(json, "blend"));
    e->tint = MapSceneJsonVec4(cJSON_GetObjectItemCaseSensitive((cJSON *)json, "tint"));
    e->render = MapSceneRenderNamed(JsonStr(json, "render"));
    e->stretch = MapSceneJsonVec2(json, "stretch");
    e->max_size = JsonFloat(json, "max_size");
    e->origin = MapSceneJsonVec3(json, "origin");
    MapSceneJsonAxes(json, "shape_axes", e->shape_axes);
    MapSceneJsonAxes(json, "axes", e->axes);
    MapSceneLoadShape(e, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "shape"));
    e->duration = JsonFloat(json, "duration");
    e->prewarm = JsonBool(json, "prewarm");
    e->rate = JsonFloat(json, "rate");
    e->lifetime = MapSceneJsonVec2(json, "lifetime");
    e->speed = MapSceneJsonVec2(json, "speed");
    e->size = MapSceneJsonVec2(json, "size");
    e->rotation = MapSceneJsonVec2(json, "rotation");
    const cJSON *colors = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "colors");
    if (cJSON_GetArraySize(colors) != 2) TraceLog(LOG_FATAL, "MapScene: emitter start colours are not a pair");
    e->colors[0] = MapSceneJsonVec4(cJSON_GetArrayItem((cJSON *)colors, 0));
    e->colors[1] = MapSceneJsonVec4(cJSON_GetArrayItem((cJSON *)colors, 1));
    e->gravity = JsonFloat(json, "gravity");
    e->has_spin = MapSceneJsonCurves(json, "spin", &e->spin, 1);
    e->has_velocity = MapSceneJsonCurves(json, "velocity", e->velocity, 3);
    e->velocity_world = JsonBool(json, "velocity_world");
    e->has_force = MapSceneJsonCurves(json, "force", e->force, 3);
    e->force_world = JsonBool(json, "force_world");
    e->has_size_curve = MapSceneJsonCurves(json, "size_curve", &e->size_curve, 1);
    e->has_gradient = MapSceneJsonOptionalGradient(json, "gradient", &e->gradient);
    e->has_gradient_low = MapSceneJsonOptionalGradient(json, "gradient_low", &e->gradient_low);
    e->has_start_gradient = MapSceneJsonOptionalGradient(json, "start_gradient", &e->start_gradient);
    const cJSON *sheet = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "sheet");
    e->has_sheet = cJSON_IsObject(sheet);
    if (e->has_sheet) {
        const cJSON *tiles = cJSON_GetObjectItemCaseSensitive((cJSON *)sheet, "tiles");
        e->tiles_x = (int)MapSceneJsonNumber(tiles, 0);
        e->tiles_y = (int)MapSceneJsonNumber(tiles, 1);
        const cJSON *row = cJSON_GetObjectItemCaseSensitive((cJSON *)sheet, "row");
        e->row = cJSON_IsNumber(row) ? row->valueint : MAP_SCENE_WHOLE_SHEET;
        e->cycles = JsonFloat(sheet, "cycles");
        MapSceneJsonCurves(sheet, "frame", &e->frame, 1);
    }
    int needed = (int)ceilf(e->rate * fmaxf(e->lifetime.x, e->lifetime.y)) + 1;
    int allowed = MapSceneClampInt(JsonInt(json, "max_particles"), 1, MAP_SCENE_MAX_EMITTER_PARTICLES);
    e->capacity = MapSceneClampInt(needed, 1, allowed);
    e->particles = MemAlloc(sizeof(MapSceneParticle) * (size_t)e->capacity);
}

static Vector3 MapSceneApplyAxes(const Vector3 axes[3], Vector3 v) {
    return Vector3Add(Vector3Add(Vector3Scale(axes[0], v.x), Vector3Scale(axes[1], v.y)), Vector3Scale(axes[2], v.z));
}

static Vector3 MapSceneRandomUnit(MapSceneParticles *fx) {
    float z = MapSceneRandom(fx) * 2.0f - 1.0f;
    float angle = MapSceneRandom(fx) * 2.0f * PI;
    float r = sqrtf(fmaxf(0.0f, 1.0f - z * z));
    return (Vector3){r * cosf(angle), r * sinf(angle), z};
}

static void
MapSceneShapeSample(MapSceneParticles *fx, const MapSceneEmitter *e, Vector3 *position, Vector3 *direction) {
    *position = (Vector3){0.0f, 0.0f, 0.0f};
    *direction = (Vector3){0.0f, 0.0f, 1.0f};
    switch (e->shape) {
        case MAP_SCENE_SHAPE_BOX:
            *position = (Vector3){
                (MapSceneRandom(fx) - 0.5f) * e->box.x,
                (MapSceneRandom(fx) - 0.5f) * e->box.y,
                (MapSceneRandom(fx) - 0.5f) * e->box.z,
            };
            if (e->random_direction) *direction = MapSceneRandomUnit(fx);
            break;
        case MAP_SCENE_SHAPE_CONE: {
            float distance = sqrtf(MapSceneRandom(fx));
            float azimuth = MapSceneRandom(fx) * 2.0f * PI;
            Vector2 spread = {cosf(azimuth) * distance, sinf(azimuth) * distance};
            float tilt = tanf(e->angle);
            *position = (Vector3){spread.x * e->radius, spread.y * e->radius, 0.0f};
            *direction = Vector3Normalize((Vector3){spread.x * tilt, spread.y * tilt, 1.0f});
            if (e->length > 0.0f)
                *position = Vector3Add(*position, Vector3Scale(*direction, e->length * MapSceneRandom(fx)));
            if (e->random_direction) *direction = MapSceneRandomUnit(fx);
            break;
        }
        case MAP_SCENE_SHAPE_SPHERE: {
            Vector3 unit = MapSceneRandomUnit(fx);
            if (e->hemisphere) unit.z = fabsf(unit.z);
            float distance = e->shell ? e->radius : e->radius * cbrtf(MapSceneRandom(fx));
            *position = Vector3Scale(unit, distance);
            *direction = unit;
            break;
        }
        case MAP_SCENE_SHAPE_POINTS: {
            int index = (int)(MapSceneRandom(fx) * (float)e->point_count) % e->point_count;
            *position = (Vector3){e->points[index * 3], e->points[index * 3 + 1], e->points[index * 3 + 2]};
            *direction = (Vector3){e->normals[index * 3], e->normals[index * 3 + 1], e->normals[index * 3 + 2]};
            break;
        }
        case MAP_SCENE_SHAPE_POINT:
            break;
    }
}

static Vector4 MapSceneStartColor(MapSceneParticles *fx, const MapSceneEmitter *e) {
    Vector4 color = Vector4Lerp(e->colors[0], e->colors[1], MapSceneRandom(fx));
    if (!e->has_start_gradient || e->duration <= 0.0f) return color;
    Vector4 loop = MapSceneGradientSample(&e->start_gradient, fmodf(e->time, e->duration) / e->duration);
    return Vector4Multiply(color, loop);
}

static void MapSceneEmit(MapSceneParticles *fx, MapSceneEmitter *e) {
    if (e->particle_count >= e->capacity) return;
    Vector3 local_position = {0}, local_direction = {0};
    MapSceneShapeSample(fx, e, &local_position, &local_direction);
    Vector3 direction = MapSceneApplyAxes(e->axes, local_direction);
    float length = Vector3Length(direction);
    if (length > 0.0f) direction = Vector3Scale(direction, 1.0f / length);
    Vector4 color = MapSceneStartColor(fx, e);
    MapSceneParticle *p = &e->particles[e->particle_count++];
    *p = (MapSceneParticle){
        .position = Vector3Add(e->origin, MapSceneApplyAxes(e->shape_axes, local_position)),
        .velocity = Vector3Scale(direction, MapSceneRandomRange(fx, e->speed)),
        .life = fmaxf(MapSceneRandomRange(fx, e->lifetime), MAP_SCENE_STEP),
        .size = MapSceneRandomRange(fx, e->size),
        .rotation = MapSceneRandomRange(fx, e->rotation),
        .color = color,
        .spin_random = MapSceneRandom(fx),
        .size_random = MapSceneRandom(fx),
        .velocity_random = MapSceneRandom(fx),
        .frame_random = MapSceneRandom(fx),
        .gradient_random = MapSceneRandom(fx),
        .row = e->row == MAP_SCENE_RANDOM_ROW ? (int)(MapSceneRandom(fx) * (float)e->tiles_y) : e->row,
    };
}

static Vector3 MapSceneCurveVector(const MapSceneCurve curves[3], float t, float random) {
    return (Vector3){
        MapSceneCurveSample(&curves[0], t, random),
        MapSceneCurveSample(&curves[1], t, random),
        MapSceneCurveSample(&curves[2], t, random),
    };
}

static Vector3 MapSceneParticleVelocity(const MapSceneEmitter *e, const MapSceneParticle *p) {
    if (!e->has_velocity) return p->velocity;
    Vector3 animated = MapSceneCurveVector(e->velocity, p->age / p->life, p->velocity_random);
    if (!e->velocity_world) animated = MapSceneApplyAxes(e->axes, animated);
    return Vector3Add(p->velocity, animated);
}

static void MapSceneEmitterStep(MapSceneParticles *fx, MapSceneEmitter *e, float dt) {
    e->time += dt;
    int kept = 0;
    for (int i = 0; i < e->particle_count; i++) {
        MapSceneParticle p = e->particles[i];
        p.age += dt;
        if (p.age >= p.life) continue;
        float t = p.age / p.life;
        p.position = Vector3Add(p.position, Vector3Scale(MapSceneParticleVelocity(e, &p), dt));
        Vector3 acceleration = {0.0f, -e->gravity, 0.0f};
        if (e->has_force) {
            Vector3 force = MapSceneCurveVector(e->force, t, p.velocity_random);
            if (!e->force_world) force = MapSceneApplyAxes(e->axes, force);
            acceleration = Vector3Add(acceleration, force);
        }
        p.velocity = Vector3Add(p.velocity, Vector3Scale(acceleration, dt));
        if (e->has_spin) p.rotation += MapSceneCurveSample(&e->spin, t, p.spin_random) * dt;
        e->particles[kept++] = p;
    }
    e->particle_count = kept;
    e->emit_accumulator += e->rate * dt;
    while (e->emit_accumulator >= 1.0f) {
        e->emit_accumulator -= 1.0f;
        MapSceneEmit(fx, e);
    }
}

static void MapSceneEmitterAdvance(MapSceneParticles *fx, MapSceneEmitter *e, float seconds) {
    while (seconds > 0.0f) {
        float step = fminf(seconds, MAP_SCENE_STEP);
        MapSceneEmitterStep(fx, e, step);
        seconds -= step;
    }
}

void MapSceneParticlesLoad(MapSceneParticles *fx, const char *owner, const cJSON *emitters) {
    memset(fx, 0, sizeof(*fx));
    StrCopy(fx->owner, sizeof(fx->owner), owner);
    fx->random_state = MAP_SCENE_RANDOM_SEED;
    fx->shader = LoadShaderFromMemory(VS_UNLIT, MAP_SCENE_FS_PARTICLE);
    fx->tint_loc = GetShaderLocation(fx->shader, "tint");
    fx->premultiply_loc = GetShaderLocation(fx->shader, "premultiply");
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, emitters) MapSceneParticlesAdd(fx, item);
    for (int i = 0; i < fx->count; i++)
        if (fx->emitters[i].prewarm) MapSceneEmitterAdvance(fx, &fx->emitters[i], fx->emitters[i].duration);
}

void MapSceneParticlesUnload(MapSceneParticles *fx) {
    for (int i = 0; i < fx->count; i++) {
        MemFree(fx->emitters[i].particles);
        MemFree(fx->emitters[i].points);
        MemFree(fx->emitters[i].normals);
    }
    MapSceneTexturesUnload(&fx->textures);
    UnloadShader(fx->shader);
    memset(fx, 0, sizeof(*fx));
}

void MapSceneParticlesUpdate(MapSceneParticles *fx, float dt) {
    dt = fminf(dt, MAP_SCENE_MAX_FRAME_TIME);
    for (int i = 0; i < fx->count; i++) MapSceneEmitterAdvance(fx, &fx->emitters[i], dt);
}

static void MapSceneLoadNode(MapScene *s, const cJSON *json) {
    if (s->node_count >= MAP_SCENE_MAX_NODES)
        TraceLog(LOG_FATAL, "MapScene: %s has more than %d nodes", s->map_id, MAP_SCENE_MAX_NODES);
    MapSceneNode *node = &s->nodes[s->node_count++];
    node->position = MapSceneJsonVec3(json, "position");
    node->text = MapSceneJsonVec3(json, "text");
    struct {
        const char *key;
        Vector3 *corners;
    } parts[] = {{"icon", node->icon}, {"banner", node->banner}, {"flask", node->flask}};
    for (int part = 0; part < 3; part++) {
        const cJSON *array = cJSON_GetObjectItemCaseSensitive((cJSON *)json, parts[part].key);
        if (cJSON_GetArraySize(array) != MAP_SCENE_QUAD_CORNERS * 3)
            TraceLog(LOG_FATAL, "MapScene: %s node %s is not four corners", s->map_id, parts[part].key);
        for (int corner = 0; corner < MAP_SCENE_QUAD_CORNERS; corner++)
            parts[part].corners[corner] = MapSceneJsonVec3At(array, corner * 3);
    }
}

static const cJSON *MapSceneJsonFor(const cJSON *root, const char *map_id) {
    const cJSON *scene = NULL;
    cJSON_ArrayForEach(scene, cJSON_GetObjectItemCaseSensitive((cJSON *)root, "scenes")) {
        if (strcmp(JsonStr(scene, "map"), map_id) == 0) return scene;
    }
    return NULL;
}

void MapSceneLoad(MapScene *s, const char *path, const char *map_id) {
    memset(s, 0, sizeof(*s));
    StrCopy(s->map_id, sizeof(s->map_id), map_id);
    s->layer_shader = LoadShaderFromMemory(VS_UNLIT, MAP_SCENE_FS_LAYER);
    s->opaque_loc = GetShaderLocation(s->layer_shader, "opaque");
    s->cutoff_loc = GetShaderLocation(s->layer_shader, "cutoff");
    s->wind_shader = LoadShaderFromMemory(MAP_SCENE_VS_WIND, MAP_SCENE_FS_WIND);
    s->wind_time_loc = GetShaderLocation(s->wind_shader, "time");
    s->wind_loc = GetShaderLocation(s->wind_shader, "wind");
    s->wind_phase_loc = GetShaderLocation(s->wind_shader, "phase");
    s->wind_flutter_loc = GetShaderLocation(s->wind_shader, "flutter");
    s->wind_frequency_loc = GetShaderLocation(s->wind_shader, "flutterFrequency");
    s->wind_scale_loc = GetShaderLocation(s->wind_shader, "colorScale");
    s->wind_shift_loc = GetShaderLocation(s->wind_shader, "colorShift");
    s->material = LoadMaterialDefault();
    s->white = s->material.maps[MATERIAL_MAP_DIFFUSE].texture;

    cJSON *root = JsonLoadFile(path);
    const cJSON *scene = MapSceneJsonFor(root, map_id);
    if (!scene) TraceLog(LOG_FATAL, "MapScene: %s is not in %s", map_id, path);
    const cJSON *camera = cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "camera");
    Vector3 position = MapSceneJsonVec3(camera, "position");
    s->camera = (Camera3D){
        .position = position,
        .target = Vector3Add(position, MapSceneJsonVec3(camera, "forward")),
        .up = MapSceneJsonVec3(camera, "up"),
        .fovy = JsonFloat(camera, "fov"),
        .projection = CAMERA_PERSPECTIVE,
    };
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "layers")) MapSceneLoadLayer(s, item);
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "winds")) MapSceneLoadWind(s, item);
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "nodes")) MapSceneLoadNode(s, item);
    MapSceneParticlesLoad(&s->particles, map_id, cJSON_GetObjectItemCaseSensitive((cJSON *)scene, "emitters"));
    cJSON_Delete(root);
    s->loaded = true;
    TraceLog(
        LOG_INFO,
        "MapScene: %s loaded %d layers, %d wind meshes, %d emitters",
        map_id,
        s->layer_count,
        s->wind_count,
        s->particles.count
    );
}

void MapSceneUnload(MapScene *s) {
    if (!s->loaded) return;
    for (int i = 0; i < s->layer_count; i++) UnloadMesh(s->layers[i].mesh);
    for (int i = 0; i < s->wind_count; i++) UnloadMesh(s->winds[i].mesh);
    MapSceneParticlesUnload(&s->particles);
    MapSceneTexturesUnload(&s->textures);
    UnloadMaterial(s->material);
    UnloadShader(s->layer_shader);
    UnloadShader(s->wind_shader);
    memset(s, 0, sizeof(*s));
}

void MapSceneUpdate(MapScene *s, float dt) {
    if (!s->loaded) return;
    s->time += fminf(dt, MAP_SCENE_MAX_FRAME_TIME);
    MapSceneParticlesUpdate(&s->particles, dt);
}

void MapSceneSetCameraX(MapScene *s, float x) {
    s->camera.position.x = x;
    s->camera.target.x = x;
}

Vector2 MapSceneProject(const MapScene *s, Vector3 world) {
    Vector2 pixels = GetWorldToScreen(world, s->camera);
    float scale = UiScale();
    return (Vector2){pixels.x / scale, pixels.y / scale};
}

static Rectangle MapSceneProjectCorners(const MapScene *s, Vector3 centre, const Vector3 corners[4]) {
    Vector2 lo = {1.0e9f, 1.0e9f}, hi = {-1.0e9f, -1.0e9f};
    for (int i = 0; i < MAP_SCENE_QUAD_CORNERS; i++) {
        Vector2 p = MapSceneProject(s, Vector3Add(centre, corners[i]));
        lo = Vector2Min(lo, p);
        hi = Vector2Max(hi, p);
    }
    return (Rectangle){lo.x, lo.y, hi.x - lo.x, hi.y - lo.y};
}

Vector2 MapSceneNodeScreen(const MapScene *s, int node) {
    return MapSceneProject(s, s->nodes[node].position);
}

Rectangle MapSceneNodeIcon(const MapScene *s, int node) {
    return MapSceneProjectCorners(s, s->nodes[node].position, s->nodes[node].icon);
}

Rectangle MapSceneNodeBanner(const MapScene *s, int node) {
    return MapSceneProjectCorners(s, s->nodes[node].position, s->nodes[node].banner);
}

Rectangle MapSceneNodeFlask(const MapScene *s, int node) {
    return MapSceneProjectCorners(s, s->nodes[node].position, s->nodes[node].flask);
}

Vector2 MapSceneNodeText(const MapScene *s, int node) {
    return MapSceneProject(s, Vector3Add(s->nodes[node].position, s->nodes[node].text));
}

float MapSceneNodeX(const MapScene *s, int node) {
    return s->nodes[node].position.x;
}

static Vector3 MapSceneCameraRight(const Camera3D *camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    return Vector3Normalize(Vector3CrossProduct(forward, camera->up));
}

static void MapSceneDrawLayer(const MapScene *s, const MapSceneLayer *layer) {
    Material material = s->material;
    material.shader = s->layer_shader;
    material.maps[MATERIAL_MAP_DIFFUSE].texture =
        layer->texture < 0 ? s->white : s->textures.entries[layer->texture].texture;
    material.maps[MATERIAL_MAP_DIFFUSE].color = layer->color;
    bool opaque = layer->blend == MAP_SCENE_BLEND_OPAQUE || layer->blend == MAP_SCENE_BLEND_CUTOUT;
    float opaque_value = opaque ? 1.0f : 0.0f;
    SetShaderValue(s->layer_shader, s->opaque_loc, &opaque_value, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s->layer_shader, s->cutoff_loc, &layer->cutoff, SHADER_UNIFORM_FLOAT);
    if (opaque)
        rlEnableDepthMask();
    else
        rlDisableDepthMask();
    BeginBlendMode(layer->blend == MAP_SCENE_BLEND_ADDITIVE ? BLEND_ADDITIVE : BLEND_ALPHA);
    DrawMesh(layer->mesh, material, MatrixIdentity());
    EndBlendMode();
}

static void MapSceneDrawWind(const MapScene *s, const MapSceneWind *wind) {
    Material material = s->material;
    material.shader = s->wind_shader;
    material.maps[MATERIAL_MAP_DIFFUSE].texture =
        wind->texture < 0 ? s->white : s->textures.entries[wind->texture].texture;
    SetShaderValue(s->wind_shader, s->wind_time_loc, &s->time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s->wind_shader, s->wind_loc, &wind->wind, SHADER_UNIFORM_VEC4);
    SetShaderValue(s->wind_shader, s->wind_phase_loc, &wind->phase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s->wind_shader, s->wind_flutter_loc, &wind->flutter, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s->wind_shader, s->wind_frequency_loc, &wind->flutter_frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s->wind_shader, s->wind_scale_loc, &wind->color_scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s->wind_shader, s->wind_shift_loc, &wind->color_shift, SHADER_UNIFORM_FLOAT);
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);
    DrawMesh(wind->mesh, material, wind->transform);
    EndBlendMode();
}

static Rectangle MapSceneParticleFrame(const MapSceneEmitter *e, const MapSceneParticle *p, float t) {
    if (!e->has_sheet || e->tiles_x <= 0 || e->tiles_y <= 0) return (Rectangle){0.0f, 0.0f, 1.0f, 1.0f};
    bool single_row = e->row != MAP_SCENE_WHOLE_SHEET;
    int frames = single_row ? e->tiles_x : e->tiles_x * e->tiles_y;
    float phase = e->cycles > 0.0f ? fmodf(t * e->cycles, 1.0f) : 0.0f;
    float value = MapSceneCurveSample(&e->frame, phase, p->frame_random);
    int frame = MapSceneClampInt((int)floorf(value * (float)frames), 0, frames - 1);
    int column = single_row ? frame : frame % e->tiles_x;
    int row = single_row ? MapSceneClampInt(p->row, 0, e->tiles_y - 1) : frame / e->tiles_x;
    float w = 1.0f / (float)e->tiles_x, h = 1.0f / (float)e->tiles_y;
    return (Rectangle){(float)column * w, (float)row * h, w, h};
}

static void MapSceneQuad(Vector3 c, Vector3 a, Vector3 b, Rectangle uv) {
    rlTexCoord2f(uv.x, uv.y + uv.height);
    rlVertex3f(c.x - a.x - b.x, c.y - a.y - b.y, c.z - a.z - b.z);
    rlTexCoord2f(uv.x + uv.width, uv.y + uv.height);
    rlVertex3f(c.x + a.x - b.x, c.y + a.y - b.y, c.z + a.z - b.z);
    rlTexCoord2f(uv.x + uv.width, uv.y);
    rlVertex3f(c.x + a.x + b.x, c.y + a.y + b.y, c.z + a.z + b.z);
    rlTexCoord2f(uv.x, uv.y);
    rlVertex3f(c.x - a.x + b.x, c.y - a.y + b.y, c.z - a.z + b.z);
}

static void MapSceneBeginParticleBlend(MapSceneBlend blend) {
    if (blend == MAP_SCENE_BLEND_SOFT_ADDITIVE) {
        rlSetBlendFactors(RL_ONE, RL_ONE_MINUS_SRC_COLOR, RL_FUNC_ADD);
        BeginBlendMode(BLEND_CUSTOM);
        return;
    }
    BeginBlendMode(blend == MAP_SCENE_BLEND_ADDITIVE ? BLEND_ADDITIVE : BLEND_ALPHA);
}

static Vector4 MapSceneOverLifeColor(const MapSceneEmitter *e, const MapSceneParticle *p, float t) {
    Vector4 over = MapSceneGradientSample(&e->gradient, t);
    if (!e->has_gradient_low) return over;
    return Vector4Lerp(MapSceneGradientSample(&e->gradient_low, t), over, p->gradient_random);
}

void MapSceneParticlesDraw(const MapSceneParticles *fx, int index, Camera3D camera) {
    const MapSceneEmitter *e = &fx->emitters[index];
    if (e->particle_count == 0) return;
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = MapSceneCameraRight(&camera);
    Vector3 up = Vector3CrossProduct(right, forward);
    float view_tangent = 2.0f * tanf(camera.fovy * 0.5f * DEG2RAD);
    float premultiply = e->blend == MAP_SCENE_BLEND_SOFT_ADDITIVE ? 1.0f : 0.0f;
    rlDrawRenderBatchActive();
    BeginShaderMode(fx->shader);
    SetShaderValue(fx->shader, fx->tint_loc, &e->tint, SHADER_UNIFORM_VEC4);
    SetShaderValue(fx->shader, fx->premultiply_loc, &premultiply, SHADER_UNIFORM_FLOAT);
    rlDisableDepthMask();
    MapSceneBeginParticleBlend(e->blend);
    rlSetTexture(fx->textures.entries[e->texture].texture.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < e->particle_count; i++) {
        const MapSceneParticle *p = &e->particles[i];
        float t = p->age / p->life;
        Vector4 color = e->has_gradient ? Vector4Multiply(p->color, MapSceneOverLifeColor(e, p, t)) : p->color;
        float size = p->size;
        if (e->has_size_curve) size *= fmaxf(0.0f, MapSceneCurveSample(&e->size_curve, t, p->size_random));
        if (e->max_size > 0.0f) {
            float distance = Vector3DotProduct(Vector3Subtract(p->position, camera.position), forward);
            size = fminf(size, e->max_size * view_tangent * distance);
        }
        if (size <= 0.0f || color.w <= 0.0f) continue;
        float half = size * 0.5f;
        float cr = cosf(p->rotation), sr = sinf(p->rotation);
        Vector3 a, b;
        if (e->render == MAP_SCENE_RENDER_HORIZONTAL) {
            a = (Vector3){cr * half, 0.0f, sr * half};
            b = (Vector3){-sr * half, 0.0f, cr * half};
        } else if (e->render == MAP_SCENE_RENDER_STRETCHED) {
            Vector3 velocity = MapSceneParticleVelocity(e, p);
            float speed = Vector3Length(velocity);
            Vector3 axis = speed > 0.0001f ? Vector3Scale(velocity, 1.0f / speed) : up;
            Vector3 side = Vector3CrossProduct(forward, axis);
            side = Vector3Length(side) > 0.0001f ? Vector3Normalize(side) : right;
            float length = size * e->stretch.x + speed * e->stretch.y;
            a = Vector3Scale(side, half);
            b = Vector3Scale(axis, length * 0.5f);
        } else {
            a = Vector3Add(Vector3Scale(right, cr * half), Vector3Scale(up, sr * half));
            b = Vector3Subtract(Vector3Scale(up, cr * half), Vector3Scale(right, sr * half));
        }
        rlColor4ub(
            (unsigned char)(Clamp(color.x, 0.0f, 1.0f) * 255.0f),
            (unsigned char)(Clamp(color.y, 0.0f, 1.0f) * 255.0f),
            (unsigned char)(Clamp(color.z, 0.0f, 1.0f) * 255.0f),
            (unsigned char)(Clamp(color.w, 0.0f, 1.0f) * 255.0f)
        );
        MapSceneQuad(p->position, a, b, MapSceneParticleFrame(e, p, t));
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    EndBlendMode();
    EndShaderMode();
}

static int MapSceneDrawCompare(const void *a, const void *b) {
    const MapSceneDrawItem *da = a, *db = b;
    if (da->queue != db->queue) return da->queue - db->queue;
    return (da->distance < db->distance) - (da->distance > db->distance);
}

static void MapSceneAddDraw(
    MapSceneDrawItem *draws, int *count, Vector3 eye, MapSceneDrawKind kind, int index, int queue, Vector3 centre
) {
    draws[(*count)++] = (MapSceneDrawItem){kind, index, queue, Vector3DistanceSqr(eye, centre)};
}

void MapSceneDraw(const MapScene *s) {
    if (!s->loaded) return;
    MapSceneDrawItem draws[MAP_SCENE_MAX_DRAWS];
    int count = 0;
    for (int i = 0; i < s->layer_count; i++) {
        const MapSceneLayer *layer = &s->layers[i];
        if (layer->blend == MAP_SCENE_BLEND_ALPHA || layer->blend == MAP_SCENE_BLEND_ADDITIVE)
            MapSceneAddDraw(draws, &count, s->camera.position, MAP_SCENE_DRAW_LAYER, i, layer->queue, layer->centre);
    }
    for (int i = 0; i < s->wind_count; i++) {
        Vector3 centre = s->winds[i].centre;
        MapSceneAddDraw(draws, &count, s->camera.position, MAP_SCENE_DRAW_WIND, i, MAP_SCENE_TRANSPARENT_QUEUE, centre);
    }
    for (int i = 0; i < s->particles.count; i++) {
        Vector3 origin = s->particles.emitters[i].origin;
        MapSceneAddDraw(
            draws, &count, s->camera.position, MAP_SCENE_DRAW_EMITTER, i, MAP_SCENE_TRANSPARENT_QUEUE, origin
        );
    }
    qsort(draws, (size_t)count, sizeof(draws[0]), MapSceneDrawCompare);

    ClearBackground(BLACK);
    BeginMode3D(s->camera);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlEnableDepthTest();
    for (int i = 0; i < s->layer_count; i++) {
        const MapSceneLayer *layer = &s->layers[i];
        if (layer->blend == MAP_SCENE_BLEND_OPAQUE || layer->blend == MAP_SCENE_BLEND_CUTOUT)
            MapSceneDrawLayer(s, layer);
    }
    for (int i = 0; i < count; i++) {
        if (draws[i].kind == MAP_SCENE_DRAW_LAYER) MapSceneDrawLayer(s, &s->layers[draws[i].index]);
        if (draws[i].kind == MAP_SCENE_DRAW_WIND) MapSceneDrawWind(s, &s->winds[draws[i].index]);
        if (draws[i].kind == MAP_SCENE_DRAW_EMITTER) MapSceneParticlesDraw(&s->particles, draws[i].index, s->camera);
    }
    rlEnableDepthMask();
    rlDisableDepthTest();
    rlEnableBackfaceCulling();
    EndMode3D();
}
