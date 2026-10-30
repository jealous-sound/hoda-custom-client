
static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
);

typedef enum { QUESTS_NONE = 0, QUESTS_MAP, QUESTS_NEXUS, QUESTS_CLOSE } QuestButton;

#define QUESTS_LANE_INSET 92.0f
#define QUESTS_CELL_STRIDE 244.0f
#define QUESTS_CELL_GUTTER 12.0f
#define QUESTS_CELL_HEIGHT 226.0f
#define QUESTS_SCROLL_STEP (QUESTS_CELL_STRIDE * 2.0f)

typedef struct {
    Texture2D background;
    Texture2D frame;
    Texture2D titlebar;
    Texture2D divider;
    Texture2D arrow;
    Texture2D energy;
    Texture2D key_icons[NEXUS_KEY_COUNT];
    Font font;
    Font body;
    Button map;
    Button esc;
    Button back;
    bool scrim_armed;

    int live[NEXUS_MAX_EVENTS];
    int live_count;
    float scroll;
    bool dragging;
    bool drag_scrolled;
    float drag_start_x;
    float scroll_start;

    int ladder_event;
    int selected_event;
    int selected_difficulty;
} Quests;

void QuestsInitialize(Quests *q) {
    memset(q, 0, sizeof(*q));
    q->background = GuiTex(GUI_QUESTS_BG);
    q->frame = GuiTex(GUI_NEXUS_EVENT_FRAME);
    q->titlebar = GuiTex(GUI_NEXUS_EVENT_TITLEBAR);
    q->divider = GuiTex(GUI_NEXUS_DIVIDER);
    q->arrow = GuiTex(GUI_STORE_SCROLL_ARROW);
    q->energy = GuiTex(GUI_HUD_ENERGY);
    for (int i = 0; i < NEXUS_KEY_COUNT; i++) q->key_icons[i] = GuiTex(GUI_NEXUS_KEY_1 + i);
    q->font = GuiFont(FNT_COOL);
    q->body = GuiFont(FNT_MENU);
    q->map = ButtonLoad(GUI_BTN_MAINQUEST, GUI_BTN_MAINQUEST_ON);
    q->esc = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    q->back = ButtonLoad(GUI_STORE_BACK, GUI_STORE_BACK);
    ButtonSetClickSound(&q->esc, SND_UI_POPUP_CLOSE);
    ButtonSetClickSound(&q->back, SND_UI_POPUP_CLOSE);
    q->ladder_event = -1;
    q->selected_event = -1;
    q->selected_difficulty = -1;
}

int QuestsSelectedNexusEvent(const Quests *q) {
    return q->selected_event;
}

int QuestsSelectedNexusDifficulty(const Quests *q) {
    return q->selected_difficulty;
}

