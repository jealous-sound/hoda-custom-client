
typedef enum { BK_LIGHTMAP, BK_GROUND, BK_CUTOUT, BK_BACKDROP, BK_GRASS, BK_FOG } BattleKind;

#define BATTLE_MAX_MESH 128

static const char *const BATTLE_VS =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec2 vertexTexCoord2;\n"
    "uniform mat4 mvp;\n"
    "out vec2 vUV0; out vec2 vUV1;\n"
    "void main(){ vUV0=vertexTexCoord; vUV1=vertexTexCoord2; gl_Position=mvp*vec4(vertexPosition,1.0); }\n";

static const char *const BATTLE_FS_LM = "#version 330\n"
                                        "in vec2 vUV0; in vec2 vUV1;\n"
                                        "uniform sampler2D texture0;\n"
                                        "uniform sampler2D texture1;\n"
                                        "uniform vec3 uTint;\n"
                                        "uniform float uCutoff;\n"
                                        "out vec4 finalColor;\n"
                                        "void main(){\n"
                                        "  vec4 a=texture(texture0,vUV0);\n"
                                        "  if (a.a < uCutoff) discard;\n"
                                        "  vec3 lm=texture(texture1,vUV1).rgb*2.0;\n"
                                        "  finalColor=vec4(a.rgb*uTint*lm, 1.0);\n"
                                        "}\n";

static const char *const BATTLE_FS_GROUND = "#version 330\n"
                                            "in vec2 vUV0; in vec2 vUV1;\n"
                                            "uniform sampler2D texture0;\n"
                                            "uniform sampler2D texture1;\n"
                                            "uniform sampler2D texture2;\n"
                                            "uniform sampler2D texture3;\n"
                                            "uniform sampler2D texture4;\n"
                                            "uniform sampler2D texture5;\n"
                                            "out vec4 finalColor;\n"
                                            "void main(){\n"
                                            "  vec3 m=texture(texture5,vUV0).rgb;\n"
                                            "  vec3 c=texture(texture0,vUV0*50.0).rgb;\n"
                                            "  c=mix(c,texture(texture2,vUV0*45.0).rgb,m.r);\n"
                                            "  c=mix(c,texture(texture3,vUV0*5.0).rgb,m.g);\n"
                                            "  c=mix(c,texture(texture4,vUV0*10.0).rgb,m.b);\n"
                                            "  vec3 lm=texture(texture1,vUV1).rgb*2.0;\n"
                                            "  finalColor=vec4(0.7015*c*lm, 1.0);\n"
                                            "}\n";

static const char *const BATTLE_FS_UNLIT = "#version 330\n"
                                           "in vec2 vUV0; in vec2 vUV1;\n"
                                           "uniform sampler2D texture0;\n"
                                           "out vec4 finalColor;\n"
                                           "void main(){ finalColor=texture(texture0,vUV0); }\n";

static const char *const BATTLE_FS_RING =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform float uCutoff; uniform vec4 uColor;\n"
    "out vec4 finalColor;\n"
    "void main(){ vec4 t=texture(texture0,fragTexCoord);\n"
    "  if (t.a < 0.5 || t.r < uCutoff) discard;\n"
    "  finalColor = vec4(uColor.rgb, 1.0); }\n";

static const char *const BATTLE_FS_FOG = "#version 330\n"
                                         "in vec2 vUV0; in vec2 vUV1;\n"
                                         "uniform sampler2D texture0;\n"
                                         "uniform vec3 uTint;\n"
                                         "out vec4 finalColor;\n"
                                         "void main(){ vec4 a=texture(texture0,vUV0);\n"
                                         "  finalColor=vec4(2.0*uTint*a.rgb, min(2.0*a.a,1.0)); }\n";

typedef struct {
    Mesh mesh;
    BattleKind kind;
    Texture2D albedo;
    float tint, cutoff;
} BattleMesh;

#define BATTLE_RAIN_MAX 256
#define BATTLE_SPLASH_MAX 13
#define BATTLE_FIRE_EMITTERS 18
#define BATTLE_FIRE_PMAX 16

typedef struct {
    Vector3 pos, vel;
    float age, life, size, rot, rotv;
} BattleParticle;

typedef enum { BFIRE_TONGUES, BFIRE_SPARKS, BFIRE_SMOKE } BattleFireType;

typedef struct {
    Vector3 pos;
    BattleFireType type;
    BattleParticle p[BATTLE_FIRE_PMAX];
    int alive;
    float accum;
} BattleFireEmitter;

static const struct {
    float x, y, z;
    int type;
} BATTLE_FIRE_SPOTS[BATTLE_FIRE_EMITTERS] = {
    {20.6408f, -1.5766f, -51.1427f, BFIRE_TONGUES},
    {21.6739f, -0.7041f, -52.0071f, BFIRE_TONGUES},
    {19.9278f, -0.9578f, -52.3581f, BFIRE_TONGUES},
    {19.9279f, -1.3166f, -52.3580f, BFIRE_SPARKS},
    {21.6740f, -1.0628f, -52.0071f, BFIRE_SPARKS},
    {20.6410f, -1.9354f, -51.1427f, BFIRE_SPARKS},
    {20.6408f, -1.5766f, -51.1427f, BFIRE_SMOKE},
    {19.9278f, -0.5767f, -52.3581f, BFIRE_SMOKE},
    {21.6739f, -0.1177f, -52.0071f, BFIRE_SMOKE},
    {-36.0001f, -2.9222f, -74.7338f, BFIRE_TONGUES},
    {-31.0171f, -2.9009f, -78.2383f, BFIRE_TONGUES},
    {-32.7632f, -3.1547f, -78.5893f, BFIRE_TONGUES},
    {-34.9669f, -2.4084f, -75.5982f, BFIRE_SPARKS},
    {-32.7632f, -3.5134f, -78.5892f, BFIRE_SPARKS},
    {-32.0500f, -4.1322f, -77.3739f, BFIRE_SPARKS},
    {-36.7132f, -2.3034f, -75.9493f, BFIRE_SMOKE},
    {-31.0171f, -2.9009f, -78.2383f, BFIRE_SMOKE},
    {-32.0502f, -3.7735f, -77.3738f, BFIRE_SMOKE},
};

static float BattleFrand(float lo, float hi) {
    return lo + (hi - lo) * ((float)GetRandomValue(0, 32767) / 32767.0f);
}

static float BattleHermite(float p0, float m0, float p1, float m1, float t) {
    float t2 = t * t, t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * p0 + (t3 - 2 * t2 + t) * m0 + (-2 * t3 + 3 * t2) * p1 + (t3 - t2) * m1;
}

static float BattleEnvelope(const float keys[][2], int n, float t) {
    if (t <= keys[0][0]) return keys[0][1];
    for (int i = 1; i < n; i++)
        if (t <= keys[i][0]) {
            float span = keys[i][0] - keys[i - 1][0];
            if (span <= 0.0001f) return keys[i][1];
            float f = (t - keys[i - 1][0]) / span;
            return keys[i - 1][1] + (keys[i][1] - keys[i - 1][1]) * f;
        }
    return keys[n - 1][1];
}

#define FX_MAX_PARTICLES 1024
#define FX_MAX_INSTANCES 96
#define FX_MAX_EMITTERS 24
#define FX_MAX_CURVE_KEYS 16

typedef enum {
    FX_AURA_GENERIC,
    FX_AURA_POWER,
    FX_AURA_INITIATIVE,
    FX_AURA_STUN,
    FX_AURA_CURSE_POWER,
    FX_AURA_CURSE_HEALTH,
    FX_AURA_TARGET,
    FX_AURA_SLOT_COUNT
} FxAuraSlot;

typedef struct {
    unsigned char count;
    float key[FX_MAX_CURVE_KEYS][2];
} FxCurve;

typedef struct {
    float time;
    Color color;
} FxColorKey;

typedef struct {
    unsigned char count;
    FxColorKey key[FX_MAX_CURVE_KEYS];
} FxColorCurve;

typedef struct {
    const char *tex;
    unsigned char additive;
    unsigned char keyout;
    unsigned char orient;
    unsigned char cols, rows;
    unsigned char sheet_once;
    float sheet_cycles;
    float max_screen_size;
    float render_scale;
    float stretch_length;
    float stretch_velocity;
    unsigned char one_shot;
    unsigned char prewarm;
    unsigned char birth_emitter[2];
    unsigned short max_particles;
    int count;
    float rate;
    FxCurve rate_curve;
    float delay, duration, period;
    float life0, life1;
    float speed0, speed1;
    Vector3 dir;
    float cone_deg;
    float radius;
    Vector3 box;
    float drag;
    float gravity;
    float size0, size1;
    float size_end;
    FxCurve size_curve;
    float alpha_in, alpha_hold;
    FxCurve alpha_curve;
    Color tint;
    FxColorCurve color_curve;
    float rot_deg, spin_deg;
    float rot_bias_deg, spin_bias_deg;
    FxCurve spin_curve;
    Vector3 offset;
} FxEmitter;

typedef struct {
    const char *name;
    float length;
    int em_count;
    FxEmitter em[FX_MAX_EMITTERS];
} FxPreset;

typedef struct {
    const FxEmitter *em;
    const FxPreset *pre;
    Texture2D tex;
    Vector3 pos, vel;
    Vector3 anchor_pos;
    float age, life, size, rot, spin;
    int anchor;
    FxAuraSlot aura_slot;
    unsigned serial;
    unsigned seed;
    Color start_color;
} FxParticle;

typedef struct {
    const FxPreset *pre;
    bool alive, loop;
    bool directed;
    int anchor;
    FxAuraSlot aura_slot;
    unsigned serial;
    Color start_color;
    Vector3 pos, offset;
    Quaternion rotation;
    float age;
    float accum[FX_MAX_EMITTERS];
    bool burst_done[FX_MAX_EMITTERS];
    unsigned burst_cycle[FX_MAX_EMITTERS];
    bool prewarm_done[FX_MAX_EMITTERS];
} FxInstance;

typedef struct {
    char unit[64];
    Tier tier;
    int vfx_type;
    int ability_type;
    float pspeed;
    float cone_delay;
    char ability[52];
    char impact[52];
    char muzzle[52];
    char sustained[52];
} BattleVfxWire;

typedef struct {
    char name[64];
    char tex[FX_MAX_EMITTERS][68];
    FxPreset pre;
} FxRegPreset;

typedef struct {
    char from[52];
    char to[64];
} FxAlias;

typedef enum {
    UNIT_SFX_MOVE_OUT,
    UNIT_SFX_MOVE_BACK,
    UNIT_SFX_LAND,
    UNIT_SFX_VOCAL,
    UNIT_SFX_ANIMATION,
    UNIT_SFX_ATTACK,
    UNIT_SFX_IMPACT,
    UNIT_SFX_DEATH,
    UNIT_SFX_COUNT
} UnitSfx;

#define BATTLE_UNIT_AUDIO_MAX_CLIPS 8
#define BATTLE_SFX_ALIAS_MAX 3
#define BATTLE_SFX_CACHE_MAX 512

typedef struct {
    const char *clips[BATTLE_UNIT_AUDIO_MAX_CLIPS];
    int count;
    float volume;
} BattleAudioPool;

typedef struct {
    bool bound;
    bool attack_capable;
    char unit[64];
    char prefab[64];
    char profile[64];
    BattleAudioPool pools[UNIT_SFX_COUNT];
} BattleUnitAudio;

typedef struct {
    unsigned short clips[BATTLE_UNIT_AUDIO_MAX_CLIPS];
    int count;
    float volume;
} BattleAudioPoolDef;

typedef struct {
    char name[40];
    BattleAudioPoolDef pools[UNIT_SFX_COUNT];
} BattleAudioProfileDef;

typedef struct {
    char unit[64];
    bool attack_capable;
    bool has_tier[TIER_COUNT];
    char prefab[TIER_COUNT][40];
    char profile[TIER_COUNT][40];
} BattleAudioUnitDef;

typedef struct {
    char name[36];
    char ref[64];
} BattleAudioSourceDef;

typedef struct {
    char name[48];
    Sound source;
    Sound aliases[BATTLE_SFX_ALIAS_MAX];
    int alias_count;
    int voice_cursor;
} BattleSfx;

typedef enum { PROJ_BALL, PROJ_ARROW, PROJ_SPIRIT, PROJ_DESICCATE } BattleProjLook;

typedef struct {
    BattleMesh meshes[BATTLE_MAX_MESH];
    int mesh_count;

    Texture2D lightmap;
    Texture2D t_woodfort, t_rock, t_forest, t_grass, t_backdrop, t_fairy;
    Texture2D t_grassbase, t_dirt, t_scorched, t_dgrass, t_mask;

    Shader lm_sh, ground_sh, unlit_sh, fog_sh;
    int lm_tint_loc, lm_cutoff_loc;
    Material mat_lm, mat_ground, mat_unlit, mat_fog;

    Texture2D t_particle, t_foam, t_fire, t_dots, t_smoke;
    BattleParticle rain[BATTLE_RAIN_MAX];
    BattleParticle splash[BATTLE_SPLASH_MAX];
    int rain_alive, splash_alive;
    float rain_accum;
    BattleFireEmitter fires[BATTLE_FIRE_EMITTERS];
    Music ambience, music_intro, music_loop;
    bool music_started;

    struct BattleRing {
        bool on;
        Vector3 pos;
        float scale;
        float pct;
    } rings[2][SQUAD_SLOT_COUNT];
    Shader ring_sh;
    int ring_cutoff_loc, ring_color_loc;
    Texture2D ring_tex;
    Mesh ring_mesh;
    Material ring_mat;

    struct BattleProj {
        Vector3 pos, vel;
        int look;
    } proj[8];
    int proj_count;

    FxParticle fxp[FX_MAX_PARTICLES];
    int fxp_count;
    FxInstance fxi[FX_MAX_INSTANCES];
    unsigned fx_serial;
    struct FxTexEntry {
        char name[96];
        Texture2D tex;
    } fxtex[320];
    int fxtex_count;

    FxRegPreset *fx_reg;
    int fx_reg_count;
    FxAlias *fx_alias;
    int fx_alias_count;

    float filter_amt;
    bool filter_on;
    bool punch[2][SQUAD_SLOT_COUNT];

    BattleVfxWire *wires;
    int wire_count;
    BattleSfx *sfx;
    int sfx_count;
    int sfx_capacity;
    char (*unit_audio_clips)[48];
    int unit_audio_clip_count;
    BattleAudioUnitDef *unit_audio_characters;
    int unit_audio_character_count;
    BattleAudioProfileDef *unit_audio_profiles;
    int unit_audio_profile_count;
    BattleAudioSourceDef *unit_audio_sources;
    int unit_audio_source_count;
    BattleUnitAudio unit_audio[2][SQUAD_SLOT_COUNT];
    bool audio_trace;
    Music combat_intro, combat_loop, result_music;
    int combat_music_state;

    BattleRoster enemy_roster;
    BattleSquads squads;
    bool squads_loaded;

    Camera3D cam;
    Camera3D cam_base, cam_matchmaking, cam_tactical, cam_transition_from;
    Vector3 cam_shake;
    bool tactical;
    bool combat_camera;
    float tac_t;
    float combat_cam_t;
} Battle;

#define BATTLE_RAIN_KILL_Y 0.1062f

static void BattleSplashSpawn(Battle *b, Vector3 at) {
    if (b->splash_alive >= BATTLE_SPLASH_MAX) return;
    BattleParticle *s = &b->splash[b->splash_alive++];
    *s = (BattleParticle){0};
    s->pos = (Vector3){at.x, BATTLE_RAIN_KILL_Y + 0.02f, at.z};
    s->life = 0.19f;
    s->size = 1.0f;
}

static void BattleFireSpawn(BattleFireEmitter *e) {
    if (e->alive >= BATTLE_FIRE_PMAX) return;
    if (e->type == BFIRE_SPARKS && e->alive >= 4) return;
    BattleParticle *p = &e->p[e->alive++];
    *p = (BattleParticle){0};
    Vector3 up = {0, 0.99962f, -0.02765f};
    float ang = BattleFrand(0, 2 * PI);
    switch (e->type) {
        case BFIRE_TONGUES: {
            float r = 0.5f * cbrtf(BattleFrand(0, 1));
            float ca = BattleFrand(-1, 1), sa = sqrtf(1 - ca * ca);
            p->pos = (Vector3){e->pos.x + r * sa * cosf(ang), e->pos.y + r * ca, e->pos.z + r * sa * sinf(ang)};
            p->vel = (Vector3){0};
            p->life = BattleFrand(1.0f, 3.0f);
            p->size = BattleFrand(1.0f, 2.0f);
            p->rotv = BattleFrand(-0.7854f, 0.7854f);
        } break;
        case BFIRE_SPARKS: {
            float r = 0.66f * sqrtf(BattleFrand(0, 1));
            p->pos = (Vector3){e->pos.x + r * cosf(ang), e->pos.y, e->pos.z + r * sinf(ang)};
            p->vel = Vector3Scale(up, 0.5979f);
            p->life = BattleFrand(0.5f, 2.0f);
            p->size = BattleFrand(1.0f, 2.0f);
            p->rot = BattleFrand(0, 2 * PI);
            p->rotv = BattleFrand(-1.0472f, 1.0472f);
        } break;
        case BFIRE_SMOKE: {
            float r = 0.5f * sqrtf(BattleFrand(0, 1));
            p->pos = (Vector3){e->pos.x + r * cosf(ang), e->pos.y, e->pos.z + r * sinf(ang)};
            p->vel = Vector3Scale(up, 1.4f);
            p->life = BattleFrand(1.0f, 4.0f);
            p->size = BattleFrand(3.0f, 5.0f);
            p->rotv = BattleFrand(-0.7854f, 0.7854f);
        } break;
    }
}

static void BattleVfxStep(Battle *b, float dt) {
    b->rain_accum += 381.6f * dt;
    while (b->rain_accum >= 1.0f && b->rain_alive < BATTLE_RAIN_MAX) {
        b->rain_accum -= 1.0f;
        BattleParticle *p = &b->rain[b->rain_alive++];
        *p = (BattleParticle){0};
        p->pos =
            (Vector3){-0.7839f + BattleFrand(-13.825f, 13.825f), 10.1062f, 34.75f + BattleFrand(-23.0853f, 23.0853f)};
        p->vel = (Vector3){28.75f, 0, 0};
        p->life = 5.0f;
        p->size = 0.07f;
    }
    for (int i = 0; i < b->rain_alive;) {
        BattleParticle *p = &b->rain[i];
        p->vel.y -= 196.2f * dt;
        p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, dt));
        p->age += dt;
        if (p->pos.y <= BATTLE_RAIN_KILL_Y || p->age >= p->life) {
            BattleSplashSpawn(b, p->pos);
            *p = b->rain[--b->rain_alive];
        } else
            i++;
    }
    for (int i = 0; i < b->splash_alive;) {
        BattleParticle *p = &b->splash[i];
        p->age += dt;
        if (p->age >= p->life)
            *p = b->splash[--b->splash_alive];
        else
            i++;
    }

    static const float FIRE_ACCEL[3] = {0.6867f, -0.2943f, -0.0981f};
    static const float FIRE_RATE[3] = {4.0f, 6.0f, 4.0f};
    for (int e = 0; e < BATTLE_FIRE_EMITTERS; e++) {
        BattleFireEmitter *fe = &b->fires[e];
        fe->accum += FIRE_RATE[fe->type] * dt;
        while (fe->accum >= 1.0f) {
            fe->accum -= 1.0f;
            BattleFireSpawn(fe);
        }
        for (int i = 0; i < fe->alive;) {
            BattleParticle *p = &fe->p[i];
            p->vel.y += FIRE_ACCEL[fe->type] * dt;
            p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, dt));
            p->rot += p->rotv * dt;
            p->age += dt;
            if (p->age >= p->life)
                *p = fe->p[--fe->alive];
            else
                i++;
        }
    }
}

static void BattleVfxUpdate(Battle *b, float dt) {
    dt = fminf(dt, 0.25f);
    while (dt > 0.0f) {
        float step = fminf(dt, 0.05f);
        BattleVfxStep(b, step);
        dt -= step;
    }
}

