#define main hod_game_main
#include "../source/main.c"
#undef main

#define STRENGTH_PATH "data/pvp_squad_strength.json"
#define STRENGTH_USAGE                                                                                                 \
    "usage: pvp_squad_strength [--check | --report]\n"                                                                 \
    "Fights every opponent squad of the PvP pool against every other from both sides, as the ladder fields them,\n"    \
    "and writes each squad's wins per 10000 battles to " STRENGTH_PATH ".\n"                                           \
    "tools/build_pvp_rules.py copies them into the pool. --check fails when the pool's strengths differ from the\n"    \
    "measured ones. --report prints how the measured strength spreads over the pool and how strong the opponents\n"    \
    "met at each trophy count are, instead of writing it."
#define STRENGTH_SCALE 10000
#define STRENGTH_PCT_PER_SCALE (100.0 / STRENGTH_SCALE)
#define STRENGTH_SIDES 2
#define STRENGTH_SEED_SALT 0x51A7E9B5u
#define STRENGTH_REPORT_BANDS 10
#define STRENGTH_LADDER_TROPHY_STEP 500
#define STRENGTH_LADDER_MATCHES 400
#define STRENGTH_LADDER_LEVEL 30
#define STRENGTH_LADDER_DAY 20000
#define STRENGTH_EXIT_STALE 1
#define STRENGTH_EXIT_USAGE 2

typedef struct {
    BattleRoster rosters[PVP_SQUAD_CAPACITY];
    int wins[PVP_SQUAD_CAPACITY];
    int strength[PVP_SQUAD_CAPACITY];
    int squad_count;
    int battles_per_squad;
} Strength;

static unsigned StrengthSeed(int attacker, int defender) {
    return STRENGTH_SEED_SALT ^ (unsigned)(attacker * PVP_SQUAD_CAPACITY + defender);
}

static void StrengthFieldSquads(Strength *s) {
    s->squad_count = PvpTableEnsure()->squad_count;
    for (int squad = 0; squad < s->squad_count; squad++) {
        PvpOpponent fielded = {.squad = squad};
        PvpOpponentRoster(&s->rosters[squad], &fielded);
    }
}

static void StrengthFight(Strength *s) {
    static BattleSim sim;
    for (int attacker = 0; attacker < s->squad_count; attacker++) {
        for (int defender = 0; defender < s->squad_count; defender++) {
            if (attacker == defender) continue;
            BattleSimRunRosters(&sim, &s->rosters[attacker], &s->rosters[defender], StrengthSeed(attacker, defender));
            s->wins[attacker] += sim.player_won;
            s->wins[defender] += !sim.player_won;
        }
    }
    s->battles_per_squad = (s->squad_count - 1) * STRENGTH_SIDES;
    for (int squad = 0; squad < s->squad_count; squad++)
        s->strength[squad] = (int)((long long)s->wins[squad] * STRENGTH_SCALE / s->battles_per_squad);
}

static void StrengthWrite(const Strength *s) {
    FILE *file = fopen(STRENGTH_PATH, "w");
    if (!file) TraceLog(LOG_FATAL, "Strength: could not write %s", STRENGTH_PATH);
    fprintf(file, "{\n \"battles_per_squad\": %d,\n \"strength\": [\n", s->battles_per_squad);
    for (int squad = 0; squad < s->squad_count; squad++)
        fprintf(file, "  %d%s\n", s->strength[squad], squad + 1 < s->squad_count ? "," : "");
    fprintf(file, " ]\n}\n");
    fclose(file);
}

static int StrengthCheck(const Strength *s) {
    const PvpTable *table = PvpTableEnsure();
    int stale = 0;
    for (int squad = 0; squad < s->squad_count; squad++) stale += table->squads[squad].strength != s->strength[squad];
    if (!stale) {
        printf("Squad strength: %d pooled squads match the sim\n", s->squad_count);
        return 0;
    }
    printf("Squad strength: %d of %d pooled squads are stale; run make squad-strength\n", stale, s->squad_count);
    return STRENGTH_EXIT_STALE;
}

static int StrengthSquadSize(const PvpSquad *squad) {
    int size = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) size += squad->cells[slot].present;
    return size;
}

static void StrengthReportLadder(const Strength *s) {
    const PvpTable *table = PvpTableEnsure();
    int top_trophies = table->bots[table->bot_count - 1].trophies;
    printf("\ntrophies  mean win share of the opponents met\n");
    for (int trophies = 0; trophies <= top_trophies; trophies += STRENGTH_LADDER_TROPHY_STEP) {
        long long met = 0;
        for (unsigned seed = 1; seed <= STRENGTH_LADDER_MATCHES; seed++)
            met += s->strength[PvpOpponentGenerate(STRENGTH_LADDER_LEVEL, trophies, STRENGTH_LADDER_DAY, seed).squad];
        printf("%8d  %5.1f%%\n", trophies, (double)met / STRENGTH_LADDER_MATCHES * STRENGTH_PCT_PER_SCALE);
    }
}

static void StrengthReport(const Strength *s) {
    const PvpTable *table = PvpTableEnsure();
    int squads[STRENGTH_REPORT_BANDS] = {0}, full[STRENGTH_REPORT_BANDS] = {0};
    for (int squad = 0; squad < s->squad_count; squad++) {
        int band = s->strength[squad] * STRENGTH_REPORT_BANDS / (STRENGTH_SCALE + 1);
        squads[band]++;
        full[band] += StrengthSquadSize(&table->squads[squad]) == SQUAD_SLOT_COUNT;
    }
    printf("%d squads, %d battles each\n", s->squad_count, s->battles_per_squad);
    printf("win share   squads  with a giant\n");
    for (int band = 0; band < STRENGTH_REPORT_BANDS; band++) {
        int low = band * 100 / STRENGTH_REPORT_BANDS, high = (band + 1) * 100 / STRENGTH_REPORT_BANDS;
        printf("%3d%%-%3d%%   %6d  %12d\n", low, high, squads[band], full[band]);
    }
    StrengthReportLadder(s);
}

int main(int argc, char **argv) {
    SetTraceLogLevel(LOG_WARNING);
    bool check = argc == 2 && !strcmp(argv[1], "--check");
    bool report = argc == 2 && !strcmp(argv[1], "--report");
    if (argc > 1 && !check && !report) {
        printf("%s\n", STRENGTH_USAGE);
        return STRENGTH_EXIT_USAGE;
    }
    static Strength strength;
    StrengthFieldSquads(&strength);
    StrengthFight(&strength);
    if (check) return StrengthCheck(&strength);
    if (report) {
        StrengthReport(&strength);
        return 0;
    }
    StrengthWrite(&strength);
    return 0;
}
