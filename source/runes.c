bool ProgressionSave(const Player *player);
static void StoreDrawNineSlice(Texture2D texture, Rectangle dst, float border, Color tint);
static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
);

#define RUNES_PANEL_WIDTH 232.0f
#define RUNES_SQUAD_TABS ((Rectangle){8.0f, 612.0f, 216.0f, 64.0f})
#define RUNES_STAT_ROW_STRIDE 1.14f
#define RUNES_CELL 112.0f
#define RUNES_GRID_COLS 2
#define RUNES_GRID_TOP 156.0f
#define RUNES_GRID_HEIGHT 400.0f
#define RUNES_DRAG_THRESHOLD 12.0f
#define RUNES_SCROLL_STEP 50.0f
#define RUNES_ICON_ASPECT 0.781f
#define RUNES_ACTIVATION_TIME 2.0f
#define RUNES_ACTION_Y 654.0f
#define RUNES_BUTTON_W 236.0f
#define RUNES_BUTTON_H 59.0f
#define RUNES_CRAFT_SLOT_COUNT 2
#define RUNES_SLOT_STRIDE 216.0f
#define RUNES_SLOTS_WIDTH (RUNES_SLOT_STRIDE * RUNE_SLOT_COUNT)
#define RUNES_MESSAGE_CAP 192

typedef enum { RUNES_MODE_ASSIGN, RUNES_MODE_CRAFT } RunesMode;

typedef enum {
    RUNES_LEAVE_NONE,
    RUNES_LEAVE_SQUAD,
    RUNES_LEAVE_HOME,
    RUNES_LEAVE_STORE,
    RUNES_LEAVE_RETURN,
    RUNES_LEAVE_SELECT_SQUAD,
    RUNES_LEAVE_GALLERY,
    RUNES_LEAVE_CONSUME
} RunesLeave;

typedef enum {
    RUNES_DIALOG_NONE,
    RUNES_DIALOG_MESSAGE,
    RUNES_DIALOG_APPLY_CHANGES,
    RUNES_DIALOG_REPLACE,
    RUNES_DIALOG_PRESTIGE,
    RUNES_DIALOG_CRAFT_SINGLE,
    RUNES_DIALOG_CRAFT_MENU,
    RUNES_DIALOG_CRAFT_ALL_TYPE,
    RUNES_DIALOG_CRAFT_ALL_RARITY,
    RUNES_DIALOG_CRAFT_DONE
} RunesDialog;

typedef struct {
    RunesMode mode;
    RunesDialog dialog;
    RunesLeave leave_pending;
    RunesLeave leave_done;
    int leave_squad;

    uint64_t pending[RUNE_SLOT_COUNT];
    uint64_t craft[RUNES_CRAFT_SLOT_COUNT];
    uint64_t roster[RUNE_INVENTORY_CAPACITY];
    int roster_count;
    bool sort_descending;
    float scroll;

    uint64_t press_uuid;
    int press_slot;
    Vector2 press_pos;
    bool dragging;

    uint64_t details_uuid;
    bool details_armed;

    int replace_slot;
    uint64_t replace_uuid;
    char message[RUNES_MESSAGE_CAP];
    char craft_done[RUNES_MESSAGE_CAP];
    int craft_done_def;

    int activation_defs[RUNE_SLOT_COUNT];
    int activation_count;
    int activation_fx_requests;
    int activation_index;
    float activation_time;

    Button assign, craft_mode, activate, cancel, sort, store;
    Button craft_one, craft_many;
    Button message_ok, prestige_unlock;
    Button dialog_yes, dialog_no, dialog_ok, dialog_craft, dialog_cancel, dialog_type, dialog_rarity;
} RunesScreen;

static float RunesCenterX(void) {
    return (RUNES_PANEL_WIDTH + UiWidth() - RUNES_CELL) * 0.5f;
}

static Rectangle RunesSlotRect(int slot) {
    return (Rectangle){RunesCenterX() - RUNES_SLOTS_WIDTH * 0.5f + slot * RUNES_SLOT_STRIDE, 172.0f, 200.0f, 124.0f};
}

static Rectangle RunesSlotIconRect(Rectangle slot) {
    float height = slot.height;
    return (Rectangle){slot.x, slot.y, height * RUNES_ICON_ASPECT, height};
}

void RunesInitialize(RunesScreen *r) {
    memset(r, 0, sizeof(*r));
    r->sort_descending = true;
    r->press_slot = -1;
    r->craft_done_def = -1;
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    float scale = 0.33f;
    r->assign = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GOLD, LocGet("RUNE_ASSIGN_MODE_BUTTON"), font, anchor, scale);
    r->craft_mode = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GOLD, LocGet("RUNE_CRAFT_MODE_BUTTON"), font, anchor, scale);
    r->activate = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("RUNES_ACTIVATE"), font, anchor, scale);
    r->cancel = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNES_CANCEL"), font, anchor, scale);
    r->sort = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "Сорт.", font, anchor, 0.4f);
    r->store = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("BUTTON_MAINSCREEN_STORE"), font, anchor, scale);
    r->craft_one = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("RUNE_CRAFT_BUTTON"), font, anchor, scale);
    r->craft_many =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNE_CRAFT_MULTIPLE_BUTTON"), font, anchor, 0.2f);
    r->message_ok = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNE_SINGLE_CONFIRMATION_OK_BUTTON"), font, anchor, scale
    );
    r->prestige_unlock = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "Разблокировать", font, anchor, 0.27f);
    r->dialog_yes = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("CONFIRM_DIALOG_YES"), font, anchor, scale);
    r->dialog_no = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("CONFIRM_DIALOG_NO"), font, anchor, scale);
    r->dialog_ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("RUNE_SINGLE_CONFIRMATION_OK_BUTTON"), font, anchor, scale
    );
    r->dialog_craft = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("RUNE_SINGLE_CONFIRMATION_CRAFT_BUTTON"), font, anchor, scale
    );
    r->dialog_cancel = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNE_SINGLE_CONFIRMATION_CANCEL_BUTTON"), font, anchor, scale
    );
    r->dialog_type =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNE_CRAFT_ALL_TYPE_BUTTON"), font, anchor, 0.27f);
    r->dialog_rarity =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNE_CRAFT_ALL_RARITY_BUTTON"), font, anchor, 0.27f);
}

