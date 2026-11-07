#define GEAR_VIEW_PAGE_SIZE 40
#define GEAR_VIEW_GRID_COLUMNS 2
#define GEAR_VIEW_TILE_FILL 0.71f
#define GEAR_VIEW_ICON_FILL 0.86f
#define GEAR_CARD_TITLE_BAND_TOP 0.081f
#define GEAR_CARD_IMAGE_BAND_TOP 0.21f
#define GEAR_CARD_TEXT_BAND_TOP 0.625f
#define GEAR_CARD_TEXT_BAND_BOTTOM 0.915f
#define GEAR_CARD_TEXT_LEFT 0.097f
#define GEAR_CARD_TEXT_WIDTH 0.798f
#define GEAR_CARD_ICON_SIZE 0.322f
#define GEAR_CARD_TITLE_TEXT 0.055f
#define GEAR_CARD_RARITY_TEXT 0.0413f
#define GEAR_CARD_LIMITS_TEXT 0.0274f
#define GEAR_CARD_DESCRIPTION_TEXT 0.0274f
#define GEAR_VIEW_SLOT_WIDTH 60.0f
#define GEAR_VIEW_SLOT_HEIGHT 58.0f
#define GEAR_VIEW_SLOT_ITEM_FILL 0.7f
#define GEAR_VIEW_SLOT_LOCK_FILL 0.5f
#define GEAR_VIEW_FILTER_COUNT 8
#define GEAR_VIEW_FILTER_ROW 68.0f
#define GEAR_VIEW_SLOT_GAP 6.0f
#define GEAR_VIEW_SLOT_TOP 103.0f
#define GEAR_VIEW_PRICE_GEM_GAP 10.0f
#define GEAR_VIEW_SCROLL_SPEED 40.0f
#define GEAR_VIEW_MESSAGE_CAPACITY 640
#define GEAR_VIEW_DETAILS_CAPACITY 960
#define GEAR_VIEW_AURA_CAPACITY 640
#define GEAR_VIEW_PAGE_LABEL_CAPACITY 64

typedef struct {
    const char *title;
    int rarity;
    const char *limits;
    const char *description;
} GearInfoText;
#define GEAR_VIEW_DETAILS_FONT_MAX 15.0f
#define GEAR_VIEW_DETAILS_FONT_MIN 9.0f

typedef enum {
    GEAR_FILTER_ALL,
    GEAR_FILTER_EQUIPPABLE,
    GEAR_FILTER_COMMON,
    GEAR_FILTER_UNCOMMON,
    GEAR_FILTER_RARE,
    GEAR_FILTER_EPIC,
    GEAR_FILTER_LEGENDARY,
    GEAR_FILTER_MYTHIC
} GearFilter;

typedef enum {
    GEAR_VIEW_DIALOG_NONE,
    GEAR_VIEW_DIALOG_MESSAGE,
    GEAR_VIEW_DIALOG_EQUIP,
    GEAR_VIEW_DIALOG_SELL,
    GEAR_VIEW_DIALOG_PRESTIGE,
    GEAR_VIEW_DIALOG_ASCEND
} GearViewDialog;

typedef struct {
    Button category, sort, equip, sell, store, prev, next, yes, no, close, unlock;
    char prev_label[GEAR_VIEW_PAGE_LABEL_CAPACITY];
    char next_label[GEAR_VIEW_PAGE_LABEL_CAPACITY];
    bool open;
    int hero;
    int slot;
    int selected;
    GearFilter filter;
    bool filter_open;
    bool reversed;
    int page;
    float scroll;
    bool store_requested;
    int changed_hero;
    GearViewDialog dialog;
    char message[GEAR_VIEW_MESSAGE_CAPACITY];
} GearView;

static GearView gear_view;

static void GearViewLoadButton(Button *button, GuiId idle, GuiId active, const char *label, float scale) {
    bool armed = button->armed;
    *button = ButtonLoadText(idle, active, label, GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, scale);
    button->armed = armed;
}

static void GearViewLoadButtons(void) {
    GearViewLoadButton(&gear_view.category, GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, "", 0.32f);
    GearViewLoadButton(
        &gear_view.sort, GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, LocGet("HODA_INVENTORY_SORT"), 0.34f
    );
    GearViewLoadButton(&gear_view.equip, GUI_STORE_BUY, GUI_STORE_BUY_ON, LocGet("HODA_GEAR_EQUIP_BUTTON_TEXT"), 0.34f);
    GearViewLoadButton(&gear_view.sell, GUI_STORE_BUY, GUI_STORE_BUY_ON, LocGet("HODA_GEAR_SELL_BUTTON_TEXT"), 0.34f);
    GearViewLoadButton(&gear_view.store, GUI_STORE_BUY, GUI_STORE_BUY_ON, LocGet("STORE_TITLE"), 0.3f);
    GearViewLoadButton(&gear_view.prev, GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, "", 0.32f);
    GearViewLoadButton(&gear_view.next, GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, "", 0.32f);
    GearViewLoadButton(&gear_view.yes, GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), 0.34f);
    GearViewLoadButton(&gear_view.no, GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNES_CANCEL"), 0.34f);
    GearViewLoadButton(&gear_view.close, GUI_BTN_CLOSE, GUI_BTN_CLOSE, NULL, 0.0f);
    GearViewLoadButton(&gear_view.unlock, GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("HODA_GEAR_PRESTIGE_UNLOCK"), 0.26f);
}

