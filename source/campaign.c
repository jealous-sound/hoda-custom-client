#define CAMPAIGN_MAP_LEFT 184
#define CAMPAIGN_OFFLINE_MASTERY_GEMS 1

typedef enum { CAMPAIGN_NONE = 0, CAMPAIGN_ENTER } CampaignAction;

typedef struct {
    char name[80];
    char desc[512];
    char cta[256];
    Vector2 position;
    int energy_cost;
    int gold, xp;
    int first_win_gold, first_win_xp;
    int mastery_gold, mastery_xp;
    int default_gems;
    int first_win_gems;
    int mastery_gems;
    int challenge_gold, challenge_xp, challenge_gems;
    int grind_gold, grind_xp, grind_gems;
    int wins_to_master;
    bool grind;
} CampaignNode;

typedef struct {
    char id[32];
    char title[64];
    Texture2D background;
    int node_count;
    CampaignNode nodes[CAMPAIGN_MAX_NODES];
} CampaignMap;

typedef struct {
    Texture2D energy;
    Texture2D locked, locked_selected;
    Texture2D open, open_selected;
    Texture2D done, done_selected;
    Texture2D mastered, mastered_selected;
    Texture2D boss, boss_selected;
    Texture2D boss_locked, boss_locked_selected;
    Texture2D grind, grind_selected;
} CampaignIcons;

typedef struct {
    Font font;
    CampaignIcons icons;
    int map_count;
    Texture2D separator;
    Texture2D lore;
    Texture2D progress_bar;
    Texture2D help;
    Texture2D gold;
    Texture2D xp;
    Texture2D gems;
    Texture2D reward_tag;
    Texture2D energy_flask;
    Button change_map;
    Button enter_map;
    float scroll_x;
    int scroll_map;
    int scroll_node;
    int selected_node;
    Vector2 press_pos;
    bool dragged;

    bool progression_active;
    bool progression_unlock_next;
    bool progression_camera_finished;
    float progression_time;
    bool progression_next_sound_played;
    int progression_from_map, progression_from_node;
    int progression_to_map, progression_to_node;
    CampaignMap maps[CAMPAIGN_MAX_MAPS];
} Campaign;

static int CampaignOfflineWinsToMaster(int story_node_index) {
    static const int historical_wins_to_master[] = {3, 5, 7, 10, 12};
    int count = (int)(sizeof(historical_wins_to_master) / sizeof(historical_wins_to_master[0]));
    if (story_node_index < 0) story_node_index = 0;
    return historical_wins_to_master[story_node_index % count];
}

typedef enum {
    CAMPAIGN_REWARD_REPEAT = 0,
    CAMPAIGN_REWARD_FIRST_WIN,
    CAMPAIGN_REWARD_MASTERY,
} CampaignRewardBucket;

typedef struct {
    int gold, xp, gems;
} CampaignRewards;

CampaignRewardBucket CampaignNodeRewardBucket(const CampaignNode *node, int wins) {
    if (!node) return CAMPAIGN_REWARD_REPEAT;
    if (wins <= 0) return CAMPAIGN_REWARD_FIRST_WIN;
    if (node->grind || node->wins_to_master <= 0) return CAMPAIGN_REWARD_REPEAT;
    return wins == node->wins_to_master - 1 ? CAMPAIGN_REWARD_MASTERY : CAMPAIGN_REWARD_REPEAT;
}

CampaignRewards CampaignNodeRewardsForWins(const CampaignNode *node, int wins) {
    CampaignRewards r = {0};
    if (!node) return r;
    r.gold = node->gold;
    r.xp = node->xp;
    r.gems = node->default_gems;
    CampaignRewardBucket bucket = CampaignNodeRewardBucket(node, wins);
    if (node->grind) {
        bool first = bucket == CAMPAIGN_REWARD_FIRST_WIN;
        r.gold += first ? node->challenge_gold : node->grind_gold;
        r.xp += first ? node->challenge_xp : node->grind_xp;
        r.gems += first ? node->challenge_gems : node->grind_gems;
    } else if (bucket == CAMPAIGN_REWARD_FIRST_WIN) {
        r.gold += node->first_win_gold;
        r.xp += node->first_win_xp;
        r.gems += node->first_win_gems;
    } else if (bucket == CAMPAIGN_REWARD_MASTERY) {
        r.gold += node->mastery_gold;
        r.xp += node->mastery_xp;
        r.gems += node->mastery_gems;
    }
    return r;
}

