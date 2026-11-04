#define TUTORIAL_SCRIPT_PATH "data/ftue_script.json"
#define TUTORIAL_REWARDS_PATH "data/tutorial_rewards.json"
#define TUTORIAL_MAX_STEPS 96
#define TUTORIAL_TEXT_KEY_CAPACITY 64
#define TUTORIAL_MAP_ID_CAPACITY 32
#define TUTORIAL_REQUEST_CAPACITY 4
#define TUTORIAL_TAP_GRACE_SECONDS 0.35f
#define TUTORIAL_DESIGN_WIDTH 1280.0f
#define TUTORIAL_TEXT_SIZE 25.0f
#define TUTORIAL_TEXT_MIN_SIZE 16.0f
#define TUTORIAL_TEXT_COLOR ((Color){240, 236, 222, 255})
#define TUTORIAL_ARROW_SIZE 110.0f
#define TUTORIAL_ARROW_BOB 14.0f
#define TUTORIAL_UNIT_ARROW_SIZE 64.0f
#define TUTORIAL_SMALL_ARROW_POINTS_DOWN 90.0f
#define TUTORIAL_HAND_SIZE 160.0f
#define TUTORIAL_GLOW_COLOR ((Color){255, 196, 64, 255})
#define TUTORIAL_GLOW_THICKNESS 5.0f
#define TUTORIAL_SCRIM_COLOR ((Color){0, 0, 0, 150})
#define FTUE_GLOW_PULSE_DURATION 0.75f
#define FTUE_GLOW_MIN_OPACITY 0.25f
#define ARROW_DRAG_TIMER_SCALE 1.5f
#define GUIDE_FADE_IN_DURATION 1.0f
#define GUIDE_SLIDE_IN_DURATION 0.25f
#define GUIDE_LABEL_X_WITH_GUIDE_ON_LEFT 0.8f
#define GUIDE_LABEL_X_WITH_GUIDE_ON_RIGHT 0.1f
#define TUTORIAL_SIDE_TEXT_PADDING 18.0f
#define TUTORIAL_TEXT_MIN_COLUMN 200.0f

typedef enum { TUTORIAL_FUNNEL_MAIN, TUTORIAL_FUNNEL_CONSUME, TUTORIAL_FUNNEL_COUNT } TutorialFunnel;

static const char *const TUTORIAL_FUNNEL_KEYS[TUTORIAL_FUNNEL_COUNT] = {"main", "consume"};

typedef enum {
    TUTORIAL_SIGNAL_ANY,
    TUTORIAL_SIGNAL_MAIN_INIT,
    TUTORIAL_SIGNAL_MAIN_SCREEN,
    TUTORIAL_SIGNAL_BATTLE_MAP,
    TUTORIAL_SIGNAL_BATTLE_MATCHMAKING,
    TUTORIAL_SIGNAL_BATTLE,
    TUTORIAL_SIGNAL_NODE_COMPLETION,
    TUTORIAL_SIGNAL_BATTLE_RESULTS,
    TUTORIAL_SIGNAL_STORE,
    TUTORIAL_SIGNAL_PACK_OPENING,
    TUTORIAL_SIGNAL_EDIT_FORMATION,
    TUTORIAL_SIGNAL_CONSUME,
    TUTORIAL_SIGNAL_UNSUPPORTED,
} TutorialSignal;

typedef enum { TUTORIAL_TEXT_TOP, TUTORIAL_TEXT_BOTTOM, TUTORIAL_TEXT_RIGHT, TUTORIAL_TEXT_LEFT } TutorialTextBar;
typedef enum { TUTORIAL_GUIDE_SMALL, TUTORIAL_GUIDE_MEDIUM, TUTORIAL_GUIDE_LARGE } TutorialGuideSize;
typedef enum {
    TUTORIAL_MOOD_DEFAULT,
    TUTORIAL_MOOD_CONTEMPLATIVE,
    TUTORIAL_MOOD_SATISFIED,
    TUTORIAL_MOOD_HAPPY,
} TutorialGuideMood;
typedef enum { TUTORIAL_ARROW_DOWN, TUTORIAL_ARROW_RIGHT, TUTORIAL_ARROW_UP, TUTORIAL_ARROW_HIDDEN } TutorialArrow;

typedef enum {
    TUTORIAL_ICON_NONE,
    TUTORIAL_ICON_HEALTH_POWER_SPEED,
    TUTORIAL_ICON_HEALTH,
    TUTORIAL_ICON_CRIT,
    TUTORIAL_ICON_FACTIONS,
    TUTORIAL_ICON_ENERGY,
    TUTORIAL_ICON_STAMINA,
    TUTORIAL_ICON_SPEEDS,
    TUTORIAL_ICON_STUN,
    TUTORIAL_ICON_BATTLE,
    TUTORIAL_ICON_GIANT,
    TUTORIAL_ICON_COUNT
} TutorialIcon;

typedef enum {
    TUTORIAL_ACTION_NONE,
    TUTORIAL_ACTION_ABOUT_SCREEN,
    TUTORIAL_ACTION_START_INTRO_BATTLE,
    TUTORIAL_ACTION_EXIT_INTRO_BATTLE,
    TUTORIAL_ACTION_FLIP_BOSS_NODE,
    TUTORIAL_ACTION_AWARD_GEM_PACK,
    TUTORIAL_ACTION_UNSUPPORTED,
} TutorialAction;

typedef enum {
    TUTORIAL_UNITS_NONE,
    TUTORIAL_UNITS_PLAYER_FRONT_ROW,
    TUTORIAL_UNITS_PLAYER_BACK_ROW,
    TUTORIAL_UNITS_ENEMY_UNCOMMON,
    TUTORIAL_UNITS_ALL_ENEMIES,
    TUTORIAL_UNITS_ENEMY_RARE,
    TUTORIAL_UNITS_PLAYER_POSITION_1,
} TutorialUnitHighlight;

typedef enum { TUTORIAL_FUNNEL_END_NONE, TUTORIAL_FUNNEL_END_MAIN, TUTORIAL_FUNNEL_END_CONSUME } TutorialFunnelEnd;

typedef struct {
    float step;
    char text[TUTORIAL_TEXT_KEY_CAPACITY];
    TutorialSignal signal;
    TutorialEvent event;
    TutorialTextBar text_bar;
    bool guide_on_left;
    TutorialGuideSize guide_size;
    TutorialGuideMood guide_mood;
    TutorialIcon icon;
    TutorialAnchor highlight;
    TutorialArrow arrow;
    bool drag_arrow;
    Rarity drag_rarity;
    bool drag_giant;
    bool highlight_required;
    TutorialAction action;
    bool award_giant;
    bool hold_battle;
    TutorialUnitHighlight highlight_units;
    int highlight_position;
    TutorialAnchor hidden;
    char map_id[TUTORIAL_MAP_ID_CAPACITY];
    int map_node;
    bool restore_point;
    bool restore_point_for_server;
    bool mock_store;
    TutorialFunnelEnd funnel_end;
} TutorialStep;

typedef enum {
    TUTORIAL_REQUEST_NONE,
    TUTORIAL_REQUEST_START_INTRO_BATTLE,
    TUTORIAL_REQUEST_EXIT_INTRO_BATTLE,
    TUTORIAL_REQUEST_SELECT_MAP_NODE,
    TUTORIAL_REQUEST_AWARD_GIANT,
    TUTORIAL_REQUEST_AWARD_GEM_PACK,
    TUTORIAL_REQUEST_GO_HOME,
    TUTORIAL_REQUEST_SAVE,
    TUTORIAL_REQUEST_MAIN_FINISHED,
    TUTORIAL_REQUEST_CONSUME_FINISHED,
} TutorialRequestKind;