bool GearViewIsOpen(void) {
    return gear_view.open || gear_view.dialog != GEAR_VIEW_DIALOG_NONE;
}

bool GearViewAcceptsInput(void) {
    return gear_view.dialog == GEAR_VIEW_DIALOG_NONE && !UiInputBlocked();
}

void GearViewClose(void) {
    gear_view.open = false;
    gear_view.dialog = GEAR_VIEW_DIALOG_NONE;
    gear_view.filter_open = false;
    gear_view.selected = -1;
}

int GearViewTakeChangedHero(void) {
    int hero = gear_view.changed_hero - 1;
    gear_view.changed_hero = 0;
    return hero;
}

bool GearViewTakeStoreRequest(void) {
    bool requested = gear_view.store_requested;
    gear_view.store_requested = false;
    return requested;
}

static void GearViewShow(int hero, int slot) {
    gear_view.open = true;
    gear_view.hero = hero;
    gear_view.slot = slot;
    gear_view.selected = -1;
    gear_view.filter_open = false;
    gear_view.filter = slot >= 0 ? GEAR_FILTER_EQUIPPABLE : GEAR_FILTER_ALL;
    gear_view.page = 0;
    gear_view.scroll = 0.0f;
}

void GearViewOpenInventory(void) {
    GearViewShow(-1, -1);
}

static void GearViewShowMessage(const char *text) {
    StrCopy(gear_view.message, sizeof(gear_view.message), text);
    gear_view.dialog = GEAR_VIEW_DIALOG_MESSAGE;
}

static bool GearViewClicked(Rectangle r) {
    return IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), r);
}

Color GearRarityColor(int rarity) {
    static const Color colors[GEAR_RARITY_COUNT] = {
        {214, 180, 143, 255},
        {242, 242, 242, 255},
        {255, 214, 51, 255},
        {255, 117, 26, 255},
        {51, 204, 51, 255},
        {255, 0, 255, 255},
    };
    return rarity >= 1 && rarity <= GEAR_RARITY_COUNT ? colors[rarity - 1] : WHITE;
}

void GearDrawTile(const GearDef *def, Rectangle r, bool selected) {
    DrawRectangleRec(UiRect(r, 0.5f, 0.5f, 0.92f, 0.92f), BLACK);
    Texture2D icon = GearIconLoad(def);
    Rectangle inner = UiRect(r, 0.5f, 0.5f, GEAR_VIEW_ICON_FILL, GEAR_VIEW_ICON_FILL);
    if (icon.id != 0) UiDrawTexture(icon, inner, WHITE);
    UiDrawTexture(GearRarityFrame(def->rarity), r, WHITE);
    if (selected) UiDrawTexture(GuiTex(GUI_CARD_SELECT), r, WHITE);
}

static void GearViewDrawNormalSlot(const GearDef *worn, Rectangle r, bool selected) {
    if (worn) {
        GearDrawTile(worn, r, selected);
        return;
    }
    UiDrawTexture(GearUiTexture("Gear_Slot_Add_Deselect"), r, WHITE);
    if (selected) UiDrawTexture(GuiTex(GUI_CARD_SELECT), r, WHITE);
}

static void GearViewDrawFramedSlot(const char *frame, const GearDef *worn, Rectangle r, bool selected) {
    UiDrawTexture(GearUiTexture(frame), r, WHITE);
    Rectangle inner = UiRect(r, 0.5f, 0.5f, GEAR_VIEW_SLOT_ITEM_FILL, GEAR_VIEW_SLOT_ITEM_FILL);
    if (worn) GearDrawTile(worn, inner, false);
    if (selected) UiDrawTexture(GuiTex(GUI_CARD_SELECT), r, WHITE);
}

static Rectangle GearViewSlotRect(Rectangle details, int row) {
    return (Rectangle){
        details.x + details.width,
        details.y + GEAR_VIEW_SLOT_TOP + row * (GEAR_VIEW_SLOT_HEIGHT + GEAR_VIEW_SLOT_GAP),
        GEAR_VIEW_SLOT_WIDTH,
        GEAR_VIEW_SLOT_HEIGHT,
    };
}

