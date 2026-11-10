#define GUILD_LEVEL_UNLOCK 5
#define GUILD_CREATION_GOLD_COST 10000
#define GUILD_INACTIVE_LEADER_SECONDS 432000
#define GUILD_SECONDS_PER_DAY 86400
#define GUILD_MAX_MEMBERS 30
#define GUILD_MAX_APPLICATIONS 10
#define GUILD_NAME_MIN_RUNES 4
#define GUILD_NAME_MAX_RUNES 18
#define GUILD_DESCRIPTION_MAX_RUNES 150
#define GUILD_MOTD_MAX_RUNES 150
#define GUILD_APPLICATION_MAX_RUNES 150
#define GUILD_CHAT_MAX_RUNES 140
#define GUILD_SEARCH_MAX_RUNES 18
#define GUILD_MAX_REQUIRED_LEVEL 423
#define GUILD_REQUIRED_LEVEL_DIGITS 3
#define GUILD_EMBLEM_COUNT 23
#define GUILD_LIST_CAPACITY 20
#define GUILD_CHAT_CAPACITY 100
#define GUILD_NOTICE_CAPACITY 16
#define GUILD_REQUEST_CAPACITY 16
#define GUILD_ID_CAP 48
#define GUILD_NAME_CAP 80
#define GUILD_TEXT_CAP 640
#define GUILD_LINE_TITLE_CAP 256
#define GUILD_LINE_TEXT_CAP 768
#define GUILD_LANGUAGE_CAP 12
#define GUILD_CODE_CAP 64
#define GUILD_ROUTE_CAP 256
#define GUILD_EVENT_PARAM_CAP 4
#define GUILD_STATE_REFRESH_SECONDS 300.0
#define GUILD_RETRY_SECONDS 10.0
#define GUILD_HTTP_OK 200
#define GUILD_HTTP_REFUSED 422
#define GUILD_HTTP_NOT_FOUND 404
#define GUILD_HTTP_TOO_MANY_REQUESTS 429
#define GUILD_WORLD_SPEAKER_LEVEL GUILD_LEVEL_UNLOCK
#define GUILD_CREATE_ROUTE "guilds"
#define GUILD_MESSAGES_ROUTE "guild/messages"
#define GUILD_WORLD_MESSAGES_ROUTE "chat/world/messages"
#define GUILD_ONSLAUGHT_CLAIM_ROUTE "guild/onslaught/claim"
#define GUILD_ONSLAUGHT_LIMIT 100
#define GUILD_ONSLAUGHT_MEMBER_CAP (GUILD_MAX_MEMBERS * 2)
#define GUILD_DUEL_RUNE_CAP 3
#define GUILD_RUNE_ID_CAP 48
#define GUILD_REPORT_KIND_GUILD "guild"
#define GUILD_REPORT_KIND_APPLICATION "application"
#define GUILD_CODE_CANT_AFFORD "cant_afford"
#define GUILD_CODE_NOT_FOUND "GUILD_NOT_FOUND"
#define GUILD_CODE_NOT_IN_GUILD "NOT_IN_GUILD"
#define GUILD_CODE_PROFILE_CHANGED "profile_changed"
#define GUILD_CODE_CHAT_LEVEL_TOO_LOW "CHAT_LEVEL_TOO_LOW"
#define GUILD_CODE_SENDING_TOO_FAST "sending_too_fast"

static const double GUILD_CHAT_ON_SCREEN_POLLS[] = {10.0, 30.0};
static const double GUILD_CHAT_OFF_SCREEN_POLLS[] = {60.0};

#define GUILD_CHAT_ON_SCREEN_POLL_COUNT                                                                                \
    ((int)(sizeof(GUILD_CHAT_ON_SCREEN_POLLS) / sizeof(GUILD_CHAT_ON_SCREEN_POLLS[0])))
#define GUILD_CHAT_OFF_SCREEN_POLL_COUNT                                                                               \
    ((int)(sizeof(GUILD_CHAT_OFF_SCREEN_POLLS) / sizeof(GUILD_CHAT_OFF_SCREEN_POLLS[0])))

static const Color GUILD_CHAT_TITLE_COLOR = {255, 166, 0, 255};
static const Color GUILD_CHAT_LEADER_COLOR = {255, 235, 4, 255};
static const Color GUILD_CHAT_TEXT_COLOR = {255, 255, 255, 255};
static const Color GUILD_EVENT_GOLD = {255, 214, 0, 255};
static const Color GUILD_EVENT_GREEN = {0, 255, 0, 255};
static const Color GUILD_EVENT_RED = {255, 0, 0, 255};
static const Color GUILD_EVENT_BLUE = {0, 138, 255, 255};

typedef enum {
    GUILD_RANK_MEMBER = 0,
    GUILD_RANK_OFFICER = 1,
    GUILD_RANK_LEADER = 2,
    GUILD_RANK_NOT_MEMBER = 999,
} GuildRank;

typedef enum {
    GUILD_STATUS_OPEN = 0,
    GUILD_STATUS_APPLICATION_ONLY = 1,
    GUILD_STATUS_CLOSED = 2,
    GUILD_STATUS_COUNT,
} GuildStatus;

typedef struct {
    const char *code;
    const char *key;
} GuildLanguage;

static const GuildLanguage GUILD_LANGUAGES[] = {
    {"en", "LANGUAGE_ENGLISH"},
    {"fr", "LANGUAGE_FRENCH"},
    {"it", "LANGUAGE_ITALIAN"},
    {"de", "LANGUAGE_GERMAN"},
    {"es", "LANGUAGE_SPANISH"},
    {"pt", "LANGUAGE_PORTUGUESE"},
    {"ja", "LANGUAGE_JAPANESE"},
    {"zh-hans", "LANGUAGE_CHINESE_SIMPLIFIED"},
    {"ko", "LANGUAGE_KOREAN"},
    {"ru", "LANGUAGE_RUSSIAN"},
    {"nl", "LANGUAGE_DUTCH"},
    {"zh-hant", "LANGUAGE_CHINESE_TRADITIONAL"},
};

#define GUILD_LANGUAGE_COUNT ((int)(sizeof(GUILD_LANGUAGES) / sizeof(GUILD_LANGUAGES[0])))

typedef struct {
    const char *text_key;
    Color color;
} GuildEventColor;

