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
#define VAULT_VIEW_SIDE_PADDING 26.0f
#define VAULT_VIEW_BADGE_WIDTH 150.0f
#define VAULT_VIEW_BADGE_NAME_GAP 10.0f
#define VAULT_VIEW_NAME_HEIGHT 34.0f
#define VAULT_VIEW_LADDER_BADGE 30.0f
#define VAULT_VIEW_LADDER_ROW_GAP 4.0f
#define VAULT_VIEW_LADDER_INSET 22.0f
#define VAULT_VIEW_EDGE_SHARE 0.3f
#define VAULT_VIEW_ROW_HEIGHT 40.0f
#define VAULT_VIEW_LABEL_SIZE 20.0f
#define VAULT_VIEW_VALUE_SIZE 22.0f
#define VAULT_VIEW_HINT_SIZE 15.0f
#define VAULT_VIEW_TITLE_SIZE 28.0f
#define VAULT_VIEW_GEM_ICON 24.0f
#define VAULT_VIEW_BUTTON_HEIGHT 52.0f
#define VAULT_VIEW_COLLECT_WIDTH 170.0f
#define VAULT_VIEW_WARNING_HEIGHT 24.0f
#define VAULT_VIEW_SECTION_GAP 10.0f
#define VAULT_VIEW_STEP_SIDE 46.0f
#define VAULT_VIEW_STEPPER_GAP 14.0f
#define VAULT_VIEW_AMOUNT_SIZE 24.0f
#define VAULT_VIEW_AMOUNT_HEIGHT 26.0f
#define VAULT_VIEW_PIP_HEIGHT 15.0f
#define VAULT_VIEW_PIP_MAX_WIDTH 30.0f
#define VAULT_VIEW_PIP_SOURCE_WIDTH 64.0f
#define VAULT_VIEW_PIP_SOURCE_HEIGHT 32.0f
#define VAULT_VIEW_LABEL_SCALE 0.4f
#define VAULT_VIEW_BUTTON_RIM_OF_WIDTH (5.0f / 256.0f)
#define VAULT_VIEW_BUTTON_GAP 16.0f
#define VAULT_VIEW_TARGET_STOPS 10LL
#define VAULT_VIEW_STOP_CAPACITY 16
#define VAULT_VIEW_TENTHS 10LL

typedef struct {
    long long values[VAULT_VIEW_STOP_CAPACITY];
    int count;
} VaultViewStops;

typedef struct {
    bool ready;
    long long amount;
    Button collect;
    Button deposit;
    Button withdraw;
    Button minus;
    Button plus;
} VaultView;

static VaultView vault_view;

static const long long VAULT_VIEW_STEP_SIZES[] = {1, 5, 10, 15, 25, 50, 100, 150, 250, 500, 1000, 1500, 2500};

static Button VaultViewButton(GuiId idle, GuiId active, const char *key) {
    return ButtonLoadText(idle, active, LocGet(key), GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, VAULT_VIEW_LABEL_SCALE);
}

static void VaultViewEnsure(void) {
    VaultView *v = &vault_view;
    if (v->ready) return;
    v->ready = true;
    v->collect = VaultViewButton(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "HODA_VAULT_COLLECT");
    v->deposit = VaultViewButton(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "HODA_VAULT_DEPOSIT");
    v->withdraw = VaultViewButton(GUI_BTN_GREY, GUI_BTN_GREY_ON, "HODA_VAULT_WITHDRAW");
    v->minus = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "-", GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.6f);
    v->plus = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "+", GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.6f);
}

static void VaultViewDrawDisabled(const Button *button, Rectangle rect) {
    UiDrawTexture(GuiTex(GUI_BTN_GREY), rect, VAULT_VIEW_DISABLED_TINT);
    StoreDrawCenteredFit(
        button->font, button->label, rect, rect.height * button->label_scale, VAULT_VIEW_DISABLED_LABEL
    );
}

static bool VaultViewDrawButton(Button *button, Rectangle rect, bool enabled, bool interactive) {
    if (enabled) return ButtonDrawEx(button, rect, interactive);
    VaultViewDrawDisabled(button, rect);
    return false;
}

static float VaultViewFrameOutset(float width) {
    float rim = VAULT_VIEW_BUTTON_RIM_OF_WIDTH;
    return rim * (width - VAULT_VIEW_STEP_SIDE) / (1.0f - rim);
}

static Rectangle VaultViewFrameFromLeft(Rectangle rect) {
    float outset = VaultViewFrameOutset(rect.width);
    return (Rectangle){rect.x - outset, rect.y, rect.width + outset, rect.height};
}

static Rectangle VaultViewFrameToRight(Rectangle rect) {
    return (Rectangle){rect.x, rect.y, rect.width + VaultViewFrameOutset(rect.width), rect.height};
}