static void GearViewClickTomeSlot(const Player *p, int hero) {
    GuiClickSoundPlay();
    if (!GearCanAscend(&p->roster[hero])) {
        GearViewShowMessage(LocGet("HODA_EVO_TOME_SLOT_LOCKED_MESSAGE"));
    } else if (p->gear.tomes <= 0) {
        GearViewShowMessage(LocGet("HODA_EVO_TOME_REQUIRED_MESSAGE"));
    } else {
        gear_view.hero = hero;
        gear_view.dialog = GEAR_VIEW_DIALOG_ASCEND;
    }
}

static void GearViewDrawTomeSlot(const Player *p, int hero, Rectangle r, bool input) {
    UiDrawTexture(GearUiTexture("tex.Ico_Gear_TomeofAscension_Deselected"), r, WHITE);
    Texture2D tome = GearTomeIconLoad();
    Rectangle book = UiRect(r, 0.5f, 0.5f, GEAR_VIEW_ICON_FILL, GEAR_VIEW_ICON_FILL);
    if (tome.id != 0) UiDrawTexture(tome, book, GearCanAscend(&p->roster[hero]) ? WHITE : GRAY);
    if (input && GearViewClicked(r)) GearViewClickTomeSlot(p, hero);
}

void GearViewDrawSlots(Player *p, int hero, Rectangle details) {
    const Character *ch = &p->roster[hero];
    bool input = GearViewAcceptsInput();
    bool tome = GearHasTomeSlot(ch);
    for (int slot = 0; slot < GearSlotCount(ch); slot++) {
        bool prestige = GearSlotIsPrestige(ch, slot);
        Rectangle r = GearViewSlotRect(details, slot + (prestige && tome ? 1 : 0));
        const GearDef *worn = GearInSlot(ch, slot);
        bool locked = prestige && !ch->gear.prestige_unlocked;
        bool selected = gear_view.open && gear_view.slot == slot;
        if (prestige) {
            GearViewDrawFramedSlot("tex.Ico_Gear_Slot_Deselected", worn, r, selected);
            Rectangle lock = UiRect(r, 0.5f, 0.5f, GEAR_VIEW_SLOT_LOCK_FILL, GEAR_VIEW_SLOT_LOCK_FILL);
            if (locked) UiDrawTexture(GuiTex(GUI_GEAR_LOCK), lock, WHITE);
        } else if (ch->ascended && slot == GEAR_NORMAL_SLOT_MAX) {
            GearViewDrawFramedSlot("tex.Ico_Gear_TomeofAscension_Deselected", worn, r, selected);
        } else {
            GearViewDrawNormalSlot(worn, r, selected);
        }
        if (!input || !GearViewClicked(r)) continue;
        GuiClickSoundPlay();
        if (locked) {
            gear_view.hero = hero;
            gear_view.dialog = GEAR_VIEW_DIALOG_PRESTIGE;
        } else {
            GearViewShow(hero, slot);
        }
    }
    if (tome) GearViewDrawTomeSlot(p, hero, GearViewSlotRect(details, GEAR_NORMAL_SLOT_MAX), input);
}

static const char *GearViewEquipFailure(GearEquipResult result) {
    switch (result) {
        case GEAR_EQUIP_DUPLICATE:
            return LocGet("HODA_GEAR_EQUIP_FAIL_DUPLICATE_CONFIRM");
        case GEAR_EQUIP_AURAS_CANNOT_STACK:
            return LocGet("HODA_GEAR_EQUIP_FAIL_DEFENDER_CONFIRM");
        case GEAR_EQUIP_INVALID_PRESTIGE:
            return LocGet("HODA_GEAR_EQUIP_FAIL_INVALID_PRESTIGE_GEAR");
        default:
            return LocGet("HODA_GEAR_EQUIP_FAIL_CATEGORY_LIMIT_CONFIRM");
    }
}

static void GearViewRequestEquip(Player *p) {
    const Character *ch = &p->roster[gear_view.hero];
    GearEquipResult result = GearCheckEquip(&p->gear, ch, gear_view.slot, gear_view.selected);
    if (result != GEAR_EQUIP_OK) {
        GearViewShowMessage(GearViewEquipFailure(result));
        return;
    }
    const GearDef *replaced = GearInSlot(ch, gear_view.slot);
    if (replaced) {
        LocFormat1(gear_view.message, sizeof(gear_view.message), "HODA_GEAR_EQUIP_FILLED_CONFIRM", GearTitle(replaced));
    } else {
        StrCopy(gear_view.message, sizeof(gear_view.message), LocGet("HODA_GEAR_EQUIP_EMPTY_CONFIRM"));
    }
    gear_view.dialog = GEAR_VIEW_DIALOG_EQUIP;
}