static Texture2D BattleLoadTex(const char *ref, bool clamp) {
    Texture2D t = ResourceLoadTexture(ref);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(t, TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureWrap(t, clamp ? TEXTURE_WRAP_CLAMP : TEXTURE_WRAP_REPEAT);
    return t;
}

static Texture2D BattleAlbedoFor(Battle *b, const char *mat) {
    if (!strcmp(mat, "woodfort_atlas_dif")) return b->t_woodfort;
    if (!strcmp(mat, "terrain_rock_dif") || !strcmp(mat, "plains_rock_back")) return b->t_rock;
    if (!strcmp(mat, "forest_atlas")) return b->t_forest;
    if (!strcmp(mat, "tallgrass_atlas")) return b->t_grass;
    if (!strcmp(mat, "plains_ground")) return b->t_grassbase;
    if (!strcmp(mat, "plains_fog_01")) return b->t_fairy;
    return b->t_backdrop;
}

static BattleKind BattleKindFromStr(const char *k) {
    if (!strcmp(k, "ground")) return BK_GROUND;
    if (!strcmp(k, "cutout")) return BK_CUTOUT;
    if (!strcmp(k, "backdrop")) return BK_BACKDROP;
    if (!strcmp(k, "grass")) return BK_GRASS;
    if (!strcmp(k, "fog")) return BK_FOG;
    return BK_LIGHTMAP;
}

static bool BattleSceneRangeFits(int bin_size, int offset, int size) {
    return offset >= 0 && size >= 0 && offset <= bin_size - size;
}

static void BattleLoadScene(Battle *b, const char *mesh_dir) {
    const char *base = TextFormat("%s/bg_plains_01_pre/reconstructed", mesh_dir);
    int bin_size = 0;
    unsigned char *bin = LoadFileData(TextFormat("%s/scene.bin", base), &bin_size);
    cJSON *root = JsonLoadFile(TextFormat("%s/scene.json", base));
    cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "meshes");
    if (!bin || !cJSON_IsArray(arr)) TraceLog(LOG_FATAL, "Battle: missing reconstructed scene in %s", base);

    b->mesh_count = 0;
    cJSON *m = NULL;
    cJSON_ArrayForEach(m, arr) {
        if (b->mesh_count >= BATTLE_MAX_MESH) break;
        int vc = JsonInt(m, "vcount"), ic = JsonInt(m, "icount");
        int pos_off = JsonInt(m, "pos_off"), uv0_off = JsonInt(m, "uv0_off");
        int uv1_off = JsonInt(m, "uv1_off"), idx_off = JsonInt(m, "idx_off");

        if (!BattleSceneRangeFits(bin_size, pos_off, vc * 3 * (int)sizeof(float)) ||
            !BattleSceneRangeFits(bin_size, uv0_off, vc * 2 * (int)sizeof(float)) ||
            !BattleSceneRangeFits(bin_size, uv1_off, vc * 2 * (int)sizeof(float)) ||
            !BattleSceneRangeFits(bin_size, idx_off, ic * (int)sizeof(unsigned short)))
            TraceLog(LOG_FATAL, "Battle: scene.json mesh %d overruns scene.bin in %s", b->mesh_count, base);

        BattleMesh *bm = &b->meshes[b->mesh_count];
        Mesh mesh = {0};
        mesh.vertexCount = vc;
        mesh.triangleCount = ic / 3;
        mesh.vertices = (float *)MemAlloc(vc * 3 * sizeof(float));
        mesh.texcoords = (float *)MemAlloc(vc * 2 * sizeof(float));
        mesh.texcoords2 = (float *)MemAlloc(vc * 2 * sizeof(float));
        mesh.indices = (unsigned short *)MemAlloc(ic * sizeof(unsigned short));
        memcpy(mesh.vertices, bin + pos_off, vc * 3 * sizeof(float));
        memcpy(mesh.texcoords, bin + uv0_off, vc * 2 * sizeof(float));
        memcpy(mesh.texcoords2, bin + uv1_off, vc * 2 * sizeof(float));
        memcpy(mesh.indices, bin + idx_off, ic * sizeof(unsigned short));
        UploadMesh(&mesh, false);

        const char *mat = JsonStr(m, "mat");
        bm->mesh = mesh;
        bm->kind = BattleKindFromStr(JsonStr(m, "kind"));
        bm->albedo = BattleAlbedoFor(b, mat);
        bm->tint = !strcmp(mat, "forest_atlas") ? 0.588f : 1.0f;
        bm->cutoff = (bm->kind == BK_CUTOUT) ? JsonFloat(m, "cutoff") : 0.0f;
        b->mesh_count++;
    }
    cJSON_Delete(root);
    UnloadFileData(bin);
    TraceLog(LOG_INFO, "Battle: loaded %d reconstructed meshes", b->mesh_count);
}

static void BattleWiresLoad(Battle *b, const char *path);
static void BattleFxRegistryLoad(Battle *b, const char *path);
static void BattleAudioManifestLoad(Battle *b, const char *path);
static void BattleAudioCacheClear(Battle *b);
static void BattleAudioBindSquads(Battle *b, const Player *player, const BattleRoster *enemy);

void BattleInitialize(Battle *b, const char *mesh_dir) {
    memset(b, 0, sizeof(*b));
    b->lm_sh = LoadShaderFromMemory(BATTLE_VS, BATTLE_FS_LM);
    b->ground_sh = LoadShaderFromMemory(BATTLE_VS, BATTLE_FS_GROUND);
    b->unlit_sh = LoadShaderFromMemory(BATTLE_VS, BATTLE_FS_UNLIT);
    b->fog_sh = LoadShaderFromMemory(BATTLE_VS, BATTLE_FS_FOG);
    b->lm_tint_loc = GetShaderLocation(b->lm_sh, "uTint");
    b->lm_cutoff_loc = GetShaderLocation(b->lm_sh, "uCutoff");
    b->ground_sh.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(b->ground_sh, "texture3");
    b->ground_sh.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(b->ground_sh, "texture4");
    b->ground_sh.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(b->ground_sh, "texture5");

    b->lightmap = BattleLoadTex(CACHE_RESOURCE("plains_lm1"), true);
    b->t_woodfort = BattleLoadTex(CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "woodfort_atlas_dif"), false);
    b->t_rock = BattleLoadTex(CACHE_RESOURCE("terrain_rock_dif"), false);
    b->t_forest = BattleLoadTex(CACHE_RESOURCE("Forest_atlas"), false);
    b->t_grass = BattleLoadTex(CACHE_RESOURCE("tallgrass_atlas"), false);
    b->t_backdrop = BattleLoadTex(CACHE_RESOURCE("plains_background_01"), false);
    b->t_fairy = BattleLoadTex(CACHE_RESOURCE("fairydust"), true);
    b->t_grassbase = BattleLoadTex(CACHE_RESOURCE("woodfort_grass_dif"), false);
    b->t_dirt = BattleLoadTex(CACHE_RESOURCE("forest_dirt_dif"), false);
    b->t_scorched = BattleLoadTex(CACHE_RESOURCE("scorched_grass"), false);
    b->t_dgrass = BattleLoadTex(CACHE_RESOURCE("dalishforest_grass04"), false);
    b->t_mask = BattleLoadTex(CACHE_RESOURCE("plains_ground_mask_rgb"), true);
    b->t_particle = BattleLoadTex(CACHE_RESOURCE("Default-Particle"), true);
    b->t_fire = BattleLoadTex(CACHE_RESOURCE("vfx_lvl_fire01"), true);
    b->ring_sh = LoadShaderFromMemory(VS_UNLIT, BATTLE_FS_RING);
    b->ring_cutoff_loc = GetShaderLocation(b->ring_sh, "uCutoff");
    b->ring_color_loc = GetShaderLocation(b->ring_sh, "uColor");
    b->ring_tex = BattleLoadTex(CACHE_RESOURCE("health_ring_new"), true);
    b->ring_mesh = GenMeshPlane(1.128f, 1.128f, 1, 1);
    b->ring_mat = LoadMaterialDefault();
    b->ring_mat.shader = b->ring_sh;
    b->ring_mat.maps[MATERIAL_MAP_DIFFUSE].texture = b->ring_tex;

    BattleWiresLoad(b, "data/unit_vfx.json");
    BattleFxRegistryLoad(b, "data/fx_presets.json");
    BattleAudioManifestLoad(b, "data/unit_audio.json");
    b->t_foam = BattleLoadTex(CACHE_RESOURCE("foam"), true);
    b->t_dots = BattleLoadTex(CACHE_RESOURCE("vfx_small_dots2"), true);
    b->t_smoke = BattleLoadTex(CACHE_RESOURCE("smoke2"), true);

    for (int i = 0; i < BATTLE_FIRE_EMITTERS; i++) {
        b->fires[i] = (BattleFireEmitter){0};
        b->fires[i].pos = (Vector3){BATTLE_FIRE_SPOTS[i].x, BATTLE_FIRE_SPOTS[i].y, -BATTLE_FIRE_SPOTS[i].z};
        b->fires[i].type = (BattleFireType)BATTLE_FIRE_SPOTS[i].type;
    }
    for (int i = 0; i < 100; i++) BattleVfxUpdate(b, 0.05f);

    b->ambience = GuiMusic(MUS_BATTLE_AMBIENCE);
    SetMusicVolume(b->ambience, 0.5f);
    b->music_intro = GuiMusic(MUS_BATTLE_INTRO);
    b->music_loop = GuiMusic(MUS_BATTLE_LOOP);
    b->music_started = false;

    BattleLoadScene(b, mesh_dir);

    b->mat_lm = LoadMaterialDefault();
    b->mat_lm.shader = b->lm_sh;
    b->mat_lm.maps[MATERIAL_MAP_METALNESS].texture = b->lightmap;

    b->mat_ground = LoadMaterialDefault();
    b->mat_ground.shader = b->ground_sh;
    b->mat_ground.maps[MATERIAL_MAP_ALBEDO].texture = b->t_grassbase;
    b->mat_ground.maps[MATERIAL_MAP_METALNESS].texture = b->lightmap;
    b->mat_ground.maps[MATERIAL_MAP_NORMAL].texture = b->t_dirt;
    b->mat_ground.maps[MATERIAL_MAP_ROUGHNESS].texture = b->t_scorched;
    b->mat_ground.maps[MATERIAL_MAP_OCCLUSION].texture = b->t_dgrass;
    b->mat_ground.maps[MATERIAL_MAP_EMISSION].texture = b->t_mask;

    b->mat_unlit = LoadMaterialDefault();
    b->mat_unlit.shader = b->unlit_sh;

    b->mat_fog = LoadMaterialDefault();
    b->mat_fog.shader = b->fog_sh;
    b->mat_fog.maps[MATERIAL_MAP_ALBEDO].texture = b->t_fairy;
    Vector3 fog_tint = {0.129055f, 0.174747f, 0.227941f};
    SetShaderValue(b->fog_sh, GetShaderLocation(b->fog_sh, "uTint"), &fog_tint, SHADER_UNIFORM_VEC3);

    Vector3 ucam = {-0.032752f, 7.313354f, -3.832929f};
    Vector3 ufwd = {-0.033352f, -0.339862f, -0.939884f};
    float d = 18.6f;
    b->cam_base = (Camera3D){0};
    b->cam_base.position = (Vector3){ucam.x, ucam.y, -ucam.z};
    b->cam_base.target = (Vector3){ucam.x + ufwd.x * d, ucam.y + ufwd.y * d, -(ucam.z + ufwd.z * d)};
    b->cam_base.up = (Vector3){-0.010280f, 0.940474f, 0.339711f};
    b->cam_base.fovy = 40.0f;
    b->cam_base.projection = CAMERA_PERSPECTIVE;

    b->cam_matchmaking = b->cam_base;
    b->cam_matchmaking.position = (Vector3){8.222285f, 7.227291f, 6.183612f};
    Vector3 mfwd = {-0.620640f, -0.378164f, 0.686876f};
    b->cam_matchmaking.target = Vector3Add(b->cam_matchmaking.position, Vector3Scale(mfwd, 18.6f));
    b->cam_matchmaking.up = (Vector3){-0.253531f, 0.925739f, 0.280589f};
    b->cam_matchmaking.fovy = 37.0f;

    b->cam_tactical = b->cam_base;
    b->cam_tactical.position = (Vector3){-0.1134f, 10.707f, 6.8663f};
    Vector3 tfwd = {-0.0298f, -0.6769f, 0.7355f};
    b->cam_tactical.target = Vector3Add(b->cam_tactical.position, Vector3Scale(tfwd, 14.0f));
    b->cam_tactical.up = (Vector3){-0.0182f, 0.7360f, 0.6767f};

    b->cam = b->cam_matchmaking;
    b->tactical = false;
    b->combat_camera = false;
    b->tac_t = 0.0f;
    b->combat_cam_t = 0.0f;
}

void BattleSetTactical(Battle *b, bool on) {
    if (b->combat_camera) return;
    b->tactical = on;
}

void BattleBeginCombatCamera(Battle *b) {
    if (b->combat_camera) return;
    b->cam_transition_from = b->cam;
    b->combat_camera = true;
    b->combat_cam_t = 0.0f;
    b->tactical = false;
}

static Quaternion BattleCameraQuaternion(Camera3D cam) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));
    Matrix rotation = {
        .m0 = right.x,
        .m1 = right.y,
        .m2 = right.z,
        .m4 = up.x,
        .m5 = up.y,
        .m6 = up.z,
        .m8 = -forward.x,
        .m9 = -forward.y,
        .m10 = -forward.z,
        .m15 = 1.0f,
    };
    return QuaternionNormalize(QuaternionFromMatrix(rotation));
}

static Camera3D BattleCameraLerp(Camera3D from, Camera3D to, float t) {
    Camera3D out = from;
    out.position = Vector3Lerp(from.position, to.position, t);
    Quaternion rotation = QuaternionSlerp(BattleCameraQuaternion(from), BattleCameraQuaternion(to), t);
    Vector3 forward = Vector3RotateByQuaternion((Vector3){0, 0, -1}, rotation);
    out.up = Vector3Normalize(Vector3RotateByQuaternion((Vector3){0, 1, 0}, rotation));
    float distance = Lerp(Vector3Distance(from.position, from.target), Vector3Distance(to.position, to.target), t);
    out.target = Vector3Add(out.position, Vector3Scale(forward, distance));
    out.fovy = from.fovy + (to.fovy - from.fovy) * t;
    out.projection = to.projection;
    return out;
}

static void BattleCameraUpdatePose(Battle *b, float dt) {
    Camera3D pose;
    if (b->combat_camera) {
        b->combat_cam_t = fminf(1.0f, b->combat_cam_t + dt / 0.5f);
        float t = b->combat_cam_t;
        float smooth = t * t * (3.0f - 2.0f * t);
        pose = BattleCameraLerp(b->cam_transition_from, b->cam_base, smooth);
    } else {
        float goal = b->tactical ? 1.0f : 0.0f;
        float step = dt / 0.5f;
        b->tac_t += (goal > b->tac_t) ? fminf(step, goal - b->tac_t) : -fminf(step, b->tac_t - goal);
        float smooth = b->tac_t * b->tac_t * (3.0f - 2.0f * b->tac_t);
        pose = BattleCameraLerp(b->cam_matchmaking, b->cam_tactical, smooth);
    }
    b->cam = pose;
    b->cam.position = Vector3Add(b->cam.position, b->cam_shake);
    b->cam.target = Vector3Add(b->cam.target, b->cam_shake);
}

