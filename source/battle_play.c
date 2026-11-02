
#define BP_STAGE_X (-0.526f)
#define BP_STAGE_Z 15.886f
#define BP_DELAY_BETWEEN_ACTIONS 0.25f
#define BP_MESSAGE_TIME 1.06f
#define BP_INTRO_DELAY 1.0f
#define BP_BANNER_TIME 1.0f
#define BP_RUNE_AURA_FX "fx_eff_initiative_charge_pre"
#define BP_RUNE_AURA_COLOR ((Color){0, 255, 0, 255})
#define BP_CRIT_TIMESCALE 0.65f
#define BP_CRIT_GOLD_REWARD 10
#define BP_REWARD_COIN_COUNT 5
#define BP_REWARD_COIN_STAGGER 0.1f
#define BP_REWARD_COIN_MOVE_TIME 0.5f
#define BP_REWARD_COIN_FADE_IN 0.15f
#define BP_REWARD_COIN_FADE_OUT 0.05f
#define BP_REWARD_ANIM_TIME (BP_REWARD_COIN_MOVE_TIME + (BP_REWARD_COIN_COUNT - 1) * BP_REWARD_COIN_STAGGER)
#define BP_DISSOLVE_TIME 1.5f
#define BP_HIT_TIME 0.45f
#define BP_DAMAGE_SHAKE_TIME 0.25f
#define BP_RING_TWEEN 0.5f
#define BP_CLIP_FPS (1000.0f / 17.0f)

typedef enum { BPU_IDLE, BPU_LEAP_OUT, BPU_WINDUP, BPU_RETURN } BpUnitPhase;

typedef struct {
    BpUnitPhase phase;
    Vector3 pos;
    Vector3 home;
    float hop_total;
    const ModelAnimation *clip;
    const ModelAnimation *shown_clip;
    float clip_time;
    float clip_speed;
    bool clip_hold;
    bool fired;
    float hit_timer;
    float damage_shake_left;
    bool dying, dead;
    float dissolve;
    float ring_pct, ring_from, ring_to, ring_t;
    int health, max_health;
    int power, start_power;
    int current_initiative;
} BpUnit;

typedef enum {
    BP_STAT_FX_NONE,
    BP_STAT_FX_POWER_UP,
    BP_STAT_FX_POWER_DOWN,
    BP_STAT_FX_INIT_UP,
    BP_STAT_FX_INIT_DOWN,
} BpStatFx;

typedef struct {
    bool active;
    Vector3 pos, target;
    float speed, age;
    int look;
    char trail[52], impact[52], sustained[52];
    float accum[FX_MAX_EMITTERS];
    bool trail_burst_done[FX_MAX_EMITTERS];
    unsigned trail_serial;
} BpProjectile;

typedef struct {
    int order_key;
    int slot;
    int icon;
    char text[48];
    char bonus[16];
    Color color;
    Color bonus_color;
    float big;
    int gold_reward;
    bool reward_applied;
} BpMessage;

enum {
    BP_MSG_FACTION_REWARD = 2,
    BP_MSG_DAMAGE = 3,
    BP_MSG_CRIT_REWARD = 4,
    BP_MSG_INIT_DOWN = 5,
    BP_MSG_POWER_DOWN = 6,
    BP_MSG_STUN = 7,
    BP_MSG_HEAL = 8,
    BP_MSG_INIT_UP = 9,
    BP_MSG_POWER_UP = 10,
    BP_MSG_CURSE_POWER = 11,
    BP_MSG_CURSE_HEALTH = 12,
};

#define BP_MAX_MESSAGES 24
#define BP_MAX_PROJECTILES 8
#define BP_MAX_INTRO_EVENTS (RUNE_SLOT_COUNT + 2)

typedef struct {
    float value;
    unsigned affected_slots;
    unsigned faction;
    int rune_def;
    bool event_bonus;
    bool no_event_bonus;
} BpIntroEvent;

typedef enum {
    BP_OFF,
    BP_INTRO,
    BP_INTRO_BANNER,
    BP_ACT_START,
    BP_ACT_LEAP,
    BP_ACT_WINDUP,
    BP_ACT_CONE_DELAY,
    BP_ACT_FLIGHT,
    BP_ACT_MESSAGES,
    BP_ACT_RETURN,
    BP_ACT_DELAY,
    BP_END_SPLASH,
    BP_SUMMARY
} BpState;

typedef struct {
    BattleSim sim;
    BpState state;
    float t;
    int action;
    int cur_round;
    int actor_init_class;
    int returning_actor;
    BpUnit units[2][SQUAD_SLOT_COUNT];
    BpProjectile projectiles[BP_MAX_PROJECTILES];
    int breath_fx;
    unsigned breath_serial;
    int breath_actor;
    BpMessage messages[BP_MAX_MESSAGES];
    int message_count;
    float message_t;
    BpIntroEvent intro_events[BP_MAX_INTRO_EVENTS];
    int intro_event_count, intro_event;
    float shake_t, shake_dur, shake_amp;
    float cone_impact_at;
    float slowmo;
    float speed_mult;
    int total_bonus_gold;
    float gold_counter_alpha;
    float reward_anim_t;
    bool reward_anim_active;
    bool won, over;
} BattlePlay;

static BpUnit *BpUnitAt(BattlePlay *bp, int idx) {
    return &bp->units[idx / SQUAD_SLOT_COUNT][idx % SQUAD_SLOT_COUNT];
}

static BpStatFx BpApplyPowerDelta(BpUnit *u, int delta) {
    u->power += delta;
    if (delta > 0 && u->power > u->start_power) return BP_STAT_FX_POWER_UP;
    if (delta < 0 && u->health > 0 && u->power < u->start_power) return BP_STAT_FX_POWER_DOWN;
    return BP_STAT_FX_NONE;
}

static BpStatFx BpApplyInitiativeDelta(BpUnit *u, int delta) {
    u->current_initiative += delta;
    if (delta > 0 && u->current_initiative > 0) return BP_STAT_FX_INIT_UP;
    if (delta < 0 && u->health > 0 && u->current_initiative < 0) return BP_STAT_FX_INIT_DOWN;
    return BP_STAT_FX_NONE;
}

static int BpInitiativeClass(const BattlePlay *bp, int unit) {
    int modifier = bp->units[unit / SQUAD_SLOT_COUNT][unit % SQUAD_SLOT_COUNT].current_initiative;
    int roll_high = (bp->sim.units[unit].init_class + 1) * SIM_INIT_CLASS_SPAN - 1 + modifier;
    if (roll_high < SIM_INIT_CLASS_SPAN) return SIM_INIT_CLASS_SLOW;
    if (roll_high < 2 * SIM_INIT_CLASS_SPAN) return SIM_INIT_CLASS_MEDIUM;
    return SIM_INIT_CLASS_FAST;
}

static const BattleSpot *BpSpot(int side, int slot) {
    return side == 0 ? &BATTLE_PLAYER_SPOTS[slot] : &BATTLE_ENEMY_SPOTS[slot];
}

static float BpLaunchPct(const Character *ch) {
    const char *p = ch->model_prefab[ch->tier];
    if (strstr(p, "_dw_")) return 0.20f;
    if (strstr(p, "crossbow") || strstr(p, "bolter")) return 0.65f;
    if (strstr(p, "_sw_") || strstr(p, "_tw_")) return 0.35f;
    return 0.40f;
}

