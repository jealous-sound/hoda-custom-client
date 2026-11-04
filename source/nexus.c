
#define NEXUS_TAP_SLOP 20.0f
#define NEXUS_GAME_CHANGERS_PATH "data/nexus_game_changers.json"
#define NEXUS_DRAG_GAIN 10.0f
#define NEXUS_WHEEL_STEP 0.07f
#define NEXUS_EDGE_BUFFER 3.0f
#define NEXUS_DRIFT_MAX_TIME 0.5f
#define NEXUS_FOCUS_TIME 4.0f
#define NEXUS_BOSS_SCALE 1.3f
#define NEXUS_ICON_HALF_WIDTH 3.5f
#define NEXUS_ICON_HALF_HEIGHT 4.0f
#define NEXUS_ICON_OFFSET_Z -0.128f
#define NEXUS_RING_HALF_SIZE 2.75f
#define NEXUS_RING_OFFSET_Z 0.56f
#define NEXUS_RING_BOSS_OFFSET_Z 0.71f
#define NEXUS_RING_DEGREES_PER_SECOND 45.0f
#define NEXUS_BANNER_HALF_WIDTH 1.84f
#define NEXUS_BANNER_HALF_HEIGHT 0.7425f
#define NEXUS_BANNER_OFFSET_Z 2.405f
#define NEXUS_FLASK_OFFSET_X 0.716f
#define NEXUS_FLASK_OFFSET_Z 2.385f
#define NEXUS_FLASK_HALF_WIDTH 0.4f
#define NEXUS_FLASK_HALF_HEIGHT 0.45f
#define NEXUS_ENERGY_TEXT_OFFSET_X -0.328f
#define NEXUS_ENERGY_TEXT_OF_BANNER 0.5f
#define NEXUS_ROTATION_VISIBLE_EVENTS 3
#define NEXUS_QUIT_BUTTON_GAP 16.0f
#define NEXUS_COMPLETE_BANNER_DELAY 1.3f
#define NEXUS_COMPLETE_LOOT_DELAY_PER_ITEM 0.1f
#define NEXUS_COMPLETE_BANNER_TIME 2.0f
#define NEXUS_COMPLETE_BANNER_FADE_TIME 0.3f

typedef enum {
    NEXUS_NONE = 0,
    NEXUS_ENTER,
    NEXUS_LEAVE,
    NEXUS_QUIT,
    NEXUS_REVIVE,
    NEXUS_SHOW_RECAP,
} NexusAction;

typedef struct {
    int step;
    float x, z;
    int gold, xp;
    bool boss;
    int reward_rarity[6];
    int reward_rarity_count;
    int potion_reward;
} NexusNode;

typedef struct {
    char id[80];
    char difficulty[16];
    int energy_cost, potion_cost;
    char battle_environment[48];
    int node_count;
    NexusNode nodes[NEXUS_MAX_NODES];
} NexusDifficulty;

typedef struct {
    char id[48];
    char name[96];
    char kind[16];
    char subtitle[64];
    char hub_image[64];
    int weekday;
    char prefab[64];
    Color highlight;
    int unlock_key, unlock_key_qty, max_runs;
    long long duration;
    long long unlock_duration;
    int node_count;
    int difficulty_count;
    NexusDifficulty difficulties[NEXUS_MAX_DIFFICULTIES];
} NexusEvent;

typedef struct {
    Font font;
    int event_count;
    NexusEvent events[NEXUS_MAX_EVENTS];

    NexusBackground bg;
    bool bg_loaded;
    char bg_prefab[64];
    int active_event, active_difficulty;

    Button enter, retreat, quit_stay, quit_confirm;
    bool quit_open, quit_opening_release;
    float ring_angle;
    Texture2D lore, progress_bar, energy_plate, energy_flask;
    Texture2D gold_icon, xp_icon, potion_icon, reward_tag, divider, timer_plate;
    Texture2D glow, ring;
    CampaignIcons icons;

    float scroll_x;
    float scroll_velocity, drift_time;
    float focus_time, focus_from, focus_to;
    bool focusing;
    Vector2 press_pos;
    bool dragged;
    int selected_node;

    bool anim_active;
    float anim_time;
    int anim_from, anim_to;
    bool anim_sound_played;

    bool final_complete;
    float complete_time;
    bool complete_sound_played;
    bool recap_requested;
    PlayerNexusRun completed_run;
    BattleGameChanger game_changers[NEXUS_MAX_EVENTS];
} Nexus;

static float NexusEaseInOutQuad(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

static float NexusFlipScale(float t) {
    if (t < 0.0f || t > 1.0f) return 1.0f;
    return t < 0.5f ? 1.0f - NexusEaseInOutQuad(t * 2.0f) : NexusEaseInOutQuad((t - 0.5f) * 2.0f);
}

int NexusFindEvent(const Nexus *n, const char *event_id) {
    for (int i = 0; i < n->event_count; i++) {
        if (strcmp(n->events[i].id, event_id) == 0) return i;
    }
    return -1;
}

static void NexusGameChangersLoad(Nexus *n, const char *path) {
    cJSON *root = JsonLoadFile(path);
    if (!cJSON_IsArray(root)) TraceLog(LOG_FATAL, "Nexus: %s is not an array", path);
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        int event = NexusFindEvent(n, JsonStr(item, "event"));
        if (event < 0) TraceLog(LOG_FATAL, "Nexus: changer for unknown event %s", JsonStr(item, "event"));
        BattleGameChanger *changer = &n->game_changers[event];
        StrCopy(changer->text_key, sizeof(changer->text_key), JsonStr(item, "text_key"));
        StrCopy(changer->text_ru, sizeof(changer->text_ru), JsonStr(item, "text_ru"));
        cJSON *effect = NULL;
        cJSON_ArrayForEach(effect, cJSON_GetObjectItemCaseSensitive(item, "effects")) {
            if (changer->effect_count == BATTLE_CHANGER_MAX_EFFECTS)
                TraceLog(LOG_FATAL, "Nexus: too many changer effects");
            BattleChangerEffectLoad(&changer->effects[changer->effect_count++], effect);
        }
    }
    cJSON_Delete(root);
}