static const FxPreset
    FX_PRESETS[] =
        {
            {.name = "fx_atk_lineslash_pre",
             .length = 0.7f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_atk_lineslash_pre", "sword slash"),
                      .additive = 1,
                      .cols = 4,
                      .rows = 2,
                      .sheet_once = 1,
                      .count = 1,
                      .life0 = 0.30f,
                      .life1 = 0.30f,
                      .size0 = 2.5f,
                      .size1 = 2.5f,
                      .alpha_hold = 0.85f,
                      .rot_deg = 20},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_atk_lineslash_pre", "sword slash"),
                      .additive = 1,
                      .cols = 4,
                      .rows = 2,
                      .sheet_once = 1,
                      .count = 1,
                      .delay = 0.09f,
                      .life0 = 0.30f,
                      .life1 = 0.30f,
                      .size0 = 2.3f,
                      .size1 = 2.3f,
                      .alpha_hold = 0.85f,
                      .rot_deg = 200},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_atk_lineslash_pre", "blood splat"),
                      .keyout = 1,
                      .cols = 5,
                      .rows = 5,
                      .count = 4,
                      .delay = 0.05f,
                      .life0 = 0.5f,
                      .life1 = 0.5f,
                      .speed0 = 1.0f,
                      .speed1 = 1.6f,
                      .dir = {0, 1, 0},
                      .cone_deg = 55,
                      .gravity = 2.4f,
                      .size0 = 0.5f,
                      .size1 = 0.62f,
                      .size_end = 1.6f,
                      .alpha_hold = 0.64f,
                      .rot_deg = 180},
                 }},
            {.name = "fx_heal_pre",
             .length = 0.71f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_heal_pre", "smoke3"),
                      .additive = 1,
                      .orient = 1,
                      .max_particles = 10,
                      .rate = 80,
                      .duration = 0.15f,
                      .life0 = 0.2f,
                      .life1 = 0.2f,
                      .drag = 4.5f,
                      .gravity = -179.4f,
                      .size0 = 2.4f,
                      .size1 = 2.4f,
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                      .tint = {152, 255, 0, 255},
                      .offset = {0, 0.1f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "chargespark"),
                      .additive = 1,
                      .max_particles = 60,
                      .rate = 300,
                      .duration = 0.2f,
                      .life0 = 0.1f,
                      .life1 = 0.3f,
                      .radius = 0.5f,
                      .gravity = -18.0f,
                      .size0 = 0.1f,
                      .size1 = 0.3f,
                      .size_curve = {.count = 3, .key = {{0, 0.2107f}, {0.4947f, 2.6743f}, {1, 1.195f}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                      .rot_bias_deg = 50.0f,
                      .tint = {152, 255, 0, 255},
                      .offset = {0, 0.2f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_heal_pre", "Default-Particle"),
                      .additive = 1,
                      .orient = 1,
                      .max_particles = 1,
                      .count = 1,
                      .delay = 0.11f,
                      .life0 = 0.5f,
                      .life1 = 0.5f,
                      .size0 = 2.0f,
                      .size1 = 2.0f,
                      .size_curve = {.count = 2, .key = {{0, 0.9992f}, {1, 1.8214f}}},
                      .alpha_curve = {.count = 3, .key = {{0, 1}, {0.49f, 1}, {1, 0}}},
                      .tint = {128, 190, 255, 255},
                      .offset = {0, 0.2f, 0}},
                 }},
            {.name = "fx_crit_pre",
             .length = 1.2f,
             .em_count = 6,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_crit_pre", "ARC_Strip003"),
                      .additive = 1,
                      .orient = 2,
                      .cols = 1,
                      .rows = 4,
                      .prewarm = 1,
                      .birth_emitter = {5, 6},
                      .max_screen_size = 10.0f,
                      .max_particles = 6,
                      .rate = 30.0f,
                      .duration = 0.4f,
                      .life0 = 0.05f,
                      .life1 = 0.05f,
                      .speed0 = 0.01f,
                      .speed1 = 0.01f,
                      .dir = {0, -1, 0},
                      .cone_deg = 5.0f,
                      .radius = 0.1f,
                      .size0 = 1.5f,
                      .size1 = 2.0f,
                      .alpha_curve =
                          {.count = 8,
                           .key =
                               {{0, 1},
                                {0.1935f, 1},
                                {0.2206f, 0},
                                {0.3275f, 1},
                                {0.4872f, 1},
                                {0.5047f, 0},
                                {0.6631f, 1},
                                {1, 1}}},
                      .color_curve =
                          {.count = 2,
                           .key =
                               {{.time = 0, .color = {62, 97, 227, 255}}, {.time = 1, .color = {159, 129, 118, 255}}}},
                      .stretch_length = 3.0f,
                      .offset = {-0.190658f, 0.894454f, -0.052110f}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_flare_01"),
                      .additive = 1,
                      .max_screen_size = 2.0f,
                      .max_particles = 3,
                      .count = 1,
                      .rate = 5.0f,
                      .rate_curve = {.count = 3, .key = {{0, 1}, {0.169453f, 1}, {0.216763f, 0}}},
                      .duration = 1.2f,
                      .life0 = 0.4f,
                      .life1 = 0.4f,
                      .size0 = 6.0f,
                      .size1 = 10.0f,
                      .size_curve =
                          {.count = 5, .key = {{0, 1}, {0.25f, 0.9375f}, {0.5f, 0.75f}, {0.75f, 0.4375f}, {1, 0}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                      .color_curve =
                          {.count = 3,
                           .key =
                               {{.time = 0, .color = {255, 121, 34, 255}},
                                {.time = 0.489845f, .color = {26, 72, 240, 255}},
                                {.time = 1, .color = {0, 0, 0, 255}}}},
                      .offset = {-0.190658f, 0.894454f, -0.052110f}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_crit_pre", "Square"),
                      .additive = 1,
                      .orient = 2,
                      .max_screen_size = 0.5f,
                      .max_particles = 100,
                      .count = 100,
                      .delay = 0.04f,
                      .life0 = 0.3f,
                      .life1 = 0.8f,
                      .speed0 = 10.0f,
                      .speed1 = 30.0f,
                      .dir = {0, 1, 0},
                      .cone_deg = 60.0f,
                      .radius = 0.2f,
                      .drag = 1.0f,
                      .gravity = 12.75f,
                      .size0 = 0.02f,
                      .size1 = 0.04f,
                      .alpha_curve = {.count = 4, .key = {{0, 0}, {0.035187f, 1}, {0.835294f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 3,
                           .key =
                               {{.time = 0, .color = {248, 183, 142, 175}},
                                {.time = 0.355886f, .color = {189, 111, 50, 175}},
                                {.time = 1, .color = {60, 25, 7, 175}}}},
                      .stretch_velocity = 0.05f,
                      .offset = {-0.190658f, 0.934932f, -0.052110f}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_smoke_whispy_cs"),
                      .additive = 1,
                      .cols = 2,
                      .rows = 2,
                      .max_screen_size = 0.5f,
                      .max_particles = 8,
                      .count = 8,
                      .life0 = 0.7f,
                      .life1 = 1.2f,
                      .speed0 = 5.0f,
                      .speed1 = 10.0f,
                      .dir = {0, 1, 0},
                      .cone_deg = 85.0f,
                      .radius = 0.1f,
                      .drag = 1.0f,
                      .gravity = -2.943f,
                      .size0 = 2.0f,
                      .size1 = 3.0f,
                      .size_curve =
                          {.count = 5,
                           .key =
                               {{0, 1.110577f}, {0.25f, 1.133903f}, {0.5f, 1.196899f}, {0.75f, 1.289089f}, {1, 1.4f}}},
                      .alpha_curve = {.count = 3, .key = {{0, 0}, {0.0230106f, 1}, {1, 0}}},
                      .rot_bias_deg = 90.0f,
                      .rot_deg = 90.0f,
                      .spin_deg = 30.0f,
                      .tint = {152, 182, 255, 209},
                      .offset = {0, 0.252f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_flare_01"),
                      .additive = 1,
                      .orient = 1,
                      .max_screen_size = 10.0f,
                      .max_particles = 4,
                      .life0 = 0.12f,
                      .life1 = 0.12f,
                      .size0 = 4.0f,
                      .size1 = 5.0f,
                      .size_curve =
                          {.count = 5, .key = {{0, 1}, {0.25f, 0.9375f}, {0.5f, 0.75f}, {0.75f, 0.4375f}, {1, 0}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                      .color_curve =
                          {.count = 5,
                           .key =
                               {{.time = 0, .color = {255, 121, 34, 255}},
                                {.time = 0.366705f, .color = {210, 111, 74, 255}},
                                {.time = 0.443839f, .color = {87, 85, 184, 255}},
                                {.time = 0.932341f, .color = {26, 72, 240, 255}},
                                {.time = 1, .color = {0, 0, 0, 255}}}},
                      .spin_bias_deg = 50.0f,
                      .spin_deg = 50.0f,
                      .offset = {-0.190658f, 0.894454f, -0.052110f}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_crit_pre", "radial_lines"),
                      .additive = 1,
                      .max_screen_size = 10.0f,
                      .max_particles = 4,
                      .life0 = 0.12f,
                      .life1 = 0.12f,
                      .size0 = 6.0f,
                      .size1 = 8.0f,
                      .size_curve =
                          {.count = 9,
                           .key =
                               {{0, 0},
                                {0.157750f, 0.920354f},
                                {0.163908f, 0},
                                {0.415174f, 0.831858f},
                                {0.420711f, 0},
                                {0.699987f, 0.584071f},
                                {0.703109f, 0},
                                {0.993007f, 0.433628f},
                                {0.993091f, 0}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                      .color_curve =
                          {.count = 5,
                           .key =
                               {{.time = 0, .color = {255, 121, 34, 255}},
                                {.time = 0.366705f, .color = {210, 111, 74, 255}},
                                {.time = 0.443839f, .color = {87, 85, 184, 255}},
                                {.time = 0.932341f, .color = {26, 72, 240, 255}},
                                {.time = 1, .color = {0, 0, 0, 255}}}},
                      .rot_bias_deg = 180.0f,
                      .rot_deg = 180.0f,
                      .spin_bias_deg = 50.0f,
                      .spin_deg = 50.0f,
                      .offset = {-0.190658f, 0.894454f, -0.052110f}},
                 }},
            {.name = "fx_placeunit_noLoop_pre",
             .length = 1.0f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_placeunit_noLoop_pre", "smoke"),
                      .count = 30,
                      .life0 = 0.8f,
                      .life1 = 0.8f,
                      .speed0 = 0.3f,
                      .speed1 = 0.5f,
                      .dir = {0, 1, 0},
                      .cone_deg = 85,
                      .radius = 0.55f,
                      .size0 = 0.5f,
                      .size1 = 0.8f,
                      .size_end = 0.65f,
                      .alpha_in = 0.26f,
                      .rot_deg = 180,
                      .spin_deg = 60,
                      .tint = {255, 255, 255, 84}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_placeunit_noLoop_pre", "RockA_4x4_CA"),
                      .cols = 4,
                      .rows = 4,
                      .count = 10,
                      .life0 = 0.8f,
                      .life1 = 0.8f,
                      .speed0 = 0.3f,
                      .speed1 = 0.5f,
                      .dir = {0, 1, 0},
                      .cone_deg = 75,
                      .radius = 0.55f,
                      .gravity = 0.49f,
                      .size0 = 0.1f,
                      .size1 = 0.2f,
                      .size_end = 0.65f,
                      .alpha_in = 0.26f,
                      .rot_deg = 180,
                      .spin_deg = 60},
                 }},
            {.name = "fx_imp_fireball_pre",
             .length = 0.9f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "fireballexp"),
                      .additive = 1,
                      .cols = 2,
                      .rows = 1,
                      .sheet_once = 1,
                      .count = 1,
                      .life0 = 0.12f,
                      .life1 = 0.12f,
                      .size0 = 1.0f,
                      .size1 = 1.0f,
                      .size_end = 3.0f,
                      .alpha_hold = 0.67f,
                      .rot_deg = 75,
                      .tint = {255, 184, 118, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "fireball06"),
                      .additive = 1,
                      .rate = 10,
                      .duration = 0.5f,
                      .life0 = 0.31f,
                      .life1 = 0.31f,
                      .size0 = 1.35f,
                      .size1 = 1.35f,
                      .size_end = 1.8f,
                      .rot_deg = 166,
                      .tint = {255, 115, 51, 255}},
                 }},
            {.name = "fx_primallightning_pre",
             .length = 0.9f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_primallightning_pre", "arc1a"),
                      .additive = 1,
                      .cols = 1,
                      .rows = 2,
                      .rate = 14,
                      .duration = 0.3f,
                      .life0 = 0.1f,
                      .life1 = 0.1f,
                      .size0 = 1.6f,
                      .size1 = 1.6f,
                      .tint = {97, 135, 255, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_primallightning_pre", "vfx_streak01"),
                      .additive = 1,
                      .rate = 50,
                      .duration = 0.4f,
                      .life0 = 0.25f,
                      .life1 = 0.3f,
                      .speed0 = 1.0f,
                      .speed1 = 1.0f,
                      .radius = 0.155f,
                      .size0 = 0.7f,
                      .size1 = 1.1f,
                      .alpha_in = 0.12f,
                      .alpha_hold = 0.5f,
                      .rot_deg = 180,
                      .spin_deg = 45,
                      .tint = {112, 129, 255, 150}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chainlightning_pre", "ColorFlare"),
                      .additive = 1,
                      .count = 1,
                      .delay = 0.01f,
                      .life0 = 0.5f,
                      .life1 = 0.5f,
                      .size0 = 1.3f,
                      .size1 = 1.3f,
                      .size_end = 1.8f,
                      .alpha_in = 0.4f,
                      .alpha_hold = 0.71f,
                      .spin_deg = 1440,
                      .tint = {148, 161, 255, 200}},
                 }},
            {.name = "fx_chainstrike_red_pre",
             .length = 1.0f,
             .em_count = 4,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chainstrike_red_pre", "sword slash"),
                      .additive = 1,
                      .cols = 4,
                      .rows = 2,
                      .sheet_once = 1,
                      .count = 1,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .size0 = 2.2f,
                      .size1 = 2.2f,
                      .alpha_hold = 0.8f,
                      .rot_deg = 15,
                      .tint = {236, 60, 60, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chainstrike_red_pre", "sword slash"),
                      .additive = 1,
                      .cols = 4,
                      .rows = 2,
                      .sheet_once = 1,
                      .count = 1,
                      .delay = 0.25f,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .size0 = 2.0f,
                      .size1 = 2.0f,
                      .alpha_hold = 0.8f,
                      .rot_deg = 100,
                      .tint = {255, 148, 148, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chainstrike_red_pre", "sword slash"),
                      .additive = 1,
                      .cols = 4,
                      .rows = 2,
                      .sheet_once = 1,
                      .count = 1,
                      .delay = 0.4f,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .size0 = 1.8f,
                      .size1 = 1.8f,
                      .alpha_hold = 0.8f,
                      .rot_deg = 170,
                      .tint = {255, 148, 148, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chainstrike_red_pre", "blood splat"),
                      .keyout = 1,
                      .cols = 5,
                      .rows = 5,
                      .count = 8,
                      .life0 = 0.5f,
                      .life1 = 0.5f,
                      .speed0 = 1.2f,
                      .speed1 = 1.8f,
                      .radius = 0.4f,
                      .gravity = 6.46f,
                      .size0 = 0.5f,
                      .size1 = 0.65f,
                      .size_end = 1.4f,
                      .alpha_in = 0.18f,
                      .alpha_hold = 0.5f,
                      .rot_deg = 180,
                      .spin_deg = 45},
                 }},
            {.name = "vfx_tripplehit_pre",
             .length = 1.7f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_tripplehit_pre", "tripple strike"),
                      .additive = 1,
                      .cols = 3,
                      .rows = 3,
                      .sheet_once = 1,
                      .rate = 2.56f,
                      .duration = 1.17f,
                      .life0 = 0.39f,
                      .life1 = 0.39f,
                      .size0 = 3.0f,
                      .size1 = 3.0f,
                      .alpha_hold = 0.8f,
                      .tint = {255, 60, 40, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_assail_pre", "vfx_streakExplode"),
                      .additive = 1,
                      .rate = 3,
                      .duration = 1.2f,
                      .life0 = 0.1f,
                      .life1 = 0.1f,
                      .size0 = 1.2f,
                      .size1 = 1.2f,
                      .size_end = 5.0f,
                      .alpha_hold = 1.0f,
                      .rot_deg = 63,
                      .tint = {255, 37, 0, 69}},
                 }},
            {.name = "vfx_ogresmash_pre",
             .length = 1.2f,
             .em_count = 4,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_ogresmash_pre", "RockA_4x4_CA"),
                      .cols = 4,
                      .rows = 4,
                      .count = 15,
                      .life0 = 0.46f,
                      .life1 = 0.46f,
                      .speed0 = 4.2f,
                      .speed1 = 10.0f,
                      .dir = {0, 1, 0},
                      .cone_deg = 45,
                      .gravity = 34.3f,
                      .size0 = 0.5f,
                      .size1 = 0.7f,
                      .size_end = 0.66f,
                      .alpha_hold = 0.63f,
                      .rot_deg = 180},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chainlightning_pre", "ColorFlare"),
                      .additive = 1,
                      .count = 3,
                      .delay = 0.03f,
                      .life0 = 0.8f,
                      .life1 = 0.8f,
                      .size0 = 2.8f,
                      .size1 = 2.8f,
                      .rot_deg = 180,
                      .tint = {255, 144, 0, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_ogresmash_pre", "centerbattlesubtract"),
                      .keyout = 2,
                      .count = 2,
                      .delay = 0.03f,
                      .life0 = 0.9f,
                      .life1 = 0.9f,
                      .size0 = 2.6f,
                      .size1 = 3.0f,
                      .rot_deg = 180,
                      .orient = 1,
                      .tint = {25, 16, 10, 235},
                      .offset = {0, -0.82f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_packOpen_legendary_pre", "Shockwave"),
                      .additive = 1,
                      .count = 4,
                      .life0 = 0.15f,
                      .life1 = 0.15f,
                      .size0 = 2.5f,
                      .size1 = 2.5f,
                      .size_end = 1.31f,
                      .orient = 1,
                      .tint = {255, 115, 51, 255},
                      .offset = {0, -0.8f, 0}},
                 }},
            {.name = "vfx_assassinate_pre",
             .length = 1.0f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_assassinate_pre", "Skullxbones"),
                      .additive = 1,
                      .cols = 6,
                      .rows = 6,
                      .sheet_once = 1,
                      .count = 1,
                      .life0 = 0.4f,
                      .life1 = 0.4f,
                      .speed0 = 1.0f,
                      .speed1 = 1.0f,
                      .dir = {0, 1, 0},
                      .size0 = 1.4f,
                      .size1 = 1.4f,
                      .tint = {255, 0, 0, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_imp_desiccate_baroness_pre", "smoke"),
                      .rate = 70,
                      .duration = 0.3f,
                      .delay = 0.1f,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .speed0 = 3.0f,
                      .speed1 = 3.0f,
                      .dir = {0, 1, 0},
                      .cone_deg = 40,
                      .size0 = 0.4f,
                      .size1 = 0.4f,
                      .size_end = 2.0f,
                      .alpha_in = 0.28f,
                      .rot_deg = 135,
                      .tint = {0, 0, 0, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_imp_desiccate_baroness_pre", "smokepuff1"),
                      .additive = 1,
                      .rate = 25,
                      .duration = 0.45f,
                      .life0 = 0.1f,
                      .life1 = 0.3f,
                      .speed0 = 5.0f,
                      .speed1 = 5.0f,
                      .dir = {0, 1, 0},
                      .radius = 0.25f,
                      .size0 = 1.0f,
                      .size1 = 1.3f,
                      .size_end = 2.0f,
                      .alpha_in = 0.28f,
                      .rot_deg = 120,
                      .spin_deg = 45,
                      .tint = {255, 34, 34, 255}},
                 }},
            {.name = "fx_lrgcrtr_massiveattack_pre",
             .length = 1.1f,
             .em_count = 4,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_lrgcrtr_massiveattack_pre", "vfx_streakExplode"),
                      .additive = 1,
                      .count = 4,
                      .delay = 0.01f,
                      .life0 = 0.15f,
                      .life1 = 0.15f,
                      .speed0 = 6.0f,
                      .speed1 = 6.0f,
                      .dir = {0, 1, 0},
                      .cone_deg = 30,
                      .size0 = 2.2f,
                      .size1 = 2.2f,
                      .alpha_in = 0.47f,
                      .rot_deg = 180,
                      .spin_deg = 90,
                      .tint = {200, 151, 63, 128}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_atk_scattershot_pre", "vfx_crack01"),
                      .keyout = 2,
                      .count = 1,
                      .delay = 0.03f,
                      .life0 = 0.9f,
                      .life1 = 0.9f,
                      .size0 = 2.8f,
                      .size1 = 2.8f,
                      .alpha_hold = 0.9f,
                      .rot_deg = 180,
                      .orient = 1,
                      .tint = {30, 22, 16, 235},
                      .offset = {0, -0.82f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_lrgcrtr_massiveattack_pre", "vfx_streakExplode"),
                      .additive = 1,
                      .count = 4,
                      .delay = 0.1f,
                      .life0 = 0.15f,
                      .life1 = 0.15f,
                      .size0 = 1.4f,
                      .size1 = 1.4f,
                      .size_end = 2.6f,
                      .alpha_hold = 0.57f,
                      .tint = {255, 115, 51, 180}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_lrgcrtr_massiveattack_pre", "vfx_wt_circleBeam_90degree"),
                      .additive = 1,
                      .count = 1,
                      .delay = 0.1f,
                      .life0 = 0.6f,
                      .life1 = 0.6f,
                      .gravity = -1.96f,
                      .size0 = 3.5f,
                      .size1 = 3.5f,
                      .size_end = 0.05f,
                      .alpha_hold = 1.0f,
                      .rot_deg = 180,
                      .tint = {255, 144, 0, 38}},
                 }},
            {.name = "vfx_perforate_pre",
             .length = 1.8f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_perforate_pre", "perforate 1"),
                      .additive = 1,
                      .cols = 4,
                      .rows = 4,
                      .sheet_once = 1,
                      .count = 2,
                      .rate = 2.19f,
                      .duration = 1.3f,
                      .life0 = 0.37f,
                      .life1 = 0.37f,
                      .size0 = 1.8f,
                      .size1 = 1.8f,
                      .alpha_hold = 0.83f},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_flare_01"),
                      .additive = 1,
                      .rate = 3,
                      .duration = 1.2f,
                      .life0 = 0.1f,
                      .life1 = 0.1f,
                      .size0 = 1.2f,
                      .size1 = 1.2f,
                      .size_end = 4.0f,
                      .alpha_hold = 1.0f,
                      .rot_deg = 75,
                      .tint = {255, 37, 0, 69}},
                 }},
            {.name = "fx_c_desiccate_pre",
             .length = 0.6f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_c_desiccate_pre", "chargegradient"),
                      .additive = 1,
                      .rate = 25,
                      .duration = 0.6f,
                      .life0 = 0.5f,
                      .life1 = 0.5f,
                      .size0 = 0.45f,
                      .size1 = 0.6f,
                      .size_end = 2.5f,
                      .alpha_in = 0.2f,
                      .rot_deg = 180,
                      .spin_deg = 30,
                      .tint = {49, 255, 65, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_chartraining_lvlup_pre", "GlowDot"),
                      .additive = 1,
                      .rate = 8,
                      .duration = 0.6f,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .size0 = 0.3f,
                      .size1 = 0.3f,
                      .size_end = 2.5f,
                      .alpha_in = 0.2f,
                      .tint = {91, 255, 104, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_atk_scattershot_pre", "vfx_crack01"),
                      .additive = 1,
                      .rate = 40,
                      .duration = 0.6f,
                      .life0 = 0.8f,
                      .life1 = 1.0f,
                      .speed0 = 0.2f,
                      .speed1 = 0.2f,
                      .dir = {0, 1, 0},
                      .radius = 0.25f,
                      .size0 = 0.12f,
                      .size1 = 0.25f,
                      .size_end = 1.9f,
                      .alpha_in = 0.12f,
                      .tint = {42, 255, 94, 255}},
                 }},
            {.name = "fx_imp_desiccate_pre",
             .length = 1.4f,
             .em_count = 4,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_flare_01"),
                      .additive = 1,
                      .count = 1,
                      .life0 = 0.4f,
                      .life1 = 0.4f,
                      .size0 = 3.4f,
                      .size1 = 3.4f,
                      .size_end = 0.4f,
                      .alpha_in = 0.05f,
                      .alpha_hold = 0.25f,
                      .tint = {99, 225, 83, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_lvl_leaf"),
                      .cols = 4,
                      .rows = 4,
                      .count = 50,
                      .life0 = 0.8f,
                      .life1 = 1.2f,
                      .speed0 = 8.0f,
                      .speed1 = 20.0f,
                      .radius = 0.4f,
                      .drag = 9.0f,
                      .gravity = 5.9f,
                      .size0 = 0.28f,
                      .size1 = 0.45f,
                      .alpha_hold = 0.72f,
                      .rot_deg = 180,
                      .spin_deg = 300,
                      .tint = {110, 190, 110, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_smoke_whispy_cs"),
                      .cols = 2,
                      .rows = 2,
                      .count = 8,
                      .life0 = 0.7f,
                      .life1 = 1.2f,
                      .speed0 = 4.0f,
                      .speed1 = 8.0f,
                      .dir = {0, 1, 0},
                      .cone_deg = 85,
                      .radius = 0.5f,
                      .drag = 5.0f,
                      .gravity = 2.9f,
                      .size0 = 1.4f,
                      .size1 = 1.9f,
                      .size_end = 1.4f,
                      .alpha_in = 0.02f,
                      .rot_deg = 180,
                      .spin_deg = 30,
                      .tint = {66, 101, 93, 209}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "shockwave01_02"),
                      .additive = 1,
                      .count = 1,
                      .life0 = 0.45f,
                      .life1 = 0.45f,
                      .size0 = 0.4f,
                      .size1 = 0.4f,
                      .size_end = 3.2f,
                      .alpha_in = 0.07f,
                      .tint = {24, 148, 30, 122}},
                 }},
            {.name = "vfx_bolter_pre",
             .length = 0.4f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_bolter_pre", "arrowsinglewhite2_tex"),
                      .additive = 1,
                      .orient = 2,
                      .rate = 4.2f,
                      .duration = 0.4f,
                      .life0 = 0.35f,
                      .life1 = 0.35f,
                      .size0 = 0.5f,
                      .size1 = 0.5f,
                      .tint = {255, 48, 0, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_bolter_pre", "arrowsinglewhite2_tex"),
                      .additive = 1,
                      .orient = 2,
                      .rate = 60,
                      .duration = 0.4f,
                      .life0 = 0.17f,
                      .life1 = 0.17f,
                      .size0 = 0.3f,
                      .size1 = 0.3f,
                      .tint = {255, 48, 0, 200}},
                 }},
            {.name = "vfx_pro_spiritball_pre",
             .length = 0.3f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_swordtornado_tex"),
                      .additive = 1,
                      .rate = 120,
                      .duration = 0.3f,
                      .life0 = 0.1f,
                      .life1 = 0.1f,
                      .speed0 = 0.66f,
                      .speed1 = 0.66f,
                      .radius = 0.01f,
                      .size0 = 0.55f,
                      .size1 = 0.55f,
                      .size_end = 0.1f,
                      .rot_deg = 360,
                      .spin_deg = 2000},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_pro_spiritball_pre", "llblue_laser_texture"),
                      .additive = 1,
                      .orient = 2,
                      .rate = 40,
                      .duration = 0.3f,
                      .life0 = 0.13f,
                      .life1 = 0.13f,
                      .size0 = 0.8f,
                      .size1 = 0.8f,
                      .size_end = 0.38f},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_pro_spiritball_pre", "plasma05"),
                      .additive = 1,
                      .orient = 2,
                      .rate = 40,
                      .duration = 0.3f,
                      .life0 = 0.12f,
                      .life1 = 0.12f,
                      .size0 = 0.5f,
                      .size1 = 0.5f,
                      .size_end = 0.19f},
                 }},
            {.name = "fx_imp_spiritbolt_pre",
             .length = 0.6f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "fireballexp"),
                      .additive = 1,
                      .cols = 2,
                      .rows = 1,
                      .sheet_once = 1,
                      .count = 2,
                      .life0 = 0.18f,
                      .life1 = 0.18f,
                      .size0 = 0.8f,
                      .size1 = 1.0f,
                      .size_end = 3.0f,
                      .alpha_hold = 0.67f,
                      .rot_deg = 75,
                      .tint = {143, 217, 255, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_swordtornado_tex"),
                      .additive = 1,
                      .rate = 100,
                      .duration = 0.2f,
                      .delay = 0.01f,
                      .life0 = 0.1f,
                      .life1 = 0.1f,
                      .speed0 = 0.66f,
                      .speed1 = 0.66f,
                      .radius = 0.01f,
                      .size0 = 0.55f,
                      .size1 = 0.55f,
                      .size_end = 0.14f,
                      .rot_deg = 360,
                      .spin_deg = 2000},
                 }},
            {.name = "vfx_crit_charge_pre",
             .length = 1.3f,
             .em_count = 4,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_crit_charge_pre", "battlerune"),
                      .additive = 1,
                      .count = 2,
                      .life0 = 0.5f,
                      .life1 = 0.5f,
                      .speed0 = 1.6f,
                      .speed1 = 2.0f,
                      .dir = {0, 1, 0},
                      .size0 = 1.0f,
                      .size1 = 1.2f,
                      .size_end = 1.5f,
                      .alpha_in = 0.57f,
                      .spin_deg = 2014,
                      .tint = {255, 70, 0, 255},
                      .offset = {0, 0.3f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_crit_charge_pre", "perforate 1"),
                      .additive = 1,
                      .cols = 2,
                      .rows = 2,
                      .orient = 2,
                      .count = 1,
                      .life0 = 0.78f,
                      .life1 = 0.78f,
                      .speed0 = 0.6f,
                      .speed1 = 0.6f,
                      .dir = {0, 1, 0},
                      .size0 = 1.5f,
                      .size1 = 1.5f,
                      .alpha_in = 0.32f,
                      .alpha_hold = 0.68f,
                      .tint = {255, 70, 0, 255},
                      .offset = {0, 0.9f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "fireballexp"),
                      .additive = 1,
                      .cols = 2,
                      .rows = 1,
                      .orient = 1,
                      .count = 1,
                      .life0 = 0.78f,
                      .life1 = 0.78f,
                      .size0 = 3.6f,
                      .size1 = 3.6f,
                      .size_end = 1.5f,
                      .alpha_in = 0.32f,
                      .alpha_hold = 0.68f,
                      .spin_deg = 1386,
                      .tint = {255, 166, 49, 255},
                      .offset = {0, 0.1f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_crit_charge_pre", "battlerune"),
                      .additive = 1,
                      .orient = 2,
                      .rate = 40,
                      .duration = 0.9f,
                      .life0 = 0.12f,
                      .life1 = 0.3f,
                      .speed0 = 11.19f,
                      .speed1 = 11.19f,
                      .dir = {0, 1, 0},
                      .radius = 1.0f,
                      .size0 = 0.6f,
                      .size1 = 0.6f,
                      .size_end = 0.05f,
                      .alpha_hold = 1.0f,
                      .tint = {255, 230, 2, 255},
                      .offset = {0, 0.9f, 0}},
                 }},
            {.name = "vfx_powerup_pre",
             .length = 0.7f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_powerup_pre", "SwipeArrow"),
                      .additive = 1,
                      .one_shot = 1,
                      .max_particles = 5,
                      .count = 5,
                      .life0 = 0.5f,
                      .life1 = 0.6f,
                      .speed0 = 1.56f,
                      .speed1 = 1.56f,
                      .dir = {0, 1, 0},
                      .radius = 0.3632f,
                      .gravity = -0.1962f,
                      .size0 = 0.6f,
                      .size1 = 0.6f,
                      .size_curve = {.count = 3, .key = {{0, 0.1503f}, {0.4158f, 1.8464f}, {1, 1.0413f}}},
                      .alpha_curve = {.count = 3, .key = {{0, 1}, {0.715f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 3,
                           .key =
                               {{.time = 0, .color = {255, 70, 0, 255}},
                                {.time = 0.542f, .color = {255, 251, 0, 255}},
                                {.time = 1, .color = {194, 255, 0, 255}}}},
                      .offset = {0, 0.5904f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_powerup_pre", "powerup"),
                      .additive = 1,
                      .cols = 3,
                      .rows = 3,
                      .sheet_once = 1,
                      .one_shot = 1,
                      .max_particles = 1,
                      .count = 1,
                      .life0 = 0.3f,
                      .life1 = 0.4f,
                      .speed0 = 2.01f,
                      .speed1 = 3.0f,
                      .dir = {0, 1, 0},
                      .size0 = 1.0f,
                      .size1 = 1.0f,
                      .alpha_curve = {.count = 3, .key = {{0, 1}, {0.498f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 2,
                           .key = {{.time = 0, .color = {255, 209, 0, 255}}, {.time = 1, .color = {152, 255, 0, 255}}}},
                      .offset = {0, 0.8225f, 0}},
                 }},
            {.name = "vfx_powerdown_pre",
             .length = 1.5f,
             .em_count = 3,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_powerdown_pre", "SwipeArrow"),
                      .additive = 1,
                      .max_particles = 18,
                      .rate = 6.2f,
                      .duration = 0.8f,
                      .period = 0.8f,
                      .life0 = 0.5f,
                      .life1 = 0.6f,
                      .speed0 = 3.06f,
                      .speed1 = 3.06f,
                      .dir = {0, 1, 0},
                      .radius = 0.56f,
                      .gravity = 17.658f,
                      .size0 = 0.35f,
                      .size1 = 0.35f,
                      .size_curve = {.count = 3, .key = {{0, 0.1503f}, {0.4158f, 1.8464f}, {1, 1.0413f}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                      .color_curve =
                          {.count = 2,
                           .key = {{.time = 0, .color = {130, 0, 255, 255}}, {.time = 1, .color = {18, 0, 139, 255}}}},
                      .rot_bias_deg = 180.0f,
                      .offset = {0, 1.0833f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_powerdown_pre", "powerdown"),
                      .additive = 1,
                      .cols = 3,
                      .rows = 3,
                      .sheet_once = 1,
                      .one_shot = 1,
                      .max_particles = 2,
                      .count = 2,
                      .life0 = 0.4f,
                      .life1 = 0.5f,
                      .speed0 = 1.5f,
                      .speed1 = 1.5f,
                      .dir = {0, -1, 0},
                      .size0 = 1.0f,
                      .size1 = 1.0f,
                      .alpha_curve = {.count = 3, .key = {{0, 1}, {0.5f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 2,
                           .key = {{.time = 0, .color = {130, 0, 255, 255}}, {.time = 1, .color = {18, 0, 139, 255}}}},
                      .offset = {0, 1.8518f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_lightning_2"),
                      .additive = 1,
                      .cols = 2,
                      .rows = 2,
                      .one_shot = 1,
                      .max_particles = 1,
                      .count = 1,
                      .life0 = 0.6f,
                      .life1 = 0.7f,
                      .size0 = 2.29f,
                      .size1 = 2.29f,
                      .alpha_curve = {.count = 3, .key = {{0, 0}, {0.475f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 5,
                           .key =
                               {{.time = 0, .color = {0, 0, 0, 255}},
                                {.time = 0.433f, .color = {0, 0, 0, 255}},
                                {.time = 0.453f, .color = {255, 255, 255, 255}},
                                {.time = 0.466f, .color = {255, 255, 255, 255}},
                                {.time = 0.481f, .color = {0, 0, 0, 255}}}},
                      .rot_bias_deg = 180.0f,
                      .rot_deg = 180.0f,
                      .offset = {0, 1.0379f, 0}},
                 }},
            {.name = "vfx_stunned_pre",
             .length = 6.3f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_stunned_pre", "halfring01"),
                      .additive = 1,
                      .orient = 1,
                      .prewarm = 1,
                      .max_particles = 12,
                      .rate = 12,
                      .duration = 5.0f,
                      .period = 5.0f,
                      .life0 = 1.0f,
                      .life1 = 1.0f,
                      .size0 = 1.2f,
                      .size1 = 1.2f,
                      .size_curve = {.count = 2, .key = {{0, 1}, {1, 0}}},
                      .alpha_curve = {.count = 4, .key = {{0, 0}, {0.172f, 1}, {0.413f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 3,
                           .key =
                               {{.time = 0.004f, .color = {255, 162, 79, 255}},
                                {.time = 0.1976f, .color = {218, 103, 0, 255}},
                                {.time = 1, .color = {118, 50, 5, 255}}}},
                      .spin_bias_deg = -300.0f,
                      .offset = {0, 2.0642f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("vfx_stunned_pre", "Star02"),
                      .additive = 1,
                      .prewarm = 1,
                      .max_particles = 40,
                      .rate = 20,
                      .duration = 5.0f,
                      .period = 5.0f,
                      .life0 = 1.2f,
                      .life1 = 1.2f,
                      .speed0 = 0.1f,
                      .speed1 = 0.1f,
                      .dir = {0, -0.9848f, -0.1736f},
                      .radius = 0.5f,
                      .gravity = 0.2943f,
                      .size0 = 0.25f,
                      .size1 = 0.25f,
                      .size_curve =
                          {.count = 5, .key = {{0, 0}, {0.0585f, 2}, {0.3872f, 0}, {0.4559f, 1.2308f}, {1, 0}}},
                      .alpha_curve = {.count = 4, .key = {{0, 0}, {0.172f, 1}, {0.843f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 2,
                           .key =
                               {{.time = 0, .color = {249, 192, 149, 255}},
                                {.time = 0.85385f, .color = {238, 69, 7, 255}}}},
                      .offset = {0, 2.1716f, 0}},
                 }},
            {.name = "fx_status_curse_power_pre",
             .length = 3.81f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_status_poison_pre", "shroomCloud_CA"),
                      .cols = 5,
                      .rows = 5,
                      .sheet_once = 1,
                      .one_shot = 1,
                      .max_particles = 25,
                      .count = 10,
                      .rate = 10.98f,
                      .duration = 0.58f,
                      .life0 = 0.4f,
                      .life1 = 0.7f,
                      .box = {0.6126f, 0.3251f, 0.7202f},
                      .drag = 10.0f,
                      .gravity = -2.943f,
                      .size0 = 1.04f,
                      .size1 = 1.04f,
                      .size_curve = {.count = 2, .key = {{0, 0.6951f}, {1, 1.2805f}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 0}}},
                      .rot_bias_deg = 64.8f,
                      .rot_deg = 101.4f,
                      .tint = {139, 255, 0, 255},
                      .offset = {0, 0.1487f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_status_poison_pre", "ColorFlare"),
                      .additive = 1,
                      .max_particles = 10,
                      .rate = 20.0f,
                      .rate_curve = {.count = 2, .key = {{0, 1}, {1, 0}}},
                      .duration = 3.0f,
                      .period = 3.0f,
                      .life0 = 0.71f,
                      .life1 = 0.71f,
                      .speed0 = 0.44f,
                      .speed1 = 0.75f,
                      .radius = 0.31f,
                      .gravity = -2.943f,
                      .size0 = 0.15f,
                      .size1 = 0.15f,
                      .size_curve = {.count = 3, .key = {{0, 4.9085f}, {0.235f, 1.54f}, {1, 0}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 0}}},
                      .tint = {146, 255, 6, 255}},
                 }},
            {.name = "fx_status_curse_health_pre",
             .length = 3.9f,
             .em_count = 2,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("fx_status_burning_pre", "fire_u_02"),
                      .additive = 1,
                      .one_shot = 1,
                      .max_particles = 15,
                      .count = 1,
                      .rate = 105.3f,
                      .rate_curve = {.count = 3, .key = {{0, 1}, {0.5f, 0.25f}, {1, 0}}},
                      .duration = 0.33f,
                      .life0 = 0.27f,
                      .life1 = 0.27f,
                      .box = {0.6126f, 0.3509f, 1.0557f},
                      .drag = 10.0f,
                      .size0 = 0.97f,
                      .size1 = 0.97f,
                      .size_curve = {.count = 2, .key = {{0, 1.0087f}, {1, 2.0f}}},
                      .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 0}}},
                      .rot_bias_deg = 64.8f,
                      .rot_deg = 101.4f,
                      .tint = {255, 188, 43, 255},
                      .offset = {0, 0.1477f, 0}},
                     {
                         .tex = CACHE_BUNDLE_RESOURCE("fx_status_burning_pre", "fx_flamefb01"),
                         .additive = 1,
                         .cols = 4,
                         .rows = 4,
                         .sheet_cycles = 2.0f,
                         .one_shot = 1,
                         .max_particles = 15,
                         .rate = 49.93f,
                         .rate_curve = {.count = 2, .key = {{0, 1}, {1, 0}}},
                         .duration = 3.0f,
                         .life0 = 0.21f,
                         .life1 = 0.8f,
                         .speed0 = 0.8f,
                         .speed1 = 0.8f,
                         .dir = {0, 1, 0},
                         .box = {0.3442f, 0.9433f, 0.3099f},
                         .size0 = 0.5f,
                         .size1 = 0.8f,
                         .size_curve = {.count = 2, .key = {{0, 1.5f}, {1, 1.5f}}},
                         .alpha_curve = {.count = 2, .key = {{0, 1}, {1, 1}}},
                         .color_curve =
                             {.count = 2,
                              .key =
                                  {{.time = 0, .color = {0, 104, 255, 255}},
                                   {.time = 0.726f, .color = {255, 142, 21, 255}}}},
                     },
                 }},
            {.name = "eff_initiativedown_pre",
             .length = 5.0f,
             .em_count = 5,
             .em =
                 {
                     {.tex = CACHE_BUNDLE_RESOURCE("eff_initiativedown_pre", "sword slash"),
                      .additive = 1,
                      .orient = 3,
                      .cols = 4,
                      .rows = 2,
                      .sheet_cycles = 6.0f,
                      .prewarm = 1,
                      .rate = 4.0f,
                      .duration = 0.6f,
                      .period = 0.6f,
                      .life0 = 1.0f,
                      .life1 = 1.0f,
                      .size0 = 0.5f,
                      .size1 = 0.5f,
                      .size_curve = {.count = 2, .key = {{0, 3}, {1, 0}}},
                      .alpha_curve = {.count = 4, .key = {{0, 0}, {0.33f, 1}, {0.62f, 1}, {1, 0}}},
                      .rot_bias_deg = 45.0f,
                      .rot_deg = 45.0f,
                      .spin_bias_deg = 200.0f,
                      .tint = {221, 138, 9, 255},
                      .offset = {0, -0.3f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("eff_initiativedown_pre", "Rune01"),
                      .additive = 1,
                      .orient = 1,
                      .max_screen_size = 0.5f,
                      .render_scale = 0.05f,
                      .max_particles = 4,
                      .count = 1,
                      .rate = 2.0f,
                      .duration = 5.0f,
                      .period = 5.0f,
                      .life0 = 2.0f,
                      .life1 = 2.0f,
                      .gravity = -0.0981f,
                      .size0 = 40.0f,
                      .size1 = 40.0f,
                      .size_curve = {.count = 2, .key = {{0, 0.1803f}, {0.996f, 1.2426f}}},
                      .alpha_curve = {.count = 4, .key = {{0, 0}, {0.059f, 1}, {0.712f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 2,
                           .key =
                               {{.time = 0, .color = {232, 117, 20, 255}}, {.time = 1, .color = {174, 37, 10, 255}}}},
                      .rot_bias_deg = 90.0f,
                      .spin_bias_deg = -200.0f,
                      .tint = {210, 87, 24, 255}},
                     {.tex = CACHE_BUNDLE_RESOURCE("eff_initiativedown_pre", "GlowDot"),
                      .additive = 1,
                      .one_shot = 1,
                      .max_particles = 5,
                      .count = 2,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .box = {0.2f, 0.2f, 0.2f},
                      .size0 = 15.0f,
                      .size1 = 15.0f,
                      .size_curve = {.count = 2, .key = {{0, 0}, {1, 1}}},
                      .alpha_curve = {.count = 3, .key = {{0, 0}, {0.0095f, 1}, {1, 0}}},
                      .spin_bias_deg = 20.0f,
                      .tint = {221, 69, 49, 255},
                      .offset = {0, 0.3465f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("eff_initiativedown_pre", "GlowDot"),
                      .additive = 1,
                      .one_shot = 1,
                      .max_particles = 5,
                      .count = 3,
                      .delay = 0.2f,
                      .life0 = 0.3f,
                      .life1 = 0.3f,
                      .box = {0.2f, 0.2f, 0.2f},
                      .size0 = 15.0f,
                      .size1 = 15.0f,
                      .size_curve = {.count = 2, .key = {{0, 0}, {1, 1}}},
                      .alpha_curve = {.count = 3, .key = {{0, 0}, {0.0095f, 1}, {1, 0}}},
                      .spin_bias_deg = 20.0f,
                      .tint = {221, 69, 49, 255},
                      .offset = {0, 0.3465f, 0}},
                     {.tex = CACHE_BUNDLE_RESOURCE("eff_initiativedown_pre", "Rune01"),
                      .additive = 1,
                      .orient = 1,
                      .max_screen_size = 0.5f,
                      .render_scale = 0.05f,
                      .prewarm = 1,
                      .max_particles = 4,
                      .rate = 2.0f,
                      .duration = 1.2f,
                      .period = 1.2f,
                      .life0 = 1.0f,
                      .life1 = 1.0f,
                      .size0 = 50.0f,
                      .size1 = 60.0f,
                      .size_curve = {.count = 2, .key = {{0, 0.2787f}, {0.996f, 0.9836f}}},
                      .alpha_curve = {.count = 4, .key = {{0, 0}, {0.059f, 1}, {0.712f, 1}, {1, 0}}},
                      .color_curve =
                          {.count = 2,
                           .key =
                               {{.time = 0, .color = {232, 117, 20, 255}}, {.time = 1, .color = {174, 37, 10, 255}}}},
                      .rot_bias_deg = 45.0f,
                      .rot_deg = 45.0f,
                      .spin_curve = {.count = 2, .key = {{0.002f, 167.2f}, {0.998f, 24.6f}}},
                      .tint = {210, 87, 24, 255},
                      .offset = {0, -0.28f, 0}},
                 }},
            {.name = "fx_eff_target_marker_pre",
             .length = 0.8f,
             .em_count = 1,
             .em = {
                 {.tex = CACHE_BUNDLE_RESOURCE("eff_initiativedown_pre", "GlowDot"),
                  .additive = 1,
                  .orient = 1,
                  .rate = 1.3f,
                  .life0 = 0.75f,
                  .life1 = 0.75f,
                  .size0 = 1.5f,
                  .size1 = 1.5f,
                  .size_end = 1.08f,
                  .alpha_in = 0.15f,
                  .tint = {196, 0, 0, 255},
                  .offset = {0, 0.05f, 0}},
             }},
};

static const FxPreset *BattleFxFindPreset(const Battle *b, const char *name) {
    if (!name || !name[0]) return NULL;
    for (int hop = 0; hop < 2; hop++) {
        for (int i = 0; i < (int)(sizeof(FX_PRESETS) / sizeof(FX_PRESETS[0])); i++)
            if (strcmp(FX_PRESETS[i].name, name) == 0) return &FX_PRESETS[i];
        for (int i = 0; i < b->fx_reg_count; i++)
            if (strcmp(b->fx_reg[i].name, name) == 0) return &b->fx_reg[i].pre;
        const char *to = NULL;
        for (int i = 0; i < b->fx_alias_count && !to; i++)
            if (strcmp(b->fx_alias[i].from, name) == 0) to = b->fx_alias[i].to;
        if (!to) return NULL;
        name = to;
    }
    return NULL;
}

static const FxPreset *BattleFxResolve(const Battle *b, const char *name) {
    if (!name || !name[0] || strcmp(name, "fx_null_pre") == 0) return NULL;
    const FxPreset *p = BattleFxFindPreset(b, name);
    if (p) return p;
    static const struct {
        const char *needle, *preset;
    } FAMILIES[] = {
        {"lightning", "fx_primallightning_pre"},
        {"spark", "fx_primallightning_pre"},
        {"shock", "fx_primallightning_pre"},
        {"chain", "fx_chainstrike_red_pre"},
        {"smash", "vfx_ogresmash_pre"},
        {"slam", "vfx_ogresmash_pre"},
        {"stomp", "vfx_ogresmash_pre"},
        {"quake", "vfx_ogresmash_pre"},
        {"assassin", "vfx_assassinate_pre"},
        {"skull", "vfx_assassinate_pre"},
        {"tripplehit", "vfx_tripplehit_pre"},
        {"claw", "vfx_tripplehit_pre"},
        {"maul", "vfx_tripplehit_pre"},
        {"rend", "vfx_tripplehit_pre"},
        {"perforate", "vfx_perforate_pre"},
        {"heal", "fx_heal_pre"},
        {"imp_", "fx_imp_fireball_pre"},
    };
    if (name)
        for (int i = 0; i < (int)(sizeof(FAMILIES) / sizeof(FAMILIES[0])); i++)
            if (strstr(name, FAMILIES[i].needle) && (p = BattleFxFindPreset(b, FAMILIES[i].preset))) return p;
    return BattleFxFindPreset(b, "fx_atk_lineslash_pre");
}

static Color FxKeyoutPixel(Color c, int keyout) {
    unsigned char lo = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
    unsigned char hi = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
    unsigned mask = keyout == 1 ? 255u - lo : hi;
    unsigned char alpha = (unsigned char)((mask * c.a + 127u) / 255u);
    return keyout == 1 ? (Color){c.r, c.g, c.b, alpha} : (Color){255, 255, 255, alpha};
}

static Texture2D BattleFxTex(Battle *b, const char *name, int keyout) {
    char id[96];
    snprintf(id, sizeof(id), "%d|%s", keyout, name);
    for (int i = 0; i < b->fxtex_count; i++)
        if (strcmp(b->fxtex[i].name, id) == 0) return b->fxtex[i].tex;
    Texture2D tex = {0};
    Image img = ResourceLoadImage(name);
    if (img.data) {
        if (keyout) {
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            Color *px = (Color *)img.data;
            for (int i = 0; i < img.width * img.height; i++) px[i] = FxKeyoutPixel(px[i], keyout);
        }
        tex = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    } else {
        TraceLog(LOG_WARNING, "Battle: fx texture %s missing", name);
    }
    if (b->fxtex_count >= (int)(sizeof(b->fxtex) / sizeof(b->fxtex[0]))) {
        TraceLog(LOG_WARNING, "Battle: fx texture cache full, reusing a cached entry for %s", name);
        if (tex.id > 0) UnloadTexture(tex);
        return b->fxtex[0].tex;
    }
    StrCopy(b->fxtex[b->fxtex_count].name, sizeof(b->fxtex[0].name), id);
    b->fxtex[b->fxtex_count++].tex = tex;
    return tex;
}

static void BattleFxUpdate(Battle *b, float dt);
static void BattleFxDraw(Battle *b);

void BattleFxPlayOnCamera(Battle *b, Camera3D camera, float dt) {
    BattleFxUpdate(b, dt);
    Camera3D battle_camera = b->cam;
    b->cam = camera;
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    BattleFxDraw(b);
    rlEnableBackfaceCulling();
    EndMode3D();
    b->cam = battle_camera;
}

static Vector3 FxConeDir(Vector3 dir, float deg) {
    float len = Vector3Length(dir);
    if (len < 0.01f) {
        float ca = BattleFrand(-1, 1), sa = sqrtf(1.0f - ca * ca), az = BattleFrand(0, 2 * PI);
        return (Vector3){sa * cosf(az), ca, sa * sinf(az)};
    }
    dir = Vector3Scale(dir, 1.0f / len);
    if (deg <= 0.01f) return dir;
    Vector3 axis = fabsf(dir.y) < 0.95f ? (Vector3){0, 1, 0} : (Vector3){1, 0, 0};
    Vector3 u = Vector3Normalize(Vector3CrossProduct(dir, axis));
    Vector3 v = Vector3CrossProduct(dir, u);
    float az = BattleFrand(0, 2 * PI);
    float t = tanf(DEG2RAD * deg) * sqrtf(BattleFrand(0, 1));
    return Vector3Normalize(Vector3Add(dir, Vector3Add(Vector3Scale(u, cosf(az) * t), Vector3Scale(v, sinf(az) * t))));
}

static float FxCurveSample(const FxCurve *curve, float t, float fallback) {
    if (!curve || curve->count == 0) return fallback;
    int count = curve->count > FX_MAX_CURVE_KEYS ? FX_MAX_CURVE_KEYS : curve->count;
    return BattleEnvelope(curve->key, count, t);
}

static Color FxColorSample(const FxColorCurve *curve, float t, Color fallback) {
    if (!curve || curve->count == 0) return fallback;
    int count = curve->count > FX_MAX_CURVE_KEYS ? FX_MAX_CURVE_KEYS : curve->count;
    if (t <= curve->key[0].time) return curve->key[0].color;
    for (int i = 1; i < count; i++) {
        if (t > curve->key[i].time) continue;
        float span = curve->key[i].time - curve->key[i - 1].time;
        float f = span > 0.0001f ? (t - curve->key[i - 1].time) / span : 1.0f;
        Color a = curve->key[i - 1].color, z = curve->key[i].color;
        return (Color){
            (unsigned char)(a.r + (z.r - a.r) * f),
            (unsigned char)(a.g + (z.g - a.g) * f),
            (unsigned char)(a.b + (z.b - a.b) * f),
            (unsigned char)(a.a + (z.a - a.a) * f),
        };
    }
    return curve->key[count - 1].color;
}

static FxParticle *
FxEmitOneAt(Battle *b, const FxInstance *ins, const FxEmitter *em, const Vector3 *birth_origin, int birth_depth) {
    if (em->max_particles > 0) {
        int live = 0;
        for (int i = 0; i < b->fxp_count; i++)
            if (b->fxp[i].serial == ins->serial && b->fxp[i].em == em) live++;
        if (live >= em->max_particles) return NULL;
    }
    if (b->fxp_count >= FX_MAX_PARTICLES) return NULL;
    FxParticle *p = &b->fxp[b->fxp_count++];
    Vector3 emitter_offset = em->offset;
    if (ins->directed) emitter_offset = Vector3RotateByQuaternion(emitter_offset, ins->rotation);
    Vector3 base = birth_origin ? *birth_origin : Vector3Add(ins->pos, Vector3Add(ins->offset, emitter_offset));
    if (fabsf(em->box.x) + fabsf(em->box.y) + fabsf(em->box.z) > 0.001f) {
        Vector3 jitter = {
            BattleFrand(-em->box.x * 0.5f, em->box.x * 0.5f),
            BattleFrand(-em->box.y * 0.5f, em->box.y * 0.5f),
            BattleFrand(-em->box.z * 0.5f, em->box.z * 0.5f),
        };
        if (ins->directed) jitter = Vector3RotateByQuaternion(jitter, ins->rotation);
        base = Vector3Add(base, jitter);
    } else if (em->radius > 0.001f) {
        float r = em->radius * cbrtf(BattleFrand(0, 1));
        float ca = BattleFrand(-1, 1), sa = sqrtf(1.0f - ca * ca), az = BattleFrand(0, 2 * PI);
        base = Vector3Add(base, (Vector3){r * sa * cosf(az), r * ca, r * sa * sinf(az)});
    }
    Vector3 emitter_dir = em->dir;
    if (ins->directed) emitter_dir = Vector3RotateByQuaternion(emitter_dir, ins->rotation);
    Vector3 dir = FxConeDir(emitter_dir, em->cone_deg);
    float sp = BattleFrand(em->speed0, em->speed1);
    Texture2D tex = {0};
    if (em->tex && em->tex[0]) tex = BattleFxTex(b, em->tex, em->keyout);
    *p = (FxParticle){
        .em = em,
        .pre = ins->pre,
        .tex = tex,
        .pos = base,
        .vel = Vector3Scale(dir, sp),
        .anchor_pos = ins->pos,
        .life = fmaxf(0.03f, BattleFrand(em->life0, em->life1)),
        .size = BattleFrand(em->size0, em->size1),
        .rot = DEG2RAD * (em->rot_bias_deg + BattleFrand(-em->rot_deg, em->rot_deg)),
        .spin = DEG2RAD * (em->spin_bias_deg + BattleFrand(-em->spin_deg, em->spin_deg)),
        .anchor = ins->anchor,
        .aura_slot = ins->aura_slot,
        .serial = ins->serial,
        .seed = (unsigned)GetRandomValue(0, 0x7FFF),
        .start_color = ins->start_color,
    };
    if (birth_depth < FX_MAX_EMITTERS && ins->pre) {
        for (int child = 0; child < 2; child++) {
            int emitter_index = em->birth_emitter[child] - 1;
            if (emitter_index < 0 || emitter_index >= ins->pre->em_count) continue;
            FxEmitOneAt(b, ins, &ins->pre->em[emitter_index], &p->pos, birth_depth + 1);
        }
    }
    return p;
}

static FxParticle *FxEmitOne(Battle *b, const FxInstance *ins, const FxEmitter *em) {
    return FxEmitOneAt(b, ins, em, NULL, 0);
}

static FxInstance *FxInstanceClaim(Battle *b) {
    for (int i = 0; i < FX_MAX_INSTANCES; i++)
        if (!b->fxi[i].alive) return &b->fxi[i];
    return NULL;
}

static void FxInstanceStart(Battle *b, FxInstance *ins) {
    ins->serial = ++b->fx_serial;
    for (int e = 0; e < ins->pre->em_count; e++)
        if (ins->pre->em[e].rate > 0) ins->accum[e] = 1.0f;
}

static void BattleFxBurstRotated(Battle *b, const char *name, Vector3 pos, Quaternion rotation, bool directed) {
    const FxPreset *pre = BattleFxResolve(b, name);
    if (!pre) return;
    FxInstance *ins = FxInstanceClaim(b);
    if (!ins) return;
    *ins = (FxInstance){
        .pre = pre,
        .alive = true,
        .directed = directed,
        .anchor = -1,
        .pos = pos,
        .rotation = rotation,
    };
    FxInstanceStart(b, ins);
}

void BattleFxBurst(Battle *b, const char *name, Vector3 pos) {
    BattleFxBurstRotated(b, name, pos, QuaternionIdentity(), false);
}

void BattleFxBurstDirected(Battle *b, const char *name, Vector3 pos, Vector3 target) {
    Vector3 direction = Vector3Subtract(target, pos);
    if (Vector3Length(direction) < 0.01f) {
        BattleFxBurst(b, name, pos);
        return;
    }
    Quaternion rotation = QuaternionFromVector3ToVector3((Vector3){0, 0, -1}, Vector3Normalize(direction));
    BattleFxBurstRotated(b, name, pos, QuaternionNormalize(rotation), true);
}

void BattleFxUnitBurst(Battle *b, const char *name, int side, int slot, Vector3 offset) {
    const FxPreset *pre = BattleFxFindPreset(b, name);
    if (!pre || side < 0 || side > 1 || slot < 0 || slot >= SQUAD_SLOT_COUNT) return;
    FxInstance *ins = FxInstanceClaim(b);
    if (!ins) return;
    *ins = (FxInstance){
        .pre = pre,
        .alive = true,
        .anchor = side * SQUAD_SLOT_COUNT + slot,
        .pos = b->rings[side][slot].pos,
        .offset = offset,
    };
    FxInstanceStart(b, ins);
}

void BattleFxUnitColorAura(Battle *b, const char *name, int side, int slot, Color color) {
    const FxPreset *pre = BattleFxFindPreset(b, name);
    if (!pre || side < 0 || side > 1 || slot < 0 || slot >= SQUAD_SLOT_COUNT) return;
    FxInstance *ins = FxInstanceClaim(b);
    if (!ins) return;
    *ins = (FxInstance){
        .pre = pre,
        .alive = true,
        .anchor = side * SQUAD_SLOT_COUNT + slot,
        .pos = b->rings[side][slot].pos,
        .start_color = color,
    };
    FxInstanceStart(b, ins);
}

void BattleFxStatusClear(Battle *b, FxAuraSlot aura_slot, int side, int slot) {
    if (aura_slot <= FX_AURA_GENERIC || aura_slot >= FX_AURA_SLOT_COUNT || side < 0 || side > 1 || slot < 0 ||
        slot >= SQUAD_SLOT_COUNT)
        return;
    int anchor = side * SQUAD_SLOT_COUNT + slot;
    for (int i = 0; i < FX_MAX_INSTANCES; i++) {
        FxInstance *ins = &b->fxi[i];
        if (ins->alive && ins->loop && ins->anchor == anchor && ins->aura_slot == aura_slot) ins->alive = false;
    }
    for (int i = 0; i < b->fxp_count;) {
        FxParticle *p = &b->fxp[i];
        if (p->anchor == anchor && p->aura_slot == aura_slot)
            *p = b->fxp[--b->fxp_count];
        else
            i++;
    }
}

void BattleFxStatusSet(Battle *b, FxAuraSlot aura_slot, const char *name, int side, int slot, Vector3 offset) {
    const FxPreset *pre = BattleFxFindPreset(b, name);
    if (!pre || aura_slot <= FX_AURA_GENERIC || aura_slot >= FX_AURA_SLOT_COUNT || side < 0 || side > 1 || slot < 0 ||
        slot >= SQUAD_SLOT_COUNT)
        return;
    BattleFxStatusClear(b, aura_slot, side, slot);
    FxInstance *ins = FxInstanceClaim(b);
    if (!ins) return;
    *ins = (FxInstance){
        .pre = pre,
        .alive = true,
        .loop = true,
        .anchor = side * SQUAD_SLOT_COUNT + slot,
        .aura_slot = aura_slot,
        .pos = b->rings[side][slot].pos,
        .offset = offset,
    };
    FxInstanceStart(b, ins);
}

void BattleFxTrail(
    Battle *b,
    const char *name,
    Vector3 pos,
    Vector3 vel,
    float age,
    float dt,
    float accum[FX_MAX_EMITTERS],
    bool burst_done[FX_MAX_EMITTERS]
) {
    const FxPreset *pre = BattleFxFindPreset(b, name);
    if (!pre) return;
    FxInstance tmp = {.pre = pre, .alive = true, .anchor = -1, .pos = pos};
    Vector3 axis = Vector3Length(vel) > 0.01f ? Vector3Normalize(vel) : (Vector3){1, 0, 0};
    for (int e = 0; e < pre->em_count; e++) {
        const FxEmitter *em = &pre->em[e];
        float source_age = age - em->delay;
        if (source_age < 0.0f) continue;
        if (em->count > 0 && !burst_done[e]) {
            burst_done[e] = true;
            for (int n = 0; n < em->count; n++) FxEmitOne(b, &tmp, em);
        }
        if (em->rate <= 0) continue;
        if (em->one_shot && em->duration > 0.001f && source_age > em->duration) continue;
        float curve_age = em->period > 0.001f ? fmodf(source_age, em->period) : source_age;
        float curve_t = em->duration > 0.001f ? fminf(curve_age / em->duration, 1.0f) : 0.0f;
        float rate = em->rate * fmaxf(0.0f, FxCurveSample(&em->rate_curve, curve_t, 1.0f));
        accum[e] += fminf(rate, 120.0f) * dt;
        while (accum[e] >= 1.0f) {
            accum[e] -= 1.0f;
            FxParticle *p = FxEmitOne(b, &tmp, em);
            if (p && em->orient == 2 && em->speed1 < 0.01f) p->vel = Vector3Scale(axis, 0.05f);
        }
    }
}

static void FxAdvancePrewarmedParticles(Battle *b, int first, float age) {
    for (int i = first; i < b->fxp_count;) {
        FxParticle *p = &b->fxp[i];
        if (age >= p->life) {
            *p = b->fxp[--b->fxp_count];
            continue;
        }
        p->age = age;
        float simulated = 0.0f;
        while (simulated < age) {
            float step = fminf(1.0f / 120.0f, age - simulated);
            if (p->em->drag > 0.001f) p->vel = Vector3Scale(p->vel, expf(-p->em->drag * step));
            p->vel.y -= p->em->gravity * step;
            p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, step));
            float spin = p->spin;
            if (p->em->spin_curve.count > 0)
                spin = DEG2RAD * FxCurveSample(&p->em->spin_curve, (simulated + step) / p->life, 0.0f);
            p->rot += spin * step;
            simulated += step;
        }
        i++;
    }
}

static float FxPrewarmEmitter(Battle *b, FxInstance *ins, const FxEmitter *em) {
    float max_life = fmaxf(em->life0, em->life1);
    if (ins->pre)
        for (int child = 0; child < 2; child++) {
            int emitter_index = em->birth_emitter[child] - 1;
            if (emitter_index < 0 || emitter_index >= ins->pre->em_count) continue;
            const FxEmitter *child_em = &ins->pre->em[emitter_index];
            max_life = fmaxf(max_life, fmaxf(child_em->life0, child_em->life1));
        }
    float warmup = em->duration > 0.001f ? em->duration : (em->period > 0.001f ? em->period : max_life);
    if (em->rate <= 0.0f || warmup <= 0.0f) return 0.0f;

    float rate_accum = 0.0f;
    float elapsed = 0.0f;
    while (elapsed < warmup) {
        float step = fminf(1.0f / 120.0f, warmup - elapsed);
        float curve_t = (elapsed + step * 0.5f) / warmup;
        float rate = em->rate * fmaxf(0.0f, FxCurveSample(&em->rate_curve, curve_t, 1.0f));
        rate_accum += fminf(rate, 600.0f) * step;
        while (rate_accum + 0.000001f >= 1.0f) {
            rate_accum -= 1.0f;
            if (rate_accum < 0.0f) rate_accum = 0.0f;
            float age = warmup - (elapsed + step);
            if (age >= max_life) continue;
            int first = b->fxp_count;
            if (FxEmitOne(b, ins, em)) FxAdvancePrewarmedParticles(b, first, age);
        }
        elapsed += step;
    }
    return rate_accum;
}

static void BattleFxUpdate(Battle *b, float dt) {
    for (int i = 0; i < FX_MAX_INSTANCES; i++) {
        FxInstance *ins = &b->fxi[i];
        if (!ins->alive) continue;
        if (ins->anchor >= 0) {
            struct BattleRing *r = &b->rings[ins->anchor / SQUAD_SLOT_COUNT][ins->anchor % SQUAD_SLOT_COUNT];
            ins->pos = r->pos;
        }
        const FxPreset *pre = ins->pre;
        if (ins->loop)
            for (int e = 0; e < pre->em_count; e++) {
                const FxEmitter *em = &pre->em[e];
                if (em->prewarm && !em->one_shot && !ins->prewarm_done[e]) {
                    ins->prewarm_done[e] = true;
                    ins->accum[e] = FxPrewarmEmitter(b, ins, em);
                }
            }
        ins->age += dt;
        for (int e = 0; e < pre->em_count; e++) {
            const FxEmitter *em = &pre->em[e];
            float source_age = ins->age - em->delay;
            bool repeating = ins->loop && !em->one_shot && em->period > 0.001f;
            unsigned cycle = repeating && source_age >= 0.0f ? (unsigned)floorf(source_age / em->period) : 0;
            float cycle_age = repeating ? source_age - cycle * em->period : source_age;
            if (em->count > 0 && source_age >= 0.0f) {
                if (repeating) {
                    unsigned marker = cycle + 1;
                    if (ins->burst_cycle[e] != marker) {
                        ins->burst_cycle[e] = marker;
                        for (int n = 0; n < em->count; n++) FxEmitOne(b, ins, em);
                    }
                } else if (!ins->burst_done[e]) {
                    ins->burst_done[e] = true;
                    for (int n = 0; n < em->count; n++) FxEmitOne(b, ins, em);
                }
            }
            bool emitting = source_age >= 0.0f && source_age <= em->duration;
            if (ins->loop && !em->one_shot) emitting = !repeating || cycle_age <= em->duration;
            if (em->rate > 0 && emitting) {
                float curve_t = em->duration > 0.001f ? fminf(cycle_age / em->duration, 1.0f) : 0.0f;
                float rate = em->rate * fmaxf(0.0f, FxCurveSample(&em->rate_curve, curve_t, 1.0f));
                ins->accum[e] += fminf(rate, 600.0f) * dt;
                while (ins->accum[e] >= 1.0f) {
                    ins->accum[e] -= 1.0f;
                    FxEmitOne(b, ins, em);
                }
            }
        }
        if (!ins->loop && ins->age >= pre->length) ins->alive = false;
    }

    for (int i = 0; i < b->fxp_count;) {
        FxParticle *p = &b->fxp[i];
        if (p->anchor >= 0) {
            struct BattleRing *r = &b->rings[p->anchor / SQUAD_SLOT_COUNT][p->anchor % SQUAD_SLOT_COUNT];
            p->pos = Vector3Add(p->pos, Vector3Subtract(r->pos, p->anchor_pos));
            p->anchor_pos = r->pos;
        }
        p->age += dt;
        if (p->age >= p->life) {
            *p = b->fxp[--b->fxp_count];
            continue;
        }
        if (p->em->drag > 0.001f) {
            float k = expf(-p->em->drag * dt);
            p->vel = Vector3Scale(p->vel, k);
        }
        p->vel.y -= p->em->gravity * dt;
        p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, dt));
        float spin = p->spin;
        if (p->em->spin_curve.count > 0) spin = DEG2RAD * FxCurveSample(&p->em->spin_curve, p->age / p->life, 0.0f);
        p->rot += spin * dt;
        i++;
    }
}

static void FxQuad(Vector3 c, Vector3 ax1, Vector3 ax2, float u0, float v0, float u1, float v1) {
    rlTexCoord2f(u0, v1);
    rlVertex3f(c.x - ax1.x - ax2.x, c.y - ax1.y - ax2.y, c.z - ax1.z - ax2.z);
    rlTexCoord2f(u1, v1);
    rlVertex3f(c.x + ax1.x - ax2.x, c.y + ax1.y - ax2.y, c.z + ax1.z - ax2.z);
    rlTexCoord2f(u1, v0);
    rlVertex3f(c.x + ax1.x + ax2.x, c.y + ax1.y + ax2.y, c.z + ax1.z + ax2.z);
    rlTexCoord2f(u0, v0);
    rlVertex3f(c.x - ax1.x + ax2.x, c.y - ax1.y + ax2.y, c.z - ax1.z + ax2.z);
}

static const float FX_CYLINDER_U[21] = {
    0.003064275f, 0.065215617f, 0.111829281f, 0.158442736f, 0.205056310f, 0.251669973f, 0.313821256f,
    0.360434920f, 0.407048374f, 0.453662038f, 0.500275671f, 0.562426984f, 0.609040618f, 0.655654132f,
    0.702267766f, 0.748881221f, 0.811032653f, 0.857646346f, 0.904259801f, 0.950873435f, 0.997486949f,
};

static Vector3 FxCylinderRingPoint(float scale, float rot, int ring) {
    float angle = rot + DEG2RAD * (72.0f - ring * 18.0f);
    return (Vector3){cosf(angle) * scale, 0.0f, sinf(angle) * scale};
}

static void FxCylinder(Vector3 c, float scale, float rot, float u0, float v0, float u1, float v1) {
    const int sides = 20;
    const float source_v_pad = 0.0019920766f;
    float tv_bottom = v1 - (v1 - v0) * source_v_pad;
    float tv_top = v0 + (v1 - v0) * source_v_pad;
    for (int i = 0; i < sides; i++) {
        Vector3 p0 = FxCylinderRingPoint(scale, rot, i);
        Vector3 p1 = FxCylinderRingPoint(scale, rot, i + 1);
        float tu0 = u0 + (u1 - u0) * FX_CYLINDER_U[i];
        float tu1 = u0 + (u1 - u0) * FX_CYLINDER_U[i + 1];
        rlTexCoord2f(tu1, tv_bottom);
        rlVertex3f(c.x + p1.x, c.y, c.z + p1.z);
        rlTexCoord2f(tu0, tv_bottom);
        rlVertex3f(c.x + p0.x, c.y, c.z + p0.z);
        rlTexCoord2f(tu0, tv_top);
        rlVertex3f(c.x + p0.x, c.y + scale * 2.0f, c.z + p0.z);
        rlTexCoord2f(tu1, tv_top);
        rlVertex3f(c.x + p1.x, c.y + scale * 2.0f, c.z + p1.z);
    }
}

static void BattleProjDraw(Battle *b, Vector3 right, Vector3 up) {
    for (int i = 0; i < b->proj_count; i++) {
        struct BattleProj *pr = &b->proj[i];
        Vector3 p = pr->pos;
        if (pr->look == PROJ_ARROW || pr->look == PROJ_SPIRIT) {
            Vector3 axis = Vector3Length(pr->vel) > 0.01f ? Vector3Normalize(pr->vel) : right;
            Vector3 side = Vector3Normalize(Vector3CrossProduct(axis, up));
            bool arrow = pr->look == PROJ_ARROW;
            Texture2D tex = BattleFxTex(
                b,
                arrow ? CACHE_BUNDLE_RESOURCE("vfx_bolter_pre", "arrowsinglewhite2_tex")
                      : CACHE_BUNDLE_RESOURCE("vfx_pro_spiritball_pre", "llblue_laser_texture"),
                0
            );
            rlSetTexture(tex.id);
            rlBegin(RL_QUADS);
            if (arrow)
                rlColor4ub(255, 96, 30, 255);
            else
                rlColor4ub(200, 230, 255, 255);
            FxQuad(p, Vector3Scale(axis, arrow ? 0.9f : 0.6f), Vector3Scale(side, arrow ? 0.12f : 0.2f), 0, 0, 1, 1);
            rlEnd();
        } else {
            Texture2D tex = BattleFxTex(
                b,
                pr->look == PROJ_DESICCATE ? CACHE_BUNDLE_RESOURCE("fx_c_desiccate_pre", "chargegradient")
                                           : CACHE_BUNDLE_RESOURCE("fx_heal_pre", "Default-Particle"),
                0
            );
            rlSetTexture(tex.id);
            rlBegin(RL_QUADS);
            if (pr->look == PROJ_DESICCATE)
                rlColor4ub(80, 255, 100, 235);
            else
                rlColor4ub(255, 210, 130, 235);
            float h = pr->look == PROJ_DESICCATE ? 0.35f : 0.22f;
            FxQuad(p, Vector3Scale(right, h), Vector3Scale(up, h), 0, 0, 1, 1);
            rlEnd();
        }
    }
}

static void BattleFxDraw(Battle *b) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(b->cam.target, b->cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, b->cam.up));
    Vector3 up = Vector3CrossProduct(right, fwd);

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int blend = 0; blend < 2; blend++) {
        rlSetBlendMode(blend ? BLEND_ADDITIVE : BLEND_ALPHA);
        if (blend && b->proj_count > 0) BattleProjDraw(b, right, up);
        for (int i = 0; i < b->fxp_count; i++) {
            FxParticle *p = &b->fxp[i];
            const FxEmitter *em = p->em;
            if ((int)em->additive != blend || p->tex.id == 0) continue;

            float t = p->age / p->life;
            float a = 1.0f;
            if (em->alpha_curve.count > 0)
                a = FxCurveSample(&em->alpha_curve, t, 1.0f);
            else if (em->alpha_in > 0.001f && t < em->alpha_in)
                a = t / em->alpha_in;
            else if (em->alpha_hold >= 0.999f)
                a = 1.0f;
            else if (t > em->alpha_hold)
                a = (1.0f - t) / (1.0f - em->alpha_hold);
            Color fallback = em->tint.a ? em->tint : (Color){255, 255, 255, 255};
            Color col = FxColorSample(&em->color_curve, t, fallback);
            if (p->start_color.a) {
                col.r = p->start_color.r;
                col.g = p->start_color.g;
                col.b = p->start_color.b;
            }
            col.a = (unsigned char)(fminf(fmaxf(a, 0.0f), 1.0f) * col.a);

            float sm = em->size_curve.count > 0 ? FxCurveSample(&em->size_curve, t, 1.0f)
                                                : (em->size_end > 0.001f ? 1.0f + (em->size_end - 1.0f) * t : 1.0f);
            float diameter = p->size * fmaxf(sm, 0.0f);
            if (em->render_scale > 0.001f) diameter *= em->render_scale;
            if (em->max_screen_size > 0.001f) {
                float distance = Vector3Distance(b->cam.position, p->pos);
                float max_world = em->max_screen_size * 2.0f * tanf(DEG2RAD * b->cam.fovy * 0.5f) * distance;
                if (diameter > max_world) diameter = max_world;
            }
            float h = diameter * 0.5f;

            float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
            if (em->cols > 0 && em->rows > 0) {
                int cells = em->cols * em->rows;
                int cell;
                if (em->sheet_cycles > 0.0f) {
                    cell = (int)(t * cells * em->sheet_cycles);
                    cell %= cells;
                } else {
                    cell = em->sheet_once ? (int)(t * cells) : (int)(p->seed % cells);
                }
                if (cell >= cells) cell = cells - 1;
                u0 = (float)(cell % em->cols) / em->cols;
                v0 = (float)(cell / em->cols) / em->rows;
                u1 = u0 + 1.0f / em->cols;
                v1 = v0 + 1.0f / em->rows;
            }

            if (em->orient == 3) {
                rlSetTexture(p->tex.id);
                rlBegin(RL_QUADS);
                rlColor4ub(col.r, col.g, col.b, col.a);
                FxCylinder(p->pos, diameter, p->rot, u0, v0, u1, v1);
                rlEnd();
                continue;
            }

            Vector3 ax1, ax2;
            float cr = cosf(p->rot), sr = sinf(p->rot);
            if (em->orient == 1) {
                ax1 = (Vector3){cr * h, 0, sr * h};
                ax2 = (Vector3){-sr * h, 0, cr * h};
            } else if (em->orient == 2) {
                Vector3 axis = Vector3Length(p->vel) > 0.005f ? Vector3Normalize(p->vel) : up;
                Vector3 side = Vector3CrossProduct(fwd, axis);
                side = Vector3Length(side) > 0.001f ? Vector3Normalize(side) : right;
                if (em->stretch_length > 0.001f || em->stretch_velocity > 0.001f) {
                    float full_length = diameter * em->stretch_length + Vector3Length(p->vel) * em->stretch_velocity;
                    ax1 = Vector3Scale(axis, full_length * 0.5f);
                    ax2 = Vector3Scale(side, h);
                } else {
                    ax1 = Vector3Scale(axis, h * 2.0f);
                    ax2 = Vector3Scale(side, h * 0.35f);
                }
            } else {
                ax1 = Vector3Scale(Vector3Add(Vector3Scale(right, cr), Vector3Scale(up, sr)), h);
                ax2 = Vector3Scale(Vector3Subtract(Vector3Scale(up, cr), Vector3Scale(right, sr)), h);
            }

            rlSetTexture(p->tex.id);
            rlBegin(RL_QUADS);
            rlColor4ub(col.r, col.g, col.b, col.a);
            FxQuad(p->pos, ax1, ax2, u0, v0, u1, v1);
            rlEnd();
        }
    }
    rlSetTexture(0);
    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
}