static bool BpIsProjectile(const Character *ch, float *speed) {
    const char *p = ch->model_prefab[ch->tier];
    *speed = 8.0f;
    if (strstr(p, "bolter") || strstr(p, "crossbow")) {
        *speed = 20.0f;
        return true;
    }
    if (strstr(p, "ladyoftheforest")) {
        *speed = 16.0f;
        return true;
    }
    if (strstr(p, "emissary")) return true;
    return false;
}

static int BpProjLook(const Battle *b, const char *ability) {
    if (!ability) return PROJ_BALL;
    if (strstr(ability, "bolter") || strstr(ability, "arrow") || strstr(ability, "pinningshot")) return PROJ_ARROW;
    if (strstr(ability, "spiritball") || strstr(ability, "spirit")) return PROJ_SPIRIT;
    if (strstr(ability, "desiccate")) return PROJ_DESICCATE;
    return BattleFxFindPreset(b, ability) ? PROJ_NONE : PROJ_BALL;
}

static Vector3 BpChest(const BattlePlay *bp, const BpUnit *u, int idx) {
    const SimUnit *su = &bp->sim.units[idx];
    float h = su->present && su->ch->giant ? 1.8f : 0.9f;
    return (Vector3){u->pos.x, u->pos.y + h, u->pos.z};
}

static float BpMoverBucket(float d) {
    if (d > 3.0f) return 0.05f;
    if (d > 2.0f) return 0.1f;
    if (d > 0.15f) return 0.25f;
    return 0.5f;
}

static float BpHopHeight(float frac) {
    if (frac <= 0.0f || frac >= 1.0f) return 0.0f;
    if (frac < 0.5f) {
        float p = frac * 2.0f;
        return p < 0.5f ? 8.0f * p * p * p * p : 1.0f - 8.0f * (p - 1) * (p - 1) * (p - 1) * (p - 1);
    }
    return sinf(frac * PI);
}

static bool BpMoveToward(BpUnit *u, Vector3 target, float dt, float arrive) {
    float dx = target.x - u->pos.x, dz = target.z - u->pos.z;
    float d = sqrtf(dx * dx + dz * dz);
    if (d < 0.001f) {
        u->pos.x = target.x;
        u->pos.z = target.z;
    } else {
        float f = (0.07f + BpMoverBucket(d)) * 75.0f * dt;
        if (f > 1.0f) f = 1.0f;
        u->pos.x += dx * f;
        u->pos.z += dz * f;
    }
    float hx = u->pos.x - u->home.x, hz = u->pos.z - u->home.z;
    float traveled = sqrtf(hx * hx + hz * hz);
    float frac = u->hop_total > 0.001f ? traveled / u->hop_total : 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    u->pos.y = u->home.y + BpHopHeight(frac);
    return d < arrive;
}

#define BP_ARRIVE_STAGE 3.0f
#define BP_ARRIVE_PAD 0.02f

static BpMessage *BpQueueMessage(
    BattlePlay *bp, int order_key, int slot, int icon, const char *text, const char *bonus, Color col, float big
) {
    int at = 0;
    while (at < bp->message_count && bp->messages[at].order_key < order_key) at++;
    if (at >= bp->message_count || bp->messages[at].order_key != order_key) {
        if (bp->message_count >= BP_MAX_MESSAGES) return NULL;
        memmove(&bp->messages[at + 1], &bp->messages[at], sizeof(bp->messages[0]) * (bp->message_count - at));
        bp->message_count++;
    }
    BpMessage *m = &bp->messages[at];
    *m = (BpMessage){
        .order_key = order_key,
        .slot = slot,
        .icon = icon,
        .color = col,
        .bonus_color = {232, 43, 43, 255},
        .big = big,
    };
    StrCopy(m->text, sizeof(m->text), text);
    StrCopy(m->bonus, sizeof(m->bonus), bonus ? bonus : "");
    return m;
}

static void BpRingSet(BpUnit *u, float pct) {
    u->ring_from = u->ring_pct;
    u->ring_to = pct;
    u->ring_t = 0.0f;
}

static void BpStartClip(BpUnit *u, const ModelAnimation *clip, float speed, bool hold) {
    if (!clip) return;
    u->clip = clip;
    u->shown_clip = NULL;
    u->clip_time = 0.0f;
    u->clip_speed = speed;
    u->clip_hold = hold;
}

static void BpAdvanceClip(BpUnit *u, float dt) {
    if (!u->clip) return;
    u->clip_time += dt * BP_CLIP_FPS * u->clip_speed;
    float last_frame = (float)u->clip->frameCount - 1.0f;
    if (u->clip_time < last_frame) return;
    if (u->clip_hold || u->hit_timer > 0.0f)
        u->clip_time = last_frame;
    else
        u->clip = NULL;
}

static void BpSetAnimationOverride(HomeSquadUnit *unit, bool enabled) {
    if (unit->anim_override && !enabled) unit->last_frame = -1;
    unit->anim_override = enabled;
}

static Vector3 BpDamageShakeOffset(const BpUnit *u, int unit) {
    if (u->damage_shake_left <= 0.0f) return (Vector3){0};
    float age = BP_DAMAGE_SHAKE_TIME - u->damage_shake_left;
    float fade = u->damage_shake_left / BP_DAMAGE_SHAKE_TIME;
    float seed = (float)(unit + 1) * 1.37f;
    return (Vector3){
        0.25f * fade * sinf(age * 83.0f + seed),
        0.0f,
        1.0f * fade * sinf(age * 101.0f + seed * 1.9f),
    };
}

static void BpTelegraphCrits(BattlePlay *bp, Battle *b) {
    bool any = false;
    bool done[SIM_UNIT_COUNT] = {false};
    for (int i = bp->action; i < bp->sim.action_count; i++) {
        const BattleSimAction *a = &bp->sim.actions[i];
        if (a->newround) break;
        if (!a->crit || !a->replay_attack || a->actor < 0 || done[a->actor]) continue;
        done[a->actor] = true;
        BpUnit *u = BpUnitAt(bp, a->actor);
        if (u->dead || u->dying) continue;
        BattleFxBurst(b, "vfx_crit_charge_pre", u->pos);
        any = true;
    }
    if (any) BattleAudioPlayGlobal(b, "gui_crit_charge");
}

static bool BpPassiveHealEntry(const BattleSimTarget *t) {
    return t->health_delta > 0 && t->health_faction_delta == 0 && t->power_delta == 0 && t->initiative_delta == 0 &&
           t->crit_delta == 0.0f && t->damage_bonus_delta == 0.0f;
}

static bool BpSwingChainContinues(const BattleSim *sim, int index, int actor) {
    if (index < 0 || index >= sim->action_count) return false;
    const BattleSimAction *a = &sim->actions[index];
    return a->same_swing && !a->newround && a->actor == actor;
}

static int BpCollectSwingTargets(const BattleSim *sim, int action, int out[SIM_MAX_TARGETS]) {
    if (action < 0 || action >= sim->action_count) return 0;
    const BattleSimAction *first = &sim->actions[action];
    if (first->newround || first->actor < 0) return 0;

    int count = 0;
    for (int pass = 0; pass < 2 && count == 0; pass++) {
        bool seen[SIM_UNIT_COUNT] = {false};
        for (int i = action; i < sim->action_count; i++) {
            const BattleSimAction *a = &sim->actions[i];
            if (i > action && !BpSwingChainContinues(sim, i, first->actor)) break;
            for (int t = 0; t < a->target_count; t++) {
                int unit = a->targets[t].unit;
                if (unit < 0 || unit >= SIM_UNIT_COUNT || seen[unit]) continue;
                if (pass == 0 && BpPassiveHealEntry(&a->targets[t])) continue;
                seen[unit] = true;
                out[count++] = unit;
            }
        }
    }
    return count;
}

