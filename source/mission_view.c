#define MISSIONS_ROW_HEIGHT 104.0f
#define MISSIONS_ROW_GAP 6.0f
#define MISSIONS_SUMMARY_HEIGHT 60.0f
#define MISSIONS_FLASH_IN_TIME 0.3f
#define MISSIONS_FLASH_OUT_TIME 0.3f
#define MISSIONS_CLAIM_TIME (MISSIONS_FLASH_IN_TIME + MISSIONS_FLASH_OUT_TIME)
#define MISSIONS_REWARD_ICON_SIZE 46.0f
#define MISSIONS_REWARD_FONT_SIZE 36.0f
#define MISSIONS_REWARD_TOP_INSET 8.0f
#define MISSIONS_BURST_TIME 1.3f
#define MISSIONS_BURST_FADE 1.0f
#define MISSIONS_TEXT_SIZE 256
#define MISSIONS_BAR_INNER_WIDTH 0.79f
#define MISSIONS_BAR_INNER_HEIGHT 0.4f
#define MISSIONS_TABS_INSET_X 61.0f
#define MISSIONS_TABS_TOP 76.0f
#define MISSIONS_TABS_WIDTH 818.0f
#define MISSIONS_TABS_HEIGHT 65.0f
#define MISSIONS_CONTENT_INSET_X 62.0f
#define MISSIONS_CONTENT_GAP 6.0f
#define MISSIONS_CONTENT_HEIGHT 414.0f

typedef enum { MISSIONS_NONE = 0, MISSIONS_CLOSE, MISSIONS_FIGHT_BOT, MISSIONS_REVENGE } MissionsAction;

static const Color MISSIONS_TEXT = {235, 232, 220, 255};
static const Color MISSIONS_TEXT_DIM = {182, 178, 168, 255};

typedef struct {
    Font font;
    Font body;
    Button close;
    Button claim;
    Texture2D icons[MISSION_CAPACITY];
    bool icon_loaded[MISSION_CAPACITY];
    int order[MISSION_SERIES_CAPACITY];
    GoalsTabs tabs;
    GoalsTab tab;
    GoalsScroll scroll;
    int claim_series;
    int claim_mission;
    float claim_time;
    bool claim_gems;
    Vector2 burst_start;
    float burst_time;
    bool burst_active;
    bool burst_gems;
    bool scrim_armed;
} MissionsScreen;

void MissionsInitialize(MissionsScreen *s) {
    memset(s, 0, sizeof(*s));
    s->font = GuiFont(FNT_COOL);
    s->body = GuiFont(FNT_MENU);
    s->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&s->close, SND_UI_POPUP_CLOSE);
    s->claim = ButtonLoadText(
        GUI_MISSION_CLAIM,
        GUI_MISSION_CLAIM_ON,
        LocGetOr("LEADERBOARD_MISSIONS_CLAIM", "Получ."),
        s->font,
        (Vector2){0.5f, 0.5f},
        0.5f
    );
    ButtonSetClickSound(&s->claim, SND_MISSION_COLLECT);
    s->claim_series = MISSION_NONE;
    GoalsTabsInitialize(&s->tabs);
}

static void MissionsSelectTab(MissionsScreen *s, Player *p, GoalsTab tab) {
    s->tab = tab;
    GoalsScrollReset(&s->scroll);
    if (tab == GOALS_TAB_TROPHIES) {
        GoalsRefreshLeaderboard(&s->tabs, p, (long long)time(NULL));
        p->pvp.trophies_seen = p->pvp.trophies;
    }
}

void MissionsOpen(MissionsScreen *s, Player *p, GoalsTab tab) {
    PvpEventSync(&p->pvp, (long long)time(NULL));
    if (PvpEventUnclaimedCount(&p->pvp) > 0 && tab == GOALS_TAB_MISSIONS) tab = GOALS_TAB_EVENTS;
    PlayerMissions *m = &p->missions;
    int count = 0;
    for (int pass = 0; pass < 2; pass++) {
        for (int series = 0; series < MissionSeriesCount(); series++) {
            if (MissionSeriesNeedsCollection(m, series) == (pass == 0)) s->order[count++] = series;
        }
    }
    s->claim_series = MISSION_NONE;
    MissionMarkAllSeen(m);
    MissionsSelectTab(s, p, tab);
}

static Texture2D MissionsIcon(MissionsScreen *s, int mission) {
    if (!s->icon_loaded[mission]) {
        s->icon_loaded[mission] = true;
        s->icons[mission] = ResourceLoadTexture(
            TextFormat("%stex.ico_achievement_%s", CACHE_RESOURCE_PREFIX, MissionAt(mission)->icon)
        );
        SetTextureFilter(s->icons[mission], TEXTURE_FILTER_BILINEAR);
    }
    return s->icons[mission];
}

