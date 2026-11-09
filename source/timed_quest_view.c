#define TIMED_QUEST_VIEW_HEADER_HEIGHT 44.0f
#define TIMED_QUEST_VIEW_TRACK_HEIGHT 146.0f
#define TIMED_QUEST_VIEW_TRACK_GAP 8.0f
#define TIMED_QUEST_VIEW_CHEST_TOP 4.0f
#define TIMED_QUEST_VIEW_BAR_FROM_FOOT 46.0f
#define TIMED_QUEST_VIEW_ROW_HEIGHT 92.0f
#define TIMED_QUEST_VIEW_ROW_GAP 6.0f
#define TIMED_QUEST_VIEW_SECTION_GAP 22.0f
#define TIMED_QUEST_VIEW_CHEST_SIZE 74.0f
#define TIMED_QUEST_VIEW_ICON_SIZE 70.0f
#define TIMED_QUEST_VIEW_BUTTON_WIDTH 124.0f
#define TIMED_QUEST_VIEW_BUTTON_HEIGHT 40.0f
#define TIMED_QUEST_VIEW_REWARD_ICON_SIZE 34.0f
#define TIMED_QUEST_VIEW_STAR_SIZE 26.0f
#define TIMED_QUEST_VIEW_ROW_INSET 24.0f
#define TIMED_QUEST_VIEW_REWARD_TOP 10.0f
#define TIMED_QUEST_VIEW_ICON_GAP 5.0f
#define TIMED_QUEST_VIEW_GROUP_GAP 22.0f
#define TIMED_QUEST_VIEW_AMOUNT_SIZE 26.0f
#define TIMED_QUEST_VIEW_POINTS_SIZE 22.0f
#define TIMED_QUEST_VIEW_TRACK_INSET 150.0f
#define TIMED_QUEST_VIEW_TRACK_TRAIL 48.0f
#define TIMED_QUEST_VIEW_BAR_HEIGHT 24.0f
#define TIMED_QUEST_VIEW_PULSE_SPEED 5.0f
#define TIMED_QUEST_VIEW_PULSE_SCALE 0.07f
#define TIMED_QUEST_VIEW_TEXT_SIZE 256

typedef enum {
    TIMED_QUEST_VIEW_NONE,
    TIMED_QUEST_VIEW_PAID,
} TimedQuestViewKind;

typedef struct {
    TimedQuestViewKind kind;
    Vector2 from;
    bool gems;
} TimedQuestViewAction;

static const Color TIMED_QUEST_VIEW_TEXT = {235, 232, 220, 255};
static const Color TIMED_QUEST_VIEW_TEXT_DIM = {182, 178, 168, 255};
static const Color TIMED_QUEST_VIEW_GOLD = {255, 214, 120, 255};
static const Color TIMED_QUEST_VIEW_LOCKED = {120, 120, 120, 255};
static const Color TIMED_QUEST_VIEW_SHADE = {0, 0, 0, 120};
static const Color TIMED_QUEST_VIEW_GLOW = {255, 196, 90, 150};

typedef struct {
    Font font;
    Font body;
    Button claim;
    Button swap;
    Button ok;
    Texture2D check;
    Texture2D stat_icons[TIMED_QUEST_STAT_COUNT];
    bool stat_icon_loaded[TIMED_QUEST_STAT_COUNT];
    Texture2D chest_art[TIMED_QUEST_PERIOD_COUNT][TIMED_QUEST_CHEST_CAPACITY];
    bool chest_art_loaded[TIMED_QUEST_PERIOD_COUNT][TIMED_QUEST_CHEST_CAPACITY];
    bool chest_waiting;
    Vector2 chest_from;
    ExpeditionReward reward;
    bool reveal_pending;
    bool burst_pending;
    bool message_open;
    char message[TIMED_QUEST_VIEW_TEXT_SIZE];
} TimedQuestView;