void NexusInitialize(Nexus *n, const char *maps_path) {
    memset(n, 0, sizeof(*n));
    n->font = GuiFont(FNT_COOL);
    n->active_event = -1;
    n->active_difficulty = -1;

    n->lore = GuiTex(GUI_MAP_LORE);
    n->progress_bar = GuiTex(GUI_MAP_PROGRESS);
    n->energy_plate = GuiTex(GUI_MAP_ENERGY);
    n->energy_flask = GuiTex(GUI_HUD_ENERGY);
    n->gold_icon = GuiTex(GUI_HUD_GOLD);
    n->xp_icon = GuiTex(GUI_HUD_XP);
    n->potion_icon = GuiTex(GUI_NEXUS_POTION);
    n->reward_tag = GuiTex(GUI_MAP_REWARD_TAG);
    n->divider = GuiTex(GUI_NEXUS_DIVIDER);
    n->timer_plate = GuiTex(GUI_NEXUS_TIMER_PLATE);
    n->glow = GuiTex(GUI_NODE_UNLOCK_GLOW);
    n->ring = GuiTex(GUI_NODE_SELECT_RING);
    n->icons.energy = GuiTex(GUI_MAP_ENERGY);
    n->icons.locked = GuiTex(GUI_NODE_LOCKED);
    n->icons.locked_selected = GuiTex(GUI_NODE_LOCKED_ON);
    n->icons.open = GuiTex(GUI_NODE_OPEN);
    n->icons.open_selected = GuiTex(GUI_NODE_OPEN_ON);
    n->icons.mastered = GuiTex(GUI_NODE_MASTERED);
    n->icons.mastered_selected = GuiTex(GUI_NODE_MASTERED_ON);
    n->icons.boss = GuiTex(GUI_NODE_BOSS);
    n->icons.boss_selected = GuiTex(GUI_NODE_BOSS_ON);
    n->icons.boss_locked = GuiTex(GUI_NODE_BOSS_LOCKED);
    n->icons.boss_locked_selected = GuiTex(GUI_NODE_BOSS_LOCKED_ON);

    n->retreat = ButtonLoadText(
        GUI_BTN_GREY,
        GUI_BTN_GREY_ON,
        LocGetOr("NEXUS_RETREAT_FROM_MAP", "Выйти с карты"),
        n->font,
        (Vector2){0.5f, 0.5f},
        0.3f
    );
    n->enter = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGetOr("NEXUS_ENTER_NODE", "Войти"), n->font, (Vector2){0.5f, 0.5f}, 0.32f
    );
    ButtonSetClickSound(&n->retreat, SND_UI_HOME);
    n->quit_stay = ButtonLoadText(
        GUI_BTN_GOLD,
        GUI_BTN_GOLD_ON,
        LocGetOr("NEXUS_QUIT_SCREEN_STAY_BUTTON", "Отмена"),
        n->font,
        (Vector2){0.5f, 0.5f},
        0.32f
    );
    n->quit_confirm = ButtonLoadText(
        GUI_BTN_GREY,
        GUI_BTN_GREY_ON,
        LocGetOr("NEXUS_QUIT_SCREEN_RETREAT_BUTTON", "Выйти с карты"),
        n->font,
        (Vector2){0.5f, 0.5f},
        0.3f
    );

    cJSON *root = JsonLoadFile(maps_path);
    if (!cJSON_IsArray(root)) TraceLog(LOG_FATAL, "Nexus: %s is not an array of events", maps_path);

    cJSON *event_json = NULL;
    cJSON_ArrayForEach(event_json, root) {
        if (n->event_count >= NEXUS_MAX_EVENTS)
            TraceLog(LOG_FATAL, "Nexus: %s has more than %d events", maps_path, NEXUS_MAX_EVENTS);
        NexusEvent *e = &n->events[n->event_count];
        StrCopy(e->id, sizeof(e->id), JsonStr(event_json, "id"));
        StrCopy(e->name, sizeof(e->name), JsonStr(event_json, "name"));
        StrCopy(e->kind, sizeof(e->kind), JsonStr(event_json, "kind"));
        StrCopy(e->prefab, sizeof(e->prefab), JsonStr(event_json, "prefab"));
        StrCopy(e->subtitle, sizeof(e->subtitle), JsonStr(event_json, "subtitle"));
        StrCopy(e->hub_image, sizeof(e->hub_image), JsonStr(event_json, "hub_image"));
        e->weekday = JsonInt(event_json, "weekday");
        e->unlock_key = JsonInt(event_json, "unlock_key");
        e->unlock_key_qty = JsonInt(event_json, "unlock_key_qty");
        e->max_runs = JsonInt(event_json, "max_runs");
        e->duration = JsonInt(event_json, "duration");
        e->unlock_duration = JsonInt(event_json, "unlock_duration");
        e->node_count = JsonInt(event_json, "node_count");

        cJSON *colour = cJSON_GetObjectItemCaseSensitive(event_json, "highlight_color");
        e->highlight = WHITE;
        if (cJSON_IsArray(colour) && cJSON_GetArraySize(colour) >= 3) {
            e->highlight = (Color){
                (unsigned char)cJSON_GetArrayItem(colour, 0)->valueint,
                (unsigned char)cJSON_GetArrayItem(colour, 1)->valueint,
                (unsigned char)cJSON_GetArrayItem(colour, 2)->valueint,
                255,
            };
        }

        NexusNode layout[NEXUS_MAX_NODES] = {0};
        int layout_count = 0;
        cJSON *position = NULL;
        cJSON_ArrayForEach(position, cJSON_GetObjectItemCaseSensitive(event_json, "positions")) {
            if (layout_count >= NEXUS_MAX_NODES) break;
            layout[layout_count].step = JsonInt(position, "step");
            layout[layout_count].x = JsonFloat(position, "x");
            layout[layout_count].z = JsonFloat(position, "z");
            layout_count++;
        }

        cJSON *difficulty_json = NULL;
        cJSON_ArrayForEach(difficulty_json, cJSON_GetObjectItemCaseSensitive(event_json, "difficulties")) {
            if (e->difficulty_count >= NEXUS_MAX_DIFFICULTIES)
                TraceLog(LOG_FATAL, "Nexus: %s has more than %d difficulties", e->id, NEXUS_MAX_DIFFICULTIES);
            NexusDifficulty *d = &e->difficulties[e->difficulty_count];
            StrCopy(d->id, sizeof(d->id), JsonStr(difficulty_json, "id"));
            StrCopy(d->difficulty, sizeof(d->difficulty), JsonStr(difficulty_json, "difficulty"));
            StrCopy(
                d->battle_environment, sizeof(d->battle_environment), JsonStr(difficulty_json, "battle_environment")
            );
            d->energy_cost = JsonInt(difficulty_json, "energy_cost");
            d->potion_cost = JsonInt(difficulty_json, "potion_cost");

            NexusNode local[NEXUS_MAX_NODES];
            int local_count = 0;
            cJSON *override_positions = cJSON_GetObjectItemCaseSensitive(difficulty_json, "positions");
            if (cJSON_IsArray(override_positions)) {
                cJSON *p = NULL;
                cJSON_ArrayForEach(p, override_positions) {
                    if (local_count >= NEXUS_MAX_NODES) break;
                    local[local_count].step = JsonInt(p, "step");
                    local[local_count].x = JsonFloat(p, "x");
                    local[local_count].z = JsonFloat(p, "z");
                    local_count++;
                }
            } else {
                memcpy(local, layout, sizeof(local));
                local_count = layout_count;
            }

            cJSON *node_json = NULL;
            cJSON_ArrayForEach(node_json, cJSON_GetObjectItemCaseSensitive(difficulty_json, "nodes")) {
                if (d->node_count >= NEXUS_MAX_NODES)
                    TraceLog(LOG_FATAL, "Nexus: %s has more than %d nodes", d->id, NEXUS_MAX_NODES);
                NexusNode *node = &d->nodes[d->node_count];
                node->step = JsonInt(node_json, "step");
                node->gold = JsonInt(node_json, "gold");
                node->xp = JsonInt(node_json, "xp");
                node->boss = JsonBool(node_json, "boss");
                node->potion_reward = JsonInt(node_json, "potion_reward");
                cJSON *rarity = NULL;
                cJSON_ArrayForEach(rarity, cJSON_GetObjectItemCaseSensitive(node_json, "reward_rarity")) {
                    if (node->reward_rarity_count >= 6) break;
                    node->reward_rarity[node->reward_rarity_count++] = rarity->valueint;
                }
                for (int i = 0; i < local_count; i++) {
                    if (local[i].step != node->step) continue;
                    node->x = local[i].x;
                    node->z = local[i].z;
                    break;
                }
                d->node_count++;
            }
            if (d->node_count == 0) TraceLog(LOG_FATAL, "Nexus: %s has no nodes", d->id);
            e->difficulty_count++;
        }
        if (e->difficulty_count == 0) TraceLog(LOG_FATAL, "Nexus: %s has no difficulties", e->id);
        n->event_count++;
    }
    cJSON_Delete(root);
    NexusGameChangersLoad(n, NEXUS_GAME_CHANGERS_PATH);
    if (n->event_count == 0) TraceLog(LOG_FATAL, "Nexus: no events in %s", maps_path);
    TraceLog(LOG_INFO, "Nexus: loaded %d events from %s", n->event_count, maps_path);
}