static void BpStageAction(BattlePlay *bp, Battle *b, int action) {
    const BattleSimAction *a = &bp->sim.actions[action];
    int targets[SIM_MAX_TARGETS];
    int target_count = BpCollectSwingTargets(&bp->sim, action, targets);
    memset(b->punch, 0, sizeof(b->punch));
    b->filter_on = a->crit;
    if (a->crit && a->actor >= 0) b->punch[a->actor / SQUAD_SLOT_COUNT][a->actor % SQUAD_SLOT_COUNT] = true;
    for (int i = 0; i < target_count; i++) {
        int side = targets[i] / SQUAD_SLOT_COUNT, slot = targets[i] % SQUAD_SLOT_COUNT;
        if (a->crit) b->punch[side][slot] = true;
        if (targets[i] != a->actor)
            BattleFxStatusSet(b, FX_AURA_TARGET, "fx_eff_target_marker_pre", side, slot, (Vector3){0, 0.03f, 0});
    }
}

static void BpUnstageAction(Battle *b) {
    b->filter_on = false;
    memset(b->punch, 0, sizeof(b->punch));
    for (int side = 0; side < 2; side++)
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) BattleFxStatusClear(b, FX_AURA_TARGET, side, slot);
}

static void BpClearUnitStatusFx(Battle *b, int side, int slot) {
    static const FxAuraSlot slots[] = {
        FX_AURA_POWER,
        FX_AURA_INITIATIVE,
        FX_AURA_STUN,
        FX_AURA_CURSE_POWER,
        FX_AURA_CURSE_HEALTH,
    };
    for (int i = 0; i < (int)(sizeof(slots) / sizeof(slots[0])); i++) BattleFxStatusClear(b, slots[i], side, slot);
}

static bool BpSwingAwardsCritGold(const BattlePlay *bp, const BattleSimAction *first) {
    int aside = first->actor / SQUAD_SLOT_COUNT;
    bool crit = false, hit_enemy = false;
    uintptr_t p = (uintptr_t)first, base = (uintptr_t)bp->sim.actions;
    uintptr_t end = (uintptr_t)(bp->sim.actions + bp->sim.action_count);
    bool in_replay = p >= base && p < end;
    int i = in_replay ? (int)(first - bp->sim.actions) : 0;
    const BattleSimAction *part = first;
    for (;;) {
        crit |= part->crit;
        for (int t = 0; t < part->target_count; t++) {
            const BattleSimTarget *bt = &part->targets[t];
            if (bt->unit / SQUAD_SLOT_COUNT != aside && bt->health_delta + bt->health_faction_delta < 0)
                hit_enemy = true;
        }
        if (!in_replay || !BpSwingChainContinues(&bp->sim, ++i, first->actor)) break;
        part = &bp->sim.actions[i];
    }
    return crit && hit_enemy;
}

static void BpApplyAction(BattlePlay *bp, Battle *b, const BattleSimAction *a) {
    const Color red = {232, 43, 43, 255};
    const Color white = {245, 244, 238, 255};
    const char *cursed = LocGet("HEALTHPOWER_CURSED_DEBUFF");
    if (!cursed[0]) cursed = "Проклятие!";

    for (int i = 0; i < a->target_count; i++) {
        const BattleSimTarget *t = &a->targets[i];
        int side = t->unit / SQUAD_SLOT_COUNT, slot = t->unit % SQUAD_SLOT_COUNT;
        BpUnit *u = &bp->units[side][slot];
        int msg_slot = t->unit == a->actor ? 0 : (side == 0 ? 1 : 2);
        bool beneficiary = BpPassiveHealEntry(t);
        const SimUnit *su = &bp->sim.units[t->unit];
        bool giant = su->present && su->ch->giant;
        Vector3 aura_lift = {0, giant ? 0.9f : 0.0f, 0};

        int hp = t->health_delta + t->health_faction_delta;
        u->health += hp;
        if (u->health < 0) u->health = 0;
        if (u->health > u->max_health) u->health = u->max_health;
        BpRingSet(u, u->max_health > 0 ? (float)u->health / (float)u->max_health : 0.0f);
        if (hp < 0) u->damage_shake_left = BP_DAMAGE_SHAKE_TIME;

        if (hp < 0) {
            char text[32], bonus[16] = "";
            snprintf(text, sizeof(text), "%d", -t->health_delta);
            if (t->health_faction_delta < 0) snprintf(bonus, sizeof(bonus), "+%d", -t->health_faction_delta);
            BpQueueMessage(
                bp,
                BP_MSG_DAMAGE,
                msg_slot,
                a->crit ? GUI_MSG_CRIT : GUI_MSG_DAMAGE,
                text,
                bonus,
                red,
                a->crit ? 1.7f : 1.0f
            );
            if (a->crit) {
                BattleFxBurst(b, "fx_crit_pre", u->pos);
            }
        } else if (hp > 0) {
            char text[32];
            snprintf(text, sizeof(text), "+%d", hp);
            BpQueueMessage(bp, BP_MSG_HEAL, msg_slot, GUI_MSG_HEAL, text, NULL, white, 1.0f);
            BattleFxBurst(b, "fx_heal_pre", u->pos);
            BattleAudioPlayGlobal(b, "gui_fx_heal");
        }
        if (t->power_delta != 0) {
            BpStatFx power_fx = BpApplyPowerDelta(u, t->power_delta);
            if (t->power_delta > 0 || u->health > 0)
                BpQueueMessage(
                    bp,
                    t->power_delta > 0 ? BP_MSG_POWER_UP : BP_MSG_POWER_DOWN,
                    msg_slot,
                    t->power_delta > 0 ? GUI_MSG_PWR_UP : GUI_MSG_PWR_DOWN,
                    LocGet(t->power_delta > 0 ? "POWER_BUFF" : "POWER_DEBUFF"),
                    NULL,
                    white,
                    1.0f
                );
            if (power_fx == BP_STAT_FX_POWER_UP) {
                BattleFxStatusSet(b, FX_AURA_POWER, "vfx_powerup_pre", side, slot, aura_lift);
                BattleAudioPlayGlobal(b, "gui_fx_power_up");
            } else if (power_fx == BP_STAT_FX_POWER_DOWN) {
                BattleFxStatusSet(b, FX_AURA_POWER, "vfx_powerdown_pre", side, slot, aura_lift);
                BattleAudioPlayGlobal(b, "gui_fx_power_down");
                u->damage_shake_left = BP_DAMAGE_SHAKE_TIME;
            }
        }
        if (t->initiative_delta != 0) {
            BpStatFx init_fx = BpApplyInitiativeDelta(u, t->initiative_delta);
            if (t->initiative_delta > 0 || u->health > 0)
                BpQueueMessage(
                    bp,
                    t->initiative_delta > 0 ? BP_MSG_INIT_UP : BP_MSG_INIT_DOWN,
                    msg_slot,
                    t->initiative_delta > 0 ? GUI_MSG_INIT_UP : GUI_MSG_INIT_DOWN,
                    LocGet(t->initiative_delta > 0 ? "INITIATIVE_BUFF" : "INITIATIVE_DEBUFF"),
                    NULL,
                    white,
                    1.0f
                );
            if (init_fx == BP_STAT_FX_INIT_UP) {
                BattleFxStatusSet(b, FX_AURA_INITIATIVE, "vfx_powerup_pre", side, slot, aura_lift);
            } else if (init_fx == BP_STAT_FX_INIT_DOWN) {
                BattleFxStatusSet(b, FX_AURA_INITIATIVE, "eff_initiativedown_pre", side, slot, aura_lift);
                BattleAudioPlayGlobal(b, "gui_fx_init_down");
                u->damage_shake_left = BP_DAMAGE_SHAKE_TIME;
            }
        }
        if (!beneficiary && u->health > 0 && (a->ailments & AIL_STUNNED)) {
            BpQueueMessage(bp, BP_MSG_STUN, msg_slot, GUI_MSG_STUN, LocGet("STUNNED_DEBUFF"), NULL, white, 1.0f);
            BattleFxStatusSet(b, FX_AURA_STUN, "vfx_stunned_pre", side, slot, (Vector3){0, giant ? 1.0f : 0.0f, 0});
            BattleAudioPlayGlobal(b, "gui_fx_stun");
            u->damage_shake_left = BP_DAMAGE_SHAKE_TIME;
        }
        if (!beneficiary && u->health > 0 && (a->ailments & AIL_CURSE_POWER)) {
            BpQueueMessage(bp, BP_MSG_CURSE_POWER, msg_slot, GUI_MSG_CURSE_PWR, cursed, NULL, white, 1.0f);
            BattleFxStatusSet(b, FX_AURA_CURSE_POWER, "fx_status_curse_power_pre", side, slot, (Vector3){0});
            u->damage_shake_left = BP_DAMAGE_SHAKE_TIME;
        }
        if (!beneficiary && u->health > 0 && (a->ailments & AIL_CURSE_HEALTH)) {
            BpQueueMessage(bp, BP_MSG_CURSE_HEALTH, msg_slot, GUI_MSG_CURSE_HP, cursed, NULL, white, 1.0f);
            BattleFxStatusSet(b, FX_AURA_CURSE_HEALTH, "fx_status_curse_health_pre", side, slot, (Vector3){0});
            u->damage_shake_left = BP_DAMAGE_SHAKE_TIME;
        }

        if (u->dead || u->dying) continue;
        if (u->health <= 0) {
            u->dying = true;
            u->dissolve = 1.0f;
            BpClearUnitStatusFx(b, side, slot);
            BattleFxStatusClear(b, FX_AURA_TARGET, side, slot);
            const ModelAnimation *death = BattleSquadClip(&b->squads, side, slot, "death");
            BpStartClip(u, death, 0.75f, true);
            BattleAudioPlayUnit(b, side, slot, UNIT_SFX_DEATH);
        } else if (hp < 0) {
            u->hit_timer = BP_HIT_TIME;
            const ModelAnimation *hit = BattleSquadClip(&b->squads, side, slot, "hit");
            if (u->phase == BPU_IDLE && !u->clip_hold) BpStartClip(u, hit, 1.1f, false);
        }
    }

    if (a->crit && a->replay_attack && BpSwingAwardsCritGold(bp, a)) {
        BattleAudioPlayGlobal(b, "vfx_critical_hit");
        bp->shake_t = 0.0f;
        bp->shake_dur = 0.75f;
        bp->shake_amp = 0.5f;
    }

    if (!a->newround && !a->same_swing && a->actor >= 0 && a->actor / SQUAD_SLOT_COUNT == 0 &&
        BpSwingAwardsCritGold(bp, a)) {
        char amount[16];
        snprintf(amount, sizeof(amount), "+ %d ", BP_CRIT_GOLD_REWARD);
        BpMessage *reward = BpQueueMessage(
            bp, BP_MSG_CRIT_REWARD, 0, GUI_MSG_CRIT_REWARD, LocGet("BATTLE_CRITICAL_AWARD_MESSAGE"), amount, white, 1.0f
        );
        if (reward) {
            reward->bonus_color = (Color){34, 204, 43, 255};
            reward->gold_reward = BP_CRIT_GOLD_REWARD;
        }
    }
}

