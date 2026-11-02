#define GOALS_WHEEL_STEP 70.0f
#define GOALS_DRAG_THRESHOLD 13.0f
#define GOALS_RANK_ROW_HEIGHT 58.0f
#define GOALS_BATTLE_ROW_HEIGHT 82.0f
#define GOALS_ROW_GAP 6.0f
#define GOALS_LIST_TOP_INSET 8.0f
#define GOALS_HEADER_HEIGHT 34.0f
#define GOALS_PLAYER_ROW_HEIGHT 61.0f
#define GOALS_TROPHY_ICON_SIZE 40.0f
#define GOALS_BUTTON_WIDTH 124.0f
#define GOALS_BUTTON_HEIGHT 48.0f
#define GOALS_RANK_COLUMN_WIDTH 96.0f
#define GOALS_NAME_INSET 20.0f
#define GOALS_TROPHY_COLUMN_WIDTH 150.0f
#define GOALS_TEXT_SIZE 256
#define GOALS_MINUTE 60LL
#define GOALS_HOUR 3600LL

typedef enum { GOALS_TAB_TROPHIES, GOALS_TAB_MISSIONS, GOALS_TAB_EVENTS, GOALS_TAB_BATTLES, GOALS_TAB_COUNT } GoalsTab;

typedef enum { GOALS_NONE = 0, GOALS_FIGHT_BOT, GOALS_REVENGE, GOALS_CLAIM_EVENT } GoalsAction;

static const Color GOALS_TEXT = {235, 232, 220, 255};
static const Color GOALS_TEXT_DIM = {182, 178, 168, 255};
static const Color GOALS_PLAYER_TEXT = {36, 22, 8, 255};
static const Color GOALS_ROW_TINT = {0, 0, 0, 120};
static const Color GOALS_GAIN = {132, 214, 112, 255};
static const Color GOALS_LOSS = {226, 104, 92, 255};

typedef struct {
    float offset;
    bool dragging;
    bool drag_scrolled;
    float drag_start_y;
    float start;
} GoalsScroll;

typedef struct {
    Font font;
    Font body;
    Button tabs[GOALS_TAB_COUNT];
    Button fight;
    Button claim;
    PvpRankEntry ranks[PVP_BOT_CAPACITY + 1];
    int rank_count;
    int player_rank;
    int selected;
} GoalsTabs;

void GoalsTabsInitialize(GoalsTabs *t) {
    memset(t, 0, sizeof(*t));
    t->font = GuiFont(FNT_COOL);
    t->body = GuiFont(FNT_MENU);
    Vector2 centre = {0.5f, 0.5f};
    t->tabs[GOALS_TAB_TROPHIES] = ButtonLoad(GUI_GOALS_TAB_FEATURED, GUI_GOALS_TAB_FEATURED_ON);
    t->tabs[GOALS_TAB_MISSIONS] = ButtonLoadText(
        GUI_GOALS_TAB, GUI_GOALS_TAB_ON, LocGetOr("LEADERBOARD_MISSIONS_BUTTON", "Достижения"), t->font, centre, 0.36f
    );
    t->tabs[GOALS_TAB_EVENTS] = ButtonLoadText(
        GUI_GOALS_TAB, GUI_GOALS_TAB_ON, LocGetOr("LEADERBOARD_MYEVENTS_BUTTON", "Мои события"), t->font, centre, 0.36f
    );
    t->tabs[GOALS_TAB_BATTLES] = ButtonLoadText(
        GUI_GOALS_TAB, GUI_GOALS_TAB_ON, LocGetOr("LEADERBOARD_MYBATTLES_BUTTON", "Бои"), t->font, centre, 0.36f
    );
    for (int tab = 0; tab < GOALS_TAB_COUNT; tab++) ButtonSetClickSound(&t->tabs[tab], SND_UI_TAB);
    t->fight = ButtonLoad(GUI_PVP_BUTTON_SHORT, GUI_PVP_BUTTON_SHORT_ON);
    t->claim = ButtonLoadText(
        GUI_MISSION_CLAIM, GUI_MISSION_CLAIM_ON, LocGetOr("LEADERBOARD_MISSIONS_CLAIM", "Получ."), t->font, centre, 0.5f
    );
    ButtonSetClickSound(&t->claim, SND_MISSION_COLLECT);
}

