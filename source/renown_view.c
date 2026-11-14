#define RENOWN_ACTION_UPGRADE "renown_upgrade"
#define RENOWN_ACTION_PERK_FIELD "perk"
#define RENOWN_VIEW_BRANCH_COUNT 3
#define RENOWN_VIEW_BRANCH_PERKS (RENOWN_PERK_COUNT / RENOWN_VIEW_BRANCH_COUNT)
#define RENOWN_VIEW_PANEL_WIDTH 940.0f
#define RENOWN_VIEW_PANEL_HEIGHT 597.0f
#define RENOWN_VIEW_PANEL_ANCHOR_Y 0.6f
#define RENOWN_VIEW_CLOSE_SIZE 61.0f
#define RENOWN_VIEW_TITLE_Y 38.0f
#define RENOWN_VIEW_TITLE_SIZE 42.0f
#define RENOWN_VIEW_GRID_HEADER 28.0f
#define RENOWN_VIEW_GRID_GAP 8.0f
#define RENOWN_VIEW_CELL_ICON 62.0f
#define RENOWN_VIEW_CELL_TEXT_X 76.0f
#define RENOWN_VIEW_PIP_WIDTH 23.2f
#define RENOWN_VIEW_PIP_HEIGHT 14.5f
#define RENOWN_VIEW_PIP_SOURCE_WIDTH 64.0f
#define RENOWN_VIEW_PIP_SOURCE_HEIGHT 32.0f
#define RENOWN_VIEW_UPGRADE_LABEL_SCALE 0.34f
#define RENOWN_VIEW_TEXT_CAP 96
#define RENOWN_VIEW_PERMILLE_PER_PCT 10
#define RENOWN_VIEW_SCRIM ((Color){0, 0, 0, 170})
#define RENOWN_VIEW_TEXT ((Color){235, 232, 220, 255})
#define RENOWN_VIEW_DIM ((Color){182, 178, 168, 255})
#define RENOWN_VIEW_GOLD ((Color){236, 200, 120, 255})
#define RENOWN_VIEW_GREEN ((Color){132, 214, 112, 255})
#define RENOWN_VIEW_LINE ((Color){120, 104, 76, 160})
#define RENOWN_VIEW_SHADE ((Color){0, 0, 0, 130})
#define RENOWN_VIEW_CARD_SHADE ((Color){0, 0, 0, 110})
#define RENOWN_VIEW_SELECTED_SHADE ((Color){60, 46, 20, 170})
#define RENOWN_VIEW_UNLEARNED_TINT ((Color){120, 120, 120, 255})
#define RENOWN_VIEW_BAR_TOP ((Color){226, 176, 64, 255})
#define RENOWN_VIEW_BAR_BOTTOM ((Color){150, 96, 22, 255})

static const Rectangle RENOWN_VIEW_CARD = {62.0f, 80.0f, 252.0f, 472.0f};
static const Rectangle RENOWN_VIEW_EMBLEM = {56.0f, 64.0f, 264.0f, 264.0f};
static const Rectangle RENOWN_VIEW_RANK = {62.0f, 306.0f, 252.0f, 34.0f};
static const Rectangle RENOWN_VIEW_TIER = {62.0f, 338.0f, 252.0f, 24.0f};
static const Rectangle RENOWN_VIEW_NEXT_RANK = {62.0f, 372.0f, 252.0f, 22.0f};
static const Rectangle RENOWN_VIEW_BAR = {70.0f, 394.0f, 236.0f, 34.0f};
static const Rectangle RENOWN_VIEW_POINTS = {88.0f, 442.0f, 200.0f, 46.0f};
static const Rectangle RENOWN_VIEW_POINTS_ICON = {102.0f, 446.0f, 38.0f, 38.0f};
static const Rectangle RENOWN_VIEW_POINTS_COUNT = {146.0f, 442.0f, 34.0f, 46.0f};
static const Rectangle RENOWN_VIEW_POINTS_LABEL = {180.0f, 442.0f, 104.0f, 46.0f};
static const Rectangle RENOWN_VIEW_CARD_DIVIDER = {78.0f, 498.0f, 220.0f, 1.0f};
static const Rectangle RENOWN_VIEW_NEXT_EMBLEM = {76.0f, 504.0f, 48.0f, 48.0f};
static const Rectangle RENOWN_VIEW_NEXT_TIER_RANK = {132.0f, 506.0f, 176.0f, 22.0f};
static const Rectangle RENOWN_VIEW_NEXT_TIER_NAME = {132.0f, 528.0f, 176.0f, 22.0f};
static const Rectangle RENOWN_VIEW_GRID = {328.0f, 80.0f, 550.0f, 362.0f};
static const Rectangle RENOWN_VIEW_DETAIL = {328.0f, 452.0f, 550.0f, 100.0f};
static const Rectangle RENOWN_VIEW_DETAIL_ICON = {340.0f, 466.0f, 72.0f, 72.0f};
static const Rectangle RENOWN_VIEW_DETAIL_NAME = {424.0f, 460.0f, 250.0f, 30.0f};
static const Rectangle RENOWN_VIEW_DETAIL_DESCRIPTION = {424.0f, 490.0f, 250.0f, 24.0f};
static const Rectangle RENOWN_VIEW_DETAIL_VALUES = {424.0f, 518.0f, 250.0f, 26.0f};
static const Rectangle RENOWN_VIEW_UPGRADE = {688.0f, 462.0f, 176.0f, 56.0f};
static const Rectangle RENOWN_VIEW_UPGRADE_NOTE = {682.0f, 520.0f, 188.0f, 26.0f};

