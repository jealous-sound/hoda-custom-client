typedef enum { HR_SKIP, HR_TERRAIN, HR_PROP, HR_GRASS, HR_FERN, HR_CLOTH } HeroRole;

typedef struct {
    Vector3 pos;
    float yaw_deg, diameter;
} HeroFormationSlot;
static const HeroFormationSlot HERO_FORMATION_SLOTS[] = {
    {{29.5238f, 1.0088f, 15.9947f}, 87.87f, 1.7756f},
    {{27.2248f, 1.0088f, 16.7325f}, -54.59f, 1.7756f},
    {{29.2759f, 1.0088f, 13.8581f}, 32.83f, 1.7756f},
    {{27.1047f, 1.0088f, 14.5371f}, 0.0f, 1.7756f},
    {{28.3563f, 1.0088f, 19.7945f}, -97.77f, 2.9813f},
};
#define HERO_FORMATION_SLOT_COUNT ((int)(sizeof(HERO_FORMATION_SLOTS) / sizeof(HERO_FORMATION_SLOTS[0])))

#define HERO_DECAL_LIFT (0.02f)

#define HERO_UNIT_YAW (DEG2RAD * -91.04f)
#define HERO_GIANT_SCALE (1.5f)

static HeroRole HeroRoleForMat(int m) {
    switch (m) {
        case 4:
            return HR_TERRAIN;
        case 3:
            return HR_PROP;
        case 5:
            return HR_PROP;
        case 7:
            return HR_PROP;
        case 9:
            return HR_GRASS;
        case 1:
            return HR_FERN;
        case 6:
            return HR_CLOTH;
        default:
            return HR_SKIP;
    }
}

#define HERO_MAX_MESH 256

static const char *const FS_HERO_MIX =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D texture1;\n"
    "uniform sampler2D texture2;\n"
    "uniform float uBright;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec3 g = texture(texture0, fragTexCoord*5.0).rgb;\n"
    "  vec3 st = texture(texture1, fragTexCoord*7.0).rgb;\n"
    "  float m = texture(texture2, fragTexCoord).r;\n"
    "  vec3 col = mix(g, st, m) * (10.0*(fragColor.rgb + 0.1)) * uBright;\n"
    "  finalColor = vec4(col, 1.0);\n"
    "}\n";
static const char *const FS_HERO_ROCK =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D texture1;\n"
    "uniform float uBright;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  float f = clamp((normalize(fragNormal).y - 0.4148) * 1.9724, 0.0, 1.0);\n"
    "  vec3 base = texture(texture0, fragTexCoord*1.5).rgb;\n"
    "  vec3 gray = texture(texture1, fragTexCoord*1.5).rgb;\n"
    "  vec3 col = mix(gray, base, f) * (16.0*(fragColor.rgb - 0.02)) * uBright;\n"
    "  finalColor = vec4(col, 1.0);\n"
    "}\n";
static const char *const FS_HERO_DIFF =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform float uBright;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec3 col = texture(texture0, fragTexCoord).rgb * (10.0*(fragColor.rgb + 0.1)) * uBright;\n"
    "  finalColor = vec4(col, 1.0);\n"
    "}\n";
static const char *const FS_HERO_WIND =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec3 uTint; uniform float uCutoff, uVcScale, uVcShift;\n"
    "out vec4 finalColor;\n"
    "void main(){ vec4 c = texture(texture0, fragTexCoord); if (c.a < uCutoff) discard;\n"
    "  finalColor = vec4(c.rgb * uTint * (fragColor.rgb*uVcScale + uVcShift), c.a);\n"
    "}\n";
static const char *const FS_HERO_DECAL =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "out vec4 finalColor;\n"
    "void main(){ finalColor = texture(texture0, fragTexCoord); }\n";

static const SceneWind SCENE_WIND_FLAG = {
    {-0.126f, 0.0f, -0.126f, 0.30f},
    {2.4f, 0, -2.4f, 0},
    {2.4f, 2.4f, -2.4f, 0},
    1.0f,
    12.0f,
    1.5f,
    0.328f,
    1.0f,
    0.0f,
    {1, 1, 1},
};

#define HERO_DRAG_PLANE_Y 1.561f

#define HERO_FX_MAX 256

#define HERO_ROW_BONUS_MOVE_TIME 0.5f
#define HERO_ROW_BONUS_DISPLAY_TIME 2.0f
#define HERO_ROW_BONUS_FADE_TIME 0.375f
#define HERO_ROW_BONUS_TOTAL_TIME (HERO_ROW_BONUS_DISPLAY_TIME + HERO_ROW_BONUS_FADE_TIME)

typedef enum { HFX_NONE, HFX_HEAL, HFX_CRIT } HeroFxEffect;

typedef enum {
    HFXP_HEAL_FLASH,
    HFXP_HEAL_RING,
    HFXP_HEAL_DOT,
    HFXP_CRIT_GLOW,
    HFXP_CRIT_SPARK,
    HFXP_CRIT_SMOKE,
    HFXP_CRIT_ARC,
    HFXP_CRIT_ARCGLOW,
    HFXP_CRIT_ARCSTAR,
} HeroFxKind;

typedef enum {
    HFXT_BLOB,
    HFXT_SMOKE3,
    HFXT_SPARKDOT,
    HFXT_FLARE,
    HFXT_SQUARE,
    HFXT_WISPY,
    HFXT_ARC,
    HFXT_RADIAL,
    HFXT_COUNT
} HeroFxTexId;

typedef struct {
    bool alive;
    HeroFxKind kind;
    Vector3 pos, vel;
    float age, life, size, rot, rotv;
    int frame;
} HeroFxParticle;

typedef struct {
    Model model;
    int mesh_count;
    HeroRole mesh_role[HERO_MAX_MESH];
    int mesh_mat[HERO_MAX_MESH];
    Vector3 mesh_centre[HERO_MAX_MESH];

    Shader mix_sh, rock_sh, diff_sh, wind_sh, plain_sh, decal_sh, mult_sh;
    WindLocs wind_loc;

    Texture2D t_terr_grass, t_terr_stone, t_mask;
    Texture2D t_rock, t_rock2, t_spike, t_pole;
    Texture2D t_grass, t_fern, t_flag;
    Texture2D t_rockshadow, t_shine;

    Texture2D t_circle;
    Mesh circle_mesh;
    Material circle_mat;
    Matrix circle_xform[HERO_FORMATION_SLOT_COUNT];
    Material rockshadow_mat;
    Matrix rockshadow_xform[HERO_FORMATION_SLOT_COUNT];

    Model backdrop;
    Texture2D t_woodfort;
    Material backdrop_mat;
    Matrix bg_xform;

    float glow_time;

    HomeSquadUnit units[SQUAD_SLOT_COUNT];
    SquadPadAssets pads;
    Matrix pad_xform[SQUAD_SLOT_COUNT];
    Texture2D pad_tex[SQUAD_SLOT_COUNT];
    unsigned char stars[SQUAD_SLOT_COUNT];
    Texture2D fac_tex[SQUAD_SLOT_COUNT];
    Matrix fac_xform[SQUAD_SLOT_COUNT];
    const char *mesh_dir, *char_dir;

    struct {
        bool active;
        bool from_board;
        bool in_world;
        bool shown;
        int roster_index;
        int from_slot;
        float pk;
        float ds;
        HomeSquadUnit unit;
        Texture2D fac_tex;
        Texture2D pad_tex;
        unsigned char stars;
        Vector3 pos;
    } drag;

    HeroFxParticle fx[HERO_FX_MAX];
    HeroFxEffect fx_effect;
    float fx_time;
    float fx_ring_accum, fx_dot_accum, fx_arc_accum, fx_glow_accum;
    Vector3 fx_origin;
    Texture2D fx_tex[HFXT_COUNT];

    HeroFxEffect row_bonus_effect;
    float row_bonus_time;

    Sound snd_pickup[3], snd_place, snd_fail;
    float shake_time;

    Camera3D cam;
} HeroesBackground;