typedef struct {
    TutorialRequestKind kind;
    char map_id[TUTORIAL_MAP_ID_CAPACITY];
    int node;
} TutorialRequest;

typedef struct {
    TutorialSignal signal;
    const Player *squad;
    const BattleRoster *enemy;
} TutorialFrame;

typedef struct {
    TutorialStep steps[TUTORIAL_FUNNEL_COUNT][TUTORIAL_MAX_STEPS];
    int step_count[TUTORIAL_FUNNEL_COUNT];
    TutorialPackReward packs[TUTORIAL_PACK_REWARD_CAPACITY];
    int pack_count;
    char giant[TUTORIAL_ID_CAPACITY];
    int gem_pack_gems;
    char gift_heroes[TUTORIAL_PACK_REWARD_HEROES][TUTORIAL_ID_CAPACITY];
    int gift_hero_count;
    char gift_runes[TUTORIAL_PACK_REWARD_HEROES][TUTORIAL_ID_CAPACITY];
    int gift_rune_count;

    bool active;
    TutorialFunnel funnel;
    int index;
    bool event_seen;
    bool entered;
    bool visible;
    float visible_time;
    float time;
    bool press_armed;
    bool button_window_open;
    Rectangle button_window;
    int pending_pack;
    TutorialRequest requests[TUTORIAL_REQUEST_CAPACITY];
    int request_count;
} Tutorial;

typedef struct {
    const char *name;
    int value;
} TutorialName;

static const TutorialName TUTORIAL_SIGNAL_NAMES[] = {
    {"None", TUTORIAL_SIGNAL_ANY},
    {"MainInitState", TUTORIAL_SIGNAL_MAIN_INIT},
    {"MainScreenState", TUTORIAL_SIGNAL_MAIN_SCREEN},
    {"BattleMapScreenState", TUTORIAL_SIGNAL_BATTLE_MAP},
    {"BattleMatchmakingScreenState", TUTORIAL_SIGNAL_BATTLE_MATCHMAKING},
    {"BattleScreenState", TUTORIAL_SIGNAL_BATTLE},
    {"PVENodeCompletionScreenState", TUTORIAL_SIGNAL_NODE_COMPLETION},
    {"BattleResultsPopupView", TUTORIAL_SIGNAL_BATTLE_RESULTS},
    {"StoreTabsPopupView", TUTORIAL_SIGNAL_STORE},
    {"PackOpeningScreenState", TUTORIAL_SIGNAL_PACK_OPENING},
    {"EditFormationScreenState", TUTORIAL_SIGNAL_EDIT_FORMATION},
    {"ConsumeScreenState", TUTORIAL_SIGNAL_CONSUME},
};

static const TutorialName TUTORIAL_EVENT_NAMES[] = {
    {"None", TUTORIAL_EVENT_NONE},
    {"UnitPurchaseSucceeded", TUTORIAL_EVENT_UNIT_PURCHASE_SUCCEEDED},
    {"UnitPurchaseFinished", TUTORIAL_EVENT_UNIT_PURCHASE_FINISHED},
    {"UnitPurchaseNextButtonAppeared", TUTORIAL_EVENT_UNIT_PURCHASE_NEXT_BUTTON_APPEARED},
    {"UnitAddedToSquad", TUTORIAL_EVENT_UNIT_ADDED_TO_SQUAD},
    {"GiantUnitAddedToSquad", TUTORIAL_EVENT_GIANT_UNIT_ADDED_TO_SQUAD},
    {"UnitDetailsShowing", TUTORIAL_EVENT_UNIT_DETAILS_SHOWING},
    {"UnitDetailsClosed", TUTORIAL_EVENT_UNIT_DETAILS_CLOSED},
    {"TargetUnitAdded", TUTORIAL_EVENT_TARGET_UNIT_ADDED},
    {"SeedUnitAdded", TUTORIAL_EVENT_SEED_UNIT_ADDED},
    {"VictoryOrDefeatEvent", TUTORIAL_EVENT_VICTORY_OR_DEFEAT},
    {"BattleMapNodePanFinished", TUTORIAL_EVENT_BATTLE_MAP_NODE_PAN_FINISHED},
};

static const TutorialName TUTORIAL_HIGHLIGHT_NAMES[] = {
    {"None", TUTORIAL_ANCHOR_NONE},
    {"MainScreen_QuestButton", TUTORIAL_ANCHOR_HOME_QUESTS},
    {"MainScreen_StoreButton", TUTORIAL_ANCHOR_HOME_STORE},
    {"PveMapScreen_EnterButton", TUTORIAL_ANCHOR_MAP_ENTER},
    {"PveMapScreen_EnergyBar", TUTORIAL_ANCHOR_HUD_ENERGY},
    {"MainScreen_StaminaBar", TUTORIAL_ANCHOR_HUD_STAMINA},
    {"BattleMatchmakingScreen_FightButton", TUTORIAL_ANCHOR_BATTLE_FIGHT},
    {"BattleMatchmakingScreen_RetreatButton", TUTORIAL_ANCHOR_BATTLE_RETREAT},
    {"BattleResultsPopup_NextQuestButton", TUTORIAL_ANCHOR_RESULTS_CONTINUE},
    {"BattleResultsPopup_StoreButton", TUTORIAL_ANCHOR_RESULTS_STORE},
    {"StoreTabsPopup_SilverPack", TUTORIAL_ANCHOR_STORE_SOLDIER_CARD},
    {"StoreTabsPopup_GoldPack", TUTORIAL_ANCHOR_STORE_CHAMPION_CARD},
    {"StoreTabsPopup_SilverPackButton", TUTORIAL_ANCHOR_STORE_SOLDIER_BUY},
    {"StoreTabsPopup_GoldPackButton", TUTORIAL_ANCHOR_STORE_CHAMPION_BUY},
    {"StoreTabsPopup_ConsumePack", TUTORIAL_ANCHOR_STORE_TUTORIAL_PACK_BUY},
    {"StoreTabsPopup_ConsumePackButton", TUTORIAL_ANCHOR_STORE_TUTORIAL_PACK_BUY},
    {"PackOpenScreen_SquadButton", TUTORIAL_ANCHOR_PACK_SQUAD},
    {"PackOpenScreen_ConsumeButton", TUTORIAL_ANCHOR_PACK_CONSUME},
    {"PackOpenScreen_NextButton", TUTORIAL_ANCHOR_PACK_NEXT},
    {"PackOpenScreen_RuneIcon", TUTORIAL_ANCHOR_PACK_RUNE},
    {"EditFormation_NewUnitButton", TUTORIAL_ANCHOR_HEROES_NEWEST_UNIT},
    {"UnitDetails_UnitPanelButton", TUTORIAL_ANCHOR_HEROES_DETAILS},
    {"EditFormation_QuestButton", TUTORIAL_ANCHOR_HEROES_QUEST},
    {"ConsumeScreen_HomeButton", TUTORIAL_ANCHOR_HUD_HOME},
    {"TrainingScreen_ConsumeButton", TUTORIAL_ANCHOR_CONSUME_BUTTON},
    {"TrainingScreen_SquadButton", TUTORIAL_ANCHOR_CONSUME_SQUAD},
};

static const TutorialName TUTORIAL_ICON_NAMES[] = {
    {"None", TUTORIAL_ICON_NONE},
    {"CenterHealthPowerSpeed", TUTORIAL_ICON_HEALTH_POWER_SPEED},
    {"RightHeath", TUTORIAL_ICON_HEALTH},
    {"RightTimes2", TUTORIAL_ICON_CRIT},
    {"FactionColors", TUTORIAL_ICON_FACTIONS},
    {"EnergyIcon", TUTORIAL_ICON_ENERGY},
    {"StaminaIcon", TUTORIAL_ICON_STAMINA},
    {"LeftSpeeds", TUTORIAL_ICON_SPEEDS},
    {"StunIcon", TUTORIAL_ICON_STUN},
    {"CenterBattle", TUTORIAL_ICON_BATTLE},
    {"CenterGiant", TUTORIAL_ICON_GIANT},
};

