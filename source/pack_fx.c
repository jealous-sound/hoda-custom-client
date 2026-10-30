
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define PACK_FX_FIRE_COLUMNS 8
#define PACK_FX_FIRE_ROWS 8
#define PACK_FX_STROBE_COLUMNS 4
#define PACK_FX_STROBE_ROWS 3
#define PACK_FX_VOLT_COLUMNS 2
#define PACK_FX_VOLT_ROWS 2
#define PACK_FX_ARC_ROWS 4

#define PACK_FX_ROULETTE_SPAN 3.30f
#define PACK_FX_ROULETTE_END 3.06f
#define PACK_FX_EXPLOSION_END (PACK_FX_ROULETTE_END + 3.0f)
#define PACK_FX_CLOSING 4.80f
#define PACK_FX_FLASH 6.60f
#define PACK_FX_LANDING 7.0157f
#define PACK_FX_DUST 7.9334f
#define PACK_FX_DUST_END 8.5334f
#define PACK_FX_REVEAL 8.0503f
#define PACK_FX_BURST_END 10.05f

// clang-format off
#define PACK_FX_TEXTURES(X)                                           \
    X(fire, CACHE_RESOURCE("fire_anim03"))                            \
    X(flamebits, CACHE_RESOURCE("Fire_Still01"))                      \
    X(embers, CACHE_RESOURCE("dust_mote_2x2_01"))                     \
    X(square, CACHE_RESOURCE("Square"))                               \
    X(strobe, CACHE_RESOURCE("image2"))                               \
    X(volt, CACHE_RESOURCE("fxa_cen_volt2"))                          \
    X(arc, CACHE_RESOURCE("ARC_Strip003"))                            \
    X(radial, CACHE_RESOURCE("radial_lines"))                         \
    X(glow, CACHE_RESOURCE("GlowDot"))                                \
    X(spark, CACHE_BUNDLE_RESOURCE("PackOpening_pre", "spark"))       \
    X(shockwave, CACHE_RESOURCE("Shockwave"))                         \
    X(star, CACHE_RESOURCE("Projectile_Head"))                        \
    X(ring, CACHE_RESOURCE("vfx_wt_circleBeamCornea"))                \
    X(ring_explode, CACHE_RESOURCE("vfx_wt_circleBeamCorneaExplode")) \
    X(cloud, CACHE_RESOURCE("cloud001"))                              \
    X(cloud_flash, CACHE_RESOURCE("cloud002"))                        \
    X(wisp, CACHE_RESOURCE("wispy10"))                                \
    X(gobo, CACHE_RESOURCE("FlashlightGobo_01_Diff"))                 \
    X(flare, CACHE_RESOURCE("vfx_flare_01"))                          \
    X(beam, CACHE_RESOURCE("Beam_004"))                               \
    X(shine, CACHE_RESOURCE("shine"))                                 \
    X(light, CACHE_RESOURCE("Light"))
// clang-format on

typedef struct PackFx {
#define X(field, ref) Texture2D field;
    PACK_FX_TEXTURES(X)
#undef X
    bool initialized;
} PackFx;

static const Vector3 PACK_FX_STORM_ROOT = {216.6290f, 5.1627f, -165.9668f};
static const Vector3 PACK_FX_CLOUD_FLASH_ROOT = {218.6340f, 4.5066f, -167.3237f};

static const Vector3 PACK_FX_FIRE_ROOTS[2] = {
    {217.8407f, 1.1215f, -170.8368f},
    {222.0443f, 1.1215f, -170.8368f},
};

static const Vector3 PACK_FX_WISP_ROOT = {220.0495f, 2.1419f, -168.3281f};
static const Vector3 PACK_FX_STROBE_ROOT = {220.0048f, 2.1197f, -168.2986f};
static const Vector3 PACK_FX_INNER_GLOW_ROOT = {220.0048f, 2.1197f, -168.1876f};
static const Vector3 PACK_FX_ARCS_ROOT = {220.0047f, 2.1197f, -168.5668f};

static const Vector3 PACK_FX_EXPLOSION_CORE = {219.9983f, 2.0480f, -170.5557f};
static const Vector3 PACK_FX_EXPLOSION_ARCS = {219.9983f, 2.0720f, -168.6247f};

static const Vector3 PACK_FX_LIGHTNING_ROOT = {220.0476f, 1.5501f, -170.7859f};
static const Vector3 PACK_FX_FLASH_ROOT = {220.0f, 2.2268f, -171.1302f};

static const Vector3 PACK_FX_DUST_ROOT = {220.0025f, -0.5361f, -171.1628f};
static const Vector3 PACK_FX_CRACKS_ROOT = {220.0025f, -0.4166f, -171.1628f};

