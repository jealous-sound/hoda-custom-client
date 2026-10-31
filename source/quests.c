typedef enum { QUESTS_NONE = 0, QUESTS_MAP, QUESTS_NEXUS, QUESTS_UNLOCK, QUESTS_FORFEIT, QUESTS_CLOSE } QuestButton;

typedef enum {
    QUESTS_DIALOG_NONE = 0,
    QUESTS_DIALOG_MESSAGE,
    QUESTS_DIALOG_UNLOCK,
    QUESTS_DIALOG_COMMITTED
} QuestsDialogKind;

#define QUESTS_LANE_INSET 92.0f
#define QUESTS_CELL_STRIDE 244.0f
#define QUESTS_CELL_GUTTER 12.0f
#define QUESTS_CELL_HEIGHT 226.0f
#define QUESTS_SCROLL_STEP (QUESTS_CELL_STRIDE * 2.0f)
#define QUESTS_HUB_IMAGE_SIZE 72.0f
#define QUESTS_MAX_HUB_IMAGES NEXUS_MAX_EVENTS
#define QUESTS_DIALOG_TEXT_SIZE 512
#define QUESTS_SECONDS_PER_MINUTE 60LL
#define QUESTS_SECONDS_PER_HOUR 3600LL
#define QUESTS_CURRENT_MAP_TEXTURE_PREFIX "tex.Btn_MapPVE_map_"
#define QUESTS_CAMPAIGN_ID_PREFIX "PVE_MAP_"
#define QUESTS_TIMER_ICON "IconTimer"

static const Color QUESTS_KEY_TINTS[NEXUS_KEY_COUNT] = {
    {31, 128, 244, 255},
    {255, 243, 69, 255},
    {98, 237, 43, 255},
    {90, 90, 175, 255},
};
static const Color QUESTS_COMMITTED_ELSEWHERE = {150, 150, 150, 255};
static const Color QUESTS_TEXT = {235, 232, 220, 255};
static const Color QUESTS_TEXT_DIM = {162, 159, 152, 255};

typedef struct {
    QuestsDialogKind kind;
    int event;
    char text[QUESTS_DIALOG_TEXT_SIZE];
    bool opening_release;
} QuestsDialog;

typedef struct {
    Texture2D background;
    Texture2D frame;
    Texture2D titlebar;
    Texture2D divider;
    Texture2D arrow;
    Texture2D energy;
    Texture2D popup;
    Texture2D timer_icon;
    Texture2D key_icons[NEXUS_KEY_COUNT];
    Font font;
    Font body;
    Button map;
    Button esc;
    Button back;
    Button dialog_yes;
    Button dialog_no;
    Button dialog_ok;
    Button dialog_close;
    bool scrim_armed;
    bool key_strip_armed;
    bool show_key_instructions;

    bool hub_images_loaded;
    char hub_image_names[QUESTS_MAX_HUB_IMAGES][64];
    Texture2D hub_images[QUESTS_MAX_HUB_IMAGES];
    int hub_image_count;
    int event_hub_image[NEXUS_MAX_EVENTS];
    Texture2D current_maps[CAMPAIGN_MAX_MAPS];
    bool current_maps_loaded;

    int live[NEXUS_MAX_EVENTS];
    int live_count;
    float scroll;
    bool dragging;
    bool drag_scrolled;
    float drag_start_x;
    float scroll_start;

    QuestsDialog dialog;
    int ladder_event;
    int selected_event;
    int selected_difficulty;
    int pending_event;
} Quests;

void QuestsInitialize(Quests *q) {
    memset(q, 0, sizeof(*q));
    q->background = GuiTex(GUI_QUESTS_BG);
    q->frame = GuiTex(GUI_NEXUS_EVENT_FRAME);
    q->titlebar = GuiTex(GUI_NEXUS_EVENT_TITLEBAR);
    q->divider = GuiTex(GUI_NEXUS_DIVIDER);
    q->arrow = GuiTex(GUI_STORE_SCROLL_ARROW);
    q->energy = GuiTex(GUI_HUD_ENERGY);
    q->popup = GuiTex(GUI_STORE_OVERLAY);
    q->timer_icon = LoadSprite(CACHE_RESOURCE(QUESTS_TIMER_ICON));
    for (int i = 0; i < NEXUS_KEY_COUNT; i++) q->key_icons[i] = GuiTex(GUI_NEXUS_KEY_1 + i);
    q->font = GuiFont(FNT_COOL);
    q->body = GuiFont(FNT_MENU);
    q->map = ButtonLoad(GUI_BTN_MAINQUEST, GUI_BTN_MAINQUEST_ON);
    q->esc = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    q->back = ButtonLoad(GUI_STORE_BACK, GUI_STORE_BACK);
    ButtonSetClickSound(&q->esc, SND_UI_POPUP_CLOSE);
    ButtonSetClickSound(&q->back, SND_UI_POPUP_CLOSE);
    q->dialog_ok = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "", q->font, (Vector2){0.5f, 0.5f}, 0.32f);
    q->dialog_yes = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "", q->font, (Vector2){0.5f, 0.5f}, 0.32f);
    q->dialog_no = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "", q->font, (Vector2){0.5f, 0.5f}, 0.32f);
    q->dialog_close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&q->dialog_close, SND_UI_POPUP_CLOSE);
    q->dialog.event = -1;
    q->ladder_event = -1;
    q->selected_event = -1;
    q->selected_difficulty = -1;
    q->pending_event = -1;
}

int QuestsSelectedNexusEvent(const Quests *q) {
    return q->selected_event;
}

int QuestsSelectedNexusDifficulty(const Quests *q) {
    return q->selected_difficulty;
}

int QuestsPendingNexusEvent(const Quests *q) {
    return q->pending_event;
}