static void BattleWiresLoad(Battle *b, const char *path) {
    b->wires = NULL;
    b->wire_count = 0;
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "Battle: %s missing, ability VFX fall back to heuristics", path);
        return;
    }
    cJSON *root = JsonLoadFile(path);
    int n = 0;
    cJSON *character = NULL;
    cJSON_ArrayForEach(character, root) {
        cJSON *tiers = cJSON_GetObjectItemCaseSensitive(character, "tiers");
        for (int tier = 0; tier < TIER_COUNT; tier++)
            if (cJSON_IsObject(cJSON_GetObjectItemCaseSensitive(tiers, TIER_NAMES[tier]))) n++;
    }
    if (n > 0) b->wires = MemAlloc(sizeof(*b->wires) * n);
    cJSON_ArrayForEach(character, root) {
        if (!cJSON_IsObject(character) || !character->string) continue;
        cJSON *tiers = cJSON_GetObjectItemCaseSensitive(character, "tiers");
        for (int tier = 0; tier < TIER_COUNT; tier++) {
            cJSON *entry = cJSON_GetObjectItemCaseSensitive(tiers, TIER_NAMES[tier]);
            if (!cJSON_IsObject(entry)) continue;
            BattleVfxWire *w = &b->wires[b->wire_count++];
            *w = (BattleVfxWire){0};
            StrCopy(w->unit, sizeof(w->unit), character->string);
            w->tier = (Tier)tier;
            w->vfx_type = JsonInt(entry, "vfx_type");
            cJSON *speed = cJSON_GetObjectItemCaseSensitive(entry, "projectile_speed");
            w->pspeed = cJSON_IsNumber(speed) ? (float)speed->valuedouble : 8.0f;
            w->cone_delay = fmaxf(JsonFloat(entry, "cone_delay"), 0.0f);
            w->ability_type = JsonInt(entry, "ability_type");
            StrCopy(w->ability, sizeof(w->ability), JsonStr(entry, "ability"));
            StrCopy(w->impact, sizeof(w->impact), JsonStr(entry, "impact"));
            StrCopy(w->muzzle, sizeof(w->muzzle), JsonStr(entry, "muzzle"));
            StrCopy(w->sustained, sizeof(w->sustained), JsonStr(entry, "sustained"));
        }
    }
    cJSON_Delete(root);
    TraceLog(LOG_INFO, "Battle: %d unit VFX wirings loaded", b->wire_count);
}