static void RunesClearPending(RunesScreen *r) {
    memset(r->pending, 0, sizeof(r->pending));
}

static void RunesClearCraft(RunesScreen *r) {
    memset(r->craft, 0, sizeof(r->craft));
}

static void RunesResetPress(RunesScreen *r) {
    r->press_uuid = 0;
    r->press_slot = -1;
    r->dragging = false;
}

void RunesOpen(RunesScreen *r, Player *p) {
    r->mode = RUNES_MODE_ASSIGN;
    r->dialog = RUNES_DIALOG_NONE;
    r->leave_pending = RUNES_LEAVE_NONE;
    r->leave_done = RUNES_LEAVE_NONE;
    r->details_uuid = 0;
    r->activation_count = 0;
    r->activation_fx_requests = 0;
    r->scroll = 0.0f;
    RunesClearPending(r);
    RunesClearCraft(r);
    RunesResetPress(r);
    RuneMarkSeen(&p->runes, RuneNow());
}

static bool RunesHasPending(const RunesScreen *r) {
    for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++)
        if (r->pending[slot]) return true;
    return false;
}

static bool RunesIsHeld(const RunesScreen *r, uint64_t uuid) {
    for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++)
        if (r->pending[slot] == uuid) return true;
    for (int slot = 0; slot < RUNES_CRAFT_SLOT_COUNT; slot++)
        if (r->craft[slot] == uuid) return true;
    return false;
}

static void RunesDropMissing(RunesScreen *r, const PlayerRunes *runes, long long now) {
    for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++) {
        const PlayerRune *rune = RuneByUuidConst(runes, r->pending[slot]);
        if (r->pending[slot] && (!rune || !RuneIsAvailable(rune, now))) r->pending[slot] = 0;
    }
    for (int slot = 0; slot < RUNES_CRAFT_SLOT_COUNT; slot++) {
        const PlayerRune *rune = RuneByUuidConst(runes, r->craft[slot]);
        if (r->craft[slot] && (!rune || !RuneIsAvailable(rune, now))) r->craft[slot] = 0;
    }
}

static void RunesRefreshRoster(RunesScreen *r, const PlayerRunes *runes, long long now) {
    uint64_t sorted[RUNE_INVENTORY_CAPACITY];
    int count = RuneAvailableSorted(runes, now, r->sort_descending, sorted, RUNE_INVENTORY_CAPACITY);
    r->roster_count = 0;
    for (int i = 0; i < count; i++)
        if (!RunesIsHeld(r, sorted[i])) r->roster[r->roster_count++] = sorted[i];
}

static void RunesShowMessage(RunesScreen *r, const char *message) {
    StrCopy(r->message, sizeof(r->message), message);
    r->dialog = RUNES_DIALOG_MESSAGE;
    GuiSoundPlay(SND_STORE_ERROR);
}

static void RunesPlayActivationStep(RunesScreen *r) {
    GuiSoundPlay(SND_RUNE_ACTIVATE);
    r->activation_fx_requests++;
}

bool RunesTakeActivationFx(RunesScreen *r) {
    if (r->activation_fx_requests <= 0) return false;
    r->activation_fx_requests--;
    return true;
}

static void RunesBeginActivation(RunesScreen *r, Player *p) {
    long long now = RuneNow();
    r->activation_count = 0;
    r->activation_index = 0;
    r->activation_time = 0.0f;
    bool expired = false;
    for (int slot = 0; slot < RUNE_SLOT_COUNT; slot++) {
        if (!r->pending[slot]) continue;
        int def = RuneByUuid(&p->runes, r->pending[slot])->def;
        if (RuneActivate(&p->runes, p->current_squad, slot, r->pending[slot], now) == RUNE_ASSIGN_OK)
            r->activation_defs[r->activation_count++] = def;
        else
            expired = true;
    }
    RunesClearPending(r);
    RuneMarkSeen(&p->runes, now);
    ProgressionSave(p);
    if (expired) RunesShowMessage(r, LocGet("EXPIRED_RUNE_MESSAGE"));
    if (r->activation_count > 0) RunesPlayActivationStep(r);
}

static void RunesFinishLeave(RunesScreen *r) {
    r->leave_done = r->leave_pending;
    r->leave_pending = RUNES_LEAVE_NONE;
}

void RunesRequestLeave(RunesScreen *r, RunesLeave leave, int squad) {
    if (r->activation_count > 0 || r->dialog != RUNES_DIALOG_NONE) return;
    r->leave_pending = leave;
    r->leave_squad = squad;
    if (RunesHasPending(r)) {
        r->dialog = RUNES_DIALOG_APPLY_CHANGES;
        GuiSoundPlay(SND_STORE_ERROR);
        return;
    }
    RunesClearCraft(r);
    RunesFinishLeave(r);
}

