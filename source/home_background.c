#define SCENE_WIND_DISP \
    "vec3 windDisplace(vec3 P, float w){\n" \
    "  vec3 d = P - uPhaseOrigin;\n" \
    "  float wave = sin(6.2831853*uWind.w*uTime    + dot(uPhaseA.xyz, d) + uPhaseA.w);\n" \
    "  float flut = sin(6.2831853*uFlutterHz*uTime + dot(uPhaseB.xyz, d) + uPhaseB.w);\n" \
    "  return P + uWind.xyz * (wave + flut*uFlutter*0.05) * w;\n" \
    "}\n"

static const char *const VS_UNLIT =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord; out vec4 fragColor;\n"
    "void main(){ fragTexCoord=vertexTexCoord; fragColor=vertexColor;\n"
    "  gl_Position = mvp*vec4(vertexPosition,1.0);}\n";
static const char *const VS_LIT =
    "#version 330\n"
    "in vec3 vertexPosition; in vec3 vertexNormal; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal;\n"
    "void main(){ fragTexCoord=vertexTexCoord; fragColor=vertexColor;\n"
    "  fragNormal = mat3(matModel)*vertexNormal;\n"
    "  gl_Position = mvp*vec4(vertexPosition,1.0);}\n";
static const char *const FS_PLAIN =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform float brightness;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main(){ vec4 t=texture(texture0,fragTexCoord);\n"
    "  if (t.a < 0.5) discard;\n"
    "  finalColor=vec4(t.rgb*colDiffuse.rgb*brightness, t.a*colDiffuse.a);}\n";
static const char *const FS_MULTIPLY =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "out vec4 finalColor;\n"
    "void main(){ vec4 t = texture(texture0, fragTexCoord);\n"
    "  finalColor = vec4(mix(vec3(1.0), t.rgb, t.a), 1.0); }\n";
static const char *const VS_WIND =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform vec4 uWind, uPhaseA, uPhaseB; uniform vec3 uPhaseOrigin;\n"
    "uniform float uFlutter, uFlutterHz, uTime;\n"
    "out vec2 fragTexCoord; out vec4 fragColor;\n"
    SCENE_WIND_DISP
    "void main(){\n"
    "  fragTexCoord=vertexTexCoord; fragColor=vertexColor;\n"
    "  gl_Position = mvp*vec4(windDisplace(vertexPosition, vertexColor.a), 1.0);\n"
    "}\n";

static const char *const FS_HOME_MIX =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D texture1;\n"
    "uniform sampler2D texture2;\n"
    "uniform float uBright;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec3 st = texture(texture0, fragTexCoord*3.0).rgb;\n"
    "  vec3 g = texture(texture1, fragTexCoord*4.0).rgb;\n"
    "  float m = texture(texture2, fragTexCoord).r;\n"
    "  finalColor = vec4(mix(st, g, m) * (10.0*(fragColor.rgb + 0.05)) * uBright, 1.0);\n"
    "}\n";
static const char *const FS_HOME_ROCK =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D texture1;\n"
    "uniform float uBright;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  float f = clamp((1.0 - normalize(fragNormal).y - 0.4148) * 1.9724, 0.0, 1.0);\n"
    "  vec3 top = texture(texture0, fragTexCoord*1.5).rgb;\n"
    "  vec3 side = texture(texture1, fragTexCoord*1.5).rgb;\n"
    "  finalColor = vec4(mix(top, side, f) * (16.0*(fragColor.rgb - 0.02)) * uBright, 1.0);\n"
    "}\n";
static const char *const FS_HOME_DIFF =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform float uBright;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  finalColor = vec4(texture(texture0, fragTexCoord).rgb * (10.0*(fragColor.rgb + 0.1)) * uBright, 1.0);\n"
    "}\n";
static const char *const FS_HOME_WIND =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec3 uTint; uniform float uCutoff, uVcScale, uVcShift;\n"
    "out vec4 finalColor;\n"
    "void main(){ vec4 c = texture(texture0, fragTexCoord); if (c.a < uCutoff) discard;\n"
    "  finalColor = vec4(c.rgb * uTint * (fragColor.rgb*uVcScale + uVcShift), c.a);\n"
    "}\n";

typedef struct {
    float wind[4];
    float phase_a[4], phase_b[4];
    float gain, flutter, flutter_hz, cutoff, vc_scale, vc_shift;
    float tint[3];
} SceneWind;

static const SceneWind SCENE_WIND_GRASS = {
    {-0.20f, 0.0f, -0.27f, 0.30f}, {1,0,-1,0}, {1,1,-1,0},
    0.33f, 3.78f, 3.0f, 0.276f, 1.4f, 0.2f, {0.8811f, 0.9412f, 0.7543f},
};
static const SceneWind SCENE_WIND_FERN = {
    {-0.12f, 0.0f, 0.0f, 0.20f}, {1,0,-1,0}, {1,1,-1,0},
    0.33f, 3.00f, 2.0f, 0.679f, 3.0f, 0.1f, {1,1,1},
};
static const SceneWind SCENE_WIND_CLOTH1 = {
    {-0.06532f, 0.02383f, 0.0f, 1.5f},
    {8.1729f, 64.3763f, -64.8935f, 2734.061f}, {72.5492f, 56.2035f, -64.8940f, 15593.565f},
    1.0f, 65.0f, 2.0f, 0.328f, 1.0f, 0.0f, {1,1,1},
};
static const SceneWind SCENE_WIND_CLOTH2 = {
    {-0.05498f, 0.01234f, 0.0f, 1.5f},
    {-0.2119f, 80.0773f, -80.0772f, 1929.756f}, {79.8656f, 80.2891f, -80.0770f, 18532.308f},
    1.0f, 65.0f, 2.0f, 0.328f, 1.0f, 0.0f, {1,1,1},
};

typedef struct {
    const char *ply, *tex;
    bool mixer;
    const SceneWind *wind;
} ScenePiece;
static const ScenePiece SCENE_PIECES[] = {
    { "main_screen_scene_pre/ply_vertexcolor/ground.ply",              NULL,                                             true,  NULL        },
    { "main_screen_scene_pre/ply_vertexcolor/main_terrain_top_04.ply", NULL,                                             true,  NULL        },
    { "main_screen_scene_pre/ply_vertexcolor/rocks.ply",               "_shared_textures/background_layer_stone_03.png", false, NULL        },
    { "main_screen_scene_pre/ply_vertexcolor/spikes.ply",              "_shared_textures/Sticks_barricade_Tex.png",      false, NULL        },
    { "main_screen_scene_pre/ply_vertexcolor/grass_a.ply",             "_shared_textures/grass_01.png",                  false, &SCENE_WIND_GRASS },
    { "main_screen_scene_pre/ply_vertexcolor/ferns.ply",               "_shared_textures/plant_atlas_UI.png",            false, &SCENE_WIND_FERN  },
    { "main_screen_scene_pre/ply_vertexcolor/env_flag_cloth1.ply",     "_shared_textures/flag_Txture.png",               false, &SCENE_WIND_CLOTH1 },
    { "main_screen_scene_pre/ply_vertexcolor/env_flag_cloth2.ply",     "_shared_textures/flag_Txture.png",               false, &SCENE_WIND_CLOTH2 },
};
#define SCENE_PIECE_COUNT ((int)(sizeof(SCENE_PIECES)/sizeof(SCENE_PIECES[0])))

typedef struct {
    int wind, phase_a, phase_b, origin, flutter, freq, time, cutoff, vc_scale, vc_shift, tint;
} WindLocs;

static WindLocs WindLocsGet(Shader sh)
{
    return (WindLocs){
        .wind     = GetShaderLocation(sh, "uWind"),
        .phase_a  = GetShaderLocation(sh, "uPhaseA"),
        .phase_b  = GetShaderLocation(sh, "uPhaseB"),
        .origin   = GetShaderLocation(sh, "uPhaseOrigin"),
        .flutter  = GetShaderLocation(sh, "uFlutter"),
        .freq     = GetShaderLocation(sh, "uFlutterHz"),
        .time     = GetShaderLocation(sh, "uTime"),
        .cutoff   = GetShaderLocation(sh, "uCutoff"),
        .vc_scale = GetShaderLocation(sh, "uVcScale"),
        .vc_shift = GetShaderLocation(sh, "uVcShift"),
        .tint     = GetShaderLocation(sh, "uTint"),
    };
}

static void WindPhaseAt(const float phase[4], Vector3 centre, float out[4])
{
    out[0] = phase[0]; out[1] = phase[1]; out[2] = phase[2];
    out[3] = fmodf(phase[0]*centre.x + phase[1]*centre.y + phase[2]*centre.z + phase[3], 2.0f*PI);
}

static void SetWindUniforms(Shader sh, WindLocs locs, const SceneWind *w, Vector3 centre, float time)
{
    float a[4], b[4];
    WindPhaseAt(w->phase_a, centre, a);
    WindPhaseAt(w->phase_b, centre, b);
    float wind[4] = {w->wind[0]*w->gain, w->wind[1]*w->gain, w->wind[2]*w->gain, w->wind[3]};
    SetShaderValue(sh, locs.wind,     wind,         SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, locs.phase_a,  a,            SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, locs.phase_b,  b,            SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, locs.origin,   &centre,      SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, locs.flutter,  &w->flutter,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locs.freq,     &w->flutter_hz, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locs.time,     &time,        SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locs.cutoff,   &w->cutoff,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locs.vc_scale, &w->vc_scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locs.vc_shift, &w->vc_shift, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locs.tint,     w->tint,      SHADER_UNIFORM_VEC3);
}

static Mesh LoadPlyMesh(const char *path)
{
    Mesh mesh = {0};
    FILE *f = fopen(path, "r");
    if (!f) TraceLog(LOG_FATAL, "PLY: cannot open %s", path);

    char line[512];
    int vertex_count = 0, face_count = 0; bool has_normals = false;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "element vertex %d", &vertex_count) == 1) continue;
        if (sscanf(line, "element face %d", &face_count) == 1) continue;
        if (strncmp(line, "property float nx", 17) == 0) { has_normals = true; continue; }
        if (strncmp(line, "end_header", 10) == 0) break;
    }

    float         *in_pos = malloc(sizeof(float) * 3 * (size_t)vertex_count);
    float         *in_nrm = malloc(sizeof(float) * 3 * (size_t)vertex_count);
    float         *in_uv  = malloc(sizeof(float) * 2 * (size_t)vertex_count);
    unsigned char *in_col = malloc(4 * (size_t)vertex_count);
    if (!in_pos || !in_nrm || !in_uv || !in_col) TraceLog(LOG_FATAL, "PLY: out of memory for %s", path);

    for (int i = 0; i < vertex_count; i++) {
        float x=0,y=0,z=0,s=0,t=0, nx=0.0f,ny=0.0f,nz=0.0f; int r=0,g=0,b=0,a=0; int ok = 0;
        if (fgets(line, sizeof(line), f)) {
            if (has_normals) ok = (sscanf(line, "%f %f %f %d %d %d %d %f %f %f %f %f",
                                   &x,&y,&z,&r,&g,&b,&a,&nx,&ny,&nz,&s,&t) == 12);
            else      ok = (sscanf(line, "%f %f %f %d %d %d %d %f %f",
                                   &x,&y,&z,&r,&g,&b,&a,&s,&t) == 9);
        }
        if (!ok) TraceLog(LOG_FATAL, "PLY: bad vertex %d in %s", i, path);
        in_pos[i*3+0]=x; in_pos[i*3+1]=y; in_pos[i*3+2]=-z;
        in_nrm[i*3+0]=nx; in_nrm[i*3+1]=ny; in_nrm[i*3+2]=-nz;
        in_uv[i*2+0]=s; in_uv[i*2+1]=1.0f - t;
        in_col[i*4+0]=(unsigned char)r; in_col[i*4+1]=(unsigned char)g;
        in_col[i*4+2]=(unsigned char)b; in_col[i*4+3]=(unsigned char)a;
    }

    int capacity = face_count * 4, index_count = 0;
    int *indices = malloc(sizeof(int) * (size_t)capacity);
    if (!indices) TraceLog(LOG_FATAL, "PLY: out of memory for %s", path);
    for (int i = 0; i < face_count; i++) {
        if (!fgets(line, sizeof(line), f)) break;
        char *cursor = line; int corner_count = (int)strtol(cursor, &cursor, 10);
        if (corner_count < 3) continue;
        if (corner_count > 16) corner_count = 16;
        int corner[16];
        for (int k = 0; k < corner_count; k++) corner[k] = (int)strtol(cursor, &cursor, 10);
        for (int k = 1; k + 1 < corner_count; k++) {
            if (corner[0] < 0 || corner[0] >= vertex_count || corner[k] < 0 || corner[k] >= vertex_count ||
                corner[k+1] < 0 || corner[k+1] >= vertex_count) continue;
            if (index_count + 3 > capacity) {
                capacity *= 2;
                indices = realloc(indices, sizeof(int) * (size_t)capacity);
                if (!indices) TraceLog(LOG_FATAL, "PLY: out of memory for %s", path);
            }
            indices[index_count++] = corner[0]; indices[index_count++] = corner[k]; indices[index_count++] = corner[k+1];
        }
    }
    fclose(f);
    if (index_count <= 0) TraceLog(LOG_FATAL, "PLY: no faces in %s", path);

    mesh.vertexCount   = index_count;
    mesh.triangleCount = index_count / 3;
    mesh.vertices  = malloc(sizeof(float) * 3 * (size_t)index_count);
    mesh.normals   = malloc(sizeof(float) * 3 * (size_t)index_count);
    mesh.texcoords = malloc(sizeof(float) * 2 * (size_t)index_count);
    mesh.colors    = malloc(4 * (size_t)index_count);
    if (!mesh.vertices || !mesh.normals || !mesh.texcoords || !mesh.colors) TraceLog(LOG_FATAL, "PLY: out of memory for %s", path);
    for (int i = 0; i < index_count; i++) {
        int src = indices[i];
        mesh.vertices [i*3+0]=in_pos[src*3+0]; mesh.vertices [i*3+1]=in_pos[src*3+1]; mesh.vertices[i*3+2]=in_pos[src*3+2];
        mesh.normals  [i*3+0]=in_nrm[src*3+0]; mesh.normals  [i*3+1]=in_nrm[src*3+1]; mesh.normals [i*3+2]=in_nrm[src*3+2];
        mesh.texcoords[i*2+0]=in_uv[src*2+0]; mesh.texcoords[i*2+1]=in_uv[src*2+1];
        mesh.colors[i*4+0]=in_col[src*4+0]; mesh.colors[i*4+1]=in_col[src*4+1];
        mesh.colors[i*4+2]=in_col[src*4+2]; mesh.colors[i*4+3]=in_col[src*4+3];
    }
    if (!has_normals) {
        for (int tri = 0; tri + 2 < index_count; tri += 3) {
            float *v0 = &mesh.vertices[tri*3], *v1 = &mesh.vertices[(tri+1)*3], *v2 = &mesh.vertices[(tri+2)*3];
            float ux=v1[0]-v0[0], uy=v1[1]-v0[1], uz=v1[2]-v0[2], wx=v2[0]-v0[0], wy=v2[1]-v0[1], wz=v2[2]-v0[2];
            float fx=uy*wz-uz*wy, fy=uz*wx-ux*wz, fz=ux*wy-uy*wx;
            float len=sqrtf(fx*fx+fy*fy+fz*fz); if (len > 1e-8f) { fx/=len; fy/=len; fz/=len; }
            for (int q=0;q<3;q++){ mesh.normals[(tri+q)*3+0]=fx; mesh.normals[(tri+q)*3+1]=fy; mesh.normals[(tri+q)*3+2]=fz; }
        }
    }
    free(in_pos); free(in_nrm); free(in_uv); free(in_col); free(indices);
    UploadMesh(&mesh, false);
    return mesh;
}

