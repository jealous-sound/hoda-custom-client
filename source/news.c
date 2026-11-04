#define NEWS_MAX_MESSAGES NEXUS_MAX_EVENTS
#define NEWS_LOC_KEY_SIZE 96
#define NEWS_FRAME_X 160.0f
#define NEWS_FRAME_Y 72.0f
#define NEWS_FRAME_W 961.0f
#define NEWS_FRAME_H 648.0f
#define NEWS_TITLE_H 65.0f
#define NEWS_CONTENT_X 242.0f
#define NEWS_CONTENT_W 798.0f
#define NEWS_FEATURED_Y 178.0f
#define NEWS_FEATURED_H 227.0f
#define NEWS_FEATURED_CELL_W 399.0f
#define NEWS_FEATURED_VISIBLE 2
#define NEWS_FEATURED_MARGIN 6.0f
#define NEWS_FEATURED_IMAGE_SIZE 170.0f
#define NEWS_LIST_Y 405.0f
#define NEWS_LIST_H 268.0f
#define NEWS_ROW_H 112.0f
#define NEWS_ROW_ICON_SIZE 92.0f
#define NEWS_ROW_TEXT_X 112.0f
#define NEWS_ROW_TIMER_W 170.0f
#define NEWS_ROW_LINE_H 6.0f
#define NEWS_ARROW_W 82.0f
#define NEWS_ARROW_H 104.0f
#define NEWS_ARROW_Y 232.0f
#define NEWS_ARROW_LEFT_X 160.0f
#define NEWS_ARROW_RIGHT_X 1039.0f
#define NEWS_WHEEL_STEP 60.0f
#define NEWS_TAP_SLOP 12.0f
#define NEWS_BUTTON_X 13.0f
#define NEWS_BUTTON_Y 227.0f
#define NEWS_BUTTON_SIZE 73.0f
#define NEWS_TICKER_W 400.0f
#define NEWS_TICKER_H 104.0f
#define NEWS_TICKER_Y 98.0f
#define NEWS_TICKER_IMAGE_X 52.0f
#define NEWS_TICKER_IMAGE_Y 22.0f
#define NEWS_TICKER_IMAGE_SIZE 60.0f
#define NEWS_TICKER_TEXT_X 120.0f
#define NEWS_TICKER_TEXT_W 232.0f
#define NEWS_TICKER_TITLE_Y 14.0f
#define NEWS_TICKER_TITLE_H 30.0f
#define NEWS_TICKER_TIMER_Y 44.0f
#define NEWS_TICKER_TIMER_H 46.0f
#define NEWS_TICKER_SECONDS 5.0f
#define NEWS_TICKER_FADE_SECONDS 0.25f
#define NEWS_POPUP_TITLE_X 232.0f
#define NEWS_POPUP_TITLE_Y 161.0f
#define NEWS_POPUP_TITLE_W 817.0f
#define NEWS_POPUP_TITLE_H 39.0f
#define NEWS_POPUP_SUBTITLE_Y 201.0f
#define NEWS_POPUP_SUBTITLE_H 65.0f
#define NEWS_POPUP_IMAGE_X 245.0f
#define NEWS_POPUP_IMAGE_Y 281.0f
#define NEWS_POPUP_IMAGE_SIZE 376.0f
#define NEWS_POPUP_MESSAGE_X 651.0f
#define NEWS_POPUP_MESSAGE_Y 286.0f
#define NEWS_POPUP_MESSAGE_W 380.0f
#define NEWS_POPUP_MESSAGE_H 292.0f
#define NEWS_POPUP_CHANGER_H 60.0f
#define NEWS_POPUP_LINK_X 711.0f
#define NEWS_POPUP_LINK_Y 594.0f
#define NEWS_POPUP_LINK_W 272.0f
#define NEWS_POPUP_LINK_H 68.0f
#define NEWS_TEXT_MIN_SIZE 11.0f
#define NEWS_ZOOM_SECONDS 0.5f
#define NEWS_ZOOM_MAX_FRAME_SECONDS (1.0f / 30.0f)
#define NEWS_ZOOM_MIN_SCALE 0.01f
#define NEWS_ZOOM_SOURCE_OFFSET_Y -30.0f

static const Color NEWS_TEXT = {235, 232, 220, 255};
static const Color NEWS_TEXT_DIM = {180, 176, 166, 255};
static const Color NEWS_TITLE_COLOR = {246, 226, 170, 255};
static const Color NEWS_CHANGER_COLOR = {255, 165, 0, 255};