bool RunesHandleBack(RunesScreen *r) {
    if (r->activation_count > 0) return true;
    if (r->dialog != RUNES_DIALOG_NONE) {
        r->dialog = RUNES_DIALOG_NONE;
        r->leave_pending = RUNES_LEAVE_NONE;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (r->details_uuid) {
        r->details_uuid = 0;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    RunesRequestLeave(r, RUNES_LEAVE_RETURN, 0);
    return true;
}

static void RunesSetMode(RunesScreen *r, RunesMode mode) {
    if (r->mode == mode) return;
    r->mode = mode;
    RunesClearPending(r);
    RunesClearCraft(r);
    GuiSoundPlay(SND_UI_TAB);
}

static void RunesDropAssign(RunesScreen *r, Player *p, int slot, uint64_t uuid, long long now) {
    const PlayerRune *rune = RuneByUuidConst(&p->runes, uuid);
    switch (RuneCheckSlot(&p->runes, p->current_squad, slot, rune, now)) {
        case RUNE_ASSIGN_OK:
            break;
        case RUNE_ASSIGN_NEEDS_PRESTIGE_RUNE:
            RunesShowMessage(r, "В эту ячейку можно вставить только престижную руну.");
            return;
        case RUNE_ASSIGN_NEEDS_NORMAL_SLOT:
            RunesShowMessage(r, "Эту руну можно вставить только в престижную ячейку.");
            return;
        case RUNE_ASSIGN_SLOT_LOCKED:
            RunesShowMessage(r, "Чтобы использовать эту престижную руну, разблокируйте ячейку при помощи самоцветов.");
            return;
        default:
            return;
    }
    if (r->pending[slot] || RuneInSlot(&p->runes, p->current_squad, slot)) {
        r->replace_slot = slot;
        r->replace_uuid = uuid;
        r->dialog = RUNES_DIALOG_REPLACE;
        GuiSoundPlay(SND_STORE_ERROR);
        return;
    }
    r->pending[slot] = uuid;
    GuiSoundPlay(SND_RUNE_PLACE);
}

static void RunesDropCraft(RunesScreen *r, const PlayerRunes *runes, int slot, uint64_t uuid, long long now) {
    const PlayerRune *rune = RuneByUuidConst(runes, uuid);
    if (!rune || !RuneCanCraft(RuneDefFor(rune))) {
        RunesShowMessage(r, LocGet("RUNE_NOT_CRAFTABLE"));
        return;
    }
    RunesClearCraft(r);
    r->craft[slot] = uuid;
    uint64_t sorted[RUNE_INVENTORY_CAPACITY];
    int count = RuneAvailableSorted(runes, now, true, sorted, RUNE_INVENTORY_CAPACITY);
    for (int i = 0; i < count; i++) {
        if (sorted[i] == uuid || RuneByUuidConst(runes, sorted[i])->def != rune->def) continue;
        r->craft[1 - slot] = sorted[i];
        break;
    }
    GuiSoundPlay(SND_RUNE_PLACE);
}

static int RunesSlotCount(const RunesScreen *r) {
    return r->mode == RUNES_MODE_CRAFT ? RUNES_CRAFT_SLOT_COUNT : RUNE_SLOT_COUNT;
}

static int RunesSlotAt(const RunesScreen *r, Vector2 point) {
    for (int slot = 0; slot < RunesSlotCount(r); slot++)
        if (CheckCollisionPointRec(point, RunesSlotRect(slot))) return slot;
    return -1;
}

static uint64_t RunesSlotRune(const RunesScreen *r, const Player *p, int slot) {
    if (r->mode == RUNES_MODE_CRAFT) return r->craft[slot];
    if (r->pending[slot]) return r->pending[slot];
    const PlayerRune *active = RuneInSlot(&p->runes, p->current_squad, slot);
    return active ? active->uuid : 0;
}

static Rectangle RunesTileRect(const RunesScreen *r, int index) {
    return (Rectangle){
        4.0f + (index % RUNES_GRID_COLS) * RUNES_CELL,
        RUNES_GRID_TOP + (index / RUNES_GRID_COLS) * RUNES_CELL - r->scroll,
        RUNES_CELL,
        RUNES_CELL
    };
}

static void RunesDrawIcon(const char *ref, Rectangle rect, Color tint) {
    Texture2D texture = RuneIconLoad(ref);
    if (texture.id != 0) UiDrawTexture(texture, rect, tint);
}

void RunesDrawTile(const RuneDef *def, Rectangle cell, bool selected) {
    Rectangle body = UiRect(cell, 0.5f, 0.5f, 0.9f, 0.9f);
    UiDrawTexture(GuiTex(GUI_RUNE_TILE), body, WHITE);
    if (selected) UiDrawTexture(GuiTex(GUI_CARD_SELECT), cell, WHITE);
    float icon_height = body.height * 0.9f;
    RunesDrawIcon(def->icon, UiRect(body, 0.5f, 0.5f, icon_height * RUNES_ICON_ASPECT, icon_height), WHITE);

    Rectangle strip = UiRect(body, 0.5f, 0.9f, 0.9f, 0.25f);
    DrawRectangleRec(strip, (Color){0, 0, 0, 128});
    float buff_size = strip.height * 1.1f;
    RunesDrawIcon(def->buff_icon, (Rectangle){strip.x + strip.width - buff_size, strip.y, buff_size, buff_size}, WHITE);
    UiDrawTextCentered(
        GuiFont(FNT_MENU),
        def->value_ru,
        (Rectangle){strip.x, strip.y, strip.width - buff_size, strip.height},
        strip.height * 0.72f,
        WHITE
    );
}

void RunesDrawCard(const RuneDef *def, const PlayerRune *rune, Rectangle card, long long now) {
    Font title = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    UiDrawTexture(GuiTex(GUI_RUNE_DETAILS), card, WHITE);

    int rarity = def->rarity < 1 ? 1 : def->rarity > 5 ? 5 : def->rarity;
    Rectangle header = UiRect(card, 0.5f, 0.0f, 1.0f, 0.165f);
    UiDrawTexture(GuiTex(GUI_PLAQUE_COMMON + rarity - 1), header, WHITE);
    UiDrawTextCentered(title, def->name_ru, UiRect(header, 0.5f, 0.15f, 0.88f, 0.5f), header.height * 0.34f, WHITE);
    UiDrawTextCentered(
        body,
        RuneRarityLabel(def->rarity),
        UiRect(header, 0.5f, 0.72f, 0.88f, 0.34f),
        header.height * 0.26f,
        RuneRarityColor(def->rarity)
    );

    Rectangle main = UiRect(card, 0.0f, 1.0f, 1.0f, 0.8f);
    float icon_height = main.height * 0.46f;
    RunesDrawIcon(def->icon, UiRect(main, 0.47f, 0.2f, icon_height * RUNES_ICON_ASPECT, icon_height), WHITE);
    HeroesRichLayout(
        body,
        def->desc_ru,
        UiRect(main, 0.5f, 0.02f, 0.84f, 0.22f),
        card.height * 0.036f,
        (Color){230, 226, 214, 255},
        true
    );

    Rectangle stats = UiRect(main, 0.47f, 0.8f, 0.815f, 0.32f);
    float row = stats.height / 3.0f, font_size = row * 0.5f, stride = row * RUNES_STAT_ROW_STRIDE;
    Color label = {196, 192, 180, 255};
    RunesDrawIcon(def->buff_icon, (Rectangle){stats.x, stats.y, row, row}, WHITE);
    Rectangle boost = {stats.x + row * 1.1f, stats.y, stats.width - row * 1.1f, row};
    UiDrawTextLeftMiddle(body, def->buff_ru, boost, font_size, label);
    UiDrawTextRight(body, def->value_ru, boost, font_size, WHITE);

    char text[32];
    RuneFormatClock(def->duration_minutes * RUNE_SECONDS_PER_MINUTE, text, sizeof(text));
    Rectangle duration = {boost.x, stats.y + stride, boost.width, row};
    UiDrawTextLeftMiddle(body, LocGet("RUNE_DURATION"), duration, font_size, label);
    UiDrawTextRight(body, text, duration, font_size, WHITE);

    if (!rune) return;
    bool active = RuneIsActivated(rune);
    RuneFormatExpiration((active ? rune->expire_at : rune->inventory_expire_at) - now, text, sizeof(text));
    Rectangle expiration = {boost.x, stats.y + stride * 2.0f, boost.width, row};
    UiDrawTextLeftMiddle(body, LocGet("RUNE_EXPIRATION"), expiration, font_size, label);
    UiDrawTextRight(body, text, expiration, font_size, WHITE);
}

static void RunesDrawRoster(RunesScreen *r, const Player *p, bool input) {
    Texture2D panel = GuiTex(GUI_HEROES_PANEL);
    UiDrawTexture(panel, (Rectangle){0, 72, panel.width / 2.0f, panel.height + 128.0f}, (Color){255, 255, 255, 236});

    Font font = GuiFont(FNT_COOL);
    const char *title = LocGet("RUNES_TITLE_TEXT_NO_COUNT");
    Vector2 size = MeasureTextEx(font, title, 20.0f, 1.0f);
    DrawTextEx(font, title, (Vector2){(150.0f - size.x) * 0.5f, 112.0f}, 20.0f, 1.0f, (Color){235, 232, 220, 255});

    if (ButtonDrawEx(&r->sort, (Rectangle){146.0f, 102.0f, 84.0f, 40.0f}, input)) {
        r->sort_descending = !r->sort_descending;
        r->scroll = 0.0f;
    }

    Rectangle view = {0.0f, RUNES_GRID_TOP, RUNES_PANEL_WIDTH, RUNES_GRID_HEIGHT};
    int rows = (r->roster_count + RUNES_GRID_COLS - 1) / RUNES_GRID_COLS;
    float max_scroll = fmaxf(rows * RUNES_CELL - view.height, 0.0f);
    bool over = input && CheckCollisionPointRec(UiMouse(), view);
    if (over) r->scroll -= GetMouseWheelMove() * RUNES_SCROLL_STEP;
    r->scroll = Clamp(r->scroll, 0.0f, max_scroll);

    float ui = UiScale();
    BeginScissorMode((int)(view.x * ui), (int)(view.y * ui), (int)(view.width * ui), (int)(view.height * ui));
    for (int i = 0; i < r->roster_count; i++) {
        Rectangle cell = RunesTileRect(r, i);
        if (cell.y + cell.height < view.y || cell.y > view.y + view.height) continue;
        const RuneDef *def = RuneDefFor(RuneByUuidConst(&p->runes, r->roster[i]));
        bool dragged = r->dragging && r->press_uuid == r->roster[i];
        if (dragged) continue;
        RunesDrawTile(def, cell, r->details_uuid == r->roster[i]);
        if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), cell)) {
            r->press_uuid = r->roster[i];
            r->press_slot = -1;
            r->press_pos = UiMouse();
            r->dragging = false;
        }
    }
    EndScissorMode();
    if (r->roster_count == 0) {
        CampaignDrawWrapped(
            GuiFont(FNT_MENU),
            LocGet("DRAG_RUNE_INTO_SLOT"),
            (Vector2){24.0f, 300.0f},
            18.0f,
            1.0f,
            4.0f,
            190.0f,
            (Color){170, 166, 156, 255}
        );
    }
}