void NexusUnload(Nexus *n) {
    if (n->bg_loaded) NexusBackgroundUnload(&n->bg);
    n->bg_loaded = false;
}

int NexusEventCount(const Nexus *n) {
    return n->event_count;
}

const BattleGameChanger *NexusEventGameChanger(const Nexus *n, int event) {
    if (event < 0 || event >= n->event_count || n->game_changers[event].effect_count == 0) return NULL;
    return &n->game_changers[event];
}

const char *NexusGameChangerText(const BattleGameChanger *changer) {
    return changer->text_key[0] ? LocGet(changer->text_key) : changer->text_ru;
}

const NexusEvent *NexusEventAt(const Nexus *n, int index) {
    return index >= 0 && index < n->event_count ? &n->events[index] : NULL;
}

const NexusDifficulty *NexusDifficultyAt(const Nexus *n, int event, int difficulty) {
    const NexusEvent *e = NexusEventAt(n, event);
    if (!e || difficulty < 0 || difficulty >= e->difficulty_count) return NULL;
    return &e->difficulties[difficulty];
}

const NexusDifficulty *NexusActiveDifficulty(const Nexus *n) {
    return NexusDifficultyAt(n, n->active_event, n->active_difficulty);
}

bool NexusRunComplete(const Nexus *n, const PlayerNexusRun *run) {
    const NexusDifficulty *d = NexusDifficultyAt(n, run->event_index, run->difficulty_index);
    if (!d || !run->committed) return false;
    return run->current_node_step == d->node_count && run->nodes[d->node_count - 1].wins > 0;
}

const char *NexusDifficultyTitle(const NexusDifficulty *d) {
    static const struct {
        const char *slug, *key, *fallback;
    } titles[] = {
        {"easy", "Event_Nexus_Easy_Title", "Легко"},
        {"medium", "Event_Nexus_Medium_Title", "Умеренно"},
        {"hard", "Event_Nexus_Hard_Title", "Сложно"},
        {"veryhard", "Event_Nexus_VERYHard_Title", "Очень сложно"},
        {"insane", "Event_Nexus_Insane_Title", "Безумие"},
        {"ludicrous", "Event_Nexus_Ludicrous_Title", "Абсурд"},
    };
    if (!d) return "";
    for (int i = 0; i < (int)(sizeof(titles) / sizeof(titles[0])); i++) {
        if (strcmp(titles[i].slug, d->difficulty) == 0) return LocGetOr(titles[i].key, titles[i].fallback);
    }
    return d->difficulty;
}

bool NexusEventLive(const NexusEvent *e, int weekday_today) {
    return !e || e->weekday == 0 || e->weekday == weekday_today;
}

int NexusWeekdayToday(void) {
    time_t now = (time_t)ServerNow();
    struct tm *local = localtime(&now);
    if (!local) return 1;
    return local->tm_wday == 0 ? 7 : local->tm_wday;
}

bool NexusEventCommitted(const Player *player, int event) {
    return player->nexus_run.committed && player->nexus_run.event_index == event;
}

bool NexusEventTimeGated(const NexusEvent *e) {
    return e && e->unlock_duration > 0;
}

bool NexusEventRunGated(const NexusEvent *e) {
    return e && e->max_runs > 0;
}

bool NexusEventGated(const NexusEvent *e) {
    return NexusEventTimeGated(e) || NexusEventRunGated(e);
}

bool NexusEventLocked(const Nexus *n, const Player *player, int event, long long now) {
    const NexusEvent *e = NexusEventAt(n, event);
    if (!e) return true;
    long long unlocked_at = player->nexus_events.unlocked_at[event];
    if (NexusEventTimeGated(e) && (unlocked_at == 0 || now - unlocked_at > e->unlock_duration)) return true;
    if (NexusEventRunGated(e) && player->nexus_events.remaining_runs[event] <= 0 && !NexusEventCommitted(player, event))
        return true;
    return false;
}