void GoalsRefreshLeaderboard(GoalsTabs *t, const Player *p, long long now) {
    t->rank_count = PvpLeaderboardBuild(t->ranks, p->pvp.trophies, PvpDay(now));
    t->player_rank = PvpLeaderboardRank(t->ranks, t->rank_count);
}

void GoalsScrollReset(GoalsScroll *scroll) {
    *scroll = (GoalsScroll){0};
}

void GoalsScrollUpdate(GoalsScroll *scroll, Rectangle list, float content_height) {
    Vector2 mouse = UiMouse();
    if (!UiInputBlocked()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, list)) {
            scroll->dragging = true;
            scroll->drag_start_y = mouse.y;
            scroll->start = scroll->offset;
        }
        if (scroll->dragging) {
            float delta = mouse.y - scroll->drag_start_y;
            if (fabsf(delta) > GOALS_DRAG_THRESHOLD) scroll->drag_scrolled = true;
            if (scroll->drag_scrolled) scroll->offset = scroll->start - delta;
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) scroll->dragging = false;
        } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            scroll->drag_scrolled = false;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && CheckCollisionPointRec(mouse, list)) scroll->offset -= wheel * GOALS_WHEEL_STEP;
    }
    float limit = content_height > list.height ? content_height - list.height : 0.0f;
    scroll->offset = Clamp(scroll->offset, 0.0f, limit);
}

static void GoalsBeginClip(Rectangle list) {
    float scale = UiScale();
    BeginScissorMode(
        (int)(list.x * scale), (int)(list.y * scale), (int)(list.width * scale), (int)(list.height * scale)
    );
}

void GoalsDrawEmpty(const GoalsTabs *t, const char *text, Rectangle area) {
    UiDrawTextCentered(t->body, text, area, 24.0f, GOALS_TEXT_DIM);
}

GoalsTab GoalsDrawTabBar(GoalsTabs *t, GoalsTab selected, Rectangle bar, bool interactive) {
    static const float widths[GOALS_TAB_COUNT] = {81.25f, 235.625f, 235.625f, 235.625f};
    static const float gaps[GOALS_TAB_COUNT] = {12.75f, 9.375f, 8.375f, 0.0f};
    float x = bar.x;
    GoalsTab pressed = selected;
    for (int tab = 0; tab < GOALS_TAB_COUNT; tab++) {
        Rectangle slot = {x, bar.y, widths[tab], bar.height};
        if (tab == (int)selected)
            ButtonDrawSelected(&t->tabs[tab], slot);
        else if (ButtonDrawEx(&t->tabs[tab], slot, interactive))
            pressed = (GoalsTab)tab;
        if (tab == GOALS_TAB_TROPHIES) {
            float inset = (slot.width - bar.height) * 0.5f;
            UiDrawTexture(GuiTex(GUI_TAB_TROPHY), (Rectangle){slot.x + inset, slot.y, bar.height, bar.height}, WHITE);
        }
        x += widths[tab] + gaps[tab];
        if (tab + 1 < GOALS_TAB_COUNT)
            UiDrawTexture(
                GuiTex(GUI_GOALS_TAB_SPACER),
                (Rectangle){x - gaps[tab], bar.y + 6.0f, 11.925f, bar.height - 12.0f},
                WHITE
            );
    }
    return pressed;
}

static void GoalsDrawRankCells(
    const GoalsTabs *t, Rectangle row, int rank, const char *name, int trophies, float right_inset, Color color
) {
    UiDrawTextCentered(
        t->font, TextFormat("%d", rank), (Rectangle){row.x, row.y, GOALS_RANK_COLUMN_WIDTH, row.height}, 26.0f, color
    );
    Rectangle trophy = {
        row.x + row.width - right_inset - GOALS_TROPHY_ICON_SIZE,
        row.y + (row.height - GOALS_TROPHY_ICON_SIZE) * 0.5f,
        GOALS_TROPHY_ICON_SIZE,
        GOALS_TROPHY_ICON_SIZE,
    };
    UiDrawTexture(GuiTex(GUI_TAB_TROPHY), trophy, WHITE);
    float name_x = row.x + GOALS_RANK_COLUMN_WIDTH + GOALS_NAME_INSET;
    Rectangle trophies_cell = {
        trophy.x - GOALS_TROPHY_COLUMN_WIDTH - 8.0f, row.y, GOALS_TROPHY_COLUMN_WIDTH, row.height
    };
    UiDrawTextLeftMiddle(t->body, name, (Rectangle){name_x, row.y, trophies_cell.x - name_x, row.height}, 24.0f, color);
    UiDrawTextRight(t->font, TextFormat("%d", trophies), trophies_cell, 26.0f, color);
}