static const TutorialName TUTORIAL_ACTION_NAMES[] = {
    {"None", TUTORIAL_ACTION_NONE},
    {"ShowReactiveScreenAbout", TUTORIAL_ACTION_ABOUT_SCREEN},
    {"ShowFtueBattle", TUTORIAL_ACTION_START_INTRO_BATTLE},
    {"ForceExitFromFtueBattle", TUTORIAL_ACTION_EXIT_INTRO_BATTLE},
    {"BattleMapFlipBossNode", TUTORIAL_ACTION_FLIP_BOSS_NODE},
    {"AwardHiddenStorePack", TUTORIAL_ACTION_AWARD_GEM_PACK},
};

static const TutorialName TUTORIAL_UNIT_HIGHLIGHT_NAMES[] = {
    {"None", TUTORIAL_UNITS_NONE},
    {"PlayerFrontRow", TUTORIAL_UNITS_PLAYER_FRONT_ROW},
    {"PlayerBackRow", TUTORIAL_UNITS_PLAYER_BACK_ROW},
    {"EnemyUncommon", TUTORIAL_UNITS_ENEMY_UNCOMMON},
    {"AllEnemies", TUTORIAL_UNITS_ALL_ENEMIES},
    {"EnemyRare", TUTORIAL_UNITS_ENEMY_RARE},
    {"PlayerPos1", TUTORIAL_UNITS_PLAYER_POSITION_1},
    {"PlayerPos2", TUTORIAL_UNITS_PLAYER_POSITION_1},
    {"PlayerPos3", TUTORIAL_UNITS_PLAYER_POSITION_1},
    {"PlayerPos4", TUTORIAL_UNITS_PLAYER_POSITION_1},
};

static const TutorialName TUTORIAL_TEXT_BAR_NAMES[] = {
    {"Top", TUTORIAL_TEXT_TOP},
    {"Bottom", TUTORIAL_TEXT_BOTTOM},
    {"Right", TUTORIAL_TEXT_RIGHT},
    {"Left", TUTORIAL_TEXT_LEFT},
};

static const TutorialName TUTORIAL_GUIDE_SIZE_NAMES[] = {
    {"Small", TUTORIAL_GUIDE_SMALL},
    {"Medium", TUTORIAL_GUIDE_MEDIUM},
    {"Large", TUTORIAL_GUIDE_LARGE},
};

static const TutorialName TUTORIAL_GUIDE_MOOD_NAMES[] = {
    {"Default", TUTORIAL_MOOD_DEFAULT},
    {"Contemplative", TUTORIAL_MOOD_CONTEMPLATIVE},
    {"Satisfied", TUTORIAL_MOOD_SATISFIED},
    {"Happy", TUTORIAL_MOOD_HAPPY},
};

static const TutorialName TUTORIAL_ARROW_NAMES[] = {
    {"Down", TUTORIAL_ARROW_DOWN},
    {"Right", TUTORIAL_ARROW_RIGHT},
    {"Up", TUTORIAL_ARROW_UP},
    {"Hidden", TUTORIAL_ARROW_HIDDEN},
};

static const TutorialName TUTORIAL_RARITY_NAMES[] = {
    {"Undefined", TUTORIAL_NO_RARITY},
    {"Common", RARITY_COMMON},
    {"Uncommon", RARITY_UNCOMMON},
    {"Rare", RARITY_RARE},
    {"Epic", RARITY_EPIC},
    {"Legendary", RARITY_LEGENDARY},
    {"Mythic", RARITY_MYTHIC},
};

static const TutorialName TUTORIAL_FUNNEL_END_NAMES[] = {
    {"None", TUTORIAL_FUNNEL_END_NONE},
    {"Regular", TUTORIAL_FUNNEL_END_MAIN},
    {"Consume", TUTORIAL_FUNNEL_END_CONSUME},
};

#define TUTORIAL_LOOKUP(table, text, fallback)                                                                         \
    TutorialLookup((table), sizeof(table) / sizeof((table)[0]), (text), (fallback))

static int TutorialLookup(const TutorialName *table, int count, const char *text, int fallback) {
    if (!text || !text[0]) return table[0].value;
    for (int i = 0; i < count; i++)
        if (!strcmp(table[i].name, text)) return table[i].value;
    return fallback;
}

static const char *TutorialJsonText(const cJSON *json, const char *key, const char *fallback) {
    const char *text = JsonStr(json, key);
    return text[0] ? text : fallback;
}

static int TutorialPlayerPositionNumber(const char *name) {
    int number = 0;
    return sscanf(name, "PlayerPos%d", &number) == 1 ? number : 0;
}

static TutorialStep TutorialParseStep(const cJSON *json) {
    TutorialStep step = {0};
    cJSON *number = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "step");
    step.step = cJSON_IsNumber(number) ? (float)number->valuedouble : 0.0f;
    StrCopy(step.text, sizeof(step.text), JsonStr(json, "text"));
    step.signal = TUTORIAL_LOOKUP(TUTORIAL_SIGNAL_NAMES, JsonStr(json, "signal"), TUTORIAL_SIGNAL_UNSUPPORTED);
    step.event = TUTORIAL_LOOKUP(TUTORIAL_EVENT_NAMES, JsonStr(json, "event"), TUTORIAL_EVENT_COUNT);
    step.text_bar = TUTORIAL_LOOKUP(TUTORIAL_TEXT_BAR_NAMES, JsonStr(json, "text_bar"), TUTORIAL_TEXT_TOP);
    step.guide_on_left = JsonBool(json, "guide_on_left");
    step.guide_size = TUTORIAL_LOOKUP(TUTORIAL_GUIDE_SIZE_NAMES, JsonStr(json, "guide_size"), TUTORIAL_GUIDE_LARGE);
    step.guide_mood = TUTORIAL_LOOKUP(TUTORIAL_GUIDE_MOOD_NAMES, JsonStr(json, "guide_type"), TUTORIAL_MOOD_DEFAULT);
    step.icon = TUTORIAL_LOOKUP(TUTORIAL_ICON_NAMES, JsonStr(json, "extra_icon"), TUTORIAL_ICON_NONE);
    step.highlight = TUTORIAL_LOOKUP(TUTORIAL_HIGHLIGHT_NAMES, JsonStr(json, "highlight"), TUTORIAL_ANCHOR_NONE);
    step.arrow = TUTORIAL_LOOKUP(TUTORIAL_ARROW_NAMES, JsonStr(json, "arrow"), TUTORIAL_ARROW_DOWN);
    step.drag_arrow = JsonBool(json, "drag_arrow");
    step.drag_rarity = TUTORIAL_LOOKUP(TUTORIAL_RARITY_NAMES, JsonStr(json, "drag_unit_rarity"), TUTORIAL_NO_RARITY);
    step.drag_giant = !strcmp(JsonStr(json, "drag_item"), "GiantUnit");
    step.highlight_required = JsonBool(json, "highlight_required");
    step.action = TUTORIAL_LOOKUP(TUTORIAL_ACTION_NAMES, JsonStr(json, "special_action"), TUTORIAL_ACTION_UNSUPPORTED);
    step.award_giant = JsonBool(json, "award_giant");
    step.hold_battle = JsonBool(json, "hold_battle");
    const char *units = JsonStr(json, "highlight_units");
    step.highlight_units = TUTORIAL_LOOKUP(TUTORIAL_UNIT_HIGHLIGHT_NAMES, units, TUTORIAL_UNITS_NONE);
    step.highlight_position = TutorialPlayerPositionNumber(units);
    const char *hidden = JsonStr(json, "hidden_element");
    step.hidden = !strcmp(hidden, "BattleMatchmakingScreen_RetreatButton") ? TUTORIAL_ANCHOR_BATTLE_RETREAT
                  : !strcmp(hidden, "BattleMatchmakingScreen_FightButton") ? TUTORIAL_ANCHOR_BATTLE_FIGHT
                                                                           : TUTORIAL_ANCHOR_NONE;
    StrCopy(step.map_id, sizeof(step.map_id), JsonStr(json, "map_id"));
    step.map_node = JsonInt(json, "map_node_step");
    step.restore_point = JsonBool(json, "restore_point");
    step.restore_point_for_server = JsonBool(json, "restore_point_for_server");
    step.mock_store = strcmp(TutorialJsonText(json, "mock_store", "None"), "None") != 0;
    step.funnel_end = TUTORIAL_LOOKUP(TUTORIAL_FUNNEL_END_NAMES, JsonStr(json, "funnel_end"), TUTORIAL_FUNNEL_END_NONE);
    return step;
}

