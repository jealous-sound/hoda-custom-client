#define PVP_HUB_COLUMN_GAP 24.0f
#define PVP_HUB_INSET 62.0f
#define PVP_HUB_TOP 96.0f
#define PVP_HUB_FOOTER_HEIGHT 30.0f
#define PVP_HUB_BUTTON_WIDTH 240.0f
#define PVP_HUB_BUTTON_HEIGHT 60.0f
#define PVP_HUB_ICON_SIZE 72.0f
#define PVP_HUB_DESCRIPTION_HEIGHT 150.0f

typedef enum { PVP_HUB_NONE = 0, PVP_HUB_CLOSE, PVP_HUB_LADDER, PVP_HUB_EVENT } PvpHubAction;

static const Color PVP_HUB_TEXT = {235, 232, 220, 255};
static const Color PVP_HUB_TEXT_DIM = {182, 178, 168, 255};
static const Color PVP_HUB_COLUMN_TINT = {0, 0, 0, 120};

typedef struct {
    Font font;
    Font body;
    Button close;
    Button ladder;
    Button event;
    int rank;
    bool scrim_armed;
} PvpHub;

void PvpHubInitialize(PvpHub *hub) {
    memset(hub, 0, sizeof(*hub));
    hub->font = GuiFont(FNT_COOL);
    hub->body = GuiFont(FNT_MENU);
    hub->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&hub->close, SND_UI_POPUP_CLOSE);
    const char *go = LocGetOr("PVP_HUB_GO_BUTTON_STRING", "Перейти");
    hub->ladder = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, go, hub->font, (Vector2){0.5f, 0.5f}, 0.32f);
    hub->event = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, go, hub->font, (Vector2){0.5f, 0.5f}, 0.32f);
}

void PvpHubOpen(PvpHub *hub, Player *p, long long now) {
    PvpEventSync(&p->pvp, now);
    static PvpRankEntry ranks[PVP_BOT_CAPACITY + 1];
    int count = PvpLeaderboardBuild(ranks, p->pvp.trophies, PvpDay(now));
    hub->rank = PvpLeaderboardRank(ranks, count);
    hub->scrim_armed = false;
}

static Rectangle PvpHubColumnBody(const PvpHub *hub, Rectangle column, const char *title, const char *description) {
    DrawRectangleRec(column, PVP_HUB_COLUMN_TINT);
    UiDrawTextCentered(
        hub->font, title, (Rectangle){column.x, column.y + 10.0f, column.width, 36.0f}, 28.0f, PVP_HUB_TEXT
    );
    Rectangle text = {column.x + 20.0f, column.y + 56.0f, column.width - 40.0f, PVP_HUB_DESCRIPTION_HEIGHT};
    HeroesDrawRich(hub->body, description, text, 19.0f, PVP_HUB_TEXT);
    return (Rectangle){text.x, text.y + text.height, text.width, column.height - text.height - 56.0f};
}

static bool PvpHubDrawGo(Button *button, Rectangle column, bool interactive) {
    Rectangle dst = {
        column.x + (column.width - PVP_HUB_BUTTON_WIDTH) * 0.5f,
        column.y + column.height - PVP_HUB_BUTTON_HEIGHT - 12.0f,
        PVP_HUB_BUTTON_WIDTH,
        PVP_HUB_BUTTON_HEIGHT,
    };
    return ButtonDrawEx(button, dst, interactive);
}

static void PvpHubDrawLadderStats(const PvpHub *hub, const PlayerPvp *pvp, Rectangle area) {
    Rectangle icon = {area.x, area.y, PVP_HUB_ICON_SIZE, PVP_HUB_ICON_SIZE};
    UiDrawTexture(GuiTex(GUI_TAB_TROPHY), icon, WHITE);
    float x = icon.x + icon.width + 12.0f;
    UiDrawTextLeftMiddle(
        hub->font,
        TextFormat("%d", pvp->trophies),
        (Rectangle){x, area.y, area.width - icon.width, 40.0f},
        34.0f,
        PVP_HUB_TEXT
    );
    UiDrawTextLeftMiddle(
        hub->body,
        TextFormat("%s #%d", LocGetOr("LEADERBOARD_RANK", "Ранг"), hub->rank),
        (Rectangle){x, area.y + 40.0f, area.width - icon.width, 28.0f},
        20.0f,
        PVP_HUB_TEXT_DIM
    );
}