static void RunesDrawSlot(RunesScreen *r, Player *p, int slot, bool input, long long now) {
    Font body = GuiFont(FNT_MENU);
    Rectangle rect = RunesSlotRect(slot);
    Rectangle icon = RunesSlotIconRect(rect);
    Rectangle text = {icon.x + icon.width + 6.0f, rect.y, rect.width - icon.width - 6.0f, rect.height};
    uint64_t uuid = RunesSlotRune(r, p, slot);
    const PlayerRune *rune = RuneByUuidConst(&p->runes, uuid);
    bool prestige = r->mode == RUNES_MODE_ASSIGN && slot == RUNE_PRESTIGE_SLOT;
    bool locked = prestige && !p->runes.prestige_unlocked[p->current_squad];

    if (locked) {
        UiDrawTexture(GuiTex(GUI_RUNE_EMPTY_SLOT), icon, (Color){150, 150, 150, 255});
        char label[64];
        snprintf(label, sizeof(label), "%d", RUNE_PRESTIGE_UNLOCK_GEMS);
        Rectangle row = {text.x, text.y + text.height * 0.28f, text.width, text.height * 0.2f};
        UiDrawTextLeftMiddle(
            body, RuneRarityLabel(RUNE_PRESTIGE_RARITY), row, 17.0f, RuneRarityColor(RUNE_PRESTIGE_RARITY)
        );
        Rectangle cost = {text.x + 28.0f, row.y + row.height, text.width - 28.0f, text.height * 0.24f};
        UiDrawTexture(GuiTex(GUI_MAP_GEMS), (Rectangle){text.x, cost.y + 2.0f, 24.0f, 24.0f}, WHITE);
        UiDrawTextLeftMiddle(body, label, cost, 20.0f, WHITE);
        if (input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), rect))
            r->dialog = RUNES_DIALOG_PRESTIGE;
        return;
    }

    if (!rune) {
        UiDrawTexture(GuiTex(prestige ? GUI_RUNE_EMPTY_SLOT : GUI_MM_RUNE_SLOT), icon, WHITE);
        UiDrawTextLeftMiddle(body, LocGet("RUNES_ADD_A_RUNE"), text, 17.0f, (Color){200, 196, 184, 255});
        return;
    }

    const RuneDef *def = RuneDefFor(rune);
    bool dragged = r->dragging && r->press_uuid == uuid;
    if (!dragged) RunesDrawIcon(def->icon, icon, WHITE);
    UiDrawTextLeftMiddle(
        body, def->value_ru, (Rectangle){text.x, text.y + 18.0f, text.width, 36.0f}, 24.0f, (Color){255, 240, 180, 255}
    );
    char clock[32];
    long long seconds = RuneIsActivated(rune) ? rune->expire_at - now : def->duration_minutes * RUNE_SECONDS_PER_MINUTE;
    RuneFormatClock(seconds, clock, sizeof(clock));
    float buff = 26.0f;
    RunesDrawIcon(def->buff_icon, (Rectangle){text.x, text.y + 60.0f, buff, buff}, WHITE);
    UiDrawTextLeftMiddle(
        body, clock, (Rectangle){text.x + buff + 4.0f, text.y + 60.0f, text.width - buff - 4.0f, buff}, 17.0f, WHITE
    );

    if (input && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), rect)) {
        r->press_uuid = uuid;
        r->press_slot = slot;
        r->press_pos = UiMouse();
        r->dragging = false;
    }
}