static void TutorialLoadPackReward(Tutorial *t, const char *id, const cJSON *json) {
    if (!cJSON_IsObject(json) || t->pack_count >= TUTORIAL_PACK_REWARD_CAPACITY) return;
    TutorialPackReward *reward = &t->packs[t->pack_count++];
    *reward = (TutorialPackReward){0};
    StrCopy(reward->pack_id, sizeof(reward->pack_id), id);
    cJSON *hero = NULL;
    cJSON_ArrayForEach(hero, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "heroes")) {
        if (!cJSON_IsString(hero) || reward->hero_count >= TUTORIAL_PACK_REWARD_HEROES) continue;
        StrCopy(reward->heroes[reward->hero_count++], TUTORIAL_ID_CAPACITY, hero->valuestring);
    }
    StrCopy(reward->rune, sizeof(reward->rune), JsonStr(json, "rune"));
}

static void TutorialLoadRewards(Tutorial *t) {
    cJSON *root = JsonLoadFile(TUTORIAL_REWARDS_PATH);
    if (!root) TraceLog(LOG_FATAL, "Tutorial: cannot read %s", TUTORIAL_REWARDS_PATH);
    cJSON *pack = NULL;
    cJSON_ArrayForEach(pack, cJSON_GetObjectItemCaseSensitive(root, "packs")) {
        TutorialLoadPackReward(t, pack->string, pack);
    }
    StrCopy(t->giant, sizeof(t->giant), JsonStr(cJSON_GetObjectItemCaseSensitive(root, "giant"), "hero"));
    t->gem_pack_gems = JsonInt(cJSON_GetObjectItemCaseSensitive(root, "hidden_gem_pack"), "gems");
    cJSON *gift = cJSON_GetObjectItemCaseSensitive(root, "parting_gift");
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(gift, "heroes")) {
        if (cJSON_IsString(item) && t->gift_hero_count < TUTORIAL_PACK_REWARD_HEROES)
            StrCopy(t->gift_heroes[t->gift_hero_count++], TUTORIAL_ID_CAPACITY, item->valuestring);
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(gift, "runes")) {
        if (cJSON_IsString(item) && t->gift_rune_count < TUTORIAL_PACK_REWARD_HEROES)
            StrCopy(t->gift_runes[t->gift_rune_count++], TUTORIAL_ID_CAPACITY, item->valuestring);
    }
    cJSON_Delete(root);
}

void TutorialLoad(Tutorial *t) {
    *t = (Tutorial){0};
    t->pending_pack = -1;
    cJSON *root = JsonLoadFile(TUTORIAL_SCRIPT_PATH);
    if (!root) TraceLog(LOG_FATAL, "Tutorial: cannot read %s", TUTORIAL_SCRIPT_PATH);
    cJSON *funnels = cJSON_GetObjectItemCaseSensitive(root, "funnels");
    for (int funnel = 0; funnel < TUTORIAL_FUNNEL_COUNT; funnel++) {
        cJSON *step = NULL;
        cJSON_ArrayForEach(step, cJSON_GetObjectItemCaseSensitive(funnels, TUTORIAL_FUNNEL_KEYS[funnel])) {
            if (t->step_count[funnel] >= TUTORIAL_MAX_STEPS)
                TraceLog(
                    LOG_FATAL, "Tutorial: more than %d %s steps", TUTORIAL_MAX_STEPS, TUTORIAL_FUNNEL_KEYS[funnel]
                );
            t->steps[funnel][t->step_count[funnel]++] = TutorialParseStep(step);
        }
    }
    cJSON_Delete(root);
    TutorialLoadRewards(t);
    TraceLog(
        LOG_INFO,
        "Tutorial: %d main and %d consume steps",
        t->step_count[TUTORIAL_FUNNEL_MAIN],
        t->step_count[TUTORIAL_FUNNEL_CONSUME]
    );
}

static const TutorialStep *TutorialCurrent(const Tutorial *t) {
    if (!t->active || t->index < 0 || t->index >= t->step_count[t->funnel]) return NULL;
    return &t->steps[t->funnel][t->index];
}

static void TutorialPush(Tutorial *t, TutorialRequest request) {
    if (t->request_count < TUTORIAL_REQUEST_CAPACITY) t->requests[t->request_count++] = request;
}

static void TutorialPushKind(Tutorial *t, TutorialRequestKind kind) {
    TutorialPush(t, (TutorialRequest){.kind = kind});
}

bool TutorialTakeRequest(Tutorial *t, TutorialRequest *out) {
    if (t->request_count == 0) return false;
    *out = t->requests[0];
    memmove(t->requests, t->requests + 1, (size_t)(t->request_count - 1) * sizeof(t->requests[0]));
    t->request_count--;
    return true;
}

static void TutorialBeginStep(Tutorial *t) {
    t->event_seen = false;
    t->entered = false;
    t->visible = false;
    t->visible_time = 0.0f;
    t->press_armed = false;
    t->button_window_open = false;
}

static void TutorialStartFunnel(Tutorial *t, TutorialFunnel funnel, int index) {
    t->active = true;
    t->funnel = funnel;
    t->index = index;
    TutorialBeginStep(t);
}

void TutorialResumeMain(Tutorial *t, const PlayerTutorial *state) {
    int index = 0;
    for (int i = 0; i < t->step_count[TUTORIAL_FUNNEL_MAIN]; i++) {
        const TutorialStep *step = &t->steps[TUTORIAL_FUNNEL_MAIN][i];
        if (step->restore_point && (int)step->step == state->restore_step) index = i;
    }
    TutorialStartFunnel(t, TUTORIAL_FUNNEL_MAIN, index);
    TraceLog(LOG_INFO, "Tutorial: main funnel from step %.1f", t->steps[TUTORIAL_FUNNEL_MAIN][index].step);
}

bool TutorialJumpTo(Tutorial *t, float step) {
    for (int funnel = 0; funnel < TUTORIAL_FUNNEL_COUNT; funnel++)
        for (int i = 0; i < t->step_count[funnel]; i++)
            if (fabsf(t->steps[funnel][i].step - step) < 0.01f) {
                TutorialStartFunnel(t, (TutorialFunnel)funnel, i);
                return true;
            }
    return false;
}