static float HeroFrand(float lo, float hi) {
    return lo + (hi - lo) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

_Static_assert(SQUAD_SLOT_COUNT == HERO_FORMATION_SLOT_COUNT, "one formation circle per squad slot");

typedef struct {
    HeroFxEffect effect;
    const char *vfx_name;
    GuiId texture;
    int percent;
} HeroRowBonusSpec;

static HeroRowBonusSpec HeroRowBonusForSlot(int slot) {
    int position = SquadSlotReferencePosition(slot);
    if (position == 0 || position == 1)
        return (HeroRowBonusSpec){HFX_HEAL, "vfx.fx_heal_pre", GUI_HEROES_ROW_HEALTH, 50};
    if (position == 2 || position == 3) return (HeroRowBonusSpec){HFX_CRIT, "vfx.fx_crit_pre", GUI_HEROES_ROW_CRIT, 50};
    return (HeroRowBonusSpec){HFX_NONE, NULL, GUI_COUNT, 0};
}

static void HeroesSquadLoad(HeroesBackground *scene, const Player *player) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        const Character *ch = PlayerSquadCharacter(player, s);
        if (!ch) continue;
        char path[512];
        const char *prefab = HomeSquadResolvePrefab(scene->char_dir, ch, path, sizeof(path));
        if (!prefab) {
            TraceLog(LOG_WARNING, "Heroes: %s has no baked model; slot hidden", ch->id);
            continue;
        }
        const HeroFormationSlot *spot = &HERO_FORMATION_SLOTS[s];
        float pk = ch->giant ? HERO_GIANT_SCALE : 1.0f;

        Matrix xform;
        SquadPadPlaceOnGround(
            spot->pos,
            pk,
            HERO_UNIT_YAW,
            UnitDisplayScale(prefab) * pk,
            &scene->pad_xform[s],
            &scene->fac_xform[s],
            &xform
        );
        SquadPadRarityLoad(scene->mesh_dir, ch, &scene->pad_tex[s], &scene->stars[s]);
        scene->fac_tex[s] = SquadFacTexLoad(scene->mesh_dir, ch->faction);
        HomeSquadUnitLoad(&scene->units[s], path, scene->plain_sh, xform, s);
    }
}

static void HeroesSlotClear(HeroesBackground *scene, int slot) {
    HomeSquadUnitDestroy(&scene->units[slot]);
    SceneTexUnload(scene->fac_tex[slot]);
    scene->fac_tex[slot] = (Texture2D){0};
    SquadPadRarityUnload(&scene->pad_tex[slot], &scene->stars[slot]);
}

void HeroesBackgroundReloadSquad(HeroesBackground *scene, const Player *player) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HeroesSlotClear(scene, s);
    HeroesSquadLoad(scene, player);
}

static void HeroFxSpawn(
    HeroesBackground *scene,
    HeroFxKind kind,
    Vector3 pos,
    Vector3 vel,
    float life,
    float size,
    float rot,
    float rotv,
    int frame
) {
    for (int i = 0; i < HERO_FX_MAX; i++) {
        HeroFxParticle *fp = &scene->fx[i];
        if (fp->alive) continue;
        *fp = (HeroFxParticle){
            .alive = true,
            .kind = kind,
            .pos = pos,
            .vel = vel,
            .life = life,
            .size = size,
            .rot = rot,
            .rotv = rotv,
            .frame = frame,
        };
        return;
    }
}

static int HeroFxLiveCount(const HeroesBackground *scene, HeroFxKind kind) {
    int count = 0;
    for (int i = 0; i < HERO_FX_MAX; i++) count += scene->fx[i].alive && scene->fx[i].kind == kind;
    return count;
}

static void HeroFxStart(HeroesBackground *scene, HeroFxEffect effect, Vector3 origin) {
    memset(scene->fx, 0, sizeof(scene->fx));
    scene->fx_effect = effect;
    scene->fx_time = 0.0f;
    scene->fx_ring_accum = scene->fx_dot_accum = scene->fx_arc_accum = scene->fx_glow_accum = 1.0f;
    scene->fx_origin = origin;
}

void HeroesBackgroundClearRowBonus(HeroesBackground *scene) {
    memset(scene->fx, 0, sizeof(scene->fx));
    scene->fx_effect = HFX_NONE;
    scene->row_bonus_effect = HFX_NONE;
    scene->row_bonus_time = 0.0f;
}

static void HeroRowBonusStart(HeroesBackground *scene, int slot) {
    HeroRowBonusSpec bonus = HeroRowBonusForSlot(slot);
    HeroesBackgroundClearRowBonus(scene);
    if (bonus.effect == HFX_NONE) return;
    HeroFxStart(scene, bonus.effect, HERO_FORMATION_SLOTS[slot].pos);
    scene->row_bonus_effect = bonus.effect;
    scene->row_bonus_time = 0.0f;
}

static Vector3 HeroFxSphereJitter(float radius) {
    Vector3 p;
    do {
        p = (Vector3){HeroFrand(-1.0f, 1.0f), HeroFrand(-1.0f, 1.0f), HeroFrand(-1.0f, 1.0f)};
    } while (Vector3LengthSqr(p) > 1.0f || Vector3LengthSqr(p) < 0.0001f);
    return Vector3Scale(p, radius);
}

static Vector3 HeroFxConeVelocity(float speed0, float speed1, float cone_deg, bool upward) {
    float az = HeroFrand(0.0f, 2.0f * PI);
    float tilt = HeroFrand(0.0f, DEG2RAD * cone_deg);
    float speed = HeroFrand(speed0, speed1);
    float y = cosf(tilt) * (upward ? 1.0f : -1.0f);
    return (Vector3){sinf(tilt) * cosf(az) * speed, y * speed, sinf(tilt) * sinf(az) * speed};
}

