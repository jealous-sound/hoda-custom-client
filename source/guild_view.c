#define GUILD_SMART_WIDTH 1280.0f
#define GUILD_MEMBER_SMART_HEIGHT 180.0f
#define GUILD_LIST_SMART_HEIGHT 180.0f
#define GUILD_CHAT_ONE_LINE_HEIGHT 87.0f
#define GUILD_CHAT_TWO_LINE_HEIGHT 130.0f
#define GUILD_SMART_LIST_FRACTION 0.25f
#define GUILD_CHAT_ONE_LINE_FRACTION 0.121f
#define GUILD_CHAT_TWO_LINE_FRACTION 0.181f
#define GUILD_CHAT_WRAP_LINES 2
#define GUILD_EMBLEM_SLOT 72.0f
#define GUILD_EMBLEM_GAP 6.0f
#define GUILD_EMBLEM_SCROLL_STEP 5
#define GUILD_EMBLEM_SCROLL_SECONDS 1.0f
#define GUILD_BADGE_SHOWN_MAX 3
#define GUILD_WAIT_SECONDS 0.6f
#define GUILD_TITLE_SIZE 40.0f
#define GUILD_TITLE_CENTER_Y 137.0f
#define GUILD_TITLE_ICON_SIZE 78.0f
#define GUILD_TITLE_ICON_GAP 18.0f
#define GUILD_WAR_FRAME_VISIBLE_LEFT 122.0f
#define GUILD_WAR_FRAME_VISIBLE_RIGHT 605.0f
#define GUILD_WAR_TOGGLE_STEP 50.0f
#define GUILD_WAR_TOGGLE_SIZE 44.0f
#define GUILD_WAR_ENTER_VISIBLE_LEFT 994.0f
#define GUILD_HEADING_SIZE 24.0f
#define GUILD_BODY_SIZE 20.0f
#define GUILD_SMALL_SIZE 18.0f
#define GUILD_MESSAGE_CAP 1024
#define GUILD_TIME_TEXT_CAP 64
#define GUILD_WAR_SHOW_NEXT_SECONDS 172800LL
#define GUILD_WAR_OPEN_RANK 10000000
#define GUILD_WAR_ROW_SMART_HEIGHT 101.0f
#define GUILD_REWARD_SMART_HEIGHT 252.0f
#define GUILD_REWARD_CENTERED_SLOTS 3
#define GUILD_REWARD_SLOTS 6
#define GUILD_REWARD_MEDALS 4
#define GUILD_OFFLINE_MESSAGE "Гильдии и чат работают только при подключении к серверу."
#define GUILD_UNREACHABLE_MESSAGE "Нет связи с сервером. Попробуйте ещё раз."
#define GUILD_BUSY_MESSAGE "Сервер занят, попробуйте ещё раз."
#define GUILD_PROFILE_CHANGED_MESSAGE "Профиль изменился на сервере, попробуйте ещё раз."

static const Color GUILD_TEXT = {235, 232, 220, 255};
static const Color GUILD_TEXT_DIM = {182, 178, 168, 255};
static const Color GUILD_TEXT_GOLD = {244, 206, 120, 255};
static const Color GUILD_PANEL_TINT = {0, 0, 0, 90};
static const Color GUILD_HEADER_TINT = {0, 0, 0, 140};
static const Color GUILD_ROW_EVEN = {15, 15, 15, 170};
static const Color GUILD_ROW_ODD = {31, 31, 31, 170};
static const Color GUILD_DISABLED_TINT = {120, 120, 120, 255};
static const Color GUILD_SCRIM = {0, 0, 0, 170};
static const Color GUILD_TEXT_ORANGE = {255, 165, 0, 255};
static const Color GUILD_PLAYER_ROW_TEXT = {36, 22, 8, 255};
static const GuiId GUILD_REWARD_MEDAL_IDS[GUILD_REWARD_MEDALS] = {
    GUI_GUILD_MEDAL_WARDEN, GUI_GUILD_MEDAL_GOLD, GUI_GUILD_MEDAL_SILVER, GUI_GUILD_MEDAL_BRONZE
};
static const Vector2 GUILD_REWARD_CENTERED_SLOT_AT[GUILD_REWARD_CENTERED_SLOTS] = {
    {512.0f, 69.0f}, {768.0f, 69.0f}, {1023.0f, 69.0f}
};
static const Vector2 GUILD_REWARD_SLOT_AT[GUILD_REWARD_SLOTS] = {
    {512.0f, 7.0f}, {768.0f, 7.0f}, {1023.0f, 7.0f}, {512.0f, 125.0f}, {768.0f, 125.0f}, {1023.0f, 125.0f}
};

typedef struct {
    GuildAbuseType type;
    const char *label_key;
    Vector2 guild_at;
    Vector2 application_at;
} GuildAbuseOption;

static const GuildAbuseOption GUILD_ABUSE_OPTIONS[] = {
    {GUILD_ABUSE_HARASSMENT, "REPORT_ABUSE_HARRASSMENT", {262.0f, 357.0f}, {262.0f, 429.0f}},
    {GUILD_ABUSE_EXPLICIT, "REPORT_ABUSE_EXPLICIT", {796.0f, 357.0f}, {796.0f, 429.0f}},
    {GUILD_ABUSE_SEXUAL, "REPORT_ABUSE_SEXUAL", {262.0f, 423.0f}, {262.0f, 495.0f}},
    {GUILD_ABUSE_OTHER, "REPORT_ABUSE_OTHER", {796.0f, 423.0f}, {796.0f, 495.0f}},
};

static const char *const GUILD_EMBLEM_REFS[GUILD_EMBLEM_COUNT] = {
    CACHE_RESOURCE("tex.Guild_Emblem_01"), CACHE_RESOURCE("tex.Guild_Emblem_02"), CACHE_RESOURCE("tex.Guild_Emblem_03"),
    CACHE_RESOURCE("tex.Guild_Emblem_04"), CACHE_RESOURCE("tex.Guild_Emblem_05"), CACHE_RESOURCE("tex.Guild_Emblem_06"),
    CACHE_RESOURCE("tex.Guild_Emblem_07"), CACHE_RESOURCE("tex.Guild_Emblem_08"), CACHE_RESOURCE("tex.Guild_Emblem_09"),
    CACHE_RESOURCE("tex.Guild_Emblem_10"), CACHE_RESOURCE("tex.Guild_Emblem_11"), CACHE_RESOURCE("tex.Guild_Emblem_12"),
    CACHE_RESOURCE("tex.Guild_Emblem_13"), CACHE_RESOURCE("tex.Guild_Emblem_14"), CACHE_RESOURCE("tex.Guild_Emblem_15"),
    CACHE_RESOURCE("tex.Guild_Emblem_16"), CACHE_RESOURCE("tex.Guild_Emblem_17"), CACHE_RESOURCE("tex.Guild_Emblem_18"),
    CACHE_RESOURCE("tex.Guild_Emblem_19"), CACHE_RESOURCE("tex.Guild_Emblem_20"), CACHE_RESOURCE("tex.Guild_Emblem_21"),
    CACHE_RESOURCE("tex.Guild_Emblem_22"), CACHE_RESOURCE("tex.Guild_Emblem_23"),
};

typedef enum { GUILD_PAGE_FIND, GUILD_PAGE_CREATE, GUILD_PAGE_HOME, GUILD_PAGE_CHAT, GUILD_PAGE_WAR } GuildPage;
typedef enum { GUILD_FIND_RECOMMENDED, GUILD_FIND_SEARCH, GUILD_FIND_APPLICATIONS } GuildFindState;
typedef enum { GUILD_HOME_MEMBER, GUILD_HOME_OBSERVE, GUILD_HOME_APPLICATIONS } GuildHomeMode;
typedef enum { GUILD_CHAT_ALL, GUILD_CHAT_EVENTS } GuildChatMode;
typedef enum { GUILD_TAB_HOME, GUILD_TAB_CHAT, GUILD_TAB_WAR, GUILD_TAB_COUNT } GuildTab;

typedef enum {
    GUILD_DIALOG_NONE,
    GUILD_DIALOG_MESSAGE,
    GUILD_DIALOG_CONFIRM,
    GUILD_DIALOG_APPLY,
    GUILD_DIALOG_REVIEW,
    GUILD_DIALOG_LANGUAGE,
} GuildDialog;

typedef enum {
    GUILD_CONFIRM_LEAVE,
    GUILD_CONFIRM_CANCEL_APPLICATION,
    GUILD_CONFIRM_CREATE,
    GUILD_CONFIRM_CREATE_OVER_APPLICATION,
    GUILD_CONFIRM_JOIN_OVER_APPLICATION,
    GUILD_CONFIRM_PROMOTE,
    GUILD_CONFIRM_DEMOTE,
    GUILD_CONFIRM_KICK,
} GuildConfirm;

typedef enum {
    GUILD_VIEW_NONE,
    GUILD_VIEW_CLOSE,
    GUILD_VIEW_DUEL,
    GUILD_VIEW_ENTER_ONSLAUGHT,
    GUILD_VIEW_CLAIM_ONSLAUGHT,
} GuildViewAction;

typedef enum { GUILD_WAR_GUILDS, GUILD_WAR_MEMBERS } GuildWarBoard;

typedef enum {
    GUILD_WAR_NEXT_EVENT,
    GUILD_WAR_CURRENT_EVENT,
    GUILD_WAR_PREVIOUS_EVENT,
    GUILD_WAR_NO_EVENT,
} GuildWarState;

typedef struct {
    GuildWarState state;
    long long previous;
    long long current;
    long long next;
    long long shown;
} GuildWarEvents;

typedef enum { GUILD_REPORT_APPLICATION, GUILD_REPORT_GUILD } GuildReportState;

typedef struct {
    Font font;
    Font body;
    GuildPage page;
    GuildFindState find_state;
    GuildHomeMode home_mode;
    GuildChatMode chat_mode;
    bool settings_mode;
    TextField search;
    TextField name;
    TextField description;
    TextField motd;
    TextField min_level;
    TextField chat_input;
    TextField application;
    GuildSettings draft;
    float emblem_offset;
    float emblem_target;
    int expanded_member;
    GoalsScroll list_scroll;
    GoalsScroll chat_scroll;
    bool chat_follow;
    int chat_seen_count;
    GuildDialog dialog;
    GuildConfirm confirm;
    char dialog_title[GUILD_LINE_TITLE_CAP];
    char dialog_message[GUILD_MESSAGE_CAP];
    char target_id[GUILD_ID_CAP];
    char target_name[GUILD_NAME_CAP];
    GuildRank target_rank;
    char review_text[GUILD_TEXT_CAP];
    GuildRequestKind waiting_for;
    float waiting_time;
    Vector2 press_point;
    bool scrim_armed;
    bool emblems_loaded;
    Texture2D emblems[GUILD_EMBLEM_COUNT];
    Button close;
    Button tabs[GUILD_TAB_COUNT];
    Button settings;
    Button applications;
    Button leave;
    Button back;
    Button gold;
    Button grey;
    Button row_button;
    Button arrow_right;
    Button ok;
    Button yes;
    Button no;
    Button report;
    Button enter;
    Button claim;
    Button help;
    Button submit;
    Button cancel;
    GuildViewAction pending;
    GuildWarBoard war_board;
    bool war_help;
    GoalsScroll war_scroll;
    GoalsScroll reward_scroll;
    bool report_open;
    GuildReportState report_state;
    GuildAbuseType report_choice;
    char report_target[GUILD_ID_CAP];
    char report_name[GUILD_NAME_CAP];
    char report_text[GUILD_TEXT_CAP];
} GuildView;

static float GuildLeft(void) {
    return (UiWidth() - UI_DESIGN_W) * 0.5f;
}

static Rectangle GuildRect(float x, float y, float width, float height) {
    return (Rectangle){GuildLeft() + x, y, width, height};
}

static Rectangle GuildSmart(Rectangle row, float smart_height, float x, float y, float width, float height) {
    float sx = row.width / GUILD_SMART_WIDTH, sy = row.height / smart_height;
    return (Rectangle){row.x + x * sx, row.y + y * sy, width * sx, height * sy};
}

void GuildViewInitialize(GuildView *v) {
    memset(v, 0, sizeof(*v));
    v->font = GuiFont(FNT_COOL);
    v->body = GuiFont(FNT_MENU);
    Vector2 centre = {0.5f, 0.5f};
    v->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&v->close, SND_UI_POPUP_CLOSE);
    static const char *const tab_keys[GUILD_TAB_COUNT] = {
        "GUILD_TABS_MY_GUILD_BUTTON", "GUILD_TABS_CHAT_BUTTON", "GUILD_TABS_ONSLAUGHT_BUTTON"
    };
    for (int tab = 0; tab < GUILD_TAB_COUNT; tab++) {
        v->tabs[tab] = ButtonLoadText(GUI_GUILD_TAB, GUI_GUILD_TAB_ON, LocGet(tab_keys[tab]), v->font, centre, 0.5f);
        ButtonSetClickSound(&v->tabs[tab], SND_UI_TAB);
    }
    v->settings = ButtonLoad(GUI_GUILD_SETTINGS, GUI_GUILD_SETTINGS_ON);
    v->applications = ButtonLoad(GUI_GUILD_APPLICATIONS, GUI_GUILD_APPLICATIONS_ON);
    v->leave = ButtonLoad(GUI_GUILD_LEAVE, GUI_GUILD_LEAVE_ON);
    v->back = ButtonLoad(GUI_STORE_BACK, GUI_STORE_BACK);
    v->gold = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "", v->font, centre, 0.42f);
    v->grey = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "", v->font, centre, 0.42f);
    v->row_button = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "", v->font, centre, 0.5f);
    v->arrow_right = ButtonLoad(GUI_GUILD_ARROW, GUI_GUILD_ARROW_ON);
    v->ok = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), v->font, centre, 0.34f);
    v->yes = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("CONFIRM_DIALOG_YES"), v->font, centre, 0.34f);
    v->no = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("CONFIRM_DIALOG_NO"), v->font, centre, 0.34f);
    v->report = ButtonLoad(GUI_GUILD_REPORT, GUI_GUILD_REPORT_ON);
    v->enter =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("GUILD_WAR_ONSLAUGHT_BUTTON"), v->font, centre, 0.42f);
    v->claim = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("GUILD_WAR_CLAIM_BUTTON"), v->font, centre, 0.42f);
    v->help = ButtonLoad(GUI_STORE_INFO_ICON, GUI_STORE_INFO_ICON_ON);
    v->submit =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("REPORT_ABUSE_SUBMIT_BUTTON"), v->font, centre, 0.34f);
    v->cancel =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("REPORT_ABUSE_CANCEL_BUTTON"), v->font, centre, 0.34f);
    TextFieldInit(&v->search, GUILD_SEARCH_MAX_RUNES, false);
    TextFieldInit(&v->name, GUILD_NAME_MAX_RUNES, false);
    TextFieldInit(&v->description, GUILD_DESCRIPTION_MAX_RUNES, false);
    TextFieldInit(&v->motd, GUILD_MOTD_MAX_RUNES, false);
    TextFieldInit(&v->min_level, GUILD_REQUIRED_LEVEL_DIGITS, true);
    TextFieldInit(&v->chat_input, GUILD_CHAT_MAX_RUNES, false);
    TextFieldInit(&v->application, GUILD_APPLICATION_MAX_RUNES, false);
    v->expanded_member = -1;
}

static void GuildViewLoadEmblems(GuildView *v) {
    if (v->emblems_loaded) return;
    for (int i = 0; i < GUILD_EMBLEM_COUNT; i++) {
        v->emblems[i] = ResourceLoadTexture(GUILD_EMBLEM_REFS[i]);
        SetTextureFilter(v->emblems[i], TEXTURE_FILTER_BILINEAR);
    }
    v->emblems_loaded = true;
}

static void GuildDrawEmblem(const GuildView *v, int icon, Rectangle rect, Color tint) {
    if (icon < 1 || icon > GUILD_EMBLEM_COUNT || !v->emblems_loaded) return;
    float side = fminf(rect.width, rect.height);
    UiDrawTexture(
        v->emblems[icon - 1],
        (Rectangle){rect.x + (rect.width - side) * 0.5f, rect.y + (rect.height - side) * 0.5f, side, side},
        tint
    );
}

static void GuildFitText(Font font, const char *text, Rectangle rect, float size, Color color, bool centred) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    if (measured.x > rect.width && measured.x > 0.0f) size *= rect.width / measured.x;
    if (centred)
        UiDrawTextCentered(font, text, rect, size, color);
    else
        UiDrawTextLeftMiddle(font, text, rect, size, color);
}

static void GuildFitTextRight(Font font, const char *text, Rectangle rect, float size, Color color) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    if (measured.x > rect.width && measured.x > 0.0f) size *= rect.width / measured.x;
    UiDrawTextRight(font, text, rect, size, color);
}

static void GuildDropParenthetical(char *text) {
    char *open = strstr(text, " (");
    char *close = open ? strchr(open, ')') : NULL;
    if (!close) return;
    memmove(open, close + 1, strlen(close + 1) + 1);
}