typedef enum {
    RENOWN_VALUE_PERCENT,
    RENOWN_VALUE_PERMILLE,
    RENOWN_VALUE_FLAT,
} RenownValueKind;

typedef struct {
    GuiId icon;
    const char *name_key;
    const char *description_key;
    RenownValueKind value_kind;
} RenownPerkText;

static const RenownPerkText RENOWN_PERK_TEXT[RENOWN_PERK_COUNT] = {
    [RENOWN_PERK_POWER] =
        {GUI_DETAILS_POWER, "HODA_RENOWN_PERK_POWER", "HODA_RENOWN_PERK_POWER_DESCRIPTION", RENOWN_VALUE_PERCENT},
    [RENOWN_PERK_HEALTH] =
        {GUI_MM_HEALTH, "HODA_RENOWN_PERK_HEALTH", "HODA_RENOWN_PERK_HEALTH_DESCRIPTION", RENOWN_VALUE_PERCENT},
    [RENOWN_PERK_CRIT] =
        {GUI_DETAILS_CRIT, "HODA_RENOWN_PERK_CRIT", "HODA_RENOWN_PERK_CRIT_DESCRIPTION", RENOWN_VALUE_PERMILLE},
    [RENOWN_PERK_GOLD] =
        {GUI_HUD_GOLD, "HODA_RENOWN_PERK_GOLD", "HODA_RENOWN_PERK_GOLD_DESCRIPTION", RENOWN_VALUE_PERCENT},
    [RENOWN_PERK_VOUCHER] =
        {GUI_RENOWN_VOUCHER, "HODA_RENOWN_PERK_VOUCHER", "HODA_RENOWN_PERK_VOUCHER_DESCRIPTION", RENOWN_VALUE_PERCENT},
    [RENOWN_PERK_KEY] =
        {GUI_NEXUS_KEY_1, "HODA_RENOWN_PERK_KEY", "HODA_RENOWN_PERK_KEY_DESCRIPTION", RENOWN_VALUE_PERCENT},
    [RENOWN_PERK_HERO_XP] =
        {GUI_CARD_LEVEL, "HODA_RENOWN_PERK_HERO_XP", "HODA_RENOWN_PERK_HERO_XP_DESCRIPTION", RENOWN_VALUE_PERCENT},
    [RENOWN_PERK_ENERGY] =
        {GUI_HUD_ENERGY, "HODA_RENOWN_PERK_ENERGY", "HODA_RENOWN_PERK_ENERGY_DESCRIPTION", RENOWN_VALUE_FLAT},
    [RENOWN_PERK_STAMINA] = {
        GUI_HUD_STAMINA, "HODA_RENOWN_PERK_STAMINA", "HODA_RENOWN_PERK_STAMINA_DESCRIPTION", RENOWN_VALUE_FLAT
    },
};

static const char *const RENOWN_BRANCH_KEYS[RENOWN_VIEW_BRANCH_COUNT] = {
    "HODA_RENOWN_BRANCH_BATTLE",
    "HODA_RENOWN_BRANCH_SPOILS",
    "HODA_RENOWN_BRANCH_ENDURANCE",
};