static void RunesReturnToTray(RunesScreen *r, int slot) {
    if (r->mode == RUNES_MODE_CRAFT)
        RunesClearCraft(r);
    else
        r->pending[slot] = 0;
    GuiSoundPlay(SND_RUNE_PICKUP);
}

static void RunesUpdateDrag(RunesScreen *r, Player *p, bool input, long long now) {
    if (!r->press_uuid) return;
    Vector2 mouse = UiMouse();
    bool movable = r->press_slot < 0 || r->mode == RUNES_MODE_CRAFT || r->pending[r->press_slot] == r->press_uuid;
    if (!r->dragging && movable && Vector2Distance(mouse, r->press_pos) >= RUNES_DRAG_THRESHOLD) {
        r->dragging = true;
        r->details_uuid = 0;
        GuiSoundPlay(SND_RUNE_PICKUP);
    }
    if (!input || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;

    uint64_t uuid = r->press_uuid;
    int from = r->press_slot;
    bool dragged = r->dragging;
    RunesResetPress(r);
    if (!dragged) {
        r->details_uuid = uuid;
        r->details_armed = false;
        GuiSoundPlay(SND_STORE_INFO);
        return;
    }
    int slot = RunesSlotAt(r, mouse);
    if (from >= 0) {
        if (mouse.x < RUNES_PANEL_WIDTH) RunesReturnToTray(r, from);
        return;
    }
    if (slot < 0) return;
    if (r->mode == RUNES_MODE_CRAFT)
        RunesDropCraft(r, &p->runes, slot, uuid, now);
    else
        RunesDropAssign(r, p, slot, uuid, now);
}

static bool RunesCraftReady(const RunesScreen *r) {
    return r->craft[0] && r->craft[1];
}

static const RuneDef *RunesCraftTarget(const RunesScreen *r, const PlayerRunes *runes) {
    const PlayerRune *rune = RuneByUuidConst(runes, r->craft[0]);
    return rune ? RuneDefAt(RuneDefFor(rune)->craft_to) : NULL;
}

static void RunesCompleteCraft(RunesScreen *r, Player *p, RuneCraftResult result, int crafted, int def) {
    switch (result) {
        case RUNE_CRAFT_SUCCESS:
            RunesClearCraft(r);
            r->craft_done_def = def;
            if (crafted > 0) {
                const char *format = LocGet("RUNE_CRAFT_MULTIPLE_CRAFTED_COUNT");
                char count[16];
                snprintf(count, sizeof(count), "%d", crafted);
                HeroesFormat2(r->craft_done, sizeof(r->craft_done), format, count, "");
            } else {
                StrCopy(r->craft_done, sizeof(r->craft_done), LocGet("RUNE_SINGLE_CRAFTING_COMPLETE_LABEL"));
            }
            r->dialog = RUNES_DIALOG_CRAFT_DONE;
            GuiSoundPlay(SND_RUNE_ACTIVATE);
            ProgressionSave(p);
            break;
        case RUNE_CRAFT_EXPIRED:
            RunesShowMessage(r, LocGet("EXPIRED_RUNE_MESSAGE"));
            break;
        default:
            RunesShowMessage(r, LocGet("RUNE_NOT_CRAFTABLE"));
            break;
    }
}

static void RunesCraftSingle(RunesScreen *r, Player *p, long long now) {
    int def = -1;
    RuneCraftResult result = RuneCraft(&p->runes, r->craft[0], r->craft[1], now, &def);
    RunesCompleteCraft(r, p, result, 0, def);
}

static void RunesCraftMultiple(RunesScreen *r, Player *p, RuneCraftMode mode, long long now) {
    const PlayerRune *rune = RuneByUuidConst(&p->runes, r->craft[0]);
    if (!rune) return;
    int crafted = RuneCraftAll(&p->runes, mode, rune->def, now);
    RunesCompleteCraft(r, p, RUNE_CRAFT_SUCCESS, crafted, -1);
}

static void RunesDrawDragged(const RunesScreen *r, const Player *p) {
    if (!r->dragging) return;
    const PlayerRune *rune = RuneByUuidConst(&p->runes, r->press_uuid);
    if (!rune) return;
    float height = 120.0f;
    Vector2 m = UiMouse();
    RunesDrawIcon(
        RuneDefFor(rune)->icon,
        (Rectangle){m.x - height * RUNES_ICON_ASPECT * 0.5f, m.y - height * 0.5f, height * RUNES_ICON_ASPECT, height},
        WHITE
    );
}

static void RunesDrawActivation(RunesScreen *r, float dt) {
    if (r->activation_count <= 0) return;
    const RuneDef *def = RuneDefAt(r->activation_defs[r->activation_index]);
    float cx = RunesCenterX();
    float pulse = 1.0f + 0.1f * sinf(fminf(r->activation_time / 0.75f, 1.0f) * PI);
    Rectangle bar = {cx - 237.5f * pulse, 140.0f, 475.0f * pulse, 65.0f * pulse};
    UiDrawTexture(GuiTex(GUI_COMBAT_PREBATTLE_BG), bar, WHITE);
    char text[128];
    snprintf(text, sizeof(text), "%s %s", def->name_ru, def->value_ru);
    UiDrawTextCentered(GuiFont(FNT_COOL), text, bar, 24.0f * pulse, WHITE);
    float icon = 72.0f * pulse;
    RunesDrawIcon(def->buff_icon, (Rectangle){cx - icon * 0.5f, bar.y + bar.height - 8.0f, icon, icon}, WHITE);

    r->activation_time += dt;
    if (r->activation_time < RUNES_ACTIVATION_TIME) return;
    r->activation_time = 0.0f;
    r->activation_index++;
    if (r->activation_index >= r->activation_count)
        r->activation_count = 0;
    else
        RunesPlayActivationStep(r);
}

static Rectangle RunesDialogPlate(float height) {
    float width = 520.0f;
    return (Rectangle){RunesCenterX() - width * 0.5f, (UiHeight() - height) * 0.5f + 20.0f, width, height};
}

static Rectangle RunesDialogButton(Rectangle plate, int index, int count) {
    float gap = 12.0f;
    float total = count * RUNES_BUTTON_W + (count - 1) * gap;
    return (Rectangle){
        plate.x + (plate.width - total) * 0.5f + index * (RUNES_BUTTON_W + gap),
        plate.y + plate.height - RUNES_BUTTON_H - 24.0f,
        RUNES_BUTTON_W,
        RUNES_BUTTON_H
    };
}

static void RunesDrawDialogFrame(Rectangle plate, const char *title) {
    MessagePopupDrawScrim();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), plate, WHITE);
    if (title && title[0])
        UiDrawTextCentered(
            GuiFont(FNT_COOL),
            title,
            (Rectangle){plate.x + 24.0f, plate.y + 20.0f, plate.width - 48.0f, 44.0f},
            26.0f,
            (Color){242, 236, 218, 255}
        );
}