static void GoalsDrawRankHeader(const GoalsTabs *t, Rectangle row) {
    UiDrawTextCentered(
        t->body,
        LocGetOr("LEADERBOARD_RANK", "Ранг"),
        (Rectangle){row.x, row.y, GOALS_RANK_COLUMN_WIDTH, row.height},
        19.0f,
        GOALS_TEXT_DIM
    );
    UiDrawTextLeftMiddle(
        t->body,
        LocGetOr("LEADERBOARD_NAME", "Имя"),
        (Rectangle){row.x + GOALS_RANK_COLUMN_WIDTH + GOALS_NAME_INSET, row.y, 240.0f, row.height},
        19.0f,
        GOALS_TEXT_DIM
    );
    UiDrawTextRight(
        t->body,
        LocGetOr("LEADERBOARD_TROPHIES", "Трофеи"),
        (Rectangle){row.x, row.y, row.width - GOALS_BUTTON_WIDTH - 16.0f, row.height},
        19.0f,
        GOALS_TEXT_DIM
    );
}

GoalsAction GoalsDrawTrophies(GoalsTabs *t, GoalsScroll *scroll, const Player *p, Rectangle content, bool interactive) {
    Rectangle header = {content.x, content.y, content.width, GOALS_HEADER_HEIGHT};
    GoalsDrawRankHeader(t, header);
    Rectangle mine = {content.x, header.y + header.height, content.width, GOALS_PLAYER_ROW_HEIGHT};
    UiDrawTexture(GuiTex(GUI_GOALS_PLAYER_ROW), mine, WHITE);
    GoalsDrawRankCells(
        t,
        mine,
        t->player_rank,
        LocGetOr("PVP_YOU", "Вы"),
        p->pvp.trophies,
        GOALS_BUTTON_WIDTH + 16.0f,
        GOALS_PLAYER_TEXT
    );

    float top = mine.y + mine.height + GOALS_LIST_TOP_INSET;
    Rectangle list = {content.x, top, content.width, content.y + content.height - top};
    float stride = GOALS_RANK_ROW_HEIGHT + GOALS_ROW_GAP;
    GoalsScrollUpdate(scroll, list, t->rank_count * stride - GOALS_ROW_GAP);

    GoalsAction action = GOALS_NONE;
    bool visible = CheckCollisionPointRec(UiMouse(), list);
    GoalsBeginClip(list);
    for (int i = 0; i < t->rank_count; i++) {
        float y = list.y + i * stride - scroll->offset;
        if (y + GOALS_RANK_ROW_HEIGHT < list.y || y > list.y + list.height) continue;
        Rectangle row = {list.x, y, list.width, GOALS_RANK_ROW_HEIGHT};
        const PvpRankEntry *entry = &t->ranks[i];
        bool self = entry->bot == PVP_RANK_PLAYER;
        DrawRectangleRec(row, GOALS_ROW_TINT);
        const char *name = self ? LocGetOr("PVP_YOU", "Вы") : PvpTableEnsure()->bots[entry->bot].name;
        GoalsDrawRankCells(t, row, i + 1, name, entry->trophies, GOALS_BUTTON_WIDTH + 16.0f, GOALS_TEXT);
        if (self) continue;
        Rectangle button = {
            row.x + row.width - GOALS_BUTTON_WIDTH - 6.0f,
            row.y + (row.height - GOALS_BUTTON_HEIGHT) * 0.5f,
            GOALS_BUTTON_WIDTH,
            GOALS_BUTTON_HEIGHT,
        };
        if (ButtonDrawEx(&t->fight, button, interactive && visible && !scroll->drag_scrolled)) {
            t->selected = entry->bot;
            action = GOALS_FIGHT_BOT;
        }
    }
    EndScissorMode();
    return action;
}

