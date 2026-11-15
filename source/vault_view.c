#define VAULT_VIEW_TEXT ((Color){236, 232, 220, 255})
#define VAULT_VIEW_DIM ((Color){176, 168, 150, 255})
#define VAULT_VIEW_GOLD ((Color){240, 200, 110, 255})
#define VAULT_VIEW_WARN ((Color){232, 128, 96, 255})
#define VAULT_VIEW_RULE ((Color){255, 255, 255, 44})
#define VAULT_VIEW_PANEL ((Color){0, 0, 0, 96})
#define VAULT_VIEW_LOCKED_BADGE ((Color){90, 90, 90, 200})
#define VAULT_VIEW_DISABLED_TINT ((Color){105, 105, 105, 255})
#define VAULT_VIEW_DISABLED_LABEL ((Color){150, 146, 138, 255})
#define VAULT_VIEW_TEXT_CAP 192
#define VAULT_VIEW_LEFT_WIDTH 0.36f
#define VAULT_VIEW_COLUMN_GAP 24.0f
#define VAULT_VIEW_BADGE_SIDE 150.0f
#define VAULT_VIEW_LADDER_BADGE 30.0f
#define VAULT_VIEW_ROW_HEIGHT 40.0f
#define VAULT_VIEW_LABEL_SIZE 20.0f
#define VAULT_VIEW_VALUE_SIZE 22.0f
#define VAULT_VIEW_HINT_SIZE 15.0f
#define VAULT_VIEW_TITLE_SIZE 28.0f
#define VAULT_VIEW_GEM_ICON 24.0f
#define VAULT_VIEW_BUTTON_HEIGHT 52.0f
#define VAULT_VIEW_STEP_SIDE 46.0f
#define VAULT_VIEW_LABEL_SCALE 0.4f
#define VAULT_VIEW_SMALL_STEP 100LL
#define VAULT_VIEW_MEDIUM_STEP 500LL
#define VAULT_VIEW_LARGE_STEP 1000LL
#define VAULT_VIEW_MEDIUM_FROM 1000LL
#define VAULT_VIEW_LARGE_FROM 5000LL
#define VAULT_VIEW_TENTHS 10LL
#define VAULT_VIEW_NAME_HEIGHT 34.0f
#define VAULT_VIEW_LADDER_ROW_GAP 4.0f
#define VAULT_VIEW_LADDER_TOP_GAP 18.0f
#define VAULT_VIEW_SECTION_GAP 10.0f
#define VAULT_VIEW_WARNING_HEIGHT 24.0f

typedef struct {
    bool ready;
    long long amount;
    Button collect;
    Button deposit;
    Button withdraw;
    Button minus;
    Button plus;
    Button max;
} VaultView;

static VaultView vault_view;

static Button VaultViewButton(GuiId idle, GuiId active, const char *key) {
    return ButtonLoadText(idle, active, LocGet(key), GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, VAULT_VIEW_LABEL_SCALE);
}

static void VaultViewEnsure(void) {
    VaultView *v = &vault_view;
    if (v->ready) return;
    v->ready = true;
    v->amount = VaultMinimumDeposit();
    v->collect = VaultViewButton(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "HODA_VAULT_COLLECT");
    v->deposit = VaultViewButton(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "HODA_VAULT_DEPOSIT");
    v->withdraw = VaultViewButton(GUI_BTN_GREY, GUI_BTN_GREY_ON, "HODA_VAULT_WITHDRAW");
    v->minus = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "-", GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.6f);
    v->plus = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "+", GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.6f);
    v->max = VaultViewButton(GUI_BTN_GREY, GUI_BTN_GREY_ON, "HODA_VAULT_MAX");
}

