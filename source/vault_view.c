#define VAULT_VIEW_TEXT ((Color){236, 232, 220, 255})
#define VAULT_VIEW_DIM ((Color){176, 168, 150, 255})
#define VAULT_VIEW_GOLD ((Color){240, 200, 110, 255})
#define VAULT_VIEW_WARN ((Color){232, 128, 96, 255})
#define VAULT_VIEW_LOCKED_BADGE ((Color){90, 90, 90, 200})
#define VAULT_VIEW_DISABLED_TINT ((Color){105, 105, 105, 255})
#define VAULT_VIEW_DISABLED_LABEL ((Color){150, 146, 138, 255})
#define VAULT_VIEW_FIELD_FILL ((Color){0, 0, 0, 150})
#define VAULT_VIEW_FIELD_EDGE ((Color){160, 140, 100, 200})
#define VAULT_VIEW_SCRIM ((Color){0, 0, 0, 150})
#define VAULT_VIEW_TEXT_CAP 192
#define VAULT_VIEW_LABEL_CAP 48
#define VAULT_VIEW_BADGE_SIDE 150.0f
#define VAULT_VIEW_TIER_SIZE 32.0f
#define VAULT_VIEW_AMOUNT_SIZE 40.0f
#define VAULT_VIEW_STATUS_SIZE 19.0f
#define VAULT_VIEW_NOTE_SIZE 20.0f
#define VAULT_VIEW_DIALOG_TITLE_SIZE 30.0f
#define VAULT_VIEW_BUTTON_WIDTH 230.0f
#define VAULT_VIEW_BUTTON_HEIGHT 58.0f
#define VAULT_VIEW_BUTTON_GAP 20.0f
#define VAULT_VIEW_STEP_SIDE 50.0f
#define VAULT_VIEW_FIELD_WIDTH 230.0f
#define VAULT_VIEW_MAX_WIDTH 100.0f
#define VAULT_VIEW_STEPPER_GAP 12.0f
#define VAULT_VIEW_LABEL_SCALE 0.4f
#define VAULT_VIEW_SMALL_STEP 100LL
#define VAULT_VIEW_MEDIUM_STEP 500LL
#define VAULT_VIEW_LARGE_STEP 1000LL
#define VAULT_VIEW_MEDIUM_FROM 1000LL
#define VAULT_VIEW_LARGE_FROM 5000LL
#define VAULT_VIEW_FIRST_DEPOSIT 1000LL
#define VAULT_VIEW_TENTHS 10LL

typedef struct {
    bool ready;
    long long amount;
    char collect_label[VAULT_VIEW_LABEL_CAP];
    Button collect;
    Button deposit;
    Button withdraw;
    Button minus;
    Button plus;
    Button max;
} VaultView;

static VaultView vault_view;

static Button VaultViewButton(GuiId idle, GuiId active, const char *label) {
    return ButtonLoadText(idle, active, label, GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, VAULT_VIEW_LABEL_SCALE);
}