static const GuildEventColor GUILD_EVENT_COLORS[] = {
    {"GUILD_CHAT_EVENT_GUILD_CREATION_MESSAGE", GUILD_EVENT_GOLD},
    {"GUILD_CHAT_EVENT_PLAYER_APPLICATION_MESSAGE", GUILD_EVENT_GREEN},
    {"GUILD_CHAT_EVENT_PROMOTION_MESSAGE", GUILD_EVENT_GREEN},
    {"GUILD_CHAT_EVENT_APPLICATION_ACCEPTED_MESSAGE", GUILD_EVENT_GREEN},
    {"GUILD_CHAT_EVENT_MEMBER_JOINED_MESSAGE", GUILD_EVENT_GREEN},
    {"GUILD_CHAT_EVENT_DEMOTION_MESSAGE", GUILD_EVENT_RED},
    {"GUILD_CHAT_EVENT_GUILD_LEADER_AUTODEMOTION_MESSAGE", GUILD_EVENT_RED},
    {"GUILD_CHAT_EVENT_KICK_MESSAGE", GUILD_EVENT_RED},
    {"GUILD_CHAT_EVENT_APPLICATION_REJECTED_MESSAGE", GUILD_EVENT_RED},
    {"GUILD_CHAT_EVENT_MEMBER_LEFT_MESSAGE", GUILD_EVENT_RED},
    {"GUILD_CHAT_EVENT_APPLICATION_WITHDRAWN_MESSAGE", GUILD_EVENT_RED},
    {"GUILD_CHAT_EVENT_GUILD_STATUS_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
    {"GUILD_CHAT_EVENT_GUILD_EMBLEM_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
    {"GUILD_CHAT_EVENT_GUILD_MINIMUM_LEVEL_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
    {"GUILD_CHAT_EVENT_GUILD_LANGUAGE_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
    {"GUILD_CHAT_EVENT_GUILD_DESCRIPTION_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
    {"GUILD_CHAT_EVENT_GUILD_MOTD_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
    {"GUILD_CHAT_EVENT_SETTINGS_CHANGED_MESSAGE", GUILD_EVENT_BLUE},
};

typedef struct {
    char id[GUILD_ID_CAP];
    char name[PLAYER_NAME_CAPACITY];
    GuildRank rank;
    long long last_login;
    int level;
} GuildMember;

typedef struct {
    char id[GUILD_ID_CAP];
    char name[PLAYER_NAME_CAPACITY];
    int level;
    char text[GUILD_TEXT_CAP];
} GuildApplicant;

typedef struct {
    char id[GUILD_ID_CAP];
    char name[GUILD_NAME_CAP];
    char description[GUILD_TEXT_CAP];
    char language[GUILD_LANGUAGE_CAP];
    int icon;
    GuildStatus status;
    int required_level;
    int member_count;
    int max_members;
} GuildSummary;

typedef struct {
    GuildSummary summary;
    char motd[GUILD_TEXT_CAP];
    GuildMember members[GUILD_MAX_MEMBERS];
    int member_total;
    GuildApplicant applicants[GUILD_MAX_APPLICATIONS];
    int applicant_count;
    int max_applications;
    long long last_message_id;
} GuildInfo;

typedef struct {
    long long id;
    long long time;
    bool event;
    char title[GUILD_LINE_TITLE_CAP];
    char text[GUILD_LINE_TEXT_CAP];
    Color text_color;
} GuildChatLine;

typedef enum { GUILD_CHANNEL_GUILD, GUILD_CHANNEL_WORLD, GUILD_CHANNEL_COUNT } GuildChannelId;

typedef struct {
    GuildChatLine lines[GUILD_CHAT_CAPACITY];
    int count;
    long long last_id;
    bool visible;
    int backoff;
    double next_poll_at;
} GuildChannel;

typedef enum {
    GUILD_NOTICE_ACCEPTED,
    GUILD_NOTICE_DENIED,
    GUILD_NOTICE_KICKED,
    GUILD_NOTICE_PROMOTED,
    GUILD_NOTICE_DEMOTED,
    GUILD_NOTICE_NEW_LEADER,
} GuildNoticeKind;

typedef struct {
    GuildNoticeKind kind;
    char guild[GUILD_NAME_CAP];
    GuildRank rank;
} GuildNotice;

typedef enum {
    GUILD_REQUEST_NONE,
    GUILD_REQUEST_STATE,
    GUILD_REQUEST_RECOMMENDED,
    GUILD_REQUEST_SEARCH,
    GUILD_REQUEST_OBSERVE,
    GUILD_REQUEST_JOIN,
    GUILD_REQUEST_CANCEL,
    GUILD_REQUEST_SETTINGS,
    GUILD_REQUEST_ACCEPT,
    GUILD_REQUEST_DENY,
    GUILD_REQUEST_RANK,
    GUILD_REQUEST_KICK,
    GUILD_REQUEST_LEAVE,
    GUILD_REQUEST_MESSAGES,
    GUILD_REQUEST_SEND,
    GUILD_REQUEST_WORLD_MESSAGES,
    GUILD_REQUEST_WORLD_SEND,
    GUILD_REQUEST_CREATE,
    GUILD_REQUEST_ONSLAUGHT,
    GUILD_REQUEST_DUEL,
    GUILD_REQUEST_REPORT,
} GuildRequestKind;

typedef enum {
    GUILD_ABUSE_NONE,
    GUILD_ABUSE_HARASSMENT,
    GUILD_ABUSE_SEXUAL,
    GUILD_ABUSE_EXPLICIT,
    GUILD_ABUSE_OTHER,
} GuildAbuseType;

static const char *const GUILD_ABUSE_NAMES[] = {"NONE", "HARRASSMENT", "SEXUAL", "EXPLICIT", "OTHER"};

typedef struct {
    char id[GUILD_ID_CAP];
    char name[GUILD_NAME_CAP];
    int icon;
    int banners;
    int rank;
} GuildStanding;

typedef struct {
    char id[GUILD_ID_CAP];
    char name[PLAYER_NAME_CAPACITY];
    int banners;
    int rank;
} GuildContribution;

typedef struct {
    long long cycle;
    bool ready;
    bool finished;
    int total_guilds;
    GuildStanding guilds[GUILD_ONSLAUGHT_LIMIT];
    int guild_count;
    bool has_mine;
    GuildStanding mine;
    GuildContribution members[GUILD_ONSLAUGHT_MEMBER_CAP];
    int member_count;
    int my_banners;
    int minimum_banners;
    int award_rank;
    bool claimed;
} GuildOnslaught;

typedef struct {
    int slot;
    char id[GUILD_ID_CAP];
    char tier[8];
    int level;
} GuildDuelUnit;

typedef struct {
    char id[GUILD_ID_CAP];
    char name[PLAYER_NAME_CAPACITY];
    int level;
    int trophies;
    GuildDuelUnit units[SQUAD_SLOT_COUNT];
    int unit_count;
    char runes[GUILD_DUEL_RUNE_CAP][GUILD_RUNE_ID_CAP];
    int rune_count;
} GuildDuel;

typedef struct {
    GuildRequestKind kind;
    char method[8];
    char route[GUILD_ROUTE_CAP];
    cJSON *body;
} GuildRequest;

typedef enum { GUILD_OUTCOME_OK, GUILD_OUTCOME_REFUSED, GUILD_OUTCOME_OFFLINE } GuildOutcome;

typedef struct {
    GuildRequestKind kind;
    GuildOutcome outcome;
    char code[GUILD_CODE_CAP];
} GuildResult;

typedef struct {
    bool known;
    GuildRank rank;
    GuildInfo mine;
    bool has_application;
    GuildSummary applied;
    int unread;
    GuildSummary list[GUILD_LIST_CAPACITY];
    int list_count;
    bool list_ready;
    GuildInfo observed;
    bool observed_applied;
    bool observed_ready;
    GuildChannel channels[GUILD_CHANNEL_COUNT];
    GuildNotice notices[GUILD_NOTICE_CAPACITY];
    int notice_count;
    GuildRequest queue[GUILD_REQUEST_CAPACITY];
    int queue_count;
    GuildRequestKind inflight;
    int ticket;
    int create_ticket;
    GuildResult results[GUILD_REQUEST_CAPACITY];
    int result_count;
    double next_state_at;
    GuildOnslaught current;
    GuildOnslaught previous;
    long long onslaught_inflight;
    GuildDuel duel;
    int claim_ticket;
} Guild;

void GuildInitialize(Guild *g) {
    memset(g, 0, sizeof(*g));
    g->rank = GUILD_RANK_NOT_MEMBER;
    g->current.cycle = -1;
    g->previous.cycle = -1;
    g->claim_ticket = BACKEND_NO_TICKET;
}

bool GuildIsMember(const Guild *g) {
    return g->rank != GUILD_RANK_NOT_MEMBER;
}

bool GuildUnlocked(const Player *p) {
    return p->level >= GUILD_LEVEL_UNLOCK;
}

bool GuildAvailable(void) {
    return BackendEnabled();
}

bool GuildCanWriteToWorld(const Player *p) {
    return p->level >= GUILD_WORLD_SPEAKER_LEVEL;
}

const char *GuildRankKey(GuildRank rank) {
    switch (rank) {
        case GUILD_RANK_LEADER:
            return "GUILD_RANK_LEADER";
        case GUILD_RANK_OFFICER:
            return "GUILD_RANK_OFFICER";
        case GUILD_RANK_MEMBER:
            return "GUILD_RANK_MEMBER";
        default:
            return "GUILD_MEMBER_PENDING";
    }
}

const char *GuildStatusKey(GuildStatus status) {
    switch (status) {
        case GUILD_STATUS_APPLICATION_ONLY:
            return "GUILD_STATUS_APPLICATION_ONLY";
        case GUILD_STATUS_CLOSED:
            return "GUILD_STATUS_CLOSED";
        default:
            return "GUILD_STATUS_OPEN";
    }
}

int GuildLanguageIndex(const char *code) {
    for (int i = 0; i < GUILD_LANGUAGE_COUNT; i++)
        if (!strcmp(GUILD_LANGUAGES[i].code, code)) return i;
    return 0;
}

const char *GuildLanguageName(const char *code) {
    return LocGet(GUILD_LANGUAGES[GuildLanguageIndex(code)].key);
}

static Color GuildEventTextColor(const char *text_key) {
    for (size_t i = 0; i < sizeof(GUILD_EVENT_COLORS) / sizeof(GUILD_EVENT_COLORS[0]); i++)
        if (!strcmp(GUILD_EVENT_COLORS[i].text_key, text_key)) return GUILD_EVENT_COLORS[i].color;
    return GUILD_CHAT_TEXT_COLOR;
}

static GuildRank GuildRankFrom(int value) {
    if (value >= GUILD_RANK_MEMBER && value <= GUILD_RANK_LEADER) return (GuildRank)value;
    return GUILD_RANK_NOT_MEMBER;
}

static GuildStatus GuildStatusFrom(int value) {
    if (value >= GUILD_STATUS_OPEN && value < GUILD_STATUS_COUNT) return (GuildStatus)value;
    return GUILD_STATUS_CLOSED;
}

static long long GuildJsonLong(const cJSON *object, const char *key) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsNumber(item) ? (long long)item->valuedouble : 0;
}

static void GuildParseSummary(GuildSummary *out, const cJSON *json) {
    memset(out, 0, sizeof(*out));
    StrCopy(out->id, sizeof(out->id), JsonStr(json, "id"));
    StrCopy(out->name, sizeof(out->name), JsonStr(json, "name"));
    StrCopy(out->description, sizeof(out->description), JsonStr(json, "description"));
    StrCopy(out->language, sizeof(out->language), JsonStr(json, "language"));
    out->icon = JsonInt(json, "icon");
    out->status = GuildStatusFrom(JsonInt(json, "status"));
    out->required_level = JsonInt(json, "required_player_level");
    out->member_count = JsonInt(json, "member_count");
    out->max_members = JsonInt(json, "max_members");
}

static int GuildCompareMembers(const void *a, const void *b) {
    const GuildMember *left = a, *right = b;
    if (left->level != right->level) return right->level - left->level;
    return strcmp(left->name, right->name);
}

static void GuildParseInfo(GuildInfo *out, const cJSON *json) {
    memset(out, 0, sizeof(*out));
    GuildParseSummary(&out->summary, json);
    StrCopy(out->motd, sizeof(out->motd), JsonStr(json, "message_of_the_day"));
    out->max_applications = JsonInt(json, "max_applications");
    out->last_message_id = GuildJsonLong(json, "last_message_id");
    const cJSON *member;
    cJSON_ArrayForEach(member, cJSON_GetObjectItemCaseSensitive(json, "members")) {
        if (out->member_total >= GUILD_MAX_MEMBERS) break;
        GuildMember *m = &out->members[out->member_total++];
        StrCopy(m->id, sizeof(m->id), JsonStr(member, "id"));
        StrCopy(m->name, sizeof(m->name), JsonStr(member, "name"));
        m->rank = GuildRankFrom(JsonInt(member, "rank"));
        m->last_login = GuildJsonLong(member, "last_login_date");
        m->level = JsonInt(member, "player_level");
    }
    qsort(out->members, (size_t)out->member_total, sizeof(out->members[0]), GuildCompareMembers);
    const cJSON *request;
    cJSON_ArrayForEach(request, cJSON_GetObjectItemCaseSensitive(json, "pending_requests")) {
        if (out->applicant_count >= GUILD_MAX_APPLICATIONS) break;
        GuildApplicant *a = &out->applicants[out->applicant_count++];
        StrCopy(a->id, sizeof(a->id), JsonStr(request, "id"));
        StrCopy(a->name, sizeof(a->name), JsonStr(request, "name"));
        a->level = JsonInt(request, "player_level");
        StrCopy(a->text, sizeof(a->text), JsonStr(request, "application_text"));
    }
}

static GuildNoticeKind GuildNoticeKindFrom(const char *kind, bool *known) {
    static const struct {
        const char *name;
        GuildNoticeKind kind;
    } kinds[] = {
        {"accepted", GUILD_NOTICE_ACCEPTED},
        {"denied", GUILD_NOTICE_DENIED},
        {"kicked", GUILD_NOTICE_KICKED},
        {"promoted", GUILD_NOTICE_PROMOTED},
        {"demoted", GUILD_NOTICE_DEMOTED},
        {"new_leader", GUILD_NOTICE_NEW_LEADER},
    };
    for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
        if (strcmp(kinds[i].name, kind)) continue;
        *known = true;
        return kinds[i].kind;
    }
    *known = false;
    return GUILD_NOTICE_ACCEPTED;
}

static void GuildClearChannel(GuildChannel *channel) {
    channel->count = 0;
    channel->last_id = 0;
}

static void GuildClearChat(Guild *g) {
    GuildClearChannel(&g->channels[GUILD_CHANNEL_GUILD]);
    g->unread = 0;
}

static void GuildAdoptState(Guild *g, const cJSON *json) {
    GuildRank before = g->rank;
    char before_id[GUILD_ID_CAP];
    StrCopy(before_id, sizeof(before_id), g->mine.summary.id);
    g->known = true;
    g->rank = GuildRankFrom(JsonInt(json, "rank"));
    const cJSON *mine = cJSON_GetObjectItemCaseSensitive(json, "guild");
    if (cJSON_IsObject(mine) && GuildIsMember(g)) {
        GuildParseInfo(&g->mine, mine);
    } else {
        g->rank = GUILD_RANK_NOT_MEMBER;
        memset(&g->mine, 0, sizeof(g->mine));
    }
    g->unread = GuildIsMember(g) ? JsonInt(json, "unread") : 0;
    const cJSON *applied = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(json, "applications"), 0);
    g->has_application = cJSON_IsObject(applied);
    if (g->has_application)
        GuildParseSummary(&g->applied, applied);
    else
        memset(&g->applied, 0, sizeof(g->applied));
    const cJSON *notice;
    cJSON_ArrayForEach(notice, cJSON_GetObjectItemCaseSensitive(json, "notices")) {
        bool known = false;
        GuildNoticeKind kind = GuildNoticeKindFrom(JsonStr(notice, "kind"), &known);
        if (!known || g->notice_count >= GUILD_NOTICE_CAPACITY) continue;
        GuildNotice *n = &g->notices[g->notice_count++];
        n->kind = kind;
        StrCopy(n->guild, sizeof(n->guild), JsonStr(notice, "guild"));
        n->rank = GuildRankFrom(JsonInt(notice, "rank"));
    }
    bool switched = strcmp(before_id, g->mine.summary.id) != 0;
    if (!GuildIsMember(g) || switched) GuildClearChat(g);
    if (GuildIsMember(g) && (before == GUILD_RANK_NOT_MEMBER || switched))
        g->channels[GUILD_CHANNEL_GUILD].next_poll_at = 0.0;
}

static void GuildFormatPlayerTitle(char *out, size_t size, const cJSON *message) {
    GuildRank rank = GuildRankFrom(JsonInt(message, "player_rank"));
    const char *name = JsonStr(message, "player_name");
    int level = JsonInt(message, "player_level");
    if (rank == GUILD_RANK_LEADER || rank == GUILD_RANK_OFFICER)
        snprintf(out, size, "%s (%d)(%s)", name, level, LocGet(GuildRankKey(rank)));
    else
        snprintf(out, size, "%s (%d) ", name, level);
}

static void GuildFormatEvent(GuildChatLine *line, const cJSON *message) {
    const char *text_key = JsonStr(message, "text");
    StrCopy(line->title, sizeof(line->title), LocGet(JsonStr(message, "title")));
    char values[GUILD_EVENT_PARAM_CAP][GUILD_NAME_CAP * 2];
    const char *args[GUILD_EVENT_PARAM_CAP];
    int count = 0;
    const cJSON *param;
    cJSON_ArrayForEach(param, cJSON_GetObjectItemCaseSensitive(message, "params")) {
        if (count >= GUILD_EVENT_PARAM_CAP) break;
        const char *text = JsonStr(param, "text");
        StrCopy(values[count], sizeof(values[count]), JsonBool(param, "loc_key") ? LocGet(text) : text);
        args[count] = values[count];
        count++;
    }
    LocFormatArgs(line->text, sizeof(line->text), text_key, args, count);
    line->text_color = GuildEventTextColor(text_key);
}

static void GuildFormatWorldTitle(char *out, size_t size, const cJSON *message) {
    const char *name = JsonStr(message, "player_name");
    const char *guild = JsonStr(message, "guild_name");
    int level = JsonInt(message, "player_level");
    if (guild[0])
        snprintf(out, size, "%s (%d) [%s]", name, level, guild);
    else
        snprintf(out, size, "%s (%d)", name, level);
}

static void GuildAppendChat(Guild *g, GuildChannelId id, const cJSON *message) {
    GuildChannel *channel = &g->channels[id];
    long long message_id = GuildJsonLong(message, "id");
    if (message_id <= channel->last_id) return;
    if (channel->count >= GUILD_CHAT_CAPACITY) {
        memmove(&channel->lines[0], &channel->lines[1], sizeof(channel->lines[0]) * (GUILD_CHAT_CAPACITY - 1));
        channel->count--;
    }
    GuildChatLine *line = &channel->lines[channel->count++];
    memset(line, 0, sizeof(*line));
    line->id = message_id;
    line->time = GuildJsonLong(message, "time");
    line->event = JsonStr(message, "title")[0] != '\0';
    line->text_color = GUILD_CHAT_TEXT_COLOR;
    if (line->event) {
        GuildFormatEvent(line, message);
    } else if (id == GUILD_CHANNEL_WORLD) {
        GuildFormatWorldTitle(line->title, sizeof(line->title), message);
        StrCopy(line->text, sizeof(line->text), JsonStr(message, "text"));
    } else {
        GuildFormatPlayerTitle(line->title, sizeof(line->title), message);
        StrCopy(line->text, sizeof(line->text), JsonStr(message, "text"));
        GuildRank rank = GuildRankFrom(JsonInt(message, "player_rank"));
        if (rank == GUILD_RANK_LEADER) line->text_color = GUILD_CHAT_LEADER_COLOR;
    }
    channel->last_id = message_id;
}

static double GuildChannelInterval(const GuildChannel *channel) {
    const double *polls = channel->visible ? GUILD_CHAT_ON_SCREEN_POLLS : GUILD_CHAT_OFF_SCREEN_POLLS;
    int count = channel->visible ? GUILD_CHAT_ON_SCREEN_POLL_COUNT : GUILD_CHAT_OFF_SCREEN_POLL_COUNT;
    return polls[channel->backoff < count ? channel->backoff : count - 1];
}

static void GuildAdoptChannelMessages(Guild *g, GuildChannelId id, const cJSON *json) {
    GuildChannel *channel = &g->channels[id];
    int before = channel->count;
    long long last_before = channel->last_id;
    const cJSON *message;
    cJSON_ArrayForEach(message, cJSON_GetObjectItemCaseSensitive(json, "messages")) GuildAppendChat(g, id, message);
    bool fresh = channel->last_id > last_before || channel->count != before;
    channel->backoff = fresh ? 0 : channel->backoff + 1;
    channel->next_poll_at = GetTime() + GuildChannelInterval(channel);
}

static void GuildAdoptMessages(Guild *g, const cJSON *json) {
    g->rank = GuildRankFrom(JsonInt(json, "rank"));
    if (!GuildIsMember(g)) {
        GuildClearChat(g);
        g->next_state_at = 0.0;
        return;
    }
    GuildAdoptChannelMessages(g, GUILD_CHANNEL_GUILD, json);
    g->unread = g->channels[GUILD_CHANNEL_GUILD].visible ? 0 : JsonInt(json, "unread");
}

static void GuildAdoptWorldMessages(Guild *g, const cJSON *json) {
    GuildChannel *world = &g->channels[GUILD_CHANNEL_WORLD];
    bool server_history_restarted = GuildJsonLong(json, "last_message_id") < world->last_id;
    if (server_history_restarted) GuildClearChannel(world);
    GuildAdoptChannelMessages(g, GUILD_CHANNEL_WORLD, json);
    if (server_history_restarted) world->next_poll_at = 0.0;
}

static bool GuildQueued(const Guild *g, GuildRequestKind kind) {
    if (g->inflight == kind) return true;
    for (int i = 0; i < g->queue_count; i++)
        if (g->queue[i].kind == kind) return true;
    return false;
}

static bool GuildEnqueue(Guild *g, GuildRequestKind kind, const char *method, const char *route, cJSON *body) {
    if (g->queue_count >= GUILD_REQUEST_CAPACITY) {
        cJSON_Delete(body);
        return false;
    }
    GuildRequest *request = &g->queue[g->queue_count++];
    request->kind = kind;
    StrCopy(request->method, sizeof(request->method), method);
    StrCopy(request->route, sizeof(request->route), route);
    request->body = body;
    return true;
}

static void GuildPushResult(Guild *g, GuildRequestKind kind, GuildOutcome outcome, const char *code) {
    if (g->result_count >= GUILD_REQUEST_CAPACITY) return;
    GuildResult *result = &g->results[g->result_count++];
    result->kind = kind;
    result->outcome = outcome;
    StrCopy(result->code, sizeof(result->code), code ? code : "");
}

bool GuildTakeResult(Guild *g, GuildResult *out) {
    if (g->result_count == 0) return false;
    *out = g->results[0];
    memmove(&g->results[0], &g->results[1], sizeof(g->results[0]) * (size_t)(g->result_count - 1));
    g->result_count--;
    return true;
}

bool GuildTakeNotice(Guild *g, GuildNotice *out) {
    if (g->notice_count == 0) return false;
    *out = g->notices[0];
    memmove(&g->notices[0], &g->notices[1], sizeof(g->notices[0]) * (size_t)(g->notice_count - 1));
    g->notice_count--;
    return true;
}

bool GuildBusy(const Guild *g) {
    return g->inflight != GUILD_REQUEST_NONE || g->queue_count > 0 || g->create_ticket != BACKEND_NO_TICKET ||
           g->claim_ticket != BACKEND_NO_TICKET;
}

static long long GuildNowCycle(void) {
    return PvpEventCycle(ServerNow());
}

long long GuildCurrentOnslaughtCycle(void) {
    long long cycle = GuildNowCycle();
    return PvpEventIsGuildCycle(cycle) ? cycle : -1;
}

long long GuildPreviousOnslaughtCycle(void) {
    return PvpGuildCycleBefore(GuildNowCycle());
}

long long GuildNextOnslaughtCycle(void) {
    return PvpGuildCycleAfter(GuildNowCycle());
}

GuildOnslaught *GuildOnslaughtFor(Guild *g, long long cycle) {
    if (cycle < 0) return NULL;
    if (cycle == GuildCurrentOnslaughtCycle()) return &g->current;
    return &g->previous;
}

void GuildRequestOnslaught(Guild *g, long long cycle) {
    GuildOnslaught *slot = GuildOnslaughtFor(g, cycle);
    if (!slot) return;
    const char *route = TextFormat("guild/onslaught?cycle=%lld", cycle);
    if (g->inflight == GUILD_REQUEST_ONSLAUGHT && g->onslaught_inflight == cycle) return;
    for (int i = 0; i < g->queue_count; i++)
        if (g->queue[i].kind == GUILD_REQUEST_ONSLAUGHT && !strcmp(g->queue[i].route, route)) return;
    if (slot->cycle != cycle) {
        memset(slot, 0, sizeof(*slot));
        slot->cycle = cycle;
    }
    GuildEnqueue(g, GUILD_REQUEST_ONSLAUGHT, "GET", route, NULL);
}

static void GuildOnslaughtFailed(Guild *g) {
    GuildOnslaught *o = GuildOnslaughtFor(g, g->onslaught_inflight);
    if (o && o->cycle == g->onslaught_inflight) o->ready = true;
}

static void GuildRequestOnslaughts(Guild *g) {
    GuildRequestOnslaught(g, GuildCurrentOnslaughtCycle());
    GuildRequestOnslaught(g, GuildPreviousOnslaughtCycle());
}

static void GuildParseStanding(GuildStanding *out, const cJSON *json) {
    memset(out, 0, sizeof(*out));
    StrCopy(out->id, sizeof(out->id), JsonStr(json, "id"));
    StrCopy(out->name, sizeof(out->name), JsonStr(json, "name"));
    out->icon = JsonInt(json, "icon");
    out->banners = JsonInt(json, "banners");
    out->rank = JsonInt(json, "rank");
}

static void GuildRankContributions(GuildOnslaught *o) {
    int rank = 1;
    for (int i = 0; i < o->member_count; i++) {
        if (i > 0 && o->members[i].banners < o->members[i - 1].banners) rank = i + 1;
        o->members[i].rank = rank;
    }
}

static void GuildHandleOnslaught(Guild *g, const cJSON *json) {
    long long cycle = GuildJsonLong(json, "cycle");
    GuildOnslaught *o = GuildOnslaughtFor(g, cycle);
    if (!o) return;
    memset(o, 0, sizeof(*o));
    o->cycle = cycle;
    o->ready = true;
    o->finished = JsonBool(json, "finished");
    o->total_guilds = JsonInt(json, "total_guilds");
    const cJSON *entry;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "guilds")) {
        if (o->guild_count >= GUILD_ONSLAUGHT_LIMIT) break;
        GuildParseStanding(&o->guilds[o->guild_count++], entry);
    }
    const cJSON *mine = cJSON_GetObjectItemCaseSensitive(json, "mine");
    o->has_mine = cJSON_IsObject(mine);
    if (o->has_mine) GuildParseStanding(&o->mine, mine);
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "members")) {
        if (o->member_count >= GUILD_ONSLAUGHT_MEMBER_CAP) break;
        GuildContribution *c = &o->members[o->member_count++];
        StrCopy(c->id, sizeof(c->id), JsonStr(entry, "id"));
        StrCopy(c->name, sizeof(c->name), JsonStr(entry, "name"));
        c->banners = JsonInt(entry, "banners");
    }
    GuildRankContributions(o);
    o->my_banners = JsonInt(json, "my_banners");
    o->minimum_banners = JsonInt(json, "minimum_banners");
    o->award_rank = JsonInt(json, "award_rank");
    o->claimed = JsonBool(json, "claimed");
}