static float frand01(void) { return (float)GetRandomValue(0, 100000) / 100000.0f; }

typedef struct { float x, z, rx, rz, rot_deg, alpha; } SceneShadowAnchor;

static const SceneShadowAnchor SCENE_SHADOW_ANCHORS[] = {
    { -196.3f, 11.5f,  2.6f, 1.6f,  0.0f, 135.0f },
    { -198.2f, 11.9f,  1.8f, 1.3f,  0.0f, 115.0f },
    { -196.8f, 17.2f,  2.5f, 1.6f,  0.0f, 120.0f },
    { -201.3f, 22.6f,  2.6f, 1.6f,  8.0f, 100.0f },
    { -200.4f, 13.6f,  1.8f, 1.3f,  0.0f,  90.0f },
    { -194.1f, 18.6f,  1.5f, 1.4f,  0.0f, 110.0f },
    { -193.4f, 18.2f,  3.0f, 2.2f,  0.0f,  82.0f },
    { -200.0f, 15.0f,  3.0f, 2.1f,  6.0f,  80.0f },
    { -204.5f, 15.3f,  3.2f, 2.2f, 12.0f,  85.0f },
};
#define SCENE_SHADOW_COUNT ((int)(sizeof(SCENE_SHADOW_ANCHORS)/sizeof(SCENE_SHADOW_ANCHORS[0])))