static void GuildDrawWrapped(Font font, const char *text, Rectangle rect, float size, Color color) {
    char lines[MESSAGE_POPUP_LINES][MESSAGE_POPUP_TEXT_CAP];
    int count = MessagePopupWrap(font, text, rect.width, size, lines, MESSAGE_POPUP_LINES);
    float line_height = size * 1.15f;
    int fit = (int)(rect.height / line_height);
    if (fit < 1) fit = 1;
    for (int i = 0; i < count && i < fit; i++)
        DrawTextEx(font, lines[i], (Vector2){rect.x, rect.y + i * line_height}, size, 1.0f, color);
}

static void GuildFormatAgo(char *out, size_t size, long long seconds) {
    if (seconds < 0) seconds = -seconds;
    long long value = seconds;
    const char *key = "MOMENTS_AGO";
    if (seconds > 5 && seconds < 60) {
        value = 1;
        key = "MINUTE_AGO_ABBREVIATED";
    } else if (seconds >= 60 && seconds < 3600) {
        value = seconds / 60;
        key = value > 1 ? "MINUTES_AGO_ABBREVIATED" : "MINUTE_AGO_ABBREVIATED";
    } else if (seconds >= 3600 && seconds < GUILD_SECONDS_PER_DAY) {
        value = seconds / 3600;
        key = value > 1 ? "HOURS_AGO_ABBREVIATED" : "HOUR_AGO_ABBREVIATED";
    } else if (seconds >= GUILD_SECONDS_PER_DAY) {
        value = seconds / GUILD_SECONDS_PER_DAY;
        key = value > 1 ? "DAYS_AGO" : "DAY_AGO";
    }
    const char *format = LocGet(key);
    const char *marker = strstr(format, "[number]");
    if (!marker) {
        StrCopy(out, (int)size, format);
        return;
    }
    snprintf(out, size, "%.*s%lld%s", (int)(marker - format), format, value, marker + strlen("[number]"));
}

static void GuildFormatLastLogin(char *out, size_t size, long long seconds) {
    if (seconds < 0) seconds = -seconds;
    long long value = seconds;
    const char *unit = "SECONDS_SHORT";
    if (seconds >= GUILD_SECONDS_PER_DAY) {
        value = seconds / GUILD_SECONDS_PER_DAY;
        unit = "DAY_SHORT";
    } else if (seconds >= 3600) {
        value = seconds / 3600;
        unit = "HOUR_SHORT";
    } else if (seconds >= 60) {
        value = seconds / 60;
        unit = "MINUTE_SHORT";
    }
    LocFormat2(out, size, "GUILD_LAST_LOGIN_MESSAGE", TextFormat("%lld", value), LocGet(unit));
}

static void GuildAbbreviate(char *out, size_t size, int value) {
    if (value > 1000 && value % 1000 == 0) {
        const char *args[] = {TextFormat("%d", value / 1000)};
        LocFormatArgs(out, size, "NUMBER_FORMAT_THOUSANDS", args, 1);
        return;
    }
    snprintf(out, size, "%d", value);
}

static void GuildViewShowMessage(GuildView *v, const char *title_key, const char *message) {
    v->dialog = GUILD_DIALOG_MESSAGE;
    StrCopy(v->dialog_title, sizeof(v->dialog_title), title_key ? LocGet(title_key) : "");
    StrCopy(v->dialog_message, sizeof(v->dialog_message), message);
}

static void GuildViewShowKeys(GuildView *v, const char *title_key, const char *message_key) {
    GuildViewShowMessage(v, title_key, LocGet(message_key));
}

static void GuildViewShowConfirm(GuildView *v, GuildConfirm confirm, const char *title_key, const char *message) {
    GuildViewShowMessage(v, title_key, message);
    v->dialog = GUILD_DIALOG_CONFIRM;
    v->confirm = confirm;
}

static void GuildViewShowOffline(GuildView *v) {
    GuildViewShowMessage(v, "ERROR_GENERAL_TITLE", GUILD_UNREACHABLE_MESSAGE);
}

static bool GuildViewOverlayInteractive(const GuildView *v) {
    return v->dialog == GUILD_DIALOG_NONE && v->waiting_for == GUILD_REQUEST_NONE && !UiInputBlocked();
}

static bool GuildViewInteractive(const GuildView *v) {
    return GuildViewOverlayInteractive(v) && !v->report_open;
}

static void GuildViewWait(GuildView *v, GuildRequestKind kind) {
    v->waiting_for = kind;
    v->waiting_time = 0.0f;
}

static void GuildViewBlurFields(GuildView *v) {
    TextField *fields[] = {
        &v->search,
        &v->name,
        &v->description,
        &v->motd,
        &v->min_level,
        &v->chat_input,
        &v->application,
    };
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) TextFieldBlur(fields[i]);
}

static void GuildViewShowFind(GuildView *v, Guild *g, GuildFindState state) {
    v->page = GUILD_PAGE_FIND;
    v->find_state = state;
    GoalsScrollReset(&v->list_scroll);
    GuildViewBlurFields(v);
    if (state == GUILD_FIND_RECOMMENDED) GuildRequestRecommended(g);
}

static void GuildViewShowHome(GuildView *v, GuildHomeMode mode) {
    v->page = GUILD_PAGE_HOME;
    v->home_mode = mode;
    v->expanded_member = -1;
    GoalsScrollReset(&v->list_scroll);
    GuildViewBlurFields(v);
}

static void GuildViewShowChat(GuildView *v, Guild *g) {
    v->page = GUILD_PAGE_CHAT;
    v->chat_mode = GUILD_CHAT_ALL;
    v->chat_follow = true;
    v->chat_seen_count = -1;
    GuildViewBlurFields(v);
    GuildSetChatVisible(g, true);
}

static int GuildParseLevel(const char *text) {
    if (!text[0]) return 0;
    return atoi(text);
}

static void GuildViewFillDraft(GuildView *v, const Guild *g) {
    memset(&v->draft, 0, sizeof(v->draft));
    if (v->settings_mode) {
        const GuildSummary *s = &g->mine.summary;
        StrCopy(v->draft.name, sizeof(v->draft.name), s->name);
        StrCopy(v->draft.language, sizeof(v->draft.language), s->language);
        v->draft.status = s->status;
        v->draft.icon = s->icon;
        v->draft.required_level = s->required_level;
        TextFieldSet(&v->name, s->name);
        TextFieldSet(&v->description, s->description);
        TextFieldSet(&v->motd, g->mine.motd);
        TextFieldSet(&v->min_level, TextFormat("%d", s->required_level));
    } else {
        StrCopy(v->draft.language, sizeof(v->draft.language), GUILD_DEFAULT_LANGUAGE);
        v->draft.status = GUILD_STATUS_OPEN;
        v->draft.required_level = GUILD_LEVEL_UNLOCK;
        TextFieldClear(&v->name);
        TextFieldClear(&v->description);
        TextFieldClear(&v->motd);
        TextFieldSet(&v->min_level, TextFormat("%d", GUILD_LEVEL_UNLOCK));
    }
    v->name.read_only = v->settings_mode;
    v->emblem_offset = v->emblem_target = 0.0f;
}

static bool GuildCanEdit(const Guild *g) {
    return g->rank == GUILD_RANK_LEADER || g->rank == GUILD_RANK_OFFICER;
}

static void GuildViewShowCreate(GuildView *v, const Guild *g, bool settings_mode) {
    v->page = GUILD_PAGE_CREATE;
    v->settings_mode = settings_mode;
    GuildViewFillDraft(v, g);
    bool read_only = settings_mode && !GuildCanEdit(g);
    v->description.read_only = read_only;
    v->motd.read_only = read_only;
    v->min_level.read_only = read_only;
    GuildViewBlurFields(v);
}

void GuildViewOpen(GuildView *v, Guild *g, bool chat) {
    GuildViewLoadEmblems(v);
    v->dialog = GUILD_DIALOG_NONE;
    v->waiting_for = GUILD_REQUEST_NONE;
    v->scrim_armed = false;
    v->report_open = false;
    v->pending = GUILD_VIEW_NONE;
    if (GuildIsMember(g)) {
        if (chat)
            GuildViewShowChat(v, g);
        else
            GuildViewShowHome(v, GUILD_HOME_MEMBER);
        GuildRefresh(g);
        return;
    }
    GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
    GuildRefresh(g);
}

void GuildViewLeave(GuildView *v, Guild *g) {
    GuildViewBlurFields(v);
    GuildSetChatVisible(g, false);
    v->dialog = GUILD_DIALOG_NONE;
}

static const char *GuildViewTitle(const GuildView *v) {
    switch (v->page) {
        case GUILD_PAGE_FIND:
            return LocGet("GUILD_FIND_TITLE");
        case GUILD_PAGE_CREATE:
            return LocGet(v->settings_mode ? "GUILD_SETTINGS_TITLE" : "GUILD_CREATE_TITLE");
        case GUILD_PAGE_CHAT:
            return LocGet("GUILD_CHAT_TITLE");
        case GUILD_PAGE_WAR:
            return LocGet("GUILD_WAR_TITLE");
        default:
            return LocGet("GUILD_HOME_TITLE");
    }
}

static bool GuildViewDrawFrame(GuildView *v, bool interactive) {
    Rectangle root = UiRoot();
    DrawRectangleRec(root, GUILD_SCRIM);
    Rectangle frame = GuildRect(0.0f, 72.0f, UI_DESIGN_W, 648.0f);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), frame, WHITE);
    const char *title = GuildViewTitle(v);
    Rectangle title_band = GuildRect(0.0f, GUILD_TITLE_CENTER_Y, UI_DESIGN_W, 0.0f);
    UiDrawTextCentered(v->font, title, title_band, GUILD_TITLE_SIZE, GUILD_TEXT);
    float title_half_width = MeasureTextEx(v->font, title, GUILD_TITLE_SIZE, 1.0f).x * 0.5f;
    Rectangle title_icon = {
        title_band.x + title_band.width * 0.5f - title_half_width - GUILD_TITLE_ICON_GAP - GUILD_TITLE_ICON_SIZE,
        GUILD_TITLE_CENTER_Y - GUILD_TITLE_ICON_SIZE * 0.5f,
        GUILD_TITLE_ICON_SIZE,
        GUILD_TITLE_ICON_SIZE
    };
    if (v->page == GUILD_PAGE_CHAT)
        UiDrawTexture(GuiTex(GUI_GUILD_CHAT_BUTTON), title_icon, WHITE);
    else if (v->page == GUILD_PAGE_HOME || v->page == GUILD_PAGE_FIND)
        UiDrawTexture(GuiTex(GUI_GUILD_BUTTON), title_icon, WHITE);
    bool closed = !v->report_open && ButtonDrawEx(&v->close, GuildRect(1188.0f, 86.0f, 74.0f, 74.0f), interactive);
    Vector2 mouse = UiMouse();
    Rectangle top = {0.0f, 0.0f, UiWidth(), 72.0f};
    bool outside = !CheckCollisionPointRec(mouse, frame) && !CheckCollisionPointRec(mouse, top);
    if (!interactive) {
        v->scrim_armed = false;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        v->scrim_armed = outside;
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (v->scrim_armed && outside) {
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            closed = true;
        }
        v->scrim_armed = false;
    }
    return closed;
}

static void GuildDrawPanel(Rectangle rect) {
    DrawRectangleRec(rect, GUILD_PANEL_TINT);
}

static void GuildDrawRule(Rectangle rect) {
    UiDrawTexture(GuiTex(GUI_GUILD_RULE), rect, WHITE);
}

static void GuildDrawBadge(const GuildView *v, Rectangle rect, int count) {
    if (count <= 0) return;
    UiDrawTexture(GuiTex(GUI_GUILD_BADGE), rect, WHITE);
    const char *text =
        count > GUILD_BADGE_SHOWN_MAX ? TextFormat("%d+", GUILD_BADGE_SHOWN_MAX) : TextFormat("%d", count);
    UiDrawTextCentered(v->body, text, rect, rect.height * 0.7f, WHITE);
}

static bool
GuildTextButton(GuildView *v, Button *button, const char *label, Rectangle rect, bool enabled, bool interactive) {
    Rectangle text = {rect.x + 10.0f, rect.y, rect.width - 20.0f, rect.height};
    float size = rect.height * button->label_scale;
    if (!enabled) {
        UiDrawTexture(button->idle, rect, GUILD_DISABLED_TINT);
        GuildFitText(button->font, label, text, size, GUILD_TEXT_DIM, true);
        return false;
    }
    bool hover = interactive && CheckCollisionPointRec(UiMouse(), rect);
    bool pressed_here = CheckCollisionPointRec(v->press_point, rect);
    UiDrawTexture(
        hover && pressed_here && IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? button->active : button->idle, rect, WHITE
    );
    GuildFitText(button->font, label, text, size, button->label_color, true);
    bool clicked = hover && pressed_here && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    if (clicked) GuiSoundPlay(SND_UI_CLICK);
    return clicked;
}

static bool GuildIconButton(Button *button, GuiId disabled, Rectangle rect, bool enabled, bool interactive) {
    if (!enabled) {
        UiDrawTexture(GuiTex(disabled), rect, WHITE);
        return false;
    }
    return ButtonDrawEx(button, rect, interactive);
}

typedef enum {
    GUILD_FOOTER_NONE,
    GUILD_FOOTER_TAB_HOME,
    GUILD_FOOTER_TAB_CHAT,
    GUILD_FOOTER_TAB_WAR,
    GUILD_FOOTER_SETTINGS,
    GUILD_FOOTER_APPLICATIONS,
    GUILD_FOOTER_REPORT,
    GUILD_FOOTER_LEAVE,
} GuildFooterAction;

static GuildFooterAction GuildDrawFooter(
    GuildView *v, const Guild *g, GuildTab active, bool tabs_shown, bool applications_hidden, bool interactive
) {
    GuildFooterAction action = GUILD_FOOTER_NONE;
    GuildDrawRule(GuildRect(97.0f, 621.0f, 1090.0f, 2.0f));
    bool member = GuildIsMember(g);
    if (GuildIconButton(&v->settings, GUI_GUILD_SETTINGS, GuildRect(92.0f, 624.0f, 56.0f, 47.0f), member, interactive))
        action = GUILD_FOOTER_SETTINGS;
    bool applications = !applications_hidden && member && g->mine.summary.status == GUILD_STATUS_APPLICATION_ONLY;
    Rectangle applications_rect = GuildRect(166.0f, 624.0f, 56.0f, 47.0f);
    if (GuildIconButton(&v->applications, GUI_GUILD_APPLICATIONS_OFF, applications_rect, applications, interactive))
        action = GUILD_FOOTER_APPLICATIONS;
    if (applications) GuildDrawBadge(v, GuildRect(200.0f, 620.0f, 28.0f, 24.0f), g->mine.applicant_count);
    if (!applications_hidden && ButtonDrawEx(&v->report, GuildRect(240.0f, 624.0f, 56.0f, 47.0f), interactive))
        action = GUILD_FOOTER_REPORT;
    if (GuildIconButton(&v->leave, GUI_GUILD_LEAVE_OFF, GuildRect(314.0f, 624.0f, 56.0f, 47.0f), member, interactive))
        action = GUILD_FOOTER_LEAVE;
    if (!tabs_shown) return action;
    static const float tab_x[GUILD_TAB_COUNT] = {670.0f, 848.0f, 1025.0f};
    for (int tab = 0; tab < GUILD_TAB_COUNT; tab++) {
        Rectangle slot = GuildRect(tab_x[tab], 629.0f, 168.0f, 37.0f);
        if (tab == (int)active)
            ButtonDrawSelected(&v->tabs[tab], slot);
        else if (ButtonDrawEx(&v->tabs[tab], slot, interactive))
            action = (GuildFooterAction)(GUILD_FOOTER_TAB_HOME + tab);
        if (tab + 1 < GUILD_TAB_COUNT)
            UiDrawTexture(GuiTex(GUI_GUILD_TAB_SPACER), GuildRect(tab_x[tab] + 170.0f, 629.0f, 6.0f, 37.0f), WHITE);
    }
    return action;
}

static void GuildViewLeavePressed(GuildView *v, const Guild *g) {
    char message[GUILD_MESSAGE_CAP];
    if (g->rank == GUILD_RANK_LEADER && g->mine.member_total <= 1) {
        GuildViewShowConfirm(
            v, GUILD_CONFIRM_LEAVE, "GUILD_LAST_MEMBER_LEAVE_TITLE", LocGet("GUILD_LAST_MEMBER_LEAVE_MESSAGE")
        );
        return;
    }
    if (g->rank == GUILD_RANK_LEADER) {
        GuildViewShowKeys(v, "GUILD_LEAVE_CONFIRMATION_LEADER_TITLE", "GUILD_LEAVE_CONFIRMATION_LEADER_MESSAGE");
        return;
    }
    const char *args[] = {g->mine.summary.name};
    LocFormatArgs(message, sizeof(message), "GUILD_LEAVE_CONFIRMATION_MESSAGE", args, 1);
    GuildViewShowConfirm(v, GUILD_CONFIRM_LEAVE, "GUILD_LEAVE_CONFIRMATION_TITLE", message);
}