static void VaultViewDrawDisabled(const Button *button, Rectangle rect) {
    UiDrawTexture(GuiTex(GUI_BTN_GREY), rect, VAULT_VIEW_DISABLED_TINT);
    float size = rect.height * VAULT_VIEW_LABEL_SCALE;
    StoreDrawCenteredFit(button->font, button->label, rect, size, VAULT_VIEW_DISABLED_LABEL);
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

static bool VaultViewCanWithdraw(const Player *player, long long amount) {
    return VaultAvailable() && amount > 0 && amount <= player->vault.locked;
}

static bool VaultViewCanDeposit(const Player *player, long long amount) {
    return VaultDepositFits(&player->vault, amount) && amount <= player->gems;
}

long long VaultViewMaxAmount(const Player *player) {
    long long room = VaultCapacity() - player->vault.locked;
    long long depositable = player->gems < room ? player->gems : room;
    return VaultViewCanDeposit(player, depositable) ? depositable : player->vault.locked;
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

static void VaultViewDrawGemsRight(Font font, const char *text, Rectangle row, float size, Color color) {
    Rectangle icon = {
        row.x + row.width - VAULT_VIEW_GEM_ICON,
        row.y + (row.height - VAULT_VIEW_GEM_ICON) * 0.5f,
        VAULT_VIEW_GEM_ICON,
        VAULT_VIEW_GEM_ICON
    };
    VaultViewDrawFit(GuiTex(GUI_STORE_CURRENCY_GEMS), icon, WHITE);
    Rectangle text_rect = {row.x, row.y, row.width - VAULT_VIEW_GEM_ICON - 6.0f, row.height};
    UiDrawTextRight(font, text, text_rect, size, color);
}

static void VaultViewDrawRule(Rectangle panel, float y) {
    DrawRectangleRec((Rectangle){panel.x, y, panel.width, 1.0f}, VAULT_VIEW_RULE);
}

static float VaultViewLadderRowHeight(void) {
    return VAULT_VIEW_LADDER_BADGE + 6.0f;
}

static void VaultViewDrawTierPanel(const Player *player, Rectangle panel) {
    DrawRectangleRec(panel, VAULT_VIEW_PANEL);
    VaultTier tier = VaultTierFor(player->vault.locked);
    float ladder = (VAULT_TIER_COUNT - 1) * (VaultViewLadderRowHeight() + VAULT_VIEW_LADDER_ROW_GAP);
    float block = VAULT_VIEW_BADGE_SIDE + 8.0f + VAULT_VIEW_NAME_HEIGHT + VAULT_VIEW_LADDER_TOP_GAP + ladder;
    Rectangle badge = UiRect(panel, 0.5f, 0.0f, VAULT_VIEW_BADGE_SIDE, VAULT_VIEW_BADGE_SIDE);
    badge.y = panel.y + (panel.height - block) * 0.5f;
    bool earned = tier != VAULT_TIER_NONE;
    VaultViewDrawFit(
        VaultBadgeTexture(earned ? tier : VAULT_TIER_BRONZE), badge, earned ? WHITE : VAULT_VIEW_LOCKED_BADGE
    );
    Rectangle title = {panel.x + 8.0f, badge.y + badge.height + 8.0f, panel.width - 16.0f, VAULT_VIEW_NAME_HEIGHT};
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        VaultViewTierName(tier),
        title,
        VAULT_VIEW_TITLE_SIZE,
        earned ? VAULT_VIEW_GOLD : VAULT_VIEW_TEXT
    );
    float y = title.y + title.height + VAULT_VIEW_LADDER_TOP_GAP;
    for (int rung = VAULT_TIER_BRONZE; rung < VAULT_TIER_COUNT; rung++) {
        Rectangle row = {panel.x + 22.0f, y, panel.width - 44.0f, VaultViewLadderRowHeight()};
        bool reached = tier >= rung;
        Rectangle icon = {row.x, row.y + 3.0f, VAULT_VIEW_LADDER_BADGE, VAULT_VIEW_LADDER_BADGE};
        VaultViewDrawFit(VaultBadgeTexture((VaultTier)rung), icon, reached ? WHITE : VAULT_VIEW_LOCKED_BADGE);
        Color color = reached ? VAULT_VIEW_TEXT : VAULT_VIEW_DIM;
        Rectangle name = {icon.x + icon.width + 10.0f, row.y, row.width * 0.55f, row.height};
        UiDrawTextLeftMiddle(
            GuiFont(FNT_MENU), VaultViewTierName((VaultTier)rung), name, VAULT_VIEW_LABEL_SIZE - 2.0f, color
        );
        char gems[32];
        VaultViewFormatGems(gems, sizeof(gems), VaultTierGems((VaultTier)rung));
        VaultViewDrawGemsRight(GuiFont(FNT_MENU), gems, row, VAULT_VIEW_LABEL_SIZE - 2.0f, color);
        y += row.height + VAULT_VIEW_LADDER_ROW_GAP;
    }
}

static Rectangle VaultViewStatRow(Rectangle panel, int index) {
    return (Rectangle){panel.x, panel.y + index * VAULT_VIEW_ROW_HEIGHT, panel.width, VAULT_VIEW_ROW_HEIGHT};
}

static void VaultViewDrawStats(const Player *player, Rectangle panel, long long now) {
    Font label = GuiFont(FNT_MENU), value = GuiFont(FNT_COOL);
    char text[VAULT_VIEW_TEXT_CAP];
    Rectangle row = VaultViewStatRow(panel, 0);
    UiDrawTextLeftMiddle(label, LocGet("HODA_VAULT_LOCKED"), row, VAULT_VIEW_LABEL_SIZE, VAULT_VIEW_DIM);
    VaultViewFormatGems(text, sizeof(text), player->vault.locked);
    VaultViewDrawGemsRight(value, text, row, VAULT_VIEW_VALUE_SIZE, VAULT_VIEW_TEXT);

    row = VaultViewStatRow(panel, 1);
    UiDrawTextLeftMiddle(label, LocGet("HODA_VAULT_INTEREST"), row, VAULT_VIEW_LABEL_SIZE, VAULT_VIEW_DIM);
    VaultViewFormatDaily(text, sizeof(text), VaultMilligemsPerDay(player->vault.locked));
    VaultViewDrawGemsRight(value, text, row, VAULT_VIEW_VALUE_SIZE, VAULT_VIEW_TEXT);

    row = VaultViewStatRow(panel, 2);
    UiDrawTextLeftMiddle(label, LocGet("HODA_VAULT_LOCK"), row, VAULT_VIEW_LABEL_SIZE, VAULT_VIEW_DIM);
    if (player->vault.locked <= 0) {
        StrCopy(text, sizeof(text), LocGet("HODA_VAULT_EMPTY"));
    } else if (VaultUnlocked(&player->vault, now)) {
        StrCopy(text, sizeof(text), LocGet("HODA_VAULT_UNLOCKED"));
    } else {
        char left[48];
        QuestsShortTime(left, sizeof(left), player->vault.unlocks_at - now);
        LocFormat1(text, sizeof(text), "HODA_VAULT_UNLOCKS_IN", left);
    }
    UiDrawTextRight(value, text, row, VAULT_VIEW_VALUE_SIZE, VAULT_VIEW_TEXT);
}

static bool VaultViewDrawCollect(Store *store, Player *player, Rectangle panel, long long now, bool interactive) {
    VaultView *v = &vault_view;
    long long ready = VaultPendingGems(&player->vault, now);
    Rectangle button = {panel.x + panel.width - 170.0f, panel.y + 4.0f, 170.0f, VAULT_VIEW_BUTTON_HEIGHT};
    Rectangle row = {panel.x, panel.y + 4.0f, panel.width - 186.0f, VAULT_VIEW_BUTTON_HEIGHT};
    UiDrawTextLeftMiddle(GuiFont(FNT_MENU), LocGet("HODA_VAULT_READY"), row, VAULT_VIEW_LABEL_SIZE, VAULT_VIEW_DIM);
    char text[32];
    VaultViewFormatGems(text, sizeof(text), ready);
    VaultViewDrawGemsRight(GuiFont(FNT_COOL), text, row, VAULT_VIEW_VALUE_SIZE + 4.0f, VAULT_VIEW_GOLD);
    if (VaultInterestStopped(&player->vault, now)) {
        Rectangle warning = {panel.x, row.y + row.height + 2.0f, panel.width, VAULT_VIEW_WARNING_HEIGHT - 2.0f};
        StoreDrawCenteredFit(
            GuiFont(FNT_MENU), LocGet("HODA_VAULT_STOPPED"), warning, VAULT_VIEW_HINT_SIZE, VAULT_VIEW_WARN
        );
    }
    if (ready <= 0) {
        VaultViewDrawDisabled(&v->collect, button);
        return false;
    }
    if (!ButtonDrawEx(&v->collect, button, interactive)) return false;
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

static void VaultViewDrawAmount(Player *player, Rectangle panel, bool interactive) {
    VaultView *v = &vault_view;
    long long max_amount = VaultViewMaxAmount(player);
    Rectangle minus = {panel.x, panel.y, VAULT_VIEW_STEP_SIDE, VAULT_VIEW_STEP_SIDE};
    Rectangle max = {panel.x + panel.width - 110.0f, panel.y, 110.0f, VAULT_VIEW_STEP_SIDE};
    Rectangle plus = {max.x - 12.0f - VAULT_VIEW_STEP_SIDE, panel.y, VAULT_VIEW_STEP_SIDE, VAULT_VIEW_STEP_SIDE};
    Rectangle field = {
        minus.x + minus.width + 10.0f, panel.y, plus.x - minus.x - minus.width - 20.0f, VAULT_VIEW_STEP_SIDE
    };
    if (ButtonDrawEx(&v->minus, minus, interactive)) v->amount = VaultViewStepDown(v->amount);
    if (ButtonDrawEx(&v->plus, plus, interactive)) v->amount = VaultViewStepUp(v->amount);
    if (ButtonDrawEx(&v->max, max, interactive) && max_amount > 0) v->amount = max_amount;
    DrawRectangleRounded(field, 0.25f, 6, (Color){0, 0, 0, 150});
    DrawRectangleRoundedLinesEx(field, 0.25f, 6, 1.5f, (Color){160, 140, 100, 200});
    char text[32];
    VaultViewFormatGems(text, sizeof(text), v->amount);
    Font font = GuiFont(FNT_COOL);
    float width = MeasureTextEx(font, text, VAULT_VIEW_VALUE_SIZE + 4.0f, 1.0f).x + 6.0f + VAULT_VIEW_GEM_ICON;
    Rectangle centred = {field.x + (field.width - width) * 0.5f, field.y, width, field.height};
    VaultViewDrawGemsRight(font, text, centred, VAULT_VIEW_VALUE_SIZE + 4.0f, VAULT_VIEW_TEXT);
}

static void VaultViewDrawMoveButtons(Store *store, const Player *player, Rectangle panel, bool interactive) {
    VaultView *v = &vault_view;
    float width = (panel.width - 16.0f) * 0.5f;
    Rectangle deposit = {panel.x, panel.y, width, VAULT_VIEW_BUTTON_HEIGHT};
    Rectangle withdraw = {panel.x + width + 16.0f, panel.y, width, VAULT_VIEW_BUTTON_HEIGHT};
    if (VaultViewCanDeposit(player, v->amount)) {
        if (ButtonDrawEx(&v->deposit, deposit, interactive)) store->dialog = STORE_DIALOG_VAULT_DEPOSIT;
    } else {
        VaultViewDrawDisabled(&v->deposit, deposit);
    }
    if (VaultViewCanWithdraw(player, v->amount)) {
        if (ButtonDrawEx(&v->withdraw, withdraw, interactive)) store->dialog = STORE_DIALOG_VAULT_WITHDRAW;
    } else {
        VaultViewDrawDisabled(&v->withdraw, withdraw);
    }
}

StoreAction VaultViewDraw(Store *store, Player *player) {
    VaultViewEnsure();
    Rectangle list = StoreListRect();
    if (!VaultAvailable()) {
        StoreDrawCenteredFit(store->title_font, LocGet("HODA_STORE_COMING_SOON"), list, 44.0f, VAULT_VIEW_TEXT);
        return STORE_ACTION_NONE;
    }
    long long now = ServerNow();
    bool interactive = !StoreInputBlocked(store);
    float left_width = list.width * VAULT_VIEW_LEFT_WIDTH;
    Rectangle left = {list.x, list.y, left_width, list.height};
    float collect_height = VAULT_VIEW_BUTTON_HEIGHT + VAULT_VIEW_WARNING_HEIGHT;
    float column = 3 * VAULT_VIEW_ROW_HEIGHT + collect_height + VAULT_VIEW_STEP_SIDE + VAULT_VIEW_BUTTON_HEIGHT +
                   6.0f * VAULT_VIEW_SECTION_GAP;
    Rectangle right = {
        list.x + left_width + VAULT_VIEW_COLUMN_GAP,
        list.y + (list.height - column) * 0.5f,
        list.width - left_width - VAULT_VIEW_COLUMN_GAP,
        column
    };
    VaultViewDrawTierPanel(player, left);
    VaultViewDrawStats(player, right, now);
    float y = right.y + 3 * VAULT_VIEW_ROW_HEIGHT + VAULT_VIEW_SECTION_GAP;
    VaultViewDrawRule(right, y);
    Rectangle collect = {right.x, y + VAULT_VIEW_SECTION_GAP, right.width, collect_height};
    bool collected = VaultViewDrawCollect(store, player, collect, now, interactive);
    y = collect.y + collect.height + VAULT_VIEW_SECTION_GAP;
    VaultViewDrawRule(right, y);
    Rectangle amount = {right.x, y + VAULT_VIEW_SECTION_GAP, right.width, VAULT_VIEW_STEP_SIDE};
    VaultViewDrawAmount(player, amount, interactive);
    Rectangle moves = {
        right.x, amount.y + amount.height + VAULT_VIEW_SECTION_GAP, right.width, VAULT_VIEW_BUTTON_HEIGHT
    };
    VaultViewDrawMoveButtons(store, player, moves, interactive);
    return collected ? STORE_ACTION_PURCHASED : STORE_ACTION_NONE;
}

void VaultViewDialogText(const Store *store, const Player *player, char *header, char *body, size_t size) {
    long long amount = vault_view.amount;
    char gems[32];
    VaultViewFormatGems(gems, sizeof(gems), amount);
    if (store->dialog == STORE_DIALOG_VAULT_DEPOSIT) {
        StrCopy(header, size, LocGet("HODA_VAULT_CONFIRM_DEPOSIT_TITLE"));
        char days[16];
        snprintf(days, sizeof(days), "%lld", VaultLockDays());
        LocFormat2(body, size, "HODA_VAULT_CONFIRM_DEPOSIT", gems, days);
        return;
    }
    StrCopy(header, size, LocGet("HODA_VAULT_CONFIRM_WITHDRAW_TITLE"));
    if (VaultUnlocked(&player->vault, ServerNow())) {
        LocFormat1(body, size, "HODA_VAULT_CONFIRM_WITHDRAW", gems);
        return;
    }
    char kept[32];
    VaultViewFormatGems(kept, sizeof(kept), amount - VaultEarlyWithdrawFee(amount));
    LocFormat2(body, size, "HODA_VAULT_CONFIRM_EARLY", kept, gems);
}

bool VaultViewConfirm(Player *player, StoreDialog dialog) {
    long long amount = vault_view.amount;
    long long now = ServerNow();
    bool deposit = dialog == STORE_DIALOG_VAULT_DEPOSIT;
    bool moved = deposit ? VaultDeposit(&player->vault, &player->gems, amount, now)
                         : VaultWithdraw(&player->vault, &player->gems, amount, now) > 0;
    if (!moved) return false;
    ProgressionSave(player);
    ServerActionSubmit(VaultAction(deposit ? VAULT_ACTION_DEPOSIT : VAULT_ACTION_WITHDRAW, amount));
    GuiSoundPlay(deposit ? SND_GEM_PURCHASE : SND_TALLY_GEM);
    if (!deposit && !VaultViewCanWithdraw(player, amount) && !VaultViewCanDeposit(player, amount))
        vault_view.amount = player->vault.locked > 0 ? player->vault.locked : VaultMinimumDeposit();
    return true;
}