static const char *const RENOWN_TIER_NAME_KEYS[RENOWN_TIER_COUNT] = {
    [RENOWN_TIER_NONE] = "HODA_RENOWN_TIER_NONE",
    [RENOWN_TIER_BRONZE] = "HODA_RENOWN_TIER_BRONZE",
    [RENOWN_TIER_SILVER] = "HODA_RENOWN_TIER_SILVER",
    [RENOWN_TIER_GOLD] = "HODA_RENOWN_TIER_GOLD",
    [RENOWN_TIER_CRIMSON] = "HODA_RENOWN_TIER_CRIMSON",
    [RENOWN_TIER_ROYAL] = "HODA_RENOWN_TIER_ROYAL",
};

typedef struct {
    bool open;
    bool scrim_armed;
    int selected;
    int armed_perk;
    Button close;
    Button upgrade;
    Button upgrade_locked;
} RenownView;

void RenownViewInitialize(RenownView *v) {
    *v = (RenownView){.armed_perk = RENOWN_NONE};
    Font font = GuiFont(FNT_COOL);
    Vector2 centre = {0.5f, 0.5f};
    v->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&v->close, SND_UI_POPUP_CLOSE);
    v->upgrade = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("HODA_RENOWN_UPGRADE"), font, centre, RENOWN_VIEW_UPGRADE_LABEL_SCALE
    );
    ButtonSetLabelColor(&v->upgrade, WHITE);
    ButtonSetClickSound(&v->upgrade, SND_CONSUME_LEVEL_1);
    v->upgrade_locked = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY, LocGet("HODA_RENOWN_UPGRADE"), font, centre, RENOWN_VIEW_UPGRADE_LABEL_SCALE
    );
    ButtonSetLabelColor(&v->upgrade_locked, RENOWN_VIEW_DIM);
}

bool RenownViewIsOpen(const RenownView *v) {
    return v->open;
}

static int RenownViewFirstUpgradable(const Player *player) {
    for (int perk = 0; perk < RENOWN_PERK_COUNT; perk++)
        if (RenownCanUpgrade(&player->renown, player->experience, (RenownPerk)perk)) return perk;
    return 0;
}

void RenownViewOpen(RenownView *v, const Player *player) {
    if (!RenownAvailable()) return;
    v->open = true;
    v->scrim_armed = false;
    v->armed_perk = RENOWN_NONE;
    v->selected = RenownViewFirstUpgradable(player);
}

void RenownViewClose(RenownView *v) {
    v->open = false;
}

bool RenownViewBack(RenownView *v) {
    if (!v->open) return false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    RenownViewClose(v);
    return true;
}

static Rectangle RenownViewPanel(void) {
    return UiRect(UiRoot(), 0.5f, RENOWN_VIEW_PANEL_ANCHOR_Y, RENOWN_VIEW_PANEL_WIDTH, RENOWN_VIEW_PANEL_HEIGHT);
}

static Rectangle RenownViewPlace(Rectangle panel, Rectangle design) {
    return (Rectangle){panel.x + design.x, panel.y + design.y, design.width, design.height};
}

static void RenownViewTextCentered(Font font, const char *text, Rectangle rect, float size, Color color) {
    UiDrawTextCentered(font, text, rect, RenownFittedTextSize(font, text, rect.width, size), color);
}

static void RenownViewTextLeft(Font font, const char *text, Rectangle rect, float size, Color color) {
    UiDrawTextLeftMiddle(font, text, rect, RenownFittedTextSize(font, text, rect.width, size), color);
}

static void RenownViewValueText(char *out, size_t size, RenownPerk perk, int perk_rank) {
    int value = RenownPerkValueAtRank(perk, perk_rank);
    switch (RENOWN_PERK_TEXT[perk].value_kind) {
        case RENOWN_VALUE_PERMILLE:
            snprintf(out, size, "+%d.%d%%", value / RENOWN_VIEW_PERMILLE_PER_PCT, value % RENOWN_VIEW_PERMILLE_PER_PCT);
            break;
        case RENOWN_VALUE_PERCENT:
            snprintf(out, size, "+%d%%", value);
            break;
        default:
            snprintf(out, size, "+%d", value);
            break;
    }
}