static Vector3 JsonVec3(const cJSON *obj, const char *key) {
    Vector3 v = {0};
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsArray(a) && cJSON_GetArraySize(a) == 3) {
        v.x = (float)cJSON_GetArrayItem(a, 0)->valuedouble;
        v.y = (float)cJSON_GetArrayItem(a, 1)->valuedouble;
        v.z = (float)cJSON_GetArrayItem(a, 2)->valuedouble;
    }
    return v;
}

static FxCurve JsonFxCurve(const cJSON *obj, const char *key) {
    FxCurve curve = {0};
    cJSON *keys = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, keys) {
        if (curve.count >= FX_MAX_CURVE_KEYS || !cJSON_IsArray(item) || cJSON_GetArraySize(item) != 2) continue;
        cJSON *time = cJSON_GetArrayItem(item, 0);
        cJSON *value = cJSON_GetArrayItem(item, 1);
        if (!cJSON_IsNumber(time) || !cJSON_IsNumber(value)) continue;
        curve.key[curve.count][0] = (float)time->valuedouble;
        curve.key[curve.count][1] = (float)value->valuedouble;
        curve.count++;
    }
    return curve;
}

static FxColorCurve JsonFxColorCurve(const cJSON *obj, const char *key) {
    FxColorCurve curve = {0};
    cJSON *keys = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, keys) {
        if (curve.count >= FX_MAX_CURVE_KEYS || !cJSON_IsArray(item) || cJSON_GetArraySize(item) != 5) continue;
        cJSON *time = cJSON_GetArrayItem(item, 0);
        if (!cJSON_IsNumber(time)) continue;
        FxColorKey *out = &curve.key[curve.count];
        out->time = (float)time->valuedouble;
        out->color = (Color){
            (unsigned char)cJSON_GetArrayItem(item, 1)->valueint,
            (unsigned char)cJSON_GetArrayItem(item, 2)->valueint,
            (unsigned char)cJSON_GetArrayItem(item, 3)->valueint,
            (unsigned char)cJSON_GetArrayItem(item, 4)->valueint,
        };
        curve.count++;
    }
    return curve;
}