bool GuildOnslaughtAwardAvailable(const GuildOnslaught *o) {
    return o->ready && o->finished && o->award_rank > 0 && !o->claimed;
}

bool GuildClaimOnslaught(Guild *g, Player *p) {
    GuildOnslaught *o = &g->previous;
    if (g->claim_ticket != BACKEND_NO_TICKET || !GuildOnslaughtAwardAvailable(o)) return false;
    if (!ProgressionSave(p)) return false;
    cJSON *fields = cJSON_CreateObject();
    cJSON_AddNumberToObject(fields, "cycle", (double)o->cycle);
    g->claim_ticket = BackendBeginAction(GUILD_ONSLAUGHT_CLAIM_ROUTE, fields);
    cJSON_Delete(fields);
    return g->claim_ticket != BACKEND_NO_TICKET;
}

bool GuildClaimPending(const Guild *g) {
    return g->claim_ticket != BACKEND_NO_TICKET;
}

static void GuildPollClaim(Guild *g, Player *p) {
    if (g->claim_ticket == BACKEND_NO_TICKET) return;
    cJSON *response = NULL;
    BackendActionState state = BackendPollAction(g->claim_ticket, &response);
    if (state == BACKEND_ACTION_PENDING) return;
    g->claim_ticket = BACKEND_NO_TICKET;
    if (state == BACKEND_ACTION_APPLIED) {
        ProgressionAdoptServerProfile(p, response);
        g->previous.claimed = true;
        GuiSoundPlay(SND_MISSION_COLLECT);
    } else {
        if (state == BACKEND_ACTION_REFUSED && cJSON_GetObjectItemCaseSensitive(response, "profile"))
            ProgressionAdoptServerProfile(p, response);
        GuildRequestOnslaught(g, g->previous.cycle);
    }
    cJSON_Delete(response);
}