static void
GuildViewOpenReport(GuildView *v, GuildReportState state, const char *target, const char *name, const char *text) {
    GuildViewBlurFields(v);
    v->report_open = true;
    v->report_state = state;
    v->report_choice = GUILD_ABUSE_NONE;
    StrCopy(v->report_target, sizeof(v->report_target), target);
    StrCopy(v->report_name, sizeof(v->report_name), name);
    StrCopy(v->report_text, sizeof(v->report_text), text);
}

static GuildWarEvents GuildWarCheckEvents(long long now) {
    GuildWarEvents events = {
        .previous = GuildPreviousOnslaughtCycle(),
        .current = GuildCurrentOnslaughtCycle(),
        .next = GuildNextOnslaughtCycle(),
        .shown = -1,
    };
    if (events.current >= 0) {
        events.state = GUILD_WAR_CURRENT_EVENT;
        events.shown = events.current;
    } else if (events.next >= 0 && PvpEventCycleStart(events.next) - now < GUILD_WAR_SHOW_NEXT_SECONDS) {
        events.state = GUILD_WAR_NEXT_EVENT;
        events.shown = events.next;
    } else if (events.previous >= 0) {
        events.state = GUILD_WAR_PREVIOUS_EVENT;
        events.shown = events.previous;
    } else {
        events.state = GUILD_WAR_NO_EVENT;
    }
    return events;
}

static bool GuildWarHasStandings(const GuildWarEvents *events) {
    return events->state == GUILD_WAR_CURRENT_EVENT || events->state == GUILD_WAR_PREVIOUS_EVENT;
}

static void GuildViewShowWar(GuildView *v, Guild *g) {
    v->page = GUILD_PAGE_WAR;
    v->war_board = GUILD_WAR_GUILDS;
    v->war_help = false;
    GoalsScrollReset(&v->war_scroll);
    GoalsScrollReset(&v->reward_scroll);
    GuildViewBlurFields(v);
    GuildWarEvents events = GuildWarCheckEvents(ServerNow());
    if (GuildWarHasStandings(&events)) GuildRequestOnslaught(g, events.shown);
}

static void GuildViewHandleFooter(GuildView *v, Guild *g, GuildFooterAction action) {
    switch (action) {
        case GUILD_FOOTER_TAB_HOME:
            GuildSetChatVisible(g, false);
            GuildViewShowHome(v, GUILD_HOME_MEMBER);
            break;
        case GUILD_FOOTER_TAB_CHAT:
            GuildViewShowChat(v, g);
            break;
        case GUILD_FOOTER_TAB_WAR:
            GuildSetChatVisible(g, false);
            GuildViewShowWar(v, g);
            break;
        case GUILD_FOOTER_REPORT:
            GuildViewOpenReport(v, GUILD_REPORT_GUILD, g->mine.summary.id, "", "");
            break;
        case GUILD_FOOTER_SETTINGS:
            GuildSetChatVisible(g, false);
            GuildViewShowCreate(v, g, true);
            break;
        case GUILD_FOOTER_APPLICATIONS:
            GuildSetChatVisible(g, false);
            GuildViewShowHome(v, GUILD_HOME_APPLICATIONS);
            break;
        case GUILD_FOOTER_LEAVE:
            GuildViewLeavePressed(v, g);
            break;
        default:
            break;
    }
}