static void GoalsAgoText(char *out, size_t size, long long seconds) {
    const char *key = "MOMENTS_AGO";
    long long number = 0;
    if (seconds >= NEXUS_SECONDS_PER_DAY) {
        number = seconds / NEXUS_SECONDS_PER_DAY;
        key = number == 1 ? "DAY_AGO" : "DAYS_AGO";
    } else if (seconds >= GOALS_HOUR) {
        number = seconds / GOALS_HOUR;
        key = number == 1 ? "HOUR_AGO" : "HOURS_AGO";
    } else if (seconds >= GOALS_MINUTE) {
        number = seconds / GOALS_MINUTE;
        key = number == 1 ? "MINUTE_AGO" : "MINUTES_AGO";
    }
    StrReplaceToken(out, size, LocGet(key), "[number]", TextFormat("%lld", number));
}

static void GoalsBattleText(char *out, size_t size, const PlayerPvpBattle *battle) {
    const char *key =
        TextFormat("BATTLE_HISTORY_%s_&_%s", battle->defense ? "DEFEND" : "ATTACK", battle->won ? "WIN" : "LOSE");
    char named[GOALS_TEXT_SIZE];
    StrReplaceToken(named, sizeof(named), LocGet(key), "[opponent]", PvpTableEnsure()->bots[battle->bot].name);
    StrReplaceToken(out, size, named, "[xp]", TextFormat("%d", battle->xp));
}

static void GoalsDrawBattleRow(Font font, Font body, const PlayerPvpBattle *battle, Rectangle row, long long now) {
    DrawRectangleRec(row, GOALS_ROW_TINT);
    float reserved = GOALS_BUTTON_WIDTH + GOALS_TROPHY_COLUMN_WIDTH + 24.0f;
    Rectangle text = {row.x + 18.0f, row.y + 8.0f, row.width - reserved, 44.0f};
    char description[GOALS_TEXT_SIZE], ago[64];
    GoalsBattleText(description, sizeof(description), battle);
    GoalsAgoText(ago, sizeof(ago), now - battle->at);
    HeroesDrawRich(body, description, text, 19.0f, GOALS_TEXT);
    UiDrawTextLeftMiddle(
        body, ago, (Rectangle){text.x, row.y + row.height - 28.0f, text.width, 22.0f}, 16.0f, GOALS_TEXT_DIM
    );
    Rectangle delta = {
        row.x + row.width - reserved + 12.0f, row.y, GOALS_TROPHY_COLUMN_WIDTH - GOALS_TROPHY_ICON_SIZE, row.height
    };
    bool event = battle->banners > 0;
    int amount = event ? battle->banners : battle->trophy_delta;
    UiDrawTextRight(font, TextFormat("%+d", amount), delta, 26.0f, amount >= 0 ? GOALS_GAIN : GOALS_LOSS);
    UiDrawTexture(
        GuiTex(event ? GUI_PVP_BANNER : GUI_TAB_TROPHY),
        (Rectangle){
            delta.x + delta.width + 4.0f,
            row.y + (row.height - GOALS_TROPHY_ICON_SIZE) * 0.5f,
            GOALS_TROPHY_ICON_SIZE,
            GOALS_TROPHY_ICON_SIZE,
        },
        WHITE
    );
}

GoalsAction GoalsDrawBattles(
    GoalsTabs *t, GoalsScroll *scroll, const PlayerPvp *pvp, Rectangle content, bool interactive, long long now
) {
    if (pvp->history_count == 0) {
        GoalsDrawEmpty(t, "Боев пока нет. Сразитесь с другим игроком!", content);
        return GOALS_NONE;
    }
    float stride = GOALS_BATTLE_ROW_HEIGHT + GOALS_ROW_GAP;
    GoalsScrollUpdate(scroll, content, pvp->history_count * stride - GOALS_ROW_GAP);

    GoalsAction action = GOALS_NONE;
    bool visible = CheckCollisionPointRec(UiMouse(), content);
    GoalsBeginClip(content);
    for (int i = 0; i < pvp->history_count; i++) {
        float y = content.y + i * stride - scroll->offset;
        if (y + GOALS_BATTLE_ROW_HEIGHT < content.y || y > content.y + content.height) continue;
        Rectangle row = {content.x, y, content.width, GOALS_BATTLE_ROW_HEIGHT};
        GoalsDrawBattleRow(t->font, t->body, &pvp->history[i], row, now);
        if (!PvpCanRevenge(&pvp->history[i])) continue;
        Rectangle button = {
            row.x + row.width - GOALS_BUTTON_WIDTH - 6.0f,
            row.y + (row.height - GOALS_BUTTON_HEIGHT) * 0.5f,
            GOALS_BUTTON_WIDTH,
            GOALS_BUTTON_HEIGHT,
        };
        if (ButtonDrawEx(&t->fight, button, interactive && visible && !scroll->drag_scrolled)) {
            t->selected = i;
            action = GOALS_REVENGE;
        }
    }
    EndScissorMode();
    return action;
}

