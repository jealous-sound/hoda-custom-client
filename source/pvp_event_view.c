#define PVP_EVENT_VIEW_INSET 62.0f
#define PVP_EVENT_VIEW_TOP 92.0f
#define PVP_EVENT_VIEW_RANK_WIDTH 250.0f
#define PVP_EVENT_VIEW_GAP 14.0f
#define PVP_EVENT_VIEW_TEXT_HEIGHT 72.0f
#define PVP_EVENT_VIEW_STATUS_HEIGHT 50.0f
#define PVP_EVENT_VIEW_ROW_HEIGHT 44.0f
#define PVP_EVENT_VIEW_ICON_SIZE 34.0f
#define PVP_EVENT_VIEW_BUTTON_WIDTH 256.0f
#define PVP_EVENT_VIEW_BUTTON_HEIGHT 64.0f

typedef enum { PVP_EVENT_VIEW_NONE = 0, PVP_EVENT_VIEW_CLOSE, PVP_EVENT_VIEW_ENTER } PvpEventViewAction;

static const Color PVP_EVENT_VIEW_TEXT = {235, 232, 220, 255};
static const Color PVP_EVENT_VIEW_DIM = {182, 178, 168, 255};
static const Color PVP_EVENT_VIEW_DONE = {132, 214, 112, 255};
static const Color PVP_EVENT_VIEW_TINT = {0, 0, 0, 120};

typedef struct {
    Font font;
    Font body;
    Button close;
    Button enter;
    bool scrim_armed;
} PvpEventView;

void PvpEventViewInitialize(PvpEventView *v) {
    memset(v, 0, sizeof(*v));
    v->font = GuiFont(FNT_COOL);
    v->body = GuiFont(FNT_MENU);
    v->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&v->close, SND_UI_POPUP_CLOSE);
    v->enter = ButtonLoadText(
        GUI_BTN_GOLD,
        GUI_BTN_GOLD_ON,
        LocGetOr("PVP_EVENT_ENTER_TODAYS_EVENT", "К событию дня"),
        v->font,
        (Vector2){0.5f, 0.5f},
        0.3f
    );
}

static void PvpEventViewDrawReward(const PvpEventView *v, int gold, int gems, Rectangle area, Color color) {
    bool premium = gems > 0;
    Rectangle icon = {
        area.x + area.width - PVP_EVENT_VIEW_ICON_SIZE,
        area.y + (area.height - PVP_EVENT_VIEW_ICON_SIZE) * 0.5f,
        PVP_EVENT_VIEW_ICON_SIZE,
        PVP_EVENT_VIEW_ICON_SIZE,
    };
    UiDrawTexture(GuiTex(premium ? GUI_MISSION_GEM : GUI_MISSION_GOLD), icon, WHITE);
    Rectangle amount = {area.x, area.y, area.width - PVP_EVENT_VIEW_ICON_SIZE - 6.0f, area.height};
    UiDrawTextRight(v->font, TextFormat("+%d", premium ? gems : gold), amount, 22.0f, color);
}

static void PvpEventViewDrawStatus(const PvpEventView *v, const PlayerPvp *pvp, Rectangle area, long long now) {
    DrawRectangleRec(area, PVP_EVENT_VIEW_TINT);
    Rectangle icon = {area.x + 8.0f, area.y + 3.0f, 44.0f, 44.0f};
    UiDrawTexture(GuiTex(GUI_PVP_BANNER), icon, WHITE);
    float third = area.width / 3.0f;
    UiDrawTextLeftMiddle(
        v->font,
        TextFormat("%d", pvp->event.banners),
        (Rectangle){icon.x + icon.width + 8.0f, area.y, third - 60.0f, area.height},
        26.0f,
        PVP_EVENT_VIEW_TEXT
    );
    char rank[64], remaining[48];
    LocFormat2(rank, sizeof(rank), "PVP_EVENT_RANK", TextFormat("%d", PvpEventCurrentRank(pvp, now)), "");
    UiDrawTextCentered(
        v->body, rank, (Rectangle){area.x + third, area.y, third, area.height}, 22.0f, PVP_EVENT_VIEW_TEXT
    );
    QuestsShortTime(remaining, sizeof(remaining), PvpEventRemaining(now));
    UiDrawTextRight(
        v->body,
        TextFormat("%s %s", LocGetOr("PVP_EVENT_ENDS_IN", "оконч.:"), remaining),
        (Rectangle){area.x, area.y, area.width - 12.0f, area.height},
        20.0f,
        PVP_EVENT_VIEW_DIM
    );
}

static void PvpEventViewDrawMilestones(const PvpEventView *v, const PlayerPvp *pvp, Rectangle area) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    UiDrawTextLeftMiddle(
        v->font,
        LocGetOr("PVP_EVENT_ACHIEVEMENTS", "Призы за знамена"),
        (Rectangle){area.x + 8.0f, area.y, area.width, 34.0f},
        22.0f,
        PVP_EVENT_VIEW_TEXT
    );
    float row_height = (area.height - 36.0f) / rules->milestone_count;
    for (int i = 0; i < rules->milestone_count; i++) {
        const PvpEventMilestone *milestone = &rules->milestones[i];
        Rectangle row = {area.x, area.y + 36.0f + i * row_height, area.width, row_height - 3.0f};
        DrawRectangleRec(row, PVP_EVENT_VIEW_TINT);
        bool paid = pvp->event.milestones_paid > i;
        Color color = paid ? PVP_EVENT_VIEW_DONE : PVP_EVENT_VIEW_TEXT;
        Rectangle icon = {row.x + 8.0f, row.y + (row.height - 28.0f) * 0.5f, 28.0f, 28.0f};
        UiDrawTexture(GuiTex(GUI_PVP_BANNER), icon, WHITE);
        UiDrawTextLeftMiddle(
            v->body,
            TextFormat("%d", milestone->banners),
            (Rectangle){icon.x + icon.width + 8.0f, row.y, 120.0f, row.height},
            20.0f,
            color
        );
        Rectangle reward = {row.x, row.y, row.width - 10.0f, row.height};
        PvpEventViewDrawReward(v, milestone->gold, milestone->gems, reward, color);
    }
}

