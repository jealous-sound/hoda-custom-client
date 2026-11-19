#define CAMPAIGN_SCENES_PATH "data/campaign_scenes.json"
#define CAMPAIGN_SWIPE_MARGIN 3.0f
#define CAMPAIGN_SWIPE_UNITS_PER_SCREEN 10.0f
#define CAMPAIGN_WHEEL_UNITS 0.7f
#define CAMPAIGN_FOCUS_SPEED 6.0f
#define CAMPAIGN_FOCUS_SETTLED 0.01f
#define CAMPAIGN_EDGE_RETURN_UNITS_PER_SECOND 0.5f
#define CAMPAIGN_MAP_INDICATOR_FADE_SECONDS 0.25f
#define CAMPAIGN_MAP_INDICATOR_PULSE 0.25f
#define CAMPAIGN_OFFLINE_MASTERY_GEMS 1
#define CAMPAIGN_ID_PREFIX "PVE_MAP_"
#define CAMPAIGN_TRAY_ICON_PREFIX "tex.Btn_MapPVE_map_"
#define CAMPAIGN_HEADER_KEY_FORMAT "%s_header"
#define CAMPAIGN_NODE_KEY_FORMAT "MAP_%s_NODE_%d_%s"
#define CAMPAIGN_ENERGY_TEXT_OF_BANNER 0.5f
#define CAMPAIGN_TRAY_SCROLL_ITEMS 5
#define CAMPAIGN_TRAY_SCROLL_SPEED 6.0f
#define CAMPAIGN_TRAY_TOGGLE_LABEL_ANCHOR_Y 0.82f
#define CAMPAIGN_TRAY_TOGGLE_LABEL_SCALE 0.17f
#define CAMPAIGN_CTA_TITLE_OF_LABEL 0.7f
#define CAMPAIGN_CTA_BODY_SIZE 17.0f
#define CAMPAIGN_CTA_BODY_MIN_SIZE 11.0f

typedef struct {
    float x, y, width, height, aspect;
} CampaignIguiAnchor;

