typedef enum {
    TUTORIAL_ANCHOR_NONE,
    TUTORIAL_ANCHOR_HOME_QUESTS,
    TUTORIAL_ANCHOR_HOME_STORE,
    TUTORIAL_ANCHOR_HUD_HOME,
    TUTORIAL_ANCHOR_HUD_ENERGY,
    TUTORIAL_ANCHOR_HUD_STAMINA,
    TUTORIAL_ANCHOR_MAP_ENTER,
    TUTORIAL_ANCHOR_BATTLE_FIGHT,
    TUTORIAL_ANCHOR_BATTLE_RETREAT,
    TUTORIAL_ANCHOR_RESULTS_CONTINUE,
    TUTORIAL_ANCHOR_RESULTS_STORE,
    TUTORIAL_ANCHOR_STORE_SOLDIER_CARD,
    TUTORIAL_ANCHOR_STORE_CHAMPION_CARD,
    TUTORIAL_ANCHOR_STORE_SOLDIER_BUY,
    TUTORIAL_ANCHOR_STORE_CHAMPION_BUY,
    TUTORIAL_ANCHOR_STORE_TUTORIAL_PACK_BUY,
    TUTORIAL_ANCHOR_PACK_SQUAD,
    TUTORIAL_ANCHOR_PACK_CONSUME,
    TUTORIAL_ANCHOR_PACK_NEXT,
    TUTORIAL_ANCHOR_PACK_RUNE,
    TUTORIAL_ANCHOR_HEROES_NEWEST_UNIT,
    TUTORIAL_ANCHOR_HEROES_DETAILS,
    TUTORIAL_ANCHOR_HEROES_QUEST,
    TUTORIAL_ANCHOR_HEROES_DRAG_SOURCE,
    TUTORIAL_ANCHOR_HEROES_DRAG_TARGET,
    TUTORIAL_ANCHOR_CONSUME_BUTTON,
    TUTORIAL_ANCHOR_CONSUME_SQUAD,
    TUTORIAL_ANCHOR_CONSUME_DRAG_SOURCE,
    TUTORIAL_ANCHOR_CONSUME_DRAG_TARGET,
    TUTORIAL_ANCHOR_PLAYER_UNIT_0,
    TUTORIAL_ANCHOR_ENEMY_UNIT_0 = TUTORIAL_ANCHOR_PLAYER_UNIT_0 + SQUAD_SLOT_COUNT,
    TUTORIAL_ANCHOR_COUNT = TUTORIAL_ANCHOR_ENEMY_UNIT_0 + SQUAD_SLOT_COUNT,
} TutorialAnchor;

typedef enum {
    TUTORIAL_EVENT_NONE,
    TUTORIAL_EVENT_UNIT_PURCHASE_SUCCEEDED,
    TUTORIAL_EVENT_UNIT_PURCHASE_FINISHED,
    TUTORIAL_EVENT_UNIT_PURCHASE_NEXT_BUTTON_APPEARED,
    TUTORIAL_EVENT_UNIT_ADDED_TO_SQUAD,
    TUTORIAL_EVENT_GIANT_UNIT_ADDED_TO_SQUAD,
    TUTORIAL_EVENT_UNIT_DETAILS_SHOWING,
    TUTORIAL_EVENT_UNIT_DETAILS_CLOSED,
    TUTORIAL_EVENT_TARGET_UNIT_ADDED,
    TUTORIAL_EVENT_SEED_UNIT_ADDED,
    TUTORIAL_EVENT_VICTORY_OR_DEFEAT,
    TUTORIAL_EVENT_BATTLE_MAP_NODE_PAN_FINISHED,
    TUTORIAL_EVENT_COUNT
} TutorialEvent;

#define TUTORIAL_EVENT_QUEUE_CAPACITY 8
#define TUTORIAL_NO_RARITY ((Rarity)0)
#define TUTORIAL_PACK_REWARD_CAPACITY 4
#define TUTORIAL_PACK_REWARD_HEROES 4
#define TUTORIAL_ID_CAPACITY 64
#define TUTORIAL_SOLDIER_PACK_ID "soldier"
#define TUTORIAL_CHAMPION_PACK_ID "champion"
#define TUTORIAL_CONSUME_PACK_ID "tutorial_consume"

typedef struct {
    char pack_id[TUTORIAL_ID_CAPACITY];
    char heroes[TUTORIAL_PACK_REWARD_HEROES][TUTORIAL_ID_CAPACITY];
    int hero_count;
    char rune[TUTORIAL_ID_CAPACITY];
} TutorialPackReward;

typedef struct {
    bool running;
    unsigned frame;
    Rectangle rects[TUTORIAL_ANCHOR_COUNT];
    unsigned published[TUTORIAL_ANCHOR_COUNT];
    TutorialEvent events[TUTORIAL_EVENT_QUEUE_CAPACITY];
    int event_count;
    TutorialAnchor required;
    TutorialAnchor hidden;
    Rarity drag_rarity;
    bool drag_giant;
    const TutorialPackReward *scripted_pack;
} TutorialHooks;

static TutorialHooks tutorial_hooks = {0};

void TutorialHooksBeginFrame(void) {
    tutorial_hooks.frame++;
}

void TutorialAnchorSet(TutorialAnchor anchor, Rectangle rect) {
    if (anchor <= TUTORIAL_ANCHOR_NONE || anchor >= TUTORIAL_ANCHOR_COUNT) return;
    tutorial_hooks.rects[anchor] = rect;
    tutorial_hooks.published[anchor] = tutorial_hooks.frame;
}

bool TutorialAnchorRect(TutorialAnchor anchor, Rectangle *out) {
    if (anchor <= TUTORIAL_ANCHOR_NONE || anchor >= TUTORIAL_ANCHOR_COUNT) return false;
    if (tutorial_hooks.published[anchor] != tutorial_hooks.frame) return false;
    *out = tutorial_hooks.rects[anchor];
    return true;
}

void TutorialRaise(TutorialEvent event) {
    if (!tutorial_hooks.running || tutorial_hooks.event_count >= TUTORIAL_EVENT_QUEUE_CAPACITY) return;
    tutorial_hooks.events[tutorial_hooks.event_count++] = event;
}

bool TutorialRunning(void) {
    return tutorial_hooks.running;
}

bool TutorialRequires(TutorialAnchor anchor) {
    return tutorial_hooks.running && tutorial_hooks.required == anchor;
}

bool TutorialHides(TutorialAnchor anchor) {
    return tutorial_hooks.running && tutorial_hooks.hidden == anchor;
}

bool TutorialWantsDrag(void) {
    return tutorial_hooks.running && (tutorial_hooks.drag_giant || tutorial_hooks.drag_rarity != TUTORIAL_NO_RARITY);
}

bool TutorialDragsGiant(void) {
    return tutorial_hooks.running && tutorial_hooks.drag_giant;
}

bool TutorialWantsDragFrom(const Character *ch) {
    if (!tutorial_hooks.running || !ch) return false;
    if (tutorial_hooks.drag_giant) return ch->giant;
    return tutorial_hooks.drag_rarity != TUTORIAL_NO_RARITY && !ch->giant && ch->rarity == tutorial_hooks.drag_rarity;
}

const TutorialPackReward *TutorialScriptedPack(const char *pack_id) {
    const TutorialPackReward *reward = tutorial_hooks.running ? tutorial_hooks.scripted_pack : NULL;
    return reward && pack_id && !strcmp(reward->pack_id, pack_id) ? reward : NULL;
}