static const char *QuestsDifficultyTitle(const char *difficulty) {
    if (!strcmp(difficulty, "easy")) return LocGetOr("Event_Nexus_Easy_Title", "Легко");
    if (!strcmp(difficulty, "medium")) return LocGetOr("Event_Nexus_Medium_Title", "Умеренно");
    if (!strcmp(difficulty, "hard")) return LocGetOr("Event_Nexus_Hard_Title", "Сложно");
    if (!strcmp(difficulty, "veryhard")) return LocGetOr("Event_Nexus_VERYHard_Title", "Очень сложно");
    if (!strcmp(difficulty, "insane")) return LocGetOr("Event_Nexus_Insane_Title", "Безумие");
    if (!strcmp(difficulty, "ludicrous")) return LocGetOr("Event_Nexus_Ludicrous_Title", "Абсурд");
    return difficulty;
}

static void QuestsDialogClose(Quests *q) {
    q->dialog.kind = QUESTS_DIALOG_NONE;
    q->dialog.event = -1;
}

void QuestsClose(Quests *q) {
    q->ladder_event = -1;
    QuestsDialogClose(q);
}

bool QuestsHandleBack(Quests *q) {
    if (q->dialog.kind != QUESTS_DIALOG_NONE) {
        QuestsDialogClose(q);
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (q->ladder_event >= 0) {
        q->ladder_event = -1;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    return false;
}

static long long QuestsNow(void) {
    return (long long)time(NULL);
}

static bool QuestsEventLocked(const Nexus *nexus, const Player *player, int event) {
    return NexusEventLocked(nexus, player, event, QuestsNow());
}

static bool QuestsEventSelectable(const Nexus *nexus, const Player *player, int event) {
    if (QuestsEventLocked(nexus, player, event)) return false;
    if (NexusRunBlocking(nexus, player)) return NexusEventCommitted(player, event);
    return true;
}

static bool QuestsCommittedElsewhere(const Nexus *nexus, const Player *player, int event) {
    return NexusRunBlocking(nexus, player) && !NexusEventCommitted(player, event);
}

static bool QuestsPlayerHasUnlockItem(const Player *player, const NexusEvent *e) {
    return e->unlock_key == 0 || PlayerKeyCount(player, e->unlock_key) >= e->unlock_key_qty;
}

static int QuestsKeyIndex(int inventory_id) {
    return Clamp(inventory_id - NEXUS_KEY_FIRST_ID, 0, NEXUS_KEY_COUNT - 1);
}

static bool QuestsEventPrecedes(const Nexus *nexus, const Player *player, int a, int b, long long now) {
    bool a_committed = NexusEventCommitted(player, a);
    bool b_committed = NexusEventCommitted(player, b);
    if (a_committed != b_committed) return a_committed;
    long long a_left = NexusEventTimeRemaining(nexus, player, a, now);
    long long b_left = NexusEventTimeRemaining(nexus, player, b, now);
    if (a_left != b_left) return a_left < b_left;
    return strcmp(NexusEventAt(nexus, a)->id, NexusEventAt(nexus, b)->id) < 0;
}

static void QuestsCollectLiveEvents(Quests *q, const Nexus *nexus, const Player *player) {
    int weekday = NexusWeekdayToday();
    long long now = QuestsNow();
    q->live_count = 0;
    for (int i = 0; i < NexusEventCount(nexus); i++) {
        if (!NexusEventLive(NexusEventAt(nexus, i), weekday)) continue;
        int at = q->live_count++;
        while (at > 0 && QuestsEventPrecedes(nexus, player, i, q->live[at - 1], now)) {
            q->live[at] = q->live[at - 1];
            at--;
        }
        q->live[at] = i;
    }
}

static Texture2D QuestsLoadCacheSprite(const char *name) {
    return LoadSprite(TextFormat("%s%s", CACHE_RESOURCE_PREFIX, name));
}

static void QuestsEnsureHubImages(Quests *q, const Nexus *nexus) {
    if (q->hub_images_loaded) return;
    q->hub_images_loaded = true;
    for (int event = 0; event < NexusEventCount(nexus); event++) {
        const char *name = NexusEventAt(nexus, event)->hub_image;
        q->event_hub_image[event] = -1;
        if (!name[0]) continue;
        int found = -1;
        for (int i = 0; i < q->hub_image_count; i++) {
            if (!strcmp(q->hub_image_names[i], name)) found = i;
        }
        if (found < 0 && q->hub_image_count < QUESTS_MAX_HUB_IMAGES) {
            found = q->hub_image_count++;
            StrCopy(q->hub_image_names[found], sizeof(q->hub_image_names[found]), name);
            q->hub_images[found] = QuestsLoadCacheSprite(name);
        }
        q->event_hub_image[event] = found;
    }
}

static Texture2D QuestsLoadAlphaMaskSprite(const char *name) {
    Image image = ResourceLoadImage(TextFormat("%s%s", CACHE_RESOURCE_PREFIX, name));
    if (!image.data) return (Texture2D){0};
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    unsigned char *pixels = image.data;
    for (int i = 0; i < image.width * image.height; i++) {
        pixels[i * 4 + 0] = 255;
        pixels[i * 4 + 1] = 255;
        pixels[i * 4 + 2] = 255;
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    return texture;
}

static void QuestsEnsureCurrentMaps(Quests *q, const Campaign *campaign) {
    if (q->current_maps_loaded) return;
    q->current_maps_loaded = true;
    size_t prefix = strlen(QUESTS_CAMPAIGN_ID_PREFIX);
    for (int i = 0; i < campaign->map_count && i < CAMPAIGN_MAX_MAPS; i++) {
        const char *id = campaign->maps[i].id;
        if (strncmp(id, QUESTS_CAMPAIGN_ID_PREFIX, prefix) != 0) continue;
        q->current_maps[i] =
            QuestsLoadAlphaMaskSprite(TextFormat("%s%s", QUESTS_CURRENT_MAP_TEXTURE_PREFIX, id + prefix));
    }
}

static int QuestsHighestUnlockedMap(const Campaign *campaign, const PlayerCampaignProgress *progress) {
    int highest = -1;
    for (int i = 0; i < campaign->map_count; i++) {
        if (CampaignMapUnlocked(campaign, progress, i)) highest = i;
    }
    return highest;
}

static bool QuestsMapCompleted(const Campaign *campaign, const PlayerCampaignProgress *progress, int map) {
    int boss = CampaignBossNodeIndex(campaign, map);
    return boss >= 0 && progress->node_completions[map][boss] > 0;
}

static void QuestsShortTime(char *out, size_t size, long long seconds) {
    if (seconds < 0) seconds = 0;
    long long unit = seconds < QUESTS_SECONDS_PER_MINUTE ? 1
                     : seconds < QUESTS_SECONDS_PER_HOUR ? QUESTS_SECONDS_PER_MINUTE
                     : seconds < NEXUS_SECONDS_PER_DAY   ? QUESTS_SECONDS_PER_HOUR
                                                         : NEXUS_SECONDS_PER_DAY;
    long long remainder = unit == 1 ? 0 : seconds % unit;
    long long sub_unit = remainder < QUESTS_SECONDS_PER_MINUTE ? 1
                         : remainder < QUESTS_SECONDS_PER_HOUR ? QUESTS_SECONDS_PER_MINUTE
                         : remainder < NEXUS_SECONDS_PER_DAY   ? QUESTS_SECONDS_PER_HOUR
                                                               : NEXUS_SECONDS_PER_DAY;
    static const struct {
        long long unit;
        const char *key, *fallback;
    } suffixes[] = {
        {1, "SECONDS_SHORT", "сек."},
        {QUESTS_SECONDS_PER_MINUTE, "MINUTE_SHORT", "мин."},
        {QUESTS_SECONDS_PER_HOUR, "HOUR_SHORT", "ч."},
        {NEXUS_SECONDS_PER_DAY, "DAY_SHORT", "д."},
    };
    const char *suffix = "", *sub_suffix = "";
    for (int i = 0; i < (int)(sizeof(suffixes) / sizeof(suffixes[0])); i++) {
        if (suffixes[i].unit == unit) suffix = LocGetOr(suffixes[i].key, suffixes[i].fallback);
        if (suffixes[i].unit == sub_unit) sub_suffix = LocGetOr(suffixes[i].key, suffixes[i].fallback);
    }
    long long sub_value = remainder / sub_unit;
    if (unit != 1 && sub_value != 0)
        snprintf(out, size, "%lld%s %lld%s", seconds / unit, suffix, sub_value, sub_suffix);
    else
        snprintf(out, size, "%lld%s", seconds / unit, suffix);
}

static void QuestsStripRichTags(char *text) {
    char *write = text;
    for (const char *read = text; *read;) {
        if (*read == '<') {
            const char *end = strchr(read, '>');
            if (end && (!strncmp(read, "<color", 6) || !strncmp(read, "</color", 7))) {
                read = end + 1;
                continue;
            }
        }
        *write++ = *read++;
    }
    *write = '\0';
}

static void QuestsHubTimeString(char *out, size_t size, const Nexus *nexus, const Player *player, int event) {
    long long now = QuestsNow();
    const NexusEvent *e = NexusEventAt(nexus, event);
    long long remaining = NexusEventTimeRemaining(nexus, player, event, now);
    if (remaining <= 0) {
        StrCopy(out, size, LocGetOr("PVE_HUB_NEXUS_TIME_EXPIRED", "Время вышло!"));
        return;
    }
    bool unlocked = NexusEventGated(e) && !NexusEventLocked(nexus, player, event, now);
    char time_text[48];
    QuestsShortTime(
        time_text,
        sizeof(time_text),
        NexusEventTimeGated(e) && unlocked ? NexusEventUnlockRemaining(nexus, player, event, now) : remaining
    );
    if (NexusEventRunGated(e) && unlocked) {
        int runs = player->nexus_events.remaining_runs[event];
        char runs_text[16];
        snprintf(runs_text, sizeof(runs_text), "%d", runs);
        LocFormat2(
            out,
            size,
            runs == 1 ? "PVE_HUB_NEXUS_TIME_AND_ONE_RUN_REMAINING_NO_DISCLAIMER"
                      : "PVE_HUB_NEXUS_TIME_AND_MULTIPLE_RUNS_REMAINING_NO_DISCLAIMER",
            time_text,
            runs_text
        );
        return;
    }
    LocFormat2(out, size, "PVE_HUB_NEXUS_TIME_REMAINING", time_text, "");
}

static void QuestsEventTitle(char *out, size_t size, const Nexus *nexus, const Player *player, int event) {
    char time_text[128];
    QuestsHubTimeString(time_text, sizeof(time_text), nexus, player, event);
    QuestsStripRichTags(time_text);
    LocFormat2(out, size, "PVE_HUB_NEXUS_EVENT_TITLE_FORMAT", NexusEventAt(nexus, event)->name, time_text);
}

static void QuestsUnlockConfirmText(char *out, size_t size, const NexusEvent *e) {
    char duration[48], runs[16];
    QuestsShortTime(duration, sizeof(duration), e->unlock_duration);
    snprintf(runs, sizeof(runs), "%d", e->max_runs);
    bool timed = NexusEventTimeGated(e), counted = NexusEventRunGated(e);
    if (timed && counted) {
        const char *args[] = {e->name, duration, runs};
        LocFormatArgs(
            out,
            size,
            e->max_runs == 1 ? "CONFIRM_NEXUS_UNLOCK_TIME_LIMIT_OR_SINGLE_RUN"
                             : "CONFIRM_NEXUS_UNLOCK_TIME_LIMIT_OR_MULT_RUNS",
            args,
            3
        );
    } else if (counted) {
        LocFormat2(
            out,
            size,
            e->max_runs == 1 ? "CONFIRM_NEXUS_UNLOCK_SINGLE_RUN" : "CONFIRM_NEXUS_UNLOCK_MULT_RUNS",
            e->name,
            runs
        );
    } else if (timed) {
        LocFormat2(out, size, "CONFIRM_NEXUS_UNLOCK_TIME_LIMIT", e->name, duration);
    } else {
        LocFormat2(out, size, "CONFIRM_NEXUS_UNLOCK", e->name, "");
    }
}

static void QuestsDialogOpen(Quests *q, QuestsDialogKind kind, int event, const char *text) {
    q->dialog.kind = kind;
    q->dialog.event = event;
    q->dialog.opening_release = true;
    StrCopy(q->dialog.text, sizeof(q->dialog.text), text);
}

static void QuestsRequestUnlock(Quests *q, const Nexus *nexus, const Player *player, int event) {
    const NexusEvent *e = NexusEventAt(nexus, event);
    if (!QuestsPlayerHasUnlockItem(player, e)) {
        QuestsDialogOpen(q, QUESTS_DIALOG_MESSAGE, -1, LocGet("INSUFFICIENT_NEXUS_KEYS"));
        return;
    }
    char text[QUESTS_DIALOG_TEXT_SIZE];
    QuestsUnlockConfirmText(text, sizeof(text), e);
    QuestsDialogOpen(q, QUESTS_DIALOG_UNLOCK, event, text);
}

static void QuestsExplainCommitment(Quests *q, const Nexus *nexus, const Player *player, int event) {
    const NexusEvent *committed = NexusEventAt(nexus, player->nexus_run.event_index);
    char text[QUESTS_DIALOG_TEXT_SIZE];
    LocFormat2(text, sizeof(text), "NEXUS_COMMIT_MAP_EXPLANATION", committed ? committed->name : "", "");
    QuestsDialogOpen(q, QUESTS_DIALOG_COMMITTED, event, text);
}

static bool QuestsInputBlocked(const Quests *q) {
    return UiInputBlocked() || q->drag_scrolled || q->dialog.kind != QUESTS_DIALOG_NONE;
}

static bool QuestsReleasedOn(const Quests *q, Rectangle rect) {
    return !QuestsInputBlocked(q) && CheckCollisionPointRec(UiMouse(), rect) &&
           IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void QuestsDrawFrame(Texture2D texture, Rectangle dst, float border, Color tint) {
    float sx[4] = {0.0f, border, (float)texture.width - border, (float)texture.width};
    float sy[4] = {0.0f, border, (float)texture.height - border, (float)texture.height};
    float dx[4] = {dst.x, dst.x + border, dst.x + dst.width - border, dst.x + dst.width};
    float dy[4] = {dst.y, dst.y + border, dst.y + dst.height - border, dst.y + dst.height};
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            Rectangle src = {sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]};
            Rectangle out = {dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row]};
            DrawTexturePro(texture, src, out, (Vector2){0}, 0.0f, tint);
        }
    }
}