static void MissionsDrawFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;
    float scale = fminf(dst.width / texture.width, dst.height / texture.height);
    float width = texture.width * scale, height = texture.height * scale;
    UiDrawTexture(
        texture,
        (Rectangle){dst.x + (dst.width - width) * 0.5f, dst.y + (dst.height - height) * 0.5f, width, height},
        tint
    );
}

static Rectangle MissionsRewardIconRect(Rectangle row, bool above_claim_button) {
    float y = above_claim_button ? MISSIONS_REWARD_TOP_INSET : (row.height - MISSIONS_REWARD_ICON_SIZE) * 0.5f;
    return (Rectangle){
        row.x + row.width - MISSIONS_REWARD_ICON_SIZE - 6.0f,
        row.y + y,
        MISSIONS_REWARD_ICON_SIZE,
        MISSIONS_REWARD_ICON_SIZE,
    };
}

static void MissionsBeginClaim(MissionsScreen *s, Hud *hud, Player *p, int series, Rectangle row) {
    int mission = MissionSeriesCurrent(&p->missions, series);
    bool gems = MissionAt(mission)->gems > 0;
    HudCurrencyHold(hud, gems);
    if (!MissionsClaim(p, mission)) {
        HudCurrencyRelease(hud, p, gems);
        return;
    }
    Rectangle icon = MissionsRewardIconRect(row, true);
    s->claim_series = series;
    s->claim_mission = mission;
    s->claim_time = 0.0f;
    s->claim_gems = gems;
    s->burst_start = (Vector2){icon.x + icon.width * 0.5f, icon.y + icon.height * 0.5f};
    ProgressionSave(p);
}

static void MissionsFinishClaim(MissionsScreen *s, Hud *hud, const Player *p) {
    s->claim_series = MISSION_NONE;
    s->burst_active = true;
    s->burst_gems = s->claim_gems;
    s->burst_time = 0.0f;
    HudCurrencyRelease(hud, p, s->claim_gems);
}

void MissionsClose(MissionsScreen *s, Hud *hud, const Player *p) {
    if (s->claim_series != MISSION_NONE) MissionsFinishClaim(s, hud, p);
    s->burst_active = false;
}

static void MissionsDrawBurst(MissionsScreen *s, float dt) {
    if (!s->burst_active) return;
    s->burst_time += dt;
    if (s->burst_time >= MISSIONS_BURST_TIME) {
        s->burst_active = false;
        return;
    }
    BattleResultsDrawCurrencyParticles(
        GuiTex(s->burst_gems ? GUI_STORE_CURRENCY_GEMS : GUI_HUD_GOLD),
        s->burst_start,
        HudCurrencyTarget(s->burst_gems),
        s->burst_time,
        MISSIONS_BURST_FADE
    );
}

static Texture2D MissionsCurrencyIcon(const MissionDef *def) {
    return GuiTex(def->gems > 0 ? GUI_MISSION_GEM : GUI_MISSION_GOLD);
}

static float MissionsFlashAlpha(float time) {
    if (time < MISSIONS_FLASH_IN_TIME) return time / MISSIONS_FLASH_IN_TIME;
    return Clamp(1.0f - (time - MISSIONS_FLASH_IN_TIME) / MISSIONS_FLASH_OUT_TIME, 0.0f, 1.0f);
}

static void MissionsDrawProgress(const MissionsScreen *s, long long value, long long target, Rectangle bar) {
    if (value > target) value = target;
    UiDrawTexture(GuiTex(GUI_MISSION_PROGRESS), bar, WHITE);
    Rectangle trough =
        UiRect(bar, 0.5f, 0.5f, bar.width * MISSIONS_BAR_INNER_WIDTH, bar.height * MISSIONS_BAR_INNER_HEIGHT);
    Rectangle fill = trough;
    fill.width *= (float)value / (float)target;
    UiDrawTexture(GuiTex(GUI_MISSION_PROGRESS_FILL), fill, WHITE);
    UiDrawTextCentered(s->body, TextFormat("%lld/%lld", value, target), bar, 15.0f, MISSIONS_TEXT);
}

static void MissionsDescription(char *out, size_t size, const PlayerMissions *m, int mission) {
    const MissionDef *def = MissionAt(mission);
    char id[MISSION_ID_CAP], quantity[24];
    StrCopy(id, sizeof(id), def->id);
    id[0] = 'A';
    snprintf(quantity, sizeof(quantity), "%lld", def->target);
    const char *state = MissionCompleted(m, mission) ? "UNLOCKED" : "LOCKED";
    LocFormat2(out, size, TextFormat("ACHIEVEMENT_%s_%s_DESC", id, state), quantity, "");
}