static void GuildDrawListRow(const GuildView *v, const GuildSummary *s, Rectangle row, int index, bool pending) {
    DrawRectangleRec(row, index % 2 ? GUILD_ROW_ODD : GUILD_ROW_EVEN);
    float h = GUILD_LIST_SMART_HEIGHT;
    GuildDrawEmblem(v, s->icon, GuildSmart(row, h, 23.0f, 10.0f, 92.0f, 160.0f), WHITE);
    GuildFitText(
        v->font, s->name, GuildSmart(row, h, 135.0f, 21.0f, 385.0f, 69.0f), GUILD_HEADING_SIZE, GUILD_TEXT, false
    );
    GuildFitText(
        v->body,
        s->description,
        GuildSmart(row, h, 135.0f, 85.0f, 385.0f, 77.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM,
        false
    );
    UiDrawTextCentered(
        v->body,
        TextFormat("%d", s->required_level),
        GuildSmart(row, h, 522.0f, 0.0f, 193.0f, h),
        GUILD_BODY_SIZE,
        GUILD_TEXT
    );
    GuildFitText(
        v->body,
        LocGet(pending ? "GUILD_MEMBER_PENDING" : GuildStatusKey(s->status)),
        GuildSmart(row, h, 717.0f, 0.0f, 257.0f, h),
        GUILD_BODY_SIZE,
        GUILD_TEXT,
        true
    );
    GuildFitText(
        v->body,
        GuildLanguageName(s->language),
        GuildSmart(row, h, 969.0f, 0.0f, 282.0f, h),
        GUILD_BODY_SIZE,
        GUILD_TEXT,
        true
    );
}

static bool GuildListClicked(const GoalsScroll *scroll, Rectangle row, Rectangle list, bool interactive) {
    return interactive && !scroll->drag_scrolled && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(UiMouse(), row) && CheckCollisionPointRec(UiMouse(), list);
}

static void GuildViewOpenObserved(GuildView *v, Guild *g, const char *guild_id) {
    GuildRequestObserve(g, guild_id);
    GuildViewWait(v, GUILD_REQUEST_OBSERVE);
}

static void GuildViewDrawFindList(GuildView *v, Guild *g, bool interactive) {
    Rectangle header = GuildRect(86.0f, 350.0f, 1112.0f, 33.0f);
    DrawRectangleRec(header, GUILD_HEADER_TINT);
    UiDrawTextCentered(
        v->body,
        LocGet("GUILD_FIND_GUILD_EMBLEM"),
        GuildRect(86.0f, 357.0f, 106.0f, 20.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    UiDrawTextLeftMiddle(
        v->body,
        LocGet("GUILD_FIND_GUILD_NAME"),
        GuildRect(199.0f, 357.0f, 334.0f, 20.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    UiDrawTextCentered(
        v->body,
        LocGet("GUILD_FIND_GUILD_MINIMUM_LEVEL"),
        GuildRect(540.0f, 357.0f, 167.0f, 20.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    UiDrawTextCentered(
        v->body,
        LocGet("GUILD_FIND_GUILD_STATUS"),
        GuildRect(709.0f, 357.0f, 223.0f, 20.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    UiDrawTextCentered(
        v->body,
        LocGet("GUILD_FIND_GUILD_LANGUAGE"),
        GuildRect(928.0f, 357.0f, 245.0f, 20.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    Rectangle list = GuildRect(86.0f, 383.0f, 1112.0f, 234.0f);
    bool applications = v->find_state == GUILD_FIND_APPLICATIONS;
    int count = applications ? (g->has_application ? 1 : 0) : g->list_count;
    bool ready = applications || g->list_ready;
    if (!ready) {
        UiDrawTextCentered(v->body, "...", list, GUILD_HEADING_SIZE, GUILD_TEXT_DIM);
        return;
    }
    if (count == 0) {
        UiDrawTextCentered(
            v->font,
            LocGet("GUILD_FIND_NO_RESULTS_TOP"),
            GuildRect(86.0f, 414.0f, 1112.0f, 54.0f),
            GUILD_HEADING_SIZE,
            GUILD_TEXT
        );
        if (v->find_state == GUILD_FIND_SEARCH)
            UiDrawTextCentered(
                v->body,
                LocGet("GUILD_FIND_NO_RESULTS_BOTTOM"),
                GuildRect(86.0f, 478.0f, 1112.0f, 54.0f),
                GUILD_BODY_SIZE,
                GUILD_TEXT_DIM
            );
        return;
    }
    float row_height = list.height * GUILD_SMART_LIST_FRACTION;
    GoalsScrollUpdate(&v->list_scroll, list, row_height * count);
    GoalsBeginClip(list);
    for (int i = 0; i < count; i++) {
        const GuildSummary *s = applications ? &g->applied : &g->list[i];
        Rectangle row = {list.x, list.y + i * row_height - v->list_scroll.offset, list.width, row_height};
        if (row.y + row.height < list.y || row.y > list.y + list.height) continue;
        GuildDrawListRow(v, s, row, i, applications);
        if (GuildListClicked(&v->list_scroll, row, list, interactive)) {
            GuiSoundPlay(SND_UI_CLICK);
            GuildViewOpenObserved(v, g, s->id);
        }
    }
    EndScissorMode();
}

static void GuildViewSearch(GuildView *v, Guild *g) {
    if (!v->search.text[0]) return;
    GuildRequestSearch(g, v->search.text);
    v->find_state = GUILD_FIND_SEARCH;
    GoalsScrollReset(&v->list_scroll);
    TextFieldBlur(&v->search);
}

static void GuildViewCreatePressed(GuildView *v, const Guild *g) {
    if (g->has_application) {
        char message[GUILD_MESSAGE_CAP];
        const char *args[] = {g->applied.name};
        LocFormatArgs(message, sizeof(message), "ERROR_CREATE_WITH_APPLICATION_MESSAGE", args, 1);
        GuildViewShowConfirm(v, GUILD_CONFIRM_CREATE_OVER_APPLICATION, "ERROR_CREATE_WITH_APPLICATION_TITLE", message);
        return;
    }
    GuildViewShowCreate(v, g, false);
}

static void GuildViewCancelPressed(GuildView *v, const Guild *g) {
    char message[GUILD_MESSAGE_CAP];
    const char *args[] = {g->applied.name};
    LocFormatArgs(message, sizeof(message), "GUILD_CANCEL_CONFIRMATION_MESSAGE", args, 1);
    StrCopy(v->target_id, sizeof(v->target_id), g->applied.id);
    GuildViewShowConfirm(v, GUILD_CONFIRM_CANCEL_APPLICATION, "GUILD_CANCEL_CONFIRMATION_TITLE", message);
}

static void GuildViewDrawFind(GuildView *v, Guild *g, bool interactive) {
    Rectangle top_left = GuildRect(86.0f, 161.0f, 603.0f, 103.0f);
    Rectangle top_right = GuildRect(697.0f, 161.0f, 501.0f, 103.0f);
    GuildDrawPanel(top_left);
    GuildDrawPanel(top_right);
    GuildFitText(
        v->font,
        LocGet("GUILD_FIND_TOP_LEFT_TITLE"),
        GuildRect(98.0f, 169.0f, 551.0f, 30.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    GuildDrawWrapped(
        v->body,
        LocGet("GUILD_FIND_TOP_LEFT_BODY"),
        GuildRect(98.0f, 206.0f, 580.0f, 56.0f),
        GUILD_BODY_SIZE,
        GUILD_TEXT
    );
    GuildFitText(
        v->font,
        LocGet("GUILD_FIND_TOP_RIGHT_TITLE"),
        GuildRect(708.0f, 165.0f, 451.0f, 30.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    bool search_clicked = GuildTextButton(
        v,
        &v->grey,
        LocGet("GUILD_FIND_SEARCH_BUTTON"),
        GuildRect(704.0f, 213.0f, 168.0f, 42.0f),
        v->search.text[0] != '\0',
        interactive
    );
    TextFieldEvent search_event = TextFieldDraw(
        &v->search,
        GuildRect(882.0f, 213.0f, 306.0f, 42.0f),
        v->body,
        GUILD_BODY_SIZE,
        LocGet("GUILD_APPLICATION_TEXTBOX"),
        interactive
    );
    if (search_clicked || search_event == TEXT_FIELD_SUBMITTED) GuildViewSearch(v, g);

    Rectangle middle = GuildRect(364.0f, 270.0f, 556.0f, 77.0f);
    GuildDrawPanel(middle);
    static const char *const list_titles[] = {
        "GUILD_FIND_RECOMMENDED_GUILDS", "GUILD_FIND_SEARCH_RESULTS", "GUILD_FIND_SEARCH_APPLICATIONS"
    };
    GuildFitText(
        v->font,
        LocGet(list_titles[v->find_state]),
        GuildRect(376.0f, 278.0f, 532.0f, 30.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        true
    );
    if (v->find_state != GUILD_FIND_APPLICATIONS)
        GuildFitText(
            v->body,
            LocGet("GUILD_FIND_INSTRUCTIONAL"),
            GuildRect(376.0f, 310.0f, 532.0f, 26.0f),
            GUILD_SMALL_SIZE,
            GUILD_TEXT_DIM,
            true
        );
    GuildViewDrawFindList(v, g, interactive);

    GuildDrawRule(GuildRect(97.0f, 616.0f, 1090.0f, 2.0f));
    Rectangle apps = GuildRect(91.0f, 622.0f, 190.0f, 46.0f);
    if (GuildTextButton(
            v,
            &v->grey,
            LocGet("GUILD_FIND_APPLICATIONS_BUTTON"),
            apps,
            g->has_application && v->find_state != GUILD_FIND_APPLICATIONS,
            interactive
        ))
        GuildViewShowFind(v, g, GUILD_FIND_APPLICATIONS);
    if (g->has_application) GuildDrawBadge(v, GuildRect(258.0f, 616.0f, 23.0f, 23.0f), 1);
    if (v->find_state == GUILD_FIND_APPLICATIONS) {
        if (ButtonDrawEx(&v->back, GuildRect(854.0f, 625.0f, 60.0f, 40.0f), interactive))
            GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
        if (GuildTextButton(
                v,
                &v->grey,
                LocGet("GUILD_FIND_CANCEL_REQUEST"),
                GuildRect(934.0f, 622.0f, 256.0f, 46.0f),
                g->has_application,
                interactive
            ))
            GuildViewCancelPressed(v, g);
        return;
    }
    if (v->find_state == GUILD_FIND_SEARCH &&
        ButtonDrawEx(&v->back, GuildRect(949.0f, 625.0f, 60.0f, 40.0f), interactive))
        GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
    if (GuildTextButton(
            v, &v->gold, LocGet("GUILD_FIND_CREATE_GUILD"), GuildRect(1022.0f, 622.0f, 167.0f, 46.0f), true, interactive
        ))
        GuildViewCreatePressed(v, g);
}

static void GuildViewDrawEmblemStrip(GuildView *v, bool editable, bool interactive) {
    Rectangle chosen = GuildRect(90.0f, 270.0f, 77.0f, 77.0f);
    DrawRectangleRec(chosen, GUILD_HEADER_TINT);
    GuildDrawEmblem(v, v->draft.icon, UiRect(chosen, 0.5f, 0.5f, 0.82f, 0.82f), WHITE);
    UiDrawTexture(GuiTex(GUI_GUILD_EMBLEM_FRAME), chosen, WHITE);
    Rectangle strip = GuildRect(189.0f, 270.0f, 1002.0f, 77.0f);
    GuildDrawPanel(strip);
    Rectangle window = GuildRect(265.0f, 270.0f, 850.0f, 77.0f);
    float stride = GUILD_EMBLEM_SLOT + GUILD_EMBLEM_GAP;
    float max_offset = fmaxf(GUILD_EMBLEM_COUNT * stride - GUILD_EMBLEM_GAP - window.width, 0.0f);
    Rectangle left = GuildRect(189.0f, 274.0f, 70.0f, 70.0f);
    Rectangle right = GuildRect(1121.0f, 274.0f, 70.0f, 70.0f);
    float left_center_x = left.x + left.width * 0.5f, left_center_y = left.y + left.height * 0.5f;
    Texture2D arrow = GuiTex(GUI_GUILD_ARROW);
    bool left_hover = interactive && CheckCollisionPointRec(UiMouse(), left);
    DrawTexturePro(
        left_hover ? GuiTex(GUI_GUILD_ARROW_ON) : arrow,
        (Rectangle){0.0f, 0.0f, (float)-arrow.width, (float)arrow.height},
        (Rectangle){left_center_x, left_center_y, left.width, left.height},
        (Vector2){left.width * 0.5f, left.height * 0.5f},
        0.0f,
        WHITE
    );
    if (left_hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        GuiSoundPlay(SND_UI_CLICK);
        v->emblem_target -= stride * GUILD_EMBLEM_SCROLL_STEP;
    }
    if (ButtonDrawEx(&v->arrow_right, right, interactive)) v->emblem_target += stride * GUILD_EMBLEM_SCROLL_STEP;
    v->emblem_target = Clamp(v->emblem_target, 0.0f, max_offset);
    float step = stride * GUILD_EMBLEM_SCROLL_STEP * GetFrameTime() / GUILD_EMBLEM_SCROLL_SECONDS;
    if (fabsf(v->emblem_target - v->emblem_offset) <= step)
        v->emblem_offset = v->emblem_target;
    else
        v->emblem_offset += v->emblem_target > v->emblem_offset ? step : -step;
    GoalsBeginClip(window);
    for (int i = 0; i < GUILD_EMBLEM_COUNT; i++) {
        Rectangle slot = {
            window.x + i * stride - v->emblem_offset, window.y + 2.5f, GUILD_EMBLEM_SLOT, GUILD_EMBLEM_SLOT
        };
        if (slot.x + slot.width < window.x || slot.x > window.x + window.width) continue;
        bool selected = v->draft.icon == i + 1;
        GuildDrawEmblem(v, i + 1, UiRect(slot, 0.5f, 0.5f, 0.8f, 0.8f), selected ? GUILD_TEXT_GOLD : WHITE);
        if (selected) UiDrawTexture(GuiTex(GUI_GUILD_EMBLEM_FRAME), slot, WHITE);
        bool hit = CheckCollisionPointRec(UiMouse(), slot) && CheckCollisionPointRec(UiMouse(), window);
        if (editable && interactive && hit && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            GuiSoundPlay(SND_UI_CLICK);
            v->draft.icon = i + 1;
        }
    }
    EndScissorMode();
}

static bool GuildStatusBlocked(GuildView *v, const Guild *g) {
    if (!v->settings_mode || g->mine.summary.status != GUILD_STATUS_APPLICATION_ONLY || g->mine.applicant_count == 0)
        return false;
    GuildViewShowKeys(v, "ERROR_MUST_RESOLVE_APPLICATIONS_TITLE", "ERROR_MUST_RESOLVE_APPLICATIONS_MESSAGE_STRING");
    return true;
}

static void GuildViewDrawStatus(GuildView *v, const Guild *g, bool editable, bool interactive) {
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_STATUS"),
        GuildRect(760.0f, 163.0f, 397.0f, 31.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    static const float status_x[GUILD_STATUS_COUNT] = {757.0f, 904.0f, 1052.0f};
    for (int status = 0; status < GUILD_STATUS_COUNT; status++) {
        Rectangle option = GuildRect(status_x[status], 202.0f, 133.0f, 62.0f);
        bool selected = (int)v->draft.status == status;
        UiDrawTexture(
            GuiTex(selected ? GUI_GUILD_STATUS_ON : GUI_GUILD_STATUS),
            (Rectangle){option.x, option.y + 4.0f, 54.0f, 54.0f},
            WHITE
        );
        GuildFitText(
            v->body,
            LocGet(GuildStatusKey((GuildStatus)status)),
            (Rectangle){option.x + 56.0f, option.y + 12.0f, 77.0f, 38.0f},
            GUILD_BODY_SIZE,
            selected ? GUILD_TEXT_GOLD : GUILD_TEXT,
            false
        );
        bool hit = interactive && editable && CheckCollisionPointRec(UiMouse(), option);
        if (!hit || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || selected) continue;
        GuiSoundPlay(SND_UI_CLICK);
        if (!GuildStatusBlocked(v, g)) v->draft.status = (GuildStatus)status;
    }
}

static void GuildViewMinimumLevelBlurred(GuildView *v, const Guild *g) {
    int level = GuildParseLevel(v->min_level.text);
    bool blocked = level != v->draft.required_level && GuildStatusBlocked(v, g);
    if (blocked) {
        TextFieldSet(&v->min_level, TextFormat("%d", v->draft.required_level));
        return;
    }
    if (level < GUILD_LEVEL_UNLOCK || level > GUILD_MAX_REQUIRED_LEVEL) {
        char message[GUILD_MESSAGE_CAP];
        LocFormat2(
            message,
            sizeof(message),
            "ERROR_INVALID_MINIMUM_LEVEL_MESSAGE_PARAMS",
            TextFormat("%d", GUILD_LEVEL_UNLOCK),
            TextFormat("%d", GUILD_MAX_REQUIRED_LEVEL)
        );
        GuildViewShowMessage(v, "ERROR_INVALID_MINIMUM_LEVEL_TITLE", message);
        level = v->settings_mode ? g->mine.summary.required_level : GUILD_LEVEL_UNLOCK;
        TextFieldSet(&v->min_level, TextFormat("%d", level));
    }
    v->draft.required_level = level;
}

static void GuildViewFieldStripped(GuildView *v, TextField *field, const char *title_key, const char *message_key) {
    if (TextFieldTakeStripped(field)) GuildViewShowKeys(v, title_key, message_key);
}

static bool GuildViewValidate(GuildView *v) {
    int name_runes = TextFieldRuneCount(v->name.text);
    if (name_runes < GUILD_NAME_MIN_RUNES || name_runes > GUILD_NAME_MAX_RUNES) {
        GuildViewShowKeys(v, "ERROR_INVALID_GUILD_NAME_TITLE", "ERROR_INVALID_GUILD_NAME_MESSAGE");
        return false;
    }
    if (TextFieldRuneCount(v->description.text) > GUILD_DESCRIPTION_MAX_RUNES) {
        GuildViewShowKeys(v, "ERROR_INVALID_GUILD_DESCRIPTION_TITLE", "ERROR_INVALID_GUILD_DESCRIPTION_MESSAGE");
        return false;
    }
    if (TextFieldRuneCount(v->motd.text) > GUILD_MOTD_MAX_RUNES) {
        GuildViewShowKeys(v, "ERROR_INVALID_MOTD_TITLE", "ERROR_INVALID_MOTD_MESSAGE");
        return false;
    }
    if (v->draft.icon < 1 || v->draft.icon > GUILD_EMBLEM_COUNT) {
        GuildViewShowKeys(v, "ERROR_NO_EMBLEM_TITLE", "ERROR_NO_EMBLEM_MESSAGE");
        return false;
    }
    return v->draft.required_level >= GUILD_LEVEL_UNLOCK && v->draft.required_level <= GUILD_MAX_REQUIRED_LEVEL;
}

static void GuildViewCollectDraft(GuildView *v) {
    StrCopy(v->draft.name, sizeof(v->draft.name), v->name.text);
    StrCopy(v->draft.description, sizeof(v->draft.description), v->description.text);
    StrCopy(v->draft.motd, sizeof(v->draft.motd), v->motd.text);
}

static bool GuildDraftChanged(const GuildView *v, const Guild *g) {
    const GuildSummary *s = &g->mine.summary;
    return strcmp(v->draft.description, s->description) || strcmp(v->draft.motd, g->mine.motd) ||
           strcmp(v->draft.language, s->language) || v->draft.status != s->status || v->draft.icon != s->icon ||
           v->draft.required_level != s->required_level;
}

static void GuildViewSubmitCreate(GuildView *v, Guild *g, const Player *p) {
    GuildViewCollectDraft(v);
    if (!GuildViewValidate(v)) return;
    if (v->settings_mode) {
        if (!GuildDraftChanged(v, g)) {
            GuildViewShowHome(v, GUILD_HOME_MEMBER);
            return;
        }
        GuildRequestSettings(g, &v->draft);
        GuildViewWait(v, GUILD_REQUEST_SETTINGS);
        return;
    }
    if (p->gold < GUILD_CREATION_GOLD_COST) {
        GuildViewShowKeys(v, "ERROR_NOT_ENOUGH_GOLD_TITLE", "ERROR_NOT_ENOUGH_GOLD_MESSAGE");
        return;
    }
    char message[GUILD_MESSAGE_CAP];
    LocFormat2(
        message,
        sizeof(message),
        "GUILD_CREATE_CREATE_GUILD_CONFIRMATION_MESSAGE",
        v->draft.name,
        TextFormat("%d", GUILD_INACTIVE_LEADER_SECONDS / GUILD_SECONDS_PER_DAY)
    );
    GuildViewShowConfirm(v, GUILD_CONFIRM_CREATE, "GUILD_CREATE_CREATE_GUILD_CONFIRMATION_TITLE", message);
}

static void GuildViewDrawCreateFields(GuildView *v, const Guild *g, bool editable, bool interactive) {
    GuildDrawPanel(GuildRect(90.0f, 161.0f, 651.0f, 103.0f));
    GuildDrawPanel(GuildRect(750.0f, 161.0f, 441.0f, 103.0f));
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_NAME"),
        GuildRect(100.0f, 169.0f, 529.0f, 31.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    TextFieldEvent name_event =
        TextFieldDraw(&v->name, GuildRect(100.0f, 206.0f, 620.0f, 46.0f), v->body, GUILD_HEADING_SIZE, "", interactive);
    if (v->settings_mode && interactive && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(UiMouse(), GuildRect(100.0f, 206.0f, 620.0f, 46.0f)))
        GuildViewShowKeys(v, "ERROR_GUILD_CANNOT_CHANGE_NAME_TITLE", "ERROR_GUILD_CANNOT_CHANGE_NAME_MESSAGE");
    if (name_event == TEXT_FIELD_BLURRED)
        GuildViewFieldStripped(v, &v->name, "ERROR_INVALID_GUILD_NAME_TITLE", "ERROR_INVALID_GUILD_NAME_MESSAGE");
    GuildViewDrawStatus(v, g, editable, interactive);
    GuildViewDrawEmblemStrip(v, editable, interactive);

    GuildDrawPanel(GuildRect(90.0f, 352.0f, 661.0f, 88.0f));
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_MINIMUM_LEVEL_TITLE"),
        GuildRect(100.0f, 359.0f, 529.0f, 27.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    if (TextFieldDraw(
            &v->min_level, GuildRect(100.0f, 391.0f, 160.0f, 42.0f), v->body, GUILD_HEADING_SIZE, "", interactive
        ) != TEXT_FIELD_IDLE)
        GuildViewMinimumLevelBlurred(v, g);

    Rectangle language_panel = GuildRect(761.0f, 352.0f, 430.0f, 88.0f);
    GuildDrawPanel(language_panel);
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_LANGUAGE_STRING"),
        GuildRect(804.0f, 359.0f, 345.0f, 27.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        true
    );
    if (GuildTextButton(
            v,
            &v->grey,
            GuildLanguageName(v->draft.language),
            GuildRect(858.0f, 390.0f, 237.0f, 44.0f),
            editable,
            interactive
        ))
        v->dialog = GUILD_DIALOG_LANGUAGE;

    GuildDrawPanel(GuildRect(90.0f, 446.0f, 1101.0f, 88.0f));
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_DESCRIPTION_TITLE"),
        GuildRect(100.0f, 451.0f, 881.0f, 27.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    if (TextFieldDraw(
            &v->description, GuildRect(100.0f, 482.0f, 1081.0f, 44.0f), v->body, GUILD_BODY_SIZE, "", interactive
        ) == TEXT_FIELD_BLURRED)
        GuildViewFieldStripped(
            v, &v->description, "ERROR_INVALID_GUILD_DESCRIPTION_TITLE", "ERROR_INVALID_GUILD_DESCRIPTION_MESSAGE"
        );

    GuildDrawPanel(GuildRect(90.0f, 540.0f, 1101.0f, 77.0f));
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_MOTD_TITLE"),
        GuildRect(100.0f, 543.0f, 881.0f, 27.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    if (TextFieldDraw(&v->motd, GuildRect(100.0f, 572.0f, 1081.0f, 40.0f), v->body, GUILD_BODY_SIZE, "", interactive) ==
        TEXT_FIELD_BLURRED)
        GuildViewFieldStripped(v, &v->motd, "ERROR_INVALID_MOTD_TITLE", "ERROR_INVALID_MOTD_MESSAGE");
}

static void GuildViewDrawCreate(GuildView *v, Guild *g, const Player *p, bool interactive) {
    bool editable = !v->settings_mode || GuildCanEdit(g);
    GuildViewDrawCreateFields(v, g, editable, interactive);
    if (v->settings_mode) {
        if (!editable)
            GuildFitText(
                v->body,
                LocGet("GUILD_EDIT_DISCLAIMER"),
                GuildRect(100.0f, 625.0f, 661.0f, 40.0f),
                GUILD_BODY_SIZE,
                GUILD_TEXT_DIM,
                false
            );
        if (ButtonDrawEx(&v->back, GuildRect(955.0f, 625.0f, 60.0f, 40.0f), interactive))
            GuildViewShowHome(v, GUILD_HOME_MEMBER);
        if (GuildTextButton(
                v,
                &v->gold,
                LocGet("GUILD_CREATE_SAVE_BUTTON"),
                GuildRect(1025.0f, 622.0f, 166.0f, 46.0f),
                editable,
                interactive
            ))
            GuildViewSubmitCreate(v, g, p);
        return;
    }
    if (ButtonDrawEx(&v->back, GuildRect(819.0f, 625.0f, 60.0f, 40.0f), interactive))
        GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
    Rectangle create = GuildRect(893.0f, 620.0f, 298.0f, 49.0f);
    v->gold.label = NULL;
    if (ButtonDrawEx(&v->gold, create, interactive)) GuildViewSubmitCreate(v, g, p);
    char cost[GUILD_TIME_TEXT_CAP];
    GuildAbbreviate(cost, sizeof(cost), GUILD_CREATION_GOLD_COST);
    float cost_width = MeasureTextEx(v->font, cost, GUILD_BODY_SIZE, 1.0f).x;
    Rectangle cost_rect = GuildRect(1128.0f - cost_width, 631.0f, cost_width, 27.0f);
    UiDrawTextRight(v->font, cost, cost_rect, GUILD_BODY_SIZE, GUILD_TEXT);
    Rectangle label = GuildRect(910.0f, 631.0f, cost_rect.x - GuildLeft() - 918.0f, 27.0f);
    GuildFitText(v->font, LocGet("GUILD_CREATE_CREATE_GUILD_BUTTON"), label, GUILD_BODY_SIZE, GUILD_TEXT, true);
    UiDrawTexture(GuiTex(GUI_MISSION_GOLD), GuildRect(1133.0f, 629.0f, 32.0f, 32.0f), WHITE);
}

static void GuildDrawHeader(const GuildView *v, const GuildInfo *info) {
    const GuildSummary *s = &info->summary;
    GuildDrawPanel(GuildRect(86.0f, 161.0f, 556.0f, 103.0f));
    GuildDrawPanel(GuildRect(642.0f, 161.0f, 556.0f, 103.0f));
    GuildDrawEmblem(v, s->icon, GuildRect(86.0f, 171.0f, 84.0f, 84.0f), WHITE);
    GuildFitText(
        v->font, s->name, GuildRect(184.0f, 172.0f, 417.0f, 32.0f), GUILD_HEADING_SIZE + 4.0f, GUILD_TEXT, false
    );
    GuildFitText(
        v->body,
        LocGet(GuildStatusKey(s->status)),
        GuildRect(184.0f, 214.0f, 417.0f, 26.0f),
        GUILD_BODY_SIZE,
        GUILD_TEXT_DIM,
        false
    );
    GuildFitText(
        v->font,
        LocGet("GUILD_HOME_GUILD_DESCRIPTION_TITLE"),
        GuildRect(652.0f, 176.0f, 501.0f, 24.0f),
        GUILD_BODY_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    GuildDrawWrapped(v->body, s->description, GuildRect(652.0f, 203.0f, 535.0f, 59.0f), GUILD_SMALL_SIZE, GUILD_TEXT);
    GuildFitText(
        v->font,
        LocGet("GUILD_CREATE_GUILD_MOTD_TITLE"),
        GuildRect(92.0f, 270.0f, 322.0f, 24.0f),
        GUILD_BODY_SIZE,
        GUILD_TEXT_GOLD,
        false
    );
    GuildDrawWrapped(v->body, info->motd, GuildRect(419.0f, 273.0f, 779.0f, 40.0f), GUILD_SMALL_SIZE, GUILD_TEXT);
    GuildDrawRule(GuildRect(86.0f, 313.0f, 1112.0f, 3.0f));
}

static void GuildViewMemberAction(GuildView *v, GuildConfirm confirm, const GuildMember *m, const Guild *g) {
    char message[GUILD_MESSAGE_CAP];
    const char *args[] = {m->name};
    StrCopy(v->target_id, sizeof(v->target_id), m->id);
    StrCopy(v->target_name, sizeof(v->target_name), m->name);
    v->target_rank = m->rank;
    if (confirm == GUILD_CONFIRM_KICK) {
        LocFormatArgs(message, sizeof(message), "GUILD_MEMBER_KICK_CONFIRMATION_MESSAGE", args, 1);
        GuildViewShowConfirm(v, confirm, "GUILD_MEMBER_KICK_CONFIRMATION_TITLE", message);
    } else if (confirm == GUILD_CONFIRM_DEMOTE) {
        LocFormatArgs(message, sizeof(message), "GUILD_MEMBER_DEMOTE_CONFIRMATION_MESSAGE", args, 1);
        GuildViewShowConfirm(v, confirm, "GUILD_MEMBER_DEMOTE_CONFIRMATION_TITLE", message);
    } else if (g->rank == GUILD_RANK_LEADER && m->rank == GUILD_RANK_OFFICER) {
        LocFormatArgs(message, sizeof(message), "GUILD_PROMOTE_NEW_LEADER_CONFIRMATION_MESSAGE", args, 1);
        GuildViewShowConfirm(v, confirm, "GUILD_PROMOTE_NEW_LEADER_CONFIRMATION_TITLE", message);
    } else {
        LocFormatArgs(message, sizeof(message), "GUILD_MEMBER_PROMOTE_CONFIRMATION_MESSAGE", args, 1);
        GuildViewShowConfirm(v, confirm, "GUILD_MEMBER_PROMOTE_CONFIRMATION_TITLE", message);
    }
}

static bool GuildRowButton(GuildView *v, const char *key, Rectangle rect, bool enabled, bool interactive) {
    return GuildTextButton(v, &v->row_button, LocGet(key), UiRect(rect, 0.5f, 0.5f, 0.94f, 0.8f), enabled, interactive);
}

static void GuildDrawMemberRow(
    GuildView *v,
    Guild *g,
    const GuildMember *m,
    Rectangle row,
    int index,
    bool member_mode,
    Rectangle list,
    bool interactive
) {
    float h = GUILD_MEMBER_SMART_HEIGHT;
    DrawRectangleRec(row, index % 2 ? GUILD_ROW_ODD : GUILD_ROW_EVEN);
    bool clip_ok = CheckCollisionPointRec(UiMouse(), list);
    bool row_interactive = interactive && clip_ok;
    UiDrawTextCentered(
        v->font, TextFormat("%d", m->level), GuildSmart(row, h, 13.0f, 0.0f, 65.0f, h), GUILD_HEADING_SIZE, GUILD_TEXT
    );
    bool me = !strcmp(m->id, GuildPlayerId());
    GuildFitText(
        v->body,
        m->name,
        GuildSmart(row, h, 108.0f, 0.0f, 385.0f, h),
        GUILD_BODY_SIZE,
        me ? GUILD_TEXT_GOLD : GUILD_TEXT,
        false
    );
    GuildFitText(
        v->body,
        LocGet(GuildRankKey(m->rank)),
        GuildSmart(row, h, 522.0f, 0.0f, 193.0f, h),
        GUILD_BODY_SIZE,
        GUILD_TEXT,
        false
    );
    bool expanded = member_mode && v->expanded_member == index;
    if (!expanded) {
        char last[GUILD_TIME_TEXT_CAP];
        GuildFormatLastLogin(last, sizeof(last), ServerNow() - m->last_login);
        GuildFitText(
            v->body, last, GuildSmart(row, h, 729.0f, 0.0f, 193.0f, h), GUILD_BODY_SIZE, GUILD_TEXT_DIM, false
        );
    }
    if (!member_mode) return;
    if (!expanded && GuildRowButton(
                         v,
                         "GUILD_MEMBER_DUEL_BUTTON",
                         GuildSmart(row, h, 1048.0f, 18.0f, 154.0f, 145.0f),
                         !me,
                         row_interactive && !v->list_scroll.drag_scrolled
                     )) {
        GuildRequestDuel(g, m->id);
        GuildViewWait(v, GUILD_REQUEST_DUEL);
    }
    Rectangle rollout = GuildSmart(row, h, 1215.0f, 18.0f, 65.0f, 145.0f);
    Texture2D arrow = GuiTex(CheckCollisionPointRec(UiMouse(), rollout) ? GUI_GUILD_ROW_ARROW_ON : GUI_GUILD_ROW_ARROW);
    float side = fminf(rollout.width, rollout.height);
    Rectangle arrow_rect = {rollout.x + rollout.width * 0.5f, rollout.y + rollout.height * 0.5f, side, side};
    DrawTexturePro(
        arrow,
        (Rectangle){0.0f, 0.0f, (float)(expanded ? -arrow.width : arrow.width), (float)arrow.height},
        arrow_rect,
        (Vector2){side * 0.5f, side * 0.5f},
        0.0f,
        WHITE
    );
    if (row_interactive && CheckCollisionPointRec(UiMouse(), rollout) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        !v->list_scroll.drag_scrolled) {
        GuiSoundPlay(SND_UI_CLICK);
        v->expanded_member = expanded ? -1 : index;
    }
    if (!expanded) return;
    bool promote = g->rank == GUILD_RANK_LEADER && m->rank != GUILD_RANK_LEADER && !me;
    bool demote = g->rank == GUILD_RANK_LEADER && m->rank == GUILD_RANK_OFFICER;
    bool kick = g->rank != GUILD_RANK_NOT_MEMBER && g->rank > m->rank && m->rank != GUILD_RANK_LEADER && !me;
    if (GuildRowButton(
            v,
            "GUILD_MEMBER_PROMOTE_BUTTON",
            GuildSmart(row, h, 732.0f, 18.0f, 154.0f, 145.0f),
            promote,
            row_interactive
        ))
        GuildViewMemberAction(v, GUILD_CONFIRM_PROMOTE, m, g);
    if (GuildRowButton(
            v, "GUILD_MEMBER_DEMOTE_BUTTON", GuildSmart(row, h, 890.0f, 18.0f, 154.0f, 145.0f), demote, row_interactive
        ))
        GuildViewMemberAction(v, GUILD_CONFIRM_DEMOTE, m, g);
    if (GuildRowButton(
            v, "GUILD_MEMBER_KICK_BUTTON", GuildSmart(row, h, 1048.0f, 18.0f, 154.0f, 145.0f), kick, row_interactive
        ))
        GuildViewMemberAction(v, GUILD_CONFIRM_KICK, m, g);
}

static void GuildViewReview(GuildView *v, const GuildApplicant *a) {
    StrCopy(v->target_id, sizeof(v->target_id), a->id);
    StrCopy(v->target_name, sizeof(v->target_name), a->name);
    StrCopy(v->review_text, sizeof(v->review_text), a->text);
    v->dialog = GUILD_DIALOG_REVIEW;
}

static void GuildViewAccept(GuildView *v, Guild *g, const char *player_id) {
    GuildRequestAccept(g, player_id);
    GuildViewWait(v, GUILD_REQUEST_ACCEPT);
}

static void GuildViewDeny(GuildView *v, Guild *g, const char *player_id) {
    GuildRequestDeny(g, player_id);
    GuildViewWait(v, GUILD_REQUEST_DENY);
}

static void GuildDrawApplicantRow(
    GuildView *v, Guild *g, const GuildApplicant *a, Rectangle row, int index, Rectangle list, bool interactive
) {
    float h = GUILD_MEMBER_SMART_HEIGHT;
    DrawRectangleRec(row, index % 2 ? GUILD_ROW_ODD : GUILD_ROW_EVEN);
    bool row_interactive = interactive && CheckCollisionPointRec(UiMouse(), list);
    UiDrawTextCentered(
        v->font, TextFormat("%d", a->level), GuildSmart(row, h, 13.0f, 0.0f, 65.0f, h), GUILD_HEADING_SIZE, GUILD_TEXT
    );
    GuildFitText(v->body, a->name, GuildSmart(row, h, 108.0f, 0.0f, 385.0f, h), GUILD_BODY_SIZE, GUILD_TEXT, false);
    GuildFitText(
        v->body,
        LocGet("GUILD_MEMBER_PENDING"),
        GuildSmart(row, h, 522.0f, 0.0f, 193.0f, h),
        GUILD_BODY_SIZE,
        GUILD_TEXT_DIM,
        false
    );
    if (!GuildCanEdit(g)) return;
    if (GuildRowButton(
            v, "GUILD_MEMBER_ACCEPT_BUTTON", GuildSmart(row, h, 732.0f, 18.0f, 154.0f, 145.0f), true, row_interactive
        ))
        GuildViewAccept(v, g, a->id);
    if (GuildRowButton(
            v, "GUILD_MEMBER_DENY_BUTTON", GuildSmart(row, h, 901.0f, 18.0f, 154.0f, 145.0f), true, row_interactive
        ))
        GuildViewDeny(v, g, a->id);
    if (GuildRowButton(
            v,
            "GUILD_APPLICATION_INFO_BUTTON",
            GuildSmart(row, h, 1070.0f, 18.0f, 154.0f, 145.0f),
            true,
            row_interactive
        ))
        GuildViewReview(v, a);
}

static void GuildViewJoinPressed(GuildView *v, Guild *g) {
    const GuildSummary *s = &g->observed.summary;
    StrCopy(v->target_id, sizeof(v->target_id), s->id);
    StrCopy(v->target_name, sizeof(v->target_name), s->name);
    if (g->has_application && strcmp(g->applied.id, s->id)) {
        char message[GUILD_MESSAGE_CAP];
        LocFormat2(message, sizeof(message), "ERROR_MULTIPLE_APPLICATIONS_MESSAGE", s->name, g->applied.name);
        GuildViewShowConfirm(v, GUILD_CONFIRM_JOIN_OVER_APPLICATION, "ERROR_MULTIPLE_APPLICATIONS_TITLE", message);
        return;
    }
    if (s->status == GUILD_STATUS_APPLICATION_ONLY) {
        TextFieldClear(&v->application);
        v->dialog = GUILD_DIALOG_APPLY;
        return;
    }
    GuildRequestJoin(g, s->id, "");
    GuildViewWait(v, GUILD_REQUEST_JOIN);
}

static void GuildViewDrawHomeFooter(GuildView *v, Guild *g, const Player *p, bool interactive) {
    if (v->home_mode == GUILD_HOME_MEMBER) {
        GuildViewHandleFooter(v, g, GuildDrawFooter(v, g, GUILD_TAB_HOME, true, false, interactive));
        return;
    }
    if (v->home_mode == GUILD_HOME_APPLICATIONS) {
        GuildDrawFooter(v, g, GUILD_TAB_HOME, false, true, interactive);
        if (ButtonDrawEx(&v->back, GuildRect(1132.0f, 629.0f, 55.5f, 37.0f), interactive))
            GuildViewShowHome(v, GUILD_HOME_MEMBER);
        return;
    }
    GuildDrawRule(GuildRect(97.0f, 621.0f, 1090.0f, 2.0f));
    if (ButtonDrawEx(&v->back, GuildRect(953.0f, 629.0f, 55.5f, 37.0f), interactive))
        GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
    const GuildSummary *s = &g->observed.summary;
    if (ButtonDrawEx(&v->report, GuildRect(240.0f, 624.0f, 56.0f, 47.0f), interactive))
        GuildViewOpenReport(v, GUILD_REPORT_GUILD, s->id, "", "");
    Rectangle action = GuildRect(1022.0f, 626.0f, 167.0f, 42.0f);
    if (g->observed_applied) {
        if (GuildTextButton(v, &v->grey, LocGet("GUILD_FIND_CANCEL_REQUEST"), action, true, interactive)) {
            char message[GUILD_MESSAGE_CAP];
            const char *args[] = {s->name};
            LocFormatArgs(message, sizeof(message), "GUILD_CANCEL_CONFIRMATION_MESSAGE", args, 1);
            StrCopy(v->target_id, sizeof(v->target_id), s->id);
            GuildViewShowConfirm(v, GUILD_CONFIRM_CANCEL_APPLICATION, "GUILD_CANCEL_CONFIRMATION_TITLE", message);
        }
        return;
    }
    bool joinable = s->status != GUILD_STATUS_CLOSED && p->level >= s->required_level && !GuildIsMember(g);
    const char *label =
        LocGet(s->status == GUILD_STATUS_APPLICATION_ONLY ? "GUILD_HOME_APPLY_BUTTON" : "GUILD_HOME_JOIN_BUTTON");
    if (GuildTextButton(v, &v->gold, label, action, joinable, interactive)) GuildViewJoinPressed(v, g);
}

static void GuildViewDrawHome(GuildView *v, Guild *g, const Player *p, bool interactive) {
    bool observe = v->home_mode == GUILD_HOME_OBSERVE;
    const GuildInfo *info = observe ? &g->observed : &g->mine;
    GuildDrawHeader(v, info);
    bool applications = v->home_mode == GUILD_HOME_APPLICATIONS;
    char count[GUILD_TIME_TEXT_CAP];
    if (applications)
        LocFormat2(
            count,
            sizeof(count),
            "GUILD_HOME_APPLICATION_COUNT_STRING",
            TextFormat("%d", info->applicant_count),
            TextFormat("%d", GUILD_MAX_APPLICATIONS)
        );
    else
        LocFormat2(
            count,
            sizeof(count),
            "GUILD_HOME_MEMBER_COUNT_STRING",
            TextFormat("%d", info->member_total),
            TextFormat("%d", GUILD_MAX_MEMBERS)
        );
    GuildFitText(v->body, count, GuildRect(92.0f, 337.0f, 390.0f, 26.0f), GUILD_BODY_SIZE, GUILD_TEXT, false);
    Rectangle header = GuildRect(86.0f, 365.0f, 1112.0f, 31.0f);
    DrawRectangleRec(header, GUILD_HEADER_TINT);
    UiDrawTextCentered(
        v->body,
        LocGet("GUILD_HOME_MEMBER_LEVEL"),
        GuildRect(86.0f, 371.0f, 80.0f, 19.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    UiDrawTextLeftMiddle(
        v->body,
        LocGet("GUILD_HOME_MEMBER_NAME"),
        GuildRect(180.0f, 371.0f, 223.0f, 19.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    UiDrawTextLeftMiddle(
        v->body,
        LocGet("GUILD_HOME_MEMBER_TITLE"),
        GuildRect(540.0f, 371.0f, 223.0f, 19.0f),
        GUILD_SMALL_SIZE,
        GUILD_TEXT_DIM
    );
    if (!applications)
        UiDrawTextLeftMiddle(
            v->body,
            LocGet("GUILD_LAST_LOGIN_TITLE"),
            GuildRect(720.0f, 371.0f, 223.0f, 19.0f),
            GUILD_SMALL_SIZE,
            GUILD_TEXT_DIM
        );
    Rectangle list = GuildRect(86.0f, 396.0f, 1112.0f, 222.0f);
    float row_height = list.height * GUILD_SMART_LIST_FRACTION;
    int rows = applications ? info->applicant_count : info->member_total;
    GoalsScrollUpdate(&v->list_scroll, list, row_height * rows);
    GoalsBeginClip(list);
    for (int i = 0; i < rows; i++) {
        Rectangle row = {list.x, list.y + i * row_height - v->list_scroll.offset, list.width, row_height};
        if (row.y + row.height < list.y || row.y > list.y + list.height) continue;
        if (applications)
            GuildDrawApplicantRow(v, g, &info->applicants[i], row, i, list, interactive);
        else
            GuildDrawMemberRow(v, g, &info->members[i], row, i, v->home_mode == GUILD_HOME_MEMBER, list, interactive);
    }
    EndScissorMode();
    GuildViewDrawHomeFooter(v, g, p, interactive);
}

static int GuildChatVisibleLines(const Guild *g, const GuildView *v, int *indices) {
    int count = 0;
    for (int i = 0; i < g->chat_count; i++)
        if (v->chat_mode == GUILD_CHAT_ALL || g->chat[i].event) indices[count++] = i;
    return count;
}

static void GuildViewDrawChatList(GuildView *v, Guild *g, bool interactive) {
    Rectangle list = GuildRect(84.0f, 158.0f, 1114.0f, 413.0f);
    GuildDrawPanel(list);
    int indices[GUILD_CHAT_CAPACITY];
    int count = GuildChatVisibleLines(g, v, indices);
    float one = list.height * GUILD_CHAT_ONE_LINE_FRACTION, two = list.height * GUILD_CHAT_TWO_LINE_FRACTION;
    Rectangle message_width =
        GuildSmart((Rectangle){0.0f, 0.0f, list.width, one}, GUILD_CHAT_ONE_LINE_HEIGHT, 31.0f, 43.0f, 1025.0f, 44.0f);
    float heights[GUILD_CHAT_CAPACITY];
    float total = 0.0f;
    for (int i = 0; i < count; i++) {
        float width = MeasureTextEx(v->body, g->chat[indices[i]].text, GUILD_BODY_SIZE, 1.0f).x;
        heights[i] = width > message_width.width ? two : one;
        total += heights[i];
    }
    float limit = total > list.height ? total - list.height : 0.0f;
    if (v->chat_follow || v->chat_seen_count != count) {
        if (v->chat_follow) v->chat_scroll.offset = limit;
        v->chat_seen_count = count;
    }
    GoalsScrollUpdate(&v->chat_scroll, list, total);
    v->chat_follow = v->chat_scroll.offset >= limit - 1.0f;
    GoalsBeginClip(list);
    float y = list.y - v->chat_scroll.offset;
    long long now = ServerNow();
    for (int i = 0; i < count; i++) {
        const GuildChatLine *line = &g->chat[indices[i]];
        Rectangle row = {list.x, y, list.width, heights[i]};
        y += heights[i];
        if (row.y + row.height < list.y || row.y > list.y + list.height) continue;
        bool two_lines = heights[i] > one;
        float smart = two_lines ? GUILD_CHAT_TWO_LINE_HEIGHT : GUILD_CHAT_ONE_LINE_HEIGHT;
        DrawRectangleRec(row, i % 2 ? GUILD_ROW_ODD : GUILD_ROW_EVEN);
        GuildFitText(
            v->font,
            line->title,
            GuildSmart(row, smart, 33.0f, 0.0f, 1080.0f, two_lines ? 43.0f : 44.0f),
            GUILD_BODY_SIZE,
            GUILD_CHAT_TITLE_COLOR,
            false
        );
        Rectangle message =
            GuildSmart(row, smart, 31.0f, two_lines ? 42.0f : 43.0f, 1025.0f, two_lines ? 88.0f : 44.0f);
        if (two_lines)
            GuildDrawWrapped(v->body, line->text, message, GUILD_BODY_SIZE, line->text_color);
        else
            UiDrawTextLeftMiddle(v->body, line->text, message, GUILD_BODY_SIZE, line->text_color);
        char ago[GUILD_TIME_TEXT_CAP];
        GuildFormatAgo(ago, sizeof(ago), now - line->time);
        GuildFitText(
            v->body, ago, GuildSmart(row, smart, 1100.0f, 0.0f, 165.0f, smart), GUILD_SMALL_SIZE, GUILD_TEXT_DIM, true
        );
    }
    EndScissorMode();
    (void)interactive;
}

static void GuildViewSend(GuildView *v, Guild *g) {
    char text[TEXT_FIELD_CAPACITY];
    StrCopy(text, sizeof(text), v->chat_input.text);
    int end = (int)strlen(text);
    while (end > 0 && isspace((unsigned char)text[end - 1])) text[--end] = '\0';
    const char *start = text;
    while (*start && isspace((unsigned char)*start)) start++;
    if (!*start) return;
    GuildSendChat(g, start);
    TextFieldClear(&v->chat_input);
    v->chat_follow = true;
}

static void GuildViewDrawChat(GuildView *v, Guild *g, bool interactive) {
    GuildSetChatVisible(g, true);
    GuildViewDrawChatList(v, g, interactive);
    if (v->chat_mode == GUILD_CHAT_ALL) {
        TextFieldEvent event = TextFieldDraw(
            &v->chat_input,
            GuildRect(90.0f, 573.0f, 406.0f, 43.0f),
            v->body,
            GUILD_BODY_SIZE,
            LocGet("GUILD_CHAT_TEXTFIELD_HELPER"),
            interactive
        );
        bool send = GuildTextButton(
            v,
            &v->gold,
            LocGet("GUILD_CHAT_SEND_BUTTON"),
            GuildRect(506.0f, 571.0f, 156.0f, 47.0f),
            v->chat_input.text[0] != '\0',
            interactive
        );
        if (send || event == TEXT_FIELD_SUBMITTED) GuildViewSend(v, g);
        if (event == TEXT_FIELD_SUBMITTED) v->chat_input.focused = true;
    }
    const char *mode_key = v->chat_mode == GUILD_CHAT_ALL ? "GUILD_EVENT_MODE_BUTTON" : "GUILD_CHAT_MODE_BUTTON";
    if (GuildTextButton(v, &v->grey, LocGet(mode_key), GuildRect(1010.0f, 571.0f, 179.0f, 47.0f), true, interactive)) {
        v->chat_mode = v->chat_mode == GUILD_CHAT_ALL ? GUILD_CHAT_EVENTS : GUILD_CHAT_ALL;
        v->chat_follow = true;
        TextFieldBlur(&v->chat_input);
    }
    GuildViewHandleFooter(v, g, GuildDrawFooter(v, g, GUILD_TAB_CHAT, true, false, interactive));
}

static void GuildDrawTitleBackground(Rectangle rect) {
    DrawRectangleRec(rect, GUILD_HEADER_TINT);
    GuildDrawRule((Rectangle){rect.x, rect.y + rect.height - 2.0f, rect.width, 2.0f});
}

static void GuildDrawEventName(const GuildView *v, const PvpEvent *event, long long seconds, Rectangle rect) {
    char time_text[GUILD_TIME_TEXT_CAP];
    QuestsShortTime(time_text, sizeof(time_text), seconds > 0 ? seconds : 0);
    const char *title = LocGet(event->title_key);
    const char *timer = TextFormat(" (%s)", time_text);
    float size = GUILD_BODY_SIZE;
    float title_width = MeasureTextEx(v->body, title, size, 1.0f).x;
    float timer_width = MeasureTextEx(v->body, timer, size, 1.0f).x;
    if (title_width + timer_width > rect.width) {
        float scale = rect.width / (title_width + timer_width);
        size *= scale;
        title_width *= scale;
        timer_width *= scale;
    }
    UiDrawTextLeftMiddle(v->body, title, (Rectangle){rect.x, rect.y, title_width, rect.height}, size, GUILD_TEXT);
    UiDrawTextLeftMiddle(
        v->body, timer, (Rectangle){rect.x + title_width, rect.y, timer_width, rect.height}, size, GUILD_TEXT_ORANGE
    );
}

static void GuildViewDrawWarHeader(GuildView *v, const Guild *g, const GuildWarEvents *events, long long now) {
    const GuildSummary *s = &g->mine.summary;
    GuildDrawPanel(GuildRect(86.0f, 161.0f, 445.0f, 103.0f));
    GuildDrawPanel(GuildRect(530.0f, 161.0f, 668.0f, 103.0f));
    GuildDrawEmblem(v, s->icon, GuildRect(86.0f, 179.0f, 67.0f, 67.0f), WHITE);
    GuildFitText(v->font, s->name, GuildRect(186.0f, 174.0f, 334.0f, 28.0f), GUILD_HEADING_SIZE, GUILD_TEXT, false);
    GuildFitText(
        v->body,
        LocGet(GuildStatusKey(s->status)),
        GuildRect(186.0f, 216.0f, 334.0f, 26.0f),
        GUILD_BODY_SIZE,
        GUILD_TEXT_DIM,
        false
    );
    Rectangle status = GuildRect(540.0f, 177.0f, 435.0f, 26.0f);
    Rectangle name = GuildRect(540.0f, 204.0f, 435.0f, 26.0f);
    if (events->state == GUILD_WAR_CURRENT_EVENT) {
        GuildFitText(
            v->body, LocGet("GUILD_WAR_GUILD_EVENT_RUNNING_NOW"), status, GUILD_BODY_SIZE, GUILD_TEXT_GOLD, false
        );
        long long end = PvpEventCycleStart(events->current + 1);
        GuildDrawEventName(v, PvpEventForCycle(events->current), end - now, name);
        return;
    }
    if (events->next < 0) return;
    GuildFitText(v->body, LocGet("GUILD_WAR_NEXT_GUILD_EVENT"), status, GUILD_BODY_SIZE, GUILD_TEXT_GOLD, false);
    GuildDrawEventName(v, PvpEventForCycle(events->next), PvpEventCycleStart(events->next) - now, name);
}

typedef struct {
    int rank;
    int icon;
    const char *name;
    int banners;
} GuildWarRow;

static void GuildDrawWarRow(const GuildView *v, const GuildWarRow *entry, Rectangle row, int index) {
    float h = GUILD_WAR_ROW_SMART_HEIGHT;
    DrawRectangleRec(row, index % 2 ? GUILD_ROW_ODD : GUILD_ROW_EVEN);
    float size = row.height * 0.42f;
    UiDrawTextCentered(
        v->font, TextFormat("%d", entry->rank), GuildSmart(row, h, 0.0f, 0.0f, 129.0f, h), size, GUILD_TEXT
    );
    GuildDrawEmblem(v, entry->icon, GuildSmart(row, h, 118.0f, 0.0f, 101.0f, h), WHITE);
    GuildFitText(v->body, entry->name, GuildSmart(row, h, 231.0f, 0.0f, 717.0f, h), size, GUILD_TEXT, false);
    UiDrawTextRight(
        v->font, TextFormat("%d", entry->banners), GuildSmart(row, h, 946.0f, 0.0f, 205.0f, h), size, GUILD_TEXT
    );
    UiDrawTexture(GuiTex(GUI_PVP_BANNER), GuildSmart(row, h, 1179.0f, 0.0f, 101.0f, h), WHITE);
}

static int
GuildWarRows(const GuildView *v, const Guild *g, const GuildOnslaught *o, GuildWarRow *rows, GuildWarRow *player) {
    int count = 0;
    if (v->war_board == GUILD_WAR_GUILDS) {
        for (int i = 0; i < o->guild_count; i++)
            rows[count++] =
                (GuildWarRow){o->guilds[i].rank, o->guilds[i].icon, o->guilds[i].name, o->guilds[i].banners};
        const GuildSummary *mine = &g->mine.summary;
        *player = o->has_mine ? (GuildWarRow){o->mine.rank, o->mine.icon, o->mine.name, o->mine.banners}
                              : (GuildWarRow){0, mine->icon, mine->name, 0};
        return count;
    }
    *player = (GuildWarRow){0};
    for (int i = 0; i < o->member_count; i++) {
        const GuildContribution *c = &o->members[i];
        rows[count++] = (GuildWarRow){c->rank, 0, c->name, c->banners};
        if (!strcmp(c->id, GuildPlayerId())) *player = rows[count - 1];
    }
    return count;
}

static void GuildDrawWarFooter(const GuildView *v, const GuildWarRow *player) {
    UiDrawTexture(GuiTex(GUI_GUILD_WAR_PLAYER_ROW), GuildRect(134.0f, 581.0f, 456.0f, 36.0f), WHITE);
    float size = GUILD_BODY_SIZE;
    UiDrawTextCentered(
        v->font, TextFormat("%d", player->rank), GuildRect(137.0f, 581.0f, 43.0f, 36.0f), size, GUILD_PLAYER_ROW_TEXT
    );
    GuildDrawEmblem(v, player->icon, GuildRect(177.0f, 581.0f, 36.0f, 36.0f), WHITE);
    GuildFitText(
        v->body,
        player->name ? player->name : "",
        GuildRect(218.0f, 581.0f, 228.0f, 36.0f),
        size,
        GUILD_PLAYER_ROW_TEXT,
        false
    );
    UiDrawTextRight(
        v->font, TextFormat("%d", player->banners), GuildRect(439.0f, 581.0f, 86.0f, 36.0f), size, GUILD_PLAYER_ROW_TEXT
    );
    UiDrawTexture(GuiTex(GUI_PVP_BANNER), GuildRect(530.0f, 581.0f, 36.0f, 36.0f), WHITE);
}

static Rectangle GuildWarToggleRect(int slot) {
    return GuildRect(
        GUILD_WAR_FRAME_VISIBLE_LEFT + slot * GUILD_WAR_TOGGLE_STEP,
        252.0f,
        GUILD_WAR_TOGGLE_SIZE,
        GUILD_WAR_TOGGLE_SIZE
    );
}

static void GuildViewDrawWarBoardToggles(GuildView *v, const Guild *g, bool interactive) {
    Rectangle guilds = GuildWarToggleRect(0);
    Rectangle members = GuildWarToggleRect(1);
    bool guild_board = v->war_board == GUILD_WAR_GUILDS;
    UiDrawTexture(GuiTex(guild_board ? GUI_GUILD_BUTTON : GUI_GUILD_BUTTON_OFF), guilds, WHITE);
    DrawRectangleRec(members, GUILD_HEADER_TINT);
    GuildDrawEmblem(v, g->mine.summary.icon, members, guild_board ? GUILD_DISABLED_TINT : WHITE);
    if (!interactive || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
    Vector2 mouse = UiMouse();
    GuildWarBoard pressed = v->war_board;
    if (CheckCollisionPointRec(mouse, guilds)) pressed = GUILD_WAR_GUILDS;
    if (CheckCollisionPointRec(mouse, members)) pressed = GUILD_WAR_MEMBERS;
    if (pressed == v->war_board && !v->war_help) return;
    if (!CheckCollisionPointRec(mouse, guilds) && !CheckCollisionPointRec(mouse, members)) return;
    GuiSoundPlay(SND_UI_CLICK);
    v->war_board = pressed;
    v->war_help = false;
    GoalsScrollReset(&v->war_scroll);
}

static void GuildViewDrawWarLeaderboard(GuildView *v, const Guild *g, const GuildOnslaught *o, bool interactive) {
    GuildDrawTitleBackground(GuildRect(GUILD_WAR_FRAME_VISIBLE_RIGHT - 334.0f, 248.0f, 334.0f, 50.0f));
    const char *title_key =
        v->war_board == GUILD_WAR_GUILDS ? "GUILD_WAR_LEADERBOARD_TITLE" : "GUILD_WAR_MY_GUILD_LEADERBOARD_TITLE";
    GuildFitTextRight(
        v->font,
        LocGet(title_key),
        GuildRect(276.0f, 258.0f, GUILD_WAR_FRAME_VISIBLE_RIGHT - 276.0f, 33.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD
    );
    GuildViewDrawWarBoardToggles(v, g, interactive);
    if (ButtonDrawEx(&v->help, GuildWarToggleRect(2), interactive)) v->war_help = !v->war_help;
    UiDrawTexture(GuiTex(GUI_GUILD_WAR_FRAME), GuildRect(90.0f, 294.0f, 545.0f, 324.0f), WHITE);
    Rectangle list = GuildRect(96.0f, 300.0f, 533.0f, 278.0f);
    if (v->war_help) {
        GuildDrawWrapped(
            v->body,
            LocGet("GUILD_WAR_LEADERBOARD_HELP"),
            UiRect(list, 0.5f, 0.5f, 0.9f, 0.9f),
            GUILD_BODY_SIZE,
            GUILD_TEXT
        );
        return;
    }
    static GuildWarRow rows[GUILD_ONSLAUGHT_LIMIT + GUILD_ONSLAUGHT_MEMBER_CAP];
    GuildWarRow player = {0};
    int count = o && o->ready ? GuildWarRows(v, g, o, rows, &player) : 0;
    if (o && !o->ready) {
        UiDrawTextCentered(v->body, "...", list, GUILD_HEADING_SIZE, GUILD_TEXT_DIM);
        return;
    }
    if (count == 0) {
        GuildDrawWrapped(
            v->body,
            LocGet("GUILD_WAR_NO_LEADERBOARD"),
            GuildRect(142.0f, 327.0f, 445.0f, 144.0f),
            GUILD_BODY_SIZE,
            GUILD_TEXT
        );
        return;
    }
    float row_height = list.width * GUILD_WAR_ROW_SMART_HEIGHT / GUILD_SMART_WIDTH;
    GoalsScrollUpdate(&v->war_scroll, list, row_height * count);
    GoalsBeginClip(list);
    for (int i = 0; i < count; i++) {
        Rectangle row = {list.x, list.y + i * row_height - v->war_scroll.offset, list.width, row_height};
        if (row.y + row.height < list.y || row.y > list.y + list.height) continue;
        GuildDrawWarRow(v, &rows[i], row, i);
    }
    EndScissorMode();
    GuildDrawWarFooter(v, &player);
}

static void GuildRankRewardLabel(char *out, size_t size, const PvpEventRankReward *tier) {
    if (tier->last >= GUILD_WAR_OPEN_RANK) {
        snprintf(out, size, "%d+", tier->first);
    } else if (tier->first == tier->last) {
        const char *args[] = {TextFormat("%d", tier->first)};
        LocFormatArgs(out, size, "GUILD_ONSLAUGHT_RANK_TITLE", args, 1);
    } else {
        snprintf(out, size, "%d - %d", tier->first, tier->last);
    }
}

static void GuildDrawRewardSlot(const GuildView *v, Rectangle row, Vector2 at, int count, Texture2D icon) {
    float h = GUILD_REWARD_SMART_HEIGHT;
    char amount[GUILD_TIME_TEXT_CAP];
    GuildAbbreviate(amount, sizeof(amount), count);
    Rectangle label = GuildSmart(row, h, at.x, at.y, 129.0f, 114.0f);
    float size = label.height * 0.42f;
    float width = MeasureTextEx(v->font, amount, size, 1.0f).x;
    if (width > label.width) size *= label.width / width;
    UiDrawTextRight(v->font, amount, label, size, GUILD_TEXT);
    UiDrawTexture(icon, GuildSmart(row, h, at.x + 143.0f, at.y, 114.0f, 114.0f), WHITE);
}

static void GuildDrawRewardRow(const GuildView *v, const PvpEventRankReward *tier, Rectangle row, int index) {
    float h = GUILD_REWARD_SMART_HEIGHT;
    if (index < GUILD_REWARD_MEDALS)
        UiDrawTexture(GuiTex(GUILD_REWARD_MEDAL_IDS[index]), GuildSmart(row, h, 0.0f, 38.0f, 177.0f, 177.0f), WHITE);
    char label[GUILD_TIME_TEXT_CAP];
    GuildRankRewardLabel(label, sizeof(label), tier);
    Rectangle rank = GuildSmart(row, h, 198.0f, 0.0f, 295.0f, h);
    GuildFitText(v->font, label, rank, rank.height * 0.2f, GUILD_TEXT, true);
    const PvpReward *reward = &tier->reward;
    int items = (reward->gems > 0) + (reward->gold > 0) + reward->prize_count;
    bool centred = items <= GUILD_REWARD_CENTERED_SLOTS;
    const Vector2 *slots = centred ? GUILD_REWARD_CENTERED_SLOT_AT : GUILD_REWARD_SLOT_AT;
    int capacity = centred ? GUILD_REWARD_CENTERED_SLOTS : GUILD_REWARD_SLOTS;
    int slot = 0;
    if (reward->gems > 0 && slot < capacity)
        GuildDrawRewardSlot(v, row, slots[slot++], reward->gems, GuiTex(GUI_MISSION_GEM));
    if (reward->gold > 0 && slot < capacity)
        GuildDrawRewardSlot(v, row, slots[slot++], reward->gold, GuiTex(GUI_MISSION_GOLD));
    for (int i = 0; i < reward->prize_count && slot < capacity; i++)
        GuildDrawRewardSlot(v, row, slots[slot++], reward->prizes[i].count, PvpPrizeTexture(&reward->prizes[i]));
    UiDrawTexture(GuiTex(GUI_GUILD_REWARD_RULE), GuildSmart(row, h, 0.0f, 239.0f, 1280.0f, 13.0f), WHITE);
}

static void
GuildViewDrawWarRewards(GuildView *v, const GuildWarEvents *events, const GuildOnslaught *o, bool interactive) {
    GuildDrawTitleBackground(GuildRect(781.0f, 248.0f, 278.0f, 50.0f));
    GuildFitText(
        v->font,
        LocGet("GUILD_WAR_REWARDS_TITLE"),
        GuildRect(791.0f, 258.0f, 258.0f, 33.0f),
        GUILD_HEADING_SIZE,
        GUILD_TEXT_GOLD,
        true
    );
    const PvpEvent *event = events->shown >= 0 ? PvpEventForCycle(events->shown) : NULL;
    bool previous = events->state == GUILD_WAR_PREVIOUS_EVENT;
    if (previous)
        GuildFitText(
            v->body,
            LocGet("GUILD_WAR_PREVIOUS_RESULTS"),
            GuildRect(531.0f, 245.0f, 239.0f, 47.0f),
            GUILD_BODY_SIZE,
            GUILD_TEXT_ORANGE,
            true
        );
    bool award = previous && o && GuildOnslaughtAwardAvailable(o);
    bool minimum = event && event->guild_minimum_banners > 0 && events->state != GUILD_WAR_NO_EVENT && !award;
    if (award && ButtonDrawEx(&v->claim, GuildRect(1055.0f, 262.0f, 139.0f, 47.0f), interactive))
        v->pending = GUILD_VIEW_CLAIM_ONSLAUGHT;
    if (minimum) {
        char text[GUILD_MESSAGE_CAP];
        const char *args[] = {TextFormat("%d", event->guild_minimum_banners)};
        LocFormatArgs(text, sizeof(text), "GUILD_WAR_MINIMUM_BANNERS", args, 1);
        GuildDropParenthetical(text);
        GuildDrawWrapped(
            v->body,
            text,
            GuildRect(GUILD_WAR_ENTER_VISIBLE_LEFT, 250.0f, 1198.0f - GUILD_WAR_ENTER_VISIBLE_LEFT, 64.0f),
            GUILD_SMALL_SIZE - 5.0f,
            GUILD_TEXT
        );
    }
    if (!event) return;
    Rectangle list = GuildRect(648.0f, 312.0f, 544.0f, 306.0f);
    float row_height = list.width * GUILD_REWARD_SMART_HEIGHT / GUILD_SMART_WIDTH;
    GoalsScrollUpdate(&v->reward_scroll, list, row_height * event->guild_rank_reward_count);
    GoalsBeginClip(list);
    for (int i = 0; i < event->guild_rank_reward_count; i++) {
        Rectangle row = {list.x, list.y + i * row_height - v->reward_scroll.offset, list.width, row_height};
        if (row.y + row.height < list.y || row.y > list.y + list.height) continue;
        GuildDrawRewardRow(v, &event->guild_rank_rewards[i], row, i);
    }
    EndScissorMode();
}

static void GuildViewDrawWar(GuildView *v, Guild *g, bool interactive) {
    long long now = ServerNow();
    GuildWarEvents events = GuildWarCheckEvents(now);
    GuildOnslaught *o = GuildWarHasStandings(&events) ? GuildOnslaughtFor(g, events.shown) : NULL;
    if (o && o->cycle != events.shown) GuildRequestOnslaught(g, events.shown);
    GuildViewDrawWarHeader(v, g, &events, now);
    if (events.state == GUILD_WAR_CURRENT_EVENT &&
        ButtonDrawEx(&v->enter, GuildRect(988.0f, 187.0f, 201.0f, 52.0f), interactive))
        v->pending = GUILD_VIEW_ENTER_ONSLAUGHT;
    GuildViewDrawWarLeaderboard(v, g, o, interactive);
    GuildViewDrawWarRewards(v, &events, o, interactive);
    GuildViewHandleFooter(v, g, GuildDrawFooter(v, g, GUILD_TAB_WAR, true, false, interactive));
}

static void GuildDrawRadio(const GuildView *v, const GuildAbuseOption *option, Vector2 at, bool selected) {
    Rectangle radio = GuildRect(at.x, at.y, 45.0f, 45.0f);
    UiDrawTexture(GuiTex(selected ? GUI_GUILD_RADIO_ON : GUI_GUILD_RADIO), radio, WHITE);
    GuildFitText(
        v->body,
        LocGet(option->label_key),
        GuildRect(at.x + 55.0f, at.y + 9.0f, 420.0f, 28.0f),
        GUILD_BODY_SIZE,
        selected ? GUILD_TEXT_GOLD : GUILD_TEXT,
        false
    );
}

static void GuildViewSubmitReport(GuildView *v, Guild *g) {
    if (v->report_choice == GUILD_ABUSE_NONE) {
        GuildViewShowKeys(
            v, "REPORT_ABUSE_ERROR_NOTHING_SELECTED_TITLE", "REPORT_ABUSE_ERROR_NOTHING_SELECTED_MESSAGE"
        );
        return;
    }
    bool guild = v->report_state == GUILD_REPORT_GUILD;
    GuildRequestReport(
        g, guild ? GUILD_REPORT_KIND_GUILD : GUILD_REPORT_KIND_APPLICATION, v->report_choice, v->report_target
    );
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    v->report_open = false;
    GuildViewWait(v, GUILD_REQUEST_REPORT);
}

static void GuildViewDrawReport(GuildView *v, Guild *g, bool open_before_frame) {
    bool interactive = open_before_frame && GuildViewOverlayInteractive(v);
    DrawRectangleRec(UiRoot(), GUILD_SCRIM);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), GuildRect(0.0f, 88.0f, UI_DESIGN_W, 616.0f), WHITE);
    bool guild = v->report_state == GUILD_REPORT_GUILD;
    if (guild) {
        GuildFitText(
            v->font,
            LocGet("REPORT_ABUSE_GUILD_ABUSE_TITLE"),
            GuildRect(194.0f, 146.0f, 892.0f, 54.0f),
            GUILD_TITLE_SIZE,
            GUILD_TEXT,
            true
        );
        GuildDrawWrapped(
            v->body,
            LocGet("REPORT_ABUSE_GUILD_ABUSE_MESSAGE"),
            GuildRect(194.0f, 233.0f, 892.0f, 88.0f),
            GUILD_HEADING_SIZE,
            GUILD_TEXT
        );
    } else {
        GuildFitText(
            v->font, v->report_name, GuildRect(194.0f, 146.0f, 892.0f, 54.0f), GUILD_TITLE_SIZE, GUILD_TEXT, true
        );
        GuildDrawWrapped(
            v->body,
            v->report_text[0] ? v->report_text : "-",
            GuildRect(194.0f, 213.0f, 892.0f, 108.0f),
            GUILD_BODY_SIZE + 2.0f,
            GUILD_TEXT
        );
        GuildDrawRule(GuildRect(139.0f, 341.0f, 1003.0f, 3.0f));
        GuildDrawWrapped(
            v->body,
            LocGet("REPORT_ABUSE_APPLICATION_MESSAGE"),
            GuildRect(194.0f, 356.0f, 892.0f, 60.0f),
            GUILD_HEADING_SIZE,
            GUILD_TEXT
        );
    }
    for (size_t i = 0; i < sizeof(GUILD_ABUSE_OPTIONS) / sizeof(GUILD_ABUSE_OPTIONS[0]); i++) {
        const GuildAbuseOption *option = &GUILD_ABUSE_OPTIONS[i];
        Vector2 at = guild ? option->guild_at : option->application_at;
        bool selected = v->report_choice == option->type;
        GuildDrawRadio(v, option, at, selected);
        bool hit = interactive && CheckCollisionPointRec(UiMouse(), GuildRect(at.x, at.y, 223.0f, 45.0f));
        if (!hit || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) continue;
        GuiSoundPlay(SND_UI_CLICK);
        v->report_choice = selected ? GUILD_ABUSE_NONE : option->type;
    }
    if (GuildTextButton(
            v,
            &v->submit,
            LocGet("REPORT_ABUSE_SUBMIT_BUTTON"),
            GuildRect(219.0f, 585.0f, 273.0f, 71.0f),
            true,
            interactive
        ))
        GuildViewSubmitReport(v, g);
    bool closed = GuildTextButton(
        v, &v->cancel, LocGet("REPORT_ABUSE_CANCEL_BUTTON"), GuildRect(790.0f, 585.0f, 273.0f, 71.0f), true, interactive
    );
    closed |= ButtonDrawEx(&v->close, GuildRect(1188.0f, 72.0f, 74.0f, 74.0f), interactive);
    if (closed) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        v->report_open = false;
    }
}

static Rectangle GuildDialogFrame(void) {
    return MessagePopupDrawFrame();
}

static void GuildViewConfirmed(GuildView *v, Guild *g, Player *p) {
    switch (v->confirm) {
        case GUILD_CONFIRM_LEAVE:
            GuildRequestLeave(g);
            GuildViewWait(v, GUILD_REQUEST_LEAVE);
            break;
        case GUILD_CONFIRM_CANCEL_APPLICATION:
            GuildRequestCancel(g, v->target_id);
            GuildViewWait(v, GUILD_REQUEST_CANCEL);
            break;
        case GUILD_CONFIRM_CREATE:
            if (GuildRequestCreate(g, p, &v->draft))
                GuildViewWait(v, GUILD_REQUEST_CREATE);
            else
                GuildViewShowMessage(v, "ERROR_GENERAL_TITLE", GUILD_BUSY_MESSAGE);
            break;
        case GUILD_CONFIRM_CREATE_OVER_APPLICATION:
            GuildViewShowCreate(v, g, false);
            break;
        case GUILD_CONFIRM_JOIN_OVER_APPLICATION:
            if (g->observed.summary.status == GUILD_STATUS_APPLICATION_ONLY) {
                TextFieldClear(&v->application);
                v->dialog = GUILD_DIALOG_APPLY;
                return;
            }
            GuildRequestJoin(g, v->target_id, "");
            GuildViewWait(v, GUILD_REQUEST_JOIN);
            break;
        case GUILD_CONFIRM_PROMOTE:
            GuildRequestRank(g, v->target_id, (GuildRank)(v->target_rank + 1));
            GuildViewWait(v, GUILD_REQUEST_RANK);
            break;
        case GUILD_CONFIRM_DEMOTE:
            GuildRequestRank(g, v->target_id, (GuildRank)(v->target_rank - 1));
            GuildViewWait(v, GUILD_REQUEST_RANK);
            break;
        case GUILD_CONFIRM_KICK:
            GuildRequestKick(g, v->target_id);
            GuildViewWait(v, GUILD_REQUEST_KICK);
            break;
    }
}

static void GuildViewDrawMessageDialog(GuildView *v, Guild *g, Player *p) {
    Rectangle popup = GuildDialogFrame();
    Rectangle content = MessagePopupContent(popup);
    bool has_title = v->dialog_title[0] != '\0';
    if (has_title)
        UiDrawTextCentered(v->font, v->dialog_title, UiRect(content, 0.5f, 0.0f, 1.0f, 0.2f), 30.0f, GUILD_TEXT_GOLD);
    Rectangle text = has_title ? UiRect(content, 0.5f, 1.0f, 1.0f, 0.78f) : content;
    MessagePopupDrawText(v->body, v->dialog_message, text, 22.0f, GUILD_TEXT);
    if (v->dialog == GUILD_DIALOG_MESSAGE) {
        if (ButtonDraw(&v->ok, MessagePopupOkButton(popup))) v->dialog = GUILD_DIALOG_NONE;
        return;
    }
    Rectangle row = MessagePopupChoiceRow(popup);
    if (ButtonDraw(&v->no, UiRect(row, 0.0f, 0.0f, 0.48f, 1.0f))) v->dialog = GUILD_DIALOG_NONE;
    if (ButtonDraw(&v->yes, UiRect(row, 1.0f, 0.0f, 0.48f, 1.0f))) {
        v->dialog = GUILD_DIALOG_NONE;
        GuildViewConfirmed(v, g, p);
    }
}

static void GuildViewDrawApplyDialog(GuildView *v, Guild *g) {
    Rectangle popup = GuildDialogFrame();
    Rectangle content = MessagePopupContent(popup);
    UiDrawTextCentered(
        v->font, LocGet("GUILD_APPLICATION_TITLE"), UiRect(content, 0.5f, 0.0f, 1.0f, 0.2f), 30.0f, GUILD_TEXT_GOLD
    );
    TextFieldEvent event = TextFieldDraw(
        &v->application,
        UiRect(content, 0.5f, 0.5f, 1.0f, 0.24f),
        v->body,
        GUILD_BODY_SIZE,
        LocGet("GUILD_APPLICATION_TEXTBOX"),
        true
    );
    if (event == TEXT_FIELD_BLURRED)
        GuildViewFieldStripped(
            v, &v->application, "ERROR_INVALID_APPLICATION_TITLE", "ERROR_INVALID_APPLICATION_MESSAGE"
        );
    Rectangle row = MessagePopupChoiceRow(popup);
    if (GuildTextButton(
            v, &v->grey, LocGet("GUILD_CREATE_CANCEL_BUTTON"), UiRect(row, 0.0f, 0.0f, 0.48f, 1.0f), true, true
        ))
        v->dialog = GUILD_DIALOG_NONE;
    if (GuildTextButton(
            v, &v->gold, LocGet("GUILD_APPLICATION_SUBMIT_BUTTON"), UiRect(row, 1.0f, 0.0f, 0.48f, 1.0f), true, true
        )) {
        v->dialog = GUILD_DIALOG_NONE;
        TextFieldBlur(&v->application);
        GuildRequestJoin(g, v->target_id, v->application.text);
        GuildViewWait(v, GUILD_REQUEST_JOIN);
    }
}

static void GuildViewDrawReviewDialog(GuildView *v, Guild *g) {
    Rectangle popup = GuildDialogFrame();
    Rectangle content = MessagePopupContent(popup);
    UiDrawTextCentered(v->font, v->target_name, UiRect(content, 0.5f, 0.0f, 1.0f, 0.2f), 30.0f, GUILD_TEXT_GOLD);
    MessagePopupDrawText(
        v->body, v->review_text[0] ? v->review_text : "-", UiRect(content, 0.5f, 1.0f, 1.0f, 0.78f), 22.0f, GUILD_TEXT
    );
    Rectangle row = MessagePopupChoiceRow(popup);
    if (GuildTextButton(
            v, &v->gold, LocGet("GUILD_APPLICATION_ACCEPT_BUTTON"), UiRect(row, 0.0f, 0.0f, 0.32f, 1.0f), true, true
        )) {
        v->dialog = GUILD_DIALOG_NONE;
        GuildViewAccept(v, g, v->target_id);
    }
    if (GuildTextButton(
            v, &v->grey, LocGet("GUILD_APPLICATION_REJECT_BUTTON"), UiRect(row, 0.5f, 0.0f, 0.32f, 1.0f), true, true
        )) {
        v->dialog = GUILD_DIALOG_NONE;
        GuildViewDeny(v, g, v->target_id);
    }
    if (GuildTextButton(
            v,
            &v->grey,
            LocGet("GUILD_APPLICATION_REPORTABUSE_BUTTON"),
            UiRect(row, 1.0f, 0.0f, 0.32f, 1.0f),
            true,
            true
        )) {
        v->dialog = GUILD_DIALOG_NONE;
        GuildViewOpenReport(v, GUILD_REPORT_APPLICATION, v->target_id, v->target_name, v->review_text);
    }
}

static void GuildViewDrawLanguageDialog(GuildView *v) {
    Rectangle popup = GuildDialogFrame();
    Rectangle content = MessagePopupContent(popup);
    UiDrawTextCentered(
        v->font, LocGet("LANGUAGE_SELECT_TITLE"), UiRect(content, 0.5f, 0.0f, 1.0f, 0.16f), 30.0f, GUILD_TEXT_GOLD
    );
    Rectangle grid = UiRect(content, 0.5f, 1.0f, 1.0f, 0.82f);
    int columns = 3, rows = (GUILD_LANGUAGE_COUNT + columns - 1) / columns;
    float cell_w = grid.width / columns, cell_h = grid.height / rows;
    for (int i = 0; i < GUILD_LANGUAGE_COUNT; i++) {
        Rectangle cell = {grid.x + (i % columns) * cell_w, grid.y + (i / columns) * cell_h, cell_w, cell_h};
        Rectangle button = UiRect(cell, 0.5f, 0.5f, 0.94f, 0.8f);
        bool selected = !strcmp(GUILD_LANGUAGES[i].code, v->draft.language);
        if (selected) {
            UiDrawTexture(GuiTex(GUI_BTN_GOLD), button, WHITE);
            GuildFitText(
                v->font,
                LocGet(GUILD_LANGUAGES[i].key),
                UiRect(button, 0.5f, 0.5f, 0.9f, 1.0f),
                button.height * 0.42f,
                GUILD_TEXT,
                true
            );
        } else if (GuildTextButton(v, &v->grey, LocGet(GUILD_LANGUAGES[i].key), button, true, true)) {
            StrCopy(v->draft.language, sizeof(v->draft.language), GUILD_LANGUAGES[i].code);
            v->dialog = GUILD_DIALOG_NONE;
        }
    }
    if (ButtonDraw(&v->ok, MessagePopupOkButton(popup))) v->dialog = GUILD_DIALOG_NONE;
}

static void GuildViewDrawDialog(GuildView *v, Guild *g, Player *p) {
    switch (v->dialog) {
        case GUILD_DIALOG_MESSAGE:
        case GUILD_DIALOG_CONFIRM:
            GuildViewDrawMessageDialog(v, g, p);
            break;
        case GUILD_DIALOG_APPLY:
            GuildViewDrawApplyDialog(v, g);
            break;
        case GUILD_DIALOG_REVIEW:
            GuildViewDrawReviewDialog(v, g);
            break;
        case GUILD_DIALOG_LANGUAGE:
            GuildViewDrawLanguageDialog(v);
            break;
        default:
            break;
    }
}

static void GuildViewGeneralError(GuildView *v, Guild *g) {
    GuildViewShowKeys(v, "GUILD_ERROR_RESTART_CLIENT_TITLE", "GUILD_ERROR_RESTART_CLIENT_MESSAGE");
    GuildRefresh(g);
}

static void GuildViewJoinRefused(GuildView *v, Guild *g, const char *code) {
    if (!strcmp(code, "GUILD_APPLICATIONS_FULL"))
        GuildViewShowKeys(v, "ERROR_MAX_APPLICATIONS_REACHED_TITLE", "ERROR_MAX_APPLICATIONS_REACHED_MESSAGE");
    else if (!strcmp(code, GUILD_CODE_NOT_FOUND))
        GuildViewShowKeys(v, "ERROR_GUILD_NOT_FOUND_TITLE", "ERROR_GUILD_NOT_FOUND_MESSAGE");
    else if (!strcmp(code, "GUILD_FULL"))
        GuildViewShowKeys(v, "ERROR_GUILD_FULL_TITLE", "ERROR_GUILD_FULL_MESSAGE");
    else if (!strcmp(code, "ALREADY_IN_GUILD"))
        GuildViewShowKeys(
            v, "ERROR_CANNOT_JOIN_GUILD_WHILE_GUILDED_TITLE", "ERROR_CANNOT_JOIN_GUILD_WHILE_GUILDED_MESSAGE"
        );
    else
        GuildViewGeneralError(v, g);
}

static void GuildViewCreateRefused(GuildView *v, Guild *g, const char *code) {
    if (!strcmp(code, GUILD_CODE_CANT_AFFORD))
        GuildViewShowKeys(v, "ERROR_NOT_ENOUGH_GOLD_TITLE", "ERROR_NOT_ENOUGH_GOLD_MESSAGE");
    else if (!strcmp(code, "GUILD_NAME_NOT_UNIQUE"))
        GuildViewShowKeys(v, "ERROR_INVALID_GUILD_NAME_TITLE", "ERROR_DUPLICATE_GUILD_NAME_MESSAGE");
    else if (!strcmp(code, "INVALID_ARGUMENT"))
        GuildViewShowKeys(v, "ERROR_INVALID_GUILD_NAME_TITLE", "ERROR_INVALID_GUILD_NAME_MESSAGE");
    else if (!strcmp(code, "ALREADY_IN_GUILD"))
        GuildViewShowKeys(
            v, "ERROR_CANNOT_CREATE_GUILD_WHILE_GUILDED_TITLE", "ERROR_CANNOT_CREATE_GUILD_WHILE_GUILDED_MESSAGE"
        );
    else
        GuildViewGeneralError(v, g);
}

static void GuildViewJoined(GuildView *v, Guild *g) {
    if (GuildIsMember(g)) {
        char message[GUILD_MESSAGE_CAP];
        const char *args[] = {g->mine.summary.name};
        LocFormatArgs(message, sizeof(message), "GUILD_PENDING_EVENT_APPLICATION_ACCEPTED", args, 1);
        GuildViewShowMessage(v, "GUILD_PENDING_EVENT_CONGRATULATIONS", message);
        GuildViewShowHome(v, GUILD_HOME_MEMBER);
        return;
    }
    GuildViewShowKeys(v, "GUILD_APPLICATION_SUCCESS_TITLE", "GUILD_APPLICATION_SUCCESS_MESSAGE");
    GuildRequestObserve(g, v->target_id);
}

static void GuildViewHandleResult(GuildView *v, Guild *g, const GuildResult *result) {
    if (result->kind == v->waiting_for) v->waiting_for = GUILD_REQUEST_NONE;
    if (result->kind == GUILD_REQUEST_REPORT) {
        if (result->outcome == GUILD_OUTCOME_OK)
            GuildViewShowKeys(v, "REPORT_ABUSE_SUCCESS_TITLE", "REPORT_ABUSE_SUCCESS_MESSAGE");
        else
            GuildViewShowKeys(v, "REPORT_ABUSE_FAILED_TITLE", "REPORT_ABUSE_FAILED_MESSAGE");
        return;
    }
    if (result->outcome == GUILD_OUTCOME_OFFLINE) {
        if (!strcmp(result->code, GUILD_CODE_PROFILE_CHANGED)) {
            GuildViewShowMessage(v, "ERROR_GENERAL_TITLE", GUILD_PROFILE_CHANGED_MESSAGE);
            return;
        }
        if (result->kind != GUILD_REQUEST_RECOMMENDED && result->kind != GUILD_REQUEST_SEARCH) GuildViewShowOffline(v);
        return;
    }
    bool ok = result->outcome == GUILD_OUTCOME_OK;
    switch (result->kind) {
        case GUILD_REQUEST_OBSERVE:
            if (ok)
                GuildViewShowHome(v, GUILD_HOME_OBSERVE);
            else
                GuildViewShowKeys(v, "ERROR_GUILD_NOT_FOUND_TITLE", "ERROR_GUILD_NOT_FOUND_MESSAGE");
            break;
        case GUILD_REQUEST_JOIN:
            if (ok)
                GuildViewJoined(v, g);
            else
                GuildViewJoinRefused(v, g, result->code);
            break;
        case GUILD_REQUEST_CREATE:
            if (ok)
                GuildViewShowHome(v, GUILD_HOME_MEMBER);
            else
                GuildViewCreateRefused(v, g, result->code);
            break;
        case GUILD_REQUEST_CANCEL:
        case GUILD_REQUEST_LEAVE:
            if (ok)
                GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
            else
                GuildViewGeneralError(v, g);
            break;
        case GUILD_REQUEST_SETTINGS:
            if (ok)
                GuildViewShowHome(v, GUILD_HOME_MEMBER);
            else if (!strcmp(result->code, "GUILD_CAN_NOT_CHANGE_STATUS_WITH_PENDING_APPLICATIONS"))
                GuildViewShowKeys(
                    v, "ERROR_MUST_RESOLVE_APPLICATIONS_TITLE", "ERROR_MUST_RESOLVE_APPLICATIONS_MESSAGE_STRING"
                );
            else
                GuildViewGeneralError(v, g);
            break;
        case GUILD_REQUEST_SEND:
            if (!ok) GuildViewShowKeys(v, "ERROR_GENERAL_TITLE", "ERROR_GENERAL_MESSAGE");
            break;
        case GUILD_REQUEST_DUEL:
            if (ok)
                v->pending = GUILD_VIEW_DUEL;
            else
                GuildViewShowKeys(v, "ERROR_GENERAL_TITLE", "ERROR_GENERAL_MESSAGE");
            break;
        case GUILD_REQUEST_ACCEPT:
        case GUILD_REQUEST_DENY:
        case GUILD_REQUEST_RANK:
        case GUILD_REQUEST_KICK:
            v->expanded_member = -1;
            if (!ok) GuildViewGeneralError(v, g);
            break;
        default:
            break;
    }
}

static void GuildViewDrawWaiting(GuildView *v) {
    v->waiting_time += GetFrameTime();
    if (v->waiting_time < GUILD_WAIT_SECONDS) return;
    DrawRectangleRec(UiRoot(), (Color){0, 0, 0, 110});
    Rectangle ring = UiRect(UiRoot(), 0.5f, 0.5f, 96.0f, 96.0f);
    Texture2D texture = GuiTex(GUI_LOADING_RING);
    DrawTexturePro(
        texture,
        (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height},
        (Rectangle){ring.x + ring.width * 0.5f, ring.y + ring.height * 0.5f, ring.width, ring.height},
        (Vector2){ring.width * 0.5f, ring.height * 0.5f},
        v->waiting_time * 360.0f,
        WHITE
    );
}

static bool GuildViewPageValid(GuildView *v, Guild *g) {
    bool member_page = v->page == GUILD_PAGE_CHAT || v->page == GUILD_PAGE_WAR ||
                       (v->page == GUILD_PAGE_HOME && v->home_mode != GUILD_HOME_OBSERVE) ||
                       (v->page == GUILD_PAGE_CREATE && v->settings_mode);
    if (!member_page || GuildIsMember(g) || v->waiting_for != GUILD_REQUEST_NONE) return true;
    GuildSetChatVisible(g, false);
    GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
    return false;
}

GuildViewAction GuildViewDraw(GuildView *v, Guild *g, Player *p) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) v->press_point = UiPointer();
    GuildResult result;
    while (GuildTakeResult(g, &result)) GuildViewHandleResult(v, g, &result);
    GuildViewPageValid(v, g);
    bool interactive = GuildViewInteractive(v);
    bool report_open_before_frame = v->report_open;
    GuildViewAction action = GuildViewDrawFrame(v, interactive) ? GUILD_VIEW_CLOSE : GUILD_VIEW_NONE;
    switch (v->page) {
        case GUILD_PAGE_FIND:
            GuildViewDrawFind(v, g, interactive);
            break;
        case GUILD_PAGE_CREATE:
            GuildViewDrawCreate(v, g, p, interactive);
            break;
        case GUILD_PAGE_HOME:
            GuildViewDrawHome(v, g, p, interactive);
            break;
        case GUILD_PAGE_CHAT:
            GuildViewDrawChat(v, g, interactive);
            break;
        case GUILD_PAGE_WAR:
            GuildViewDrawWar(v, g, interactive);
            break;
    }
    if (v->page != GUILD_PAGE_CHAT) GuildSetChatVisible(g, false);
    if (v->report_open) GuildViewDrawReport(v, g, report_open_before_frame);
    if (v->dialog != GUILD_DIALOG_NONE) GuildViewDrawDialog(v, g, p);
    if (v->waiting_for != GUILD_REQUEST_NONE) GuildViewDrawWaiting(v);
    if (action == GUILD_VIEW_NONE && v->pending != GUILD_VIEW_NONE) action = v->pending;
    v->pending = GUILD_VIEW_NONE;
    if (action != GUILD_VIEW_NONE) GuildViewLeave(v, g);
    return action;
}

static bool GuildViewAnyFocused(const GuildView *v) {
    return v->search.focused || v->name.focused || v->description.focused || v->motd.focused || v->min_level.focused ||
           v->chat_input.focused || v->application.focused;
}

bool GuildViewHandleBack(GuildView *v, Guild *g) {
    if (v->waiting_for != GUILD_REQUEST_NONE) return true;
    if (v->dialog != GUILD_DIALOG_NONE) {
        v->dialog = GUILD_DIALOG_NONE;
        GuildViewBlurFields(v);
        return true;
    }
    if (v->report_open) {
        v->report_open = false;
        return true;
    }
    if (GuildViewAnyFocused(v)) {
        GuildViewBlurFields(v);
        return true;
    }
    if (v->page == GUILD_PAGE_FIND && v->find_state != GUILD_FIND_RECOMMENDED) {
        GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
        return true;
    }
    if (v->page == GUILD_PAGE_CREATE) {
        if (v->settings_mode)
            GuildViewShowHome(v, GUILD_HOME_MEMBER);
        else
            GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
        return true;
    }
    if (v->page == GUILD_PAGE_HOME && v->home_mode == GUILD_HOME_OBSERVE) {
        GuildViewShowFind(v, g, GUILD_FIND_RECOMMENDED);
        return true;
    }
    if (v->page == GUILD_PAGE_HOME && v->home_mode == GUILD_HOME_APPLICATIONS) {
        GuildViewShowHome(v, GUILD_HOME_MEMBER);
        return true;
    }
    return false;
}

bool GuildViewCapturesKeys(const GuildView *v) {
    return GuildViewAnyFocused(v);
}

typedef struct {
    bool open;
    char title[GUILD_LINE_TITLE_CAP];
    char message[GUILD_MESSAGE_CAP];
    Button ok;
} GuildNoticePopup;

void GuildNoticePopupInitialize(GuildNoticePopup *popup) {
    memset(popup, 0, sizeof(*popup));
    popup->ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.34f
    );
}

static void GuildNoticeCompose(GuildNoticePopup *popup, const GuildNotice *notice) {
    const char *rank = LocGet(GuildRankKey(notice->rank));
    const char *args[] = {notice->guild};
    const char *rank_args[] = {rank};
    const char *day_args[] = {TextFormat("%d", GUILD_INACTIVE_LEADER_SECONDS / GUILD_SECONDS_PER_DAY)};
    popup->title[0] = '\0';
    switch (notice->kind) {
        case GUILD_NOTICE_ACCEPTED:
            StrCopy(popup->title, sizeof(popup->title), LocGet("GUILD_PENDING_EVENT_CONGRATULATIONS"));
            LocFormatArgs(popup->message, sizeof(popup->message), "GUILD_PENDING_EVENT_APPLICATION_ACCEPTED", args, 1);
            break;
        case GUILD_NOTICE_DENIED:
            LocFormatArgs(popup->message, sizeof(popup->message), "GUILD_PENDING_EVENT_APPLICATION_DENIED", args, 1);
            break;
        case GUILD_NOTICE_KICKED:
            StrCopy(popup->title, sizeof(popup->title), LocGet("GUILD_PENDING_EVENT_RANK_CHANGE_TITLE"));
            LocFormatArgs(popup->message, sizeof(popup->message), "GUILD_PENDING_EVENT_KICKED", args, 1);
            break;
        case GUILD_NOTICE_PROMOTED:
            StrCopy(popup->title, sizeof(popup->title), LocGet("GUILD_PENDING_EVENT_RANK_CHANGE_TITLE"));
            LocFormatArgs(popup->message, sizeof(popup->message), "GUILD_PENDING_EVENT_PROMOTED", rank_args, 1);
            break;
        case GUILD_NOTICE_DEMOTED:
            StrCopy(popup->title, sizeof(popup->title), LocGet("GUILD_PENDING_EVENT_RANK_CHANGE_TITLE"));
            LocFormatArgs(popup->message, sizeof(popup->message), "GUILD_PENDING_EVENT_DEMOTED", rank_args, 1);
            break;
        case GUILD_NOTICE_NEW_LEADER:
            StrCopy(popup->title, sizeof(popup->title), LocGet("GUILD_PENDING_EVENT_RANK_CHANGE_TITLE"));
            LocFormatArgs(popup->message, sizeof(popup->message), "GUILD_PENDING_EVENT_NEW_LEADER", day_args, 1);
            break;
    }
}

void GuildNoticePopupShow(GuildNoticePopup *popup, const char *title, const char *message) {
    StrCopy(popup->title, sizeof(popup->title), title);
    StrCopy(popup->message, sizeof(popup->message), message);
    popup->open = true;
}

bool GuildNoticePopupIsOpen(const GuildNoticePopup *popup) {
    return popup->open;
}

bool GuildNoticePopupClose(GuildNoticePopup *popup) {
    if (!popup->open) return false;
    popup->open = false;
    return true;
}

bool GuildNoticePopupDraw(GuildNoticePopup *popup, Guild *g) {
    if (!popup->open) {
        GuildNotice notice;
        if (!GuildTakeNotice(g, &notice)) return false;
        GuildNoticeCompose(popup, &notice);
        popup->open = true;
    }
    Rectangle frame = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(frame);
    Font body = GuiFont(FNT_MENU);
    bool has_title = popup->title[0] != '\0';
    if (has_title)
        UiDrawTextCentered(
            GuiFont(FNT_COOL), popup->title, UiRect(content, 0.5f, 0.0f, 1.0f, 0.2f), 30.0f, GUILD_TEXT_GOLD
        );
    MessagePopupDrawText(
        body, popup->message, has_title ? UiRect(content, 0.5f, 1.0f, 1.0f, 0.78f) : content, 22.0f, GUILD_TEXT
    );
    if (ButtonDraw(&popup->ok, MessagePopupOkButton(frame))) popup->open = false;
    return true;
}