static void QuestsDrawFit(Font font, const char *text, Rectangle rect, float size, Color color) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    if (measured.x > rect.width && measured.x > 0.0f) size *= rect.width / measured.x;
    UiDrawTextCentered(font, text, rect, size, color);
}

static void QuestsDrawActionPlate(const Quests *q, Rectangle rect, const char *label, bool gold, Color tint) {
    bool hover = !QuestsInputBlocked(q) && CheckCollisionPointRec(UiMouse(), rect);
    GuiId art = gold ? (hover ? GUI_BTN_GOLD_ON : GUI_BTN_GOLD) : (hover ? GUI_BTN_GREY_ON : GUI_BTN_GREY);
    UiDrawTexture(GuiTex(art), rect, tint);
    QuestsDrawFit(
        q->font,
        label,
        (Rectangle){rect.x + 12.0f, rect.y, rect.width - 24.0f, rect.height},
        rect.height * 0.42f,
        ColorTint(QUESTS_TEXT, tint)
    );
}

static void QuestsDrawScrollArrow(const Quests *q, Rectangle rect, bool left, bool enabled) {
    Rectangle src = {0.0f, 0.0f, (float)q->arrow.width, (float)q->arrow.height};
    if (left) {
        src.x = (float)q->arrow.width;
        src.width = -(float)q->arrow.width;
    }
    Color tint = enabled ? WHITE : (Color){105, 105, 105, 115};
    if (enabled && !QuestsInputBlocked(q) && CheckCollisionPointRec(UiMouse(), rect))
        tint = (Color){255, 224, 133, 255};
    DrawTexturePro(q->arrow, src, rect, (Vector2){0}, 0.0f, tint);
}