static void RenownViewDrawBar(Rectangle bar, float fraction, const char *label) {
    UiDrawTexture(GuiTex(GUI_MISSION_PROGRESS), bar, WHITE);
    Rectangle fill = UiRect(bar, 0.5f, 0.5f, bar.width * GOALS_BAR_INNER_WIDTH, bar.height * GOALS_BAR_INNER_HEIGHT);
    fill.width *= Clamp(fraction, 0.0f, 1.0f);
    DrawRectangleGradientV(
        (int)fill.x, (int)fill.y, (int)fill.width, (int)fill.height, RENOWN_VIEW_BAR_TOP, RENOWN_VIEW_BAR_BOTTOM
    );
    RenownViewTextCentered(GuiFont(FNT_MENU), label, bar, 15.0f, WHITE);
}

static RenownTier RenownViewNextTier(RenownTier tier) {
    return tier + 1 < RENOWN_TIER_COUNT ? (RenownTier)(tier + 1) : RENOWN_TIER_NONE;
}

static void RenownViewDrawCard(Rectangle panel, const Player *player) {
    Font cool = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    Rectangle card = RenownViewPlace(panel, RENOWN_VIEW_CARD);
    DrawRectangleRec(card, RENOWN_VIEW_CARD_SHADE);
    DrawRectangleLinesEx(card, 1.0f, RENOWN_VIEW_LINE);

    int rank = RenownRank(player->experience);
    RenownTier tier = RenownTierForRank(rank);
    RenownDrawEmblem(rank, RenownViewPlace(panel, RENOWN_VIEW_EMBLEM), WHITE);

    char text[RENOWN_VIEW_TEXT_CAP];
    LocFormatInt(text, sizeof(text), "HODA_RENOWN_RANK", rank);
    RenownViewTextCentered(cool, text, RenownViewPlace(panel, RENOWN_VIEW_RANK), 30.0f, RENOWN_VIEW_TEXT);
    RenownViewTextCentered(
        body, LocGet(RENOWN_TIER_NAME_KEYS[tier]), RenownViewPlace(panel, RENOWN_VIEW_TIER), 18.0f, RENOWN_VIEW_GOLD
    );

    int floor_xp = ProgressionRenownRankXp(rank), ceiling_xp = ProgressionRenownRankXp(rank + 1);
    float span = (float)(ceiling_xp - floor_xp);
    float fraction = span > 0.0f ? (float)(player->experience - floor_xp) / span : 1.0f;
    RenownViewTextCentered(
        body, LocGet("HODA_RENOWN_NEXT_RANK"), RenownViewPlace(panel, RENOWN_VIEW_NEXT_RANK), 16.0f, RENOWN_VIEW_DIM
    );
    LocFormatInt2(text, sizeof(text), "HODA_RENOWN_XP", player->experience - floor_xp, ceiling_xp - floor_xp);
    RenownViewDrawBar(RenownViewPlace(panel, RENOWN_VIEW_BAR), fraction, text);

    UiDrawTexture(GuiTex(GUI_REWARD_BRUSH), RenownViewPlace(panel, RENOWN_VIEW_POINTS), WHITE);
    RenownDrawFit(GuiTex(GUI_MISSION_STAR), RenownViewPlace(panel, RENOWN_VIEW_POINTS_ICON), WHITE);
    snprintf(text, sizeof(text), "%d", RenownPointsFree(&player->renown, player->experience));
    RenownViewTextCentered(cool, text, RenownViewPlace(panel, RENOWN_VIEW_POINTS_COUNT), 28.0f, WHITE);
    RenownViewTextLeft(
        body, LocGet("HODA_RENOWN_POINTS"), RenownViewPlace(panel, RENOWN_VIEW_POINTS_LABEL), 18.0f, RENOWN_VIEW_TEXT
    );

    RenownTier next = RenownViewNextTier(tier);
    if (next == RENOWN_TIER_NONE) return;
    DrawRectangleRec(RenownViewPlace(panel, RENOWN_VIEW_CARD_DIVIDER), RENOWN_VIEW_LINE);
    RenownDrawFit(RenownEmblemTexture(next), RenownViewPlace(panel, RENOWN_VIEW_NEXT_EMBLEM), WHITE);
    LocFormatInt(text, sizeof(text), "HODA_RENOWN_RANK", RenownTierFirstRank(next));
    RenownViewTextLeft(body, text, RenownViewPlace(panel, RENOWN_VIEW_NEXT_TIER_RANK), 17.0f, RENOWN_VIEW_TEXT);
    RenownViewTextLeft(
        body,
        LocGet(RENOWN_TIER_NAME_KEYS[next]),
        RenownViewPlace(panel, RENOWN_VIEW_NEXT_TIER_NAME),
        16.0f,
        RENOWN_VIEW_DIM
    );
}