static long long NexusSecondsToLocalMidnight(long long now) {
    time_t t = (time_t)now;
    struct tm *local = localtime(&t);
    if (!local) return 0;
    long long elapsed = (long long)local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec;
    return NEXUS_SECONDS_PER_DAY - elapsed;
}

static long long NexusEventWindowRemaining(const NexusEvent *e, long long now) {
    if (e->weekday != 0) return NexusSecondsToLocalMidnight(now);
    if (e->duration <= 0) return 0;
    return e->duration - now % e->duration;
}

long long NexusEventWindowStart(const NexusEvent *e, long long now) {
    long long length = e->weekday != 0 ? NEXUS_SECONDS_PER_DAY : e->duration;
    return now + NexusEventWindowRemaining(e, now) - length;
}

long long NexusEventUnlockRemaining(const Nexus *n, const Player *player, int event, long long now) {
    const NexusEvent *e = NexusEventAt(n, event);
    if (!NexusEventTimeGated(e) || NexusEventLocked(n, player, event, now)) return 0;
    return player->nexus_events.unlocked_at[event] + e->unlock_duration - now;
}

long long NexusEventTimeRemaining(const Nexus *n, const Player *player, int event, long long now) {
    const NexusEvent *e = NexusEventAt(n, event);
    if (!e) return 0;
    long long window = NexusEventWindowRemaining(e, now);
    long long unlock = NexusEventUnlockRemaining(n, player, event, now);
    long long remaining = window > unlock ? window : unlock;
    return remaining > 0 ? remaining : 0;
}

bool NexusEventUnlocked(const Nexus *n, const Player *player, int event) {
    const NexusEvent *e = NexusEventAt(n, event);
    if (!e || !NexusEventLive(e, NexusWeekdayToday())) return false;
    return !NexusEventLocked(n, player, event, ServerNow());
}

bool NexusEventScheduled(const Nexus *n, const Player *player, int event, long long now) {
    const NexusEvent *e = NexusEventAt(n, event);
    if (!e || !NexusEventLive(e, NexusWeekdayToday())) return false;
    if (e->weekday != 0 || e->unlock_key != 0 || NexusEventCommitted(player, event) || e->duration <= 0) return true;
    int pool = 0, position = 0;
    for (int i = 0; i < n->event_count; i++) {
        if (n->events[i].weekday != 0 || n->events[i].unlock_key != 0) continue;
        if (i == event) position = pool;
        pool++;
    }
    int offset = (int)((now / e->duration * NEXUS_ROTATION_VISIBLE_EVENTS) % pool);
    return (position - offset + pool) % pool < NEXUS_ROTATION_VISIBLE_EVENTS;
}

bool NexusRunExpired(const Nexus *n, const Player *player, long long now) {
    const PlayerNexusRun *run = &player->nexus_run;
    const NexusEvent *e = NexusEventAt(n, run->event_index);
    if (!run->committed || !e || NexusRunComplete(n, run)) return false;
    return !NexusEventLive(e, NexusWeekdayToday()) || NexusEventLocked(n, player, run->event_index, now);
}

bool NexusRunBlocking(const Nexus *n, const Player *player) {
    const PlayerNexusRun *run = &player->nexus_run;
    if (!run->committed) return false;
    return NexusEventLive(NexusEventAt(n, run->event_index), NexusWeekdayToday());
}

static float NexusNodeScrollX(const Nexus *n, const NexusNode *node) {
    return NexusBackgroundScrollForNode(&n->bg, node->x);
}

static void NexusScrollLimits(const Nexus *n, const NexusDifficulty *d, float *out_lo, float *out_hi) {
    float lo = 0.0f, hi = 0.0f;
    NexusBackgroundArtScrollLimits(&n->bg, &lo, &hi);
    float node_lo = NexusNodeScrollX(n, &d->nodes[0]), node_hi = node_lo;
    for (int i = 1; i < d->node_count; i++) {
        float x = NexusNodeScrollX(n, &d->nodes[i]);
        node_lo = fminf(node_lo, x);
        node_hi = fmaxf(node_hi, x);
    }
    node_lo -= NEXUS_EDGE_BUFFER;
    node_hi += NEXUS_EDGE_BUFFER;
    if (node_hi < lo || node_lo > hi) {
        *out_lo = node_lo;
        *out_hi = node_hi;
        return;
    }
    *out_lo = fmaxf(lo, node_lo);
    *out_hi = fminf(hi, node_hi);
    if (*out_lo > *out_hi) *out_lo = *out_hi = (fmaxf(lo, node_lo) + fminf(hi, node_hi)) * 0.5f;
}

static void NexusFocusNode(Nexus *n, const NexusDifficulty *d, int index, bool immediate) {
    n->selected_node = Clamp(index, 0, d->node_count - 1);
    float lo = 0.0f, hi = 0.0f;
    NexusScrollLimits(n, d, &lo, &hi);
    n->focus_to = Clamp(NexusNodeScrollX(n, &d->nodes[n->selected_node]), lo, hi);
    n->scroll_velocity = 0.0f;
    n->drift_time = 0.0f;
    if (immediate) {
        n->scroll_x = n->focus_to;
        n->focusing = false;
    } else {
        n->focus_from = n->scroll_x;
        n->focus_time = 0.0f;
        n->focusing = true;
    }
}

bool NexusEnterMap(Nexus *n, Player *player, int event, int difficulty) {
    const NexusDifficulty *d = NexusDifficultyAt(n, event, difficulty);
    const NexusEvent *e = NexusEventAt(n, event);
    if (!d || !e) return false;

    PlayerNexusRun *run = &player->nexus_run;
    bool resuming = run->committed && strcmp(run->map_id, d->id) == 0;
    if (run->committed && !resuming && NexusRunBlocking(n, player)) {
        TraceLog(LOG_WARNING, "Nexus: %s is committed; refusing to open %s", run->map_id, d->id);
        return false;
    }

    if (n->bg_loaded && strcmp(n->bg_prefab, e->prefab) != 0) {
        NexusBackgroundUnload(&n->bg);
        n->bg_loaded = false;
    }
    if (!n->bg_loaded) {
        NexusBackgroundLoad(&n->bg, HOD_CACHE "meshes", e->prefab);
        StrCopy(n->bg_prefab, sizeof(n->bg_prefab), e->prefab);
        n->bg_loaded = true;
    }

    if (!resuming) {
        *run = (PlayerNexusRun){0};
        StrCopy(run->map_id, sizeof(run->map_id), d->id);
        run->current_node_step = 1;
        run->selected_node_step = 1;
    }
    run->event_index = event;
    run->difficulty_index = difficulty;

    n->active_event = event;
    n->active_difficulty = difficulty;
    n->final_complete = NexusRunComplete(n, run);
    n->complete_time = 0.0f;
    n->complete_sound_played = false;
    n->recap_requested = false;
    n->anim_active = false;
    n->dragged = false;
    NexusFocusNode(n, d, run->current_node_step - 1, true);
    return true;
}