static Texture2D MakeShadowTexture(void)
{
    Image img = GenImageGradientRadial(128, 128, 0.40f, (Color){0,0,0,255}, (Color){0,0,0,0});
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

static Texture2D LoadSpriteX2(const char *path)
{
    Image img = LoadImage(path);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    unsigned char *px = img.data;
    for (int i = 0; i < img.width*img.height*4; i++) px[i] = px[i] > 127 ? 255 : px[i]*2;
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

static float Polyline(const float *t, const float *v, int n, float u)
{
    if (u <= t[0]) return v[0];
    for (int i = 1; i < n; i++)
        if (u < t[i]) return v[i-1] + (v[i]-v[i-1]) * ((u - t[i-1]) / (t[i] - t[i-1]));
    return v[n-1];
}

static float Hash01(unsigned int x)
{
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return (float)(x & 0xFFFFFF) / 16777216.0f;
}

typedef struct { Vector3 p, v0, lam; float life; int cell, em; } SceneDust;
#define SCENE_DUST_COUNT 30
#define SCENE_DUST_LIFE  10.0f
static const Vector3 SCENE_DUST_CENTER[2] = { {-203.2703f, 2.5745f, 16.1765f},
                                              {-195.4792f, 2.6314f, 18.5911f} };

static float DustWander(float u, float lam)
{
    static const float TMAX[5] = {0.0f, 0.384f, 0.447f, 0.485f, 0.816f};
    static const float VMAX[5] = {0.0f, -0.5833f, -0.0459f, 0.3125f, 0.9048f};
    static const float TMIN[4] = {0.011f, 0.416f, 0.514f, 0.853f};
    static const float VMIN[4] = {0.1964f, 0.631f, -0.2132f, -0.5f};
    float lo = Polyline(TMIN, VMIN, 4, u), hi = Polyline(TMAX, VMAX, 5, u);
    return lo + (hi - lo)*lam;
}
static void DustSpawn(SceneDust *d, int em)
{
    Vector3 c = SCENE_DUST_CENTER[em];
    d->p  = (Vector3){ c.x + (frand01()-0.5f)*10.3043f,
                       c.y + (frand01()-0.5f)*3.48f,
                       c.z + (frand01()-0.5f)*8.32f };
    float th = frand01()*2.0f*PI, ph = acosf(frand01()*2.0f - 1.0f);
    d->v0 = (Vector3){ 0.23f*sinf(ph)*cosf(th), 0.23f*cosf(ph), 0.23f*sinf(ph)*sinf(th) };
    d->lam  = (Vector3){ frand01(), frand01(), frand01() };
    d->cell = GetRandomValue(0, 3) * 4;
    d->em   = em;
}
static void DustInit(SceneDust *dust)
{
    for (int i = 0; i < SCENE_DUST_COUNT; i++) {
        DustSpawn(&dust[i], i < 10 ? 0 : 1);
        dust[i].life = frand01()*SCENE_DUST_LIFE;
    }
}
static void DustUpdate(SceneDust *dust, float dt)
{
    for (int i = 0; i < SCENE_DUST_COUNT; i++) {
        SceneDust *d = &dust[i];
        d->life += dt;
        if (d->life >= SCENE_DUST_LIFE) { DustSpawn(d, d->em); d->life = 0.0f; }
        float u = d->life / SCENE_DUST_LIFE;
        Vector3 v = { d->v0.x + DustWander(u, d->lam.x),
                      d->v0.y + DustWander(u, d->lam.y),
                      d->v0.z + DustWander(u, d->lam.z) };
        if (d->em == 1) {
            v.x += -1.0f*d->life; v.y += 0.3f*d->life; v.z += 0.96f*d->life;
        }
        d->p = Vector3Add(d->p, Vector3Scale(v, dt));
    }
}
static void DustDraw(Camera cam, Texture2D tex, SceneDust *dust)
{
    float cs = tex.width / 4.0f;
    for (int i = 0; i < SCENE_DUST_COUNT; i++) {
        float u = dust[i].life / SCENE_DUST_LIFE;
        float fade = u < 0.221f ? u / 0.221f : u < 0.724f ? 1.0f : u < 0.95f ? (0.95f - u) / 0.226f : 0.0f;
        Rectangle src = { (dust[i].cell%4)*cs + 0.5f, (dust[i].cell/4)*cs + 0.5f, cs - 1.0f, cs - 1.0f };
        DrawBillboardRec(cam, tex, src, dust[i].p, (Vector2){0.09f, 0.09f},
                         (Color){255,255,255,(unsigned char)(fade*255.0f)});
    }
}

static void HomeFxQuad(Vector3 c, Vector3 ax1, Vector3 ax2, Color col)
{
    rlColor4ub(col.r, col.g, col.b, col.a);
    rlTexCoord2f(0, 1); rlVertex3f(c.x - ax1.x - ax2.x, c.y - ax1.y - ax2.y, c.z - ax1.z - ax2.z);
    rlTexCoord2f(1, 1); rlVertex3f(c.x + ax1.x - ax2.x, c.y + ax1.y - ax2.y, c.z + ax1.z - ax2.z);
    rlTexCoord2f(1, 0); rlVertex3f(c.x + ax1.x + ax2.x, c.y + ax1.y + ax2.y, c.z + ax1.z + ax2.z);
    rlTexCoord2f(0, 0); rlVertex3f(c.x - ax1.x + ax2.x, c.y - ax1.y + ax2.y, c.z - ax1.z + ax2.z);
}

typedef struct { Vector3 p; float life, max, size; } SceneSmoke;
#define SCENE_SMOKE_COUNT 13
static void SmokeSpawn(SceneSmoke *smoke)
{
    smoke->max  = 18.0f;
    smoke->p    = (Vector3){ -193.02f + (frand01()-0.5f)*17.15f, 1.9f + frand01()*1.0f, 35.59f + (frand01()-0.5f)*4.57f };
    smoke->size = 7.0f + frand01()*7.0f;
}
static void SmokeInit(SceneSmoke *smokes)
{
    for (int i = 0; i < SCENE_SMOKE_COUNT; i++) { SmokeSpawn(&smokes[i]); smokes[i].life = frand01()*smokes[i].max; }
}
static void SmokeUpdate(SceneSmoke *smokes, float dt)
{
    for (int i = 0; i < SCENE_SMOKE_COUNT; i++) {
        smokes[i].life += dt;
        if (smokes[i].life >= smokes[i].max) { SmokeSpawn(&smokes[i]); smokes[i].life = 0.0f; }
        smokes[i].p.x -= 0.7f * dt;
    }
}
static void SmokeDraw(Camera cam, Texture2D tex, SceneSmoke *smokes)
{
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    for (int i = 0; i < SCENE_SMOKE_COUNT; i++) {
        float u = smokes[i].life / smokes[i].max;
        float fade = u < 0.307f ? u / 0.307f : u < 0.707f ? 1.0f : (1.0f - u) / 0.293f;
        unsigned char alpha = (unsigned char)(fade*105.0f);
        DrawBillboardRec(cam, tex, src, smokes[i].p, (Vector2){smokes[i].size, smokes[i].size}, (Color){170,166,150,alpha});
    }
}

static void DrawFarBolt(Camera cam, Texture2D tex, float t, Vector3 base, Vector2 size, int seed)
{
    float period = 6.5f;
    float cycle = (t + seed*2.3f) / period;
    int   slot  = (int)cycle;
    float phase = (cycle - (float)slot) * period;
    unsigned hash = ((unsigned)slot*2654435761u + (unsigned)seed*40503u) >> 9;
    if ((hash & 3u) != 0u) return;
    if (phase > 0.5f) return;
    float envelope = expf(-phase*6.0f) * (0.55f + 0.45f*sinf(phase*55.0f));
    int   cell = (slot + seed) & 3;
    float cell_size = tex.width / 2.0f;
    Rectangle src = { (cell%2)*cell_size, (cell/2)*cell_size, cell_size, cell_size };
    if ((slot + seed) & 1) src.width = -src.width;
    unsigned char alpha = (unsigned char)(fminf(1.0f, envelope)*205.0f);
    DrawBillboardRec(cam, tex, src, base, size, (Color){236, 240, 255, alpha});
}

static void DrawBirdFlight(Camera cam, Texture2D tex, float t, int seed)
{
    float period = 16.0f;
    float cycle = (t + seed*5.7f) / period;
    int   slot  = (int)cycle;
    float phase = (cycle - (float)slot) * period;
    unsigned hash = (unsigned)slot*2654435761u + (unsigned)seed*2246822519u;
    if (((hash >> 13) & 3u) != 0u) return;
    float flight_dur = 7.0f;
    if (phase > flight_dur) return;
    float progress = phase / flight_dur;
    float rand_y = (float)((hash >> 3)  & 255) / 255.0f;
    float rand_z = (float)((hash >> 11) & 255) / 255.0f;
    int   dir = (int)((hash >> 1) & 1u);
    float x0  = dir ? -232.0f : -188.0f, x1 = dir ? -188.0f : -232.0f;
    float x   = x0 + (x1 - x0)*progress;
    float y   = 7.5f + rand_y*8.5f + sinf(t*1.2f + seed)*0.8f;
    float z   = 40.0f + rand_z*22.0f;
    float cell_size = tex.width / 4.0f;
    int   cell = 8 + ((int)(t*8.0f + seed*3) & 7);
    Rectangle src = { (cell%4)*cell_size + 0.5f, (cell/4)*cell_size + 0.5f, cell_size - 1.0f, cell_size - 1.0f };
    if (dir) src.width = -src.width;
    float fade = fminf(1.0f, sinf(progress*PI)*2.5f);
    unsigned char alpha = (unsigned char)(fade*230.0f);
    DrawBillboardRec(cam, tex, src, (Vector3){x,y,z}, (Vector2){1.4f, 1.4f}, (Color){48,48,56,alpha});
}

typedef struct {
    Shader mix_sh, plain_sh, rock_sh, diff_sh, wind_sh, mult_sh;
    WindLocs wind_loc;

    Texture2D stone, grass, mask, rock_top;
    Mesh     meshes[SCENE_PIECE_COUNT];
    Material mats[SCENE_PIECE_COUNT];
    Vector3  centre[SCENE_PIECE_COUNT];

    Model bg_model;     Texture2D bg_tex;     Matrix bg_xform;
    Model castle_model; Texture2D castle_tex; Matrix castle_xform;

    Texture2D tex_godray, tex_bird, tex_shine, tex_fluff, tex_arc, tex_smoke, tex_banner;
    Vector3 sun_pos, ray_ground;
    SceneDust  dust[SCENE_DUST_COUNT];
    SceneSmoke smoke[SCENE_SMOKE_COUNT];
    Vector3 bolt_pos[3];

    Mesh pole_mesh; Material pole_mat;

    Texture2D shadow_tex;
    Color     shadow_tint;
    SceneShadowDecal shadow_decals[SCENE_SHADOW_COUNT];

    HomeSquad squad;
    const char *mesh_dir, *char_dir;

    Camera3D cam;
} HomeBackground;

#define SCENE_GROUND_COUNT 2
#define SCENE_TERRAIN_COUNT 3
static void HomeBackgroundTerrain(const HomeBackground *scene, Mesh *out) {
    for (int i = 0; i < SCENE_TERRAIN_COUNT; i++) out[i] = scene->meshes[i];
}

void HomeBackgroundInitialize(HomeBackground *scene, const char *mesh_dir, const char *char_dir, const Player *player)
{
    scene->mix_sh   = LoadShaderFromMemory(VS_UNLIT, FS_HOME_MIX);
    scene->rock_sh  = LoadShaderFromMemory(VS_LIT, FS_HOME_ROCK);
    scene->diff_sh  = LoadShaderFromMemory(VS_UNLIT, FS_HOME_DIFF);
    scene->wind_sh  = LoadShaderFromMemory(VS_WIND, FS_HOME_WIND);
    scene->plain_sh = LoadShaderFromMemory(VS_UNLIT, FS_PLAIN);
    scene->mult_sh  = LoadShaderFromMemory(VS_UNLIT, FS_MULTIPLY);
    scene->wind_loc = WindLocsGet(scene->wind_sh);

    float gain = EnvFloat("HOD_GAIN", 0.25f);
    Shader gain_shaders[3] = { scene->mix_sh, scene->rock_sh, scene->diff_sh };
    for (int i = 0; i < 3; i++)
        SetShaderValue(gain_shaders[i], GetShaderLocation(gain_shaders[i], "uBright"), &gain, SHADER_UNIFORM_FLOAT);
    float plain_bright = 1.0f;
    SetShaderValue(scene->plain_sh, GetShaderLocation(scene->plain_sh, "brightness"), &plain_bright, SHADER_UNIFORM_FLOAT);

    scene->stone    = LoadTiled(TextFormat("%s/_shared_textures/background_layer_stone.png", mesh_dir));
    scene->grass    = LoadTiled(TextFormat("%s/main_screen_scene_pre/background_grass_01.png", mesh_dir));
    scene->mask     = LoadTiled(TextFormat("%s/_shared_textures/background_platform_01_mask.png", mesh_dir));
    scene->rock_top = LoadTiled(TextFormat("%s/_shared_textures/background_main_stone_02.png", mesh_dir));

    for (int i = 0; i < SCENE_PIECE_COUNT; i++) {
        scene->meshes[i] = LoadPlyMesh(TextFormat("%s/%s", mesh_dir, SCENE_PIECES[i].ply));
        BoundingBox bounds = GetMeshBoundingBox(scene->meshes[i]);
        scene->centre[i] = Vector3Scale(Vector3Add(bounds.min, bounds.max), 0.5f);
        scene->mats[i]   = LoadMaterialDefault();
        if (SCENE_PIECES[i].mixer) {
            scene->mats[i].shader = scene->mix_sh;
            scene->mats[i].maps[MATERIAL_MAP_DIFFUSE].texture  = scene->stone;
            scene->mats[i].maps[MATERIAL_MAP_SPECULAR].texture = scene->grass;
            scene->mats[i].maps[MATERIAL_MAP_NORMAL].texture   = scene->mask;
        } else if (SCENE_PIECES[i].wind) {
            scene->mats[i].shader = scene->wind_sh;
            scene->mats[i].maps[MATERIAL_MAP_DIFFUSE].texture  = LoadTiled(TextFormat("%s/%s", mesh_dir, SCENE_PIECES[i].tex));
        } else if (i == 2) {
            scene->mats[i].shader = scene->rock_sh;
            scene->mats[i].maps[MATERIAL_MAP_DIFFUSE].texture  = scene->rock_top;
            scene->mats[i].maps[MATERIAL_MAP_SPECULAR].texture = LoadTiled(TextFormat("%s/%s", mesh_dir, SCENE_PIECES[i].tex));
        } else {
            scene->mats[i].shader = scene->diff_sh;
            scene->mats[i].maps[MATERIAL_MAP_DIFFUSE].texture  = LoadTiled(TextFormat("%s/%s", mesh_dir, SCENE_PIECES[i].tex));
        }
    }

    scene->bg_model = LoadModel(TextFormat("%s/main_screen_scene_pre/pPlane1.obj", mesh_dir));
    scene->bg_tex   = LoadSprite(TextFormat("%s/main_screen_scene_pre/woodfort_background.png", mesh_dir));
    if (scene->bg_model.meshCount > 0) {
        scene->bg_model.materials[0].shader = scene->plain_sh;
        scene->bg_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene->bg_tex;
    }
    scene->bg_xform = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixMultiply(
        MatrixRotateY(PI),
        MatrixScale(17.7927f, 13.4522f, 15.5323f)),
        QuaternionToMatrix((Quaternion){0.0f, 0.75386f, 0.0f, 0.65703f})),
        MatrixTranslate(-216.5435f, 21.4172f, -121.0702f)),
        MatrixScale(1.0f, 1.0f, -1.0f));

    scene->castle_model = LoadModel(TextFormat("%s/main_screen_scene_pre/pPlane1_1.obj", mesh_dir));
    scene->castle_tex   = LoadSprite(TextFormat("%s/_shared_textures/Castle3.png", mesh_dir));
    if (scene->castle_model.meshCount > 0) {
        scene->castle_model.materials[0].shader = scene->plain_sh;
        scene->castle_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene->castle_tex;
        scene->castle_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){141, 137, 131, 255};
    }
    scene->castle_xform = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixMultiply(
        MatrixRotateY(PI),
        MatrixScale(215.1939f, 152.242f, 181.0211f)),
        QuaternionToMatrix((Quaternion){0.0f, 0.0f, -0.0117f, 0.99993f})),
        MatrixTranslate(-212.6689f, 1.6166f, -61.3985f)),
        MatrixScale(1.0f, 1.0f, -1.0f));

    scene->tex_godray = LoadSpriteX2(TextFormat("%s/main_screen_scene_pre/mask_godray2_c.png", mesh_dir));
    scene->tex_bird   = LoadSprite(TextFormat("%s/main_screen_scene_pre/vfx_bird_01.png", mesh_dir));
    scene->tex_shine  = LoadSprite(TextFormat("%s/_shared_textures/shine.png", mesh_dir));
    scene->tex_fluff  = LoadSpriteX2(TextFormat("%s/main_screen_scene_pre/vfx_fire_ring_fb.png", mesh_dir));
    scene->tex_arc    = LoadSprite(TextFormat("%s/_shared_textures/vfx_lightning_2.png", mesh_dir));
    scene->tex_smoke  = LoadSprite(TextFormat("%s/main_screen_scene_pre/DustPoof.png", mesh_dir));
    scene->tex_banner = LoadSprite(TextFormat("%s/_shared_textures/flag_Txture.png", mesh_dir));

    scene->sun_pos    = (Vector3){ -202.0074f, 20.1058f, 55.7182f };
    scene->ray_ground = (Vector3){ -204.0965f, 5.8276f, 43.0511f };
    DustInit(scene->dust);
    SmokeInit(scene->smoke);
    scene->bolt_pos[0] = (Vector3){ -233.0f, 21.0f, 78.0f };
    scene->bolt_pos[1] = (Vector3){ -245.0f, 18.0f, 84.0f };
    scene->bolt_pos[2] = (Vector3){ -226.0f, 24.0f, 72.0f };

    scene->pole_mesh = LoadPlyMesh(TextFormat("%s/main_screen_scene_pre/ply_vertexcolor/env_flag_poles.ply", mesh_dir));
    scene->pole_mat = LoadMaterialDefault();
    scene->pole_mat.shader = scene->plain_sh;
    scene->pole_mat.maps[MATERIAL_MAP_DIFFUSE].texture = scene->tex_banner;
    scene->pole_mat.maps[MATERIAL_MAP_DIFFUSE].color = (Color){38, 31, 24, 255};

    scene->shadow_tex  = MakeShadowTexture();
    scene->shadow_tint = (Color){ 10, 9, 12, 255 };
    Mesh terrain_meshes[SCENE_TERRAIN_COUNT];
    HomeBackgroundTerrain(scene, terrain_meshes);
    for (int i = 0; i < SCENE_SHADOW_COUNT; i++) {
        const SceneShadowAnchor *anchor = &SCENE_SHADOW_ANCHORS[i];
        float ground_y = TerrainHeightAt(anchor->x, anchor->z, terrain_meshes, SCENE_GROUND_COUNT, 0.0f);
        scene->shadow_decals[i] = (SceneShadowDecal){
            .c = (Vector3){ anchor->x, ground_y + 0.05f, anchor->z },
            .rx = anchor->rx, .rz = anchor->rz, .rot = anchor->rot_deg*DEG2RAD,
            .a = (unsigned char)(anchor->alpha*0.45f),
        };
    }

    Quaternion cam_rot   = {0.01139f, 0.99505f, -0.01239f, -0.09789f};
    Vector3 cam_pos    = {-198.0811f, 1.8075f, -9.0828f};
    Vector3 cam_fwd    = Vector3RotateByQuaternion((Vector3){0,0,1}, cam_rot);
    Vector3 cam_target = Vector3Add(cam_pos, cam_fwd);
    cam_pos.z    = -cam_pos.z;
    cam_target.z = -cam_target.z;
    scene->cam = (Camera3D){0};
    scene->cam.position = cam_pos; scene->cam.target = cam_target; scene->cam.up = (Vector3){0,1,0};
    scene->cam.fovy = 60.0f; scene->cam.projection = CAMERA_PERSPECTIVE;

    scene->mesh_dir = mesh_dir;
    scene->char_dir = char_dir;
    HomeSquadLoad(&scene->squad, player, mesh_dir, char_dir, terrain_meshes, SCENE_TERRAIN_COUNT,
                  scene->plain_sh, scene->plain_sh, scene->mult_sh);
}