void CampaignNodeMasteryLabel(const CampaignNode *node, int wins, char *out, int cap) {
    if (cap <= 0) return;
    out[0] = '\0';
    if (!node || node->grind || node->wins_to_master <= 0) return;
    if (wins >= node->wins_to_master)
        StrCopy(out, cap, LocGet("PVE_NODE_MASTERED"));
    else if (wins <= 0)
        StrCopy(out, cap, LocGet("PVE_UNLOCKED"));
    else
        snprintf(out, (size_t)cap, "Закрыто (%d / %d)", wins, node->wins_to_master);
}

void CampaignMapsInitialize(Campaign *c, const char *path) {
    static const GuiId backgrounds[CAMPAIGN_MAX_MAPS] = {
        GUI_MAP_BG_0,
        GUI_MAP_BG_1,
        GUI_MAP_BG_2,
        GUI_MAP_BG_3,
        GUI_MAP_BG_4,
        GUI_MAP_BG_5,
        GUI_MAP_BG_6,
        GUI_MAP_BG_7,
        GUI_MAP_BG_8,
        GUI_MAP_BG_9,
        GUI_MAP_BG_10,
        GUI_MAP_BG_11,
        GUI_MAP_BG_12,
        GUI_MAP_BG_13,
        GUI_MAP_BG_14,
        GUI_MAP_BG_15,
    };

    cJSON *root = JsonLoadFile(path);
    if (!cJSON_IsArray(root)) TraceLog(LOG_FATAL, "Campaign: %s is not an array of chapters", path);

    c->map_count = 0;
    cJSON *chapter = NULL;
    cJSON_ArrayForEach(chapter, root) {
        const char *id = JsonStr(chapter, "id");

        if (c->map_count >= CAMPAIGN_MAX_MAPS)
            TraceLog(LOG_FATAL, "Campaign: %s has more than %d maps", path, CAMPAIGN_MAX_MAPS);
        CampaignMap *m = &c->maps[c->map_count];

        m->background = GuiTex(backgrounds[c->map_count]);
        if (m->background.id == 0) TraceLog(LOG_FATAL, "Campaign: %s has no panorama", id);
        StrCopy(m->id, sizeof(m->id), id);
        StrCopy(m->title, sizeof(m->title), JsonStr(chapter, "chapter"));

        m->node_count = 0;
        int authored_node_count = JsonInt(chapter, "node_count");
        cJSON *n = NULL;
        cJSON_ArrayForEach(n, cJSON_GetObjectItemCaseSensitive(chapter, "nodes")) {
            if (m->node_count >= CAMPAIGN_MAX_NODES)
                TraceLog(LOG_FATAL, "Campaign: %s has more than %d nodes", id, CAMPAIGN_MAX_NODES);
            CampaignNode *node = &m->nodes[m->node_count];
            StrCopy(node->name, sizeof(node->name), JsonStr(n, "name"));
            StrCopy(node->desc, sizeof(node->desc), JsonStr(n, "desc"));
            StrCopy(node->cta, sizeof(node->cta), JsonStr(n, "cta"));
            cJSON *position = cJSON_GetObjectItemCaseSensitive(n, "position");
            cJSON *energy = cJSON_GetObjectItemCaseSensitive(n, "energy_cost");
            if (!cJSON_IsNumber(energy))
                TraceLog(LOG_FATAL, "Campaign: %s node %d (%s) lacks energy data", id, m->node_count + 1, node->name);
            if (cJSON_IsObject(position)) {
                node->position = (Vector2){JsonFloat(position, "x"), JsonFloat(position, "y")};
            } else {
                float u = (float)(m->node_count + 1) / (float)(authored_node_count + 1);
                static const float lanes[] = {0.70f, 0.57f, 0.74f, 0.61f, 0.69f, 0.55f};
                node->position = (Vector2){0.04f + 0.92f * u, lanes[m->node_count % 6]};
            }
            node->energy_cost = energy->valueint;
            cJSON *default_gold = cJSON_GetObjectItemCaseSensitive(n, "default_gold");
            cJSON *default_xp = cJSON_GetObjectItemCaseSensitive(n, "default_xp");
            node->gold = cJSON_IsNumber(default_gold) ? default_gold->valueint : JsonInt(n, "gold");
            node->xp = cJSON_IsNumber(default_xp) ? default_xp->valueint : JsonInt(n, "xp");
            node->first_win_gold = JsonInt(n, "first_win_gold");
            node->first_win_xp = JsonInt(n, "first_win_xp");
            node->mastery_gold = JsonInt(n, "mastery_gold");
            node->mastery_xp = JsonInt(n, "mastery_xp");
            node->default_gems = JsonInt(n, "default_gems");
            cJSON *first_win_gems = cJSON_GetObjectItemCaseSensitive(n, "first_win_gems");
            node->first_win_gems = cJSON_IsNumber(first_win_gems) ? first_win_gems->valueint : JsonInt(n, "gems");
            node->challenge_gold = JsonInt(n, "challenge_gold");
            node->challenge_xp = JsonInt(n, "challenge_xp");
            node->challenge_gems = JsonInt(n, "challenge_gems");
            node->grind_gold = JsonInt(n, "grind_gold");
            node->grind_xp = JsonInt(n, "grind_xp");
            node->grind_gems = JsonInt(n, "grind_gems");
            node->grind = JsonBool(n, "grind");

            node->wins_to_master = JsonInt(n, "wins_to_master");
            cJSON *mastery_gems = cJSON_GetObjectItemCaseSensitive(n, "mastery_gems");
            node->mastery_gems =
                cJSON_IsNumber(mastery_gems) && mastery_gems->valueint > 0 ? mastery_gems->valueint : 0;
            if (node->grind) {
                node->wins_to_master = 0;
                node->mastery_gems = 0;
            } else {
                if (node->wins_to_master <= 0) node->wins_to_master = CampaignOfflineWinsToMaster(m->node_count);
                if (!cJSON_IsNumber(mastery_gems)) node->mastery_gems = CAMPAIGN_OFFLINE_MASTERY_GEMS;
            }
            m->node_count++;
        }
        if (m->node_count == 0) TraceLog(LOG_FATAL, "Campaign: %s has no nodes", id);
        if (m->node_count != JsonInt(chapter, "node_count"))
            TraceLog(LOG_FATAL, "Campaign: %s node_count disagrees with its node list", id);
        for (int node = 0; node < m->node_count - 1; node++) {
            if (m->nodes[node].grind) TraceLog(LOG_FATAL, "Campaign: %s grind node must be the final node", id);
        }
        if (m->nodes[m->node_count - 1].grind && m->node_count < 2)
            TraceLog(LOG_FATAL, "Campaign: %s grind node has no preceding boss", id);
        c->map_count++;
    }
    cJSON_Delete(root);

    if (c->map_count == 0) TraceLog(LOG_FATAL, "Campaign: no maps in %s", path);
    TraceLog(LOG_INFO, "Campaign: loaded %d maps from %s", c->map_count, path);
}