typedef enum { NEWS_TIMING_ACTIVE = 0, NEWS_TIMING_NEXT, NEWS_TIMING_EXPIRED } NewsTiming;
typedef enum { NEWS_NONE = 0, NEWS_OPEN_EVENT } NewsAction;
typedef enum { NEWS_VIEW_CLOSED = 0, NEWS_VIEW_FEED, NEWS_VIEW_MESSAGE } NewsView;
typedef enum { NEWS_ZOOM_NONE = 0, NEWS_ZOOM_OPENING, NEWS_ZOOM_CLOSING } NewsZoom;

typedef struct {
    char title[NEWS_LOC_KEY_SIZE];
    char subtitle[NEWS_LOC_KEY_SIZE];
    char summary[NEWS_LOC_KEY_SIZE];
    char message[NEWS_LOC_KEY_SIZE];
} NewsText;

typedef struct {
    int event;
    NewsTiming timing;
    long long started_at;
    long long seconds;
} NewsMessage;

typedef struct {
    NewsText text[NEXUS_MAX_EVENTS];
    NewsMessage messages[NEWS_MAX_MESSAGES];
    int message_count;
    int featured_count;
    int unseen_count;
    NewsView view;
    NewsZoom zoom;
    float zoom_progress;
    NewsMessage opened;
    bool opened_from_feed;
    bool auto_shown;
    int target_event;
    float ticker_time;
    bool pvp_event_requested;
    int featured_scroll;
    float list_scroll;
    bool dragging;
    bool drag_moved;
    Vector2 drag_start;
    float drag_scroll;
    bool scrim_armed;
    Button link;
    Button feed_button;
} News;

static float NewsLeft(void) {
    return (UiWidth() - UI_DESIGN_W) * 0.5f;
}

static float NewsTop(void) {
    return (UiHeight() - UI_DESIGN_H) * 0.5f;
}

static Rectangle NewsDesignRect(float x, float y, float width, float height) {
    return (Rectangle){NewsLeft() + x, NewsTop() + y, width, height};
}

void NewsInitialize(News *news, const Nexus *nexus, const char *path) {
    memset(news, 0, sizeof(*news));
    news->target_event = -1;
    news->auto_shown = getenv("HOD_SHOT") != NULL;
    Font font = GuiFont(FNT_COOL);
    news->link = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "", font, (Vector2){0.5f, 0.5f}, 0.32f);
    news->feed_button = ButtonLoad(GUI_NEWS_BUTTON, GUI_NEWS_BUTTON);
    ButtonSetClickSound(&news->feed_button, SND_UI_BIG_BUTTON);

    cJSON *root = JsonLoadFile(path);
    if (!cJSON_IsArray(root)) TraceLog(LOG_FATAL, "News: %s is not an array", path);
    int loaded = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        int event = NexusFindEvent(nexus, JsonStr(item, "event"));
        if (event < 0) TraceLog(LOG_FATAL, "News: unknown event %s", JsonStr(item, "event"));
        NewsText *text = &news->text[event];
        StrCopy(text->title, sizeof(text->title), JsonStr(item, "title"));
        StrCopy(text->subtitle, sizeof(text->subtitle), JsonStr(item, "subtitle"));
        StrCopy(text->summary, sizeof(text->summary), JsonStr(item, "summary"));
        StrCopy(text->message, sizeof(text->message), JsonStr(item, "message"));
        loaded++;
    }
    cJSON_Delete(root);
    if (loaded != NexusEventCount(nexus))
        TraceLog(LOG_FATAL, "News: %d of %d events have text", loaded, NexusEventCount(nexus));
}

static int NewsRank(const NexusEvent *e, NewsTiming timing) {
    if (timing != NEWS_TIMING_ACTIVE) return 2 + timing;
    if (e->weekday == 0 && e->unlock_key == 0) return 0;
    return e->weekday != 0 ? 1 : 2;
}

static void NewsInsert(News *news, const Nexus *nexus, NewsMessage message) {
    int rank = NewsRank(NexusEventAt(nexus, message.event), message.timing);
    int at = news->message_count++;
    while (at > 0 &&
           NewsRank(NexusEventAt(nexus, news->messages[at - 1].event), news->messages[at - 1].timing) > rank) {
        news->messages[at] = news->messages[at - 1];
        at--;
    }
    news->messages[at] = message;
}