static const char *QuestsKindLabel(const char *kind) {
    if (!strcmp(kind, "chase")) return "Погоня";
    if (!strcmp(kind, "boss")) return "Босс";
    if (!strcmp(kind, "gauntlet")) return "Испытание";
    if (!strcmp(kind, "raid")) return "Рейд";
    if (!strcmp(kind, "puzzle")) return "Головоломка";
    if (!strcmp(kind, "daily")) return "Ежедневное";
    if (!strcmp(kind, "map")) return "Карта";
    return "Задание";
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

void QuestsClose(Quests *q) {
    q->ladder_event = -1;
}

static bool QuestsEventCommitted(const Player *player, int event) {
    return player->nexus_run.committed && player->nexus_run.event_index == event;
}

static bool QuestsEventUnlocked(const Nexus *nexus, const Player *player, int event) {
    const NexusEvent *e = NexusEventAt(nexus, event);
    if (!e) return false;
    if (NexusEventUnlocked(nexus, player, event)) return true;
    return PlayerKeyCount(player, e->unlock_key) >= e->unlock_key_qty;
}

static bool QuestsEventSelectable(const Nexus *nexus, const Player *player, int event) {
    if (NexusRunBlocking(nexus, player)) return QuestsEventCommitted(player, event);
    return QuestsEventUnlocked(nexus, player, event);
}

static void QuestsCollectLiveEvents(Quests *q, const Nexus *nexus, const Player *player) {
    int weekday = NexusWeekdayToday();
    q->live_count = 0;
    for (int i = 0; i < NexusEventCount(nexus); i++) {
        const NexusEvent *e = NexusEventAt(nexus, i);
        if (!NexusEventLive(e, weekday)) continue;
        int at = q->live_count++;
        while (at > 0) {
            const NexusEvent *prev = NexusEventAt(nexus, q->live[at - 1]);
            bool prev_committed = QuestsEventCommitted(player, q->live[at - 1]);
            bool committed = QuestsEventCommitted(player, i);
            if (prev_committed != committed ? prev_committed : strcmp(prev->id, e->id) <= 0) break;
            q->live[at] = q->live[at - 1];
            at--;
        }
        q->live[at] = i;
    }
}

static bool QuestsInputBlocked(const Quests *q) {
    return UiInputBlocked() || q->drag_scrolled;
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

static void QuestsDrawActionPlate(const Quests *q, Rectangle rect, const char *label, bool gold, bool enabled) {
    bool hover = enabled && !QuestsInputBlocked(q) && CheckCollisionPointRec(UiMouse(), rect);
    GuiId art = gold ? (hover ? GUI_BTN_GOLD_ON : GUI_BTN_GOLD) : (hover ? GUI_BTN_GREY_ON : GUI_BTN_GREY);
    Color tint = enabled ? WHITE : (Color){118, 118, 118, 210};
    UiDrawTexture(GuiTex(art), rect, tint);
    QuestsDrawFit(
        q->font,
        label,
        (Rectangle){rect.x + 12.0f, rect.y, rect.width - 24.0f, rect.height},
        rect.height * 0.42f,
        enabled ? (Color){235, 232, 220, 255} : (Color){166, 163, 155, 255}
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

static float QuestsMaxScroll(const Quests *q, Rectangle viewport) {
    float content = q->live_count * QUESTS_CELL_STRIDE;
    return content > viewport.width ? content - viewport.width : 0.0f;
}

static void QuestsUpdateScrollInput(Quests *q, Rectangle viewport) {
    Vector2 mouse = UiMouse();
    if (!UiInputBlocked()) {
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

static void QuestsDrawEventCell(const Quests *q, const Nexus *nexus, const Player *player, int event, Rectangle cell) {
    const NexusEvent *e = NexusEventAt(nexus, event);
    bool committed = QuestsEventCommitted(player, event);
    bool unlocked = QuestsEventUnlocked(nexus, player, event);
    bool selectable = QuestsEventSelectable(nexus, player, event);

    Color dim = {162, 159, 152, 255};
    QuestsDrawFrame(q->frame, cell, 4.0f, selectable ? WHITE : (Color){150, 150, 150, 255});
    if (committed) DrawRectangleLinesEx(cell, 3.0f, e->highlight);

    Rectangle title = {cell.x + 5.0f, cell.y + 5.0f, cell.width - 10.0f, 46.0f};
    DrawTexturePro(
        q->titlebar,
        (Rectangle){0.0f, 140.0f, (float)q->titlebar.width, 232.0f},
        title,
        (Vector2){0},
        0.0f,
        e->highlight
    );
    QuestsDrawFit(q->font, QuestsKindLabel(e->kind), title, 23.0f, (Color){246, 244, 236, 255});

    CampaignDrawWrapped(
        q->body,
        e->name,
        (Vector2){cell.x + 16.0f, cell.y + 58.0f},
        18.0f,
        1.0f,
        3.0f,
        cell.width - 32.0f,
        selectable ? (Color){236, 232, 220, 255} : dim
    );

    float foot = cell.y + cell.height;
    UiDrawTexture(q->divider, (Rectangle){cell.x + 14.0f, foot - 102.0f, cell.width - 28.0f, 10.0f}, e->highlight);

    Rectangle band = {cell.x + 20.0f, foot - 86.0f, cell.width - 40.0f, 32.0f};
    if (e->unlock_key != 0) {
        int key = Clamp(e->unlock_key - NEXUS_KEY_FIRST_ID, 0, NEXUS_KEY_COUNT - 1);
        UiDrawTexture(
            q->key_icons[key], (Rectangle){band.x, band.y, 32.0f, 32.0f}, unlocked ? WHITE : (Color){140, 140, 140, 220}
        );
        UiDrawTextLeftMiddle(
            q->font,
            TextFormat("%d / %d", PlayerKeyCount(player, e->unlock_key), e->unlock_key_qty),
            (Rectangle){band.x + 42.0f, band.y, band.width - 42.0f, band.height},
            23.0f,
            unlocked ? (Color){245, 244, 238, 255} : (Color){214, 106, 96, 255}
        );
    } else {
        UiDrawTextLeftMiddle(q->body, TextFormat("Карт: %d", e->difficulty_count), band, 19.0f, dim);
        UiDrawTextRight(q->body, TextFormat("Узлов: %d", e->node_count), band, 19.0f, dim);
    }

    const char *label = LocGetOr("PVE_HUB_EVENT_ENTER_BUTTON", "Войти");
    if (committed)
        label = LocGetOr("PVE_HUB_NEXUS_EVENT_CONTINUE_BUTTON", "Продолжить");
    else if (!unlocked)
        label = LocGetOr("BUTTON_LOCKED", "Закрыто");
    else if (e->unlock_key != 0)
        label = LocGetOr("BUTTON_UNLOCKED", "Открыто");
    QuestsDrawActionPlate(
        q, (Rectangle){cell.x + 16.0f, foot - 48.0f, cell.width - 32.0f, 40.0f}, label, committed, selectable
    );
}

static bool QuestsDrawCarousel(Quests *q, const Nexus *nexus, const Player *player, Rectangle panel) {
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
            LocGetOr("PVE_HUB_NEXUS_TIME_EXPIRED", "Время вышло!"),
            (Rectangle){viewport.x, viewport.y, viewport.width, viewport.height},
            28.0f,
            (Color){178, 178, 178, 255}
        );
        return false;
    }

    bool opened = false;
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
        QuestsDrawEventCell(q, nexus, player, q->live[i], cell);
        Rectangle hit = GetCollisionRec(cell, viewport);
        if (!opened && QuestsEventSelectable(nexus, player, q->live[i]) && QuestsReleasedOn(q, hit)) {
            q->ladder_event = q->live[i];
            GuiSoundPlay(SND_UI_TAB);
            opened = true;
        }
    }
    EndScissorMode();
    return opened;
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
        QuestsKindLabel(e->kind),
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
        bool committed = QuestsEventCommitted(player, q->ladder_event) && player->nexus_run.difficulty_index == i;
        bool selectable =
            NexusRunBlocking(nexus, player) ? committed : QuestsEventUnlocked(nexus, player, q->ladder_event);
        Color label_color = selectable ? (Color){240, 236, 224, 255} : (Color){164, 161, 154, 255};
        QuestsDrawFrame(q->frame, row, 4.0f, selectable ? WHITE : (Color){150, 150, 150, 255});
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
        QuestsDrawActionPlate(q, plate, label, committed, selectable);
        if (!entered && selectable && QuestsReleasedOn(q, row)) {
            q->selected_event = q->ladder_event;
            q->selected_difficulty = i;
            GuiSoundPlay(SND_PVE_NODE_SELECTED);
            entered = true;
        }
    }
    return entered;
}

QuestButton QuestsDraw(Quests *q, const Nexus *nexus, const Player *player) {
    QuestButton clicked = QUESTS_NONE;

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
        (Color){235, 232, 220, 255}
    );

    QuestsCollectLiveEvents(q, nexus, player);
    if (q->ladder_event >= 0 && !NexusEventLive(NexusEventAt(nexus, q->ladder_event), NexusWeekdayToday()))
        q->ladder_event = -1;

    bool ladder = q->ladder_event >= 0;
    if (ladder) {
        if (QuestsDrawLadder(q, nexus, player, panel)) clicked = QUESTS_NEXUS;
    } else if (QuestsDrawCarousel(q, nexus, player, panel)) {
        ladder = true;
    }

    Rectangle back_rect = {0};
    if (!ladder) {
        Rectangle dst = UiRect(panel, 0.5f, 0.88f, 211.0f, 95.0f);

        float caption_fs = 30.0f;
        UiDrawTextCentered(
            q->font,
            "Продолжить основное задание",
            (Rectangle){0.0f, dst.y - caption_fs, UiWidth(), 0.0f},
            caption_fs,
            (Color){235, 232, 220, 255}
        );

        if (ButtonDraw(&q->map, dst)) clicked = QUESTS_MAP;
    } else {
        back_rect = (Rectangle){panel.x + 62.0f, panel.y + panel.height * 0.07f, 54.0f, 54.0f};
        if (ButtonDrawEx(&q->back, back_rect, !QuestsInputBlocked(q))) q->ladder_event = -1;
    }

    Rectangle close_rect;
    {
        float s = 61.0f;
        close_rect = (Rectangle){panel.x + panel.width - s * 1.25f, panel.y + panel.height * 0.07f, s, s};
        if (ButtonDraw(&q->esc, close_rect)) clicked = QUESTS_CLOSE;
    }

    Vector2 mouse = UiMouse();
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close_rect) &&
                       !CheckCollisionPointRec(mouse, back_rect) && !CheckCollisionPointRec(mouse, top_ui);
    if (UiInputBlocked())
        q->scrim_armed = false;
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        q->scrim_armed = scrim_hover;

    if (!UiInputBlocked() && clicked == QUESTS_NONE && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && q->scrim_armed &&
        scrim_hover) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        if (q->ladder_event >= 0)
            q->ladder_event = -1;
        else
            clicked = QUESTS_CLOSE;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) q->scrim_armed = false;

    if (clicked == QUESTS_CLOSE || clicked == QUESTS_NEXUS) q->ladder_event = -1;
    return clicked;
}