void GuildRequestDuel(Guild *g, const char *member_id) {
    memset(&g->duel, 0, sizeof(g->duel));
    GuildEnqueue(g, GUILD_REQUEST_DUEL, "GET", TextFormat("guild/members/%s/duel", member_id), NULL);
}

static void GuildHandleDuel(Guild *g, const cJSON *json) {
    GuildDuel *d = &g->duel;
    memset(d, 0, sizeof(*d));
    StrCopy(d->id, sizeof(d->id), JsonStr(json, "id"));
    StrCopy(d->name, sizeof(d->name), JsonStr(json, "name"));
    d->level = JsonInt(json, "level");
    d->trophies = JsonInt(json, "trophies");
    const cJSON *item;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(json, "units")) {
        if (d->unit_count >= SQUAD_SLOT_COUNT) break;
        GuildDuelUnit *unit = &d->units[d->unit_count++];
        unit->slot = JsonInt(item, "slot");
        StrCopy(unit->id, sizeof(unit->id), JsonStr(item, "id"));
        StrCopy(unit->tier, sizeof(unit->tier), JsonStr(item, "tier"));
        unit->level = JsonInt(item, "level");
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(json, "runes")) {
        if (d->rune_count >= GUILD_DUEL_RUNE_CAP || !cJSON_IsString(item)) continue;
        StrCopy(d->runes[d->rune_count++], GUILD_RUNE_ID_CAP, item->valuestring);
    }
}