int NexusSelectedEnergyCost(const Nexus *n, const Player *player) {
    const NexusDifficulty *d = NexusActiveDifficulty(n);
    if (!d) return 0;
    const PlayerNexusRun *run = &player->nexus_run;
    if (n->selected_node != 0) return 0;
    return run->nodes[0].losses > 0 ? 0 : d->energy_cost;
}

bool NexusMapOpen(const Nexus *n) {
    return n->bg_loaded && NexusActiveDifficulty(n) != NULL;
}

void NexusBeginProgressionAnimation(Nexus *n, int from_step, int to_step) {
    const NexusDifficulty *d = NexusActiveDifficulty(n);
    if (!d) return;
    n->anim_active = true;
    n->anim_time = 0.0f;
    n->anim_from = Clamp(from_step - 1, 0, d->node_count - 1);
    n->anim_to = Clamp(to_step - 1, 0, d->node_count - 1);
    n->anim_sound_played = false;
    GuiSoundPlay(SND_PVE_NODE_TRANSITION);
    NexusFocusNode(n, d, n->anim_from, true);
    n->selected_node = n->anim_to;
}

void NexusBeginCompletion(Nexus *n) {
    n->final_complete = true;
    n->complete_time = 0.0f;
    n->complete_sound_played = false;
    n->recap_requested = false;
    n->anim_active = false;
}

bool NexusCompletionActive(const Nexus *n) {
    return n->final_complete;
}

static float NexusCompleteBannerDelay(const PlayerNexusRun *run) {
    int units = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC; rarity++) units += run->escrow_units[rarity];
    int items = units > run->escrow_potions ? units : run->escrow_potions;
    return NEXUS_COMPLETE_BANNER_DELAY + (float)items * NEXUS_COMPLETE_LOOT_DELAY_PER_ITEM;
}

static void NexusPlayCompleteSound(Nexus *n) {
    if (n->complete_sound_played) return;
    n->complete_sound_played = true;
    GuiSoundPlay(SND_LEVEL_UP);
}

static NexusAction NexusRequestRecap(Nexus *n, const PlayerNexusRun *run) {
    n->completed_run = *run;
    n->recap_requested = true;
    return NEXUS_SHOW_RECAP;
}

static void NexusDrawCompleteBanner(const Nexus *n, float elapsed, float screen_w, float screen_h) {
    float fade = Clamp(elapsed / NEXUS_COMPLETE_BANNER_FADE_TIME, 0.0f, 1.0f);
    float width = screen_w * 0.92f, height = width * 0.25f, y = screen_h * 0.435f;
    Rectangle banner = {(screen_w - width) * 0.5f, y - height * 0.5f, width, height};
    UiDrawTexture(GuiTex(GUI_SPLASH_INKSMEAR), banner, (Color){0, 0, 0, (unsigned char)(255.0f * fade)});
    UiDrawTexture(
        GuiTex(GUI_SPLASH_INKSPLAT),
        (Rectangle){banner.x - width * 0.02f, y - height * 0.6f, width * 1.04f, height * 1.2f},
        (Color){0, 0, 0, (unsigned char)(205.0f * fade)}
    );
    UiDrawTextCentered(
        n->font,
        LocGetOr("NEXUS_MAP_COMPLETE", "Завершено"),
        banner,
        60.0f,
        (Color){240, 240, 240, (unsigned char)(255.0f * fade)}
    );
}

static Texture2D NexusNodeIcon(const Nexus *n, const NexusNode *node, int current_step, bool selected) {
    if (node->step < current_step) return selected ? n->icons.mastered_selected : n->icons.mastered;
    if (node->step > current_step) {
        if (node->boss) return selected ? n->icons.boss_locked_selected : n->icons.boss_locked;
        return selected ? n->icons.locked_selected : n->icons.locked;
    }
    if (node->boss) return selected ? n->icons.boss_selected : n->icons.boss;
    return selected ? n->icons.open_selected : n->icons.open;
}

static void NexusDrawNodeQuad(Texture2D tex, Vector3 centre, float half_x, float half_z, Color tint) {
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(centre.x + half_x, centre.y, centre.z + half_z);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(centre.x + half_x, centre.y, centre.z - half_z);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(centre.x - half_x, centre.y, centre.z - half_z);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(centre.x - half_x, centre.y, centre.z + half_z);
    rlEnd();
    rlSetTexture(0);
}

static void
NexusDrawSelectionRing(Texture2D tex, Vector3 centre, float half_x, float half_z, float degrees, Color tint) {
    float c = cosf(degrees * DEG2RAD), s = sinf(degrees * DEG2RAD);
    static const float corners[4][4] = {{1, 1, 0, 0}, {1, -1, 0, 1}, {-1, -1, 1, 1}, {-1, 1, 1, 0}};
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    for (int i = 0; i < 4; i++) {
        float x = corners[i][0] * c - corners[i][1] * s, z = corners[i][0] * s + corners[i][1] * c;
        rlTexCoord2f(corners[i][2], corners[i][3]);
        rlVertex3f(centre.x + x * half_x, centre.y, centre.z + z * half_z);
    }
    rlEnd();
    rlSetTexture(0);
}

static Vector3 NexusNodeOffset(Vector3 world, Vector2 unit, float x, float z) {
    return (Vector3){world.x + x * unit.x, world.y, world.z - z * unit.y};
}