static void NewsCollect(News *news, const Nexus *nexus, const Player *player, long long now) {
    news->message_count = 0;
    news->unseen_count = 0;
    for (int event = 0; event < NexusEventCount(nexus); event++) {
        const NexusEvent *e = NexusEventAt(nexus, event);
        NewsMessage message = {.event = event, .timing = NEWS_TIMING_ACTIVE};
        bool rotates = e->weekday == 0 && e->unlock_key == 0 && e->duration > 0;
        if (NexusEventScheduled(nexus, player, event, now)) {
            message.seconds = NexusEventTimeRemaining(nexus, player, event, now);
            message.started_at = e->unlock_key == 0 ? NexusEventWindowStart(e, now) : 0;
        } else if (rotates && NexusEventScheduled(nexus, player, event, now + e->duration)) {
            message.timing = NEWS_TIMING_NEXT;
            message.started_at = NexusEventWindowStart(e, now + e->duration);
            message.seconds = message.started_at - now;
        } else if (rotates && NexusEventScheduled(nexus, player, event, now - e->duration)) {
            message.timing = NEWS_TIMING_EXPIRED;
            message.started_at = NexusEventWindowStart(e, now - e->duration);
            message.seconds = now - NexusEventWindowStart(e, now);
        } else {
            continue;
        }
        NewsInsert(news, nexus, message);
        if (message.timing == NEWS_TIMING_ACTIVE && message.started_at > player->news_seen_at) news->unseen_count++;
    }
    news->featured_count = 0;
    while (news->featured_count < news->message_count &&
           NewsRank(NexusEventAt(nexus, news->messages[news->featured_count].event), NEWS_TIMING_ACTIVE) == 0 &&
           news->messages[news->featured_count].timing == NEWS_TIMING_ACTIVE)
        news->featured_count++;
    if (news->featured_count == 0 && news->message_count > 0) news->featured_count = 1;
}

bool NewsIsOpen(const News *news) {
    return news->view != NEWS_VIEW_CLOSED;
}

int NewsTargetEvent(const News *news) {
    return news->target_event;
}

void NewsOpen(News *news, Player *player) {
    news->view = NEWS_VIEW_FEED;
    news->zoom = NEWS_ZOOM_OPENING;
    news->zoom_progress = 0.0f;
    news->list_scroll = 0.0f;
    news->featured_scroll = 0;
    news->scrim_armed = false;
    player->news_seen_at = ServerNow();
}

bool NewsAutoShow(News *news, Player *player) {
    if (news->auto_shown) return false;
    news->auto_shown = true;
    NewsOpen(news, player);
    return true;
}

static void NewsOpenMessage(News *news, const NewsMessage *message, bool from_feed) {
    news->opened = *message;
    news->opened_from_feed = from_feed;
    news->view = NEWS_VIEW_MESSAGE;
    news->scrim_armed = false;
    GuiSoundPlay(SND_UI_TAB);
}

void NewsOpenEvent(News *news, const Nexus *nexus, const Player *player, int event) {
    NewsCollect(news, nexus, player, ServerNow());
    for (int i = 0; i < news->message_count; i++) {
        if (news->messages[i].event == event) {
            NewsOpenMessage(news, &news->messages[i], false);
            return;
        }
    }
}

static void NewsCloseCurrent(News *news) {
    news->scrim_armed = false;
    if (news->view == NEWS_VIEW_FEED) {
        news->zoom = NEWS_ZOOM_CLOSING;
        news->zoom_progress = 0.0f;
        return;
    }
    news->view = news->opened_from_feed ? NEWS_VIEW_FEED : NEWS_VIEW_CLOSED;
}

bool NewsHandleBack(News *news) {
    if (!NewsIsOpen(news)) return false;
    if (news->zoom != NEWS_ZOOM_NONE) return true;
    NewsCloseCurrent(news);
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    return true;
}

static const char *NewsTitle(const News *news, const Nexus *nexus, int event) {
    const char *title = news->text[event].title;
    return title[0] ? LocGet(title) : NexusEventAt(nexus, event)->name;
}

static void
NewsTitleWithState(char *out, size_t size, const News *news, const Nexus *nexus, const NewsMessage *message) {
    char time_text[48];
    const char *title = NewsTitle(news, nexus, message->event);
    if (message->timing == NEWS_TIMING_EXPIRED) {
        snprintf(out, size, "%s %s", title, LocGet("NEWSFEED_TITLE_DESCRIPTOR_EXPIRED"));
    } else if (message->timing == NEWS_TIMING_NEXT) {
        snprintf(out, size, "%s %s", title, LocGet("NEWSFEED_TITLE_DESCRIPTOR_NEXT"));
    } else if (message->seconds > 0) {
        QuestsShortTime(time_text, sizeof(time_text), message->seconds);
        snprintf(out, size, "%s <color=orange>(%s)</color>", title, time_text);
    } else {
        StrCopy(out, size, title);
    }
}