static void BpQueueIntroEvent(BattlePlay *bp, float value, unsigned affected_slots, unsigned faction) {
    if (bp->intro_event_count >= BP_MAX_INTRO_EVENTS || affected_slots == 0) return;
    bp->intro_events[bp->intro_event_count++] = (BpIntroEvent){
        .value = value,
        .affected_slots = affected_slots,
        .faction = faction,
        .rune_def = -1,
    };
}

static unsigned BpFormationSlots(const BattleSim *sim, int side) {
    unsigned slots = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const SimUnit *u = &sim->units[side * SQUAD_SLOT_COUNT + slot];
        if (u->present && !u->ch->giant) slots |= 1u << slot;
    }
    return slots;
}

static bool BpAnyGameChangerApplied(const BattleSim *sim) {
    for (int i = 0; i < SIM_UNIT_COUNT; i++)
        if (sim->units[i].present && sim->units[i].game_changer_applied) return true;
    return false;
}

static void BpBuildIntroEvents(BattlePlay *bp) {
    bp->intro_event_count = 0;
    bp->intro_event = -1;

    for (int i = 0; i < bp->sim.rune_def_count; i++)
        bp->intro_events[bp->intro_event_count++] = (BpIntroEvent){.rune_def = bp->sim.rune_defs[i]};

    if (bp->sim.formation_bonus_pct[0] > 0.0f)
        BpQueueIntroEvent(
            bp, bp->sim.formation_bonus_pct[0], BpFormationSlots(&bp->sim, 0), bp->sim.formation_faction[0]
        );
}

static void BpQueueGameChangerEvent(BattlePlay *bp, const BattleRoster *enemy) {
    if (!enemy->game_changer) return;
    bool applied = BpAnyGameChangerApplied(&bp->sim);
    bp->intro_events[bp->intro_event_count++] =
        (BpIntroEvent){.rune_def = -1, .event_bonus = applied, .no_event_bonus = !applied};
}

static void BpResetInitiative(BattlePlay *bp, Battle *b, int unit) {
    if (unit < 0 || unit >= SIM_UNIT_COUNT) return;
    BpUnitAt(bp, unit)->current_initiative = 0;
    BattleFxStatusClear(b, FX_AURA_INITIATIVE, unit / SQUAD_SLOT_COUNT, unit % SQUAD_SLOT_COUNT);
}

static void BpCleanupStatusFx(Battle *b) {
    for (int side = 0; side < 2; side++)
        for (int unit_slot = 0; unit_slot < SQUAD_SLOT_COUNT; unit_slot++) BpClearUnitStatusFx(b, side, unit_slot);
}

void BattlePlayStart(BattlePlay *bp, Battle *b, const Player *player, unsigned seed) {
    BpCleanupStatusFx(b);
    BpUnstageAction(b);
    BattleBeginCombatCamera(b);
    memset(bp, 0, sizeof(*bp));
    BattleSimRun(&bp->sim, player, &b->enemy_roster, seed);
    BpBuildIntroEvents(bp);
    BpQueueGameChangerEvent(bp, &b->enemy_roster);
    bp->state = BP_INTRO;
    bp->action = 0;
    bp->cur_round = 1;
    bp->slowmo = 1.0f;
    bp->speed_mult = 1.0f;

    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            BpUnit *u = &bp->units[side][slot];
            const SimUnit *su = &bp->sim.units[side * SQUAD_SLOT_COUNT + slot];
            const BattleSpot *spot = BpSpot(side, slot);
            float feet_y = BattleFeetY(su->present && su->ch->giant ? 2.0f : 1.0f);
            u->home = u->pos = (Vector3){spot->x, feet_y, spot->z};
            float dx = BP_STAGE_X - spot->x, dz = BP_STAGE_Z - spot->z;
            u->hop_total = sqrtf(dx * dx + dz * dz);
            u->max_health = su->present ? su->max_health : 0;
            u->health = u->max_health;
            u->power = u->start_power = su->present ? su->start_power : 0;
            u->ring_pct = u->ring_from = u->ring_to = 1.0f;
        }
    }
    BattleAudioCombatMusic(b);
    TraceLog(
        LOG_INFO,
        "Play: battle started, %d actions, sim says %s",
        bp->sim.action_count,
        bp->sim.player_won ? "WIN" : "LOSS"
    );
}