void CampaignInitialize(Campaign *c, const char *maps_path) {
    c->font = GuiFont(FNT_COOL);
    c->scroll_x = 0.0f;
    c->scroll_map = -1;
    c->scroll_node = 0;
    c->selected_node = 0;
    c->press_pos = (Vector2){0, 0};
    c->dragged = false;
    c->progression_active = false;
    c->progression_unlock_next = false;
    c->progression_camera_finished = true;
    c->progression_time = 0.0f;
    c->progression_next_sound_played = false;
    c->separator = GuiTex(GUI_MAP_SEPARATOR);
    c->icons.energy = GuiTex(GUI_MAP_ENERGY);
    c->icons.locked = GuiTex(GUI_NODE_LOCKED);
    c->icons.locked_selected = GuiTex(GUI_NODE_LOCKED_ON);
    c->icons.open = GuiTex(GUI_NODE_OPEN);
    c->icons.open_selected = GuiTex(GUI_NODE_OPEN_ON);
    c->icons.done = GuiTex(GUI_NODE_DONE);
    c->icons.done_selected = GuiTex(GUI_NODE_DONE_ON);
    c->icons.mastered = GuiTex(GUI_NODE_MASTERED);
    c->icons.mastered_selected = GuiTex(GUI_NODE_MASTERED_ON);
    c->icons.boss = GuiTex(GUI_NODE_BOSS);
    c->icons.boss_selected = GuiTex(GUI_NODE_BOSS_ON);
    c->icons.boss_locked = GuiTex(GUI_NODE_BOSS_LOCKED);
    c->icons.boss_locked_selected = GuiTex(GUI_NODE_BOSS_LOCKED_ON);
    c->icons.grind = GuiTex(GUI_NODE_GRIND);
    c->icons.grind_selected = GuiTex(GUI_NODE_GRIND_ON);

    c->lore = GuiTex(GUI_MAP_LORE);
    c->progress_bar = GuiTex(GUI_MAP_PROGRESS);
    c->help = GuiTex(GUI_MAP_HELP);
    c->gold = GuiTex(GUI_HUD_GOLD);
    c->xp = GuiTex(GUI_HUD_XP);
    c->gems = GuiTex(GUI_MAP_GEMS);
    c->reward_tag = GuiTex(GUI_MAP_REWARD_TAG);
    c->energy_flask = GuiTex(GUI_HUD_ENERGY);

    c->change_map = ButtonLoadText(GUI_BTN_MAPS, GUI_BTN_MAPS, "Кампании", c->font, (Vector2){0.5f, 0.82f}, 0.17f);
    c->enter_map = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "Войти", c->font, (Vector2){0.5f, 0.5f}, 0.35f);
    ButtonDisableClickSound(&c->change_map);

    CampaignMapsInitialize(c, maps_path);
}