static void NewsTimerText(char *out, size_t size, const NewsMessage *message) {
    char time_text[48];
    QuestsShortTime(time_text, sizeof(time_text), message->seconds);
    LocFormat2(
        out,
        size,
        message->timing == NEWS_TIMING_EXPIRED ? "NEWSFEED_PREVIOUS_TIME" : "NEWSFEED_COMING_TIME",
        time_text,
        ""
    );
}

static void NewsDrawRich(Font font, const char *text, Rectangle box, float size, Color color) {
    HeroesDrawRich(font, text, box, CampaignFitWrappedSize(font, text, box, size, NEWS_TEXT_MIN_SIZE), color);
}

static float NewsZoomScale(const News *news) {
    if (news->zoom == NEWS_ZOOM_NONE) return 1.0f;
    float shown = news->zoom == NEWS_ZOOM_OPENING ? news->zoom_progress : 1.0f - news->zoom_progress;
    return Lerp(NEWS_ZOOM_MIN_SCALE, 1.0f, shown);
}

static Vector2 NewsZoomOffset(const News *news) {
    if (news->zoom == NEWS_ZOOM_NONE) return (Vector2){0.0f, 0.0f};
    float hidden = news->zoom == NEWS_ZOOM_OPENING ? 1.0f - news->zoom_progress : news->zoom_progress;
    float source_x = NEWS_BUTTON_X + NEWS_BUTTON_SIZE * 0.5f - UiWidth() * 0.5f;
    return (Vector2){source_x * hidden, NEWS_ZOOM_SOURCE_OFFSET_Y * hidden};
}

static Vector2 NewsZoomPoint(const News *news, Vector2 point) {
    Vector2 center = {UiWidth() * 0.5f, UiHeight() * 0.5f};
    Vector2 offset = NewsZoomOffset(news);
    float scale = NewsZoomScale(news);
    return (Vector2){
        center.x + offset.x + (point.x - center.x) * scale,
        center.y + offset.y + (point.y - center.y) * scale,
    };
}

static void NewsZoomBegin(const News *news) {
    Vector2 origin = NewsZoomPoint(news, (Vector2){0.0f, 0.0f});
    float scale = NewsZoomScale(news);
    rlPushMatrix();
    rlTranslatef(origin.x, origin.y, 0.0f);
    rlScalef(scale, scale, 1.0f);
}

static void NewsZoomEnd(void) {
    rlPopMatrix();
}

static bool NewsZoomAdvance(News *news, float dt) {
    if (news->zoom == NEWS_ZOOM_NONE) return false;
    news->zoom_progress += fminf(dt, NEWS_ZOOM_MAX_FRAME_SECONDS) / NEWS_ZOOM_SECONDS;
    if (news->zoom_progress < 1.0f) return false;
    bool closed = news->zoom == NEWS_ZOOM_CLOSING;
    news->zoom = NEWS_ZOOM_NONE;
    if (closed) news->view = NEWS_VIEW_CLOSED;
    return closed;
}

static void NewsDrawScrim(void) {
    DrawRectangle(0, 0, (int)UiWidth() + 1, (int)UiHeight() + 1, (Color){0, 0, 0, 150});
}

static void NewsDrawFrame(const char *title) {
    Rectangle frame = NewsDesignRect(NEWS_FRAME_X, NEWS_FRAME_Y, NEWS_FRAME_W, NEWS_FRAME_H);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), frame, WHITE);
    if (title) {
        Rectangle band = {frame.x, frame.y + NEWS_TITLE_H, frame.width, 0.0f};
        UiDrawTextCentered(GuiFont(FNT_COOL), title, band, 40.0f, NEWS_TEXT);
    }
}

static bool NewsScrimClicked(News *news) {
    Vector2 mouse = UiMouse();
    Rectangle frame = NewsDesignRect(NEWS_FRAME_X, NEWS_FRAME_Y, NEWS_FRAME_W, NEWS_FRAME_H);
    bool outside = !CheckCollisionPointRec(mouse, frame);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) news->scrim_armed = outside;
    bool clicked = IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && news->scrim_armed && outside;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) news->scrim_armed = false;
    return clicked;
}

static bool NewsCellTapped(const News *news, Rectangle cell) {
    return IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !news->drag_moved && CheckCollisionPointRec(UiMouse(), cell) &&
           CheckCollisionPointRec(news->drag_start, cell);
}

static void NewsDrawImageFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0) return;
    float fit = fminf(dst.width / texture.width, dst.height / texture.height);
    Rectangle fitted = {
        dst.x + (dst.width - texture.width * fit) * 0.5f,
        dst.y + (dst.height - texture.height * fit) * 0.5f,
        texture.width * fit,
        texture.height * fit,
    };
    UiDrawTexture(texture, fitted, tint);
}

