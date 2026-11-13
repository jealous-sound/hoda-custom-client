#define PVP_LADDER_CAPACITY 100
#define PVP_LADDER_ROUTE "pvp/ladder"
#define PVP_LADDER_RIVAL_ROUTE_FORMAT PVP_LADDER_ROUTE "/%s"
#define PVP_LEADERBOARD_CAPACITY (PVP_BOT_CAPACITY + PVP_LADDER_CAPACITY + 1)

typedef struct {
    char id[PVP_DUEL_ID_CAP];
    char name[PLAYER_NAME_CAPACITY];
    int level;
    int trophies;
} PvpLadderPlayer;

typedef struct {
    PvpLadderPlayer players[PVP_LADDER_CAPACITY];
    int player_count;
    int revision;
    bool refresh_wanted;
} PvpLadder;

static PvpLadder pvp_ladder;

void PvpLadderRefresh(PvpLadder *ladder) {
    ladder->refresh_wanted = BackendEnabled();
}

bool PvpLadderTakeRefresh(PvpLadder *ladder) {
    bool wanted = ladder->refresh_wanted;
    ladder->refresh_wanted = false;
    return wanted;
}

static bool PvpLadderCanChallenge(const char *id, const char *name) {
    return id[0] && strlen(id) < PVP_DUEL_ID_CAP && name[0];
}

void PvpLadderAdopt(PvpLadder *ladder, const cJSON *reply) {
    ladder->player_count = 0;
    const cJSON *listed;
    cJSON_ArrayForEach(listed, cJSON_GetObjectItemCaseSensitive(reply, "players")) {
        if (ladder->player_count >= PVP_LADDER_CAPACITY) break;
        const char *id = JsonStr(listed, "id");
        const char *name = JsonStr(listed, "name");
        if (!PvpLadderCanChallenge(id, name)) continue;
        PvpLadderPlayer *player = &ladder->players[ladder->player_count++];
        StrCopy(player->id, sizeof(player->id), id);
        StrCopy(player->name, sizeof(player->name), name);
        player->level = JsonInt(listed, "level");
        player->trophies = JsonInt(listed, "trophies");
    }
    ladder->revision++;
}

const PvpLadderPlayer *PvpLadderPlayerAt(const PvpLadder *ladder, int rival) {
    return rival >= 0 && rival < ladder->player_count ? &ladder->players[rival] : NULL;
}

int PvpLadderBuild(const PvpLadder *ladder, PvpRankEntry *out, int trophies, long long day) {
    int count = PvpLeaderboardBuild(out, trophies, day);
    for (int rival = 0; rival < ladder->player_count; rival++)
        out[count++] =
            (PvpRankEntry){.bot = PVP_RANK_RIVAL, .trophies = ladder->players[rival].trophies, .rival = rival};
    qsort(out, (size_t)count, sizeof(out[0]), PvpRankCompare);
    return count;
}

const char *PvpLadderEntryName(const PvpLadder *ladder, const PvpRankEntry *entry) {
    if (entry->bot == PVP_RANK_PLAYER) return LocGet("HODA_PVP_YOU");
    if (entry->bot == PVP_RANK_RIVAL) return ladder->players[entry->rival].name;
    return PvpTableEnsure()->bots[entry->bot].name;
}