int CampaignBossNodeIndex(const Campaign *c, int map_index) {
    if (!c || map_index < 0 || map_index >= c->map_count) return -1;
    const CampaignMap *m = &c->maps[map_index];
    if (m->node_count <= 0) return -1;
    int boss = m->node_count - 1;
    if (m->nodes[boss].grind) boss--;
    return boss;
}

static int CampaignGrindNodeIndex(const Campaign *c, int map_index) {
    if (!c || map_index < 0 || map_index >= c->map_count) return -1;
    const CampaignMap *m = &c->maps[map_index];
    int last = m->node_count - 1;
    return last >= 0 && m->nodes[last].grind ? last : -1;
}

static Texture2D
CampaignNodeIcon(Campaign *c, const CampaignMap *map, int node, int frontier, int completions, bool selected) {
    const CampaignNode *campaign_node = &map->nodes[node];
    if (campaign_node->grind) {
        if (node > frontier && completions <= 0) return c->icons.grind;
        return selected ? c->icons.grind_selected : c->icons.grind;
    }
    bool boss = node == map->node_count - 1 - (map->nodes[map->node_count - 1].grind ? 1 : 0);
    if (node < frontier || completions > 0) {
        bool mastered =
            !campaign_node->grind && campaign_node->wins_to_master > 0 && completions >= campaign_node->wins_to_master;
        if (mastered) return selected ? c->icons.mastered_selected : c->icons.mastered;
        return selected ? c->icons.done_selected : c->icons.done;
    }
    if (node > frontier) {
        if (boss) return selected ? c->icons.boss_locked_selected : c->icons.boss_locked;
        return selected ? c->icons.locked_selected : c->icons.locked;
    }
    if (boss) return selected ? c->icons.boss_selected : c->icons.boss;
    return selected ? c->icons.open_selected : c->icons.open;
}

int CampaignFrontierForMap(const Campaign *c, const PlayerCampaignProgress *pcp, int map_index) {
    if (map_index < 0 || map_index >= c->map_count) return 0;
    int boss = CampaignBossNodeIndex(c, map_index);
    for (int node = 0; node <= boss; node++) {
        if (pcp->node_completions[map_index][node] <= 0) return node;
    }
    int grind = CampaignGrindNodeIndex(c, map_index);
    return grind >= 0 ? grind : boss;
}

bool CampaignMapUnlocked(const Campaign *c, const PlayerCampaignProgress *pcp, int map_index) {
    if (map_index <= 0) return map_index == 0;
    if (map_index >= c->map_count) return false;
    for (int previous = 0; previous < map_index; previous++) {
        int boss = CampaignBossNodeIndex(c, previous);
        if (boss < 0 || pcp->node_completions[previous][boss] <= 0) return false;
    }
    return true;
}