static void BpEnterAction(BattlePlay *bp, Battle *b) {
    if (bp->action >= bp->sim.action_count) {
        BpUnstageAction(b);
        BpCleanupStatusFx(b);
        bp->state = BP_END_SPLASH;
        bp->t = 0.0f;
        bp->won = bp->sim.player_won;
        BattleAudioResultMusic(b, bp->won);
        return;
    }
    if (bp->action == 0) BpTelegraphCrits(bp, b);
    const BattleSimAction *a = &bp->sim.actions[bp->action];
    if (a->newround) {
        bp->cur_round++;
        bp->action++;
        BpTelegraphCrits(bp, b);
        BpEnterAction(bp, b);
        return;
    }
    BattleAilment wear = AIL_UNSTUNNED | AIL_UNCURSE_POWER | AIL_UNCURSE_HEALTH;
    if (a->ailments & wear) {
        if (a->actor >= 0) {
            int side = a->actor / SQUAD_SLOT_COUNT, slot = a->actor % SQUAD_SLOT_COUNT;
            if (a->ailments & AIL_UNSTUNNED) BattleFxStatusClear(b, FX_AURA_STUN, side, slot);
            if (a->ailments & AIL_UNCURSE_POWER) BattleFxStatusClear(b, FX_AURA_CURSE_POWER, side, slot);
            if (a->ailments & AIL_UNCURSE_HEALTH) BattleFxStatusClear(b, FX_AURA_CURSE_HEALTH, side, slot);
            BpResetInitiative(bp, b, a->actor);
        }
        bp->action++;
        bp->state = BP_ACT_DELAY;
        bp->t = 0.0f;
        return;
    }
    bp->actor_init_class = BpInitiativeClass(bp, a->actor);
    BpStageAction(bp, b, bp->action);
    bp->state = BP_ACT_START;
    bp->t = 0.0f;
    bp->slowmo = 1.0f;
}

static const char *BpFactionFx(unsigned faction) {
    switch (faction) {
        case 1u:
            return "fx_eff_blackfaction_charge_pre";
        case 4u:
            return "fx_eff_redfaction_charge_pre";
        case 8u:
            return "fx_eff_bluefaction_charge_pre";
        default:
            return "fx_eff_faction_charge_pre";
    }
}

static void BpPlayIntroEvent(BattlePlay *bp, Battle *b, const BpIntroEvent *event) {
    if (event->no_event_bonus) return;
    if (event->event_bonus) {
        BattleAudioPlayGlobal(b, "gui_bonus_faction");
        for (int i = 0; i < SIM_UNIT_COUNT; i++) {
            if (bp->sim.units[i].present && bp->sim.units[i].game_changer_applied)
                BattleFxUnitColorAura(
                    b, BP_RUNE_AURA_FX, i / SQUAD_SLOT_COUNT, i % SQUAD_SLOT_COUNT, BP_RUNE_AURA_COLOR
                );
        }
        return;
    }
    if (event->rune_def >= 0) {
        BattleAudioPlayGlobal(b, "gui_bonus_rune");
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
            if (bp->sim.units[slot].present) BattleFxUnitColorAura(b, BP_RUNE_AURA_FX, 0, slot, BP_RUNE_AURA_COLOR);
        return;
    }
    BattleAudioPlayGlobal(b, "gui_bonus_faction");
    const char *fx = BpFactionFx(event->faction);
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (event->affected_slots & (1u << slot)) BattleFxUnitBurst(b, fx, 0, slot, (Vector3){0, 0, 0});
    (void)bp;
}

static void BpBeginNextIntroEvent(BattlePlay *bp, Battle *b) {
    bp->intro_event++;
    if (bp->intro_event >= bp->intro_event_count) {
        BpEnterAction(bp, b);
        return;
    }
    bp->state = BP_INTRO_BANNER;
    bp->t = 0.0f;
    BpPlayIntroEvent(bp, b, &bp->intro_events[bp->intro_event]);
}

static bool BpBreathMuzzle(const BattlePlay *bp, Battle *b, int actor, Vector3 *position, Vector3 *aim) {
    const Character *ch = bp->sim.units[actor].ch;
    const BattleVfxWire *wire = ch ? BattleUnitWire(b, ch->id, ch->tier) : NULL;
    if (!wire || !wire->muzzle_bone[0]) return false;
    const HomeSquadUnit *unit = &b->squads.units[actor / SQUAD_SLOT_COUNT][actor % SQUAD_SLOT_COUNT];
    return HomeSquadUnitBoneAnchor(unit, wire->muzzle_bone, wire->muzzle_offset, wire->muzzle_forward, position, aim);
}

static void BpSteerBreath(BattlePlay *bp, Battle *b) {
    if (bp->breath_fx < 0) return;
    Vector3 position, aim;
    bool steered = BpBreathMuzzle(bp, b, bp->breath_actor, &position, &aim) &&
                   BattleFxSteer(b, bp->breath_fx, bp->breath_serial, position, aim);
    if (!steered) bp->breath_fx = -1;
}

static float BpConeImpactDelay(const BattleVfxWire *wire) {
    if (!wire || wire->ability_type != 6 || wire->vfx_type != 2) return 0.0f;
    return wire->cone_delay;
}