void TutorialStartConsume(Tutorial *t) {
    if (t->active || t->step_count[TUTORIAL_FUNNEL_CONSUME] == 0) return;
    TutorialStartFunnel(t, TUTORIAL_FUNNEL_CONSUME, 0);
    TraceLog(LOG_INFO, "Tutorial: consume funnel started");
}

bool TutorialActive(const Tutorial *t) {
    return t->active;
}

bool TutorialMainActive(const Tutorial *t) {
    return t->active && t->funnel == TUTORIAL_FUNNEL_MAIN;
}

bool TutorialHoldsBattle(const Tutorial *t) {
    const TutorialStep *step = TutorialCurrent(t);
    return step && t->visible && step->hold_battle;
}

bool TutorialWantsVictory(const Tutorial *t) {
    if (!t->active) return true;
    for (int i = t->index; i < t->step_count[t->funnel]; i++) {
        const TutorialStep *step = &t->steps[t->funnel][i];
        if (step->signal == TUTORIAL_SIGNAL_BATTLE_RESULTS || step->signal == TUTORIAL_SIGNAL_NODE_COMPLETION)
            return step->highlight != TUTORIAL_ANCHOR_RESULTS_STORE;
    }
    return true;
}

const TutorialPackReward *TutorialPackRewardFor(const Tutorial *t, const char *pack_id) {
    for (int i = 0; i < t->pack_count; i++)
        if (!strcmp(t->packs[i].pack_id, pack_id)) return &t->packs[i];
    return NULL;
}

static int TutorialPackForAnchor(const Tutorial *t, TutorialAnchor anchor) {
    const char *id = anchor == TUTORIAL_ANCHOR_STORE_SOLDIER_BUY         ? TUTORIAL_SOLDIER_PACK_ID
                     : anchor == TUTORIAL_ANCHOR_STORE_CHAMPION_BUY      ? TUTORIAL_CHAMPION_PACK_ID
                     : anchor == TUTORIAL_ANCHOR_STORE_TUTORIAL_PACK_BUY ? TUTORIAL_CONSUME_PACK_ID
                                                                         : NULL;
    if (!id) return -1;
    for (int i = 0; i < t->pack_count; i++)
        if (!strcmp(t->packs[i].pack_id, id)) return i;
    return -1;
}

static int TutorialNextRestorePoint(const Tutorial *t) {
    for (int i = t->index + 1; i < t->step_count[t->funnel]; i++)
        if (t->steps[t->funnel][i].restore_point) return (int)t->steps[t->funnel][i].step;
    return -1;
}

static void TutorialUpdateRestorePoint(Tutorial *t, Player *player) {
    const TutorialStep *step = TutorialCurrent(t);
    if (!step || t->funnel != TUTORIAL_FUNNEL_MAIN) return;
    int restore = -1;
    if (step->restore_point && !step->restore_point_for_server)
        restore = (int)step->step;
    else if (step->restore_point_for_server)
        restore = TutorialNextRestorePoint(t);
    if (restore < 0 || restore == player->tutorial.restore_step) return;
    player->tutorial.restore_step = restore;
    TutorialPushKind(t, TUTORIAL_REQUEST_SAVE);
}

static void TutorialFinishFunnel(Tutorial *t, Player *player) {
    if (t->funnel == TUTORIAL_FUNNEL_MAIN) {
        player->tutorial.main_complete = true;
        player->tutorial.restore_step = 0;
        TutorialPushKind(t, TUTORIAL_REQUEST_MAIN_FINISHED);
    } else {
        player->tutorial.consume_complete = true;
        TutorialPushKind(t, TUTORIAL_REQUEST_CONSUME_FINISHED);
    }
    t->active = false;
    TraceLog(LOG_INFO, "Tutorial: %s funnel finished", TUTORIAL_FUNNEL_KEYS[t->funnel]);
}

static void TutorialAdvance(Tutorial *t, Player *player) {
    const TutorialStep *left = TutorialCurrent(t);
    if (!left) return;
    if (left->action == TUTORIAL_ACTION_EXIT_INTRO_BATTLE) TutorialPushKind(t, TUTORIAL_REQUEST_EXIT_INTRO_BATTLE);
    if (left->highlight == TUTORIAL_ANCHOR_BATTLE_RETREAT) TutorialPushKind(t, TUTORIAL_REQUEST_GO_HOME);
    bool finished = left->funnel_end != TUTORIAL_FUNNEL_END_NONE || t->index + 1 >= t->step_count[t->funnel];
    if (finished) {
        TutorialFinishFunnel(t, player);
        return;
    }
    t->index++;
    TutorialBeginStep(t);
    TutorialUpdateRestorePoint(t, player);
}

static bool TutorialStepIsSilent(const TutorialStep *step) {
    bool anchor_unsupported = step->highlight_required && step->highlight == TUTORIAL_ANCHOR_NONE;
    return (!step->text[0] && !step->highlight_required && !step->drag_arrow) || anchor_unsupported ||
           step->action == TUTORIAL_ACTION_ABOUT_SCREEN || step->action == TUTORIAL_ACTION_UNSUPPORTED;
}

static bool TutorialEnterStep(Tutorial *t, Player *player, const TutorialStep *step) {
    t->entered = true;
    if (step->map_id[0] && step->signal == TUTORIAL_SIGNAL_BATTLE_MAP) {
        TutorialRequest request = {.kind = TUTORIAL_REQUEST_SELECT_MAP_NODE, .node = step->map_node};
        StrCopy(request.map_id, sizeof(request.map_id), step->map_id);
        TutorialPush(t, request);
    }
    if (step->action == TUTORIAL_ACTION_START_INTRO_BATTLE) TutorialPushKind(t, TUTORIAL_REQUEST_START_INTRO_BATTLE);
    if (step->action == TUTORIAL_ACTION_AWARD_GEM_PACK) TutorialPushKind(t, TUTORIAL_REQUEST_AWARD_GEM_PACK);
    if (step->award_giant) {
        TutorialPushKind(t, TUTORIAL_REQUEST_AWARD_GIANT);
        TutorialAdvance(t, player);
        return false;
    }
    if (TutorialStepIsSilent(step)) {
        TutorialAdvance(t, player);
        return false;
    }
    return true;
}

static void TutorialTakeEvents(Tutorial *t, const TutorialStep *step) {
    for (int i = 0; i < tutorial_hooks.event_count; i++) {
        TutorialEvent event = tutorial_hooks.events[i];
        if (event == TUTORIAL_EVENT_UNIT_PURCHASE_SUCCEEDED) t->pending_pack = -1;
        if (step && step->event == event) t->event_seen = true;
    }
    tutorial_hooks.event_count = 0;
}

static void TutorialPublishHooks(const Tutorial *t, const TutorialStep *step, bool drag_phase) {
    tutorial_hooks.running = t->active;
    tutorial_hooks.required = step && t->visible && step->highlight_required ? step->highlight : TUTORIAL_ANCHOR_NONE;
    tutorial_hooks.hidden = step ? step->hidden : TUTORIAL_ANCHOR_NONE;
    tutorial_hooks.drag_rarity = drag_phase ? step->drag_rarity : TUTORIAL_NO_RARITY;
    tutorial_hooks.drag_giant = drag_phase && step->drag_giant;
    tutorial_hooks.scripted_pack = t->pending_pack >= 0 ? &t->packs[t->pending_pack] : NULL;
}

static float TutorialWidthOffset(void) {
    return UiWidth() - TUTORIAL_DESIGN_WIDTH;
}

static Rectangle TutorialDesignRect(float x, float y, float w, float h, float side) {
    return (Rectangle){x + TutorialWidthOffset() * side, y, w, h};
}

static bool TutorialSideBar(TutorialTextBar bar) {
    return bar == TUTORIAL_TEXT_LEFT || bar == TUTORIAL_TEXT_RIGHT;
}