static void PvpEventViewDrawRanks(const PvpEventView *v, Rectangle area) {
    const PvpEventRules *rules = &PvpTableEnsure()->events;
    DrawRectangleRec(area, PVP_EVENT_VIEW_TINT);
    UiDrawTextCentered(
        v->font,
        LocGetOr("PVP_EVENT_RANK_REWARDS", "Награды за ранг"),
        (Rectangle){area.x, area.y + 4.0f, area.width, 36.0f},
        21.0f,
        PVP_EVENT_VIEW_TEXT
    );
    for (int i = 0; i < rules->rank_reward_count; i++) {
        const PvpEventRankReward *reward = &rules->rank_rewards[i];
        Rectangle row = {
            area.x + 10.0f,
            area.y + 46.0f + i * PVP_EVENT_VIEW_ROW_HEIGHT,
            area.width - 20.0f,
            PVP_EVENT_VIEW_ROW_HEIGHT
        };
        const char *bracket = reward->first == reward->last ? TextFormat("#%d", reward->first)
                                                            : TextFormat("#%d-%d", reward->first, reward->last);
        UiDrawTextLeftMiddle(v->body, bracket, row, 20.0f, PVP_EVENT_VIEW_DIM);
        PvpEventViewDrawReward(v, reward->gold, reward->gems, row, PVP_EVENT_VIEW_TEXT);
    }
}

PvpEventViewAction PvpEventViewDraw(PvpEventView *v, const Player *p) {
    long long now = (long long)time(NULL);
    const PvpEvent *event = PvpEventForCycle(PvpEventCycle(now));
    Rectangle root = UiRoot();
    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 170});
    Rectangle panel = UiRect(root, 0.5f, 0.6f, 940.0f, 597.0f);
    UiDrawTexture(GuiTex(GUI_QUESTS_BG), panel, WHITE);
    UiDrawTextCentered(
        v->font,
        LocGet(event->title_key),
        (Rectangle){0.0f, panel.y + 38.0f, UiWidth(), 0.0f},
        38.0f,
        PVP_EVENT_VIEW_TEXT
    );

    float width = panel.width - PVP_EVENT_VIEW_INSET * 2.0f;
    float top = panel.y + PVP_EVENT_VIEW_TOP;
    float bottom = panel.y + panel.height - 40.0f;
    Rectangle ranks = {
        panel.x + panel.width - PVP_EVENT_VIEW_INSET - PVP_EVENT_VIEW_RANK_WIDTH,
        top,
        PVP_EVENT_VIEW_RANK_WIDTH,
        bottom - PVP_EVENT_VIEW_BUTTON_HEIGHT - PVP_EVENT_VIEW_GAP - top,
    };
    float left_width = width - PVP_EVENT_VIEW_RANK_WIDTH - PVP_EVENT_VIEW_GAP;
    Rectangle text = {panel.x + PVP_EVENT_VIEW_INSET + 12.0f, top, left_width - 24.0f, PVP_EVENT_VIEW_TEXT_HEIGHT};
    Rectangle status = {
        panel.x + PVP_EVENT_VIEW_INSET, text.y + text.height + 4.0f, left_width, PVP_EVENT_VIEW_STATUS_HEIGHT
    };
    float milestones_top = status.y + status.height + 6.0f;
    Rectangle milestones = {status.x, milestones_top, left_width, bottom - milestones_top};

    HeroesDrawRich(v->body, NexusGameChangerText(&event->changer), text, 20.0f, PVP_EVENT_VIEW_TEXT);
    PvpEventViewDrawStatus(v, &p->pvp, status, now);
    PvpEventViewDrawMilestones(v, &p->pvp, milestones);
    PvpEventViewDrawRanks(v, ranks);

    bool interactive = !UiInputBlocked();
    PvpEventViewAction action = PVP_EVENT_VIEW_NONE;
    Rectangle enter = {
        ranks.x + (ranks.width - PVP_EVENT_VIEW_BUTTON_WIDTH) * 0.5f,
        bottom - PVP_EVENT_VIEW_BUTTON_HEIGHT,
        PVP_EVENT_VIEW_BUTTON_WIDTH,
        PVP_EVENT_VIEW_BUTTON_HEIGHT,
    };
    if (ButtonDrawEx(&v->enter, enter, interactive)) action = PVP_EVENT_VIEW_ENTER;

    float close_size = 61.0f;
    Rectangle close = {
        panel.x + panel.width - close_size * 1.25f, panel.y + panel.height * 0.07f, close_size, close_size
    };
    if (ButtonDrawEx(&v->close, close, interactive)) action = PVP_EVENT_VIEW_CLOSE;

    Vector2 mouse = UiMouse();
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close) &&
                       !CheckCollisionPointRec(mouse, top_ui);
    if (!interactive) {
        v->scrim_armed = false;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        v->scrim_armed = scrim_hover;
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (v->scrim_armed && scrim_hover && action == PVP_EVENT_VIEW_NONE) {
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            action = PVP_EVENT_VIEW_CLOSE;
        }
        v->scrim_armed = false;
    }
    return action;
}