static void RunesDrawDialogBody(Rectangle plate, float y, const char *text) {
    HeroesRichLayout(
        GuiFont(FNT_MENU),
        text,
        (Rectangle){plate.x + 40.0f, plate.y + y, plate.width - 80.0f, plate.height - y - 100.0f},
        20.0f,
        (Color){228, 224, 212, 255},
        true
    );
}

static void RunesCloseDialog(RunesScreen *r) {
    r->dialog = RUNES_DIALOG_NONE;
}

static void RunesApplyConfirm(RunesScreen *r, Player *p, RunesDialog dialog, bool yes) {
    RunesCloseDialog(r);
    if (dialog == RUNES_DIALOG_APPLY_CHANGES) {
        if (yes) RunesBeginActivation(r, p);
        RunesClearPending(r);
        r->activation_count = 0;
        RunesFinishLeave(r);
    } else if (dialog == RUNES_DIALOG_REPLACE && yes) {
        r->pending[r->replace_slot] = r->replace_uuid;
        GuiSoundPlay(SND_RUNE_PLACE);
    }
}

static void RunesDrawConfirm(RunesScreen *r, Player *p) {
    bool applying = r->dialog == RUNES_DIALOG_APPLY_CHANGES;
    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    MessagePopupDrawText(
        GuiFont(FNT_COOL),
        LocGet(applying ? "RUNE_APPLY_CHANGES_CONFIRMATION_TITLE" : "RUNE_REPLACEMENT_CONFIRMATION_TITLE"),
        UiRect(content, 0.0f, 0.0f, 1.0f, 0.2f),
        30.0f,
        WHITE
    );
    MessagePopupDrawText(
        GuiFont(FNT_MENU),
        LocGet(applying ? "RUNE_APPLY_CHANGES_CONFIRMATION_MESSAGE" : "RUNE_REPLACEMENT_CONFIRMATION_MESSAGE"),
        UiRect(content, 0.0f, 0.3f, 1.0f, 0.5f),
        26.0f,
        WHITE
    );
    Rectangle row = MessagePopupChoiceRow(popup);
    bool yes = ButtonDraw(&r->dialog_yes, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f));
    bool no = ButtonDraw(&r->dialog_no, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f));
    if (yes || no) RunesApplyConfirm(r, p, r->dialog, yes);
}

static void RunesDrawPrestigeUnlock(RunesScreen *r, Player *p) {
    MessagePopupDrawScrim();
    Rectangle popup = UiRect(UiRoot(), 0.5f, 0.5f, 0.75f, 0.75f);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), popup, WHITE);
    Rectangle content = UiRect(popup, 0.5f, 0.15f, 0.86f, 0.5f);
    MessagePopupDrawText(
        GuiFont(FNT_COOL),
        RuneRarityLabel(RUNE_PRESTIGE_RARITY),
        UiRect(content, 0.5f, 0.05f, 0.85f, 0.39f),
        30.0f,
        WHITE
    );
    MessagePopupDrawText(
        GuiFont(FNT_MENU),
        "Разблокировать престижную ячейку этого отряда?",
        UiRect(content, 0.5f, 1.0f, 0.95f, 0.5f),
        24.0f,
        WHITE
    );
    Rectangle currency = UiRect(popup, 0.65f, 0.625f, 0.2f, 0.14f);
    UiDrawTextRight(
        GuiFont(FNT_MENU),
        "Цена:",
        (Rectangle){0.0f, currency.y + currency.height * 0.5f, currency.x - 12.0f, 0.0f},
        22.0f,
        WHITE
    );
    UiDrawTexture(GuiTex(GUI_REWARD_BRUSH), currency, WHITE);
    UiDrawTexture(
        GuiTex(GUI_MAP_GEMS), UiRect(currency, 0.9f, 0.4f, currency.height * 0.9f, currency.height * 0.9f), WHITE
    );
    char cost[16];
    snprintf(cost, sizeof(cost), "%d", RUNE_PRESTIGE_UNLOCK_GEMS);
    UiDrawTextLeftMiddle(GuiFont(FNT_COOL), cost, UiRect(currency, 0.1f, 0.3f, 0.4f, 0.6f), 26.0f, WHITE);

    Rectangle choice = UiRect(popup, 0.5f, 0.85f, 0.8f, 0.2f);
    bool cancel = ButtonDraw(&r->cancel, UiRect(choice, 0.1f, 0.5f, 0.4f, 0.9f));
    bool unlock = ButtonDraw(&r->prestige_unlock, UiRect(choice, 0.95f, 0.5f, 0.4f, 0.9f));
    if (!cancel && !unlock) return;
    RunesCloseDialog(r);
    if (!unlock) return;
    if (PlayerUnlockPrestigeRuneSlot(p, p->current_squad))
        ProgressionSave(p);
    else
        RunesShowMessage(r, LocGet("MESSAGE_CANT_AFFORD_BODY"));
}