static Rectangle TutorialGuideRect(const TutorialStep *step) {
    static const float sizes[] = {360.0f, 433.0f, 504.0f};
    float size = sizes[TutorialSideBar(step->text_bar) ? TUTORIAL_GUIDE_SMALL : step->guide_size];
    float y = UiHeight() - size;
    return step->guide_on_left ? (Rectangle){0.0f, y, size, size} : (Rectangle){UiWidth() - size, y, size, size};
}

static GuiId TutorialGuideTexture(TutorialGuideMood mood) {
    static const GuiId textures[] = {
        GUI_FTUE_HOST, GUI_FTUE_HOST_CONTEMPLATIVE, GUI_FTUE_HOST_SATISFIED, GUI_FTUE_HOST_HAPPY
    };
    return textures[mood];
}

static void TutorialDrawGuide(const Tutorial *t, const TutorialStep *step) {
    Rectangle dst = TutorialGuideRect(step);
    float slide = Clamp(t->visible_time / GUIDE_SLIDE_IN_DURATION, 0.0f, 1.0f);
    float offscreen = (1.0f - slide) * dst.width * (step->guide_on_left ? -1.0f : 1.0f);
    dst.x += offscreen;
    Texture2D texture = GuiTex(TutorialGuideTexture(step->guide_mood));
    float src_width = step->guide_on_left ? (float)texture.width : -(float)texture.width;
    Rectangle src = {0.0f, 0.0f, src_width, (float)texture.height};
    DrawTexturePro(texture, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, Fade(WHITE, slide));
}

typedef struct {
    Rectangle background;
    Rectangle label;
} TutorialTextLayout;

static Rectangle TutorialSlideLabel(Rectangle label, bool guide_on_left) {
    float anchor = guide_on_left ? GUIDE_LABEL_X_WITH_GUIDE_ON_LEFT : GUIDE_LABEL_X_WITH_GUIDE_ON_RIGHT;
    label.x = (UiWidth() - label.width) * anchor;
    return label;
}

static TutorialTextLayout TutorialTextBarLayout(TutorialTextBar bar) {
    float stretch = TutorialWidthOffset();
    switch (bar) {
        case TUTORIAL_TEXT_LEFT:
            return (TutorialTextLayout){
                TutorialDesignRect(-304.0f, 126.0f, 936.0f, 468.0f, 0.0f),
                TutorialDesignRect(28.0f, 29.0f, 319.0f, 577.0f, 0.0f)
            };
        case TUTORIAL_TEXT_RIGHT:
            return (TutorialTextLayout){
                TutorialDesignRect(650.0f, 126.0f, 936.0f, 468.0f, 1.0f),
                TutorialDesignRect(934.0f, 29.0f, 319.0f, 577.0f, 1.0f)
            };
        case TUTORIAL_TEXT_BOTTOM:
            return (TutorialTextLayout){
                (Rectangle){-9.0f, 488.0f, 1299.0f + stretch, 433.0f},
                TutorialDesignRect(424.0f, 520.0f, 433.0f, 206.0f, 0.5f)
            };
        case TUTORIAL_TEXT_TOP:
        default:
            return (TutorialTextLayout){
                (Rectangle){-9.0f, 58.0f, 1299.0f + stretch, 433.0f},
                TutorialDesignRect(443.0f, 53.0f, 528.0f, 330.0f, 0.5f)
            };
    }
}

static float TutorialFitTextSize(Font font, const char *text, Rectangle area) {
    float size = TUTORIAL_TEXT_SIZE;
    while (size > TUTORIAL_TEXT_MIN_SIZE &&
           HeroesRichLayout(font, text, area, size, TUTORIAL_TEXT_COLOR, false) > area.height)
        size -= 1.0f;
    return size;
}

typedef struct {
    GuiId texture;
    Rectangle design;
    float side;
} TutorialIconLayout;

static const TutorialIconLayout TUTORIAL_ICON_LAYOUTS[TUTORIAL_ICON_COUNT] = {
    [TUTORIAL_ICON_HEALTH_POWER_SPEED] = {GUI_FTUE_ICON_HEALTH_POWER_SPEED, {47.0f, 293.0f, 282.0f, 70.5f}, 0.0f},
    [TUTORIAL_ICON_HEALTH] = {GUI_FTUE_ICON_HEALTH, {930.0f, 259.0f, 145.0f, 145.0f}, 1.0f},
    [TUTORIAL_ICON_CRIT] = {GUI_FTUE_ICON_CRIT, {930.0f, 231.0f, 145.0f, 145.0f}, 1.0f},
    [TUTORIAL_ICON_FACTIONS] = {GUI_FTUE_ICON_FACTIONS, {83.0f, 275.0f, 209.28f, 109.0f}, 0.0f},
    [TUTORIAL_ICON_ENERGY] = {GUI_HUD_ENERGY, {377.0f, 582.0f, 83.0f, 83.0f}, 0.5f},
    [TUTORIAL_ICON_STAMINA] = {GUI_HUD_STAMINA, {377.0f, 582.0f, 83.0f, 83.0f}, 0.5f},
    [TUTORIAL_ICON_SPEEDS] = {GUI_FTUE_ICON_SPEEDS, {47.0f, 293.0f, 282.0f, 70.5f}, 0.0f},
    [TUTORIAL_ICON_STUN] = {GUI_FTUE_ICON_STUN, {185.0f, 288.0f, 145.0f, 145.0f}, 0.0f},
    [TUTORIAL_ICON_BATTLE] = {GUI_FTUE_ICON_BATTLE, {579.0f, 329.0f, 124.0f, 124.0f}, 0.5f},
    [TUTORIAL_ICON_GIANT] = {GUI_FTUE_ICON_GIANT, {569.0f, 310.0f, 145.0f, 145.0f}, 0.5f},
};

static Rectangle TutorialIconRect(const TutorialStep *step) {
    if (step->icon == TUTORIAL_ICON_NONE) return (Rectangle){0};
    const TutorialIconLayout *layout = &TUTORIAL_ICON_LAYOUTS[step->icon];
    Rectangle r = layout->design;
    return TutorialDesignRect(r.x, r.y, r.width, r.height, layout->side);
}

static void TutorialDrawIconAt(const TutorialStep *step, Rectangle rect) {
    if (step->icon == TUTORIAL_ICON_NONE) return;
    UiDrawTexture(GuiTex(TUTORIAL_ICON_LAYOUTS[step->icon].texture), rect, WHITE);
}

static Rectangle TutorialSideColumn(const TutorialStep *step, const TutorialTextLayout *layout) {
    float top = layout->background.y + TUTORIAL_SIDE_TEXT_PADDING;
    float height = layout->background.height - TUTORIAL_SIDE_TEXT_PADDING * 2.0f;
    bool on_left = step->text_bar == TUTORIAL_TEXT_LEFT;
    if (step->guide_on_left != on_left) return (Rectangle){layout->label.x, top, layout->label.width, height};
    Rectangle guide = TutorialGuideRect(step);
    float left = on_left ? guide.x + guide.width : fmaxf(layout->background.x, 0.0f);
    float right = on_left ? layout->background.x + layout->background.width : guide.x;
    left += TUTORIAL_SIDE_TEXT_PADDING;
    right -= TUTORIAL_SIDE_TEXT_PADDING;
    return (Rectangle){left, top, fmaxf(right - left, TUTORIAL_TEXT_MIN_SIZE), height};
}

static bool TutorialIconInColumn(Rectangle icon, Rectangle column) {
    return icon.width > 0.0f && icon.x < column.x + column.width && icon.x + icon.width > column.x;
}

