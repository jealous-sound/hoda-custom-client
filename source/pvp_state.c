#define PVP_HISTORY_CAPACITY 30
#define PVP_EVENT_RESULT_CAPACITY 19
#define PVP_HOLD_THRESHOLD_COUNT 5
#define PVP_OPPONENT_NAME_CAP 105
#define PVP_HOLD_SECONDS (7LL * NEXUS_SECONDS_PER_DAY)

static const int PVP_HOLD_THRESHOLDS[PVP_HOLD_THRESHOLD_COUNT] = {500, 750, 1000, 1500, 2000};

typedef struct {
    long long at;
    int bot;
    int squad;
    int level;
    int opponent_trophies;
    int trophy_delta;
    int gold;
    int xp;
    int banners;
    unsigned seed;
    bool defense;
    bool won;
    bool revenged;
    char opponent[PVP_OPPONENT_NAME_CAP];
} PlayerPvpBattle;

_Static_assert(
    sizeof(PlayerPvpBattle) == offsetof(PlayerPvpBattle, opponent) + PVP_OPPONENT_NAME_CAP,
    "PvP history records carry no padding, so equal histories compare equal byte for byte"
);

typedef struct {
    long long cycle;
    int banners;
    int milestones_paid;
    bool buffed;
} PlayerPvpEvent;

typedef struct {
    long long cycle;
    int banners;
    int rank;
    int gold;
    int gems;
    bool claimed;
} PlayerPvpEventResult;

typedef struct {
    int trophies;
    int best_trophies;
    int trophies_seen;
    int wins;
    int losses;
    int win_streak;
    int best_win_streak;
    int trophies_won;
    long long held_since[PVP_HOLD_THRESHOLD_COUNT];
    int defense_wins;
    int defense_losses;
    int nexts_used;
    long long nexts_day;
    long long defense_checked_at;
    int history_count;
    PlayerPvpBattle history[PVP_HISTORY_CAPACITY];
    PlayerPvpEvent event;
    int event_result_count;
    PlayerPvpEventResult event_results[PVP_EVENT_RESULT_CAPACITY];
} PlayerPvp;