void GuildRequestReport(Guild *g, const char *kind, GuildAbuseType abuse, const char *target_id) {
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "kind", kind);
    cJSON_AddStringToObject(body, "abuse_type", GUILD_ABUSE_NAMES[abuse]);
    bool guild = !strcmp(kind, GUILD_REPORT_KIND_GUILD);
    cJSON_AddStringToObject(body, guild ? "guild_id" : "applicant_id", target_id);
    GuildEnqueue(g, GUILD_REQUEST_REPORT, "POST", "reports", body);
}

static cJSON *GuildMessagesBody(const Guild *g, GuildChannelId id, const char *text) {
    const GuildChannel *channel = &g->channels[id];
    cJSON *body = cJSON_CreateObject();
    cJSON_AddNumberToObject(body, "after", (double)channel->last_id);
    if (text) cJSON_AddStringToObject(body, "text", text);
    if (id == GUILD_CHANNEL_GUILD && channel->visible)
        cJSON_AddNumberToObject(body, "read_through", (double)channel->last_id);
    return body;
}

static void GuildRequestOnslaughts(Guild *g);

void GuildRefresh(Guild *g) {
    if (!GuildQueued(g, GUILD_REQUEST_STATE)) GuildEnqueue(g, GUILD_REQUEST_STATE, "GET", "guild", NULL);
    GuildRequestOnslaughts(g);
    g->next_state_at = GetTime() + GUILD_STATE_REFRESH_SECONDS;
}