static void GearViewRequestSell(void) {
    snprintf(
        gear_view.message,
        sizeof(gear_view.message),
        "%s\n%s %d",
        LocGet("HODA_GEAR_SELL_MAIN_CONFIRM"),
        LocGet("HODA_GEAR_SELL_RECEIVE_CONFIRM"),
        GearSellValue(GearDefAt(gear_view.selected))
    );
    gear_view.dialog = GEAR_VIEW_DIALOG_SELL;
}

static void GearViewSubmit(const char *action, const char *gear, uint64_t instance_id) {
    cJSON *fields = ServerActionCreate(action);
    if (!fields) return;
    if (gear) cJSON_AddStringToObject(fields, "gear", gear);
    if (instance_id) cJSON_AddNumberToObject(fields, "instance_id", (double)instance_id);
    ServerActionSubmit(fields);
}

static void GearViewConfirm(Player *p) {
    GearViewDialog dialog = gear_view.dialog;
    const char *submit = NULL;
    const char *gear = NULL;
    uint64_t instance_id = 0;
    gear_view.dialog = GEAR_VIEW_DIALOG_NONE;
    if (dialog == GEAR_VIEW_DIALOG_EQUIP) {
        GearEquip(&p->gear, &p->roster[gear_view.hero], gear_view.slot, gear_view.selected);
        GearViewClose();
    } else if (dialog == GEAR_VIEW_DIALOG_SELL) {
        if (PlayerSellGear(p, gear_view.selected)) {
            submit = "gear_sell";
            gear = GearDefAt(gear_view.selected)->id;
        }
        if (p->gear.counts[gear_view.selected] <= 0) gear_view.selected = -1;
    } else if (dialog == GEAR_VIEW_DIALOG_ASCEND) {
        if (GearAscend(&p->gear, &p->roster[gear_view.hero])) {
            submit = "ascend";
            instance_id = p->roster[gear_view.hero].instance_id;
            gear_view.changed_hero = gear_view.hero + 1;
            GuiSoundPlay(SND_PACK_RARE);
        }
    } else if (dialog == GEAR_VIEW_DIALOG_PRESTIGE && !PlayerUnlockPrestigeGearSlot(p, &p->roster[gear_view.hero])) {
        GearViewShowMessage(LocGet("MESSAGE_CANT_AFFORD_BODY"));
        return;
    } else if (dialog == GEAR_VIEW_DIALOG_PRESTIGE) {
        submit = "gear_prestige";
        instance_id = p->roster[gear_view.hero].instance_id;
    }
    ProgressionSave(p);
    if (submit) GearViewSubmit(submit, gear, instance_id);
}

static void GearViewDrawAscendBody(const Player *p, Rectangle content) {
    Rectangle book = UiRect(content, 0.5f, 0.0f, content.height * 0.42f, content.height * 0.42f);
    Texture2D tome = GearTomeIconLoad();
    if (tome.id != 0) UiDrawTexture(tome, book, WHITE);
    float y = book.y + book.height + 4.0f;
    Rectangle count = {content.x, y, content.width, 26.0f};
    char tomes[64];
    LocFormatInt(tomes, sizeof(tomes), "HODA_EVO_TOME_COUNT", p->gear.tomes);
    UiDrawTextCentered(GuiFont(FNT_COOL), tomes, count, 20.0f, GOLD);
    Rectangle ask = {content.x, y + 28.0f, content.width, 56.0f};
    MessagePopupDrawText(GuiFont(FNT_MENU), LocGet("HODA_EVO_TOME_USE_CONFIRM"), ask, 21.0f, WHITE);
    Rectangle bonus = {content.x, y + 88.0f, content.width, 50.0f};
    HeroesDrawRich(GuiFont(FNT_MENU), LocGet("HODA_EVO_TOME_EXPLAIN_DESC"), bonus, 17.0f, WHITE);
}