void CampaignSelectMap(Campaign *c, PlayerCampaignProgress *pcp, int map_index) {
    if (!CampaignMapUnlocked(c, pcp, map_index)) return;
    pcp->campaign_index = map_index;
    pcp->node_index = CampaignFrontierForMap(c, pcp, map_index);
    c->selected_node = pcp->node_index;
    c->scroll_map = -1;
}

void CampaignBeginProgressionAnimation(Campaign *c, int from_map, int from_node, int to_map, int to_node) {
    c->progression_active = true;
    c->progression_unlock_next = true;
    c->progression_camera_finished = false;
    c->progression_time = 0.0f;
    c->progression_from_map = from_map;
    c->progression_from_node = from_node;
    c->progression_to_map = to_map;
    c->progression_to_node = to_node;
    c->progression_next_sound_played = false;
    c->selected_node = to_node;
    c->scroll_map = -1;
    if (from_map == to_map)
        GuiSoundPlay(SND_PVE_NODE_TRANSITION);
    else
        GuiSoundPlay(SND_MAP_TRANSITION);
}

void CampaignBeginMasteryAnimation(Campaign *c, int map, int completed_node, int selected_node) {
    CampaignBeginProgressionAnimation(c, map, completed_node, map, selected_node);
    c->progression_unlock_next = false;
}

static float CampaignEaseInOutQuad(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

static float CampaignFlipScale(float t) {
    if (t < 0.0f || t > 1.0f) return 1.0f;
    return t < 0.5f ? 1.0f - CampaignEaseInOutQuad(t * 2.0f) : CampaignEaseInOutQuad((t - 0.5f) * 2.0f);
}

static float CampaignProgressionPanEnd(const Campaign *c) {
    return c->progression_from_map == c->progression_to_map ? 2.0f : 3.5f;
}

static float CampaignNodeCenterX(float origin, float position_x, float world_w, float icon_width) {
    return origin + position_x * world_w - icon_width / 4.0f;
}

static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
) {
    char line[640] = {0};
    float y = pos.y;

    const char *w = text;
    while (*w) {
        const char *e = w;
        while (*e && *e != ' ') e++;
        int wlen = (int)(e - w);

        char trial[640];
        if (line[0])
            snprintf(trial, sizeof(trial), "%s %.*s", line, wlen, w);
        else
            snprintf(trial, sizeof(trial), "%.*s", wlen, w);

        if (line[0] && MeasureTextEx(font, trial, size, spacing).x > max_width) {
            DrawTextEx(font, line, (Vector2){pos.x, y}, size, spacing, tint);
            y += size + line_gap;
            snprintf(line, sizeof(line), "%.*s", wlen, w);
        } else {
            snprintf(line, sizeof(line), "%s", trial);
        }

        w = e;
        while (*w == ' ') w++;
    }
    if (line[0]) DrawTextEx(font, line, (Vector2){pos.x, y}, size, spacing, tint);
}

int CampaignSelectedEnergyCost(const Campaign *c, const PlayerCampaignProgress *pcp) {
    return c->maps[pcp->campaign_index].nodes[c->selected_node].energy_cost;
}

const CampaignNode *CampaignFindNode(const Campaign *c, const char *map_id, int step) {
    for (int m = 0; m < c->map_count; m++) {
        if (strcmp(c->maps[m].id, map_id) != 0) continue;
        if (step >= 1 && step <= c->maps[m].node_count) return &c->maps[m].nodes[step - 1];
    }
    return NULL;
}