static bool MissionsDrawRow(
    MissionsScreen *s, const PlayerMissions *m, int series, Rectangle row, Rectangle clip, bool interactive
) {
    bool animating = s->claim_series == series;
    int mission =
        animating && s->claim_time < MISSIONS_FLASH_IN_TIME ? s->claim_mission : MissionSeriesCurrent(m, series);
    const MissionDef *def = MissionAt(mission);
    bool needs_collection = MissionSeriesNeedsCollection(m, series) && !animating;

    UiDrawTexture(GuiTex(GUI_MISSION_ROW), row, WHITE);
    float icon_size = row.height * 0.8f;
    Rectangle icon = {row.x + 14.0f, row.y + (row.height - icon_size) * 0.5f, icon_size, icon_size};
    MissionsDrawFit(MissionsIcon(s, mission), icon, WHITE);

    float text_x = icon.x + icon.width + 12.0f;
    float reward_width = row.width * 0.2f;
    float text_width = row.x + row.width - reward_width - text_x;
    char id[MISSION_ID_CAP];
    StrCopy(id, sizeof(id), def->id);
    id[0] = 'A';
    UiDrawTextLeftMiddle(
        s->font,
        LocGet(TextFormat("ACHIEVEMENT_%s_TITLE", id)),
        (Rectangle){text_x, row.y + 6.0f, text_width, 26.0f},
        23.0f,
        MISSIONS_TEXT
    );
    char description[MISSIONS_TEXT_SIZE];
    MissionsDescription(description, sizeof(description), m, mission);
    HeroesDrawRich(
        s->body, description, (Rectangle){text_x, row.y + 32.0f, text_width, 40.0f}, 16.0f, MISSIONS_TEXT_DIM
    );

    Rectangle footer = {text_x, row.y + row.height - 28.0f, text_width * 0.72f, 22.0f};
    bool show_progress = !needs_collection && def->target > 1;
    if (show_progress) MissionsDrawProgress(s, m->progress[mission], def->target, footer);

    const char *amount = TextFormat("+%d", def->gems > 0 ? def->gems : def->gold);
    Rectangle reward_icon = MissionsRewardIconRect(row, needs_collection);
    Rectangle reward = {
        row.x + row.width - reward_width,
        reward_icon.y,
        reward_icon.x - 8.0f - (row.x + row.width - reward_width),
        reward_icon.height,
    };
    UiDrawTextRight(s->font, amount, reward, MISSIONS_REWARD_FONT_SIZE, MISSIONS_TEXT);
    MissionsDrawFit(MissionsCurrencyIcon(def), reward_icon, WHITE);

    bool claimed = false;
    if (needs_collection) {
        Rectangle button = {
            row.x + row.width - reward_width + 4.0f, row.y + row.height - 44.0f, reward_width - 12.0f, 38.0f
        };
        Vector2 mouse = UiMouse();
        bool visible = CheckCollisionPointRec(mouse, clip);
        claimed = ButtonDrawEx(&s->claim, button, interactive && visible && !s->scroll.drag_scrolled);
    }
    if (animating) DrawRectangleRec(row, Fade(WHITE, MissionsFlashAlpha(s->claim_time)));
    return claimed;
}

static void MissionsDrawSummaryEntry(const MissionsScreen *s, Texture2D icon, const char *text, Rectangle area) {
    MissionsDrawFit(icon, (Rectangle){area.x, area.y + 8.0f, area.height - 16.0f, area.height - 16.0f}, WHITE);
    UiDrawTextLeftMiddle(
        s->body,
        text,
        (Rectangle){area.x + area.height - 8.0f, area.y, area.width - area.height + 8.0f, area.height},
        20.0f,
        MISSIONS_TEXT
    );
}

static void MissionsDrawSummary(const MissionsScreen *s, const PlayerMissions *m, Rectangle bar) {
    MissionTotals totals = MissionComputeTotals(m);
    DrawRectangleRec(bar, (Color){0, 0, 0, 120});
    float third = bar.width / 3.0f;
    Rectangle slot = {bar.x + 16.0f, bar.y, third - 16.0f, bar.height};
    MissionsDrawSummaryEntry(s, GuiTex(GUI_MISSION_STAR), TextFormat("%d/%d", totals.claimed, totals.total), slot);
    slot.x += third;
    MissionsDrawSummaryEntry(
        s, GuiTex(GUI_MISSION_GOLD), TextFormat("%d/%d", totals.gold_claimed, totals.gold_total), slot
    );
    slot.x += third;
    MissionsDrawSummaryEntry(
        s, GuiTex(GUI_MISSION_GEM), TextFormat("%d/%d", totals.gems_claimed, totals.gems_total), slot
    );
}