void TimedQuestViewInitialize(TimedQuestView *v) {
    memset(v, 0, sizeof(*v));
    v->font = GuiFont(FNT_COOL);
    v->body = GuiFont(FNT_MENU);
    Vector2 centre = {0.5f, 0.5f};
    v->claim = ButtonLoadText(
        GUI_MISSION_CLAIM, GUI_MISSION_CLAIM_ON, LocGet("LEADERBOARD_MISSIONS_CLAIM"), v->font, centre, 0.5f
    );
    ButtonSetClickSound(&v->claim, SND_MISSION_COLLECT);
    v->swap = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_QUESTS_SWAP"), v->font, centre, 0.4f);
    ButtonSetClickSound(&v->swap, SND_UI_TAB);
    v->ok = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), v->font, centre, 0.32f);
    v->check = LoadSprite(CACHE_RESOURCE("Ico_InUseCheck"));
}

bool TimedQuestViewBlocking(const TimedQuestView *v) {
    return v->chest_waiting || v->message_open;
}

static Texture2D TimedQuestViewLoadArt(const char *art, const char *fallback) {
    const char *extra = TextFormat(EXTRA_ASSET_DIR "%s", art);
    bool use_extra = art[0] && ResourceExists(extra);
    Texture2D texture = ResourceLoadTexture(use_extra ? extra : fallback);
    if (texture.id) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    }
    return texture;
}

static Texture2D TimedQuestViewStatIcon(TimedQuestView *v, TimedQuestStat stat) {
    if (!v->stat_icon_loaded[stat]) {
        const TimedQuestStatText *text = TimedQuestText(stat);
        v->stat_icon_loaded[stat] = true;
        v->stat_icons[stat] = TimedQuestViewLoadArt(text->art, text->icon);
    }
    return v->stat_icons[stat];
}

static Texture2D TimedQuestViewChestArt(TimedQuestView *v, TimedQuestPeriod period, int index) {
    if (!v->chest_art_loaded[period][index]) {
        const TimedQuestChest *chest = TimedQuestChestAt(period, index);
        v->chest_art_loaded[period][index] = true;
        v->chest_art[period][index] = TimedQuestViewLoadArt(chest->art, chest->fallback);
    }
    return v->chest_art[period][index];
}

static void TimedQuestViewDrawFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;
    float scale = fminf(dst.width / texture.width, dst.height / texture.height);
    float width = texture.width * scale, height = texture.height * scale;
    UiDrawTexture(
        texture,
        (Rectangle){dst.x + (dst.width - width) * 0.5f, dst.y + (dst.height - height) * 0.5f, width, height},
        tint
    );
}

static void TimedQuestViewShowMessage(TimedQuestView *v, const char *text) {
    StrCopy(v->message, sizeof(v->message), text);
    v->message_open = true;
}

static float TimedQuestViewSectionHeight(int rows) {
    return TIMED_QUEST_VIEW_HEADER_HEIGHT + TIMED_QUEST_VIEW_TRACK_HEIGHT +
           rows * (TIMED_QUEST_VIEW_ROW_HEIGHT + TIMED_QUEST_VIEW_ROW_GAP);
}

static void TimedQuestViewDrawHeader(const TimedQuestView *v, TimedQuestPeriod period, Rectangle header) {
    const char *title = LocGet(period == TIMED_QUEST_DAILY ? "HODA_QUESTS_DAILY_TITLE" : "HODA_QUESTS_WEEKLY_TITLE");
    UiDrawTextLeftMiddle(
        v->font, title, (Rectangle){header.x + 8.0f, header.y, 360.0f, header.height}, 26.0f, TIMED_QUEST_VIEW_TEXT
    );
    char time_text[48], reset[TIMED_QUEST_VIEW_TEXT_SIZE];
    QuestsShortTime(time_text, sizeof(time_text), TimedQuestResetIn(period, ServerNow()));
    LocFormat1(reset, sizeof(reset), "HODA_QUESTS_RESETS_IN", time_text);
    UiDrawTextRight(
        v->body,
        reset,
        (Rectangle){header.x, header.y, header.width - 8.0f, header.height},
        18.0f,
        TIMED_QUEST_VIEW_TEXT_DIM
    );
}

