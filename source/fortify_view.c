#define FORTIFY_DRAG_THRESHOLD 12.0f
#define FORTIFY_HIGHLIGHT_SPEED 0.55f
#define FORTIFY_HIGHLIGHT_MIN 0.3f
#define FORTIFY_HIGHLIGHT_MAX 1.0f
#define FORTIFY_CONFIRMED_OPACITY 0.75f
#define FORTIFY_BOTTOM_ROW_LIFT 0.05f
#define FORTIFY_CONFIRM_SLOT_LIFT 0.2f
#define FORTIFY_BONUS_MOVE_TIME 0.15f
#define FORTIFY_BONUS_HOLD_TIME 0.65f
#define FORTIFY_BONUS_FADE_TIME 0.15f
#define FORTIFY_BONUS_GAP_TIME 0.2f
#define FORTIFY_BONUS_START_Y 0.47f
#define FORTIFY_BONUS_END_Y 0.42f
#define FORTIFY_BONUS_STEPS 3
#define FORTIFY_BURST_DEGREES_PER_SECOND 15.0f
#define FORTIFY_BURST_PULSE_SECONDS 1.0f
#define FORTIFY_BURST_SMALLEST_SCALE 0.8f
#define FORTIFY_TALLY_SECONDS 1.5f
#define FORTIFY_TALLY_DELAY 1.0f
#define FORTIFY_SECOND_TALLY_DELAY 2.5f
#define FORTIFY_PORTRAIT_CACHE 24
#define FORTIFY_MESSAGE_CAP 256
#define FORTIFY_DRAG_ICON_SIZE (82.0f * 0.85f * 1.9f)
#define FORTIFY_DETAILS_RIGHT_MARGIN 134.0f
#define FORTIFY_BUTTON_LABEL_SCALE 0.33f
#define FORTIFY_PATH_COLUMNS FORTIFY_SLOTS_PER_ROW

static const Color FORTIFY_GAIN_COLOR = {0, 255, 16, 255};
static const Color FORTIFY_LOSS_COLOR = {255, 0, 0, 255};
static const Color FORTIFY_TEXT_COLOR = {235, 232, 220, 255};
static const Color FORTIFY_SOFT_TEXT_COLOR = {220, 216, 204, 255};

typedef struct {
    float x, y, width, height, aspect;
} FortifyAnchor;

static const FortifyAnchor FORTIFY_CENTER = {1.0f, 0.5f, 1.0f, 1.2f, 0.0f};
static const FortifyAnchor FORTIFY_SLOTS_AREA = {0.92f, 0.0f, 0.75f, 1.0f, 0.0f};
static const FortifyAnchor FORTIFY_FRAME_BACKGROUND = {0.5f, 0.5f, 1.9f, 0.82f, 0.9f};
static const FortifyAnchor FORTIFY_SELECTED_UNIT = {0.5f, 0.5f, 1.9f, 1.0f, 1.43f};
static const FortifyAnchor FORTIFY_TOP_ROW = {0.5f, 0.275f, 1.9f, 0.16f, 4.16f};
static const FortifyAnchor FORTIFY_BOTTOM_ROW = {0.5f, 0.73f, 1.9f, 0.16f, 4.16f};
static const FortifyAnchor FORTIFY_PORTRAIT = {0.5f, 0.5f, 1.9f, 0.25f, 1.0f};
static const FortifyAnchor FORTIFY_PEDESTAL = {0.5f, 0.5f, 1.9f, 0.7f, 1.0f};
static const FortifyAnchor FORTIFY_PARENT_PORTRAIT = {0.5f, 0.5f, 1.9f, 0.5f, 1.0f};
static const FortifyAnchor FORTIFY_PARENT_DROP = {0.5f, 0.5f, 1.9f, 0.8f, 1.0f};
static const FortifyAnchor FORTIFY_PARENT_XP = {0.5f, 0.7f, 1.0f, 0.04f, 10.0f};
static const FortifyAnchor FORTIFY_PARENT_XP_BACKGROUND = {0.5f, 0.5f, 1.05f, 1.5f, 0.0f};
static const FortifyAnchor FORTIFY_PARENT_LEVEL = {0.27f, 0.73f, 1.0f, 0.17f, 1.0f};
static const FortifyAnchor FORTIFY_BASE_STATS = {0.33f, 0.5f, 1.9f, 0.16f, 1.0f};
static const FortifyAnchor FORTIFY_FORTIFIED_STATS = {0.665f, 0.5f, 1.9f, 0.16f, 1.0f};
static const FortifyAnchor FORTIFY_STATS_BACKGROUND = {0.5f, 0.5f, 1.9f, 1.1f, 1.2f};
static const FortifyAnchor FORTIFY_STATS_POWER_ROW = {1.17f, 0.39f, 1.9f, 0.39f, 2.56f};
static const FortifyAnchor FORTIFY_STATS_HEALTH_ROW = {1.17f, 0.95f, 1.9f, 0.39f, 2.56f};
static const FortifyAnchor FORTIFY_STATS_ROW_LABEL = {0.94f, 0.44f, 1.9f, 0.62f, 2.82f};
static const FortifyAnchor FORTIFY_STATS_ROW_ICON = {0.02f, 0.3f, 1.9f, 0.6f, 1.0f};
static const FortifyAnchor FORTIFY_STATS_TITLE = {0.5f, 0.03f, 1.9f, 0.25f, 4.64f};
static const FortifyAnchor FORTIFY_BASE_PATH = {0.43f, 0.514f, 1.9f, 0.04f, 2.35f};
static const FortifyAnchor FORTIFY_FORTIFIED_PATH = {0.58f, 0.514f, 1.9f, 0.04f, 2.35f};
static const FortifyAnchor FORTIFY_TOP_PATHS = {0.49f, 0.38f, 1.9f, 0.067f, 8.57f};
static const FortifyAnchor FORTIFY_BOTTOM_PATHS = {0.49f, 0.624f, 1.9f, 0.067f, 8.57f};
static const FortifyAnchor FORTIFY_PATH_PIECES[FORTIFY_PATH_COLUMNS] = {
    {0.06f, 0.5f, 1.0f, 1.0f, 4.38f},
    {0.37f, 0.5f, 1.0f, 1.0f, 2.3f},
    {0.517f, 0.5f, 1.9f, 1.05f, 0.4f},
    {0.66f, 0.5f, 1.0f, 1.0f, 2.3f},
    {1.0f, 0.5f, 1.0f, 1.0f, 4.38f},
};
static const bool FORTIFY_PATH_MIRRORED[FORTIFY_PATH_COLUMNS] = {false, false, false, true, true};
static const FortifyAnchor FORTIFY_PARENT_INSTRUCTIONS = {0.66f, 0.5f, 1.9f, 0.25f, 1.4f};
static const FortifyAnchor FORTIFY_PARENT_INSTRUCTIONS_ARROW = {0.35f, 0.69f, 1.9f, 0.14f, 1.54f};
static const FortifyAnchor FORTIFY_PARENT_INSTRUCTIONS_TITLE = {0.64f, 0.16f, 1.9f, 0.32f, 2.35f};
static const FortifyAnchor FORTIFY_PARENT_INSTRUCTIONS_BODY = {0.64f, 0.47f, 1.9f, 0.32f, 2.35f};
static const FortifyAnchor FORTIFY_TOP_INSTRUCTIONS = {0.5f, 0.135f, 1.9f, 0.19f, 2.97f};
static const FortifyAnchor FORTIFY_TOP_INSTRUCTIONS_TEXT = {0.5f, 1.1f, 1.9f, 0.88f, 3.25f};
static const FortifyAnchor FORTIFY_BOTTOM_INSTRUCTIONS = {0.5f, 0.82f, 1.0f, 0.08f, 6.4f};
static const FortifyAnchor FORTIFY_BOTTOM_INSTRUCTIONS_ARROW = {0.04f, 0.1f, 1.9f, 0.4f, 1.54f};
static const FortifyAnchor FORTIFY_BOTTOM_INSTRUCTIONS_TEXT = {0.85f, 1.7f, 1.9f, 0.8f, 6.41f};
static const float FORTIFY_BOTTOM_ARROW_DEGREES = 59.0f;
static const FortifyAnchor FORTIFY_APPLY_CANCEL = {0.5f, 0.835f, 1.0f, 0.08f, 6.4f};
static const FortifyAnchor FORTIFY_CANCEL_BUTTON = {0.0f, 0.5f, 0.5f, 0.8f, 4.0f};
static const FortifyAnchor FORTIFY_APPLY_BUTTON = {1.0f, 0.5f, 0.5f, 0.8f, 4.0f};
static const FortifyAnchor FORTIFY_BONUS_POPUP = {0.5f, 0.49f, 1.9f, 0.25f, 2.0f};
static const FortifyAnchor FORTIFY_BONUS_POPUP_BACKGROUND = {0.5f, 0.5f, 1.9f, 0.3f, 3.52f};
static const FortifyAnchor FORTIFY_BONUS_POPUP_LABEL = {0.54f, 0.52f, 1.9f, 0.5f, 2.0f};
static const FortifyAnchor FORTIFY_MODE_BUTTON = {0.5f, 0.5f, 0.6f, 0.8f, 4.8f};

static const FortifyAnchor FORTIFY_SLOT_CELL = {0.0f, 0.5f, 1.0f, 1.0f, 1.0f};
static const FortifyAnchor FORTIFY_SLOT_BACKGROUND = {0.5f, 0.5f, 1.9f, 0.85f, 1.0f};
static const FortifyAnchor FORTIFY_SLOT_PORTRAIT = {0.5f, 0.5f, 1.9f, 0.63f, 1.0f};
static const FortifyAnchor FORTIFY_SLOT_TIER = {0.5f, 0.765f, 1.0f, 0.072f, 7.2f};
static const FortifyAnchor FORTIFY_SLOT_TOP_BONUS = {0.3f, -0.2f, 1.0f, 0.3f, 2.0f};
static const FortifyAnchor FORTIFY_SLOT_BOTTOM_BONUS = {0.3f, 1.185f, 1.0f, 0.3f, 2.0f};
static const FortifyAnchor FORTIFY_SLOT_BONUS_ICON = {0.0f, 0.5f, 1.0f, 0.8f, 1.0f};
static const FortifyAnchor FORTIFY_SLOT_BONUS_AMOUNT = {1.0f, 0.6f, 1.9f, 0.48f, 2.0f};
static const FortifyAnchor FORTIFY_SLOT_REMOVE = {1.0f, 0.0f, 0.34f, 0.34f, 1.0f};

