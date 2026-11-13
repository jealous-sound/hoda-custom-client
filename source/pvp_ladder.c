#define PVP_LADDER_CAPACITY 250
#define PVP_LADDER_ROUTE "pvp/ladder"
#define PVP_LADDER_RIVAL_ROUTE_FORMAT PVP_LADDER_ROUTE "/%s"
#define PVP_EVENT_LADDER_ROUTE "pvp/event/ladder"
#define PVP_LADDER_TROPHIES_FIELD "trophies"
#define PVP_LADDER_BANNERS_FIELD "banners"
#define PVP_LEADERBOARD_CAPACITY (PVP_BOT_CAPACITY + PVP_LADDER_CAPACITY + 1)

typedef struct {
    char id[PVP_DUEL_ID_CAP];
    char name[PLAYER_NAME_CAPACITY];
    int level;
    int score;
} PvpLadderPlayer;

typedef struct {
    PvpLadderPlayer players[PVP_LADDER_CAPACITY];
    int player_count;
    int revision;
    bool refresh_wanted;
} PvpLadder;

typedef struct {
    long long cycle;
    int rank;
} PvpEventStanding;

typedef struct {
    PvpLadder rivals;
    long long cycle;
    PvpEventStanding finished[PVP_EVENT_RESULT_CAPACITY];
    int finished_count;
} PvpEventLadder;

static PvpLadder pvp_ladder;
static PvpEventLadder pvp_event_ladder;

void PvpLadderRefresh(PvpLadder *ladder) {
    ladder->refresh_wanted = BackendEnabled();
}

bool PvpLadderTakeRefresh(PvpLadder *ladder) {
    bool wanted = ladder->refresh_wanted;
    ladder->refresh_wanted = false;
    return wanted;
}

static bool PvpLadderCanList(const char *id, const char *name) {
    return id[0] && strlen(id) < PVP_DUEL_ID_CAP && name[0];
}

static void PvpLadderAdoptPlayers(PvpLadder *ladder, const cJSON *reply, const char *score_field) {
    ladder->player_count = 0;
    const cJSON *listed;
    cJSON_ArrayForEach(listed, cJSON_GetObjectItemCaseSensitive(reply, "players")) {
        if (ladder->player_count >= PVP_LADDER_CAPACITY) break;
        const char *id = JsonStr(listed, "id");
        const char *name = JsonStr(listed, "name");
        if (!PvpLadderCanList(id, name)) continue;
        PvpLadderPlayer *player = &ladder->players[ladder->player_count++];
        StrCopy(player->id, sizeof(player->id), id);
        StrCopy(player->name, sizeof(player->name), name);
        player->level = JsonInt(listed, "level");
        player->score = JsonInt(listed, score_field);
    }
    ladder->revision++;
}

void PvpLadderAdopt(PvpLadder *ladder, const cJSON *reply) {
    PvpLadderAdoptPlayers(ladder, reply, PVP_LADDER_TROPHIES_FIELD);
}

void PvpEventLadderAdopt(PvpEventLadder *ladder, const cJSON *reply) {
    ladder->cycle = JsonInt(reply, "cycle");
    ladder->finished_count = 0;
    const cJSON *finished;
    cJSON_ArrayForEach(finished, cJSON_GetObjectItemCaseSensitive(reply, "results")) {
        if (ladder->finished_count == PVP_EVENT_RESULT_CAPACITY) break;
        PvpEventStanding standing = {.cycle = JsonInt(finished, "cycle"), .rank = JsonInt(finished, "rank")};
        if (standing.rank >= 1) ladder->finished[ladder->finished_count++] = standing;
    }
    PvpLadderAdoptPlayers(&ladder->rivals, reply, PVP_LADDER_BANNERS_FIELD);
}

const PvpLadderPlayer *PvpLadderPlayerAt(const PvpLadder *ladder, int rival) {
    return rival >= 0 && rival < ladder->player_count ? &ladder->players[rival] : NULL;
}

static int PvpLadderMerge(const PvpLadder *ladder, PvpRankEntry *out, int count) {
    for (int rival = 0; rival < ladder->player_count; rival++)
        out[count++] = (PvpRankEntry){.bot = PVP_RANK_RIVAL, .trophies = ladder->players[rival].score, .rival = rival};
    qsort(out, (size_t)count, sizeof(out[0]), PvpRankCompare);
    return count;
}

int PvpLadderBuild(const PvpLadder *ladder, PvpRankEntry *out, int trophies, long long day) {
    return PvpLadderMerge(ladder, out, PvpLeaderboardBuild(out, trophies, day));
}

int PvpEventLadderBuild(
    const PvpEventLadder *ladder, PvpRankEntry *out, int banners, long long cycle, long long elapsed
) {
    int count = PvpEventLeaderboardBuild(out, banners, cycle, elapsed);
    return ladder->cycle == cycle ? PvpLadderMerge(&ladder->rivals, out, count) : count;
}

int PvpEventRivalsAhead(long long cycle, int banners) {
    const PvpEventLadder *ladder = &pvp_event_ladder;
    if (ladder->cycle != cycle) return 0;
    int ahead = 0;
    for (int rival = 0; rival < ladder->rivals.player_count; rival++)
        ahead += ladder->rivals.players[rival].score > banners;
    return ahead;
}

void PvpEventLadderRankResults(const PvpEventLadder *ladder, PlayerPvp *pvp) {
    for (int i = 0; i < ladder->finished_count; i++) {
        const PvpEventStanding *standing = &ladder->finished[i];
        for (int r = 0; r < pvp->event_result_count; r++) {
            PlayerPvpEventResult *result = &pvp->event_results[r];
            bool ranked_here = result->cycle == standing->cycle && !result->claimed;
            if (ranked_here && result->rank != standing->rank) PvpEventResultRank(result, standing->rank);
        }
    }
}

const char *PvpLadderEntryName(const PvpLadder *ladder, const PvpRankEntry *entry) {
    if (entry->bot == PVP_RANK_PLAYER) return LocGet("HODA_PVP_YOU");
    if (entry->bot == PVP_RANK_RIVAL) return ladder->players[entry->rival].name;
    return PvpTableEnsure()->bots[entry->bot].name;
}