static void RunesDrawCraftCardDialog(RunesScreen *r, Player *p, long long now) {
    const RuneDef *target = RunesCraftTarget(r, &p->runes);
    Rectangle plate = RunesDialogPlate(560.0f);
    bool single = r->dialog == RUNES_DIALOG_CRAFT_SINGLE;
    RunesDrawDialogFrame(plate, LocGet(single ? "RUNE_SINGLE_CONFIRMATION_LABEL" : "RUNE_CRAFT_MULTIPLE_CONFIRM_TYPE"));
    if (target) {
        float height = 390.0f;
        RunesDrawCard(
            target,
            NULL,
            (Rectangle){plate.x + (plate.width - height * 0.78f) * 0.5f, plate.y + 68.0f, height * 0.78f, height},
            now
        );
    }
    bool craft = ButtonDraw(&r->dialog_craft, RunesDialogButton(plate, 0, 2));
    bool cancel = ButtonDraw(&r->dialog_cancel, RunesDialogButton(plate, 1, 2));
    if (cancel) {
        RunesCloseDialog(r);
    } else if (craft) {
        RunesCloseDialog(r);
        if (single)
            RunesCraftSingle(r, p, now);
        else
            RunesCraftMultiple(r, p, RUNE_CRAFT_MODE_TYPE, now);
    }
}

static void RunesDrawCraftMenu(RunesScreen *r) {
    Rectangle plate = RunesDialogPlate(360.0f);
    RunesDrawDialogFrame(plate, LocGet("RUNE_CRAFT_MULTIPLE_TITLE"));
    Font body = GuiFont(FNT_MENU);
    Color tone = {228, 224, 212, 255};
    Rectangle type_text = {plate.x + 36.0f, plate.y + 76.0f, plate.width - 300.0f, 100.0f};
    Rectangle rarity_text = {plate.x + 36.0f, plate.y + 186.0f, plate.width - 300.0f, 100.0f};
    HeroesRichLayout(body, LocGet("RUNE_CRAFT_ALL_TYPE_DESCRIPTION"), type_text, 17.0f, tone, true);
    HeroesRichLayout(body, LocGet("RUNE_CRAFT_ALL_RARITY_DESCRIPTION"), rarity_text, 17.0f, tone, true);
    float x = plate.x + plate.width - RUNES_BUTTON_W - 20.0f;
    if (ButtonDraw(&r->dialog_type, (Rectangle){x, plate.y + 86.0f, RUNES_BUTTON_W, RUNES_BUTTON_H}))
        r->dialog = RUNES_DIALOG_CRAFT_ALL_TYPE;
    if (ButtonDraw(&r->dialog_rarity, (Rectangle){x, plate.y + 196.0f, RUNES_BUTTON_W, RUNES_BUTTON_H}))
        r->dialog = RUNES_DIALOG_CRAFT_ALL_RARITY;
    if (ButtonDraw(&r->dialog_cancel, RunesDialogButton(plate, 0, 1))) RunesCloseDialog(r);
}

static void RunesDrawCraftRarityConfirm(RunesScreen *r, Player *p, long long now) {
    Rectangle plate = RunesDialogPlate(280.0f);
    RunesDrawDialogFrame(plate, LocGet("RUNE_CRAFT_MULTIPLE_CONFIRM_TITLE"));
    RunesDrawDialogBody(plate, 80.0f, LocGet("RUNE_CRAFT_MULTIPLE_CONFIRM_RARITY"));
    bool craft = ButtonDraw(&r->dialog_craft, RunesDialogButton(plate, 0, 2));
    bool cancel = ButtonDraw(&r->dialog_cancel, RunesDialogButton(plate, 1, 2));
    if (cancel) {
        RunesCloseDialog(r);
    } else if (craft) {
        RunesCloseDialog(r);
        RunesCraftMultiple(r, p, RUNE_CRAFT_MODE_RARITY, now);
    }
}

static void RunesDrawCraftDone(RunesScreen *r, long long now) {
    const RuneDef *def = RuneDefAt(r->craft_done_def);
    Rectangle plate = RunesDialogPlate(def ? 560.0f : 240.0f);
    RunesDrawDialogFrame(plate, r->craft_done);
    if (def) {
        float height = 390.0f;
        RunesDrawCard(
            def,
            NULL,
            (Rectangle){plate.x + (plate.width - height * 0.78f) * 0.5f, plate.y + 68.0f, height * 0.78f, height},
            now
        );
    }
    if (ButtonDraw(&r->dialog_ok, RunesDialogButton(plate, 0, 1))) RunesCloseDialog(r);
}

static void RunesDrawMessage(RunesScreen *r) {
    Rectangle popup = MessagePopupDrawFrame();
    MessagePopupDrawText(
        GuiFont(FNT_MENU), r->message, UiRect(MessagePopupContent(popup), 0.0f, 0.0f, 1.0f, 0.5f), 26.0f, WHITE
    );
    if (ButtonDraw(&r->message_ok, MessagePopupOkButton(popup))) {
        RunesCloseDialog(r);
        r->leave_pending = RUNES_LEAVE_NONE;
    }
}

static void RunesDrawDialog(RunesScreen *r, Player *p, long long now) {
    switch (r->dialog) {
        case RUNES_DIALOG_MESSAGE:
            RunesDrawMessage(r);
            break;
        case RUNES_DIALOG_APPLY_CHANGES:
        case RUNES_DIALOG_REPLACE:
            RunesDrawConfirm(r, p);
            break;
        case RUNES_DIALOG_PRESTIGE:
            RunesDrawPrestigeUnlock(r, p);
            break;
        case RUNES_DIALOG_CRAFT_SINGLE:
        case RUNES_DIALOG_CRAFT_ALL_TYPE:
            RunesDrawCraftCardDialog(r, p, now);
            break;
        case RUNES_DIALOG_CRAFT_MENU:
            RunesDrawCraftMenu(r);
            break;
        case RUNES_DIALOG_CRAFT_ALL_RARITY:
            RunesDrawCraftRarityConfirm(r, p, now);
            break;
        case RUNES_DIALOG_CRAFT_DONE:
            RunesDrawCraftDone(r, now);
            break;
        default:
            break;
    }
}

