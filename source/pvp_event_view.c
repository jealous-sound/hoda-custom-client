#define PVP_EVENT_VIEW_RULE_CAPACITY 256
#define PVP_EVENT_VIEW_POPUP_ASPECT 1.48f
#define PVP_EVENT_VIEW_POPUP_HEIGHT 0.9f
#define PVP_EVENT_VIEW_LABEL_SCALE 0.36f
#define PVP_EVENT_VIEW_ACTION_SCALE 0.34f
#define PVP_EVENT_VIEW_METER_DELAY 1.0f
#define PVP_EVENT_VIEW_METER_SPEED 3.0f
#define PVP_EVENT_VIEW_SUNBURST_DEGREES_PER_SECOND 5.0f
#define PVP_EVENT_VIEW_ALL_REWARDS_ROW_HEIGHT 120.0f
#define PVP_EVENT_VIEW_TIER_COLUMNS 5
#define PVP_EVENT_VIEW_BATTLE_ROW_HEIGHT 82.0f
#define PVP_EVENT_VIEW_ROW_GAP 6.0f
#define PVP_EVENT_VIEW_TAP_TOLERANCE 13.0f

typedef enum {
    PVP_EVENT_VIEW_NONE = 0,
    PVP_EVENT_VIEW_CLOSE,
    PVP_EVENT_VIEW_ENTER,
    PVP_EVENT_VIEW_FIGHT,
    PVP_EVENT_VIEW_GALLERY
} PvpEventViewAction;

typedef enum {
    PVP_EVENT_TAB_BANNERS,
    PVP_EVENT_TAB_EVENT,
    PVP_EVENT_TAB_REWARDS,
    PVP_EVENT_TAB_BATTLES,
    PVP_EVENT_TAB_COUNT
} PvpEventTab;

static const Color PVP_EVENT_VIEW_TEXT = {235, 232, 220, 255};
static const Color PVP_EVENT_VIEW_DIM = {128, 128, 128, 255};
static const Color PVP_EVENT_VIEW_SOFT = {182, 178, 168, 255};
static const Color PVP_EVENT_VIEW_ORANGE = {255, 165, 0, 255};
static const Color PVP_EVENT_VIEW_DARK = {36, 22, 8, 255};
static const Color PVP_EVENT_VIEW_ROW_TINT = {0, 0, 0, 204};
static const Color PVP_EVENT_VIEW_PANEL_TINT = {0, 0, 0, 178};
static const Color PVP_EVENT_VIEW_COLUMN_TINT = {31, 31, 31, 255};
static const Color PVP_EVENT_VIEW_STRIP_TINT = {255, 255, 255, 178};
static const Color PVP_EVENT_VIEW_SELECTION_TINT = {178, 178, 178, 255};

typedef struct {
    Font font;
    Font body;
    Button close;
    Button enter;
    Button view_all;
    Button tabs[PVP_EVENT_TAB_COUNT];
    Button fight;
    PvpEventTab tab;
    PvpRankEntry ranks[PVP_BOT_CAPACITY + 1];
    int rank_count;
    GoalsScroll rank_scroll;
    GoalsScroll bracket_scroll;
    GoalsScroll reward_scroll;
    GoalsScroll battle_scroll;
    bool bracket_focus;
    float meter_position;
    float meter_delay;
    int meter_target;
    bool meter_chosen;
    bool instructions;
    int selected_bot;
    Vector2 press;
    bool scrim_armed;
} PvpEventView;

static Rectangle PvpEventFit(Rectangle parent, float x, float y, float width, float height, float aspect) {
    float w = parent.width * width;
    float h = parent.height * height;
    if (aspect > 0.0f) {
        if (w > h * aspect)
            w = h * aspect;
        else
            h = w / aspect;
    }
    return (Rectangle){parent.x + (parent.width - w) * x, parent.y + (parent.height - h) * y, w, h};
}

void PvpEventViewInitialize(PvpEventView *v) {
    memset(v, 0, sizeof(*v));
    v->font = GuiFont(FNT_COOL);
    v->body = GuiFont(FNT_MENU);
    Vector2 centre = {0.5f, 0.5f};
    v->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&v->close, SND_UI_POPUP_CLOSE);
    v->enter = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("PVE_EVENT_MAP_ENTER"), v->font, centre, PVP_EVENT_VIEW_ACTION_SCALE
    );
    v->view_all = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("PVP_EVENT_VIEW_ALL"), v->font, centre, PVP_EVENT_VIEW_ACTION_SCALE
    );
    v->fight = ButtonLoad(GUI_PVP_BUTTON_SHORT, GUI_PVP_BUTTON_SHORT_ON);
    v->tabs[PVP_EVENT_TAB_BANNERS] = ButtonLoad(GUI_GOALS_TAB_FEATURED, GUI_GOALS_TAB_FEATURED_ON);
    static const struct {
        PvpEventTab tab;
        const char *key;
    } labels[] = {
        {PVP_EVENT_TAB_EVENT, "PVP_EVENT_DETAILS"},
        {PVP_EVENT_TAB_REWARDS, "PVP_EVENT_ALL_REWARDS"},
        {PVP_EVENT_TAB_BATTLES, "PVP_EVENT_BATTLES"},
    };
    for (size_t i = 0; i < sizeof(labels) / sizeof(labels[0]); i++) {
        const char *label = LocGet(labels[i].key);
        v->tabs[labels[i].tab] =
            ButtonLoadText(GUI_GOALS_TAB, GUI_GOALS_TAB_ON, label, v->font, centre, PVP_EVENT_VIEW_LABEL_SCALE);
    }
    for (int tab = 0; tab < PVP_EVENT_TAB_COUNT; tab++) ButtonSetClickSound(&v->tabs[tab], SND_UI_TAB);
}