void GuildRequestRecommended(Guild *g) {
    g->list_ready = false;
    g->list_count = 0;
    GuildEnqueue(g, GUILD_REQUEST_RECOMMENDED, "GET", "guilds/recommended", NULL);
}

static void GuildEncodeQuery(char *out, size_t size, const char *text) {
    static const char hex[] = "0123456789ABCDEF";
    size_t used = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c && used + 4 < size; c++) {
        if (isalnum(*c) || *c == '-' || *c == '_' || *c == '.' || *c == '~') {
            out[used++] = (char)*c;
        } else {
            out[used++] = '%';
            out[used++] = hex[*c >> 4];
            out[used++] = hex[*c & 0x0F];
        }
    }
    out[used] = '\0';
}

void GuildRequestSearch(Guild *g, const char *name) {
    char encoded[GUILD_ROUTE_CAP / 2];
    GuildEncodeQuery(encoded, sizeof(encoded), name);
    g->list_ready = false;
    g->list_count = 0;
    GuildEnqueue(g, GUILD_REQUEST_SEARCH, "GET", TextFormat("guilds/search?name=%s", encoded), NULL);
}

void GuildRequestObserve(Guild *g, const char *guild_id) {
    g->observed_ready = false;
    GuildEnqueue(g, GUILD_REQUEST_OBSERVE, "GET", TextFormat("guilds/%s", guild_id), NULL);
}