static const FortifyAnchor FORTIFY_POPUP_MESSAGE = {0.5f, 0.7f, 0.93f, 0.85f, 1.6f};
static const FortifyAnchor FORTIFY_POPUP_CONTENT = {0.5f, 0.4f, 1.9f, 0.85f, 1.53f};
static const FortifyAnchor FORTIFY_POPUP_TITLE = {0.0f, -0.25f, 1.0f, 0.15f, 0.0f};
static const FortifyAnchor FORTIFY_CONFIRM_BODY = {0.5f, -0.28f, 1.9f, 0.35f, 3.58f};
static const FortifyAnchor FORTIFY_CONFIRM_BUTTONS = {0.5f, 1.16f, 0.6f, 0.12f, 0.0f};
static const FortifyAnchor FORTIFY_CONFIRM_OK = {1.0f, 0.5f, 0.5f, 1.0f, 0.0f};
static const FortifyAnchor FORTIFY_CONFIRM_EDIT = {0.0f, 0.5f, 0.5f, 1.0f, 0.0f};
static const FortifyAnchor FORTIFY_CONFIRM_SLOTS = {0.5f, 0.27f, 1.05f, 0.59f, 0.0f};
static const FortifyAnchor FORTIFY_CONFIRM_TOTALS = {0.5f, 1.0f, 1.9f, 0.25f, 1.88f};
static const FortifyAnchor FORTIFY_CONFIRM_TOTALS_BACKGROUND = {0.5f, 0.5f, 1.9f, 0.9f, 2.42f};
static const FortifyAnchor FORTIFY_CONFIRM_POWER_TOTAL = {1.16f, 0.19f, 1.9f, 0.48f, 3.0f};
static const FortifyAnchor FORTIFY_CONFIRM_HEALTH_TOTAL = {1.16f, 0.91f, 1.9f, 0.48f, 3.0f};
static const FortifyAnchor FORTIFY_CONFIRM_TOTAL_LABEL = {-0.73f, 0.66f, 1.9f, 1.13f, 2.88f};
static const FortifyAnchor FORTIFY_CONFIRM_TOTAL_ICON = {-0.21f, 0.27f, 1.9f, 0.66f, 1.0f};
static const FortifyAnchor FORTIFY_CONFIRM_POWER_TALLY = {0.19f, 0.96f, 1.9f, 0.3f, 0.81f};
static const FortifyAnchor FORTIFY_CONFIRM_HEALTH_TALLY = {0.84f, 0.96f, 1.9f, 0.3f, 0.81f};
static const FortifyAnchor FORTIFY_CONFIRM_POWER_TALLY_LABEL = {1.64f, 0.38f, 1.9f, 0.3f, 5.0f};
static const FortifyAnchor FORTIFY_CONFIRM_HEALTH_TALLY_LABEL = {-0.5f, 0.38f, 1.9f, 0.3f, 5.0f};
static const FortifyAnchor FORTIFY_CONFIRM_POWER_TALLY_ICON = {0.81f, 0.34f, 1.9f, 0.3f, 1.0f};
static const FortifyAnchor FORTIFY_CONFIRM_HEALTH_TALLY_ICON = {0.0f, 0.34f, 1.9f, 0.3f, 1.0f};
static const FortifyAnchor FORTIFY_CONFIRM_BREAKDOWN = {0.55f, -0.9f, 1.9f, 1.8f, 0.93f};
static const FortifyAnchor FORTIFY_CONFIRM_BREAKDOWN_ROW = {0.5f, 0.0f, 1.9f, 0.1f, 4.0f};