void HomeBackgroundReloadSquad(HomeBackground *scene, const Player *player)
{
    HomeSquadDestroy(&scene->squad);
    Mesh terrain_meshes[SCENE_TERRAIN_COUNT];
    HomeBackgroundTerrain(scene, terrain_meshes);
    HomeSquadLoad(&scene->squad, player, scene->mesh_dir, scene->char_dir, terrain_meshes, SCENE_TERRAIN_COUNT,
                  scene->plain_sh, scene->plain_sh, scene->mult_sh);
}

void HomeBackgroundUpdate(HomeBackground *scene, float dt)
{
    DustUpdate(scene->dust, dt);
    SmokeUpdate(scene->smoke, dt);
    HomeSquadUpdate(&scene->squad, dt);
}

void HomeBackgroundDraw(HomeBackground *scene)
{
    float tnow = (float)GetTime();
    int   n    = SCENE_PIECE_COUNT;

    ClearBackground(BLACK);
    BeginMode3D(scene->cam);
        rlDisableBackfaceCulling();
        if (scene->bg_model.meshCount > 0)
            DrawMesh(scene->bg_model.meshes[0], scene->bg_model.materials[0], scene->bg_xform);
        if (scene->castle_model.meshCount > 0)
            DrawMesh(scene->castle_model.meshes[0], scene->castle_model.materials[0], scene->castle_xform);
        rlEnableBackfaceCulling();
        rlSetCullFace(RL_CULL_FACE_FRONT);
        for (int i = 0; i < n; i++) {
            if (!SCENE_PIECES[i].mixer) continue;
            DrawMesh(scene->meshes[i], scene->mats[i], MatrixIdentity());
        }
        rlDisableBackfaceCulling();
        BeginBlendMode(BLEND_ALPHA);
        rlDisableDepthTest();
        rlDisableDepthMask();
        for (int i = 0; i < SCENE_SHADOW_COUNT; i++) DrawGroundDecal(scene->shadow_tex, scene->shadow_decals[i], scene->shadow_tint);
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        rlEnableDepthTest();
        EndBlendMode();
        rlEnableBackfaceCulling();
        rlSetCullFace(RL_CULL_FACE_FRONT);
        for (int i = 0; i < n; i++) {
            if (SCENE_PIECES[i].mixer || SCENE_PIECES[i].wind) continue;
            DrawMesh(scene->meshes[i], scene->mats[i], MatrixIdentity());
        }
        DrawMesh(scene->pole_mesh, scene->pole_mat, MatrixIdentity());
        rlDisableBackfaceCulling();
        HomeSquadDrawShadows(&scene->squad);
        HomeSquadDrawPads(&scene->squad);
        rlEnableBackfaceCulling();
        rlSetCullFace(RL_CULL_FACE_BACK);
        HomeSquadDrawUnits(&scene->squad);
        rlDisableBackfaceCulling();
        BeginBlendMode(BLEND_ALPHA);
        for (int i = 0; i < n; i++) {
            if (!SCENE_PIECES[i].wind) continue;
            SetWindUniforms(scene->wind_sh, scene->wind_loc, SCENE_PIECES[i].wind, scene->centre[i], tnow);
            DrawMesh(scene->meshes[i], scene->mats[i], MatrixIdentity());
        }
        EndBlendMode();

        rlDisableDepthMask();
        BeginBlendMode(BLEND_ALPHA);
            SmokeDraw(scene->cam, scene->tex_smoke, scene->smoke);
        EndBlendMode();
        BeginBlendMode(BLEND_ADDITIVE);
            Vector3 cam_fwd   = Vector3Normalize(Vector3Subtract(scene->cam.target, scene->cam.position));
            Vector3 cam_right = Vector3Normalize(Vector3CrossProduct(cam_fwd, scene->cam.up));
            Vector3 cam_up    = Vector3CrossProduct(cam_right, cam_fwd);
            {
                float life = fmodf(tnow, 18.0f);
                unsigned int cyc = (unsigned int)(tnow / 18.0f);
                float a = life < 5.53f ? 0.655f * (life / 5.53f)
                          : life < 12.73f ? 0.66f
                                          : 0.66f * ((18.0f - life) / 5.27f);
                float rot = Hash01(cyc)*2.0f*PI + DEG2RAD*life;
                float sz = 20.0f + 6.0f*Hash01(cyc + 77u);
                float w = life / 18.0f;
                Color col = { (unsigned char)(186 - 15*w), (unsigned char)(178 - 21*w),
                              (unsigned char)(153 - 18*w), (unsigned char)(a*255.0f) };
                float h = sz * 0.5f, cr = cosf(rot), sr = sinf(rot);
                Vector3 ax1 = Vector3Add(Vector3Scale(cam_right, h*cr), Vector3Scale(cam_up, h*sr));
                Vector3 ax2 = Vector3Add(Vector3Scale(cam_right, -h*sr), Vector3Scale(cam_up, h*cr));
                rlSetTexture(scene->tex_shine.id);
                rlBegin(RL_QUADS);
                HomeFxQuad(scene->sun_pos, ax1, ax2, col);
                rlEnd();
                rlSetTexture(0);
            }
            {
                static const float RAY_SAND_T[4]  = {0.0f, 0.209f, 0.438f, 1.0f};
                static const float RAY_SAND_A[4]  = {0.0f, 24.0f, 24.0f, 0.0f};
                static const float RAY_WHITE_T[6] = {0.0f, 0.191f, 0.271f, 0.385f, 0.482f, 1.0f};
                static const float RAY_WHITE_A[6] = {0.0f, 36.0f, 33.0f, 49.0f, 19.0f, 0.0f};
                static const float RAY_SZMAX_T[3] = {0.0f, 0.442f, 1.0f};
                static const float RAY_SZMAX_V[3] = {0.3339f, 0.5054f, 0.1688f};
                static const float RAY_SZMIN_T[2] = {0.0f, 1.0f};
                static const float RAY_SZMIN_V[2] = {0.3321f, 0.1346f};
                Vector3 to_ray = Vector3Subtract(scene->ray_ground, scene->cam.position);
                float sz_cap = Vector3DotProduct(to_ray, cam_fwd) * 1.1547f * 0.5f;
                rlSetTexture(scene->tex_godray.id);
                rlBegin(RL_QUADS);
                for (int c = 0; c < 3; c++) {
                    float tt = tnow + c*6.0f;
                    float u = fmodf(tt, 18.0f) / 18.0f;
                    unsigned int cyc = ((unsigned int)(tt / 18.0f)*3u + (unsigned int)c) * 8u;
                    float a;
                    Color col;
                    if (Hash01(cyc) < 0.5f) {
                        a   = Polyline(RAY_SAND_T, RAY_SAND_A, 4, u);
                        col = (Color){ 187, 181, 123, 0 };
                    } else {
                        a   = Polyline(RAY_WHITE_T, RAY_WHITE_A, 6, u);
                        col = WHITE;
                    }
                    col.a = (unsigned char)(a * 2.0f);
                    float smax = Polyline(RAY_SZMAX_T, RAY_SZMAX_V, 3, u);
                    float smin = Polyline(RAY_SZMIN_T, RAY_SZMIN_V, 2, u);
                    float sz = 17.36f * 3.0f * (smin + (smax - smin)*Hash01(cyc + 1u));
                    if (sz > sz_cap) sz = sz_cap;
                    float rot = DEG2RAD * (4.04f + (2.0f + 1.1f*Hash01(cyc + 2u)) * u * 18.0f);
                    float t_live = u * 18.0f;
                    Vector3 p = scene->ray_ground;
                    p.x += (Hash01(cyc + 3u) - 0.5f) * 0.05f * t_live * t_live;
                    p.z += (Hash01(cyc + 4u) - 0.5f) * 0.05f * t_live * t_live;
                    float h = sz * 0.5f, cr = cosf(rot), sr = sinf(rot);
                    Vector3 ax1 = Vector3Add(Vector3Scale(cam_right, h*cr), Vector3Scale(cam_up, h*sr));
                    Vector3 ax2 = Vector3Add(Vector3Scale(cam_right, -h*sr), Vector3Scale(cam_up, h*cr));
                    HomeFxQuad(p, ax1, ax2, col);
                }
                rlEnd();
                rlSetTexture(0);
            }
            DustDraw(scene->cam, scene->tex_fluff, scene->dust);
            for (int i = 0; i < 3; i++)
                DrawFarBolt(scene->cam, scene->tex_arc, tnow, scene->bolt_pos[i], (Vector2){13.0f, 19.0f}, i);
        EndBlendMode();
        BeginBlendMode(BLEND_ALPHA);
            for (int i = 0; i < 3; i++) DrawBirdFlight(scene->cam, scene->tex_bird, tnow, i);
        EndBlendMode();
    EndMode3D();
    rlEnableDepthMask();
    rlSetCullFace(RL_CULL_FACE_BACK);
    rlEnableBackfaceCulling();
}