static Rectangle NexusNodeScreenRect(const Nexus *n, Vector3 world, Vector2 unit, Vector2 centre, Vector2 half) {
    Vector2 a = NexusBackgroundNodeScreen(&n->bg, NexusNodeOffset(world, unit, centre.x + half.x, centre.y - half.y));
    Vector2 b = NexusBackgroundNodeScreen(&n->bg, NexusNodeOffset(world, unit, centre.x - half.x, centre.y + half.y));
    return (Rectangle){fminf(a.x, b.x), fminf(a.y, b.y), fabsf(b.x - a.x), fabsf(b.y - a.y)};
}

static bool NexusDrawQuitDialog(Nexus *n, const PlayerNexusRun *run) {
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) n->quit_opening_release = false;
    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    Font body = GuiFont(FNT_MENU);
    UiDrawTextCentered(
        n->font,
        LocGetOr("NEXUS_QUIT_SCREEN_TITLE", "Вы уверены?"),
        (Rectangle){content.x, content.y, content.width, 48.0f},
        34.0f,
        (Color){235, 232, 220, 255}
    );
    bool loot = run->escrow_gold > 0 || run->escrow_xp > 0 || run->escrow_potions > 0;
    HeroesDrawRich(
        body,
        LocGet(loot ? "NEXUS_QUIT_SCREEN_BODY" : "NEXUS_QUIT_LOSE_ENERGY_BODY"),
        (Rectangle){content.x, content.y + 64.0f, content.width, content.height - 140.0f},
        24.0f,
        (Color){235, 232, 220, 255}
    );
    Rectangle row = MessagePopupChoiceRow(popup);
    float width = (row.width - NEXUS_QUIT_BUTTON_GAP) * 0.5f;
    Rectangle left = {row.x, row.y, width, row.height};
    Rectangle right = {row.x + width + NEXUS_QUIT_BUTTON_GAP, row.y, width, row.height};
    bool interactive = !n->quit_opening_release && !UiInputBlocked();
    bool quit = ButtonDrawEx(&n->quit_confirm, left, interactive);
    if (ButtonDrawEx(&n->quit_stay, right, interactive)) n->quit_open = false;
    if (quit) n->quit_open = false;
    return quit;
}

bool NexusHandleBack(Nexus *n) {
    if (!n->quit_open) return false;
    n->quit_open = false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    return true;
}

static void NexusUpdateInput(Nexus *n, const NexusDifficulty *d, float dt) {
    float lo = 0.0f, hi = 0.0f;
    NexusScrollLimits(n, d, &lo, &hi);

    if (n->focusing) {
        n->focus_time += dt;
        float t = Clamp(n->focus_time / NEXUS_FOCUS_TIME, 0.0f, 1.0f);
        n->scroll_x = Lerp(n->focus_from, n->focus_to, 1.0f - powf(2.0f, -10.0f * t));
        if (t >= 1.0f) {
            n->scroll_x = n->focus_to;
            n->focusing = false;
        }
    }

    bool blocked = UiInputBlocked() || n->anim_active || n->final_complete || n->quit_open;
    if (!blocked && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        n->press_pos = UiMouse();
        n->dragged = false;
        n->focusing = false;
        n->scroll_velocity = 0.0f;
    }
    if (!blocked && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float delta = UiMouseDelta().x / UiWidth() * NEXUS_DRAG_GAIN;
        n->scroll_x += delta;
        n->scroll_velocity = Lerp(n->scroll_velocity, delta, 0.5f);
        if (fabsf(UiMouse().x - n->press_pos.x) > NEXUS_TAP_SLOP) n->dragged = true;
    }
    if (!blocked && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && n->dragged) n->drift_time = NEXUS_DRIFT_MAX_TIME;
    if (!blocked) n->scroll_x += GetMouseWheelMove() * NEXUS_WHEEL_STEP * NEXUS_DRAG_GAIN;

    if (n->drift_time > 0.0f) {
        n->scroll_x += n->scroll_velocity;
        n->drift_time -= dt;
        n->scroll_velocity = Lerp(n->scroll_velocity, 0.0f, Clamp(dt / NEXUS_DRIFT_MAX_TIME, 0.0f, 1.0f));
        if (n->drift_time <= 0.0f) n->scroll_velocity = 0.0f;
    }
    n->scroll_x = Clamp(n->scroll_x, lo, hi);
}