static void VaultViewEnsure(void) {
    VaultView *v = &vault_view;
    if (v->ready) return;
    v->ready = true;
    StrCopy(v->collect_label, sizeof(v->collect_label), LocGet("HODA_VAULT_COLLECT"));
    v->collect = VaultViewButton(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, v->collect_label);
    v->deposit = VaultViewButton(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_VAULT_DEPOSIT"));
    v->withdraw = VaultViewButton(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_VAULT_WITHDRAW"));
    v->minus = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "-", GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.6f);
    v->plus = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "+", GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.6f);
    v->max = VaultViewButton(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_VAULT_MAX"));
}

static void VaultViewDrawDisabled(const Button *button, Rectangle rect) {
    UiDrawTexture(GuiTex(GUI_BTN_GREY), rect, VAULT_VIEW_DISABLED_TINT);
    float size = rect.height * VAULT_VIEW_LABEL_SCALE;
    StoreDrawCenteredFit(button->font, button->label, rect, size, VAULT_VIEW_DISABLED_LABEL);
}

static bool VaultViewDrawButton(Button *button, Rectangle rect, bool enabled, bool interactive) {
    if (enabled) return ButtonDrawEx(button, rect, interactive);
    VaultViewDrawDisabled(button, rect);
    return false;
}

static long long VaultViewStepFrom(long long amount) {
    return amount >= VAULT_VIEW_LARGE_FROM    ? VAULT_VIEW_LARGE_STEP
           : amount >= VAULT_VIEW_MEDIUM_FROM ? VAULT_VIEW_MEDIUM_STEP
                                              : VAULT_VIEW_SMALL_STEP;
}

long long VaultViewStepUp(long long amount) {
    long long step = VaultViewStepFrom(amount);
    long long next = (amount / step + 1) * step;
    return next < VaultCapacity() ? next : VaultCapacity();
}

long long VaultViewStepDown(long long amount) {
    long long step = VaultViewStepFrom(amount - 1);
    long long next = ((amount - 1) / step) * step;
    return next > VAULT_VIEW_SMALL_STEP ? next : VAULT_VIEW_SMALL_STEP;
}

long long VaultViewMaxDeposit(const Player *player) {
    long long room = VaultCapacity() - player->vault.locked;
    long long depositable = player->gems < room ? player->gems : room;
    return VaultDepositFits(&player->vault, depositable) ? depositable : 0;
}

static long long VaultViewLimit(const Player *player, StoreDialog dialog) {
    return dialog == STORE_DIALOG_VAULT_DEPOSIT ? VaultViewMaxDeposit(player) : player->vault.locked;
}

static long long VaultViewFloor(const Player *player, StoreDialog dialog) {
    long long limit = VaultViewLimit(player, dialog);
    long long smallest = dialog == STORE_DIALOG_VAULT_DEPOSIT ? VaultMinimumDeposit() : VAULT_VIEW_SMALL_STEP;
    return smallest < limit ? smallest : limit;
}

static long long VaultViewClamp(const Player *player, StoreDialog dialog, long long amount) {
    long long floor = VaultViewFloor(player, dialog), limit = VaultViewLimit(player, dialog);
    return amount < floor ? floor : amount > limit ? limit : amount;
}

static bool VaultViewAmountValid(const Player *player, StoreDialog dialog, long long amount) {
    if (dialog == STORE_DIALOG_VAULT_DEPOSIT) return VaultDepositFits(&player->vault, amount) && amount <= player->gems;
    return VaultAvailable() && amount > 0 && amount <= player->vault.locked;
}

void VaultViewOpen(Store *store, const Player *player, StoreDialog dialog) {
    long long limit = VaultViewLimit(player, dialog);
    if (limit <= 0) return;
    bool deposit = dialog == STORE_DIALOG_VAULT_DEPOSIT;
    vault_view.amount = deposit && limit > VAULT_VIEW_FIRST_DEPOSIT ? VAULT_VIEW_FIRST_DEPOSIT : limit;
    store->dialog = dialog;
}

long long VaultViewAmount(void) {
    return vault_view.amount;
}

static void VaultViewFormatGems(char *out, size_t size, long long gems) {
    snprintf(out, size, "%lld", gems);
}

static void VaultViewFormatDaily(char *out, size_t size, long long milligems) {
    long long tenths = milligems * VAULT_VIEW_TENTHS / VAULT_MILLIGEMS_PER_GEM;
    char number[32];
    snprintf(number, sizeof(number), "%lld.%lld", tenths / VAULT_VIEW_TENTHS, tenths % VAULT_VIEW_TENTHS);
    LocFormat1(out, size, "HODA_VAULT_INTEREST_VALUE", number);
}

static const char *VaultViewTierName(VaultTier tier) {
    static const char *const keys[VAULT_TIER_COUNT] = {
        [VAULT_TIER_NONE] = "HODA_VAULT_TIER_NONE",
        [VAULT_TIER_BRONZE] = "HODA_VAULT_TIER_BRONZE",
        [VAULT_TIER_SILVER] = "HODA_VAULT_TIER_SILVER",
        [VAULT_TIER_GOLD] = "HODA_VAULT_TIER_GOLD",
    };
    return LocGet(keys[tier >= VAULT_TIER_NONE && tier < VAULT_TIER_COUNT ? tier : VAULT_TIER_NONE]);
}

static void VaultViewDrawFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;
    float scale = fminf(dst.width / texture.width, dst.height / texture.height);
    float width = texture.width * scale, height = texture.height * scale;
    UiDrawTexture(
        texture,
        (Rectangle){dst.x + (dst.width - width) * 0.5f, dst.y + (dst.height - height) * 0.5f, width, height},
        tint
    );
}