static void GoalsDrawEventRow(const GoalsTabs *t, const PlayerPvpEventResult *result, Rectangle row) {
    DrawRectangleRec(row, GOALS_ROW_TINT);
    Rectangle banner = {row.x + 12.0f, row.y + (row.height - 58.0f) * 0.5f, 58.0f, 58.0f};
    UiDrawTexture(GuiTex(GUI_PVP_BANNER), banner, WHITE);
    float x = banner.x + banner.width + 12.0f;
    float width = row.width - GOALS_BUTTON_WIDTH - banner.width - 200.0f;
    UiDrawTextLeftMiddle(
        t->font,
        LocGet(PvpEventForCycle(result->cycle)->title_key),
        (Rectangle){x, row.y + 8.0f, width, 34.0f},
        22.0f,
        GOALS_TEXT
    );
    char rank[64];
    LocFormat2(rank, sizeof(rank), "PVP_EVENT_RANKING", TextFormat("%d", result->rank), "");
    UiDrawTextLeftMiddle(
        t->body,
        TextFormat("%s   %s %d", rank, LocGetOr("PVP_EVENT_BANNERS", "Знамена"), result->banners),
        (Rectangle){x, row.y + 44.0f, width, 26.0f},
        18.0f,
        GOALS_TEXT_DIM
    );
    bool gems = result->gems > 0;
    int value = gems ? result->gems : result->gold;
    const PvpEventRankReward *reward = PvpEventRewardForRank(PvpEventForCycle(result->cycle), result->rank);
    Rectangle amount = {row.x + row.width - GOALS_BUTTON_WIDTH - 180.0f, row.y, 120.0f, row.height};
    bool lead_prize = reward && reward->reward.prize_count > 0;
    if (value > 0 && !lead_prize) UiDrawTextRight(t->font, TextFormat("+%d", value), amount, 28.0f, GOALS_TEXT);
    UiDrawTexture(
        reward ? PvpRewardTexture(&reward->reward) : GuiTex(gems ? GUI_MISSION_GEM : GUI_MISSION_GOLD),
        (Rectangle){amount.x + amount.width + 6.0f, row.y + (row.height - 40.0f) * 0.5f, 40.0f, 40.0f},
        WHITE
    );
}

GoalsAction
GoalsDrawEvents(GoalsTabs *t, GoalsScroll *scroll, const PlayerPvp *pvp, Rectangle content, bool interactive) {
    if (pvp->event_result_count == 0) {
        GoalsDrawEmpty(t, "Завершённых событий пока нет.", content);
        return GOALS_NONE;
    }
    float stride = GOALS_BATTLE_ROW_HEIGHT + GOALS_ROW_GAP;
    GoalsScrollUpdate(scroll, content, pvp->event_result_count * stride - GOALS_ROW_GAP);
    GoalsAction action = GOALS_NONE;
    bool visible = CheckCollisionPointRec(UiMouse(), content);
    GoalsBeginClip(content);
    for (int i = 0; i < pvp->event_result_count; i++) {
        float y = content.y + i * stride - scroll->offset;
        if (y + GOALS_BATTLE_ROW_HEIGHT < content.y || y > content.y + content.height) continue;
        Rectangle row = {content.x, y, content.width, GOALS_BATTLE_ROW_HEIGHT};
        GoalsDrawEventRow(t, &pvp->event_results[i], row);
        Rectangle button = {
            row.x + row.width - GOALS_BUTTON_WIDTH - 6.0f,
            row.y + (row.height - GOALS_BUTTON_HEIGHT) * 0.5f,
            GOALS_BUTTON_WIDTH,
            GOALS_BUTTON_HEIGHT,
        };
        if (pvp->event_results[i].claimed) {
            UiDrawTextCentered(t->body, LocGetOr("PVP_EVENT_DONE", "готово"), button, 20.0f, GOALS_TEXT_DIM);
        } else if (ButtonDrawEx(&t->claim, button, interactive && visible && !scroll->drag_scrolled)) {
            t->selected = i;
            action = GOALS_CLAIM_EVENT;
        }
    }
    EndScissorMode();
    return action;
}