static void BattleFxRegistryLoad(Battle *b, const char *path) {
    b->fx_reg = NULL;
    b->fx_reg_count = 0;
    b->fx_alias = NULL;
    b->fx_alias_count = 0;
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "Battle: %s missing, per-unit VFX fall back to families", path);
        return;
    }
    cJSON *root = JsonLoadFile(path);
    cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "presets");
    int n = cJSON_GetArraySize(arr);
    if (n > 0) b->fx_reg = MemAlloc(sizeof(FxRegPreset) * n);
    cJSON *p = NULL;
    cJSON_ArrayForEach(p, arr) {
        if (b->fx_reg_count >= n) break;
        FxRegPreset *r = &b->fx_reg[b->fx_reg_count];
        *r = (FxRegPreset){0};
        StrCopy(r->name, sizeof(r->name), JsonStr(p, "name"));
        r->pre.name = r->name;
        r->pre.length = JsonFloat(p, "length");
        cJSON *ems = cJSON_GetObjectItemCaseSensitive(p, "emitters");
        cJSON *em = NULL;
        cJSON_ArrayForEach(em, ems) {
            if (r->pre.em_count >= FX_MAX_EMITTERS) break;
            int i = r->pre.em_count++;
            FxEmitter *e = &r->pre.em[i];
            StrCopy(r->tex[i], sizeof(r->tex[i]), JsonStr(em, "tex"));
            e->tex = r->tex[i];
            e->additive = (unsigned char)JsonInt(em, "additive");
            e->keyout = (unsigned char)JsonInt(em, "keyout");
            e->orient = (unsigned char)JsonInt(em, "orient");
            e->cols = (unsigned char)JsonInt(em, "cols");
            e->rows = (unsigned char)JsonInt(em, "rows");
            e->sheet_once = (unsigned char)JsonInt(em, "sheet_once");
            e->sheet_cycles = JsonFloat(em, "sheet_cycles");
            e->max_screen_size = JsonFloat(em, "max_screen_size");
            e->render_scale = JsonFloat(em, "render_scale");
            e->stretch_length = JsonFloat(em, "stretch_length");
            e->stretch_velocity = JsonFloat(em, "stretch_velocity");
            e->one_shot = (unsigned char)JsonInt(em, "one_shot");
            e->prewarm = (unsigned char)JsonInt(em, "prewarm");
            cJSON *birth = cJSON_GetObjectItemCaseSensitive(em, "birth_emitter");
            if (cJSON_IsArray(birth))
                for (int child = 0; child < 2 && child < cJSON_GetArraySize(birth); child++)
                    e->birth_emitter[child] = (unsigned char)cJSON_GetArrayItem(birth, child)->valueint;
            e->max_particles = (unsigned short)JsonInt(em, "max_particles");
            e->count = JsonInt(em, "count");
            e->rate = JsonFloat(em, "rate");
            e->rate_curve = JsonFxCurve(em, "rate_curve");
            e->delay = JsonFloat(em, "delay");
            e->duration = JsonFloat(em, "duration");
            e->period = JsonFloat(em, "period");
            e->life0 = JsonFloat(em, "life0");
            e->life1 = JsonFloat(em, "life1");
            e->speed0 = JsonFloat(em, "speed0");
            e->speed1 = JsonFloat(em, "speed1");
            e->dir = JsonVec3(em, "dir");
            e->cone_deg = JsonFloat(em, "cone_deg");
            e->radius = JsonFloat(em, "radius");
            e->box = JsonVec3(em, "box");
            e->drag = JsonFloat(em, "drag");
            e->gravity = JsonFloat(em, "gravity");
            e->size0 = JsonFloat(em, "size0");
            e->size1 = JsonFloat(em, "size1");
            e->size_end = JsonFloat(em, "size_end");
            e->size_curve = JsonFxCurve(em, "size_curve");
            e->alpha_in = JsonFloat(em, "alpha_in");
            e->alpha_hold = JsonFloat(em, "alpha_hold");
            e->alpha_curve = JsonFxCurve(em, "alpha_curve");
            cJSON *t = cJSON_GetObjectItemCaseSensitive(em, "tint");
            if (cJSON_IsArray(t) && cJSON_GetArraySize(t) == 4)
                e->tint = (Color){
                    (unsigned char)cJSON_GetArrayItem(t, 0)->valueint,
                    (unsigned char)cJSON_GetArrayItem(t, 1)->valueint,
                    (unsigned char)cJSON_GetArrayItem(t, 2)->valueint,
                    (unsigned char)cJSON_GetArrayItem(t, 3)->valueint
                };
            e->color_curve = JsonFxColorCurve(em, "color_curve");
            e->rot_deg = JsonFloat(em, "rot_deg");
            e->spin_deg = JsonFloat(em, "spin_deg");
            e->rot_bias_deg = JsonFloat(em, "rot_bias_deg");
            e->spin_bias_deg = JsonFloat(em, "spin_bias_deg");
            e->spin_curve = JsonFxCurve(em, "spin_curve");
            e->offset = JsonVec3(em, "offset");
        }
        if (r->pre.em_count > 0) b->fx_reg_count++;
    }
    cJSON *aliases = cJSON_GetObjectItemCaseSensitive(root, "aliases");
    int an = 0;
    cJSON *al = NULL;
    cJSON_ArrayForEach(al, aliases) an++;
    if (an > 0) {
        b->fx_alias = MemAlloc(sizeof(FxAlias) * an);
        cJSON_ArrayForEach(al, aliases) {
            if (!cJSON_IsString(al) || !al->string) continue;
            FxAlias *a = &b->fx_alias[b->fx_alias_count++];
            StrCopy(a->from, sizeof(a->from), al->string);
            StrCopy(a->to, sizeof(a->to), al->valuestring);
        }
    }
    cJSON_Delete(root);
    TraceLog(LOG_INFO, "Battle: %d converted FX presets, %d aliases loaded", b->fx_reg_count, b->fx_alias_count);
}