NexusAction NexusDraw(Nexus *n, Player *player, PlayerNexusRun *run) {
    NexusAction action = NEXUS_NONE;
    const NexusEvent *e = NexusEventAt(n, n->active_event);
    const NexusDifficulty *d = NexusActiveDifficulty(n);
    if (!e || !d || !n->bg_loaded) return action;
    float dt = GetFrameTime();
    float screen_w = UiWidth(), screen_h = UiHeight();

    if (n->selected_node >= d->node_count) n->selected_node = d->node_count - 1;
    if (n->recap_requested) run = &n->completed_run;
    if (NexusRunComplete(n, run)) n->final_complete = true;
    int current_step = Clamp(run->current_node_step, 1, d->node_count);

    if (n->anim_active) {
        n->anim_time += dt;
        if (!n->anim_sound_played && n->anim_time >= 2.0f && n->anim_to != n->anim_from) {
            GuiSoundPlay(SND_PVE_NODE_TRANSITION);
            n->anim_sound_played = true;
        }
        float from_x = NexusNodeScrollX(n, &d->nodes[n->anim_from]);
        float to_x = NexusNodeScrollX(n, &d->nodes[n->anim_to]);
        float lo = 0.0f, hi = 0.0f;
        NexusScrollLimits(n, d, &lo, &hi);
        n->scroll_x = Lerp(Clamp(from_x, lo, hi), Clamp(to_x, lo, hi), NexusEaseInOutQuad(n->anim_time - 1.0f));
        if (n->anim_time >= 3.0f) n->anim_active = false;
    } else {
        NexusUpdateInput(n, d, dt);
    }
    if (n->final_complete) n->complete_time += dt;
    NexusBackgroundSetScroll(&n->bg, n->scroll_x);

    float plane_y = NexusBackgroundNodePlaneY(&n->bg);
    Vector2 unit = NexusBackgroundNodeUnit(&n->bg);
    n->ring_angle = fmodf(n->ring_angle + dt * NEXUS_RING_DEGREES_PER_SECOND, 360.0f);

    ClearBackground(n->bg.clear);
    BeginMode3D(n->bg.cam);
    rlDisableDepthTest();
    NexusBackgroundDrawRange(&n->bg, -1.0e9f, plane_y);

    Vector2 node_screen[NEXUS_MAX_NODES] = {0};
    Vector3 node_world[NEXUS_MAX_NODES] = {0};
    for (int i = 0; i < d->node_count; i++) {
        const NexusNode *node = &d->nodes[i];
        int step_for_state = current_step;
        float flip = 1.0f, pulse = -1.0f;
        if (n->anim_active && i == n->anim_from) {
            flip = NexusFlipScale(n->anim_time);
            pulse = n->anim_time;
            if (n->anim_time < 0.5f) step_for_state = n->anim_from + 1;
        } else if (n->anim_active && i == n->anim_to && n->anim_to != n->anim_from) {
            float phase = n->anim_time - 2.0f;
            flip = NexusFlipScale(phase);
            pulse = phase;
            if (phase < 0.5f) step_for_state = n->anim_from + 1;
        }

        Texture2D icon = NexusNodeIcon(n, node, step_for_state, i == n->selected_node);
        float scale = node->boss ? NEXUS_BOSS_SCALE : 1.0f;
        float half_x = NEXUS_ICON_HALF_WIDTH * unit.x * scale;
        float half_z = NEXUS_ICON_HALF_HEIGHT * unit.y * scale;
        Vector3 world = NexusBackgroundNodeWorld(&n->bg, node->x, node->z);
        node_world[i] = world;
        node_screen[i] = NexusBackgroundNodeScreen(&n->bg, world);

        if (pulse >= 0.0f && pulse <= 1.0f) {
            float glow = sinf(pulse * PI);
            float size = half_z * (1.0f + 0.8f * glow);
            BeginBlendMode(BLEND_ADDITIVE);
            NexusDrawNodeQuad(n->glow, world, size, size, (Color){246, 216, 111, (unsigned char)(175.0f * glow)});
            rlDrawRenderBatchActive();
            EndBlendMode();
        }
        if (i == n->selected_node && !n->final_complete) {
            float ring_scale = NEXUS_RING_HALF_SIZE * scale;
            float ring_z = node->boss ? NEXUS_RING_BOSS_OFFSET_Z : NEXUS_RING_OFFSET_Z;
            BeginBlendMode(BLEND_ADDITIVE);
            NexusDrawSelectionRing(
                n->ring,
                NexusNodeOffset(world, unit, 0.0f, ring_z),
                ring_scale * unit.x,
                ring_scale * unit.y,
                n->ring_angle,
                e->highlight
            );
            rlDrawRenderBatchActive();
            EndBlendMode();
        }
        NexusDrawNodeQuad(
            icon, NexusNodeOffset(world, unit, 0.0f, NEXUS_ICON_OFFSET_Z), half_x * flip, half_z, e->highlight
        );
    }
    rlDrawRenderBatchActive();

    NexusBackgroundDrawRange(&n->bg, plane_y, 1.0e9f);
    rlEnableDepthTest();
    EndMode3D();

    UiFrameBegin();

    bool interactive = !UiInputBlocked() && !n->anim_active && !n->final_complete && !n->quit_open;
    if (interactive && !n->dragged && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = UiMouse();
        for (int i = 0; i < d->node_count; i++) {
            float scale = d->nodes[i].boss ? NEXUS_BOSS_SCALE : 1.0f;
            Rectangle hit = NexusNodeScreenRect(
                n,
                node_world[i],
                unit,
                (Vector2){0.0f, NEXUS_ICON_OFFSET_Z},
                (Vector2){NEXUS_ICON_HALF_WIDTH * scale, NEXUS_ICON_HALF_HEIGHT * scale}
            );
            if (!CheckCollisionPointRec(mouse, hit)) continue;
            GuiSoundPlay(SND_PVE_NODE_SELECTED);
            bool refight = i == n->selected_node && i == current_step - 1;
            NexusFocusNode(n, d, i, false);
            if (refight) action = run->reset ? NEXUS_REVIVE : NEXUS_ENTER;
            break;
        }
    }

    if (d->energy_cost > 0 && !run->committed) {
        Rectangle banner = NexusNodeScreenRect(
            n,
            node_world[0],
            unit,
            (Vector2){0.0f, NEXUS_BANNER_OFFSET_Z},
            (Vector2){NEXUS_BANNER_HALF_WIDTH, NEXUS_BANNER_HALF_HEIGHT}
        );
        UiDrawTexture(n->energy_plate, banner, WHITE);
        Rectangle flask = NexusNodeScreenRect(
            n,
            node_world[0],
            unit,
            (Vector2){NEXUS_FLASK_OFFSET_X, NEXUS_FLASK_OFFSET_Z},
            (Vector2){NEXUS_FLASK_HALF_WIDTH, NEXUS_FLASK_HALF_HEIGHT}
        );
        UiDrawTexture(n->energy_flask, flask, WHITE);
        const char *text = TextFormat("%d", d->energy_cost);
        float fs = banner.height * NEXUS_ENERGY_TEXT_OF_BANNER;
        Vector2 measured = MeasureTextEx(n->font, text, fs, 1.0f);
        Vector2 anchor = NexusBackgroundNodeScreen(
            &n->bg, NexusNodeOffset(node_world[0], unit, NEXUS_ENERGY_TEXT_OFFSET_X, NEXUS_BANNER_OFFSET_Z)
        );
        DrawTextEx(
            n->font, text, (Vector2){anchor.x - measured.x / 2.0f, anchor.y - measured.y / 2.0f}, fs, 1.0f, WHITE
        );
    }

    float banner_delay = NexusCompleteBannerDelay(run);
    if (n->final_complete && n->complete_time >= banner_delay) {
        NexusPlayCompleteSound(n);
        if (!n->recap_requested) {
            NexusDrawCompleteBanner(n, n->complete_time - banner_delay, screen_w, screen_h);
            if (n->complete_time >= banner_delay + NEXUS_COMPLETE_BANNER_TIME) action = NexusRequestRecap(n, run);
        }
        UiFrameEnd();
        return action;
    }

    Rectangle lore = {0.0f, 72.0f, n->lore.width / 1.3f, n->lore.height / 1.3f};
    {
        Rectangle src = {2, 0, -(float)n->lore.width, (float)n->lore.height};
        DrawTexturePro(n->lore, src, lore, (Vector2){0, 0}, 0.0f, WHITE);
    }
    {
        float size = 22.0f, spacing = 1.0f;
        Vector2 measured = MeasureTextEx(n->font, e->name, size, spacing);
        float x = lore.x + 18.0f;
        if (measured.x > lore.width - 36.0f) size *= (lore.width - 36.0f) / measured.x;
        DrawTextEx(n->font, e->name, (Vector2){x, lore.y + 44.0f}, size, spacing, (Color){235, 232, 220, 255});
    }
    {
        Rectangle bar = {lore.x + 2.0f, lore.y + 84.0f, 364, 64};
        UiDrawTexture(n->progress_bar, bar, WHITE);
        int completed = run->current_node_step - 1;
        if (run->current_node_step == d->node_count && run->nodes[d->node_count - 1].wins > 0)
            completed = d->node_count;
        char label[64];
        LocFormat2(
            label,
            sizeof(label),
            "PVE_HUB_NEXUS_MAP_PROGRESS_BAR",
            TextFormat("%d", completed),
            TextFormat("%d", d->node_count)
        );
        if (!label[0]) snprintf(label, sizeof(label), "Пройд. (%d / %d)", completed, d->node_count);
        Vector2 measured = MeasureTextEx(n->font, label, 24.0f, 1.0f);
        DrawTextEx(
            n->font,
            label,
            (Vector2){bar.x + 40.0f, bar.y + (bar.height - measured.y) / 2.0f},
            24.0f,
            1.0f,
            (Color){240, 236, 224, 255}
        );
    }
    UiDrawTexture(n->divider, (Rectangle){lore.x + 24.0f, lore.y + 152.0f, lore.width - 60.0f, 10.0f}, e->highlight);

    {
        Font body = GuiFont(FNT_MENU);
        float y = lore.y + 190.0f;
        DrawTextEx(
            body,
            LocGetOr("NEXUS_MAP_LOOT", "Добыча:"),
            (Vector2){lore.x + 30.0f, y},
            20.0f,
            1.0f,
            (Color){228, 224, 212, 255}
        );
        y += 30.0f;
        if (run->escrow_gold == 0 && run->escrow_xp == 0) {
            CampaignDrawWrapped(
                body,
                LocGetOr("NEXUS_MAP_NO_LOOT_CTA", "Получайте добычу, побеждая в боях!"),
                (Vector2){lore.x + 30.0f, y},
                17.0f,
                1.0f,
                4.0f,
                lore.width - 60.0f,
                (Color){178, 178, 178, 255}
            );
        } else {
            Texture2D icons[2] = {n->gold_icon, n->xp_icon};
            int values[2] = {run->escrow_gold, run->escrow_xp};
            for (int i = 0; i < 2; i++) {
                UiDrawTexture(icons[i], (Rectangle){lore.x + 30.0f, y, 34.0f, 34.0f}, WHITE);
                DrawTextEx(
                    n->font,
                    TextFormat("+%d", values[i]),
                    (Vector2){lore.x + 74.0f, y + 4.0f},
                    24.0f,
                    1.0f,
                    (Color){245, 244, 238, 255}
                );
                y += 40.0f;
            }
        }
    }

    if (!n->final_complete) {
        Rectangle retreat = {7.0f, screen_h - 76.0f, 270.0f, 67.0f};
        if (ButtonDrawEx(&n->retreat, retreat, interactive)) {
            if (run->committed) {
                n->quit_open = true;
                n->quit_opening_release = true;
            } else {
                action = NEXUS_LEAVE;
            }
        }
    }
    {
        bool playable = n->selected_node == current_step - 1;
        Rectangle enter = {screen_w - 277.0f, screen_h - 76.0f, 270.0f, 67.0f};
        if (n->final_complete) {
            n->enter.label = LocGetOr("NEXUS_EXIT_MAP", "Выход с карты");
            if (ButtonDrawEx(&n->enter, enter, !UiInputBlocked())) {
                NexusPlayCompleteSound(n);
                action = NexusRequestRecap(n, run);
            }
        } else {
            n->enter.label =
                run->reset ? LocGetOr("NEXUS_REVIVE_TITLE", "Возродить") : LocGetOr("NEXUS_ENTER_NODE", "Войти");
            if (ButtonDrawEx(&n->enter, enter, interactive) && playable)
                action = run->reset ? NEXUS_REVIVE : NEXUS_ENTER;
        }
    }

    {
        float plate_w = 230.0f, plate_h = 52.0f, icon_size = 64.0f;
        Rectangle plate = {screen_w / 2.0f - plate_w / 2.0f, screen_h - 66.0f, plate_w, plate_h};
        UiDrawTexture(n->timer_plate, plate, WHITE);
        UiDrawTexture(
            n->potion_icon,
            (Rectangle){plate.x - 6.0f, plate.y + (plate_h - icon_size) / 2.0f, icon_size, icon_size},
            WHITE
        );
        const char *label = TextFormat("%d / %d", player->potions, player->potion_max);
        UiDrawTextCentered(
            n->font,
            label,
            (Rectangle){plate.x + 58.0f, plate.y, plate.width - 68.0f, plate_h},
            30.0f,
            (Color){245, 244, 238, 255}
        );
    }

    if (!n->final_complete) {
        const NexusNode *node = &d->nodes[n->selected_node];
        Texture2D icons[2] = {n->gold_icon, n->xp_icon};
        int values[2] = {node->gold, node->xp};
        float icon_box = 44.0f, row_h = 42.0f, tag_w = 96.0f, tag_h = 36.0f;
        float icon_x = screen_w - 44.0f - icon_box;
        float start_y = screen_h - 96.0f - 2 * row_h;
        for (int i = 0; i < 2; i++) {
            float cy = start_y + i * row_h + row_h / 2.0f;
            UiDrawTexture(n->reward_tag, (Rectangle){icon_x - tag_w, cy - tag_h / 2.0f, tag_w, tag_h}, WHITE);
            const char *text = TextFormat("+%d", values[i]);
            Vector2 measured = MeasureTextEx(n->font, text, 26.0f, 1.0f);
            DrawTextEx(
                n->font,
                text,
                (Vector2){icon_x - 6.0f - measured.x, cy - measured.y / 2.0f},
                26.0f,
                1.0f,
                (Color){245, 244, 238, 255}
            );
            UiDrawTexture(icons[i], (Rectangle){icon_x, cy - icon_box / 2.0f, icon_box, icon_box}, WHITE);
        }
    }

    if (n->quit_open && NexusDrawQuitDialog(n, run)) action = NEXUS_QUIT;

    UiFrameEnd();
    return action;
}