static float TutorialTextHeight(Font font, const char *text, Rectangle area, float size) {
    return HeroesRichLayout(font, text, area, size, TUTORIAL_TEXT_COLOR, false) -
           size * (HEROES_RICH_LINE_HEIGHT - 1.0f);
}

static Rectangle TutorialAboveGuide(const TutorialStep *step, const TutorialTextLayout *layout) {
    float left = fmaxf(layout->background.x, 0.0f) + TUTORIAL_SIDE_TEXT_PADDING;
    float right = fminf(layout->background.x + layout->background.width, UiWidth()) - TUTORIAL_SIDE_TEXT_PADDING;
    float top = layout->background.y + TUTORIAL_SIDE_TEXT_PADDING;
    float bottom = TutorialGuideRect(step).y - TUTORIAL_SIDE_TEXT_PADDING;
    return (Rectangle){
        left, top, fmaxf(right - left, TUTORIAL_TEXT_MIN_SIZE), fmaxf(bottom - top, TUTORIAL_TEXT_MIN_SIZE)
    };
}

static Rectangle TutorialTextBeside(Rectangle area, const Rectangle *avoid) {
    if (!avoid || !CheckCollisionRecs(area, *avoid)) return area;
    float left = area.x, right = area.x + area.width;
    if (avoid->x > left)
        right = fminf(right, avoid->x - TUTORIAL_SIDE_TEXT_PADDING);
    else
        left = fmaxf(left, avoid->x + avoid->width + TUTORIAL_SIDE_TEXT_PADDING);
    area.x = left;
    area.width = fmaxf(right - left, TUTORIAL_TEXT_MIN_SIZE);
    return area;
}

static void TutorialDrawText(const Tutorial *t, const TutorialStep *step, const Rectangle *avoid) {
    float alpha = Clamp(t->visible_time / GUIDE_FADE_IN_DURATION * 2.0f, 0.0f, 1.0f);
    TutorialTextLayout layout = TutorialTextBarLayout(step->text_bar);
    UiDrawTexture(GuiTex(GUI_FTUE_CONTAINER), layout.background, Fade(WHITE, alpha));
    Font font = GuiFont(FNT_MENU);
    const char *text = LocGet(step->text);
    Rectangle icon = TutorialIconRect(step);
    Rectangle area = TutorialSideBar(step->text_bar) ? TutorialSideColumn(step, &layout)
                                                     : TutorialSlideLabel(layout.label, step->guide_on_left);
    area = TutorialTextBeside(area, avoid);
    if (TutorialSideBar(step->text_bar) && area.width < TUTORIAL_TEXT_MIN_COLUMN)
        area = TutorialTextBeside(TutorialAboveGuide(step, &layout), avoid);
    bool stacked = TutorialSideBar(step->text_bar) && TutorialIconInColumn(icon, area);
    float reserved = stacked ? icon.height + TUTORIAL_SIDE_TEXT_PADDING : 0.0f;
    Rectangle fit = {area.x, area.y, area.width, fmaxf(area.height - reserved, TUTORIAL_TEXT_MIN_SIZE)};
    float size = TutorialFitTextSize(font, text, fit);
    float height = TutorialTextHeight(font, text, fit, size);
    Rectangle text_rect = area;
    text_rect.y += (area.height - height - reserved) * 0.5f;
    HeroesDrawRich(font, text, text_rect, size, Fade(TUTORIAL_TEXT_COLOR, alpha));
    if (stacked) {
        icon.x = area.x + (area.width - icon.width) * 0.5f;
        icon.y = text_rect.y + height + TUTORIAL_SIDE_TEXT_PADDING;
    }
    TutorialDrawIconAt(step, icon);
}

static void TutorialDrawGlow(const Tutorial *t, Rectangle rect) {
    float phase = 0.5f + 0.5f * sinf(t->time * PI / FTUE_GLOW_PULSE_DURATION);
    float opacity = FTUE_GLOW_MIN_OPACITY + (1.0f - FTUE_GLOW_MIN_OPACITY) * phase;
    Rectangle ring = {rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f};
    DrawRectangleLinesEx(ring, TUTORIAL_GLOW_THICKNESS, Fade(TUTORIAL_GLOW_COLOR, opacity));
}

static void TutorialDrawArrowAt(Vector2 tip, float rotation, float size, GuiId texture) {
    Texture2D arrow = GuiTex(texture);
    Rectangle src = {0.0f, 0.0f, (float)arrow.width, (float)arrow.height};
    DrawTexturePro(arrow, src, (Rectangle){tip.x, tip.y, size, size}, (Vector2){size * 0.5f, size}, rotation, WHITE);
}

static void TutorialDrawArrow(const Tutorial *t, Rectangle rect, TutorialArrow direction) {
    if (direction == TUTORIAL_ARROW_HIDDEN) return;
    float bob = TUTORIAL_ARROW_BOB * (0.5f + 0.5f * sinf(t->time * 2.0f * PI / (FTUE_GLOW_PULSE_DURATION * 2.0f)));
    Vector2 center = {rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f};
    if (direction == TUTORIAL_ARROW_UP) {
        TutorialDrawArrowAt(
            (Vector2){center.x, rect.y + rect.height + bob}, 180.0f, TUTORIAL_ARROW_SIZE, GUI_FTUE_ARROW
        );
    } else if (direction == TUTORIAL_ARROW_RIGHT) {
        TutorialDrawArrowAt((Vector2){rect.x - bob, center.y}, -90.0f, TUTORIAL_ARROW_SIZE, GUI_FTUE_ARROW);
    } else {
        Vector2 tip = {center.x, rect.y - bob};
        if (tip.y < TUTORIAL_ARROW_SIZE) {
            TutorialDrawArrowAt(
                (Vector2){center.x, rect.y + rect.height + bob}, 180.0f, TUTORIAL_ARROW_SIZE, GUI_FTUE_ARROW
            );
            return;
        }
        TutorialDrawArrowAt(tip, 0.0f, TUTORIAL_ARROW_SIZE, GUI_FTUE_ARROW);
    }
}

static bool TutorialUnitMarked(const TutorialStep *step, const TutorialFrame *frame, bool enemy, int slot) {
    if (!enemy && (!frame->squad || PlayerSquadUnitAt(frame->squad, frame->squad->current_squad, slot) < 0))
        return false;
    switch (step->highlight_units) {
        case TUTORIAL_UNITS_PLAYER_FRONT_ROW:
            return !enemy && SquadSlotIsFront(slot);
        case TUTORIAL_UNITS_PLAYER_BACK_ROW:
            return !enemy && SquadSlotIsBack(slot);
        case TUTORIAL_UNITS_PLAYER_POSITION_1:
            return !enemy && SquadSlotReferencePosition(slot) == step->highlight_position - 1;
        case TUTORIAL_UNITS_ALL_ENEMIES:
            return enemy && frame->enemy && frame->enemy->present[slot];
        case TUTORIAL_UNITS_ENEMY_UNCOMMON:
        case TUTORIAL_UNITS_ENEMY_RARE: {
            if (!enemy || !frame->enemy || !frame->enemy->present[slot]) return false;
            Rarity wanted = step->highlight_units == TUTORIAL_UNITS_ENEMY_RARE ? RARITY_RARE : RARITY_UNCOMMON;
            Rarity strongest = TUTORIAL_NO_RARITY;
            for (int other = 0; other < SQUAD_SLOT_COUNT; other++)
                if (frame->enemy->present[other] && frame->enemy->chars[other].rarity > strongest)
                    strongest = frame->enemy->chars[other].rarity;
            bool any_wanted = false;
            for (int other = 0; other < SQUAD_SLOT_COUNT; other++)
                any_wanted |= frame->enemy->present[other] && frame->enemy->chars[other].rarity == wanted;
            return frame->enemy->chars[slot].rarity == (any_wanted ? wanted : strongest);
        }
        case TUTORIAL_UNITS_NONE:
        default:
            return false;
    }
}