static void RunesDrawDetails(RunesScreen *r, const Player *p, bool input, long long now) {
    const PlayerRune *rune = RuneByUuidConst(&p->runes, r->details_uuid);
    if (!rune) {
        r->details_uuid = 0;
        return;
    }
    float height = 500.0f;
    Rectangle card = {RunesCenterX() - height * 0.39f, 120.0f, height * 0.78f, height};
    RunesDrawCard(RuneDefFor(rune), rune, card, now);
    if (!input) return;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) r->details_armed = true;
    if (r->details_armed && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        r->details_uuid = 0;
        r->details_armed = false;
    }
}

static void RunesDrawInstructions(const RunesScreen *r) {
    float cx = RunesCenterX();
    Rectangle bar = {cx - 280.0f, 84.0f, 560.0f, 62.0f};
    UiDrawTexture(GuiTex(GUI_COMBAT_PREBATTLE_BG), bar, WHITE);
    const char *text = LocGet(r->mode == RUNES_MODE_CRAFT ? "RUNES_CRAFT_CTA_TEXT" : "RUNES_CTA_TEXT");
    Font font = GuiFont(FNT_MENU);
    float size = 19.0f, width = MeasureTextEx(font, text, size, 1.0f).x, limit = bar.width * 0.8f;
    if (width > limit) size *= limit / width;
    UiDrawTextCentered(font, text, bar, size, WHITE);
}

static void RunesDrawActions(RunesScreen *r, Player *p, bool input) {
    float cx = RunesCenterX();
    Rectangle left = {cx - RUNES_BUTTON_W - 6.0f, RUNES_ACTION_Y, RUNES_BUTTON_W, RUNES_BUTTON_H};
    Rectangle right = {cx + 6.0f, RUNES_ACTION_Y, RUNES_BUTTON_W, RUNES_BUTTON_H};

    if (r->mode == RUNES_MODE_ASSIGN && RunesHasPending(r)) {
        if (ButtonDrawEx(&r->cancel, left, input)) RunesClearPending(r);
        if (ButtonDrawEx(&r->activate, right, input)) RunesBeginActivation(r, p);
        return;
    }

    if (r->mode == RUNES_MODE_ASSIGN) {
        ButtonDrawSelected(&r->assign, left);
        if (ButtonDrawEx(&r->craft_mode, right, input)) RunesSetMode(r, RUNES_MODE_CRAFT);
    } else {
        if (ButtonDrawEx(&r->assign, left, input)) RunesSetMode(r, RUNES_MODE_ASSIGN);
        ButtonDrawSelected(&r->craft_mode, right);
        bool ready = input && RunesCraftReady(r);
        Color dim = ready ? WHITE : (Color){120, 120, 120, 255};
        ButtonSetLabelColor(&r->craft_one, dim);
        ButtonSetLabelColor(&r->craft_many, dim);
        Rectangle one = {left.x, RUNES_ACTION_Y - RUNES_BUTTON_H - 6.0f, RUNES_BUTTON_W, RUNES_BUTTON_H};
        Rectangle many = {right.x, one.y, RUNES_BUTTON_W, RUNES_BUTTON_H};
        if (ButtonDrawEx(&r->craft_one, one, ready)) r->dialog = RUNES_DIALOG_CRAFT_SINGLE;
        if (ButtonDrawEx(&r->craft_many, many, ready)) r->dialog = RUNES_DIALOG_CRAFT_MENU;
    }
}

RunesLeave RunesDraw(RunesScreen *r, Heroes *h, Player *p, float dt) {
    long long now = RuneNow();
    RunesDropMissing(r, &p->runes, now);
    RunesRefreshRoster(r, &p->runes, now);
    bool blocked = UiInputBlocked() || r->dialog != RUNES_DIALOG_NONE || r->activation_count > 0;
    bool input = !blocked && !r->details_uuid;

    if (r->activation_count <= 0) {
        RunesDrawRoster(r, p, input);
        if (ButtonDrawEx(&r->store, (Rectangle){20.0f, 560.0f, 192.0f, 48.0f}, input))
            RunesRequestLeave(r, RUNES_LEAVE_STORE, 0);
        int squad = HeroesDrawSquadTabs(h, p, RUNES_SQUAD_TABS);
        if (squad >= 0 && !blocked) RunesRequestLeave(r, RUNES_LEAVE_SELECT_SQUAD, squad);

        RunesDrawInstructions(r);
        Rectangle panel = {
            RunesCenterX() - RUNES_SLOTS_WIDTH * 0.5f - 24.0f, 156.0f, RUNES_SLOTS_WIDTH + 48.0f, 156.0f
        };
        UiDrawTexture(GuiTex(GUI_NEXUS_TIMER_PLATE), panel, (Color){255, 255, 255, 230});
        for (int slot = 0; slot < RunesSlotCount(r); slot++) RunesDrawSlot(r, p, slot, input, now);
        RunesDrawActions(r, p, input);
    }

    HeroesButton nav = HeroesDrawNav(h, HEROES_RUNES, !blocked, 0, p->gallery.badge);
    if (nav == HEROES_SQUAD) RunesRequestLeave(r, RUNES_LEAVE_SQUAD, 0);
    if (nav == HEROES_GALLERY) RunesRequestLeave(r, RUNES_LEAVE_GALLERY, 0);
    if (nav == HEROES_CONSUME) RunesRequestLeave(r, RUNES_LEAVE_CONSUME, 0);

    if (input || r->press_uuid) RunesUpdateDrag(r, p, !blocked, now);
    RunesDrawDragged(r, p);
    if (r->details_uuid) RunesDrawDetails(r, p, !blocked, now);
    RunesDrawActivation(r, dt);
    RunesDrawDialog(r, p, now);

    RunesLeave done = r->leave_done;
    r->leave_done = RUNES_LEAVE_NONE;
    return done;
}