static void BpFire(BattlePlay *bp, Battle *b) {
    const BattleSimAction *a = &bp->sim.actions[bp->action];
    int targets[SIM_MAX_TARGETS];
    int target_count = BpCollectSwingTargets(&bp->sim, bp->action, targets);
    BpUnit *actor = BpUnitAt(bp, a->actor);
    int actor_side = a->actor / SQUAD_SLOT_COUNT, actor_slot = a->actor % SQUAD_SLOT_COUNT;
    const Character *ch = bp->sim.units[a->actor].ch;

    const BattleVfxWire *w = BattleUnitWire(b, ch->id, ch->tier);
    const char *ability = w && w->ability[0] ? w->ability : NULL;
    const char *impact = w && w->impact[0] ? w->impact : NULL;
    const char *muzzle = w && w->muzzle[0] ? w->muzzle : NULL;
    const char *sustained = w && w->sustained[0] ? w->sustained : NULL;
    int atype = w ? w->ability_type : -1;
    bool aoe = atype == 3 || atype == 4 || atype == 5;
    float pspeed = 8.0f;
    bool projectile;
    if (w) {
        projectile = w->vfx_type == 0;
        if (w->pspeed > 0) pspeed = w->pspeed;
    } else {
        projectile = BpIsProjectile(ch, &pspeed);
    }

    Vector3 attack_origin = BpChest(bp, actor, a->actor);
    Vector3 attack_aim = attack_origin;
    if (target_count > 0) {
        attack_aim = (Vector3){0};
        for (int i = 0; i < target_count; i++) {
            BpUnit *victim = BpUnitAt(bp, targets[i]);
            attack_aim = Vector3Add(attack_aim, BpChest(bp, victim, targets[i]));
        }
        attack_aim = Vector3Scale(attack_aim, 1.0f / target_count);
    }
    if (muzzle) BattleFxBurstDirected(b, muzzle, attack_origin, attack_aim);

    Vector3 centroid = {0};
    if (target_count > 0) {
        for (int i = 0; i < target_count; i++) {
            BpUnit *v = BpUnitAt(bp, targets[i]);
            centroid = Vector3Add(centroid, v->pos);
        }
        centroid = Vector3Scale(centroid, 1.0f / target_count);
        centroid.y = BattleFeetY(1.0f);
    }

    if (projectile && ability && target_count > 0) {
        int shots = aoe ? 1 : target_count;
        for (int i = 0; i < shots && i < BP_MAX_PROJECTILES; i++) {
            BpProjectile *p = &bp->projectiles[i];
            BpUnit *victim = BpUnitAt(bp, targets[i]);
            *p = (BpProjectile){0};
            p->active = true;
            p->pos = (Vector3){actor->pos.x, actor->pos.y + 1.1f, actor->pos.z};
            p->target = aoe ? centroid : BpChest(bp, victim, targets[i]);
            p->speed = pspeed;
            p->look = BpProjLook(b, ability);
            StrCopy(p->trail, sizeof(p->trail), ability);
            if (impact) StrCopy(p->impact, sizeof(p->impact), impact);
            if (sustained) StrCopy(p->sustained, sizeof(p->sustained), sustained);
        }
        bp->state = BP_ACT_FLIGHT;
        bp->t = 0.0f;
        BattleAudioPlayUnit(b, actor_side, actor_slot, UNIT_SFX_ATTACK);
        return;
    }
    BattleAudioPlayUnit(b, actor_side, actor_slot, UNIT_SFX_ATTACK);
    if (projectile && !ability) {
    } else if (w && w->vfx_type == 2) {
        const char *cone = ability ? ability : "fx_atk_lineslash_pre";
        Vector3 mouth, mouth_aim;
        if (BpBreathMuzzle(bp, b, a->actor, &mouth, &mouth_aim)) {
            bp->breath_fx = BattleFxBurstSteered(b, cone, mouth, mouth_aim, &bp->breath_serial);
            bp->breath_actor = a->actor;
        } else {
            BattleFxBurstDirected(b, cone, attack_origin, attack_aim);
        }
    } else if (aoe && target_count > 0) {
        BattleFxBurst(b, ability ? ability : "fx_atk_lineslash_pre", centroid);
    } else {
        for (int i = 0; i < target_count; i++) {
            BpUnit *v = BpUnitAt(bp, targets[i]);
            Vector3 at = atype == 7 ? v->pos : BpChest(bp, v, targets[i]);
            BattleFxBurst(b, ability ? ability : "fx_atk_lineslash_pre", at);
        }
    }
    float cone_delay = BpConeImpactDelay(w);
    if (cone_delay > bp->t) {
        bp->cone_impact_at = cone_delay;
        bp->state = BP_ACT_CONE_DELAY;
        return;
    }
    BpApplyAction(bp, b, a);
    bp->message_t = 0.0f;
    bp->state = BP_ACT_MESSAGES;
    bp->t = 0.0f;
}

static void BpAfterMessages(BattlePlay *bp, Battle *b) {
    const BattleSimAction *cur = &bp->sim.actions[bp->action];
    int actor = cur->actor;
    bp->action++;
    bp->message_count = 0;

    if (bp->action < bp->sim.action_count) {
        const BattleSimAction *next = &bp->sim.actions[bp->action];
        BattleAilment wear = AIL_UNSTUNNED | AIL_UNCURSE_POWER | AIL_UNCURSE_HEALTH;
        if (!next->newround && next->actor == actor && !(next->ailments & wear)) {
            if (!next->replay_attack) {
                BpApplyAction(bp, b, next);
                bp->message_t = 0.0f;
                bp->state = BP_ACT_MESSAGES;
                bp->t = 0.0f;
                return;
            }
            BpUnstageAction(b);
            BpStageAction(bp, b, bp->action);
            bp->slowmo = next->crit ? BP_CRIT_TIMESCALE : 1.0f;
            BpUnit *u = BpUnitAt(bp, actor);
            const ModelAnimation *atk =
                BattleSquadClip(&b->squads, actor / SQUAD_SLOT_COUNT, actor % SQUAD_SLOT_COUNT, "attack");
            BpStartClip(u, atk, 1.0f, false);
            BattleAudioPlayUnit(b, actor / SQUAD_SLOT_COUNT, actor % SQUAD_SLOT_COUNT, UNIT_SFX_VOCAL);
            BattleAudioPlayUnit(b, actor / SQUAD_SLOT_COUNT, actor % SQUAD_SLOT_COUNT, UNIT_SFX_ANIMATION);
            u->fired = false;
            bp->state = BP_ACT_WINDUP;
            bp->t = 0.0f;
            return;
        }
    }
    BpUnstageAction(b);
    bp->slowmo = 1.0f;
    bp->returning_actor = actor;
    BpUnit *u = BpUnitAt(bp, actor);
    u->phase = BPU_RETURN;
    bp->state = BP_ACT_RETURN;
    bp->t = 0.0f;
    BattleAudioPlayUnit(b, actor / SQUAD_SLOT_COUNT, actor % SQUAD_SLOT_COUNT, UNIT_SFX_MOVE_BACK);
}

static void BpRevealMessageAt(BattlePlay *bp, Battle *b, int idx) {
    if (idx < 0 || idx >= bp->message_count) return;
    BpMessage *m = &bp->messages[idx];
    if (m->gold_reward <= 0 || m->reward_applied) return;
    m->reward_applied = true;
    bp->total_bonus_gold += m->gold_reward;
    if (bp->gold_counter_alpha < 0.001f) bp->gold_counter_alpha = 0.001f;
    bp->reward_anim_t = 0.0f;
    bp->reward_anim_active = true;
    BattleAudioPlayGlobal(b, "gui_add_gold1");
}

static void BpRevealMessagesBetween(BattlePlay *bp, Battle *b, float from, float through) {
    if (bp->message_count <= 0 || through < from || through < 0.0f) return;
    if (from < 0.0f) from = 0.0f;
    int first = (int)(from / BP_MESSAGE_TIME);
    if (first >= bp->message_count) return;
    int last = (int)(through / BP_MESSAGE_TIME);
    if (last >= bp->message_count) last = bp->message_count - 1;
    for (int idx = first; idx <= last; idx++) BpRevealMessageAt(bp, b, idx);
}