static void QuestsDrawImageFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;
    float scale = fminf(dst.width / texture.width, dst.height / texture.height);
    Rectangle fit = {
        dst.x + (dst.width - texture.width * scale) * 0.5f,
        dst.y + (dst.height - texture.height * scale) * 0.5f,
        texture.width * scale,
        texture.height * scale,
    };
    UiDrawTexture(texture, fit, tint);
}

static float QuestsMaxScroll(const Quests *q, Rectangle viewport) {
    float content = q->live_count * QUESTS_CELL_STRIDE;
    return content > viewport.width ? content - viewport.width : 0.0f;
}

static void QuestsUpdateScrollInput(Quests *q, Rectangle viewport) {
    Vector2 mouse = UiMouse();
    if (!UiInputBlocked() && q->dialog.kind == QUESTS_DIALOG_NONE) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, viewport)) {
            q->dragging = true;
            q->drag_start_x = mouse.x;
            q->scroll_start = q->scroll;
        }
        if (q->dragging) {
            float delta = mouse.x - q->drag_start_x;
            if (fabsf(delta) > 13.0f) q->drag_scrolled = true;
            if (q->drag_scrolled) q->scroll = q->scroll_start - delta;
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) q->dragging = false;
        } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            q->drag_scrolled = false;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && CheckCollisionPointRec(mouse, viewport)) q->scroll -= wheel * 160.0f;
    }
    q->scroll = Clamp(q->scroll, 0.0f, QuestsMaxScroll(q, viewport));
}