static void GearViewDrawPrestigeDialog(Player *p, Rectangle popup) {
    Rectangle text = UiRect(popup, 0.5f, 0.2f, 0.84f, 0.5f);
    MessagePopupDrawText(GuiFont(FNT_MENU), LocGet("HODA_GEAR_PRESTIGE_MESSAGE"), text, 22.0f, WHITE);
    Rectangle currency = UiRect(popup, 0.62f, 0.68f, 0.2f, 0.11f);
    UiDrawTextRight(
        GuiFont(FNT_MENU),
        LocGet("REFRESH_COST"),
        (Rectangle){0.0f, currency.y + currency.height * 0.5f, currency.x - 12.0f, 0.0f},
        22.0f,
        WHITE
    );
    UiDrawTexture(GuiTex(GUI_REWARD_BRUSH), currency, WHITE);
    Rectangle gem = UiRect(currency, 0.9f, 0.4f, currency.height * 0.9f, currency.height * 0.9f);
    UiDrawTexture(GuiTex(GUI_MAP_GEMS), gem, WHITE);
    UiDrawTextRight(
        GuiFont(FNT_COOL),
        TextFormat("%d", GearPrestigeSlotGemCost()),
        (Rectangle){0.0f, currency.y + currency.height * 0.5f, gem.x - GEAR_VIEW_PRICE_GEM_GAP, 0.0f},
        26.0f,
        WHITE
    );
    Rectangle row = MessagePopupChoiceRow(popup);
    if (ButtonDraw(&gear_view.no, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f))) gear_view.dialog = GEAR_VIEW_DIALOG_NONE;
    if (ButtonDraw(&gear_view.unlock, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f))) GearViewConfirm(p);
}

static void GearViewDrawDialog(Player *p) {
    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    if (gear_view.dialog == GEAR_VIEW_DIALOG_ASCEND) {
        GearViewDrawAscendBody(p, content);
        Rectangle row = MessagePopupChoiceRow(popup);
        if (ButtonDraw(&gear_view.no, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f))) gear_view.dialog = GEAR_VIEW_DIALOG_NONE;
        if (ButtonDraw(&gear_view.yes, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f))) GearViewConfirm(p);
        return;
    }
    if (gear_view.dialog == GEAR_VIEW_DIALOG_PRESTIGE) {
        GearViewDrawPrestigeDialog(p, popup);
        return;
    }
    MessagePopupDrawText(GuiFont(FNT_MENU), gear_view.message, content, 24.0f, WHITE);
    if (gear_view.dialog == GEAR_VIEW_DIALOG_MESSAGE) {
        if (ButtonDraw(&gear_view.yes, MessagePopupOkButton(popup))) gear_view.dialog = GEAR_VIEW_DIALOG_NONE;
        return;
    }
    Rectangle row = MessagePopupChoiceRow(popup);
    if (ButtonDraw(&gear_view.no, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f))) gear_view.dialog = GEAR_VIEW_DIALOG_NONE;
    if (ButtonDraw(&gear_view.yes, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f))) GearViewConfirm(p);
}

static bool GearViewMatchesFilter(const Player *p, GearFilter filter, int def_index) {
    if (filter == GEAR_FILTER_ALL) return true;
    if (filter == GEAR_FILTER_EQUIPPABLE) {
        if (gear_view.hero < 0 || gear_view.slot < 0) return false;
        return GearCheckEquip(&p->gear, &p->roster[gear_view.hero], gear_view.slot, def_index) == GEAR_EQUIP_OK;
    }
    return GearDefAt(def_index)->rarity == (int)filter - (int)GEAR_FILTER_COMMON + 1;
}

static int GearViewFilteredItems(const Player *p, GearFilter filter, bool reversed, int *out) {
    int sorted[GEAR_CATALOG_CAPACITY];
    int total = GearInventorySorted(&p->gear, sorted, GEAR_CATALOG_CAPACITY);
    int count = 0;
    for (int i = 0; i < total; i++) {
        int item = sorted[reversed ? total - 1 - i : i];
        if (GearViewMatchesFilter(p, filter, item)) out[count++] = item;
    }
    return count;
}

static const char *GearViewFilterName(GearFilter filter) {
    static const char *const rarity_keys[] = {
        "ROSTER_FILTER_COMMON",
        "ROSTER_FILTER_UNCOMMON",
        "ROSTER_FILTER_RARE",
        "HODA_ROSTER_FILTER_EPIC",
        "HODA_ROSTER_FILTER_LEGENDARY",
        "HODA_ROSTER_FILTER_MYTHIC",
    };
    if (filter == GEAR_FILTER_ALL) return LocGet("HODA_GEAR_FILTER_ALL_TITLE");
    if (filter == GEAR_FILTER_EQUIPPABLE) return LocGet("HODA_GEAR_FILTER_EQUIPPABLE_TITLE");
    return LocGet(rarity_keys[filter - GEAR_FILTER_COMMON]);
}