static const FortifyAnchor FORTIFY_CELEBRATION_UNIT = {0.5f, -0.17f, 1.9f, 0.55f, 1.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_PORTRAIT = {0.5f, 0.4f, 1.9f, 0.6f, 1.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_BURST = {0.5f, 0.55f, 1.9f, 1.9f, 1.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_XP = {0.52f, 0.7f, 1.0f, 0.05f, 10.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_LEVEL = {0.22f, 0.75f, 1.0f, 0.2f, 1.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_INSTRUCTIONS = {0.0f, 1.19f, 1.0f, 0.15f, 0.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_BONUS = {0.51f, 0.66f, 1.9f, 0.25f, 1.88f};
static const FortifyAnchor FORTIFY_CELEBRATION_BONUS_BACKGROUND = {0.7f, 0.5f, 1.9f, 0.9f, 2.42f};
static const FortifyAnchor FORTIFY_CELEBRATION_POWER = {0.57f, 0.21f, 1.9f, 0.38f, 3.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_HEALTH = {0.57f, 0.81f, 1.9f, 0.38f, 3.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_STAT_LABEL = {0.03f, 2.05f, 1.9f, 1.0f, 4.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_STAT_ICON = {-0.4f, 0.3f, 1.9f, 0.8f, 1.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_MAX = {0.5f, 0.93f, 1.9f, 0.2f, 5.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_MAX_TITLE = {0.5f, 0.17f, 1.0f, 0.4f, 0.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_MAX_STATS = {0.0f, 1.9f, 1.9f, 1.0f, 6.57f};
static const FortifyAnchor FORTIFY_CELEBRATION_MAX_POWER = {0.4f, 0.93f, 1.9f, 0.62f, 3.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_MAX_HEALTH = {0.65f, 0.93f, 1.9f, 0.62f, 3.0f};
static const FortifyAnchor FORTIFY_CELEBRATION_MAX_ICON = {-0.25f, 0.3f, 1.9f, 0.5f, 1.0f};

typedef enum {
    FORTIFY_LEAVE_NONE,
    FORTIFY_LEAVE_SQUAD,
    FORTIFY_LEAVE_HOME,
    FORTIFY_LEAVE_STORE,
    FORTIFY_LEAVE_RUNES,
    FORTIFY_LEAVE_GALLERY,
    FORTIFY_LEAVE_CONSUME
} FortifyLeave;

typedef enum {
    FORTIFY_DIALOG_NONE,
    FORTIFY_DIALOG_MESSAGE,
    FORTIFY_DIALOG_WARN_FORTIFIED,
    FORTIFY_DIALOG_WARN_GEAR,
    FORTIFY_DIALOG_CONFIRM,
    FORTIFY_DIALOG_CELEBRATION,
    FORTIFY_DIALOG_REMOVE,
    FORTIFY_DIALOG_REMOVE_ALL
} FortifyDialog;

typedef enum { FORTIFY_PRESS_NONE, FORTIFY_PRESS_ROSTER, FORTIFY_PRESS_PARENT, FORTIFY_PRESS_SLOT } FortifyPress;

typedef struct {
    char icon[64];
    Texture2D texture;
} FortifyPortrait;

typedef struct {
    Rectangle slots_area, frame, selected, top_row, bottom_row;
    Rectangle portrait, parent_drop, base_stats, fortified_stats, base_path, fortified_path, top_paths, bottom_paths;
    Rectangle parent_instructions, top_instructions, bottom_instructions, apply_cancel, bonus_popup, mode_button;
} FortifyLayout;

typedef struct {
    Roster roster;
    int parent;
    int pending[FORTIFY_SLOT_COUNT];
    bool remove_mode;

    FortifyPress press;
    int press_index;
    int press_slot;
    Vector2 press_pos;
    bool dragging;
    int details_index;
    bool details_armed;

    FortifyDialog dialog;
    char message[FORTIFY_MESSAGE_CAP];
    int warn_slot, warn_unit;
    bool warning_checked;
    bool fortified_warning_disabled, gear_warning_disabled;
    int remove_slot;

    int fly_slot;
    FortifyBonus fly_bonus;
    float fly_time;
    float highlight_opacity;
    float highlight_speed;

    int celebrated;
    int celebrated_power, celebrated_health;
    float celebration_time;
    float tally_loop_left;
    bool tally_started, tally_finished;

    FortifyPortrait portraits[FORTIFY_PORTRAIT_CACHE];
    int portrait_count;

    FortifyLeave leave_done;
    Button apply, cancel, store, mode, confirm_ok, confirm_edit, dialog_yes, dialog_no, dialog_ok;
    Button remove_ok, remove_cancel, remove_all;
} FortifyScreen;

static Rectangle FortifyFit(Rectangle parent, FortifyAnchor anchor) {
    return HudFitRect(parent, anchor.x, anchor.y, anchor.width, anchor.height, anchor.aspect);
}

static FortifyLayout FortifyLayoutCompute(void) {
    FortifyLayout l;
    Rectangle root = {0.0f, ROSTER_PREFAB_TOP, UiWidth(), UiHeight() - ROSTER_PREFAB_TOP};
    l.slots_area = FortifyFit(FortifyFit(root, FORTIFY_CENTER), FORTIFY_SLOTS_AREA);
    l.frame = FortifyFit(l.slots_area, FORTIFY_FRAME_BACKGROUND);
    l.selected = FortifyFit(l.slots_area, FORTIFY_SELECTED_UNIT);
    l.top_row = FortifyFit(l.selected, FORTIFY_TOP_ROW);
    l.bottom_row = FortifyFit(l.selected, FORTIFY_BOTTOM_ROW);
    l.portrait = FortifyFit(l.selected, FORTIFY_PORTRAIT);
    l.parent_drop = FortifyFit(l.portrait, FORTIFY_PARENT_DROP);
    l.base_stats = FortifyFit(l.selected, FORTIFY_BASE_STATS);
    l.fortified_stats = FortifyFit(l.selected, FORTIFY_FORTIFIED_STATS);
    l.base_path = FortifyFit(l.selected, FORTIFY_BASE_PATH);
    l.fortified_path = FortifyFit(l.selected, FORTIFY_FORTIFIED_PATH);
    l.top_paths = FortifyFit(l.selected, FORTIFY_TOP_PATHS);
    l.bottom_paths = FortifyFit(l.selected, FORTIFY_BOTTOM_PATHS);
    l.parent_instructions = FortifyFit(l.selected, FORTIFY_PARENT_INSTRUCTIONS);
    l.top_instructions = FortifyFit(l.slots_area, FORTIFY_TOP_INSTRUCTIONS);
    l.bottom_instructions = FortifyFit(l.slots_area, FORTIFY_BOTTOM_INSTRUCTIONS);
    l.apply_cancel = FortifyFit(l.slots_area, FORTIFY_APPLY_CANCEL);
    l.bonus_popup = FortifyFit(l.selected, FORTIFY_BONUS_POPUP);
    l.mode_button = FortifyFit(l.apply_cancel, FORTIFY_MODE_BUTTON);
    return l;
}

static Rectangle FortifyRowCell(Rectangle row, int column) {
    float width = row.width / FORTIFY_SLOTS_PER_ROW;
    return (Rectangle){row.x + column * width, row.y, width, row.height};
}

static Rectangle FortifySlotRect(const FortifyLayout *l, int slot) {
    bool bottom = slot >= FORTIFY_SLOTS_PER_ROW;
    Rectangle cell = FortifyRowCell(bottom ? l->bottom_row : l->top_row, slot % FORTIFY_SLOTS_PER_ROW);
    Rectangle rect = FortifyFit(cell, FORTIFY_SLOT_CELL);
    if (bottom) rect.y -= rect.height * FORTIFY_BOTTOM_ROW_LIFT;
    return rect;
}

static int FortifySlotAt(const FortifyLayout *l, Vector2 point) {
    bool top = CheckCollisionPointRec(point, l->top_row);
    if (!top && !CheckCollisionPointRec(point, l->bottom_row)) return -1;
    Rectangle row = top ? l->top_row : l->bottom_row;
    int column = (int)floorf((point.x - row.x) / row.width * FORTIFY_SLOTS_PER_ROW);
    int slot = column + (top ? 0 : FORTIFY_SLOTS_PER_ROW);
    return column >= 0 && column < FORTIFY_SLOTS_PER_ROW && slot < FORTIFY_SLOT_COUNT ? slot : -1;
}

static void FortifyDrawFlipped(Texture2D texture, Rectangle dst, bool mirror, bool flip, float degrees, Color tint) {
    if (!texture.id) return;
    Rectangle src = {
        0.0f,
        0.0f,
        mirror ? -(float)texture.width : (float)texture.width,
        flip ? -(float)texture.height : (float)texture.height
    };
    Vector2 origin = {dst.width * 0.5f, dst.height * 0.5f};
    Rectangle centered = {dst.x + origin.x, dst.y + origin.y, dst.width, dst.height};
    DrawTexturePro(texture, src, centered, origin, degrees, tint);
}

static void FortifyDrawTextFit(Font font, const char *text, Rectangle rect, float size, Color color) {
    float width = MeasureTextEx(font, text, size, 1.0f).x;
    if (width > rect.width && width > 0.0f) size *= rect.width / width;
    UiDrawTextCentered(font, text, rect, size, color);
}

static void FortifyFormatSigned(char *out, int cap, int value) {
    snprintf(out, (size_t)cap, value < 0 ? "%d" : "+%d", value);
}

static Color FortifyDeltaColor(int delta) {
    return delta == 0 ? WHITE : delta > 0 ? FORTIFY_GAIN_COLOR : FORTIFY_LOSS_COLOR;
}

static Texture2D FortifyPortraitFor(FortifyScreen *f, const FortifyChild *child) {
    const FortifyHero *hero = FortifyHeroFind(child->id);
    if (!hero || !hero->icon_prefab[child->tier][0]) return (Texture2D){0};
    const char *icon = hero->icon_prefab[child->tier];
    for (int i = 0; i < f->portrait_count; i++)
        if (!strcmp(f->portraits[i].icon, icon)) return f->portraits[i].texture;
    if (f->portrait_count == FORTIFY_PORTRAIT_CACHE) {
        for (int i = 0; i < f->portrait_count; i++) UnloadTexture(f->portraits[i].texture);
        f->portrait_count = 0;
    }
    FortifyPortrait *entry = &f->portraits[f->portrait_count++];
    StrCopy(entry->icon, sizeof(entry->icon), icon);
    entry->texture = GuiPortraitLoad(icon);
    return entry->texture;
}

static void FortifyPortraitsUnload(FortifyScreen *f) {
    for (int i = 0; i < f->portrait_count; i++) UnloadTexture(f->portraits[i].texture);
    f->portrait_count = 0;
}

void FortifyInitialize(FortifyScreen *f, const Player *p) {
    memset(f, 0, sizeof(*f));
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    RosterInitialize(&f->roster, p);
    f->apply = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("FORTIFY_OK_BUTTON"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->cancel = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("FORTIFY_CANCEL_BUTTON"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->store = ButtonLoad(GUI_CONSUME_SHOP, GUI_CONSUME_SHOP_ON);
    f->mode = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "", font, anchor, 0.3f);
    f->confirm_ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("FORTIFY_CONFIRM_OK_BUTTON"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->confirm_edit = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("FORTIFY_CONFIRM_CANCEL_BUTTON"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->dialog_yes = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("CONFIRM_DIALOG_YES"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->dialog_no = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("CONFIRM_DIALOG_NO"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->dialog_ok = ButtonLoadText(
        GUI_BTN_GREY,
        GUI_BTN_GREY_ON,
        LocGet("RUNE_SINGLE_CONFIRMATION_OK_BUTTON"),
        font,
        anchor,
        FORTIFY_BUTTON_LABEL_SCALE
    );
    f->remove_ok = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "", font, anchor, FORTIFY_BUTTON_LABEL_SCALE);
    f->remove_cancel = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("FORTIFY_CANCEL_BUTTON"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->remove_all = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_UI_UNFORTIFY_REMOVE_ALL"), font, anchor, FORTIFY_BUTTON_LABEL_SCALE
    );
    f->parent = -1;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) f->pending[slot] = -1;
}

void FortifyDestroy(FortifyScreen *f) {
    FortifyPortraitsUnload(f);
}

static void FortifyClearPending(FortifyScreen *f) {
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) f->pending[slot] = -1;
    f->fly_slot = -1;
}

static void FortifyResetPress(FortifyScreen *f) {
    f->press = FORTIFY_PRESS_NONE;
    f->press_index = -1;
    f->press_slot = -1;
    f->dragging = false;
}

static void FortifySelectParent(FortifyScreen *f, int index, bool silent) {
    f->parent = index;
    f->remove_mode = false;
    FortifyClearPending(f);
    if (!silent) GuiSoundPlay(SND_FORTIFY_PARENT);
}

void FortifyOpen(FortifyScreen *f, const Player *p) {
    FortifySelectParent(f, -1, true);
    FortifyResetPress(f);
    f->roster.scroll = 0.0f;
    f->dialog = FORTIFY_DIALOG_NONE;
    f->details_index = -1;
    f->highlight_opacity = FORTIFY_HIGHLIGHT_MAX;
    f->highlight_speed = FORTIFY_HIGHLIGHT_SPEED;
    f->leave_done = FORTIFY_LEAVE_NONE;
    if (f->roster.filter == ROSTER_FILTER_TUTORIAL) f->roster.filter = ROSTER_FILTER_ALL;
    RosterRefreshOrder(&f->roster, p);
}

void FortifyRequestLeave(FortifyScreen *f, FortifyLeave leave) {
    if (f->dialog != FORTIFY_DIALOG_NONE) return;
    f->leave_done = leave;
}

bool FortifyDialogOpen(const FortifyScreen *f) {
    return f->dialog != FORTIFY_DIALOG_NONE;
}

static int FortifyPendingSlotOf(const FortifyScreen *f, int index) {
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++)
        if (f->pending[slot] == index) return slot;
    return -1;
}

static bool FortifyHasPending(const FortifyScreen *f) {
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++)
        if (f->pending[slot] >= 0) return true;
    return false;
}

static bool FortifyIsInUse(const FortifyScreen *f, int index) {
    return index == f->parent || FortifyPendingSlotOf(f, index) >= 0;
}

static FortifyChild FortifyShownChild(const FortifyScreen *f, const Player *p, int slot) {
    const Character *parent = &p->roster[f->parent];
    if (f->pending[slot] >= 0) return FortifyMergedChild(parent, slot, &p->roster[f->pending[slot]]);
    return parent->fortify[slot];
}

static FortifyBonus FortifyPendingDelta(const FortifyScreen *f, const Player *p, int slot) {
    if (f->parent < 0 || f->pending[slot] < 0) return (FortifyBonus){0};
    const Character *parent = &p->roster[f->parent];
    FortifyChild shown = FortifyShownChild(f, p, slot);
    FortifyBonus next = FortifySlotBonus(parent, slot, &shown);
    FortifyBonus current = FortifySlotBonus(parent, slot, &parent->fortify[slot]);
    return (FortifyBonus){next.rarity - current.rarity, next.faction - current.faction, next.tier - current.tier};
}

static int FortifyPendingStatDelta(const FortifyScreen *f, const Player *p, FortifyStat stat) {
    int total = 0;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++)
        if (f->pending[slot] >= 0 && FortifySlotDefAt(&p->roster[f->parent], slot)->stat == stat)
            total += FortifyBonusTotal(FortifyPendingDelta(f, p, slot));
    return total;
}

static void FortifyShowMessage(FortifyScreen *f, const char *text) {
    StrCopy(f->message, sizeof(f->message), text);
    f->dialog = FORTIFY_DIALOG_MESSAGE;
    GuiSoundPlay(SND_STORE_ERROR);
}

static void FortifyPlace(FortifyScreen *f, const Player *p, int slot, int index) {
    int previous = FortifyPendingSlotOf(f, index);
    if (previous >= 0) f->pending[previous] = -1;
    f->pending[slot] = index;
    f->fly_slot = slot;
    f->fly_bonus = FortifyPendingDelta(f, p, slot);
    f->fly_time = 0.0f;
    bool health = FortifySlotDefAt(&p->roster[f->parent], slot)->stat == FORTIFY_STAT_HEALTH;
    SoundId first = health ? SND_FORTIFY_HEALTH_1 : SND_FORTIFY_POWER_1;
    GuiSoundPlay(GetRandomValue(0, 1) ? first : (SoundId)(first + 1));
}

static void FortifyRelease(FortifyScreen *f, const FortifyLayout *l, const Player *p, int index, Vector2 point) {
    if (index == f->parent) return;
    int slot = FortifySlotAt(l, point);
    if (slot < 0) {
        if (CheckCollisionPointRec(point, l->parent_drop)) FortifySelectParent(f, index, false);
        return;
    }
    if (f->parent < 0 || f->remove_mode) return;
    const Character *parent = &p->roster[f->parent];
    const Character *unit = &p->roster[index];
    if (slot >= FortifySlotsUnlocked(parent)) return;
    if (PlayerSquadsContain(p, index)) {
        FortifyShowMessage(f, LocGet("FORTIFY_ERROR_MEMBER_OF_SQUAD_BODY"));
        return;
    }
    if (FortifyReplacementBlocked(parent, slot, unit)) return;
    f->warn_slot = slot;
    f->warn_unit = index;
    f->warning_checked = false;
    if (!f->fortified_warning_disabled && FortifyHasChildren(unit) && FortifyPendingSlotOf(f, index) < 0) {
        f->dialog = FORTIFY_DIALOG_WARN_FORTIFIED;
        GuiSoundPlay(SND_STORE_ERROR);
    } else if (!f->gear_warning_disabled && GearHasEquipped(unit)) {
        f->dialog = FORTIFY_DIALOG_WARN_GEAR;
        GuiSoundPlay(SND_STORE_ERROR);
    } else {
        FortifyPlace(f, p, slot, index);
    }
}

static void FortifyReleaseInRoster(FortifyScreen *f, int index) {
    if (index == f->parent) {
        FortifySelectParent(f, -1, true);
        return;
    }
    int slot = FortifyPendingSlotOf(f, index);
    if (slot >= 0) f->pending[slot] = -1;
    if (f->fly_slot == slot) f->fly_slot = -1;
}

static void FortifyStartPress(FortifyScreen *f, FortifyPress press, int index, int slot) {
    f->press = press;
    f->press_index = index;
    f->press_slot = slot;
    f->press_pos = UiMouse();
    f->dragging = false;
}

static void FortifyUpdatePress(FortifyScreen *f, const FortifyLayout *l, const Player *p, bool input) {
    Vector2 mouse = UiMouse();
    if (input && f->press == FORTIFY_PRESS_NONE && mouse.x > ROSTER_PANEL_WIDTH &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int slot = FortifySlotAt(l, mouse);
        if (slot >= 0 && f->parent >= 0 && f->remove_mode && FortifySlotFilled(&p->roster[f->parent], slot)) {
            f->remove_slot = slot;
            f->dialog = FORTIFY_DIALOG_REMOVE;
            GuiClickSoundPlay();
            return;
        }
        if (slot >= 0 && f->pending[slot] >= 0 && !f->remove_mode)
            FortifyStartPress(f, FORTIFY_PRESS_SLOT, f->pending[slot], slot);
        else if (f->parent >= 0 && CheckCollisionPointRec(mouse, l->parent_drop))
            FortifyStartPress(f, FORTIFY_PRESS_PARENT, f->parent, -1);
    }
    if (f->press == FORTIFY_PRESS_NONE) return;
    bool draggable = f->press != FORTIFY_PRESS_ROSTER || !FortifyIsInUse(f, f->press_index);
    if (draggable && !f->dragging && Vector2Distance(mouse, f->press_pos) >= FORTIFY_DRAG_THRESHOLD) {
        f->dragging = true;
        f->details_index = -1;
        GuiSoundPlay(SND_RUNE_PICKUP);
    }
    if (!input || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
    FortifyPress press = f->press;
    int index = f->press_index;
    bool dragged = f->dragging;
    FortifyResetPress(f);
    if (!dragged) {
        if (press == FORTIFY_PRESS_ROSTER) {
            f->details_index = index;
            f->details_armed = false;
            GuiClickSoundPlayDuplicate();
        }
        return;
    }
    if (mouse.x < ROSTER_PANEL_WIDTH)
        FortifyReleaseInRoster(f, index);
    else
        FortifyRelease(f, l, p, index, mouse);
}

static void FortifyDrawStatsBox(
    Rectangle box,
    bool mirrored,
    const char *title,
    const char *power,
    const char *health,
    Color power_color,
    Color health_color
) {
    Font title_font = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    FortifyDrawFlipped(
        GuiTex(GUI_FORTIFY_STATS_FRAME), FortifyFit(box, FORTIFY_STATS_BACKGROUND), mirrored, false, 0.0f, WHITE
    );
    Rectangle title_rect = FortifyFit(box, FORTIFY_STATS_TITLE);
    FortifyDrawTextFit(title_font, title, title_rect, title_rect.height * 0.7f, FORTIFY_TEXT_COLOR);
    const FortifyAnchor *rows[2] = {&FORTIFY_STATS_POWER_ROW, &FORTIFY_STATS_HEALTH_ROW};
    const char *values[2] = {power, health};
    Color colors[2] = {power_color, health_color};
    GuiId icons[2] = {GUI_DETAILS_POWER, GUI_FORTIFY_HEALTH_ICON};
    for (int i = 0; i < 2; i++) {
        Rectangle row = FortifyFit(box, *rows[i]);
        UiDrawTexture(GuiTex(icons[i]), FortifyFit(row, FORTIFY_STATS_ROW_ICON), WHITE);
        Rectangle label = FortifyFit(row, FORTIFY_STATS_ROW_LABEL);
        FortifyDrawTextFit(body, values[i], label, label.height * 0.8f, colors[i]);
    }
}

static void FortifyDrawStats(FortifyScreen *f, const FortifyLayout *l, const Player *p) {
    const Character *parent = &p->roster[f->parent];
    char power[32], health[32];
    snprintf(power, sizeof(power), "%d", CharacterStatPower(parent));
    snprintf(health, sizeof(health), "%d", CharacterStatHealth(parent));
    FortifyDrawStatsBox(
        l->base_stats,
        true,
        LocGet("FORTIFY_UNIT_BASE_STATS_TITLE"),
        power,
        health,
        FORTIFY_TEXT_COLOR,
        FORTIFY_TEXT_COLOR
    );

    int power_delta = FortifyPendingStatDelta(f, p, FORTIFY_STAT_POWER);
    int health_delta = FortifyPendingStatDelta(f, p, FORTIFY_STAT_HEALTH);
    char power_delta_text[16], health_delta_text[16];
    FortifyFormatSigned(power_delta_text, sizeof(power_delta_text), power_delta);
    FortifyFormatSigned(health_delta_text, sizeof(health_delta_text), health_delta);
    if (power_delta)
        snprintf(power, sizeof(power), "%d (%s)", CharacterStatPower(parent) + power_delta, power_delta_text);
    if (health_delta)
        snprintf(health, sizeof(health), "%d (%s)", CharacterStatHealth(parent) + health_delta, health_delta_text);
    bool highlighted = FortifyHasPending(f);
    FortifyDrawStatsBox(
        l->fortified_stats,
        false,
        LocGet("FORTIFY_UNIT_FORTIFY_STATS_TITLE"),
        power,
        health,
        power_delta ? FortifyDeltaColor(power_delta) : FORTIFY_TEXT_COLOR,
        health_delta ? FortifyDeltaColor(health_delta) : FORTIFY_TEXT_COLOR
    );
    if (highlighted)
        UiDrawTexture(
            GuiTex(GUI_FORTIFY_STATS_HIGHLIGHT),
            FortifyFit(l->fortified_stats, FORTIFY_STATS_BACKGROUND),
            Fade(WHITE, f->highlight_opacity)
        );
}

static void FortifyDrawPaths(const FortifyScreen *f, const FortifyLayout *l, bool parent_selected) {
    static const GuiId paths[FORTIFY_PATH_COLUMNS] = {
        GUI_FORTIFY_PATH_LONG, GUI_FORTIFY_PATH_MED, GUI_FORTIFY_PATH_SHORT, GUI_FORTIFY_PATH_MED, GUI_FORTIFY_PATH_LONG
    };
    for (int row = 0; row < 2; row++) {
        Rectangle container = row == 0 ? l->top_paths : l->bottom_paths;
        for (int column = 0; column < FORTIFY_PATH_COLUMNS; column++) {
            Rectangle piece = FortifyFit(container, FORTIFY_PATH_PIECES[column]);
            bool mirrored = FORTIFY_PATH_MIRRORED[column];
            FortifyDrawFlipped(GuiTex(paths[column]), piece, mirrored, row == 1, 0.0f, WHITE);
            int slot = row * FORTIFY_SLOTS_PER_ROW + column;
            if (parent_selected && f->pending[slot] >= 0)
                FortifyDrawFlipped(GuiTex(paths[column] + 1), piece, mirrored, row == 1, 0.0f, WHITE);
        }
    }
    if (!parent_selected) return;
    UiDrawTexture(GuiTex(GUI_FORTIFY_PATH_OUTPUT), l->base_path, WHITE);
    UiDrawTexture(GuiTex(GUI_FORTIFY_PATH_OUTPUT), l->fortified_path, WHITE);
    if (FortifyHasPending(f)) UiDrawTexture(GuiTex(GUI_FORTIFY_PATH_OUTPUT_ON), l->fortified_path, WHITE);
}

static void FortifyDrawSlotBonus(Rectangle slot, bool bottom, FortifyStat stat, const char *text, Color color) {
    Rectangle box = FortifyFit(slot, bottom ? FORTIFY_SLOT_BOTTOM_BONUS : FORTIFY_SLOT_TOP_BONUS);
    GuiId icon = stat == FORTIFY_STAT_HEALTH ? GUI_FORTIFY_HEALTH_ICON : GUI_DETAILS_POWER;
    UiDrawTexture(GuiTex(icon), FortifyFit(box, FORTIFY_SLOT_BONUS_ICON), WHITE);
    Rectangle amount = FortifyFit(box, FORTIFY_SLOT_BONUS_AMOUNT);
    UiDrawTextLeftMiddle(GuiFont(FNT_COOL), text, amount, amount.height * 1.1f, color);
}

static void FortifyDrawSlot(
    FortifyScreen *f,
    const FortifySlotDef *def,
    const FortifyChild *shown,
    bool pending,
    Rectangle rect,
    bool locked,
    bool bottom,
    int bonus_delta,
    int bonus_total
) {
    bool health = def->stat == FORTIFY_STAT_HEALTH;
    bool filled = shown && shown->id[0];
    if (!filled) {
        GuiId background = health ? (locked ? GUI_FORTIFY_SLOT_HEALTH_LOCKED : GUI_FORTIFY_SLOT_HEALTH)
                                  : (locked ? GUI_FORTIFY_SLOT_POWER_LOCKED : GUI_FORTIFY_SLOT_POWER);
        UiDrawTexture(GuiTex(background), FortifyFit(rect, FORTIFY_SLOT_BACKGROUND), WHITE);
        return;
    }
    Color tint = Fade(WHITE, pending ? 1.0f : FORTIFY_CONFIRMED_OPACITY);
    UiDrawTexture(FortifyPortraitFor(f, shown), FortifyFit(rect, FORTIFY_SLOT_PORTRAIT), tint);
    UiDrawTexture(GuiTex(GUI_FORTIFY_TIER_1 + shown->tier), FortifyFit(rect, FORTIFY_SLOT_TIER), tint);
    if (pending)
        UiDrawTexture(
            GuiTex(GUI_FORTIFY_SLOT_HIGHLIGHT),
            FortifyFit(rect, FORTIFY_SLOT_BACKGROUND),
            Fade(WHITE, f->highlight_opacity)
        );
    char text[16];
    FortifyFormatSigned(text, sizeof(text), pending ? bonus_delta : bonus_total);
    FortifyDrawSlotBonus(rect, bottom, def->stat, text, pending ? FortifyDeltaColor(bonus_delta) : WHITE);
}

static void FortifyDrawSlots(FortifyScreen *f, const FortifyLayout *l, const Player *p) {
    const Character *parent = f->parent >= 0 ? &p->roster[f->parent] : NULL;
    int unlocked = parent ? FortifySlotsUnlocked(parent) : FORTIFY_SLOTS_PER_ROW;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) {
        Rectangle rect = FortifySlotRect(l, slot);
        bool locked = slot >= unlocked;
        bool bottom = slot >= FORTIFY_SLOTS_PER_ROW;
        if (!parent) {
            FortifyDrawSlot(f, FortifyDefaultSlotDef(slot), NULL, false, rect, locked, bottom, 0, 0);
            continue;
        }
        bool pending = f->pending[slot] >= 0;
        FortifyChild shown = FortifyShownChild(f, p, slot);
        int total = FortifyBonusTotal(FortifySlotBonus(parent, slot, &parent->fortify[slot]));
        int delta = FortifyBonusTotal(FortifyPendingDelta(f, p, slot));
        FortifyDrawSlot(f, FortifySlotDefAt(parent, slot), &shown, pending, rect, locked, bottom, delta, total);
        if (f->remove_mode && FortifySlotFilled(parent, slot) && !pending)
            UiDrawTexture(GuiTex(GUI_BTN_CLOSE), FortifyFit(rect, FORTIFY_SLOT_REMOVE), WHITE);
    }
}

static void FortifyDrawParent(FortifyScreen *f, const FortifyLayout *l, const Heroes *h, const Player *p) {
    UiDrawTexture(GuiTex(GUI_FORTIFY_PEDESTAL), FortifyFit(l->portrait, FORTIFY_PEDESTAL), WHITE);
    if (f->parent < 0) return;
    const Character *parent = &p->roster[f->parent];
    bool lifted = f->dragging && f->press == FORTIFY_PRESS_PARENT;
    UiDrawTexture(
        h->cards[f->parent].portrait,
        FortifyFit(l->portrait, FORTIFY_PARENT_PORTRAIT),
        lifted ? Fade(WHITE, 0.4f) : WHITE
    );

    Rectangle xp = FortifyFit(l->portrait, FORTIFY_PARENT_XP);
    UiDrawTexture(GuiTex(GUI_FORTIFY_XP_BG), FortifyFit(xp, FORTIFY_PARENT_XP_BACKGROUND), WHITE);
    float fraction = h->cards[f->parent].xp_frac;
    Texture2D fill = GuiTex(GUI_FORTIFY_XP_FILL);
    DrawTexturePro(
        fill,
        (Rectangle){0, 0, fill.width * fraction, (float)fill.height},
        (Rectangle){xp.x, xp.y, xp.width * fraction, xp.height},
        (Vector2){0, 0},
        0.0f,
        WHITE
    );
    Rectangle level = FortifyFit(l->portrait, FORTIFY_PARENT_LEVEL);
    UiDrawTexture(GuiTex(GUI_FORTIFY_LEVEL), level, WHITE);
    char text[16];
    snprintf(text, sizeof(text), "%d", parent->level);
    UiDrawTextCentered(GuiFont(FNT_COOL), text, level, level.height * 0.55f, WHITE);
}

static bool FortifyModeButtonShown(const FortifyScreen *f, const Player *p) {
    return f->parent >= 0 && FortifyHasChildren(&p->roster[f->parent]) && !FortifyHasPending(f);
}

static void FortifyDrawInstructions(const FortifyScreen *f, const FortifyLayout *l, bool apply_shown) {
    Font body = GuiFont(FNT_MENU), title = GuiFont(FNT_COOL);
    Rectangle top = FortifyFit(l->top_instructions, FORTIFY_TOP_INSTRUCTIONS_TEXT);
    Rectangle first = {top.x, top.y, top.width, top.height * 0.45f};
    MessagePopupDrawText(body, LocGet("FORTIFY_TOP_INSTRUCTIONS_FIRST_LINE"), first, 18.0f, FORTIFY_TEXT_COLOR);
    if (f->parent < 0) {
        Rectangle second = {top.x, first.y + first.height, top.width, top.height * 0.2f};
        MessagePopupDrawText(
            body, LocGet("FORTIFY_TOP_INSTRUCTIONS_SECOND_LINE"), second, 14.0f, FORTIFY_SOFT_TEXT_COLOR
        );
        Rectangle box = l->parent_instructions;
        Rectangle arrow = FortifyFit(box, FORTIFY_PARENT_INSTRUCTIONS_ARROW);
        UiDrawTexture(GuiTex(GUI_FORTIFY_ARROW), arrow, WHITE);
        Rectangle heading = FortifyFit(box, FORTIFY_PARENT_INSTRUCTIONS_TITLE);
        MessagePopupDrawText(
            title, LocGet("FORTIFY_SELECTED_UNIT_INSTRUCTIONS_TITLE"), heading, 20.0f, FORTIFY_TEXT_COLOR
        );
        MessagePopupDrawText(
            body,
            LocGet("FORTIFY_SELECTED_UNIT_INSTRUCTIONS_BODY"),
            FortifyFit(box, FORTIFY_PARENT_INSTRUCTIONS_BODY),
            15.0f,
            FORTIFY_SOFT_TEXT_COLOR
        );
    }
    if (apply_shown) return;
    Rectangle bottom = l->bottom_instructions;
    if (f->parent < 0)
        FortifyDrawFlipped(
            GuiTex(GUI_FORTIFY_ARROW),
            FortifyFit(bottom, FORTIFY_BOTTOM_INSTRUCTIONS_ARROW),
            false,
            false,
            FORTIFY_BOTTOM_ARROW_DEGREES,
            WHITE
        );
    MessagePopupDrawText(
        body,
        LocGet("FORTIFY_BOTTOM_INSTRUCTIONS_BODY"),
        FortifyFit(bottom, FORTIFY_BOTTOM_INSTRUCTIONS_TEXT),
        13.0f,
        FORTIFY_SOFT_TEXT_COLOR
    );
}

static void FortifyDrawBonusFly(FortifyScreen *f, const FortifyLayout *l, float dt) {
    if (f->fly_slot < 0) return;
    float step_length = FORTIFY_BONUS_MOVE_TIME + FORTIFY_BONUS_HOLD_TIME + FORTIFY_BONUS_FADE_TIME;
    float cycle = step_length + FORTIFY_BONUS_GAP_TIME;
    int step = (int)(f->fly_time / cycle);
    float t = f->fly_time - step * cycle;
    f->fly_time += dt;
    if (step >= FORTIFY_BONUS_STEPS) {
        f->fly_slot = -1;
        return;
    }
    if (t > step_length) return;
    static const char *const keys[FORTIFY_BONUS_STEPS] = {
        "FORTIFY_CONFIRM_RARITY", "FORTIFY_CONFIRM_FACTION", "FORTIFY_CONFIRM_TIER"
    };
    int values[FORTIFY_BONUS_STEPS] = {f->fly_bonus.rarity, f->fly_bonus.faction, f->fly_bonus.tier};
    float rise = fminf(t / FORTIFY_BONUS_MOVE_TIME, 1.0f);
    float fade = t < FORTIFY_BONUS_MOVE_TIME + FORTIFY_BONUS_HOLD_TIME
                     ? rise
                     : 1.0f - (t - FORTIFY_BONUS_MOVE_TIME - FORTIFY_BONUS_HOLD_TIME) / FORTIFY_BONUS_FADE_TIME;
    float eased = rise < 0.5f ? 4.0f * rise * rise * rise : 1.0f - powf(-2.0f * rise + 2.0f, 3.0f) * 0.5f;
    FortifyAnchor anchor = FORTIFY_BONUS_POPUP;
    anchor.y = Lerp(FORTIFY_BONUS_START_Y, FORTIFY_BONUS_END_Y, eased);
    Rectangle popup = FortifyFit(l->selected, anchor);
    Color tint = Fade(WHITE, Clamp(fade, 0.0f, 1.0f));
    UiDrawTexture(GuiTex(GUI_FORTIFY_FLY), FortifyFit(popup, FORTIFY_BONUS_POPUP_BACKGROUND), tint);
    char amount[16], label[96];
    FortifyFormatSigned(amount, sizeof(amount), values[step]);
    char colored[48];
    snprintf(
        colored,
        sizeof(colored),
        "<color=%s>%s</color>",
        values[step] == 0  ? "grey"
        : values[step] > 0 ? "#00FF10"
                           : "#FF0000",
        amount
    );
    LocFormat1(label, sizeof(label), keys[step], colored);
    MessagePopupDrawText(GuiFont(FNT_COOL), label, FortifyFit(popup, FORTIFY_BONUS_POPUP_LABEL), 24.0f, tint);
}

static void FortifyUpdateHighlight(FortifyScreen *f, float dt) {
    f->highlight_opacity += dt * f->highlight_speed;
    if (f->highlight_opacity >= FORTIFY_HIGHLIGHT_MAX || f->highlight_opacity <= FORTIFY_HIGHLIGHT_MIN) {
        f->highlight_opacity = Clamp(f->highlight_opacity, FORTIFY_HIGHLIGHT_MIN, FORTIFY_HIGHLIGHT_MAX);
        f->highlight_speed = -f->highlight_speed;
    }
}

typedef struct {
    const FortifyScreen *screen;
    const Player *player;
} FortifyCardContext;

static void FortifyDrawRosterCard(const HeroCard *card, Rectangle cell, int roster_index, const void *context) {
    const FortifyCardContext *c = (const FortifyCardContext *)context;
    bool lifted = c->screen->dragging && c->screen->press_index == roster_index;
    RosterCardState state = {
        .progress = card->xp_frac,
        .maxed = card->xp_frac >= 1.0f,
        .in_squad = PlayerSquadsContain(c->player, roster_index),
        .in_use = FortifyIsInUse(c->screen, roster_index) || lifted,
        .selected = roster_index == c->screen->details_index,
        .fortified = FortifyHasChildren(card->ch),
    };
    RosterDrawCard(card, cell, &state);
}

static void FortifyDrawRoster(FortifyScreen *f, const Heroes *h, const Player *p, bool input) {
    FortifyCardContext context = {f, p};
    int hovered = RosterDraw(&f->roster, h->cards, p, input, FortifyDrawRosterCard, &context);
    if (input && hovered >= 0 && f->press == FORTIFY_PRESS_NONE && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        FortifyStartPress(f, FORTIFY_PRESS_ROSTER, hovered, -1);
    if (ButtonDrawEx(&f->store, RosterPx(13.0f, 828.0f, 81.0f, 61.0f), input))
        FortifyRequestLeave(f, FORTIFY_LEAVE_STORE);
    UiDrawTextLeftMiddle(
        GuiFont(FNT_MENU),
        LocGet("TAP_PORTRAIT_FOR_INFO"),
        RosterPx(105.0f, 831.0f, 287.0f, 66.0f),
        15.0f,
        FORTIFY_SOFT_TEXT_COLOR
    );
}

static void FortifyDrawDragged(const FortifyScreen *f, const Heroes *h) {
    if (!f->dragging || f->press_index < 0) return;
    Vector2 m = UiMouse();
    float size = FORTIFY_DRAG_ICON_SIZE;
    UiDrawTexture(
        h->cards[f->press_index].portrait, (Rectangle){m.x - size * 0.5f, m.y - size * 0.5f, size, size}, WHITE
    );
}

static void FortifyDrawActions(FortifyScreen *f, const FortifyLayout *l, const Player *p, bool input) {
    if (FortifyModeButtonShown(f, p)) {
        f->mode.label = LocGet(f->remove_mode ? "HODA_UI_EXIT_REMOVE_FORTIFICATIONS" : "HODA_UI_REMOVE_FORTIFICATIONS");
        if (ButtonDrawEx(&f->mode, l->mode_button, input)) f->remove_mode = !f->remove_mode;
    } else {
        f->remove_mode = false;
    }
    if (!FortifyHasPending(f)) return;
    if (ButtonDrawEx(&f->cancel, FortifyFit(l->apply_cancel, FORTIFY_CANCEL_BUTTON), input)) {
        FortifyClearPending(f);
        return;
    }
    if (ButtonDrawEx(&f->apply, FortifyFit(l->apply_cancel, FORTIFY_APPLY_BUTTON), input))
        f->dialog = FORTIFY_DIALOG_CONFIRM;
}

static Rectangle FortifyPopupContainer(void) {
    return FortifyFit(UiRoot(), FORTIFY_POPUP_MESSAGE);
}

static void FortifyDrawPopupPanel(Rectangle top, Rectangle bottom) {
    MessagePopupDrawScrim();
    Rectangle container = FortifyPopupContainer();
    float y = fminf(container.y, top.y);
    float end = fmaxf(container.y + container.height, bottom.y + bottom.height + container.height * 0.03f);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), (Rectangle){container.x, y, container.width, end - y}, WHITE);
}

static void FortifyDrawBreakdown(
    Rectangle tally, const FortifyAnchor *label_anchor, const FortifyAnchor *icon_anchor, GuiId icon, FortifyBonus bonus
) {
    if (FortifyBonusTotal(bonus) <= 0) return;
    Font body = GuiFont(FNT_MENU), title = GuiFont(FNT_COOL);
    Rectangle label = FortifyFit(tally, *label_anchor);
    FortifyDrawTextFit(title, LocGet("FORTIFY_CONFIRM_BONUS"), label, label.height * 0.7f, FORTIFY_TEXT_COLOR);
    UiDrawTexture(GuiTex(icon), FortifyFit(tally, *icon_anchor), WHITE);
    Rectangle rows = FortifyFit(tally, FORTIFY_CONFIRM_BREAKDOWN);
    static const char *const keys[FORTIFY_BONUS_STEPS] = {
        "FORTIFY_CONFIRM_RARITY", "FORTIFY_CONFIRM_FACTION", "FORTIFY_CONFIRM_TIER"
    };
    int values[FORTIFY_BONUS_STEPS] = {bonus.rarity, bonus.faction, bonus.tier};
    Rectangle row = FortifyFit(rows, FORTIFY_CONFIRM_BREAKDOWN_ROW);
    for (int i = 0; i < FORTIFY_BONUS_STEPS; i++) {
        char amount[16], parens[24], text[96];
        FortifyFormatSigned(amount, sizeof(amount), values[i]);
        snprintf(parens, sizeof(parens), "(%s)", amount);
        LocFormat1(text, sizeof(text), keys[i], parens);
        FortifyDrawTextFit(
            body,
            text,
            (Rectangle){row.x - row.width * 0.5f, row.y + i * row.height, row.width * 2.0f, row.height},
            row.height * 0.75f,
            FORTIFY_TEXT_COLOR
        );
    }
}

static void FortifyDrawTotal(Rectangle row, GuiId icon, int delta) {
    UiDrawTexture(GuiTex(icon), FortifyFit(row, FORTIFY_CONFIRM_TOTAL_ICON), WHITE);
    char text[16];
    FortifyFormatSigned(text, sizeof(text), delta);
    Rectangle label = FortifyFit(row, FORTIFY_CONFIRM_TOTAL_LABEL);
    UiDrawTextLeftMiddle(GuiFont(FNT_COOL), text, label, label.height * 0.5f, FortifyDeltaColor(delta));
}

static void FortifyConfirm(FortifyScreen *f, Heroes *h, Player *p);

static void FortifyDrawConfirm(FortifyScreen *f, Heroes *h, Player *p) {
    Rectangle content = FortifyFit(FortifyPopupContainer(), FORTIFY_POPUP_CONTENT);
    Rectangle title = FortifyFit(content, FORTIFY_POPUP_TITLE);
    Rectangle buttons = FortifyFit(content, FORTIFY_CONFIRM_BUTTONS);
    FortifyDrawPopupPanel(title, buttons);
    Font title_font = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    MessagePopupDrawText(title_font, LocGet("FORTIFY_CONFIRM_TITLE"), title, 30.0f, FORTIFY_TEXT_COLOR);
    MessagePopupDrawText(
        body, LocGet("FORTIFY_CONFIRM_BODY"), FortifyFit(content, FORTIFY_CONFIRM_BODY), 20.0f, FORTIFY_TEXT_COLOR
    );

    const Character *parent = &p->roster[f->parent];
    int slots[FORTIFY_SLOT_COUNT], count = 0;
    FortifyBonus power = {0}, health = {0};
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) {
        if (f->pending[slot] < 0) continue;
        slots[count++] = slot;
        FortifyBonus delta = FortifyPendingDelta(f, p, slot);
        FortifyBonus *sum = FortifySlotDefAt(parent, slot)->stat == FORTIFY_STAT_HEALTH ? &health : &power;
        sum->rarity += delta.rarity;
        sum->faction += delta.faction;
        sum->tier += delta.tier;
    }
    Rectangle area = FortifyFit(content, FORTIFY_CONFIRM_SLOTS);
    int rows = count > FORTIFY_SLOTS_PER_ROW ? 2 : 1;
    float side = fminf(area.width / FORTIFY_SLOTS_PER_ROW, area.height / rows * 0.8f);
    for (int i = 0; i < count; i++) {
        int row = i / FORTIFY_SLOTS_PER_ROW,
            in_row = rows == 2 && row == 0 ? FORTIFY_SLOTS_PER_ROW : count - row * FORTIFY_SLOTS_PER_ROW;
        if (in_row > FORTIFY_SLOTS_PER_ROW) in_row = FORTIFY_SLOTS_PER_ROW;
        int column = i % FORTIFY_SLOTS_PER_ROW;
        float start = area.x + (area.width - in_row * side) * 0.5f;
        float top = area.y + (area.height - rows * side / 0.8f) * 0.5f + row * side / 0.8f;
        Rectangle rect = {start + column * side, top - side * FORTIFY_CONFIRM_SLOT_LIFT + side * 0.25f, side, side};
        FortifyChild shown = FortifyShownChild(f, p, slots[i]);
        int delta = FortifyBonusTotal(FortifyPendingDelta(f, p, slots[i]));
        FortifyDrawSlot(f, FortifySlotDefAt(parent, slots[i]), &shown, false, rect, false, true, delta, delta);
    }

    FortifyDrawBreakdown(
        FortifyFit(content, FORTIFY_CONFIRM_POWER_TALLY),
        &FORTIFY_CONFIRM_POWER_TALLY_LABEL,
        &FORTIFY_CONFIRM_POWER_TALLY_ICON,
        GUI_DETAILS_POWER,
        power
    );
    FortifyDrawBreakdown(
        FortifyFit(content, FORTIFY_CONFIRM_HEALTH_TALLY),
        &FORTIFY_CONFIRM_HEALTH_TALLY_LABEL,
        &FORTIFY_CONFIRM_HEALTH_TALLY_ICON,
        GUI_FORTIFY_HEALTH_ICON,
        health
    );
    Rectangle totals = FortifyFit(content, FORTIFY_CONFIRM_TOTALS);
    UiDrawTexture(GuiTex(GUI_FORTIFY_STATS_CONFIRM), FortifyFit(totals, FORTIFY_CONFIRM_TOTALS_BACKGROUND), WHITE);
    FortifyDrawTotal(FortifyFit(totals, FORTIFY_CONFIRM_POWER_TOTAL), GUI_DETAILS_POWER, FortifyBonusTotal(power));
    FortifyDrawTotal(
        FortifyFit(totals, FORTIFY_CONFIRM_HEALTH_TOTAL), GUI_FORTIFY_HEALTH_ICON, FortifyBonusTotal(health)
    );

    if (ButtonDraw(&f->confirm_edit, FortifyFit(buttons, FORTIFY_CONFIRM_EDIT))) f->dialog = FORTIFY_DIALOG_NONE;
    if (ButtonDraw(&f->confirm_ok, FortifyFit(buttons, FORTIFY_CONFIRM_OK))) FortifyConfirm(f, h, p);
}

static void FortifyConfirm(FortifyScreen *f, Heroes *h, Player *p) {
    FortifyPlacement placements[FORTIFY_SLOT_COUNT];
    int units[FORTIFY_SLOT_COUNT], count = 0;
    for (int slot = 0; slot < FORTIFY_SLOT_COUNT; slot++) {
        if (f->pending[slot] < 0) continue;
        placements[count] = (FortifyPlacement){slot, f->pending[slot]};
        units[count++] = f->pending[slot];
    }
    int power = FortifyPendingStatDelta(f, p, FORTIFY_STAT_POWER);
    int health = FortifyPendingStatDelta(f, p, FORTIFY_STAT_HEALTH);
    cJSON *action = FortifyServerAction(p, f->parent, placements, count);
    int parent = PlayerFortify(p, f->parent, placements, count);
    if (parent < 0) {
        cJSON_Delete(action);
        f->dialog = FORTIFY_DIALOG_NONE;
        FortifyClearPending(f);
        return;
    }
    ProgressionSave(p);
    ServerActionSubmit(action);
    HeroesApplyConsume(h, p, units, count, parent);
    RosterRefreshOrder(&f->roster, p);
    FortifySelectParent(f, parent, true);
    f->celebrated = parent;
    f->celebrated_power = power;
    f->celebrated_health = health;
    f->celebration_time = 0.0f;
    f->tally_started = f->tally_finished = false;
    f->tally_loop_left = 0.0f;
    f->dialog = FORTIFY_DIALOG_CELEBRATION;
    GuiSoundPlay(SND_CONSUME_LEVEL_1);
}

static float FortifyTallyProgress(float time, float delay) {
    return Clamp((time - delay) / FORTIFY_TALLY_SECONDS, 0.0f, 1.0f);
}

static void FortifyUpdateTallySound(FortifyScreen *f, float power_delay, float health_delay, float dt) {
    float first =
        fminf(f->celebrated_power ? power_delay : health_delay, f->celebrated_health ? health_delay : power_delay);
    float last = fmaxf(f->celebrated_power ? power_delay : 0.0f, f->celebrated_health ? health_delay : 0.0f) +
                 FORTIFY_TALLY_SECONDS;
    if (!f->celebrated_power && !f->celebrated_health) return;
    bool running = f->celebration_time >= first && f->celebration_time < last;
    if (running) {
        f->tally_started = true;
        f->tally_loop_left -= dt;
        if (f->tally_loop_left <= 0.0f) {
            GuiSoundPlay(SND_FORTIFY_TALLY_LOOP);
            float length = GuiSoundSeconds(SND_FORTIFY_TALLY_LOOP);
            f->tally_loop_left = length > 0.0f ? length : FORTIFY_TALLY_SECONDS;
        }
    } else if (f->tally_started && !f->tally_finished && f->celebration_time >= last) {
        f->tally_finished = true;
        GuiSoundStop(SND_FORTIFY_TALLY_LOOP);
        GuiSoundPlay(SND_FORTIFY_TALLY_END);
    }
}

static void FortifyDrawTalliedStat(Rectangle row, GuiId icon, int stat, int bonus, float progress) {
    UiDrawTexture(GuiTex(icon), FortifyFit(row, FORTIFY_CELEBRATION_STAT_ICON), WHITE);
    Rectangle label = FortifyFit(row, FORTIFY_CELEBRATION_STAT_LABEL);
    int counted = (int)lroundf(bonus * progress);
    char text[64];
    int left = bonus - counted;
    if (left == 0) {
        snprintf(text, sizeof(text), "%d", stat);
    } else {
        char amount[16];
        FortifyFormatSigned(amount, sizeof(amount), left);
        snprintf(
            text, sizeof(text), "%d <color=%s>(%s)</color>", stat - left, left > 0 ? "#00FF10" : "#FF0000", amount
        );
    }
    HeroesDrawRich(
        GuiFont(FNT_COOL),
        text,
        (Rectangle){label.x, label.y + label.height * 0.2f, label.width * 1.5f, label.height},
        label.height * 0.55f,
        WHITE
    );
}

static void FortifyDrawCelebration(FortifyScreen *f, Heroes *h, const Player *p, float dt) {
    const Character *parent = &p->roster[f->celebrated];
    Rectangle content = FortifyFit(FortifyPopupContainer(), FORTIFY_POPUP_CONTENT);
    Rectangle title = FortifyFit(content, FORTIFY_POPUP_TITLE);
    Rectangle instructions = FortifyFit(content, FORTIFY_CELEBRATION_INSTRUCTIONS);
    FortifyDrawPopupPanel(title, instructions);
    f->celebration_time += dt;
    Font title_font = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    char text[256];
    LocFormat1(text, sizeof(text), "FORTIFY_CELEBRATION_TITLE", CharacterName(parent));
    MessagePopupDrawText(title_font, text, title, 30.0f, FORTIFY_TEXT_COLOR);

    Rectangle unit = FortifyFit(content, FORTIFY_CELEBRATION_UNIT);
    Rectangle burst = FortifyFit(unit, FORTIFY_CELEBRATION_BURST);
    float pulse = fabsf(fmodf(f->celebration_time / FORTIFY_BURST_PULSE_SECONDS, 2.0f) - 1.0f);
    float scale = Lerp(FORTIFY_BURST_SMALLEST_SCALE, 1.0f, pulse);
    Rectangle scaled = UiRect(burst, 0.5f, 0.5f, burst.width * scale, burst.height * scale);
    FortifyDrawFlipped(
        GuiTex(GUI_FORTIFY_STARBURST),
        scaled,
        false,
        false,
        fmodf(f->celebration_time * FORTIFY_BURST_DEGREES_PER_SECOND, 360.0f),
        WHITE
    );
    UiDrawTexture(h->cards[f->celebrated].portrait, FortifyFit(unit, FORTIFY_CELEBRATION_PORTRAIT), WHITE);
    Rectangle xp = FortifyFit(unit, FORTIFY_CELEBRATION_XP);
    UiDrawTexture(GuiTex(GUI_FORTIFY_XP_BG), FortifyFit(xp, FORTIFY_PARENT_XP_BACKGROUND), WHITE);
    float fraction = h->cards[f->celebrated].xp_frac;
    Texture2D fill = GuiTex(GUI_FORTIFY_XP_FILL);
    DrawTexturePro(
        fill,
        (Rectangle){0, 0, fill.width * fraction, (float)fill.height},
        (Rectangle){xp.x, xp.y, xp.width * fraction, xp.height},
        (Vector2){0, 0},
        0.0f,
        WHITE
    );
    Rectangle level = FortifyFit(unit, FORTIFY_CELEBRATION_LEVEL);
    UiDrawTexture(GuiTex(GUI_FORTIFY_LEVEL), level, WHITE);
    snprintf(text, sizeof(text), "%d", parent->level);
    UiDrawTextCentered(title_font, text, level, level.height * 0.55f, WHITE);

    float power_delay = FORTIFY_TALLY_DELAY;
    float health_delay = f->celebrated_power ? FORTIFY_SECOND_TALLY_DELAY : FORTIFY_TALLY_DELAY;
    FortifyUpdateTallySound(f, power_delay, health_delay, dt);
    Rectangle bonus = FortifyFit(content, FORTIFY_CELEBRATION_BONUS);
    UiDrawTexture(GuiTex(GUI_FORTIFY_STATS_CONFIRM), FortifyFit(bonus, FORTIFY_CELEBRATION_BONUS_BACKGROUND), WHITE);
    FortifyDrawTalliedStat(
        FortifyFit(bonus, FORTIFY_CELEBRATION_POWER),
        GUI_DETAILS_POWER,
        CharacterStatPower(parent),
        f->celebrated_power,
        FortifyTallyProgress(f->celebration_time, power_delay)
    );
    FortifyDrawTalliedStat(
        FortifyFit(bonus, FORTIFY_CELEBRATION_HEALTH),
        GUI_FORTIFY_HEALTH_ICON,
        CharacterStatHealth(parent),
        f->celebrated_health,
        FortifyTallyProgress(f->celebration_time, health_delay)
    );

    Rectangle max = FortifyFit(content, FORTIFY_CELEBRATION_MAX);
    MessagePopupDrawText(
        title_font,
        LocGet("FORTIFY_CELEBRATION_MAX_STATS"),
        FortifyFit(max, FORTIFY_CELEBRATION_MAX_TITLE),
        22.0f,
        FORTIFY_TEXT_COLOR
    );
    Rectangle max_stats = FortifyFit(max, FORTIFY_CELEBRATION_MAX_STATS);
    const TierStats *st = &parent->stats[parent->tier];
    int max_power =
        st->base_power + (st->max_level - 1) * st->power_per_level + FortifyMaxBonus(parent, FORTIFY_STAT_POWER);
    int max_health =
        st->base_health + (st->max_level - 1) * st->health_per_level + FortifyMaxBonus(parent, FORTIFY_STAT_HEALTH);
    const FortifyAnchor *anchors[2] = {&FORTIFY_CELEBRATION_MAX_POWER, &FORTIFY_CELEBRATION_MAX_HEALTH};
    GuiId icons[2] = {GUI_DETAILS_POWER, GUI_FORTIFY_HEALTH_ICON};
    int values[2] = {max_power, max_health};
    for (int i = 0; i < 2; i++) {
        Rectangle row = FortifyFit(max_stats, *anchors[i]);
        UiDrawTexture(GuiTex(icons[i]), FortifyFit(row, FORTIFY_CELEBRATION_MAX_ICON), WHITE);
        snprintf(text, sizeof(text), "%d", values[i]);
        UiDrawTextLeftMiddle(title_font, text, row, row.height * 0.45f, WHITE);
    }
    MessagePopupDrawText(
        body, LocGet("FORTIFY_CELEBRATION_INSTRUCTIONS"), instructions, 20.0f, FORTIFY_SOFT_TEXT_COLOR
    );
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && f->celebration_time > FORTIFY_BONUS_MOVE_TIME) {
        if (f->tally_started && !f->tally_finished) {
            GuiSoundStop(SND_FORTIFY_TALLY_LOOP);
            GuiSoundPlay(SND_FORTIFY_TALLY_END);
        }
        f->dialog = FORTIFY_DIALOG_NONE;
    }
}

static void FortifyDrawWarning(FortifyScreen *f, const Player *p) {
    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    bool fortified = f->dialog == FORTIFY_DIALOG_WARN_FORTIFIED;
    Rectangle body = UiRect(content, 0.0f, 0.0f, 1.0f, 0.5f);
    MessagePopupDrawText(
        GuiFont(FNT_MENU),
        LocGet(fortified ? "FORTIFY_WARN_HAS_CHILD_UNITS_BODY" : CONSUME_GEAR_WARNING_KEY),
        body,
        24.0f,
        WHITE
    );
    Rectangle check_row = {content.x, body.y + body.height, content.width, content.height * 0.187f};
    float box_size = check_row.height * 0.8f;
    UiDrawTexture(
        GuiTex(f->warning_checked ? GUI_CONSUME_CHECK_ON : GUI_CONSUME_CHECK_OFF),
        UiRect(check_row, 0.14f, 0.5f, box_size, box_size),
        WHITE
    );
    UiDrawTextLeftMiddle(
        GuiFont(FNT_MENU),
        LocGet(fortified ? "FORTIFY_WARN_HAS_CHILD_UNITS_CHECKBOX" : "CONSUME_CONFIRMATION_DISABLE_WARNING"),
        UiRect(check_row, 1.0f, 0.5f, 0.75f, 1.0f),
        20.0f,
        WHITE
    );
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), check_row))
        f->warning_checked = !f->warning_checked;
    Rectangle row = MessagePopupChoiceRow(popup);
    bool yes = ButtonDraw(&f->dialog_yes, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f));
    bool no = ButtonDraw(&f->dialog_no, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f));
    if (!yes && !no) return;
    if (fortified)
        f->fortified_warning_disabled = f->warning_checked;
    else
        f->gear_warning_disabled = f->warning_checked;
    f->dialog = FORTIFY_DIALOG_NONE;
    if (yes) FortifyPlace(f, p, f->warn_slot, f->warn_unit);
}

static void FortifyDrawMessage(FortifyScreen *f) {
    Rectangle popup = MessagePopupDrawFrame();
    MessagePopupDrawText(
        GuiFont(FNT_MENU), f->message, UiRect(MessagePopupContent(popup), 0.0f, 0.0f, 1.0f, 0.6f), 24.0f, WHITE
    );
    if (ButtonDraw(&f->dialog_ok, MessagePopupOkButton(popup))) f->dialog = FORTIFY_DIALOG_NONE;
}

static void FortifyRemove(FortifyScreen *f, Heroes *h, Player *p, int slot) {
    const Character *parent = &p->roster[f->parent];
    if (p->gems < FortifyRemoveCost(parent, slot)) {
        char text[FORTIFY_MESSAGE_CAP];
        snprintf(
            text, sizeof(text), "%s\n%s", LocGet("MESSAGE_CANT_AFFORD_HEADER"), LocGet("MESSAGE_CANT_AFFORD_BODY")
        );
        FortifyShowMessage(f, text);
        return;
    }
    cJSON *action = FortifyRemoveServerAction(p, f->parent, slot);
    if (PlayerRemoveFortify(p, f->parent, slot, NULL) == 0) {
        cJSON_Delete(action);
        f->dialog = FORTIFY_DIALOG_NONE;
        return;
    }
    ProgressionSave(p);
    ServerActionSubmit(action);
    HeroesSyncRoster(h, p);
    HeroesRebuildCard(h, p, f->parent);
    RosterRefreshOrder(&f->roster, p);
    if (!FortifyHasChildren(&p->roster[f->parent])) f->remove_mode = false;
    f->dialog = FORTIFY_DIALOG_NONE;
    GuiSoundPlay(SND_RUNE_PICKUP);
}

static void FortifyDrawRemoveCosts(Rectangle area) {
    Font body = GuiFont(FNT_MENU);
    MessagePopupDrawText(
        body,
        LocGet("HODA_UI_CONFIRM_UNFORTIFY_COST_HEADER"),
        (Rectangle){area.x, area.y, area.width, area.height * 0.35f},
        18.0f,
        FORTIFY_SOFT_TEXT_COLOR
    );
    static const Rarity shown[3] = {RARITY_EPIC, RARITY_LEGENDARY, RARITY_MYTHIC};
    static const char *const names[3] = {"RARITY_EPIC", "RARITY_LEGENDARY", "HODA_RARITY_MYTHIC"};
    for (int i = 0; i < 3; i++) {
        Rectangle cell = {
            area.x + area.width * i / 3.0f, area.y + area.height * 0.4f, area.width / 3.0f, area.height * 0.6f
        };
        int cost = fortify_catalog.remove_gem_cost[shown[i] - 1];
        char text[64];
        snprintf(text, sizeof(text), "%s: %d", LocGet(names[i]), cost);
        UiDrawTextCentered(
            body, text, (Rectangle){cell.x, cell.y, cell.width - cell.height * 0.6f, cell.height}, 16.0f, WHITE
        );
        UiDrawTexture(
            GuiTex(GUI_HUD_GEMS),
            (Rectangle){
                cell.x + cell.width - cell.height * 0.7f,
                cell.y + cell.height * 0.2f,
                cell.height * 0.6f,
                cell.height * 0.6f
            },
            WHITE
        );
    }
}

static void FortifyDrawRemove(FortifyScreen *f, Heroes *h, Player *p) {
    bool all = f->dialog == FORTIFY_DIALOG_REMOVE_ALL;
    int slot = all ? FORTIFY_REMOVE_ALL : f->remove_slot;
    const Character *parent = &p->roster[f->parent];
    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    Font title = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    MessagePopupDrawText(
        title,
        LocGet(all ? "HODA_UI_UNFORTIFY_ALL_CONFIRM_DESC" : "HODA_UI_UNFORTIFY_CONFIRM_DESC"),
        UiRect(content, 0.0f, 0.0f, 1.0f, 0.18f),
        24.0f,
        FORTIFY_TEXT_COLOR
    );
    Rectangle middle = UiRect(content, 0.0f, 0.3f, 1.0f, 0.36f);
    if (!all) {
        float side = middle.height;
        UiDrawTexture(
            FortifyPortraitFor(f, &parent->fortify[slot]), (Rectangle){middle.x, middle.y, side, side}, WHITE
        );
        middle.x += side * 1.1f;
        middle.width -= side * 1.1f;
    }
    MessagePopupDrawText(body, LocGet("HODA_UI_UNFORTIFY_CONFIRM_DESC_NOTE"), middle, 18.0f, WHITE);
    FortifyDrawRemoveCosts(UiRect(content, 0.0f, 0.86f, 1.0f, 0.22f));

    char label[64];
    snprintf(label, sizeof(label), "%s (%d)", LocGet("HODA_UI_CONFIRM_UNFORTIFY"), FortifyRemoveCost(parent, slot));
    f->remove_ok.label = label;
    Rectangle row = UiRect(UiRect(popup, 0.0f, 0.97f, 1.0f, 0.17f), 0.5f, 0.5f, all ? 0.56f : 0.84f, 0.8f);
    int columns = all ? 2 : 3;
    bool cancel = ButtonDraw(&f->remove_cancel, UiRect(row, 0.0f, 0.0f, 0.95f / columns, 1.0f));
    bool remove = ButtonDraw(&f->remove_ok, UiRect(row, 1.0f, 0.0f, 0.95f / columns, 1.0f));
    bool remove_all = !all && FortifyFilledCount(parent) > 1 &&
                      ButtonDraw(&f->remove_all, UiRect(row, 0.5f, 0.0f, 0.95f / columns, 1.0f));
    f->remove_ok.label = NULL;
    if (cancel) f->dialog = FORTIFY_DIALOG_NONE;
    if (remove_all) f->dialog = FORTIFY_DIALOG_REMOVE_ALL;
    if (remove) FortifyRemove(f, h, p, slot);
}

static void FortifyDrawDialog(FortifyScreen *f, Heroes *h, Player *p, float dt) {
    switch (f->dialog) {
        case FORTIFY_DIALOG_MESSAGE:
            FortifyDrawMessage(f);
            break;
        case FORTIFY_DIALOG_WARN_FORTIFIED:
        case FORTIFY_DIALOG_WARN_GEAR:
            FortifyDrawWarning(f, p);
            break;
        case FORTIFY_DIALOG_CONFIRM:
            FortifyDrawConfirm(f, h, p);
            break;
        case FORTIFY_DIALOG_CELEBRATION:
            FortifyDrawCelebration(f, h, p, dt);
            break;
        case FORTIFY_DIALOG_REMOVE:
        case FORTIFY_DIALOG_REMOVE_ALL:
            FortifyDrawRemove(f, h, p);
            break;
        default:
            break;
    }
}

static void FortifyDrawUnitDetails(FortifyScreen *f, Heroes *h, bool input) {
    if (f->details_index < 0) return;
    HeroesDrawDetails(
        &h->cards[f->details_index], ROSTER_PANEL_WIDTH, UiWidth() - ROSTER_PANEL_WIDTH - FORTIFY_DETAILS_RIGHT_MARGIN
    );
    if (!input) return;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) f->details_armed = true;
    if (f->details_armed && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        f->details_index = -1;
        f->details_armed = false;
    }
}

bool FortifyHandleBack(FortifyScreen *f) {
    if (f->dialog != FORTIFY_DIALOG_NONE) {
        if (f->dialog == FORTIFY_DIALOG_CELEBRATION && f->tally_started && !f->tally_finished)
            GuiSoundStop(SND_FORTIFY_TALLY_LOOP);
        f->dialog = FORTIFY_DIALOG_NONE;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (f->details_index >= 0) {
        f->details_index = -1;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (f->remove_mode) {
        f->remove_mode = false;
        return true;
    }
    if (FortifyHasPending(f)) {
        FortifyClearPending(f);
        return true;
    }
    if (f->parent >= 0) {
        FortifySelectParent(f, -1, true);
        return true;
    }
    f->leave_done = FORTIFY_LEAVE_SQUAD;
    return true;
}

FortifyLeave FortifyDraw(FortifyScreen *f, Heroes *h, Player *p, float dt) {
    bool blocked = UiInputBlocked() || f->dialog != FORTIFY_DIALOG_NONE;
    bool input = !blocked && f->details_index < 0;
    FortifyLayout l = FortifyLayoutCompute();
    FortifyUpdateHighlight(f, dt);

    FortifyDrawRoster(f, h, p, input);
    UiDrawTexture(GuiTex(GUI_FORTIFY_FRAME), l.frame, WHITE);
    bool parent_selected = f->parent >= 0;
    FortifyDrawPaths(f, &l, parent_selected);
    FortifyDrawParent(f, &l, h, p);
    if (parent_selected) FortifyDrawStats(f, &l, p);
    FortifyDrawSlots(f, &l, p);
    FortifyDrawInstructions(f, &l, FortifyHasPending(f) || FortifyModeButtonShown(f, p));
    FortifyDrawActions(f, &l, p, input);
    FortifyDrawBonusFly(f, &l, dt);
    if (!FortifyHasPending(f) && !f->dragging) {
        HeroesButton nav =
            HeroesDrawNav(h, HEROES_FORTIFY, input, RuneUnseenCount(&p->runes, RuneNow()), p->gallery.badge);
        if (nav == HEROES_SQUAD) FortifyRequestLeave(f, FORTIFY_LEAVE_SQUAD);
        if (nav == HEROES_CONSUME) FortifyRequestLeave(f, FORTIFY_LEAVE_CONSUME);
        if (nav == HEROES_RUNES) FortifyRequestLeave(f, FORTIFY_LEAVE_RUNES);
        if (nav == HEROES_GALLERY) FortifyRequestLeave(f, FORTIFY_LEAVE_GALLERY);
    }
    if (input || f->press != FORTIFY_PRESS_NONE) FortifyUpdatePress(f, &l, p, !blocked);
    FortifyDrawDragged(f, h);
    FortifyDrawUnitDetails(f, h, !blocked);
    FortifyDrawDialog(f, h, p, dt);

    FortifyLeave done = f->leave_done;
    f->leave_done = FORTIFY_LEAVE_NONE;
    return done;
}