static void QuestsDrawUnlockRequirement(const Quests *q, const Player *player, const NexusEvent *e, Rectangle band) {
    bool has_item = QuestsPlayerHasUnlockItem(player, e);
    Color tint = has_item ? WHITE : QUESTS_COMMITTED_ELSEWHERE;
    Texture2D icon = q->timer_icon;
    Color icon_tint = tint;
    if (e->unlock_key != 0) {
        int key = QuestsKeyIndex(e->unlock_key);
        icon = q->key_icons[key];
        icon_tint = ColorTint(QUESTS_KEY_TINTS[key], tint);
    }
    bool show_quantity = e->unlock_key != 0 && e->unlock_key_qty > 1;
    float icon_size = band.height;
    const char *quantity = TextFormat("%d", e->unlock_key_qty);
    float quantity_width = show_quantity ? MeasureTextEx(q->font, quantity, 23.0f, 1.0f).x + 8.0f : 0.0f;
    float x = band.x + (band.width - icon_size - quantity_width) * 0.5f;
    QuestsDrawImageFit(icon, (Rectangle){x, band.y, icon_size, icon_size}, icon_tint);
    if (show_quantity)
        UiDrawTextLeftMiddle(
            q->font,
            quantity,
            (Rectangle){x + icon_size + 8.0f, band.y, quantity_width, band.height},
            23.0f,
            ColorTint(QUESTS_TEXT, tint)
        );
}

static void QuestsHandleCellClick(Quests *q, const Nexus *nexus, const Player *player, int event) {
    if (QuestsEventLocked(nexus, player, event)) {
        QuestsRequestUnlock(q, nexus, player, event);
    } else if (QuestsCommittedElsewhere(nexus, player, event)) {
        QuestsExplainCommitment(q, nexus, player, event);
    } else {
        q->ladder_event = event;
    }
    GuiSoundPlay(SND_UI_TAB);
}

static bool QuestsDrawEventCell(Quests *q, const Nexus *nexus, const Player *player, int event, Rectangle cell) {
    const NexusEvent *e = NexusEventAt(nexus, event);
    bool committed = NexusEventCommitted(player, event);
    bool locked = QuestsEventLocked(nexus, player, event);
    bool selectable = QuestsEventSelectable(nexus, player, event);

    QuestsDrawFrame(q->frame, cell, 4.0f, selectable || locked ? WHITE : QUESTS_COMMITTED_ELSEWHERE);
    if (committed) DrawRectangleLinesEx(cell, 3.0f, e->highlight);

    Rectangle category = {cell.x + 5.0f, cell.y + 5.0f, cell.width - 10.0f, 46.0f};
    DrawTexturePro(
        q->titlebar,
        (Rectangle){0.0f, 140.0f, (float)q->titlebar.width, 232.0f},
        category,
        (Vector2){0},
        0.0f,
        e->highlight
    );
    QuestsDrawFit(q->font, LocGet(e->subtitle), category, 23.0f, (Color){246, 244, 236, 255});

    Rectangle image = {cell.x + 12.0f, cell.y + 60.0f, QUESTS_HUB_IMAGE_SIZE, QUESTS_HUB_IMAGE_SIZE};
    int hub = q->event_hub_image[event];
    if (hub >= 0) QuestsDrawImageFit(q->hub_images[hub], image, WHITE);

    char title[256];
    QuestsEventTitle(title, sizeof(title), nexus, player, event);
    float text_x = image.x + image.width + 10.0f;
    HeroesDrawRich(
        q->body,
        title,
        (Rectangle){text_x, image.y, cell.x + cell.width - 12.0f - text_x, 100.0f},
        17.0f,
        selectable || locked ? QUESTS_TEXT : QUESTS_TEXT_DIM
    );

    float foot = cell.y + cell.height;
    UiDrawTexture(q->divider, (Rectangle){cell.x + 14.0f, foot - 92.0f, cell.width - 28.0f, 10.0f}, e->highlight);

    Rectangle plate = {cell.x + 16.0f, foot - 48.0f, cell.width - 32.0f, 40.0f};
    if (locked) {
        QuestsDrawUnlockRequirement(q, player, e, (Rectangle){cell.x + 20.0f, foot - 80.0f, cell.width - 40.0f, 30.0f});
        const char *label =
            e->unlock_key == 0 ? LocGetOr("GAUNTLET_START_TIMER", "Запустить") : LocGetOr("BUTTON_LOCKED", "Закрыто");
        QuestsDrawActionPlate(
            q, plate, label, false, QuestsPlayerHasUnlockItem(player, e) ? WHITE : QUESTS_COMMITTED_ELSEWHERE
        );
        return QuestsReleasedOn(q, plate);
    }

    bool unlocked = NexusEventGated(e);
    const char *label = !unlocked || e->unlock_key == 0 ? LocGetOr("PVE_HUB_EVENT_ENTER_BUTTON", "Войти")
                                                        : LocGetOr("BUTTON_UNLOCKED", "Открыто");
    if (committed) label = LocGetOr("PVE_HUB_NEXUS_EVENT_CONTINUE_BUTTON", "Продолжить");
    QuestsDrawActionPlate(
        q, plate, label, committed, QuestsCommittedElsewhere(nexus, player, event) ? QUESTS_COMMITTED_ELSEWHERE : WHITE
    );
    return QuestsReleasedOn(q, cell);
}