static void HeroFxUpdateStep(HeroesBackground *scene, float dt) {
    for (int i = 0; i < HERO_FX_MAX; i++) {
        HeroFxParticle *fp = &scene->fx[i];
        if (!fp->alive) continue;
        fp->age += dt;
        if (fp->age >= fp->life) {
            fp->alive = false;
            continue;
        }
        if (fp->kind == HFXP_HEAL_RING) fp->vel = Vector3Scale(fp->vel, expf(-4.5f * dt));
        if (fp->kind == HFXP_CRIT_SPARK || fp->kind == HFXP_CRIT_SMOKE)
            fp->vel = Vector3Scale(fp->vel, expf(-1.0f * dt));
        if (fp->kind == HFXP_HEAL_RING) fp->vel.y += 179.4f * dt;
        if (fp->kind == HFXP_HEAL_DOT) fp->vel.y += 18.0f * dt;
        if (fp->kind == HFXP_CRIT_SPARK) fp->vel.y -= 12.75f * dt;
        if (fp->kind == HFXP_CRIT_SMOKE) fp->vel.y += 2.943f * dt;
        fp->pos = Vector3Add(fp->pos, Vector3Scale(fp->vel, dt));
        fp->rot += fp->rotv * dt;
    }

    if (scene->fx_effect == HFX_NONE) return;
    float t0 = scene->fx_time, t1 = t0 + dt;
    scene->fx_time = t1;
    Vector3 o = scene->fx_origin;

    if (scene->fx_effect == HFX_HEAL) {
        if (t0 < 0.15f) {
            scene->fx_ring_accum += 80.0f * dt;
            while (scene->fx_ring_accum >= 1.0f) {
                scene->fx_ring_accum -= 1.0f;
                if (HeroFxLiveCount(scene, HFXP_HEAL_RING) < 10)
                    HeroFxSpawn(
                        scene, HFXP_HEAL_RING, (Vector3){o.x, o.y + 0.1f, o.z}, (Vector3){0}, 0.2f, 2.4f, 0, 0, 0
                    );
            }
        }
        if (t0 < 0.2f) {
            scene->fx_dot_accum += 578.900024f * dt;
            while (scene->fx_dot_accum >= 1.0f) {
                scene->fx_dot_accum -= 1.0f;
                if (HeroFxLiveCount(scene, HFXP_HEAL_DOT) >= 60) continue;
                Vector3 pp = Vector3Add((Vector3){o.x, o.y + 0.2f, o.z}, HeroFxSphereJitter(0.5f));
                HeroFxSpawn(
                    scene,
                    HFXP_HEAL_DOT,
                    pp,
                    (Vector3){0},
                    HeroFrand(0.1f, 0.3f),
                    HeroFrand(0.1f, 0.3f),
                    50.0f * DEG2RAD,
                    0,
                    0
                );
            }
        }
        if (t0 < 0.11f && t1 >= 0.11f)
            HeroFxSpawn(scene, HFXP_HEAL_FLASH, (Vector3){o.x, o.y + 0.2f, o.z}, (Vector3){0}, 0.5f, 2.0f, 0, 0, 0);
        if (t1 > 0.71f) scene->fx_effect = HFX_NONE;
    }

    if (scene->fx_effect == HFX_CRIT) {
        Vector3 chest = {o.x - 0.190658f, o.y + 0.894454f, o.z - 0.052110f};
        if (t0 <= 0.0f) {
            HeroFxSpawn(scene, HFXP_CRIT_GLOW, chest, (Vector3){0}, 0.4f, HeroFrand(6.0f, 10.0f), 0, 0, 0);
            for (int i = 0; i < 8; i++) {
                Vector3 vel = HeroFxConeVelocity(5.0f, 10.0f, 85.0f, true);
                Vector3 pp = Vector3Add((Vector3){o.x, o.y + 0.252267f, o.z}, HeroFxSphereJitter(0.1f));
                HeroFxSpawn(
                    scene,
                    HFXP_CRIT_SMOKE,
                    pp,
                    vel,
                    HeroFrand(0.7f, 1.2f),
                    HeroFrand(2.0f, 3.0f),
                    HeroFrand(0.0f, PI),
                    HeroFrand(-30.0f, 30.0f) * DEG2RAD,
                    1 + GetRandomValue(0, 1) * 2
                );
            }
        }
        if (t0 < 0.260116f) {
            float q = t0 / 1.2f;
            float rate_scale = q <= 0.169453f ? 1.0f : fmaxf(0.0f, (0.216763f - q) / 0.04731f);
            scene->fx_glow_accum += 5.0f * rate_scale * dt;
            while (scene->fx_glow_accum >= 1.0f) {
                scene->fx_glow_accum -= 1.0f;
                if (HeroFxLiveCount(scene, HFXP_CRIT_GLOW) < 3)
                    HeroFxSpawn(scene, HFXP_CRIT_GLOW, chest, (Vector3){0}, 0.4f, HeroFrand(6.0f, 10.0f), 0, 0, 0);
            }
        }
        if (t0 < 0.04f && t1 >= 0.04f) {
            for (int i = 0; i < 100; i++) {
                Vector3 vel = HeroFxConeVelocity(10.0f, 30.0f, 60.0f, true);
                Vector3 pp =
                    Vector3Add((Vector3){o.x - 0.190658f, o.y + 0.934932f, o.z - 0.052110f}, HeroFxSphereJitter(0.2f));
                HeroFxSpawn(scene, HFXP_CRIT_SPARK, pp, vel, HeroFrand(0.3f, 0.8f), HeroFrand(0.02f, 0.04f), 0, 0, 0);
            }
        }
        if (t0 < 0.4f) {
            scene->fx_arc_accum += 30.0f * dt;
            while (scene->fx_arc_accum >= 1.0f) {
                scene->fx_arc_accum -= 1.0f;
                if (HeroFxLiveCount(scene, HFXP_CRIT_ARC) >= 6) continue;
                Vector3 pp = Vector3Add(chest, HeroFxSphereJitter(0.1f));
                HeroFxSpawn(
                    scene,
                    HFXP_CRIT_ARC,
                    pp,
                    HeroFxConeVelocity(0.01f, 0.01f, 5.0f, false),
                    0.05f,
                    HeroFrand(1.5f, 2.0f),
                    0,
                    0,
                    GetRandomValue(0, 3)
                );
                if (HeroFxLiveCount(scene, HFXP_CRIT_ARCGLOW) < 4)
                    HeroFxSpawn(
                        scene,
                        HFXP_CRIT_ARCGLOW,
                        pp,
                        (Vector3){0},
                        0.12f,
                        HeroFrand(4.0f, 5.0f),
                        0,
                        HeroFrand(0.0f, 100.0f) * DEG2RAD,
                        0
                    );
                if (HeroFxLiveCount(scene, HFXP_CRIT_ARCSTAR) < 4)
                    HeroFxSpawn(
                        scene,
                        HFXP_CRIT_ARCSTAR,
                        pp,
                        (Vector3){0},
                        0.12f,
                        HeroFrand(6.0f, 8.0f),
                        HeroFrand(0.0f, 2.0f * PI),
                        HeroFrand(0.0f, 100.0f) * DEG2RAD,
                        0
                    );
            }
        }
        if (t1 > 1.2f) scene->fx_effect = HFX_NONE;
    }
}

static void HeroFxUpdate(HeroesBackground *scene, float dt) {
    while (dt > 0.0f) {
        float step = fminf(dt, 0.05f);
        HeroFxUpdateStep(scene, step);
        dt -= step;
    }
}

static void HeroFxQuad(Vector3 c, Vector3 ax1, Vector3 ax2, Color col, float u0, float v0, float u1, float v1) {
    rlColor4ub(col.r, col.g, col.b, col.a);
    rlTexCoord2f(u0, v1);
    rlVertex3f(c.x - ax1.x - ax2.x, c.y - ax1.y - ax2.y, c.z - ax1.z - ax2.z);
    rlTexCoord2f(u1, v1);
    rlVertex3f(c.x + ax1.x - ax2.x, c.y + ax1.y - ax2.y, c.z + ax1.z - ax2.z);
    rlTexCoord2f(u1, v0);
    rlVertex3f(c.x + ax1.x + ax2.x, c.y + ax1.y + ax2.y, c.z + ax1.z + ax2.z);
    rlTexCoord2f(u0, v0);
    rlVertex3f(c.x - ax1.x + ax2.x, c.y - ax1.y + ax2.y, c.z - ax1.z + ax2.z);
}