static const PvpEvent *PvpEventViewEvent(void) {
    return PvpEventForCycle(PvpEventCycle(ServerNow()));
}

void PvpEventViewOpen(PvpEventView *v) {
    v->tab = PVP_EVENT_TAB_EVENT;
    v->meter_position = (float)(PvpEventViewEvent()->milestone_count - 1);
    v->meter_delay = PVP_EVENT_VIEW_METER_DELAY;
    v->meter_chosen = false;
    v->instructions = false;
    v->bracket_focus = true;
    v->scrim_armed = false;
    GoalsScrollReset(&v->rank_scroll);
    GoalsScrollReset(&v->bracket_scroll);
    GoalsScrollReset(&v->reward_scroll);
    GoalsScrollReset(&v->battle_scroll);
}

static void PvpEventViewSelectTab(PvpEventView *v, PvpEventTab tab) {
    v->tab = tab;
    v->bracket_focus = true;
}

static bool PvpEventViewTapped(const PvpEventView *v, Rectangle area, bool interactive) {
    if (!interactive || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return false;
    Vector2 mouse = UiMouse();
    bool still = Vector2Distance(mouse, v->press) <= PVP_EVENT_VIEW_TAP_TOLERANCE;
    return still && CheckCollisionPointRec(mouse, area) && CheckCollisionPointRec(v->press, area);
}

static void PvpEventViewDrawAmount(const PvpEventView *v, const PvpReward *reward, Rectangle area, float size) {
    if (reward->prize_count > 0) return;
    int amount = reward->gems > 0 ? reward->gems : reward->gold;
    if (amount > 0) UiDrawTextRight(v->font, TextFormat("+%d", amount), area, size, PVP_EVENT_VIEW_TEXT);
}

static void PvpEventBracketText(char *out, size_t size, const char *key, const PvpEventRankReward *reward) {
    if (reward->first == reward->last) {
        LocFormat2(out, size, "PVP_EVENT_RANK", TextFormat("%d", reward->first), "");
        return;
    }
    LocFormat2(out, size, key, TextFormat("%d", reward->first), TextFormat("%d", reward->last));
}

static void PvpEventViewDrawTabs(PvpEventView *v, Rectangle panel, bool interactive) {
    static const struct {
        PvpEventTab tab;
        float x;
        float aspect;
    } slots[] = {
        {PVP_EVENT_TAB_BANNERS, 0.0f, 1.25f},
        {PVP_EVENT_TAB_EVENT, 0.16f, 3.62f},
        {PVP_EVENT_TAB_REWARDS, 0.58f, 3.62f},
        {PVP_EVENT_TAB_BATTLES, 1.0f, 3.62f},
    };
    static const float separators[] = {0.101f, 0.407f, 0.71f};
    Rectangle bar = PvpEventFit(panel, 0.5f, 0.15f, 1.0f, 0.1f, 12.6f);
    for (size_t i = 0; i < sizeof(slots) / sizeof(slots[0]); i++) {
        Rectangle slot = PvpEventFit(bar, slots[i].x, 0.0f, 1.0f, 1.0f, slots[i].aspect);
        Button *button = &v->tabs[slots[i].tab];
        if (slots[i].tab == v->tab) {
            ButtonSetLabelColor(button, WHITE);
            ButtonDrawSelected(button, slot);
        } else {
            ButtonSetLabelColor(button, GRAY);
            if (ButtonDrawEx(button, slot, interactive)) PvpEventViewSelectTab(v, slots[i].tab);
        }
    }
    for (size_t i = 0; i < sizeof(separators) / sizeof(separators[0]); i++) {
        Rectangle spacer = PvpEventFit(bar, separators[i], 0.5f, 1.0f, 0.8f, 0.225f);
        UiDrawTexture(GuiTex(GUI_GOALS_TAB_SPACER), spacer, WHITE);
    }
    UiDrawTexture(GuiTex(GUI_EVENT_BANNER), PvpEventFit(bar, 0.01f, 0.0f, 1.0f, 1.0f, 1.0f), WHITE);
}

static void PvpEventViewDrawTitle(const PvpEventView *v, const PvpEvent *event, Rectangle area, long long now) {
    char remaining[48];
    QuestsShortTime(remaining, sizeof(remaining), PvpEventRemaining(now));
    const char *title = LocGet(event->title_key);
    const char *timer = TextFormat(" (%s)", remaining);
    float size = 28.0f;
    float title_width = MeasureTextEx(v->font, title, size, 1.0f).x;
    float timer_width = MeasureTextEx(v->font, timer, size, 1.0f).x;
    float x = area.x + (area.width - title_width - timer_width) * 0.5f;
    UiDrawTextLeftMiddle(v->font, title, (Rectangle){x, area.y, title_width, area.height}, size, PVP_EVENT_VIEW_TEXT);
    Rectangle time_area = {x + title_width, area.y, timer_width, area.height};
    UiDrawTextLeftMiddle(v->font, timer, time_area, size, PVP_EVENT_VIEW_ORANGE);
}

static void PvpEventViewDrawRankColumn(
    PvpEventView *v, const PvpEventRankReward *reward, int tier, int index, bool mine, Rectangle row, bool interactive
) {
    int count = PVP_EVENT_VIEW_TIER_COLUMNS;
    float width = row.width / count;
    float height = row.height * (0.5f + 0.5f * (float)(count - index) / count);
    Rectangle column = {row.x + width * (count - index - 1), row.y + row.height - height, width, height};
    DrawRectangleRec(PvpEventFit(column, 0.5f, 1.0f, 0.8f, 1.0f, 0.0f), PVP_EVENT_VIEW_COLUMN_TINT);
    if (mine) UiDrawTexture(GuiTex(GUI_EVENT_TIER_COLUMN), PvpEventFit(column, 0.5f, 1.0f, 0.8f, 1.0f, 0.0f), WHITE);

    Rectangle portrait = PvpEventFit(column, 0.5f, 0.0f, 0.8f, 1.0f, 1.0f);
    if (index == 0) {
        Rectangle burst = PvpEventFit(portrait, 0.5f, 0.5f, 1.9f, 1.9f, 1.0f);
        Texture2D texture = GuiTex(GUI_RESULTS_STARBURST);
        Rectangle source = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
        Rectangle centred = {burst.x + burst.width * 0.5f, burst.y + burst.height * 0.5f, burst.width, burst.height};
        Vector2 origin = {burst.width * 0.5f, burst.height * 0.5f};
        float angle = (float)GetTime() * PVP_EVENT_VIEW_SUNBURST_DEGREES_PER_SECOND;
        DrawTexturePro(texture, source, centred, origin, angle, WHITE);
    }
    UiDrawTexture(PvpRewardTexture(&reward->reward), PvpEventFit(portrait, 0.5f, 0.2f, 1.0f, 0.8f, 1.0f), WHITE);
    Rectangle quantity = {portrait.x, portrait.y + portrait.height * 0.7f, portrait.width, portrait.height * 0.3f};
    PvpEventViewDrawAmount(v, &reward->reward, quantity, 16.0f);

    char bracket[32];
    PvpEventBracketText(bracket, sizeof(bracket), "PVP_EVENT_BRACKET", reward);
    Rectangle label = PvpEventFit(column, 0.5f, 1.0f, 1.0f, 1.0f, 3.0f);
    UiDrawTextCentered(v->body, bracket, label, 15.0f, PVP_EVENT_VIEW_TEXT);

    if (PvpEventViewTapped(v, column, interactive)) {
        GuiSoundPlay(SND_UI_CLICK);
        v->reward_scroll.offset = tier * (PVP_EVENT_VIEW_ALL_REWARDS_ROW_HEIGHT + PVP_EVENT_VIEW_ROW_GAP);
        PvpEventViewSelectTab(v, PVP_EVENT_TAB_REWARDS);
    }
}

static void PvpEventViewDrawTiers(PvpEventView *v, int rank, Rectangle content, bool interactive) {
    Rectangle tiers = PvpEventFit(content, 0.1f, 0.45f, 1.0f, 0.62f, 1.7f);
    UiDrawTexture(GuiTex(GUI_EVENT_TIERS_BG), tiers, WHITE);
    if (PvpEventViewTapped(v, PvpEventFit(tiers, 1.0f, 0.0f, 1.0f, 0.2f, 1.0f), interactive)) {
        GuiSoundPlay(SND_UI_CLICK);
        v->instructions = !v->instructions;
    }
    if (v->instructions) {
        Rectangle text = PvpEventFit(tiers, 0.5f, 0.5f, 0.8f, 0.6f, 0.0f);
        HeroesDrawRich(v->body, LocGet("PVP_EVENT_BANNERS_INSTRUCTIONS"), text, 20.0f, PVP_EVENT_VIEW_TEXT);
        return;
    }
    UiDrawTexture(GuiTex(GUI_EVENT_BANNER), PvpEventFit(tiers, 0.04f, 0.025f, 1.0f, 0.2f, 1.0f), WHITE);
    QuestsDrawFit(
        v->font,
        LocGet("PVP_EVENT_FINAL_RANK_REWARDS"),
        PvpEventFit(tiers, 0.85f, 0.0f, 0.8f, 0.15f, 0.0f),
        20.0f,
        PVP_EVENT_VIEW_TEXT
    );
    UiDrawTextLeftMiddle(
        v->body,
        LocGet("PVP_EVENT_TAP_PORTRAITS_FOR_DETAILS"),
        PvpEventFit(tiers, 0.85f, 0.12f, 0.8f, 0.15f, 0.0f),
        15.0f,
        PVP_EVENT_VIEW_SOFT
    );

    Rectangle strip = PvpEventFit(tiers, 0.0f, 1.0f, 1.0f, 0.235f, 0.0f);
    UiDrawTexture(GuiTex(GUI_GOALS_PLAYER_ROW), strip, PVP_EVENT_VIEW_STRIP_TINT);
    char you[64];
    LocFormat2(you, sizeof(you), "PVP_EVENT_YOU_ARE_RANK", TextFormat("%d", rank), "");
    Rectangle label = PvpEventFit(tiers, 0.05f, 0.975f, 0.4f, 0.2f, 0.0f);
    UiDrawTextLeftMiddle(v->font, you, label, 18.0f, PVP_EVENT_VIEW_DARK);

    Rectangle row = PvpEventFit(tiers, 0.5f, 0.46f, 0.9f, 0.56f, 0.0f);
    const PvpEvent *event = PvpEventViewEvent();
    int mine = PvpEventRankTier(event, rank);
    for (int i = 0; i < PVP_EVENT_VIEW_TIER_COLUMNS; i++) {
        int tier = i * (event->rank_reward_count - 1) / (PVP_EVENT_VIEW_TIER_COLUMNS - 1);
        int next = (i + 1) * (event->rank_reward_count - 1) / (PVP_EVENT_VIEW_TIER_COLUMNS - 1);
        bool highlighted = mine >= tier && (i == PVP_EVENT_VIEW_TIER_COLUMNS - 1 || mine < next);
        int shown = highlighted ? mine : tier;
        PvpEventViewDrawRankColumn(v, &event->rank_rewards[shown], shown, i, highlighted, row, interactive);
    }

    if (ButtonDrawEx(&v->view_all, PvpEventFit(tiers, 1.0f, 1.0f, 1.0f, 0.232f, 4.0f), interactive))
        PvpEventViewSelectTab(v, PVP_EVENT_TAB_REWARDS);
}

static void PvpEventViewMeterStep(PvpEventView *v, const PlayerPvp *pvp, int count) {
    if (!v->meter_chosen) {
        int next = 0;
        while (next < count - 1 && pvp->event.milestones_paid > next) next++;
        v->meter_target = next;
    }
    float dt = GetFrameTime();
    if (v->meter_delay > 0.0f) {
        v->meter_delay -= dt;
        return;
    }
    float blend = fminf(1.0f, dt * PVP_EVENT_VIEW_METER_SPEED);
    v->meter_position += ((float)v->meter_target - v->meter_position) * blend;
}

static bool PvpEventViewDrawArrow(const PvpEventView *v, Rectangle area, float angle, bool enabled, bool interactive) {
    if (!enabled) return false;
    Texture2D texture = GuiTex(GUI_EVENT_ARROW);
    Rectangle source = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
    Rectangle centred = {area.x + area.width * 0.5f, area.y + area.height * 0.5f, area.width, area.height};
    DrawTexturePro(texture, source, centred, (Vector2){area.width * 0.5f, area.height * 0.5f}, angle, WHITE);
    return PvpEventViewTapped(v, area, interactive);
}

static void PvpEventViewChooseMeter(PvpEventView *v, int target) {
    GuiSoundPlay(SND_UI_CLICK);
    v->meter_target = target;
    v->meter_chosen = true;
    v->meter_delay = 0.0f;
}

static void PvpEventViewDrawMeterList(PvpEventView *v, const PlayerPvp *pvp, Rectangle meter, bool interactive) {
    const PvpEvent *event = PvpEventForCycle(pvp->event.cycle);
    Rectangle list = {
        meter.x + meter.width * 0.5f - meter.width,
        meter.y + meter.height * 0.1f,
        meter.width * 2.0f,
        meter.height * 0.8f,
    };
    float step = list.height * 0.25f;
    float side = step * 0.8f;
    float centre_x = list.x + list.width * 0.5f;
    float centre_y = list.y + list.height * 0.5f;
    Rectangle window = {list.x, centre_y - step * 1.5f, list.width, step * 3.0f};
    bool hovering = CheckCollisionPointRec(UiMouse(), window);
    GoalsBeginClip(window);
    for (int i = 0; i < event->milestone_count; i++) {
        float y = centre_y - ((float)i - v->meter_position) * step;
        Rectangle tile = {centre_x - side * 0.5f, y - side * 0.5f, side, side};
        UiDrawTexture(PvpRewardTexture(&event->milestones[i].reward), tile, WHITE);
        if (pvp->event.milestones_paid > i) UiDrawTexture(GuiTex(GUI_EVENT_CHECK), tile, WHITE);
        if (hovering && PvpEventViewTapped(v, tile, interactive)) PvpEventViewChooseMeter(v, i);
    }
    EndScissorMode();
}

static void PvpEventViewDrawCallout(const PvpEventView *v, const PlayerPvp *pvp, int rank, Rectangle callout) {
    const PvpEvent *event = PvpEventForCycle(pvp->event.cycle);
    int last = event->milestone_count - 1;
    int index = (int)Clamp(roundf(v->meter_position), 0.0f, (float)last);
    const PvpEventMilestone *milestone = &event->milestones[index];
    UiDrawTexture(GuiTex(GUI_EVENT_CALLOUT), callout, WHITE);
    UiDrawTextLeftMiddle(
        v->font,
        LocGet("PVP_EVENT_ACHIEVEMENTS"),
        PvpEventFit(callout, 0.0f, -1.7f, 0.6f, 0.5f, 0.0f),
        20.0f,
        PVP_EVENT_VIEW_TEXT
    );
    Rectangle next = PvpEventFit(callout, 0.0f, -1.0f, 0.61f, 0.5f, 0.0f);
    UiDrawTextLeftMiddle(v->body, LocGet("PVP_EVENT_NEXT"), next, 18.0f, PVP_EVENT_VIEW_DIM);
    UiDrawTextRight(v->font, TextFormat("%d", milestone->banners), next, 20.0f, PVP_EVENT_VIEW_TEXT);
    UiDrawTexture(PvpRewardTexture(&milestone->reward), PvpEventFit(callout, 0.7f, -1.0f, 1.0f, 0.5f, 1.0f), WHITE);

    char left[48], earned[48];
    int remaining = milestone->banners - pvp->event.banners;
    if (remaining > 0)
        LocFormat2(left, sizeof(left), "PVP_EVENT_BANNERS_LEFT", TextFormat("%d", remaining), "");
    else
        StrCopy(left, sizeof(left), LocGet("PVP_EVENT_DONE"));
    LocFormat2(earned, sizeof(earned), "PVP_EVENT_BANNERS_EARNED", TextFormat("%d", pvp->event.banners), "");
    UiDrawTextCentered(v->font, left, PvpEventFit(callout, 0.35f, 0.0f, 0.4f, 0.65f, 0.0f), 20.0f, PVP_EVENT_VIEW_TEXT);
    UiDrawTextCentered(
        v->body, earned, PvpEventFit(callout, 0.22f, 1.2f, 0.5f, 0.6f, 0.0f), 17.0f, PVP_EVENT_VIEW_TEXT
    );
    char rank_text[48];
    LocFormat2(rank_text, sizeof(rank_text), "PVP_EVENT_RANK", TextFormat("%d", rank), "");
    UiDrawTextLeftMiddle(
        v->body, rank_text, PvpEventFit(callout, 0.0f, 1.9f, 0.7f, 0.45f, 0.0f), 18.0f, PVP_EVENT_VIEW_TEXT
    );
}

static void
PvpEventViewDrawMeter(PvpEventView *v, const PlayerPvp *pvp, int rank, Rectangle content, bool interactive) {
    int count = PvpEventForCycle(pvp->event.cycle)->milestone_count;
    PvpEventViewMeterStep(v, pvp, count);
    Rectangle rewards = PvpEventFit(content, 0.96f, 0.05f, 1.0f, 0.8f, 0.8f);
    Rectangle meter = PvpEventFit(rewards, 1.0f, 0.5f, 1.0f, 1.0f, 0.201f);
    UiDrawTexture(GuiTex(GUI_EVENT_METER), meter, WHITE);
    PvpEventViewDrawMeterList(v, pvp, meter, interactive);
    PvpEventViewDrawCallout(v, pvp, rank, PvpEventFit(rewards, 1.0f, 0.5f, 1.0f, 1.0f, 4.12f));

    Rectangle arrows = PvpEventFit(content, 1.0f, 0.0f, 1.0f, 1.0f, 1.48f);
    Rectangle top = PvpEventFit(arrows, 0.955f, -0.025f, 1.0f, 0.15f, 0.928f);
    Rectangle bottom = PvpEventFit(arrows, 0.955f, 0.805f, 1.0f, 0.15f, 0.928f);
    if (PvpEventViewDrawArrow(v, top, 270.0f, v->meter_target < count - 1, interactive))
        PvpEventViewChooseMeter(v, v->meter_target + 1);
    if (PvpEventViewDrawArrow(v, bottom, 90.0f, v->meter_target > 0, interactive))
        PvpEventViewChooseMeter(v, v->meter_target - 1);
}

static float PvpEventViewDrawRichMiddle(Font font, const char *text, Rectangle area, float size, Color color) {
    float line_gap = size * (HEROES_RICH_LINE_HEIGHT - 1.0f);
    float height = HeroesRichLayout(font, text, area, size, color, false) - line_gap;
    area.y += (area.height - height) * 0.5f;
    area.height = height;
    return HeroesDrawRich(font, text, area, size, color);
}

void PvpEventViewRuleText(char *out, size_t size, const PvpEvent *event) {
    const char *rule = NexusGameChangerText(&event->changer);
    if (!PvpTableEnsure()->rules.stat_budget) {
        StrCopy(out, size, rule);
        return;
    }
    char cap[PVP_EVENT_VIEW_RULE_CAPACITY];
    LocFormatInt(cap, sizeof(cap), "HODA_PVP_EVENT_BUFF_CAP", StatBudgetEnsure()->changer_cap_pct);
    snprintf(out, size, "%s\n%s", rule, cap);
}

static PvpEventViewAction
PvpEventViewDrawBottomBar(PvpEventView *v, const PvpEvent *event, Rectangle content, bool interactive) {
    Rectangle bar = PvpEventFit(content, 0.5f, 0.97f, 1.0f, 0.165f, 10.7f);
    UiDrawTexture(GuiTex(GUI_EVENT_BOTTOM_BAR), bar, WHITE);
    Rectangle gallery = PvpEventFit(bar, 0.005f, 0.5f, 1.0f, 0.9f, 2.0f);
    UiDrawTexture(GuiTex(GUI_EVENT_BANNER), PvpEventFit(gallery, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f), WHITE);
    Rectangle rules = PvpEventFit(bar, 0.35f, 0.55f, 0.48f, 0.8f, 0.0f);
    char rule[PVP_EVENT_VIEW_RULE_CAPACITY * 2];
    PvpEventViewRuleText(rule, sizeof(rule), event);
    PvpEventViewDrawRichMiddle(v->body, rule, rules, 18.0f, PVP_EVENT_VIEW_TEXT);
    if (ButtonDrawEx(&v->enter, PvpEventFit(bar, 0.99f, 0.5f, 1.0f, 0.873f, 4.0f), interactive))
        return PVP_EVENT_VIEW_ENTER;
    if (PvpEventViewTapped(v, gallery, interactive)) {
        GuiSoundPlay(SND_UI_CLICK);
        return PVP_EVENT_VIEW_GALLERY;
    }
    return PVP_EVENT_VIEW_NONE;
}

static PvpEventViewAction PvpEventViewDrawEventTab(
    PvpEventView *v, const PlayerPvp *pvp, const PvpEvent *event, Rectangle content, long long now, bool interactive
) {
    int rank = PvpEventCurrentRank(pvp, now);
    PvpEventViewDrawTitle(v, event, PvpEventFit(content, 0.0f, -0.02f, 1.0f, 0.12f, 0.0f), now);
    UiDrawTextCentered(
        v->body,
        LocGet("PVP_EVENT_BANNERS_TO_EARN_REWARDS"),
        PvpEventFit(content, 0.5f, 0.08f, 1.0f, 0.12f, 0.0f),
        18.0f,
        PVP_EVENT_VIEW_TEXT
    );
    PvpEventViewDrawTiers(v, rank, content, interactive);
    PvpEventViewDrawMeter(v, pvp, rank, content, interactive);
    return PvpEventViewDrawBottomBar(v, event, content, interactive);
}

static void
PvpEventViewDrawBannerCell(const PvpEventView *v, Rectangle row, float x, float width, int banners, Color color) {
    Rectangle cell = PvpEventFit(row, x, 0.0f, width, 1.0f, 0.0f);
    Rectangle label = PvpEventFit(cell, 0.0f, 0.75f, 0.6f, 1.0f, 0.0f);
    UiDrawTextRight(v->font, TextFormat("%d", banners), label, 20.0f, color);
    UiDrawTexture(GuiTex(GUI_PVP_BANNER), PvpEventFit(cell, 1.0f, 0.5f, 1.0f, 0.8f, 1.0f), WHITE);
}

static void PvpEventViewDrawLeaderboard(PvpEventView *v, const Player *p, Rectangle flex, bool interactive) {
    Rectangle header = {flex.x, flex.y, flex.width, flex.height * 0.125f};
    Rectangle list = {flex.x, header.y + header.height, flex.width, flex.height * 0.735f};
    Rectangle mine = {flex.x, list.y + list.height, flex.width, flex.height * 0.14f};

    Rectangle titles = PvpEventFit(header, 0.0f, 1.0f, 1.0f, 0.9f, 0.0f);
    UiDrawTextCentered(
        v->body,
        LocGet("LEADERBOARD_RANK"),
        PvpEventFit(titles, 0.0f, 0.5f, 0.15f, 1.0f, 0.0f),
        18.0f,
        PVP_EVENT_VIEW_SOFT
    );
    UiDrawTextCentered(
        v->body,
        LocGet("LEADERBOARD_NAME"),
        PvpEventFit(titles, 0.5f, 0.5f, 0.7f, 1.0f, 0.0f),
        18.0f,
        PVP_EVENT_VIEW_SOFT
    );
    UiDrawTextCentered(
        v->body,
        LocGet("PVP_EVENT_BANNERS"),
        PvpEventFit(titles, 0.7f, 0.5f, 0.2f, 1.0f, 0.0f),
        18.0f,
        PVP_EVENT_VIEW_SOFT
    );

    float row_height = list.height * 0.238f;
    GoalsScrollUpdate(&v->rank_scroll, list, v->rank_count * row_height);
    bool visible = CheckCollisionPointRec(UiMouse(), list);
    int player_rank = v->rank_count;
    for (int i = 0; i < v->rank_count; i++)
        if (v->ranks[i].bot == PVP_RANK_PLAYER) player_rank = i + 1;
    GoalsBeginClip(list);
    for (int i = 0; i < v->rank_count; i++) {
        float y = list.y + i * row_height - v->rank_scroll.offset;
        if (y + row_height < list.y || y > list.y + list.height) continue;
        Rectangle slot = {list.x, y, list.width, row_height};
        const PvpRankEntry *entry = &v->ranks[i];
        bool self = entry->bot == PVP_RANK_PLAYER;
        DrawRectangleRec(PvpEventFit(slot, 0.0f, 0.5f, 1.0f, 0.85f, 0.0f), PVP_EVENT_VIEW_ROW_TINT);
        Rectangle row = PvpEventFit(slot, 0.5f, 0.5f, 1.0f, 0.8f, 0.0f);
        UiDrawTextCentered(
            v->font,
            TextFormat("%d", i + 1),
            PvpEventFit(row, 0.0f, 0.0f, 0.15f, 1.0f, 0.0f),
            20.0f,
            PVP_EVENT_VIEW_TEXT
        );
        const char *name = self ? LocGet("HODA_PVP_YOU") : PvpTableEnsure()->bots[entry->bot].name;
        QuestsDrawFit(v->body, name, PvpEventFit(row, 0.265f, 0.5f, 0.45f, 1.0f, 0.0f), 19.0f, PVP_EVENT_VIEW_TEXT);
        PvpEventViewDrawBannerCell(v, row, 0.725f, 0.2f, entry->trophies, PVP_EVENT_VIEW_TEXT);
        if (self) continue;
        Rectangle button = PvpEventFit(row, 1.0f, 0.5f, 1.0f, 1.0f, 2.08f);
        if (ButtonDrawEx(&v->fight, button, interactive && visible && !v->rank_scroll.drag_scrolled))
            v->selected_bot = entry->bot;
    }
    EndScissorMode();

    UiDrawTexture(GuiTex(GUI_GOALS_PLAYER_ROW), mine, PVP_EVENT_VIEW_STRIP_TINT);
    UiDrawTextCentered(
        v->font,
        TextFormat("%d", player_rank),
        PvpEventFit(mine, 0.0f, 0.0f, 0.15f, 1.0f, 0.0f),
        20.0f,
        PVP_EVENT_VIEW_DARK
    );
    QuestsDrawFit(
        v->body, LocGet("HODA_PVP_YOU"), PvpEventFit(mine, 0.25f, 0.5f, 0.4f, 1.0f, 0.0f), 19.0f, PVP_EVENT_VIEW_DARK
    );
    PvpEventViewDrawBannerCell(v, mine, 0.71f, 0.25f, p->pvp.event.banners, PVP_EVENT_VIEW_DARK);
}

static void PvpEventViewDrawBracketList(PvpEventView *v, int rank, Rectangle panel, bool interactive) {
    const PvpEvent *event = PvpEventViewEvent();
    Rectangle list = PvpEventFit(panel, 0.5f, 0.5f, 1.0f, 0.75f, 0.0f);
    float row_height = list.height * 0.22f;
    if (v->bracket_focus) {
        for (int i = 0; i < event->rank_reward_count; i++) {
            const PvpEventRankReward *reward = &event->rank_rewards[i];
            if (rank >= reward->first && rank <= reward->last) v->bracket_scroll.offset = i * row_height;
        }
        v->bracket_focus = false;
    }
    GoalsScrollUpdate(&v->bracket_scroll, list, event->rank_reward_count * row_height);
    GoalsBeginClip(list);
    for (int i = 0; i < event->rank_reward_count; i++) {
        const PvpEventRankReward *reward = &event->rank_rewards[i];
        float y = list.y + i * row_height - v->bracket_scroll.offset;
        Rectangle row = {list.x, y, list.width, row_height};
        if (rank >= reward->first && rank <= reward->last)
            UiDrawTexture(
                GuiTex(GUI_GOALS_PLAYER_ROW),
                PvpEventFit(row, 0.5f, 0.5f, 1.0f, 0.8f, 0.0f),
                PVP_EVENT_VIEW_SELECTION_TINT
            );
        if (i + 1 < event->rank_reward_count)
            UiDrawTexture(GuiTex(GUI_EVENT_SEPARATOR), PvpEventFit(row, 0.5f, 1.05f, 1.0f, 0.1f, 0.0f), WHITE);
        char bracket[32];
        PvpEventBracketText(bracket, sizeof(bracket), "PVP_EVENT_BRACKET", reward);
        Rectangle label = PvpEventFit(row, 0.0f, 0.5f, 0.65f, 0.6f, 0.0f);
        label.x += 10.0f;
        UiDrawTextLeftMiddle(v->font, bracket, label, 18.0f, PVP_EVENT_VIEW_TEXT);
        UiDrawTexture(PvpRewardTexture(&reward->reward), PvpEventFit(row, 0.97f, 0.5f, 1.0f, 0.7f, 1.0f), WHITE);
        Rectangle quantity = PvpEventFit(row, 0.97f, 0.9f, 0.4f, 0.4f, 0.0f);
        PvpEventViewDrawAmount(v, &reward->reward, quantity, 15.0f);
        if (PvpEventViewTapped(v, row, interactive)) {
            GuiSoundPlay(SND_UI_CLICK);
            v->reward_scroll.offset = i * (PVP_EVENT_VIEW_ALL_REWARDS_ROW_HEIGHT + PVP_EVENT_VIEW_ROW_GAP);
            PvpEventViewSelectTab(v, PVP_EVENT_TAB_REWARDS);
        }
    }
    EndScissorMode();
}

static PvpEventViewAction
PvpEventViewDrawBannersTab(PvpEventView *v, const Player *p, Rectangle content, long long now, bool interactive) {
    long long elapsed = now % PvpTableEnsure()->events.duration_seconds;
    v->rank_count = PvpEventLeaderboardBuild(v->ranks, p->pvp.event.banners, p->pvp.event.cycle, elapsed);
    v->selected_bot = PVP_RANK_PLAYER;
    PvpEventViewDrawLeaderboard(v, p, PvpEventFit(content, 0.03f, 0.0f, 0.7f, 0.98f, 0.0f), interactive);

    Rectangle panel = PvpEventFit(content, 1.0f, 0.0f, 0.28f, 0.98f, 0.0f);
    DrawRectangleRec(panel, PVP_EVENT_VIEW_PANEL_TINT);
    QuestsDrawFit(
        v->font,
        LocGet("PVP_EVENT_RANK_REWARDS"),
        PvpEventFit(panel, 0.5f, 0.0f, 0.9f, 0.1f, 0.0f),
        20.0f,
        PVP_EVENT_VIEW_TEXT
    );
    PvpEventViewDrawBracketList(v, PvpEventCurrentRank(&p->pvp, now), panel, interactive);
    Rectangle note = PvpEventFit(panel, 0.5f, 1.0f, 0.95f, 0.1f, 0.0f);
    HeroesDrawRich(v->body, LocGet("PVP_EVENT_TIER_REWARDS_EVENT_END"), note, 14.0f, PVP_EVENT_VIEW_DIM);
    return v->selected_bot == PVP_RANK_PLAYER ? PVP_EVENT_VIEW_NONE : PVP_EVENT_VIEW_FIGHT;
}

static void PvpEventViewDrawPrizeTile(const PvpEventView *v, const PvpPrize *prize, Rectangle tile) {
    UiDrawTexture(PvpPrizeTexture(prize), tile, WHITE);
    if (prize->count <= 1) return;
    Rectangle badge = {tile.x, tile.y + tile.height * 0.68f, tile.width, tile.height * 0.32f};
    UiDrawTextRight(v->font, TextFormat("x%d", prize->count), badge, 20.0f, PVP_EVENT_VIEW_TEXT);
}

static void PvpEventViewDrawCurrency(const PvpEventView *v, int amount, GuiId icon, Rectangle area) {
    if (amount <= 0) return;
    Rectangle text = PvpEventFit(area, 0.0f, 0.5f, 0.72f, 1.0f, 0.0f);
    UiDrawTextRight(v->font, TextFormat("+%d", amount), text, 20.0f, PVP_EVENT_VIEW_TEXT);
    UiDrawTexture(GuiTex(icon), PvpEventFit(area, 1.0f, 0.5f, 1.0f, 0.8f, 1.0f), WHITE);
}

static void PvpEventViewDrawRewardRow(const PvpEventView *v, const PvpReward *reward, Rectangle row) {
    float side = row.height * 0.72f;
    float gap = row.height * 0.06f;
    float x = row.x + row.width * 0.2f;
    float y = row.y + (row.height - side) * 0.5f;
    for (int i = 0; i < reward->prize_count; i++) {
        Rectangle tile = {x + i * (side + gap), y, side, side};
        PvpEventViewDrawPrizeTile(v, &reward->prizes[i], tile);
    }
    PvpEventViewDrawCurrency(v, reward->gems, GUI_MISSION_GEM, PvpEventFit(row, 0.985f, 0.22f, 0.15f, 0.34f, 0.0f));
    PvpEventViewDrawCurrency(v, reward->gold, GUI_MISSION_GOLD, PvpEventFit(row, 0.985f, 0.78f, 0.15f, 0.34f, 0.0f));
}

static void PvpEventViewDrawRewardsTab(PvpEventView *v, Rectangle content, int rank) {
    const PvpEvent *event = PvpEventViewEvent();
    Rectangle list = PvpEventFit(content, 0.5f, 0.0f, 0.98f, 0.98f, 0.0f);
    float stride = PVP_EVENT_VIEW_ALL_REWARDS_ROW_HEIGHT + PVP_EVENT_VIEW_ROW_GAP;
    GoalsScrollUpdate(&v->reward_scroll, list, event->rank_reward_count * stride - PVP_EVENT_VIEW_ROW_GAP);
    GoalsBeginClip(list);
    for (int i = 0; i < event->rank_reward_count; i++) {
        const PvpEventRankReward *reward = &event->rank_rewards[i];
        float y = list.y + i * stride - v->reward_scroll.offset;
        Rectangle row = {list.x, y, list.width, PVP_EVENT_VIEW_ALL_REWARDS_ROW_HEIGHT};
        UiDrawTexture(GuiTex(GUI_MISSION_ROW), row, WHITE);
        char bracket[48];
        PvpEventBracketText(bracket, sizeof(bracket), "PVP_EVENT_RANK_BRACKET", reward);
        Rectangle label = PvpEventFit(row, 0.03f, 0.5f, 0.2f, 0.3f, 0.0f);
        bool mine = rank >= reward->first && rank <= reward->last;
        UiDrawTextLeftMiddle(v->font, bracket, label, 24.0f, mine ? PVP_EVENT_VIEW_ORANGE : PVP_EVENT_VIEW_TEXT);

        PvpEventViewDrawRewardRow(v, &reward->reward, row);
    }
    EndScissorMode();
}

static void PvpEventViewDrawBattlesTab(PvpEventView *v, const PlayerPvp *pvp, Rectangle content, long long now) {
    Rectangle list = PvpEventFit(content, 0.5f, 0.0f, 0.98f, 0.98f, 0.0f);
    int count = 0;
    for (int i = 0; i < pvp->history_count; i++) count += pvp->history[i].banners > 0;
    if (count == 0) {
        UiDrawTextCentered(v->body, LocGet("HODA_PVP_NO_BATTLES"), list, 24.0f, PVP_EVENT_VIEW_SOFT);
        return;
    }
    float stride = PVP_EVENT_VIEW_BATTLE_ROW_HEIGHT + PVP_EVENT_VIEW_ROW_GAP;
    GoalsScrollUpdate(&v->battle_scroll, list, count * stride - PVP_EVENT_VIEW_ROW_GAP);
    GoalsBeginClip(list);
    int shown = 0;
    for (int i = 0; i < pvp->history_count; i++) {
        if (pvp->history[i].banners <= 0) continue;
        float y = list.y + shown++ * stride - v->battle_scroll.offset;
        if (y + PVP_EVENT_VIEW_BATTLE_ROW_HEIGHT < list.y || y > list.y + list.height) continue;
        Rectangle row = {list.x, y, list.width, PVP_EVENT_VIEW_BATTLE_ROW_HEIGHT};
        GoalsDrawBattleRow(v->font, v->body, &pvp->history[i], row, now);
    }
    EndScissorMode();
}

PvpEventViewAction PvpEventViewDraw(PvpEventView *v, const Player *p) {
    long long now = ServerNow();
    const PvpEvent *event = PvpEventForCycle(PvpEventCycle(now));
    Rectangle root = UiRoot();
    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 170});
    float panel_height = root.height * PVP_EVENT_VIEW_POPUP_HEIGHT;
    float panel_width = panel_height * PVP_EVENT_VIEW_POPUP_ASPECT;
    Rectangle panel = UiRect(root, 0.5f, 1.0f, panel_width, panel_height);
    UiDrawTexture(GuiTex(GUI_QUESTS_BG), panel, WHITE);
    UiDrawTextCentered(
        v->font,
        LocGet("PVP_EVENT_TODAYS_EVENT"),
        PvpEventFit(panel, 0.5f, 0.0f, 1.0f, 0.1f, 0.0f),
        38.0f,
        PVP_EVENT_VIEW_TEXT
    );

    bool interactive = !UiInputBlocked();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) v->press = UiMouse();
    PvpEventViewAction action = PVP_EVENT_VIEW_NONE;
    PvpEventViewDrawTabs(v, panel, interactive);
    Rectangle content = PvpEventFit(panel, 0.5f, 0.75f, 0.86f, 0.68f, 0.0f);
    if (v->tab == PVP_EVENT_TAB_EVENT) {
        action = PvpEventViewDrawEventTab(v, &p->pvp, event, content, now, interactive);
    } else if (v->tab == PVP_EVENT_TAB_BANNERS) {
        action = PvpEventViewDrawBannersTab(v, p, content, now, interactive);
    } else if (v->tab == PVP_EVENT_TAB_REWARDS) {
        PvpEventViewDrawRewardsTab(v, content, PvpEventCurrentRank(&p->pvp, now));
    } else {
        PvpEventViewDrawBattlesTab(v, &p->pvp, content, now);
    }

    Rectangle close = PvpEventFit(panel, 0.985f, 0.065f, 1.0f, 0.113f, 1.0f);
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