static void VaultViewDrawGemsCentred(Font font, const char *text, Rectangle row, float size, Color color) {
    float icon = size * 0.9f, gap = size * 0.2f;
    float width = MeasureTextEx(font, text, size, 1.0f).x + gap + icon;
    float x = row.x + (row.width - width) * 0.5f;
    UiDrawTextLeftMiddle(font, text, (Rectangle){x, row.y, width - gap - icon, row.height}, size, color);
    Rectangle gem = {x + width - icon, row.y + (row.height - icon) * 0.5f, icon, icon};
    VaultViewDrawFit(GuiTex(GUI_STORE_CURRENCY_GEMS), gem, WHITE);
}

static void VaultViewStatusLine(const Player *player, long long now, char *out, size_t size, Color *color) {
    *color = VAULT_VIEW_DIM;
    if (player->vault.locked <= 0) {
        StrCopy(out, size, LocGet("HODA_VAULT_INVITE"));
        return;
    }
    if (VaultInterestStopped(&player->vault, now)) {
        *color = VAULT_VIEW_WARN;
        StrCopy(out, size, LocGet("HODA_VAULT_STOPPED"));
        return;
    }
    char daily[VAULT_VIEW_LABEL_CAP], lock[VAULT_VIEW_LABEL_CAP];
    VaultViewFormatDaily(daily, sizeof(daily), VaultMilligemsPerDay(player->vault.locked));
    if (VaultUnlocked(&player->vault, now)) {
        StrCopy(lock, sizeof(lock), LocGet("HODA_VAULT_UNLOCKED"));
    } else {
        char left[VAULT_VIEW_LABEL_CAP];
        QuestsShortTime(left, sizeof(left), player->vault.unlocks_at - now);
        LocFormat1(lock, sizeof(lock), "HODA_VAULT_UNLOCKS_IN", left);
    }
    snprintf(out, size, "%s   \xC2\xB7   %s", daily, lock);
}

static void VaultViewDrawStatus(const Player *player, Rectangle list, long long now) {
    VaultTier tier = VaultTierFor(player->vault.locked);
    bool earned = tier != VAULT_TIER_NONE;
    Rectangle badge = UiRect(list, 0.5f, 0.0f, VAULT_VIEW_BADGE_SIDE, VAULT_VIEW_BADGE_SIDE);
    VaultViewDrawFit(
        VaultBadgeTexture(earned ? tier : VAULT_TIER_BRONZE), badge, earned ? WHITE : VAULT_VIEW_LOCKED_BADGE
    );
    Rectangle name = {list.x, badge.y + badge.height + 6.0f, list.width, 40.0f};
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        VaultViewTierName(tier),
        name,
        VAULT_VIEW_TIER_SIZE,
        earned ? VAULT_VIEW_GOLD : VAULT_VIEW_TEXT
    );
    Rectangle amount = {list.x, name.y + name.height + 4.0f, list.width, 50.0f};
    char locked[32];
    VaultViewFormatGems(locked, sizeof(locked), player->vault.locked);
    VaultViewDrawGemsCentred(GuiFont(FNT_COOL), locked, amount, VAULT_VIEW_AMOUNT_SIZE, VAULT_VIEW_TEXT);
    char status[VAULT_VIEW_TEXT_CAP];
    Color color;
    VaultViewStatusLine(player, now, status, sizeof(status), &color);
    Rectangle line = {list.x, amount.y + amount.height + 4.0f, list.width, 28.0f};
    StoreDrawCenteredFit(GuiFont(FNT_MENU), status, line, VAULT_VIEW_STATUS_SIZE, color);
}

static bool VaultViewCollect(Store *store, Player *player, long long now) {
    long long collected = VaultClaim(&player->vault, &player->gems, now);
    if (collected <= 0) return false;
    ProgressionSave(player);
    ServerActionSubmit(VaultAction(VAULT_ACTION_CLAIM, 0));
    GuiSoundPlay(SND_TALLY_GEM);
    char status[VAULT_VIEW_TEXT_CAP];
    LocFormatInt(status, sizeof(status), "HODA_VAULT_COLLECTED", (int)collected);
    StoreSetStatus(store, status);
    return true;
}