static void RenownViewDrawPips(Rectangle row, int perk_rank, bool preview_next) {
    for (int pip = 0; pip < RenownPerkMaxRank(); pip++) {
        bool lit = pip < perk_rank;
        bool next = preview_next && pip == perk_rank;
        GuiId sheet = lit ? GUI_FORTIFY_TIER_4 : next ? GUI_RENOWN_PIP_NEXT : GUI_FORTIFY_TIER_1;
        Rectangle source = {
            lit ? 0.0f : RENOWN_VIEW_PIP_SOURCE_WIDTH,
            0.0f,
            RENOWN_VIEW_PIP_SOURCE_WIDTH,
            RENOWN_VIEW_PIP_SOURCE_HEIGHT,
        };
        Rectangle target = {row.x + pip * RENOWN_VIEW_PIP_WIDTH, row.y, RENOWN_VIEW_PIP_WIDTH, RENOWN_VIEW_PIP_HEIGHT};
        DrawTexturePro(GuiTex(sheet), source, target, (Vector2){0}, 0.0f, WHITE);
    }
}

static Rectangle RenownViewCell(Rectangle panel, int perk) {
    Rectangle grid = RenownViewPlace(panel, RENOWN_VIEW_GRID);
    float gap = RENOWN_VIEW_GRID_GAP;
    float width = (grid.width - gap * (RENOWN_VIEW_BRANCH_COUNT - 1)) / RENOWN_VIEW_BRANCH_COUNT;
    float height =
        (grid.height - RENOWN_VIEW_GRID_HEADER - gap * (RENOWN_VIEW_BRANCH_PERKS - 1)) / RENOWN_VIEW_BRANCH_PERKS;
    int branch = perk / RENOWN_VIEW_BRANCH_PERKS, row = perk % RENOWN_VIEW_BRANCH_PERKS;
    return (Rectangle){
        grid.x + branch * (width + gap),
        grid.y + RENOWN_VIEW_GRID_HEADER + row * (height + gap),
        width,
        height,
    };
}

static void RenownViewDrawCell(const RenownView *v, Rectangle panel, const Player *player, int perk) {
    Font cool = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    const RenownPerkText *perk_text = &RENOWN_PERK_TEXT[perk];
    Rectangle cell = RenownViewCell(panel, perk);
    bool selected = perk == v->selected;
    int perk_rank = RenownPerkRank(&player->renown, (RenownPerk)perk);
    bool owned = perk_rank > 0, maxed = RenownPerkMaxed(&player->renown, (RenownPerk)perk);

    DrawRectangleRec(cell, selected ? RENOWN_VIEW_SELECTED_SHADE : RENOWN_VIEW_SHADE);
    DrawRectangleLinesEx(cell, selected ? 2.0f : 1.0f, selected ? RENOWN_VIEW_GOLD : RENOWN_VIEW_LINE);
    Rectangle icon = {cell.x + 8.0f, cell.y + 10.0f, RENOWN_VIEW_CELL_ICON, RENOWN_VIEW_CELL_ICON};
    RenownDrawFit(GuiTex(perk_text->icon), icon, owned ? WHITE : RENOWN_VIEW_UNLEARNED_TINT);

    float text_width = cell.width - RENOWN_VIEW_CELL_TEXT_X - 6.0f;
    Rectangle name = {cell.x + RENOWN_VIEW_CELL_TEXT_X, cell.y + 12.0f, text_width, 26.0f};
    RenownViewTextLeft(cool, LocGet(perk_text->name_key), name, 19.0f, owned ? RENOWN_VIEW_TEXT : RENOWN_VIEW_DIM);

    char value[RENOWN_VIEW_TEXT_CAP];
    RenownViewValueText(value, sizeof(value), (RenownPerk)perk, perk_rank);
    Rectangle value_rect = {cell.x + RENOWN_VIEW_CELL_TEXT_X, cell.y + 40.0f, text_width, 24.0f};
    RenownViewTextLeft(
        body,
        owned ? value : LocGet("HODA_RENOWN_PERK_UNLEARNED"),
        value_rect,
        17.0f,
        maxed   ? RENOWN_VIEW_GOLD
        : owned ? RENOWN_VIEW_GREEN
                : RENOWN_VIEW_DIM
    );

    Rectangle pips = {cell.x + 10.0f, cell.y + cell.height - 26.0f, 0.0f, 0.0f};
    bool preview_next = selected && RenownCanUpgrade(&player->renown, player->experience, (RenownPerk)perk);
    RenownViewDrawPips(pips, perk_rank, preview_next);
    if (!maxed) return;
    Rectangle tag = {cell.x + cell.width - 58.0f, cell.y + cell.height - 30.0f, 50.0f, 22.0f};
    UiDrawTextRight(cool, LocGet("HODA_RENOWN_MAX"), tag, 15.0f, RENOWN_VIEW_GOLD);
}