static void GearViewDrawFilterList(const Player *p, bool input) {
    Font font = GuiFont(FNT_COOL);
    int items[GEAR_CATALOG_CAPACITY];
    for (int i = 0; i < GEAR_VIEW_FILTER_COUNT; i++) {
        Rectangle row = RosterPx(8.0f, 124.0f + i * GEAR_VIEW_FILTER_ROW, 378.0f, 66.0f);
        int count = GearViewFilteredItems(p, (GearFilter)i, false, items);
        bool enabled = count > 0;
        UiDrawTexture(
            GuiTex(gear_view.filter == (GearFilter)i ? GUI_CONSUME_CATEGORY_ON : GUI_CONSUME_CATEGORY),
            row,
            enabled ? WHITE : GRAY
        );
        const char *label = TextFormat("%s(%d)", GearViewFilterName((GearFilter)i), count);
        UiDrawTextCentered(font, label, row, row.height * 0.38f, enabled ? WHITE : GRAY);
        if (!input || !enabled || !GearViewClicked(row)) continue;
        gear_view.filter = (GearFilter)i;
        gear_view.filter_open = false;
        gear_view.page = 0;
        gear_view.scroll = 0.0f;
        GuiSoundPlay(SND_UI_TAB);
    }
}

static void GearViewDrawGrid(const Player *p, const int *items, int count, Rectangle view, bool input) {
    if (count == 0) {
        Rectangle text = UiRect(view, 0.5f, 0.3f, 0.9f, 0.4f);
        HeroesDrawRich(GuiFont(FNT_MENU), LocGet("HODA_GEAR_EMPTY_INVENTORY_MESSAGE"), text, 17.0f, LIGHTGRAY);
        return;
    }
    int first = gear_view.page * GEAR_VIEW_PAGE_SIZE;
    int shown = count - first < GEAR_VIEW_PAGE_SIZE ? count - first : GEAR_VIEW_PAGE_SIZE;
    float cell = view.width / GEAR_VIEW_GRID_COLUMNS;
    int rows = (shown + GEAR_VIEW_GRID_COLUMNS - 1) / GEAR_VIEW_GRID_COLUMNS;
    float max_scroll = fmaxf(0.0f, rows * cell - view.height);
    bool over = input && CheckCollisionPointRec(UiMouse(), view);
    if (over) gear_view.scroll -= GetMouseWheelMove() * GEAR_VIEW_SCROLL_SPEED;
    gear_view.scroll = Clamp(gear_view.scroll, 0.0f, max_scroll);

    float ui = UiScale();
    BeginScissorMode((int)(view.x * ui), (int)(view.y * ui), (int)(view.width * ui), (int)(view.height * ui));
    for (int i = 0; i < shown; i++) {
        int item = items[first + i];
        Rectangle cell_rect = {
            view.x + (i % GEAR_VIEW_GRID_COLUMNS) * cell,
            view.y + (i / GEAR_VIEW_GRID_COLUMNS) * cell - gear_view.scroll,
            cell,
            cell,
        };
        Rectangle tile = UiRect(cell_rect, 0.5f, 0.5f, GEAR_VIEW_TILE_FILL, GEAR_VIEW_TILE_FILL);
        GearDrawTile(GearDefAt(item), tile, item == gear_view.selected);
        if (p->gear.counts[item] > 1)
            UiDrawTextRight(
                GuiFont(FNT_MENU),
                TextFormat("x%d", p->gear.counts[item]),
                (Rectangle){0.0f, tile.y + tile.height - 13.0f, tile.x + tile.width - 6.0f, 0.0f},
                17.0f,
                WHITE
            );
        if (over && GearViewClicked(tile)) {
            GuiClickSoundPlay();
            gear_view.selected = item == gear_view.selected ? -1 : item;
        }
    }
    EndScissorMode();
}

static const char *GearViewRarityText(int rarity) {
    static const char *const keys[GEAR_RARITY_COUNT] = {
        "HODA_GEAR_RARITY_COMMON",
        "HODA_GEAR_RARITY_UNCOMMON",
        "HODA_GEAR_RARITY_RARE",
        "HODA_GEAR_RARITY_EPIC",
        "HODA_GEAR_RARITY_LEGENDARY",
        "HODA_GEAR_RARITY_MYTHIC",
    };
    return LocGet(keys[rarity - 1]);
}

static float GearViewDrawRarityAndLimits(
    const GearInfoText *info, Font rarity_font, float rarity_size, float limits_size, Rectangle area
) {
    Font body = GuiFont(FNT_MENU);
    float y = area.y;
    Rectangle rarity = {area.x, y, area.width, rarity_size * 1.4f};
    y += HeroesDrawRich(rarity_font, GearViewRarityText(info->rarity), rarity, rarity_size, WHITE) + 2.0f;
    if (info->limits[0]) {
        Rectangle limits = {area.x, y, area.width, 36.0f};
        y += HeroesDrawRich(body, info->limits, limits, limits_size, LIGHTGRAY) + 4.0f;
    }
    return y;
}

static void GearViewDrawFittedDescription(const char *text, Rectangle area, float max_size) {
    Font body = GuiFont(FNT_MENU);
    float size = max_size;
    while (size > GEAR_VIEW_DETAILS_FONT_MIN && HeroesRichLayout(body, text, area, size, WHITE, false) > area.height)
        size -= 0.5f;
    HeroesDrawRich(body, text, area, size, WHITE);
}