static bool VaultViewDrawActions(Store *store, Player *player, Rectangle list, long long now, bool interactive) {
    VaultView *v = &vault_view;
    long long ready = VaultPendingGems(&player->vault, now);
    if (ready > 0) {
        char gems[32];
        VaultViewFormatGems(gems, sizeof(gems), ready);
        LocFormat1(v->collect_label, sizeof(v->collect_label), "HODA_VAULT_COLLECT_GEMS", gems);
    } else {
        StrCopy(v->collect_label, sizeof(v->collect_label), LocGet("HODA_VAULT_COLLECT"));
    }
    float row_width = 3.0f * VAULT_VIEW_BUTTON_WIDTH + 2.0f * VAULT_VIEW_BUTTON_GAP;
    float x = list.x + (list.width - row_width) * 0.5f;
    float y = list.y + list.height - VAULT_VIEW_BUTTON_HEIGHT;
    Rectangle deposit = {x, y, VAULT_VIEW_BUTTON_WIDTH, VAULT_VIEW_BUTTON_HEIGHT};
    Rectangle collect = deposit, withdraw = deposit;
    collect.x += VAULT_VIEW_BUTTON_WIDTH + VAULT_VIEW_BUTTON_GAP;
    withdraw.x = collect.x + VAULT_VIEW_BUTTON_WIDTH + VAULT_VIEW_BUTTON_GAP;
    if (VaultViewDrawButton(&v->deposit, deposit, VaultViewMaxDeposit(player) > 0, interactive))
        VaultViewOpen(store, player, STORE_DIALOG_VAULT_DEPOSIT);
    if (VaultViewDrawButton(&v->withdraw, withdraw, player->vault.locked > 0, interactive))
        VaultViewOpen(store, player, STORE_DIALOG_VAULT_WITHDRAW);
    return VaultViewDrawButton(&v->collect, collect, ready > 0, interactive) && VaultViewCollect(store, player, now);
}

StoreAction VaultViewDraw(Store *store, Player *player) {
    VaultViewEnsure();
    Rectangle list = StoreListRect();
    if (!VaultAvailable()) {
        StoreDrawCenteredFit(store->title_font, LocGet("HODA_STORE_COMING_SOON"), list, 44.0f, VAULT_VIEW_TEXT);
        return STORE_ACTION_NONE;
    }
    long long now = ServerNow();
    VaultViewDrawStatus(player, list, now);
    bool collected = VaultViewDrawActions(store, player, list, now, !StoreInputBlocked(store));
    return collected ? STORE_ACTION_PURCHASED : STORE_ACTION_NONE;
}

static void VaultViewDrawStepper(const Player *player, StoreDialog dialog, Rectangle row) {
    VaultView *v = &vault_view;
    float width =
        2.0f * VAULT_VIEW_STEP_SIDE + VAULT_VIEW_FIELD_WIDTH + VAULT_VIEW_MAX_WIDTH + 3.0f * VAULT_VIEW_STEPPER_GAP;
    float x = row.x + (row.width - width) * 0.5f;
    float y = row.y + (row.height - VAULT_VIEW_STEP_SIDE) * 0.5f;
    Rectangle minus = {x, y, VAULT_VIEW_STEP_SIDE, VAULT_VIEW_STEP_SIDE};
    Rectangle field = {minus.x + minus.width + VAULT_VIEW_STEPPER_GAP, y, VAULT_VIEW_FIELD_WIDTH, VAULT_VIEW_STEP_SIDE};
    Rectangle plus = {field.x + field.width + VAULT_VIEW_STEPPER_GAP, y, VAULT_VIEW_STEP_SIDE, VAULT_VIEW_STEP_SIDE};
    Rectangle max = {plus.x + plus.width + VAULT_VIEW_STEPPER_GAP, y, VAULT_VIEW_MAX_WIDTH, VAULT_VIEW_STEP_SIDE};
    if (ButtonDraw(&v->minus, minus)) v->amount = VaultViewClamp(player, dialog, VaultViewStepDown(v->amount));
    if (ButtonDraw(&v->plus, plus)) v->amount = VaultViewClamp(player, dialog, VaultViewStepUp(v->amount));
    if (ButtonDraw(&v->max, max)) v->amount = VaultViewLimit(player, dialog);
    DrawRectangleRounded(field, 0.25f, 6, VAULT_VIEW_FIELD_FILL);
    DrawRectangleRoundedLinesEx(field, 0.25f, 6, 1.5f, VAULT_VIEW_FIELD_EDGE);
    char text[32];
    VaultViewFormatGems(text, sizeof(text), v->amount);
    VaultViewDrawGemsCentred(GuiFont(FNT_COOL), text, field, VAULT_VIEW_NOTE_SIZE + 8.0f, VAULT_VIEW_TEXT);
}