static Color HeroColLerp(Color a, Color b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

typedef struct {
    float time, value;
} HeroFxCurveKey;

typedef struct {
    float time, value, in_slope, out_slope;
} HeroFxHermiteKey;

typedef struct {
    float time;
    Color color;
} HeroFxColorKey;

static float HeroFxCurve(const HeroFxCurveKey *keys, int count, float t) {
    if (t <= keys[0].time) return keys[0].value;
    for (int i = 1; i < count; i++) {
        if (t > keys[i].time) continue;
        float span = keys[i].time - keys[i - 1].time;
        float q = span > 0.000001f ? (t - keys[i - 1].time) / span : 1.0f;
        return keys[i - 1].value + (keys[i].value - keys[i - 1].value) * q;
    }
    return keys[count - 1].value;
}

static float HeroFxHermiteCurve(const HeroFxHermiteKey *keys, int count, float t) {
    if (t <= keys[0].time) return keys[0].value;
    for (int i = 1; i < count; i++) {
        if (t > keys[i].time) continue;
        const HeroFxHermiteKey *a = &keys[i - 1], *b = &keys[i];
        float span = b->time - a->time;
        if (span <= 0.000001f) return b->value;
        float q = (t - a->time) / span, q2 = q * q, q3 = q2 * q;
        return (2.0f * q3 - 3.0f * q2 + 1.0f) * a->value + (q3 - 2.0f * q2 + q) * span * a->out_slope +
               (-2.0f * q3 + 3.0f * q2) * b->value + (q3 - q2) * span * b->in_slope;
    }
    return keys[count - 1].value;
}

static Color HeroFxColor(const HeroFxColorKey *keys, int count, float t) {
    if (t <= keys[0].time) return keys[0].color;
    for (int i = 1; i < count; i++) {
        if (t > keys[i].time) continue;
        float span = keys[i].time - keys[i - 1].time;
        float q = span > 0.000001f ? (t - keys[i - 1].time) / span : 1.0f;
        return HeroColLerp(keys[i - 1].color, keys[i].color, q);
    }
    return keys[count - 1].color;
}

static void HeroFxDraw(HeroesBackground *scene) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(scene->cam.target, scene->cam.position));
    Vector3 cam_right = Vector3Normalize(Vector3CrossProduct(fwd, scene->cam.up));
    Vector3 cam_up = Vector3CrossProduct(cam_right, fwd);

    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();

    for (int t = 0; t < HFXT_COUNT; t++) {
        rlSetTexture(scene->fx_tex[t].id);
        rlBegin(RL_QUADS);
        for (int i = 0; i < HERO_FX_MAX; i++) {
            HeroFxParticle *fp = &scene->fx[i];
            if (!fp->alive) continue;
            float u = fp->age / fp->life;
            float size = fp->size;
            Color col = WHITE;
            HeroFxTexId tex = HFXT_BLOB;
            Vector3 ax1, ax2;
            float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
            bool horizontal = false, stretch = false;

            switch (fp->kind) {
                case HFXP_HEAL_FLASH: {
                    static const HeroFxHermiteKey size_keys[] = {{0, 0.999184f, 0, 0}, {1, 1.821430f, 0, 0}};
                    static const HeroFxCurveKey alpha_keys[] = {{0, 1}, {0.489769f, 1}, {1, 0}};
                    static const HeroFxColorKey color_keys[] = {
                        {0, {33, 118, 255, 255}}, {0.760876f, {128, 190, 255, 255}}, {1, {128, 190, 255, 255}}
                    };
                    tex = HFXT_BLOB;
                    horizontal = true;
                    size = fp->size * HeroFxHermiteCurve(size_keys, 2, u);
                    col = HeroFxColor(color_keys, 3, u);
                    col.a = (unsigned char)(255.0f * HeroFxCurve(alpha_keys, 3, u));
                    break;
                }
                case HFXP_HEAL_RING: {
                    static const HeroFxColorKey color_keys[] = {{0, {65, 255, 0, 255}}, {1, {152, 255, 0, 255}}};
                    tex = HFXT_SMOKE3;
                    horizontal = true;
                    col = HeroFxColor(color_keys, 2, u);
                    col.a = (unsigned char)(255.0f * (1.0f - u));
                    break;
                }
                case HFXP_HEAL_DOT: {
                    static const HeroFxHermiteKey size_keys[] = {
                        {0, 0.210652f, 0, 0},
                        {0.494737f, 2.674286f, 0, 0},
                        {1, 1.194999f, -5.170068f, -5.170068f},
                    };
                    tex = HFXT_SPARKDOT;
                    size = fp->size * HeroFxHermiteCurve(size_keys, 3, u);
                    col = (Color){255, 255, 255, 255};
                    break;
                }
                case HFXP_CRIT_GLOW: {
                    static const HeroFxHermiteKey size_keys[] = {{0, 1, 0, 0}, {1, 0, -2, -2}};
                    static const HeroFxColorKey color_keys[] = {
                        {0, {255, 121, 34, 255}}, {0.489845f, {26, 72, 240, 255}}, {1, {0, 0, 0, 255}}
                    };
                    tex = HFXT_FLARE;
                    size = fp->size * HeroFxHermiteCurve(size_keys, 2, u);
                    col = HeroFxColor(color_keys, 3, u);
                    break;
                }
                case HFXP_CRIT_SPARK: {
                    static const HeroFxCurveKey alpha_keys[] = {{0, 0}, {0.035187f, 1}, {0.835294f, 1}, {1, 0}};
                    static const HeroFxColorKey color_keys[] = {
                        {0, {248, 183, 142, 175}}, {0.355886f, {189, 111, 50, 175}}, {1, {60, 25, 7, 175}}
                    };
                    tex = HFXT_SQUARE;
                    stretch = true;
                    col = HeroFxColor(color_keys, 3, u);
                    col.a = (unsigned char)(175.0f * HeroFxCurve(alpha_keys, 4, u));
                    break;
                }
                case HFXP_CRIT_SMOKE: {
                    static const HeroFxHermiteKey size_keys[] = {{0, 1.110577f, 0, 0}, {1, 1.4f, 0.467117f, 0.467117f}};
                    static const HeroFxCurveKey alpha_keys[] = {{0, 0}, {0.023011f, 1}, {1, 0}};
                    tex = HFXT_WISPY;
                    size = fp->size * HeroFxHermiteCurve(size_keys, 2, u);
                    col = (Color){152, 182, 255, (unsigned char)(209.0f * HeroFxCurve(alpha_keys, 3, u))};
                    u0 = 0.5f * (fp->frame % 2);
                    v0 = 0.5f * (fp->frame / 2);
                    u1 = u0 + 0.5f;
                    v1 = v0 + 0.5f;
                    break;
                }
                case HFXP_CRIT_ARC: {
                    static const HeroFxCurveKey alpha_keys[] = {
                        {0, 1},
                        {0.1935f, 1},
                        {0.2206f, 0},
                        {0.3275f, 1},
                        {0.4872f, 1},
                        {0.5047f, 0},
                        {0.6631f, 1},
                        {1, 1}
                    };
                    tex = HFXT_ARC;
                    stretch = true;
                    col = HeroColLerp((Color){62, 97, 227, 255}, (Color){159, 129, 118, 255}, u);
                    col.a = (unsigned char)(255.0f * HeroFxCurve(alpha_keys, 8, u));
                    v0 = 0.25f * fp->frame;
                    v1 = v0 + 0.25f;
                    break;
                }
                case HFXP_CRIT_ARCGLOW: {
                    static const HeroFxHermiteKey size_keys[] = {{0, 1, 0, 0}, {1, 0, -2, -2}};
                    static const HeroFxColorKey color_keys[] = {
                        {0, {255, 121, 34, 255}},
                        {0.366705f, {210, 111, 74, 255}},
                        {0.443839f, {87, 85, 184, 255}},
                        {0.932341f, {26, 72, 240, 255}},
                        {1, {0, 0, 0, 255}}
                    };
                    tex = HFXT_FLARE;
                    horizontal = true;
                    size = fp->size * HeroFxHermiteCurve(size_keys, 2, u);
                    col = HeroFxColor(color_keys, 5, u);
                    break;
                }
                case HFXP_CRIT_ARCSTAR: {
                    static const HeroFxHermiteKey size_keys[] = {
                        {0, 0, 0, 5.834248f},
                        {0.157750f, 0.920354f, 5.834248f, -149.465820f},
                        {0.163908f, 0, -149.465820f, 3.310663f},
                        {0.415174f, 0.831858f, 3.310663f, -150.253189f},
                        {0.420711f, 0, -150.253189f, 2.091373f},
                        {0.699987f, 0.584071f, 2.091373f, -187.080353f},
                        {0.703109f, 0, -187.080353f, 1.495796f},
                        {0.993007f, 0.433628f, 1.495796f, -5130.519043f},
                        {0.993091f, 0, -5130.519043f, 0},
                    };
                    static const HeroFxColorKey color_keys[] = {
                        {0, {255, 121, 34, 255}},
                        {0.366705f, {210, 111, 74, 255}},
                        {0.443839f, {87, 85, 184, 255}},
                        {0.932341f, {26, 72, 240, 255}},
                        {1, {0, 0, 0, 255}}
                    };
                    tex = HFXT_RADIAL;
                    size = fp->size * HeroFxHermiteCurve(size_keys, 9, u);
                    col = HeroFxColor(color_keys, 5, u);
                    break;
                }
            }
            if (tex != (HeroFxTexId)t || size <= 0.0f || col.a == 0) continue;

            float h = size * 0.5f;
            Vector3 c = fp->pos;
            if (stretch) {
                float speed = Vector3Length(fp->vel);
                Vector3 dir = speed > 0.001f ? Vector3Scale(fp->vel, 1.0f / speed) : (Vector3){0, 1, 0};
                Vector3 side = Vector3CrossProduct(fwd, dir);
                side = Vector3Length(side) > 0.001f ? Vector3Normalize(side) : cam_right;
                float full_length = fp->kind == HFXP_CRIT_ARC ? size * 3.0f : speed * 0.05f;
                ax1 = Vector3Scale(dir, 0.5f * full_length);
                ax2 = Vector3Scale(side, h);
            } else if (horizontal) {
                float cr = cosf(fp->rot), sr = sinf(fp->rot);
                ax1 = (Vector3){h * cr, 0, h * sr};
                ax2 = (Vector3){-h * sr, 0, h * cr};
            } else {
                float cr = cosf(fp->rot), sr = sinf(fp->rot);
                ax1 = Vector3Add(Vector3Scale(cam_right, h * cr), Vector3Scale(cam_up, h * sr));
                ax2 = Vector3Add(Vector3Scale(cam_right, -h * sr), Vector3Scale(cam_up, h * cr));
            }
            HeroFxQuad(c, ax1, ax2, col, u0, v0, u1, v1);
        }
        rlEnd();
    }
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

static Vector3 HeroMouseOnPlane(const HeroesBackground *scene, float plane_y, bool *ok) {
    Ray ray = GetScreenToWorldRay(GetMousePosition(), scene->cam);
    *ok = false;
    if (fabsf(ray.direction.y) < 1e-4f) return (Vector3){0};
    float t = (plane_y - ray.position.y) / ray.direction.y;
    if (t <= 0.0f) return (Vector3){0};
    *ok = true;
    return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
}

bool HeroesBackgroundDragActive(const HeroesBackground *scene) {
    return scene->drag.active;
}

int HeroesBackgroundDragRosterIndex(const HeroesBackground *scene) {
    return scene->drag.active ? scene->drag.roster_index : -1;
}

bool HeroesBackgroundDragFromBoard(const HeroesBackground *scene) {
    return scene->drag.active && scene->drag.from_board;
}