static bool NewsDrawFeaturedCell(
    News *news, Quests *quests, const Nexus *nexus, const NewsMessage *message, Rectangle cell, bool enabled
) {
    const NexusEvent *e = NexusEventAt(nexus, message->event);
    Rectangle frame = {
        cell.x + NEWS_FEATURED_MARGIN,
        cell.y + NEWS_FEATURED_MARGIN,
        cell.width - NEWS_FEATURED_MARGIN * 2.0f,
        cell.height - NEWS_FEATURED_MARGIN * 2.0f,
    };
    QuestsDrawFrame(quests->frame, frame, 4.0f, WHITE);
    Rectangle image = {
        frame.x + 10.0f,
        frame.y + (frame.height - NEWS_FEATURED_IMAGE_SIZE) * 0.5f,
        NEWS_FEATURED_IMAGE_SIZE,
        NEWS_FEATURED_IMAGE_SIZE
    };
    NewsDrawImageFit(QuestsHubImage(quests, message->event), image, WHITE);
    float text_x = image.x + image.width + 10.0f;
    float text_w = frame.x + frame.width - 10.0f - text_x;
    char title[256];
    NewsTitleWithState(title, sizeof(title), news, nexus, message);
    Rectangle title_box = {text_x, frame.y + 10.0f, text_w, 76.0f};
    NewsDrawRich(GuiFont(FNT_MENU), title, title_box, 18.0f, NEWS_TITLE_COLOR);
    DrawRectangle((int)text_x, (int)(frame.y + 90.0f), (int)text_w, 2, e->highlight);
    const char *summary = LocGet(news->text[message->event].summary);
    Rectangle summary_box = {text_x, frame.y + 98.0f, text_w, frame.height - 106.0f};
    NewsDrawRich(GuiFont(FNT_MENU), summary, summary_box, 15.0f, NEWS_TEXT);
    return enabled && NewsCellTapped(news, frame);
}

static bool NewsDrawArrow(News *news, Rectangle rect, bool left, bool enabled) {
    if (!enabled) return false;
    QuestsDrawArrowTexture(rect, left);
    return NewsCellTapped(news, rect);
}

static void NewsDrawFeatured(News *news, Quests *quests, const Nexus *nexus, bool enabled) {
    int visible = news->featured_count < NEWS_FEATURED_VISIBLE ? news->featured_count : NEWS_FEATURED_VISIBLE;
    int max_scroll = news->featured_count - visible;
    if (news->featured_scroll > max_scroll) news->featured_scroll = max_scroll;
    float total = visible * NEWS_FEATURED_CELL_W;
    float x = NewsLeft() + NEWS_CONTENT_X + (NEWS_CONTENT_W - total) * 0.5f;
    float y = NewsTop() + NEWS_FEATURED_Y;
    for (int i = 0; i < visible; i++) {
        const NewsMessage *message = &news->messages[news->featured_scroll + i];
        Rectangle cell = {x + i * NEWS_FEATURED_CELL_W, y, NEWS_FEATURED_CELL_W, NEWS_FEATURED_H};
        if (NewsDrawFeaturedCell(news, quests, nexus, message, cell, enabled)) NewsOpenMessage(news, message, true);
    }
    Rectangle left = NewsDesignRect(NEWS_ARROW_LEFT_X, NEWS_ARROW_Y, NEWS_ARROW_W, NEWS_ARROW_H);
    Rectangle right = NewsDesignRect(NEWS_ARROW_RIGHT_X, NEWS_ARROW_Y, NEWS_ARROW_W, NEWS_ARROW_H);
    if (NewsDrawArrow(news, left, true, enabled && news->featured_scroll > 0)) news->featured_scroll--;
    if (NewsDrawArrow(news, right, false, enabled && news->featured_scroll < max_scroll)) news->featured_scroll++;
}