void BattlePlayUpdate(BattlePlay *bp, Battle *b, float raw_dt) {
    if (bp->state == BP_OFF) return;
    float dt = raw_dt * bp->speed_mult * bp->slowmo;

    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            BpUnit *u = &bp->units[side][slot];
            if (u->ring_t < BP_RING_TWEEN) {
                u->ring_t += raw_dt;
                float f = u->ring_t >= BP_RING_TWEEN ? 1.0f : u->ring_t / BP_RING_TWEEN;
                u->ring_pct = u->ring_from + (u->ring_to - u->ring_from) * f;
            }
            if (u->dying) {
                u->dissolve -= dt / BP_DISSOLVE_TIME;
                if (u->dissolve <= 0.0f) {
                    u->dissolve = 0.0f;
                    u->dying = false;
                    u->dead = true;
                }
            }
            if (u->damage_shake_left > 0.0f) {
                u->damage_shake_left -= dt;
                if (u->damage_shake_left < 0.0f) u->damage_shake_left = 0.0f;
            }
            if (u->hit_timer > 0.0f) {
                u->hit_timer -= dt;
                if (u->hit_timer <= 0.0f && !u->clip_hold) u->clip = NULL;
            }
            BpAdvanceClip(u, dt);
        }
    }
    if (bp->shake_dur > 0.0f) {
        bp->shake_t += raw_dt;
        if (bp->shake_t >= bp->shake_dur) bp->shake_dur = 0.0f;
    }
    if (bp->reward_anim_active) {
        bp->reward_anim_t += dt;
        if (bp->reward_anim_t >= BP_REWARD_ANIM_TIME) bp->reward_anim_active = false;
    }
    if (bp->total_bonus_gold > 0 && bp->gold_counter_alpha < 1.0f) {
        bp->gold_counter_alpha += dt / 0.15f;
        if (bp->gold_counter_alpha > 1.0f) bp->gold_counter_alpha = 1.0f;
    }
    Vector3 shake = {0};
    if (bp->shake_dur > 0.0f) {
        float decay = 1.0f - bp->shake_t / bp->shake_dur;
        shake.x = bp->shake_amp * decay * sinf(bp->shake_t * 43.0f);
        shake.y = bp->shake_amp * decay * sinf(bp->shake_t * 57.0f + 1.7f);
    }
    b->cam_shake = shake;

    switch (bp->state) {
        case BP_INTRO:
            if (b->combat_cam_t < 1.0f) break;
            bp->t += dt;
            if (bp->t >= BP_INTRO_DELAY) BpBeginNextIntroEvent(bp, b);
            break;
        case BP_INTRO_BANNER:
            bp->t += dt;
            if (bp->t >= BP_BANNER_TIME) BpBeginNextIntroEvent(bp, b);
            break;
        case BP_ACT_START: {
            const BattleSimAction *a = &bp->sim.actions[bp->action];
            BpUnit *u = BpUnitAt(bp, a->actor);
            u->phase = BPU_LEAP_OUT;
            BattleAudioPlayUnit(b, a->actor / SQUAD_SLOT_COUNT, a->actor % SQUAD_SLOT_COUNT, UNIT_SFX_MOVE_OUT);
            bp->state = BP_ACT_LEAP;
            bp->t = 0.0f;
            break;
        }
        case BP_ACT_LEAP: {
            const BattleSimAction *a = &bp->sim.actions[bp->action];
            BpUnit *u = BpUnitAt(bp, a->actor);
            if (BpMoveToward(u, (Vector3){BP_STAGE_X, u->home.y, BP_STAGE_Z}, dt, BP_ARRIVE_STAGE)) {
                u->phase = BPU_WINDUP;
                const ModelAnimation *atk =
                    BattleSquadClip(&b->squads, a->actor / SQUAD_SLOT_COUNT, a->actor % SQUAD_SLOT_COUNT, "attack");
                BpStartClip(u, atk, 1.0f, false);
                u->fired = false;
                BattleFxBurst(b, "fx_placeunit_noLoop_pre", (Vector3){BP_STAGE_X, BattleFeetY(1.0f), BP_STAGE_Z});
                BattleAudioPlayUnit(b, a->actor / SQUAD_SLOT_COUNT, a->actor % SQUAD_SLOT_COUNT, UNIT_SFX_LAND);
                BattleAudioPlayUnit(b, a->actor / SQUAD_SLOT_COUNT, a->actor % SQUAD_SLOT_COUNT, UNIT_SFX_VOCAL);
                BattleAudioPlayUnit(b, a->actor / SQUAD_SLOT_COUNT, a->actor % SQUAD_SLOT_COUNT, UNIT_SFX_ANIMATION);
                bp->slowmo = a->crit ? BP_CRIT_TIMESCALE : 1.0f;
                bp->state = BP_ACT_WINDUP;
                bp->t = 0.0f;
            }
            break;
        }
        case BP_ACT_WINDUP: {
            const BattleSimAction *a = &bp->sim.actions[bp->action];
            BpUnit *u = BpUnitAt(bp, a->actor);
            bp->t += dt;
            BpMoveToward(u, (Vector3){BP_STAGE_X, u->home.y, BP_STAGE_Z}, dt, BP_ARRIVE_STAGE);
            float launch = BpLaunchPct(bp->sim.units[a->actor].ch);
            float frac = u->clip && u->clip->frameCount > 1 ? u->clip_time / (float)(u->clip->frameCount - 1) : 1.0f;
            if (!u->clip) frac = 1.0f;
            if (!u->fired && frac >= launch) {
                u->fired = true;
                BpFire(bp, b);
            }
            break;
        }
        case BP_ACT_CONE_DELAY:
            bp->t += dt;
            if (bp->t >= bp->cone_impact_at) {
                bp->cone_impact_at = 0.0f;
                BpApplyAction(bp, b, &bp->sim.actions[bp->action]);
                bp->message_t = 0.0f;
                bp->state = BP_ACT_MESSAGES;
                bp->t = 0.0f;
            }
            break;
        case BP_ACT_FLIGHT: {
            bool all_done = true;
            for (int i = 0; i < BP_MAX_PROJECTILES; i++) {
                BpProjectile *p = &bp->projectiles[i];
                if (!p->active) continue;
                Vector3 d = Vector3Subtract(p->target, p->pos);
                float dist = Vector3Length(d);
                float step = p->speed * dt;
                if (step >= dist) {
                    p->active = false;
                    BattleFxTrailEnd(b, p->trail_serial);
                    if (p->impact[0]) BattleFxBurst(b, p->impact, p->target);
                    if (p->sustained[0]) BattleFxBurst(b, p->sustained, p->target);
                } else {
                    Vector3 vel = Vector3Scale(Vector3Normalize(d), p->speed);
                    p->pos = Vector3Add(p->pos, Vector3Scale(vel, dt));
                    p->age += dt;
                    if (p->trail[0])
                        BattleFxTrail(
                            b, p->trail, p->pos, vel, p->age, dt, p->accum, p->trail_burst_done, &p->trail_serial
                        );
                    all_done = false;
                }
            }
            if (all_done) {
                int actor = bp->sim.actions[bp->action].actor;
                BattleAudioPlayUnit(b, actor / SQUAD_SLOT_COUNT, actor % SQUAD_SLOT_COUNT, UNIT_SFX_IMPACT);
                BpApplyAction(bp, b, &bp->sim.actions[bp->action]);
                bp->message_t = 0.0f;
                bp->state = BP_ACT_MESSAGES;
                bp->t = 0.0f;
            }
            break;
        }
        case BP_ACT_MESSAGES: {
            float previous_message_t = bp->message_t;
            bp->message_t += dt;
            BpRevealMessagesBetween(bp, b, previous_message_t, bp->message_t);
            if (bp->message_t >= BP_MESSAGE_TIME * (bp->message_count > 0 ? bp->message_count : 0.3f))
                BpAfterMessages(bp, b);
            break;
        }
        case BP_ACT_RETURN: {
            bool arrived = true;
            for (int side = 0; side < 2; side++) {
                for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
                    BpUnit *u = &bp->units[side][slot];
                    if (u->phase != BPU_RETURN) continue;
                    if (!BpMoveToward(u, u->home, dt, BP_ARRIVE_PAD)) {
                        arrived = false;
                        continue;
                    }
                    u->pos = u->home;
                    u->phase = BPU_IDLE;
                    if (!u->clip_hold) u->clip = NULL;
                }
            }
            if (arrived) {
                BpResetInitiative(bp, b, bp->returning_actor);
                bp->slowmo = 1.0f;
                bp->state = BP_ACT_DELAY;
                bp->t = 0.0f;
            }
            break;
        }
        case BP_ACT_DELAY:
            bp->t += dt;
            if (bp->t >= BP_DELAY_BETWEEN_ACTIONS) BpEnterAction(bp, b);
            break;
        case BP_END_SPLASH:
            bp->t += dt;
            if (bp->t >= (bp->won ? 2.0f : 5.0f)) {
                bp->state = BP_SUMMARY;
                bp->t = 0.0f;
            }
            break;
        case BP_SUMMARY:
            bp->t += dt;
            break;
        default:
            break;
    }

    b->proj_count = 0;
    for (int i = 0; i < BP_MAX_PROJECTILES && b->proj_count < 8; i++) {
        BpProjectile *p = &bp->projectiles[i];
        if (!p->active || bp->state != BP_ACT_FLIGHT) continue;
        Vector3 d = Vector3Subtract(p->target, p->pos);
        Vector3 vel = Vector3Length(d) > 0.001f ? Vector3Scale(Vector3Normalize(d), p->speed) : d;
        b->proj[b->proj_count++] = (struct BattleProj){p->pos, vel, p->look};
    }

    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            BpUnit *u = &bp->units[side][slot];
            const SimUnit *su = &bp->sim.units[side * SQUAD_SLOT_COUNT + slot];
            Vector3 render_pos = Vector3Add(u->pos, BpDamageShakeOffset(u, side * SQUAD_SLOT_COUNT + slot));
            float gk = su->present && su->ch->giant ? 2.0f : 1.0f;
            b->rings[side][slot] = (struct BattleRing){su->present, render_pos, gk, u->ring_pct};

            if (su->present)
                SquadPadPlaceAtFeet(
                    render_pos,
                    gk,
                    &b->squads.pad_xform[side][slot],
                    &b->squads.fac_xform[side][slot],
                    &b->squads.shadow_xform[side][slot]
                );
            HomeSquadUnit *hu = &b->squads.units[side][slot];
            if (!hu->active) continue;
            const Character *ch = bp->sim.units[side * SQUAD_SLOT_COUNT + slot].ch;
            if (!ch) continue;
            float ds = b->squads.unit_scale[side][slot];
            if (u->dead) ds = 0.0f;
            hu->fade = u->dying ? u->dissolve : 1.0f;
            hu->xform = MatrixMultiply(
                MatrixMultiply(
                    MatrixScale(ds, ds, ds), MatrixRotateY(side == 0 ? BATTLE_PLAYER_YAW : BATTLE_ENEMY_YAW)
                ),
                MatrixTranslate(render_pos.x, render_pos.y, render_pos.z)
            );
            BpSetAnimationOverride(hu, u->clip != NULL || u->dead);
            if (u->dead) continue;
            if (u->clip != u->shown_clip) {
                HomeSquadUnitBeginBlend(hu, u->clip ? CLIP_ENTER_BLEND_TIME : CLIP_EXIT_BLEND_TIME);
                u->shown_clip = u->clip;
            }
            if (u->clip) {
                int frame = (int)u->clip_time;
                if (frame >= u->clip->frameCount) frame = u->clip->frameCount - 1;
                HomeSquadUnitShowFrame(hu, *u->clip, frame);
            }
        }
    }
    BpSteerBreath(bp, b);
}