static GearInfoText GearViewInfoTextFor(const GearDef *def, char *limits, int limits_size, char *text, int text_size) {
    GearFormatLimits(def, limits, limits_size);
    char aura[GEAR_VIEW_AURA_CAPACITY];
    GearFormatAura(def, aura, sizeof(aura));
    snprintf(text, (size_t)text_size, "%s\n%s", GearDescription(def), aura);
    return (GearInfoText){GearTitle(def), def->rarity, limits, text};
}

void GearDrawRevealCard(const GearDef *def, Rectangle panel) {
    UiDrawTexture(GearUiTexture("Gear_Details_Box"), panel, WHITE);
    float side = panel.width;
    Font title_font = GuiFont(FNT_COOL);

    char limits[192] = "";
    char text[GEAR_VIEW_DETAILS_CAPACITY];
    GearInfoText info =
        def ? GearViewInfoTextFor(def, limits, sizeof(limits), text, sizeof(text))
            : (GearInfoText){
                  LocGet("HODA_TOME_POPUP_TITLE"), GEAR_RARITY_COUNT, "", LocGet("HODA_EVO_TOME_EXPLAIN_DESC")
              };

    Rectangle title = {
        panel.x + side * GEAR_CARD_TEXT_LEFT,
        panel.y + side * GEAR_CARD_TITLE_BAND_TOP,
        side * GEAR_CARD_TEXT_WIDTH,
        side * (GEAR_CARD_IMAGE_BAND_TOP - GEAR_CARD_TITLE_BAND_TOP)
    };
    float title_size = side * GEAR_CARD_TITLE_TEXT;
    float title_width = MeasureTextEx(title_font, info.title, title_size, 1.0f).x;
    if (title_width > title.width) title_size *= title.width / title_width;
    UiDrawTextCentered(title_font, info.title, title, title_size, WHITE);

    float icon_size = side * GEAR_CARD_ICON_SIZE;
    float image_center = (GEAR_CARD_IMAGE_BAND_TOP + GEAR_CARD_TEXT_BAND_TOP) * 0.5f;
    Rectangle icon = {
        panel.x + (side - icon_size) * 0.5f, panel.y + side * image_center - icon_size * 0.5f, icon_size, icon_size
    };
    if (def)
        GearDrawTile(def, icon, false);
    else
        UiDrawTexture(GearTomeIconLoad(), icon, WHITE);

    Rectangle band = {
        panel.x + side * GEAR_CARD_TEXT_LEFT,
        panel.y + side * GEAR_CARD_TEXT_BAND_TOP,
        side * GEAR_CARD_TEXT_WIDTH,
        side * (GEAR_CARD_TEXT_BAND_BOTTOM - GEAR_CARD_TEXT_BAND_TOP)
    };
    float y = GearViewDrawRarityAndLimits(
        &info, title_font, side * GEAR_CARD_RARITY_TEXT, side * GEAR_CARD_LIMITS_TEXT, band
    );
    Rectangle description = {band.x, y, band.width, band.y + band.height - y};
    GearViewDrawFittedDescription(info.description, description, side * GEAR_CARD_DESCRIPTION_TEXT);
}

static void GearViewDrawDetails(Player *p, bool input) {
    const GearDef *def = GearDefAt(gear_view.selected);
    Font body = GuiFont(FNT_MENU);
    Rectangle panel = RosterPx(8.0f, 461.0f, 378.0f, 359.0f);
    DrawRectangleRec(panel, BLACK);
    UiDrawTexture(GearUiTexture("Gear_Details_Box"), panel, WHITE);
    Rectangle inner = UiRect(panel, 0.5f, 0.5f, 0.87f, 0.9f);

    float footer = 42.0f;
    char limits[192] = "";
    char text[GEAR_VIEW_DETAILS_CAPACITY];
    GearInfoText info = GearViewInfoTextFor(def, limits, sizeof(limits), text, sizeof(text));
    Rectangle title = {inner.x, inner.y, inner.width - 22.0f, 40.0f};
    float y = title.y + HeroesDrawRich(GuiFont(FNT_COOL), info.title, title, 16.0f, WHITE) + 2.0f;
    y = GearViewDrawRarityAndLimits(&info, body, 13.0f, 13.0f, (Rectangle){inner.x, y, inner.width, 0.0f});
    Rectangle area = {inner.x, y, inner.width, inner.y + inner.height - footer - 4.0f - y};
    GearViewDrawFittedDescription(info.description, area, GEAR_VIEW_DETAILS_FONT_MAX);

    Rectangle row = {inner.x, inner.y + inner.height - footer, inner.width, footer};
    if (gear_view.slot >= 0) {
        if (ButtonDrawEx(&gear_view.equip, UiRect(row, 0.0f, 0.0f, 0.49f, 1.0f), input)) GearViewRequestEquip(p);
    } else {
        HeroesDrawRich(body, LocGet("HODA_GEAR_SELECT_HERO_HELP"), UiRect(row, 0.0f, 0.5f, 0.49f, 1.0f), 11.0f, WHITE);
    }
    if (ButtonDrawEx(&gear_view.sell, UiRect(row, 1.0f, 0.0f, 0.49f, 1.0f), input)) GearViewRequestSell();

    Rectangle close = RosterPx(348.0f, 436.0f, 62.0f, 50.0f);
    if (ButtonDrawEx(&gear_view.close, close, input)) gear_view.selected = -1;
}