void GuildRequestJoin(Guild *g, const char *guild_id, const char *text) {
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "application_text", text ? text : "");
    GuildEnqueue(g, GUILD_REQUEST_JOIN, "POST", TextFormat("guilds/%s/join", guild_id), body);
}

void GuildRequestCancel(Guild *g, const char *guild_id) {
    GuildEnqueue(g, GUILD_REQUEST_CANCEL, "POST", TextFormat("guilds/%s/cancel", guild_id), cJSON_CreateObject());
}

typedef struct {
    char name[GUILD_NAME_CAP];
    char description[GUILD_TEXT_CAP];
    char motd[GUILD_TEXT_CAP];
    char language[GUILD_LANGUAGE_CAP];
    GuildStatus status;
    int icon;
    int required_level;
} GuildSettings;

static cJSON *GuildSettingsJson(const GuildSettings *settings) {
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "name", settings->name);
    cJSON_AddStringToObject(body, "description", settings->description);
    cJSON_AddStringToObject(body, "message_of_the_day", settings->motd);
    cJSON_AddStringToObject(body, "language", settings->language);
    cJSON_AddNumberToObject(body, "status", settings->status);
    cJSON_AddNumberToObject(body, "icon", settings->icon);
    cJSON_AddNumberToObject(body, "required_level", settings->required_level);
    return body;
}

void GuildRequestSettings(Guild *g, const GuildSettings *settings) {
    GuildEnqueue(g, GUILD_REQUEST_SETTINGS, "POST", "guild/settings", GuildSettingsJson(settings));
}

bool GuildRequestCreate(Guild *g, Player *p, const GuildSettings *settings) {
    if (g->create_ticket != BACKEND_NO_TICKET) return false;
    if (!ProgressionSave(p)) return false;
    cJSON *fields = GuildSettingsJson(settings);
    g->create_ticket = BackendBeginAction(GUILD_CREATE_ROUTE, fields);
    cJSON_Delete(fields);
    return g->create_ticket != BACKEND_NO_TICKET;
}

void GuildRequestAccept(Guild *g, const char *player_id) {
    GuildEnqueue(
        g, GUILD_REQUEST_ACCEPT, "POST", TextFormat("guild/applications/%s/accept", player_id), cJSON_CreateObject()
    );
}

void GuildRequestDeny(Guild *g, const char *player_id) {
    GuildEnqueue(
        g, GUILD_REQUEST_DENY, "POST", TextFormat("guild/applications/%s/deny", player_id), cJSON_CreateObject()
    );
}

void GuildRequestRank(Guild *g, const char *player_id, GuildRank target) {
    cJSON *body = cJSON_CreateObject();
    cJSON_AddNumberToObject(body, "target_rank", target);
    GuildEnqueue(g, GUILD_REQUEST_RANK, "POST", TextFormat("guild/members/%s/rank", player_id), body);
}

void GuildRequestKick(Guild *g, const char *player_id) {
    GuildEnqueue(g, GUILD_REQUEST_KICK, "POST", TextFormat("guild/members/%s/kick", player_id), cJSON_CreateObject());
}

void GuildRequestLeave(Guild *g) {
    GuildEnqueue(g, GUILD_REQUEST_LEAVE, "POST", "guild/leave", cJSON_CreateObject());
}

void GuildSendChat(Guild *g, GuildChannelId id, const char *text) {
    bool world = id == GUILD_CHANNEL_WORLD;
    GuildEnqueue(
        g,
        world ? GUILD_REQUEST_WORLD_SEND : GUILD_REQUEST_SEND,
        "POST",
        world ? GUILD_WORLD_MESSAGES_ROUTE : GUILD_MESSAGES_ROUTE,
        GuildMessagesBody(g, id, text)
    );
    g->channels[id].backoff = 0;
}

static bool GuildChannelSetVisible(GuildChannel *channel, bool visible) {
    if (channel->visible == visible) return false;
    channel->visible = visible;
    channel->backoff = 0;
    if (visible) channel->next_poll_at = 0.0;
    return true;
}

void GuildSetChatVisible(Guild *g, bool visible) {
    if (GuildChannelSetVisible(&g->channels[GUILD_CHANNEL_GUILD], visible) && visible) g->unread = 0;
}

void GuildSetWorldChatVisible(Guild *g, bool visible) {
    GuildChannelSetVisible(&g->channels[GUILD_CHANNEL_WORLD], visible);
}

void GuildHideChat(Guild *g) {
    GuildSetChatVisible(g, false);
    GuildSetWorldChatVisible(g, false);
}

static void GuildHandleList(Guild *g, const cJSON *json) {
    g->list_count = 0;
    const cJSON *entry;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "guilds")) {
        if (g->list_count >= GUILD_LIST_CAPACITY) break;
        GuildParseSummary(&g->list[g->list_count++], entry);
    }
    g->list_ready = true;
}

static void GuildHandleObserved(Guild *g, const cJSON *json) {
    GuildParseInfo(&g->observed, cJSON_GetObjectItemCaseSensitive(json, "guild"));
    g->observed_applied = JsonBool(json, "applied");
    g->observed_ready = true;
}

static void GuildHandleAnswer(Guild *g, GuildRequestKind kind, int status, const cJSON *json) {
    if (status != GUILD_HTTP_OK) {
        const char *code = JsonStr(json, "code");
        if (kind == GUILD_REQUEST_RECOMMENDED || kind == GUILD_REQUEST_SEARCH) g->list_ready = true;
        if (!strcmp(code, GUILD_CODE_NOT_IN_GUILD) || !strcmp(code, GUILD_CODE_NOT_FOUND)) g->next_state_at = 0.0;
        if (kind == GUILD_REQUEST_MESSAGES) {
            g->channels[GUILD_CHANNEL_GUILD].next_poll_at = GetTime() + GUILD_RETRY_SECONDS;
            return;
        }
        if (kind == GUILD_REQUEST_WORLD_MESSAGES) {
            g->channels[GUILD_CHANNEL_WORLD].next_poll_at = GetTime() + GUILD_RETRY_SECONDS;
            return;
        }
        if (status == GUILD_HTTP_TOO_MANY_REQUESTS &&
            (kind == GUILD_REQUEST_SEND || kind == GUILD_REQUEST_WORLD_SEND)) {
            GuildPushResult(g, kind, GUILD_OUTCOME_REFUSED, GUILD_CODE_SENDING_TOO_FAST);
            return;
        }
        if (kind == GUILD_REQUEST_STATE) {
            g->next_state_at = GetTime() + GUILD_RETRY_SECONDS;
            return;
        }
        if (kind == GUILD_REQUEST_ONSLAUGHT) {
            GuildOnslaughtFailed(g);
            return;
        }
        GuildPushResult(
            g,
            kind,
            status == GUILD_HTTP_REFUSED || status == GUILD_HTTP_NOT_FOUND ? GUILD_OUTCOME_REFUSED
                                                                           : GUILD_OUTCOME_OFFLINE,
            code
        );
        return;
    }
    switch (kind) {
        case GUILD_REQUEST_RECOMMENDED:
        case GUILD_REQUEST_SEARCH:
            GuildHandleList(g, json);
            break;
        case GUILD_REQUEST_OBSERVE:
            GuildHandleObserved(g, json);
            break;
        case GUILD_REQUEST_MESSAGES:
        case GUILD_REQUEST_SEND:
            GuildAdoptMessages(g, json);
            break;
        case GUILD_REQUEST_WORLD_MESSAGES:
        case GUILD_REQUEST_WORLD_SEND:
            GuildAdoptWorldMessages(g, json);
            break;
        case GUILD_REQUEST_ONSLAUGHT:
            GuildHandleOnslaught(g, json);
            return;
        case GUILD_REQUEST_DUEL:
            GuildHandleDuel(g, json);
            break;
        case GUILD_REQUEST_REPORT:
            break;
        default:
            GuildAdoptState(g, json);
            break;
    }
    if (kind != GUILD_REQUEST_MESSAGES && kind != GUILD_REQUEST_WORLD_MESSAGES && kind != GUILD_REQUEST_STATE)
        GuildPushResult(g, kind, GUILD_OUTCOME_OK, "");
}

