
#define PACK_HERO_YAW PI
#define PACK_HERO_TALL_BOUNDS 3.0f

typedef struct PackHero {
    HomeSquadUnit unit;

    Shader shader;
    bool shader_loaded;
    bool loaded;
    bool visible;
    bool tall;

    char char_dir[256];
} PackHero;

bool PackHeroInitialize(PackHero *hero, const char *char_dir) {
    if (!hero || !char_dir || !char_dir[0]) return false;
    memset(hero, 0, sizeof(*hero));
    StrCopy(hero->char_dir, sizeof(hero->char_dir), char_dir);

    hero->shader = LoadShaderFromMemory(VS_UNLIT, FS_PLAIN);
    hero->shader_loaded = IsShaderValid(hero->shader);
    if (!hero->shader_loaded) {
        TraceLog(LOG_WARNING, "PackHero: failed to load unit shader");
        return false;
    }
    float brightness = 1.12f;
    SetShaderValue(hero->shader, GetShaderLocation(hero->shader, "brightness"), &brightness, SHADER_UNIFORM_FLOAT);
    return true;
}

void PackHeroClear(PackHero *hero) {
    if (!hero) return;
    HomeSquadUnitDestroy(&hero->unit);
    hero->loaded = false;
    hero->visible = false;
    hero->tall = false;
}

bool PackHeroSetCharacter(PackHero *hero, const Character *character) {
    if (!hero || !hero->shader_loaded || !character) return false;
    PackHeroClear(hero);

    char path[512];
    const char *prefab = HomeSquadResolvePrefab(hero->char_dir, character, path, sizeof(path));
    if (!prefab) {
        TraceLog(LOG_WARNING, "PackHero: %s has no baked model", character->id);
        return false;
    }

    float scale = UnitDisplayScale(prefab);
    Matrix xform = MatrixMultiply(
        MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(PACK_HERO_YAW)),
        MatrixTranslate(PACK_UNIT_X, PACK_UNIT_FEET, PACK_UNIT_Z)
    );
    HomeSquadUnitLoad(&hero->unit, path, hero->shader, xform, 0);
    hero->tall = Vector3Length(Vector3Scale(GetModelBoundingBox(hero->unit.model).max, scale)) > PACK_HERO_TALL_BOUNDS;

    hero->loaded = true;
    hero->visible = false;
    hero->unit.fade = 0.0f;
    return true;
}

void PackHeroSetVisible(PackHero *hero, bool visible) {
    if (!hero) return;
    hero->visible = visible;
    hero->unit.fade = visible ? 1.0f : 0.0f;
}

bool PackHeroIsTall(const PackHero *hero) {
    return hero && hero->loaded && hero->tall;
}

bool PackHeroPlayMelee(PackHero *hero) {
    return hero && hero->loaded && HomeSquadUnitPlayMelee(&hero->unit);
}

void PackHeroUpdate(PackHero *hero, float dt) {
    if (!hero || !hero->loaded) return;
    HomeSquadUnitUpdate(&hero->unit, fmaxf(dt, 0.0f));
}

void PackHeroDraw(PackHero *hero, Camera3D camera) {
    if (!hero || !hero->loaded || !hero->visible) return;
    BeginMode3D(camera);
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    HomeSquadUnitDraw(&hero->unit);
    EndMode3D();
}

void PackHeroDestroy(PackHero *hero) {
    if (!hero) return;
    PackHeroClear(hero);
    if (hero->shader_loaded) UnloadShader(hero->shader);
    memset(hero, 0, sizeof(*hero));
}