const BattleVfxWire *BattleUnitWire(const Battle *b, const char *unit_id, Tier tier) {
    for (int i = 0; i < b->wire_count; i++)
        if (b->wires[i].tier == tier && strcmp(b->wires[i].unit, unit_id) == 0) return &b->wires[i];
    return NULL;
}

static const char *const UNIT_SFX_NAMES[UNIT_SFX_COUNT] = {
    [UNIT_SFX_MOVE_OUT] = "move_out",
    [UNIT_SFX_MOVE_BACK] = "move_back",
    [UNIT_SFX_LAND] = "land",
    [UNIT_SFX_VOCAL] = "vocal",
    [UNIT_SFX_ANIMATION] = "animation",
    [UNIT_SFX_ATTACK] = "attack",
    [UNIT_SFX_IMPACT] = "impact",
    [UNIT_SFX_DEATH] = "death",
};

static int BattleAudioClipIntern(Battle *b, const char *name) {
    for (int i = 0; i < b->unit_audio_clip_count; i++)
        if (!strcmp(b->unit_audio_clips[i], name)) return i;
    if (b->unit_audio_clip_count > 0xFFFF) return -1;
    StrCopy(b->unit_audio_clips[b->unit_audio_clip_count], sizeof(b->unit_audio_clips[0]), name);
    return b->unit_audio_clip_count++;
}

static void BattleAudioManifestUnload(Battle *b) {
    if (b->unit_audio_characters) MemFree(b->unit_audio_characters);
    if (b->unit_audio_profiles) MemFree(b->unit_audio_profiles);
    if (b->unit_audio_sources) MemFree(b->unit_audio_sources);
    if (b->unit_audio_clips) MemFree(b->unit_audio_clips);
    b->unit_audio_characters = NULL;
    b->unit_audio_profiles = NULL;
    b->unit_audio_sources = NULL;
    b->unit_audio_clips = NULL;
    b->unit_audio_character_count = b->unit_audio_profile_count = 0;
    b->unit_audio_source_count = b->unit_audio_clip_count = 0;
}

static void BattleAudioManifestLoad(Battle *b, const char *path) {
    BattleAudioManifestUnload(b);
    cJSON *root = JsonLoadFile(path);
    if (JsonInt(root, "version") != 1) TraceLog(LOG_FATAL, "Battle: unsupported unit audio manifest %s", path);
    cJSON *characters = cJSON_GetObjectItemCaseSensitive(root, "characters");
    cJSON *profiles = cJSON_GetObjectItemCaseSensitive(root, "profiles");
    cJSON *sources = cJSON_GetObjectItemCaseSensitive(root, "audio_sources");
    if (!cJSON_IsObject(characters) || !cJSON_IsObject(profiles) || !cJSON_IsObject(sources))
        TraceLog(LOG_FATAL, "Battle: malformed unit audio manifest %s", path);

    int n_sources = 0, n_profiles = 0, n_characters = 0, n_clip_refs = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, sources) n_sources++;
    cJSON_ArrayForEach(item, characters) n_characters++;
    cJSON_ArrayForEach(item, profiles) {
        n_profiles++;
        cJSON *pool = NULL;
        cJSON_ArrayForEach(pool, item) n_clip_refs +=
            cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(pool, "clips"));
    }
    if (n_sources > 0) b->unit_audio_sources = MemAlloc(sizeof(*b->unit_audio_sources) * n_sources);
    if (n_profiles > 0) b->unit_audio_profiles = MemAlloc(sizeof(*b->unit_audio_profiles) * n_profiles);
    if (n_characters > 0) b->unit_audio_characters = MemAlloc(sizeof(*b->unit_audio_characters) * n_characters);
    if (n_clip_refs > 0) b->unit_audio_clips = MemAlloc(sizeof(*b->unit_audio_clips) * n_clip_refs);

    cJSON_ArrayForEach(item, sources) {
        if (!cJSON_IsString(item) || !item->string) continue;
        BattleAudioSourceDef *source = &b->unit_audio_sources[b->unit_audio_source_count++];
        StrCopy(source->name, sizeof(source->name), item->string);
        StrCopy(source->ref, sizeof(source->ref), item->valuestring);
    }
    cJSON_ArrayForEach(item, profiles) {
        if (!cJSON_IsObject(item) || !item->string) continue;
        BattleAudioProfileDef *p = &b->unit_audio_profiles[b->unit_audio_profile_count++];
        *p = (BattleAudioProfileDef){0};
        StrCopy(p->name, sizeof(p->name), item->string);
        for (int kind = 0; kind < UNIT_SFX_COUNT; kind++) {
            BattleAudioPoolDef *pool = &p->pools[kind];
            cJSON *obj = cJSON_GetObjectItemCaseSensitive(item, UNIT_SFX_NAMES[kind]);
            cJSON *clips = cJSON_GetObjectItemCaseSensitive(obj, "clips");
            if (!cJSON_IsObject(obj) || !cJSON_IsArray(clips)) {
                TraceLog(
                    LOG_WARNING, "Battle: malformed %s pool in unit-audio profile %s", UNIT_SFX_NAMES[kind], p->name
                );
                continue;
            }
            cJSON *volume = cJSON_GetObjectItemCaseSensitive(obj, "volume");
            pool->volume = cJSON_IsNumber(volume) ? (float)volume->valuedouble : 1.0f;
            cJSON *clip = NULL;
            cJSON_ArrayForEach(clip, clips) {
                if (!cJSON_IsString(clip) || !clip->valuestring[0]) continue;
                if (pool->count >= BATTLE_UNIT_AUDIO_MAX_CLIPS) {
                    TraceLog(
                        LOG_WARNING, "Battle: oversized %s pool in unit-audio profile %s", UNIT_SFX_NAMES[kind], p->name
                    );
                    break;
                }
                int index = BattleAudioClipIntern(b, clip->valuestring);
                if (index >= 0) pool->clips[pool->count++] = (unsigned short)index;
            }
        }
    }
    cJSON_ArrayForEach(item, characters) {
        if (!cJSON_IsObject(item) || !item->string) continue;
        BattleAudioUnitDef *u = &b->unit_audio_characters[b->unit_audio_character_count++];
        *u = (BattleAudioUnitDef){0};
        StrCopy(u->unit, sizeof(u->unit), item->string);
        u->attack_capable = JsonBool(item, "attack_capable");
        cJSON *tiers = cJSON_GetObjectItemCaseSensitive(item, "tiers");
        for (int tier = 0; tier < TIER_COUNT; tier++) {
            cJSON *binding = cJSON_GetObjectItemCaseSensitive(tiers, TIER_NAMES[tier]);
            if (!cJSON_IsObject(binding)) continue;
            u->has_tier[tier] = true;
            StrCopy(u->prefab[tier], sizeof(u->prefab[tier]), JsonStr(binding, "prefab"));
            StrCopy(u->profile[tier], sizeof(u->profile[tier]), JsonStr(binding, "profile"));
        }
    }
    cJSON_Delete(root);
    if (b->unit_audio_clip_count > 0)
        b->unit_audio_clips = MemRealloc(b->unit_audio_clips, sizeof(*b->unit_audio_clips) * b->unit_audio_clip_count);

    const char *trace = getenv("HOD_AUDIO_TRACE");
    b->audio_trace = trace && strcmp(trace, "0") != 0;
    TraceLog(
        LOG_INFO,
        "Battle: %d unit-audio characters, %d profiles, %d clip names loaded",
        b->unit_audio_character_count,
        b->unit_audio_profile_count,
        b->unit_audio_clip_count
    );
}

static const BattleAudioUnitDef *BattleAudioUnitFind(const Battle *b, const char *unit_id) {
    for (int i = 0; i < b->unit_audio_character_count; i++)
        if (!strcmp(b->unit_audio_characters[i].unit, unit_id)) return &b->unit_audio_characters[i];
    return NULL;
}

static const BattleAudioProfileDef *BattleAudioProfileFind(const Battle *b, const char *name) {
    for (int i = 0; i < b->unit_audio_profile_count; i++)
        if (!strcmp(b->unit_audio_profiles[i].name, name)) return &b->unit_audio_profiles[i];
    return NULL;
}

static const char *BattleAudioSourceRef(const Battle *b, const char *name) {
    for (int i = 0; i < b->unit_audio_source_count; i++)
        if (!strcasecmp(b->unit_audio_sources[i].name, name)) return b->unit_audio_sources[i].ref;
    return TextFormat(CACHE_RESOURCE("%s"), name);
}

static bool BattleSfxReserve(Battle *b, int count) {
    if (count <= b->sfx_capacity) return true;
    int capacity = b->sfx_capacity > 0 ? b->sfx_capacity : 64;
    while (capacity < count && capacity < BATTLE_SFX_CACHE_MAX) capacity *= 2;
    if (capacity > BATTLE_SFX_CACHE_MAX) capacity = BATTLE_SFX_CACHE_MAX;
    if (capacity < count) {
        TraceLog(LOG_WARNING, "Battle: SFX cache limit reached (%d)", BATTLE_SFX_CACHE_MAX);
        return false;
    }
    BattleSfx *entries =
        b->sfx ? MemRealloc(b->sfx, capacity * sizeof(*entries)) : MemAlloc(capacity * sizeof(*entries));
    if (!entries) {
        TraceLog(LOG_WARNING, "Battle: could not grow SFX cache to %d entries", capacity);
        return false;
    }
    b->sfx = entries;
    b->sfx_capacity = capacity;
    return true;
}

static BattleSfx *BattleSfxGet(Battle *b, const char *name) {
    if (!name || !name[0]) return NULL;
    for (int i = 0; i < b->sfx_count; i++)
        if (!strcmp(b->sfx[i].name, name)) return &b->sfx[i];
    if (!BattleSfxReserve(b, b->sfx_count + 1)) return NULL;

    BattleSfx *entry = &b->sfx[b->sfx_count++];
    memset(entry, 0, sizeof(*entry));
    StrCopy(entry->name, sizeof(entry->name), name);

    const char *ref = BattleAudioSourceRef(b, name);
    if (!ResourceExists(ref)) {
        TraceLog(LOG_WARNING, "Battle: audio pool has no clip %s (%s)", name, ref);
        return entry;
    }
    entry->source = ResourceLoadSound(ref);
    if (!IsSoundValid(entry->source)) TraceLog(LOG_WARNING, "Battle: failed to load SFX %s", ref);
    return entry;
}

static bool BattleSfxPlay(Battle *b, const char *name, float volume) {
    BattleSfx *entry = BattleSfxGet(b, name);
    if (!entry || !IsSoundValid(entry->source)) return false;

    Sound *voice = &entry->source;
    if (IsSoundPlaying(*voice)) {
        voice = NULL;
        for (int i = 0; i < entry->alias_count; i++) {
            if (!IsSoundPlaying(entry->aliases[i])) {
                voice = &entry->aliases[i];
                break;
            }
        }
        if (!voice && entry->alias_count < BATTLE_SFX_ALIAS_MAX) {
            Sound alias = LoadSoundAlias(entry->source);
            if (IsSoundValid(alias)) {
                entry->aliases[entry->alias_count] = alias;
                voice = &entry->aliases[entry->alias_count++];
            }
        }
        if (!voice) {
            int index = entry->voice_cursor++ % (entry->alias_count + 1);
            voice = index == 0 ? &entry->source : &entry->aliases[index - 1];
        }
    }
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    SetSoundVolume(*voice, volume);
    PlaySound(*voice);
    return true;
}

static void BattleAudioCacheClear(Battle *b) {
    for (int i = 0; i < b->sfx_count; i++) {
        BattleSfx *entry = &b->sfx[i];
        for (int a = 0; a < entry->alias_count; a++)
            if (IsSoundValid(entry->aliases[a])) UnloadSoundAlias(entry->aliases[a]);
        if (IsSoundValid(entry->source)) UnloadSound(entry->source);
    }
    if (b->sfx) MemFree(b->sfx);
    b->sfx = NULL;
    b->sfx_count = 0;
    b->sfx_capacity = 0;
}

static void BattleAudioPoolResolve(const Battle *b, const BattleAudioPoolDef *def, BattleAudioPool *out) {
    memset(out, 0, sizeof(*out));
    out->volume = def->volume;
    for (int i = 0; i < def->count; i++) out->clips[out->count++] = b->unit_audio_clips[def->clips[i]];
}

static bool BattleAudioBindUnit(Battle *b, int side, int slot, const Character *ch) {
    BattleUnitAudio *unit = &b->unit_audio[side][slot];
    memset(unit, 0, sizeof(*unit));
    if (!ch) return true;
    StrCopy(unit->unit, sizeof(unit->unit), ch->id);

    const BattleAudioUnitDef *def = BattleAudioUnitFind(b, ch->id);
    if (!def || !def->has_tier[ch->tier]) {
        TraceLog(LOG_WARNING, "Battle: no unit-audio mapping for %s tier %s", ch->id, TIER_NAMES[ch->tier]);
        return false;
    }
    unit->attack_capable = def->attack_capable;
    StrCopy(unit->prefab, sizeof(unit->prefab), def->prefab[ch->tier]);
    StrCopy(unit->profile, sizeof(unit->profile), def->profile[ch->tier]);
    if (!unit->profile[0]) {
        if (unit->attack_capable) {
            TraceLog(LOG_WARNING, "Battle: attack-capable %s has no unit-audio profile", ch->id);
            return false;
        }
        unit->bound = true;
        return true;
    }

    const BattleAudioProfileDef *profile = BattleAudioProfileFind(b, unit->profile);
    if (!profile) {
        TraceLog(LOG_WARNING, "Battle: unit-audio profile %s for %s is missing", unit->profile, ch->id);
        return false;
    }
    for (int kind = 0; kind < UNIT_SFX_COUNT; kind++)
        BattleAudioPoolResolve(b, &profile->pools[kind], &unit->pools[kind]);
    if (unit->attack_capable && unit->pools[UNIT_SFX_ATTACK].count == 0) {
        TraceLog(LOG_WARNING, "Battle: attack-capable %s has an empty attack-audio pool", ch->id);
        return false;
    }
    unit->bound = true;
    return true;
}

static void BattleAudioPrewarm(Battle *b) {
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            const BattleUnitAudio *unit = &b->unit_audio[side][slot];
            if (!unit->bound) continue;
            for (int kind = 0; kind < UNIT_SFX_COUNT; kind++)
                for (int i = 0; i < unit->pools[kind].count; i++) BattleSfxGet(b, unit->pools[kind].clips[i]);
        }
    }
}

static void BattleAudioBindSquads(Battle *b, const Player *player, const BattleRoster *enemy) {
    memset(b->unit_audio, 0, sizeof(b->unit_audio));
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            const Character *ch =
                side == 0 ? PlayerSquadCharacter(player, slot) : (enemy->present[slot] ? &enemy->chars[slot] : NULL);
            BattleAudioBindUnit(b, side, slot, ch);
        }
    }
    BattleAudioPrewarm(b);
    TraceLog(LOG_INFO, "Battle: %d unit/global SFX clips prewarmed", b->sfx_count);
}

void BattleAudioPlayGlobal(Battle *b, const char *name) {
    BattleSfxPlay(b, name, 1.0f);
}

static void BattleAudioPlayFallback(Battle *b, const BattleUnitAudio *unit, UnitSfx kind) {
    char name[48] = {0};
    switch (kind) {
        case UNIT_SFX_MOVE_OUT:
            snprintf(name, sizeof(name), "fly_move_leather%d", GetRandomValue(1, 4));
            break;
        case UNIT_SFX_MOVE_BACK:
            snprintf(name, sizeof(name), "fly_move_leather%d", GetRandomValue(5, 8));
            break;
        case UNIT_SFX_LAND:
            snprintf(name, sizeof(name), "fly_basedrop_stone%d", GetRandomValue(1, 4));
            break;
        case UNIT_SFX_ATTACK:
            snprintf(name, sizeof(name), "wep_gen_slice_0%d", GetRandomValue(1, 6));
            break;
        case UNIT_SFX_IMPACT:
            StrCopy(name, sizeof(name), "abi_r_glo_impactshot1");
            break;
        default:
            return;
    }
    if (b->audio_trace)
        TraceLog(
            LOG_INFO,
            "BattleAudio: unit=%s prefab=%s profile=<fallback> event=%s clip=%s volume=1.00",
            unit->unit[0] ? unit->unit : "<unbound>",
            unit->prefab[0] ? unit->prefab : "<unknown>",
            UNIT_SFX_NAMES[kind],
            name
        );
    BattleSfxPlay(b, name, 1.0f);
}

void BattleAudioPlayUnit(Battle *b, int side, int slot, int kind) {
    if (side < 0 || side > 1 || slot < 0 || slot >= SQUAD_SLOT_COUNT || kind < 0 || kind >= UNIT_SFX_COUNT) {
        TraceLog(LOG_WARNING, "Battle: invalid unit-audio event side=%d slot=%d kind=%d", side, slot, kind);
        return;
    }
    BattleUnitAudio *unit = &b->unit_audio[side][slot];
    if (!unit->bound) {
        BattleAudioPlayFallback(b, unit, (UnitSfx)kind);
        return;
    }
    const BattleAudioPool *pool = &unit->pools[kind];
    if (pool->count == 0) return;
    const char *clip = pool->clips[GetRandomValue(0, pool->count - 1)];
    if (b->audio_trace)
        TraceLog(
            LOG_INFO,
            "BattleAudio: unit=%s prefab=%s profile=%s event=%s clip=%s volume=%.2f",
            unit->unit,
            unit->prefab,
            unit->profile,
            UNIT_SFX_NAMES[kind],
            clip,
            pool->volume
        );
    BattleSfxPlay(b, clip, pool->volume);
}

