#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>

static HomeSquadUnit PooledTestUnit(const char *path) {
    HomeSquadUnit unit = {.active = true, .phase = 7, .frame_time = 3.0f};
    StrCopy(unit.path, sizeof(unit.path), path);
    return unit;
}

static void TestUnitPoolReusesReleasedUnits(void) {
    HomeSquadUnit unit = PooledTestUnit("a.glb");
    HomeSquadUnitDestroy(&unit);
    assert(!unit.active);
    assert(unit_pool_count == 1);

    HomeSquadUnit taken = {0};
    assert(!UnitPoolTake(&taken, "b.glb"));
    assert(UnitPoolTake(&taken, "a.glb"));
    assert(taken.active && taken.phase == 7 && taken.frame_time == 3.0f);
    assert(unit_pool_count == 0);
    assert(!UnitPoolTake(&taken, "a.glb"));

    HomeSquadUnit first = PooledTestUnit("first.glb");
    UnitPoolPut(&first);
    for (int i = 1; i < UNIT_POOL_CAPACITY; i++) {
        HomeSquadUnit filler = PooledTestUnit(TextFormat("filler%d.glb", i));
        UnitPoolPut(&filler);
    }
    assert(unit_pool_count == UNIT_POOL_CAPACITY);
    HomeSquadUnit newest = PooledTestUnit("newest.glb");
    UnitPoolPut(&newest);
    assert(unit_pool_count == UNIT_POOL_CAPACITY);
    assert(!UnitPoolTake(&taken, "first.glb"));
    assert(UnitPoolTake(&taken, "filler1.glb"));
    assert(UnitPoolTake(&taken, "newest.glb"));
    UnitPoolClear();
    assert(unit_pool_count == 0);
}

static void TestWispIdleDriftsLikeTheOriginalFx(void) {
    assert(WispDriftProgress(0.0f) == 0.0f);
    assert(fabsf(WispDriftProgress(0.25f) - 0.15625f) < 1e-5f);
    assert(fabsf(WispDriftProgress(0.5f) - 0.5f) < 1e-5f);
    assert(fabsf(WispDriftProgress(1.0f) - 1.0f) < 1e-5f);
    assert(fabsf(WispDriftProgress(1.75f) - 0.15625f) < 1e-5f);

    Quaternion chest_rest = QuaternionNormalize((Quaternion){0.4669f, 0.5310f, 0.4669f, 0.5310f});
    Vector3 reach = WispDriftReach(chest_rest);
    assert(Vector3Distance(reach, (Vector3){0.2f, -0.1856f, 0.1248f}) < 1e-3f);

    HomeSquadUnit wisp = {.active = true, .wisp_fx = "fx_wispchar_t1_pre", .wisp_bone = -1, .wisp_drift_reach = reach};
    wisp.xform = MatrixMultiply(
        MatrixMultiply(MatrixScale(2.0f, 2.0f, 2.0f), MatrixRotateY(PI / 2.0f)), MatrixTranslate(10.0f, 0.0f, 0.0f)
    );
    Vector3 rest = HomeSquadWispPosition(&wisp);
    HomeSquadUnitUpdate(&wisp, 1.0f);
    Vector3 turned_reach =
        Vector3RotateByQuaternion(reach, QuaternionFromAxisAngle((Vector3){0.0f, 1.0f, 0.0f}, PI / 2.0f));
    assert(Vector3Distance(Vector3Subtract(HomeSquadWispPosition(&wisp), rest), turned_reach) < 1e-4f);
    HomeSquadUnitUpdate(&wisp, 1.0f);
    assert(Vector3Distance(HomeSquadWispPosition(&wisp), rest) < 1e-4f);
}

static void TestCharacter(
    Character *ch, Ability *ability, const char *id, const char *key, TargetPattern target, int power, int health
) {
    memset(ch, 0, sizeof(*ch));
    memset(ability, 0, sizeof(*ability));
    StrCopy(ch->id, sizeof(ch->id), id);
    StrCopy(ability->key, sizeof(ability->key), key);
    ability->target = target;
    ch->ability = ability;
    ch->tier = TIER_I;
    ch->level = 1;
    ch->speed = SPEED_NORMAL;
    ch->stats[TIER_I].base_power = power;
    ch->stats[TIER_I].base_health = health;
}

static void SimUnitInit(BattleSim *s, int side, int slot, const Character *ch) {
    SimUnitsInit(s->units, side, slot, ch, NULL);
}

static void SimApplyBattleStartPassives(BattleSim *s) {
    SimUnitsBattleStartPassives(s->units);
}

static void SimApplyFormationBonus(BattleSim *s, int side) {
    SimUnitsFormationBonus(s->units, side, &s->formation_faction[side], &s->formation_bonus_pct[side]);
}

static void TestSquadSlotReferenceLayout(void) {
    const int expected_position[SQUAD_SLOT_COUNT] = {2, 0, 3, 1, -1};
    const int expected_x[SQUAD_SLOT_COUNT] = {1, 0, 1, 0, -1};
    const int expected_y[SQUAD_SLOT_COUNT] = {0, 0, 1, 1, -1};
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        assert(SquadSlotReferencePosition(slot) == expected_position[slot]);
        assert(SquadSlotReferenceX(slot) == expected_x[slot]);
        assert(SquadSlotReferenceY(slot) == expected_y[slot]);
        assert(SquadSlotIsFront(slot) == (slot == 1 || slot == 3));
        assert(SquadSlotIsBack(slot) == (slot == 0 || slot == 2));
    }
    assert(SquadSlotReferencePosition(-1) == -1);
    assert(SquadSlotReferenceX(SQUAD_SLOT_COUNT) == -1);
    assert(SquadSlotReferenceY(SQUAD_SLOT_COUNT) == -1);
    assert(!SquadSlotIsFront(-1) && !SquadSlotIsBack(-1));
    assert(!SquadSlotIsFront(SQUAD_SLOT_COUNT) && !SquadSlotIsBack(SQUAD_SLOT_COUNT));
}

static void TestBattleRowBonuses(void) {
    BattleSim s = {0};
    Character characters[SQUAD_SLOT_COUNT];
    Ability abilities[SQUAD_SLOT_COUNT];
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        TestCharacter(&characters[slot], &abilities[slot], "row_bonus", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 1001);
        characters[slot].giant = slot == SQUAD_SLOT_LARGE;
        SimUnitInit(&s, 0, slot, &characters[slot]);
    }

    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const SimUnit *unit = &s.units[slot];
        int expected_health = SquadSlotIsFront(slot) ? 1502 : 1001;
        float expected_crit = SquadSlotIsBack(slot) ? 1.5f : 1.0f;
        assert(unit->max_health == expected_health);
        assert(unit->health == expected_health);
        assert(fabsf(unit->crit_pct - expected_crit) < 0.001f);
    }
}

static void TestHeroRowBonusReferencePresentation(void) {
    const int front_slots[] = {1, 3};
    const int back_slots[] = {0, 2};
    for (int i = 0; i < 2; i++) {
        HeroRowBonusSpec front = HeroRowBonusForSlot(front_slots[i]);
        assert(front.effect == HFX_HEAL);
        assert(strcmp(front.vfx_name, "vfx.fx_heal_pre") == 0);
        assert(front.texture == GUI_HEROES_ROW_HEALTH && front.percent == 50);

        HeroRowBonusSpec back = HeroRowBonusForSlot(back_slots[i]);
        assert(back.effect == HFX_CRIT);
        assert(strcmp(back.vfx_name, "vfx.fx_crit_pre") == 0);
        assert(back.texture == GUI_HEROES_ROW_CRIT && back.percent == 50);
    }
    HeroRowBonusSpec none = HeroRowBonusForSlot(SQUAD_SLOT_LARGE);
    assert(none.effect == HFX_NONE && none.vfx_name == NULL && none.texture == GUI_COUNT && none.percent == 0);

    static const char *const health_ref = CACHE_RESOURCE("tex.rowbonus_front_health");
    static const char *const crit_ref = CACHE_RESOURCE("tex.rowbonus_back_critchance");
    assert(strcmp(gui_paths[GUI_HEROES_ROW_HEALTH], health_ref) == 0);
    assert(strcmp(gui_paths[GUI_HEROES_ROW_CRIT], crit_ref) == 0);
    assert(ResourceExists(health_ref) && ResourceExists(crit_ref));
    Image health = ResourceLoadImage(health_ref), crit = ResourceLoadImage(crit_ref);
    assert(health.data && health.width == 512 && health.height == 512);
    assert(crit.data && crit.width == 512 && crit.height == 512);
    UnloadImage(health);
    UnloadImage(crit);

    assert(HERO_ROW_BONUS_MOVE_TIME == 0.5f);
    assert(HERO_ROW_BONUS_DISPLAY_TIME == 2.0f);
    assert(HERO_ROW_BONUS_FADE_TIME == 0.375f);
    assert(HeroRowBonusAlphaAt(0.0f) == 0.0f);
    assert(HeroRowBonusAlphaAt(0.25f) > 0.0f && HeroRowBonusAlphaAt(0.25f) < 1.0f);
    assert(HeroRowBonusAlphaAt(0.5f) == 1.0f);
    assert(HeroRowBonusAlphaAt(2.0f) == 1.0f);
    assert(HeroRowBonusAlphaAt(2.1875f) > 0.0f && HeroRowBonusAlphaAt(2.1875f) < 1.0f);
    assert(HeroRowBonusAlphaAt(2.375f) == 0.0f);
    const HeroFxHermiteKey reference_curve[] = {{0, 1, 0, 0}, {1, 0, -2, -2}};
    assert(fabsf(HeroFxHermiteCurve(reference_curve, 2, 0.25f) - 0.9375f) < 0.000001f);
    assert(fabsf(HeroFxHermiteCurve(reference_curve, 2, 0.5f) - 0.75f) < 0.000001f);
    Rectangle start = HeroRowBonusRectAt(0.0f, 1280.0f, 720.0f);
    Rectangle target = HeroRowBonusRectAt(0.5f, 1280.0f, 720.0f);
    assert(start.y > target.y && target.y == 73.0f);
    assert(target.width == 404.0f && target.height == 202.0f);

    HeroesBackground scene = {0};
    HeroRowBonusStart(&scene, 1);
    assert(scene.fx_effect == HFX_HEAL && scene.row_bonus_effect == HFX_HEAL);
    assert(Vector3Distance(scene.fx_origin, HERO_FORMATION_SLOTS[1].pos) < 0.000001f);
    scene.fx[0].alive = true;
    HeroRowBonusStart(&scene, 0);
    assert(scene.fx_effect == HFX_CRIT && scene.row_bonus_effect == HFX_CRIT && !scene.fx[0].alive);
    assert(Vector3Distance(scene.fx_origin, HERO_FORMATION_SLOTS[0].pos) < 0.000001f);
    HeroRowBonusStart(&scene, SQUAD_SLOT_LARGE);
    assert(scene.fx_effect == HFX_NONE && scene.row_bonus_effect == HFX_NONE);
    HeroRowBonusStart(&scene, 1);
    HeroesBackgroundClearRowBonus(&scene);
    assert(scene.fx_effect == HFX_NONE && scene.row_bonus_effect == HFX_NONE && scene.row_bonus_time == 0.0f);

    HeroFxStart(&scene, HFX_HEAL, (Vector3){0});
    HeroFxUpdate(&scene, 0.01f);
    int heal_dots = 0;
    for (int i = 0; i < HERO_FX_MAX; i++) heal_dots += scene.fx[i].alive && scene.fx[i].kind == HFXP_HEAL_DOT;
    assert(heal_dots == 6);

    HeroFxStart(&scene, HFX_CRIT, (Vector3){0});
    HeroFxUpdate(&scene, 0.001f);
    int smoke = 0;
    for (int i = 0; i < HERO_FX_MAX; i++) {
        if (!scene.fx[i].alive || scene.fx[i].kind != HFXP_CRIT_SMOKE) continue;
        assert(scene.fx[i].frame == 1 || scene.fx[i].frame == 3);
        smoke++;
    }
    assert(smoke == 8);

    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    assert(strcmp(LocGet("SQUAD_ROW_BONUS"), "Бонус шеренги!") == 0);
    LocUnload();
}

static int RecordedPowerDelta(const BattleSim *s, int unit) {
    int total = 0;
    for (int i = 0; i < s->action_count; i++)
        for (int t = 0; t < s->actions[i].target_count; t++)
            if (s->actions[i].targets[t].unit == unit) total += s->actions[i].targets[t].power_delta;
    return total;
}

static void TestFormationBonusMetadata(void) {
    BattleSim s = {0};
    Character units[10];
    Ability abilities[10];
    const Rarity rarity[4] = {RARITY_COMMON, RARITY_UNCOMMON, RARITY_RARE, RARITY_EPIC};
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < 4; slot++) {
            int i = side * 5 + slot;
            TestCharacter(&units[i], &abilities[i], "formation", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 1000);
            units[i].rarity = side == 0 ? rarity[slot] : RARITY_LEGENDARY;
            units[i].faction = side == 0 ? FACTION_WHITE : FACTION_BLACK;
            SimUnitInit(&s, side, slot, &units[i]);
        }
        int i = side * 5 + 4;
        TestCharacter(&units[i], &abilities[i], "giant", "ABILITY_DAMAGE_ONE", TARGET_ONE, 500, 5000);
        units[i].rarity = RARITY_LEGENDARY;
        units[i].giant = true;
        units[i].faction = FACTION_LARGE;
        SimUnitInit(&s, side, 4, &units[i]);
        SimApplyFormationBonus(&s, side);
    }

    assert(s.formation_faction[0] == 2u);
    assert(fabsf(s.formation_bonus_pct[0] - 10.0f) < 0.001f);
    assert(s.units[0].power == 110 && s.units[3].power == 110);
    assert(s.units[4].power == 500);

    assert(s.formation_faction[1] == 1u);
    assert(fabsf(s.formation_bonus_pct[1] - 20.0f) < 0.001f);
    assert(s.units[SQUAD_SLOT_COUNT].power == 120);

    BattlePlay bp = {.sim = s};
    BpBuildIntroEvents(&bp);
    assert(bp.intro_event_count == 1);
    assert(bp.intro_events[0].affected_slots == 0x0fu);
    assert(strcmp(BpFactionFx(bp.intro_events[0].faction), "fx_eff_faction_charge_pre") == 0);

    bp.sim.formation_bonus_pct[0] = 0.0f;
    BpBuildIntroEvents(&bp);
    assert(bp.intro_event_count == 0 && bp.intro_event == -1);
}

static void TestFormationBonusRequiresSharedFaction(void) {
    BattleSim s = {0};
    Character units[4];
    Ability abilities[4];
    const Faction faction[4] = {FACTION_WHITE, FACTION_WHITE_RED, FACTION_WHITE_BLUE, FACTION_BLACK};
    for (int slot = 0; slot < 4; slot++) {
        TestCharacter(&units[slot], &abilities[slot], "formation", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 1000);
        units[slot].rarity = RARITY_LEGENDARY;
        units[slot].faction = faction[slot];
        SimUnitInit(&s, 0, slot, &units[slot]);
    }
    SimApplyFormationBonus(&s, 0);
    assert(s.formation_faction[0] == 0u && s.formation_bonus_pct[0] == 0.0f);
    for (int slot = 0; slot < 4; slot++) assert(s.units[slot].power == 100);
}

static void TestIntroQueueShowsOnlyPlayerFactionBonus(void) {
    BattlePlay bp = {0};
    Character characters[8] = {0};
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < 4; slot++) {
            SimUnit *unit = &bp.sim.units[side * SQUAD_SLOT_COUNT + slot];
            unit->present = true;
            unit->ch = &characters[side * 4 + slot];
        }
    }

    bp.sim.formation_faction[0] = 2u;
    bp.sim.formation_faction[1] = 1u;
    bp.sim.formation_bonus_pct[0] = 10.0f;
    bp.sim.formation_bonus_pct[1] = 20.0f;

    BpBuildIntroEvents(&bp);

    assert(bp.intro_event_count == 1);
    assert(bp.intro_events[0].faction == 2u);
}

static void TestProjectileTrailFollowsFlight(Battle *battle) {
    const Vector3 flight = {24, 0, 0};
    const FxEmitter *lance = &BattleFxFindPreset(battle, "vfx_sentinellance_pre")->em[1];
    assert(lance->count > 0 && lance->orient == 2 && !lance->follows_emitter);
    FxParticle lance_particle = {.em = lance};
    FxTrailParticleLaunch(&lance_particle, flight);
    assert(fabsf(lance_particle.vel.x - 168.0f) < 0.1f);

    const FxEmitter *bolt = &BattleFxFindPreset(battle, "fx_Duke_prjctl_pre")->em[1];
    assert(bolt->count > 0 && bolt->orient == 2 && bolt->follows_emitter);
    FxParticle bolt_particle = {.em = bolt};
    FxTrailParticleLaunch(&bolt_particle, flight);
    assert(bolt_particle.carried_vel.x == flight.x && bolt_particle.vel.x == 0.0f);

    battle->fxp_count = 2;
    battle->fxp[0] = (FxParticle){.serial = 7};
    battle->fxp[1] = (FxParticle){.serial = 8};
    BattleFxTrailEnd(battle, 7);
    assert(battle->fxp_count == 1 && battle->fxp[0].serial == 8);
    battle->fxp_count = 0;
}

static void TestBreathStartsAtTheJawBone(Battle *battle) {
    BattleWiresLoad(battle, "data/unit_vfx.json");
    const BattleVfxWire *dragon = BattleUnitWire(battle, "UNIT_HIGHDRAGON", TIER_IV);
    assert(dragon && strcmp(dragon->muzzle_bone, "HighDragon_Jaw") == 0);
    assert(Vector3Length(dragon->muzzle_offset) < 0.2f);
    const BattleVfxWire *storm = BattleUnitWire(battle, "UNIT_GAMORDANSTORMRIDER", TIER_III);
    assert(storm && storm->muzzle_bone[0] && Vector3Length(storm->muzzle_offset) < 1.0f);
    const BattleVfxWire *archer = BattleUnitWire(battle, "UNIT_SENTINELARCHER", TIER_IV);
    assert(archer && !archer->muzzle_bone[0]);

    BoneInfo bones[2] = {{.name = "Root"}, {.name = "HighDragon_Jaw"}};
    Transform pose[2] = {
        {.rotation = QuaternionIdentity()},
        {.translation = {1, 2, 3}, .rotation = QuaternionFromAxisAngle((Vector3){0, 1, 0}, PI / 2.0f)},
    };
    HomeSquadUnit unit = {
        .active = true,
        .pose_ready = true,
        .pose = pose,
        .pose_bone_count = 2,
        .model = {.bones = bones},
        .xform = MatrixTranslate(10, 0, 0),
    };
    Vector3 position, aim;
    assert(HomeSquadUnitBoneAnchor(&unit, "HighDragon_Jaw", (Vector3){0, 0, 1}, (Vector3){0, 0, 1}, &position, &aim));
    assert(Vector3Distance(position, (Vector3){12, 2, 3}) < 0.001f);
    assert(Vector3Distance(aim, (Vector3){13, 2, 3}) < 0.001f);
    assert(!HomeSquadUnitBoneAnchor(&unit, "Missing", Vector3Zero(), Vector3Zero(), &position, &aim));
}

static void TestFactionIntroAssetsAndText(void) {
    assert(BATTLE_UI_FACTION_INTRO_ICON == GUI_DETAILS_POWER);
    assert(ResourceExists(CACHE_RESOURCE("tex.Ico_Power")));
    assert(strcmp(gui_paths[GUI_DETAILS_POWER], CACHE_RESOURCE("tex.Ico_Power")) == 0);
    Battle *battle = calloc(1, sizeof(*battle));
    assert(battle != NULL);
    BattleFxRegistryLoad(battle, "data/fx_presets.json");
    static const char *const names[] = {
        "fx_eff_faction_charge_pre",
        "fx_eff_redfaction_charge_pre",
        "fx_eff_bluefaction_charge_pre",
        "fx_eff_blackfaction_charge_pre",
    };
    for (int i = 0; i < 4; i++) {
        const FxPreset *preset = BattleFxFindPreset(battle, names[i]);
        assert(preset && preset->em_count > 0 && preset->length >= 0.7f);
    }
    battle->rings[0][2].pos = (Vector3){1, 2, 3};
    BattleFxUnitBurst(battle, names[1], 0, 2, (Vector3){0, 0, 0});
    int instance = -1;
    for (int i = 0; i < FX_MAX_INSTANCES; i++)
        if (battle->fxi[i].alive) {
            instance = i;
            break;
        }
    assert(instance >= 0);
    assert(battle->fxi[instance].anchor == 2 && !battle->fxi[instance].loop);
    TestProjectileTrailFollowsFlight(battle);
    TestBreathStartsAtTheJawBone(battle);

    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    BpIntroEvent event = {.value = 10.0f};
    char text[128];
    BattleUiIntroText(&event, text, sizeof(text));
    assert(strcmp(text, "Бонус за фракцию! +10 %") == 0);
    LocUnload();

    if (battle->fx_reg) MemFree(battle->fx_reg);
    if (battle->fx_alias) MemFree(battle->fx_alias);
    free(battle);
}

static void TestPowerDrain(void) {
    BattleSim s = {.rng = 7};
    Character actor, enemy_a, enemy_b;
    Ability actor_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(&actor, &actor_ability, "actor", "attack_all_drain65_crit7", TARGET_ALL, 200, 10000);
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 300, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy_a);
    SimUnitInit(&s, 1, 1, &enemy_b);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);

    assert(s.units[SQUAD_SLOT_COUNT].power == 0);
    assert(s.units[SQUAD_SLOT_COUNT + 1].power == 170);
    assert(RecordedPowerDelta(&s, SQUAD_SLOT_COUNT) == -100);
    assert(RecordedPowerDelta(&s, SQUAD_SLOT_COUNT + 1) == -130);
}

static void TestAllyPowerGain(void) {
    BattleSim s = {.rng = 11};
    Character actor, ally_a, ally_b, enemy;
    Ability actor_ability, ally_a_ability, ally_b_ability, enemy_ability;
    TestCharacter(&actor, &actor_ability, "actor", "attack_all_slow_allypowergain20", TARGET_ALL, 200, 10000);
    TestCharacter(&ally_a, &ally_a_ability, "ally-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&ally_b, &ally_b_ability, "ally-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 150, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 0, 1, &ally_a);
    SimUnitInit(&s, 0, 2, &ally_b);
    SimUnitInit(&s, 1, 0, &enemy);
    s.units[2].curse_power = true;

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);

    assert(s.units[0].power == 200);
    assert(s.units[1].power == 120);
    assert(s.units[2].power == 150);
    assert(RecordedPowerDelta(&s, 1) == 20);
    assert(RecordedPowerDelta(&s, 2) == 0);
}

static void TestAllySlow(void) {
    BattleSim s = {.rng = 12};
    Character actor, ally, enemy;
    Ability actor_ability, ally_ability, enemy_ability;
    TestCharacter(&actor, &actor_ability, "actor", "attack_column_slow_all_slow_ally", TARGET_COLUMN, 200, 10000);
    TestCharacter(&ally, &ally_ability, "ally", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 0, 1, &ally);
    SimUnitInit(&s, 1, 0, &enemy);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);

    assert(s.units[0].cur_initiative == 0);
    assert(s.units[1].cur_initiative == -50);
    assert(s.action_count == 2);
    assert(s.actions[1].target_count == 1);
    assert(s.actions[1].targets[0].initiative_delta == -50);
}

static void TestAllyCritGain(void) {
    BattleSim s = {.rng = 14};
    Character actor, ally, enemy;
    Ability actor_ability, ally_ability, enemy_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_MOTHERGISELLE",
        "ability_heal_allies_crit_boost_allies5_no_attack",
        TARGET_ALL,
        200,
        10000
    );
    TestCharacter(&ally, &ally_ability, "ally", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    actor_ability.mechanics = ABILITY_ATTACK_NONE | ABILITY_HEAL_ALLIES | ABILITY_GAIN_CRIT_ALLIES;
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 0, 1, &ally);
    SimUnitInit(&s, 1, 0, &enemy);

    SimTakeTurn(&s, 0);

    assert(fabsf(s.units[0].crit_pct - 1.5f) < 0.001f);
    assert(fabsf(s.units[1].crit_pct - 6.0f) < 0.001f);
    assert(s.action_count == 1 && s.actions[0].target_count == 1);
    assert(fabsf(s.actions[0].targets[0].crit_delta - 5.0f) < 0.001f);
}

static void TestSlowCarriesIntoNextRound(void) {
    BattleSim s = {.rng = 12};
    Character slowed, enemy;
    Ability slowed_ability, enemy_ability;
    TestCharacter(&slowed, &slowed_ability, "slowed", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &slowed);
    SimUnitInit(&s, 1, 0, &enemy);

    s.units[0].cur_initiative = -50;
    SimTakeTurn(&s, 0);
    assert(s.units[0].cur_initiative == -50);

    s.units[0].stunned = true;
    SimTakeTurn(&s, 0);
    assert(s.units[0].cur_initiative == -50);

    int order[SIM_UNIT_COUNT];
    assert(SimRollRoundOrder(&s, order) == 2);
    assert(order[0] == SQUAD_SLOT_COUNT && order[1] == 0);
    assert(s.units[0].cur_initiative == 0);
}

static void TestTwoRoundCurse(void) {
    BattleSim s = {.rng = 16};
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_XENONTHEANTIQUARIAN",
        "heals_and_powergains_allies_15_and_curses_enemies_2_rounds_no_attack",
        TARGET_ALL,
        200,
        10000
    );
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    AttackRecipe hit = recipe->hit;
    hit.status_only = true;
    SimAttack(&s, 0, recipe, &hit, true, false);
    int target = SQUAD_SLOT_COUNT;
    assert(s.units[target].curse_power && s.units[target].curse_health);
    assert(s.units[target].curse_power_turns == 2 && s.units[target].curse_health_turns == 2);

    SimExpireAilments(&s, target, false);
    assert(s.units[target].curse_power && s.units[target].curse_health);
    assert(s.units[target].curse_power_turns == 1 && s.units[target].curse_health_turns == 1);

    SimExpireAilments(&s, target, false);
    assert(!s.units[target].curse_power && !s.units[target].curse_health);
    assert(
        (s.actions[s.action_count - 1].ailments & (AIL_UNCURSE_POWER | AIL_UNCURSE_HEALTH)) ==
        (AIL_UNCURSE_POWER | AIL_UNCURSE_HEALTH)
    );
}

static void TestBackRowCurse(void) {
    BattleSim s = {.rng = 18};
    Character actor, enemies[4];
    Ability actor_ability, enemy_abilities[4];
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_FERELDENBANNER",
        "slows_all_curse_power_backrow_stun_column",
        TARGET_ALL,
        200,
        10000
    );
    SimUnitInit(&s, 0, 0, &actor);
    for (int slot = 0; slot < 4; slot++) {
        TestCharacter(&enemies[slot], &enemy_abilities[slot], "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
        SimUnitInit(&s, 1, slot, &enemies[slot]);
    }

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->effect, true, false);

    assert(s.units[SQUAD_SLOT_COUNT + 0].curse_power);
    assert(!s.units[SQUAD_SLOT_COUNT + 1].curse_power);
    assert(s.units[SQUAD_SLOT_COUNT + 2].curse_power);
    assert(!s.units[SQUAD_SLOT_COUNT + 3].curse_power);
}

static void TestFallbackSelfCosts(void) {
    BattleSim s = {.rng = 20};
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(&actor, &actor_ability, "actor", "unknown_self_cost", TARGET_ONE, 200, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    actor_ability.mechanics = ABILITY_ATTACK | ABILITY_DAMAGE_SELF | ABILITY_LOSE_POWER_SELF;
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);

    SimTakeTurn(&s, 0);

    assert(s.units[0].health == 9990);
    assert(s.units[0].power == 180);
}

static void TestCombinedAilments(void) {
    BattleSim s = {.rng = 13};
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(&actor, &actor_ability, "actor", "test", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);
    AbilityRecipe recipe = {
        .key = "test",
        .hit = {.stun_pct = 100, .curse_power_pct = 100, .curse_health_pct = 100},
    };

    SimAttack(&s, 0, &recipe, &recipe.hit, true, true);

    const SimUnit *target = &s.units[SQUAD_SLOT_COUNT];
    assert(target->stunned && target->curse_power && target->curse_health);
    assert(s.action_count == 1);
    assert(
        (s.actions[0].ailments & (AIL_STUNNED | AIL_CURSE_POWER | AIL_CURSE_HEALTH)) ==
        (AIL_STUNNED | AIL_CURSE_POWER | AIL_CURSE_HEALTH)
    );
}

static void TestMarkLandsOnStruckTargets(void) {
    BattleSim s = {.rng = 23};
    Character actor, enemies[SQUAD_SLOT_COUNT];
    Ability actor_ability, enemy_abilities[SQUAD_SLOT_COUNT];
    TestCharacter(&actor, &actor_ability, "actor", "test_mark_row", TARGET_ROW, 200, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        TestCharacter(&enemies[slot], &enemy_abilities[slot], "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
        if (slot == SQUAD_SLOT_LARGE) enemies[slot].giant = true;
        SimUnitInit(&s, 1, slot, &enemies[slot]);
    }
    AbilityRecipe recipe = {.key = "test_mark_row", .hit = {.mark_pct = 20}};

    SimAttack(&s, 0, &recipe, &recipe.hit, true, true);

    bool struck[SQUAD_SLOT_COUNT] = {false};
    int struck_count = 0;
    for (int i = 0; i < s.action_count; i++)
        for (int t = 0; t < s.actions[i].target_count; t++)
            if (s.actions[i].targets[t].unit >= SQUAD_SLOT_COUNT)
                struck[s.actions[i].targets[t].unit - SQUAD_SLOT_COUNT] = true;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        if (struck[slot]) struck_count++;
        assert(s.units[SQUAD_SLOT_COUNT + slot].marked == (struck[slot] ? 20 : 0));
    }
    assert(struck_count == 2);
    assert(s.units[SQUAD_SLOT_COUNT + SQUAD_SLOT_LARGE].marked == 0);

    BattleSim plain_sim = {.rng = 23};
    SimUnitInit(&plain_sim, 0, 0, &actor);
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) SimUnitInit(&plain_sim, 1, slot, &enemies[slot]);
    AbilityRecipe plain = {.key = "test_mark_row"};
    SimAttack(&plain_sim, 0, &plain, &plain.hit, true, true);
    assert(plain_sim.rng == s.rng);
    assert(plain_sim.action_count == s.action_count);
    for (int i = 0; i < s.action_count; i++) {
        assert(plain_sim.actions[i].target_count == s.actions[i].target_count);
        for (int t = 0; t < s.actions[i].target_count; t++)
            assert(plain_sim.actions[i].targets[t].health_delta == s.actions[i].targets[t].health_delta);
    }

    BattleSim f = {.rng = 29};
    Character follow_actor, follow_enemies[4];
    Ability follow_actor_ability, follow_enemy_abilities[4];
    TestCharacter(
        &follow_actor,
        &follow_actor_ability,
        "actor",
        "damage_all_drainpower_25_and_slow_50_followup_single_followup_mark",
        TARGET_ALL,
        200,
        10000
    );
    SimUnitInit(&f, 0, 0, &follow_actor);
    for (int slot = 0; slot < 4; slot++) {
        TestCharacter(
            &follow_enemies[slot], &follow_enemy_abilities[slot], "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000
        );
        SimUnitInit(&f, 1, slot, &follow_enemies[slot]);
    }
    const AbilityRecipe *r = SimRecipe(&follow_actor);
    AttackRecipe followup = SimFollowupAttack(r, 0);
    SimAttack(&f, 0, r, &followup, true, false);
    assert(f.action_count == 1 && f.actions[0].target_count == 1);
    int struck_unit = f.actions[0].targets[0].unit;
    for (int slot = 0; slot < 4; slot++) {
        int idx = SQUAD_SLOT_COUNT + slot;
        assert(f.units[idx].marked == (idx == struck_unit ? 30 : 0));
    }
}

static void TestSplitAilmentSwing(void) {
    BattleSim s = {.rng = 2};
    Character actor, enemies[4];
    Ability actor_ability, enemy_abilities[4];
    TestCharacter(&actor, &actor_ability, "actor", "test", TARGET_ALL, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    for (int i = 0; i < 4; i++) {
        TestCharacter(&enemies[i], &enemy_abilities[i], "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
        SimUnitInit(&s, 1, i, &enemies[i]);
    }
    AbilityRecipe recipe = {
        .key = "test",
        .hit = {.stun_pct = 50, .target = RECIPE_TARGET_ALL, .status_only = true},
    };

    SimAttack(&s, 0, &recipe, &recipe.hit, true, false);

    assert(s.action_count == 2);
    assert(!s.actions[0].same_swing && s.actions[1].same_swing);
    bool recorded[SIM_UNIT_COUNT] = {false};
    for (int i = 0; i < s.action_count; i++) {
        const BattleSimAction *a = &s.actions[i];
        assert(a->actor == 0 && !a->replay_attack);
        for (int t = 0; t < a->target_count; t++) {
            int unit = a->targets[t].unit;
            assert(!recorded[unit]);
            recorded[unit] = true;
            assert(s.units[unit].stunned == !!(a->ailments & AIL_STUNNED));
        }
    }

    int targets[SIM_MAX_TARGETS];
    int target_count = BpCollectSwingTargets(&s, 0, targets);
    assert(target_count == 4);
    bool collected[SIM_UNIT_COUNT] = {false};
    for (int i = 0; i < target_count; i++) {
        assert(!collected[targets[i]]);
        collected[targets[i]] = true;
    }
    for (int i = 0; i < 4; i++) {
        int unit = SQUAD_SLOT_COUNT + i;
        assert(recorded[unit] && collected[unit]);
    }

    for (int i = 0; i < s.action_count; i++) {
        s.actions[i].crit = true;
        for (int t = 0; t < s.actions[i].target_count; t++) {
            s.actions[i].targets[t].health_delta = -10;
            s.actions[i].targets[t].health_faction_delta = -2;
        }
    }
    BattlePlay bp = {.sim = s};
    BattleSummary summary;
    BattlePlaySummary(&bp, &summary);
    assert(summary.dmg[0] == 48);
    assert(summary.crits[0] == 1);
    assert(summary.fac_hits[0] == 1);
    assert(BattlePlayCriticalGold(&bp) == BP_CRIT_GOLD_REWARD);
}

static void TestCurseLastsThroughTurn(void) {
    BattleSim s = {.rng = 17};
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(&actor, &actor_ability, "actor", "ABILITY_DAMAGE_ONE_INCREASE_POWER", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);
    s.units[0].curse_power = true;

    SimTakeTurn(&s, 0);

    assert(s.units[0].power == 100);
    assert(!s.units[0].curse_power);
    assert(s.actions[s.action_count - 1].ailments & AIL_UNCURSE_POWER);
}

static void TestNecromancerDorianCurse(void) {
    BattleSim s = {.rng = 23};
    Character dorian, enemy_a, enemy_b;
    Ability dorian_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(
        &dorian, &dorian_ability, "UNIT_NECROMANCERDORIAN", "damage_all_cursehealth_chance_stun", TARGET_ALL, 100, 10000
    );
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &dorian);
    SimUnitInit(&s, 1, 0, &enemy_a);
    SimUnitInit(&s, 1, 1, &enemy_b);

    const AbilityRecipe *recipe = SimRecipe(&dorian);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);

    assert(s.units[SQUAD_SLOT_COUNT].curse_health);
    assert(s.units[SQUAD_SLOT_COUNT + 1].curse_health);
    assert(s.actions[0].ailments & AIL_CURSE_HEALTH);
}

static void TestCurseFx(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));

    const FxPreset *power = BattleFxFindPreset(&b, "fx_status_curse_power_pre");
    const FxPreset *health = BattleFxFindPreset(&b, "fx_status_curse_health_pre");
    assert(power && health);

    assert(strcmp(health->em[0].tex, CACHE_BUNDLE_RESOURCE("fx_status_burning_pre", "fire_u_02")) == 0);
    assert(health->em[0].cols == 0 && health->em[0].rows == 0);
    assert(strcmp(health->em[1].tex, CACHE_BUNDLE_RESOURCE("fx_status_burning_pre", "fx_flamefb01")) == 0);
    assert(health->em[1].cols == 4 && health->em[1].rows == 4);
    assert(fabsf(health->em[1].sheet_cycles - 2.0f) < 0.001f);
    assert(health->em[0].one_shot && health->em[1].one_shot);
    assert(power->em[0].one_shot && !power->em[1].one_shot);
    assert(health->em[0].box.z > health->em[0].box.x);
    assert(power->em[0].box.x > 0.6f && power->em[0].size_curve.count == 2);

    FxEmitter inert = {.life0 = 2.0f, .life1 = 2.0f};
    b.rings[0][0].pos = (Vector3){1, 2, 3};
    b.fxp[0] = (FxParticle){
        .em = &inert,
        .pre = health,
        .pos = {1.25f, 2.5f, 3.75f},
        .anchor_pos = {1, 2, 3},
        .life = 2.0f,
        .anchor = 0,
    };
    b.fxp_count = 1;
    b.rings[0][0].pos = (Vector3){2, 4, 6};
    BattleFxUpdate(&b, 0.1f);
    assert(fabsf(b.fxp[0].pos.x - 2.25f) < 0.001f);
    assert(fabsf(b.fxp[0].pos.y - 4.5f) < 0.001f);
    assert(fabsf(b.fxp[0].pos.z - 6.75f) < 0.001f);

    b.fxi[0] = (FxInstance){.pre = health, .alive = true, .loop = true, .anchor = 0, .aura_slot = FX_AURA_CURSE_HEALTH};
    b.fxp[0].pre = health;
    b.fxp[0].aura_slot = FX_AURA_CURSE_HEALTH;
    b.fxp[1] = (FxParticle){.pre = power, .anchor = 0, .aura_slot = FX_AURA_CURSE_POWER};
    b.fxp_count = 2;
    BattleFxStatusClear(&b, FX_AURA_CURSE_HEALTH, 0, 0);
    assert(!b.fxi[0].alive);
    assert(b.fxp_count == 1 && b.fxp[0].pre == power);
}

static void TestSlowFx(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));

    const FxPreset *slow = BattleFxFindPreset(&b, "eff_initiativedown_pre");
    assert(slow && slow->em_count == 5);
    assert(fabsf(slow->length - 5.0f) < 0.001f);

    assert(slow->em[0].orient == 3);
    assert(slow->em[1].orient == 1);
    assert(slow->em[4].orient == 1);
    assert(slow->em[2].one_shot && slow->em[3].one_shot);
    assert(slow->em[0].prewarm && slow->em[0].sheet_cycles == 6.0f);
    assert(slow->em[1].max_screen_size == 0.5f && slow->em[1].size0 == 40.0f);
    assert(fabsf(slow->em[1].render_scale - 0.05f) < 0.001f);
    assert(fabsf(slow->em[1].size0 * slow->em[1].render_scale - 2.0f) < 0.001f);
    assert(slow->em[4].size0 == 50.0f && slow->em[4].size1 == 60.0f);
    assert(fabsf(slow->em[4].render_scale - 0.05f) < 0.001f);
    assert(fabsf(slow->em[4].size0 * slow->em[4].render_scale - 2.5f) < 0.001f);
    assert(fabsf(slow->em[4].size1 * slow->em[4].render_scale - 3.0f) < 0.001f);
    assert(slow->em[1].alpha_curve.count == 4 && slow->em[4].alpha_curve.count == 4);
    assert(slow->em[4].spin_curve.count == 2);

    Vector3 seam = FxCylinderRingPoint(1.0f, 0.0f, 0);
    Vector3 next = FxCylinderRingPoint(1.0f, 0.0f, 1);
    assert(fabsf(seam.x - 0.309017f) < 0.00001f && fabsf(seam.z - 0.951057f) < 0.00001f);
    assert(fabsf(next.x - 0.587785f) < 0.00001f && fabsf(next.z - 0.809017f) < 0.00001f);
    assert(fabsf(FX_CYLINDER_U[0] - 0.003064275f) < 0.0000001f);
    assert(fabsf(FX_CYLINDER_U[1] - 0.065215617f) < 0.0000001f);

    FxPreset warm = {
        .name = "test_slow_prewarm",
        .em_count = 1,
        .em = {
            {.prewarm = 1,
             .max_particles = 16,
             .rate = 4.0f,
             .duration = 0.6f,
             .period = 0.6f,
             .life0 = 1.0f,
             .life1 = 1.0f,
             .dir = {0, 1, 0}}
        },
    };
    static Battle warmed;
    memset(&warmed, 0, sizeof(warmed));
    warmed.fxi[0] = (FxInstance){.pre = &warm, .alive = true, .loop = true, .anchor = -1, .serial = 1};
    warmed.fxi[0].accum[0] = 1.0f;
    BattleFxUpdate(&warmed, 0.0f);
    assert(warmed.fxi[0].prewarm_done[0]);
    assert(warmed.fxp_count == 2);
    assert(warmed.fxp[0].age <= warm.em[0].duration && warmed.fxp[1].age <= warm.em[0].duration);
    assert(fabsf(warmed.fxi[0].accum[0] - 0.4f) < 0.001f);
    BattleFxUpdate(&warmed, 0.14f);
    assert(warmed.fxp_count == 2);
    BattleFxUpdate(&warmed, 0.02f);
    assert(warmed.fxp_count == 3);
}

static void TestStatusPrefabCabParameters(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));
    const FxPreset *up = BattleFxFindPreset(&b, "vfx_powerup_pre");
    const FxPreset *down = BattleFxFindPreset(&b, "vfx_powerdown_pre");
    const FxPreset *stun = BattleFxFindPreset(&b, "vfx_stunned_pre");
    const FxPreset *heal = BattleFxFindPreset(&b, "fx_heal_pre");
    assert(up && down && stun && heal);

    assert(up->em_count == 2);
    assert(up->em[0].one_shot && up->em[1].one_shot);
    assert(fabsf(up->em[0].offset.y - 0.5904f) < 0.0001f);
    assert(fabsf(up->em[1].offset.y - 0.8225f) < 0.0001f);
    assert(fabsf(FxCurveSample(&up->em[0].size_curve, 0.4158f, 0) - 1.8464f) < 0.0001f);
    assert(up->em[1].color_curve.count == 2);

    assert(down->em_count == 3);
    assert(fabsf(down->em[0].offset.y - 1.0833f) < 0.0001f);
    assert(strcmp(down->em[2].tex, CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "vfx_lightning_2")) == 0);
    assert(down->em[2].cols == 2 && down->em[2].rows == 2 && !down->em[2].sheet_once);
    assert(down->em[0].alpha_curve.count == 2 && down->em[0].color_curve.count == 2);

    assert(stun->em_count == 2 && stun->em[0].prewarm && stun->em[1].prewarm);
    assert(stun->em[0].rate == 12 && stun->em[1].rate == 20);
    assert(fabsf(stun->em[0].offset.y - 2.0642f) < 0.0001f);
    assert(fabsf(stun->em[1].offset.y - 2.1716f) < 0.0001f);
    assert(stun->em[0].spin_bias_deg == -300.0f && stun->em[0].spin_deg == 0);
    assert(stun->em[0].alpha_curve.count == 4 && stun->em[0].color_curve.count == 3);
    assert(stun->em[1].alpha_curve.count == 4 && stun->em[1].color_curve.count == 2);

    assert(heal->em_count == 3);
    assert(strcmp(heal->em[1].tex, CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "chargespark")) == 0);
    assert(heal->em[0].additive && heal->em[0].orient == 1);
    assert(heal->em[1].max_particles == 60 && heal->em[2].orient == 1);
    assert(heal->em[0].alpha_curve.count == 2 && heal->em[0].alpha_curve.key[1][1] == 1);
    assert(heal->em[1].alpha_curve.count == 2 && heal->em[1].alpha_curve.key[1][1] == 1);
}

static void TestCriticalFxAndReward(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));

    const FxPreset *crit = BattleFxFindPreset(&b, "fx_crit_pre");
    assert(crit && crit->em_count == 6);
    const FxEmitter *arc = &crit->em[0];
    assert(strcmp(arc->tex, CACHE_BUNDLE_RESOURCE("fx_crit_pre", "ARC_Strip003")) == 0);
    assert(arc->additive && arc->orient == 2 && arc->cols == 1 && arc->rows == 4);
    assert(arc->max_particles == 6 && fabsf(arc->rate - 30.0f) < 0.001f);
    assert(arc->birth_emitter[0] == 5 && arc->birth_emitter[1] == 6);
    assert(arc->dir.y == -1.0f && arc->size0 == 1.5f && arc->size1 == 2.0f);
    assert(fabsf(arc->stretch_length - 3.0f) < 0.001f);
    assert(fabsf(arc->offset.y - 0.894454f) < 0.000001f);
    assert(arc->alpha_curve.count == 8 && arc->color_curve.count == 2);

    const FxEmitter *glow = &crit->em[1];
    assert(glow->max_particles == 3 && glow->count == 1);
    assert(fabsf(glow->rate - 5.0f) < 0.001f && glow->rate_curve.count == 3);
    assert(fabsf(glow->life0 - 0.4f) < 0.001f && glow->size0 == 6.0f && glow->size1 == 10.0f);
    assert(glow->size_curve.count == 5 && glow->alpha_curve.count == 2 && glow->color_curve.count == 3);

    const FxEmitter *sparks = &crit->em[2];
    assert(sparks->orient == 2 && sparks->max_particles == 100 && sparks->count == 100);
    assert(fabsf(sparks->delay - 0.04f) < 0.001f && sparks->life0 == 0.3f && sparks->life1 == 0.8f);
    assert(sparks->speed0 == 10.0f && sparks->speed1 == 30.0f && sparks->gravity > 12.7f);
    assert(sparks->stretch_length == 0.0f && fabsf(sparks->stretch_velocity - 0.05f) < 0.001f);
    assert(fabsf(sparks->offset.y - 0.934932f) < 0.000001f);
    assert(sparks->alpha_curve.count == 4 && sparks->color_curve.count == 3);

    const FxEmitter *smoke = &crit->em[3];
    assert(smoke->max_particles == 8 && smoke->count == 8 && smoke->cols == 2 && smoke->rows == 2);
    assert(smoke->life0 == 0.7f && smoke->life1 == 1.2f && smoke->speed0 == 5.0f && smoke->speed1 == 10.0f);
    assert(fabsf(smoke->gravity + 2.943f) < 0.001f && smoke->size_curve.count == 5);
    assert(smoke->tint.r == 152 && smoke->tint.g == 182 && smoke->tint.b == 255 && smoke->tint.a == 209);

    const FxEmitter *arc_glow = &crit->em[4];
    assert(arc_glow->orient == 1 && arc_glow->max_particles == 4 && arc_glow->count == 0);
    assert(arc_glow->life0 == 0.12f && arc_glow->size0 == 4.0f && arc_glow->size1 == 5.0f);
    assert(arc_glow->color_curve.count == 5 && arc_glow->spin_bias_deg == 50.0f && arc_glow->spin_deg == 50.0f);

    const FxEmitter *radial = &crit->em[5];
    assert(radial->max_particles == 4 && radial->count == 0 && radial->life0 == 0.12f);
    assert(radial->size0 == 6.0f && radial->size1 == 8.0f && radial->size_curve.count == 9);
    assert(radial->color_curve.count == 5 && radial->rot_bias_deg == 180.0f && radial->rot_deg == 180.0f);

    FxPreset nested = {
        .name = "test_birth_subemitters",
        .em_count = 3,
        .em = {
            {.birth_emitter = {2, 3}, .life0 = 1, .life1 = 1, .dir = {0, 1, 0}, .offset = {0.2f, 0.3f, 0.4f}},
            {.max_particles = 4, .life0 = 0.12f, .life1 = 0.12f, .dir = {0, 1, 0}, .offset = {7, 8, 9}},
            {.max_particles = 4, .life0 = 0.12f, .life1 = 0.12f, .dir = {0, 1, 0}, .offset = {-7, -8, -9}},
        },
    };
    static Battle nested_battle;
    memset(&nested_battle, 0, sizeof(nested_battle));
    FxInstance nested_instance = {
        .pre = &nested, .anchor = -1, .serial = 77, .pos = {1, 2, 3}, .offset = {0.1f, 0.2f, 0.3f}
    };
    FxParticle *parent = FxEmitOne(&nested_battle, &nested_instance, &nested.em[0]);
    assert(parent && nested_battle.fxp_count == 3);
    assert(nested_battle.fxp[1].em == &nested.em[1] && nested_battle.fxp[2].em == &nested.em[2]);
    assert(Vector3Distance(nested_battle.fxp[1].pos, parent->pos) < 0.000001f);
    assert(Vector3Distance(nested_battle.fxp[2].pos, parent->pos) < 0.000001f);
    assert(FxEmitOne(&nested_battle, &nested_instance, &nested.em[0]));
    assert(nested_battle.fxp_count == 6);

    BattleSfx silent[2] = {0};
    StrCopy(silent[0].name, sizeof(silent[0].name), "vfx_critical_hit");
    StrCopy(silent[1].name, sizeof(silent[1].name), "gui_add_gold1");
    b.sfx = silent;
    b.sfx_count = b.sfx_capacity = 2;

    BattlePlay bp = {0};
    bp.units[1][0] = (BpUnit){.health = 100, .max_health = 100, .pos = {2.0f, 0.45f, 3.0f}};
    bp.units[1][1] = (BpUnit){.health = 100, .max_health = 100, .pos = {4.0f, 0.45f, 5.0f}};
    BattleSimAction action = {
        .actor = 0,
        .crit = true,
        .replay_attack = true,
        .target_count = 2,
        .targets = {
            {.unit = SQUAD_SLOT_COUNT, .health_delta = -10}, {.unit = SQUAD_SLOT_COUNT + 1, .health_delta = -10}
        },
    };
    BpApplyAction(&bp, &b, &action);

    int strike_count = 0;
    for (int i = 0; i < FX_MAX_INSTANCES; i++) {
        const FxInstance *ins = &b.fxi[i];
        if (!ins->alive || ins->pre != crit) continue;
        assert(ins->anchor == -1);
        Vector3 expected = bp.units[1][strike_count].pos;
        assert(Vector3Distance(ins->pos, expected) < 0.001f);
        strike_count++;
    }
    assert(strike_count == 2);
    assert(bp.message_count == 2);
    assert(bp.messages[0].order_key == BP_MSG_DAMAGE);
    assert(bp.messages[1].order_key == BP_MSG_CRIT_REWARD);
    assert(bp.messages[1].icon == GUI_MSG_CRIT_REWARD);
    assert(bp.messages[1].gold_reward == BP_CRIT_GOLD_REWARD);

    BpRevealMessageAt(&bp, &b, 1);
    BpRevealMessageAt(&bp, &b, 1);
    assert(bp.total_bonus_gold == BP_CRIT_GOLD_REWARD);
    assert(bp.messages[1].reward_applied);

    BattlePlay enemy_bp = {0};
    enemy_bp.units[0][0] = (BpUnit){.health = 100, .max_health = 100, .pos = {-2, 0.45f, -1}};
    BattleSimAction enemy_action = {
        .actor = SQUAD_SLOT_COUNT,
        .crit = true,
        .replay_attack = true,
        .target_count = 1,
        .targets = {{.unit = 0, .health_delta = -10}},
    };
    BpApplyAction(&enemy_bp, &b, &enemy_action);
    assert(enemy_bp.message_count == 1 && enemy_bp.messages[0].gold_reward == 0);
}

static void TestBattleMessageDictionary(void) {
    BattlePlay bp = {0};
    Color white = WHITE;
    assert(BpQueueMessage(&bp, BP_MSG_CURSE_HEALTH, 2, -1, "health curse", NULL, white, 1.0f));
    assert(BpQueueMessage(&bp, BP_MSG_DAMAGE, 2, -1, "first damage", NULL, white, 1.0f));
    assert(BpQueueMessage(&bp, BP_MSG_POWER_DOWN, 2, -1, "power down", NULL, white, 1.0f));
    assert(BpQueueMessage(&bp, BP_MSG_DAMAGE, 1, -1, "last damage", NULL, white, 1.7f));

    assert(bp.message_count == 3);
    assert(bp.messages[0].order_key == BP_MSG_DAMAGE);
    assert(bp.messages[1].order_key == BP_MSG_POWER_DOWN);
    assert(bp.messages[2].order_key == BP_MSG_CURSE_HEALTH);
    assert(strcmp(bp.messages[0].text, "last damage") == 0);
    assert(bp.messages[0].slot == 1 && fabsf(bp.messages[0].big - 1.7f) < 0.001f);

    for (int key = 20; bp.message_count < BP_MAX_MESSAGES; key++)
        assert(BpQueueMessage(&bp, key, 0, -1, "filler", NULL, white, 1.0f));
    assert(BpQueueMessage(&bp, BP_MSG_DAMAGE, 0, -1, "replacement", NULL, white, 1.0f));
    assert(strcmp(bp.messages[0].text, "replacement") == 0);
    assert(!BpQueueMessage(&bp, 1000, 0, -1, "overflow", NULL, white, 1.0f));
}

static void TestBattleRewardMessageTiming(void) {
    Rectangle actor = BattleUiCombatMessageRect(1280.0f, 0, 0.0f);
    Rectangle actor_raised = BattleUiCombatMessageRect(1280.0f, 0, 72.0f);
    assert(fabsf(actor.x - 424.0f) < 0.001f && fabsf(actor.y - 270.0f) < 0.001f);
    assert(fabsf(actor_raised.y - 198.0f) < 0.001f);

    Vector2 start = BattleUiRewardCoinStart(1280.0f);
    Vector2 wide_start = BattleUiRewardCoinStart(1600.0f);
    assert(fabsf(start.x - 642.0f) < 0.001f && fabsf(start.y - 258.75f) < 0.001f);
    assert(fabsf(wide_start.x - 762.0f) < 0.001f && fabsf(wide_start.y - 258.75f) < 0.001f);

    assert(fabsf(BP_REWARD_ANIM_TIME - 0.9f) < 0.001f);
    assert(BattleUiRewardCoinAlpha(0.0f) == 0.0f);
    assert(fabsf(BattleUiRewardCoinAlpha(0.075f) - 0.5f) < 0.001f);
    assert(BattleUiRewardCoinAlpha(0.15f) == 1.0f);
    assert(BattleUiRewardCoinAlpha(0.45f) == 1.0f);
    assert(fabsf(BattleUiRewardCoinAlpha(0.475f) - 0.5f) < 0.001f);
    assert(BattleUiRewardCoinAlpha(0.5f) == 0.0f);

    BattleSfx silent = {0};
    StrCopy(silent.name, sizeof(silent.name), "gui_add_gold1");
    Battle b = {.sfx = &silent, .sfx_count = 1, .sfx_capacity = 1};
    BattlePlay bp = {.message_count = 2};
    bp.messages[0].order_key = BP_MSG_DAMAGE;
    bp.messages[1] = (BpMessage){.order_key = BP_MSG_CRIT_REWARD, .gold_reward = BP_CRIT_GOLD_REWARD};
    BpRevealMessagesBetween(&bp, &b, 0.25f, BP_MESSAGE_TIME * 2.5f);
    assert(bp.total_bonus_gold == BP_CRIT_GOLD_REWARD);
    assert(bp.messages[1].reward_applied && bp.reward_anim_active);
    BpRevealMessagesBetween(&bp, &b, 0.0f, BP_MESSAGE_TIME * 3.0f);
    assert(bp.total_bonus_gold == BP_CRIT_GOLD_REWARD);

    BattlePlay timing = {
        .state = BP_INTRO,
        .slowmo = 0.5f,
        .speed_mult = 1.0f,
        .reward_anim_active = true,
    };
    BattlePlayUpdate(&timing, &b, 0.2f);
    assert(fabsf(timing.reward_anim_t - 0.1f) < 0.001f);
}

static void TestBattleCameraTransitions(void) {
    Battle b = {0};
    b.cam_matchmaking = (Camera3D){
        .position = {8, 7, 6}, .target = {8, 7, -4}, .up = {0, 1, 0}, .fovy = 37, .projection = CAMERA_PERSPECTIVE
    };
    b.cam_tactical = (Camera3D){
        .position = {0, 11, 7}, .target = {0, 1, 7}, .up = {0, 0, -1}, .fovy = 40, .projection = CAMERA_PERSPECTIVE
    };
    b.cam_base = (Camera3D){
        .position = {0, 7, 4},
        .target = {0, 1, -10},
        .up = {0, 0.919145f, -0.393919f},
        .fovy = 40,
        .projection = CAMERA_PERSPECTIVE
    };
    b.cam = b.cam_matchmaking;

    BattleSetTactical(&b, true);
    BattleCameraUpdatePose(&b, 0.25f);
    assert(fabsf(b.tac_t - 0.5f) < 0.001f);
    assert(
        Vector3Distance(b.cam.position, Vector3Lerp(b.cam_matchmaking.position, b.cam_tactical.position, 0.5f)) < 0.001f
    );
    assert(fabsf(b.cam.fovy - 38.5f) < 0.001f);

    Camera3D transition_from = b.cam;
    BattleBeginCombatCamera(&b);
    assert(b.combat_camera && !b.tactical && b.combat_cam_t == 0);
    BattleCameraUpdatePose(&b, 0.25f);
    assert(Vector3Distance(b.cam.position, Vector3Lerp(transition_from.position, b.cam_base.position, 0.5f)) < 0.001f);
    BattleCameraUpdatePose(&b, 0.25f);
    assert(Vector3Distance(b.cam.position, b.cam_base.position) < 0.001f);
    assert(Vector3Distance(b.cam.target, b.cam_base.target) < 0.001f);
    assert(Vector3Distance(b.cam.up, b.cam_base.up) < 0.001f);
    assert(fabsf(b.cam.fovy - 40.0f) < 0.001f);

    BattleSetTactical(&b, true);
    assert(!b.tactical);
}

static FxInstance *TestStatusInstance(Battle *b, int anchor, FxAuraSlot aura_slot) {
    for (int i = 0; i < FX_MAX_INSTANCES; i++) {
        FxInstance *ins = &b->fxi[i];
        if (ins->alive && ins->loop && ins->anchor == anchor && ins->aura_slot == aura_slot) return ins;
    }
    return NULL;
}

static void TestStatusDeltaFxRules(void) {
    BpUnit power = {.health = 100, .power = 100, .start_power = 100};
    assert(BpApplyPowerDelta(&power, 20) == BP_STAT_FX_POWER_UP && power.power == 120);
    assert(BpApplyPowerDelta(&power, -10) == BP_STAT_FX_NONE && power.power == 110);
    assert(BpApplyPowerDelta(&power, -10) == BP_STAT_FX_NONE && power.power == 100);
    assert(BpApplyPowerDelta(&power, -1) == BP_STAT_FX_POWER_DOWN && power.power == 99);
    assert(BpApplyPowerDelta(&power, 1) == BP_STAT_FX_NONE && power.power == 100);
    assert(BpApplyPowerDelta(&power, 1) == BP_STAT_FX_POWER_UP && power.power == 101);

    BpUnit initiative = {.health = 100};
    assert(BpApplyInitiativeDelta(&initiative, -20) == BP_STAT_FX_INIT_DOWN);
    assert(BpApplyInitiativeDelta(&initiative, 10) == BP_STAT_FX_NONE);
    assert(BpApplyInitiativeDelta(&initiative, -10) == BP_STAT_FX_INIT_DOWN);
    assert(BpApplyInitiativeDelta(&initiative, 20) == BP_STAT_FX_NONE);
    assert(initiative.current_initiative == 0);
    assert(BpApplyInitiativeDelta(&initiative, 1) == BP_STAT_FX_INIT_UP);

    power.health = 0;
    assert(BpApplyPowerDelta(&power, -200) == BP_STAT_FX_NONE);
    assert(power.power == -99);
    initiative.health = 0;
    assert(BpApplyInitiativeDelta(&initiative, -10) == BP_STAT_FX_NONE);
}

static void TestStatusAuraSlots(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));
    b.rings[0][0].pos = (Vector3){1, 2, 3};

    BattleFxStatusSet(&b, FX_AURA_POWER, "vfx_powerup_pre", 0, 0, (Vector3){0});
    BattleFxStatusSet(&b, FX_AURA_INITIATIVE, "vfx_powerup_pre", 0, 0, (Vector3){0});
    FxInstance *power = TestStatusInstance(&b, 0, FX_AURA_POWER);
    FxInstance *initiative = TestStatusInstance(&b, 0, FX_AURA_INITIATIVE);
    assert(power && initiative && power != initiative && power->pre == initiative->pre);
    unsigned initiative_serial = initiative->serial;

    b.fxp[0] = (FxParticle){.pre = power->pre, .anchor = 0, .aura_slot = FX_AURA_POWER, .serial = power->serial};
    b.fxp[1] = (FxParticle){
        .pre = initiative->pre, .anchor = 0, .aura_slot = FX_AURA_INITIATIVE, .serial = initiative->serial
    };
    b.fxp_count = 2;
    BattleFxStatusSet(&b, FX_AURA_POWER, "vfx_powerdown_pre", 0, 0, (Vector3){0});
    power = TestStatusInstance(&b, 0, FX_AURA_POWER);
    initiative = TestStatusInstance(&b, 0, FX_AURA_INITIATIVE);
    assert(power && strcmp(power->pre->name, "vfx_powerdown_pre") == 0);
    assert(initiative && initiative->serial == initiative_serial);
    assert(b.fxp_count == 1 && b.fxp[0].aura_slot == FX_AURA_INITIATIVE);

    BattleFxStatusClear(&b, FX_AURA_INITIATIVE, 0, 0);
    assert(!TestStatusInstance(&b, 0, FX_AURA_INITIATIVE));
    assert(TestStatusInstance(&b, 0, FX_AURA_POWER));
    assert(b.fxp_count == 0);
}

static void TestStatusOffsetsAndShake(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));
    BattleSfx silent[4] = {0};
    static const char *const names[] = {"gui_fx_power_down", "gui_fx_init_down", "gui_fx_stun", "gui_fx_power_up"};
    for (int i = 0; i < 4; i++) StrCopy(silent[i].name, sizeof(silent[i].name), names[i]);
    b.sfx = silent;
    b.sfx_count = b.sfx_capacity = 4;

    BattlePlay bp = {0};
    Character normal = {0};
    bp.sim.units[0] = (SimUnit){.present = true, .alive = true, .ch = &normal};
    bp.units[0][0] = (BpUnit){.health = 100, .max_health = 100, .power = 100, .start_power = 100};
    BattleSimAction action = {
        .actor = 1,
        .ailments = AIL_STUNNED | AIL_CURSE_POWER | AIL_CURSE_HEALTH,
        .target_count = 1,
        .targets = {{.unit = 0, .power_delta = -1, .initiative_delta = -1}},
    };
    BpApplyAction(&bp, &b, &action);

    static const FxAuraSlot slots[] = {
        FX_AURA_POWER,
        FX_AURA_INITIATIVE,
        FX_AURA_STUN,
        FX_AURA_CURSE_POWER,
        FX_AURA_CURSE_HEALTH,
    };
    for (int i = 0; i < 5; i++) {
        FxInstance *ins = TestStatusInstance(&b, 0, slots[i]);
        assert(ins);
        assert(ins->offset.x == 0 && ins->offset.y == 0 && ins->offset.z == 0);
    }
    assert(fabsf(bp.units[0][0].damage_shake_left - BP_DAMAGE_SHAKE_TIME) < 0.001f);

    Character giant = {.giant = true};
    bp.sim.units[0].ch = &giant;
    bp.units[0][0].damage_shake_left = 0;
    BattleSimAction stun = {.actor = 1, .ailments = AIL_STUNNED, .target_count = 1, .targets = {{.unit = 0}}};
    BpApplyAction(&bp, &b, &stun);
    FxInstance *giant_stun = TestStatusInstance(&b, 0, FX_AURA_STUN);
    assert(giant_stun && fabsf(giant_stun->offset.y - 1.0f) < 0.001f);
    assert(fabsf(bp.units[0][0].damage_shake_left - BP_DAMAGE_SHAKE_TIME) < 0.001f);

    bp.units[0][0] = (BpUnit){.health = 100, .max_health = 100, .power = 150, .start_power = 100};
    BattleFxStatusSet(&b, FX_AURA_POWER, "vfx_powerup_pre", 0, 0, (Vector3){0});
    unsigned serial = TestStatusInstance(&b, 0, FX_AURA_POWER)->serial;
    BattleSimAction partial = {.actor = 1, .target_count = 1, .targets = {{.unit = 0, .power_delta = -10}}};
    BpApplyAction(&bp, &b, &partial);
    assert(bp.units[0][0].power == 140 && bp.units[0][0].damage_shake_left == 0);
    assert(TestStatusInstance(&b, 0, FX_AURA_POWER)->serial == serial);
}

static void TestInitiativeAndStatusCleanup(void) {
    static Battle b;
    memset(&b, 0, sizeof(b));
    BattlePlay bp = {0};
    bp.units[0][0].current_initiative = 25;
    BattleFxStatusSet(&b, FX_AURA_POWER, "vfx_powerup_pre", 0, 0, (Vector3){0});
    BattleFxStatusSet(&b, FX_AURA_INITIATIVE, "vfx_powerup_pre", 0, 0, (Vector3){0});
    BpResetInitiative(&bp, &b, 0);
    assert(bp.units[0][0].current_initiative == 0);
    assert(!TestStatusInstance(&b, 0, FX_AURA_INITIATIVE));
    assert(TestStatusInstance(&b, 0, FX_AURA_POWER));

    BattleFxStatusSet(&b, FX_AURA_INITIATIVE, "eff_initiativedown_pre", 0, 0, (Vector3){0});
    BattleFxStatusSet(&b, FX_AURA_STUN, "vfx_stunned_pre", 0, 0, (Vector3){0});
    BattleFxStatusSet(&b, FX_AURA_CURSE_POWER, "fx_status_curse_power_pre", 0, 0, (Vector3){0});
    BattleFxStatusSet(&b, FX_AURA_CURSE_HEALTH, "fx_status_curse_health_pre", 0, 0, (Vector3){0});
    const FxPreset *world = BattleFxFindPreset(&b, "fx_heal_pre");
    b.fxi[FX_MAX_INSTANCES - 1] =
        (FxInstance){.pre = world, .alive = true, .anchor = -1, .serial = ++b.fx_serial, .pos = {9, 9, 9}};
    BpCleanupStatusFx(&b);
    for (int slot = FX_AURA_POWER; slot <= FX_AURA_CURSE_HEALTH; slot++)
        assert(!TestStatusInstance(&b, 0, (FxAuraSlot)slot));
    assert(b.fxi[FX_MAX_INSTANCES - 1].alive && !b.fxi[FX_MAX_INSTANCES - 1].loop);
}

static void TestInitiativeBannerFollowsModifier(void) {
    static BattlePlay bp;
    memset(&bp, 0, sizeof(bp));
    bp.sim.units[0].init_class = SIM_INIT_CLASS_MEDIUM;
    assert(BpInitiativeClass(&bp, 0) == SIM_INIT_CLASS_MEDIUM);
    bp.units[0][0].current_initiative = -50;
    assert(BpInitiativeClass(&bp, 0) == SIM_INIT_CLASS_SLOW);
    bp.units[0][0].current_initiative = 50;
    assert(BpInitiativeClass(&bp, 0) == SIM_INIT_CLASS_FAST);
    bp.sim.units[0].init_class = SIM_INIT_CLASS_FAST;
    bp.units[0][0].current_initiative = -40;
    assert(BpInitiativeClass(&bp, 0) == SIM_INIT_CLASS_MEDIUM);
}

static void TestFollowupTarget(void) {
    BattleSim s = {.rng = 19};
    Character actor, enemy_a, enemy_b;
    Ability actor_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(&actor, &actor_ability, "actor", "attack_single_drain_power_and_attack_all", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy_a);
    SimUnitInit(&s, 1, 1, &enemy_b);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);
    int first_count = s.action_count;
    SimAttack(&s, 0, recipe, &recipe->followup, true, false);

    assert(recipe->followup.target == RECIPE_TARGET_ALL);
    assert(s.action_count == first_count + 1);
    assert(s.actions[first_count].target_count == 2);
}

static int RecordedAttackDamage(const BattleSim *s, int actor, int target, int out[], int cap) {
    int count = 0;
    for (int i = 0; i < s->action_count; i++) {
        const BattleSimAction *a = &s->actions[i];
        if (a->actor != actor || !a->replay_attack) continue;
        for (int t = 0; t < a->target_count; t++) {
            const BattleSimTarget *bt = &a->targets[t];
            if (bt->unit != target || bt->health_delta >= 0) continue;
            if (count < cap) out[count] = -bt->health_delta;
            count++;
        }
    }
    return count;
}

static void TestRedJennySeraFinalHit(void) {
    BattleSim s = {.rng = 43};
    Character sera, enemy;
    Ability sera_ability, enemy_ability;
    TestCharacter(&sera, &sera_ability, "UNIT_REDJENNYSERA", "damage_one_5x_150", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &sera);
    SimUnitInit(&s, 1, 0, &enemy);
    s.units[0].crit_pct = 0.0f;

    SimTakeTurn(&s, 0);

    int damage[5] = {0};
    assert(RecordedAttackDamage(&s, 0, SQUAD_SLOT_COUNT, damage, 5) == 5);
    assert(damage[0] == 100 && damage[1] == 100 && damage[2] == 100 && damage[3] == 100);
    assert(damage[4] == 150);
}

static void TestColeProfanePerHitGrowth(void) {
    BattleSim s = {.rng = 47};
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_COLE",
        "damage_one_10x_heal5_crit2_small_chance_neg_ailment",
        TARGET_ONE,
        100,
        10000
    );
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);
    s.units[0].health = 8000;
    s.units[0].crit_pct = 0.0f;

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);
    SimAttack(&s, 0, recipe, &recipe->followup, true, false);

    assert(s.units[0].health == 9000);
    assert(fabsf(s.units[0].crit_pct - 4.0f) < 0.001f);
    int per_hit_packets = 0;
    for (int i = 0; i < s.action_count; i++) {
        const BattleSimAction *a = &s.actions[i];
        for (int t = 0; t < a->target_count; t++) {
            const BattleSimTarget *bt = &a->targets[t];
            if (bt->unit == 0 && bt->health_delta == 500 && fabsf(bt->crit_delta - 2.0f) < 0.001f) per_hit_packets++;
        }
    }
    assert(per_hit_packets == 2);
}

static void TestVivienneIncreasingFollowups(void) {
    BattleSim s = {.rng = 53};
    Character vivienne, enemy;
    Ability vivienne_ability, enemy_ability;
    TestCharacter(
        &vivienne,
        &vivienne_ability,
        "UNIT_KNIGHT-ENCHANTERVIVIENNE",
        "damage_single_heal_all_medium_chance_followup3x_plus_damage",
        TARGET_ONE,
        100,
        10000
    );
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &vivienne);
    SimUnitInit(&s, 1, 0, &enemy);
    s.units[0].crit_pct = 0.0f;

    const AbilityRecipe *recipe = SimRecipe(&vivienne);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);
    for (int i = 0; i < recipe->followup_count; i++) {
        AttackRecipe hit = SimFollowupAttack(recipe, i);
        SimAttack(&s, 0, recipe, &hit, true, false);
    }

    int damage[4] = {0};
    assert(RecordedAttackDamage(&s, 0, SQUAD_SLOT_COUNT, damage, 4) == 4);
    assert(damage[0] == 100 && damage[1] == 150 && damage[2] == 200 && damage[3] == 250);
}

static void TestRiverDanePersistentAllyDamage(void) {
    BattleSim s = {.rng = 59};
    Character river, ally, enemy;
    Ability river_ability, ally_ability, enemy_ability;
    TestCharacter(
        &river,
        &river_ability,
        "UNIT_HEROOFRIVERDANE",
        "attack_one_levy50_stun_crit5all_bnsdmg3all_assassinate15",
        TARGET_ONE,
        100,
        10000
    );
    TestCharacter(&ally, &ally_ability, "ally", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &river);
    SimUnitInit(&s, 0, 1, &ally);
    SimUnitInit(&s, 1, 0, &enemy);
    s.units[0].crit_pct = 0.0f;
    s.units[1].crit_pct = 0.0f;

    AbilityRecipe river_recipe = *SimRecipe(&river);
    river_recipe.hit.assassinate_pct = 0;
    SimAttack(&s, 0, &river_recipe, &river_recipe.hit, true, true);

    assert(fabsf(s.units[1].crit_pct - 5.0f) < 0.001f);
    assert(fabsf(s.units[1].damage_bonus_pct - 3.0f) < 0.001f);
    bool recorded = false;
    for (int i = 0; i < s.action_count; i++) {
        for (int t = 0; t < s.actions[i].target_count; t++) {
            const BattleSimTarget *bt = &s.actions[i].targets[t];
            if (bt->unit == 1 && fabsf(bt->damage_bonus_delta - 3.0f) < 0.001f) recorded = true;
        }
    }
    assert(recorded);

    int enemy_health = s.units[SQUAD_SLOT_COUNT].health;
    const AbilityRecipe *ally_recipe = SimRecipe(&ally);
    SimAttack(&s, 1, ally_recipe, &ally_recipe->hit, true, true);
    assert(enemy_health - s.units[SQUAD_SLOT_COUNT].health == 103);
}

static void TestBonusDamageRecipes(void) {
    Character actor, ally, enemy;
    Ability actor_ability, ally_ability, enemy_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_THEGRANDOAK",
        "attack_single_levy35_heal40all_2bnsdmgall_damage5all",
        TARGET_ONE,
        100,
        10000
    );
    TestCharacter(&ally, &ally_ability, "ally", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    BattleSim s = {.rng = 61};
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 0, 1, &ally);
    SimUnitInit(&s, 1, 0, &enemy);
    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);
    assert(fabsf(s.units[1].damage_bonus_pct - 2.0f) < 0.001f);

    TestCharacter(
        &actor, &actor_ability, "UNIT_CILLIAN", "attack_row_drain10self_stun50all_gain5bnsdmg", TARGET_ROW, 100, 10000
    );
    memset(&s, 0, sizeof(s));
    s.rng = 67;
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);
    recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->hit, true, true);
    assert(s.units[0].power == 90);
    assert(fabsf(s.units[0].damage_bonus_pct - 5.0f) < 0.001f);

    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_ZITHER",
        "attack_all_chance75_slow_drain35_5bnsdmgall_gain5critall",
        TARGET_ALL,
        100,
        10000
    );
    recipe = SimRecipe(&actor);
    assert(recipe->hit.slow_pct == 75 && recipe->hit.drain_pct == 35 && recipe->hit.drain_chance == 75);
    assert(recipe->fx.ally_crit_pct == 5 && recipe->fx.ally_damage_bonus_pct == 5);
}

static void TestMechanicsFallback(void) {
    Character actor;
    Ability ability;
    TestCharacter(&actor, &ability, "fallback", "unknown_hoda_key", TARGET_ALL, 100, 10000);
    ability.mechanics =
        ABILITY_ATTACK | ABILITY_POWER_DRAIN | ABILITY_STUN | ABILITY_CURSE_HEALTH | ABILITY_GAIN_POWER_ALLIES;

    const AbilityRecipe *recipe = SimRecipe(&actor);
    assert(recipe != &SIM_RECIPE_DEFAULT);
    assert(recipe->hit.drain_pct > 0);
    assert(recipe->hit.stun_pct > 0);
    assert(recipe->hit.curse_health_pct > 0);
    assert(recipe->fx.ally_power_pct > 0);
}

static void TestNoAttackFallback(void) {
    BattleSim s = {.rng = 37};
    Character actor, ally, enemy_a, enemy_b;
    Ability actor_ability, ally_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(&actor, &actor_ability, "banner", "unknown_status_banner", TARGET_ALL, 200, 10000);
    TestCharacter(&ally, &ally_ability, "ally", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 300, 10000);
    actor_ability.mechanics =
        ABILITY_ATTACK_NONE | ABILITY_POWER_DRAIN | ABILITY_SLOW | ABILITY_CURSE_HEALTH | ABILITY_GAIN_POWER_ALLIES;
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 0, 1, &ally);
    SimUnitInit(&s, 1, 0, &enemy_a);
    SimUnitInit(&s, 1, 1, &enemy_b);
    int health_a = s.units[SQUAD_SLOT_COUNT].health;
    int health_b = s.units[SQUAD_SLOT_COUNT + 1].health;

    SimTakeTurn(&s, 0);

    assert(s.units[SQUAD_SLOT_COUNT].health == health_a);
    assert(s.units[SQUAD_SLOT_COUNT + 1].health == health_b);
    assert(s.units[SQUAD_SLOT_COUNT].power == 50);
    assert(s.units[SQUAD_SLOT_COUNT + 1].power == 250);
    assert(s.units[SQUAD_SLOT_COUNT].cur_initiative == -50);
    assert(s.units[SQUAD_SLOT_COUNT + 1].cur_initiative == -50);
    assert(s.units[SQUAD_SLOT_COUNT].curse_health && s.units[SQUAD_SLOT_COUNT + 1].curse_health);
    assert(s.units[1].power == 125);
    for (int i = 0; i < s.action_count; i++) {
        assert(!s.actions[i].replay_attack);
        for (int t = 0; t < s.actions[i].target_count; t++) assert(s.actions[i].targets[t].health_delta == 0);
    }
}

static void TestNoAttackCompoundTargets(void) {
    BattleSim s = {.rng = 41};
    Character vaea, enemy_a, enemy_b;
    Ability vaea_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(
        &vaea, &vaea_ability, "UNIT_VAEA", "stun_single_drain_all25_critgain10_allies_no_attack", TARGET_ONE, 200, 10000
    );
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 300, 10000);
    vaea_ability.mechanics = ABILITY_ATTACK_NONE | ABILITY_POWER_DRAIN | ABILITY_STUN;
    SimUnitInit(&s, 0, 0, &vaea);
    SimUnitInit(&s, 1, 0, &enemy_a);
    SimUnitInit(&s, 1, 1, &enemy_b);
    int health_a = s.units[SQUAD_SLOT_COUNT].health;
    int health_b = s.units[SQUAD_SLOT_COUNT + 1].health;

    SimTakeTurn(&s, 0);

    assert(s.units[SQUAD_SLOT_COUNT].health == health_a);
    assert(s.units[SQUAD_SLOT_COUNT + 1].health == health_b);
    assert(s.units[SQUAD_SLOT_COUNT].power == 50);
    assert(s.units[SQUAD_SLOT_COUNT + 1].power == 250);
    assert(s.units[SQUAD_SLOT_COUNT].stunned != s.units[SQUAD_SLOT_COUNT + 1].stunned);
}

static void TestStatusOnlyTargetPacket(void) {
    BattleSim s = {.rng = 29};
    Character actor, enemy_a, enemy_b;
    Ability actor_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "actor",
        "attack_row_slowall_stun50column_attack_ally5_chance10heal80self",
        TARGET_ROW,
        100,
        10000
    );
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy_a);
    SimUnitInit(&s, 1, 1, &enemy_b);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->effect, true, false);

    assert(s.units[SQUAD_SLOT_COUNT].health == 10000);
    assert(s.units[SQUAD_SLOT_COUNT + 1].health == 15000);
    assert(s.units[SQUAD_SLOT_COUNT].cur_initiative == -50);
    assert(s.units[SQUAD_SLOT_COUNT + 1].cur_initiative == -50);
    assert(s.action_count == 1 && !s.actions[0].replay_attack);
}

static void TestRandomAllyAttack(void) {
    BattleSim s = {.rng = 31};
    Character actor, ally_a, ally_b, enemy;
    Ability actor_ability, ally_a_ability, ally_b_ability, enemy_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "actor",
        "attack_row_cursepower_drain50_followup25random50_followup25ally50",
        TARGET_ROW,
        100,
        10000
    );
    TestCharacter(&ally_a, &ally_a_ability, "ally-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&ally_b, &ally_b_ability, "ally-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 0, 1, &ally_a);
    SimUnitInit(&s, 0, 2, &ally_b);
    SimUnitInit(&s, 1, 0, &enemy);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    SimAttack(&s, 0, recipe, &recipe->effect, true, false);

    assert(s.action_count == 1 && s.actions[0].target_count == 1);
    int target = s.actions[0].targets[0].unit;
    assert(target == 1 || target == 2);
    assert(s.actions[0].targets[0].health_delta == -50);
    assert(s.units[SQUAD_SLOT_COUNT].health == 10000);
}

static void TestGeneratedCorypheusRecipe(void) {
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_CORYPHEUS",
        "damage_all_and_drainpower_and_10pctchance_attackagain",
        TARGET_ALL,
        200,
        10000
    );
    actor_ability.mechanics = ABILITY_ATTACK | ABILITY_POWER_DRAIN | ABILITY_MULTIATTACK;
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 300, 10000);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    assert(recipe->generated && recipe->generated_step_count == 2);
    assert(recipe->generated_steps[0].attack.target == RECIPE_TARGET_ALL);
    assert(recipe->generated_steps[0].attack.drain_pct == 50);
    assert(recipe->generated_steps[1].chance_pct == 10);
    assert(recipe->generated_steps[1].attack.drain_pct == 50);

    bool saw_followup = false;
    for (unsigned seed = 1; seed < 1000 && !saw_followup; seed++) {
        BattleSim s = {.rng = seed};
        SimUnitInit(&s, 0, 0, &actor);
        SimUnitInit(&s, 1, 0, &enemy);
        SimTakeTurn(&s, 0);
        if (s.units[SQUAD_SLOT_COUNT].health == 9600) {
            assert(s.units[SQUAD_SLOT_COUNT].power == 100);
            saw_followup = true;
        }
    }
    assert(saw_followup);
}

static void TestGeneratedLoghainRecipes(void) {
    Character actor, enemy_a, enemy_b;
    Ability actor_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_LOGHAINMACTIR",
        "attack_one_levy25_drain75_followup35_column_stun",
        TARGET_ONE,
        200,
        10000
    );
    actor_ability.mechanics = ABILITY_ATTACK | ABILITY_POWER_DRAIN | ABILITY_STUN | ABILITY_MULTIATTACK | ABILITY_LEVY;
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 200, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 300, 10000);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    assert(recipe->generated && recipe->generated_step_count == 2);
    assert(recipe->generated_steps[0].attack.target == RECIPE_TARGET_ONE);
    assert(recipe->generated_steps[0].attack.levy_pct == 25);
    assert(recipe->generated_steps[0].attack.drain_pct == 75);
    assert(recipe->generated_steps[1].chance_pct == 35);
    assert(recipe->generated_steps[1].attack.target == RECIPE_TARGET_COLUMN);
    assert(recipe->generated_steps[1].attack.stun_pct == 100);

    bool saw_column_stun = false;
    for (unsigned seed = 1; seed < 1000 && !saw_column_stun; seed++) {
        BattleSim s = {.rng = seed};
        SimUnitInit(&s, 0, 0, &actor);
        SimUnitInit(&s, 1, 0, &enemy_a);
        SimUnitInit(&s, 1, 1, &enemy_b);
        SimTakeTurn(&s, 0);
        if (s.units[SQUAD_SLOT_COUNT].stunned || s.units[SQUAD_SLOT_COUNT + 1].stunned) {
            assert(s.units[SQUAD_SLOT_COUNT].power == 50 || s.units[SQUAD_SLOT_COUNT + 1].power == 150);
            saw_column_stun = true;
        }
    }
    assert(saw_column_stun);

    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_GREYWARDENLOGHAIN",
        "attack_row_stun75_followup30_single200_drain40",
        TARGET_ROW,
        200,
        10000
    );
    actor_ability.mechanics = ABILITY_ATTACK | ABILITY_POWER_DRAIN | ABILITY_STUN | ABILITY_MULTIATTACK;
    recipe = SimRecipe(&actor);
    assert(recipe->generated && recipe->generated_step_count == 2);
    assert(recipe->generated_steps[0].attack.target == RECIPE_TARGET_ROW);
    assert(recipe->generated_steps[0].attack.stun_pct == 75);
    assert(recipe->generated_steps[1].chance_pct == 30);
    assert(recipe->generated_steps[1].attack.target == RECIPE_TARGET_ONE);
    assert(recipe->generated_steps[1].attack.dmg_pct == 200);
    assert(recipe->generated_steps[1].attack.drain_pct == 40);

    recipe = SimGeneratedRecipe("attackcolumn_20pwrdrn_secondattack30stun_heal20all");
    assert(recipe->generated && recipe->generated_step_count == 2);
    assert(recipe->generated_steps[0].attack.drain_pct == 20);
    assert(recipe->generated_steps[0].fx.ally_heal_pct == 20);
    assert(recipe->generated_steps[1].chance_pct == 30);
    assert(recipe->generated_steps[1].attack.target == RECIPE_TARGET_COLUMN);
    assert(recipe->generated_steps[1].attack.stun_pct == 100);
}

static void TestGeneratedChanceGroup(void) {
    Character actor, enemy_a, enemy_b;
    Ability actor_ability, enemy_a_ability, enemy_b_ability;
    TestCharacter(
        &actor,
        &actor_ability,
        "UNIT_KALTENZAHN",
        "attack_all_curse_followup50_single_slow_all_chance25_heal50",
        TARGET_ALL,
        200,
        10000
    );
    actor_ability.mechanics = ABILITY_ATTACK | ABILITY_SLOW | ABILITY_CURSE_POWER | ABILITY_CURSE_HEALTH |
                              ABILITY_HEAL_SELF | ABILITY_MULTIATTACK;
    TestCharacter(&enemy_a, &enemy_a_ability, "enemy-a", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&enemy_b, &enemy_b_ability, "enemy-b", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);

    const AbilityRecipe *recipe = SimRecipe(&actor);
    assert(recipe->generated && recipe->generated_step_count == 4);
    assert(recipe->generated_steps[1].chance_group == 1);
    assert(recipe->generated_steps[2].chance_group == 1);

    bool saw_proc = false, saw_skip = false;
    for (unsigned seed = 1; seed < 300; seed++) {
        BattleSim s = {.rng = seed};
        SimUnitInit(&s, 0, 0, &actor);
        SimUnitInit(&s, 1, 0, &enemy_a);
        SimUnitInit(&s, 1, 1, &enemy_b);
        SimTakeTurn(&s, 0);
        int damaging_actions = 0;
        for (int action = 0; action < s.action_count; action++) {
            bool damaged = false;
            for (int target = 0; target < s.actions[action].target_count; target++)
                damaged |= s.actions[action].targets[target].health_delta < 0;
            damaging_actions += damaged;
        }
        bool attacked = damaging_actions == 2;
        bool slowed =
            s.units[SQUAD_SLOT_COUNT].cur_initiative == -50 && s.units[SQUAD_SLOT_COUNT + 1].cur_initiative == -50;
        assert(attacked == slowed);
        saw_proc |= attacked;
        saw_skip |= !attacked;
        if (saw_proc && saw_skip) break;
    }
    assert(saw_proc && saw_skip);
}

static void TestTimelineCapacity(void) {
    BattleSim s = {.rng = 1};
    Character actor, enemy;
    Ability actor_ability, enemy_ability;
    TestCharacter(&actor, &actor_ability, "actor", "ABILITY_DAMAGE_ONE", TARGET_ONE, 1, 10000);
    TestCharacter(&enemy, &enemy_ability, "enemy", "ABILITY_DAMAGE_ONE", TARGET_ONE, 1, 100000);
    SimUnitInit(&s, 0, 0, &actor);
    SimUnitInit(&s, 1, 0, &enemy);

    AbilityRecipe recipe = {
        .generated = true,
        .generated_step_count = 1,
        .generated_steps = {{
            .attack = {.self_crit_pct = 1},
            .chance_pct = 100,
            .repeat = 10,
        }},
    };
    for (int turn = 0; turn < 100; turn++) {
        SimGeneratedTakeTurn(&s, 0, &recipe, false);
        s.units[0].curse_power = true;
        s.units[0].curse_power_turns = 1;
        SimExpireAilments(&s, 0, false);
    }
    for (int round = 1; round < 10; round++) {
        BattleSimAction *action = SimEmit(&s, -1);
        action->newround = true;
    }

    assert(s.action_count == 2109);
}

static void TestLegendaryStars(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    static const struct {
        const char *id;
        int stars;
    } cases[] = {
        {"UNIT_HIGHDRAGON", 1},
        {"UNIT_ADVISORMORRIGAN", 2},
        {"UNIT_NECROMANCERDORIAN", 3},
    };

    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        Character ch;
        Ability ability;
        CharacterParse(CatalogFindCharacter(catalog, cases[i].id), &ch, &ability);
        assert(ch.rarity == RARITY_LEGENDARY);
        assert(ch.stars == cases[i].stars);
        assert(LegendaryStarRating(&ch) == cases[i].stars);
    }

    Character mythic = {.rarity = RARITY_MYTHIC, .stars = 3};
    Character epic = {.rarity = RARITY_EPIC, .stars = 3};
    assert(LegendaryStarRating(&mythic) == 0);
    assert(LegendaryStarRating(&epic) == 0);
    assert(RarityBaseTier(RARITY_LEGENDARY) == 5);
    assert(RarityBaseTier(RARITY_MYTHIC) == 6);
    assert(RarityBaseTier(RARITY_EPIC) == 4);

    int ratings[4] = {0};
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, catalog) {
        Character ch;
        Ability ability;
        CharacterParse(entry, &ch, &ability);
        if (ch.rarity == RARITY_LEGENDARY) {
            assert(ch.stars >= 1 && ch.stars <= 3);
            assert(LegendaryStarRating(&ch) == ch.stars);
            ratings[ch.stars]++;
        } else {
            assert(LegendaryStarRating(&ch) == 0);
            if (ch.rarity <= RARITY_EPIC) assert(ch.stars == 0);
        }
    }
    assert(ratings[1] > 0 && ratings[2] > 0 && ratings[3] > 0);

    assert(ResourceExists(CACHE_RESOURCE("base_gold_star")));
    assert(FileExists(HOD_CACHE "meshes/legendary_base/stars_1.obj"));
    assert(FileExists(HOD_CACHE "meshes/legendary_base/stars_2.obj"));
    assert(FileExists(HOD_CACHE "meshes/legendary_base/stars_3.obj"));
    cJSON_Delete(root);
}

static void TestCatalogStatGrowth(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    Character ch;
    Ability ability;

    CharacterParse(CatalogFindCharacter(catalog, "UNIT_CARTASMUGGLER"), &ch, &ability);
    ch.tier = TIER_II;
    ch.level = 20;
    assert(ch.stats[TIER_II].power_per_level == 9);
    assert(CharacterStatPower(&ch) == 278);
    assert(CharacterStatHealth(&ch) == 534);

    CharacterParse(CatalogFindCharacter(catalog, "UNIT_ELVENMAGE"), &ch, &ability);
    ch.tier = TIER_II;
    ch.level = 25;
    assert(ch.stats[TIER_II].power_per_level == 10);
    assert(CharacterStatPower(&ch) == 395);
    assert(CharacterStatHealth(&ch) == 1278);

    cJSON_Delete(root);
}

static void TestStatBudgetScalesEveryHeroToOneCostAndKeepsItsShape(void) {
    const StatBudget budget = {.budget = 3000, .health_per_power = 5, .giant_budget_pct = 150};
    int weak_power = 100, weak_health = 500;
    int strong_power = 2000, strong_health = 20000;
    int giant_power = 100, giant_health = 500;
    int statless_power = 0, statless_health = 0;

    StatBudgetApply(&budget, false, &weak_power, &weak_health);
    StatBudgetApply(&budget, false, &strong_power, &strong_health);
    StatBudgetApply(&budget, true, &giant_power, &giant_health);
    StatBudgetApply(&budget, false, &statless_power, &statless_health);

    assert(weak_power == 300 && weak_health == 1500);
    assert(strong_power == 200 && strong_health == 2000);
    assert(giant_power == 450 && giant_health == 2250);
    assert(statless_power == 0 && statless_health == 0);
    assert(StatBudgetCost(&budget, weak_power, weak_health) == StatBudgetCost(&budget, strong_power, strong_health));
}

static void TestStatBudgetOnTheEnemyRosterLevelsBothSides(void) {
    static const StatBudget budget = {.budget = 3000, .health_per_power = 5, .giant_budget_pct = 150};
    static BattleRoster player, enemy;
    static BattleSim sim;
    memset(&player, 0, sizeof(player));
    memset(&enemy, 0, sizeof(enemy));
    TestCharacter(&player.chars[0], &player.abilities[0], "weak", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 500);
    TestCharacter(&enemy.chars[0], &enemy.abilities[0], "strong", "ABILITY_DAMAGE_ONE", TARGET_ONE, 2000, 20000);
    player.present[0] = enemy.present[0] = true;

    BattleSimRunRosters(&sim, &player, &enemy, 1);
    assert(sim.units[0].start_power == 100 && sim.units[SQUAD_SLOT_COUNT].start_power == 2000);

    enemy.stat_budget = &budget;
    BattleSimRunRosters(&sim, &player, &enemy, 1);
    assert(sim.units[0].start_power == 300 && sim.units[SQUAD_SLOT_COUNT].start_power == 200);
    assert(sim.units[0].max_health * 2000 == sim.units[SQUAD_SLOT_COUNT].max_health * 1500);
}

static void TestStatBudgetTableLoads(void) {
    const StatBudget *budget = StatBudgetEnsure();
    assert(budget->budget > 0 && budget->health_per_power > 0 && budget->giant_budget_pct >= 100);
}

static void TestCustomHeroesInheritTheirBaseHeroData(void) {
    Battle *battle = calloc(1, sizeof(*battle));
    assert(battle);
    BattleAudioManifestLoad(battle, "data/unit_audio.json");
    PassiveManifestEnsure();
    cJSON *custom_root = JsonLoadFile(CUSTOM_HERO_CATALOG_PATH);
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    int heroes = 0;
    cJSON *hero = NULL;
    cJSON_ArrayForEach(hero, cJSON_GetObjectItemCaseSensitive(custom_root, "characters")) {
        Character custom, base;
        Ability custom_ability, base_ability;
        const cJSON *custom_definition = CatalogFindCharacter(catalog, JsonStr(hero, "id"));
        assert(custom_definition);
        CharacterParse(custom_definition, &custom, &custom_ability);
        const cJSON *base_definition = CatalogFindCharacter(catalog, custom.based_on);
        assert(base_definition);
        CharacterParse(base_definition, &base, &base_ability);
        assert(!base.based_on[0]);
        assert(!strcmp(CharacterDataId(&base), base.id));
        assert(!strcmp(CharacterDataId(&custom), base.id));
        assert(!strcmp(custom_ability.key, base_ability.key));
        custom.level = base.level = 1;
        for (int tier = 0; tier < TIER_COUNT; tier++) {
            if (!custom.has_tier[tier]) continue;
            custom.tier = base.tier = (Tier)tier;
            assert(strcmp(custom.model_prefab[tier], base.model_prefab[tier]) != 0);
            assert(strcmp(custom.icon_prefab[tier], base.icon_prefab[tier]) != 0);
            assert(ConsumeProfileFor(&custom) == ConsumeProfileFor(&base));

            SimUnit units[2 * SQUAD_SLOT_COUNT];
            SimUnitsInit(units, 0, 0, &custom, NULL);
            SimUnitsInit(units, 1, 0, &base, NULL);
            assert(units[0].passives == units[SQUAD_SLOT_COUNT].passives);

            assert(BattleAudioBindUnit(battle, 0, 0, &custom) == BattleAudioBindUnit(battle, 1, 0, &base));
            assert(!strcmp(battle->unit_audio[0][0].unit, custom.id));
            assert(!strcmp(battle->unit_audio[0][0].profile, battle->unit_audio[1][0].profile));
            assert(battle->unit_audio[0][0].attack_capable == battle->unit_audio[1][0].attack_capable);
        }
        heroes++;
    }
    assert(heroes > 0);
    cJSON_Delete(root);
    cJSON_Delete(custom_root);
    BattleAudioManifestUnload(battle);
    free(battle);
}

static Campaign *TestOneNodeCampaign(int xp) {
    Campaign *campaign = calloc(1, sizeof(*campaign));
    assert(campaign);
    campaign->map_count = 1;
    campaign->maps[0].node_count = 1;
    campaign->maps[0].nodes[0].xp = xp;
    campaign->maps[0].nodes[0].wins_to_master = 3;
    return campaign;
}

static Player TestStarterPlayer(void) {
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", "data/new_player.json");
    assert(player.roster_count > 0);
    return player;
}

static void TestBattlesGrantTheirXpToEverySquadHero(void) {
    enum { NODE_XP = 20, LEVEL_TWO_FLOOR = 11, LEVEL_THREE_FLOOR = 36 };
    Player player = TestStarterPlayer();
    Campaign *campaign = TestOneNodeCampaign(NODE_XP);
    int squad_size = PlayerSquadCount(&player, player.current_squad);
    assert(squad_size > 0);

    BattleRewardResult loss = CampaignResolveBattle(&player, campaign, 0, 0, false, 0);
    assert(loss.resolved && loss.squad_leveling.count == squad_size);
    for (int i = 0; i < loss.squad_leveling.count; i++) {
        const BattleHeroLeveling *hero = &loss.squad_leveling.heroes[i];
        assert(hero->post.xp == hero->pre.xp && hero->post.level == hero->pre.level);
    }

    BattleRewardResult win = CampaignResolveBattle(&player, campaign, 0, 0, true, 0);
    assert(win.resolved && win.xp_reward == NODE_XP && win.squad_leveling.count == squad_size);
    for (int i = 0; i < win.squad_leveling.count; i++) {
        const BattleHeroLeveling *hero = &win.squad_leveling.heroes[i];
        const Character *ch = &player.roster[hero->roster_index];
        assert(PlayerSquadUnitAt(&player, player.current_squad, hero->slot) == hero->roster_index);
        assert(hero->pre.level == 1 && hero->pre.xp == 0);
        assert(hero->post.xp == NODE_XP && ch->xp == NODE_XP);
        assert(hero->post.level == 2 && ch->level == 2);
        assert(hero->post.band_low == LEVEL_TWO_FLOOR && hero->post.band_high == LEVEL_THREE_FLOOR);
        assert(hero->post.power == CharacterStatPower(ch) && hero->post.power > hero->pre.power);
        assert(hero->post.health == CharacterStatHealth(ch) && hero->post.health > hero->pre.health);
    }

    BattleResultsFlow flow;
    BattleResultsFlowInitialize(&flow);
    BattleResultsFlowStart(
        &flow,
        (BattleResultsPresentation){
            .victory = true,
            .first_win = win.first_win,
            .player_level_pre = win.player_level_pre,
            .player_level_post = win.player_level_post,
            .squad = win.squad_leveling,
        }
    );
    assert(flow.stage == BATTLE_RESULTS_PLAYER_LEVEL);
    BattleResultsAdvance(&flow);
    for (int i = 0; i < squad_size; i++) {
        const BattleHeroLeveling *shown = BattleResultsFlowLevelUpHero(&flow);
        assert(flow.stage == BATTLE_RESULTS_HERO_LEVEL && shown == &flow.presentation.squad.heroes[i]);
        assert(flow.hero_level.value == 0.0f && flow.hero_level.max == LEVEL_TWO_FLOOR);
        int ease_steps = (int)(HERO_LEVEL_XP_EASE_SECONDS / 0.1f) + 1;
        for (int step = 0; step < ease_steps; step++) BattleResultsFlowUpdate(&flow, 0.1f, false);
        assert(flow.hero_level.crossed_level && flow.hero_level.min == LEVEL_TWO_FLOOR);
        for (int step = 0; step < ease_steps; step++) BattleResultsFlowUpdate(&flow, 0.1f, false);
        assert(flow.hero_level.value == NODE_XP && flow.hero_level.max == LEVEL_THREE_FLOOR);
        BattleResultsAdvance(&flow);
    }
    assert(flow.stage == BATTLE_RESULTS_NODE_COMPLETION && !BattleResultsFlowLevelUpHero(&flow));

    Character *capped = &player.roster[win.squad_leveling.heroes[0].roster_index];
    capped->level = capped->stats[capped->tier].max_level;
    capped->xp = ConsumeHeroLevelXp(capped, capped->level);
    int capped_xp = capped->xp;
    BattleRewardResult repeat = CampaignResolveBattle(&player, campaign, 0, 0, true, 0);
    assert(repeat.squad_leveling.heroes[0].post.maxed);
    assert(capped->xp == capped_xp && capped->level == capped->stats[capped->tier].max_level);

    free(campaign);
    PlayerUnloadRoster(&player);
}

static void TestFxPresetLiveEmitterSizes(void) {
    cJSON *root = JsonLoadFile("data/fx_presets.json");
    assert(root);
    cJSON *presets = cJSON_GetObjectItemCaseSensitive(root, "presets");
    assert(cJSON_IsArray(presets));

    int checked = 0;
    cJSON *preset = NULL;
    cJSON_ArrayForEach(preset, presets) {
        cJSON *emitters = cJSON_GetObjectItemCaseSensitive(preset, "emitters");
        assert(cJSON_IsArray(emitters));
        cJSON *emitter = NULL;
        cJSON_ArrayForEach(emitter, emitters) {
            if (JsonInt(emitter, "count") <= 0 && JsonFloat(emitter, "rate") <= 0) continue;
            float size0 = JsonFloat(emitter, "size0");
            float size1 = JsonFloat(emitter, "size1");
            assert(size0 > 0 || size1 > 0);
            checked++;
        }
    }
    assert(checked > 0);
    cJSON_Delete(root);
}

static void TestGeneratedFxCurvesAndDragonJet(void) {
    Battle *battle = calloc(1, sizeof(*battle));
    assert(battle);
    BattleFxRegistryLoad(battle, "data/fx_presets.json");

    const FxPreset *dragon = BattleFxFindPreset(battle, "vfx_dragonfire_pre");
    assert(dragon);
    const FxEmitter *fire = NULL, *beam = NULL;
    for (int i = 0; i < dragon->em_count; i++) {
        const FxEmitter *emitter = &dragon->em[i];
        if (strstr(emitter->tex, "Fire_Still01")) fire = emitter;
        if (emitter->box.z > 0.9f && emitter->orient == 2) beam = emitter;
    }
    assert(fire && fire->size0 == 8.0f && fire->size1 == 12.0f);
    assert(fire->size_end == 0.0f && fire->size_curve.count == 3);
    assert(fabsf(FxCurveSample(&fire->size_curve, 0.0f, 1.0f) - 0.023579f) < 0.00001f);
    assert(fire->size1 * FxCurveSample(&fire->size_curve, 1.0f, 1.0f) < 7.5f);
    assert(beam && beam->dir.z < -0.9f && beam->size_curve.count == 2);

    BattleFxBurstDirected(battle, "vfx_dragonfire_pre", (Vector3){0}, (Vector3){1, 0, 0});
    int directed_instance = -1;
    for (int i = 0; i < FX_MAX_INSTANCES; i++)
        if (battle->fxi[i].alive && battle->fxi[i].directed) directed_instance = i;
    assert(directed_instance >= 0);
    Vector3 facing = Vector3RotateByQuaternion((Vector3){0, 0, -1}, battle->fxi[directed_instance].rotation);
    assert(facing.x > 0.999f && fabsf(facing.y) < 0.001f && fabsf(facing.z) < 0.001f);

    const FxPreset *inquisitor = BattleFxFindPreset(battle, "fx_abelas_pre");
    assert(inquisitor);
    bool found_keyout_smoke = false;
    for (int i = 0; i < inquisitor->em_count; i++) {
        const FxEmitter *emitter = &inquisitor->em[i];
        if (emitter->keyout == 1) {
            found_keyout_smoke = true;
            assert(emitter->alpha_curve.count >= 2 && emitter->max_screen_size > 0.0f);
        }
    }
    assert(found_keyout_smoke);

    for (int p = 0; p < battle->fx_reg_count; p++)
        for (int e = 0; e < battle->fx_reg[p].pre.em_count; e++) {
            const FxEmitter *emitter = &battle->fx_reg[p].pre.em[e];
            assert(emitter->size_end <= 8.0f);
            assert(emitter->size_curve.count <= FX_MAX_CURVE_KEYS);
            assert(emitter->alpha_curve.count <= FX_MAX_CURVE_KEYS);
            assert(emitter->color_curve.count <= FX_MAX_CURVE_KEYS);
            assert(emitter->rate_curve.count <= FX_MAX_CURVE_KEYS);
        }

    if (battle->fx_reg) MemFree(battle->fx_reg);
    if (battle->fx_alias) MemFree(battle->fx_alias);
    free(battle);
}

static void TestPortraitCoverage(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    assert(root);
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    assert(cJSON_IsArray(catalog));

    int checked = 0;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, catalog) {
        Character ch;
        Ability ability;
        CharacterParse(entry, &ch, &ability);
        for (int tier = 0; tier < TIER_COUNT; tier++) {
            assert(ch.icon_prefab[tier][0]);
            const char *ref = GuiPortraitRef(ch.icon_prefab[tier]);
            if (!ResourceExists(ref)) fprintf(stderr, "missing portrait: %s\n", ref);
            assert(ResourceExists(ref));
            checked++;
        }
    }

    assert(checked == cJSON_GetArraySize(catalog) * TIER_COUNT);
    cJSON_Delete(root);
}

static const char *TestLocaleFile(const char *locale) {
    const LocLanguage *language = LocLanguageForLocale(locale);
    return language ? language->file : NULL;
}

static void TestSystemLocalesPickTheirOriginalLocalization(void) {
    assert(!strcmp(TestLocaleFile("en-US"), "Loc_ENG_US.txt"));
    assert(!strcmp(TestLocaleFile("fr_FR.UTF-8"), "Loc_FRE_FR.txt"));
    assert(!strcmp(TestLocaleFile("de"), "Loc_GER_DE.txt"));
    assert(!strcmp(TestLocaleFile("pt-BR"), "Loc_POR_BR.txt"));
    assert(!strcmp(TestLocaleFile("ru-RU"), "Loc_RUS_RU.txt"));
    assert(!strcmp(TestLocaleFile("ja-JP"), "Loc_JPN_JP.txt"));
    assert(!strcmp(TestLocaleFile("zh-Hans-CN"), "Loc_CHS_CN.txt"));
    assert(!strcmp(TestLocaleFile("zh-Hant-TW"), "Loc_CHT_CN.txt"));
    assert(!strcmp(TestLocaleFile("zh_TW"), "Loc_CHT_CN.txt"));
    assert(!strcmp(TestLocaleFile("zh-CN"), "Loc_CHS_CN.txt"));
    assert(TestLocaleFile("sv-SE") == NULL);
    assert(TestLocaleFile("english") == NULL);
}

static void TestFontCoversEveryGlyphOfTheLoadedLanguage(void) {
    Loc saved = loc;
    loc = (Loc){0};
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_JPN_JP.txt");
    static int codepoints[FONT_GLYPH_CAP];
    int count = FontCodepoints(codepoints, FNT_MENU);
    assert(count > FONT_DENSE_GLYPHS && count < FONT_GLYPH_CAP);
    bool found = false;
    for (int i = 0; i < count; i++) found |= codepoints[i] == 0x30AD;
    assert(found);
    LocUnload();
    loc = saved;
}

static void TestLocalizationLayersProjectStringsOverTheOriginalTable(void) {
    Loc saved = loc;
    loc = (Loc){0};
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_ENG_US.txt");
    assert(!strcmp(LocGet("OPTIONS_MORE_TITLE"), "More"));
    assert(!strcmp(LocGet("options_more_title"), "More"));
    assert(!strcmp(LocGet("HODA_OPTIONS_LANGUAGE"), "Language"));
    assert(!strcmp(LocGet("HODA_NO_SUCH_KEY"), ""));
    char text[64];
    LocFormat1(text, sizeof(text), "OPTIONS_PLAYER_ID", "42");
    assert(!strcmp(text, "Player ID: 42"));
    LocUnload();
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_GER_DE.txt");
    assert(!strcmp(LocGet("HODA_OPTIONS_LANGUAGE"), "Sprache"));
    LocUnload();
    loc = saved;
}

static bool HasNonAscii(const char *text) {
    for (const unsigned char *c = (const unsigned char *)text; *c; c++)
        if (*c >= 0x80) return true;
    return false;
}

static bool TestFontIncludesCodepoint(int codepoint) {
    static int codepoints[FONT_GLYPH_CAP];
    static int count;
    static const LocEntry *collected_from;
    static int collected_count;
    static bool collected;
    if (!collected || collected_from != loc.entries || collected_count != loc.count) {
        count = FontCodepoints(codepoints, FNT_MENU);
        collected = true;
        collected_from = loc.entries;
        collected_count = loc.count;
    }
    for (int i = 0; i < count; i++)
        if (codepoints[i] == codepoint) return true;
    return false;
}

static int TestUtf8Codepoint(const unsigned char *text, int *bytes) {
    unsigned char a = text[0];
    if (a < 0x80) {
        *bytes = 1;
        return a;
    }
    int n = a < 0xE0 ? 2 : a < 0xF0 ? 3 : a < 0xF8 ? 4 : 0;
    if (!n) return -1;
    int value = a & (0x7F >> n);
    for (int i = 1; i < n; i++) {
        if ((text[i] & 0xC0) != 0x80) return -1;
        value = (value << 6) | (text[i] & 0x3F);
    }
    *bytes = n;
    return value;
}

static void TestRichText(const char *text) {
    const char *cursor = text;
    int steps = 0;
    while (*cursor) {
        HeroesRichToken token = HeroesRichNextToken(cursor, WHITE);
        assert(token.kind != HEROES_RICH_END && token.bytes > 0);
        assert(!(token.kind == HEROES_RICH_TEXT && *cursor == '<'));
        cursor += token.bytes;
        assert(++steps <= (int)strlen(text));
    }

    for (const unsigned char *c = (const unsigned char *)text; *c;) {
        int bytes = 0;
        int codepoint = TestUtf8Codepoint(c, &bytes);
        assert(codepoint >= 0 && codepoint != 0xFFFD);
        if (codepoint >= 0x80) assert(TestFontIncludesCodepoint(codepoint));
        c += bytes;
    }
}

static void TestRichTextParserFallback(void) {
    HeroesRichToken token = HeroesRichNextToken("<Color=green>text</Color>", WHITE);
    assert(token.kind == HEROES_RICH_COLOR_OPEN && token.bytes == 13);
    assert(token.color.g == 128 && token.color.r == 0);
    token = HeroesRichNextToken("<color=\"orange\">text", WHITE);
    assert(token.kind == HEROES_RICH_COLOR_OPEN && token.color.r == 255 && token.color.g == 165);
    token = HeroesRichNextToken("<unknown>", WHITE);
    assert(token.kind == HEROES_RICH_TEXT && token.bytes == 1);

    assert(TestFontIncludesCodepoint(0x00A0));
    assert(TestFontIncludesCodepoint(0x00AB));
    assert(TestFontIncludesCodepoint(0x00BB));
    assert(TestFontIncludesCodepoint(0x2212));
}

static void TestCharacterTextResolvesInEveryLanguage(void) {
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    for (int language = 0; language < LocLanguageCount(); language++) {
        char path[LOC_PATH_CAP];
        LocLanguagePath(LocLanguageAt(language), path, sizeof(path));
        LocLoad(path);
        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, catalog) {
            Character ch;
            Ability ability;
            CharacterParse(entry, &ch, &ability);
            for (int tier = 0; tier < TIER_COUNT; tier++)
                if (ch.has_tier[tier]) assert(CharacterTierName(&ch, (Tier)tier) != ch.name);
            assert(CharacterAbilityText(&ability) != ability.text);
            if (ch.equipped_rune[0]) assert(CharacterLocText(ch.equipped_rune)[0]);
            if (ch.target_weight[0]) assert(CharacterLocText(ch.target_weight)[0]);
            for (int g = 0; g < ch.group_count; g++) assert(CharacterGroupTitle(ch.groups[g]) != ch.groups[g]);
            ch.tier = ch.max_tier;
            ch.level = 1;
            HeroCard card = {0};
            HeroCardBuild(&card, &ch);
            size_t description = strlen(CharacterAbilityText(&ability)) + 2 +
                                 strlen(CharacterLocText(ch.equipped_rune)) + 2 +
                                 strlen(CharacterLocText(ch.target_weight));
            assert(description < sizeof(card.description));
        }
        LocUnload();
    }
    cJSON_Delete(root);
}

static void TestCharacterTextCoverage(void) {
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    assert(root && JsonInt(root, "version") >= 2);
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    assert(cJSON_IsArray(catalog) && cJSON_GetArraySize(catalog) == 676);

    int checked = 0;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, catalog) {
        Character ch;
        Ability ability;
        CharacterParse(entry, &ch, &ability);
        assert(ch.id[0] && ch.name[0] && HasNonAscii(CharacterTierName(&ch, ch.max_tier)));
        assert(ability.key[0] && ability.text[0] && HasNonAscii(CharacterAbilityText(&ability)));
        assert(ability.mechanics != 0);
        assert(!!(ability.mechanics & ABILITY_ATTACK) != !!(ability.mechanics & ABILITY_ATTACK_NONE));
        ch.tier = TIER_I;
        ch.level = 1;
        HeroCard card = {0};
        HeroCardBuild(&card, &ch);
        TestRichText(card.info);
        TestRichText(card.description);
        checked++;
    }
    assert(checked == 676);
    LocUnload();

    Character ch;
    Ability ability;
    CharacterParse(CatalogFindCharacter(catalog, "UNIT_SHAPESHIFTERMORRIGANI"), &ch, &ability);
    assert(ability.mechanics & ABILITY_POWER_DRAIN);
    CharacterParse(CatalogFindCharacter(catalog, "UNIT_SHAPESHIFTERMORRIGANII"), &ch, &ability);
    assert(!(ability.mechanics & ABILITY_POWER_DRAIN));
    assert(ability.mechanics & ABILITY_GAIN_POWER_ALLIES);
    CharacterParse(CatalogFindCharacter(catalog, "UNIT_NECROMANCERDORIAN"), &ch, &ability);
    assert((ability.mechanics & (ABILITY_CURSE_HEALTH | ABILITY_STUN)) == (ABILITY_CURSE_HEALTH | ABILITY_STUN));
    assert(!(ability.mechanics & ABILITY_POWER_DRAIN));
    CharacterParse(CatalogFindCharacter(catalog, "UNIT_LAMBERTVANREEVES"), &ch, &ability);
    assert(ability.mechanics & ABILITY_GAIN_POWER_ALLIES);
    CharacterParse(CatalogFindCharacter(catalog, "UNIT_THORNTON"), &ch, &ability);
    assert(
        (ability.mechanics & (ABILITY_CURSE_POWER | ABILITY_MULTIATTACK)) == (ABILITY_CURSE_POWER | ABILITY_MULTIATTACK)
    );
    assert(strstr(ability.text, "Chance for second attack"));

    cJSON_Delete(root);
}

static void TestAbilityMechanicParsing(void) {
    static const struct {
        const char *name;
        unsigned mechanic;
    } cases[] = {
        {"ATTACK", ABILITY_ATTACK},
        {"ATTACK_NONE", ABILITY_ATTACK_NONE},
        {"POWER_DRAIN", ABILITY_POWER_DRAIN},
        {"STUN", ABILITY_STUN},
        {"SLOW", ABILITY_SLOW},
        {"CURSE_POWER", ABILITY_CURSE_POWER},
        {"CURSE_HEALTH", ABILITY_CURSE_HEALTH},
        {"HEAL_ALLIES", ABILITY_HEAL_ALLIES},
        {"HEAL_SELF", ABILITY_HEAL_SELF},
        {"GAIN_POWER_ALLIES", ABILITY_GAIN_POWER_ALLIES},
        {"GAIN_POWER_SELF", ABILITY_GAIN_POWER_SELF},
        {"MULTIATTACK", ABILITY_MULTIATTACK},
        {"MARK", ABILITY_MARK},
        {"LEVY", ABILITY_LEVY},
        {"ASSASSINATE", ABILITY_ASSASSINATE},
        {"ATTACK_FRIENDLY", ABILITY_ATTACK_FRIENDLY},
        {"DAMAGE_SELF", ABILITY_DAMAGE_SELF},
        {"LOSE_POWER_SELF", ABILITY_LOSE_POWER_SELF},
        {"SLOW_ALLIES", ABILITY_SLOW_ALLIES},
        {"GAIN_CRIT_ALLIES", ABILITY_GAIN_CRIT_ALLIES},
        {"CURSE_TWO_ROUNDS", ABILITY_CURSE_TWO_ROUNDS},
        {"GAIN_DAMAGE_ALLIES", ABILITY_GAIN_DAMAGE_ALLIES},
        {"GAIN_DAMAGE_SELF", ABILITY_GAIN_DAMAGE_SELF},
    };
    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++)
        assert(AbilityMechanicFromString(cases[i].name) == cases[i].mechanic);
    assert(AbilityMechanicFromString("UNKNOWN") == 0);
}

static unsigned RecipeMechanics(const AbilityRecipe *recipe) {
    unsigned mechanics = ABILITY_ATTACK;
    if (recipe->hit.drain_pct || recipe->effect.drain_pct || recipe->followup.drain_pct)
        mechanics |= ABILITY_POWER_DRAIN;
    if (recipe->hit.stun_pct || recipe->effect.stun_pct || recipe->followup.stun_pct) mechanics |= ABILITY_STUN;
    if (recipe->hit.slow_pct || recipe->effect.slow_pct || recipe->followup.slow_pct) mechanics |= ABILITY_SLOW;
    if (recipe->hit.curse_power_pct || recipe->effect.curse_power_pct || recipe->followup.curse_power_pct)
        mechanics |= ABILITY_CURSE_POWER;
    if (recipe->hit.curse_health_pct || recipe->effect.curse_health_pct || recipe->followup.curse_health_pct)
        mechanics |= ABILITY_CURSE_HEALTH;
    if (recipe->fx.ally_heal_pct) mechanics |= ABILITY_HEAL_ALLIES;
    if (recipe->fx.self_heal_pct || recipe->hit.self_heal_pct || recipe->effect.self_heal_pct ||
        recipe->followup.self_heal_pct)
        mechanics |= ABILITY_HEAL_SELF;
    if (recipe->fx.ally_power_pct || recipe->fx.ally_power_from_actor_pct) mechanics |= ABILITY_GAIN_POWER_ALLIES;
    if (recipe->fx.self_power_pct > 0) mechanics |= ABILITY_GAIN_POWER_SELF;
    if (recipe->fx.self_power_pct < 0) mechanics |= ABILITY_LOSE_POWER_SELF;
    if (recipe->fx.self_damage_pct < 0) mechanics |= ABILITY_DAMAGE_SELF;
    if (recipe->fx.ally_slow_amt < 0) mechanics |= ABILITY_SLOW_ALLIES;
    if (recipe->fx.ally_crit_pct > 0) mechanics |= ABILITY_GAIN_CRIT_ALLIES;
    if (recipe->fx.ally_damage_bonus_pct > 0) mechanics |= ABILITY_GAIN_DAMAGE_ALLIES;
    if (recipe->fx.self_damage_bonus_pct > 0) mechanics |= ABILITY_GAIN_DAMAGE_SELF;
    if (recipe->hit.curse_turns > 1 || recipe->effect.curse_turns > 1 || recipe->followup.curse_turns > 1)
        mechanics |= ABILITY_CURSE_TWO_ROUNDS;
    if (recipe->followup_count && !recipe->followup.status_only) mechanics |= ABILITY_MULTIATTACK;
    if (recipe->hit.mark_pct || recipe->effect.mark_pct || recipe->followup.mark_pct) mechanics |= ABILITY_MARK;
    if (recipe->hit.levy_pct || recipe->effect.levy_pct || recipe->followup.levy_pct) mechanics |= ABILITY_LEVY;
    if (recipe->hit.assassinate_pct || recipe->effect.assassinate_pct || recipe->followup.assassinate_pct)
        mechanics |= ABILITY_ASSASSINATE;
    if (recipe->fx.ally_damage_pct || recipe->effect.target == RECIPE_TARGET_ONE_ALLY ||
        recipe->followup.target == RECIPE_TARGET_ONE_ALLY)
        mechanics |= ABILITY_ATTACK_FRIENDLY;
    return mechanics;
}

static void TestExactRecipesMatchDescriptions(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    unsigned core = ABILITY_POWER_DRAIN | ABILITY_STUN | ABILITY_SLOW | ABILITY_CURSE_POWER | ABILITY_CURSE_HEALTH |
                    ABILITY_HEAL_ALLIES | ABILITY_HEAL_SELF | ABILITY_GAIN_POWER_ALLIES | ABILITY_GAIN_POWER_SELF |
                    ABILITY_MULTIATTACK | ABILITY_MARK | ABILITY_LEVY | ABILITY_ASSASSINATE | ABILITY_ATTACK_FRIENDLY |
                    ABILITY_DAMAGE_SELF | ABILITY_LOSE_POWER_SELF | ABILITY_SLOW_ALLIES | ABILITY_GAIN_CRIT_ALLIES |
                    ABILITY_CURSE_TWO_ROUNDS | ABILITY_GAIN_DAMAGE_ALLIES | ABILITY_GAIN_DAMAGE_SELF;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, catalog) {
        Character ch;
        Ability ability;
        CharacterParse(entry, &ch, &ability);
        const AbilityRecipe *recipe = SimExactRecipe(&ch);
        if (recipe == &SIM_RECIPE_DEFAULT) continue;
        unsigned expected = ability.mechanics & core;
        unsigned implemented = RecipeMechanics(recipe) & core;
        unsigned missing = expected & ~implemented;
        unsigned extra = implemented & ~expected;
        if (missing) fprintf(stderr, "recipe missing mechanics: %s (%s) mask=%u\n", ch.id, ability.key, missing);
        if (extra) fprintf(stderr, "recipe has extra mechanics: %s (%s) mask=%u\n", ch.id, ability.key, extra);
        assert(missing == 0);
        assert(extra == 0);
    }
    cJSON_Delete(root);
}

static void TestGeneratedRecipeCoverage(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    char keys[421][128] = {{0}};
    int key_count = 0, generated_count = 0;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, catalog) {
        Character ch;
        Ability ability;
        CharacterParse(entry, &ch, &ability);
        bool seen = false;
        for (int i = 0; i < key_count; i++)
            if (!strcmp(keys[i], ability.key)) seen = true;
        if (seen) continue;
        assert(key_count < 421);
        StrCopy(keys[key_count++], sizeof(keys[0]), ability.key);
        const AbilityRecipe *recipe = SimRecipe(&ch);
        assert(recipe != &SIM_RECIPE_DEFAULT);
        if (recipe->generated) generated_count++;
    }
    assert(key_count == 421);
    assert(generated_count >= 300);
    cJSON_Delete(root);
}

static void CheckExactRecipe(const cJSON *catalog, const char *id, int *checked) {
    cJSON *def = CatalogFindCharacter(catalog, id);
    assert(def != NULL);
    Character ch;
    Ability ability;
    CharacterParse(def, &ch, &ability);
    assert(SimExactRecipe(&ch) != &SIM_RECIPE_DEFAULT);
    (*checked)++;
}

static void TestRecipeCoverage(void) {
    cJSON *catalog_root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *player_root = JsonLoadFile(HOD_CACHE "player_starter.json");
    cJSON *campaign_root = JsonLoadFile("data/campaign_rosters.json");
    assert(catalog_root && player_root && campaign_root);
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    int checked = 0;

    cJSON *squads = cJSON_GetObjectItemCaseSensitive(player_root, "squads");
    cJSON *squad = cJSON_GetArrayItem(squads, JsonInt(player_root, "current_squad"));
    cJSON *id = NULL;
    cJSON_ArrayForEach(id, squad) {
        if (cJSON_IsString(id)) CheckExactRecipe(catalog, id->valuestring, &checked);
    }
    cJSON *campaign = NULL;
    cJSON_ArrayForEach(campaign, cJSON_GetObjectItemCaseSensitive(campaign_root, "campaigns")) {
        cJSON *node = NULL;
        cJSON_ArrayForEach(node, cJSON_GetObjectItemCaseSensitive(campaign, "nodes")) {
            cJSON *unit = NULL;
            cJSON_ArrayForEach(unit, cJSON_GetObjectItemCaseSensitive(node, "roster")) {
                if (cJSON_IsObject(unit)) CheckExactRecipe(catalog, JsonStr(unit, "id"), &checked);
            }
        }
    }
    assert(checked > 45);
    cJSON_Delete(campaign_root);
    cJSON_Delete(player_root);
    cJSON_Delete(catalog_root);
}

static void TestAttackPresentationBindings(void) {
    Battle *battle = calloc(1, sizeof(*battle));
    assert(battle != NULL);

    BattleWiresLoad(battle, "data/unit_vfx.json");
    assert(battle->wire_count == 676 * TIER_COUNT);

    const BattleVfxWire *nug_i = BattleUnitWire(battle, "UNIT_CHOCOLATENUG", TIER_I);
    const BattleVfxWire *nug_ii = BattleUnitWire(battle, "UNIT_CHOCOLATENUG", TIER_II);
    assert(nug_i && nug_ii);
    assert(strcmp(nug_i->ability, "vfx_fangs_pre") == 0);
    assert(strcmp(nug_ii->ability, "fx_NugShmooples_pre") == 0);

    const BattleVfxWire *wisp = BattleUnitWire(battle, "UNIT_COMMONWHITEWISP", TIER_I);
    assert(wisp && wisp->vfx_type == 0);
    assert(wisp->ability[0] == '\0');
    assert(strcmp(wisp->muzzle, "fx_wispchar_t1_imp_pre") == 0);

    const BattleVfxWire *sandal = BattleUnitWire(battle, "UNIT_SANDAL", TIER_I);
    const BattleVfxWire *meredith = BattleUnitWire(battle, "UNIT_REDLYRIUMMEREDITH", TIER_IV);
    const BattleVfxWire *josephine = BattleUnitWire(battle, "UNIT_JOSEPHINE", TIER_I);
    assert(sandal && sandal->ability_type == 6 && sandal->vfx_type == 2 && sandal->cone_delay == 5.0f);
    assert(meredith && meredith->ability_type == 6 && meredith->vfx_type == 2 && meredith->cone_delay == 2.5f);
    assert(josephine && josephine->ability_type == 0 && josephine->vfx_type == 1 && josephine->cone_delay == 2.5f);
    assert(BpConeImpactDelay(sandal) == 5.0f && BpConeImpactDelay(meredith) == 2.5f);
    assert(BpConeImpactDelay(josephine) == 0.0f);

    BattleAudioManifestLoad(battle, "data/unit_audio.json");
    assert(battle->unit_audio_character_count == 676);
    const char *faction_sound = BattleAudioSourceRef(battle, "gui_bonus_faction");
    assert(strcmp(faction_sound, CACHE_RESOURCE("gui_bonus_faction")) == 0);
    assert(ResourceExists(faction_sound));
    const char *dwarf_vox = BattleAudioSourceRef(battle, "Dark_Dwarves_Attack_VOX_01");
    assert(strcmp(dwarf_vox, CACHE_RESOURCE("Dark_Dwarves_Attack_VOX_01")) == 0);
    Character unit = {0};
    StrCopy(unit.id, sizeof(unit.id), "UNIT_NECROMANCERDORIAN");
    unit.tier = TIER_IV;
    assert(BattleAudioBindUnit(battle, 0, 0, &unit));
    const BattleAudioPool *dorian = &battle->unit_audio[0][0].pools[UNIT_SFX_ATTACK];
    assert(dorian->count == 1);
    assert(strcmp(dorian->clips[0], "vfx_NecroDorian_01") == 0);

    StrCopy(unit.id, sizeof(unit.id), "UNIT_SEEKERCASSANDRA");
    unit.tier = TIER_I;
    assert(BattleAudioBindUnit(battle, 0, 0, &unit));
    unit.tier = TIER_II;
    assert(BattleAudioBindUnit(battle, 0, 1, &unit));
    const char *cassandra_i = battle->unit_audio[0][0].pools[UNIT_SFX_ATTACK].clips[0];
    const char *cassandra_ii = battle->unit_audio[0][1].pools[UNIT_SFX_ATTACK].clips[0];
    assert(strcmp(cassandra_i, "vfx_Aveline_01") == 0);
    assert(strcmp(cassandra_ii, "vfx_Leliana_Imp_01") == 0);

    BattleAudioManifestUnload(battle);
    MemFree(battle->wires);
    free(battle);
}

static void TestConeDelayDefersImpact(void) {
    Battle *battle = calloc(1, sizeof(*battle));
    assert(battle);
    BattleVfxWire wire = {
        .tier = TIER_I,
        .vfx_type = 2,
        .ability_type = 6,
        .cone_delay = 0.5f,
    };
    StrCopy(wire.unit, sizeof(wire.unit), "TEST_DELAYED_CONE");
    StrCopy(wire.ability, sizeof(wire.ability), "test_missing_cone_pre");
    battle->wires = &wire;
    battle->wire_count = 1;
    battle->unit_audio[0][0].bound = true;

    Character actor = {.tier = TIER_I};
    Character target = {.tier = TIER_I};
    StrCopy(actor.id, sizeof(actor.id), wire.unit);
    BattlePlay bp = {
        .state = BP_ACT_WINDUP,
        .t = 0.2f,
        .speed_mult = 1.0f,
        .slowmo = 0.5f,
    };
    bp.sim.action_count = 1;
    bp.sim.units[0] = (SimUnit){.present = true, .alive = true, .ch = &actor};
    bp.sim.units[SQUAD_SLOT_COUNT] = (SimUnit){.present = true, .alive = true, .ch = &target};
    bp.sim.actions[0] = (BattleSimAction){
        .actor = 0,
        .replay_attack = true,
        .target_count = 1,
        .targets = {{.unit = SQUAD_SLOT_COUNT, .health_delta = -25}},
    };
    bp.units[0][0] = (BpUnit){.health = 100, .max_health = 100, .pos = {0, BATTLE_MARKER_Y, 0}};
    bp.units[1][0] = (BpUnit){.health = 100, .max_health = 100, .pos = {2, BATTLE_MARKER_Y, 2}};

    BpFire(&bp, battle);
    assert(bp.state == BP_ACT_CONE_DELAY);
    assert(bp.cone_impact_at == 0.5f);
    assert(bp.units[1][0].health == 100 && bp.message_count == 0);

    BattlePlayUpdate(&bp, battle, 0.58f);
    assert(bp.state == BP_ACT_CONE_DELAY && bp.units[1][0].health == 100);
    BattlePlayUpdate(&bp, battle, 0.04f);
    assert(bp.state == BP_ACT_MESSAGES && bp.units[1][0].health == 75);
    assert(bp.message_count == 1 && bp.cone_impact_at == 0.0f);

    battle->wires = NULL;
    free(battle);
}

static void TestAddGroup(Character *ch, const char *group) {
    assert(ch->group_count < MAX_GROUPS);
    StrCopy(ch->groups[ch->group_count++], sizeof(ch->groups[0]), group);
}

static void TestAddSpecialty(Character *ch, Specialty specialty) {
    assert(ch->specialty_count < MAX_SPECIALTIES);
    ch->specialties[ch->specialty_count++] = specialty;
}

static void TestBattlePreviewUsesFinalSetupStats(void) {
    Character characters[4];
    Ability abilities[4];
    BattleRoster enemy = {0};
    for (int slot = 0; slot < 4; slot++) {
        char id[32];
        snprintf(id, sizeof(id), "preview-demon-%d", slot);
        TestCharacter(&characters[slot], &abilities[slot], id, "test", TARGET_ONE, 100, 1000);
        characters[slot].faction = FACTION_BLACK;
        characters[slot].rarity = RARITY_COMMON;
        TestAddGroup(&characters[slot], "Demon");

        snprintf(id, sizeof(id), "preview-enemy-%d", slot);
        TestCharacter(&enemy.chars[slot], &enemy.abilities[slot], id, "test", TARGET_ONE, 120, 1000);
        enemy.chars[slot].faction = FACTION_BLACK;
        enemy.chars[slot].rarity = RARITY_COMMON;
        enemy.present[slot] = true;
    }
    StrCopy(characters[0].id, sizeof(characters[0].id), "UNIT_ENVYDEMON");

    Player player = {.roster = characters, .roster_abilities = abilities, .roster_count = 4};
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++)
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) player.squads[squad][slot] = -1;
    for (int slot = 0; slot < 4; slot++) player.squads[0][slot] = slot;

    BattleSim *setup = (BattleSim *)calloc(1, sizeof(*setup));
    assert(setup != NULL);
    BattleSimSetup(setup, &player, &enemy);
    assert(setup->units[0].start_power == 156 && setup->units[0].max_health == 1500);
    assert(setup->units[1].start_power == 156 && setup->units[1].max_health == 2250);
    assert(setup->units[SQUAD_SLOT_COUNT].start_power == 125);
    assert(setup->formation_faction[0] == 1u && setup->formation_bonus_pct[0] == 4.0f);

    SimUnit preview[SIM_UNIT_COUNT];
    BattleSimPreview(preview, &player, &enemy);
    assert(memcmp(preview, setup->units, sizeof(preview)) == 0);

    float ratio = BattleUiDifficultyRatio(preview);
    assert(ratio < 0.85f);
    assert(strcmp(BattleUiDifficultyKey(ratio), "PVE_COLORED_EASY") == 0);
    assert(strcmp(BattleUiDifficultyKey(1.2f), "PVE_COLORED_MEDIUM") == 0);

    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    BattleUi ui = {0};
    BattleUiEnterNode(&ui, &player, &enemy, NULL, "PVE_MAP_01", 1);
    assert(ui.plates[0][0].power == 156 && ui.plates[0][0].health == 1500);
    assert(ui.plates[0][1].power == 156 && ui.plates[0][1].health == 2250);
    assert(ui.plates[1][0].power == 125 && ui.plates[1][0].health == 1000);

    char easy[64];
    Color easy_color = WHITE;
    BattleUiParseColored(LocGet("PVE_COLORED_EASY"), easy, sizeof(easy), &easy_color);
    assert(strcmp(ui.diff_value, easy) == 0);

    BattleUiOpenPlateDetails(&ui, &ui.plates[0][1]);
    assert(ui.detail_open);
    assert(ui.detail_card.power == 100 && ui.detail_card.health == 1000);
    assert(ui.detail_card.power != ui.plates[0][1].power);
    assert(ui.detail_card.health != ui.plates[0][1].health);
    LocUnload();
    free(setup);
}

static void TestIntrinsicPassiveManifest(void) {
    PassiveManifestEnsure();
    assert(passive_manifest.loaded);
    assert(passive_manifest.character_count >= 500);
    const PassiveCharacter *envy = PassiveManifestFind("UNIT_ENVYDEMON");
    assert(envy && envy->effect_count == 7);
    assert(envy->effects[0].kind == PASSIVE_STAT_PERCENT);
    assert(envy->effects[0].stats == PASSIVE_STAT_HEALTH);
    assert(envy->effects[0].value_pct == 50.0f);
    assert(envy->effects[3].kind == PASSIVE_INCOMING_TARGET_WEIGHT);
    assert(envy->effects[3].value_multiplier == 0.25f);
    assert(envy->effects[4].kind == PASSIVE_HEAL_PERCENT_MAX_HEALTH);
    assert(envy->effects[5].kind == PASSIVE_BONUS_DAMAGE_PERCENT);
    assert(envy->effects[6].selector == PASSIVE_SELECTOR_LOWEST_HEALTH_PERCENT);
}

static void TestEnvyBattleStartAuras(void) {
    PassiveManifestEnsure();
    BattleSim s = {0};
    Character envy, demon_red, red, lucius, plain;
    Ability abilities[5];
    TestCharacter(&envy, &abilities[0], "UNIT_ENVYDEMON", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&demon_red, &abilities[1], "demon-red", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&red, &abilities[2], "red", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&lucius, &abilities[3], "UNIT_LUCIUSCORIN", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&plain, &abilities[4], "plain", "test", TARGET_ONE, 100, 1000);
    TestAddGroup(&envy, "Demon");
    TestAddGroup(&demon_red, "Demon");
    TestAddGroup(&demon_red, "Red_Lyrium");
    TestAddGroup(&red, "Red_Lyrium");
    SimUnitInit(&s, 0, 0, &envy);
    SimUnitInit(&s, 0, 1, &demon_red);
    SimUnitInit(&s, 0, 2, &red);
    SimUnitInit(&s, 0, 3, &lucius);
    SimUnitInit(&s, 0, 4, &plain);

    SimApplyBattleStartPassives(&s);

    assert(s.units[0].max_health == 1500 && s.units[0].power == 150);
    assert(s.units[1].max_health == 2250 && s.units[1].power == 150);
    assert(s.units[2].max_health == 1000 && s.units[2].power == 150);
    assert(s.units[3].max_health == 1500 && s.units[3].power == 125);
    assert(s.units[4].max_health == 1000 && s.units[4].power == 100);
}

static void TestAvvarWarNugCompositionHealth(void) {
    PassiveManifestEnsure();
    Character characters[4];
    Ability abilities[4];
    TestCharacter(&characters[0], &abilities[0], "UNIT_AVVARWARNUG", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&characters[1], &abilities[1], "avvar-a", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&characters[2], &abilities[2], "avvar-b", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&characters[3], &abilities[3], "plain", "test", TARGET_ONE, 100, 1000);
    characters[0].giant = true;
    TestAddGroup(&characters[0], "Avvar");
    TestAddGroup(&characters[1], "Avvar");
    TestAddGroup(&characters[2], "Avvar");

    Player player = {.roster = characters, .roster_abilities = abilities, .roster_count = 4};
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++)
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) player.squads[squad][slot] = -1;
    player.squads[0][SQUAD_SLOT_LARGE] = 0;
    player.squads[0][0] = 1;
    player.squads[0][2] = 2;
    player.squads[0][1] = 3;

    BattleRoster enemy = {0};
    BattleSim *setup = calloc(1, sizeof(*setup));
    assert(setup != NULL);
    BattleSimSetup(setup, &player, &enemy);

    assert(setup->units[SQUAD_SLOT_LARGE].max_health == 2300);
    assert(setup->units[0].max_health == 1600);
    assert(setup->units[2].max_health == 1600);

    player.squads[0][0] = player.squads[0][1] = player.squads[0][2] = -1;
    BattleSimSetup(setup, &player, &enemy);
    assert(setup->units[SQUAD_SLOT_LARGE].max_health == 1600);
    free(setup);
}

static void TestPowerVsTargetDamage(void) {
    PassiveManifestEnsure();
    static const struct {
        const char *unit_id;
        const char *group;
        const char *second_group;
        int value_pct;
    } cases[] = {
        {"UNIT_MALIPHANT", "Inquisition", NULL, 35},
        {"UNIT_NIGHTMAREASPECT", "Mage", "Spirit", 15},
        {"UNIT_THELOST", "Blight", NULL, 30},
    };

    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        BattleSim s = {.rng = (unsigned)(100 + i)};
        Character attacker, matching, plain;
        Ability abilities[3];
        TestCharacter(&attacker, &abilities[0], cases[i].unit_id, "test", TARGET_ONE, 100, 1000);
        TestCharacter(&matching, &abilities[1], "matching", "test", TARGET_ONE, 100, 1000);
        TestCharacter(&plain, &abilities[2], "plain", "test", TARGET_ONE, 100, 1000);
        TestAddGroup(&matching, cases[i].group);
        if (cases[i].second_group) TestAddGroup(&matching, cases[i].second_group);
        SimUnitInit(&s, 0, 0, &attacker);
        SimUnitInit(&s, 1, 0, &matching);
        SimUnitInit(&s, 1, 2, &plain);

        assert(
            fabsf(SimPassiveBonusDamagePct(&s, &s.units[0], &s.units[SQUAD_SLOT_COUNT]) - cases[i].value_pct) < 0.001f
        );
        assert(fabsf(SimPassiveBonusDamagePct(&s, &s.units[0], &s.units[SQUAD_SLOT_COUNT + 2])) < 0.001f);

        s.units[0].crit_pct = 0.0f;
        s.units[SQUAD_SLOT_COUNT + 2].alive = false;
        AbilityRecipe recipe = {.key = "test"};
        SimAttack(&s, 0, &recipe, &recipe.hit, true, false);
        int damage = (int)lroundf(100.0f * (1.0f + cases[i].value_pct / 100.0f));
        assert(s.units[0].start_power == 100 && s.units[0].power == 100);
        assert(s.units[SQUAD_SLOT_COUNT].health == 1000 - damage);
        assert(s.actions[0].targets[0].health_delta == -damage);
    }
}

static void TestStackingAuraMatchMode(void) {
    PassiveManifestEnsure();
    BattleSim s = {0};
    Character solas, triple_match, single_match;
    Ability abilities[3];
    TestCharacter(&solas, &abilities[0], "UNIT_APOSTATESOLAS", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&triple_match, &abilities[1], "triple", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&single_match, &abilities[2], "single", "test", TARGET_ONE, 100, 1000);
    TestAddGroup(&triple_match, "Spirit");
    TestAddGroup(&triple_match, "Elves");
    triple_match.faction = FACTION_BLUE;
    TestAddGroup(&single_match, "Spirit");
    SimUnitInit(&s, 0, 0, &solas);
    SimUnitInit(&s, 0, 2, &triple_match);
    SimUnitInit(&s, 0, 4, &single_match);

    SimApplyBattleStartPassives(&s);

    assert(s.units[0].max_health == 1000 && s.units[0].power == 100);
    assert(s.units[2].max_health == 1450 && s.units[2].power == 145);
    assert(s.units[4].max_health == 1150 && s.units[4].power == 115);
}

static void TestPassivePredicatesAndSignedEnemyAura(void) {
    PassiveManifestEnsure();
    Character target;
    Ability ability;
    TestCharacter(&target, &ability, "UNIT_TARGET", "test", TARGET_ONE, 100, 1000);
    TestAddGroup(&target, "Demon");
    TestAddSpecialty(&target, SPEC_POWERGAIN);
    target.faction = FACTION_BLUE_BLACK;
    target.rarity = RARITY_LEGENDARY;
    target.stars = 3;
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_GROUP, "Demon"}, &target));
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_FACTION, "Blue"}, &target));
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_CHARACTER_ID, "UNIT_TARGET"}, &target));
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_SPECIALTY, "POWERGAIN"}, &target));
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_RARITY, "Legendary"}, &target));
    target.speed = SPEED_QUICK;
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_SPEED, "Quick"}, &target));
    assert(!SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_SPEED, "Slow"}, &target));
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_TAG, "3STAR"}, &target));
    target.giant = true;
    assert(SimPassiveTermMatches(&(PassiveMatchTerm){PASSIVE_TERM_TAG, "GIANT"}, &target));
    target.giant = false;

    BattleSim s = {0};
    Character source;
    Ability source_ability;
    TestCharacter(&source, &source_ability, "source", "test", TARGET_ONE, 100, 1000);
    SimUnitInit(&s, 0, 0, &source);
    SimUnitInit(&s, 1, 0, &target);
    PassiveEffect effect = {
        .kind = PASSIVE_STAT_PERCENT,
        .trigger = PASSIVE_TRIGGER_BATTLE_START,
        .stats = PASSIVE_STAT_HEALTH | PASSIVE_STAT_POWER,
        .value_pct = -200.0f,
        .recipient = {.scope = PASSIVE_SCOPE_ENEMIES, .include_source = true},
        .additive = true,
    };
    PassiveCharacter passives = {.effects = &effect, .effect_count = 1};
    s.units[0].passives = &passives;

    SimApplyBattleStartPassives(&s);

    assert(s.units[SQUAD_SLOT_COUNT].max_health == 1);
    assert(s.units[SQUAD_SLOT_COUNT].health == 1);
    assert(s.units[SQUAD_SLOT_COUNT].power == 0);

    BattleSim reference = {0};
    Character dragon, demon, plain;
    Ability reference_abilities[3];
    TestCharacter(&dragon, &reference_abilities[0], "UNIT_ABYSSALHIGHDRAGON", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&demon, &reference_abilities[1], "demon", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&plain, &reference_abilities[2], "plain", "test", TARGET_ONE, 100, 1000);
    TestAddGroup(&demon, "Demon");
    SimUnitInit(&reference, 0, 0, &dragon);
    SimUnitInit(&reference, 1, 0, &demon);
    SimUnitInit(&reference, 1, 2, &plain);

    SimApplyBattleStartPassives(&reference);

    assert(reference.units[SQUAD_SLOT_COUNT].power == 85);
    assert(reference.units[SQUAD_SLOT_COUNT + 2].power == 100);

    BattleSim hivernal_battle = {0};
    Character hivernal, quick, normal;
    Ability speed_abilities[3];
    TestCharacter(&hivernal, &speed_abilities[0], "UNIT_HIVERNAL", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&quick, &speed_abilities[1], "quick", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&normal, &speed_abilities[2], "normal", "test", TARGET_ONE, 100, 1000);
    quick.speed = SPEED_QUICK;
    normal.speed = SPEED_NORMAL;
    SimUnitInit(&hivernal_battle, 0, 0, &hivernal);
    SimUnitInit(&hivernal_battle, 1, 0, &quick);
    SimUnitInit(&hivernal_battle, 1, 2, &normal);

    SimApplyBattleStartPassives(&hivernal_battle);

    assert(hivernal_battle.units[SQUAD_SLOT_COUNT].power == 85);
    assert(hivernal_battle.units[SQUAD_SLOT_COUNT + 2].power == 100);
}

static void TestAurasAddAcrossSources(void) {
    PassiveManifestEnsure();
    BattleSim s = {0};
    Character envy_a, envy_b, demon;
    Ability abilities[3];
    TestCharacter(&envy_a, &abilities[0], "UNIT_ENVYDEMON", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&envy_b, &abilities[1], "UNIT_ENVYDEMON", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&demon, &abilities[2], "demon", "test", TARGET_ONE, 100, 1000);
    TestAddGroup(&envy_a, "Demon");
    TestAddGroup(&envy_b, "Demon");
    TestAddGroup(&demon, "Demon");
    SimUnitInit(&s, 0, 0, &envy_a);
    SimUnitInit(&s, 0, 2, &envy_b);
    SimUnitInit(&s, 0, 4, &demon);

    SimApplyBattleStartPassives(&s);

    for (int i = 0; i <= 4; i += 2) {
        assert(s.units[i].max_health == 2000);
        assert(s.units[i].power == 200);
    }
}

static void TestPassivePrecedesFormationRounding(void) {
    Character characters[4];
    Ability abilities[4];
    for (int i = 0; i < 4; i++) {
        char id[32];
        snprintf(id, sizeof(id), "black-%d", i);
        TestCharacter(&characters[i], &abilities[i], id, "test", TARGET_ONE, 9, 1000);
        characters[i].faction = FACTION_BLACK;
        characters[i].rarity = RARITY_COMMON;
    }
    StrCopy(characters[0].id, sizeof(characters[0].id), "UNIT_ENVYDEMON");
    TestAddGroup(&characters[0], "Demon");

    Player player = {.roster = characters, .roster_abilities = abilities, .roster_count = 4};
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++)
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) player.squads[squad][slot] = -1;
    for (int slot = 0; slot < 4; slot++) player.squads[0][slot] = slot;
    BattleRoster enemy = {0};
    BattleSim s;

    BattleSimSetup(&s, &player, &enemy);

    assert(s.units[0].start_power == 15);
    assert(s.formation_bonus_pct[0] == 4.0f);
    assert(s.action_count == 0 && s.rounds == 0);
}

static void TestEnvyTargetWeights(void) {
    PassiveManifestEnsure();
    BattleSim s = {0};
    Character envy, attacker, high, low;
    Ability abilities[4];
    TestCharacter(&envy, &abilities[0], "UNIT_ENVYDEMON", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&attacker, &abilities[1], "attacker", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&high, &abilities[2], "high", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&low, &abilities[3], "low", "test", TARGET_ONE, 100, 1000);
    TestAddSpecialty(&envy, SPEC_ELUSIVE);
    SimUnitInit(&s, 0, 0, &envy);
    SimUnitInit(&s, 0, 2, &attacker);
    SimUnitInit(&s, 1, 0, &high);
    SimUnitInit(&s, 1, 2, &low);
    s.units[SQUAD_SLOT_COUNT + 0].health = 900;
    s.units[SQUAD_SLOT_COUNT + 2].health = 200;

    assert(fabsf(SimTargetWeight(&s, &s.units[2], &s.units[0]) - 0.25f) < 0.001f);
    assert(fabsf(SimTargetWeight(&s, &s.units[0], &s.units[SQUAD_SLOT_COUNT + 0]) - 1.0f) < 0.001f);
    assert(fabsf(SimTargetWeight(&s, &s.units[0], &s.units[SQUAD_SLOT_COUNT + 2]) - 2.0f) < 0.001f);
}

static void TestEnvyDamageAndEnemyDeathHeal(void) {
    PassiveManifestEnsure();
    BattleSim s = {.rng = 99};
    Character envy, power_gainer;
    Ability abilities[2];
    TestCharacter(&envy, &abilities[0], "UNIT_ENVYDEMON", "test", TARGET_ONE, 100, 1000);
    TestCharacter(&power_gainer, &abilities[1], "power-gainer", "test", TARGET_ONE, 100, 1000);
    TestAddSpecialty(&power_gainer, SPEC_POWERGAIN);
    SimUnitInit(&s, 0, 0, &envy);
    SimUnitInit(&s, 1, 0, &power_gainer);
    s.units[0].crit_pct = 0.0f;
    s.units[0].health = 500;
    s.units[SQUAD_SLOT_COUNT].health = 100;
    AbilityRecipe recipe = {.key = "test"};

    SimAttack(&s, 0, &recipe, &recipe.hit, true, false);

    assert(!s.units[SQUAD_SLOT_COUNT].alive);
    assert(s.units[0].health == 750);
    assert(s.action_count == 1 && s.actions[0].target_count == 2);
    assert(s.actions[0].targets[0].health_delta == -135);
    assert(s.actions[0].targets[1].unit == 0 && s.actions[0].targets[1].health_delta == 250);

    assert(!BpPassiveHealEntry(&s.actions[0].targets[0]));
    assert(BpPassiveHealEntry(&s.actions[0].targets[1]));
    int targets[SIM_MAX_TARGETS];
    int swing_count = BpCollectSwingTargets(&s, 0, targets);
    assert(swing_count == 1 && targets[0] == SQUAD_SLOT_COUNT);

    BattleSim h = {.action_count = 1};
    h.actions[0].actor = 0;
    h.actions[0].target_count = 2;
    h.actions[0].targets[0] = (BattleSimTarget){.unit = 1, .health_delta = 120};
    h.actions[0].targets[1] = (BattleSimTarget){.unit = 2, .health_delta = 80};
    assert(BpCollectSwingTargets(&h, 0, targets) == 2 && targets[0] == 1 && targets[1] == 2);
}

static void TestBattleClipAliases(void) {
    BattleSquads squads = {0};
    ModelAnimation clips[4] = {0};
    StrCopy(clips[0].name, sizeof(clips[0].name), "TwoHanded_GetHit");
    clips[0].frameCount = 20;
    StrCopy(clips[1].name, sizeof(clips[1].name), "ProjectileAbility");
    clips[1].frameCount = 30;
    StrCopy(clips[2].name, sizeof(clips[2].name), "hit");
    clips[2].frameCount = 10;
    StrCopy(clips[3].name, sizeof(clips[3].name), "death");
    clips[3].frameCount = 0;
    squads.clips[0][0] = clips;
    squads.clip_counts[0][0] = 4;

    assert(BattleSquadClip(&squads, 0, 0, "hit") == &clips[2]);
    clips[2].frameCount = 0;
    assert(BattleSquadClip(&squads, 0, 0, "hit") == &clips[0]);
    assert(BattleSquadClip(&squads, 0, 0, "attack") == &clips[1]);
    assert(BattleSquadClip(&squads, 0, 0, "death") == NULL);
}

static void TestTierlessWispModelAlias(void) {
    char path[512];
    assert(HomeSquadModelPath(path, sizeof(path), HOD_CACHE "char_pack", "unit.c_wisp_common_pre"));
    assert(strstr(path, "/c_wisp_common/c_wisp_common_t0_rl.glb") != NULL);
}

static void TestExactTierModelResolution(void) {
    Character ch = {.tier = TIER_I};
    StrCopy(ch.model_prefab[TIER_I], sizeof(ch.model_prefab[TIER_I]), "unit.missing_t1_pre");
    StrCopy(ch.model_prefab[TIER_IV], sizeof(ch.model_prefab[TIER_IV]), "unit.hm_sw_alistairking_t4_pre");
    char path[512];
    assert(HomeSquadResolvePrefab(HOD_CACHE "char_pack", &ch, path, sizeof(path)) == ch.model_prefab[TIER_IV]);
    assert(strstr(path, "hm_sw_alistairking_t4_rl.glb") != NULL);

    ch.tier = TIER_II;
    StrCopy(ch.model_prefab[TIER_I], sizeof(ch.model_prefab[TIER_I]), "unit.hm_sw_alistairking_t1_pre");
    StrCopy(ch.model_prefab[TIER_II], sizeof(ch.model_prefab[TIER_II]), "unit.missing_t2_pre");
    assert(HomeSquadResolvePrefab(HOD_CACHE "char_pack", &ch, path, sizeof(path)) == ch.model_prefab[TIER_I]);
    assert(strstr(path, "hm_sw_alistairking_t1_rl.glb") != NULL);
}

static void TestDisplayScaleManifestIsFullyLoaded(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "anim_tools/display_scales.json");
    assert(root);
    int expected = cJSON_GetArraySize(root);
    assert(fabsf(UnitDisplayScale("unit.c_titanheartguardian_t1_pre") - 0.65f) < 0.0001f);
    assert(unit_scale_count == expected);
    cJSON_Delete(root);
}

static void TestDamageShakeFallback(void) {
    BpUnit unit = {.damage_shake_left = BP_DAMAGE_SHAKE_TIME};
    Vector3 start = BpDamageShakeOffset(&unit, 0);
    assert(fabsf(start.x) <= 0.25f && fabsf(start.z) <= 1.0f);
    assert(fabsf(start.x) + fabsf(start.z) > 0.01f);

    unit.damage_shake_left = BP_DAMAGE_SHAKE_TIME * 0.5f;
    Vector3 half = BpDamageShakeOffset(&unit, 0);
    assert(fabsf(half.x) <= 0.1251f && fabsf(half.z) <= 0.5001f);

    unit.damage_shake_left = 0.0f;
    Vector3 done = BpDamageShakeOffset(&unit, 0);
    assert(done.x == 0.0f && done.y == 0.0f && done.z == 0.0f);
}

static void TestAttackClipReturnsToIdle(void) {
    ModelAnimation attack = {.frameCount = 12};
    BpUnit windup = {
        .phase = BPU_WINDUP,
        .clip = &attack,
        .clip_time = 10.0f,
        .clip_speed = 1.0f,
    };
    BpAdvanceClip(&windup, 1.0f / BP_CLIP_FPS);
    assert(windup.clip == NULL);

    BpUnit returning = windup;
    returning.phase = BPU_RETURN;
    returning.clip = &attack;
    returning.clip_time = 10.0f;
    BpAdvanceClip(&returning, 1.0f / BP_CLIP_FPS);
    assert(returning.clip == NULL);

    BpUnit death = {
        .phase = BPU_RETURN,
        .clip = &attack,
        .clip_time = 10.0f,
        .clip_speed = 1.0f,
        .clip_hold = true,
    };
    BpAdvanceClip(&death, 1.0f / BP_CLIP_FPS);
    assert(death.clip == &attack);
    assert(death.clip_time == 11.0f);

    HomeSquadUnit rendered = {.anim_override = true, .last_frame = 11};
    BpSetAnimationOverride(&rendered, false);
    assert(!rendered.anim_override && rendered.last_frame == -1);
}

static Campaign TestProgressionCampaign(void) {
    Campaign campaign = {.map_count = 2};
    campaign.maps[0].node_count = 4;
    campaign.maps[0].nodes[0] =
        (CampaignNode){.gold = 55, .xp = 5, .first_win_gems = 3, .mastery_gems = 2, .wins_to_master = 3};
    campaign.maps[0].nodes[1] =
        (CampaignNode){.gold = 80, .xp = 100, .first_win_gems = 4, .mastery_gems = 1, .wins_to_master = 5};
    campaign.maps[0].nodes[2] =
        (CampaignNode){.gold = 140, .xp = 20, .first_win_gems = 10, .mastery_gems = 1, .wins_to_master = 7};
    campaign.maps[0].nodes[3] = (CampaignNode){
        .gold = 280,
        .xp = 40,
        .challenge_gold = 17,
        .challenge_xp = 9,
        .challenge_gems = 2,
        .grind_gold = 5,
        .grind_xp = 3,
        .grind_gems = 1,
        .grind = true,
    };
    campaign.maps[1].node_count = 3;
    campaign.maps[1].nodes[0] = (CampaignNode){.gold = 110, .xp = 15, .mastery_gems = 1, .wins_to_master = 3};
    campaign.maps[1].nodes[1] =
        (CampaignNode){.gold = 280, .xp = 40, .first_win_gems = 10, .mastery_gems = 1, .wins_to_master = 5};
    campaign.maps[1].nodes[2] = (CampaignNode){.gold = 560, .xp = 80, .grind = true};
    return campaign;
}

static void TestCriticalGoldCommitOnce(void) {
    static Game g;
    memset(&g, 0, sizeof(g));
    g.map = TestProgressionCampaign();
    ProgressionInitializePlayerDefaults(&g.player);
    g.battle_map_index = 0;
    g.battle_node_index = 0;
    g.battle_play.sim.action_count = 1;
    g.battle_play.sim.actions[0] = (BattleSimAction){
        .actor = 0,
        .crit = true,
        .replay_attack = true,
        .target_count = 1,
        .targets = {{.unit = SQUAD_SLOT_COUNT, .health_delta = -10}},
    };

    char previous[32] = {0};
    const char *old_no_save = getenv("HOD_NO_SAVE");
    if (old_no_save) StrCopy(previous, sizeof(previous), old_no_save);
    setenv("HOD_NO_SAVE", "1", 1);

    int gold_before = g.player.gold;
    GameCommitBattleOutcome(&g, true);
    assert(g.battle_outcome_committed);
    assert(g.battle_reward.crit_gold_reward == BP_CRIT_GOLD_REWARD);
    assert(g.battle_reward.gold_reward == 55 + BP_CRIT_GOLD_REWARD);
    assert(g.player.gold == gold_before + 55 + BP_CRIT_GOLD_REWARD);
    BattleResultsPresentation snapshot = GameBattleResultsSnapshot(&g);
    assert(snapshot.gold == 55 + BP_CRIT_GOLD_REWARD);

    int committed_gold = g.player.gold;
    GameCommitBattleOutcome(&g, true);
    assert(g.player.gold == committed_gold);
    assert(g.battle_reward.gold_reward == 55 + BP_CRIT_GOLD_REWARD);

    memset(&g, 0, sizeof(g));
    g.map = TestProgressionCampaign();
    ProgressionInitializePlayerDefaults(&g.player);
    g.battle_map_index = 0;
    g.battle_node_index = 0;
    g.battle_play.sim.action_count = 1;
    g.battle_play.sim.actions[0] = (BattleSimAction){
        .actor = 0,
        .crit = true,
        .replay_attack = true,
        .target_count = 1,
        .targets = {{.unit = SQUAD_SLOT_COUNT, .health_delta = -10}},
    };
    assert(BattlePlayCriticalGold(&g.battle_play) == BP_CRIT_GOLD_REWARD);
    gold_before = g.player.gold;
    GameCommitBattleOutcome(&g, false);
    assert(g.battle_reward.resolved);
    assert(g.battle_reward.crit_gold_reward == 0);
    assert(g.battle_reward.gold_reward == 0);
    assert(g.player.gold == gold_before);

    if (old_no_save)
        setenv("HOD_NO_SAVE", previous, 1);
    else
        unsetenv("HOD_NO_SAVE");
}

static void TestCampaignOpenerCanBeWon(void) {
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");

    BattleRoster enemy = {0};
    BattleRosterLoad(&enemy, "data/campaign_rosters.json", HOD_CACHE "characters.json", "PVE_MAP_01", 1);
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        if (enemy.present[slot]) assert(enemy.chars[slot].level <= 2);
    }

    BattleSim *sim = (BattleSim *)calloc(1, sizeof(*sim));
    assert(sim != NULL);
    int wins = 0;
    for (unsigned seed = 1; seed <= 32; seed++) {
        BattleSimRun(sim, &player, &enemy, seed);
        wins += sim->player_won;
    }
    assert(wins > 0);

    free(sim);
    PlayerUnloadRoster(&player);
}

static void
TestCatalogSquad(BattleRoster *r, const cJSON *catalog, const char *const ids[SQUAD_SLOT_COUNT], int level) {
    memset(r, 0, sizeof(*r));
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        cJSON *def = CatalogFindCharacter(catalog, ids[slot]);
        assert(def != NULL);
        CharacterParse(def, &r->chars[slot], &r->abilities[slot]);
        r->chars[slot].tier = TIER_I;
        assert(level <= r->chars[slot].stats[TIER_I].max_level);
        r->chars[slot].level = level;
        r->present[slot] = true;
    }
}

static int TestCampaignNodeWins(BattleSim *sim, const BattleRoster *squad, const char *map_id, int step, int seeds) {
    BattleRoster enemy = {0};
    BattleRosterLoad(&enemy, "data/campaign_rosters.json", HOD_CACHE "characters.json", map_id, step);
    int wins = 0;
    for (unsigned seed = 1; seed <= (unsigned)seeds; seed++) {
        BattleSimRunRosters(sim, squad, &enemy, seed);
        wins += sim->player_won;
    }
    return wins;
}

static void TestZathriansCurseBossFitsAPostTutorialSquad(void) {
    static const char *const post_tutorial_squad[SQUAD_SLOT_COUNT] = {
        "UNIT_GREYWARDENMAGE",
        "UNIT_GREYWARDENWARRIOR",
        "UNIT_ASHAAD",
        "UNIT_FERELDANASHWARRIOR",
        "UNIT_BEAR",
    };
    enum { SEEDS = 32, CURSED_RUINS = 4, LADY_OF_THE_FOREST = 5 };
    cJSON *catalog_root = JsonLoadFile(HOD_CACHE "characters.json");
    assert(catalog_root != NULL);
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    BattleSim *sim = (BattleSim *)calloc(1, sizeof(*sim));
    assert(sim != NULL);

    BattleRoster squad;
    TestCatalogSquad(&squad, catalog, post_tutorial_squad, 10);
    assert(TestCampaignNodeWins(sim, &squad, "PVE_MAP_02", CURSED_RUINS, SEEDS) == SEEDS);
    int boss_wins_at_10 = TestCampaignNodeWins(sim, &squad, "PVE_MAP_02", LADY_OF_THE_FOREST, SEEDS);
    assert(boss_wins_at_10 >= SEEDS / 4 && boss_wins_at_10 < SEEDS);

    TestCatalogSquad(&squad, catalog, post_tutorial_squad, 15);
    assert(TestCampaignNodeWins(sim, &squad, "PVE_MAP_02", LADY_OF_THE_FOREST, SEEDS) >= SEEDS * 3 / 4);

    free(sim);
    cJSON_Delete(catalog_root);
}

static void TestCampaignNodeCoordinates(void) {
    const Vector2 centre = {512.0f, 300.0f};
    const float world_w = 2880.0f;
    const float screen_h = 720.0f;
    cJSON *maps = JsonLoadFile("data/campaign_maps.json");
    assert(cJSON_IsArray(maps));
    cJSON *chapter = NULL;
    cJSON_ArrayForEach(chapter, maps) {
        const char *id = JsonStr(chapter, "id");
        CampaignNodeLayout layout =
            CampaignReadNodeLayout(cJSON_GetObjectItemCaseSensitive(chapter, "node_layout"), id);
        Rectangle icon = CampaignLayoutRect(layout.icon, centre, world_w, screen_h);
        assert(fabsf(icon.x + icon.width / 2.0f - centre.x) < 0.5f);
        assert(icon.y < centre.y && icon.y + icon.height > centre.y);
        Rectangle banner = CampaignLayoutRect(layout.banner, centre, world_w, screen_h);
        assert(banner.y > centre.y);
        cJSON *node = NULL;
        cJSON_ArrayForEach(node, cJSON_GetObjectItemCaseSensitive(chapter, "nodes")) {
            cJSON *position = cJSON_GetObjectItemCaseSensitive(node, "position");
            assert(cJSON_IsObject(position));
            float x = JsonFloat(position, "x"), y = JsonFloat(position, "y");
            assert(x > 0.0f && x < 1.0f && y > 0.0f && y < 1.0f);
        }
    }
    cJSON_Delete(maps);

    Campaign same_map_pan = {.progression_from_map = 0, .progression_to_map = 0};
    Campaign map_change_pan = {.progression_from_map = 0, .progression_to_map = 1};
    assert(CampaignProgressionPanEnd(&same_map_pan) == 2.0f);
    assert(CampaignProgressionPanEnd(&map_change_pan) == 3.5f);
}

static void TestCampaignMapRewardSchema(void) {
    cJSON *maps = JsonLoadFile("data/campaign_maps.json");
    assert(cJSON_IsArray(maps));
    assert(cJSON_GetArraySize(maps) == CAMPAIGN_MAX_MAPS);
    for (int map = 0; map < cJSON_GetArraySize(maps); map++) {
        cJSON *chapter = cJSON_GetArrayItem(maps, map);
        cJSON *nodes = cJSON_GetObjectItemCaseSensitive(chapter, "nodes");
        int node_count = cJSON_GetArraySize(nodes);
        assert(node_count == JsonInt(chapter, "node_count"));
        cJSON *last = cJSON_GetArrayItem(nodes, node_count - 1);
        cJSON *boss = cJSON_GetArrayItem(nodes, node_count - (map == 0 ? 1 : 2));
        assert(JsonInt(boss, "gems") == 10);
        if (map == 0) {
            assert(!JsonBool(last, "grind"));
        } else {
            assert(JsonBool(last, "grind"));
            assert(JsonInt(last, "gems") == 0);
        }
        int story_count = node_count - (JsonBool(last, "grind") ? 1 : 0);
        for (int node = 0; node < story_count; node++) {
            int override = JsonInt(cJSON_GetArrayItem(nodes, node), "wins_to_master");
            int wins_to_master = override > 0 ? override : CampaignOfflineWinsToMaster(node);
            static const int expected[] = {3, 5, 7, 10, 12};
            assert(wins_to_master == expected[node % 5]);
        }
    }
    cJSON_Delete(maps);
}

static void TestCampaignAndNexusTextResolves(void) {
    if (loc.count == 0) LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    static const char *const node_fields[] = {"NAME", "DESCRIPTION", "CTA"};
    cJSON *maps = JsonLoadFile("data/campaign_maps.json");
    cJSON *chapter = NULL;
    cJSON_ArrayForEach(chapter, maps) {
        const char *id = JsonStr(chapter, "id");
        assert(LocGet(TextFormat(CAMPAIGN_HEADER_KEY_FORMAT, id))[0] != '\0');
        cJSON *node = NULL;
        cJSON_ArrayForEach(node, cJSON_GetObjectItemCaseSensitive(chapter, "nodes")) {
            for (size_t field = 0; field < sizeof(node_fields) / sizeof(node_fields[0]); field++)
                assert(LocGet(TextFormat(CAMPAIGN_NODE_KEY_FORMAT, id, JsonInt(node, "step"), node_fields[field]))[0]);
        }
    }
    cJSON_Delete(maps);
    cJSON *events = JsonLoadFile("data/nexus_maps.json");
    cJSON *event = NULL;
    cJSON_ArrayForEach(event, events) assert(LocGet(JsonStr(event, "name_key"))[0] != '\0');
    cJSON_Delete(events);
    CampaignNode node = {.wins_to_master = 12};
    char label[64];
    CampaignNodeMasteryLabel(&node, 11, label, sizeof(label));
    assert(strcmp(label, "Закрыто (11 / 12)") == 0);
}

static void TestCampaignProgressionRewards(void) {
    Campaign campaign = TestProgressionCampaign();
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);
    player.energy = 2;
    player.stamina = 3;

    const int historical_mastery[] = {3, 5, 7, 10, 12, 3, 5, 7, 10, 12};
    for (int node = 0; node < 10; node++) assert(CampaignOfflineWinsToMaster(node) == historical_mastery[node]);

    assert(ProgressionXpForNextLevel(1) == 20);
    assert(ProgressionXpForCurrentLevel(4) == 90);
    assert(ProgressionXpForNextLevel(4) == 140);
    assert(ProgressionEnergyMaxForLevel(1) == 12);
    assert(ProgressionEnergyMaxForLevel(4) == 15);
    Player max_level_player = {0};
    ProgressionInitializePlayerDefaults(&max_level_player);
    max_level_player.level = 100;
    max_level_player.experience_min = ProgressionXpForCurrentLevel(max_level_player.level);
    max_level_player.experience_max = ProgressionXpForNextLevel(max_level_player.level);
    max_level_player.experience = max_level_player.experience_max;
    assert(max_level_player.experience_min == max_level_player.experience_max);
    assert(HudDisplayedXpMinimum(&max_level_player) == 0);
    assert(CampaignMapUnlocked(&campaign, &player.campaign_progress, 0));
    assert(!CampaignMapUnlocked(&campaign, &player.campaign_progress, 1));
    assert(CampaignFrontierForMap(&campaign, &player.campaign_progress, 0) == 0);
    assert(CampaignBossNodeIndex(&campaign, 0) == 2);
    assert(CampaignBossNodeIndex(&campaign, 1) == 1);
    Campaign no_grind = {.map_count = 1};
    no_grind.maps[0].node_count = 5;
    assert(CampaignBossNodeIndex(&no_grind, 0) == 4);

    BattleRewardResult loss = CampaignResolveBattle(&player, &campaign, 0, 0, false, 0);
    assert(loss.resolved && !loss.won && !loss.first_win);
    assert(loss.gold_reward == 0 && loss.xp_reward == 0 && loss.gems_reward == 0);
    assert(player.gold == 1725 && player.experience == 0);
    assert(player.campaign_progress.node_completions[0][0] == 0);

    BattleRewardResult locked = CampaignResolveBattle(&player, &campaign, 0, 1, true, 0);
    assert(!locked.resolved);
    assert(player.campaign_progress.node_completions[0][1] == 0);

    BattleRewardResult first = CampaignResolveBattle(&player, &campaign, 0, 0, true, 0);
    assert(first.resolved && first.won && first.first_win && first.advanced && !first.unlocked_map);
    assert(first.gold_reward == 55 && first.xp_reward == 5);
    assert(first.first_win_gems_reward == 3 && first.mastery_gems_reward == 0 && first.gems_reward == 3);
    assert(first.next_map_index == 0 && first.next_node_index == 1);
    PlayerCampaignAnimation first_animation = GamePendingCampaignAnimation(&campaign, &first);
    assert(first_animation.active && first_animation.unlock_next);
    assert(first_animation.from_map == 0 && first_animation.from_node == 0);
    assert(first_animation.to_map == 0 && first_animation.to_node == 1);
    assert(player.campaign_progress.node_index == 1);
    assert(player.gold == 1780 && player.gems == 44 && player.experience == 5);

    BattleRewardResult replay = CampaignResolveBattle(&player, &campaign, 0, 0, true, 0);
    assert(replay.resolved && replay.won && !replay.first_win && !replay.advanced);
    assert(replay.gold_reward == 55 && replay.xp_reward == 5 && replay.gems_reward == 0);
    assert(replay.next_node_index == 1);
    assert(!GamePendingCampaignAnimation(&campaign, &replay).active);
    assert(player.campaign_progress.node_completions[0][0] == 2);
    assert(player.gold == 1835 && player.gems == 44 && player.experience == 10);

    BattleRewardResult levels = CampaignResolveBattle(&player, &campaign, 0, 1, true, 0);
    assert(levels.first_win && levels.levels_gained == 3);
    assert(levels.player_level_pre == 1 && levels.player_level_post == 4);
    assert(levels.player_xp_pre == 10 && levels.player_xp_post == 110);
    assert(levels.player_xp_min_post == 90 && levels.player_xp_max_post == 140);
    assert(levels.energy_max_pre == 12 && levels.energy_max_post == 15);
    assert(levels.energy_post == 15 && levels.stamina_post == 8);
    assert(levels.first_win_gems_reward == 4 && levels.gems_reward == 4);
    assert(player.gems == 48);
    assert(player.campaign_progress.node_index == 2);

    BattleRewardResult boss = CampaignResolveBattle(&player, &campaign, 0, 2, true, 0);
    assert(boss.first_win && boss.advanced && boss.unlocked_map);
    assert(boss.first_win_gems_reward == 10 && boss.mastery_gems_reward == 0 && boss.gems_reward == 10);
    assert(boss.next_map_index == 0 && boss.next_node_index == 3);
    PlayerCampaignAnimation boss_animation = GamePendingCampaignAnimation(&campaign, &boss);
    assert(boss_animation.active && boss_animation.unlock_next && boss_animation.from_node == 2);
    assert(boss_animation.to_map == 0 && boss_animation.to_node == 3);
    assert(CampaignMapUnlocked(&campaign, &player.campaign_progress, 1));
    assert(CampaignFrontierForMap(&campaign, &player.campaign_progress, 0) == 3);
    assert(player.campaign_progress.campaign_index == 0);

    BattleRewardResult boss_replay = CampaignResolveBattle(&player, &campaign, 0, 2, true, 0);
    assert(!boss_replay.first_win && !boss_replay.advanced && boss_replay.gems_reward == 0);
    assert(boss_replay.next_map_index == 0 && boss_replay.next_node_index == 3);

    BattleRewardResult grind = CampaignResolveBattle(&player, &campaign, 0, 3, true, 0);
    assert(grind.resolved && grind.won && grind.first_win && grind.grind);
    assert(!grind.mastered && grind.advanced && !grind.unlocked_map);
    assert(grind.wins_to_master == 0 && grind.gold_reward == 297 && grind.xp_reward == 49);
    assert(grind.challenge_gold_reward == 17 && grind.challenge_xp_reward == 9);
    assert(grind.challenge_gems_reward == 2 && grind.gems_reward == 2);
    assert(grind.next_map_index == 1 && grind.next_node_index == 0);
    player.campaign_progress.node_completions[0][3] = 99;
    player.campaign_progress.campaign_index = 0;
    BattleRewardResult grind_replay = CampaignResolveBattle(&player, &campaign, 0, 3, true, 0);
    assert(grind_replay.resolved && !grind_replay.mastered);
    assert(grind_replay.gold_reward == 285 && grind_replay.xp_reward == 43 && grind_replay.gems_reward == 1);
    assert(grind_replay.grind_gold_reward == 5 && grind_replay.grind_xp_reward == 3);
    assert(grind_replay.grind_gems_reward == 1);
    assert(!grind_replay.advanced && grind_replay.next_map_index == 0 && grind_replay.next_node_index == 3);

    Player mastery_player = {0};
    ProgressionInitializePlayerDefaults(&mastery_player);
    mastery_player.campaign_progress.node_completions[0][0] = campaign.maps[0].nodes[0].wins_to_master - 1;
    BattleRewardResult mastery = CampaignResolveBattle(&mastery_player, &campaign, 0, 0, true, 0);
    assert(mastery.resolved && mastery.mastered && !mastery.first_win && !mastery.advanced);
    assert(mastery.completions_post == campaign.maps[0].nodes[0].wins_to_master);
    assert(mastery.first_win_gems_reward == 0 && mastery.mastery_gems_reward == 2);
    assert(mastery.gems_reward == 2 && mastery_player.gems == 43);
    PlayerCampaignAnimation mastery_animation = GamePendingCampaignAnimation(&campaign, &mastery);
    assert(mastery_animation.active && !mastery_animation.unlock_next);
    assert(mastery_animation.from_node == 0 && mastery_animation.to_node == 1);

    Campaign final_mastery_campaign = {.map_count = 2};
    final_mastery_campaign.maps[0].node_count = 1;
    final_mastery_campaign.maps[0].nodes[0] = (CampaignNode){.gold = 10, .mastery_gems = 1, .wins_to_master = 3};
    final_mastery_campaign.maps[1].node_count = 1;
    final_mastery_campaign.maps[1].nodes[0] = (CampaignNode){.gold = 20, .wins_to_master = 3};
    Player final_mastery_player = {0};
    ProgressionInitializePlayerDefaults(&final_mastery_player);
    final_mastery_player.campaign_progress.node_completions[0][0] = 2;
    BattleRewardResult final_mastery =
        CampaignResolveBattle(&final_mastery_player, &final_mastery_campaign, 0, 0, true, 0);
    assert(final_mastery.mastered && !final_mastery.first_win);
    assert(!final_mastery.advanced && !final_mastery.unlocked_map);
    assert(final_mastery.next_map_index == 1 && final_mastery.next_node_index == 0);
    assert(final_mastery_player.campaign_progress.campaign_index == 1);
    assert(final_mastery_player.campaign_progress.node_index == 0);
    assert(!GamePendingCampaignAnimation(&final_mastery_campaign, &final_mastery).active);

    CampaignNode gem_node = {.default_gems = 2, .first_win_gems = 10, .mastery_gems = 1, .wins_to_master = 12};
    BattleUi preview = {.node = &gem_node};
    BattleUiSetNodeWins(&preview, 0);
    assert(preview.gems == 12);
    BattleUiSetNodeWins(&preview, 1);
    assert(preview.gems == 2);
    BattleUiSetNodeWins(&preview, 11);
    assert(preview.gems == 3);
    assert(strcmp(preview.mastery, "Закрыто (11 / 12)") == 0);
    BattleUiSetNodeWins(&preview, 12);
    assert(preview.gems == 2);
    CampaignNode grind_node = {
        .gold = 10,
        .xp = 4,
        .default_gems = 3,
        .challenge_gold = 5,
        .challenge_xp = 3,
        .challenge_gems = 2,
        .grind_gold = 7,
        .grind_xp = 6,
        .grind_gems = 1,
        .grind = true,
    };
    preview = (BattleUi){.node = &grind_node};
    BattleUiSetNodeWins(&preview, 0);
    assert(preview.gold == 15 && preview.xp == 7 && preview.gems == 5 && preview.mastery[0] == '\0');
    BattleUiSetNodeWins(&preview, 1);
    assert(preview.gold == 17 && preview.xp == 10 && preview.gems == 4 && preview.mastery[0] == '\0');

    Campaign bucket_campaign = {.map_count = 1};
    bucket_campaign.maps[0].node_count = 1;
    bucket_campaign.maps[0].nodes[0] = (CampaignNode){
        .gold = 10,
        .xp = 4,
        .default_gems = 3,
        .first_win_gold = 5,
        .first_win_xp = 3,
        .mastery_gold = 7,
        .mastery_xp = 6,
        .first_win_gems = 2,
        .mastery_gems = 1,
        .wins_to_master = 3,
    };
    Player bucket_player = {0};
    ProgressionInitializePlayerDefaults(&bucket_player);
    BattleRewardResult bucket_first = CampaignResolveBattle(&bucket_player, &bucket_campaign, 0, 0, true, 0);
    assert(bucket_first.gold_reward == 15 && bucket_first.xp_reward == 7 && bucket_first.gems_reward == 5);
    assert(bucket_first.default_gold_reward == 10);
    assert(bucket_first.default_gems_reward == 3);
    assert(bucket_first.first_win_gold_reward == 5 && bucket_first.first_win_xp_reward == 3);
    BattleRewardResult bucket_replay = CampaignResolveBattle(&bucket_player, &bucket_campaign, 0, 0, true, 0);
    assert(bucket_replay.gold_reward == 10 && bucket_replay.xp_reward == 4 && bucket_replay.gems_reward == 3);

    Player bucket_mastery_player = {0};
    ProgressionInitializePlayerDefaults(&bucket_mastery_player);
    bucket_mastery_player.campaign_progress.node_completions[0][0] = 2;
    BattleRewardResult bucket_mastery = CampaignResolveBattle(&bucket_mastery_player, &bucket_campaign, 0, 0, true, 0);
    assert(bucket_mastery.mastered);
    assert(bucket_mastery.gold_reward == 17 && bucket_mastery.xp_reward == 10 && bucket_mastery.gems_reward == 4);
    assert(bucket_mastery.mastery_gold_reward == 7 && bucket_mastery.mastery_xp_reward == 6);

    BattleUi bucket_preview = {.node = &bucket_campaign.maps[0].nodes[0]};
    BattleUiSetNodeWins(&bucket_preview, 0);
    assert(bucket_preview.gold == 15 && bucket_preview.xp == 7 && bucket_preview.gems == 5);
    BattleUiSetNodeWins(&bucket_preview, 1);
    assert(bucket_preview.gold == 10 && bucket_preview.xp == 4 && bucket_preview.gems == 3);
    BattleUiSetNodeWins(&bucket_preview, 2);
    assert(bucket_preview.gold == 17 && bucket_preview.xp == 10 && bucket_preview.gems == 4);

    Campaign one_win = {.map_count = 1};
    one_win.maps[0].node_count = 1;
    one_win.maps[0].nodes[0] = (CampaignNode){
        .gold = 10,
        .first_win_gold = 5,
        .mastery_gold = 7,
        .first_win_gems = 2,
        .mastery_gems = 1,
        .wins_to_master = 1,
    };
    Player one_win_player = {0};
    ProgressionInitializePlayerDefaults(&one_win_player);
    BattleRewardResult one_win_result = CampaignResolveBattle(&one_win_player, &one_win, 0, 0, true, 0);
    assert(one_win_result.first_win && !one_win_result.mastered);
    assert(one_win_result.gold_reward == 15 && one_win_result.gems_reward == 2);
    assert(one_win_result.mastery_gold_reward == 0 && one_win_result.mastery_gems_reward == 0);

    player.campaign_progress.campaign_index = 15;
    player.campaign_progress.node_index = 11;
    CampaignProgressSync(&campaign, &player.campaign_progress);
    assert(player.campaign_progress.campaign_index == 1);
    assert(player.campaign_progress.node_index == 0);
}

static void TestProgressionPersistence(void) {
    const char *path = "tests/progression_save_test.dat";
    const char *temporary = "tests/progression_save_test.dat.tmp";
    remove(path);
    remove(temporary);

    Player source = {0};
    ProgressionInitializePlayerDefaults(&source);
    source.level = 4;
    source.experience = 109;
    source.experience_min = ProgressionXpForCurrentLevel(source.level);
    source.experience_max = ProgressionXpForNextLevel(source.level);
    source.energy_max = ProgressionEnergyMaxForLevel(source.level);
    source.energy = 7;
    source.energy_regen_timer = 123.5f;
    source.stamina = 5;
    source.stamina_regen_timer = 42.25f;
    source.gold = 9876;
    source.gems = 77;
    source.store_rng_state = 0x1234abcdu;
    source.store_opened_total = 10;
    source.store_opened_by_pack[0] = 1;
    source.store_opened_by_pack[1] = 2;
    source.store_opened_by_pack[2] = 3;
    source.store_opened_by_pack[3] = 4;
    source.champion_since_legendary = 2;
    source.campaign_progress.campaign_index = 1;
    source.campaign_progress.node_index = 1;
    source.campaign_progress.node_completions[0][0] = 3;
    source.campaign_progress.node_completions[0][2] = 1;
    source.campaign_progress.node_completions[1][0] = 2;
    source.campaign_progress.node_losses[1][2] = 4;
    source.campaign_progress.pending_animation = (PlayerCampaignAnimation){
        .active = true,
        .unlock_next = false,
        .from_map = 0,
        .from_node = 0,
        .to_map = 0,
        .to_node = 1,
    };
    assert(ProgressionSaveToPath(&source, path));
    assert(FileExists(path) && !FileExists(temporary));

    char *json = LoadFileText(path);
    assert(json && strstr(json, "\"version\"") && strstr(json, "\"completions\""));
    assert(strstr(json, "\"roster\"") && strstr(json, "\"store\""));
    UnloadFileText(json);

    Character sentinel_roster = {0};
    Player loaded = {.roster = &sentinel_roster, .roster_count = 1};
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(loaded.roster == &sentinel_roster && loaded.roster_count == 1);
    assert(loaded.level == 4 && loaded.experience == 109);
    assert(loaded.experience_min == 90 && loaded.experience_max == 140);
    assert(loaded.energy == 7 && loaded.energy_max == 15);
    assert(fabsf(loaded.energy_regen_timer - 123.5f) < 0.001f);
    assert(loaded.stamina == 5 && fabsf(loaded.stamina_regen_timer - 42.25f) < 0.001f);
    assert(loaded.gold == 9876 && loaded.gems == 77);
    assert(loaded.store_rng_state == 0x1234abcdu && loaded.store_opened_total == 10);
    assert(loaded.store_opened_by_pack[0] == 1 && loaded.store_opened_by_pack[3] == 4);
    assert(loaded.champion_since_legendary == 2);
    assert(loaded.campaign_progress.node_completions[0][0] == 3);
    assert(loaded.campaign_progress.node_completions[1][0] == 2);
    assert(loaded.campaign_progress.node_losses[1][2] == 4);
    assert(loaded.campaign_progress.pending_animation.active);
    assert(!loaded.campaign_progress.pending_animation.unlock_next);
    assert(loaded.campaign_progress.pending_animation.from_map == 0);
    assert(loaded.campaign_progress.pending_animation.from_node == 0);
    assert(loaded.campaign_progress.pending_animation.to_map == 0);
    assert(loaded.campaign_progress.pending_animation.to_node == 1);

    json = LoadFileText(path);
    cJSON *legacy_root = cJSON_Parse(json);
    UnloadFileText(json);
    cJSON_ReplaceItemInObjectCaseSensitive(legacy_root, "version", cJSON_CreateNumber(2));
    cJSON *legacy_campaign = cJSON_GetObjectItemCaseSensitive(legacy_root, "campaign");
    cJSON_DeleteItemFromObjectCaseSensitive(legacy_campaign, "pending_animation");
    cJSON_DeleteItemFromObjectCaseSensitive(legacy_root, "store");
    cJSON_DeleteItemFromObjectCaseSensitive(legacy_root, "roster");
    cJSON_DeleteItemFromObjectCaseSensitive(legacy_root, "squads");
    cJSON_DeleteItemFromObjectCaseSensitive(legacy_root, "current_squad");
    char *legacy_json = cJSON_Print(legacy_root);
    cJSON_Delete(legacy_root);
    assert(legacy_json && SaveFileText(path, legacy_json));
    cJSON_free(legacy_json);
    Player legacy_loaded = {0};
    ProgressionInitializePlayerDefaults(&legacy_loaded);
    uint32_t legacy_rng = legacy_loaded.store_rng_state;
    assert(ProgressionLoadFromPath(&legacy_loaded, path));
    assert(legacy_loaded.level == 4 && legacy_loaded.campaign_progress.node_completions[0][0] == 3);
    assert(!legacy_loaded.campaign_progress.pending_animation.active);
    assert(legacy_loaded.store_rng_state == legacy_rng && legacy_loaded.store_opened_total == 0);

    char corrupt[] = "{\"version\":999,\"account\":{},\"campaign\":{}}";
    assert(SaveFileText(path, corrupt));
    Player before = loaded;
    assert(!ProgressionLoadFromPath(&loaded, path));
    assert(memcmp(&before, &loaded, sizeof(loaded)) == 0);

    remove(path);
    remove(temporary);
}

static void TestRosterInstancesAndPersistence(void) {
    const char *path = "tests/roster_save_test.dat";
    const char *temporary = "tests/roster_save_test.dat.tmp";
    remove(path);
    remove(temporary);

    Player source = {0};
    ProgressionInitializePlayerDefaults(&source);
    PlayerLoadRoster(&source, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    assert(source.roster_capacity == PLAYER_ROSTER_CAPACITY);
    assert(source.starter_roster_count == source.roster_count);
    assert(source.roster_capacity >= source.starter_roster_count + 100);
    for (int i = 0; i < source.roster_count; i++) {
        assert(source.roster[i].instance_id == (uint64_t)i + 1);
        assert(source.roster[i].ability == &source.roster_abilities[i]);
    }

    cJSON *catalog_root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    cJSON *def = CatalogFindCharacter(catalog, "UNIT_ELVENMAGE");
    assert(def);
    int definition_count = PlayerRosterDefinitionCount(&source, "UNIT_ELVENMAGE");
    int initial_count = source.roster_count;
    uint64_t initial_next = source.next_instance_id;
    const cJSON *defs[2] = {def, def};
    int first = -1;
    assert(PlayerRosterHasCapacity(&source, 2));
    assert(PlayerRosterAppendDefinitions(&source, defs, 2, &first));
    assert(first == initial_count && source.roster_count == initial_count + 2);
    assert(source.next_instance_id == initial_next + 2);
    assert(source.roster[first].instance_id == initial_next);
    assert(source.roster[first + 1].instance_id == initial_next + 1);
    assert(source.roster[first].instance_id != source.roster[first + 1].instance_id);
    assert(source.roster[first].ability == &source.roster_abilities[first]);
    assert(PlayerRosterDefinitionCount(&source, "UNIT_ELVENMAGE") == definition_count + 2);

    cJSON *invalid = cJSON_CreateObject();
    const cJSON *bad_defs[2] = {def, invalid};
    int count_before_failure = source.roster_count;
    uint64_t next_before_failure = source.next_instance_id;
    int first_bad = 123;
    assert(!PlayerRosterAppendDefinitions(&source, bad_defs, 2, &first_bad));
    assert(first_bad == -1);
    assert(source.roster_count == count_before_failure && source.next_instance_id == next_before_failure);
    cJSON_Delete(invalid);

    int gold = source.gold;
    int gems = source.gems;
    assert(PlayerSpendGold(&source, 25) && source.gold == gold - 25);
    assert(PlayerSpendGems(&source, 3) && source.gems == gems - 3);
    gold = source.gold;
    gems = source.gems;
    assert(!PlayerSpendGold(&source, -1) && !PlayerSpendGold(&source, gold + 1));
    assert(!PlayerSpendGems(&source, -1) && !PlayerSpendGems(&source, gems + 1));
    assert(source.gold == gold && source.gems == gems);

    source.store_rng_state = 0x76543210u;
    source.store_opened_total = 10;
    source.store_opened_by_pack[0] = 1;
    source.store_opened_by_pack[1] = 2;
    source.store_opened_by_pack[2] = 3;
    source.store_opened_by_pack[3] = 4;
    source.champion_since_legendary = 2;
    assert(PlayerSquadPlace(&source, 2, 0, first));
    PlayerSquadSelect(&source, 2);
    assert(ProgressionSaveToPath(&source, path));
    assert(FileExists(path) && !FileExists(temporary));

    Player loaded = {0};
    ProgressionInitializePlayerDefaults(&loaded);
    PlayerLoadRoster(&loaded, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(loaded.roster_count == source.roster_count);
    assert(loaded.starter_roster_count == 0);
    assert(loaded.next_instance_id == source.next_instance_id);
    assert(loaded.roster[first].instance_id == source.roster[first].instance_id);
    assert(loaded.roster[first + 1].instance_id == source.roster[first + 1].instance_id);
    assert(strcmp(loaded.roster[first].id, "UNIT_ELVENMAGE") == 0);
    assert(loaded.roster[first].ability == &loaded.roster_abilities[first]);
    assert(PlayerRosterDefinitionCount(&loaded, "UNIT_ELVENMAGE") == definition_count + 2);
    assert(loaded.store_rng_state == source.store_rng_state && loaded.store_opened_total == 10);
    assert(loaded.store_opened_by_pack[2] == 3 && loaded.champion_since_legendary == 2);
    assert(loaded.current_squad == 2 && loaded.squads[2][0] == first);

    Character addition_before = loaded.roster[first];
    Player player_before = loaded;
    char *json = LoadFileText(path);
    cJSON *corrupt_root = cJSON_Parse(json);
    UnloadFileText(json);
    cJSON *corrupt_roster = cJSON_GetObjectItemCaseSensitive(corrupt_root, "roster");
    cJSON *corrupt_additions = cJSON_GetObjectItemCaseSensitive(corrupt_roster, "additions");
    cJSON *corrupt_entry = cJSON_GetArrayItem(corrupt_additions, 0);
    cJSON_ReplaceItemInObjectCaseSensitive(corrupt_entry, "id", cJSON_CreateString("UNIT_DOES_NOT_EXIST"));
    char *corrupt_json = cJSON_Print(corrupt_root);
    cJSON_Delete(corrupt_root);
    assert(corrupt_json && SaveFileText(path, corrupt_json));
    cJSON_free(corrupt_json);
    assert(!ProgressionLoadFromPath(&loaded, path));
    assert(memcmp(&player_before, &loaded, sizeof(loaded)) == 0);
    assert(memcmp(&addition_before, &loaded.roster[first], sizeof(addition_before)) == 0);

    cJSON_Delete(catalog_root);
    PlayerUnloadRoster(&loaded);
    PlayerUnloadRoster(&source);
    remove(path);
    remove(temporary);
}

static Character *ConsumeTestAdd(Player *player, const cJSON *catalog, const char *id, int *index) {
    const cJSON *definition = CatalogFindCharacter(catalog, id);
    assert(definition && PlayerRosterAppendDefinitions(player, &definition, 1, index));
    return &player->roster[*index];
}

static const char *ConsumeTestFirstId(const cJSON *catalog, const char *rarity, bool giant, bool full_tiers) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)catalog) {
        const cJSON *tiers = cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "tiers");
        if (strcmp(JsonStr(entry, "rarity"), rarity) == 0 && JsonBool(entry, "giant") == giant &&
            (cJSON_GetArraySize((cJSON *)tiers) == TIER_COUNT) == full_tiers)
            return JsonStr(entry, "id");
    }
    assert(false);
    return NULL;
}

static void TestConsumeTablesAndCurves(void) {
    ConsumeXpTable table = {.xp = {0, 10, 30, 60}, .count = 4};
    assert(ConsumeXpForLevel(&table, 1) == 0 && ConsumeXpForLevel(&table, 4) == 60);
    assert(ConsumeLevelForXp(&table, 0) == 1 && ConsumeLevelForXp(&table, 9) == 1);
    assert(ConsumeLevelForXp(&table, 10) == 2 && ConsumeLevelForXp(&table, 60) == 4);
    assert(ConsumeLevelForXp(&table, 999) == 4);
    assert(fabsf(ConsumeBandFraction(&table, 2, 20) - 0.5f) < 0.001f);
    assert(ConsumeBandFraction(&table, 4, 60) == 1.0f);

    ConsumeCatalogEnsure();
    const ConsumeProgression *standard = ConsumeFindProgression("standard");
    ConsumeProfile profile = {.progression = standard};
    assert(ConsumeUnlockKindAt(&profile, 1) == CONSUME_UNLOCK_CRIT);
    assert(ConsumeUnlockKindAt(&profile, 3) == CONSUME_UNLOCK_FORTIFY);
    assert(ConsumeUnlockKindAt(&profile, 13) == CONSUME_UNLOCK_PROMOTION);
    assert(ConsumeProgressionLevelOfTier(&profile, 1) == 13);
    assert(ConsumeProgressionLevelOfTier(&profile, 3) == 32);
    assert(ConsumeUnlockedTotal(&profile, 1, CONSUME_UNLOCK_CRIT) == 1);
    assert(ConsumeUnlockedTotal(&profile, 2, CONSUME_UNLOCK_CRIT) == 2);
    assert(ConsumeUnlockKindAt(&profile, standard->count) == CONSUME_UNLOCK_NONE);
}

static void TestConsumeSeedWorthAndBonuses(void) {
    cJSON *root = ProgressionLoadCharacterCatalog();
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    const char *common = ConsumeTestFirstId(catalog, "Common", false, true);
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    int target_index, seed_index;
    Character *target = ConsumeTestAdd(&player, catalog, common, &target_index);
    Character *seed = ConsumeTestAdd(&player, catalog, common, &seed_index);
    assert(target->consume_level == 1 && target->crit_chance == CONSUME_BASE_CRIT_CHANCE && target->xp == 0);

    const Character *seeds[] = {seed};
    ConsumeOutcome outcome = ConsumeCompute(target, seeds, 1);
    const ConsumeProfile *profile = ConsumeProfileFor(target);
    int combine = ConsumeXpForLevel(profile->consume_table, 14);
    int expected_worth = (int)rint(100.0f * profile->worth[TIER_I] * (1.0f + CONSUME_SAME_FACTION_BONUS));
    assert(outcome.consume_xp == combine + expected_worth);
    assert(outcome.tier == TIER_II);
    ConsumeSeedPreview preview = ConsumePreviewSeed(target, seed);
    assert(preview.same_group && preview.shares_faction);
    assert(preview.tier_bonus_pct == 0 && preview.faction_bonus_pct == 20 && preview.combine_bonus_pct == 0);
    assert(preview.xp_gain == (int)rint(rint(100.0f * profile->worth[TIER_I]) * CONSUME_HERO_XP_SHARE_OF_WORTH));

    PlayerUnloadRoster(&player);
    cJSON_Delete(root);
}

static void TestConsumeCommitCompactsRoster(void) {
    cJSON *root = ProgressionLoadCharacterCatalog();
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    const char *rare = ConsumeTestFirstId(catalog, "Rare", false, true);
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    int squad_unit = player.squads[0][0];
    uint64_t squad_instance = player.roster[squad_unit].instance_id;
    int count = player.roster_count;
    int seed_a, target, seed_b;
    ConsumeTestAdd(&player, catalog, rare, &seed_a);
    ConsumeTestAdd(&player, catalog, rare, &target);
    ConsumeTestAdd(&player, catalog, rare, &seed_b);
    assert(PlayerSquadPlace(&player, 2, 0, target));
    uint64_t target_instance = player.roster[target].instance_id;

    int blocked[] = {squad_unit};
    assert(!PlayerConsumeSeedAllowed(&player, target, squad_unit));
    assert(PlayerConsumeUnits(&player, target, blocked, 1) == -1);
    int self[] = {target};
    assert(PlayerConsumeUnits(&player, target, self, 1) == -1);
    int repeated[] = {seed_a, seed_a};
    assert(PlayerConsumeUnits(&player, target, repeated, 2) == -1);
    assert(player.roster_count == count + 3);

    int seeds[] = {seed_a, seed_b};
    int moved = PlayerConsumeUnits(&player, target, seeds, 2);
    assert(moved == count);
    assert(player.roster_count == count + 1);
    assert(player.roster[moved].instance_id == target_instance);
    assert(player.roster[moved].ability == &player.roster_abilities[moved]);
    assert(player.roster[moved].tier == TIER_III);
    assert(player.squads[2][0] == moved);
    assert(player.roster[player.squads[0][0]].instance_id == squad_instance);
    assert(player.roster[count + 1].instance_id == 0);

    const char *path = "tests/consume_save_test.dat";
    player.next_instance_id = PlayerRosterNextAvailableInstanceId(&player);
    assert(ProgressionSaveToPath(&player, path));
    Player loaded = {0};
    ProgressionInitializePlayerDefaults(&loaded);
    PlayerLoadRoster(&loaded, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(loaded.roster_count == player.roster_count);
    const Character *a = &player.roster[moved], *b = &loaded.roster[moved];
    assert(b->tier == a->tier && b->level == a->level && b->xp == a->xp && b->consume_xp == a->consume_xp);
    assert(b->consume_level == a->consume_level && b->crit_chance == a->crit_chance);
    assert(loaded.squads[2][0] == moved);
    remove(path);
    PlayerUnloadRoster(&loaded);
    PlayerUnloadRoster(&player);
    cJSON_Delete(root);
}

static void TestWispWorthFollowsReferenceMultipliers(void) {
    Character wisp = {.rarity = RARITY_RARE, .tier = TIER_I};
    StrCopy(wisp.id, sizeof(wisp.id), "UNIT_RAREWHITEWISP");
    Character hero = {.rarity = RARITY_RARE, .tier = TIER_I};
    StrCopy(hero.id, sizeof(hero.id), "UNIT_ELVENMAGE");
    float wisp_worth = ConsumeSeedBaseWorth(&wisp);
    assert(wisp_worth == rint(600.0f * 2.5f * 1.15f));
    wisp.tier = TIER_IV;
    assert(fabsf(ConsumeSeedBaseWorth(&wisp) - wisp_worth * 4.0f) < 4.0f);
    assert(ConsumeSeedBaseWorth(&hero) < wisp_worth);
}

static void TestConsumeRespectsCaps(void) {
    cJSON *root = ProgressionLoadCharacterCatalog();
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");

    int unique_index, seed_index;
    const char *unique = ConsumeTestFirstId(catalog, "Legendary", false, false);
    Character *target = ConsumeTestAdd(&player, catalog, unique, &unique_index);
    Character *seed = ConsumeTestAdd(&player, catalog, unique, &seed_index);
    const Character *seeds[] = {seed};
    Tier before = target->tier;
    assert(ConsumeCompute(target, seeds, 1).tier == before);

    const ConsumeProfile *profile = ConsumeProfileFor(target);
    target->consume_xp = ConsumeXpForLevel(profile->consume_table, profile->consume_table->count);
    target->level = target->stats[target->tier].max_level;
    target->xp = ConsumeXpForLevel(profile->level_table, target->level);
    ConsumeNormalizeCharacter(target);
    assert(ConsumeCharacterMaxed(target));
    ConsumeOutcome outcome = ConsumeCompute(target, seeds, 1);
    assert(outcome.consume_xp == target->consume_xp && outcome.xp == target->xp && outcome.level == target->level);
    assert(outcome.crit_chance == 30);

    int tiered_index;
    Character *tiered =
        ConsumeTestAdd(&player, catalog, ConsumeTestFirstId(catalog, "Rare", false, true), &tiered_index);
    tiered->tier = TIER_II;
    tiered->consume_xp = 0;
    ConsumeNormalizeCharacter(tiered);
    assert(tiered->consume_level == 14 && tiered->crit_chance == 10);

    for (int i = 0; i < player.roster_count; i++) assert(ConsumeProfileFor(&player.roster[i]));
    PlayerUnloadRoster(&player);
    cJSON_Delete(root);
}

static void TestSimUsesConsumedCritChance(void) {
    Character ch = {.crit_chance = 12};
    Ability ability = {0};
    ch.ability = &ability;
    SimUnit units[SQUAD_SLOT_COUNT * 2];
    int front_slot = 1;
    assert(SquadSlotIsFront(front_slot));
    SimUnitsInit(units, 0, front_slot, &ch, NULL);
    assert(units[front_slot].crit_pct == 12.0f);
    ch.crit_chance = 0;
    SimUnitsInit(units, 0, front_slot, &ch, NULL);
    assert(units[front_slot].crit_pct == 1.0f);
}

static void TestAllCampaignMapsAdvance(void) {
    Campaign campaign = {.map_count = CAMPAIGN_MAX_MAPS};
    for (int map = 0; map < CAMPAIGN_MAX_MAPS; map++) {
        campaign.maps[map].node_count = 1;
        campaign.maps[map].nodes[0] = (CampaignNode){.gold = 1, .xp = 1};
    }
    Player player = {0};
    ProgressionInitializePlayerDefaults(&player);

    for (int map = 0; map < CAMPAIGN_MAX_MAPS; map++) {
        assert(CampaignMapUnlocked(&campaign, &player.campaign_progress, map));
        BattleRewardResult result = CampaignResolveBattle(&player, &campaign, map, 0, true, 0);
        assert(result.resolved && result.first_win);
        if (map + 1 < CAMPAIGN_MAX_MAPS) {
            assert(result.advanced && result.unlocked_map);
            assert(result.next_map_index == map + 1 && result.next_node_index == 0);
        } else {
            assert(!result.advanced && !result.unlocked_map);
            assert(result.next_map_index == map && result.next_node_index == 0);
        }
    }
}

static void TestEnergyAndStaminaPools(void) {
    Player player = {
        .energy = 1,
        .energy_max = 3,
        .stamina = 2,
        .stamina_max = 4,
        .energy_regen_timer = 299.0f,
        .stamina_regen_timer = 298.0f,
        .gems = ENERGY_REFILL_GEM_COST + STAMINA_REFILL_GEM_COST,
    };

    assert(fabsf(PlayerEnergyRegenRemaining(&player) - 1.0f) < 0.001f);
    assert(fabsf(PlayerStaminaRegenRemaining(&player) - 2.0f) < 0.001f);
    PlayerUpdate(&player, 2.0f);
    assert(player.energy == 2 && fabsf(player.energy_regen_timer - 1.0f) < 0.001f);
    assert(player.stamina == 3 && fabsf(player.stamina_regen_timer) < 0.001f);

    assert(PlayerRefillEnergy(&player));
    assert(player.energy == player.energy_max && player.energy_regen_timer == 0.0f);
    assert(PlayerRefillStamina(&player));
    assert(player.stamina == player.stamina_max && player.stamina_regen_timer == 0.0f);
    assert(player.gems == 0);
    assert(!PlayerRefillEnergy(&player) && !PlayerRefillStamina(&player));
    assert(PlayerEnergyRegenRemaining(&player) == 0.0f);
    assert(PlayerStaminaRegenRemaining(&player) == 0.0f);
}

static void TestFxKeyoutPreservesSourceTransparency(void) {
    Color transparent_grey = {128, 128, 128, 0};
    Color white_key = FxKeyoutPixel(transparent_grey, 1);
    Color black_key = FxKeyoutPixel(transparent_grey, 2);
    assert(white_key.a == 0 && black_key.a == 0);

    Color opaque_grey = {128, 128, 128, 255};
    white_key = FxKeyoutPixel(opaque_grey, 1);
    black_key = FxKeyoutPixel(opaque_grey, 2);
    assert(white_key.a == 127 && black_key.a == 128);
}

static unsigned int ImageFnv(Image image) {
    unsigned int hash = 2166136261u;
    const unsigned char *pixels = image.data;
    for (int i = 0; i < image.width * image.height * 4; i++) hash = (hash ^ pixels[i]) * 16777619u;
    return hash;
}

static void TestOriginalResourcesDecodeLikeUnityPy(void) {
    static const struct {
        const char *ref;
        int width, height;
        unsigned int unitypy_hash;
    } textures[] = {
        {CACHE_RESOURCE("Bg_PlayerRow"), 32, 32, 0x975c18aau},
        {CACHE_RESOURCE("SmallButtonDarkBlue"), 35, 31, 0xce5ffbddu},
        {CACHE_RESOURCE("Default-Particle"), 64, 64, 0x7d8d4582u},
        {CACHE_RESOURCE("tex.c_abomination_t1_pt"), 128, 128, 0xbe28c1fau},
        {CACHE_RESOURCE("tex.Btn_MapPVE_map_08"), 256, 256, 0xafb8507cu},
        {APK_RESOURCE("Btn_CTA_gold_cost"), 256, 64, 0xb57fbb67u},
        {APK_RESOURCE("Btn_CTA_gold"), 256, 64, 0x1d6ea15cu},
    };
    for (int i = 0; i < (int)(sizeof(textures) / sizeof(textures[0])); i++) {
        Image image = ResourceLoadImage(textures[i].ref);
        assert(image.data && image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        assert(image.width == textures[i].width && image.height == textures[i].height);
        assert(ImageFnv(image) == textures[i].unitypy_hash);
        UnloadImage(image);
    }

    Wave wave = ResourceLoadWave(CACHE_RESOURCE("gui_bonus_faction"));
    assert(IsWaveValid(wave));
    UnloadWave(wave);

    unsigned int object_size = 0;
    unsigned char *font = ResourceLoadObject(APK_RESOURCE("inquisition"), UNITY_CLASS_FONT, &object_size);
    int ttf_size = 0;
    const unsigned char *ttf = font ? UnityFontData(font, object_size, &ttf_size) : NULL;
    static const unsigned char truetype_magic[4] = {0x00, 0x01, 0x00, 0x00};
    assert(ttf && ttf_size == 98036 && memcmp(ttf, truetype_magic, sizeof(truetype_magic)) == 0);
    MemFree(font);

    assert(ResourceExists(CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "base_T1")));
    assert(!ResourceExists(CACHE_BUNDLE_RESOURCE("PackOpening_pre", "base_T2")));
    assert(!ResourceExists(CACHE_RESOURCE("no_such_texture")));
}

static void TestEveryReferencedResourceResolves(void) {
    for (int i = 0; i < GUI_COUNT; i++) assert(ResourceExists(gui_paths[i]));
    for (int i = 0; i < SND_COUNT; i++) assert(ResourceExists(sound_paths[i]));
    for (int i = 0; i < MUS_COUNT; i++) assert(ResourceExists(music_defs[i].path));
    for (int i = 0; i < FNT_COUNT; i++) assert(ResourceExists(font_paths[i]));
    for (int r = RARITY_COMMON; r <= RARITY_MYTHIC; r++) assert(ResourceExists(RarityBaseRef((Rarity)r)));
    for (int f = FACTION_LARGE; f < FACTION_ALL; f++) {
        const char *emblem = HomeFactionTexName((Faction)f);
        if (emblem) assert(ResourceExists(TextFormat(CACHE_RESOURCE("%s"), emblem)));
    }

    cJSON *root = JsonLoadFile("data/fx_presets.json");
    int textures = 0;
    cJSON *preset = NULL;
    cJSON_ArrayForEach(preset, cJSON_GetObjectItemCaseSensitive(root, "presets")) {
        cJSON *emitter = NULL;
        cJSON_ArrayForEach(emitter, cJSON_GetObjectItemCaseSensitive(preset, "emitters")) {
            const char *tex = JsonStr(emitter, "tex");
            if (!tex[0]) continue;
            if (!ResourceExists(tex)) fprintf(stderr, "unresolved fx texture: %s\n", tex);
            assert(ResourceExists(tex));
            textures++;
        }
    }
    assert(textures > 2000);
    cJSON_Delete(root);

    for (int i = 0; i < (int)(sizeof(FX_PRESETS) / sizeof(FX_PRESETS[0])); i++)
        for (int e = 0; e < FX_PRESETS[i].em_count; e++) {
            const char *tex = FX_PRESETS[i].em[e].tex;
            if (!tex || !tex[0]) continue;
            if (!ResourceExists(tex)) fprintf(stderr, "unresolved hand fx texture: %s\n", tex);
            assert(ResourceExists(tex));
        }
}

static void TestRuneCatalogCraftChains(void) {
    assert(RuneCatalogCount() == 122);
    const char *const chain[] = {
        "rune-health-1",
        "rune-health-2",
        "rune-health-3",
        "rune-health-4",
        "rune-health-5",
        "rune-health-6",
        "rune-health-6-2",
        "rune-health-6-4",
        "rune-health-6-8",
    };
    int count = (int)(sizeof(chain) / sizeof(chain[0]));
    for (int i = 0; i + 1 < count; i++)
        assert(RuneDefAt(RuneDefIndex(chain[i]))->craft_to == RuneDefIndex(chain[i + 1]));
    assert(!RuneCanCraft(RuneDefAt(RuneDefIndex(chain[count - 1]))));
    assert(!RuneCanCraft(RuneDefAt(RuneDefIndex("rune-guard-7"))));
    assert(!RuneCanCraft(RuneDefAt(RuneDefIndex("rune-knowledge-Special"))));
    assert(RuneDefAt(RuneDefIndex("rune-health-6-8"))->duration_minutes == 480);
    Loc saved = loc;
    loc = (Loc){0};
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    assert(!strcmp(RuneName(RuneDefAt(RuneDefIndex("rune-guard-7"))), "Руна охраны"));
    assert(!strcmp(RuneValueText(RuneDefAt(RuneDefIndex("rune-guard-7"))), "+75 %"));
    assert(!strcmp(RuneValueText(RuneDefAt(RuneDefIndex("rune-brave-targeting-1"))), "Ур. 1"));
    assert(!strcmp(RuneValueText(RuneDefAt(RuneDefIndex("rune-dispel-7"))), "Иммунный"));
    assert(!strcmp(RuneRarityLabel(RUNE_PRESTIGE_RARITY), "Престижный"));
    char expiration[32];
    RuneFormatExpiration(3 * NEXUS_SECONDS_PER_DAY + 5, expiration, sizeof(expiration));
    assert(!strcmp(expiration, "3 дн."));
    for (int i = 0; i < RuneCatalogCount(); i++) {
        const RuneDef *def = RuneDefAt(i);
        assert(RuneName(def)[0] && RuneDescription(def)[0] && RuneBuffLabel(def)[0] && RuneValueText(def)[0]);
        assert(ResourceExists(TextFormat(
            strncmp(def->icon, RUNE_RESOURCE_CACHE_PREFIX, strlen(RUNE_RESOURCE_CACHE_PREFIX)) ? EXTRA_ASSET_DIR "%s"
                                                                                               : "%s",
            def->icon
        )));
    }
    LocUnload();
    loc = saved;
}

static void TestRuneSlotRulesAndReplacement(void) {
    static PlayerRunes runes;
    runes = (PlayerRunes){0};
    long long now = 1000;
    uint64_t power = RuneGrantById(&runes, "rune-power-5", now);
    uint64_t health = RuneGrantById(&runes, "rune-health-5", now);
    uint64_t guard = RuneGrantById(&runes, "rune-guard-7", now);

    assert(RuneActivate(&runes, 0, RUNE_PRESTIGE_SLOT, power, now) == RUNE_ASSIGN_SLOT_LOCKED);
    assert(RuneActivate(&runes, 0, 0, guard, now) == RUNE_ASSIGN_NEEDS_NORMAL_SLOT);
    runes.prestige_unlocked[0] = true;
    assert(RuneActivate(&runes, 0, RUNE_PRESTIGE_SLOT, power, now) == RUNE_ASSIGN_NEEDS_PRESTIGE_RUNE);
    assert(RuneActivate(&runes, 0, RUNE_PRESTIGE_SLOT, guard, now) == RUNE_ASSIGN_OK);
    assert(RuneByUuid(&runes, guard)->expire_at == now + 30 * RUNE_SECONDS_PER_MINUTE);

    assert(RuneActivate(&runes, 0, 0, power, now) == RUNE_ASSIGN_OK);
    assert(RuneAvailableCount(&runes, now) == 1);
    assert(RuneActivate(&runes, 1, 0, power, now) == RUNE_ASSIGN_NOT_AVAILABLE);
    assert(RuneActivate(&runes, 0, 0, health, now) == RUNE_ASSIGN_OK);
    assert(!RuneByUuid(&runes, power));
    assert(RuneInSlot(&runes, 0, 0)->uuid == health);
}

static void TestRuneExpiry(void) {
    static PlayerRunes runes;
    runes = (PlayerRunes){0};
    long long now = 1000;
    uint64_t active = RuneGrantById(&runes, "rune-power-1", now);
    uint64_t idle = RuneGrantById(&runes, "rune-health-1", now);
    RuneActivate(&runes, 2, 1, active, now);

    assert(RuneExpire(&runes, now + 5 * RUNE_SECONDS_PER_MINUTE) == 0);
    assert(RuneExpire(&runes, now + 5 * RUNE_SECONDS_PER_MINUTE + 1) == 1);
    assert(!RuneInSlot(&runes, 2, 1));
    assert(RuneByUuid(&runes, idle));
    assert(RuneExpire(&runes, now + RUNE_INVENTORY_LIFETIME_SECONDS + 1) == 1);
    assert(runes.count == 0);
}

static void TestRuneCrafting(void) {
    static PlayerRunes runes;
    runes = (PlayerRunes){0};
    long long now = 1000;
    uint64_t a = RuneGrantById(&runes, "rune-power-1", now);
    uint64_t b = RuneGrantById(&runes, "rune-power-1", now);
    uint64_t other = RuneGrantById(&runes, "rune-health-1", now);
    uint64_t top = RuneGrantById(&runes, "rune-power-6-8", now);
    uint64_t top_twin = RuneGrantById(&runes, "rune-power-6-8", now);

    assert(RuneCraft(&runes, a, other, now, NULL) == RUNE_CRAFT_MISMATCH);
    assert(RuneCraft(&runes, a, a, now, NULL) == RUNE_CRAFT_RUNES_NOT_FOUND);
    assert(RuneCraft(&runes, top, top_twin, now, NULL) == RUNE_CRAFT_NO_UPGRADE);
    int crafted = -1;
    assert(RuneCraft(&runes, a, b, now, &crafted) == RUNE_CRAFT_SUCCESS);
    assert(crafted == RuneDefIndex("rune-power-2"));
    assert(runes.count == 4);

    uint64_t active = RuneGrantById(&runes, "rune-stun-1", now);
    uint64_t idle = RuneGrantById(&runes, "rune-stun-1", now);
    RuneActivate(&runes, 0, 0, active, now);
    assert(RuneCraft(&runes, active, idle, now, NULL) == RUNE_CRAFT_ALREADY_ASSIGNED);

    runes = (PlayerRunes){0};
    for (int i = 0; i < 5; i++) RuneGrantById(&runes, "rune-power-1", now);
    for (int i = 0; i < 2; i++) RuneGrantById(&runes, "rune-health-1", now);
    static RuneCraftLog log;
    log.count = 0;
    assert(RuneCraftAll(&runes, RUNE_CRAFT_MODE_TYPE, RuneDefIndex("rune-power-1"), now, &log) == 2);
    assert(runes.count == 5);
    assert(log.count == 2 && log.pairs[0][0] == 1 && log.pairs[0][1] == 2 && log.pairs[1][0] == 3);
    assert(RuneCraftAll(&runes, RUNE_CRAFT_MODE_RARITY, RuneDefIndex("rune-health-1"), now, NULL) == 1);
    assert(runes.count == 4);
}

static void TestNexusAwardReadsTheServerReply(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    const Character *last = &player.roster[player.roster_count - 1];
    char text[256];
    snprintf(
        text,
        sizeof(text),
        "{\"granted\":{\"gold\":500,\"granted\":{\"heroes\":[{\"id\":\"%s\",\"instance_id\":%llu},"
        "{\"id\":\"UNIT_X\",\"instance_id\":999999}]}}}",
        last->id,
        (unsigned long long)last->instance_id
    );
    cJSON *reply = cJSON_Parse(text);
    NexusAward award = GameNexusAwardFromReply(&player, reply, 2);
    assert(award.gold == 500 && award.potions == 2 && award.unit_count == 1);
    assert(award.unit_roster[0] == player.roster_count - 1);
    cJSON_Delete(reply);
}

static void TestServerActionsStayLocalWithoutABackend(void) {
    ServerActionSubmit(ServerActionCreate("battle"));
    assert(ServerActionsDrained());
    cJSON *reply = NULL;
    assert(!ServerActionsTakeReply(SERVER_REPLY_NEXUS_AWARD, &reply) && !reply);
}

static void TestRuneBonusStacksAndScalesRewards(void) {
    static PlayerRunes runes;
    runes = (PlayerRunes){0};
    long long now = 1000;
    RuneActivate(&runes, 0, 0, RuneGrantById(&runes, "rune-power-3", now), now);
    RuneActivate(&runes, 0, 1, RuneGrantById(&runes, "rune-power-5", now), now);
    RuneBonus bonus = RuneSquadBonus(&runes, 0, now);
    assert(bonus.power_pct == 70);
    assert(RuneSquadBonus(&runes, 1, now).power_pct == 0);
    assert(RuneApplyPct(100, 70) == 170);
    assert(RuneApplyPct(100, 0) == 100);
    assert(RuneSquadBonus(&runes, 0, now + 11 * RUNE_SECONDS_PER_MINUTE).power_pct == 50);

    runes = (PlayerRunes){0};
    runes.prestige_unlocked[0] = true;
    RuneActivate(&runes, 0, RUNE_PRESTIGE_SLOT, RuneGrantById(&runes, "rune-dispel-7", now), now);
    assert(RuneSquadBonus(&runes, 0, now).curse_power_immune);
}

static void TestRuneMitigationAndRetaliation(void) {
    assert(SimDamageAfterResist(1000, 600, 0) == 400);
    assert(SimDamageAfterResist(100, 600, 0) == SIM_RUNE_SHIELDED_HIT_MIN_DAMAGE);
    assert(SimDamageAfterResist(1000, 0, 75) == 250);
    assert(SimDamageAfterResist(1000, 0, 0) == 1000);

    BattleSim s = {0};
    Character attacker, target;
    Ability attacker_ability, target_ability;
    TestCharacter(&attacker, &attacker_ability, "rune_attacker", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&target, &target_ability, "rune_target", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &attacker);
    SimUnitInit(&s, 1, 0, &target);
    s.rng = 7;
    s.rune_bonus[1].retaliation_pct = 10;
    int health = s.units[0].health;
    SimAttack(&s, 0, &SIM_RECIPE_DEFAULT, &(AttackRecipe){.target = RECIPE_TARGET_ABILITY}, true, false);
    int dealt = s.units[SQUAD_SLOT_COUNT].max_health - s.units[SQUAD_SLOT_COUNT].health;
    assert(dealt > 0);
    assert(s.units[0].health == health - (int)lroundf(dealt * 0.1f));

    int after = s.units[0].health;
    s.rune_bonus[0].stealth = true;
    SimAttack(&s, 0, &SIM_RECIPE_DEFAULT, &(AttackRecipe){.target = RECIPE_TARGET_ABILITY}, true, false);
    assert(s.units[0].health == after);
    s.rune_bonus[1].perceptive = true;
    SimAttack(&s, 0, &SIM_RECIPE_DEFAULT, &(AttackRecipe){.target = RECIPE_TARGET_ABILITY}, true, false);
    assert(s.units[0].health < after);
}

static void TestRuneResistsBlockAilments(void) {
    BattleSim s = {0};
    Character attacker, target;
    Ability attacker_ability, target_ability;
    TestCharacter(&attacker, &attacker_ability, "rune_attacker", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    TestCharacter(&target, &target_ability, "rune_target", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 10000);
    SimUnitInit(&s, 0, 0, &attacker);
    SimUnitInit(&s, 1, 0, &target);
    s.rng = 11;
    s.rune_bonus[1] = (RuneBonus){
        .stun_resist_pct = 100, .slow_resist_pct = 100, .drain_resist_pct = 100, .curse_power_immune = true
    };
    AttackRecipe hit = {
        .target = RECIPE_TARGET_ABILITY,
        .stun_pct = 100,
        .slow_pct = 100,
        .slow_amt = -50,
        .drain_pct = 50,
        .curse_power_pct = 100
    };
    SimAttack(&s, 0, &SIM_RECIPE_DEFAULT, &hit, true, false);
    const SimUnit *t = &s.units[SQUAD_SLOT_COUNT];
    assert(!t->stunned && !t->curse_power);
    assert(t->cur_initiative == 0 && t->power == t->start_power);

    s.rune_bonus[1] = (RuneBonus){0};
    SimAttack(&s, 0, &SIM_RECIPE_DEFAULT, &hit, true, false);
    assert(t->stunned && t->curse_power);
    assert(t->cur_initiative == -50 && t->power < t->start_power);
}

static void TestRuneStatsAndTargeting(void) {
    BattleSim s = {0};
    Character attacker, weak, strong;
    Ability a0, a1, a2;
    TestCharacter(&attacker, &a0, "rune_attacker", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 1000);
    TestCharacter(&weak, &a1, "rune_weak", "ABILITY_DAMAGE_ONE", TARGET_ONE, 50, 1000);
    TestCharacter(&strong, &a2, "rune_strong", "ABILITY_DAMAGE_ONE", TARGET_ONE, 500, 1000);
    SimUnitInit(&s, 0, 0, &attacker);
    SimUnitInit(&s, 1, 0, &weak);
    SimUnitInit(&s, 1, 2, &strong);
    int power = s.units[0].power, health = s.units[0].max_health;
    RuneBonus stats = {.power_pct = 50, .health_pct = 20};
    SimUnitsRuneBonus(s.units, 0, &stats);
    assert(s.units[0].power == RuneApplyPct(power, 50) && s.units[0].start_power == s.units[0].power);
    assert(s.units[0].max_health == RuneApplyPct(health, 20) && s.units[0].health == s.units[0].max_health);

    const SimUnit *weak_unit = &s.units[SQUAD_SLOT_COUNT], *strong_unit = &s.units[SQUAD_SLOT_COUNT + 2];
    float base_weak = SimTargetWeight(&s, &s.units[0], weak_unit);
    float base_strong = SimTargetWeight(&s, &s.units[0], strong_unit);
    s.rune_bonus[0].target_level[RUNE_TARGET_HIGHEST_POWER] = 6;
    assert(SimTargetWeight(&s, &s.units[0], weak_unit) == base_weak);
    assert(SimTargetWeight(&s, &s.units[0], strong_unit) == base_strong * 4.0f);

    s.rng = 3;
    s.rune_bonus[0] = (RuneBonus){.initiative_steps = 1};
    int roll = SimRollInitiative(&s, &s.units[0]);
    assert(roll >= (s.units[0].init_class + 1) * SIM_INIT_CLASS_SPAN);
}

static void TestRuneSaveRoundTrip(void) {
    static Player source, loaded;
    source = (Player){0};
    ProgressionInitializePlayerDefaults(&source);
    long long now = RuneNow();
    uint64_t power = RuneGrantById(&source.runes, "rune-power-6-2", now);
    uint64_t idle = RuneGrantById(&source.runes, "rune-greed-3", now);
    source.runes.prestige_unlocked[1] = true;
    assert(RuneActivate(&source.runes, 3, 1, power, now) == RUNE_ASSIGN_OK);
    source.runes.seen = 1;

    const char *path = "tests/rune_save_test.dat";
    assert(ProgressionSaveToPath(&source, path));
    loaded = (Player){0};
    ProgressionInitializePlayerDefaults(&loaded);
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(loaded.runes.count == 2 && loaded.runes.seen == 1);
    assert(loaded.runes.next_uuid == source.runes.next_uuid);
    assert(loaded.runes.prestige_unlocked[1] && !loaded.runes.prestige_unlocked[0]);
    assert(RuneInSlot(&loaded.runes, 3, 1)->uuid == power);
    assert(RuneInSlot(&loaded.runes, 3, 1)->expire_at == source.runes.items[0].expire_at);
    assert(RuneDefFor(RuneByUuid(&loaded.runes, idle)) == RuneDefAt(RuneDefIndex("rune-greed-3")));
    remove(path);
}

static void TestRuneActivationFx(void) {
    RunesScreen runes = {0};
    assert(!RunesTakeActivationFx(&runes));
    RunesPlayActivationStep(&runes);
    assert(RunesTakeActivationFx(&runes));
    assert(!RunesTakeActivationFx(&runes));

    cJSON *root = JsonLoadFile("data/fx_presets.json");
    bool activation = false, aura = false;
    const cJSON *preset = NULL;
    cJSON_ArrayForEach(preset, cJSON_GetObjectItemCaseSensitive(root, "presets")) {
        activation = activation || !strcmp(JsonStr(preset, "name"), RUNE_ACTIVATION_FX);
        aura = aura || !strcmp(JsonStr(preset, "name"), BP_RUNE_AURA_FX);
    }
    cJSON_Delete(root);
    assert(activation && aura);
}

static void TestPrestigeSlotCostsGems(void) {
    static Player player;
    player = (Player){.gems = RUNE_PRESTIGE_UNLOCK_GEMS - 1};
    assert(!PlayerUnlockPrestigeRuneSlot(&player, 0));
    player.gems = RUNE_PRESTIGE_UNLOCK_GEMS;
    assert(PlayerUnlockPrestigeRuneSlot(&player, 0));
    assert(player.gems == 0 && player.runes.prestige_unlocked[0]);
    assert(!PlayerUnlockPrestigeRuneSlot(&player, 0));
}

static void TestGalleryRecordsTrackDiscoveryAndBadge(void) {
    PlayerGallery gallery = {0};
    GalleryRecordOwned(&gallery, "UNIT_A", TIER_I, true);
    assert(gallery.badge == 0);
    const GalleryRecord *record = GalleryRecordFind(&gallery, "UNIT_A");
    assert(record && record->owned_tiers == 1 && !record->new_owned);

    GalleryRecordOwned(&gallery, "UNIT_A", TIER_I, false);
    assert(gallery.badge == 0);
    GalleryRecordOwned(&gallery, "UNIT_A", TIER_III, false);
    record = GalleryRecordFind(&gallery, "UNIT_A");
    assert(gallery.badge == 1 && record->owned_tiers == 5 && record->new_owned);

    GalleryRecordBattleUnit(&gallery, "UNIT_B", false);
    assert(gallery.badge == 2);
    GalleryRecordBattleUnit(&gallery, "UNIT_B", false);
    assert(gallery.badge == 2);
    GalleryRecordBattleUnit(&gallery, "UNIT_B", true);
    record = GalleryRecordFind(&gallery, "UNIT_B");
    assert(gallery.badge == 3 && record->fought && record->defeated && record->new_defeated);

    assert(GalleryRecordClearNew(&gallery, "UNIT_B"));
    assert(!GalleryRecordClearNew(&gallery, "UNIT_B"));
    assert(gallery.badge == 3);
    GalleryFree(&gallery);
}

static void TestGalleryCatalogMatchesReferenceRules(void) {
    GalleryCatalog catalog;
    GalleryCatalogLoad(&catalog, HOD_CACHE "characters.json");
    assert(catalog.count > 0 && catalog.group_count > 0 && catalog.specialty_count > 0);
    for (int i = 0; i < catalog.count; i++) {
        const Character *unit = &catalog.units[i];
        assert(unit->rarity != RARITY_MYTHIC);
        assert(unit->ability->key[0] == '\0' || unit->ability == &catalog.abilities[i]);
        if (i == 0) continue;
        const Character *previous = &catalog.units[i - 1];
        assert(previous->rarity >= unit->rarity);
        if (previous->rarity == unit->rarity)
            assert(
                previous->stats[GalleryLowestTier(previous)].base_power >=
                unit->stats[GalleryLowestTier(unit)].base_power
            );
    }
    for (int g = 1; g < catalog.group_count; g++) assert(strcasecmp(catalog.groups[g - 1], catalog.groups[g]) < 0);

    PlayerGallery gallery = {0};
    int *matches = (int *)MemAlloc((unsigned int)catalog.count * sizeof(int));
    GalleryProgress progress;
    GallerySelection selection = {.category = GALLERY_CATEGORY_RARITY, .rarity = RARITY_COMMON};
    int commons = GalleryQuery(&catalog, &gallery, &selection, GALLERY_FILTER_NONE, matches, &progress);
    assert(commons > 0 && progress.total == commons && progress.owned == 0);
    const char *first_id = catalog.units[matches[0]].id;

    GalleryRecordOwned(&gallery, first_id, GalleryLowestTier(&catalog.units[matches[0]]), false);
    GalleryRecordBattleUnit(&gallery, first_id, true);
    assert(GalleryQuery(&catalog, &gallery, &selection, GALLERY_FILTER_OWNED, matches, &progress) == 1);
    assert(progress.total == commons && progress.owned == 1 && progress.defeated == 1 && progress.new_owned);
    assert(GalleryQuery(&catalog, &gallery, &selection, GALLERY_FILTER_DEFEATED, matches, &progress) == 1);

    GalleryTotals totals = GalleryTotalsCompute(&catalog, &gallery);
    assert(totals.total == catalog.count && totals.owned == 1);
    assert(totals.rarity_owned[RARITY_COMMON - RARITY_COMMON] == 1);

    selection = (GallerySelection){.category = GALLERY_CATEGORY_FACTION, .faction = GALLERY_FACTION_DUAL};
    assert(GalleryQuery(&catalog, &gallery, &selection, GALLERY_FILTER_NONE, matches, &progress) > 0);
    for (int i = 0; i < progress.total; i++) assert(catalog.units[matches[i]].faction >= FACTION_WHITE_RED);

    char key[GALLERY_KEY_CAP];
    GallerySubcategoryTitleKey(key, sizeof(key), &catalog, &selection);
    assert(!strcmp(key, "GALLERY_DUAL_TITLE"));

    assert(GalleryRarityStep(RARITY_LEGENDARY, 1) == RARITY_COMMON);
    assert(GalleryRarityStep(RARITY_COMMON, -1) == RARITY_LEGENDARY);
    assert(GalleryFactionStep(GALLERY_FACTION_BLACK, -1) == GALLERY_FACTION_DUAL);
    assert(GalleryCategoryStep(GALLERY_CATEGORY_SPECIALTY, 1) == GALLERY_CATEGORY_ALL);
    assert(GalleryCategoryStep(GALLERY_CATEGORY_ALL, -1) == GALLERY_CATEGORY_SPECIALTY);

    MemFree(matches);
    GalleryFree(&gallery);
    GalleryCatalogUnload(&catalog);
}

static void TestGalleryRecordsBattleCasualties(void) {
    static BattleRoster enemies;
    static BattleSim sim;
    memset(&enemies, 0, sizeof(enemies));
    memset(&sim, 0, sizeof(sim));
    StrCopy(enemies.chars[0].id, sizeof(enemies.chars[0].id), "UNIT_FALLEN");
    StrCopy(enemies.chars[1].id, sizeof(enemies.chars[1].id), "UNIT_STANDING");
    enemies.present[0] = enemies.present[1] = true;
    sim.units[SQUAD_SLOT_COUNT + 1].health = 10;

    PlayerGallery gallery = {0};
    GalleryRecordBattle(&gallery, &enemies, &sim);
    assert(gallery.count == 2);
    assert(GalleryRecordFind(&gallery, "UNIT_FALLEN")->defeated);
    assert(GalleryRecordFind(&gallery, "UNIT_STANDING")->fought);
    assert(!GalleryRecordFind(&gallery, "UNIT_STANDING")->defeated);
    GalleryFree(&gallery);
}

static void TestGalleryPersistence(void) {
    Player source = {0}, loaded = {0};
    ProgressionInitializePlayerDefaults(&source);
    ProgressionInitializePlayerDefaults(&loaded);
    GalleryRecordOwned(&source.gallery, "UNIT_A", TIER_II, false);
    GalleryRecordBattleUnit(&source.gallery, "UNIT_B", true);
    const char *path = "/tmp/hod_gallery_save_test.dat";
    assert(ProgressionSaveToPath(&source, path));
    assert(ProgressionLoadFromPath(&loaded, path));
    remove(path);

    assert(loaded.gallery.seeded && loaded.gallery.badge == 2 && loaded.gallery.count == 2);
    const GalleryRecord *owned = GalleryRecordFind(&loaded.gallery, "UNIT_A");
    const GalleryRecord *fought = GalleryRecordFind(&loaded.gallery, "UNIT_B");
    assert(owned && owned->owned_tiers == 2 && owned->new_owned);
    assert(fought && fought->fought && fought->defeated && fought->new_fought && fought->new_defeated);
    GalleryFree(&source.gallery);
    GalleryFree(&loaded.gallery);
}

static const char *const MISSION_TEST_TABLE =
    "{\"missions\":["
    "{\"series\":0,\"tier\":0,\"id\":\"a51\",\"stat\":\"faction_hits\",\"target\":1,\"gold\":0,\"gems\":2},"
    "{\"series\":0,\"tier\":1,\"id\":\"a52\",\"stat\":\"squad_faction\",\"target\":1,\"gold\":500,\"gems\":0},"
    "{\"series\":0,\"tier\":2,\"id\":\"a53\",\"stat\":\"squad_faction\",\"target\":1,\"gold\":0,\"gems\":3,"
    "\"faction\":1},"
    "{\"series\":1,\"tier\":0,\"id\":\"a19\",\"stat\":\"gold_earned\",\"target\":100,\"gold\":500,\"gems\":0}"
    "]}";

static void MissionTestLoadTable(void) {
    const char *path = "/tmp/hoda_mission_test.json";
    assert(SaveFileText(path, (char *)MISSION_TEST_TABLE));
    assert(MissionCatalogLoad(path));
    assert(MissionCount() == 4 && MissionSeriesCount() == 2);
    assert(MissionSeriesTierCount(0) == 3 && MissionSeriesTierCount(1) == 1);
    remove(path);
}

static void TestMissionSeriesPicksTheClaimableTier(void) {
    MissionTestLoadTable();
    PlayerMissions m = {0};
    assert(MissionSeriesCurrent(&m, 0) == 0);
    assert(!MissionSeriesNeedsCollection(&m, 0));

    MissionRaiseProgress(&m, 1, 1);
    assert(MissionSeriesNeedsCollection(&m, 0));
    assert(MissionSeriesCurrent(&m, 0) == 1);
    assert(MissionUnseenClaimableCount(&m) == 1);
    MissionMarkAllSeen(&m);
    assert(MissionUnseenClaimableCount(&m) == 0);

    m.claimed[0] = m.claimed[1] = m.claimed[2] = true;
    assert(MissionSeriesCurrent(&m, 0) == 2);
}

static void TestMissionProgressNeverDrops(void) {
    MissionTestLoadTable();
    PlayerMissions m = {0};
    MissionRaiseProgress(&m, 3, 40);
    MissionRaiseProgress(&m, 3, 10);
    assert(m.progress[3] == 40);
}

static void TestMissionClaimPaysOnce(void) {
    MissionTestLoadTable();
    static Player player;
    memset(&player, 0, sizeof(player));
    assert(!MissionsClaim(&player, 3));
    MissionRaiseProgress(&player.missions, 3, 100);
    player.gold = 7;
    assert(MissionsClaim(&player, 3));
    assert(player.gold == 507 && player.missions.claimed[3]);
    assert(!MissionsClaim(&player, 3));
    assert(player.gold == 507);
    assert(!MissionsClaim(&player, MISSION_CAPACITY));

    MissionTotals totals = MissionComputeTotals(&player.missions);
    assert(totals.claimed == 1 && totals.total == 4);
    assert(totals.gold_claimed == 500 && totals.gold_total == 1000);
    assert(totals.gems_claimed == 0 && totals.gems_total == 5);
}

static void TestMissionGoldTrackerIgnoresSpendingAndClaims(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    player.gold = 100;
    MissionTrackEarnedGold(&player);
    assert(player.missions.counters[MISSION_STAT_GOLD_EARNED] == 0);
    player.gold = 160;
    MissionTrackEarnedGold(&player);
    assert(player.missions.counters[MISSION_STAT_GOLD_EARNED] == 60);
    player.gold = 10;
    MissionTrackEarnedGold(&player);
    assert(player.missions.counters[MISSION_STAT_GOLD_EARNED] == 60);
}

static void TestMissionBattleRecording(void) {
    MissionTestLoadTable();
    static Player player;
    static BattleSim sim;
    memset(&player, 0, sizeof(player));
    memset(&sim, 0, sizeof(sim));
    sim.action_count = 2;
    sim.actions[0].actor = 0;
    sim.actions[0].target_count = 2;
    sim.actions[0].targets[0] =
        (BattleSimTarget){.unit = SQUAD_SLOT_COUNT, .health_delta = -100, .health_faction_delta = -20};
    sim.actions[0].targets[1] = (BattleSimTarget){.unit = 1, .health_delta = 50};
    sim.actions[1].actor = SQUAD_SLOT_COUNT;
    sim.actions[1].target_count = 1;
    sim.actions[1].targets[0] = (BattleSimTarget){.unit = 0, .health_delta = -999};

    long long first_day = 1700000000LL;
    MissionsRecordBattle(&player, &sim, 3, true, first_day);
    MissionsRecordBattle(&player, &sim, 2, false, first_day);
    PlayerMissions *m = &player.missions;
    assert(m->counters[MISSION_STAT_CRITS] == 5);
    assert(m->counters[MISSION_STAT_BATTLE_DAMAGE] == 120);
    assert(m->counters[MISSION_STAT_CHALLENGE_ATTEMPTS] == 1);
    assert(m->counters[MISSION_STAT_FACTION_HITS] == 2);
    assert(m->counters[MISSION_STAT_BATTLE_DAYS] == 1);
    MissionsRecordBattle(&player, &sim, 0, false, first_day + 2 * NEXUS_SECONDS_PER_DAY);
    assert(m->counters[MISSION_STAT_BATTLE_DAYS] == 2);
}

static void TestHudCurrencyTallyHoldsThenTicksUp(void) {
    HudCurrencyTally tally = {0};
    HudCurrencyTallyUpdate(&tally, 100, 0.016f);
    assert(tally.shown == 100.0);

    HudCurrencyTallyHold(&tally);
    HudCurrencyTallyUpdate(&tally, 325, 1.0f);
    assert(tally.shown == 100.0);

    assert(HudCurrencyTallyRelease(&tally, 325));
    assert(!tally.held && tally.ticking);
    HudCurrencyTallyUpdate(&tally, 325, HUD_TALLY_TIME * 0.5f);
    assert(tally.ticking && fabs(tally.shown - 212.5) < 0.01);
    HudCurrencyTallyUpdate(&tally, 325, HUD_TALLY_TIME);
    assert(!tally.ticking && tally.shown == 325.0);

    HudCurrencyTallyUpdate(&tally, 50, 0.016f);
    assert(tally.shown == 50.0);
}

static void TestHudCurrencyTallyReleaseWithoutGainSnaps(void) {
    HudCurrencyTally tally = {.initialized = true, .shown = 40.0};
    assert(!HudCurrencyTallyRelease(&tally, 40));
    HudCurrencyTallyHold(&tally);
    assert(!HudCurrencyTallyRelease(&tally, 40));
    assert(!tally.held && !tally.ticking);
}

static void TestMissionSaveRoundTrip(void) {
    MissionTestLoadTable();
    PlayerMissions saved = {0};
    saved.counters[MISSION_STAT_GOLD_EARNED] = 2500000000LL;
    saved.last_battle_day = 125300;
    MissionRaiseProgress(&saved, 3, 100);
    saved.claimed[3] = true;
    MissionRaiseProgress(&saved, 0, 1);

    cJSON *json = ProgressionSaveMissions(&saved);
    assert(json);
    PlayerMissions loaded;
    ProgressionLoadMissions(json, &loaded);
    assert(loaded.counters[MISSION_STAT_GOLD_EARNED] == 2500000000LL);
    assert(loaded.last_battle_day == 125300);
    assert(loaded.claimed[3] && !loaded.claimed[0] && MissionClaimable(&loaded, 0));
    cJSON_Delete(json);

    ProgressionLoadMissions(NULL, &loaded);
    assert(loaded.progress[3] == 0 && !loaded.claimed[3]);
}

#define GEAR_TEST_MAX_EFFECTS 16

static GearEffect gear_test_effects[GEAR_TEST_MAX_EFFECTS];
static PassiveMatchTerm gear_test_terms[GEAR_TEST_MAX_EFFECTS];
static int gear_test_effect_count;

static void GearTestReset(void) {
    GearCatalogEnsure();
    static int shipped_count;
    if (!shipped_count) shipped_count = gear_catalog.count;
    gear_catalog.count = shipped_count;
    gear_test_effect_count = 0;
}

static int GearTestDef(GearStat stat, int value, GearTrigger on, PassiveScope scope, GearBasis basis) {
    assert(gear_test_effect_count < GEAR_TEST_MAX_EFFECTS && gear_catalog.count < GEAR_CATALOG_CAPACITY);
    GearEffect *effect = &gear_test_effects[gear_test_effect_count++];
    *effect = (GearEffect){.stat = stat, .value = value, .on = on, .basis = basis};
    effect->recipient.scope = scope;
    effect->recipient.include_source = true;
    effect->wearer.scope = PASSIVE_SCOPE_SELF;
    GearDef *def = &gear_catalog.defs[gear_catalog.count];
    memset(def, 0, sizeof(*def));
    snprintf(def->id, sizeof(def->id), "test_gear_%d", gear_catalog.count);
    def->rarity = 1;
    def->effects = effect;
    def->effect_count = 1;
    return gear_catalog.count++;
}

static PassiveFilter GearTestGroupFilter(const char *group) {
    PassiveMatchTerm *term = &gear_test_terms[gear_test_effect_count];
    term->kind = PASSIVE_TERM_GROUP;
    StrCopy(term->value, sizeof(term->value), group);
    return (PassiveFilter){.has_match = true, .terms = term, .term_count = 1, .include_source = true};
}

static void GearTestUnit(BattleSim *s, Character *ch, Ability *ability, int side, int slot, int health) {
    char id[32];
    snprintf(id, sizeof(id), "gear_unit_%d_%d", side, slot);
    GearLoadout loadout = ch->gear;
    TestCharacter(ch, ability, id, "ABILITY_DAMAGE_ONE", TARGET_ONE, 1000, health);
    ch->gear = loadout;
    SimUnitInit(s, side, slot, ch);
}

static void GearTestSwing(BattleSim *s, int actor) {
    SimAttack(s, actor, &SIM_RECIPE_DEFAULT, &(AttackRecipe){.target = RECIPE_TARGET_ABILITY}, true, false);
}

static void TestGearSlotsFollowRarity(void) {
    Character ch = {.gear_slots = 4, .has_prestige_slot = true};
    assert(GearSlotCount(&ch) == 4 && GearHasPrestigeSlot(&ch));
    assert(!GearSlotIsPrestige(&ch, 2) && GearSlotIsPrestige(&ch, 3));
    ch.gear_slots = 5;
    assert(GearSlotIsPrestige(&ch, 4) && !GearSlotIsPrestige(&ch, 3));
    ch.gear_slots = 3;
    assert(!GearHasPrestigeSlot(&ch) && !GearSlotIsPrestige(&ch, 2));
    ch.gear_slots = 0;
    assert(GearSlotCount(&ch) == 0);
}

static void TestGearEquipRules(void) {
    GearTestReset();
    PlayerGear gear = {0};
    Character warrior = {.gear_slots = 4, .has_prestige_slot = true, .group_count = 1};
    StrCopy(warrior.groups[0], sizeof(warrior.groups[0]), "Warrior");

    int shield = GearDefIndex("templar_shield");
    int plate = GearDefIndex("winterweight");
    int warden_plate = GearDefIndex("griffon_plate");
    int common = GearDefIndex("red_iron_maul");
    assert(shield >= 0 && plate >= 0 && warden_plate >= 0 && common >= 0);

    assert(GearCheckEquip(&gear, &warrior, 0, common) == GEAR_EQUIP_NOT_OWNED);
    gear.counts[common] = 2;
    gear.counts[shield] = gear.counts[plate] = gear.counts[warden_plate] = 1;
    assert(GearCheckEquip(&gear, &warrior, 9, common) == GEAR_EQUIP_INVALID_SLOT);
    assert(GearCheckEquip(&gear, &warrior, 0, warden_plate) == GEAR_EQUIP_CATEGORY);

    assert(GearEquip(&gear, &warrior, 0, common) == GEAR_EQUIP_OK);
    assert(gear.counts[common] == 1);
    assert(GearCheckEquip(&gear, &warrior, 1, common) == GEAR_EQUIP_DUPLICATE);
    assert(GearCheckEquip(&gear, &warrior, 0, common) == GEAR_EQUIP_OK);

    assert(GearEquip(&gear, &warrior, 1, shield) == GEAR_EQUIP_OK);
    assert(GearCheckEquip(&gear, &warrior, 2, plate) == GEAR_EQUIP_AURAS_CANNOT_STACK);
    assert(GearEquip(&gear, &warrior, 1, plate) == GEAR_EQUIP_OK);
    assert(GearInSlot(&warrior, 1) == GearDefAt(plate));
    assert(gear.counts[shield] == 0);

    warrior.specialties[warrior.specialty_count++] = SPEC_DEFENDER;
    Character defender = warrior;
    memset(&defender.gear, 0, sizeof(defender.gear));
    gear.counts[plate] = 1;
    assert(GearCheckEquip(&gear, &defender, 0, plate) == GEAR_EQUIP_AURAS_CANNOT_STACK);

    int legendary = GearDefIndex("fadesbane");
    int low = GearDefIndex("lotherings_lament");
    gear.counts[legendary] = gear.counts[low] = 1;
    assert(GearCheckEquip(&gear, &warrior, 3, low) == GEAR_EQUIP_PRESTIGE_LOCKED);
    warrior.gear.prestige_unlocked = true;
    assert(GearCheckEquip(&gear, &warrior, 3, low) == GEAR_EQUIP_INVALID_PRESTIGE);
    assert(GearCheckEquip(&gear, &warrior, 3, legendary) == GEAR_EQUIP_OK);
}

static void TestGearCategoriesUseOrAndAllSemantics(void) {
    GearTestReset();
    const GearDef *bianca = GearDefAt(GearDefIndex("bianca"));
    assert(bianca && bianca->categories_all && bianca->category_count == 2);
    Character dwarf = {.faction = FACTION_RED, .group_count = 1};
    StrCopy(dwarf.groups[0], sizeof(dwarf.groups[0]), "Dwarf");
    assert(GearUnitCanWear(&dwarf, bianca));
    dwarf.faction = FACTION_BLUE;
    assert(!GearUnitCanWear(&dwarf, bianca));

    const GearDef *maul = GearDefAt(GearDefIndex("maul_of_tacitus"));
    assert(maul && !maul->categories_all && maul->category_count == 2);
    Character tevinter = {.group_count = 1};
    StrCopy(tevinter.groups[0], sizeof(tevinter.groups[0]), "Tevinter");
    assert(GearUnitCanWear(&tevinter, maul));
}

static void TestGearSellValueAndPrestigeUnlock(void) {
    GearTestReset();
    Player p = {.gems = GearPrestigeSlotGemCost() - 1};
    Character legend = {.gear_slots = 4, .has_prestige_slot = true};
    assert(!PlayerUnlockPrestigeGearSlot(&p, &legend) && !legend.gear.prestige_unlocked);
    p.gems = GearPrestigeSlotGemCost();
    assert(PlayerUnlockPrestigeGearSlot(&p, &legend) && legend.gear.prestige_unlocked && p.gems == 0);
    assert(!PlayerUnlockPrestigeGearSlot(&p, &legend));

    int item = GearDefIndex("red_iron_maul");
    assert(!PlayerSellGear(&p, item));
    p.gear.counts[item] = 1;
    assert(PlayerSellGear(&p, item) && p.gear.counts[item] == 0);
    assert(p.gold == GearSellValue(GearDefAt(item)) && p.gold > 0);
}

static void TestGearPacksRollInsideTheirRarityRange(void) {
    GearTestReset();
    assert(GearPackCount() == GEAR_PACK_CAPACITY);
    uint32_t rng = 12345;
    for (int p = 0; p < GearPackCount(); p++) {
        const GearPackDef *pack = GearPackAt(p);
        for (int roll = 0; roll < 200; roll++) {
            const GearDef *def = GearDefAt(GearRollFromPack(pack, &rng));
            assert(def && def->rarity >= pack->min_rarity && def->rarity <= pack->max_rarity);
        }
    }
    Player broke = {.gold = 0};
    int rolled[GEAR_PACK_MAX_ROLLS];
    assert(!PlayerBuyGearPack(&broke, GearPackAt(0), rolled));
    Player rich = {.gold = GearPackAt(0)->price, .store_rng_state = 9};
    assert(PlayerBuyGearPack(&rich, GearPackAt(0), rolled));
    assert(rich.gold == 0 && GearInventoryTotal(&rich.gear) == GearPackAt(0)->count);
}

static void TestGearStatAurasAddThenMultiply(void) {
    GearTestReset();
    int a = GearTestDef(GEAR_STAT_POWER, 10, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int b = GearTestDef(GEAR_STAT_POWER, 10, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int weaken = GearTestDef(GEAR_STAT_HEALTH, -50, GEAR_ON_BATTLE, PASSIVE_SCOPE_ENEMIES, GEAR_BASIS_BASE);
    int crit = GearTestDef(GEAR_STAT_CRIT, 7, GEAR_ON_BATTLE, PASSIVE_SCOPE_ALLIES, GEAR_BASIS_BASE);

    BattleSim s = {0};
    Character wearer = {0}, ally = {0}, enemy = {0};
    Ability abilities[3];
    wearer.gear.defs[0] = (short)(a + 1);
    wearer.gear.defs[1] = (short)(b + 1);
    wearer.gear.defs[2] = (short)(weaken + 1);
    wearer.gear.defs[3] = (short)(crit + 1);
    GearTestUnit(&s, &wearer, &abilities[0], 0, 2, 10000);
    GearTestUnit(&s, &ally, &abilities[1], 0, 3, 10000);
    GearTestUnit(&s, &enemy, &abilities[2], 1, 2, 10000);
    float ally_crit = s.units[3].crit_pct;
    SimUnitsBattleStartPassives(s.units);

    assert(s.units[2].power == 1200 && s.units[2].start_power == 1200);
    assert(s.units[3].power == 1000);
    assert(s.units[SQUAD_SLOT_COUNT + 2].health == s.units[SQUAD_SLOT_COUNT + 2].max_health);
    assert(s.units[SQUAD_SLOT_COUNT + 2].max_health == CharacterStatHealth(&enemy) / 2);
    assert(s.units[3].crit_pct == ally_crit + 7.0f);
    GearTestReset();
}

static void TestGearAuraRespectsGroupAndWearerFilters(void) {
    GearTestReset();
    int aura = GearTestDef(GEAR_STAT_POWER, 50, GEAR_ON_BATTLE, PASSIVE_SCOPE_ALLIES, GEAR_BASIS_BASE);
    gear_test_effects[gear_test_effect_count - 1].recipient =
        (PassiveFilter){.scope = PASSIVE_SCOPE_ALLIES, .include_source = true};
    PassiveFilter dwarves = GearTestGroupFilter("Dwarf");
    dwarves.scope = PASSIVE_SCOPE_ALLIES;
    gear_test_effects[gear_test_effect_count - 1].recipient = dwarves;

    BattleSim s = {0};
    Character wearer = {0}, dwarf = {0};
    Ability abilities[2];
    wearer.gear.defs[0] = (short)(aura + 1);
    GearTestUnit(&s, &wearer, &abilities[0], 0, 2, 10000);
    GearTestUnit(&s, &dwarf, &abilities[1], 0, 3, 10000);
    dwarf.group_count = 1;
    StrCopy(dwarf.groups[0], sizeof(dwarf.groups[0]), "Dwarf");
    SimUnitsBattleStartPassives(s.units);
    assert(s.units[2].power == 1000 && s.units[3].power == 1500);

    int personal = GearTestDef(GEAR_STAT_POWER, 50, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    PassiveFilter dwarf_only = GearTestGroupFilter("Dwarf");
    gear_test_effects[gear_test_effect_count - 1].wearer = dwarf_only;
    BattleSim second = {0};
    Character human = {0};
    Ability human_ability;
    human.gear.defs[0] = (short)(personal + 1);
    GearTestUnit(&second, &human, &human_ability, 0, 2, 10000);
    SimUnitsBattleStartPassives(second.units);
    assert(second.units[2].power == 1000);
    GearTestReset();
}

static void TestGearDamageMitigationAndVersus(void) {
    GearTestReset();
    int resist = GearTestDef(GEAR_STAT_DAMAGE_RESIST, 50, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int raw = GearTestDef(GEAR_STAT_RAW_DAMAGE_RESIST, 100, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int versus = GearTestDef(GEAR_STAT_BONUS_DAMAGE, 40, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    gear_test_effects[gear_test_effect_count - 1].versus = GearTestGroupFilter("Mage");

    BattleSim s = {0};
    Character attacker = {0}, target = {0};
    Ability abilities[2];
    attacker.gear.defs[0] = (short)(versus + 1);
    target.gear.defs[0] = (short)(resist + 1);
    target.gear.defs[1] = (short)(raw + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 10000);
    GearTestUnit(&s, &target, &abilities[1], 1, 2, 10000);
    SimUnitsBattleStartPassives(s.units);
    s.units[2].crit_pct = 0.0f;
    s.rng = 11;

    int idx = SQUAD_SLOT_COUNT + 2;
    assert(SimMitigatedDamage(&s, &s.units[2], &s.units[idx], 1000) == 450);
    int before = s.units[idx].health;
    GearTestSwing(&s, 2);
    assert(before - s.units[idx].health == 450);

    target.group_count = 1;
    StrCopy(target.groups[0], sizeof(target.groups[0]), "Mage");
    before = s.units[idx].health;
    GearTestSwing(&s, 2);
    assert(before - s.units[idx].health == 650);
    GearTestReset();
}

static void TestGearResistsScaleAilments(void) {
    GearTestReset();
    int immune = GearTestDef(GEAR_STAT_STUN_RESIST, 100, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int fragile = GearTestDef(GEAR_STAT_STUN_RESIST, -50, GEAR_ON_BATTLE, PASSIVE_SCOPE_ENEMIES, GEAR_BASIS_BASE);
    BattleSim s = {.rng = 3};
    for (int i = 0; i < 200; i++) assert(SimEffectLands(&s, 50, 100) == false);
    for (int i = 0; i < 200; i++) assert(SimEffectLands(&s, 50, -50) == true);
    assert(!SimEffectLands(&s, 0, -50));

    Character attacker = {0}, target = {0};
    Ability abilities[2];
    attacker.gear.defs[0] = (short)(fragile + 1);
    target.gear.defs[0] = (short)(immune + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 10000);
    GearTestUnit(&s, &target, &abilities[1], 1, 2, 10000);
    SimUnitsBattleStartPassives(s.units);
    assert(s.units[SQUAD_SLOT_COUNT + 2].gear.stun_resist_pct == 50);
    GearTestReset();
}

static void TestGearOnHitReactionsAndStealth(void) {
    GearTestReset();
    int power = GearTestDef(GEAR_STAT_POWER, 10, GEAR_ON_SELF_HIT, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int convert = GearTestDef(GEAR_STAT_HEALTH, 50, GEAR_ON_SELF_HIT, PASSIVE_SCOPE_SELF, GEAR_BASIS_DAMAGE);
    int stealth = GearTestDef(GEAR_STAT_STEALTH, 1, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int perceptive = GearTestDef(GEAR_STAT_PERCEPTIVE, 1, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);

    BattleSim s = {.rng = 5};
    Character attacker = {0}, target = {0};
    Ability abilities[2];
    target.gear.defs[0] = (short)(power + 1);
    target.gear.defs[1] = (short)(convert + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
    GearTestUnit(&s, &target, &abilities[1], 1, 2, 100000);
    SimUnitsBattleStartPassives(s.units);
    s.units[2].crit_pct = 0.0f;
    int idx = SQUAD_SLOT_COUNT + 2;

    GearTestSwing(&s, 2);
    assert(s.units[idx].power == 1100);
    assert(s.units[idx].max_health - s.units[idx].health == 500);

    attacker.gear.defs[0] = (short)(stealth + 1);
    SimUnitsGearAuras(s.units, (float[SIM_UNIT_COUNT]){0}, (float[SIM_UNIT_COUNT]){0});
    GearTestSwing(&s, 2);
    assert(s.units[idx].power == 1100);

    target.gear.defs[2] = (short)(perceptive + 1);
    attacker.gear.defs[0] = 0;
    s.units[idx].gear.perceptive = true;
    s.units[2].gear.stealth = true;
    GearTestSwing(&s, 2);
    assert(s.units[idx].power == 1200);
    GearTestReset();
}

static void TestGearReactionsRespectCurses(void) {
    GearTestReset();
    int power = GearTestDef(GEAR_STAT_POWER, 10, GEAR_ON_SELF_HIT, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    BattleSim s = {.rng = 5};
    Character attacker = {0}, target = {0};
    Ability abilities[2];
    target.gear.defs[0] = (short)(power + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
    GearTestUnit(&s, &target, &abilities[1], 1, 2, 100000);
    s.units[2].crit_pct = 0.0f;
    s.units[SQUAD_SLOT_COUNT + 2].curse_power = true;
    GearTestSwing(&s, 2);
    assert(s.units[SQUAD_SLOT_COUNT + 2].power == 1000);
    GearTestReset();
}

static void TestGearDeathTriggersFireForDeadWearer(void) {
    GearTestReset();
    int heal = GearTestDef(GEAR_STAT_HEALTH, 60, GEAR_ON_SELF_DEATH, PASSIVE_SCOPE_ALLIES, GEAR_BASIS_BASE);
    int vengeful = GearTestDef(GEAR_STAT_POWER, 25, GEAR_ON_ALLY_DEATH, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);

    BattleSim s = {.rng = 5};
    Character attacker = {0}, doomed = {0}, ally = {0};
    Ability abilities[3];
    doomed.gear.defs[0] = (short)(heal + 1);
    ally.gear.defs[0] = (short)(vengeful + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
    GearTestUnit(&s, &doomed, &abilities[1], 1, 2, 10);
    GearTestUnit(&s, &ally, &abilities[2], 1, 3, 10000);
    s.units[2].crit_pct = 0.0f;
    int ally_idx = SQUAD_SLOT_COUNT + 3;
    s.units[ally_idx].health = 1000;
    GearTestSwing(&s, 2);

    assert(!s.units[SQUAD_SLOT_COUNT + 2].alive);
    assert(s.units[ally_idx].health == 1000 + s.units[ally_idx].max_health * 60 / 100);
    assert(s.units[ally_idx].power == 1250);
    assert(s.gear_pending_deaths == 0);
    GearTestReset();
}

static void TestGearReactionDamageCanEndAChain(void) {
    GearTestReset();
    int explode = GearTestDef(GEAR_STAT_DAMAGE, 100, GEAR_ON_ENEMY_HIT, PASSIVE_SCOPE_ENEMIES, GEAR_BASIS_DAMAGE);
    int chain = GearTestDef(GEAR_STAT_DAMAGE, 100, GEAR_ON_ENEMY_DEATH, PASSIVE_SCOPE_ENEMIES, GEAR_BASIS_DAMAGE);
    BattleSim s = {.rng = 5};
    Character attacker = {0}, first = {0}, second = {0};
    Ability abilities[3];
    attacker.gear.defs[0] = (short)(explode + 1);
    attacker.gear.defs[1] = (short)(chain + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
    GearTestUnit(&s, &first, &abilities[1], 1, 2, 10);
    GearTestUnit(&s, &second, &abilities[2], 1, 3, 10);
    s.units[2].crit_pct = 0.0f;
    GearTestSwing(&s, 2);
    assert(SimTeamDead(&s, 1));
    GearTestReset();
}

static void TestGearDefenderAbsorbsAoeShare(void) {
    GearTestReset();
    int absorb = GearTestDef(GEAR_STAT_DEFENDER, 10, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    BattleSim s = {.rng = 5};
    Character attacker = {0}, tank = {0}, squishy = {0};
    Ability abilities[3];
    tank.gear.defs[0] = (short)(absorb + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
    GearTestUnit(&s, &tank, &abilities[1], 1, 2, 100000);
    GearTestUnit(&s, &squishy, &abilities[2], 1, 3, 100000);
    SimUnitsBattleStartPassives(s.units);
    s.units[2].crit_pct = 0.0f;
    abilities[0].target = TARGET_ALL;
    GearTestSwing(&s, 2);
    assert(s.units[SQUAD_SLOT_COUNT + 2].max_health - s.units[SQUAD_SLOT_COUNT + 2].health == 1100);
    assert(s.units[SQUAD_SLOT_COUNT + 3].max_health - s.units[SQUAD_SLOT_COUNT + 3].health == 900);
    GearTestReset();
}

static void TestGearTargetPreferences(void) {
    GearTestReset();
    int flank = GearTestDef(GEAR_STAT_TARGET_BACK_ROW, 1, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int reduce = GearTestDef(GEAR_STAT_FLANK_REDUCTION, 1, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    int haste = GearTestDef(GEAR_STAT_HASTE, 1, GEAR_ON_BATTLE, PASSIVE_SCOPE_SELF, GEAR_BASIS_BASE);
    BattleSim s = {.rng = 21};
    Character attacker = {0}, back = {0};
    Ability abilities[2];
    attacker.gear.defs[0] = (short)(flank + 1);
    attacker.gear.defs[1] = (short)(haste + 1);
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 10000);
    GearTestUnit(&s, &back, &abilities[1], 1, 2, 10000);
    assert(SquadSlotIsBack(2));
    SimUnitsBattleStartPassives(s.units);
    assert(SimFlankWeight(&s.units[2], &s.units[SQUAD_SLOT_COUNT + 2]) == SIM_GEAR_FLANK_WEIGHT);
    back.gear.defs[0] = (short)(reduce + 1);
    s.units[SQUAD_SLOT_COUNT + 2].gear.flank_reduction = true;
    assert(SimFlankWeight(&s.units[2], &s.units[SQUAD_SLOT_COUNT + 2]) < SIM_GEAR_FLANK_WEIGHT);

    long hasted = 0, plain = 0;
    SimUnit normal = s.units[2];
    normal.gear.haste = false;
    for (int i = 0; i < 2000; i++) {
        hasted += SimRollInitiative(&s, &s.units[2]);
        plain += SimRollInitiative(&s, &normal);
    }
    assert(hasted > plain);
    GearTestReset();
}

static void TestGearlessFightsAreUntouched(void) {
    BattleSim s = {.rng = 5};
    Character attacker = {0}, target = {0};
    Ability abilities[2];
    GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
    GearTestUnit(&s, &target, &abilities[1], 1, 2, 100000);
    s.units[2].crit_pct = 0.0f;
    GearTestSwing(&s, 2);
    assert(s.action_count == 1);
}

static void TestEveryCatalogItemFightsCleanly(void) {
    GearTestReset();
    for (int def = 0; def < GearCatalogCount(); def++) {
        BattleSim s = {.rng = (unsigned)def + 1};
        Character attacker = {0}, target = {0}, ally = {0};
        Ability abilities[3];
        attacker.gear.defs[0] = (short)(def + 1);
        target.gear.defs[0] = (short)(def + 1);
        GearTestUnit(&s, &attacker, &abilities[0], 0, 2, 100000);
        GearTestUnit(&s, &target, &abilities[1], 1, 2, 100000);
        GearTestUnit(&s, &ally, &abilities[2], 1, 3, 100000);
        SimUnitsBattleStartPassives(s.units);
        for (int swing = 0; swing < 4; swing++) {
            GearTestSwing(&s, 2);
            GearTestSwing(&s, SQUAD_SLOT_COUNT + 2);
        }
        for (int i = 0; i < SIM_UNIT_COUNT; i++) {
            const SimUnit *u = &s.units[i];
            assert(u->health >= 0 && u->health <= u->max_health && u->power >= 0);
            assert(u->crit_pct == u->crit_pct && u->damage_bonus_pct == u->damage_bonus_pct);
        }
    }
}

static void TestRichTextReadsHexColors(void) {
    Color color = HeroesRichColor("#ff751a", WHITE);
    assert(color.r == 255 && color.g == 117 && color.b == 26);
    color = HeroesRichColor("#ffa500ff", WHITE);
    assert(color.r == 255 && color.g == 165 && color.b == 0);
    color = HeroesRichColor("#bad", WHITE);
    assert(color.r == 255 && color.g == 255 && color.b == 255);
}

static void TestGearTextFitsItsBuffersInEveryLanguage(void) {
    Loc saved = loc;
    char aura[GEAR_VIEW_AURA_CAPACITY];
    char details[GEAR_VIEW_DETAILS_CAPACITY];
    char limits[192];
    for (int language = 0; language < LocLanguageCount(); language++) {
        loc = (Loc){0};
        LocLoad(TextFormat("%s/%s", ORIGINAL_LOCALIZATION_DIR, LocLanguageAt(language)->file));
        const char *message = LocGet("HODA_GEAR_PRESTIGE_MESSAGE");
        assert(message[0] && strlen(message) < GEAR_VIEW_MESSAGE_CAPACITY);
        assert(LocGet("HODA_RARITY_MYTHIC")[0] && LocGet("HODA_GEAR_REVEAL_WINDOW_TITLE")[0]);
        for (int i = 0; i < GearCatalogCount(); i++) {
            const GearDef *def = GearDefAt(i);
            GearFormatAura(def, aura, sizeof(aura));
            assert(GearTitle(def)[0] && GearDescription(def)[0] && aura[0] && strlen(aura) < sizeof(aura) - 1);
            GearInfoText info = GearViewInfoTextFor(def, limits, sizeof(limits), details, sizeof(details));
            assert(strlen(info.description) < sizeof(details) - 1);
        }
        LocUnload();
    }
    loc = (Loc){0};
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    const GearDef *hammer = GearDefAt(GearDefIndex("aeducans_hammer"));
    assert(!strcmp(GearTitle(hammer), "Молот Эдукана"));
    assert(!strcmp(LocGet("HODA_RARITY_MYTHIC"), "Мифический"));
    LocUnload();
    loc = saved;
}

static void TestGearCatalogIsComplete(void) {
    GearTestReset();
    assert(GearCatalogCount() == 266);
    for (int i = 0; i < GearCatalogCount(); i++) {
        const GearDef *def = GearDefAt(i);
        assert(def->title_key[0] && def->description_key[0] && def->aura_key_count > 0 && def->icon[0]);
        assert(def->rarity >= 1 && def->rarity <= GEAR_RARITY_COUNT);
        assert(GearSellValue(def) > 0);
    }
    assert(GearDefIndex("no_such_gear") == -1);
}

static void TestConsumeWarnsAboutEquippedGear(void) {
    ConsumeScreen c = {0};
    Character plain = {.rarity = RARITY_COMMON}, geared = {.rarity = RARITY_COMMON};
    assert(!ConsumeSeedNeedsWarning(&c, &plain));
    geared.gear.defs[0] = 1;
    assert(ConsumeSeedNeedsWarning(&c, &geared));
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    assert(!strcmp(ConsumeSeedWarning(&c, &geared), "Снаряжение этого героя будет уничтожено. Вы уверены?"));
    LocUnload();

    Character fortified = plain;
    StrCopy(fortified.fortify[3].id, sizeof(fortified.fortify[3].id), "UNIT_OGRE");
    assert(ConsumeSeedNeedsWarning(&c, &fortified));
    assert(ConsumeSeedWarning(&c, &fortified) == LocGet("CONSUME_CONFIRMATION_FORTIFIED_WARNING"));
    c.fortified_warning_disabled = true;
    assert(!ConsumeSeedNeedsWarning(&c, &fortified));
}

static void TestGearViewEquipFlow(void) {
    GearTestReset();
    setenv("HOD_NO_SAVE", "1", 1);
    Loc saved = loc;
    loc = (Loc){0};
    LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    Character roster[1];
    memset(roster, 0, sizeof(roster));
    roster[0].gear_slots = 2;
    Player p = {.roster = roster, .roster_count = 1};
    int cheap = GearDefIndex("red_iron_maul");
    int other = GearDefIndex("lotherings_lament");
    p.gear.counts[cheap] = p.gear.counts[other] = 1;
    int items[GEAR_CATALOG_CAPACITY];

    GearViewShow(0, 0);
    assert(gear_view.filter == GEAR_FILTER_EQUIPPABLE);
    assert(GearViewFilteredItems(&p, GEAR_FILTER_EQUIPPABLE, false, items) == 2);
    assert(GearViewFilteredItems(&p, GEAR_FILTER_MYTHIC, false, items) == 0);
    gear_view.selected = cheap;
    GearViewRequestEquip(&p);
    assert(gear_view.dialog == GEAR_VIEW_DIALOG_EQUIP);
    GearViewConfirm(&p);
    assert(GearInSlot(&roster[0], 0) == GearDefAt(cheap) && !gear_view.open);

    GearViewShow(0, 0);
    gear_view.selected = other;
    GearViewRequestEquip(&p);
    assert(gear_view.dialog == GEAR_VIEW_DIALOG_EQUIP && strstr(gear_view.message, "Молот Кровавых Клинков"));
    GearViewConfirm(&p);
    assert(GearInSlot(&roster[0], 0) == GearDefAt(other) && p.gear.counts[cheap] == 0);

    GearViewOpenInventory();
    assert(gear_view.filter == GEAR_FILTER_ALL);
    assert(GearViewFilteredItems(&p, GEAR_FILTER_EQUIPPABLE, false, items) == 0);
    GearViewClose();
    assert(!GearViewIsOpen());
    LocUnload();
    loc = saved;
    unsetenv("HOD_NO_SAVE");
}

static void GearTestLegend(Character *ch) {
    memset(ch, 0, sizeof(*ch));
    ch->rarity = RARITY_LEGENDARY;
    ch->gear_slots = 4;
    ch->has_prestige_slot = true;
    ch->stats[TIER_I].max_level = 80;
    ch->stats[TIER_IV].max_level = 90;
    ch->has_tier[TIER_I] = true;
}

static void TestAscensionRequiresFilledSlotsAndTome(void) {
    GearTestReset();
    Character legend;
    GearTestLegend(&legend);
    PlayerGear gear = {.tomes = 1};
    assert(GearHasTomeSlot(&legend) && !GearCanAscend(&legend));
    assert(!GearAscend(&gear, &legend) && gear.tomes == 1 && legend.rarity == RARITY_LEGENDARY);

    legend.gear.defs[0] = 1;
    legend.gear.defs[1] = 2;
    assert(!GearCanAscend(&legend));
    legend.gear.defs[2] = 3;
    assert(GearCanAscend(&legend));
    gear.tomes = 0;
    assert(!GearAscend(&gear, &legend) && legend.rarity == RARITY_LEGENDARY);

    Character epic = legend;
    epic.rarity = RARITY_EPIC;
    assert(!GearHasTomeSlot(&epic));
}

static void TestAscensionPromotesHeroAndMovesPrestigeGear(void) {
    GearTestReset();
    Character legend;
    GearTestLegend(&legend);
    PlayerGear gear = {.tomes = 2};
    legend.gear.prestige_unlocked = true;
    for (int slot = 0; slot < 4; slot++) legend.gear.defs[slot] = (short)(slot + 1);

    assert(GearAscend(&gear, &legend));
    assert(gear.tomes == 1 && legend.ascended && legend.rarity == RARITY_MYTHIC);
    assert(GearSlotCount(&legend) == 5 && !GearHasTomeSlot(&legend));
    assert(legend.stats[TIER_I].max_level == 85 && legend.stats[TIER_IV].max_level == 95);
    assert(GearSlotIsPrestige(&legend, 4) && !GearSlotIsPrestige(&legend, 3));
    assert(legend.gear.defs[4] == 4 && legend.gear.defs[3] == GEAR_EMPTY_SLOT);
    assert(!GearAscend(&gear, &legend) && gear.tomes == 1);

    GearApplyAscension(&legend);
    assert(legend.stats[TIER_I].max_level == 85 && GearSlotCount(&legend) == 5);
}

static void TestAscensionSurvivesSave(void) {
    GearTestReset();
    Character roster[1];
    GearTestLegend(&roster[0]);
    roster[0].instance_id = 7;
    roster[0].gear.defs[0] = roster[0].gear.defs[1] = roster[0].gear.defs[2] = 1;
    Player saved = {.roster = roster, .roster_count = 1, .gear = {.tomes = 3}};
    assert(GearAscend(&saved.gear, &roster[0]));

    cJSON *json = ProgressionSaveGear(&saved);
    assert(json);
    GearTestLegend(&roster[0]);
    roster[0].instance_id = 7;
    saved.gear.tomes = 0;
    ProgressionLoadGear(json, &saved);
    cJSON_Delete(json);
    assert(saved.gear.tomes == 2 && roster[0].ascended && roster[0].rarity == RARITY_MYTHIC);
    assert(roster[0].stats[TIER_I].max_level == 85);
}

static void TestGearPackBundlesATome(void) {
    GearTestReset();
    GearPackDef pack = *GearPackAt(GearPackCount() - 1);
    assert(pack.tome_chance_pct > 0);
    pack.tome_chance_pct = 100;
    Player p = {.gems = pack.price, .store_rng_state = 9};
    int rolled[GEAR_PACK_MAX_ROLLS];
    assert(PlayerBuyGearPack(&p, &pack, rolled));
    assert(p.gear.tomes == 1 && p.gems == 0 && rolled[pack.count] == GEAR_ROLL_TOME);
    assert(GearInventoryTotal(&p.gear) == pack.count);
    pack.tome_chance_pct = 0;
    p.gems = pack.price;
    assert(PlayerBuyGearPack(&p, &pack, rolled));
    assert(p.gear.tomes == 1 && rolled[pack.count] == GEAR_ROLL_NONE);
}

static void TestSeasonalPackFollowsTheCalendar(void) {
    StorePackDef winter = {.flash_index = -1, .schedule_start = 1210, .schedule_end = 115};
    struct tm inside = {.tm_year = 126, .tm_mon = 11, .tm_mday = 20, .tm_hour = 12, .tm_isdst = -1};
    struct tm january = {.tm_year = 127, .tm_mon = 0, .tm_mday = 10, .tm_hour = 12, .tm_isdst = -1};
    struct tm outside = {.tm_year = 126, .tm_mon = 5, .tm_mday = 1, .tm_hour = 12, .tm_isdst = -1};
    long long december_left = StorePackSecondsLeft(&winter, 7, 12, (long long)mktime(&inside));
    assert(december_left > 26 * STORE_SECONDS_PER_DAY && december_left < 28 * STORE_SECONDS_PER_DAY);
    assert(StorePackSecondsLeft(&winter, 7, 12, (long long)mktime(&january)) > 0);
    assert(StorePackSecondsLeft(&winter, 7, 12, (long long)mktime(&outside)) == STORE_PACK_UNAVAILABLE);
    StorePackDef always = {.flash_index = -1};
    assert(StorePackSecondsLeft(&always, 7, 12, (long long)mktime(&outside)) == STORE_PACK_ALWAYS);
}

static void TestFlashPacksRotateOnePerPeriod(void) {
    long long span = 7 * STORE_SECONDS_PER_DAY;
    long long now = span * 25 + 100;
    int active = 0;
    for (int index = 0; index < 12; index++) {
        StorePackDef flash = {.flash_index = index};
        long long left = StorePackSecondsLeft(&flash, 7, 12, now);
        if (left != STORE_PACK_UNAVAILABLE) {
            active++;
            assert(left == span - 100);
        }
    }
    assert(active == 1);
}

static void TestStoreDialogTextFitsRussianPackNames(void) {
    char header[STORE_DIALOG_TEXT_CAPACITY];
    StoreFormatPackName(header, sizeof(header), "{0}: недоступно", "Усилитель \"черных\" и \"белых\" героев");
    assert(!strcmp(header, "Усилитель \"черных\" и \"белых\" героев: недоступно"));
    StoreFormatPackName(header, sizeof(header), "без подстановки", "Набор");
    assert(!strcmp(header, "без подстановки"));
}

static void TestVoucherOfferWinsOnlyWhenOwned(void) {
    StorePackDef pack = {.has_paid_offer = true, .paid = {.voucher = -1, .gems = true, .price = 30}, .voucher = 3};
    Player player = {0};
    assert(StoreActiveOffer(&pack, &player).voucher == -1);
    player.pack_vouchers[3] = 1;
    assert(StoreActiveOffer(&pack, &player).voucher == 3);
    StoreOfferPay(StoreActiveOffer(&pack, &player), &player);
    assert(player.pack_vouchers[3] == 0);
    pack.has_paid_offer = false;
    StoreOffer find = StoreActiveOffer(&pack, &player);
    assert(find.voucher == 3 && !StoreOfferAffordable(find, &player));
}

static void TestAscendedHeroLoadsAboveTheOldLevelCap(void) {
    GearTestReset();
    const char *path = "tests/ascension_save_test.dat";
    remove(path);
    Player source = {0};
    ProgressionInitializePlayerDefaults(&source);
    PlayerLoadRoster(&source, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    cJSON *catalog_root = JsonLoadFile(HOD_CACHE "characters.json");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    const cJSON *entry = NULL, *legend = NULL;
    cJSON_ArrayForEach(entry, catalog) {
        if (!legend && !strcmp(JsonStr(entry, "rarity"), "Legendary") && JsonInt(entry, "gear_slots") == 4)
            legend = entry;
    }
    assert(legend);
    const cJSON *defs[1] = {legend};
    int index = -1;
    assert(PlayerRosterAppendDefinitions(&source, defs, 1, &index));
    Character *hero = &source.roster[index];
    int old_cap = hero->stats[hero->tier].max_level;
    GearApplyAscension(hero);
    hero->gear.defs[0] = 1;
    const ConsumeProfile *profile = ConsumeProfileFor(hero);
    assert(profile);
    hero->level = old_cap + 3;
    hero->consume_xp = ConsumeXpForLevel(profile->level_table, hero->level);
    hero->xp = hero->consume_xp;
    ConsumeNormalizeCharacter(hero);
    int saved_level = hero->level;
    assert(saved_level > old_cap);
    assert(ProgressionSaveToPath(&source, path));

    Player loaded = {0};
    ProgressionInitializePlayerDefaults(&loaded);
    PlayerLoadRoster(&loaded, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    assert(ProgressionLoadFromPath(&loaded, path));
    const Character *restored = &loaded.roster[index];
    assert(restored->ascended && restored->rarity == RARITY_MYTHIC && GearSlotCount(restored) == 5);
    assert(restored->level == saved_level && restored->stats[restored->tier].max_level == old_cap + 5);
    assert(restored->gear.defs[0] == 1);
    cJSON_Delete(catalog_root);
    remove(path);
}

static void TestHeroLevelTableCoversAscendedCap(void) {
    Character hero = {0};
    StrCopy(hero.id, sizeof(hero.id), "UNIT_MERRILL");
    const ConsumeProfile *profile = ConsumeProfileFor(&hero);
    assert(profile && profile->level_table->count >= CONSUME_ASCENDED_LEVEL_CAP);
    for (int i = 1; i < profile->level_table->count; i++)
        assert(profile->level_table->xp[i] >= profile->level_table->xp[i - 1]);
}

static void TestBoardIgnoresClicksBehindGearAndDetails(void) {
    Heroes heroes = {.selected = -1};
    GearViewClose();
    assert(!HeroesBoardInputBlocked(&heroes));
    heroes.selected = 0;
    assert(HeroesBoardInputBlocked(&heroes));
    heroes.selected = -1;
    GearViewOpenInventory();
    assert(HeroesBoardInputBlocked(&heroes));
    GearViewClose();
    gear_view.dialog = GEAR_VIEW_DIALOG_PRESTIGE;
    assert(HeroesBoardInputBlocked(&heroes));
    GearViewClose();
    assert(!HeroesBoardInputBlocked(&heroes));
}

static void TestGearSaveRoundTrip(void) {
    GearTestReset();
    Character roster[2];
    memset(roster, 0, sizeof(roster));
    roster[0].instance_id = 41;
    roster[1].instance_id = 42;
    Player saved = {.roster = roster, .roster_count = 2};
    int item = GearDefIndex("red_iron_maul");
    int worn = GearDefIndex("fadesbane");
    saved.gear.counts[item] = 3;
    roster[1].gear.defs[2] = (short)(worn + 1);
    roster[1].gear.prestige_unlocked = true;

    cJSON *json = ProgressionSaveGear(&saved);
    assert(json);
    memset(roster, 0, sizeof(roster));
    roster[0].instance_id = 41;
    roster[1].instance_id = 42;
    saved.gear.counts[item] = 0;
    ProgressionLoadGear(json, &saved);
    assert(saved.gear.counts[item] == 3);
    assert(GearInSlot(&roster[1], 2) == GearDefAt(worn) && roster[1].gear.prestige_unlocked);
    assert(!GearHasEquipped(&roster[0]));
    cJSON_Delete(json);

    ProgressionLoadGear(NULL, &saved);
    assert(saved.gear.counts[item] == 0 && !GearHasEquipped(&roster[1]));
}

static void TestNexusGameChangerBoostsMatchingUnitsOnBothSides(void) {
    Player player = {0};
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    BattleRoster enemy = {0};
    BattleRosterLoad(&enemy, "data/campaign_rosters.json", HOD_CACHE "characters.json", "PVE_MAP_01", 1);
    SimUnit *base = (SimUnit *)calloc(SIM_UNIT_COUNT, sizeof(*base));
    SimUnit *boosted = (SimUnit *)calloc(SIM_UNIT_COUNT, sizeof(*boosted));
    assert(base != NULL && boosted != NULL);
    BattleSimPreview(base, &player, &enemy);

    int matched = -1;
    for (int i = 0; i < SIM_UNIT_COUNT && matched < 0; i++)
        if (base[i].present && base[i].ch->group_count > 0) matched = i;
    assert(matched >= 0);
    BattleGameChanger changer = {.effect_count = 1};
    changer.effects[0] = (BattleChangerEffect){.group_count = 1, .health_pct = 150, .power_pct = 100};
    StrCopy(changer.effects[0].groups[0], sizeof(changer.effects[0].groups[0]), base[matched].ch->groups[0]);
    enemy.game_changer = &changer;
    BattleSimPreview(boosted, &player, &enemy);

    bool matched_side[2] = {false, false};
    for (int i = 0; i < SIM_UNIT_COUNT; i++) {
        if (!base[i].present) continue;
        bool matches = SimPassiveHasGroup(base[i].ch, changer.effects[0].groups[0]);
        assert(boosted[i].game_changer_applied == matches);
        int health = matches ? (int)lroundf(base[i].max_health * 2.5f) : base[i].max_health;
        int power = matches ? base[i].power * 2 : base[i].power;
        assert(boosted[i].max_health == health);
        assert(boosted[i].health == health);
        assert(boosted[i].power == power);
        assert(boosted[i].start_power == power);
        if (matches) matched_side[base[i].side] = true;
    }
    assert(matched_side[0] || matched_side[1]);
    free(base);
    free(boosted);
}

static void TestNexusGameChangerNarrowsByFactionAndUnit(void) {
    Character character;
    Ability ability;
    TestCharacter(&character, &ability, "changer_target", "ABILITY_DAMAGE_ONE", TARGET_ONE, 100, 1000);
    SimUnit unit = {.ch = &character, .faction = 2};
    BattleChangerEffect faction = {.faction_mask = 2};
    BattleChangerEffect other_faction = {.faction_mask = 4};
    assert(SimChangerEffectMatches(&faction, &unit));
    assert(!SimChangerEffectMatches(&other_faction, &unit));
    BattleChangerEffect wrong_unit = {.unit_count = 1};
    StrCopy(wrong_unit.units[0], sizeof(wrong_unit.units[0]), "UNIT_SOMEONE_ELSE");
    assert(!SimChangerEffectMatches(&wrong_unit, &unit));
    BattleChangerEffect same_unit = {.unit_count = 1};
    StrCopy(same_unit.units[0], sizeof(same_unit.units[0]), character.id);
    assert(SimChangerEffectMatches(&same_unit, &unit));
}

static void TestNexusGameChangerTableAffectsRealRosters(void) {
    Nexus *nexus = (Nexus *)calloc(1, sizeof(*nexus));
    assert(nexus != NULL);
    cJSON *table = JsonLoadFile(NEXUS_GAME_CHANGERS_PATH);
    assert(cJSON_IsArray(table));
    cJSON *maps = JsonLoadFile("data/nexus_maps.json");
    assert(cJSON_IsArray(maps));
    int event_index = 0;
    cJSON *map = NULL;
    cJSON_ArrayForEach(map, maps) {
        StrCopy(nexus->events[event_index].id, sizeof(nexus->events[0].id), JsonStr(map, "id"));
        event_index++;
    }
    nexus->event_count = event_index;
    if (loc.count == 0) LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    NexusGameChangersLoad(nexus, NEXUS_GAME_CHANGERS_PATH);
    int loaded = 0;
    for (int event = 0; event < nexus->event_count; event++) {
        const BattleGameChanger *changer = NexusEventGameChanger(nexus, event);
        if (!changer) continue;
        loaded++;
        assert(changer->text_key[0] != '\0' && NexusGameChangerText(changer)[0] != '\0');
        for (int e = 0; e < changer->effect_count; e++)
            assert(changer->effects[e].health_pct > 0 || changer->effects[e].power_pct > 0);
    }
    assert(loaded == cJSON_GetArraySize(table));
    cJSON_Delete(maps);
    cJSON_Delete(table);
    free(nexus);
}

static void TestPvpEventLeaderboardMatchesRank(void) {
    static PvpRankEntry entries[PVP_BOT_CAPACITY + 1];
    const int cycle = 7, elapsed = 100000;
    const int banners[] = {0, 320, 1500, 100000};
    for (size_t i = 0; i < sizeof(banners) / sizeof(banners[0]); i++) {
        int count = PvpEventLeaderboardBuild(entries, banners[i], cycle, elapsed);
        assert(count == PvpTableEnsure()->bot_count + 1);
        assert(PvpLeaderboardRank(entries, count) == PvpEventRankAt(banners[i], cycle, elapsed));
        for (int row = 1; row < count; row++) assert(entries[row - 1].trophies >= entries[row].trophies);
    }
}

static void TestPvpOpponentsAreDeterministic(void) {
    for (unsigned seed = 1; seed < 40; seed++) {
        PvpOpponent a = PvpOpponentGenerate(18, 400, 20000, seed);
        PvpOpponent b = PvpOpponentGenerate(18, 400, 20000, seed);
        assert(memcmp(&a, &b, sizeof(a)) == 0);
    }
    PvpOpponent first = PvpOpponentGenerate(18, 400, 20000, 1);
    bool varied = false;
    for (unsigned seed = 2; seed < 40; seed++) {
        PvpOpponent other = PvpOpponentGenerate(18, 400, 20000, seed);
        varied = varied || other.bot != first.bot || other.squad != first.squad;
    }
    assert(varied);
}

static double PvpTestMeanSquadRating(int level) {
    const PvpTable *table = PvpTableEnsure();
    double total = 0.0;
    for (unsigned seed = 1; seed <= 200; seed++)
        total += table->squads[PvpOpponentGenerate(level, 0, 20000, seed).squad].rating;
    return total / 200.0;
}

static void TestPvpOpponentsScaleWithLevel(void) {
    double previous = 0.0;
    int levels[] = {1, 10, 25, 50, 100};
    for (int i = 0; i < 5; i++) {
        double rating = PvpTestMeanSquadRating(levels[i]);
        assert(rating > previous);
        previous = rating;
    }
    assert(PvpTargetRating(20, 200) > PvpTargetRating(20, 0));
    assert(PvpTargetRating(20, -200) < PvpTargetRating(20, 0));
    assert(PvpTargetRating(20, 1000000) == PvpTargetRating(20, 2000000));
    PvpOpponent o = PvpOpponentGenerate(1, 0, 20000, 7);
    assert(o.level >= 1 && o.level <= 3 && o.rune_count == 0);
    PvpOpponent veteran = PvpOpponentGenerate(100, 3000, 20000, 7);
    assert(veteran.level <= PVP_LOCAL_MAX_LEVEL && veteran.rune_count > 0 && veteran.rune_count <= RUNE_SLOT_COUNT);
}

static void TestPvpMatchingStaysInTheTrophyWindow(void) {
    const PvpRules *rules = &PvpTableEnsure()->rules;
    for (unsigned seed = 1; seed < 100; seed++) {
        PvpOpponent o = PvpOpponentGenerate(30, 2000, 20000, seed);
        assert(o.trophies >= 2000 - rules->window_below && o.trophies <= 2000 + rules->window_above);
    }
    PvpOpponent past_the_top = PvpOpponentGenerate(30, 900000, 20000, 3);
    assert(past_the_top.bot >= 0 && past_the_top.bot < PvpTableEnsure()->bot_count);
}

static void TestPvpTrophyBounds(void) {
    const PvpRules *rules = &PvpTableEnsure()->rules;
    assert(PvpTrophyGain(1000, 1000) == rules->base_gain);
    assert(PvpTrophyLoss(1000, 1000) == rules->base_loss);
    assert(PvpTrophyGain(0, 1000000) == rules->max_gain);
    assert(PvpTrophyGain(1000000, 0) == rules->min_gain);
    assert(PvpTrophyLoss(1000000, 0) == rules->max_loss);
    assert(PvpTrophyLoss(0, 1000000) == rules->min_loss);

    PlayerPvp pvp = {.trophies = 3};
    PvpOpponent o = PvpOpponentGenerate(10, 3, 20000, 5);
    PlayerPvpBattle lost = PvpRecordAttack(&pvp, &o, false, 1000);
    assert(pvp.trophies == 0 && lost.trophy_delta == -3 && pvp.losses == 1);
    PlayerPvpBattle won = PvpRecordAttack(&pvp, &o, true, 2000);
    assert(pvp.trophies == o.gain && won.trophy_delta == o.gain);
    assert(pvp.best_trophies == o.gain && pvp.win_streak == 1);
    assert(pvp.history_count == 2 && pvp.history[0].at == 2000 && pvp.history[1].at == 1000);
    PvpRecordAttack(&pvp, &o, false, 3000);
    assert(pvp.best_trophies == o.gain && pvp.win_streak == 0);
    assert(PvpGoldReward(&o, true) > PvpGoldReward(&o, false));
}

static void TestPvpNextsResetDaily(void) {
    PlayerPvp pvp = {0};
    long long day = 20000 * NEXUS_SECONDS_PER_DAY;
    int limit = PvpNextsRemaining(&pvp, day);
    assert(limit > 0);
    for (int i = 0; i < limit; i++) assert(PvpSpendNext(&pvp, day + i));
    assert(PvpNextsRemaining(&pvp, day) == 0 && !PvpSpendNext(&pvp, day));
    assert(PvpNextsRemaining(&pvp, day + NEXUS_SECONDS_PER_DAY) == limit);
    assert(PvpSpendNext(&pvp, day + NEXUS_SECONDS_PER_DAY));
    assert(PvpNextsRemaining(&pvp, day + NEXUS_SECONDS_PER_DAY) == limit - 1);
}

static void TestPvpDefenseLogWaitsForFirstBattleAndCapsAttacks(void) {
    const PvpRules *rules = &PvpTableEnsure()->rules;
    long long now = 20000 * NEXUS_SECONDS_PER_DAY;
    PlayerPvp fresh = {0};
    assert(PvpSimulateDefense(&fresh, 10, now) == 0 && fresh.history_count == 0);

    PlayerPvp pvp = {.trophies = 500};
    PvpOpponent o = PvpOpponentGenerate(10, 500, PvpDay(now), 9);
    PvpRecordAttack(&pvp, &o, true, now);
    assert(pvp.defense_checked_at == now);
    assert(PvpSimulateDefense(&pvp, 10, now + rules->attack_interval_seconds - 1) == 0);
    long long later = now + 100LL * rules->attack_interval_seconds;
    assert(PvpSimulateDefense(&pvp, 10, later) == rules->max_attacks_per_login);
    assert(pvp.history_count == 1 + rules->max_attacks_per_login);
    assert(pvp.defense_wins + pvp.defense_losses == rules->max_attacks_per_login);
    assert(pvp.history[0].defense && pvp.history[0].at == later);
    assert(PvpSimulateDefense(&pvp, 10, later) == 0);

    PlayerPvp copy = {.trophies = 500};
    PvpRecordAttack(&copy, &o, true, now);
    PvpSimulateDefense(&copy, 10, later);
    assert(memcmp(&copy, &pvp, sizeof(copy)) == 0);
}

static void TestPvpRevengeRebuildsTheSameOpponent(void) {
    PvpOpponent o = PvpOpponentGenerate(40, 800, 20000, 11);
    PlayerPvpBattle battle = {
        .bot = o.bot,
        .squad = o.squad,
        .level = o.level,
        .opponent_trophies = o.trophies,
        .seed = o.seed,
        .defense = true,
    };
    assert(PvpCanRevenge(&battle));
    PvpOpponent rebuilt = PvpOpponentFromBattle(&battle, 800);
    assert(memcmp(&rebuilt, &o, sizeof(o)) == 0);
    battle.revenged = true;
    assert(!PvpCanRevenge(&battle));
    battle.revenged = false;
    battle.won = true;
    assert(!PvpCanRevenge(&battle));
}

static void TestPvpEveryPooledSquadFights(void) {
    static Player player;
    static BattleRoster roster;
    static BattleSim sim;
    memset(&player, 0, sizeof(player));
    Character ch = {0};
    Ability ability = {0};
    TestCharacter(&ch, &ability, "UNIT_A", "ABILITY_DAMAGE", TARGET_ONE, 50, 500);
    player.roster = &ch;
    player.roster_count = 1;
    for (int q = 0; q < PLAYER_MAX_SQUADS; q++)
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) player.squads[q][s] = -1;
    player.squads[0][0] = 0;
    const PvpTable *table = PvpTableEnsure();
    int stride = table->squad_count / 12;
    for (int squad = 0; squad < table->squad_count; squad += stride) {
        PvpOpponent o = {.squad = squad, .level = 50, .seed = 3};
        PvpOpponentDerive(&o, 0);
        PvpOpponentRoster(&roster, &o);
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            const PvpSquadCell *cell = &table->squads[squad].cells[slot];
            assert(roster.present[slot] == cell->present);
            if (!cell->present) continue;
            assert(roster.chars[slot].giant == (slot == SQUAD_SLOT_LARGE));
            assert(roster.chars[slot].has_tier[roster.chars[slot].tier]);
        }
        BattleSimRun(&sim, &player, &roster, 77);
        assert(sim.rounds >= 1);
    }
}

static void TestPvpOpponentRunesApplyToTheEnemySide(void) {
    int power_rune = -1;
    for (int def = 0; def < RuneCatalogCount() && power_rune < 0; def++)
        if (RuneDefAt(def)->buff == RUNE_BUFF_POWER) power_rune = def;
    assert(power_rune >= 0);
    static Player player;
    static BattleRoster roster;
    static SimUnit plain[SIM_UNIT_COUNT], buffed[SIM_UNIT_COUNT];
    memset(&player, 0, sizeof(player));
    memset(&roster, 0, sizeof(roster));
    for (int q = 0; q < PLAYER_MAX_SQUADS; q++)
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) player.squads[q][s] = -1;
    Ability ability = {0};
    TestCharacter(&roster.chars[0], &ability, "UNIT_A", "ABILITY_DAMAGE", TARGET_ONE, 100, 500);
    roster.present[0] = true;
    BattleSimPreview(plain, &player, &roster);
    roster.rune_defs[0] = power_rune;
    roster.rune_def_count = 1;
    BattleSimPreview(buffed, &player, &roster);
    int enemy = SQUAD_SLOT_COUNT;
    assert(buffed[enemy].power > plain[enemy].power);
    assert(buffed[enemy].max_health == plain[enemy].max_health);
}

static void TestPvpSaveRoundTrip(void) {
    static Player saved, loaded;
    memset(&saved, 0, sizeof(saved));
    memset(&loaded, 0, sizeof(loaded));
    ProgressionInitializePlayerDefaults(&saved);
    ProgressionInitializePlayerDefaults(&loaded);
    for (int q = 0; q < PLAYER_MAX_SQUADS; q++) {
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) saved.squads[q][s] = loaded.squads[q][s] = -1;
    }
    PvpOpponent o = PvpOpponentGenerate(5, 0, 20000, 21);
    PvpRecordAttack(&saved.pvp, &o, true, 1700000000);
    saved.pvp.history[0].revenged = true;
    saved.pvp.nexts_used = 2;
    saved.pvp.nexts_day = 20000;
    saved.pvp.event = (PlayerPvpEvent){.cycle = 700, .banners = 90, .milestones_paid = 1, .buffed = true};
    saved.pvp.event_result_count = 1;
    saved.pvp.event_results[0] = (PlayerPvpEventResult){.cycle = 699, .banners = 40, .rank = 12, .gold = 5, .gems = 2};
    const char *path = "/tmp/hod_pvp_save_test.json";
    assert(ProgressionSaveToPath(&saved, path));
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(memcmp(&saved.pvp, &loaded.pvp, sizeof(saved.pvp)) == 0);

    char *text = LoadFileText(path);
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    cJSON_DeleteItemFromObjectCaseSensitive(root, "pvp");
    cJSON_ReplaceItemInObjectCaseSensitive(root, "version", cJSON_CreateNumber(PROGRESSION_FIRST_PVP_VERSION - 1));
    char *legacy = cJSON_Print(root);
    assert(SaveFileText(path, legacy));
    cJSON_free(legacy);
    cJSON_Delete(root);
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(loaded.pvp.trophies == 0 && loaded.pvp.history_count == 0);

    cJSON *bad_root = NULL;
    assert(ProgressionSaveToPath(&saved, path));
    text = LoadFileText(path);
    bad_root = cJSON_Parse(text);
    UnloadFileText(text);
    cJSON_DeleteItemFromObjectCaseSensitive(bad_root, "pvp");
    char *corrupt = cJSON_Print(bad_root);
    assert(SaveFileText(path, corrupt));
    cJSON_free(corrupt);
    cJSON_Delete(bad_root);
    assert(!ProgressionLoadFromPath(&loaded, path));
    remove(path);

    saved.pvp.history_count = 1;
    saved.pvp.history[0].bot = 1000000;
    PvpHistorySanitize(&saved.pvp);
    assert(saved.pvp.history_count == 0);
}

static void TestPvpLeaderboardRanksThePlayer(void) {
    static PvpRankEntry entries[PVP_BOT_CAPACITY + 1];
    int count = PvpLeaderboardBuild(entries, 1000000, 20000);
    assert(count == PvpTableEnsure()->bot_count + 1);
    assert(PvpLeaderboardRank(entries, count) == 1);
    for (int i = 1; i < count; i++) assert(entries[i - 1].trophies >= entries[i].trophies);
    count = PvpLeaderboardBuild(entries, 0, 20000);
    assert(PvpLeaderboardRank(entries, count) > 1);
}

static void TestGemPurchaseCreditsGemsAndRefreshesStamina(void) {
    Player player = {.gems = 41, .stamina = 3, .stamina_max = 12, .stamina_regen_timer = 120.0f};
    PlayerCreditGemPurchase(&player, 121);
    assert(player.gems == 162);
    assert(player.stamina == 12 && player.stamina_regen_timer == 0.0f);

    player.stamina = 15;
    PlayerCreditGemPurchase(&player, 22);
    assert(player.gems == 184 && player.stamina == 15);

    player.gems = INT_MAX - 10;
    PlayerCreditGemPurchase(&player, 1600);
    assert(player.gems == INT_MAX);
}

static void TestPvpTrophiesHeldNeedsAWeekAboveTheLine(void) {
    PlayerPvp pvp = {.trophies = 490};
    long long start = 1700000000;
    PvpOpponent o = PvpOpponentGenerate(10, 490, 20000, 2);
    PvpRecordAttack(&pvp, &o, true, start);
    assert(pvp.trophies > 500 && pvp.held_since[0] == start);
    assert(PvpTrophiesHeld(&pvp, start + PVP_HOLD_SECONDS - 1) == 0);
    assert(PvpTrophiesHeld(&pvp, start + PVP_HOLD_SECONDS) == 500);
    pvp.trophies = 480;
    PvpHoldUpdate(&pvp, start + 1);
    assert(pvp.held_since[0] == 0 && PvpTrophiesHeld(&pvp, start + 2 * PVP_HOLD_SECONDS) == 0);
}

static void TestPvpAchievementsFollowTheLadder(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    Campaign campaign = {0};
    MissionCatalogLoad("data/missions.json");
    player.level = 1;
    player.pvp.wins = 25;
    player.pvp.best_win_streak = 5;
    player.pvp.trophies_won = 500;
    MissionsSync(&player, &campaign);
    assert(MissionCompleted(&player.missions, MissionIndexById("a11")));
    assert(MissionCompleted(&player.missions, MissionIndexById("a12")));
    assert(!MissionCompleted(&player.missions, MissionIndexById("a13")));
    assert(MissionCompleted(&player.missions, MissionIndexById("a59")));
    assert(MissionCompleted(&player.missions, MissionIndexById("a5")));
    assert(MissionCompleted(&player.missions, MissionIndexById("a34")));
    assert(!MissionCompleted(&player.missions, MissionIndexById("a35")));
}

static void TestPvpResolveBattlePaysAndMarksRevenge(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    player.gold = 100;
    PvpOpponent o = PvpOpponentGenerate(10, 0, 20000, 4);
    player.pvp.history_count = 1;
    player.pvp.history[0] = (PlayerPvpBattle){.bot = o.bot, .defense = true};
    BattleRewardResult win = PvpResolveBattle(&player, &o, true, 0, 12, 1700000000, false);
    assert(win.resolved && win.won && win.trophy_delta == o.gain);
    assert(win.gold_reward == PvpGoldReward(&o, true) + 12);
    assert(player.gold == 100 + win.gold_reward);
    assert(player.experience == win.xp_reward && win.xp_reward == PvpXpReward(&o, true));
    assert(player.pvp.history_count == 2 && player.pvp.history[1].revenged);
    assert(!PvpCanRevenge(&player.pvp.history[1]));

    int gold = player.gold;
    BattleRewardResult loss = PvpResolveBattle(&player, &o, false, -1, 0, 1700000100, false);
    assert(!loss.won && loss.trophy_delta < 0 && loss.gold_reward == PvpGoldReward(&o, false));
    assert(player.gold == gold + loss.gold_reward && player.pvp.losses == 1);
}

static void TestPvpSpendsStaminaNotEnergy(void) {
    Player player = {.stamina = 1, .energy = 5};
    assert(PlayerSpendStamina(&player, PVP_STAMINA_COST));
    assert(player.stamina == 0 && player.energy == 5);
    assert(!PlayerSpendStamina(&player, PVP_STAMINA_COST));
}

static void TestPvpEventRotatesThroughTheCatalog(void) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    long long duration = rules->duration_seconds;
    long long start = 1700000000 / duration * duration;
    assert(PvpEventRemaining(start) == duration);
    assert(PvpEventRemaining(start + duration - 1) == 1);
    long long cycle = PvpEventCycle(start);
    assert(PvpEventForCycle(cycle) != PvpEventForCycle(cycle + 1));
    assert(PvpEventForCycle(cycle) == PvpEventForCycle(cycle + rules->catalog_count));
    for (int i = 0; i < rules->catalog_count; i++) assert(rules->catalog[i].changer.effect_count > 0);
}

static void TestOnslaughtReplacesEveryFewPvpEvents(void) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    assert(rules->guild_catalog_count > 0 && rules->guild_every_cycles > 1);
    long long cycles = rules->guild_every_cycles * rules->guild_catalog_count;
    int guild_cycles = 0;
    for (long long cycle = 1; cycle <= cycles; cycle++) {
        const PvpEvent *event = PvpEventForCycle(cycle);
        assert(event->guild == PvpEventIsGuildCycle(cycle));
        long long before = PvpGuildCycleBefore(cycle), after = PvpGuildCycleAfter(cycle);
        assert(PvpEventIsGuildCycle(before) && before < cycle && cycle - before <= rules->guild_every_cycles);
        assert(PvpEventIsGuildCycle(after) && after > cycle && after - cycle <= rules->guild_every_cycles);
        if (!event->guild) continue;
        guild_cycles++;
        assert(event->guild_minimum_banners > 0 && event->rank_reward_count > 0);
        const PvpEventRankReward *first = PvpGuildRewardForRank(event, 1);
        assert(first && first->reward.gems > 0 && PvpGuildRewardForRank(event, 0) == NULL);
    }
    assert(guild_cycles == rules->guild_catalog_count);
}

static void TestDuelOpponentFightsTheMembersSquad(void) {
    PvpOpponent o = PvpOpponentFromDuel("ally", "Varric", 22, 340, 100, 5);
    assert(o.duel && !strcmp(PvpOpponentName(&o), "Varric") && o.level == 22 && o.trophies == 340);
    assert(o.gain == PvpTrophyGain(100, 340) && o.loss == PvpTrophyLoss(100, 340));
    assert(PvpOpponentAddDuelUnit(&o, 0, "UNIT_VIVIENNE", TIER_II, 9));
    assert(PvpOpponentAddDuelUnit(&o, 2, "UNIT_NOT_IN_THE_CATALOG", TIER_I, 3));
    assert(!PvpOpponentAddDuelUnit(&o, SQUAD_SLOT_COUNT, "UNIT_VIVIENNE", TIER_I, 1));
    PvpOpponentAddDuelRune(&o, "rune-power-1");
    PvpOpponentAddDuelRune(&o, "rune-that-does-not-exist");
    static BattleRoster roster;
    PvpOpponentRoster(&roster, &o);
    assert(roster.present[0] && !roster.present[2] && roster.chars[0].level == 9 && roster.chars[0].tier == TIER_II);
    assert(roster.rune_def_count == 1 && roster.rune_defs[0] == RuneDefIndex("rune-power-1"));
    PlayerPvp pvp = {.trophies = 100};
    PlayerPvpBattle battle = PvpRecordAttack(&pvp, &o, true, 1700000000);
    assert(!strcmp(battle.opponent, "Varric") && battle.bot == 0 && pvp.trophies == 100 + o.gain);
    assert(battle.gold == PvpGoldReward(&o, true));
}

static void TestPvpEventBannersMilestonesAndClaim(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    long long duration = rules->duration_seconds;
    long long start = 1700000000 / duration * duration;
    PlayerPvp *pvp = &player.pvp;
    PvpOpponent o = PvpOpponentGenerate(10, 0, PvpDay(start), 8);

    PvpEventSync(pvp, start);
    assert(pvp->event.cycle == PvpEventCycle(start) && pvp->event_result_count == 0);
    const PvpEvent *event = PvpEventForCycle(pvp->event.cycle);
    pvp->event.banners = event->milestones[0].banners - 1;
    int gold = player.gold;
    int expected_banners = PvpEventBannerReward(&o, pvp->event.banners, true);
    BattleRewardResult win = PvpResolveBattle(&player, &o, true, -1, 0, start + 10, true);
    assert(win.trophy_delta == 0 && pvp->trophies == 0);
    assert(win.banner_delta == expected_banners);
    assert(pvp->event.banners == event->milestones[0].banners - 1 + win.banner_delta);
    assert(pvp->event.milestones_paid == 1);
    assert(player.gold >= gold + event->milestones[0].reward.gold);
    assert(pvp->history[0].banners == win.banner_delta);
    BattleRewardResult loss = PvpResolveBattle(&player, &o, false, -1, 0, start + 20, true);
    assert(loss.banner_delta == rules->loss_banners);

    int banners = pvp->event.banners;
    PvpEventSync(pvp, start + duration + 1);
    assert(pvp->event.banners == 0 && pvp->event.milestones_paid == 0);
    assert(pvp->event_result_count == 1 && PvpEventUnclaimedCount(pvp) == 1);
    PlayerPvpEventResult *result = &pvp->event_results[0];
    assert(result->banners == banners && result->rank >= 1);
    const PvpEventRankReward *reward = PvpEventRewardForRank(event, result->rank);
    assert(reward && result->gold == reward->reward.gold && result->gems == reward->reward.gems);
    gold = player.gold;
    assert(PvpEventClaim(&player, 0));
    assert(player.gold == gold + result->gold);
    assert(!PvpEventClaim(&player, 0) && PvpEventUnclaimedCount(pvp) == 0);

    PvpEventSync(pvp, start + 3 * duration);
    assert(pvp->event_result_count == 1);
    PlayerUnloadRoster(&player);
}

static int TestRosterCount(const Player *player, const char *id) {
    int count = 0;
    for (int i = 0; i < player->roster_count; i++) count += strcmp(player->roster[i].id, id) == 0;
    return count;
}

static void TestPvpEventPrizesAreOwnableAndHaveArt(void) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    for (int i = 0; i < rules->catalog_count; i++) {
        const PvpEvent *event = &rules->catalog[i];
        assert(event->milestones[event->milestone_count - 1].banners == rules->ladder_banners);
        for (int tier = 0; tier < event->rank_reward_count; tier++) {
            const PvpReward *reward = &event->rank_rewards[tier].reward;
            assert(reward->prize_count > 0 || reward->gold > 0);
            for (int prize = 0; prize < reward->prize_count; prize++) {
                const PvpPrize *item = &reward->prizes[prize];
                assert(item->count > 0 && item->id[0]);
                if (item->kind == PVP_PRIZE_UNIT) assert(ResourceExists(GuiPortraitRef(item->icon)));
                if (item->kind == PVP_PRIZE_GEAR) assert(GearDefAt(item->index));
                if (item->kind == PVP_PRIZE_VOUCHER) {
                    assert(item->index >= 0 && item->index < PLAYER_VOUCHER_CAPACITY);
                    assert(ResourceExists(item->icon));
                }
            }
        }
    }
}

static const PvpEvent *TestPvpEventNamed(const char *key) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    for (int i = 0; i < rules->catalog_count; i++)
        if (!strcmp(rules->catalog[i].title_key, key)) return &rules->catalog[i];
    assert(false);
    return NULL;
}

static long long TestPvpCycleOf(const PvpEvent *event) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    return 700 - 700 % rules->catalog_count + (long long)(event - rules->catalog);
}

static const PvpEventMilestone *TestMilestoneWith(const PvpEvent *event, PvpPrizeKind kind, int *index) {
    for (int i = 0; i < event->milestone_count; i++) {
        const PvpReward *reward = &event->milestones[i].reward;
        if (reward->prize_count > 0 && reward->prizes[0].kind == kind) {
            *index = i;
            return &event->milestones[i];
        }
    }
    assert(false);
    return NULL;
}

static void TestPvpEventMilestonesPayWispGearAndTome(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    const PvpEvent *event = TestPvpEventNamed("PVP_AVVAR_AND_DRAGONS_JUN_27_TITLE_KEY");
    long long duration = PvpTableEnsure()->events.duration_seconds;
    long long start = TestPvpCycleOf(event) * duration;
    PlayerPvp *pvp = &player.pvp;
    PvpOpponent o = PvpOpponentGenerate(10, 0, PvpDay(start), 8);
    PvpEventSync(pvp, start);
    assert(PvpEventForCycle(pvp->event.cycle) == event);

    int index = 0;
    const PvpEventMilestone *wisp = TestMilestoneWith(event, PVP_PRIZE_UNIT, &index);
    pvp->event.banners = wisp->banners;
    pvp->event.milestones_paid = index;
    const char *wisp_id = wisp->reward.prizes[0].id;
    int wisps = TestRosterCount(&player, wisp_id);
    int roster = player.roster_count;
    player.roster_count = player.roster_capacity;
    BattleRewardResult full = PvpResolveBattle(&player, &o, true, -1, 0, start + 10, true);
    assert(full.event_payout.prize_count == 0 && pvp->event.milestones_paid == index);
    player.roster_count = roster;
    BattleRewardResult paid = PvpResolveBattle(&player, &o, true, -1, 0, start + 20, true);
    assert(paid.event_payout.prize_count == 1 && paid.event_payout.prizes[0] == &wisp->reward.prizes[0]);
    assert(TestRosterCount(&player, wisp_id) == wisps + 1);

    const PvpEventMilestone *tome = TestMilestoneWith(event, PVP_PRIZE_TOME, &index);
    pvp->event.banners = tome->banners;
    pvp->event.milestones_paid = index;
    int tomes = player.gear.tomes;
    PvpResolveBattle(&player, &o, true, -1, 0, start + 30, true);
    assert(player.gear.tomes == tomes + 1 && pvp->event.milestones_paid == event->milestone_count);
    PlayerUnloadRoster(&player);
}

static void TestPvpEventClaimPaysWholeBundleOnce(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    const PvpEvent *event = TestPvpEventNamed("PVP_AVVAR_AND_DRAGONS_JUN_27_TITLE_KEY");
    const PvpReward *top = &event->rank_rewards[0].reward;
    assert(top->prize_count == 4 && PvpRewardUnitCount(top) == 4 && top->gems == 175 && top->gold == 100000);
    const char *id = top->prizes[0].id;
    int owned = TestRosterCount(&player, id);
    player.gold = 0;
    player.gems = 0;
    player.pvp.event_result_count = 1;
    player.pvp.event_results[0] = (PlayerPvpEventResult){
        .cycle = TestPvpCycleOf(event), .banners = 3000, .rank = 1, .gold = top->gold, .gems = top->gems
    };
    int roster = player.roster_count;
    player.roster_count = player.roster_capacity;
    assert(!PvpEventClaim(&player, 0) && !player.pvp.event_results[0].claimed && player.gold == 0);
    player.roster_count = roster;
    assert(PvpEventClaim(&player, 0));
    assert(player.roster_count == roster + 4);
    assert(TestRosterCount(&player, id) == owned + 1);
    assert(player.gold == top->gold && player.gems == top->gems);
    assert(!PvpEventClaim(&player, 0) && player.roster_count == roster + 4);
    PlayerUnloadRoster(&player);
}

static void TestPvpEventClaimPaysPackVouchers(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    const PvpEvent *event = TestPvpEventNamed("PVP_AVVAR_AND_DRAGONS_JUN_27_TITLE_KEY");
    int tier = 8;
    const PvpReward *reward = &event->rank_rewards[tier].reward;
    assert(reward->prizes[0].kind == PVP_PRIZE_VOUCHER && !strcmp(reward->prizes[0].id, "iron"));
    int voucher = reward->prizes[0].index;
    int before = player.pack_vouchers[voucher];
    player.pvp.event_result_count = 1;
    player.pvp.event_results[0] =
        (PlayerPvpEventResult){.cycle = TestPvpCycleOf(event), .rank = event->rank_rewards[tier].first};
    assert(PvpEventClaim(&player, 0));
    assert(player.pack_vouchers[voucher] == before + reward->prizes[0].count);
    PlayerUnloadRoster(&player);
}

static void TestPvpEventRankRisesWithBannersAndCoversEveryone(void) {
    long long cycle = 5000;
    int duration = PvpTableEnsure()->events.duration_seconds;
    assert(PvpEventRankAt(0, cycle, duration) > PvpEventRankAt(2000, cycle, duration));
    assert(PvpEventRankAt(1000000, cycle, duration) == 1);
    assert(PvpEventRankAt(0, cycle, 0) == 1);
    int last = PvpTableEnsure()->bot_count + 1;
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    for (int i = 0; i < rules->catalog_count; i++)
        for (int rank = 1; rank <= last; rank++) assert(PvpEventRewardForRank(&rules->catalog[i], rank));
}

static void TestPvpEventSquadBuffCountsOncePerEvent(void) {
    static Player player;
    static BattleSim sim;
    memset(&player, 0, sizeof(player));
    memset(&sim, 0, sizeof(sim));
    sim.units[0].present = sim.units[1].present = true;
    sim.units[0].game_changer_applied = true;
    MissionsRecordEventBattle(&player, &sim, 30);
    assert(player.missions.counters[MISSION_STAT_EVENT_BANNERS] == 30);
    assert(player.missions.counters[MISSION_STAT_EVENT_BUFFED_SQUADS] == 0);
    sim.units[1].game_changer_applied = true;
    MissionsRecordEventBattle(&player, &sim, 0);
    MissionsRecordEventBattle(&player, &sim, 0);
    assert(player.missions.counters[MISSION_STAT_EVENT_BUFFED_SQUADS] == 1);
}

static void TestPvpBannerCurvePeaksAtTheLadderMidpoint(void) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    int ladder = rules->ladder_banners;
    int start = rules->win_banners_start, peak = start + rules->win_banners_peak_bonus;
    assert(PvpEventMaxBannersPerWin(0) == start);
    assert(PvpEventMaxBannersPerWin(ladder / rules->win_banners_steps_to_peak) == start + 1);
    assert(PvpEventMaxBannersPerWin(ladder / 2) == peak);
    int end = PvpEventMaxBannersPerWin(ladder);
    assert(end < peak && end > start);
    assert(PvpEventMaxBannersPerWin(ladder * 3) == end);
    for (int banners = 0; banners <= ladder; banners += 50) assert(PvpEventMaxBannersPerWin(banners) <= peak);
    PvpOpponent weak = {.gain = PvpTableEnsure()->rules.min_gain};
    PvpOpponent strong = {.gain = PvpTableEnsure()->rules.max_gain};
    assert(PvpEventBannerReward(&strong, ladder / 2, true) == peak);
    assert(PvpEventBannerReward(&weak, ladder / 2, true) < peak);
}

static void TestPvpEventResultsKeepNineteenUnclaimedFirst(void) {
    PlayerPvp pvp = {0};
    long long duration = PvpTableEnsure()->events.duration_seconds;
    for (long long cycle = 100; cycle < 100 + PVP_EVENT_RESULT_CAPACITY + 1; cycle++) {
        pvp.event = (PlayerPvpEvent){.cycle = cycle, .banners = 10};
        PvpEventSync(&pvp, (cycle + 1) * duration);
    }
    assert(pvp.event_result_count == PVP_EVENT_RESULT_CAPACITY && PVP_EVENT_RESULT_CAPACITY == 19);
    assert(pvp.event_results[0].cycle == 100 + PVP_EVENT_RESULT_CAPACITY);
    assert(pvp.event_results[PVP_EVENT_RESULT_CAPACITY - 1].cycle == 101);
    pvp.event_results[0].claimed = true;
    PvpEventSortResults(&pvp);
    assert(!pvp.event_results[0].claimed && pvp.event_results[PVP_EVENT_RESULT_CAPACITY - 1].claimed);
    assert(pvp.event_results[0].cycle == 100 + PVP_EVENT_RESULT_CAPACITY - 1);
}

static void TestPvpEventMatchmakingFollowsBanners(void) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    long long duration = rules->duration_seconds;
    long long now = 1700000000 / duration * duration + duration - 1;
    long long cycle = PvpEventCycle(now), elapsed = now % duration;
    for (unsigned seed = 1; seed < 60; seed++) {
        PvpOpponent o = PvpEventOpponentGenerate(30, 500, 900, now, seed);
        assert(abs(PvpBotBanners(o.bot, cycle, elapsed) - 900) <= rules->matchmaking_banner_window);
        PvpOpponent again = PvpEventOpponentGenerate(30, 500, 900, now, seed);
        assert(memcmp(&o, &again, sizeof(o)) == 0);
    }
}

static void TestBattleFeetSitOnPadsWhoseBottomsShareOnePlane(void) {
    float small_feet = BattleFeetY(1.0f);
    float giant_feet = BattleFeetY(2.0f);
    assert(fabsf(small_feet - (BATTLE_MARKER_Y - BATTLE_PREFAB_BASE_DROP + HOME_PAD_HALF_THICK)) < 0.0001f);
    assert(fabsf((small_feet - 2.0f * HOME_PAD_HALF_THICK) - BATTLE_PAD_BOTTOM_Y) < 0.0001f);
    assert(fabsf((giant_feet - 4.0f * HOME_PAD_HALF_THICK) - BATTLE_PAD_BOTTOM_Y) < 0.0001f);
}

static void TestCreatureClipFramesStayOnIdleGround(void) {
    BoneInfo bones[2] = {{.name = "Dragon_Foot", .parent = -1}, {.name = "Dragon_Tail04", .parent = -1}};
    Transform idle_frame[2] = {{.translation = {0.0f, 0.2f, 0.0f}}, {.translation = {0.0f, 0.0f, 0.0f}}};
    Transform jump_frame[2] = {{.translation = {0.0f, 0.9f, 0.0f}}, {.translation = {0.0f, 0.5f, 0.0f}}};
    Transform *idle_poses[1] = {idle_frame};
    Transform *jump_poses[1] = {jump_frame};
    ModelAnimation idle = {.boneCount = 2, .frameCount = 1, .bones = bones, .framePoses = idle_poses};
    ModelAnimation jump = {.boneCount = 2, .frameCount = 1, .bones = bones, .framePoses = jump_poses};
    ClipHoldGroundOfIdle(jump, idle);
    assert(fabsf(jump_frame[0].translation.y - 0.2f) < 0.0001f);
    assert(fabsf(jump_frame[1].translation.y + 0.2f) < 0.0001f);
    assert(UnitClipsHoldGround("client-additional-data/char_pack/c_giant/c_giant_t1_rl.glb"));
    assert(UnitClipsHoldGround("mnt_sw_greywardenknight_t1_rl.glb"));
    assert(!UnitClipsHoldGround("client-additional-data/char_pack/x/hm_sw_advisorcullen_t1_rl.glb"));
    assert(!UnitClipsHoldGround("c_wisp_common_t1_rl.glb"));
}

static const TutorialStep *TutorialStepAt(Tutorial *t, float step) {
    assert(TutorialJumpTo(t, step));
    return TutorialCurrent(t);
}

static void TestTutorialScriptCoversBothFunnels(void) {
    static Tutorial t;
    if (loc.count == 0) LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    TutorialLoad(&t);
    assert(t.step_count[TUTORIAL_FUNNEL_MAIN] == 78);
    assert(t.step_count[TUTORIAL_FUNNEL_CONSUME] == 10);
    for (int funnel = 0; funnel < TUTORIAL_FUNNEL_COUNT; funnel++) {
        for (int i = 0; i < t.step_count[funnel]; i++) {
            const TutorialStep *step = &t.steps[funnel][i];
            assert(step->signal != TUTORIAL_SIGNAL_UNSUPPORTED);
            assert(step->event != TUTORIAL_EVENT_COUNT);
            assert(step->action != TUTORIAL_ACTION_UNSUPPORTED);
            assert(
                !step->highlight_required || step->highlight != TUTORIAL_ANCHOR_NONE ||
                step->action == TUTORIAL_ACTION_ABOUT_SCREEN
            );
            assert(!step->text[0] || LocGet(step->text)[0]);
            if (i > 0) assert(step->step > t.steps[funnel][i - 1].step);
        }
    }
    assert(TutorialStepAt(&t, 0.2f)->action == TUTORIAL_ACTION_START_INTRO_BATTLE);
    assert(TutorialStepAt(&t, 1.3f)->highlight == TUTORIAL_ANCHOR_HOME_QUESTS);
    assert(TutorialStepAt(&t, 6.0f)->hold_battle && TutorialStepAt(&t, 6.0f)->highlight_position == 2);
    assert(TutorialStepAt(&t, 103.0f)->funnel_end == TUTORIAL_FUNNEL_END_MAIN);
    assert(TutorialStepAt(&t, 128.0f)->funnel_end == TUTORIAL_FUNNEL_END_CONSUME);
    assert(!TutorialJumpTo(&t, 63.0f));
}

static void TestTutorialScriptsTheTwoLostBattles(void) {
    static Tutorial t;
    TutorialLoad(&t);
    float wins[] = {5.0f, 11.75f, 31.0f, 36.0f, 57.0f, 93.0f};
    float losses[] = {16.0f, 41.0f};
    for (int i = 0; i < (int)(sizeof(wins) / sizeof(wins[0])); i++) {
        TutorialStepAt(&t, wins[i]);
        assert(TutorialWantsVictory(&t));
    }
    for (int i = 0; i < (int)(sizeof(losses) / sizeof(losses[0])); i++) {
        TutorialStepAt(&t, losses[i]);
        assert(!TutorialWantsVictory(&t));
    }
}

static void TestTutorialResumesAtTheSavedRestorePoint(void) {
    static Tutorial t;
    TutorialLoad(&t);
    TutorialResumeMain(&t, &(PlayerTutorial){.restore_step = 19});
    assert(TutorialCurrent(&t)->step == 19.0f);
    TutorialResumeMain(&t, &(PlayerTutorial){0});
    assert(TutorialCurrent(&t)->step == 0.2f);
    TutorialResumeMain(&t, &(PlayerTutorial){.restore_step = 63});
    assert(TutorialCurrent(&t)->step == 0.2f);
}

static void TestTutorialPackRewardsResolve(void) {
    static Tutorial t;
    TutorialLoad(&t);
    cJSON *catalog_root = ProgressionLoadCharacterCatalog();
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    const char *packs[] = {TUTORIAL_SOLDIER_PACK_ID, TUTORIAL_CHAMPION_PACK_ID, TUTORIAL_CONSUME_PACK_ID};
    for (int i = 0; i < 3; i++) {
        const TutorialPackReward *reward = TutorialPackRewardFor(&t, packs[i]);
        assert(reward && reward->hero_count > 0);
        for (int h = 0; h < reward->hero_count; h++) assert(CatalogFindCharacter(catalog, reward->heroes[h]));
        assert(!reward->rune[0] || RuneDefIndex(reward->rune) >= 0);
    }
    assert(CatalogFindCharacter(catalog, t.giant) && JsonBool(CatalogFindCharacter(catalog, t.giant), "giant"));
    assert(t.gem_pack_gems > 0 && t.gift_hero_count > 0);
    cJSON_Delete(catalog_root);
}

static void TestPlayerNameFollowsTheOriginalRule(void) {
    assert(PlayerNameValid("Knight-Commander Meredith"));
    assert(PlayerNameValid("Морриган"));
    assert(PlayerNameValid("Hawke 2"));
    assert(!PlayerNameValid(""));
    assert(!PlayerNameValid(" Hawke"));
    assert(!PlayerNameValid("Hawke "));
    assert(!PlayerNameValid("Grey_Warden"));
    assert(!PlayerNameValid("Hawke!"));
    assert(!PlayerNameValid("abcdefghijklmnopqrstuvwxyz"));
}

static void TestProgressionKeepsNameAndTutorialState(void) {
    static Player saved, loaded;
    memset(&saved, 0, sizeof(saved));
    memset(&loaded, 0, sizeof(loaded));
    ProgressionInitializePlayerDefaults(&saved);
    ProgressionInitializePlayerDefaults(&loaded);
    for (int q = 0; q < PLAYER_MAX_SQUADS; q++) {
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) saved.squads[q][s] = loaded.squads[q][s] = -1;
    }
    StrCopy(saved.name, sizeof(saved.name), "Морриган");
    saved.tutorial = (PlayerTutorial){.main_complete = false, .consume_complete = false, .restore_step = 44};
    saved.fast_battles = true;
    const char *path = "/tmp/hod_tutorial_save_test.json";
    assert(ProgressionSaveToPath(&saved, path));
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(!strcmp(loaded.name, "Морриган"));
    assert(!loaded.tutorial.main_complete && !loaded.tutorial.consume_complete && loaded.tutorial.restore_step == 44);
    assert(loaded.fast_battles);

    char *text = LoadFileText(path);
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    cJSON_DeleteItemFromObjectCaseSensitive(root, "tutorial");
    cJSON_DeleteItemFromObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(root, "account"), "name");
    char *older = cJSON_Print(root);
    assert(SaveFileText(path, older));
    cJSON_free(older);
    cJSON_Delete(root);
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(loaded.tutorial.main_complete && loaded.tutorial.consume_complete && !loaded.name[0]);
    remove(path);
}

static cJSON *ServerPurchaseResponse(const Player *player, const char *hero, uint64_t instance_id) {
    char *text = ProgressionSerialize(player);
    cJSON *profile = cJSON_Parse(text);
    cJSON_free(text);
    cJSON *account = cJSON_GetObjectItemCaseSensitive(profile, "account");
    cJSON_ReplaceItemInObjectCaseSensitive(account, "gems", cJSON_CreateNumber(player->gems - 30));
    cJSON *roster = cJSON_GetObjectItemCaseSensitive(profile, "roster");
    cJSON *addition = cJSON_CreateObject();
    cJSON_AddStringToObject(addition, "id", hero);
    cJSON_AddNumberToObject(addition, "instance_id", (double)instance_id);
    cJSON_AddStringToObject(addition, "tier", "I");
    cJSON_AddNumberToObject(addition, "level", 1);
    cJSON_AddNumberToObject(addition, "xp", 0);
    cJSON_AddBoolToObject(addition, "ascended", false);
    cJSON_AddNumberToObject(addition, "consume_xp", 0);
    cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(roster, "additions"), addition);
    cJSON_ReplaceItemInObjectCaseSensitive(roster, "next_instance_id", cJSON_CreateNumber((double)instance_id + 1));
    cJSON *runes = cJSON_GetObjectItemCaseSensitive(profile, "runes");
    uint64_t uuid = (uint64_t)cJSON_GetObjectItemCaseSensitive(runes, "next_uuid")->valuedouble;
    if (!uuid) uuid = 1;
    cJSON *rune = cJSON_CreateObject();
    cJSON_AddNumberToObject(rune, "uuid", (double)uuid);
    cJSON_AddStringToObject(rune, "def", "rune-power-2");
    cJSON_AddNumberToObject(rune, "expire_at", 0);
    cJSON_AddNumberToObject(rune, "inventory_expire_at", 4102444800.0);
    cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(runes, "items"), rune);
    cJSON_ReplaceItemInObjectCaseSensitive(runes, "next_uuid", cJSON_CreateNumber((double)uuid + 1));
    cJSON *inventory = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(profile, "gear"), "inventory");
    cJSON_AddNumberToObject(inventory, GearDefAt(0)->id, 1);
    cJSON *response = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "profile", profile);
    return response;
}

static void TestServerPurchaseProfileReplacesThePlayer(void) {
    static Player player;
    memset(&player, 0, sizeof(player));
    ProgressionInitializePlayerDefaults(&player);
    PlayerLoadRoster(&player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    player.gems = 100;
    int roster_before = player.roster_count;
    int runes_before = player.runes.count;
    uint64_t instance_id = player.next_instance_id + 5;
    cJSON *response = ServerPurchaseResponse(&player, "UNIT_ELVENMAGE", instance_id);

    assert(ProgressionAdoptServerProfile(&player, response));
    assert(player.gems == 70);
    assert(player.roster_count == roster_before + 1);
    const Character *added = &player.roster[player.roster_count - 1];
    assert(!strcmp(added->id, "UNIT_ELVENMAGE") && added->instance_id == instance_id && added->level == 1);
    assert(player.next_instance_id == instance_id + 1);
    assert(player.runes.count == runes_before + 1);
    assert(player.runes.items[player.runes.count - 1].def == RuneDefIndex("rune-power-2"));
    assert(player.gear.counts[0] == 1);
    cJSON_Delete(response);

    cJSON *broken = cJSON_CreateObject();
    assert(!ProgressionAdoptServerProfile(&player, broken));
    assert(player.gems == 70);
    cJSON_Delete(broken);
    PlayerUnloadRoster(&player);
}

static void AssertBattleStartReplaysTheBattle(Game *g, unsigned seed) {
    cJSON *fields = GameBattleStartFields(g);
    assert(fields);
    cJSON *replay = cJSON_CreateObject();
    cJSON_AddNumberToObject(replay, "seed", seed);
    cJSON_AddItemToObject(replay, "player", cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(fields, "squad"), true));
    cJSON_AddItemToObject(replay, "enemy", cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(fields, "enemy"), true));
    cJSON_AddItemToObject(
        replay, "changer", cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(fields, "changer"), true)
    );
    cJSON *catalog_root = BattleReplayCatalogLoad();
    BattleReplayOutcome outcome;
    assert(BattleReplayRun(cJSON_GetObjectItemCaseSensitive(catalog_root, "characters"), replay, &outcome));

    static BattleSim played;
    BattleSimRun(&played, &g->player, &g->battle.enemy_roster, seed);
    char timeline[BATTLE_REPLAY_DIGEST_HEX];
    BattleReplayDigest(&played, timeline);
    assert(outcome.won == played.player_won && outcome.actions == played.action_count);
    assert(!strcmp(outcome.timeline, timeline));

    cJSON_Delete(catalog_root);
    cJSON_Delete(replay);
    cJSON_Delete(fields);
}

static Character FortifyTestHero(const cJSON *catalog, const char *id, Tier tier) {
    static Ability ability;
    Character ch;
    CharacterParse(CatalogFindCharacter(catalog, id), &ch, &ability);
    ch.tier = tier;
    ch.level = 1;
    ConsumeNormalizeCharacter(&ch);
    return ch;
}

static FortifyChild FortifyTestChild(const char *id, Tier tier, bool ascended) {
    FortifyChild child = {.tier = tier, .ascended = ascended};
    StrCopy(child.id, sizeof(child.id), id);
    return child;
}

static void AssertFortifyBonus(
    const Character *parent, int slot, FortifyChild child, FortifyStat stat, int rarity, int faction, int tier
) {
    FortifyBonus bonus = FortifySlotBonus(parent, slot, &child);
    assert(FortifySlotDefAt(parent, slot)->stat == stat);
    assert(bonus.rarity == rarity && bonus.faction == faction && bonus.tier == tier);
}

static void TestFortifyTablesCarryThe544Rules(void) {
    FortifyCatalogEnsure();
    assert(fortify_catalog.faction_bonus == 0.125f);
    assert(fortify_catalog.rarity_tier_bonus[RARITY_LEGENDARY - 1][TIER_IV] == 1.0f);
    assert(fortify_catalog.rarity_tier_bonus[RARITY_MYTHIC - 1][TIER_IV] == 1.24f);
    static const int remove_costs[RARITY_MYTHIC] = {5, 5, 5, 5, 15, 30};
    assert(memcmp(fortify_catalog.remove_gem_cost, remove_costs, sizeof(remove_costs)) == 0);
    assert(FortifyHeroFind("UNIT_ELVENMAGE") && !FortifyHeroFind("UNIT_DOES_NOT_EXIST"));
}

static void TestFortifyBonusesFollowThe544Calculator(void) {
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");

    Character elven_mage = FortifyTestHero(catalog, "UNIT_ELVENMAGE", TIER_IV);
    AssertFortifyBonus(
        &elven_mage, 0, FortifyTestChild("UNIT_REDIRONMAGE", TIER_II, false), FORTIFY_STAT_HEALTH, 68, 0, 7
    );
    elven_mage = FortifyTestHero(catalog, "UNIT_ELVENMAGE", TIER_II);
    AssertFortifyBonus(
        &elven_mage, 1, FortifyTestChild("UNIT_ELVENMAGE", TIER_IV, true), FORTIFY_STAT_POWER, 150, 24, 46
    );
    Character wanderer = FortifyTestHero(catalog, "UNIT_APOSTATEWANDERER", TIER_III);
    AssertFortifyBonus(
        &wanderer, 3, FortifyTestChild("UNIT_CASSANDRAPENTAGHAST", TIER_I, false), FORTIFY_STAT_POWER, 49, 0, 0
    );
    Character golem = FortifyTestHero(catalog, "UNIT_INFERNOGOLEM", TIER_IV);
    GearApplyAscension(&golem);
    AssertFortifyBonus(&golem, 2, FortifyTestChild("UNIT_OGRE", TIER_III, false), FORTIFY_STAT_HEALTH, 360, 54, 72);

    int power = CharacterStatPower(&golem), health = CharacterStatHealth(&golem);
    golem.fortify[2] = FortifyTestChild("UNIT_OGRE", TIER_III, false);
    golem.fortify[1] = FortifyTestChild("UNIT_ELVENMAGE", TIER_I, false);
    int power_bonus = FortifyBonusTotal(FortifySlotBonus(&golem, 1, &golem.fortify[1]));
    assert(power_bonus > 0 && FortifyBonusPower(&golem) == power_bonus && FortifyBonusHealth(&golem) == 486);
    assert(CharacterStatPower(&golem) == power + power_bonus && CharacterStatHealth(&golem) == health + 486);
    assert(CharacterLevelPower(&golem) == power && CharacterLevelHealth(&golem) == health);
    cJSON_Delete(root);
}

static void TestFortifyMaximaMatchThe5xCatalog(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    int checked = 0;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)catalog) {
        const cJSON *tiers = cJSON_GetObjectItemCaseSensitive((cJSON *)entry, "tiers");
        for (int tier = 0; tier < TIER_COUNT; tier++) {
            const cJSON *row = cJSON_GetObjectItemCaseSensitive((cJSON *)tiers, TIER_NAMES[tier]);
            const cJSON *max_power = cJSON_GetObjectItemCaseSensitive((cJSON *)row, "max_fortify_power");
            const cJSON *max_health = cJSON_GetObjectItemCaseSensitive((cJSON *)row, "max_fortify_health");
            if (!cJSON_IsNumber(max_power) || !cJSON_IsNumber(max_health)) continue;
            Character ch = FortifyTestHero(catalog, JsonStr(entry, "id"), (Tier)tier);
            assert(FortifyMaxBonus(&ch, FORTIFY_STAT_POWER) == max_power->valueint);
            assert(FortifyMaxBonus(&ch, FORTIFY_STAT_HEALTH) == max_health->valueint);
            checked++;
        }
    }
    assert(checked > 2000);
    cJSON_Delete(root);
}

static void TestFortifyMergesTiersLikeThe544Slot(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    Character parent = FortifyTestHero(catalog, "UNIT_ELVENMAGE", TIER_IV);
    Character same = FortifyTestHero(catalog, "UNIT_REDIRONMAGE", TIER_I);
    Character other = FortifyTestHero(catalog, "UNIT_OGRE", TIER_II);

    FortifyChild placed = FortifyMergedChild(&parent, 0, &same);
    assert(!strcmp(placed.id, "UNIT_REDIRONMAGE") && placed.tier == TIER_I && !placed.ascended);
    parent.fortify[0] = placed;
    assert(FortifyMergedChild(&parent, 0, &same).tier == TIER_II);
    parent.fortify[0].tier = TIER_II;
    assert(FortifyMergedChild(&parent, 0, &same).tier == TIER_III);
    FortifyChild replaced = FortifyMergedChild(&parent, 0, &other);
    assert(!strcmp(replaced.id, "UNIT_OGRE") && replaced.tier == TIER_II);

    parent.fortify[0].tier = TIER_IV;
    assert(FortifyReplacementBlocked(&parent, 0, &same));
    Character ascended = same;
    GearApplyAscension(&ascended);
    assert(!FortifyReplacementBlocked(&parent, 0, &ascended));
    FortifyChild promoted = FortifyMergedChild(&parent, 0, &ascended);
    assert(promoted.tier == TIER_IV && promoted.ascended);
    parent.fortify[0] = promoted;
    assert(FortifyReplacementBlocked(&parent, 0, &ascended) && FortifyReplacementBlocked(&parent, 0, &same));
    assert(!FortifyReplacementBlocked(&parent, 0, &other));
    cJSON_Delete(root);
}

static void TestFortifySlotsUnlockWithConsumeLevel(void) {
    cJSON *root = JsonLoadFile(HOD_CACHE "characters.json");
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    Character ch = FortifyTestHero(catalog, "UNIT_ELVENMAGE", TIER_I);
    assert(ch.consume_level == 1 && FortifySlotsUnlocked(&ch) == 2);
    const ConsumeProfile *profile = ConsumeProfileFor(&ch);
    ch.consume_xp = ConsumeXpForLevel(profile->consume_table, 4);
    ConsumeNormalizeCharacter(&ch);
    assert(FortifySlotsUnlocked(&ch) == 3);
    ch.consume_xp = ConsumeXpForLevel(profile->consume_table, profile->consume_table->count);
    ConsumeNormalizeCharacter(&ch);
    assert(FortifySlotsUnlocked(&ch) == FORTIFY_SLOT_COUNT);
    cJSON_Delete(root);
}

static void TestPlayerFortifyConsumesTheChildren(void) {
    Player player = TestStarterPlayer();
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    int parent = -1, first_child = -1, second_child = -1, locked_child = -1;
    ConsumeTestAdd(&player, catalog, "UNIT_ELVENMAGE", &parent);
    ConsumeTestAdd(&player, catalog, "UNIT_REDIRONMAGE", &first_child);
    ConsumeTestAdd(&player, catalog, "UNIT_OGRE", &second_child);
    ConsumeTestAdd(&player, catalog, "UNIT_APOSTATEWANDERER", &locked_child);
    int squad_member = PlayerSquadUnitAt(&player, player.current_squad, 0);
    assert(squad_member >= 0);
    uint64_t parent_id = player.roster[parent].instance_id;
    int roster_before = player.roster_count;

    FortifyPlacement squad[] = {{0, squad_member}};
    assert(PlayerFortify(&player, parent, squad, 1) < 0);
    FortifyPlacement locked[] = {{2, locked_child}};
    assert(PlayerFortify(&player, parent, locked, 1) < 0);
    FortifyPlacement twice[] = {{0, first_child}, {1, first_child}};
    assert(PlayerFortify(&player, parent, twice, 2) < 0);
    assert(player.roster_count == roster_before);

    FortifyPlacement placements[] = {{0, first_child}, {1, second_child}};
    cJSON *action = FortifyServerAction(&player, parent, placements, 2);
    assert(!strcmp(JsonStr(action, "action"), "fortify"));
    assert((uint64_t)cJSON_GetObjectItemCaseSensitive(action, "parent")->valuedouble == parent_id);
    const cJSON *slots = cJSON_GetObjectItemCaseSensitive(action, "slots");
    assert(cJSON_GetArraySize(slots) == 2 && JsonInt(cJSON_GetArrayItem(slots, 1), "index") == 1);
    assert((uint64_t)cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(slots, 1), "unit")->valuedouble ==
           player.roster[second_child].instance_id);
    cJSON_Delete(action);

    int power = CharacterStatPower(&player.roster[parent]), health = CharacterStatHealth(&player.roster[parent]);
    int kept = PlayerFortify(&player, parent, placements, 2);
    assert(kept >= 0 && player.roster[kept].instance_id == parent_id && player.roster_count == roster_before - 2);
    const Character *fortified = &player.roster[kept];
    assert(!strcmp(fortified->fortify[0].id, "UNIT_REDIRONMAGE") && !strcmp(fortified->fortify[1].id, "UNIT_OGRE"));
    assert(FortifyHasChildren(fortified) && FortifyFilledCount(fortified) == 2);
    assert(CharacterStatHealth(fortified) == health + FortifyBonusHealth(fortified) && FortifyBonusHealth(fortified) > 0);
    assert(CharacterStatPower(fortified) == power + FortifyBonusPower(fortified) && FortifyBonusPower(fortified) > 0);
    for (int i = 0; i < player.roster_count; i++) assert(strcmp(player.roster[i].id, "UNIT_OGRE") != 0);

    cJSON_Delete(root);
    PlayerUnloadRoster(&player);
}

static void TestPlayerRemoveFortifyReturnsHeroesForGems(void) {
    Player player = TestStarterPlayer();
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    int parent = -1;
    Character *ch = ConsumeTestAdd(&player, catalog, "UNIT_ELVENMAGE", &parent);
    ch->fortify[0] = FortifyTestChild("UNIT_REDIRONMAGE", TIER_II, false);
    ch->fortify[3] = FortifyTestChild("UNIT_OGRE", TIER_III, true);
    assert(FortifyRemoveCost(ch, 0) == 5 && FortifyRemoveCost(ch, 3) == 30);
    assert(FortifyRemoveCost(ch, FORTIFY_REMOVE_ALL) == 35);

    cJSON *action = FortifyRemoveServerAction(&player, parent, FORTIFY_REMOVE_ALL);
    assert(!strcmp(JsonStr(action, "action"), "remove_fortify") && JsonInt(action, "index") == FORTIFY_REMOVE_ALL);
    cJSON_Delete(action);

    player.gems = 34;
    int roster_before = player.roster_count;
    assert(PlayerRemoveFortify(&player, parent, FORTIFY_REMOVE_ALL, NULL) == 0);
    assert(player.gems == 34 && player.roster_count == roster_before && FortifyFilledCount(ch) == 2);
    assert(PlayerRemoveFortify(&player, parent, 5, NULL) == 0);

    uint64_t next_instance = player.next_instance_id;
    int first = -1;
    assert(PlayerRemoveFortify(&player, parent, 3, &first) == 1);
    assert(player.gems == 4 && player.roster_count == roster_before + 1 && first == roster_before);
    const Character *returned = &player.roster[first];
    assert(!strcmp(returned->id, "UNIT_OGRE") && returned->tier == TIER_III && returned->ascended);
    assert(returned->rarity == RARITY_MYTHIC && returned->level == 1 && returned->instance_id == next_instance);
    assert(ConsumeProgressionLevelOfTier(ConsumeProfileFor(returned), TIER_III) + 1 <= returned->consume_level);
    assert(!FortifySlotFilled(&player.roster[parent], 3) && FortifySlotFilled(&player.roster[parent], 0));

    assert(PlayerRemoveFortify(&player, parent, FORTIFY_REMOVE_ALL, &first) == 0);
    player.gems = 5;
    assert(PlayerRemoveFortify(&player, parent, FORTIFY_REMOVE_ALL, &first) == 1 && player.gems == 0);
    assert(!FortifyHasChildren(&player.roster[parent]));
    cJSON_Delete(root);
    PlayerUnloadRoster(&player);
}

static void TestFortifySlotsTravelThroughSavesAndBattles(void) {
    const char *path = "tests/fortify_save_test.dat";
    static Game game;
    Game *g = &game;
    ProgressionInitializePlayerDefaults(&g->player);
    PlayerLoadRoster(&g->player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    int hero = PlayerSquadUnitAt(&g->player, g->player.current_squad, 0);
    Character *ch = &g->player.roster[hero];
    ch->fortify[0] = FortifyTestChild("UNIT_REDIRONMAGE", TIER_II, false);
    ch->fortify[1] = FortifyTestChild("UNIT_OGRE", TIER_IV, true);
    ch->fortify[7] = FortifyTestChild("UNIT_ELVENMAGE", TIER_I, false);

    cJSON *slots = FortifyChildrenJson(ch);
    assert(cJSON_GetArraySize(slots) == 3);
    const cJSON *ogre = cJSON_GetArrayItem(slots, 1);
    assert(JsonInt(ogre, "slot") == 1 && !strcmp(JsonStr(ogre, "tier"), "IV") && JsonBool(ogre, "ascended"));
    assert(!cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(slots, 0), "ascended"));
    cJSON_Delete(slots);
    Character blank = *ch;
    assert(FortifyChildrenLoad(&blank, NULL) && !FortifyHasChildren(&blank));
    cJSON *bad = cJSON_Parse("[{\"slot\":2,\"id\":\"UNIT_OGRE\",\"tier\":\"IV\"},{\"slot\":2,\"id\":\"UNIT_OGRE\",\"tier\":\"I\"}]");
    assert(!FortifyChildrenLoad(&blank, bad) && !FortifyHasChildren(&blank));
    cJSON_Delete(bad);

    assert(ProgressionSaveToPath(&g->player, path));
    Player loaded = {0};
    ProgressionInitializePlayerDefaults(&loaded);
    PlayerLoadRoster(&loaded, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    assert(ProgressionLoadFromPath(&loaded, path));
    assert(memcmp(loaded.roster[hero].fortify, ch->fortify, sizeof(ch->fortify)) == 0);
    PlayerUnloadRoster(&loaded);
    remove(path);

    BattleRosterLoad(
        &g->battle.enemy_roster, "data/campaign_rosters.json", HOD_CACHE "characters.json", "PVE_MAP_01", 2
    );
    g->battle_map_index = 0;
    g->battle_node_index = 1;
    cJSON *fields = GameBattleStartFields(g);
    const cJSON *unit = cJSON_GetArrayItem(
        cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(fields, "squad"), "units"), 0
    );
    assert(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(unit, "fortify")) == 3);
    cJSON_Delete(fields);
    for (unsigned seed = 1; seed <= 4; seed++) AssertBattleStartReplaysTheBattle(g, seed);

    SimUnit units[SIM_UNIT_COUNT];
    SimUnitsInit(units, 0, 2, ch, NULL);
    assert(units[2].start_power == CharacterLevelPower(ch) + FortifyBonusPower(ch));
    assert(units[2].max_health == CharacterLevelHealth(ch) + FortifyBonusHealth(ch));
    assert(FortifyBonusHealth(ch) > 0 && FortifyBonusPower(ch) > 0);
    PlayerUnloadRoster(&g->player);
}

static void TestBattleStartSendsTheBattleTheClientPlays(void) {
    static Game game;
    Game *g = &game;
    ProgressionInitializePlayerDefaults(&g->player);
    PlayerLoadRoster(&g->player, HOD_CACHE "characters.json", HOD_CACHE "player_starter.json");
    long long now = RuneNow();
    uint64_t uuid = RuneGrant(&g->player.runes, RuneDefIndex("rune-power-1"), now);
    assert(uuid && RuneActivate(&g->player.runes, g->player.current_squad, 0, uuid, now) == RUNE_ASSIGN_OK);
    for (int r = 0; r < g->player.roster_count; r++) g->player.roster[r].level = 1 + r;

    BattleRosterLoad(
        &g->battle.enemy_roster, "data/campaign_rosters.json", HOD_CACHE "characters.json", "PVE_MAP_01", 2
    );
    g->battle_map_index = 0;
    g->battle_node_index = 1;
    cJSON *fields = GameBattleStartFields(g);
    assert(!strcmp(JsonStr(fields, "mode"), "campaign") && JsonInt(fields, "node") == 1);
    const cJSON *rune_ids = cJSON_GetObjectItemCaseSensitive(fields, "rune_ids");
    assert(cJSON_GetArraySize(rune_ids) == 1 && (uint64_t)cJSON_GetArrayItem(rune_ids, 0)->valuedouble == uuid);
    const cJSON *squad_runes =
        cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(fields, "squad"), "runes");
    assert(!strcmp(cJSON_GetArrayItem(squad_runes, 0)->valuestring, "rune-power-1"));
    cJSON_Delete(fields);
    for (unsigned seed = 1; seed <= 8; seed++) AssertBattleStartReplaysTheBattle(g, seed);

    g->battle_is_pvp = true;
    g->pvp_event = true;
    g->pvp_match = (GamePvpMatch){.id = 7, .at = ServerNow()};
    GamePvpMatchRequest event = {.event = true, .bot = GAME_PVP_MATCHMAKING};
    g->pvp_opponent = GameLocalOpponent(g, &event, g->pvp_match.at);
    PvpOpponentRoster(&g->battle.enemy_roster, &g->pvp_opponent);
    g->battle.enemy_roster.game_changer = &PvpEventForCycle(PvpEventCycle(g->pvp_match.at))->changer;
    fields = GameBattleStartFields(g);
    assert(JsonInt(fields, "match") == 7 && JsonBool(fields, "event"));
    assert(!cJSON_GetObjectItemCaseSensitive(fields, "opponent"));
    assert(cJSON_IsObject(cJSON_GetObjectItemCaseSensitive(fields, "changer")));
    cJSON_Delete(fields);
    for (unsigned seed = 1; seed <= 8; seed++) AssertBattleStartReplaysTheBattle(g, seed);

    g->pvp_event = false;
    g->pvp_revenge_at = 1234;
    fields = GameBattleStartFields(g);
    const cJSON *opponent = cJSON_GetObjectItemCaseSensitive(fields, "opponent");
    assert(!strcmp(JsonStr(opponent, "kind"), "revenge") && JsonInt(fields, "revenge_at") == 1234);
    assert((unsigned)cJSON_GetObjectItemCaseSensitive(opponent, "seed")->valuedouble == g->pvp_opponent.seed);
    assert(!cJSON_GetObjectItemCaseSensitive(fields, "match"));
    cJSON_Delete(fields);
    PlayerUnloadRoster(&g->player);
}

static void TestPvpMatchAsksTheServerForTheOpponent(void) {
    GamePvpMatchRequest ladder = {.bot = GAME_PVP_MATCHMAKING};
    cJSON *fields = GamePvpMatchFields(&ladder);
    assert(!strcmp(JsonStr(fields, "action"), "pvp_match") && !JsonBool(fields, "event"));
    assert(!cJSON_GetObjectItemCaseSensitive(fields, "bot") && !cJSON_GetObjectItemCaseSensitive(fields, "next"));
    cJSON_Delete(fields);

    GamePvpMatchRequest next = {.event = true, .bot = GAME_PVP_MATCHMAKING, .next = true};
    fields = GamePvpMatchFields(&next);
    assert(JsonBool(fields, "event") && JsonBool(fields, "next") && !cJSON_GetObjectItemCaseSensitive(fields, "bot"));
    cJSON_Delete(fields);

    GamePvpMatchRequest chosen = {.event = true, .bot = 0};
    fields = GamePvpMatchFields(&chosen);
    assert(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(fields, "bot")) && JsonInt(fields, "bot") == 0);
    cJSON_Delete(fields);
}

static void GameTestSettlePvpMatch(cJSON *reply) {
    ServerActionsPop(&server_actions);
    server_actions.reply_tag = SERVER_REPLY_PVP_MATCH;
    server_actions.reply = reply;
}

static void TestPvpMatchWaitsForTheServersReply(void) {
    static Game game;
    Game *g = &game;
    memset(g, 0, sizeof(*g));
    ProgressionInitializePlayerDefaults(&g->player);
    bool backend_was_enabled = backend_state.enabled;
    backend_state.enabled = true;

    g->screen = SCREEN_PVP_EVENT;
    GameMatchPvp(g, true, 3, false, SCREEN_PVP_EVENT);
    GameMatchPvp(g, false, GAME_PVP_MATCHMAKING, false, SCREEN_PVP_HUB);
    assert(g->pvp_match_waiting && g->screen == SCREEN_PVP_EVENT && server_actions.count == 1);
    const ServerActionEntry *entry = &server_actions.queue[0];
    assert(entry->tag == SERVER_REPLY_PVP_MATCH && !strcmp(JsonStr(entry->fields, "action"), "pvp_match"));
    assert(JsonBool(entry->fields, "event") && JsonInt(entry->fields, "bot") == 3);

    GameTestSettlePvpMatch(NULL);
    GamePollPvpMatch(g);
    assert(!g->pvp_match_waiting && g->screen == SCREEN_PVP_EVENT && !g->battle_is_pvp);

    g->screen = SCREEN_PVP_HUB;
    GameMatchPvp(g, false, GAME_PVP_MATCHMAKING, false, SCREEN_PVP_HUB);
    assert(g->pvp_match_waiting && server_actions.count == 1);
    g->screen = SCREEN_HOME;
    PvpOpponent offered = PvpOpponentGenerate(1, 0, 20000, 11);
    cJSON *reply = cJSON_CreateObject();
    cJSON *match = cJSON_AddObjectToObject(cJSON_AddObjectToObject(reply, "granted"), "match");
    cJSON_AddNumberToObject(match, "id", 4);
    cJSON_AddNumberToObject(match, "bot", offered.bot);
    cJSON_AddNumberToObject(match, "squad", offered.squad);
    cJSON_AddNumberToObject(match, "level", offered.level);
    cJSON_AddNumberToObject(match, "trophies", offered.trophies);
    cJSON_AddNumberToObject(match, "seed", offered.seed);
    GameTestSettlePvpMatch(reply);
    GamePollPvpMatch(g);
    assert(!g->pvp_match_waiting && g->screen == SCREEN_HOME && !g->battle_is_pvp && g->pvp_match.id == 0);
    assert(ServerActionsDrained());

    backend_state.enabled = backend_was_enabled;
}

static void TestPvpOpponentFromServerMatchRebuildsTheBot(void) {
    PvpOpponent expected = PvpOpponentFromBot(3, 20, 400, 20000, 0x9E3779B1u);
    cJSON *match = cJSON_CreateObject();
    cJSON_AddNumberToObject(match, "id", 5);
    cJSON_AddNumberToObject(match, "bot", expected.bot);
    cJSON_AddNumberToObject(match, "squad", expected.squad);
    cJSON_AddNumberToObject(match, "level", expected.level);
    cJSON_AddNumberToObject(match, "trophies", expected.trophies);
    cJSON_AddNumberToObject(match, "seed", expected.seed);
    PvpOpponent rebuilt;
    assert(PvpOpponentFromServerMatch(match, 400, &rebuilt));
    assert(memcmp(&rebuilt, &expected, sizeof(rebuilt)) == 0);

    cJSON_ReplaceItemInObject(match, "bot", cJSON_CreateNumber(PvpTableEnsure()->bot_count));
    assert(!PvpOpponentFromServerMatch(match, 400, &rebuilt));
    cJSON_ReplaceItemInObject(match, "bot", cJSON_CreateNumber(expected.bot));
    cJSON_ReplaceItemInObject(match, "squad", cJSON_CreateNumber(-1));
    assert(!PvpOpponentFromServerMatch(match, 400, &rebuilt));
    cJSON_ReplaceItemInObject(match, "squad", cJSON_CreateNumber(expected.squad));
    cJSON_DeleteItemFromObject(match, "seed");
    assert(!PvpOpponentFromServerMatch(match, 400, &rebuilt));
    assert(!PvpOpponentFromServerMatch(NULL, 400, &rebuilt));
    cJSON_Delete(match);
}

static int ExpeditionTestDef(const char *id) {
    int def = ExpeditionIndexById(id);
    assert(def >= 0);
    return def;
}

#define TIMED_QUEST_TEST_DAY 20402LL
#define TIMED_QUEST_TEST_NOON (TIMED_QUEST_TEST_DAY * TIMED_QUEST_SECONDS_PER_DAY + 43200LL)

static int TimedQuestTestSlot(const char *id) {
    for (int slot = 0; slot < TimedQuestSlotCount(); slot++)
        if (!strcmp(TimedQuestSlotAt(slot)->id, id)) return slot;
    assert(false);
    return TIMED_QUEST_NONE;
}

static void TestTimedQuestCatalogFollowsTheTable(void) {
    if (loc.count == 0) LocLoad(ORIGINAL_LOCALIZATION_DIR "/Loc_RUS_RU.txt");
    assert(TimedQuestCatalogLoad(TIMED_QUESTS_PATH) && TimedQuestsAvailable());
    assert(TimedQuestUnlockLevel() == 3 && TimedQuestDailySwaps() == 1);
    assert(TimedQuestSlotCount() == 5 && TimedQuestWeeklyCount() == 5);
    assert(TimedQuestChestCount(TIMED_QUEST_DAILY) == 3 && TimedQuestChestCount(TIMED_QUEST_WEEKLY) == 2);
    assert(TimedQuestMaxPoints(TIMED_QUEST_DAILY) == 100 && TimedQuestMaxPoints(TIMED_QUEST_WEEKLY) == 100);
    int daily_points = 0;
    for (int slot = 0; slot < TimedQuestSlotCount(); slot++) daily_points += TimedQuestSlotAt(slot)->quests[0].points;
    assert(daily_points == TimedQuestMaxPoints(TIMED_QUEST_DAILY));
    for (int period = 0; period < TIMED_QUEST_PERIOD_COUNT; period++)
        for (int i = 0; i < TimedQuestChestCount((TimedQuestPeriod)period); i++) {
            const TimedQuestChest *chest = TimedQuestChestAt((TimedQuestPeriod)period, i);
            assert(ResourceExists(chest->fallback));
        }
    for (int stat = 0; stat < TIMED_QUEST_STAT_COUNT; stat++) {
        const TimedQuestStatText *text = TimedQuestText((TimedQuestStat)stat);
        if (!text->title_key[0]) continue;
        assert(ResourceExists(text->icon));
        assert(LocGet(text->title_key)[0] && LocGet(text->description_key)[0]);
    }
    assert(!TimedQuestCatalogLoad("data/missing_timed_quests.json") && !TimedQuestsAvailable());
    assert(TimedQuestCatalogLoad(TIMED_QUESTS_PATH));
}

static void TestTimedQuestDaysTurnAtUtcMidnightAndWeeksOnMonday(void) {
    assert(TimedQuestDay(86399) == 0 && TimedQuestDay(86400) == 1 && TimedQuestDay(-1) == -1);
    assert(
        TimedQuestWeek(3 * TIMED_QUEST_SECONDS_PER_DAY) == 0 && TimedQuestWeek(4 * TIMED_QUEST_SECONDS_PER_DAY) == 1
    );
    assert(TimedQuestResetIn(TIMED_QUEST_DAILY, TIMED_QUEST_TEST_NOON) == 43200);
    assert(
        TimedQuestWeek(TIMED_QUEST_TEST_NOON) != TimedQuestWeek(TIMED_QUEST_TEST_NOON - TIMED_QUEST_SECONDS_PER_DAY)
    );
    assert(TimedQuestResetIn(TIMED_QUEST_WEEKLY, TIMED_QUEST_TEST_NOON) == 6 * TIMED_QUEST_SECONDS_PER_DAY + 43200);
    long long monday = TIMED_QUEST_TEST_DAY * TIMED_QUEST_SECONDS_PER_DAY;
    assert(TimedQuestResetIn(TIMED_QUEST_WEEKLY, monday) == 7 * TIMED_QUEST_SECONDS_PER_DAY);
}

static void TestTimedQuestDailyPicksMatchTheServer(void) {
    static const int expected[][5] = {{1, 1, 2, 1, 0}, {0, 1, 2, 2, 2}, {1, 0, 0, 1, 0}};
    PlayerTimedQuests q = {0};
    for (int day = 0; day < 3; day++) {
        TimedQuestsRoll(&q, (20400LL + day) * TIMED_QUEST_SECONDS_PER_DAY);
        for (int slot = 0; slot < 5; slot++) assert(TimedQuestDailyPool(&q, slot) == expected[day][slot]);
    }
    TimedQuestsRoll(&q, TIMED_QUEST_TEST_NOON);
    assert(!strcmp(TimedQuestDailyAt(&q, TimedQuestTestSlot("campaign"))->id, "d_campaign_5"));
    assert(!strcmp(TimedQuestDailyAt(&q, TimedQuestTestSlot("pvp"))->id, "d_pvp_fight_3"));
}

static void TestTimedQuestCountsResetDailyAndWeekly(void) {
    PlayerTimedQuests q = {0};
    long long monday = TIMED_QUEST_TEST_DAY * TIMED_QUEST_SECONDS_PER_DAY;
    TimedQuestsRecordBattle(&q, TIMED_QUEST_BATTLE_PVP, true, 4, true, monday);
    TimedQuestsRecordBattle(&q, TIMED_QUEST_BATTLE_CAMPAIGN, false, 1, false, monday + 60);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_PVP_WINS] == 1);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_PVP_BATTLES] == 1);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_BATTLE_WINS] == 1);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_CAMPAIGN_WINS] == 0);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_CRITS] == 5);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_FACTION_HITS] == 1);
    assert(q.counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_ACTIVE_DAYS] == 1);

    long long tuesday = monday + TIMED_QUEST_SECONDS_PER_DAY;
    TimedQuestsRecordBattle(&q, TIMED_QUEST_BATTLE_NEXUS, true, 0, false, tuesday);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_PVP_WINS] == 0);
    assert(q.counts[TIMED_QUEST_DAILY][TIMED_QUEST_STAT_NEXUS_WINS] == 1);
    assert(q.counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_PVP_WINS] == 1);
    assert(q.counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_BATTLE_WINS] == 2);
    assert(q.counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_ACTIVE_DAYS] == 2);

    TimedQuestsRoll(&q, monday + 7 * TIMED_QUEST_SECONDS_PER_DAY);
    assert(q.counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_BATTLE_WINS] == 0 && q.week == TimedQuestWeek(monday) + 1);
}

static Player TimedQuestTestPlayer(void) {
    Player p = TestStarterPlayer();
    p.level = TimedQuestUnlockLevel();
    p.timed_quests = (PlayerTimedQuests){0};
    GearCatalogEnsure();
    RuneCatalogEnsure();
    return p;
}

static void TestTimedQuestClaimsPayOnceAndFillTheChestTrack(void) {
    Player p = TimedQuestTestPlayer();
    PlayerTimedQuests *q = &p.timed_quests;
    int campaign = TimedQuestTestSlot("campaign"), pvp = TimedQuestTestSlot("pvp");
    p.level = TimedQuestUnlockLevel() - 1;
    for (int i = 0; i < 5; i++)
        TimedQuestsRecordBattle(q, TIMED_QUEST_BATTLE_CAMPAIGN, true, 0, false, TIMED_QUEST_TEST_NOON);
    assert(!TimedQuestClaimDaily(&p, campaign, TIMED_QUEST_TEST_NOON) && TimedQuestsClaimableCount(&p) == 0);
    p.level = TimedQuestUnlockLevel();
    assert(TimedQuestDailyClaimable(q, campaign) && TimedQuestsClaimableCount(&p) == 1);
    int gold = p.gold;
    assert(TimedQuestClaimDaily(&p, campaign, TIMED_QUEST_TEST_NOON));
    assert(p.gold == gold + 900 && TimedQuestDailyClaimed(q, campaign) && !TimedQuestDailyClaimable(q, campaign));
    assert(!TimedQuestClaimDaily(&p, campaign, TIMED_QUEST_TEST_NOON) && p.gold == gold + 900);
    assert(
        TimedQuestPoints(q, TIMED_QUEST_DAILY) == 20 &&
        q->counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_DAILY_QUESTS] == 1
    );
    assert(!TimedQuestChestReached(q, TIMED_QUEST_DAILY, 0));
    assert(!TimedQuestClaimDaily(&p, pvp, TIMED_QUEST_TEST_NOON));

    int gems = p.gems;
    for (int i = 0; i < 3; i++)
        TimedQuestsRecordBattle(q, TIMED_QUEST_BATTLE_PVP, true, 0, false, TIMED_QUEST_TEST_NOON);
    assert(TimedQuestClaimDaily(&p, pvp, TIMED_QUEST_TEST_NOON) && p.gems == gems + 2);
    assert(TimedQuestChestOpenable(q, TIMED_QUEST_DAILY, 0) && TimedQuestsClaimableCount(&p) == 1);
    ExpeditionReward reward = TimedQuestOpenChest(&p, TIMED_QUEST_DAILY, 0, TIMED_QUEST_TEST_NOON);
    assert(reward.gold == 1000 && !ExpeditionRewardHasItems(&reward) && q->chests_opened[TIMED_QUEST_DAILY][0]);
    assert(TimedQuestCheckChest(&p, TIMED_QUEST_DAILY, 0) == TIMED_QUEST_CHEST_OPENED);
    assert(TimedQuestCheckChest(&p, TIMED_QUEST_DAILY, 1) == TIMED_QUEST_CHEST_LOCKED);

    TimedQuestsRoll(q, TIMED_QUEST_TEST_NOON + TIMED_QUEST_SECONDS_PER_DAY);
    assert(TimedQuestPoints(q, TIMED_QUEST_DAILY) == 0 && !q->chests_opened[TIMED_QUEST_DAILY][0]);
    assert(q->counts[TIMED_QUEST_WEEKLY][TIMED_QUEST_STAT_DAILY_QUESTS] == 2);
}

static void TestTimedQuestSwapsOneUnfinishedQuestADay(void) {
    Player p = TimedQuestTestPlayer();
    PlayerTimedQuests *q = &p.timed_quests;
    int campaign = TimedQuestTestSlot("campaign"), pvp = TimedQuestTestSlot("pvp");
    TimedQuestsRoll(q, TIMED_QUEST_TEST_NOON);
    const char *before = TimedQuestDailyAt(q, pvp)->id;
    assert(TimedQuestSwap(&p, pvp, TIMED_QUEST_TEST_NOON));
    assert(strcmp(TimedQuestDailyAt(q, pvp)->id, before) != 0 && q->swaps_used == 1);
    assert(!TimedQuestCanSwap(q, campaign) && !TimedQuestSwap(&p, campaign, TIMED_QUEST_TEST_NOON));
    cJSON *swap = TimedQuestSwapAction(pvp);
    assert(!strcmp(JsonStr(swap, "action"), "quest_swap") && !strcmp(JsonStr(swap, "slot"), "pvp"));
    cJSON_Delete(swap);

    TimedQuestsRoll(q, TIMED_QUEST_TEST_NOON + TIMED_QUEST_SECONDS_PER_DAY);
    for (int i = 0; i < 6; i++)
        TimedQuestsRecordBattle(
            q, TIMED_QUEST_BATTLE_CAMPAIGN, true, 0, false, TIMED_QUEST_TEST_NOON + TIMED_QUEST_SECONDS_PER_DAY
        );
    assert(TimedQuestDailyClaimable(q, campaign) && !TimedQuestCanSwap(q, campaign));
}

static void TestTimedQuestHeroChestsNeedRosterRoom(void) {
    Player p = TimedQuestTestPlayer();
    PlayerTimedQuests *q = &p.timed_quests;
    TimedQuestsRoll(q, TIMED_QUEST_TEST_NOON);
    for (int i = 0; i < TimedQuestWeeklyCount(); i++) q->weekly_claimed[i] = true;
    int capacity = p.roster_capacity;
    p.roster_capacity = p.roster_count;
    assert(TimedQuestCheckChest(&p, TIMED_QUEST_WEEKLY, 1) == TIMED_QUEST_CHEST_ROSTER_FULL);
    assert(TimedQuestOpenChest(&p, TIMED_QUEST_WEEKLY, 1, TIMED_QUEST_TEST_NOON).gems == 0);
    p.roster_capacity = capacity;
    int roster = p.roster_count, gems = p.gems;
    ExpeditionReward reward = TimedQuestOpenChest(&p, TIMED_QUEST_WEEKLY, 1, TIMED_QUEST_TEST_NOON);
    assert(reward.hero_count == 1 && reward.rune_count == 1 && p.roster_count == roster + 1 && p.gems == gems + 15);
    Rarity rarity = p.roster[reward.heroes[0]].rarity;
    assert(rarity == RARITY_EPIC || rarity == RARITY_LEGENDARY);
    cJSON *chest = TimedQuestChestAction(TIMED_QUEST_WEEKLY, 1);
    assert(!strcmp(JsonStr(chest, "action"), "quest_chest") && !strcmp(JsonStr(chest, "chest"), "weekly_chest_2"));
    cJSON_Delete(chest);
}

static void TestTimedQuestsSurviveSaveAndLoad(void) {
    Player p = TimedQuestTestPlayer();
    PlayerTimedQuests *q = &p.timed_quests;
    int heroes = TimedQuestTestSlot("heroes");
    TimedQuestsCount(q, TIMED_QUEST_STAT_CONSUMED, 5, TIMED_QUEST_TEST_NOON);
    TimedQuestsCount(q, TIMED_QUEST_STAT_SUMMONED, 2, TIMED_QUEST_TEST_NOON);
    assert(TimedQuestClaimDaily(&p, heroes, TIMED_QUEST_TEST_NOON));
    assert(TimedQuestSwap(&p, TimedQuestTestSlot("pvp"), TIMED_QUEST_TEST_NOON));
    q->weekly_claimed[2] = true;
    q->chests_opened[TIMED_QUEST_WEEKLY][0] = true;
    cJSON *json = TimedQuestsSave(q);
    PlayerTimedQuests loaded;
    TimedQuestsLoad(json, &loaded);
    assert(memcmp(&loaded, q, sizeof(loaded)) == 0);
    cJSON *claimed = cJSON_GetObjectItemCaseSensitive(json, "daily_claimed");
    cJSON_AddItemToArray(claimed, cJSON_CreateString("no_such_quest"));
    TimedQuestsLoad(json, &loaded);
    assert(memcmp(&loaded, q, sizeof(loaded)) == 0);
    cJSON_Delete(json);
}

static void TestExpeditionCatalogFollowsTheTable(void) {
    assert(ExpeditionCatalogLoad(EXPEDITIONS_PATH) && ExpeditionsAvailable());
    assert(ExpeditionCount() == 10 && ExpeditionUnlockLevel() == 3);
    assert(ExpeditionSlotsUnlocked(1) == 0 && ExpeditionSlotsUnlocked(3) == 1 && ExpeditionSlotsUnlocked(8) == 2);
    assert(ExpeditionSlotsUnlocked(40) == EXPEDITION_SLOT_CAPACITY && ExpeditionNextSlotLevel(40) == 0);
    assert(ExpeditionNextSlotLevel(8) == 15);
    const ExpeditionDef *patrol = ExpeditionAt(ExpeditionTestDef("lothering"));
    assert(patrol->minutes == 15 && patrol->party == 1 && ExpeditionDurationSeconds(patrol) == 900);
    assert(patrol->bonus.kind == EXPEDITION_TRAIT_GROUP && !strcmp(patrol->bonus.group, "Ferelden"));
    assert(ExpeditionGreatSuccessAmount(120, true) == 180 && ExpeditionGreatSuccessAmount(120, false) == 120);
    assert(ExpeditionHeroDropLimit(ExpeditionAt(ExpeditionTestDef("tevinter"))) == 1);
    assert(ExpeditionHeroDropLimit(patrol) == 0);
    assert(!ExpeditionCatalogLoad("data/missing_expeditions.json") && !ExpeditionsAvailable());
    assert(ExpeditionCatalogLoad(EXPEDITIONS_PATH));
}

static void TestExpeditionTraitsMatchRealHeroes(void) {
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    static Character hero;
    static Ability ability;
    for (int def = 0; def < ExpeditionCount(); def++) {
        const ExpeditionDef *expedition = ExpeditionAt(def);
        int requirement_matches[EXPEDITION_REQUIREMENT_CAPACITY] = {0};
        int bonus_matches = 0;
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, (cJSON *)catalog) {
            if (!StoreHeroDefinitionEligible(entry)) continue;
            CharacterParse(entry, &hero, &ability);
            bonus_matches += ExpeditionTraitMatches(&expedition->bonus, &hero);
            for (int i = 0; i < expedition->requirement_count; i++)
                requirement_matches[i] += ExpeditionTraitMatches(&expedition->requirements[i], &hero);
        }
        assert(bonus_matches >= expedition->bonus.count * 4);
        for (int i = 0; i < expedition->requirement_count; i++)
            assert(requirement_matches[i] >= expedition->requirements[i].count);
        assert(ResourceExists(TextFormat(EXTRA_ASSET_DIR "%s", expedition->art)));
    }
    cJSON_Delete(root);
}

typedef struct {
    Player player;
    int dwarf, apprentice, skeleton_mage, red_mage, rare, epic, giant, ferelden;
} ExpeditionBench;

static ExpeditionBench ExpeditionTestBench(void) {
    ExpeditionBench bench = {.player = TestStarterPlayer()};
    cJSON *root = CharacterCatalogAddCustomHeroes(JsonLoadFile(HOD_CACHE "characters.json"));
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    Player *p = &bench.player;
    p->level = 30;
    ConsumeTestAdd(p, catalog, "UNIT_CARTATHUG", &bench.dwarf);
    ConsumeTestAdd(p, catalog, "UNIT_CIRCLEAPPRENTICE", &bench.apprentice);
    ConsumeTestAdd(p, catalog, "UNIT_SKELETONMAGE", &bench.skeleton_mage);
    ConsumeTestAdd(p, catalog, "UNIT_REDIRONMAGE", &bench.red_mage);
    ConsumeTestAdd(p, catalog, ConsumeTestFirstId(catalog, "Rare", false, true), &bench.rare);
    ConsumeTestAdd(p, catalog, ConsumeTestFirstId(catalog, "Epic", false, true), &bench.epic);
    ConsumeTestAdd(p, catalog, "UNIT_OGRE", &bench.giant);
    ConsumeTestAdd(p, catalog, "UNIT_BLACKSTONEIRREGULAR", &bench.ferelden);
    cJSON_Delete(root);
    GearCatalogEnsure();
    RuneCatalogEnsure();
    return bench;
}

static void TestExpeditionPartiesNeedBenchHeroesAndTheirRequirements(void) {
    ExpeditionBench bench = ExpeditionTestBench();
    Player *p = &bench.player;
    int rift = ExpeditionTestDef("fade_rift");
    int squad_member = PlayerSquadUnitAt(p, p->current_squad, 0);
    assert(squad_member >= 0 && !ExpeditionHeroAvailable(p, squad_member));

    int commons[] = {bench.dwarf, bench.apprentice, bench.skeleton_mage};
    assert(!ExpeditionPartyValid(p, rift, commons, 3));
    int with_rare[] = {bench.dwarf, bench.ferelden, bench.rare};
    assert(ExpeditionPartyValid(p, rift, with_rare, 3));
    assert(!ExpeditionPartyGreatSuccess(p, rift, with_rare, 3));
    int with_squad[] = {squad_member, bench.apprentice, bench.rare};
    assert(!ExpeditionPartyValid(p, rift, with_squad, 3));
    int twice[] = {bench.rare, bench.rare, bench.apprentice};
    assert(!ExpeditionPartyValid(p, rift, twice, 3));
    assert(!ExpeditionPartyValid(p, rift, with_rare, 2));

    int party[EXPEDITION_PARTY_CAPACITY];
    assert(ExpeditionAutoFill(p, rift, party) == 3);
    assert(ExpeditionPartyValid(p, rift, party, 3) && ExpeditionPartyGreatSuccess(p, rift, party, 3));

    assert(ExpeditionStart(p, rift, party, 3, 1000));
    assert(p->expeditions.count == 1 && p->expeditions.active[0].ends_at == 1000 + 240 * 60);
    for (int i = 0; i < 3; i++) {
        assert(PlayerHeroAway(p, party[i]) && !ExpeditionHeroAvailable(p, party[i]));
        assert(!PlayerSquadPlace(p, 2, 0, party[i]));
        assert(!PlayerConsumeSeedAllowed(p, bench.giant, party[i]));
        assert(!PlayerConsumeSeedAllowed(p, party[i], bench.giant));
        assert(!FortifyPlacementAllowed(p, bench.giant, 0, party[i]));
    }
    assert(!ExpeditionStart(p, rift, party, 3, 1000));
    assert(!ExpeditionCanStart(p, rift));

    p->level = 3;
    int patrol[] = {bench.ferelden};
    assert(ExpeditionSlotsUnlocked(p->level) == 1 && !ExpeditionStart(p, ExpeditionTestDef("lothering"), patrol, 1, 1000));
    p->level = 2;
    assert(!ExpeditionCanStart(p, ExpeditionTestDef("storm_coast")));
}

static void TestExpeditionClaimPaysOnceWhenThePartyIsBack(void) {
    ExpeditionBench bench = ExpeditionTestBench();
    Player *p = &bench.player;
    int lothering = ExpeditionTestDef("lothering");
    int party[] = {bench.ferelden};
    assert(ExpeditionStart(p, lothering, party, 1, 1000));
    int gold = p->gold, gear = 0, xp = p->roster[bench.ferelden].xp;
    for (int i = 0; i < GearCatalogCount(); i++) gear += p->gear.counts[i];

    assert(ExpeditionCheckClaim(p, &lothering, 1, 1000 + 899) == EXPEDITION_CLAIM_NOTHING_READY);
    ExpeditionReward early = ExpeditionClaim(p, &lothering, 1, 1000 + 899);
    assert(early.expeditions == 0 && p->expeditions.count == 1 && p->gold == gold);
    assert(ExpeditionsReadyCount(&p->expeditions, 1000 + 900) == 1);

    ExpeditionReward reward = ExpeditionClaim(p, &lothering, 1, 1000 + 900);
    assert(reward.expeditions == 1 && reward.great_successes == 1);
    assert(reward.gold >= 180 && reward.gold <= 270 && p->gold == gold + reward.gold);
    assert(reward.hero_xp == 60 && p->roster[bench.ferelden].xp == xp + 60);
    assert(reward.gear_count == 1 && reward.gear[0] >= 0 && GearDefAt(reward.gear[0])->rarity == 1);
    int gear_after = 0;
    for (int i = 0; i < GearCatalogCount(); i++) gear_after += p->gear.counts[i];
    assert(gear_after == gear + 1);
    assert(p->expeditions.count == 0 && !PlayerHeroAway(p, bench.ferelden));
    assert(ExpeditionClaim(p, &lothering, 1, 1000 + 900).expeditions == 0 && p->gold == gold + reward.gold);

    int storm = ExpeditionTestDef("storm_coast");
    int crew[] = {bench.dwarf, bench.red_mage};
    assert(ExpeditionStart(p, storm, crew, 2, 2000));
    assert(ExpeditionRecall(p, storm) && !ExpeditionRecall(p, storm));
    assert(p->expeditions.count == 0 && !PlayerHeroAway(p, bench.dwarf) && p->gold == gold + reward.gold);
}

static void TestExpeditionHeroDropsNeedRosterRoom(void) {
    ExpeditionBench bench = ExpeditionTestBench();
    Player *p = &bench.player;
    int hunt = ExpeditionTestDef("western_approach");
    int party[EXPEDITION_PARTY_CAPACITY];
    assert(ExpeditionAutoFill(p, hunt, party) == 4 && ExpeditionStart(p, hunt, party, 4, 0));
    int capacity = p->roster_capacity;
    p->roster_capacity = p->roster_count;
    assert(ExpeditionCheckClaim(p, &hunt, 1, 1 << 30) == EXPEDITION_CLAIM_ROSTER_FULL);
    assert(ExpeditionClaim(p, &hunt, 1, 1 << 30).expeditions == 0 && p->expeditions.count == 1);
    p->roster_capacity = capacity;
    int roster = p->roster_count;
    ExpeditionReward reward = ExpeditionClaim(p, &hunt, 1, 1 << 30);
    assert(reward.expeditions == 1 && reward.hero_count == 1 && p->roster_count == roster + 1);
    Rarity rarity = p->roster[reward.heroes[0]].rarity;
    assert(rarity == RARITY_RARE || rarity == RARITY_EPIC);
}

static void TestExpeditionsSurviveSaveAndLoad(void) {
    PlayerExpeditions saved = {
        .count = 2,
        .active =
            {
                {.def = ExpeditionTestDef("deep_roads"), .heroes = {11, 12, 13}, .hero_count = 3, .started_at = 50,
                 .ends_at = 50 + 240 * 60},
                {.def = ExpeditionTestDef("lothering"), .heroes = {14}, .hero_count = 1, .started_at = 70,
                 .ends_at = 970},
            },
    };
    cJSON *json = ExpeditionsSave(&saved);
    PlayerExpeditions loaded;
    ExpeditionsLoad(json, &loaded);
    assert(loaded.count == 2 && memcmp(&loaded.active, &saved.active, sizeof(saved.active)) == 0);
    assert(ExpeditionsHoldInstance(&loaded, 12) && !ExpeditionsHoldInstance(&loaded, 15));

    cJSON *active = cJSON_GetObjectItemCaseSensitive(json, "active");
    cJSON *copy = cJSON_Duplicate(cJSON_GetArrayItem(active, 1), true);
    cJSON_ReplaceItemInObject(copy, "id", cJSON_CreateString("storm_coast"));
    cJSON_AddItemToArray(active, copy);
    cJSON *unknown = cJSON_Duplicate(cJSON_GetArrayItem(active, 1), true);
    cJSON_ReplaceItemInObject(unknown, "id", cJSON_CreateString("no_such_place"));
    cJSON_AddItemToArray(active, unknown);
    ExpeditionsLoad(json, &loaded);
    assert(loaded.count == 2);
    cJSON_Delete(json);
}

static void TestExpeditionServerActionsNameTheHeroes(void) {
    ExpeditionBench bench = ExpeditionTestBench();
    Player *p = &bench.player;
    int deep_roads = ExpeditionTestDef("deep_roads");
    int party[] = {bench.dwarf, bench.apprentice, bench.giant};
    cJSON *start = ExpeditionStartAction(p, deep_roads, party, 3);
    assert(!strcmp(JsonStr(start, "action"), "expedition_start") && !strcmp(JsonStr(start, "expedition"), "deep_roads"));
    const cJSON *heroes = cJSON_GetObjectItemCaseSensitive(start, "heroes");
    assert(cJSON_GetArraySize(heroes) == 3);
    assert((uint64_t)cJSON_GetArrayItem(heroes, 2)->valuedouble == p->roster[bench.giant].instance_id);
    cJSON_Delete(start);

    int defs[] = {deep_roads, ExpeditionTestDef("lothering")};
    cJSON *claim = ExpeditionClaimAction(defs, 2);
    const cJSON *ids = cJSON_GetObjectItemCaseSensitive(claim, "expeditions");
    assert(!strcmp(JsonStr(claim, "action"), "expedition_claim") && cJSON_GetArraySize(ids) == 2);
    assert(!strcmp(cJSON_GetArrayItem(ids, 1)->valuestring, "lothering"));
    cJSON_Delete(claim);
    cJSON *recall = ExpeditionRecallAction(deep_roads);
    assert(!strcmp(JsonStr(recall, "action"), "expedition_recall") && !strcmp(JsonStr(recall, "expedition"), "deep_roads"));
    cJSON_Delete(recall);

    char reply_text[512];
    snprintf(
        reply_text,
        sizeof(reply_text),
        "{\"granted\":{\"gold\":900,\"gems\":3,\"xp\":200,\"granted\":{\"heroes\":[{\"id\":\"%s\",\"instance_id\":%llu}],"
        "\"gear\":[\"%s\"],\"runes\":[\"%s\"]}}}",
        p->roster[bench.rare].id,
        (unsigned long long)p->roster[bench.rare].instance_id,
        GearDefAt(0)->id,
        RuneDefAt(0)->id
    );
    cJSON *reply = cJSON_Parse(reply_text);
    ExpeditionReward reward = ExpeditionRewardFromReply(p, reply, 2, 1);
    assert(reward.expeditions == 2 && reward.great_successes == 1);
    assert(reward.gold == 900 && reward.gems == 3 && reward.hero_xp == 200);
    assert(reward.hero_count == 1 && reward.heroes[0] == bench.rare && reward.hero_new[0]);
    assert(reward.gear_count == 1 && reward.gear[0] == 0 && reward.rune_count == 1 && reward.runes[0] == 0);
    assert(reward.rune_new[0] && ExpeditionRewardHasItems(&reward));
    RuneGrant(&p->runes, 0, 0);
    RuneGrant(&p->runes, 0, 0);
    assert(!ExpeditionRewardFromReply(p, reply, 2, 1).rune_new[0]);
    ExpeditionReward coins = {.gold = 100};
    assert(!ExpeditionRewardHasItems(&coins));
    cJSON_Delete(reply);
}

static void TestStoreRevealsExpeditionRewardsHeroesThenGearThenRunes(void) {
    static Store store;
    memset(&store, 0, sizeof(store));
    int heroes[] = {4, 7};
    int gear[] = {0, 1};
    bool gear_new[] = {true, true};
    int runes[] = {RuneDefIndex("rune-power-1")};
    bool runes_new[] = {false};
    assert(StoreRevealRewards(&store, heroes, 2, gear, gear_new, 2, runes, runes_new, 1));
    assert(store.view == STORE_VIEW_OPENING && store.selected_pack < 0);
    assert(store.result_count == 2 && store.result_roster_indices[1] == 7);
    assert(StoreFinishRewardStage(&store) == STORE_ACTION_NONE);
    assert(store.view == STORE_VIEW_ITEM_REVEAL && store.item_kind == STORE_KIND_GEAR && store.item_result_count == 2);
    assert(StoreFinishRewardStage(&store) == STORE_ACTION_NONE);
    assert(store.item_kind == STORE_KIND_RUNE && store.item_result_count == 1 && !store.item_results[0].newly_owned);
    assert(StoreFinishRewardStage(&store) == STORE_ACTION_REWARDS_SHOWN && !store.reward_reveal.active);

    assert(StoreRevealRewards(&store, heroes, 0, gear, gear_new, 0, runes, runes_new, 1));
    assert(store.view == STORE_VIEW_ITEM_REVEAL && store.item_kind == STORE_KIND_RUNE);
    assert(StoreFinishRewardStage(&store) == STORE_ACTION_REWARDS_SHOWN);
    assert(!StoreRevealRewards(&store, heroes, 0, gear, gear_new, 0, runes, runes_new, 0) && !store.reward_reveal.active);
}

int main(void) {
    TestSystemLocalesPickTheirOriginalLocalization();
    TestFontCoversEveryGlyphOfTheLoadedLanguage();
    TestLocalizationLayersProjectStringsOverTheOriginalTable();
    TestBattleFeetSitOnPadsWhoseBottomsShareOnePlane();
    TestPvpBannerCurvePeaksAtTheLadderMidpoint();
    TestPvpEventResultsKeepNineteenUnclaimedFirst();
    TestPvpEventMatchmakingFollowsBanners();
    TestPvpEventRotatesThroughTheCatalog();
    TestOnslaughtReplacesEveryFewPvpEvents();
    TestDuelOpponentFightsTheMembersSquad();
    TestPvpEventBannersMilestonesAndClaim();
    TestPvpEventPrizesAreOwnableAndHaveArt();
    TestPvpEventMilestonesPayWispGearAndTome();
    TestPvpEventClaimPaysWholeBundleOnce();
    TestPvpEventClaimPaysPackVouchers();
    TestPvpEventRankRisesWithBannersAndCoversEveryone();
    TestPvpEventSquadBuffCountsOncePerEvent();
    TestPvpResolveBattlePaysAndMarksRevenge();
    TestPvpSpendsStaminaNotEnergy();
    TestPvpLeaderboardRanksThePlayer();
    TestGemPurchaseCreditsGemsAndRefreshesStamina();
    TestPvpTrophiesHeldNeedsAWeekAboveTheLine();
    TestPvpAchievementsFollowTheLadder();
    TestPvpOpponentsAreDeterministic();
    TestPvpEventLeaderboardMatchesRank();
    TestPvpOpponentsScaleWithLevel();
    TestPvpMatchingStaysInTheTrophyWindow();
    TestPvpTrophyBounds();
    TestPvpNextsResetDaily();
    TestPvpMatchAsksTheServerForTheOpponent();
    TestPvpMatchWaitsForTheServersReply();
    TestPvpOpponentFromServerMatchRebuildsTheBot();
    TestPvpDefenseLogWaitsForFirstBattleAndCapsAttacks();
    TestPvpRevengeRebuildsTheSameOpponent();
    TestPvpEveryPooledSquadFights();
    TestPvpOpponentRunesApplyToTheEnemySide();
    TestPvpSaveRoundTrip();
    TestBattleStartSendsTheBattleTheClientPlays();
    TestFortifyTablesCarryThe544Rules();
    TestFortifyBonusesFollowThe544Calculator();
    TestFortifyMaximaMatchThe5xCatalog();
    TestFortifyMergesTiersLikeThe544Slot();
    TestFortifySlotsUnlockWithConsumeLevel();
    TestPlayerFortifyConsumesTheChildren();
    TestExpeditionCatalogFollowsTheTable();
    TestExpeditionTraitsMatchRealHeroes();
    TestExpeditionPartiesNeedBenchHeroesAndTheirRequirements();
    TestExpeditionClaimPaysOnceWhenThePartyIsBack();
    TestExpeditionHeroDropsNeedRosterRoom();
    TestExpeditionsSurviveSaveAndLoad();
    TestExpeditionServerActionsNameTheHeroes();
    TestTimedQuestCatalogFollowsTheTable();
    TestTimedQuestDaysTurnAtUtcMidnightAndWeeksOnMonday();
    TestTimedQuestDailyPicksMatchTheServer();
    TestTimedQuestCountsResetDailyAndWeekly();
    TestTimedQuestClaimsPayOnceAndFillTheChestTrack();
    TestTimedQuestSwapsOneUnfinishedQuestADay();
    TestTimedQuestHeroChestsNeedRosterRoom();
    TestTimedQuestsSurviveSaveAndLoad();
    TestStoreRevealsExpeditionRewardsHeroesThenGearThenRunes();
    TestPlayerRemoveFortifyReturnsHeroesForGems();
    TestFortifySlotsTravelThroughSavesAndBattles();
    TestTutorialScriptCoversBothFunnels();
    TestTutorialScriptsTheTwoLostBattles();
    TestTutorialResumesAtTheSavedRestorePoint();
    TestTutorialPackRewardsResolve();
    TestPlayerNameFollowsTheOriginalRule();
    TestProgressionKeepsNameAndTutorialState();
    TestServerPurchaseProfileReplacesThePlayer();
    TestNexusGameChangerBoostsMatchingUnitsOnBothSides();
    TestNexusGameChangerNarrowsByFactionAndUnit();
    TestNexusGameChangerTableAffectsRealRosters();
    TestGearCatalogIsComplete();
    TestGearSlotsFollowRarity();
    TestGearEquipRules();
    TestGearCategoriesUseOrAndAllSemantics();
    TestGearSellValueAndPrestigeUnlock();
    TestGearPacksRollInsideTheirRarityRange();
    TestGearStatAurasAddThenMultiply();
    TestGearAuraRespectsGroupAndWearerFilters();
    TestGearDamageMitigationAndVersus();
    TestGearResistsScaleAilments();
    TestGearOnHitReactionsAndStealth();
    TestGearReactionsRespectCurses();
    TestGearDeathTriggersFireForDeadWearer();
    TestGearReactionDamageCanEndAChain();
    TestGearDefenderAbsorbsAoeShare();
    TestGearTargetPreferences();
    TestGearlessFightsAreUntouched();
    TestEveryCatalogItemFightsCleanly();
    TestGearSaveRoundTrip();
    TestConsumeWarnsAboutEquippedGear();
    TestGearTextFitsItsBuffersInEveryLanguage();
    TestRichTextReadsHexColors();
    TestGearViewEquipFlow();
    TestBoardIgnoresClicksBehindGearAndDetails();
    TestAscensionRequiresFilledSlotsAndTome();
    TestAscensionPromotesHeroAndMovesPrestigeGear();
    TestAscensionSurvivesSave();
    TestGearPackBundlesATome();
    TestSeasonalPackFollowsTheCalendar();
    TestFlashPacksRotateOnePerPeriod();
    TestVoucherOfferWinsOnlyWhenOwned();
    TestStoreDialogTextFitsRussianPackNames();
    TestAscendedHeroLoadsAboveTheOldLevelCap();
    TestHeroLevelTableCoversAscendedCap();
    TestMissionSeriesPicksTheClaimableTier();
    TestMissionProgressNeverDrops();
    TestMissionClaimPaysOnce();
    TestMissionGoldTrackerIgnoresSpendingAndClaims();
    TestMissionBattleRecording();
    TestMissionSaveRoundTrip();
    TestHudCurrencyTallyHoldsThenTicksUp();
    TestHudCurrencyTallyReleaseWithoutGainSnaps();
    SetTraceLogLevel(LOG_WARNING);
    TestGalleryRecordsTrackDiscoveryAndBadge();
    TestGalleryCatalogMatchesReferenceRules();
    TestGalleryRecordsBattleCasualties();
    TestGalleryPersistence();
    TestRuneCatalogCraftChains();
    TestRuneSlotRulesAndReplacement();
    TestRuneExpiry();
    TestRuneCrafting();
    TestServerActionsStayLocalWithoutABackend();
    TestNexusAwardReadsTheServerReply();
    TestRuneBonusStacksAndScalesRewards();
    TestRuneMitigationAndRetaliation();
    TestRuneResistsBlockAilments();
    TestRuneStatsAndTargeting();
    TestRuneSaveRoundTrip();
    TestPrestigeSlotCostsGems();
    TestRuneActivationFx();
    TestOriginalResourcesDecodeLikeUnityPy();
    TestEveryReferencedResourceResolves();
    TestSquadSlotReferenceLayout();
    TestBattleRowBonuses();
    TestHeroRowBonusReferencePresentation();
    TestFormationBonusMetadata();
    TestFormationBonusRequiresSharedFaction();
    TestIntroQueueShowsOnlyPlayerFactionBonus();
    TestFactionIntroAssetsAndText();
    TestPowerDrain();
    TestAllyPowerGain();
    TestAllySlow();
    TestSlowCarriesIntoNextRound();
    TestAllyCritGain();
    TestTwoRoundCurse();
    TestBackRowCurse();
    TestFallbackSelfCosts();
    TestCombinedAilments();
    TestMarkLandsOnStruckTargets();
    TestSplitAilmentSwing();
    TestCurseLastsThroughTurn();
    TestNecromancerDorianCurse();
    TestCurseFx();
    TestSlowFx();
    TestStatusPrefabCabParameters();
    TestCriticalFxAndReward();
    TestBattleMessageDictionary();
    TestBattleRewardMessageTiming();
    TestBattleCameraTransitions();
    TestStatusDeltaFxRules();
    TestStatusAuraSlots();
    TestStatusOffsetsAndShake();
    TestInitiativeAndStatusCleanup();
    TestInitiativeBannerFollowsModifier();
    TestFollowupTarget();
    TestRedJennySeraFinalHit();
    TestColeProfanePerHitGrowth();
    TestVivienneIncreasingFollowups();
    TestRiverDanePersistentAllyDamage();
    TestBonusDamageRecipes();
    TestMechanicsFallback();
    TestNoAttackFallback();
    TestNoAttackCompoundTargets();
    TestStatusOnlyTargetPacket();
    TestRandomAllyAttack();
    TestGeneratedCorypheusRecipe();
    TestGeneratedLoghainRecipes();
    TestGeneratedChanceGroup();
    TestTimelineCapacity();
    TestLegendaryStars();
    TestCatalogStatGrowth();
    TestStatBudgetScalesEveryHeroToOneCostAndKeepsItsShape();
    TestStatBudgetOnTheEnemyRosterLevelsBothSides();
    TestStatBudgetTableLoads();
    TestCustomHeroesInheritTheirBaseHeroData();
    TestBattlesGrantTheirXpToEverySquadHero();
    TestFxPresetLiveEmitterSizes();
    TestGeneratedFxCurvesAndDragonJet();
    TestPortraitCoverage();
    TestRichTextParserFallback();
    TestCharacterTextCoverage();
    TestCharacterTextResolvesInEveryLanguage();
    TestAbilityMechanicParsing();
    TestExactRecipesMatchDescriptions();
    TestGeneratedRecipeCoverage();
    TestRecipeCoverage();
    TestAttackPresentationBindings();
    TestConeDelayDefersImpact();
    TestBattlePreviewUsesFinalSetupStats();
    TestIntrinsicPassiveManifest();
    TestEnvyBattleStartAuras();
    TestAvvarWarNugCompositionHealth();
    TestPowerVsTargetDamage();
    TestStackingAuraMatchMode();
    TestPassivePredicatesAndSignedEnemyAura();
    TestAurasAddAcrossSources();
    TestPassivePrecedesFormationRounding();
    TestEnvyTargetWeights();
    TestEnvyDamageAndEnemyDeathHeal();
    TestBattleClipAliases();
    TestTierlessWispModelAlias();
    TestExactTierModelResolution();
    TestDisplayScaleManifestIsFullyLoaded();
    TestDamageShakeFallback();
    TestAttackClipReturnsToIdle();
    TestCampaignOpenerCanBeWon();
    TestZathriansCurseBossFitsAPostTutorialSquad();
    TestCriticalGoldCommitOnce();
    TestCampaignNodeCoordinates();
    TestCampaignMapRewardSchema();
    TestCampaignAndNexusTextResolves();
    TestCampaignProgressionRewards();
    TestProgressionPersistence();
    TestRosterInstancesAndPersistence();
    TestConsumeTablesAndCurves();
    TestConsumeSeedWorthAndBonuses();
    TestConsumeCommitCompactsRoster();
    TestConsumeRespectsCaps();
    TestWispWorthFollowsReferenceMultipliers();
    TestSimUsesConsumedCritChance();
    TestAllCampaignMapsAdvance();
    TestEnergyAndStaminaPools();
    TestUnitPoolReusesReleasedUnits();
    TestWispIdleDriftsLikeTheOriginalFx();
    TestFxKeyoutPreservesSourceTransparency();
    TestCreatureClipFramesStayOnIdleGround();
    puts("battle_sim_test: ok");
    return 0;
}