static bool VaultViewCanWithdraw(const Player *player, long long amount) {
    return VaultAvailable() && amount > 0 && amount <= player->vault.locked;
}

static bool VaultViewCanDeposit(const Player *player, long long amount) {
    return VaultDepositFits(&player->vault, amount) && amount <= player->gems;
}

long long VaultViewDepositable(const Player *player) {
    long long room = VaultCapacity() - player->vault.locked;
    long long depositable = player->gems < room ? player->gems : room;
    return VaultViewCanDeposit(player, depositable) ? depositable : 0;
}

static void VaultViewAddStop(VaultViewStops *stops, long long value) {
    if (value <= 0 || stops->count == VAULT_VIEW_STOP_CAPACITY) return;
    int at = stops->count;
    while (at > 0 && stops->values[at - 1] > value) at--;
    if (at > 0 && stops->values[at - 1] == value) return;
    for (int i = stops->count; i > at; i--) stops->values[i] = stops->values[i - 1];
    stops->values[at] = value;
    stops->count++;
}

long long VaultViewStepFor(long long top) {
    int last = (int)(sizeof(VAULT_VIEW_STEP_SIZES) / sizeof(VAULT_VIEW_STEP_SIZES[0])) - 1;
    for (int i = 0; i < last; i++)
        if (top <= VAULT_VIEW_STEP_SIZES[i] * VAULT_VIEW_TARGET_STOPS) return VAULT_VIEW_STEP_SIZES[i];
    return VAULT_VIEW_STEP_SIZES[last];
}

VaultViewStops VaultViewStopsFor(const Player *player) {
    VaultViewStops stops = {0};
    long long depositable = VaultViewDepositable(player), locked = player->vault.locked;
    long long top = depositable > locked ? depositable : locked;
    if (!VaultAvailable() || top <= 0) return stops;
    long long step = VaultViewStepFor(top);
    for (long long value = step; value < top; value += step)
        if (VaultViewCanDeposit(player, value) || VaultViewCanWithdraw(player, value)) VaultViewAddStop(&stops, value);
    VaultViewAddStop(&stops, depositable);
    VaultViewAddStop(&stops, locked);
    return stops;
}

static int VaultViewStopIndex(const VaultViewStops *stops, long long amount) {
    int index = 0;
    for (int i = 0; i < stops->count; i++)
        if (stops->values[i] <= amount) index = i;
    return index;
}