static const Vector3 PACK_FX_BURST_ROOT = {220.0f, -0.1409f, -171.1302f};

static float PackFxSaturate(float value) {
    return Clamp(value, 0.0f, 1.0f);
}

static float PackFxFrac(float value) {
    return value - floorf(value);
}

static bool PackFxBetween(float value, float from, float to) {
    return value >= from && value < to;
}

static float PackFxSmoothstep(float from, float to, float value) {
    float t = to > from ? PackFxSaturate((value - from) / (to - from)) : 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static float PackFxHash01(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return (float)(value & 0x00ffffffu) / 16777215.0f;
}

static Color PackFxWithAlpha(Color color, float alpha) {
    color.a = (unsigned char)(PackFxSaturate(alpha) * 255.0f + 0.5f);
    return color;
}

static Color PackFxMix(Color from, Color to, float t) {
    t = PackFxSaturate(t);
    return (Color){
        (unsigned char)((float)from.r + ((float)to.r - (float)from.r) * t),
        (unsigned char)((float)from.g + ((float)to.g - (float)from.g) * t),
        (unsigned char)((float)from.b + ((float)to.b - (float)from.b) * t),
        255,
    };
}

static Rectangle PackFxFullFrame(Texture2D texture) {
    return (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height};
}

static Rectangle PackFxAtlasFrame(Texture2D texture, int columns, int rows, int frame) {
    int count = columns * rows;
    if (count <= 0) return PackFxFullFrame(texture);
    frame %= count;
    if (frame < 0) frame += count;
    float width = (float)texture.width / (float)columns;
    float height = (float)texture.height / (float)rows;
    return (Rectangle){(float)(frame % columns) * width, (float)(frame / columns) * height, width, height};
}

static void PackFxBillboard(
    Camera3D camera, Texture2D texture, Rectangle source, Vector3 position, Vector2 size, float rotation, Color tint
) {
    if (!IsTextureValid(texture) || tint.a == 0 || size.x <= 0.0f || size.y <= 0.0f) return;
    DrawBillboardPro(
        camera,
        texture,
        source,
        position,
        (Vector3){0.0f, 1.0f, 0.0f},
        size,
        (Vector2){size.x * 0.5f, size.y * 0.5f},
        rotation,
        tint
    );
}

void PackFxDestroy(PackFx *fx) {
    if (!fx) return;
#define X(field, ref)                                                                                                  \
    if (IsTextureValid(fx->field)) UnloadTexture(fx->field);
    PACK_FX_TEXTURES(X)
#undef X
    memset(fx, 0, sizeof(*fx));
}

static Texture2D PackFxLoadTexture(const char *ref) {
    Texture2D texture = ResourceLoadTexture(ref);
    if (IsTextureValid(texture)) SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

bool PackFxInitialize(PackFx *fx) {
    if (!fx) return false;
    memset(fx, 0, sizeof(*fx));

#define X(field, ref) fx->field = PackFxLoadTexture(ref);
    PACK_FX_TEXTURES(X)
#undef X

    fx->initialized =
        IsTextureValid(fx->fire) && IsTextureValid(fx->strobe) && IsTextureValid(fx->volt) && IsTextureValid(fx->wisp);
    if (!fx->initialized) {
        TraceLog(LOG_WARNING, "PackFx: required PackOpening_pre textures are unavailable");
        PackFxDestroy(fx);
        return false;
    }
    return true;
}

static void PackFxDrawFire(PackFx *fx, Camera3D camera, float time, int fire_index) {
    Vector3 root = PACK_FX_FIRE_ROOTS[fire_index];
    float sway = sinf(time * 5.1f + (float)fire_index * 2.3f) * 0.055f;

    PackFxBillboard(
        camera,
        fx->light,
        PackFxFullFrame(fx->light),
        (Vector3){root.x, root.y - 0.2f, root.z + 0.04f},
        (Vector2){1.80f, 1.10f},
        0.0f,
        (Color){109, 47, 3, 60}
    );

    for (int i = 0; i < 6; i++) {
        uint32_t seed = (uint32_t)(fire_index * 131 + i * 37 + 3);
        float h0 = PackFxHash01(seed);
        float hs = PackFxHash01(seed + 13u);
        float hx = PackFxHash01(seed + 91u);
        float hy = PackFxHash01(seed + 107u);
        float hz = PackFxHash01(seed + 131u);
        float hv = PackFxHash01(seed + 57u);
        float life = 1.6f + h0 * 0.4f;
        float age = PackFxFrac((time + h0 * life) / life);
        int frame = (int)(age * (float)(PACK_FX_FIRE_COLUMNS * PACK_FX_FIRE_ROWS));
        float fade = age < 0.0744f ? age / 0.0744f : (age <= 0.8390f ? 1.0f : (1.0f - age) / 0.1610f);
        float size = (1.4f + hs * 0.6f) * (1.0f - 0.2116f * age);
        PackFxBillboard(
            camera,
            fx->fire,
            PackFxAtlasFrame(fx->fire, PACK_FX_FIRE_COLUMNS, PACK_FX_FIRE_ROWS, frame),
            (Vector3){
                root.x + (hx - 0.5f) * 0.20f + sway * (0.5f + h0),
                root.y + (hy - 0.5f) * 0.10f + hv * 0.4f * age * life,
                root.z + (hz - 0.5f) * 0.20f - (float)i * 0.01f,
            },
            (Vector2){size, size},
            0.0f,
            PackFxWithAlpha(PackFxMix((Color){255, 255, 255, 255}, (Color){216, 171, 143, 255}, age), fade)
        );
    }

    PackFxBillboard(
        camera,
        fx->flamebits,
        PackFxFullFrame(fx->flamebits),
        (Vector3){root.x - sway * 0.6f, root.y + 0.35f, root.z - 0.05f},
        (Vector2){0.70f, 0.70f},
        0.0f,
        (Color){208, 166, 141, 132}
    );

    for (int i = 0; i < 24; i++) {
        uint32_t seed = (uint32_t)(fire_index * 97 + i * 23 + 11);
        float h0 = PackFxHash01(seed);
        float h1 = PackFxHash01(seed + 31u);
        float h2 = PackFxHash01(seed + 67u);
        float phase = PackFxFrac(time * (0.9f + h0 * 0.7f) + h1);
        float fade = sinf(phase * PI);
        Vector3 position = {
            root.x + (h0 - 0.5f) * 0.4f + sinf(time * 2.0f + h2 * 8.0f) * 0.08f,
            root.y + phase * (1.4f + h2 * 0.6f),
            root.z - 0.09f + (h1 - 0.5f) * 0.16f,
        };
        Color tint = phase < 0.5f ? PackFxMix((Color){255, 255, 255, 255}, (Color){255, 144, 58, 255}, phase * 2.0f)
                                  : PackFxMix((Color){255, 144, 58, 255}, (Color){255, 0, 0, 255}, phase * 2.0f - 1.0f);
        PackFxBillboard(
            camera,
            fx->square,
            PackFxFullFrame(fx->square),
            position,
            (Vector2){0.030f, 0.088f},
            0.0f,
            PackFxWithAlpha(tint, fade)
        );
    }

    for (int i = 0; i < 4; i++) {
        uint32_t seed = (uint32_t)(fire_index * 61 + i * 53 + 7);
        float h0 = PackFxHash01(seed);
        float h1 = PackFxHash01(seed + 19u);
        float phase = PackFxFrac(time * 0.55f + h1);
        Vector3 position = {
            root.x + (h0 - 0.5f) * 0.5f,
            root.y - 0.2f + phase * 1.6f,
            root.z - 0.06f,
        };
        PackFxBillboard(
            camera,
            fx->embers,
            PackFxFullFrame(fx->embers),
            position,
            (Vector2){0.16f, 0.16f},
            h0 * 360.0f,
            PackFxWithAlpha((Color){255, 133, 47, 255}, sinf(phase * PI) * 0.7f)
        );
    }
}

#define PACK_FX_CLOUD_COUNT 3
#define PACK_FX_CLOUD_FLASH_COUNT 3
#define PACK_FX_SKY_EYE_Y 2.2886f
#define PACK_FX_SKY_EYE_Z (-173.8590f)
#define PACK_FX_SKY_ELEVATION 0.2309f
static void PackFxDrawStormSky(PackFx *fx, Camera3D camera, float time) {
    for (int i = 0; i < PACK_FX_CLOUD_COUNT; i++) {
        uint32_t seed = (uint32_t)(i * 449 + 7);
        float h = PackFxHash01(seed);
        float distance = 72.0f + (float)i * 20.0f;
        float drift = sinf(time * (0.030f + h * 0.020f) + h * 6.283f);
        Vector3 position = {
            PACK_UNIT_X + drift * distance * 0.12f,
            PACK_FX_SKY_EYE_Y + distance * PACK_FX_SKY_ELEVATION,
            PACK_FX_SKY_EYE_Z + distance,
        };
        PackFxBillboard(
            camera,
            fx->cloud,
            PackFxFullFrame(fx->cloud),
            position,
            (Vector2){distance * 1.95f, distance * 1.10f},
            0.0f,
            PackFxWithAlpha((Color){150, 164, 182, 255}, 0.38f - (float)i * 0.05f)
        );
    }
    for (int i = 0; i < PACK_FX_CLOUD_FLASH_COUNT; i++) {
        uint32_t seed = (uint32_t)(i * 617 + 53);
        float h = PackFxHash01(seed);
        float h2 = PackFxHash01(seed + 41u);
        float phase = PackFxFrac(time * 0.30f + h);
        float strobe = phase < 0.13f ? 1.0f - phase / 0.13f : (phase > 0.24f && phase < 0.30f ? 0.65f : 0.0f);
        if (strobe <= 0.0f) continue;
        float distance = 78.0f + h2 * 26.0f;
        PackFxBillboard(
            camera,
            fx->cloud_flash,
            PackFxFullFrame(fx->cloud_flash),
            (Vector3){
                PACK_UNIT_X + (h2 - 0.5f) * distance * 0.5f,
                PACK_FX_SKY_EYE_Y + distance * PACK_FX_SKY_ELEVATION,
                PACK_FX_SKY_EYE_Z + distance,
            },
            (Vector2){distance * 1.95f, distance * 1.10f},
            0.0f,
            PackFxWithAlpha((Color){206, 220, 238, 255}, strobe * 0.62f)
        );
    }
}

static void PackFxDrawWisp(PackFx *fx, Camera3D camera, float time) {
    for (int i = 0; i < 5; i++) {
        float h = PackFxHash01((uint32_t)(i * 977 + 5));
        float age = PackFxFrac(time / 4.0f + h);
        float alpha = age < 0.25f ? age / 0.25f * 0.357f : (age < 0.6f ? 0.357f : (1.0f - age) / 0.4f * 0.157f);
        Color tint = age < 0.5f ? PackFxMix((Color){255, 255, 255, 255}, (Color){151, 213, 253, 255}, age * 2.0f)
                                : PackFxMix((Color){151, 213, 253, 255}, (Color){17, 23, 90, 255}, age * 2.0f - 1.0f);
        float size = 5.0f * age;
        PackFxBillboard(
            camera,
            fx->wisp,
            PackFxFullFrame(fx->wisp),
            (Vector3){PACK_FX_WISP_ROOT.x, PACK_FX_WISP_ROOT.y, PACK_FX_WISP_ROOT.z + (float)i * 0.01f},
            (Vector2){size, size},
            h * 90.0f,
            PackFxWithAlpha(tint, alpha)
        );
    }
}

static float PackFxStrobeLife(float birth) {
    return 0.200f - 0.125f * PackFxSaturate(birth / PACK_FX_ROULETTE_SPAN);
}

static void PackFxDrawStrobe(PackFx *fx, Camera3D camera, float time) {
    float birth = 0.0f;
    int slot = 0;
    for (int k = 1; (float)k * 0.1f <= time; k++) {
        float tick = (float)k * 0.1f;
        if (tick >= birth + PackFxStrobeLife(birth)) {
            birth = tick;
            slot = k;
        }
    }
    float age = (time - birth) / PackFxStrobeLife(birth);
    if (age > 1.0f) return;

    uint32_t seed = (uint32_t)slot * 2654435761u;
    int cell = (int)(PackFxHash01(seed) * (float)(PACK_FX_STROBE_COLUMNS * PACK_FX_STROBE_ROWS));
    float size = 2.5f * (2.071f + (2.649f - 2.071f) * age);

    EndBlendMode();
    rlSetBlendFactors(RL_DST_COLOR, RL_ZERO, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    PackFxBillboard(
        camera,
        fx->strobe,
        PackFxAtlasFrame(fx->strobe, PACK_FX_STROBE_COLUMNS, PACK_FX_STROBE_ROWS, cell),
        PACK_FX_STROBE_ROOT,
        (Vector2){size * 0.34f, size * 0.34f},
        0.0f,
        WHITE
    );
    rlDrawRenderBatchActive();
    EndBlendMode();
    BeginBlendMode(BLEND_ADDITIVE);
}

static void PackFxDrawStrobeGlows(PackFx *fx, Camera3D camera, float time) {
    for (int i = 0; i < 2; i++) {
        float h = PackFxHash01((uint32_t)(i * 313 + 11));
        float age = PackFxFrac(time / (1.4f + h * 0.6f) + h);
        int frame = (int)(age * 4.0f);
        PackFxBillboard(
            camera,
            fx->volt,
            PackFxAtlasFrame(fx->volt, PACK_FX_VOLT_COLUMNS, PACK_FX_VOLT_ROWS, frame),
            PACK_FX_WISP_ROOT,
            (Vector2){3.2f, 3.2f},
            h * 360.0f + time * (5.0f + h * 10.0f),
            PackFxWithAlpha(WHITE, sinf(age * PI) * 0.34f)
        );
    }
    for (int i = 0; i < 2; i++) {
        float h = PackFxHash01((uint32_t)(i * 733 + 29));
        float life = 0.15f + h * 0.25f;
        float age = PackFxFrac(time / life + h);
        float size = 1.0f + h * 2.4f;
        Color tint = age < 0.5f ? PackFxMix((Color){200, 233, 223, 255}, (Color){251, 71, 122, 255}, age * 2.0f)
                                : PackFxMix((Color){251, 71, 122, 255}, (Color){46, 130, 158, 255}, age * 2.0f - 1.0f);
        PackFxBillboard(
            camera,
            fx->gobo,
            PackFxFullFrame(fx->gobo),
            PACK_FX_INNER_GLOW_ROOT,
            (Vector2){size, size},
            0.0f,
            PackFxWithAlpha(tint, sinf(age * PI) * 0.6f)
        );
    }
}

static void PackFxDrawArcs(PackFx *fx, Camera3D camera, float time) {
    int slot = (int)floorf(time * 6.0f);
    float age = PackFxSaturate((time - (float)slot / 6.0f) / 0.4f);
    Color tint = PackFxMix((Color){255, 154, 255, 255}, (Color){44, 67, 221, 255}, age);
    PackFxBillboard(
        camera,
        fx->volt,
        PackFxAtlasFrame(fx->volt, PACK_FX_VOLT_COLUMNS, PACK_FX_VOLT_ROWS, slot),
        PACK_FX_ARCS_ROOT,
        (Vector2){2.3f, 2.3f},
        PackFxHash01((uint32_t)slot * 7919u) * 540.0f - 180.0f,
        PackFxWithAlpha(tint, (1.0f - age) * 0.76f)
    );
}

static void PackFxDrawExplosion(PackFx *fx, Camera3D camera, float time) {
    float age = time - PACK_FX_ROULETTE_END;
    float envelope = PackFxSaturate((3.0f - age) / 0.5f);

    for (int i = 0; i < 3; i++) {
        uint32_t seed = (uint32_t)(i * 401 + 13);
        float h = PackFxHash01(seed);
        float life = 0.1f + h * 0.1f;
        float phase = PackFxFrac(age / life + h);
        float teeth = fabsf(sinf(phase * PI * 4.0f)) * (1.0f - phase * 0.6f);
        Color tint = phase < 0.44f
                         ? PackFxMix((Color){255, 220, 255, 255}, (Color){255, 110, 220, 255}, phase / 0.44f)
                         : PackFxMix((Color){255, 110, 220, 255}, (Color){60, 90, 240, 255}, (phase - 0.44f) / 0.56f);
        PackFxBillboard(
            camera,
            fx->radial,
            PackFxFullFrame(fx->radial),
            PACK_FX_EXPLOSION_CORE,
            (Vector2){(1.5f + h * 0.5f) * teeth, (1.5f + h * 0.5f) * teeth},
            h * 360.0f + age * 40.0f,
            PackFxWithAlpha(tint, envelope * 0.84f)
        );
    }

    for (int i = 0; i < 4; i++) {
        float t = (float)i / 3.0f;
        float size = 1.7f + t * 1.65f;
        Color tint = PackFxMix((Color){255, 245, 255, 255}, (Color){255, 120, 205, 255}, t);
        PackFxBillboard(
            camera,
            fx->flare,
            PackFxFullFrame(fx->flare),
            PACK_FX_EXPLOSION_CORE,
            (Vector2){size, size},
            0.0f,
            PackFxWithAlpha(tint, envelope * 0.55f)
        );
    }

    for (int i = 0; i < 10; i++) {
        uint32_t seed = (uint32_t)(i * 149 + 5);
        float h0 = PackFxHash01(seed);
        float h1 = PackFxHash01(seed + 41u);
        float life = 0.4f + h0 * 0.2f;
        float phase = PackFxFrac(age / life + h1);
        float angle = h0 * 2.0f * PI;
        Vector3 position = {
            PACK_FX_EXPLOSION_ARCS.x + cosf(angle) * 1.6f,
            PACK_FX_EXPLOSION_ARCS.y + sinf(angle) * 1.6f * 0.6f,
            PACK_FX_EXPLOSION_ARCS.z - 0.05f,
        };
        Color tint = PackFxMix((Color){140, 170, 255, 255}, (Color){255, 200, 245, 255}, phase);
        float flicker = PackFxFrac(phase * 8.0f) < 0.25f ? 0.0f : 1.0f;
        PackFxBillboard(
            camera,
            fx->arc,
            PackFxAtlasFrame(fx->arc, 1, PACK_FX_ARC_ROWS, (int)(h1 * 4.0f) + (int)(phase * 2.0f)),
            position,
            (Vector2){1.8f, 0.4f},
            angle * RAD2DEG + 90.0f,
            PackFxWithAlpha(tint, envelope * flicker)
        );
        PackFxBillboard(
            camera,
            fx->gobo,
            PackFxFullFrame(fx->gobo),
            position,
            (Vector2){0.5f + h1 * 1.0f, 0.5f + h1 * 1.0f},
            0.0f,
            PackFxWithAlpha((Color){170, 200, 255, 255}, envelope * (1.0f - phase) * 0.5f)
        );
    }
}

#define PACK_FX_LIGHTNING_GLOW_ALPHA 0.35f
static void PackFxDrawLightning(PackFx *fx, Camera3D camera, float age) {
    for (int i = 0; i < 8; i++) {
        uint32_t seed = (uint32_t)(i * 617 + 23);
        float h0 = PackFxHash01(seed);
        float h1 = PackFxHash01(seed + 13u);
        float phase = PackFxFrac(age / 1.2f + h0);
        if (phase > 0.383f && phase < 0.399f) continue;
        Color tint =
            phase < 0.373f
                ? PackFxMix((Color){99, 136, 188, 255}, (Color){255, 220, 255, 255}, phase / 0.373f)
                : PackFxMix((Color){255, 220, 255, 255}, (Color){79, 106, 206, 255}, (phase - 0.373f) / 0.385f);
        Vector3 position = {
            PACK_FX_LIGHTNING_ROOT.x + (h0 - 0.5f) * 1.5f,
            PACK_FX_LIGHTNING_ROOT.y + (h1 - 0.5f) * 0.9f,
            PACK_FX_LIGHTNING_ROOT.z + (PackFxHash01(seed + 29u) - 0.5f) * 0.1f,
        };
        if (i < 4) {
            PackFxBillboard(
                camera,
                fx->arc,
                PackFxAtlasFrame(fx->arc, 1, PACK_FX_ARC_ROWS, (int)(h0 * 4.0f) + (int)(phase * 2.0f)),
                position,
                (Vector2){0.85f, 0.10f},
                h1 * 360.0f - 180.0f,
                PackFxWithAlpha(tint, 1.0f - phase)
            );
            continue;
        }
        float size = (1.2f + h1 * 0.6f) * (1.0f - phase);
        PackFxBillboard(
            camera,
            fx->volt,
            PackFxAtlasFrame(fx->volt, PACK_FX_VOLT_COLUMNS, PACK_FX_VOLT_ROWS, (int)(h0 * 4.0f) + (int)(phase * 2.0f)),
            position,
            (Vector2){size, size},
            h1 * 360.0f - 180.0f,
            PackFxWithAlpha(tint, PACK_FX_LIGHTNING_GLOW_ALPHA)
        );
    }
}

static void PackFxDrawFlash(PackFx *fx, Camera3D camera, float time) {
    float age = (time - PACK_FX_FLASH) / 0.15f;
    if (age < 0.0f || age > 1.0f) return;
    float alpha = age < 0.35f ? age / 0.35f : 1.0f - (age - 0.35f) / 0.65f;
    PackFxBillboard(
        camera,
        fx->glow,
        PackFxFullFrame(fx->glow),
        PACK_FX_FLASH_ROOT,
        (Vector2){1.9f, 1.9f},
        0.0f,
        PackFxWithAlpha((Color){221, 69, 49, 255}, alpha)
    );
}

static void PackFxDrawLandingDust(PackFx *fx, Camera3D camera, float time) {
    float age = PackFxSaturate((time - PACK_FX_DUST) / 0.6f);
    Color cracks = age < 0.19f ? PackFxMix((Color){249, 192, 149, 255}, (Color){255, 83, 0, 255}, age / 0.19f)
                               : PackFxMix((Color){255, 83, 0, 255}, (Color){48, 15, 0, 255}, (age - 0.19f) / 0.81f);
    PackFxBillboard(
        camera,
        fx->square,
        PackFxFullFrame(fx->square),
        PACK_FX_CRACKS_ROOT,
        (Vector2){1.6f * (0.4f + age), 0.35f},
        0.0f,
        PackFxWithAlpha(cracks, (1.0f - age) * 0.30f)
    );

    for (int i = 0; i < 40; i++) {
        uint32_t seed = (uint32_t)(i * 271 + 17);
        float h0 = PackFxHash01(seed);
        float h1 = PackFxHash01(seed + 43u);
        float angle = h0 * 2.0f * PI;
        float radius = 0.55f + h1 * 0.40f;
        Vector3 position = {
            PACK_FX_DUST_ROOT.x + cosf(angle) * radius,
            PACK_FX_DUST_ROOT.y + age * 0.35f,
            PACK_FX_DUST_ROOT.z + sinf(angle) * radius * 0.35f,
        };
        Color tint = PackFxMix((Color){255, 214, 150, 255}, (Color){255, 120, 20, 255}, age);
        PackFxBillboard(
            camera,
            fx->beam,
            PackFxFullFrame(fx->beam),
            position,
            (Vector2){0.10f + h1 * 0.06f, 0.45f + h1 * 0.30f},
            0.0f,
            PackFxWithAlpha(tint, 1.0f - age)
        );
    }
}

static void PackFxDrawRarityBurst(PackFx *fx, Camera3D camera, float time, Rarity rarity) {
    float age = (time - PACK_FX_REVEAL) / 1.6f;
    if (age < 0.0f || age > 1.0f) return;
    float env = PackFxSmoothstep(0.0f, 0.22f, age) * (1.0f - PackFxSmoothstep(0.50f, 1.0f, age));

    Color base_star = (Color){77, 56, 25, 255};
    Color vert_burst = (Color){231, 164, 120, 255};
    Color starburst = (Color){109, 123, 129, 255};
    bool twinkle = false, lines = false, waves = false, legendary = false;
    switch (rarity) {
        case RARITY_UNCOMMON:
            vert_burst = (Color){200, 207, 219, 255};
            twinkle = true;
            break;
        case RARITY_RARE:
            base_star = (Color){221, 166, 29, 255};
            vert_burst = (Color){218, 134, 6, 255};
            twinkle = true;
            break;
        case RARITY_EPIC:
            starburst = (Color){176, 34, 37, 255};
            vert_burst = (Color){255, 161, 176, 255};
            twinkle = lines = waves = true;
            break;
        case RARITY_LEGENDARY:
        case RARITY_MYTHIC:
            base_star = starburst = vert_burst = (Color){76, 169, 68, 255};
            waves = legendary = true;
            break;
        default:
            break;
    }

    float fade = 1.0f - age;
    PackFxBillboard(
        camera,
        fx->gobo,
        PackFxFullFrame(fx->gobo),
        PACK_FX_BURST_ROOT,
        (Vector2){3.2f * (1.0f - age), 3.2f * (1.0f - age)},
        age * 45.0f,
        PackFxWithAlpha(base_star, age < 0.686f ? age / 0.686f : fade / 0.314f)
    );
    if (rarity != RARITY_COMMON) {
        float burst = age < 0.05f ? age / 0.05f * 4.0f : fmaxf(0.0f, 4.0f - (age - 0.05f) * 6.0f);
        PackFxBillboard(
            camera,
            fx->shine,
            PackFxFullFrame(fx->shine),
            PACK_FX_BURST_ROOT,
            (Vector2){burst, burst},
            0.0f,
            PackFxWithAlpha(starburst, fade)
        );
    }

    static const float VERT_BURST[3][2] = {{1.05f, 2.6f}, {1.60f, 3.6f}, {2.40f, 4.4f}};
    for (int i = 0; i < 3; i++) {
        PackFxBillboard(
            camera,
            fx->beam,
            PackFxFullFrame(fx->beam),
            (Vector3){PACK_FX_BURST_ROOT.x, PACK_FX_BURST_ROOT.y - 1.03f + age * 1.5f, PACK_FX_BURST_ROOT.z},
            (Vector2){VERT_BURST[i][0], VERT_BURST[i][1] + age * 7.3f},
            0.0f,
            PackFxWithAlpha(vert_burst, env)
        );
    }

    if (twinkle) {
        for (int i = 0; i < 60; i++) {
            uint32_t seed = (uint32_t)(i * 331 + 19);
            float h0 = PackFxHash01(seed);
            float h1 = PackFxHash01(seed + 7u);
            float angle = h0 * 2.0f * PI;
            float travel = age * (2.2f + h1 * 1.8f);
            Vector3 position = {
                PACK_FX_BURST_ROOT.x + cosf(angle) * travel * 0.5f,
                PACK_FX_BURST_ROOT.y + travel * 0.6f,
                PACK_FX_BURST_ROOT.z + sinf(angle) * travel * 0.2f,
            };
            PackFxBillboard(
                camera,
                fx->star,
                PackFxFullFrame(fx->star),
                position,
                (Vector2){0.10f + h1 * 0.22f, 0.10f + h1 * 0.22f},
                h0 * 360.0f,
                PackFxWithAlpha(PackFxMix((Color){227, 172, 160, 255}, (Color){255, 41, 47, 255}, age), env)
            );
        }
    }

    if (lines) {
        for (int i = 0; i < 16; i++) {
            uint32_t seed = (uint32_t)(i * 523 + 3);
            float h0 = PackFxHash01(seed);
            float travel = age * (4.0f + h0 * 6.0f);
            PackFxBillboard(
                camera,
                fx->flare,
                PackFxFullFrame(fx->flare),
                (Vector3){
                    PACK_FX_BURST_ROOT.x + (h0 - 0.5f) * 0.5f,
                    PACK_FX_BURST_ROOT.y + travel,
                    PACK_FX_BURST_ROOT.z + (PackFxHash01(seed + 11u) - 0.5f) * 0.5f,
                },
                (Vector2){0.06f, 1.2f},
                0.0f,
                PackFxWithAlpha((Color){248, 134, 16, 255}, fade * 0.8f)
            );
        }
    }

    if (waves) {
        float wave = PackFxSaturate(age / 0.35f);
        PackFxBillboard(
            camera,
            fx->ring,
            PackFxFullFrame(fx->ring),
            PACK_FX_BURST_ROOT,
            (Vector2){wave * 4.5f, wave * 4.5f},
            0.0f,
            PackFxWithAlpha(legendary ? (Color){104, 255, 120, 255} : (Color){255, 82, 42, 255}, 1.0f - wave)
        );
        float wave2 = PackFxSaturate((age - 0.05f) / 0.35f);
        if (wave2 > 0.0f) {
            PackFxBillboard(
                camera,
                fx->ring_explode,
                PackFxFullFrame(fx->ring_explode),
                PACK_FX_BURST_ROOT,
                (Vector2){wave2 * 3.8f, wave2 * 3.8f},
                0.0f,
                PackFxWithAlpha((Color){255, 62, 0, 255}, 1.0f - wave2)
            );
        }
    }

    if (legendary) {
        for (int i = 0; i < 32; i++) {
            uint32_t seed = (uint32_t)(i * 907 + 31);
            float h0 = PackFxHash01(seed);
            float angle = h0 * 2.0f * PI;
            float travel = age * 6.0f;
            PackFxBillboard(
                camera,
                fx->spark,
                PackFxFullFrame(fx->spark),
                (Vector3){
                    PACK_FX_BURST_ROOT.x + cosf(angle) * travel * 0.7f,
                    PACK_FX_BURST_ROOT.y + travel * 0.8f,
                    PACK_FX_BURST_ROOT.z + sinf(angle) * travel * 0.25f,
                },
                (Vector2){0.06f, 0.5f},
                angle * RAD2DEG,
                PackFxWithAlpha((Color){76, 169, 68, 255}, fade)
            );
        }
        float second = PackFxSaturate((age * 2.0f - 0.9f) / 0.7f);
        if (second > 0.0f) {
            PackFxBillboard(
                camera,
                fx->shockwave,
                PackFxFullFrame(fx->shockwave),
                PACK_FX_BURST_ROOT,
                (Vector2){second * 5.0f, second * 5.0f},
                0.0f,
                PackFxWithAlpha((Color){33, 122, 5, 255}, (1.0f - second) * 0.46f)
            );
        }
    }
}

void PackFxDraw(PackFx *fx, Camera3D camera, float timeline, bool final, Rarity rarity) {
    if (!fx || !fx->initialized) return;
    if (!isfinite(timeline) || timeline < 0.0f) timeline = 0.0f;

    BeginMode3D(camera);
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    PackFxDrawStormSky(fx, camera, timeline);
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);

    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    PackFxDrawWisp(fx, camera, timeline);
    if (!final) {
        if (timeline < PACK_FX_ROULETTE_END) {
            PackFxDrawArcs(fx, camera, timeline);
            PackFxDrawStrobe(fx, camera, timeline);
        }
        if (timeline < PACK_FX_LANDING) PackFxDrawStrobeGlows(fx, camera, timeline);
        if (PackFxBetween(timeline, PACK_FX_ROULETTE_END, PACK_FX_EXPLOSION_END))
            PackFxDrawExplosion(fx, camera, timeline);
    }
    rlDrawRenderBatchActive();
    rlEnableDepthTest();

    if (!final) {
        if (PackFxBetween(timeline, PACK_FX_CLOSING, PACK_FX_REVEAL)) {
            float restart = timeline < PACK_FX_LANDING ? PACK_FX_CLOSING : PACK_FX_LANDING;
            PackFxDrawLightning(fx, camera, timeline - restart);
        }
        PackFxDrawFlash(fx, camera, timeline);
        if (PackFxBetween(timeline, PACK_FX_DUST, PACK_FX_DUST_END)) PackFxDrawLandingDust(fx, camera, timeline);
    }
    if (timeline < PACK_FX_BURST_END) PackFxDrawRarityBurst(fx, camera, timeline, rarity);
    for (int brazier = 0; brazier < 2; brazier++) PackFxDrawFire(fx, camera, timeline, brazier);

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    EndMode3D();
}
