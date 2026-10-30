
#define PACK_HERO_YAW PI
#define PACK_HERO_TALL_BOUNDS 3.0f

typedef struct PackHero {
    HomeSquadUnit unit;

    ModelAnimation *clips;
    int clip_count;
    int melee_clip;

    Shader shader;
    bool shader_loaded;
    bool loaded;
    bool melee_playing;
    bool visible;
    bool tall;

    float melee_frame;

    char char_dir[256];
} PackHero;

static void PackHeroClipKey(char out[64], const char *name) {
    int length = 0;
    if (!name) name = "";
    for (; *name && length < 63; name++) {
        char c = *name;
        if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) out[length++] = c;
    }
    out[length] = '\0';
}

static int PackHeroMeleeRank(const char *name) {
    char key[64];
    PackHeroClipKey(key, name);
    if (!key[0]) return -1;
    if (!strcmp(key, "melee") || !strcmp(key, "meleeattack")) return 100;
    if (strstr(key, "melee") && strstr(key, "attack")) return 95;
    if (strstr(key, "attack")) return 90;
    if (strstr(key, "attk") || strstr(key, "atk")) return 80;
    if (strstr(key, "meleeability")) return 75;
    if (strstr(key, "projectileability") || strstr(key, "fireball")) return 60;
    return -1;
}

static void PackHeroReleaseClips(PackHero *hero) {
    if (!hero || !hero->clips) return;
    for (int i = 0; i < hero->clip_count; i++) UnloadModelAnimation(hero->clips[i]);
    MemFree(hero->clips);
    hero->clips = NULL;
    hero->clip_count = 0;
    hero->melee_clip = -1;
}

bool PackHeroInitialize(PackHero *hero, const char *char_dir) {
    if (!hero || !char_dir || !char_dir[0]) return false;
    memset(hero, 0, sizeof(*hero));
    hero->melee_clip = -1;
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
    hero->melee_playing = false;
    PackHeroReleaseClips(hero);
    HomeSquadUnitDestroy(&hero->unit);
    hero->loaded = false;
    hero->visible = false;
    hero->tall = false;
    hero->melee_frame = 0.0f;
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

    hero->clips = LoadModelAnimations(path, &hero->clip_count);
    int best_rank = -1;
    for (int i = 0; i < hero->clip_count; i++) {
        if (hero->clips[i].frameCount <= 0 || !IsModelAnimationValid(hero->unit.model, hero->clips[i])) continue;
        int rank = PackHeroMeleeRank(hero->clips[i].name);
        if (rank > best_rank) {
            hero->melee_clip = i;
            best_rank = rank;
        }
    }

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

bool PackHeroHasMelee(const PackHero *hero) {
    return hero && hero->loaded && hero->melee_clip >= 0 && hero->melee_clip < hero->clip_count;
}

bool PackHeroPlayMelee(PackHero *hero) {
    if (!PackHeroHasMelee(hero)) return false;
    const ModelAnimation *clip = &hero->clips[hero->melee_clip];
    hero->melee_playing = true;
    hero->melee_frame = 0.0f;
    hero->unit.anim_override = true;
    UpdateModelAnimation(hero->unit.model, *clip, 0);
    return true;
}

void PackHeroUpdate(PackHero *hero, float dt) {
    if (!hero || !hero->loaded) return;
    if (dt < 0.0f) dt = 0.0f;

    if (hero->melee_playing && PackHeroHasMelee(hero)) {
        const ModelAnimation *clip = &hero->clips[hero->melee_clip];
        hero->melee_frame += dt * HOME_CLIP_FPS;
        int frame = (int)hero->melee_frame;
        if (frame >= clip->frameCount) {
            hero->melee_playing = false;
            hero->unit.anim_override = false;
            hero->unit.last_frame = -1;
            HomeSquadUnitUpdate(&hero->unit, 0.0f);
        } else {
            UpdateModelAnimation(hero->unit.model, *clip, frame);
        }
    } else {
        hero->melee_playing = false;
        hero->unit.anim_override = false;
        HomeSquadUnitUpdate(&hero->unit, dt);
    }
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