static void QuestsDrawCarousel(Quests *q, const Nexus *nexus, const Player *player, Rectangle panel) {
    UiDrawTextCentered(
        q->font,
        LocGetOr("PVE_HUB_SELECT_MAP", "Выберите карту"),
        (Rectangle){panel.x, panel.y + 86.0f, panel.width, 0.0f},
        24.0f,
        (Color){206, 198, 176, 255}
    );

    Rectangle viewport = {
        panel.x + QUESTS_LANE_INSET, panel.y + 120.0f, panel.width - 2.0f * QUESTS_LANE_INSET, QUESTS_CELL_HEIGHT
    };
    QuestsUpdateScrollInput(q, viewport);

    float max_scroll = QuestsMaxScroll(q, viewport);
    Rectangle left = {viewport.x - 48.0f, viewport.y + viewport.height * 0.5f - 26.0f, 42.0f, 52.0f};
    Rectangle right = {viewport.x + viewport.width + 6.0f, left.y, 42.0f, 52.0f};
    QuestsDrawScrollArrow(q, left, true, q->scroll > 0.0f);
    QuestsDrawScrollArrow(q, right, false, q->scroll < max_scroll);
    if (q->scroll > 0.0f && QuestsReleasedOn(q, left)) {
        q->scroll -= QUESTS_SCROLL_STEP;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (q->scroll < max_scroll && QuestsReleasedOn(q, right)) {
        q->scroll += QUESTS_SCROLL_STEP;
        GuiSoundPlay(SND_UI_TAB);
    }
    q->scroll = Clamp(q->scroll, 0.0f, max_scroll);

    if (q->live_count == 0) {
        UiDrawTextCentered(
            q->font,
            LocGetOr("NEXUS_HUB_FEATURED_PLACEHOLDER", "Готовьтесь к новой карте!"),
            (Rectangle){viewport.x, viewport.y, viewport.width, viewport.height},
            28.0f,
            (Color){178, 178, 178, 255}
        );
        return;
    }

    int clicked = -1;
    float scale = UiScale();
    BeginScissorMode(
        (int)(viewport.x * scale),
        (int)(viewport.y * scale),
        (int)(viewport.width * scale),
        (int)(viewport.height * scale)
    );
    for (int i = 0; i < q->live_count; i++) {
        float x = viewport.x + i * QUESTS_CELL_STRIDE - q->scroll;
        if (x + QUESTS_CELL_STRIDE < viewport.x || x > viewport.x + viewport.width) continue;
        Rectangle cell = {
            x + QUESTS_CELL_GUTTER * 0.5f, viewport.y, QUESTS_CELL_STRIDE - QUESTS_CELL_GUTTER, viewport.height
        };
        bool released = QuestsDrawEventCell(q, nexus, player, q->live[i], cell);
        Vector2 mouse = UiMouse();
        if (clicked < 0 && released && CheckCollisionPointRec(mouse, viewport)) clicked = q->live[i];
    }
    EndScissorMode();
    if (clicked >= 0) QuestsHandleCellClick(q, nexus, player, clicked);
}

static bool QuestsDrawLadder(Quests *q, const Nexus *nexus, const Player *player, Rectangle panel) {
    const NexusEvent *e = NexusEventAt(nexus, q->ladder_event);
    if (!e) {
        q->ladder_event = -1;
        return false;
    }

    QuestsDrawFit(
        q->font,
        e->name,
        (Rectangle){panel.x + 130.0f, panel.y + 86.0f, panel.width - 260.0f, 32.0f},
        30.0f,
        e->highlight
    );
    UiDrawTextCentered(
        q->body,
        LocGet(e->subtitle),
        (Rectangle){panel.x, panel.y + 122.0f, panel.width, 0.0f},
        20.0f,
        (Color){196, 192, 182, 255}
    );

    Rectangle list = {panel.x + 96.0f, panel.y + 156.0f, panel.width - 192.0f, 376.0f};
    float stride = fminf(list.height / (float)e->difficulty_count, 96.0f);
    float top = list.y + (list.height - stride * e->difficulty_count) * 0.5f;
    bool entered = false;
    for (int i = 0; i < e->difficulty_count; i++) {
        const NexusDifficulty *d = NexusDifficultyAt(nexus, q->ladder_event, i);
        Rectangle row = {list.x, top + i * stride, list.width, stride - 10.0f};
        bool committed = NexusEventCommitted(player, q->ladder_event) && player->nexus_run.difficulty_index == i;
        bool selectable =
            NexusRunBlocking(nexus, player) ? committed : QuestsEventSelectable(nexus, player, q->ladder_event);
        Color label_color = selectable ? (Color){240, 236, 224, 255} : (Color){164, 161, 154, 255};
        QuestsDrawFrame(q->frame, row, 4.0f, selectable ? WHITE : QUESTS_COMMITTED_ELSEWHERE);
        if (committed) DrawRectangleLinesEx(row, 3.0f, e->highlight);

        float cy = row.y + row.height * 0.5f;
        Rectangle plate = {row.x + row.width - 172.0f, cy - 23.0f, 156.0f, 46.0f};
        Rectangle nodes = {plate.x - 148.0f, row.y, 132.0f, row.height};
        Rectangle cost = {nodes.x - 196.0f, row.y, 180.0f, row.height};

        char title[96];
        StrCopy(title, sizeof(title), QuestsDifficultyTitle(d->difficulty));
        if (player->nexus_events.completions[q->ladder_event][i] > 0) {
            char done[96];
            LocFormat2(done, sizeof(done), "PVE_HUB_NEXUS_MAP_COMPLETED", title, "");
            if (done[0]) StrCopy(title, sizeof(title), done);
        }
        QuestsDrawFit(
            q->font, title, (Rectangle){row.x + 22.0f, row.y, cost.x - row.x - 36.0f, row.height}, 26.0f, label_color
        );

        Color muted = selectable ? (Color){198, 194, 184, 255} : (Color){136, 133, 128, 255};
        UiDrawTextLeftMiddle(q->body, LocGetOr("PVE_HUB_MAP_COST", "Цена:"), cost, 19.0f, muted);
        UiDrawTexture(
            q->energy,
            (Rectangle){cost.x + cost.width - 78.0f, cy - 15.0f, 30.0f, 30.0f},
            selectable ? WHITE : (Color){140, 140, 140, 220}
        );
        UiDrawTextRight(q->font, TextFormat("%d", d->energy_cost), cost, 24.0f, label_color);
        UiDrawTextRight(q->body, TextFormat("Узлов: %d", d->node_count), nodes, 19.0f, muted);

        const char *label = committed ? LocGetOr("PVE_HUB_NEXUS_EVENT_CONTINUE_BUTTON", "Продолжить")
                                      : LocGetOr("PVE_HUB_EVENT_ENTER_BUTTON", "Войти");
        QuestsDrawActionPlate(q, plate, label, committed, selectable ? WHITE : QUESTS_COMMITTED_ELSEWHERE);
        if (!entered && selectable && QuestsReleasedOn(q, row)) {
            q->selected_event = q->ladder_event;
            q->selected_difficulty = i;
            GuiSoundPlay(SND_PVE_NODE_SELECTED);
            entered = true;
        }
    }
    return entered;
}

static void QuestsDrawKeyStrip(Quests *q, const Player *player, Rectangle strip) {
    Vector2 mouse = UiMouse();
    bool hover = !QuestsInputBlocked(q) && CheckCollisionPointRec(mouse, strip);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) q->key_strip_armed = true;
    if (hover && q->key_strip_armed && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        q->show_key_instructions = !q->show_key_instructions;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) q->key_strip_armed = false;

    if (q->show_key_instructions) {
        HeroesDrawRich(q->body, LocGet("NEXUS_UNLOCK_INSTRUCTIONS"), strip, 15.0f, QUESTS_TEXT_DIM);
        return;
    }
    int owned = 0;
    for (int key = 0; key < NEXUS_KEY_COUNT; key++) {
        if (player->keys[key] > 0) owned++;
    }
    if (owned == 0) {
        HeroesDrawRich(q->body, LocGet("NEXUS_UNLOCK_NO_KEYS"), strip, 15.0f, QUESTS_TEXT_DIM);
        return;
    }
    float slot = strip.width / (float)(owned + 1);
    float icon = fminf(strip.height - 8.0f, 52.0f);
    int drawn = 0;
    for (int key = 0; key < NEXUS_KEY_COUNT; key++) {
        if (player->keys[key] <= 0) continue;
        drawn++;
        float cx = strip.x + slot * drawn;
        Rectangle icon_rect = {cx - icon * 0.5f, strip.y + (strip.height - icon) * 0.5f, icon, icon};
        QuestsDrawImageFit(q->key_icons[key], icon_rect, QUESTS_KEY_TINTS[key]);
        UiDrawTextLeftMiddle(
            q->font,
            TextFormat("%d", player->keys[key]),
            (Rectangle){icon_rect.x + icon * 0.72f, icon_rect.y + icon * 0.55f, icon, icon * 0.45f},
            22.0f,
            QUESTS_TEXT
        );
    }
}