int HeroesBackgroundSlotAtMouse(const HeroesBackground *scene, const Player *player) {
    bool ok;
    Vector3 p = HeroMouseOnPlane(scene, HERO_FORMATION_SLOTS[0].pos.y, &ok);
    if (!ok) return -1;
    int best = -1;
    float best_d = 0.0f;
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        if (PlayerSquadUnitAt(player, player->current_squad, s) < 0) continue;
        const HeroFormationSlot *spot = &HERO_FORMATION_SLOTS[s];
        float dx = p.x - spot->pos.x, dz = p.z - spot->pos.z;
        float d = sqrtf(dx * dx + dz * dz);
        float radius = spot->diameter * 0.5f + 0.15f;
        if (d < radius && (best < 0 || d < best_d)) {
            best = s;
            best_d = d;
        }
    }
    return best;
}

bool HeroesBackgroundDragStart(HeroesBackground *scene, const Player *player, int roster_index, int from_slot) {
    if (scene->drag.active) return false;
    const Character *ch = &player->roster[roster_index];
    char path[512];
    const char *prefab = HomeSquadResolvePrefab(scene->char_dir, ch, path, sizeof(path));
    if (!prefab) return false;
    HomeSquadUnitLoad(&scene->drag.unit, path, scene->plain_sh, MatrixTranslate(0, -1000.0f, 0), roster_index);
    const char *fac = HomeFactionTexName(ch->faction);
    scene->drag.fac_tex =
        fac ? LoadSprite(TextFormat("%s/_shared_textures/%s.png", scene->mesh_dir, fac)) : (Texture2D){0};
    SquadPadRarityLoad(scene->mesh_dir, ch, &scene->drag.pad_tex, &scene->drag.stars);
    scene->drag.pk = ch->giant ? HERO_GIANT_SCALE : 1.0f;
    scene->drag.ds = UnitDisplayScale(prefab);
    scene->drag.roster_index = roster_index;
    scene->drag.from_board = from_slot >= 0;
    scene->drag.from_slot =
        from_slot >= 0 ? from_slot : PlayerSquadFindUnit(player, player->current_squad, roster_index);
    scene->drag.active = true;
    scene->drag.shown = false;
    scene->drag.in_world = scene->drag.from_board;
    if (scene->drag.from_board) {
        HeroesSlotClear(scene, from_slot);
        PlaySound(scene->snd_pickup[GetRandomValue(0, 2)]);
    }
    return true;
}

void HeroesBackgroundDragMove(HeroesBackground *scene, bool over_tray) {
    if (!scene->drag.active) return;
    if (over_tray) {
        scene->drag.shown = false;
        return;
    }
    bool ok;
    Vector3 p = HeroMouseOnPlane(scene, HERO_DRAG_PLANE_Y, &ok);
    if (!ok) return;
    if (!scene->drag.in_world) {
        scene->drag.in_world = true;
        PlaySound(scene->snd_pickup[GetRandomValue(0, 2)]);
        if (scene->drag.from_slot >= 0 && scene->units[scene->drag.from_slot].active)
            HeroesSlotClear(scene, scene->drag.from_slot);
    }
    scene->drag.shown = true;
    scene->drag.pos = p;
    float k = scene->drag.ds * scene->drag.pk;
    scene->drag.unit.xform = MatrixMultiply(
        MatrixMultiply(MatrixScale(k, k, k), MatrixRotateY(HERO_UNIT_YAW)), MatrixTranslate(p.x, p.y, p.z)
    );
}

bool HeroesBackgroundDragDrop(HeroesBackground *scene, Player *player, bool over_tray) {
    if (!scene->drag.active) return false;
    bool changed = false;
    int cur = player->current_squad;
    const Character *ch = &player->roster[scene->drag.roster_index];

    if (!over_tray && scene->drag.in_world && scene->drag.shown) {
        int slot = SQUAD_SLOT_LARGE;
        if (!ch->giant) {
            float best = 0.0f;
            slot = 0;
            for (int s = 0; s < SQUAD_SLOT_LARGE; s++) {
                float dx = scene->drag.pos.x - HERO_FORMATION_SLOTS[s].pos.x;
                float dz = scene->drag.pos.z - HERO_FORMATION_SLOTS[s].pos.z;
                float d = dx * dx + dz * dz;
                if (s == 0 || d < best) {
                    best = d;
                    slot = s;
                }
            }
        }
        if (PlayerSquadPlace(player, cur, slot, scene->drag.roster_index)) {
            changed = true;
            PlaySound(scene->snd_place);
            HeroRowBonusStart(scene, slot);
        } else {
            PlaySound(scene->snd_fail);
            scene->shake_time = 0.5f;
        }
    } else if (scene->drag.from_slot >= 0) {
        if (PlayerSquadWithdraw(player, cur, scene->drag.from_slot)) {
            changed = true;
            HeroesBackgroundClearRowBonus(scene);
        } else {
            PlaySound(scene->snd_fail);
            scene->shake_time = 0.5f;
        }
    }

    HomeSquadUnitDestroy(&scene->drag.unit);
    SceneTexUnload(scene->drag.fac_tex);
    SquadPadRarityUnload(&scene->drag.pad_tex, &scene->drag.stars);
    memset(&scene->drag, 0, sizeof(scene->drag));
    HeroesBackgroundReloadSquad(scene, player);
    return changed;
}

#define HERO_NRM_SIGN (-1.0f)

static void HeroMeshRebuild(Model *model, int i) {
    Mesh *src = &model->meshes[i];
    int tris = src->triangleCount;
    if (tris <= 0 || !src->vertices) return;
    int nv = tris * 3;

    Mesh m = {0};
    m.vertexCount = nv;
    m.triangleCount = tris;
    m.vertices = (float *)malloc(sizeof(float) * 3 * nv);
    m.normals = (float *)malloc(sizeof(float) * 3 * nv);
    m.texcoords = (float *)malloc(sizeof(float) * 2 * nv);
    if (src->colors) m.colors = (unsigned char *)malloc(4 * nv);
    if (!m.vertices || !m.normals || !m.texcoords) {
        free(m.vertices);
        free(m.normals);
        free(m.texcoords);
        free(m.colors);
        return;
    }

    for (int t = 0; t < tris; t++) {
        int idx[3];
        if (src->indices) {
            idx[0] = src->indices[t * 3];
            idx[1] = src->indices[t * 3 + 1];
            idx[2] = src->indices[t * 3 + 2];
        } else {
            idx[0] = t * 3;
            idx[1] = t * 3 + 1;
            idx[2] = t * 3 + 2;
        }
        float p[3][3];
        for (int k = 0; k < 3; k++) {
            int s = idx[k];
            p[k][0] = src->vertices[s * 3 + 0];
            p[k][1] = src->vertices[s * 3 + 1];
            p[k][2] = -src->vertices[s * 3 + 2];
        }
        float ux = p[1][0] - p[0][0], uy = p[1][1] - p[0][1], uz = p[1][2] - p[0][2];
        float vx = p[2][0] - p[0][0], vy = p[2][1] - p[0][1], vz = p[2][2] - p[0][2];
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        float len = sqrtf(nx * nx + ny * ny + nz * nz);
        if (len > 1e-8f) {
            nx /= len;
            ny /= len;
            nz /= len;
        }
        nx *= HERO_NRM_SIGN;
        ny *= HERO_NRM_SIGN;
        nz *= HERO_NRM_SIGN;
        for (int k = 0; k < 3; k++) {
            int o = t * 3 + k, s = idx[k];
            m.vertices[o * 3 + 0] = p[k][0];
            m.vertices[o * 3 + 1] = p[k][1];
            m.vertices[o * 3 + 2] = p[k][2];
            m.normals[o * 3 + 0] = nx;
            m.normals[o * 3 + 1] = ny;
            m.normals[o * 3 + 2] = nz;
            m.texcoords[o * 2 + 0] = src->texcoords ? src->texcoords[s * 2 + 0] : 0.0f;
            m.texcoords[o * 2 + 1] = src->texcoords ? src->texcoords[s * 2 + 1] : 0.0f;
            if (m.colors) {
                m.colors[o * 4 + 0] = src->colors[s * 4 + 0];
                m.colors[o * 4 + 1] = src->colors[s * 4 + 1];
                m.colors[o * 4 + 2] = src->colors[s * 4 + 2];
                m.colors[o * 4 + 3] = src->colors[s * 4 + 3];
            }
        }
    }
    UnloadMesh(*src);
    UploadMesh(&m, false);
    *src = m;
}

#define ARENA_MIN_X (5.0f)
#define ARENA_MAX_X (62.0f)
#define ARENA_MIN_Y (-10.0f)
#define ARENA_MAX_Y (20.0f)
#define ARENA_MIN_Z (0.0f)
#define ARENA_MAX_Z (48.0f)