static void MissionsDrawAchievements(MissionsScreen *s, Hud *hud, Player *p, Rectangle content, bool interactive) {
    Rectangle list = {content.x, content.y, content.width, content.height - MISSIONS_SUMMARY_HEIGHT - 8.0f};
    Rectangle summary = {list.x, list.y + list.height + 8.0f, list.width, MISSIONS_SUMMARY_HEIGHT};
    float stride = MISSIONS_ROW_HEIGHT + MISSIONS_ROW_GAP;
    GoalsScrollUpdate(&s->scroll, list, MissionSeriesCount() * stride - MISSIONS_ROW_GAP);

    GoalsBeginClip(list);
    int claimed_series = MISSION_NONE;
    Rectangle claimed_row = {0};
    for (int i = 0; i < MissionSeriesCount(); i++) {
        float y = list.y + i * stride - s->scroll.offset;
        if (y + MISSIONS_ROW_HEIGHT < list.y || y > list.y + list.height) continue;
        Rectangle row = {list.x, y, list.width, MISSIONS_ROW_HEIGHT};
        if (MissionsDrawRow(s, &p->missions, s->order[i], row, list, interactive)) {
            claimed_series = s->order[i];
            claimed_row = row;
        }
    }
    EndScissorMode();

    if (claimed_series != MISSION_NONE) MissionsBeginClaim(s, hud, p, claimed_series, claimed_row);
    MissionsDrawSummary(s, &p->missions, summary);
}

MissionsAction MissionsDraw(MissionsScreen *s, Hud *hud, Player *p, float dt) {
    Rectangle root = UiRoot();
    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 170});
    Rectangle panel = UiRect(root, 0.5f, 0.6f, 940.0f, 597.0f);
    UiDrawTexture(GuiTex(GUI_QUESTS_BG), panel, WHITE);
    UiDrawTextCentered(
        s->font,
        LocGetOr("LEADERBOARD_TITLE", "Цели"),
        (Rectangle){0.0f, panel.y + 38.0f, UiWidth(), 0.0f},
        42.0f,
        MISSIONS_TEXT
    );

    if (s->claim_series != MISSION_NONE) {
        s->claim_time += dt;
        if (s->claim_time >= MISSIONS_CLAIM_TIME) MissionsFinishClaim(s, hud, p);
    }

    bool interactive = !UiInputBlocked() && s->claim_series == MISSION_NONE;
    Rectangle bar = {
        panel.x + MISSIONS_TABS_INSET_X, panel.y + MISSIONS_TABS_TOP, MISSIONS_TABS_WIDTH, MISSIONS_TABS_HEIGHT
    };
    GoalsTab pressed_tab = GoalsDrawTabBar(&s->tabs, s->tab, bar, interactive);
    if (pressed_tab != s->tab) MissionsSelectTab(s, p, pressed_tab);
    Rectangle content = {
        panel.x + MISSIONS_CONTENT_INSET_X,
        bar.y + bar.height + MISSIONS_CONTENT_GAP,
        panel.width - MISSIONS_CONTENT_INSET_X * 2.0f,
        MISSIONS_CONTENT_HEIGHT,
    };

    MissionsAction action = MISSIONS_NONE;
    GoalsAction goal = GOALS_NONE;
    long long now = (long long)time(NULL);
    if (s->tab == GOALS_TAB_MISSIONS) {
        MissionsDrawAchievements(s, hud, p, content, interactive);
    } else if (s->tab == GOALS_TAB_TROPHIES) {
        goal = GoalsDrawTrophies(&s->tabs, &s->scroll, p, content, interactive);
    } else if (s->tab == GOALS_TAB_BATTLES) {
        goal = GoalsDrawBattles(&s->tabs, &s->scroll, &p->pvp, content, interactive, now);
    } else {
        goal = GoalsDrawEvents(&s->tabs, &s->scroll, &p->pvp, content, interactive);
    }
    if (goal == GOALS_CLAIM_EVENT && PvpEventClaim(p, s->tabs.selected)) ProgressionSave(p);
    if (goal == GOALS_FIGHT_BOT) action = MISSIONS_FIGHT_BOT;
    if (goal == GOALS_REVENGE) action = MISSIONS_REVENGE;

    float close_size = 61.0f;
    Rectangle close = {
        panel.x + panel.width - close_size * 1.25f, panel.y + panel.height * 0.07f, close_size, close_size
    };
    if (ButtonDrawEx(&s->close, close, !UiInputBlocked())) action = MISSIONS_CLOSE;

    MissionsDrawBurst(s, dt);

    Vector2 mouse = UiMouse();
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close) &&
                       !CheckCollisionPointRec(mouse, top_ui);
    if (UiInputBlocked()) {
        s->scrim_armed = false;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        s->scrim_armed = scrim_hover;
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (s->scrim_armed && scrim_hover && action == MISSIONS_NONE) {
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            action = MISSIONS_CLOSE;
        }
        s->scrim_armed = false;
    }
    return action;
}