static bool
QuestsDrawCurrentMap(Quests *q, const Campaign *campaign, const PlayerCampaignProgress *progress, Rectangle area) {
    int map = QuestsHighestUnlockedMap(campaign, progress);
    if (map < 0 || QuestsMapCompleted(campaign, progress, map) || q->current_maps[map].id == 0) return false;
    Rectangle image = {area.x + (area.width - 84.0f) * 0.5f, area.y, 84.0f, 84.0f};
    bool hover = !QuestsInputBlocked(q) && CheckCollisionPointRec(UiMouse(), image);
    QuestsDrawImageFit(q->current_maps[map], image, hover ? (Color){255, 236, 180, 255} : WHITE);
    UiDrawTextCentered(
        q->body,
        LocGetOr("NEXUS_HUB_FEATURED_CURRENT", "текущая карта"),
        (Rectangle){area.x, image.y + image.height + 12.0f, area.width, 0.0f},
        17.0f,
        QUESTS_TEXT
    );
    return QuestsReleasedOn(q, image);
}

static QuestButton QuestsDrawDialog(Quests *q, const Player *player) {
    QuestsDialog *d = &q->dialog;
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) d->opening_release = false;
    Rectangle root = UiRoot();
    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 150});
    Rectangle panel = UiRect(root, 0.5f, 0.5f, 760.0f, 356.0f);
    UiDrawTexture(q->popup, panel, WHITE);
    HeroesDrawRich(
        q->body,
        d->text,
        (Rectangle){panel.x + 80.0f, panel.y + 70.0f, panel.width - 160.0f, panel.height - 170.0f},
        24.0f,
        QUESTS_TEXT
    );

    Rectangle left = {panel.x + panel.width * 0.5f - 272.0f, panel.y + panel.height - 98.0f, 260.0f, 66.0f};
    Rectangle right = {panel.x + panel.width * 0.5f + 12.0f, left.y, 260.0f, 66.0f};
    Rectangle single = {panel.x + (panel.width - 260.0f) * 0.5f, left.y, 260.0f, 66.0f};
    bool interactive = !d->opening_release && !UiInputBlocked();
    QuestButton result = QUESTS_NONE;
    int event = d->event;

    if (d->kind == QUESTS_DIALOG_MESSAGE) {
        q->dialog_ok.label = LocGetOr("MAIN_MESSAGE_DIALOG_OK", "ОК");
        if (ButtonDrawEx(&q->dialog_ok, single, interactive)) QuestsDialogClose(q);
    } else if (d->kind == QUESTS_DIALOG_UNLOCK) {
        q->dialog_yes.label = LocGetOr("CONFIRM_DIALOG_YES", "Да");
        q->dialog_no.label = LocGetOr("CONFIRM_DIALOG_NO", "Нет");
        if (ButtonDrawEx(&q->dialog_yes, left, interactive)) {
            QuestsDialogClose(q);
            q->pending_event = event;
            result = QUESTS_UNLOCK;
        } else if (ButtonDrawEx(&q->dialog_no, right, interactive)) {
            QuestsDialogClose(q);
        }
    } else if (d->kind == QUESTS_DIALOG_COMMITTED) {
        q->dialog_yes.label = LocGetOr("NEXUS_COMMIT_MAP_EXPLANATION_CONTINUE_BUTTON", "Продолжить карту");
        q->dialog_no.label = LocGetOr("NEXUS_COMMIT_MAP_EXPLANATION_QUIT_BUTTON", "Покинуть карту");
        if (ButtonDrawEx(&q->dialog_yes, left, interactive)) {
            QuestsDialogClose(q);
            q->ladder_event = player->nexus_run.event_index;
        } else if (ButtonDrawEx(&q->dialog_no, right, interactive)) {
            QuestsDialogClose(q);
            q->pending_event = event;
            result = QUESTS_FORFEIT;
        }
    }

    Rectangle close = {panel.x + panel.width - 72.0f, panel.y - 10.0f, 61.0f, 61.0f};
    if (ButtonDrawEx(&q->dialog_close, close, interactive)) QuestsDialogClose(q);
    return result;
}