static bool HeroInArena(Vector3 c) {
    return c.x >= ARENA_MIN_X && c.x <= ARENA_MAX_X && c.y >= ARENA_MIN_Y && c.y <= ARENA_MAX_Y && c.z >= ARENA_MIN_Z &&
           c.z <= ARENA_MAX_Z;
}

void HeroesBackgroundInitialize(
    HeroesBackground *scene, const char *mesh_dir, const char *char_dir, const Player *player
) {
    scene->mesh_dir = mesh_dir;
    scene->char_dir = char_dir;
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        scene->units[s] = (HomeSquadUnit){0};
        scene->pad_tex[s] = (Texture2D){0};
        scene->fac_tex[s] = (Texture2D){0};
        scene->stars[s] = 0;
    }
    scene->mix_sh = LoadShaderFromMemory(VS_UNLIT, FS_HERO_MIX);
    scene->rock_sh = LoadShaderFromMemory(VS_LIT, FS_HERO_ROCK);
    scene->diff_sh = LoadShaderFromMemory(VS_UNLIT, FS_HERO_DIFF);
    scene->wind_sh = LoadShaderFromMemory(VS_WIND, FS_HERO_WIND);
    scene->plain_sh = LoadShaderFromMemory(VS_UNLIT, FS_PLAIN);
    scene->decal_sh = LoadShaderFromMemory(VS_UNLIT, FS_HERO_DECAL);
    scene->mult_sh = LoadShaderFromMemory(VS_UNLIT, FS_MULTIPLY);
    scene->wind_loc = WindLocsGet(scene->wind_sh);

    float gain = EnvFloat("HOD_GAIN", 0.25f);
    Shader gain_shaders[3] = {scene->mix_sh, scene->rock_sh, scene->diff_sh};
    for (int i = 0; i < 3; i++)
        SetShaderValue(gain_shaders[i], GetShaderLocation(gain_shaders[i], "uBright"), &gain, SHADER_UNIFORM_FLOAT);

    scene->t_terr_grass = LoadTiled(TextFormat("%s/_shared_textures/background_grass_01_PSD.png", mesh_dir));
    scene->t_terr_stone = LoadTiled(TextFormat("%s/_shared_textures/background_stone_01_PSD.png", mesh_dir));
    scene->t_mask = LoadTiled(TextFormat("%s/MainMenuContainer/background_platform_02_mask.png", mesh_dir));
    scene->t_rock = LoadTiled(TextFormat("%s/_shared_textures/background_layer_stone_03.png", mesh_dir));
    scene->t_rock2 = LoadTiled(TextFormat("%s/_shared_textures/background_main_stone_02.png", mesh_dir));
    scene->t_spike = LoadTiled(TextFormat("%s/_shared_textures/Sticks_barricade_Tex.png", mesh_dir));
    scene->t_pole = LoadTiled(TextFormat("%s/MainMenuContainer/flag_pole.png", mesh_dir));
    scene->t_grass = LoadTiled(TextFormat("%s/_shared_textures/grass_01.png", mesh_dir));
    scene->t_fern = LoadTiled(TextFormat("%s/_shared_textures/plant_atlas_UI.png", mesh_dir));
    scene->t_flag = LoadTiled(TextFormat("%s/_shared_textures/flag_Txture.png", mesh_dir));

    scene->t_circle = LoadSprite(TextFormat("%s/MainMenuContainer/formation_location_circle.png", mesh_dir));
    scene->circle_mesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
    scene->circle_mat = LoadMaterialDefault();
    scene->circle_mat.shader = scene->decal_sh;
    scene->circle_mat.maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_circle;
    scene->circle_mat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    for (int i = 0; i < HERO_FORMATION_SLOT_COUNT; i++) {
        const HeroFormationSlot *slot = &HERO_FORMATION_SLOTS[i];
        scene->circle_xform[i] = MatrixMultiply(
            MatrixMultiply(MatrixScale(slot->diameter, 1.0f, slot->diameter), MatrixRotateY(DEG2RAD * slot->yaw_deg)),
            MatrixTranslate(slot->pos.x, slot->pos.y + HERO_DECAL_LIFT, slot->pos.z)
        );
    }

    scene->t_rockshadow = LoadSprite(TextFormat("%s/MainMenuContainer/rockbase_shadow.png", mesh_dir));
    scene->rockshadow_mat = LoadMaterialDefault();
    scene->rockshadow_mat.shader = scene->mult_sh;
    scene->rockshadow_mat.maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_rockshadow;
    static const Vector3 rockshadow_pos[HERO_FORMATION_SLOT_COUNT] = {
        {29.46f, 1.02f, 16.06f},
        {27.16f, 1.01f, 16.64f},
        {29.32f, 1.01f, 13.77f},
        {27.06f, 1.01f, 14.55f},
        {28.27f, 1.05f, 19.85f},
    };
    for (int i = 0; i < HERO_FORMATION_SLOT_COUNT; i++) {
        float d = i == 4 ? 3.2f : 1.9f;
        scene->rockshadow_xform[i] = MatrixMultiply(
            MatrixScale(d, 1.0f, d),
            MatrixTranslate(rockshadow_pos[i].x, rockshadow_pos[i].y + HERO_DECAL_LIFT * 0.6f, rockshadow_pos[i].z)
        );
    }
    scene->t_shine = LoadSprite(TextFormat("%s/_shared_textures/shine.png", mesh_dir));
    scene->glow_time = 0.0f;

    scene->model = LoadModel(TextFormat("%s/MainMenuContainer/MainMenuContainer_vertexcolor.glb", mesh_dir));
    scene->mesh_count = scene->model.meshCount;
    if (scene->mesh_count > HERO_MAX_MESH) {
        TraceLog(LOG_WARNING, "HeroesBackground: GLB has %d meshes, clamping to %d", scene->mesh_count, HERO_MAX_MESH);
        scene->mesh_count = HERO_MAX_MESH;
    }

    for (int m = 1; m < scene->model.materialCount; m++) {
        Texture2D *embedded = &scene->model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (embedded->id > 0 && embedded->id != rlGetTextureIdDefault()) rlUnloadTexture(embedded->id);
        embedded->id = rlGetTextureIdDefault();
    }

    for (int i = 0; i < scene->mesh_count; i++) {
        int mat = scene->model.meshMaterial ? scene->model.meshMaterial[i] : 0;
        HeroRole role = HeroRoleForMat(mat);
        BoundingBox b = GetMeshBoundingBox(scene->model.meshes[i]);
        scene->mesh_mat[i] = mat;
        scene->mesh_centre[i] =
            (Vector3){(b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f, -(b.min.z + b.max.z) * 0.5f};
        if (role != HR_SKIP && !HeroInArena(scene->mesh_centre[i])) role = HR_SKIP;
        scene->mesh_role[i] = role;
    }

    for (int i = 0; i < scene->mesh_count; i++) {
        if (scene->mesh_role[i] != HR_SKIP) {
            HeroMeshRebuild(&scene->model, i);
        } else {
            UnloadMesh(scene->model.meshes[i]);
            scene->model.meshes[i] = (Mesh){0};
        }
    }

    Material *mats = scene->model.materials;
    int mc = scene->model.materialCount;
    if (mc > 4) {
        mats[4].shader = scene->mix_sh;
        mats[4].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_terr_grass;
        mats[4].maps[MATERIAL_MAP_SPECULAR].texture = scene->t_terr_stone;
        mats[4].maps[MATERIAL_MAP_NORMAL].texture = scene->t_mask;
    }
    if (mc > 3) {
        mats[3].shader = scene->rock_sh;
        mats[3].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_rock;
        mats[3].maps[MATERIAL_MAP_SPECULAR].texture = scene->t_rock2;
    }
    if (mc > 5) {
        mats[5].shader = scene->diff_sh;
        mats[5].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_spike;
    }
    if (mc > 7) {
        mats[7].shader = scene->plain_sh;
        mats[7].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_pole;
    }
    if (mc > 9) {
        mats[9].shader = scene->wind_sh;
        mats[9].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_grass;
    }
    if (mc > 1) {
        mats[1].shader = scene->wind_sh;
        mats[1].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_fern;
    }
    if (mc > 6) {
        mats[6].shader = scene->wind_sh;
        mats[6].maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_flag;
    }
    float plain_bright = 1.0f;
    SetShaderValue(
        scene->plain_sh, GetShaderLocation(scene->plain_sh, "brightness"), &plain_bright, SHADER_UNIFORM_FLOAT
    );

    scene->backdrop = LoadModel(TextFormat("%s/MainMenuContainer/pPlane1.obj", mesh_dir));
    scene->t_woodfort = LoadSprite(TextFormat("%s/main_screen_scene_pre/woodfort_background.png", mesh_dir));
    scene->backdrop_mat = LoadMaterialDefault();
    scene->backdrop_mat.shader = scene->plain_sh;
    scene->backdrop_mat.maps[MATERIAL_MAP_DIFFUSE].texture = scene->t_woodfort;
    scene->backdrop_mat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    scene->bg_xform = MatrixMultiply(
        MatrixMultiply(
            MatrixMultiply(
                MatrixMultiply(MatrixRotateY(PI), MatrixScale(13.65f, 15.53f, 13.65f)),
                QuaternionToMatrix((Quaternion){0.0f, 0.5204f, 0.0f, 0.8539f})
            ),
            MatrixTranslate(95.88f, 16.48f, -119.8f)
        ),
        MatrixScale(1.0f, 1.0f, -1.0f)
    );

    scene->cam = (Camera3D){0};
    scene->cam.position = (Vector3){24.4100f, 5.8277f, 8.0397f};
    scene->cam.target = (Vector3){33.126f, 1.0088f, 22.524f};
    scene->cam.up = (Vector3){0.0989f, 0.9605f, 0.2600f};
    scene->cam.fovy = 50.00f;
    scene->cam.projection = CAMERA_PERSPECTIVE;

    scene->pads = SquadPadAssetsLoad(mesh_dir, scene->plain_sh);

    static const char *const fx_paths[HFXT_COUNT] = {
        [HFXT_BLOB] = HOD_CACHE "vfx/textures/fx_heal_pre__Default-Particle.png",
        [HFXT_SMOKE3] = HOD_CACHE "vfx/textures/fx_heal_pre__smoke3.png",
        [HFXT_SPARKDOT] = HOD_CACHE "meshes/_shared_textures/chargespark.png",
        [HFXT_FLARE] = HOD_CACHE "meshes/_shared_textures/vfx_flare_01.png",
        [HFXT_SQUARE] = HOD_CACHE "vfx/textures/fx_crit_pre__Square.png",
        [HFXT_WISPY] = HOD_CACHE "meshes/_shared_textures/vfx_smoke_whispy_cs.png",
        [HFXT_ARC] = HOD_CACHE "vfx/textures/fx_crit_pre__ARC_Strip003.png",
        [HFXT_RADIAL] = HOD_CACHE "vfx/textures/fx_crit_pre__radial_lines.png",
    };
    for (int t = 0; t < HFXT_COUNT; t++) scene->fx_tex[t] = LoadSprite(fx_paths[t]);
    memset(scene->fx, 0, sizeof(scene->fx));
    scene->fx_effect = HFX_NONE;
    scene->row_bonus_effect = HFX_NONE;
    scene->row_bonus_time = 0.0f;
    memset(&scene->drag, 0, sizeof(scene->drag));
    scene->shake_time = 0.0f;

    scene->snd_pickup[0] = LoadSound(HOD_CACHE "vfx/audio/fly_leather_dark_normal_medium_1.mp3");
    scene->snd_pickup[1] = LoadSound(HOD_CACHE "vfx/audio/fly_leather_dark_normal_medium_3.mp3");
    scene->snd_pickup[2] = LoadSound(HOD_CACHE "vfx/audio/fly_leather_dark_normal_medium_5.mp3");
    scene->snd_place = LoadSound(HOD_CACHE "vfx/audio/gui_character_place_stone1.mp3");
    scene->snd_fail = LoadSound(HOD_CACHE "vfx/audio/gui_character_place_fail.mp3");

    HeroesSquadLoad(scene, player);
}