static const CampaignIguiAnchor CAMPAIGN_TRAY_BACKGROUND = {0.5f, 1.055f, 1.9f, 0.25f, 8.0f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_CONTAINER = {1.0f, 1.0f, 0.897f, 1.0f, 0.0f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_LIST = {0.44f, 1.0f, 0.84f, 0.16f, 0.0f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_MAP_BUTTON = {0.0f, 0.5f, 1.0f, 1.3f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_BACK_ARROW = {0.0f, 0.97f, 1.0f, 0.1f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_FORWARD_ARROW = {0.98f, 0.97f, 1.0f, 0.1f, 1.0f};
static const Vector2 CAMPAIGN_TRAY_BACKGROUND_SCALE = {1.0f, -1.0f};
static const Vector2 CAMPAIGN_TRAY_BACK_ARROW_SCALE = {-1.0f, 1.0f};
static const Vector2 CAMPAIGN_TRAY_FORWARD_ARROW_SCALE = {1.0f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_TOGGLE_CONTAINER = {0.0f, 1.0f, 1.0f, 0.15f, 1.1f};
static const CampaignIguiAnchor CAMPAIGN_TRAY_TOGGLE = {0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_CTA_PANEL = {0.0f, 0.04f, 1.0f, 0.32f, 2.251f};
static const CampaignIguiAnchor CAMPAIGN_CTA_TITLE = {0.1f, 0.2f, 0.8f, 0.2f, 0.0f};
static const CampaignIguiAnchor CAMPAIGN_CTA_BODY = {0.775f, 0.725f, 0.725f, 0.38f, 0.0f};
static const CampaignIguiAnchor CAMPAIGN_CTA_INFO = {0.925f, 0.175f, 1.0f, 0.275f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_CTA_ICON = {-0.025f, 0.775f, 1.0f, 0.55f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_PREVIOUS_MAP_INDICATOR = {0.0818f, 0.5f, 0.2795f, 0.22f, 1.0f};
static const CampaignIguiAnchor CAMPAIGN_NEXT_MAP_INDICATOR = {0.9f, 0.5f, 0.2795f, 0.22f, 1.0f};
static const float CAMPAIGN_PREVIOUS_MAP_ARROW_ROTATION = 90.0f;
static const float CAMPAIGN_NEXT_MAP_ARROW_ROTATION = -90.0f;

#define CAMPAIGN_ABOUT_CANVAS_WIDTH 1280.0f
#define CAMPAIGN_ABOUT_TITLE_SIZE 34.0f
#define CAMPAIGN_ABOUT_TAB_SIZE 24.0f
#define CAMPAIGN_ABOUT_QUOTE_SIZE 16.0f
#define CAMPAIGN_ABOUT_TEXT_SIZE 18.0f
#define CAMPAIGN_ABOUT_CTA_SIZE 18.0f
#define CAMPAIGN_ABOUT_MIN_TEXT_SIZE 11.0f
#define CAMPAIGN_DETAILS_TITLE_SIZE 22.0f
#define CAMPAIGN_DETAILS_TITLE_GAP 8.0f

static const Rectangle CAMPAIGN_ABOUT_FRAME = {208.0f, 137.0f, 865.0f, 583.0f};
static const Rectangle CAMPAIGN_ABOUT_TITLE = {208.0f, 137.0f, 865.0f, 59.0f};
static const Rectangle CAMPAIGN_ABOUT_TAB_LEFT = {269.0f, 216.0f, 368.0f, 59.0f};
static const Rectangle CAMPAIGN_ABOUT_TAB_MIDDLE = {636.0f, 216.0f, 9.0f, 59.0f};
static const Rectangle CAMPAIGN_ABOUT_TAB_RIGHT = {642.0f, 216.0f, 372.0f, 59.0f};
static const Rectangle CAMPAIGN_ABOUT_TAB_ABOUT_LABEL = {269.0f, 216.0f, 365.0f, 54.0f};
static const Rectangle CAMPAIGN_ABOUT_TAB_HELP_LABEL = {647.0f, 216.0f, 365.0f, 54.0f};
static const Rectangle CAMPAIGN_ABOUT_CLOSE = {995.0f, 171.0f, 66.0f, 66.0f};
static const Rectangle CAMPAIGN_ABOUT_QUOTE_BG = {258.0f, 274.0f, 242.0f, 421.0f};
static const Rectangle CAMPAIGN_ABOUT_QUOTE_ART = {325.0f, 305.0f, 109.5f, 116.0f};
static const Rectangle CAMPAIGN_ABOUT_QUOTE_TEXT = {295.0f, 434.0f, 170.0f, 222.0f};
static const Rectangle CAMPAIGN_ABOUT_MAP_ART = {497.0f, 283.0f, 503.0f, 125.3f};
static const Rectangle CAMPAIGN_ABOUT_TEXT = {497.0f, 420.0f, 503.0f, 172.0f};
static const Rectangle CAMPAIGN_ABOUT_CTA_BG = {467.0f, 575.0f, 561.0f, 119.6f};
static const Rectangle CAMPAIGN_ABOUT_CTA_TEXT = {509.0f, 612.0f, 360.0f, 43.0f};
static const Rectangle CAMPAIGN_ABOUT_GEMS_TEXT = {853.0f, 614.0f, 72.0f, 43.0f};
static const Rectangle CAMPAIGN_ABOUT_COLLECTED = {876.0f, 607.0f, 58.0f, 58.0f};

static Rectangle CampaignIguiRect(Rectangle parent, CampaignIguiAnchor anchor) {
    return HudFitRect(parent, anchor.x, anchor.y, anchor.width, anchor.height, anchor.aspect);
}

static Rectangle CampaignScreenRoot(float screen_w, float screen_h) {
    return (Rectangle){0.0f, HUD_TOP_HEIGHT, screen_w, screen_h - HUD_TOP_HEIGHT};
}

static Rectangle CampaignScreenIgui(float screen_w, float screen_h, CampaignIguiAnchor anchor) {
    Rectangle full_scale = {0.0f, HUD_TOP_HEIGHT, UiFullScaleWidth(), UI_DESIGN_H - HUD_TOP_HEIGHT};
    Rectangle root = CampaignScreenRoot(screen_w, screen_h);
    Rectangle rect = CampaignIguiRect(full_scale, anchor);
    rect.x += (root.width - full_scale.width) * Clamp(anchor.x, 0.0f, 1.0f);
    rect.y += (root.height - full_scale.height) * Clamp(anchor.y, 0.0f, 1.0f);
    return rect;
}

typedef enum { CAMPAIGN_NONE = 0, CAMPAIGN_ENTER } CampaignAction;

typedef struct {
    bool active;
    float from;
    float to;
    float elapsed;
    float duration;
} CampaignEdgeReturn;

typedef struct {
    float opacity;
    float from;
    float target;
    float elapsed;
} CampaignIndicatorFade;

typedef enum {
    CAMPAIGN_SWIPE_INSIDE,
    CAMPAIGN_SWIPE_AT_LOW_EDGE,
    CAMPAIGN_SWIPE_AT_HIGH_EDGE,
} CampaignSwipeEdge;

typedef struct {
    char name[80];
    char desc[512];
    char cta[256];
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
    Texture2D tray_icon;
    Texture2D about_quote;
    Texture2D about_thumbnail;
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
    MapScene scene;
    int scene_map;
    Texture2D lore;
    Texture2D progress_bar;
    Texture2D help;
    Texture2D gold;
    Texture2D xp;
    Texture2D gems;
    Texture2D reward_tag;
    Texture2D energy_flask;
    Texture2D tray_bg;
    Texture2D tray_node;
    Texture2D tray_node_selected;
    Texture2D cta_bg;
    Texture2D map_indicator_arrow;
    Texture2D about_frame;
    Texture2D about_quote_bg;
    Texture2D about_cta_bg;
    Texture2D about_tab_left;
    Texture2D about_tab_middle;
    Texture2D about_tab_right;
    Texture2D about_collected;
    Button about_close;
    bool about_open;
    bool about_opening_release;
    Button change_map;
    Button tray_back;
    Button tray_forward;
    Button enter_map;
    bool tray_open;
    bool press_blocked;
    bool tray_pressed;
    float tray_scroll;
    float tray_scroll_target;
    float scroll_x;
    CampaignEdgeReturn edge_return;
    CampaignSwipeEdge swipe_edge;
    bool arrive_at_first_node;
    CampaignIndicatorFade previous_map_indicator;
    CampaignIndicatorFade next_map_indicator;
    float map_indicator_pulse_time;
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
        LocFormatInt2(out, (size_t)cap, "BUTTON_BATTLERESULTS_MASTERYPROGRESS", wins, node->wins_to_master);
}

static int CampaignMapNumber(const char *map_id) {
    size_t prefix = strlen(CAMPAIGN_ID_PREFIX);
    if (strncmp(map_id, CAMPAIGN_ID_PREFIX, prefix) != 0)
        TraceLog(LOG_FATAL, "Campaign: map id %s does not start with %s", map_id, CAMPAIGN_ID_PREFIX);
    return atoi(map_id + prefix);
}

static const char *CampaignMapPrefab(int map_number) {
    return map_number == 1 ? "map_placeholder_pre" : TextFormat("map_placeholder%d_pre", map_number);
}

static Texture2D CampaignLoadMapPrefabTexture(int map_number, const char *name_format) {
    char prefab[48];
    StrCopy(prefab, sizeof(prefab), CampaignMapPrefab(map_number));
    char name[48];
    snprintf(name, sizeof(name), name_format, map_number);
    return LoadSprite(TextFormat("%s%s/%s", CACHE_RESOURCE_PREFIX, prefab, name));
}

static void CampaignCopyNodeText(char *out, int cap, const char *map_id, int step, const char *field) {
    StrCopy(out, cap, LocGet(TextFormat(CAMPAIGN_NODE_KEY_FORMAT, map_id, step, field)));
}

static Texture2D CampaignLoadTrayIcon(const char *map_id) {
    size_t prefix = strlen(CAMPAIGN_ID_PREFIX);
    if (strncmp(map_id, CAMPAIGN_ID_PREFIX, prefix) != 0)
        TraceLog(LOG_FATAL, "Campaign: map id %s does not start with %s", map_id, CAMPAIGN_ID_PREFIX);
    return LoadSprite(TextFormat("%s%s%s", CACHE_RESOURCE_PREFIX, CAMPAIGN_TRAY_ICON_PREFIX, map_id + prefix));
}

void CampaignMapsInitialize(Campaign *c, const char *path) {
    cJSON *root = JsonLoadFile(path);
    if (!cJSON_IsArray(root)) TraceLog(LOG_FATAL, "Campaign: %s is not an array of chapters", path);

    c->map_count = 0;
    cJSON *chapter = NULL;
    cJSON_ArrayForEach(chapter, root) {
        const char *id = JsonStr(chapter, "id");

        if (c->map_count >= CAMPAIGN_MAX_MAPS)
            TraceLog(LOG_FATAL, "Campaign: %s has more than %d maps", path, CAMPAIGN_MAX_MAPS);
        CampaignMap *m = &c->maps[c->map_count];

        StrCopy(m->id, sizeof(m->id), id);
        StrCopy(m->title, sizeof(m->title), LocGet(TextFormat(CAMPAIGN_HEADER_KEY_FORMAT, id)));

        m->tray_icon = CampaignLoadTrayIcon(id);
        m->about_quote = CampaignLoadMapPrefabTexture(CampaignMapNumber(id), "map%d_character");
        m->about_thumbnail = CampaignLoadMapPrefabTexture(CampaignMapNumber(id), "map%d");
        m->node_count = 0;
        cJSON *n = NULL;
        cJSON_ArrayForEach(n, cJSON_GetObjectItemCaseSensitive(chapter, "nodes")) {
            if (m->node_count >= CAMPAIGN_MAX_NODES)
                TraceLog(LOG_FATAL, "Campaign: %s has more than %d nodes", id, CAMPAIGN_MAX_NODES);
            CampaignNode *node = &m->nodes[m->node_count];
            int step = JsonInt(n, "step");
            CampaignCopyNodeText(node->name, sizeof(node->name), id, step, "NAME");
            CampaignCopyNodeText(node->desc, sizeof(node->desc), id, step, "DESCRIPTION");
            CampaignCopyNodeText(node->cta, sizeof(node->cta), id, step, "CTA");
            cJSON *energy = cJSON_GetObjectItemCaseSensitive(n, "energy_cost");
            if (!cJSON_IsNumber(energy))
                TraceLog(LOG_FATAL, "Campaign: %s node %d (%s) lacks energy data", id, m->node_count + 1, node->name);
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
    c->scene.loaded = false;
    c->scene_map = -1;
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

    c->tray_bg = GuiTex(GUI_MAP_TRAY_BG);
    c->tray_node = GuiTex(GUI_MAP_TRAY_NODE);
    c->tray_node_selected = GuiTex(GUI_MAP_TRAY_NODE_SELECTED);
    c->tray_back = ButtonLoad(GUI_MAP_TRAY_ARROW, GUI_MAP_TRAY_ARROW_ON);
    c->tray_forward = ButtonLoad(GUI_MAP_TRAY_ARROW, GUI_MAP_TRAY_ARROW_ON);
    c->cta_bg = GuiTex(GUI_MAP_TRAY_CTA_BG);
    c->map_indicator_arrow = GuiTex(GUI_MAP_INDICATOR_ARROW);
    c->about_frame = GuiTex(GUI_QUESTS_BG);
    c->about_quote_bg = GuiTex(GUI_MAP_ABOUT_QUOTE_BG);
    c->about_cta_bg = GuiTex(GUI_MAP_ABOUT_CTA_BG);
    c->about_tab_left = GuiTex(GUI_MAP_ABOUT_TAB_LEFT);
    c->about_tab_middle = GuiTex(GUI_MAP_ABOUT_TAB_MIDDLE);
    c->about_tab_right = GuiTex(GUI_MAP_ABOUT_TAB_RIGHT);
    c->about_collected = GuiTex(GUI_MAP_ABOUT_COLLECTED);
    c->about_close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&c->about_close, SND_UI_POPUP_CLOSE);
    c->about_open = false;
    c->tray_open = false;
    c->tray_scroll = 0.0f;
    c->tray_scroll_target = 0.0f;
    c->change_map = ButtonLoadText(
        GUI_BTN_MAPS,
        GUI_BTN_MAPS_ON,
        LocGet("PVE_MAP_LABEL"),
        c->font,
        (Vector2){0.5f, CAMPAIGN_TRAY_TOGGLE_LABEL_ANCHOR_Y},
        CAMPAIGN_TRAY_TOGGLE_LABEL_SCALE
    );
    c->enter_map =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("PVE_MAP_ENTER"), c->font, (Vector2){0.5f, 0.5f}, 0.35f);
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

static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
) {
    char line[640] = {0};
    float y = pos.y;

    const char *w = text;
    bool spaced = false;
    while (*w) {
        if (*w == ' ' || *w == '\n') {
            spaced = true;
            w++;
            continue;
        }
        const char *e = UiBreakUnitEnd(w);
        int wlen = (int)(e - w);

        char trial[640];
        snprintf(trial, sizeof(trial), "%s%s%.*s", line, line[0] && spaced ? " " : "", wlen, w);
        spaced = false;

        if (line[0] && MeasureTextEx(font, trial, size, spacing).x > max_width) {
            DrawTextEx(font, line, (Vector2){pos.x, y}, size, spacing, tint);
            y += size + line_gap;
            snprintf(line, sizeof(line), "%.*s", wlen, w);
        } else {
            snprintf(line, sizeof(line), "%s", trial);
        }

        w = e;
    }
    if (line[0]) DrawTextEx(font, line, (Vector2){pos.x, y}, size, spacing, tint);
}

typedef struct {
    Rectangle background;
    Rectangle list;
    Rectangle back_arrow;
    Rectangle forward_arrow;
} CampaignTrayLayout;

static CampaignTrayLayout CampaignTrayLayoutFor(float screen_w, float screen_h) {
    Rectangle container = CampaignScreenIgui(screen_w, screen_h, CAMPAIGN_TRAY_CONTAINER);
    float toggle_room = UiFullScaleWidth() - container.width;
    container.x = toggle_room;
    container.width = screen_w - toggle_room;
    return (CampaignTrayLayout){
        .background = HudSpanCanvasWidth(CampaignScreenIgui(screen_w, screen_h, CAMPAIGN_TRAY_BACKGROUND)),
        .list = CampaignIguiRect(container, CAMPAIGN_TRAY_LIST),
        .back_arrow = CampaignIguiRect(container, CAMPAIGN_TRAY_BACK_ARROW),
        .forward_arrow = CampaignIguiRect(container, CAMPAIGN_TRAY_FORWARD_ARROW),
    };
}

static Rectangle CampaignTrayToggleRect(float screen_w, float screen_h) {
    Rectangle container = CampaignScreenIgui(screen_w, screen_h, CAMPAIGN_TRAY_TOGGLE_CONTAINER);
    return CampaignIguiRect(container, CAMPAIGN_TRAY_TOGGLE);
}

static int CampaignUnlockedMapCount(const Campaign *c, const PlayerCampaignProgress *pcp) {
    int count = 0;
    while (count < c->map_count && CampaignMapUnlocked(c, pcp, count)) count++;
    return count;
}

static Rectangle CampaignTrayButtonRow(const CampaignTrayLayout *layout) {
    return CampaignIguiRect(layout->list, CAMPAIGN_TRAY_MAP_BUTTON);
}

static float CampaignTrayItemSize(const CampaignTrayLayout *layout) {
    return CampaignTrayButtonRow(layout).width;
}

static float CampaignTrayStride(float list_w, float item_size, int count) {
    float fit = list_w / item_size;
    int visible = (int)floorf(fit);
    if (visible > count) visible = count;
    if (visible < 1) visible = 1;
    float margin = roundf((fit - (float)visible) * item_size / (2.0f * (float)visible));
    return item_size + 2.0f * margin;
}

static float CampaignTrayMaxScroll(const CampaignTrayLayout *layout, int count) {
    return fmaxf(
        0.0f, count * CampaignTrayStride(layout->list.width, CampaignTrayItemSize(layout), count) - layout->list.width
    );
}

static void CampaignTrayRevealMap(Campaign *c, const CampaignTrayLayout *layout, int count, int map) {
    float stride = CampaignTrayStride(layout->list.width, CampaignTrayItemSize(layout), count);
    float left = map * stride, right = left + stride;
    if (left < c->tray_scroll_target) c->tray_scroll_target = left;
    if (right > c->tray_scroll_target + layout->list.width) c->tray_scroll_target = right - layout->list.width;
    c->tray_scroll_target = Clamp(c->tray_scroll_target, 0.0f, CampaignTrayMaxScroll(layout, count));
    c->tray_scroll = c->tray_scroll_target;
}

static bool CampaignTrayCanScrollBack(const Campaign *c) {
    return c->tray_scroll_target > 0.0f;
}

static bool CampaignTrayCanScrollForward(const Campaign *c, const CampaignTrayLayout *layout, int count) {
    return c->tray_scroll_target < CampaignTrayMaxScroll(layout, count);
}

static Rectangle CampaignMirroredSource(Texture2D texture, Vector2 scale) {
    return (Rectangle){0.0f, 0.0f, copysignf((float)texture.width, scale.x), copysignf((float)texture.height, scale.y)};
}

static bool CampaignDrawTrayArrow(Button *arrow, Rectangle rect, Vector2 scale, bool interactive) {
    ButtonTouch touch = ButtonTouchAt(arrow, rect, interactive, ButtonPointerNow());
    Texture2D texture = touch.held ? arrow->active : arrow->idle;
    DrawTexturePro(texture, CampaignMirroredSource(texture, scale), rect, (Vector2){0}, 0.0f, WHITE);
    return touch.clicked;
}

static int CampaignDrawTray(
    Campaign *c, const PlayerCampaignProgress *pcp, const CampaignTrayLayout *layout, bool interactive, float dt
) {
    int count = CampaignUnlockedMapCount(c, pcp);
    float stride = CampaignTrayStride(layout->list.width, CampaignTrayItemSize(layout), count);
    float max_scroll = CampaignTrayMaxScroll(layout, count);
    DrawTexturePro(
        c->tray_bg,
        CampaignMirroredSource(c->tray_bg, CAMPAIGN_TRAY_BACKGROUND_SCALE),
        layout->background,
        (Vector2){0},
        0.0f,
        WHITE
    );

    if (CampaignTrayCanScrollBack(c) &&
        CampaignDrawTrayArrow(&c->tray_back, layout->back_arrow, CAMPAIGN_TRAY_BACK_ARROW_SCALE, interactive)) {
        c->tray_scroll_target -= CAMPAIGN_TRAY_SCROLL_ITEMS * stride;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (CampaignTrayCanScrollForward(c, layout, count) &&
        CampaignDrawTrayArrow(
            &c->tray_forward, layout->forward_arrow, CAMPAIGN_TRAY_FORWARD_ARROW_SCALE, interactive
        )) {
        c->tray_scroll_target += CAMPAIGN_TRAY_SCROLL_ITEMS * stride;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (interactive && CheckCollisionPointRec(UiMouse(), layout->list))
        c->tray_scroll_target -= GetMouseWheelMove() * stride;
    c->tray_scroll_target = Clamp(c->tray_scroll_target, 0.0f, max_scroll);
    c->tray_scroll += (c->tray_scroll_target - c->tray_scroll) * fminf(1.0f, dt * CAMPAIGN_TRAY_SCROLL_SPEED);

    int clicked = -1;
    Rectangle row = CampaignTrayButtonRow(layout);
    float item_size = row.width;
    float scale = UiScale();
    BeginScissorMode(
        (int)(layout->list.x * scale),
        (int)(row.y * scale),
        (int)(layout->list.width * scale),
        (int)(row.height * scale)
    );
    for (int map = 0; map < count; map++) {
        Rectangle item = {
            layout->list.x + map * stride - c->tray_scroll + (stride - item_size) * 0.5f,
            row.y,
            item_size,
            item_size,
        };
        bool current = map == pcp->campaign_index;
        UiDrawTexture(current ? c->tray_node_selected : c->tray_node, item, WHITE);
        UiDrawTexture(c->maps[map].tray_icon, item, WHITE);
        if (interactive && c->tray_pressed && !c->dragged && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(UiMouse(), item) && CheckCollisionPointRec(UiMouse(), layout->list))
            clicked = map;
    }
    EndScissorMode();
    return clicked;
}

static bool CampaignDrawHelpButton(const Campaign *c, Rectangle rect, bool interactive) {
    bool hover = interactive && CheckCollisionPointRec(UiMouse(), rect);
    UiDrawTexture(c->help, rect, hover ? (Color){255, 236, 180, 255} : WHITE);
    return hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void CampaignOpenAbout(Campaign *c) {
    c->about_open = true;
    c->about_opening_release = true;
    GuiSoundPlay(SND_UI_TAB);
}

bool CampaignHandleBack(Campaign *c) {
    if (!c->about_open) return false;
    c->about_open = false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    return true;
}

static float CampaignFitWrappedSize(Font font, const char *text, Rectangle box, float size, float min_size) {
    while (size > min_size && HeroesRichLayout(font, text, box, size, WHITE, false) > box.height) size -= 0.5f;
    return size;
}

static Rectangle CampaignDrawTrayCallToAction(const Campaign *c, const CampaignMap *m, float screen_w, float screen_h) {
    Rectangle panel = CampaignScreenIgui(screen_w, screen_h, CAMPAIGN_CTA_PANEL);
    Rectangle title_box = CampaignIguiRect(panel, CAMPAIGN_CTA_TITLE);
    Rectangle body_box = CampaignIguiRect(panel, CAMPAIGN_CTA_BODY);
    Rectangle icon_box = CampaignIguiRect(panel, CAMPAIGN_CTA_ICON);
    UiDrawTexture(c->cta_bg, panel, WHITE);

    const char *title = m->title;
    float title_size = title_box.height * CAMPAIGN_CTA_TITLE_OF_LABEL;
    float title_w = MeasureTextEx(c->font, title, title_size, 1.0f).x;
    if (title_w > title_box.width) title_size *= title_box.width / title_w;
    UiDrawTextLeftMiddle(c->font, title, title_box, title_size, (Color){235, 232, 220, 255});

    Font body_font = GuiFont(FNT_MENU);
    const char *body = LocGet(TextFormat("%s_CTA", m->id));
    float body_size =
        CampaignFitWrappedSize(body_font, body, body_box, CAMPAIGN_CTA_BODY_SIZE, CAMPAIGN_CTA_BODY_MIN_SIZE);
    HeroesDrawRich(body_font, body, body_box, body_size, (Color){214, 210, 198, 255});

    UiDrawTexture(c->tray_node, icon_box, WHITE);
    UiDrawTexture(m->tray_icon, icon_box, WHITE);
    return CampaignIguiRect(panel, CAMPAIGN_CTA_INFO);
}

static Rectangle CampaignAboutRect(Rectangle rect, float offset_x) {
    rect.x += offset_x;
    rect.y = UiCentredY(rect.y);
    return rect;
}

static void CampaignDrawFittedText(Font font, const char *text, Rectangle box, float size, Color color) {
    HeroesDrawRich(font, text, box, CampaignFitWrappedSize(font, text, box, size, CAMPAIGN_ABOUT_MIN_TEXT_SIZE), color);
}

static void CampaignDrawAbout(Campaign *c, const PlayerCampaignProgress *pcp, int map_index, float screen_w) {
    const CampaignMap *m = &c->maps[map_index];
    float dx = (screen_w - CAMPAIGN_ABOUT_CANVAS_WIDTH) * 0.5f;
    Rectangle root = UiRoot();
    DrawRectangle(0, (int)HUD_TOP_HEIGHT, (int)root.width, (int)(root.height - HUD_TOP_HEIGHT), (Color){0, 0, 0, 150});

    Rectangle frame = CampaignAboutRect(CAMPAIGN_ABOUT_FRAME, dx);
    UiDrawTexture(c->about_frame, frame, WHITE);
    UiDrawTexture(c->about_tab_left, CampaignAboutRect(CAMPAIGN_ABOUT_TAB_LEFT, dx), WHITE);
    UiDrawTexture(c->about_tab_middle, CampaignAboutRect(CAMPAIGN_ABOUT_TAB_MIDDLE, dx), WHITE);
    UiDrawTexture(c->about_tab_right, CampaignAboutRect(CAMPAIGN_ABOUT_TAB_RIGHT, dx), WHITE);

    Color text = {235, 232, 220, 255};
    Color muted = {196, 192, 182, 255};
    const char *title = m->title;
    Rectangle title_box = CampaignAboutRect(CAMPAIGN_ABOUT_TITLE, dx);
    float title_size = CAMPAIGN_ABOUT_TITLE_SIZE;
    float title_w = MeasureTextEx(c->font, title, title_size, 1.0f).x;
    if (title_w > title_box.width) title_size *= title_box.width / title_w;
    UiDrawTextCentered(c->font, title, title_box, title_size, text);
    UiDrawTextCentered(
        c->font,
        LocGet("HELPOVERLAY_TAB_ABOUT"),
        CampaignAboutRect(CAMPAIGN_ABOUT_TAB_ABOUT_LABEL, dx),
        CAMPAIGN_ABOUT_TAB_SIZE,
        text
    );
    UiDrawTextCentered(
        c->font,
        LocGet("HELPOVERLAY_TAB_HELP"),
        CampaignAboutRect(CAMPAIGN_ABOUT_TAB_HELP_LABEL, dx),
        CAMPAIGN_ABOUT_TAB_SIZE,
        muted
    );

    UiDrawTexture(c->about_quote_bg, CampaignAboutRect(CAMPAIGN_ABOUT_QUOTE_BG, dx), WHITE);
    UiDrawTexture(m->about_quote, CampaignAboutRect(CAMPAIGN_ABOUT_QUOTE_ART, dx), WHITE);
    Font body = GuiFont(FNT_MENU);
    CampaignDrawFittedText(
        body,
        LocGet(TextFormat("%s_quote", m->id)),
        CampaignAboutRect(CAMPAIGN_ABOUT_QUOTE_TEXT, dx),
        CAMPAIGN_ABOUT_QUOTE_SIZE,
        muted
    );
    UiDrawTexture(m->about_thumbnail, CampaignAboutRect(CAMPAIGN_ABOUT_MAP_ART, dx), WHITE);
    CampaignDrawFittedText(
        body,
        LocGet(TextFormat("%s_desc", m->id)),
        CampaignAboutRect(CAMPAIGN_ABOUT_TEXT, dx),
        CAMPAIGN_ABOUT_TEXT_SIZE,
        text
    );

    UiDrawTexture(c->about_cta_bg, CampaignAboutRect(CAMPAIGN_ABOUT_CTA_BG, dx), WHITE);
    CampaignDrawFittedText(
        c->font,
        LocGet(TextFormat("%s_details_CTA", m->id)),
        CampaignAboutRect(CAMPAIGN_ABOUT_CTA_TEXT, dx),
        CAMPAIGN_ABOUT_CTA_SIZE,
        text
    );
    int boss = CampaignBossNodeIndex(c, map_index);
    if (boss >= 0) {
        UiDrawTextCentered(
            c->font,
            TextFormat("+%d", m->nodes[boss].first_win_gems),
            CampaignAboutRect(CAMPAIGN_ABOUT_GEMS_TEXT, dx),
            CAMPAIGN_ABOUT_CTA_SIZE,
            text
        );
        if (pcp->node_completions[map_index][boss] > 0)
            UiDrawTexture(c->about_collected, CampaignAboutRect(CAMPAIGN_ABOUT_COLLECTED, dx), WHITE);
    }

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) c->about_opening_release = false;
    bool interactive = !c->about_opening_release && !UiInputBlocked();
    Rectangle close = CampaignAboutRect(CAMPAIGN_ABOUT_CLOSE, dx);
    if (ButtonDrawEx(&c->about_close, close, interactive)) {
        c->about_open = false;
    } else if (
        interactive && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && UiMouse().y > HUD_TOP_HEIGHT &&
        !CheckCollisionPointRec(UiMouse(), frame)
    ) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        c->about_open = false;
    }
}

static int CampaignCurrentEnergyCost(const CampaignNode *node, int losses) {
    return losses > 0 ? 0 : node->energy_cost;
}

int CampaignNodeEnergyCost(const Campaign *c, const PlayerCampaignProgress *pcp, int map, int node) {
    return CampaignCurrentEnergyCost(&c->maps[map].nodes[node], pcp->node_losses[map][node]);
}

int CampaignSelectedEnergyCost(const Campaign *c, const PlayerCampaignProgress *pcp) {
    return CampaignNodeEnergyCost(c, pcp, pcp->campaign_index, c->selected_node);
}

const CampaignNode *CampaignFindNode(const Campaign *c, const char *map_id, int step) {
    for (int m = 0; m < c->map_count; m++) {
        if (strcmp(c->maps[m].id, map_id) != 0) continue;
        if (step >= 1 && step <= c->maps[m].node_count) return &c->maps[m].nodes[step - 1];
    }
    return NULL;
}

static void CampaignShowMapScene(Campaign *c, int map_index) {
    if (c->scene.loaded && c->scene_map == map_index) return;
    const CampaignMap *m = &c->maps[map_index];
    MapSceneUnload(&c->scene);
    MapSceneLoad(&c->scene, CAMPAIGN_SCENES_PATH, m->id);
    if (c->scene.node_count != m->node_count)
        TraceLog(LOG_FATAL, "Campaign: %s scene has %d nodes, not %d", m->id, c->scene.node_count, m->node_count);
    c->scene_map = map_index;
}

static void CampaignNodeSpan(const Campaign *c, float *out_lo, float *out_hi) {
    float lo = MapSceneNodeX(&c->scene, 0), hi = lo;
    for (int i = 1; i < c->scene.node_count; i++) {
        lo = fminf(lo, MapSceneNodeX(&c->scene, i));
        hi = fmaxf(hi, MapSceneNodeX(&c->scene, i));
    }
    *out_lo = lo;
    *out_hi = hi;
}

static void CampaignScrollLimits(const Campaign *c, float *out_lo, float *out_hi) {
    CampaignNodeSpan(c, out_lo, out_hi);
    *out_lo -= CAMPAIGN_SWIPE_MARGIN;
    *out_hi += CAMPAIGN_SWIPE_MARGIN;
}

static float CampaignEaseOutExpo(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return 1.0f - powf(2.0f, -10.0f * t);
}

static void CampaignReturnFromEdge(Campaign *c, bool steered, float dt) {
    CampaignEdgeReturn *edge = &c->edge_return;
    if (steered) {
        edge->active = false;
        return;
    }
    if (!edge->active) {
        float lo = 0.0f, hi = 0.0f;
        CampaignNodeSpan(c, &lo, &hi);
        float rest = Clamp(c->scroll_x, lo, hi);
        if (rest == c->scroll_x) return;
        float duration = fabsf(rest - c->scroll_x) / CAMPAIGN_EDGE_RETURN_UNITS_PER_SECOND;
        *edge = (CampaignEdgeReturn){.active = true, .from = c->scroll_x, .to = rest, .duration = duration};
    }
    edge->elapsed += dt;
    c->scroll_x = Lerp(edge->from, edge->to, CampaignEaseOutExpo(edge->elapsed / edge->duration));
    if (edge->elapsed >= edge->duration) edge->active = false;
}

static bool CampaignInputLocked(const Campaign *c) {
    return (c->progression_active && !c->progression_camera_finished) || UiInputBlocked() || c->about_open;
}

static CampaignSwipeEdge CampaignSwipeEdgeAt(float scroll_x, float lo, float hi) {
    if (scroll_x <= lo) return CAMPAIGN_SWIPE_AT_LOW_EDGE;
    if (scroll_x >= hi) return CAMPAIGN_SWIPE_AT_HIGH_EDGE;
    return CAMPAIGN_SWIPE_INSIDE;
}

static bool CampaignSwipeEndedAtEdge(const Campaign *c) {
    return c->swipe_edge != CAMPAIGN_SWIPE_INSIDE && c->dragged && !c->press_blocked && !CampaignInputLocked(c) &&
           IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static int CampaignSwipeMapStep(const Campaign *c) {
    bool path_ends_high = MapSceneNodeX(&c->scene, c->scene.node_count - 1) > MapSceneNodeX(&c->scene, 0);
    bool toward_path_end = (c->swipe_edge == CAMPAIGN_SWIPE_AT_HIGH_EDGE) == path_ends_high;
    return toward_path_end ? 1 : -1;
}

static void CampaignSwipeToNeighbourMap(Campaign *c, PlayerCampaignProgress *pcp) {
    bool ended_at_edge = CampaignSwipeEndedAtEdge(c) && c->scene.loaded && c->scene_map == pcp->campaign_index;
    if (!ended_at_edge) return;
    int step = CampaignSwipeMapStep(c);
    int map = pcp->campaign_index + step;
    c->swipe_edge = CAMPAIGN_SWIPE_INSIDE;
    if (!CampaignMapUnlocked(c, pcp, map)) return;
    GuiSoundPlay(SND_MAP_TRANSITION);
    CampaignSelectMap(c, pcp, map);
    c->arrive_at_first_node = step > 0;
}

static float CampaignEaseInOutQuart(float t) {
    if (t < 0.5f) return 8.0f * powf(t, 4.0f);
    return 1.0f - 8.0f * powf(t - 1.0f, 4.0f);
}

static void CampaignIndicatorFadeTo(CampaignIndicatorFade *fade, float target) {
    if (fade->target == target) return;
    *fade = (CampaignIndicatorFade){.opacity = fade->opacity, .from = fade->opacity, .target = target};
}

static void CampaignIndicatorFadeUpdate(CampaignIndicatorFade *fade, float dt) {
    if (fade->opacity == fade->target) return;
    fade->elapsed += dt;
    float progress = fminf(fade->elapsed / CAMPAIGN_MAP_INDICATOR_FADE_SECONDS, 1.0f);
    fade->opacity = Lerp(fade->from, fade->target, CampaignEaseInOutQuart(progress));
}

static void CampaignAimMapIndicators(Campaign *c, const PlayerCampaignProgress *pcp, bool swiping) {
    int step = swiping && c->swipe_edge != CAMPAIGN_SWIPE_INSIDE ? CampaignSwipeMapStep(c) : 0;
    bool neighbour = step != 0 && CampaignMapUnlocked(c, pcp, pcp->campaign_index + step);
    CampaignIndicatorFadeTo(&c->previous_map_indicator, neighbour && step < 0 ? 1.0f : 0.0f);
    CampaignIndicatorFadeTo(&c->next_map_indicator, neighbour && step > 0 ? 1.0f : 0.0f);
}

static void CampaignUpdateMapIndicators(Campaign *c, float dt) {
    CampaignIndicatorFadeUpdate(&c->previous_map_indicator, dt);
    CampaignIndicatorFadeUpdate(&c->next_map_indicator, dt);
    bool shown = c->previous_map_indicator.opacity > 0.0f || c->next_map_indicator.opacity > 0.0f;
    c->map_indicator_pulse_time = shown ? c->map_indicator_pulse_time + dt : 0.0f;
}

static void CampaignDrawMapIndicator(
    const Campaign *c,
    const CampaignIndicatorFade *fade,
    CampaignIguiAnchor anchor,
    float rotation,
    float screen_w,
    float screen_h
) {
    if (fade->opacity <= 0.0f) return;
    Rectangle box = CampaignScreenIgui(screen_w, screen_h, anchor);
    float scale = 1.0f + sinf(c->map_indicator_pulse_time) * CAMPAIGN_MAP_INDICATOR_PULSE;
    Vector2 size = {box.width * scale, box.height * scale};
    Rectangle dst = {box.x + box.width * 0.5f, box.y + box.height * 0.5f, size.x, size.y};
    Texture2D arrow = c->map_indicator_arrow;
    Rectangle src = {0.0f, 0.0f, (float)arrow.width, (float)arrow.height};
    DrawTexturePro(arrow, src, dst, Vector2Scale(size, 0.5f), rotation, Fade(WHITE, fade->opacity));
}

static void CampaignDrawMapIndicators(const Campaign *c, float screen_w, float screen_h) {
    CampaignDrawMapIndicator(
        c,
        &c->previous_map_indicator,
        CAMPAIGN_PREVIOUS_MAP_INDICATOR,
        CAMPAIGN_PREVIOUS_MAP_ARROW_ROTATION,
        screen_w,
        screen_h
    );
    CampaignDrawMapIndicator(
        c, &c->next_map_indicator, CAMPAIGN_NEXT_MAP_INDICATOR, CAMPAIGN_NEXT_MAP_ARROW_ROTATION, screen_w, screen_h
    );
}

void CampaignDrawWorld(Campaign *c, PlayerCampaignProgress *pcp) {
    float dt = GetFrameTime();
    if (pcp->campaign_index < 0 || pcp->campaign_index >= c->map_count ||
        !CampaignMapUnlocked(c, pcp, pcp->campaign_index))
        pcp->campaign_index = 0;
    CampaignSwipeToNeighbourMap(c, pcp);
    pcp->node_index = CampaignFrontierForMap(c, pcp, pcp->campaign_index);
    CampaignMap *m = &c->maps[pcp->campaign_index];
    CampaignShowMapScene(c, pcp->campaign_index);
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
        int arrival_node = c->arrive_at_first_node ? 0 : pcp->node_index;
        c->selected_node = c->progression_active ? c->progression_to_node : arrival_node;
        c->arrive_at_first_node = false;
        c->previous_map_indicator = (CampaignIndicatorFade){0};
        c->next_map_indicator = (CampaignIndicatorFade){0};
    }
    if (c->selected_node >= m->node_count) c->selected_node = m->node_count - 1;

    float lo = 0.0f, hi = 0.0f;
    CampaignScrollLimits(c, &lo, &hi);
    float target = Clamp(MapSceneNodeX(&c->scene, c->selected_node), lo, hi);
    if (c->progression_active && !c->progression_camera_finished && c->progression_to_map == pcp->campaign_index) {
        bool same_map = c->progression_from_map == c->progression_to_map;
        int from_node = same_map ? Clamp(c->progression_from_node, 0, m->node_count - 1) : m->node_count - 1;
        float from_scroll = Clamp(MapSceneNodeX(&c->scene, from_node), lo, hi);
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
        c->scroll_x += (target - c->scroll_x) * fminf(1.0f, dt * CAMPAIGN_FOCUS_SPEED);
        if (fabsf(target - c->scroll_x) < CAMPAIGN_FOCUS_SETTLED) c->scroll_node = c->selected_node;
    }

    bool input_locked = CampaignInputLocked(c);
    CampaignTrayLayout tray = CampaignTrayLayoutFor(screen_w, screen_h);
    Rectangle toggle_rect = CampaignTrayToggleRect(screen_w, screen_h);
    bool over_tray = c->tray_open && UiMouse().y >= tray.list.y;
    if (!input_locked && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        c->press_pos = UiMouse();
        c->dragged = false;
        c->tray_pressed = over_tray;
        c->press_blocked = over_tray || CheckCollisionPointRec(c->press_pos, toggle_rect);
        c->swipe_edge = CAMPAIGN_SWIPE_INSIDE;
    }
    bool swiping = !input_locked && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !c->press_blocked;
    if (!input_locked && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (fabsf(UiMouse().x - c->press_pos.x) > 6.0f) c->dragged = true;
        if (swiping) {
            c->scroll_x += UiMouseDelta().x / screen_w * CAMPAIGN_SWIPE_UNITS_PER_SCREEN;
            c->scroll_node = c->selected_node;
        }
    }
    float wheel = !input_locked && !over_tray ? GetMouseWheelMove() : 0.0f;
    c->scroll_x = Clamp(c->scroll_x + wheel * CAMPAIGN_WHEEL_UNITS, lo, hi);
    if (swiping) c->swipe_edge = CampaignSwipeEdgeAt(c->scroll_x, lo, hi);
    CampaignAimMapIndicators(c, pcp, swiping);
    CampaignUpdateMapIndicators(c, dt);
    bool focusing = c->scroll_node != c->selected_node;
    CampaignReturnFromEdge(c, swiping || wheel != 0.0f || focusing || map_changed, dt);

    MapSceneSetCameraX(&c->scene, c->scroll_x);
    MapSceneUpdate(&c->scene, dt);
    MapSceneDraw(&c->scene);
}

CampaignAction CampaignDraw(Campaign *c, PlayerCampaignProgress *pcp) {
    CampaignAction action = CAMPAIGN_NONE;
    if (!c->scene.loaded || c->scene_map != pcp->campaign_index) return action;
    float dt = GetFrameTime();
    CampaignMap *m = &c->maps[pcp->campaign_index];
    float screen_w = UiWidth();
    float screen_h = UiHeight();
    bool progression_input_locked = CampaignInputLocked(c);
    CampaignTrayLayout tray = CampaignTrayLayoutFor(screen_w, screen_h);
    Rectangle toggle_rect = CampaignTrayToggleRect(screen_w, screen_h);

    bool tap =
        !progression_input_locked && !c->dragged && !c->press_blocked && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
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
        bool selected = j == c->selected_node && !c->tray_open;
        Texture2D icon = CampaignNodeIcon(c, m, j, frontier, completions, selected);
        Vector2 centre = MapSceneNodeScreen(&c->scene, j);
        Rectangle dst = MapSceneNodeIcon(&c->scene, j);
        dst.x += dst.width * (1.0f - flip_scale) * 0.5f;
        dst.width *= flip_scale;
        if (transition_phase >= 0.0f && transition_phase <= 1.0f) {
            float pulse = sinf(transition_phase * PI);
            float glow_size = 92.0f + 70.0f * pulse;
            Texture2D glow = GuiTex(GUI_NODE_UNLOCK_GLOW);
            BeginBlendMode(BLEND_ADDITIVE);
            UiDrawTexture(
                glow,
                (Rectangle){centre.x - glow_size * 0.5f, centre.y - glow_size * 0.5f, glow_size, glow_size},
                (Color){246, 216, 111, (unsigned char)(175.0f * pulse)}
            );
            EndBlendMode();
        }
        UiDrawTexture(icon, dst, WHITE);

        int energy_cost = CampaignCurrentEnergyCost(&m->nodes[j], pcp->node_losses[pcp->campaign_index][j]);
        bool node_locked = j > frontier && completions <= 0;
        if (!node_locked && energy_cost > 0) {
            Rectangle banner = MapSceneNodeBanner(&c->scene, j);
            UiDrawTexture(c->icons.energy, banner, WHITE);
            UiDrawTexture(c->energy_flask, MapSceneNodeFlask(&c->scene, j), WHITE);
            const char *txt = TextFormat("%d", energy_cost);
            float fs = banner.height * CAMPAIGN_ENERGY_TEXT_OF_BANNER;
            Vector2 tsz = MeasureTextEx(c->font, txt, fs, 1.0f);
            Vector2 anchor = MapSceneNodeText(&c->scene, j);
            DrawTextEx(c->font, txt, (Vector2){anchor.x - tsz.x / 2.0f, anchor.y - tsz.y / 2.0f}, fs, 1.0f, WHITE);
        }

        if (tap && CheckCollisionPointRec(mouse, dst)) {
            GuiSoundPlay(SND_PVE_NODE_SELECTED);
            c->selected_node = j;
            c->tray_open = false;
        }
    }

    if (c->tray_open) {
        int picked = CampaignDrawTray(c, pcp, &tray, !progression_input_locked, dt);
        if (picked >= 0 && picked != pcp->campaign_index) {
            GuiSoundPlay(SND_MAP_TRANSITION);
            CampaignSelectMap(c, pcp, picked);
        }
        Rectangle info = CampaignDrawTrayCallToAction(c, &c->maps[pcp->campaign_index], screen_w, screen_h);
        if (CampaignDrawHelpButton(c, info, !progression_input_locked)) CampaignOpenAbout(c);
    }

    if (ButtonDrawEx(&c->change_map, toggle_rect, !progression_input_locked)) {
        GuiSoundPlay(SND_UI_TAB);
        c->tray_open = !c->tray_open;
        if (c->tray_open) CampaignTrayRevealMap(c, &tray, CampaignUnlockedMapCount(c, pcp), pcp->campaign_index);
    }

    if (!c->tray_open) {
        {
            bool unlocked = c->selected_node <= pcp->node_index;
            Rectangle enter = {
                screen_w - 264.0f, screen_h - 72.0f, (float)c->enter_map.idle.width, (float)c->enter_map.idle.height
            };
            TutorialAnchorSet(TUTORIAL_ANCHOR_MAP_ENTER, enter);
            if (ButtonDrawEx(&c->enter_map, enter, !progression_input_locked) && unlocked) action = CAMPAIGN_ENTER;
        }

        Rectangle lore = {screen_w - c->lore.width / 1.3f, 72.0f, c->lore.width / 1.3f, c->lore.height / 1.3f};
        {
            Rectangle src = {2, 0, c->lore.width, c->lore.height};
            DrawTexturePro(c->lore, src, lore, (Vector2){0, 0}, 0.0f, WHITE);
        }

        CampaignNode *n = &m->nodes[c->selected_node];
        {
            Rectangle help = {screen_w - 356.0f, 106, 48, 48};
            if (CampaignDrawHelpButton(c, help, !progression_input_locked)) CampaignOpenAbout(c);
            float left = help.x + help.width + CAMPAIGN_DETAILS_TITLE_GAP;
            Rectangle title = {left, help.y, lore.x + lore.width - CAMPAIGN_DETAILS_TITLE_GAP - left, help.height};
            float size = CAMPAIGN_DETAILS_TITLE_SIZE;
            float width = MeasureTextEx(c->font, n->name, size, 1.0f).x;
            if (width > title.width) size *= title.width / width;
            UiDrawTextLeftMiddle(c->font, n->name, title, size, (Color){235, 232, 220, 255});
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
    }

    CampaignDrawMapIndicators(c, screen_w, screen_h);
    if (c->progression_active) {
        float duration =
            c->progression_from_map == c->progression_to_map ? (c->progression_unlock_next ? 3.0f : 2.0f) : 3.5f;
        if (c->progression_time >= duration) c->progression_active = false;
    }
    if (c->about_open) CampaignDrawAbout(c, pcp, pcp->campaign_index, screen_w);
    return action;
}