static void NewsDrawRow(News *news, Quests *quests, const Nexus *nexus, const NewsMessage *message, Rectangle row) {
    UiDrawTexture(GuiTex(GUI_NEWS_ROW_LINE), (Rectangle){row.x, row.y, row.width, NEWS_ROW_LINE_H}, WHITE);
    Rectangle icon = {row.x, row.y + 12.0f, NEWS_ROW_ICON_SIZE, NEWS_ROW_ICON_SIZE};
    NewsDrawImageFit(QuestsHubImage(quests, message->event), icon, WHITE);
    bool timed = message->timing != NEWS_TIMING_ACTIVE;
    float text_w = row.width - NEWS_ROW_TEXT_X - (timed ? NEWS_ROW_TIMER_W : 0.0f);
    char title[256];
    NewsTitleWithState(title, sizeof(title), news, nexus, message);
    Rectangle title_box = {row.x + NEWS_ROW_TEXT_X, row.y + 12.0f, text_w, 40.0f};
    NewsDrawRich(GuiFont(FNT_MENU), title, title_box, 18.0f, NEWS_TITLE_COLOR);
    const char *subtitle = message->timing == NEWS_TIMING_EXPIRED ? LocGet("ALLY_EXPIRED")
                           : message->timing == NEWS_TIMING_NEXT  ? ""
                                                                  : LocGet(news->text[message->event].summary);
    Rectangle subtitle_box = {row.x + NEWS_ROW_TEXT_X, row.y + 54.0f, text_w, row.height - 58.0f};
    NewsDrawRich(GuiFont(FNT_MENU), subtitle, subtitle_box, 15.0f, NEWS_TEXT_DIM);
    if (timed) {
        char timer[64];
        NewsTimerText(timer, sizeof(timer), message);
        Rectangle timer_box = {row.x + row.width - NEWS_ROW_TIMER_W, row.y, NEWS_ROW_TIMER_W, row.height};
        UiDrawTextCentered(GuiFont(FNT_MENU), timer, timer_box, 18.0f, NEWS_TEXT);
    }
}

static void NewsUpdateListScroll(News *news, Rectangle viewport, float max_scroll) {
    Vector2 mouse = UiMouse();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        news->drag_start = mouse;
        news->drag_moved = false;
        news->dragging = CheckCollisionPointRec(mouse, viewport);
        news->drag_scroll = news->list_scroll;
    }
    if (news->dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float delta = mouse.y - news->drag_start.y;
        if (fabsf(delta) > NEWS_TAP_SLOP) news->drag_moved = true;
        if (news->drag_moved) news->list_scroll = news->drag_scroll - delta;
    }
    if (CheckCollisionPointRec(mouse, viewport)) news->list_scroll -= GetMouseWheelMove() * NEWS_WHEEL_STEP;
    news->list_scroll = Clamp(news->list_scroll, 0.0f, max_scroll);
}

static void NewsDrawList(News *news, Quests *quests, const Nexus *nexus, bool enabled) {
    Rectangle viewport = NewsDesignRect(NEWS_CONTENT_X, NEWS_LIST_Y, NEWS_CONTENT_W, NEWS_LIST_H);
    int rows = news->message_count - news->featured_count;
    float max_scroll = fmaxf(rows * NEWS_ROW_H - viewport.height, 0.0f);
    if (enabled) NewsUpdateListScroll(news, viewport, max_scroll);
    Vector2 clip_min = NewsZoomPoint(news, (Vector2){viewport.x, viewport.y});
    Vector2 clip_max = NewsZoomPoint(news, (Vector2){viewport.x + viewport.width, viewport.y + viewport.height});
    float scale = UiScale();
    BeginScissorMode(
        (int)(clip_min.x * scale),
        (int)(clip_min.y * scale),
        (int)((clip_max.x - clip_min.x) * scale),
        (int)((clip_max.y - clip_min.y) * scale)
    );
    NewsMessage *tapped = NULL;
    for (int i = 0; i < rows; i++) {
        Rectangle row = {viewport.x, viewport.y + i * NEWS_ROW_H - news->list_scroll, viewport.width, NEWS_ROW_H};
        if (row.y + row.height < viewport.y || row.y > viewport.y + viewport.height) continue;
        NewsMessage *message = &news->messages[news->featured_count + i];
        NewsDrawRow(news, quests, nexus, message, row);
        Rectangle tappable = GetCollisionRec(row, viewport);
        if (enabled && NewsCellTapped(news, tappable)) tapped = message;
    }
    EndScissorMode();
    if (tapped) NewsOpenMessage(news, tapped, true);
}

static void NewsDrawFeed(News *news, Quests *quests, const Nexus *nexus) {
    bool enabled = news->zoom == NEWS_ZOOM_NONE;
    NewsZoomBegin(news);
    NewsDrawFrame(LocGetOr("NEWSFEED_TITLE", "Новости"));
    NewsDrawFeatured(news, quests, nexus, enabled);
    if (news->view == NEWS_VIEW_FEED) NewsDrawList(news, quests, nexus, enabled);
    NewsZoomEnd();
    if (enabled && news->view == NEWS_VIEW_FEED && NewsScrimClicked(news)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        NewsCloseCurrent(news);
    }
}