void HeroesBackgroundUpdate(HeroesBackground *scene, float dt) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitUpdate(&scene->units[s], dt);
    if (scene->drag.active) HomeSquadUnitUpdate(&scene->drag.unit, dt);
    HeroFxUpdate(scene, dt);
    if (scene->row_bonus_effect != HFX_NONE) {
        scene->row_bonus_time += dt;
        if (scene->row_bonus_time >= HERO_ROW_BONUS_TOTAL_TIME) {
            scene->row_bonus_effect = HFX_NONE;
            scene->row_bonus_time = 0.0f;
        }
    }
    scene->glow_time += dt;
    if (scene->shake_time > 0.0f) scene->shake_time -= dt;

    const char *fx = getenv("HOD_FX");
    if (fx && scene->row_bonus_effect == HFX_NONE) HeroRowBonusStart(scene, strcmp(fx, "crit") == 0 ? 0 : 1);

    if (!FlycamEnabled()) return;
    UpdateCamera(&scene->cam, CAMERA_FREE);
    if (IsKeyDown(KEY_LEFT_BRACKET)) scene->cam.fovy = fmaxf(10.0f, scene->cam.fovy - 30.0f * dt);
    if (IsKeyDown(KEY_RIGHT_BRACKET)) scene->cam.fovy = fminf(120.0f, scene->cam.fovy + 30.0f * dt);
    if (IsKeyPressed(KEY_P)) {
        Vector3 p = scene->cam.position, tg = scene->cam.target;
        TraceLog(
            LOG_INFO,
            "CAM position={%.3ff, %.3ff, %.3ff} target={%.3ff, %.3ff, %.3ff} fovy=%.2ff",
            p.x,
            p.y,
            p.z,
            tg.x,
            tg.y,
            tg.z,
            scene->cam.fovy
        );
    }
}

static const SceneWind *HeroWind(HeroRole role) {
    return (role == HR_GRASS) ? &SCENE_WIND_GRASS : (role == HR_FERN) ? &SCENE_WIND_FERN : &SCENE_WIND_FLAG;
}