static void TimedQuestViewDrawBar(Rectangle bar, float fraction) {
    UiDrawTexture(GuiTex(GUI_MISSION_PROGRESS), bar, WHITE);
    Rectangle fill = UiRect(bar, 0.5f, 0.5f, bar.width * GOALS_BAR_INNER_WIDTH, bar.height * GOALS_BAR_INNER_HEIGHT);
    fill.width *= Clamp(fraction, 0.0f, 1.0f);
    UiDrawTexture(GuiTex(GUI_MISSION_PROGRESS_FILL), fill, WHITE);
}

static bool TimedQuestViewDrawChest(
    TimedQuestView *v, const Player *p, TimedQuestPeriod period, int index, Vector2 centre, bool interactive
) {
    const PlayerTimedQuests *q = &p->timed_quests;
    bool opened = q->chests_opened[period][index];
    bool openable = TimedQuestChestOpenable(q, period, index);
    float size = TIMED_QUEST_VIEW_CHEST_SIZE;
    if (openable) {
        size *= 1.0f + TIMED_QUEST_VIEW_PULSE_SCALE * sinf((float)GetTime() * TIMED_QUEST_VIEW_PULSE_SPEED);
        DrawCircleGradient(
            (int)centre.x, (int)centre.y, size * 0.62f, TIMED_QUEST_VIEW_GLOW, Fade(TIMED_QUEST_VIEW_GLOW, 0.0f)
        );
    }
    Rectangle art = {centre.x - size * 0.5f, centre.y - size * 0.5f, size, size};
    Color tint = openable ? WHITE : opened ? (Color){150, 150, 150, 255} : TIMED_QUEST_VIEW_LOCKED;
    TimedQuestViewDrawFit(TimedQuestViewChestArt(v, period, index), art, tint);
    if (opened) {
        float mark = size * 0.46f;
        TimedQuestViewDrawFit(
            v->check, (Rectangle){art.x + art.width - mark * 0.8f, art.y + art.height - mark, mark, mark}, WHITE
        );
    }
    bool hover = interactive && CheckCollisionPointRec(UiMouse(), art);
    if (openable && hover) DrawRectangleLinesEx(art, 2.0f, TIMED_QUEST_VIEW_GOLD);
    return openable && hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static int TimedQuestViewDrawTrack(
    TimedQuestView *v, const Player *p, TimedQuestPeriod period, Rectangle track, bool interactive
) {
    DrawRectangleRec(track, TIMED_QUEST_VIEW_SHADE);
    int points = TimedQuestPoints(&p->timed_quests, period);
    int max_points = TimedQuestMaxPoints(period);
    Rectangle star = {track.x + 18.0f, track.y + (track.height - 40.0f) * 0.5f, 40.0f, 40.0f};
    TimedQuestViewDrawFit(GuiTex(GUI_MISSION_STAR), star, WHITE);
    UiDrawTextLeftMiddle(
        v->font,
        TextFormat("%d/%d", points, max_points),
        (Rectangle){star.x + star.width + 8.0f, track.y, TIMED_QUEST_VIEW_TRACK_INSET - 70.0f, track.height},
        24.0f,
        TIMED_QUEST_VIEW_TEXT
    );
    float bar_x = track.x + TIMED_QUEST_VIEW_TRACK_INSET;
    float bar_width = track.width - TIMED_QUEST_VIEW_TRACK_INSET - TIMED_QUEST_VIEW_TRACK_TRAIL;
    Rectangle bar = {
        bar_x, track.y + track.height - TIMED_QUEST_VIEW_BAR_FROM_FOOT, bar_width, TIMED_QUEST_VIEW_BAR_HEIGHT
    };
    TimedQuestViewDrawBar(bar, max_points > 0 ? (float)points / (float)max_points : 0.0f);
    int clicked = TIMED_QUEST_NONE;
    for (int i = 0; i < TimedQuestChestCount(period); i++) {
        const TimedQuestChest *chest = TimedQuestChestAt(period, i);
        float x = bar.x + bar.width * (float)chest->points / (float)max_points;
        Vector2 centre = {x, track.y + TIMED_QUEST_VIEW_CHEST_TOP + TIMED_QUEST_VIEW_CHEST_SIZE * 0.5f};
        if (TimedQuestViewDrawChest(v, p, period, i, centre, interactive)) {
            clicked = i;
            v->chest_from = centre;
        }
        UiDrawTextCentered(
            v->body,
            TextFormat("%d", chest->points),
            (Rectangle){x - 30.0f, bar.y + bar.height + 2.0f, 60.0f, 18.0f},
            16.0f,
            TIMED_QUEST_VIEW_TEXT_DIM
        );
    }
    if (v->chest_waiting)
        UiDrawTextCentered(
            v->body,
            LocGet("HODA_QUESTS_OPENING"),
            (Rectangle){bar.x, track.y, bar.width, 20.0f},
            18.0f,
            TIMED_QUEST_VIEW_GOLD
        );
    return clicked;
}

typedef enum { TIMED_QUEST_ROW_NONE, TIMED_QUEST_ROW_CLAIM, TIMED_QUEST_ROW_SWAP } TimedQuestRowPress;

static TimedQuestRowPress TimedQuestViewDrawRow(
    TimedQuestView *v,
    const TimedQuestDef *def,
    long long progress,
    bool claimed,
    bool claimable,
    bool swappable,
    Rectangle row,
    bool interactive,
    Rectangle *reward_icon
) {
    const TimedQuestStatText *text = TimedQuestText(def->stat);
    UiDrawTexture(GuiTex(GUI_MISSION_ROW), row, claimed ? (Color){170, 170, 170, 255} : WHITE);
    Rectangle icon = {
        row.x + 12.0f,
        row.y + (row.height - TIMED_QUEST_VIEW_ICON_SIZE) * 0.5f,
        TIMED_QUEST_VIEW_ICON_SIZE,
        TIMED_QUEST_VIEW_ICON_SIZE
    };
    TimedQuestViewDrawFit(TimedQuestViewStatIcon(v, def->stat), icon, claimed ? (Color){170, 170, 170, 255} : WHITE);

    float right_width = 250.0f;
    float text_x = icon.x + icon.width + 12.0f;
    float text_width = row.x + row.width - right_width - text_x;
    UiDrawTextLeftMiddle(
        v->font,
        LocGet(text->title_key),
        (Rectangle){text_x, row.y + 6.0f, text_width, 26.0f},
        22.0f,
        TIMED_QUEST_VIEW_TEXT
    );
    char description[TIMED_QUEST_VIEW_TEXT_SIZE];
    LocFormat1(description, sizeof(description), text->description_key, TextFormat("%lld", def->target));
    UiDrawTextLeftMiddle(
        v->body, description, (Rectangle){text_x, row.y + 32.0f, text_width, 22.0f}, 16.0f, TIMED_QUEST_VIEW_TEXT_DIM
    );
    Rectangle bar = {text_x, row.y + row.height - 30.0f, fminf(text_width * 0.8f, 260.0f), 22.0f};
    TimedQuestViewDrawBar(bar, (float)progress / (float)def->target);
    UiDrawTextCentered(v->body, TextFormat("%lld/%lld", progress, def->target), bar, 15.0f, TIMED_QUEST_VIEW_TEXT);

    float right = row.x + row.width - TIMED_QUEST_VIEW_ROW_INSET;
    Rectangle line = {row.x, row.y + TIMED_QUEST_VIEW_REWARD_TOP, 0.0f, TIMED_QUEST_VIEW_REWARD_ICON_SIZE};
    Rectangle star = {
        right - TIMED_QUEST_VIEW_STAR_SIZE,
        line.y + (line.height - TIMED_QUEST_VIEW_STAR_SIZE) * 0.5f,
        TIMED_QUEST_VIEW_STAR_SIZE,
        TIMED_QUEST_VIEW_STAR_SIZE,
    };
    TimedQuestViewDrawFit(GuiTex(GUI_MISSION_STAR), star, WHITE);
    char points[16];
    snprintf(points, sizeof(points), "+%d", def->points);
    float points_width = MeasureTextEx(v->font, points, TIMED_QUEST_VIEW_POINTS_SIZE, 1.0f).x;
    float points_x = star.x - TIMED_QUEST_VIEW_ICON_GAP - points_width;
    UiDrawTextRight(
        v->font,
        points,
        (Rectangle){points_x, line.y, points_width, line.height},
        TIMED_QUEST_VIEW_POINTS_SIZE,
        TIMED_QUEST_VIEW_TEXT
    );
    bool gems = def->gems > 0;
    *reward_icon = (Rectangle){
        points_x - TIMED_QUEST_VIEW_GROUP_GAP - TIMED_QUEST_VIEW_REWARD_ICON_SIZE,
        line.y,
        TIMED_QUEST_VIEW_REWARD_ICON_SIZE,
        TIMED_QUEST_VIEW_REWARD_ICON_SIZE,
    };
    TimedQuestViewDrawFit(GuiTex(gems ? GUI_MISSION_GEM : GUI_MISSION_GOLD), *reward_icon, WHITE);
    char amount[16];
    snprintf(amount, sizeof(amount), "+%d", gems ? def->gems : def->gold);
    float amount_width = MeasureTextEx(v->font, amount, TIMED_QUEST_VIEW_AMOUNT_SIZE, 1.0f).x;
    UiDrawTextRight(
        v->font,
        amount,
        (Rectangle){reward_icon->x - TIMED_QUEST_VIEW_ICON_GAP - amount_width, line.y, amount_width, line.height},
        TIMED_QUEST_VIEW_AMOUNT_SIZE,
        TIMED_QUEST_VIEW_TEXT
    );

    Rectangle button = {
        right - TIMED_QUEST_VIEW_BUTTON_WIDTH,
        row.y + row.height - TIMED_QUEST_VIEW_BUTTON_HEIGHT - 6.0f,
        TIMED_QUEST_VIEW_BUTTON_WIDTH,
        TIMED_QUEST_VIEW_BUTTON_HEIGHT,
    };
    if (claimed) {
        TimedQuestViewDrawFit(v->check, (Rectangle){button.x, button.y, 34.0f, button.height}, WHITE);
        UiDrawTextLeftMiddle(
            v->body,
            LocGet("PVP_EVENT_DONE"),
            (Rectangle){button.x + 40.0f, button.y, 84.0f, button.height},
            19.0f,
            TIMED_QUEST_VIEW_TEXT_DIM
        );
        return TIMED_QUEST_ROW_NONE;
    }
    if (claimable) return ButtonDrawEx(&v->claim, button, interactive) ? TIMED_QUEST_ROW_CLAIM : TIMED_QUEST_ROW_NONE;
    if (swappable) return ButtonDrawEx(&v->swap, button, interactive) ? TIMED_QUEST_ROW_SWAP : TIMED_QUEST_ROW_NONE;
    return TIMED_QUEST_ROW_NONE;
}

static void TimedQuestViewShowReward(TimedQuestView *v) {
    v->reveal_pending = ExpeditionRewardHasItems(&v->reward);
    v->burst_pending = v->reward.gold > 0 || v->reward.gems > 0;
}

static void TimedQuestViewOpenChest(TimedQuestView *v, Heroes *heroes, Player *p, TimedQuestPeriod period, int index) {
    TimedQuestChestCheck check = TimedQuestCheckChest(p, period, index);
    if (check == TIMED_QUEST_CHEST_ROSTER_FULL) {
        TimedQuestViewShowMessage(v, LocGet("HODA_QUESTS_ROSTER_FULL"));
        return;
    }
    if (check != TIMED_QUEST_CHEST_OK) return;
    if (ServerActionsActive()) {
        v->chest_waiting = true;
        ProgressionSave(p);
        ServerActionSubmitTagged(TimedQuestChestAction(period, index), SERVER_REPLY_QUEST_CHEST);
        return;
    }
    v->reward = TimedQuestOpenChest(p, period, index, ServerNow());
    if (!HeroesSyncRoster(heroes, p)) TraceLog(LOG_WARNING, "Quests: granted heroes cannot sync to Heroes");
    ProgressionSave(p);
    TimedQuestViewShowReward(v);
}

void TimedQuestViewTakeChestReply(TimedQuestView *v, Heroes *heroes, const Player *p, const cJSON *reply) {
    v->chest_waiting = false;
    if (!reply) {
        TimedQuestViewShowMessage(v, LocGet("HODA_QUESTS_CHEST_FAILED"));
        return;
    }
    if (!HeroesSyncRoster(heroes, p)) TraceLog(LOG_WARNING, "Quests: granted heroes cannot sync to Heroes");
    v->reward = ExpeditionRewardFromReply(p, reply, 0, 0);
    TimedQuestViewShowReward(v);
}

bool TimedQuestViewRevealRewards(TimedQuestView *v, Store *store) {
    if (!v->reveal_pending) return false;
    v->reveal_pending = false;
    const ExpeditionReward *r = &v->reward;
    bool gear_new[EXPEDITION_REWARD_ITEM_CAPACITY];
    for (int i = 0; i < r->gear_count; i++) gear_new[i] = true;
    return StoreRevealRewards(
        store, r->heroes, r->hero_count, r->gear, gear_new, r->gear_count, r->runes, r->rune_new, r->rune_count
    );
}

static TimedQuestViewAction TimedQuestViewClaim(Player *p, TimedQuestPeriod period, int index, Rectangle reward_icon) {
    TimedQuestViewAction action = {0};
    const TimedQuestDef *def =
        period == TIMED_QUEST_DAILY ? TimedQuestDailyAt(&p->timed_quests, index) : TimedQuestWeeklyAt(index);
    cJSON *fields = TimedQuestClaimAction(def);
    long long now = ServerNow();
    bool paid =
        period == TIMED_QUEST_DAILY ? TimedQuestClaimDaily(p, index, now) : TimedQuestClaimWeekly(p, index, now);
    if (!paid) {
        cJSON_Delete(fields);
        return action;
    }
    ProgressionSave(p);
    ServerActionSubmit(fields);
    action.kind = TIMED_QUEST_VIEW_PAID;
    action.gems = def->gems > 0;
    action.from = (Vector2){reward_icon.x + reward_icon.width * 0.5f, reward_icon.y + reward_icon.height * 0.5f};
    return action;
}

static void TimedQuestViewSwap(Player *p, int slot) {
    cJSON *fields = TimedQuestSwapAction(slot);
    if (!TimedQuestSwap(p, slot, ServerNow())) {
        cJSON_Delete(fields);
        return;
    }
    ProgressionSave(p);
    ServerActionSubmit(fields);
}

static TimedQuestViewAction TimedQuestViewDrawSection(
    TimedQuestView *v, Heroes *heroes, Player *p, TimedQuestPeriod period, float top, Rectangle list, bool interactive
) {
    TimedQuestViewAction action = {0};
    PlayerTimedQuests *q = &p->timed_quests;
    Rectangle header = {list.x, top, list.width, TIMED_QUEST_VIEW_HEADER_HEIGHT};
    TimedQuestViewDrawHeader(v, period, header);
    Rectangle track = {
        list.x, header.y + header.height, list.width, TIMED_QUEST_VIEW_TRACK_HEIGHT - TIMED_QUEST_VIEW_TRACK_GAP
    };
    int chest = TimedQuestViewDrawTrack(v, p, period, track, interactive);
    if (chest != TIMED_QUEST_NONE) TimedQuestViewOpenChest(v, heroes, p, period, chest);

    int rows = period == TIMED_QUEST_DAILY ? TimedQuestSlotCount() : TimedQuestWeeklyCount();
    float y = header.y + TIMED_QUEST_VIEW_HEADER_HEIGHT + TIMED_QUEST_VIEW_TRACK_HEIGHT;
    for (int i = 0; i < rows; i++, y += TIMED_QUEST_VIEW_ROW_HEIGHT + TIMED_QUEST_VIEW_ROW_GAP) {
        if (y + TIMED_QUEST_VIEW_ROW_HEIGHT < list.y || y > list.y + list.height) continue;
        Rectangle row = {list.x, y, list.width, TIMED_QUEST_VIEW_ROW_HEIGHT};
        bool daily = period == TIMED_QUEST_DAILY;
        const TimedQuestDef *def = daily ? TimedQuestDailyAt(q, i) : TimedQuestWeeklyAt(i);
        bool claimed = daily ? TimedQuestDailyClaimed(q, i) : q->weekly_claimed[i];
        bool claimable = daily ? TimedQuestDailyClaimable(q, i) : TimedQuestWeeklyClaimable(q, i);
        bool swappable = daily && TimedQuestCanSwap(q, i);
        Rectangle reward_icon = {0};
        TimedQuestRowPress press = TimedQuestViewDrawRow(
            v, def, TimedQuestProgress(q, period, def), claimed, claimable, swappable, row, interactive, &reward_icon
        );
        if (press == TIMED_QUEST_ROW_CLAIM) action = TimedQuestViewClaim(p, period, i, reward_icon);
        if (press == TIMED_QUEST_ROW_SWAP) TimedQuestViewSwap(p, i);
    }
    return action;
}

TimedQuestViewAction TimedQuestViewDraw(
    TimedQuestView *v, GoalsScroll *scroll, Heroes *heroes, Player *p, Rectangle content, bool interactive
) {
    TimedQuestViewAction action = {0};
    if (!TimedQuestsUnlocked(p->level)) {
        char locked[TIMED_QUEST_VIEW_TEXT_SIZE];
        LocFormat1(locked, sizeof(locked), "HODA_QUESTS_LOCKED", TextFormat("%d", TimedQuestUnlockLevel()));
        UiDrawTextCentered(v->body, locked, content, 24.0f, TIMED_QUEST_VIEW_TEXT_DIM);
        return action;
    }
    TimedQuestsRoll(&p->timed_quests, ServerNow());
    float daily_height = TimedQuestViewSectionHeight(TimedQuestSlotCount());
    float weekly_height = TimedQuestViewSectionHeight(TimedQuestWeeklyCount());
    GoalsScrollUpdate(scroll, content, daily_height + TIMED_QUEST_VIEW_SECTION_GAP + weekly_height);
    bool visible = CheckCollisionPointRec(UiMouse(), content);
    bool live = interactive && visible && !scroll->drag_scrolled && !TimedQuestViewBlocking(v);
    GoalsBeginClip(content);
    float top = content.y - scroll->offset;
    TimedQuestViewAction daily = TimedQuestViewDrawSection(v, heroes, p, TIMED_QUEST_DAILY, top, content, live);
    top += daily_height + TIMED_QUEST_VIEW_SECTION_GAP;
    TimedQuestViewAction weekly = TimedQuestViewDrawSection(v, heroes, p, TIMED_QUEST_WEEKLY, top, content, live);
    EndScissorMode();
    action = daily.kind != TIMED_QUEST_VIEW_NONE ? daily : weekly;
    if (action.kind == TIMED_QUEST_VIEW_NONE && v->burst_pending && !v->reveal_pending && !v->chest_waiting) {
        v->burst_pending = false;
        action =
            (TimedQuestViewAction){.kind = TIMED_QUEST_VIEW_PAID, .from = v->chest_from, .gems = v->reward.gems > 0};
    }
    return action;
}

void TimedQuestViewDrawOverlay(TimedQuestView *v) {
    if (!v->message_open) return;
    if (MessagePopupDrawAlert(
            GuiTex(GUI_STORE_OVERLAY), v->font, v->body, LocGet("HODA_QUESTS_TAB"), v->message, &v->ok
        ))
        v->message_open = false;
}