static NewsAction NewsDrawMessage(News *news, Quests *quests, const Nexus *nexus) {
    const NewsMessage *message = &news->opened;
    const NewsText *text = &news->text[message->event];
    NewsDrawFrame(NULL);
    char title[256];
    NewsTitleWithState(title, sizeof(title), news, nexus, message);
    Rectangle title_box =
        NewsDesignRect(NEWS_POPUP_TITLE_X, NEWS_POPUP_TITLE_Y, NEWS_POPUP_TITLE_W, NEWS_POPUP_TITLE_H);
    float title_size = CampaignFitWrappedSize(GuiFont(FNT_MENU), title, title_box, 30.0f, NEWS_TEXT_MIN_SIZE);
    HeroesDrawRich(GuiFont(FNT_MENU), title, title_box, title_size, NEWS_TITLE_COLOR);
    if (message->timing == NEWS_TIMING_ACTIVE) {
        const char *subtitle = LocGet(text->subtitle[0] ? text->subtitle : text->summary);
        Rectangle subtitle_box =
            NewsDesignRect(NEWS_POPUP_TITLE_X, NEWS_POPUP_SUBTITLE_Y, NEWS_POPUP_TITLE_W, NEWS_POPUP_SUBTITLE_H);
        NewsDrawRich(GuiFont(FNT_MENU), subtitle, subtitle_box, 20.0f, NEWS_TEXT);
    }
    Rectangle image =
        NewsDesignRect(NEWS_POPUP_IMAGE_X, NEWS_POPUP_IMAGE_Y, NEWS_POPUP_IMAGE_SIZE, NEWS_POPUP_IMAGE_SIZE);
    NewsDrawImageFit(QuestsHubImage(quests, message->event), image, WHITE);
    Rectangle body =
        NewsDesignRect(NEWS_POPUP_MESSAGE_X, NEWS_POPUP_MESSAGE_Y, NEWS_POPUP_MESSAGE_W, NEWS_POPUP_MESSAGE_H);
    const BattleGameChanger *changer = NexusEventGameChanger(nexus, message->event);
    if (changer) {
        body.height -= NEWS_POPUP_CHANGER_H;
        Rectangle rule = {body.x, body.y + body.height, body.width, NEWS_POPUP_CHANGER_H};
        NewsDrawRich(GuiFont(FNT_MENU), NexusGameChangerText(changer), rule, 17.0f, NEWS_CHANGER_COLOR);
    }
    NewsDrawRich(GuiFont(FNT_MENU), LocGet(text->message), body, 19.0f, NEWS_TEXT);

    NewsAction action = NEWS_NONE;
    if (message->timing == NEWS_TIMING_ACTIVE) {
        news->link.label = LocGetOr("INTERSTITIAL_BUTTON_GO", "Перейти");
        Rectangle link = NewsDesignRect(NEWS_POPUP_LINK_X, NEWS_POPUP_LINK_Y, NEWS_POPUP_LINK_W, NEWS_POPUP_LINK_H);
        if (ButtonDrawEx(&news->link, link, true)) {
            news->target_event = message->event;
            news->view = NEWS_VIEW_CLOSED;
            action = NEWS_OPEN_EVENT;
        }
    }
    if (action == NEWS_NONE && NewsScrimClicked(news)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        NewsCloseCurrent(news);
    }
    return action;
}

NewsAction NewsDrawOverlay(News *news, Quests *quests, const Nexus *nexus, const Player *player) {
    if (!NewsIsOpen(news) || NewsZoomAdvance(news, GetFrameTime())) return NEWS_NONE;
    QuestsEnsureHubImages(quests, nexus);
    long long now = ServerNow();
    NewsCollect(news, nexus, player, now);
    NewsDrawScrim();
    if (news->view == NEWS_VIEW_FEED) {
        NewsDrawFeed(news, quests, nexus);
        return NEWS_NONE;
    }
    return NewsDrawMessage(news, quests, nexus);
}