static void GearViewDrawFooter(bool input) {
    Rectangle footer = RosterPx(16.0f, 764.0f, 372.0f, 56.0f);
    Rectangle hint = UiRect(footer, 0.0f, 0.5f, 0.62f, 1.0f);
    HeroesDrawRich(GuiFont(FNT_MENU), LocGet("HODA_GEAR_INVENTORY_FOOTER_TEXT"), hint, 12.0f, WHITE);
    if (ButtonDrawEx(&gear_view.store, UiRect(footer, 1.0f, 0.0f, 0.36f, 1.0f), input))
        gear_view.store_requested = true;
}

static void GearViewDrawPageButton(Button *button, char *label, const char *key, Rectangle r, bool input, int delta) {
    LocFormatInt(label, GEAR_VIEW_PAGE_LABEL_CAPACITY, key, GEAR_VIEW_PAGE_SIZE);
    button->label = label;
    if (!ButtonDrawEx(button, r, input)) return;
    gear_view.page += delta;
    gear_view.scroll = 0.0f;
}

void GearViewDrawPanel(Player *p) {
    GearViewLoadButtons();
    bool input = GearViewAcceptsInput();
    UiDrawTexture(GuiTex(GUI_CONSUME_TRAY), RosterPx(-7.0f, 0.0f, 455.0f, 938.0f), WHITE);

    int items[GEAR_CATALOG_CAPACITY];
    int count = GearViewFilteredItems(p, gear_view.filter, gear_view.reversed, items);
    gear_view.category.label = TextFormat("%s(%d)", GearViewFilterName(gear_view.filter), count);
    if (ButtonDrawEx(&gear_view.category, RosterPx(16.0f, 56.0f, 372.0f, 68.0f), input)) {
        gear_view.filter_open = !gear_view.filter_open;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (gear_view.filter_open) {
        GearViewDrawFilterList(p, input);
        if (ButtonDrawEx(
                &gear_view.sort,
                RosterPx(8.0f, 124.0f + GEAR_VIEW_FILTER_COUNT * GEAR_VIEW_FILTER_ROW, 378.0f, 66.0f),
                input
            )) {
            gear_view.reversed = !gear_view.reversed;
            gear_view.scroll = 0.0f;
            gear_view.filter_open = false;
        }
        return;
    }

    int pages = (count + GEAR_VIEW_PAGE_SIZE - 1) / GEAR_VIEW_PAGE_SIZE;
    if (gear_view.page >= pages) gear_view.page = pages > 0 ? pages - 1 : 0;
    if (gear_view.selected >= 0 && p->gear.counts[gear_view.selected] <= 0) gear_view.selected = -1;

    bool details = gear_view.selected >= 0;
    float top = 124.0f, bottom = details ? 456.0f : 756.0f;
    bool has_prev = gear_view.page > 0, has_next = gear_view.page + 1 < pages;
    if (has_prev) {
        GearViewDrawPageButton(
            &gear_view.prev,
            gear_view.prev_label,
            "HODA_GEAR_SHOW_PREVIOUS",
            RosterPx(16.0f, top, 372.0f, 60.0f),
            input,
            -1
        );
        top += 64.0f;
    }
    if (has_next) {
        bottom -= 64.0f;
        GearViewDrawPageButton(
            &gear_view.next,
            gear_view.next_label,
            "HODA_GEAR_SHOW_NEXT",
            RosterPx(16.0f, bottom + 2.0f, 372.0f, 60.0f),
            input,
            1
        );
    }
    GearViewDrawGrid(p, items, count, RosterPx(8.0f, top, 378.0f, bottom - top), input);
    if (details)
        GearViewDrawDetails(p, input);
    else
        GearViewDrawFooter(input);
}

void GearViewDrawDialogs(Player *p) {
    GearViewLoadButtons();
    if (gear_view.dialog != GEAR_VIEW_DIALOG_NONE) GearViewDrawDialog(p);
}