void QuestsOpenLadder(Quests *q, int event) {
    q->ladder_event = event;
}

QuestButton QuestsDraw(Quests *q, const Nexus *nexus, const Player *player, const Campaign *campaign) {
    QuestButton clicked = QUESTS_NONE;
    QuestsEnsureHubImages(q, nexus);
    QuestsEnsureCurrentMaps(q, campaign);
    bool dialog_open = q->dialog.kind != QUESTS_DIALOG_NONE;

    Rectangle root = UiRoot();

    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 170});

    Rectangle panel = UiRect(root, 0.5f, 0.6f, 940.0f, 597.0f);
    {
        Rectangle src = {0, 0, q->background.width, q->background.height};
        DrawTexturePro(q->background, src, panel, (Vector2){0, 0}, 0.0f, WHITE);
    }

    UiDrawTextCentered(
        q->font,
        LocGetOr("PVE_HUB_TITLE", "Задания"),
        (Rectangle){0.0f, panel.y + 38.0f, UiWidth(), 0.0f},
        42.0f,
        QUESTS_TEXT
    );

    QuestsCollectLiveEvents(q, nexus, player);
    if (q->ladder_event >= 0 && !NexusEventLive(NexusEventAt(nexus, q->ladder_event), NexusWeekdayToday()))
        q->ladder_event = -1;

    bool ladder = q->ladder_event >= 0;
    if (ladder) {
        if (QuestsDrawLadder(q, nexus, player, panel)) clicked = QUESTS_NEXUS;
    } else {
        QuestsDrawCarousel(q, nexus, player, panel);
    }
    bool ladder_opened = !ladder && q->ladder_event >= 0;

    Rectangle back_rect = {0};
    bool interactive = !QuestsInputBlocked(q);
    if (!ladder) {
        Rectangle dst = UiRect(panel, 0.5f, 0.88f, 211.0f, 95.0f);

        float caption_fs = 30.0f;
        UiDrawTextCentered(
            q->font,
            LocGetOr("NEXUS_HUB_FEATURED_CONTINUE", "Продолжить основное задание"),
            (Rectangle){0.0f, dst.y - caption_fs, UiWidth(), 0.0f},
            caption_fs,
            QUESTS_TEXT
        );

        if (ButtonDrawEx(&q->map, dst, interactive)) clicked = QUESTS_MAP;
        Rectangle current = {dst.x + dst.width + 40.0f, dst.y - 22.0f, 150.0f, 110.0f};
        if (QuestsDrawCurrentMap(q, campaign, &player->campaign_progress, current)) {
            GuiClickSoundPlay();
            clicked = QUESTS_MAP;
        }
        QuestsDrawKeyStrip(q, player, (Rectangle){panel.x + 72.0f, dst.y - 4.0f, dst.x - panel.x - 112.0f, 92.0f});
    } else {
        back_rect = (Rectangle){panel.x + 62.0f, panel.y + panel.height * 0.07f, 54.0f, 54.0f};
        if (ButtonDrawEx(&q->back, back_rect, interactive)) q->ladder_event = -1;
    }

    Rectangle close_rect;
    {
        float s = 61.0f;
        close_rect = (Rectangle){panel.x + panel.width - s * 1.25f, panel.y + panel.height * 0.07f, s, s};
        if (ButtonDrawEx(&q->esc, close_rect, interactive)) clicked = QUESTS_CLOSE;
    }

    Vector2 mouse = UiMouse();
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close_rect) &&
                       !CheckCollisionPointRec(mouse, back_rect) && !CheckCollisionPointRec(mouse, top_ui);
    if (UiInputBlocked() || dialog_open)
        q->scrim_armed = false;
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        q->scrim_armed = scrim_hover;

    if (!UiInputBlocked() && !dialog_open && !ladder_opened && clicked == QUESTS_NONE &&
        IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && q->scrim_armed && scrim_hover) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        if (q->ladder_event >= 0)
            q->ladder_event = -1;
        else
            clicked = QUESTS_CLOSE;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) q->scrim_armed = false;

    if (dialog_open) {
        QuestButton dialog_result = QuestsDrawDialog(q, player);
        if (dialog_result != QUESTS_NONE) clicked = dialog_result;
    }

    if (clicked == QUESTS_CLOSE || clicked == QUESTS_NEXUS) {
        q->ladder_event = -1;
        QuestsDialogClose(q);
    }
    return clicked;
}