static void HeroGlowDraw(HeroesBackground *scene) {
    Vector3 pos = {44.6251f, 13.6060f, 58.3862f};
    Vector3 fwd = Vector3Normalize(Vector3Subtract(scene->cam.target, scene->cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, scene->cam.up));
    Vector3 up = Vector3CrossProduct(right, fwd);

    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    rlSetTexture(scene->t_shine.id);
    rlBegin(RL_QUADS);
    for (int k = 0; k < 2; k++) {
        float life = fmodf(scene->glow_time + k * 9.0f, 18.0f);
        float a = life < 5.53f ? 0.655f * (life / 5.53f) : life < 12.73f ? 0.66f : 0.66f * ((18.0f - life) / 5.27f);
        float rot = DEG2RAD * (scene->glow_time * 1.0f) * (k == 0 ? 1.0f : -1.0f) + k * 2.1f;
        float h = (k == 0 ? 20.6f : 26.7f) * 0.5f;
        float cr = cosf(rot), sr = sinf(rot);
        Vector3 ax1 = Vector3Add(Vector3Scale(right, h * cr), Vector3Scale(up, h * sr));
        Vector3 ax2 = Vector3Add(Vector3Scale(right, -h * sr), Vector3Scale(up, h * cr));
        HeroFxQuad(pos, ax1, ax2, (Color){186, 178, 153, (unsigned char)(a * 255.0f)}, 0, 0, 1, 1);
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

static float HeroRowBonusEaseOutExpo(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return 1.0f - powf(2.0f, -10.0f * t);
}

static float HeroRowBonusAlphaAt(float time) {
    if (time < 0.0f || time >= HERO_ROW_BONUS_TOTAL_TIME) return 0.0f;
    if (time < HERO_ROW_BONUS_MOVE_TIME) return HeroRowBonusEaseOutExpo(time / HERO_ROW_BONUS_MOVE_TIME);
    if (time < HERO_ROW_BONUS_DISPLAY_TIME) return 1.0f;
    return 1.0f - HeroRowBonusEaseOutExpo((time - HERO_ROW_BONUS_DISPLAY_TIME) / HERO_ROW_BONUS_FADE_TIME);
}

static Rectangle HeroRowBonusRectAt(float time, float width, float height) {
    float button_w = ceilf(width * 0.7408673763f);
    float button_x = ceilf(width - button_w);
    float center_w = ceilf(button_w * 0.8799999952f);
    float instructions_h = ceilf(height * 0.1500000060f);
    float instructions_w = fminf(center_w, instructions_h * 5.03000021f);
    float instructions_x = button_x + ceilf((center_w - instructions_w) * 0.5f);
    float message_h = ceilf(height * 0.2800000012f);
    float message_w = fminf(center_w, message_h * 2.0f);
    float target_y = 73.0f;
    float start_y = target_y + instructions_h + message_h;
    float move = HeroRowBonusEaseOutExpo(fminf(fmaxf(time / HERO_ROW_BONUS_MOVE_TIME, 0.0f), 1.0f));
    return (Rectangle){instructions_x, start_y + (target_y - start_y) * move, message_w, message_h};
}

static Rectangle HeroRowBonusChildRect(Rectangle parent, float x, float y, float w, float h) {
    float width = ceilf(parent.width * w), height = ceilf(parent.height * h);
    return (Rectangle){
        parent.x + ceilf((parent.width - width) * x),
        parent.y + ceilf((parent.height - height) * y),
        width,
        height,
    };
}

void HeroesBackgroundDrawRowBonus(const HeroesBackground *scene) {
    if (scene->row_bonus_effect == HFX_NONE) return;
    float alpha = HeroRowBonusAlphaAt(scene->row_bonus_time);
    if (alpha <= 0.0f) return;

    HeroRowBonusSpec bonus = scene->row_bonus_effect == HFX_HEAL ? HeroRowBonusForSlot(1) : HeroRowBonusForSlot(0);
    Texture2D texture = GuiTex(bonus.texture);
    Rectangle panel = HeroRowBonusRectAt(scene->row_bonus_time, UiWidth(), UiHeight());
    unsigned char a = (unsigned char)(255.0f * alpha);
    UiDrawTexture(texture, panel, (Color){255, 255, 255, a});

    Rectangle labels = HeroRowBonusChildRect(panel, 0.5f, 0.0f, 0.525f, 1.0f);
    Rectangle desc = HeroRowBonusChildRect(labels, 0.0f, 0.45f, 0.65f, 0.5f);
    Rectangle amount = HeroRowBonusChildRect(labels, 0.7f, 0.87f, 0.7f, 0.5f);
    Font font = GuiFont(FNT_COOL);
    UiDrawTextCentered(font, LocGet("SQUAD_ROW_BONUS"), desc, 23.0f, (Color){255, 255, 255, a});
    UiDrawTextCentered(font, TextFormat("+%d%%", bonus.percent), amount, 35.0f, (Color){0, 255, 20, a});
}

void HeroesBackgroundDraw(HeroesBackground *scene) {
    float t = (float)GetTime();
    Matrix I = MatrixIdentity();

    ClearBackground(BLACK);
    Camera3D cam = scene->cam;
    if (scene->shake_time > 0.0f) {
        float a = 0.25f * (scene->shake_time / 0.5f);
        Vector3 offs = {HeroFrand(-a, a), HeroFrand(-a, a), HeroFrand(-a, a)};
        cam.position = Vector3Add(cam.position, offs);
        cam.target = Vector3Add(cam.target, offs);
    }
    BeginMode3D(cam);
    rlDisableBackfaceCulling();

    if (scene->backdrop.meshCount > 0) DrawMesh(scene->backdrop.meshes[0], scene->backdrop_mat, scene->bg_xform);

    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_FRONT);
    for (int i = 0; i < scene->mesh_count; i++) {
        HeroRole r = scene->mesh_role[i];
        if (r == HR_TERRAIN || r == HR_PROP)
            DrawMesh(scene->model.meshes[i], scene->model.materials[scene->mesh_mat[i]], I);
    }

    rlDisableBackfaceCulling();
    rlDrawRenderBatchActive();
    rlSetBlendFactors(RL_ZERO, RL_SRC_COLOR, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    rlDisableDepthMask();
    for (int i = 0; i < HERO_FORMATION_SLOT_COUNT; i++) {
        if (!scene->units[i].active) continue;
        DrawMesh(scene->circle_mesh, scene->rockshadow_mat, scene->rockshadow_xform[i]);
    }
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();

    rlDisableBackfaceCulling();
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        if (!scene->units[s].active) continue;
        SquadPadDraw(
            &scene->pads,
            scene->pad_tex[s],
            scene->pad_xform[s],
            scene->stars[s],
            scene->fac_tex[s],
            scene->fac_xform[s]
        );
    }
    if (scene->drag.active && scene->drag.shown) {
        Matrix pad, fac;
        SquadPadPlaceAtFeet(scene->drag.pos, scene->drag.pk, &pad, &fac, NULL);
        SquadPadDraw(&scene->pads, scene->drag.pad_tex, pad, scene->drag.stars, scene->drag.fac_tex, fac);
    }
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HomeSquadUnitDraw(&scene->units[s]);
    if (scene->drag.active && scene->drag.shown) HomeSquadUnitDraw(&scene->drag.unit);
    rlSetCullFace(RL_CULL_FACE_FRONT);

    rlDisableBackfaceCulling();
    BeginBlendMode(BLEND_ALPHA);

    rlDisableDepthMask();
    for (int i = 0; i < HERO_FORMATION_SLOT_COUNT; i++)
        DrawMesh(scene->circle_mesh, scene->circle_mat, scene->circle_xform[i]);
    rlEnableDepthMask();

    for (int i = 0; i < scene->mesh_count; i++) {
        HeroRole r = scene->mesh_role[i];
        if (r != HR_GRASS && r != HR_FERN && r != HR_CLOTH) continue;
        SetWindUniforms(scene->wind_sh, scene->wind_loc, HeroWind(r), scene->mesh_centre[i], t);
        DrawMesh(scene->model.meshes[i], scene->model.materials[scene->mesh_mat[i]], I);
    }
    EndBlendMode();

    HeroGlowDraw(scene);
    HeroFxDraw(scene);

    EndMode3D();
    rlSetCullFace(RL_CULL_FACE_BACK);
    rlEnableBackfaceCulling();

    if (FlycamEnabled()) {
        Vector3 p = scene->cam.position, tg = scene->cam.target;
        const char *info = TextFormat(
            "FLYCAM  pos (%.2f, %.2f, %.2f)   target (%.2f, %.2f, %.2f)   fovy %.1f\n"
            "WASD move | mouse look | Space/Ctrl up/down | wheel zoom | [ ] fov | P print to console",
            p.x,
            p.y,
            p.z,
            tg.x,
            tg.y,
            tg.z,
            scene->cam.fovy
        );
        int fs = 14;
        int tw = MeasureText(info, fs);
        int bx = (GetScreenWidth() - tw) / 2;
        DrawRectangle(bx, 150, tw + 20, 2 * fs + 12, (Color){0, 0, 0, 160});
        DrawText(info, bx + 10, 156, fs, RAYWHITE);
    }
}

void HeroesBackgroundDestroy(HeroesBackground *scene) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) HeroesSlotClear(scene, s);
    if (scene->drag.active) {
        HomeSquadUnitDestroy(&scene->drag.unit);
        SceneTexUnload(scene->drag.fac_tex);
        SquadPadRarityUnload(&scene->drag.pad_tex, &scene->drag.stars);
    }
    for (int t = 0; t < HFXT_COUNT; t++) SceneTexUnload(scene->fx_tex[t]);
    for (int i = 0; i < 3; i++) UnloadSound(scene->snd_pickup[i]);
    UnloadSound(scene->snd_place);
    UnloadSound(scene->snd_fail);
    SquadPadAssetsDestroy(&scene->pads);
    SceneTexUnload(scene->t_terr_grass);
    SceneTexUnload(scene->t_terr_stone);
    SceneTexUnload(scene->t_mask);
    SceneTexUnload(scene->t_rock);
    SceneTexUnload(scene->t_rock2);
    SceneTexUnload(scene->t_spike);
    SceneTexUnload(scene->t_pole);
    SceneTexUnload(scene->t_grass);
    SceneTexUnload(scene->t_fern);
    SceneTexUnload(scene->t_flag);

    SceneTexUnload(scene->t_circle);
    SceneTexUnload(scene->t_rockshadow);
    SceneTexUnload(scene->t_shine);
    UnloadMesh(scene->circle_mesh);
    SceneMaterialUnload(scene->circle_mat);
    SceneMaterialUnload(scene->rockshadow_mat);

    UnloadModel(scene->model);

    SceneTexUnload(scene->t_woodfort);
    SceneMaterialUnload(scene->backdrop_mat);
    UnloadModel(scene->backdrop);

    UnloadShader(scene->mix_sh);
    UnloadShader(scene->rock_sh);
    UnloadShader(scene->diff_sh);
    UnloadShader(scene->wind_sh);
    UnloadShader(scene->plain_sh);
    UnloadShader(scene->decal_sh);
    UnloadShader(scene->mult_sh);
}