void BattleAudioCombatMusic(Battle *b) {
    StopMusicStream(b->music_intro);
    StopMusicStream(b->music_loop);
    b->combat_intro = GuiMusic(MUS_COMBAT_INTRO);
    b->combat_loop = GuiMusic((MusicId)(MUS_COMBAT_1 + GetRandomValue(0, 2)));
    PlayMusicStream(b->combat_intro);
    b->combat_music_state = 1;
}

void BattleAudioResultMusic(Battle *b, bool won) {
    StopMusicStream(b->combat_intro);
    StopMusicStream(b->combat_loop);
    b->result_music = GuiMusic(won ? MUS_VICTORY : MUS_DEFEAT);
    PlayMusicStream(b->result_music);
    b->combat_music_state = 3;
}

void BattleAudioStop(Battle *b) {
    if (b->combat_music_state == 0) return;
    StopMusicStream(b->combat_intro);
    StopMusicStream(b->combat_loop);
    StopMusicStream(b->result_music);
    b->combat_music_state = 0;
}

void BattleExitNode(Battle *b) {
    memset(b->rings, 0, sizeof(b->rings));
    memset(b->fxi, 0, sizeof(b->fxi));
    memset(b->punch, 0, sizeof(b->punch));
    b->fxp_count = 0;
    b->proj_count = 0;
    for (int i = 0; i < b->fxtex_count; i++)
        if (b->fxtex[i].tex.id > 0) UnloadTexture(b->fxtex[i].tex);
    b->fxtex_count = 0;
    b->filter_amt = 0.0f;
    b->filter_on = false;
    b->cam_shake = (Vector3){0};
    BattleAudioCacheClear(b);
    memset(b->unit_audio, 0, sizeof(b->unit_audio));
    if (!b->squads_loaded) return;
    BattleSquadsDestroy(&b->squads);
    b->squads_loaded = false;
}

void BattleEnterNodeFrom(Battle *b, const Player *player, const char *rosters_path, const char *map_id, int step) {
    BattleExitNode(b);
    BattleRosterLoad(&b->enemy_roster, rosters_path, HOD_CACHE "characters.json", map_id, step);
    BattleAudioBindSquads(b, player, &b->enemy_roster);
    BattleSquadsLoad(&b->squads, player, &b->enemy_roster, HOD_CACHE "meshes", HOD_CACHE "char_pack");
    b->squads_loaded = true;
    b->tactical = false;
    b->combat_camera = false;
    b->tac_t = 0.0f;
    b->combat_cam_t = 0.0f;
    b->cam = b->cam_matchmaking;
    TraceLog(LOG_INFO, "Battle: entered %s step %d", map_id, step);
}

void BattleEnterNode(Battle *b, const Player *player, const char *map_id, int step) {
    BattleEnterNodeFrom(b, player, "data/campaign_rosters.json", map_id, step);
}

void BattleUpdate(Battle *b, float dt) {
    BattleVfxUpdate(b, dt);
    BattleFxUpdate(b, dt);
    float fgoal = b->filter_on ? 1.0f : 0.0f;
    b->filter_amt += (fgoal - b->filter_amt) * fminf(1.0f, 5.0f * dt);
    if (b->squads_loaded) BattleSquadsUpdate(&b->squads, dt);
    if (!IsMusicStreamPlaying(b->ambience)) PlayMusicStream(b->ambience);
    UpdateMusicStream(b->ambience);

    if (b->combat_music_state == 1) {
        if (IsMusicStreamPlaying(b->combat_intro))
            UpdateMusicStream(b->combat_intro);
        else {
            PlayMusicStream(b->combat_loop);
            b->combat_music_state = 2;
        }
    } else if (b->combat_music_state == 2) {
        UpdateMusicStream(b->combat_loop);
    } else if (b->combat_music_state == 3) {
        UpdateMusicStream(b->result_music);
    } else if (!b->music_started) {
        PlayMusicStream(b->music_intro);
        b->music_started = true;
    } else if (IsMusicStreamPlaying(b->music_intro)) {
        UpdateMusicStream(b->music_intro);
    } else {
        if (!IsMusicStreamPlaying(b->music_loop)) PlayMusicStream(b->music_loop);
        UpdateMusicStream(b->music_loop);
    }

    if (!FlycamEnabled()) {
        BattleCameraUpdatePose(b, dt);
        return;
    }
    if (b->combat_camera) b->combat_cam_t = 1.0f;
    UpdateCamera(&b->cam, CAMERA_FREE);
    if (IsKeyDown(KEY_LEFT_BRACKET)) b->cam.fovy = fmaxf(10.0f, b->cam.fovy - 30.0f * dt);
    if (IsKeyDown(KEY_RIGHT_BRACKET)) b->cam.fovy = fminf(120.0f, b->cam.fovy + 30.0f * dt);
    if (IsKeyPressed(KEY_P)) {
        Vector3 p = b->cam.position, t = b->cam.target;
        TraceLog(
            LOG_INFO,
            "CAM pos={%.3ff,%.3ff,%.3ff} target={%.3ff,%.3ff,%.3ff} fovy=%.2ff",
            p.x,
            p.y,
            p.z,
            t.x,
            t.y,
            t.z,
            b->cam.fovy
        );
    }
}

static void BattleQuad(Vector3 c, Vector3 r, Vector3 u, float u0, float v0, float u1, float v1) {
    rlTexCoord2f(u0, v0);
    rlVertex3f(c.x - r.x + u.x, c.y - r.y + u.y, c.z - r.z + u.z);
    rlTexCoord2f(u0, v1);
    rlVertex3f(c.x - r.x - u.x, c.y - r.y - u.y, c.z - r.z - u.z);
    rlTexCoord2f(u1, v1);
    rlVertex3f(c.x + r.x - u.x, c.y + r.y - u.y, c.z + r.z - u.z);
    rlTexCoord2f(u1, v0);
    rlVertex3f(c.x + r.x + u.x, c.y + r.y + u.y, c.z + r.z + u.z);
}

static void BattleVfxDraw(Battle *b) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(b->cam.target, b->cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, b->cam.up));
    Vector3 up = Vector3CrossProduct(right, fwd);

    static const float FIRE_ALPHA[][2] = {{0, 0}, {0.10155f, 1}, {0.87146f, 1}, {1, 0}};
    static const float SMOKE_ALPHA[][2] = {{0, 0}, {0.10155f, 1}, {0.73008f, 1}, {1, 0}};
    static const float SPLASH_ALPHA[][2] = {{0, 0}, {0.17917f, 1}, {0.38437f, 0.957f}, {0.53906f, 1}, {1, 0}};
    static const float SMOKE_R[][2] = {{0, 212 / 255.0f}, {0.72622f, 203 / 255.0f}, {0.99872f, 66 / 255.0f}};
    static const float SMOKE_G[][2] = {{0, 129 / 255.0f}, {0.72622f, 82 / 255.0f}, {0.99872f, 29 / 255.0f}};
    static const float SMOKE_B[][2] = {{0, 53 / 255.0f}, {0.72622f, 16 / 255.0f}, {0.99872f, 15 / 255.0f}};

    rlSetBlendMode(BLEND_ADDITIVE);
    for (int e = 0; e < BATTLE_FIRE_EMITTERS; e++) {
        BattleFireEmitter *fe = &b->fires[e];
        Texture2D tex = fe->type == BFIRE_TONGUES ? b->t_fire : (fe->type == BFIRE_SPARKS ? b->t_dots : b->t_smoke);
        rlSetTexture(tex.id);
        rlBegin(RL_QUADS);
        for (int i = 0; i < fe->alive; i++) {
            BattleParticle *p = &fe->p[i];
            float t = p->age / p->life;
            float sz, cr, cg, cb, ca;
            float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
            if (fe->type == BFIRE_SMOKE) {
                sz = p->size * (0.5f + 0.5f * t);
                ca = BattleEnvelope(SMOKE_ALPHA, 4, t);
                float k = 62 / 255.0f;
                cr = k * BattleEnvelope(SMOKE_R, 3, t);
                cg = k * BattleEnvelope(SMOKE_G, 3, t);
                cb = k * BattleEnvelope(SMOKE_B, 3, t);
            } else {
                sz = p->size * BattleHermite(1.0f, 0, 0.7531645f, -0.7531647f, t);
                ca = BattleEnvelope(FIRE_ALPHA, 4, t);
                if (fe->type == BFIRE_TONGUES) {
                    cr = 1;
                    cg = 1 - t * (1 - 152 / 255.0f);
                    cb = 1 - t * (1 - 114 / 255.0f);
                    int f = (int)(t * 2.0f * 32) % 32;
                    u0 = (f % 8) / 8.0f;
                    v0 = (f / 8) / 4.0f;
                    u1 = u0 + 1 / 8.0f;
                    v1 = v0 + 1 / 4.0f;
                } else {
                    cr = 1;
                    cg = 1 - t * (1 - 101 / 255.0f);
                    cb = 1 - t * (1 - 28 / 255.0f);
                }
            }
            float cs = cosf(p->rot), sn = sinf(p->rot), h = sz * 0.5f;
            Vector3 r2 = Vector3Scale(Vector3Add(Vector3Scale(right, cs), Vector3Scale(up, sn)), h);
            Vector3 u2 = Vector3Scale(Vector3Subtract(Vector3Scale(up, cs), Vector3Scale(right, sn)), h);
            rlColor4ub(
                (unsigned char)(cr * 255),
                (unsigned char)(cg * 255),
                (unsigned char)(cb * 255),
                (unsigned char)(ca * 255)
            );
            BattleQuad(p->pos, r2, u2, u0, v0, u1, v1);
        }
        rlEnd();
    }

    rlSetBlendMode(BLEND_ALPHA_PREMULTIPLY);
    rlSetTexture(b->t_particle.id);
    rlBegin(RL_QUADS);
    rlColor4ub(126, 129, 133, 255);
    for (int i = 0; i < b->rain_alive; i++) {
        BattleParticle *p = &b->rain[i];
        Vector3 dir = Vector3Normalize(p->vel);
        Vector3 side = Vector3Scale(Vector3Normalize(Vector3CrossProduct(dir, fwd)), 0.035f);
        Vector3 tail = Vector3Scale(dir, 0.63f);
        Vector3 mid = Vector3Subtract(p->pos, Vector3Scale(tail, 0.5f));
        BattleQuad(mid, side, Vector3Scale(tail, 0.5f), 0, 0, 1, 1);
    }
    rlEnd();

    rlSetBlendMode(BLEND_ALPHA);
    rlSetTexture(b->t_foam.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < b->splash_alive; i++) {
        BattleParticle *p = &b->splash[i];
        float t = p->age / p->life;
        float ss = 3 * t * t - 2 * t * t * t;
        float h = (0.4107f + (1 - 0.4107f) * ss) * 0.5f;
        rlColor4ub(255, 255, 255, (unsigned char)(BattleEnvelope(SPLASH_ALPHA, 5, t) * 255));
        BattleQuad(p->pos, (Vector3){h, 0, 0}, (Vector3){0, 0, h}, 0, 0, 1, 1);
    }
    rlEnd();
    rlSetTexture(0);
}

static void BattleDrawMesh(Battle *b, BattleMesh *bm, Matrix xform) {
    switch (bm->kind) {
        case BK_BACKDROP:
        case BK_GRASS:
            b->mat_unlit.maps[MATERIAL_MAP_ALBEDO].texture = bm->albedo;
            DrawMesh(bm->mesh, b->mat_unlit, xform);
            break;
        case BK_GROUND:
            DrawMesh(bm->mesh, b->mat_ground, xform);
            break;
        case BK_FOG:
            DrawMesh(bm->mesh, b->mat_fog, xform);
            break;
        default: {
            Vector3 tint = {bm->tint, bm->tint, bm->tint};
            SetShaderValue(b->lm_sh, b->lm_tint_loc, &tint, SHADER_UNIFORM_VEC3);
            SetShaderValue(b->lm_sh, b->lm_cutoff_loc, &bm->cutoff, SHADER_UNIFORM_FLOAT);
            b->mat_lm.maps[MATERIAL_MAP_ALBEDO].texture = bm->albedo;
            DrawMesh(bm->mesh, b->mat_lm, xform);
        } break;
    }
}

void BattleDraw(Battle *b) {
    ClearBackground(BLACK);
    Matrix xform = MatrixScale(1.0f, 1.0f, -1.0f);

    rlSetClipPlanes(0.30, 300.0);
    BeginMode3D(b->cam);
    rlDisableBackfaceCulling();

    static const BattleKind PASS_KINDS[5][2] = {
        {BK_BACKDROP, -1}, {BK_LIGHTMAP, BK_GROUND}, {BK_CUTOUT, -1}, {BK_FOG, -1}, {BK_GRASS, -1}
    };
    for (int pass = 0; pass < 5; pass++) {
        if (pass == 3) {
            if (b->squads_loaded) BattleSquadsDraw(&b->squads);
            if (b->filter_amt > 0.01f && b->squads_loaded) {
                Vector3 fwd = Vector3Normalize(Vector3Subtract(b->cam.target, b->cam.position));
                Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, b->cam.up));
                Vector3 up = Vector3CrossProduct(right, fwd);
                Vector3 c = Vector3Add(b->cam.position, Vector3Scale(fwd, 0.5f));
                float hh = 0.5f * tanf(DEG2RAD * b->cam.fovy * 0.5f) * 3.0f;
                float hw = hh * 4.0f;
                rlDrawRenderBatchActive();
                rlDisableDepthTest();
                rlDisableDepthMask();
                rlSetBlendMode(BLEND_ALPHA);
                rlSetTexture(0);
                rlBegin(RL_QUADS);
                rlColor4ub(26, 0, 0, (unsigned char)(204.0f * b->filter_amt));
                rlTexCoord2f(0, 1);
                rlVertex3f(
                    c.x - right.x * hw - up.x * hh, c.y - right.y * hw - up.y * hh, c.z - right.z * hw - up.z * hh
                );
                rlTexCoord2f(1, 1);
                rlVertex3f(
                    c.x + right.x * hw - up.x * hh, c.y + right.y * hw - up.y * hh, c.z + right.z * hw - up.z * hh
                );
                rlTexCoord2f(1, 0);
                rlVertex3f(
                    c.x + right.x * hw + up.x * hh, c.y + right.y * hw + up.y * hh, c.z + right.z * hw + up.z * hh
                );
                rlTexCoord2f(0, 0);
                rlVertex3f(
                    c.x - right.x * hw + up.x * hh, c.y - right.y * hw + up.y * hh, c.z - right.z * hw + up.z * hh
                );
                rlEnd();
                rlDrawRenderBatchActive();
                rlEnableDepthTest();
                rlEnableDepthMask();
                for (int side = 0; side < 2; side++)
                    for (int s = 0; s < SQUAD_SLOT_COUNT; s++)
                        if (b->punch[side][s] && b->squads.present[side][s])
                            SquadPadDraw(
                                &b->squads.pads,
                                b->squads.pad_tex[side][s],
                                b->squads.pad_xform[side][s],
                                b->squads.stars[side][s],
                                b->squads.fac_tex[side][s],
                                b->squads.fac_xform[side][s]
                            );
                rlEnableBackfaceCulling();
                rlSetCullFace(RL_CULL_FACE_BACK);
                for (int side = 0; side < 2; side++)
                    for (int s = 0; s < SQUAD_SLOT_COUNT; s++)
                        if (b->punch[side][s] && b->squads.present[side][s])
                            HomeSquadUnitDraw(&b->squads.units[side][s]);
                rlDisableBackfaceCulling();
            }
            rlDisableDepthMask();
            for (int side = 0; side < 2; side++) {
                for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
                    struct BattleRing *r = &b->rings[side][s];
                    if (!r->on || r->pct <= 0.001f) continue;
                    static const Color RING_COLORS[5] = {
                        {20, 212, 0, 255},
                        {210, 234, 24, 255},
                        {223, 163, 8, 255},
                        {221, 77, 10, 255},
                        {231, 30, 0, 255}
                    };
                    int ci = (int)lroundf((1.0f - r->pct) * 4.0f);
                    if (ci < 0) ci = 0;
                    if (ci > 4) ci = 4;
                    float cutoff = 1.0f - r->pct;
                    Vector4 col = {
                        RING_COLORS[ci].r / 255.0f, RING_COLORS[ci].g / 255.0f, RING_COLORS[ci].b / 255.0f, 1.0f
                    };
                    SetShaderValue(b->ring_sh, b->ring_cutoff_loc, &cutoff, SHADER_UNIFORM_FLOAT);
                    SetShaderValue(b->ring_sh, b->ring_color_loc, &col, SHADER_UNIFORM_VEC4);
                    float k = 0.85f * r->scale;
                    Matrix m = MatrixMultiply(
                        MatrixMultiply(MatrixTranslate(-0.0311f, 0.0f, -0.0433f), MatrixScale(k, 1.0f, k)),
                        MatrixTranslate(r->pos.x, r->pos.y + 0.012f, r->pos.z)
                    );
                    DrawMesh(b->ring_mesh, b->ring_mat, m);
                }
            }
            rlEnableDepthMask();
            BattleFxDraw(b);
            rlDrawRenderBatchActive();
            rlDisableDepthMask();
            rlSetBlendMode(BLEND_ADDITIVE);
        }
        if (pass == 4) rlSetBlendMode(BLEND_ALPHA);
        for (int i = 0; i < b->mesh_count; i++) {
            BattleKind k = b->meshes[i].kind;
            if (k != PASS_KINDS[pass][0] && k != PASS_KINDS[pass][1]) continue;
            BattleDrawMesh(b, &b->meshes[i], xform);
        }
    }
    BattleVfxDraw(b);
    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();

    EndMode3D();
    rlEnableBackfaceCulling();
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);

    if (FlycamEnabled()) {
        Vector3 p = b->cam.position, t = b->cam.target;
        const char *info = TextFormat(
            "FLYCAM pos (%.2f,%.2f,%.2f) target (%.2f,%.2f,%.2f) fovy %.1f", p.x, p.y, p.z, t.x, t.y, t.z, b->cam.fovy
        );
        int fs = 14, tw = MeasureText(info, fs), bx = (GetScreenWidth() - tw) / 2;
        DrawRectangle(bx, 150, tw + 20, fs + 12, (Color){0, 0, 0, 160});
        DrawText(info, bx + 10, 156, fs, RAYWHITE);
    }
}

void BattleStopAmbience(Battle *b) {
    StopMusicStream(b->ambience);
    StopMusicStream(b->music_intro);
    StopMusicStream(b->music_loop);
    BattleAudioStop(b);
    b->music_started = false;
}

void BattleDestroy(Battle *b) {
    BattleExitNode(b);
    for (int i = 0; i < b->fxtex_count; i++) UnloadTexture(b->fxtex[i].tex);
    BattleAudioManifestUnload(b);
    if (b->wires) MemFree(b->wires);
    if (b->fx_reg) MemFree(b->fx_reg);
    if (b->fx_alias) MemFree(b->fx_alias);
    SceneMaterialUnload(b->ring_mat);
    UnloadMesh(b->ring_mesh);
    UnloadTexture(b->ring_tex);
    UnloadShader(b->ring_sh);
    UnloadTexture(b->t_particle);
    UnloadTexture(b->t_foam);
    UnloadTexture(b->t_fire);
    UnloadTexture(b->t_dots);
    UnloadTexture(b->t_smoke);
    RL_FREE(b->mat_lm.maps);
    RL_FREE(b->mat_ground.maps);
    RL_FREE(b->mat_unlit.maps);
    RL_FREE(b->mat_fog.maps);
    for (int i = 0; i < b->mesh_count; i++) UnloadMesh(b->meshes[i].mesh);
    UnloadTexture(b->lightmap);
    UnloadTexture(b->t_woodfort);
    UnloadTexture(b->t_rock);
    UnloadTexture(b->t_forest);
    UnloadTexture(b->t_grass);
    UnloadTexture(b->t_backdrop);
    UnloadTexture(b->t_fairy);
    UnloadTexture(b->t_grassbase);
    UnloadTexture(b->t_dirt);
    UnloadTexture(b->t_scorched);
    UnloadTexture(b->t_dgrass);
    UnloadTexture(b->t_mask);
    UnloadShader(b->lm_sh);
    UnloadShader(b->ground_sh);
    UnloadShader(b->unlit_sh);
    UnloadShader(b->fog_sh);
}