static void RenownViewDrawGrid(RenownView *v, Rectangle panel, const Player *player, bool interactive) {
    Rectangle grid = RenownViewPlace(panel, RENOWN_VIEW_GRID);
    for (int branch = 0; branch < RENOWN_VIEW_BRANCH_COUNT; branch++) {
        Rectangle first = RenownViewCell(panel, branch * RENOWN_VIEW_BRANCH_PERKS);
        Rectangle title = {first.x, grid.y, first.width, RENOWN_VIEW_GRID_HEADER - 4.0f};
        RenownViewTextCentered(GuiFont(FNT_COOL), LocGet(RENOWN_BRANCH_KEYS[branch]), title, 20.0f, RENOWN_VIEW_GOLD);
    }
    Vector2 mouse = UiMouse();
    int hovered = RENOWN_NONE;
    for (int perk = 0; perk < RENOWN_PERK_COUNT; perk++) {
        if (interactive && CheckCollisionPointRec(mouse, RenownViewCell(panel, perk))) hovered = perk;
        RenownViewDrawCell(v, panel, player, perk);
    }
    if (!interactive) v->armed_perk = RENOWN_NONE;
    if (interactive && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) v->armed_perk = hovered;
    if (!interactive || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
    if (hovered != RENOWN_NONE && hovered == v->armed_perk && hovered != v->selected) {
        v->selected = hovered;
        GuiSoundPlay(SND_UI_TAB);
    }
    v->armed_perk = RENOWN_NONE;
}

cJSON *RenownUpgradeAction(RenownPerk perk) {
    cJSON *fields = ServerActionCreate(RENOWN_ACTION_UPGRADE);
    if (fields) cJSON_AddStringToObject(fields, RENOWN_ACTION_PERK_FIELD, RenownPerkId(perk));
    return fields;
}

static void RenownViewSubmitUpgrade(Player *player, RenownPerk perk) {
    if (!ProgressionRenownUpgrade(player, perk)) return;
    ProgressionSave(player);
    ServerActionSubmit(RenownUpgradeAction(perk));
}

static void RenownViewDrawDetailValues(Rectangle panel, const Player *player, RenownPerk perk) {
    Font body = GuiFont(FNT_MENU);
    int perk_rank = RenownPerkRank(&player->renown, perk);
    bool maxed = RenownPerkMaxed(&player->renown, perk);
    Rectangle line = RenownViewPlace(panel, RENOWN_VIEW_DETAIL_VALUES);
    char current[RENOWN_VIEW_TEXT_CAP], next[RENOWN_VIEW_TEXT_CAP];
    RenownViewValueText(current, sizeof(current), perk, perk_rank);
    RenownViewValueText(next, sizeof(next), perk, perk_rank + 1);
    const char *shown = perk_rank > 0 ? current : LocGet("HODA_RENOWN_PERK_UNLEARNED");
    float size = 19.0f;
    UiDrawTextLeftMiddle(body, shown, line, size, maxed ? RENOWN_VIEW_GOLD : RENOWN_VIEW_TEXT);
    if (maxed) return;
    float gap = 10.0f;
    line.x += MeasureTextEx(body, shown, size, 1.0f).x + gap;
    UiDrawTextLeftMiddle(body, ">", line, size, RENOWN_VIEW_DIM);
    line.x += MeasureTextEx(body, ">", size, 1.0f).x + gap;
    UiDrawTextLeftMiddle(body, next, line, size, RENOWN_VIEW_GREEN);
}

static void RenownViewDrawDetail(RenownView *v, Rectangle panel, Player *player, bool interactive) {
    Font cool = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    RenownPerk perk = (RenownPerk)v->selected;
    const RenownPerkText *perk_text = &RENOWN_PERK_TEXT[perk];
    Rectangle detail = RenownViewPlace(panel, RENOWN_VIEW_DETAIL);
    DrawRectangleRec(detail, RENOWN_VIEW_SHADE);
    DrawRectangleLinesEx(detail, 1.0f, RENOWN_VIEW_LINE);
    RenownDrawFit(GuiTex(perk_text->icon), RenownViewPlace(panel, RENOWN_VIEW_DETAIL_ICON), WHITE);
    RenownViewTextLeft(
        cool, LocGet(perk_text->name_key), RenownViewPlace(panel, RENOWN_VIEW_DETAIL_NAME), 24.0f, RENOWN_VIEW_TEXT
    );
    RenownViewTextLeft(
        body,
        LocGet(perk_text->description_key),
        RenownViewPlace(panel, RENOWN_VIEW_DETAIL_DESCRIPTION),
        17.0f,
        RENOWN_VIEW_DIM
    );
    RenownViewDrawDetailValues(panel, player, perk);

    Rectangle button = RenownViewPlace(panel, RENOWN_VIEW_UPGRADE);
    Rectangle note = RenownViewPlace(panel, RENOWN_VIEW_UPGRADE_NOTE);
    if (RenownPerkMaxed(&player->renown, perk)) {
        RenownViewTextCentered(cool, LocGet("HODA_RENOWN_MAXED"), button, 24.0f, RENOWN_VIEW_GOLD);
        return;
    }
    if (!RenownCanUpgrade(&player->renown, player->experience, perk)) {
        ButtonDrawEx(&v->upgrade_locked, button, false);
        RenownViewTextCentered(body, LocGet("HODA_RENOWN_NO_POINTS"), note, 16.0f, RENOWN_VIEW_DIM);
        return;
    }
    bool pressed = ButtonDrawEx(&v->upgrade, button, interactive);
    RenownViewTextCentered(body, LocGet("HODA_RENOWN_COST"), note, 16.0f, RENOWN_VIEW_TEXT);
    if (pressed) RenownViewSubmitUpgrade(player, perk);
}

static bool RenownViewScrimClicked(RenownView *v, Rectangle panel) {
    bool outside = !CheckCollisionPointRec(UiMouse(), panel);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) v->scrim_armed = outside;
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return false;
    bool clicked = v->scrim_armed && outside;
    v->scrim_armed = false;
    return clicked;
}