static void NewsDrawPvpTicker(News *news, Color tint, bool interactive) {
    long long now = ServerNow();
    Rectangle frame = {(UiWidth() - NEWS_TICKER_W) * 0.5f, NEWS_TICKER_Y, NEWS_TICKER_W, NEWS_TICKER_H};
    UiDrawTexture(GuiTex(GUI_NEWS_TICKER), frame, WHITE);
    Rectangle image = {
        frame.x + NEWS_TICKER_IMAGE_X, frame.y + NEWS_TICKER_IMAGE_Y, NEWS_TICKER_IMAGE_SIZE, NEWS_TICKER_IMAGE_SIZE
    };
    NewsDrawImageFit(GuiTex(GUI_PVP_BANNER), image, tint);
    Rectangle title = {
        frame.x + NEWS_TICKER_TEXT_X, frame.y + NEWS_TICKER_TITLE_Y, NEWS_TICKER_TEXT_W, NEWS_TICKER_TITLE_H
    };
    QuestsDrawFit(
        GuiFont(FNT_COOL),
        LocGetOr("INTERSTITIAL_TICKER_TODAYS_EVENT", "Событие дня"),
        title,
        24.0f,
        Fade(NEWS_TEXT, tint.a / 255.0f)
    );
    char time_text[48], timer[160];
    QuestsShortTime(time_text, sizeof(time_text), PvpEventRemaining(now));
    snprintf(timer, sizeof(timer), "%s (%s)", LocGet(PvpEventForCycle(PvpEventCycle(now))->title_key), time_text);
    Rectangle timer_box = {title.x, frame.y + NEWS_TICKER_TIMER_Y, title.width, NEWS_TICKER_TIMER_H};
    NewsDrawRich(GuiFont(FNT_MENU), timer, timer_box, 16.0f, Fade(NEWS_TITLE_COLOR, tint.a / 255.0f));
    if (interactive && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), frame)) {
        GuiSoundPlay(SND_UI_BIG_BUTTON);
        news->pvp_event_requested = true;
    }
}

bool NewsTakePvpEventRequest(News *news) {
    bool requested = news->pvp_event_requested;
    news->pvp_event_requested = false;
    return requested;
}

static void NewsDrawTicker(News *news, Quests *quests, const Nexus *nexus, bool interactive, float dt) {
    news->ticker_time += dt;
    int slot = (int)(news->ticker_time / NEWS_TICKER_SECONDS) % (news->featured_count + 1);
    float phase = fmodf(news->ticker_time, NEWS_TICKER_SECONDS);
    float fade = fminf(phase, NEWS_TICKER_SECONDS - phase) / NEWS_TICKER_FADE_SECONDS;
    Color tint = Fade(WHITE, Clamp(fade, 0.0f, 1.0f));
    if (slot == 0) {
        NewsDrawPvpTicker(news, tint, interactive);
        return;
    }

    const NewsMessage *message = &news->messages[slot - 1];
    Rectangle frame = {(UiWidth() - NEWS_TICKER_W) * 0.5f, NEWS_TICKER_Y, NEWS_TICKER_W, NEWS_TICKER_H};
    UiDrawTexture(GuiTex(GUI_NEWS_TICKER), frame, WHITE);
    Rectangle image = {
        frame.x + NEWS_TICKER_IMAGE_X, frame.y + NEWS_TICKER_IMAGE_Y, NEWS_TICKER_IMAGE_SIZE, NEWS_TICKER_IMAGE_SIZE
    };
    NewsDrawImageFit(QuestsHubImage(quests, message->event), image, tint);
    Rectangle title = {
        frame.x + NEWS_TICKER_TEXT_X, frame.y + NEWS_TICKER_TITLE_Y, NEWS_TICKER_TEXT_W, NEWS_TICKER_TITLE_H
    };
    QuestsDrawFit(
        GuiFont(FNT_COOL),
        LocGetOr("INTERSTITIAL_TICKER_TODAYS_EVENT", "Событие дня"),
        title,
        24.0f,
        Fade(NEWS_TEXT, tint.a / 255.0f)
    );
    char time_text[48], timer[160];
    QuestsShortTime(time_text, sizeof(time_text), message->seconds);
    snprintf(timer, sizeof(timer), "%s (%s)", NexusEventAt(nexus, message->event)->name, time_text);
    Rectangle timer_box = {title.x, frame.y + NEWS_TICKER_TIMER_Y, title.width, NEWS_TICKER_TIMER_H};
    NewsDrawRich(GuiFont(FNT_MENU), timer, timer_box, 16.0f, Fade(NEWS_TITLE_COLOR, tint.a / 255.0f));

    bool pressed = interactive && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), frame);
    if (pressed) {
        GuiSoundPlay(SND_UI_BIG_BUTTON);
        NewsOpenMessage(news, message, false);
    }
}

void NewsDrawHome(News *news, Quests *quests, const Nexus *nexus, Player *player, bool interactive, float dt) {
    QuestsEnsureHubImages(quests, nexus);
    NewsCollect(news, nexus, player, ServerNow());
    Rectangle button = {NEWS_BUTTON_X, NEWS_BUTTON_Y, NEWS_BUTTON_SIZE, NEWS_BUTTON_SIZE};
    if (ButtonDrawEx(&news->feed_button, button, interactive)) NewsOpen(news, player);
    ButtonDrawBadge(button, news->unseen_count);
    NewsDrawTicker(news, quests, nexus, interactive, dt);
}