static void GuildPollInflight(Guild *g) {
    if (g->inflight == GUILD_REQUEST_NONE) return;
    cJSON *json = NULL;
    int status = 0;
    BackendQueryState state = BackendPollQuery(g->ticket, &json, &status);
    if (state == BACKEND_QUERY_PENDING) return;
    GuildRequestKind kind = g->inflight;
    g->inflight = GUILD_REQUEST_NONE;
    g->ticket = BACKEND_NO_TICKET;
    if (state == BACKEND_QUERY_ANSWERED) {
        GuildHandleAnswer(g, kind, status, json);
    } else if (kind == GUILD_REQUEST_MESSAGES) {
        g->channels[GUILD_CHANNEL_GUILD].next_poll_at = GetTime() + GUILD_RETRY_SECONDS;
    } else if (kind == GUILD_REQUEST_WORLD_MESSAGES) {
        g->channels[GUILD_CHANNEL_WORLD].next_poll_at = GetTime() + GUILD_RETRY_SECONDS;
    } else if (kind == GUILD_REQUEST_STATE) {
        g->next_state_at = GetTime() + GUILD_RETRY_SECONDS;
    } else if (kind == GUILD_REQUEST_ONSLAUGHT) {
        GuildOnslaughtFailed(g);
    } else {
        if (kind == GUILD_REQUEST_RECOMMENDED || kind == GUILD_REQUEST_SEARCH) g->list_ready = true;
        GuildPushResult(g, kind, GUILD_OUTCOME_OFFLINE, "");
    }
    cJSON_Delete(json);
}

static void GuildStartNext(Guild *g) {
    if (g->inflight != GUILD_REQUEST_NONE || g->queue_count == 0) return;
    GuildRequest *request = &g->queue[0];
    int ticket = BackendBeginQuery(request->method, request->route, request->body);
    if (ticket == BACKEND_NO_TICKET) return;
    g->inflight = request->kind;
    g->ticket = ticket;
    const char *cycle = strchr(request->route, '=');
    if (request->kind == GUILD_REQUEST_ONSLAUGHT && cycle) g->onslaught_inflight = atoll(cycle + 1);
    cJSON_Delete(request->body);
    memmove(&g->queue[0], &g->queue[1], sizeof(g->queue[0]) * (size_t)(g->queue_count - 1));
    g->queue_count--;
}

static void GuildPollCreate(Guild *g, Player *p) {
    if (g->create_ticket == BACKEND_NO_TICKET) return;
    cJSON *response = NULL;
    BackendActionState state = BackendPollAction(g->create_ticket, &response);
    if (state == BACKEND_ACTION_PENDING) return;
    g->create_ticket = BACKEND_NO_TICKET;
    if (state == BACKEND_ACTION_APPLIED) {
        ProgressionAdoptServerProfile(p, response);
        GuildAdoptState(g, cJSON_GetObjectItemCaseSensitive(response, "granted"));
        GuildPushResult(g, GUILD_REQUEST_CREATE, GUILD_OUTCOME_OK, "");
    } else if (state == BACKEND_ACTION_REFUSED) {
        if (cJSON_GetObjectItemCaseSensitive(response, "profile")) ProgressionAdoptServerProfile(p, response);
        GuildPushResult(g, GUILD_REQUEST_CREATE, GUILD_OUTCOME_REFUSED, JsonStr(response, "code"));
    } else {
        const char *code = state == BACKEND_ACTION_CONFLICT ? GUILD_CODE_PROFILE_CHANGED : "";
        GuildPushResult(g, GUILD_REQUEST_CREATE, GUILD_OUTCOME_OFFLINE, code);
    }
    cJSON_Delete(response);
}

void GuildUpdate(Guild *g, Player *p) {
    if (!GuildAvailable()) return;
    GuildPollCreate(g, p);
    GuildPollClaim(g, p);
    GuildPollInflight(g);
    double now = GetTime();
    bool engaged = GuildUnlocked(p) || GuildIsMember(g) || g->has_application;
    if (engaged && now >= g->next_state_at) GuildRefresh(g);
    GuildChannel *guild = &g->channels[GUILD_CHANNEL_GUILD];
    if (GuildIsMember(g) && now >= guild->next_poll_at && !GuildQueued(g, GUILD_REQUEST_MESSAGES) &&
        !GuildQueued(g, GUILD_REQUEST_SEND)) {
        cJSON *body = GuildMessagesBody(g, GUILD_CHANNEL_GUILD, NULL);
        GuildEnqueue(g, GUILD_REQUEST_MESSAGES, "POST", GUILD_MESSAGES_ROUTE, body);
        guild->next_poll_at = now + GuildChannelInterval(guild);
    }
    GuildChannel *world = &g->channels[GUILD_CHANNEL_WORLD];
    if (world->visible && now >= world->next_poll_at && !GuildQueued(g, GUILD_REQUEST_WORLD_MESSAGES) &&
        !GuildQueued(g, GUILD_REQUEST_WORLD_SEND)) {
        cJSON *body = GuildMessagesBody(g, GUILD_CHANNEL_WORLD, NULL);
        GuildEnqueue(g, GUILD_REQUEST_WORLD_MESSAGES, "POST", GUILD_WORLD_MESSAGES_ROUTE, body);
        world->next_poll_at = now + GuildChannelInterval(world);
    }
    GuildStartNext(g);
}

const GuildMember *GuildFindMember(const GuildInfo *info, const char *player_id) {
    for (int i = 0; i < info->member_total; i++)
        if (!strcmp(info->members[i].id, player_id)) return &info->members[i];
    return NULL;
}

const char *GuildPlayerId(void) {
    return BackendPlayerId();
}