bool BattlePlayActive(const BattlePlay *bp) {
    return bp->state != BP_OFF;
}
float BattlePlayTimeScale(const BattlePlay *bp) {
    return bp->state != BP_OFF ? bp->speed_mult * bp->slowmo : 1.0f;
}
bool BattlePlayFinished(const BattlePlay *bp) {
    return bp->state == BP_SUMMARY;
}
void BattlePlayStop(BattlePlay *bp) {
    bp->state = BP_OFF;
}
void BattlePlayToggleSpeed(BattlePlay *bp) {
    bp->speed_mult = bp->speed_mult > 1.5f ? 1.0f : 3.0f;
}

float BattlePlayTeamPct(const BattlePlay *bp, int side) {
    long cur = 0, max = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const BpUnit *u = &bp->units[side][slot];
        cur += u->health;
        max += u->max_health;
    }
    return max > 0 ? (float)cur / (float)max : 0.0f;
}

int BattlePlayActorInitiativeClass(const BattlePlay *bp) {
    return bp->actor_init_class;
}

int BattlePlayCurrentActor(const BattlePlay *bp) {
    if (bp->state < BP_ACT_START || bp->state > BP_ACT_DELAY) return -1;
    if (bp->action >= bp->sim.action_count) return -1;
    const BattleSimAction *a = &bp->sim.actions[bp->action];
    return a->newround ? -1 : a->actor;
}

typedef struct {
    int dmg[2], crits[2], fac_hits[2], survivors[2];
    long start_hp[2];
    char mvp[2][64];
} BattleSummary;

void BattlePlaySummary(const BattlePlay *bp, BattleSummary *out) {
    memset(out, 0, sizeof(*out));
    int dealt[SIM_UNIT_COUNT] = {0};
    for (int side = 0; side < 2; side++)
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            const SimUnit *su = &bp->sim.units[side * SQUAD_SLOT_COUNT + slot];
            if (!su->present) continue;
            out->start_hp[side] += su->max_health;
            if (su->alive) out->survivors[side]++;
        }
    for (int i = 0; i < bp->sim.action_count;) {
        const BattleSimAction *a = &bp->sim.actions[i];
        if (a->newround || a->actor < 0) {
            i++;
            continue;
        }
        int aside = a->actor / SQUAD_SLOT_COUNT;
        bool crit = false, hit_enemy = false, fac = false;
        int next = i;
        do {
            const BattleSimAction *part = &bp->sim.actions[next];
            crit |= part->crit;
            for (int t = 0; t < part->target_count; t++) {
                const BattleSimTarget *bt = &part->targets[t];
                int hp = bt->health_delta + bt->health_faction_delta;
                if (bt->unit / SQUAD_SLOT_COUNT != aside && hp < 0) {
                    out->dmg[aside] += -hp;
                    dealt[a->actor] += -hp;
                    hit_enemy = true;
                    if (bt->health_faction_delta < 0) fac = true;
                }
            }
            next++;
        } while (BpSwingChainContinues(&bp->sim, next, a->actor));
        if (crit && hit_enemy) out->crits[aside]++;
        if (fac) out->fac_hits[aside]++;
        i = next;
    }
    for (int side = 0; side < 2; side++) {
        int best = -1, best_dmg = -1;
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            int idx = side * SQUAD_SLOT_COUNT + slot;
            if (bp->sim.units[idx].present && dealt[idx] > best_dmg) {
                best_dmg = dealt[idx];
                best = idx;
            }
        }
        if (best >= 0) {
            const Character *ch = bp->sim.units[best].ch;
            char key[96];
            snprintf(key, sizeof(key), "%s%s", ch->id, TIER_NAMES[ch->tier]);
            const char *name = LocGet(key);
            StrCopy(out->mvp[side], sizeof(out->mvp[side]), name[0] ? name : ch->name_ru[0] ? ch->name_ru : ch->name);
        }
    }
}

int BattlePlayCriticalGold(const BattlePlay *bp) {
    BattleSummary summary;
    BattlePlaySummary(bp, &summary);
    return summary.crits[0] > INT_MAX / BP_CRIT_GOLD_REWARD ? INT_MAX : summary.crits[0] * BP_CRIT_GOLD_REWARD;
}