CampaignAction CampaignDraw(Campaign *c, PlayerCampaignProgress *pcp) {
    CampaignAction action = CAMPAIGN_NONE;
    float dt = GetFrameTime();
    if (pcp->campaign_index < 0 || pcp->campaign_index >= c->map_count ||
        !CampaignMapUnlocked(c, pcp, pcp->campaign_index))
        pcp->campaign_index = 0;
    pcp->node_index = CampaignFrontierForMap(c, pcp, pcp->campaign_index);
    CampaignMap *m = &c->maps[pcp->campaign_index];
    float screen_w = UiWidth();
    float screen_h = UiHeight();

    if (c->progression_active) {
        c->progression_time += dt;
        if (c->progression_unlock_next && !c->progression_next_sound_played &&
            c->progression_from_map == c->progression_to_map && c->progression_to_node != c->progression_from_node &&
            c->progression_time >= 2.0f) {
            GuiSoundPlay(SND_PVE_NODE_TRANSITION);
            c->progression_next_sound_played = true;
        }
    }

    bool map_changed = c->scroll_map != pcp->campaign_index;
    if (map_changed) {
        c->scroll_map = pcp->campaign_index;
        c->selected_node = c->progression_active ? c->progression_to_node : pcp->node_index;
    }
    if (c->selected_node >= m->node_count) c->selected_node = m->node_count - 1;

    float strip_src_h = m->background.height / 2.0f;
    float strip_w = m->background.width * (screen_h / strip_src_h);
    float world_w = 2.0f * strip_w;
    float max_scroll = fmaxf(0.0f, CAMPAIGN_MAP_LEFT + world_w - screen_w);

    float node_x = CAMPAIGN_MAP_LEFT + m->nodes[c->selected_node].position.x * world_w;
    float target = Clamp(node_x - screen_w / 2.0f, 0.0f, max_scroll);
    if (c->progression_active && !c->progression_camera_finished && c->progression_to_map == pcp->campaign_index) {
        bool same_map = c->progression_from_map == c->progression_to_map;
        int from_node = same_map ? Clamp(c->progression_from_node, 0, m->node_count - 1) : m->node_count - 1;
        float from_x = CAMPAIGN_MAP_LEFT + m->nodes[from_node].position.x * world_w;
        float from_scroll = Clamp(from_x - screen_w / 2.0f, 0.0f, max_scroll);
        float pan = same_map ? CampaignEaseInOutQuad(c->progression_time - 1.0f)
                             : CampaignEaseInOutQuad((c->progression_time - 1.5f) / 2.0f);
        c->scroll_x = Lerp(from_scroll, target, pan);
        c->scroll_node = c->selected_node;
        if (c->progression_time >= CampaignProgressionPanEnd(c)) {
            c->scroll_x = target;
            c->progression_camera_finished = true;
        }
    } else if (map_changed) {
        c->scroll_node = c->selected_node;
        c->scroll_x = target;
    } else if (c->scroll_node != c->selected_node) {
        c->scroll_x += (target - c->scroll_x) * fminf(1.0f, dt * 6.0f);
        if (fabsf(target - c->scroll_x) < 1.0f) c->scroll_node = c->selected_node;
    }

    bool progression_input_locked = (c->progression_active && !c->progression_camera_finished) || UiInputBlocked();
    if (!progression_input_locked && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        c->press_pos = UiMouse();
        c->dragged = false;
    }
    if (!progression_input_locked && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        c->scroll_x -= UiMouseDelta().x;
        c->scroll_node = c->selected_node;
        if (fabsf(UiMouse().x - c->press_pos.x) > 6.0f) c->dragged = true;
    }
    if (!progression_input_locked) c->scroll_x -= GetMouseWheelMove() * 60.0f;
    c->scroll_x = Clamp(c->scroll_x, 0.0f, max_scroll);

    float origin = CAMPAIGN_MAP_LEFT - c->scroll_x;

    for (int part = 0; part < 2; part++) {
        Rectangle src = {0, part * strip_src_h, m->background.width, strip_src_h};
        Rectangle dst = {origin + part * strip_w, 0, strip_w, screen_h};
        DrawTexturePro(m->background, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }

    {
        Rectangle dst = {origin - 2.0f * CAMPAIGN_MAP_LEFT, 0, c->separator.width, screen_h};
        UiDrawTexture(c->separator, dst, WHITE);
    }

    bool tap = !progression_input_locked && !c->dragged && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    Vector2 mouse = UiMouse();

    for (int j = 0; j < m->node_count; j++) {
        int frontier = pcp->node_index;
        int completions = pcp->node_completions[pcp->campaign_index][j];
        float flip_scale = 1.0f;
        float transition_phase = -1.0f;
        bool same_map_progression = c->progression_active && c->progression_from_map == pcp->campaign_index &&
                                    c->progression_to_map == pcp->campaign_index;
        bool new_map_progression = c->progression_active && c->progression_from_map != c->progression_to_map &&
                                   c->progression_to_map == pcp->campaign_index;
        if (same_map_progression && j == c->progression_from_node) {
            float phase = c->progression_time;
            transition_phase = phase;
            flip_scale = CampaignFlipScale(phase);
            if (phase < 0.5f) {
                completions = fmaxf(0, completions - 1);
                frontier = c->progression_from_node;
            }
        }
        if (c->progression_unlock_next && same_map_progression && j == c->progression_to_node &&
            c->progression_to_node != c->progression_from_node) {
            float phase = c->progression_time - 2.0f;
            transition_phase = phase;
            flip_scale = CampaignFlipScale(phase);
            if (phase < 0.5f) frontier = c->progression_from_node;
        }
        if (new_map_progression && j == m->node_count - 1) {
            float phase = c->progression_time - 0.5f;
            transition_phase = phase;
            flip_scale = CampaignFlipScale(phase);
        }
        Texture2D icon = CampaignNodeIcon(c, m, j, frontier, completions, j == c->selected_node);
        float icon_w = icon.width / 2.0f;
        float icon_h = icon.height / 1.5f;
        float node_cx = CampaignNodeCenterX(origin, m->nodes[j].position.x, world_w, icon.width);
        Rectangle dst = {
            node_cx - icon_w * flip_scale / 2.0f,
            m->nodes[j].position.y * screen_h - icon.width / 2.0f,
            icon_w * flip_scale,
            icon_h
        };
        if (transition_phase >= 0.0f && transition_phase <= 1.0f) {
            float pulse = sinf(transition_phase * PI);
            float cy = dst.y + dst.height * 0.5f;
            float glow_size = 92.0f + 70.0f * pulse;
            Texture2D glow = GuiTex(GUI_NODE_UNLOCK_GLOW);
            BeginBlendMode(BLEND_ADDITIVE);
            UiDrawTexture(
                glow,
                (Rectangle){node_cx - glow_size * 0.5f, cy - glow_size * 0.5f, glow_size, glow_size},
                (Color){246, 216, 111, (unsigned char)(175.0f * pulse)}
            );
            EndBlendMode();
        }
        UiDrawTexture(icon, dst, WHITE);

        {
            float bw = 112.0f, bh = 56.0f;
            Rectangle banner = {node_cx - bw / 2.0f, dst.y + dst.height - 6.0f, bw, bh};
            UiDrawTexture(c->icons.energy, banner, WHITE);

            const char *txt = TextFormat("%d", m->nodes[j].energy_cost);
            float fs = 24.0f, flask = 30.0f, gap = 8.0f;
            Vector2 tsz = MeasureTextEx(c->font, txt, fs, 1.0f);
            float gx = node_cx - (flask + gap + tsz.x) / 2.0f, gcy = banner.y + bh / 2.0f;
            UiDrawTexture(c->energy_flask, (Rectangle){gx, gcy - flask / 2.0f, flask, flask}, WHITE);
            DrawTextEx(c->font, txt, (Vector2){gx + flask + gap, gcy - tsz.y / 2.0f}, fs, 1.0f, WHITE);
        }

        if (tap && CheckCollisionPointRec(mouse, dst)) {
            GuiSoundPlay(SND_PVE_NODE_SELECTED);
            c->selected_node = j;
        }
    }

    {
        if (ButtonDrawEx(
                &c->change_map, (Rectangle){0.0f, screen_h - 88.0f, 88.0f, 88.0f}, !progression_input_locked
            )) {
            int next = pcp->campaign_index;
            for (int tries = 0; tries < c->map_count; tries++) {
                next = (next + 1) % c->map_count;
                if (CampaignMapUnlocked(c, pcp, next)) break;
            }
            GuiClickSoundPlayDuplicate();
            if (next != pcp->campaign_index) {
                GuiSoundPlay(SND_MAP_TRANSITION);
            }
            CampaignSelectMap(c, pcp, next);
        }
    }

    {
        bool unlocked = c->selected_node <= pcp->node_index;
        if (ButtonDrawEx(
                &c->enter_map, (Rectangle){screen_w - 264.0f, screen_h - 72.0f, -1, -1}, !progression_input_locked
            ) &&
            unlocked)
            action = CAMPAIGN_ENTER;
    }

    Rectangle lore = {screen_w - c->lore.width / 1.3f, 72.0f, c->lore.width / 1.3f, c->lore.height / 1.3f};
    {
        Rectangle src = {2, 0, c->lore.width, c->lore.height};
        DrawTexturePro(c->lore, src, lore, (Vector2){0, 0}, 0.0f, WHITE);
    }

    CampaignNode *n = &m->nodes[c->selected_node];
    {
        float size = 22.0f, spacing = 1.0f;
        Vector2 tw = MeasureTextEx(c->font, n->name, size, spacing);
        Vector2 pos = {lore.x + lore.width - tw.x - 8.0f, lore.y + 44.0f};
        DrawTextEx(c->font, n->name, pos, size, spacing, (Color){235, 232, 220, 255});
    }

    {
        Rectangle dst = {screen_w - 356.0f, 106, 48, 48};
        UiDrawTexture(c->help, dst, WHITE);
    }

    {
        Font font = GuiFont(FNT_MENU);
        float size = 16.0f, spacing = 1.0f;
        Vector2 pos = {lore.x + 42.0f, lore.y + 148.0f};
        float wrap_w = lore.width - 46.0f;
        CampaignDrawWrapped(font, n->desc, pos, size, spacing, 4.0f, wrap_w, (Color){178, 178, 178, 255});
    }

    {
        Rectangle dst = {lore.x + 24.0f, lore.y + 84.0f, 364, 64};
        UiDrawTexture(c->progress_bar, dst, WHITE);

        bool unlocked = c->selected_node <= pcp->node_index;
        int wins = pcp->node_completions[pcp->campaign_index][c->selected_node];
        char progress[64] = {0};
        if (!unlocked)
            StrCopy(progress, sizeof(progress), LocGet("PVE_NODE_NODE_LOCKED"));
        else
            CampaignNodeMasteryLabel(n, wins, progress, sizeof(progress));
        Color tint = unlocked ? (Color){240, 236, 224, 255} : (Color){150, 145, 138, 255};
        float size = 24.0f, spacing = 1.0f;
        Vector2 tw = MeasureTextEx(c->font, progress, size, spacing);
        Vector2 pos = {dst.x + 40.0f, dst.y + (dst.height - tw.y) / 2.0f};
        DrawTextEx(c->font, progress, pos, size, spacing, tint);
    }

    {
        float size = 15.0f, spacing = 1.0f;
        Vector2 pos = {lore.x + 42.0f, lore.y + lore.height - 78.0f};
        float wrap_w = lore.width - 84.0f;
        CampaignDrawWrapped(c->font, n->cta, pos, size, spacing, 4.0f, wrap_w, (Color){228, 224, 212, 255});
    }

    {
        Texture2D icons[3] = {c->gold, c->xp, c->gems};
        int wins = pcp->node_completions[pcp->campaign_index][c->selected_node];
        CampaignRewards rewards = CampaignNodeRewardsForWins(n, wins);
        int values[3] = {rewards.gold, rewards.xp, rewards.gems};
        int count = rewards.gems > 0 ? 3 : 2;

        float icon_box = 44.0f, row_h = 42.0f, tag_w = 96.0f, tag_h = 36.0f;
        float icon_x = screen_w - 44.0f - icon_box;
        float start_y = screen_h - 84.0f - count * row_h;

        for (int i = 0; i < count; i++) {
            float cy = start_y + i * row_h + row_h / 2.0f;

            Rectangle tag = {icon_x - tag_w, cy - tag_h / 2.0f, tag_w, tag_h};
            Rectangle tag_src = {0, 0, c->reward_tag.width, c->reward_tag.height};
            DrawTexturePro(c->reward_tag, tag_src, tag, (Vector2){0, 0}, 0.0f, WHITE);

            const char *txt = TextFormat("+%d", values[i]);
            float size = 26.0f, spacing = 1.0f;
            Vector2 tw = MeasureTextEx(c->font, txt, size, spacing);
            Vector2 tpos = {icon_x - 6.0f - tw.x, cy - tw.y / 2.0f};
            DrawTextEx(c->font, txt, tpos, size, spacing, (Color){245, 244, 238, 255});

            Rectangle idst = {icon_x, cy - icon_box / 2.0f, icon_box, icon_box};
            UiDrawTexture(icons[i], idst, WHITE);
        }
    }

    if (c->progression_active) {
        float duration =
            c->progression_from_map == c->progression_to_map ? (c->progression_unlock_next ? 3.0f : 2.0f) : 3.5f;
        if (c->progression_time >= duration) c->progression_active = false;
    }
    return action;
}