void HomeBackgroundDestroy(HomeBackground *scene)
{
    HomeSquadDestroy(&scene->squad);
    SceneTexUnload(scene->stone); SceneTexUnload(scene->grass); SceneTexUnload(scene->mask);
    for (int i = 0; i < SCENE_PIECE_COUNT; i++) {
        if (!SCENE_PIECES[i].mixer) SceneTexUnload(scene->mats[i].maps[MATERIAL_MAP_DIFFUSE].texture);
        if (i == 2) SceneTexUnload(scene->mats[i].maps[MATERIAL_MAP_SPECULAR].texture);
        SceneMaterialUnload(scene->mats[i]);
        UnloadMesh(scene->meshes[i]);
    }
    UnloadMesh(scene->pole_mesh);
    SceneMaterialUnload(scene->pole_mat);
    UnloadTexture(scene->shadow_tex);
    UnloadTexture(scene->tex_godray); SceneTexUnload(scene->tex_bird); SceneTexUnload(scene->tex_shine);
    UnloadTexture(scene->tex_fluff); SceneTexUnload(scene->tex_arc); SceneTexUnload(scene->tex_smoke);
    SceneTexUnload(scene->tex_banner);
    SceneTexUnload(scene->bg_tex);
    UnloadModel(scene->bg_model);
    SceneTexUnload(scene->castle_tex);
    UnloadModel(scene->castle_model);
    UnloadShader(scene->mix_sh); UnloadShader(scene->plain_sh); UnloadShader(scene->rock_sh);
    UnloadShader(scene->diff_sh); UnloadShader(scene->wind_sh); UnloadShader(scene->mult_sh);
}