static void TutorialDrawUnitMarkers(const Tutorial *t, const TutorialStep *step, const TutorialFrame *frame) {
    if (step->highlight_units == TUTORIAL_UNITS_NONE) return;
    float bob = TUTORIAL_ARROW_BOB * 0.5f * (0.5f + 0.5f * sinf(t->time * PI / FTUE_GLOW_PULSE_DURATION));
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            if (!TutorialUnitMarked(step, frame, side == 1, slot)) continue;
            Rectangle unit;
            TutorialAnchor anchor = (side == 0 ? TUTORIAL_ANCHOR_PLAYER_UNIT_0 : TUTORIAL_ANCHOR_ENEMY_UNIT_0) + slot;
            if (!TutorialAnchorRect(anchor, &unit)) continue;
            Vector2 tip = {unit.x + unit.width * 0.5f, unit.y - bob};
            TutorialDrawArrowAt(tip, TUTORIAL_SMALL_ARROW_POINTS_DOWN, TUTORIAL_UNIT_ARROW_SIZE, GUI_FTUE_ARROW_SMALL);
        }
    }
}

static void TutorialDrawDragHand(const Tutorial *t, TutorialAnchor from, TutorialAnchor to) {
    Rectangle source, target;
    if (!TutorialAnchorRect(from, &source) || !TutorialAnchorRect(to, &target)) return;
    float progress = fmodf(t->time / ARROW_DRAG_TIMER_SCALE, 1.0f);
    float eased = progress < 0.8f ? progress / 0.8f : 1.0f;
    Vector2 a = {source.x + source.width * 0.5f, source.y + source.height * 0.5f};
    Vector2 b = {target.x + target.width * 0.5f, target.y + target.height * 0.5f};
    Vector2 at = Vector2Lerp(a, b, eased * eased * (3.0f - 2.0f * eased));
    float alpha = progress < 0.8f ? 1.0f : 1.0f - (progress - 0.8f) / 0.2f;
    UiDrawTexture(
        GuiTex(GUI_FTUE_HAND),
        (Rectangle){
            at.x - TUTORIAL_HAND_SIZE * 0.25f, at.y - TUTORIAL_HAND_SIZE * 0.1f, TUTORIAL_HAND_SIZE, TUTORIAL_HAND_SIZE
        },
        Fade(WHITE, alpha)
    );
}

static bool TutorialTapped(Tutorial *t, Rectangle area) {
    Vector2 pointer = UiPointer();
    bool inside = CheckCollisionPointRec(pointer, area);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) t->press_armed = inside;
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return false;
    bool tapped = t->press_armed && inside;
    t->press_armed = false;
    return tapped;
}

static void TutorialDrawSpeech(const Tutorial *t, const TutorialStep *step, const Rectangle *avoid) {
    TutorialDrawText(t, step, avoid);
    TutorialDrawGuide(t, step);
}

static void TutorialDrawSpeechAround(const Tutorial *t, const TutorialStep *step, Rectangle hole) {
    float margin = TUTORIAL_GLOW_THICKNESS + 4.0f;
    hole = (Rectangle){hole.x - margin, hole.y - margin, hole.width + margin * 2.0f, hole.height + margin * 2.0f};
    float w = UiWidth(), h = UiHeight(), scale = UiScale();
    Rectangle strips[] = {
        {0.0f, 0.0f, w, hole.y},
        {0.0f, hole.y + hole.height, w, h - hole.y - hole.height},
        {0.0f, hole.y, hole.x, hole.height},
        {hole.x + hole.width, hole.y, w - hole.x - hole.width, hole.height},
    };
    for (int i = 0; i < (int)(sizeof(strips) / sizeof(strips[0])); i++) {
        Rectangle strip = strips[i];
        if (strip.width <= 0.0f || strip.height <= 0.0f) continue;
        BeginScissorMode(
            (int)(strip.x * scale),
            (int)(strip.y * scale),
            (int)ceilf(strip.width * scale),
            (int)ceilf(strip.height * scale)
        );
        TutorialDrawSpeech(t, step, &hole);
        EndScissorMode();
    }
}

void TutorialDraw(Tutorial *t, Player *player, const TutorialFrame *frame, float dt) {
    t->time += dt;
    const TutorialStep *step = TutorialCurrent(t);
    if (step && t->button_window_open && TutorialTapped(t, t->button_window)) {
        TutorialAdvance(t, player);
        step = TutorialCurrent(t);
    }
    t->button_window_open = false;
    TutorialTakeEvents(t, step);
    if (!step) {
        TutorialPublishHooks(t, NULL, false);
        UiSetInputWindow(false, (Rectangle){0});
        return;
    }

    bool signal_matches = step->signal == TUTORIAL_SIGNAL_ANY || step->signal == frame->signal;
    bool event_ready = step->event == TUTORIAL_EVENT_NONE || t->event_seen;
    bool drag_phase = signal_matches && step->drag_arrow && !event_ready;
    t->visible = signal_matches && event_ready;
    if (t->visible && !t->entered && !TutorialEnterStep(t, player, step)) {
        TutorialPublishHooks(t, TutorialCurrent(t), false);
        UiSetInputWindow(false, (Rectangle){0});
        return;
    }
    if (t->visible) t->visible_time += dt;
    int buy_pack = TutorialPackForAnchor(t, step->highlight);
    if (buy_pack >= 0) t->pending_pack = buy_pack;
    TutorialPublishHooks(t, step, drag_phase);

    if (drag_phase) {
        TutorialAnchor from = TUTORIAL_ANCHOR_HEROES_DRAG_SOURCE, to = TUTORIAL_ANCHOR_HEROES_DRAG_TARGET;
        if (frame->signal == TUTORIAL_SIGNAL_CONSUME) {
            from = TUTORIAL_ANCHOR_CONSUME_DRAG_SOURCE;
            to = TUTORIAL_ANCHOR_CONSUME_DRAG_TARGET;
        }
        TutorialDrawDragHand(t, from, to);
        UiSetInputWindow(false, (Rectangle){0});
        return;
    }
    if (!t->visible) {
        UiSetInputWindow(false, (Rectangle){0});
        return;
    }

    Rectangle target = {0};
    bool has_target = TutorialAnchorRect(step->highlight, &target);
    bool scrim = step->highlight == TUTORIAL_ANCHOR_NONE && step->text_bar == TUTORIAL_TEXT_TOP;
    if (scrim) DrawRectangleRec(UiRoot(), TUTORIAL_SCRIM_COLOR);
    TutorialDrawUnitMarkers(t, step, frame);
    bool pressable = has_target && step->highlight_required;
    if (has_target && !pressable) TutorialDrawGlow(t, target);
    if (step->text[0] && pressable)
        TutorialDrawSpeechAround(t, step, target);
    else if (step->text[0])
        TutorialDrawSpeech(t, step, NULL);
    if (pressable) TutorialDrawGlow(t, target);
    if (has_target) TutorialDrawArrow(t, target, step->arrow);
    bool settled = t->visible_time >= TUTORIAL_TAP_GRACE_SECONDS;
    if (step->highlight_required) {
        t->button_window_open = settled && has_target;
        t->button_window = target;
        UiSetInputWindow(has_target, t->button_window_open ? target : (Rectangle){0});
        return;
    }
    UiSetInputWindow(true, (Rectangle){0});
    if (settled && TutorialTapped(t, UiRoot())) TutorialAdvance(t, player);
}