long long VaultViewSnap(const Player *player, long long amount, int move) {
    VaultViewStops stops = VaultViewStopsFor(player);
    if (stops.count == 0) return 0;
    int index = VaultViewStopIndex(&stops, amount) + move;
    index = index < 0 ? 0 : index >= stops.count ? stops.count - 1 : index;
    return stops.values[index];
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

static void VaultViewDrawGemsCentred(Font font, const char *text, Rectangle row, float size, Color color) {
    float width = MeasureTextEx(font, text, size, 1.0f).x + 6.0f + VAULT_VIEW_GEM_ICON;
    Rectangle centred = {row.x + (row.width - width) * 0.5f, row.y, width, row.height};
    VaultViewDrawGemsRight(font, text, centred, size, color);
}

static float VaultViewLadderRowHeight(void) {
    return VAULT_VIEW_LADDER_BADGE + 6.0f;
}

static float VaultViewBadgeHeight(VaultTier tier) {
    Rectangle art = VaultBadgeArt(tier);
    return art.width > 0.0f ? VAULT_VIEW_BADGE_WIDTH * art.height / art.width : VAULT_VIEW_BADGE_WIDTH;
}

static void VaultViewDrawBadgeArt(VaultTier tier, Rectangle dst, Color tint) {
    Texture2D texture = VaultBadgeTexture(tier);
    Rectangle art = VaultBadgeArt(tier);
    if (texture.id == 0 || art.width <= 0.0f || art.height <= 0.0f) return;
    DrawTexturePro(texture, art, dst, (Vector2){0}, 0.0f, tint);
}

static void VaultViewDrawTierPanel(const Player *player, Rectangle panel) {
    DrawRectangleRec(panel, VAULT_VIEW_PANEL);
    VaultTier tier = VaultTierFor(player->vault.locked);
    bool earned = tier != VAULT_TIER_NONE;
    VaultTier shown = earned ? tier : VAULT_TIER_BRONZE;
    float badge_height = VaultViewBadgeHeight(shown);
    float rungs = VAULT_TIER_COUNT - 1;
    float ladder = rungs * VaultViewLadderRowHeight() + (rungs - 1.0f) * VAULT_VIEW_LADDER_ROW_GAP;
    float used = badge_height + VAULT_VIEW_BADGE_NAME_GAP + VAULT_VIEW_NAME_HEIGHT + ladder;
    float edge = fmaxf(panel.height - used, 0.0f) * VAULT_VIEW_EDGE_SHARE;
    Rectangle badge = {
        panel.x + (panel.width - VAULT_VIEW_BADGE_WIDTH) * 0.5f, panel.y + edge, VAULT_VIEW_BADGE_WIDTH, badge_height
    };
    VaultViewDrawBadgeArt(shown, badge, earned ? WHITE : VAULT_VIEW_LOCKED_BADGE);
    Rectangle title = {
        panel.x + 8.0f, badge.y + badge.height + VAULT_VIEW_BADGE_NAME_GAP, panel.width - 16.0f, VAULT_VIEW_NAME_HEIGHT
    };
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        VaultViewTierName(tier),
        title,
        VAULT_VIEW_TITLE_SIZE,
        earned ? VAULT_VIEW_GOLD : VAULT_VIEW_TEXT
    );
    float y = panel.y + panel.height - edge - ladder;
    Rectangle rule_span = {panel.x + VAULT_VIEW_LADDER_INSET, 0.0f, panel.width - 2.0f * VAULT_VIEW_LADDER_INSET, 0.0f};
    VaultViewDrawRule(rule_span, roundf((title.y + title.height + y) * 0.5f));
    for (int rung = VAULT_TIER_BRONZE; rung < VAULT_TIER_COUNT; rung++) {
        Rectangle row = {
            panel.x + VAULT_VIEW_LADDER_INSET,
            y,
            panel.width - 2.0f * VAULT_VIEW_LADDER_INSET,
            VaultViewLadderRowHeight()
        };
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

static bool VaultViewDrawCollect(Store *store, Player *player, Rectangle section, long long now, bool interactive) {
    VaultView *v = &vault_view;
    long long ready = VaultPendingGems(&player->vault, now);
    bool stopped = VaultInterestStopped(&player->vault, now);
    float block = VAULT_VIEW_BUTTON_HEIGHT + (stopped ? VAULT_VIEW_WARNING_HEIGHT : 0.0f);
    float top = section.y + (section.height - block) * 0.5f;
    Rectangle button = VaultViewFrameToRight((Rectangle){
        section.x + section.width - VAULT_VIEW_COLLECT_WIDTH, top, VAULT_VIEW_COLLECT_WIDTH, VAULT_VIEW_BUTTON_HEIGHT
    });
    Rectangle row = {section.x, top, section.width - VAULT_VIEW_COLLECT_WIDTH - 16.0f, VAULT_VIEW_BUTTON_HEIGHT};
    UiDrawTextLeftMiddle(GuiFont(FNT_MENU), LocGet("HODA_VAULT_READY"), row, VAULT_VIEW_LABEL_SIZE, VAULT_VIEW_DIM);
    char text[32];
    VaultViewFormatGems(text, sizeof(text), ready);
    VaultViewDrawGemsRight(GuiFont(FNT_COOL), text, row, VAULT_VIEW_VALUE_SIZE + 4.0f, VAULT_VIEW_GOLD);
    if (stopped) {
        Rectangle warning = {section.x, row.y + row.height + 2.0f, section.width, VAULT_VIEW_WARNING_HEIGHT - 2.0f};
        StoreDrawCenteredFit(
            GuiFont(FNT_MENU), LocGet("HODA_VAULT_STOPPED"), warning, VAULT_VIEW_HINT_SIZE, VAULT_VIEW_WARN
        );
    }
    if (!VaultViewDrawButton(&v->collect, button, ready > 0, interactive)) return false;
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

static void VaultViewDrawPips(const VaultViewStops *stops, int lit, Rectangle area) {
    if (stops->count <= 0) return;
    float width = fminf(area.width / stops->count, VAULT_VIEW_PIP_MAX_WIDTH);
    float x = area.x + (area.width - width * stops->count) * 0.5f;
    for (int pip = 0; pip < stops->count; pip++) {
        bool on = pip <= lit;
        Rectangle source = {
            on ? 0.0f : VAULT_VIEW_PIP_SOURCE_WIDTH, 0.0f, VAULT_VIEW_PIP_SOURCE_WIDTH, VAULT_VIEW_PIP_SOURCE_HEIGHT
        };
        Rectangle target = {x + pip * width, area.y, width, VAULT_VIEW_PIP_HEIGHT};
        DrawTexturePro(GuiTex(on ? GUI_FORTIFY_TIER_4 : GUI_FORTIFY_TIER_1), source, target, (Vector2){0}, 0.0f, WHITE);
    }
}

static void VaultViewDrawAmount(Player *player, Rectangle panel, bool interactive) {
    VaultView *v = &vault_view;
    VaultViewStops stops = VaultViewStopsFor(player);
    v->amount = VaultViewSnap(player, v->amount, 0);
    int index = VaultViewStopIndex(&stops, v->amount);
    Rectangle minus = {panel.x, panel.y, VAULT_VIEW_STEP_SIDE, VAULT_VIEW_STEP_SIDE};
    Rectangle plus = {
        panel.x + panel.width - VAULT_VIEW_STEP_SIDE, panel.y, VAULT_VIEW_STEP_SIDE, VAULT_VIEW_STEP_SIDE
    };
    if (VaultViewDrawButton(&v->minus, minus, stops.count > 0 && index > 0, interactive))
        v->amount = VaultViewSnap(player, v->amount, -1);
    if (VaultViewDrawButton(&v->plus, plus, index + 1 < stops.count, interactive))
        v->amount = VaultViewSnap(player, v->amount, 1);
    float inner_x = minus.x + minus.width + VAULT_VIEW_STEPPER_GAP;
    float inner_width = plus.x - VAULT_VIEW_STEPPER_GAP - inner_x;
    if (stops.count == 0) {
        char minimum[VAULT_VIEW_TEXT_CAP];
        LocFormatInt(minimum, sizeof(minimum), "HODA_VAULT_MINIMUM", (int)VaultMinimumDeposit());
        Rectangle hint = {inner_x, panel.y, inner_width, panel.height};
        StoreDrawCenteredFit(GuiFont(FNT_MENU), minimum, hint, VAULT_VIEW_LABEL_SIZE, VAULT_VIEW_DIM);
        return;
    }
    float block = VAULT_VIEW_AMOUNT_HEIGHT + 3.0f + VAULT_VIEW_PIP_HEIGHT;
    float top = panel.y + (panel.height - block) * 0.5f;
    char text[32];
    VaultViewFormatGems(text, sizeof(text), v->amount);
    Rectangle amount = {inner_x, top, inner_width, VAULT_VIEW_AMOUNT_HEIGHT};
    VaultViewDrawGemsCentred(GuiFont(FNT_COOL), text, amount, VAULT_VIEW_AMOUNT_SIZE, VAULT_VIEW_TEXT);
    Rectangle pips = {inner_x, amount.y + amount.height + 3.0f, inner_width, VAULT_VIEW_PIP_HEIGHT};
    VaultViewDrawPips(&stops, stops.count > 0 ? index : -1, pips);
}

static void VaultViewDrawMoveButtons(Store *store, const Player *player, Rectangle panel, bool interactive) {
    VaultView *v = &vault_view;
    float width = (panel.width - VAULT_VIEW_BUTTON_GAP) * 0.5f;
    Rectangle deposit = VaultViewFrameFromLeft((Rectangle){panel.x, panel.y, width, VAULT_VIEW_BUTTON_HEIGHT});
    Rectangle withdraw = VaultViewFrameToRight(
        (Rectangle){panel.x + width + VAULT_VIEW_BUTTON_GAP, panel.y, width, VAULT_VIEW_BUTTON_HEIGHT}
    );
    if (VaultViewDrawButton(&v->deposit, deposit, VaultViewCanDeposit(player, v->amount), interactive))
        store->dialog = STORE_DIALOG_VAULT_DEPOSIT;
    if (VaultViewDrawButton(&v->withdraw, withdraw, VaultViewCanWithdraw(player, v->amount), interactive))
        store->dialog = STORE_DIALOG_VAULT_WITHDRAW;
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
    Rectangle plate = StorePlateRect();
    float inner_top = plate.y + plate.height * STORE_PLATE_INNER_TOP_OF_PLATE;
    float inner_bottom = plate.y + plate.height * STORE_PLATE_INNER_BOTTOM_OF_PLATE;
    Rectangle left = {list.x, inner_top, left_width, inner_bottom - inner_top};
    float collect_height = VAULT_VIEW_BUTTON_HEIGHT + VAULT_VIEW_WARNING_HEIGHT;
    float column = 3 * VAULT_VIEW_ROW_HEIGHT + collect_height + VAULT_VIEW_STEP_SIDE + VAULT_VIEW_BUTTON_HEIGHT +
                   8.0f * VAULT_VIEW_SECTION_GAP;
    float right_x = left.x + left.width + VAULT_VIEW_SIDE_PADDING;
    Rectangle right = {
        right_x, list.y + (list.height - column) * 0.5f, list.x + list.width - VAULT_VIEW_SIDE_PADDING - right_x, column
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
    y = amount.y + amount.height + VAULT_VIEW_SECTION_GAP;
    VaultViewDrawRule(right, y);
    Rectangle moves = {right.x, y + VAULT_VIEW_SECTION_GAP, right.width, VAULT_VIEW_BUTTON_HEIGHT};
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
    vault_view.amount = VaultViewSnap(player, amount, 0);
    return true;
}