static void VaultViewDialogNote(const Player *player, StoreDialog dialog, char *out, size_t size, Color *color) {
    *color = VAULT_VIEW_DIM;
    out[0] = '\0';
    if (dialog == STORE_DIALOG_VAULT_DEPOSIT) {
        LocFormatInt(out, size, "HODA_VAULT_DEPOSIT_NOTE", (int)VaultLockDays());
        return;
    }
    if (VaultUnlocked(&player->vault, ServerNow())) return;
    char kept[32];
    VaultViewFormatGems(kept, sizeof(kept), vault_view.amount - VaultEarlyWithdrawFee(vault_view.amount));
    LocFormat1(out, size, "HODA_VAULT_EARLY_NOTE", kept);
    *color = VAULT_VIEW_WARN;
}

static bool VaultViewMove(Player *player, StoreDialog dialog) {
    long long amount = vault_view.amount;
    long long now = ServerNow();
    bool deposit = dialog == STORE_DIALOG_VAULT_DEPOSIT;
    bool moved = deposit ? VaultDeposit(&player->vault, &player->gems, amount, now)
                         : VaultWithdraw(&player->vault, &player->gems, amount, now) > 0;
    if (!moved) return false;
    ProgressionSave(player);
    ServerActionSubmit(VaultAction(deposit ? VAULT_ACTION_DEPOSIT : VAULT_ACTION_WITHDRAW, amount));
    GuiSoundPlay(deposit ? SND_GEM_PURCHASE : SND_TALLY_GEM);
    return true;
}

bool VaultViewDrawDialog(Store *store, Player *player) {
    VaultViewEnsure();
    StoreDialog dialog = store->dialog;
    vault_view.amount = VaultViewClamp(player, dialog, vault_view.amount);
    DrawRectangle(0, 0, (int)UiWidth(), (int)UiHeight(), VAULT_VIEW_SCRIM);
    Rectangle plate = MessagePopupRect();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), plate, WHITE);
    Rectangle content = UiRect(plate, 0.5f, 0.5f, 0.8f, 0.8f);
    bool deposit = dialog == STORE_DIALOG_VAULT_DEPOSIT;
    const char *title = LocGet(deposit ? "HODA_VAULT_CONFIRM_DEPOSIT_TITLE" : "HODA_VAULT_CONFIRM_WITHDRAW_TITLE");
    StoreDrawCenteredFit(
        store->title_font, title, UiRect(content, 0.5f, 0.0f, 1.0f, 0.2f), VAULT_VIEW_DIALOG_TITLE_SIZE, VAULT_VIEW_TEXT
    );
    VaultViewDrawStepper(player, dialog, UiRect(content, 0.5f, 0.38f, 1.0f, 0.24f));
    char note[VAULT_VIEW_TEXT_CAP];
    Color note_color;
    VaultViewDialogNote(player, dialog, note, sizeof(note), &note_color);
    StoreDrawCenteredFit(
        store->body_font, note, UiRect(content, 0.5f, 0.66f, 1.0f, 0.14f), VAULT_VIEW_NOTE_SIZE, note_color
    );
    Rectangle row = MessagePopupChoiceRow(plate);
    if (ButtonDraw(&store->dialog_cancel, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f))) {
        store->dialog = STORE_DIALOG_NONE;
        return false;
    }
    bool valid = VaultViewAmountValid(player, dialog, vault_view.amount);
    if (VaultViewDrawButton(&store->dialog_ok, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f), valid, true)) {
        store->dialog = STORE_DIALOG_NONE;
        return VaultViewMove(player, dialog);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) store->dialog_press_armed = true;
    if (!store->dialog_press_armed || CheckCollisionPointRec(UiMouse(), plate) ||
        !IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        return false;
    store->dialog = STORE_DIALOG_NONE;
    store->dialog_press_armed = false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    return false;
}