void RenownViewDraw(RenownView *v, Player *player, bool interactive) {
    if (!v->open) return;
    DrawRectangleRec(UiRoot(), RENOWN_VIEW_SCRIM);
    Rectangle panel = RenownViewPanel();
    UiDrawTexture(GuiTex(GUI_QUESTS_BG), panel, WHITE);
    RenownViewTextCentered(
        GuiFont(FNT_COOL),
        LocGet("HODA_RENOWN_TITLE"),
        (Rectangle){
            panel.x, panel.y + RENOWN_VIEW_TITLE_Y - RENOWN_VIEW_TITLE_SIZE * 0.5f, panel.width, RENOWN_VIEW_TITLE_SIZE
        },
        RENOWN_VIEW_TITLE_SIZE,
        RENOWN_VIEW_TEXT
    );
    RenownViewDrawCard(panel, player);
    RenownViewDrawGrid(v, panel, player, interactive);
    RenownViewDrawDetail(v, panel, player, interactive);

    float close_size = RENOWN_VIEW_CLOSE_SIZE;
    Rectangle close = {
        panel.x + panel.width - close_size * 1.25f, panel.y + panel.height * 0.07f, close_size, close_size
    };
    bool closed = ButtonDrawEx(&v->close, close, interactive);
    if (interactive && (closed || RenownViewScrimClicked(v, panel))) RenownViewClose(v);
}