static void PvpHubDrawEventColumn(const PvpHub *hub, const PlayerPvp *pvp, Rectangle area, long long now) {
    const PvpEvent *event = PvpEventForCycle(PvpEventCycle(now));
    Rectangle icon = {area.x, area.y, PVP_HUB_ICON_SIZE, PVP_HUB_ICON_SIZE};
    UiDrawTexture(GuiTex(GUI_PVP_BANNER), icon, WHITE);
    float x = icon.x + icon.width + 12.0f;
    QuestsDrawFit(
        hub->font,
        LocGet(event->title_key),
        (Rectangle){x, area.y, area.width - icon.width - 12.0f, 40.0f},
        24.0f,
        PVP_HUB_TEXT
    );
    char remaining[48], earned[64];
    QuestsShortTime(remaining, sizeof(remaining), PvpEventRemaining(now));
    LocFormat2(earned, sizeof(earned), "PVP_EVENT_BANNERS_EARNED", TextFormat("%d", pvp->event.banners), "");
    UiDrawTextLeftMiddle(
        hub->body,
        TextFormat("%s %s   %s", LocGetOr("PVP_EVENT_ENDS_IN", "оконч.:"), remaining, earned),
        (Rectangle){x, area.y + 40.0f, area.width - icon.width - 12.0f, 28.0f},
        20.0f,
        PVP_HUB_TEXT_DIM
    );
}

PvpHubAction PvpHubDraw(PvpHub *hub, const Player *p) {
    Rectangle root = UiRoot();
    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 170});
    Rectangle panel = UiRect(root, 0.5f, 0.6f, 940.0f, 597.0f);
    UiDrawTexture(GuiTex(GUI_QUESTS_BG), panel, WHITE);
    UiDrawTextCentered(
        hub->font,
        LocGetOr("PVP_HUB_TITLE", "Выберите режим PvP"),
        (Rectangle){0.0f, panel.y + 38.0f, UiWidth(), 0.0f},
        42.0f,
        PVP_HUB_TEXT
    );

    bool interactive = !UiInputBlocked();
    float width = (panel.width - PVP_HUB_INSET * 2.0f - PVP_HUB_COLUMN_GAP) * 0.5f;
    float height = panel.height - PVP_HUB_TOP - PVP_HUB_FOOTER_HEIGHT - 56.0f;
    Rectangle left = {panel.x + PVP_HUB_INSET, panel.y + PVP_HUB_TOP, width, height};
    Rectangle right = {left.x + width + PVP_HUB_COLUMN_GAP, left.y, width, height};

    PvpHubAction action = PVP_HUB_NONE;
    Rectangle event_area = PvpHubColumnBody(
        hub,
        left,
        LocGetOr("PVP_HUB_EVENT_SIDE_TITLE", "Сегодняшнее событие"),
        LocGetOr("PVP_HUB_EVENT_SIDE_DESCRIPTION", "")
    );
    PvpHubDrawEventColumn(hub, &p->pvp, event_area, ServerNow());
    if (PvpHubDrawGo(&hub->event, left, interactive)) action = PVP_HUB_EVENT;

    Rectangle ladder_area = PvpHubColumnBody(
        hub,
        right,
        LocGetOr("PVP_HUB_PROVING_GROUND_SIDE_TITLE", "Арена Испытаний"),
        LocGetOr("PVP_HUB_PROVING_GROUND_SIDE_DESCRIPTION", "")
    );
    PvpHubDrawLadderStats(hub, &p->pvp, ladder_area);
    if (PvpHubDrawGo(&hub->ladder, right, interactive)) action = PVP_HUB_LADDER;

    UiDrawTextCentered(
        hub->body,
        LocGetOr("PVP_HUB_STAMINA", "Используйте выносливость в PvP"),
        (Rectangle){panel.x, left.y + left.height + 6.0f, panel.width, PVP_HUB_FOOTER_HEIGHT},
        20.0f,
        PVP_HUB_TEXT_DIM
    );

    float close_size = 61.0f;
    Rectangle close = {
        panel.x + panel.width - close_size * 1.25f, panel.y + panel.height * 0.07f, close_size, close_size
    };
    if (ButtonDrawEx(&hub->close, close, interactive)) action = PVP_HUB_CLOSE;

    Vector2 mouse = UiMouse();
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close) &&
                       !CheckCollisionPointRec(mouse, top_ui);
    if (!interactive) {
        hub->scrim_armed = false;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        hub->scrim_armed = scrim_hover;
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (hub->scrim_armed && scrim_hover && action == PVP_HUB_NONE) {
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            action = PVP_HUB_CLOSE;
        }
        hub->scrim_armed = false;
    }
    return action;
}
