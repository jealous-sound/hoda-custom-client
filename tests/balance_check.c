#define main hod_game_main
#include "../source/main.c"
#undef main

#define BALANCE_USAGE                                                                                                  \
    "usage: balance_check [--raw | --neutral] [--holdout] [--seeds N] [--squads N] [--band LOW HIGH] [--all]\n"        \
    "                     [--stacks | --squad-report | --search TRIES [--unique]] [--nudge PCT]\n"                     \
    "                     [--calibrate ROUNDS] [--hero UNIT_ID]...\n"                                                  \
    "Seats every hero in reference squads of its faction, fights the same squad with a peer in that seat from both\n"  \
    "sides and reports the win rate in the hero's better row. SEEDS is the number of peers per seat.\n"                \
    "--raw fights with catalog stats, --neutral with the stat budget but without the per-hero handicaps.\n"            \
    "--holdout uses reference squads and peers the calibration never saw. --all lists every hero, weakest first.\n"    \
    "--stacks fields four copies of each hero against the unchanged squads.\n"                                         \
    "--squad-report fights the reference squads against each other and prints squad and faction win rates.\n"          \
    "--search climbs from each faction's strongest pooled squad by single swaps and prints the squad it ends on;\n"    \
    "--unique forbids two copies of a hero in it.\n"                                                                   \
    "--nudge fights each hero with its own handicap raised by PCT percent, to show how steep the win rate is.\n"       \
    "--calibrate moves the handicaps toward an even win rate for ROUNDS rounds and writes them back.\n"                \
    "--hero limits the run to that hero and fails when it ends outside the band."
#define BALANCE_HERO_CAPACITY STAT_BUDGET_HERO_CAPACITY
#define BALANCE_SQUAD_CAPACITY 64
#define BALANCE_GATED_HERO_CAPACITY 32
#define BALANCE_DEFAULT_SQUADS 32
#define BALANCE_DEFAULT_SEEDS 4
#define BALANCE_SEED_CAPACITY 16
#define BALANCE_PEER_PICK_MULTIPLIER 2654435761u
#define BALANCE_DEFAULT_BAND_LOW_PCT 45.0f
#define BALANCE_DEFAULT_BAND_HIGH_PCT 55.0f
#define BALANCE_EVEN_WIN_PCT 50.0f
#define BALANCE_LOPSIDED_LOW_PCT 25.0f
#define BALANCE_LOPSIDED_HIGH_PCT 75.0f
#define BALANCE_SQUAD_BAND_LOW_PCT 40.0f
#define BALANCE_SQUAD_BAND_HIGH_PCT 60.0f
#define BALANCE_EXTREMES_SHOWN 10
#define BALANCE_SQUAD_EXTREMES_SHOWN 3
#define BALANCE_STAR_CAPACITY 4
#define BALANCE_FODDER_GROUP "Wisp"
#define BALANCE_ANY_FACTION 15u
#define BALANCE_FACTION_COUNT 4
#define BALANCE_NO_FACTION -1
#define BALANCE_ROW_COUNT 2
#define BALANCE_FRONT_ROW 0
#define BALANCE_BACK_ROW 1
#define BALANCE_SIDES 2
#define BALANCE_SEED_SALT 0x9E3779B9u
#define BALANCE_EVALUATION_SEED_BASE 0
#define BALANCE_HOLDOUT_SEED_BASE 500000
#define BALANCE_CALIBRATION_SEED_BASE 300000
#define BALANCE_CALIBRATION_GAIN 2.0f
#define BALANCE_CALIBRATION_FIRST_STEP_LIMIT 0.4f
#define BALANCE_CALIBRATION_STEP_LIMIT_DECAY 0.9f
#define BALANCE_SEARCH_SEED_BASE 700000
#define BALANCE_SEARCH_RNG_SEED 0x5EA4C4EDu
#define BALANCE_SEARCH_DUEL_SEED_FACTOR 50
#define BALANCE_EXIT_OUT_OF_BAND 1
#define BALANCE_EXIT_USAGE 2

static const char *const BALANCE_FACTION_NAMES[BALANCE_FACTION_COUNT] = {"Black", "White", "Red", "Blue"};
static const char *const BALANCE_RARITY_NAMES[] = {"", "Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic"};

typedef struct {
    int wins, battles;
} BalanceTally;

typedef struct {
    char id[64];
    char name[64];
    Rarity rarity;
    int stars;
    bool giant;
    unsigned faction_mask;
    const cJSON *def;
    BalanceTally rows[BALANCE_ROW_COUNT];
    BalanceTally matchups[BALANCE_ROW_COUNT][BALANCE_SQUAD_CAPACITY];
} BalanceHero;

typedef struct {
    bool raw;
    bool neutral;
    bool holdout;
    bool list_all;
    bool stacks;
    bool squad_report;
    bool unique;
    int search_tries;
    int nudge_pct;
    int calibrate_rounds;
    int seeds;
    int squads;
    float band_low_pct, band_high_pct;
    const char *gated[BALANCE_GATED_HERO_CAPACITY];
    int gated_count;
} BalanceOptions;

typedef struct {
    const cJSON *catalog;
    StatBudget budget;
    const StatBudget *stat_budget;
    BalanceHero heroes[BALANCE_HERO_CAPACITY];
    int hero_count;
    BattleRoster squads[BALANCE_SQUAD_CAPACITY];
    unsigned squad_faction[BALANCE_SQUAD_CAPACITY];
    BalanceTally squad_tally[BALANCE_SQUAD_CAPACITY];
    BalanceTally faction_tally[BALANCE_FACTION_COUNT][BALANCE_FACTION_COUNT];
    int squad_count;
    int seeds, seed_base;
    unsigned search_rng;
    BalanceTally mirrors;
} Balance;

static float BalanceTallyPct(BalanceTally tally) {
    return tally.battles ? 100.0f * tally.wins / tally.battles : 0.0f;
}

static void BalanceTallyAdd(BalanceTally *tally, int wins, int battles) {
    tally->wins += wins;
    tally->battles += battles;
}

static int BalanceBestRow(const BalanceHero *hero) {
    bool back_is_better =
        BalanceTallyPct(hero->rows[BALANCE_BACK_ROW]) > BalanceTallyPct(hero->rows[BALANCE_FRONT_ROW]);
    return back_is_better ? BALANCE_BACK_ROW : BALANCE_FRONT_ROW;
}

static BalanceTally BalanceBestTally(const BalanceHero *hero) {
    return hero->rows[BalanceBestRow(hero)];
}

static float BalanceWinPct(const BalanceHero *hero) {
    return BalanceTallyPct(BalanceBestTally(hero));
}

static bool BalanceFought(const BalanceHero *hero) {
    return BalanceBestTally(hero).battles > 0;
}

static bool BalanceInBand(const BalanceOptions *o, const BalanceHero *hero) {
    float pct = BalanceWinPct(hero);
    return pct >= o->band_low_pct && pct <= o->band_high_pct;
}

static void BalanceMaxOut(Character *ch) {
    ch->tier = ch->max_tier;
    int max_level = ch->stats[ch->tier].max_level;
    ch->level = max_level > 0 ? max_level : 1;
}

static bool BalanceFieldsInBattle(const Character *ch) {
    return ch->has_tier[ch->max_tier] && ConsumeProfileFor(ch) && !SimPassiveHasGroup(ch, BALANCE_FODDER_GROUP);
}

static int BalanceHeroById(const void *a, const void *b) {
    return strcmp(((const BalanceHero *)a)->id, ((const BalanceHero *)b)->id);
}

static void BalanceLoadHeroes(Balance *b) {
    static Character ch;
    static Ability ability;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, b->catalog) {
        CharacterParse(row, &ch, &ability);
        if (!BalanceFieldsInBattle(&ch) || b->hero_count == BALANCE_HERO_CAPACITY) continue;
        BalanceHero *hero = &b->heroes[b->hero_count++];
        StrCopy(hero->id, sizeof(hero->id), ch.id);
        StrCopy(hero->name, sizeof(hero->name), ch.name);
        hero->rarity = ch.rarity;
        hero->stars = ch.stars;
        hero->giant = ch.giant;
        hero->faction_mask = SimFactionMask(ch.faction);
        hero->def = row;
    }
    qsort(b->heroes, (size_t)b->hero_count, sizeof(b->heroes[0]), BalanceHeroById);
}

static void BalanceLoadBudget(Balance *b, const BalanceOptions *o) {
    if (o->raw) return;
    const StatBudget *stored = StatBudgetEnsure();
    b->budget = *stored;
    b->budget.hero_count = b->hero_count;
    for (int i = 0; i < b->hero_count; i++) {
        StatBudgetHero *handicap = &b->budget.heroes[i];
        StrCopy(handicap->id, sizeof(handicap->id), b->heroes[i].id);
        handicap->budget_pct = o->neutral ? STAT_BUDGET_NEUTRAL_HERO_PCT : StatBudgetHeroPct(stored, b->heroes[i].id);
    }
    b->stat_budget = &b->budget;
}

static int BalanceHandicap(const Balance *b, const BalanceHero *hero) {
    return b->stat_budget ? b->budget.heroes[hero - b->heroes].budget_pct : STAT_BUDGET_NEUTRAL_HERO_PCT;
}

static bool BalanceSquadIsFull(const PvpSquad *squad) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (!squad->cells[slot].present) return false;
    return true;
}

static void BalanceRosterFromSquad(Balance *b, BattleRoster *roster, const PvpSquad *squad) {
    memset(roster, 0, sizeof(*roster));
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const cJSON *def = CatalogFindCharacter((cJSON *)b->catalog, squad->cells[slot].id);
        if (!def) TraceLog(LOG_FATAL, "Balance: unit %s is not in the catalog", squad->cells[slot].id);
        CharacterParse(def, &roster->chars[slot], &roster->abilities[slot]);
        BalanceMaxOut(&roster->chars[slot]);
        roster->present[slot] = true;
    }
    roster->stat_budget = b->stat_budget;
}

static unsigned BalanceFormationFaction(const BattleRoster *roster) {
    unsigned shared = BALANCE_ANY_FACTION;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (roster->present[slot] && !roster->chars[slot].giant) shared &= SimFactionMask(roster->chars[slot].faction);
    return shared ? shared : BALANCE_ANY_FACTION;
}

static int BalanceSquadFactionIndex(const Balance *b, int squad) {
    unsigned shared = b->squad_faction[squad];
    if (shared == BALANCE_ANY_FACTION) return BALANCE_NO_FACTION;
    for (int faction = 0; faction < BALANCE_FACTION_COUNT; faction++)
        if (shared & (1u << faction)) return faction;
    return BALANCE_NO_FACTION;
}

static bool BalanceHeroKeepsFormation(const Balance *b, const BalanceHero *hero, int squad) {
    return hero->giant || (hero->faction_mask & b->squad_faction[squad]) != 0;
}

static int BalanceEvenlySpread(int pick, int wanted, int pooled) {
    return wanted > 1 ? pick * (pooled - 1) / (wanted - 1) : pooled - 1;
}

static int BalanceDropCalibrationSquads(int *pool, int pooled, int wanted) {
    bool calibrated[PVP_SQUAD_CAPACITY] = {0};
    for (int i = 0; i < wanted; i++) calibrated[BalanceEvenlySpread(i, wanted, pooled)] = true;
    int kept = 0;
    for (int i = 0; i < pooled; i++)
        if (!calibrated[i]) pool[kept++] = pool[i];
    return kept;
}

static void BalanceLoadSquads(Balance *b, const BalanceOptions *o) {
    const PvpTable *table = PvpTableEnsure();
    int pool[PVP_SQUAD_CAPACITY];
    int pooled = 0;
    for (int i = 0; i < table->squad_count; i++)
        if (BalanceSquadIsFull(&table->squads[i])) pool[pooled++] = i;
    int wanted = o->squads < pooled ? o->squads : pooled;
    if (o->holdout) pooled = BalanceDropCalibrationSquads(pool, pooled, wanted);
    if (wanted > pooled) wanted = pooled;
    for (int i = 0; i < wanted; i++) {
        const PvpSquad *squad = &table->squads[pool[BalanceEvenlySpread(i, wanted, pooled)]];
        BalanceRosterFromSquad(b, &b->squads[b->squad_count], squad);
        b->squad_faction[b->squad_count] = BalanceFormationFaction(&b->squads[b->squad_count]);
        b->squad_count++;
    }
}

static void BalanceRosterPlaceHero(BattleRoster *roster, const BalanceHero *hero, int slot) {
    CharacterParse(hero->def, &roster->chars[slot], &roster->abilities[slot]);
    BalanceMaxOut(&roster->chars[slot]);
}

static void BalanceRosterCopy(BattleRoster *out, const BattleRoster *squad) {
    *out = *squad;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) out->chars[slot].ability = &out->abilities[slot];
}

static int BalanceSlotRow(int slot) {
    return SquadSlotIsBack(slot) ? BALANCE_BACK_ROW : BALANCE_FRONT_ROW;
}

static unsigned BalanceSeed(int squad, int slot, int seed) {
    return BALANCE_SEED_SALT ^ (unsigned)(squad * 8191 + slot * 127 + seed);
}

static int BalanceWinsFromBothSides(const BattleRoster *challenger, const BattleRoster *reference, unsigned seed) {
    static BattleSim sim;
    BattleSimRunRosters(&sim, challenger, reference, seed);
    int wins = sim.player_won;
    BattleSimRunRosters(&sim, reference, challenger, seed);
    return wins + !sim.player_won;
}

static void BalanceHeroFights(
    Balance *b, BalanceHero *hero, int squad, int slot, const BattleRoster *challenger, const BattleRoster *opponents
) {
    int row = BalanceSlotRow(slot);
    for (int seed = 0; seed < b->seeds; seed++) {
        unsigned battle_seed = BalanceSeed(squad, slot, b->seed_base + seed);
        int wins = BalanceWinsFromBothSides(challenger, &opponents[seed], battle_seed);
        BalanceTallyAdd(&hero->rows[row], wins, BALANCE_SIDES);
        BalanceTallyAdd(&hero->matchups[row][squad], wins, BALANCE_SIDES);
    }
}

static bool BalanceHeroTakesSeat(const Balance *b, const BalanceHero *hero, int squad, int slot) {
    return hero->giant == (slot == SQUAD_SLOT_LARGE) && BalanceHeroKeepsFormation(b, hero, squad);
}

static bool BalanceIsGated(const BalanceOptions *o, const BalanceHero *hero) {
    if (!o->gated_count) return true;
    for (int g = 0; g < o->gated_count; g++)
        if (!strcmp(o->gated[g], hero->id)) return true;
    return false;
}

static int BalanceNudgeHandicap(Balance *b, int hero, int nudge_pct) {
    if (!b->stat_budget) return 0;
    int settled = b->budget.heroes[hero].budget_pct;
    b->budget.heroes[hero].budget_pct = settled * (100 + nudge_pct) / 100;
    return settled;
}

static void BalanceSettleHandicap(Balance *b, int hero, int settled) {
    if (b->stat_budget) b->budget.heroes[hero].budget_pct = settled;
}

static void
BalanceSeatPeers(Balance *b, int squad, int slot, const int *seated, int seated_count, BattleRoster *peers) {
    for (int seed = 0; seed < b->seeds; seed++) {
        unsigned pick = BalanceSeed(squad, slot, b->seed_base + seed) * BALANCE_PEER_PICK_MULTIPLIER;
        BalanceRosterCopy(&peers[seed], &b->squads[squad]);
        BalanceRosterPlaceHero(&peers[seed], &b->heroes[seated[(pick >> 8) % (unsigned)seated_count]], slot);
    }
}

static void BalanceFightSeat(Balance *b, const BalanceOptions *o, int squad, int slot) {
    static BattleRoster peers[BALANCE_SEED_CAPACITY];
    static BattleRoster challenger;
    static int seated[BALANCE_HERO_CAPACITY];
    int seated_count = 0;
    for (int i = 0; i < b->hero_count; i++)
        if (BalanceHeroTakesSeat(b, &b->heroes[i], squad, slot)) seated[seated_count++] = i;
    if (!seated_count) return;
    BalanceSeatPeers(b, squad, slot, seated, seated_count, peers);
    for (int i = 0; i < seated_count; i++) {
        BalanceHero *hero = &b->heroes[seated[i]];
        if (!BalanceIsGated(o, hero)) continue;
        int settled = BalanceNudgeHandicap(b, seated[i], o->nudge_pct);
        BalanceRosterCopy(&challenger, &b->squads[squad]);
        BalanceRosterPlaceHero(&challenger, hero, slot);
        BalanceHeroFights(b, hero, squad, slot, &challenger, peers);
        BalanceSettleHandicap(b, seated[i], settled);
    }
}

static void BalanceFightStacked(Balance *b, const BalanceOptions *o, int squad) {
    static BattleRoster unchanged[BALANCE_SEED_CAPACITY];
    static BattleRoster challenger;
    for (int seed = 0; seed < b->seeds; seed++) BalanceRosterCopy(&unchanged[seed], &b->squads[squad]);
    for (int i = 0; i < b->hero_count; i++) {
        BalanceHero *hero = &b->heroes[i];
        if (hero->giant || !BalanceIsGated(o, hero)) continue;
        BalanceRosterCopy(&challenger, &b->squads[squad]);
        for (int slot = 0; slot < SQUAD_SLOT_LARGE; slot++) BalanceRosterPlaceHero(&challenger, hero, slot);
        BalanceHeroFights(b, hero, squad, SQUAD_SLOT_LARGE, &challenger, unchanged);
    }
}

static void BalanceFightHeroes(Balance *b, const BalanceOptions *o, int seed_base) {
    b->seed_base = seed_base;
    for (int i = 0; i < b->hero_count; i++) {
        if (!BalanceIsGated(o, &b->heroes[i])) continue;
        memset(b->heroes[i].rows, 0, sizeof(b->heroes[i].rows));
        memset(b->heroes[i].matchups, 0, sizeof(b->heroes[i].matchups));
    }
    for (int q = 0; q < b->squad_count; q++) {
        if (o->stacks) {
            BalanceFightStacked(b, o, q);
            continue;
        }
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) BalanceFightSeat(b, o, q, slot);
    }
}

static void BalanceFightMirrors(Balance *b, int seeds, int seed_base) {
    static BattleSim sim;
    for (int q = 0; q < b->squad_count; q++) {
        for (int seed = 1; seed <= seeds * SQUAD_SLOT_COUNT; seed++) {
            BattleSimRunRosters(&sim, &b->squads[q], &b->squads[q], BalanceSeed(q, SQUAD_SLOT_COUNT, seed_base + seed));
            BalanceTallyAdd(&b->mirrors, sim.player_won, 1);
        }
    }
}

static void BalanceFightSquads(Balance *b, int seeds, int seed_base) {
    static BattleSim sim;
    for (int attacker = 0; attacker < b->squad_count; attacker++) {
        for (int defender = 0; defender < b->squad_count; defender++) {
            if (attacker == defender) continue;
            int attacker_faction = BalanceSquadFactionIndex(b, attacker);
            int defender_faction = BalanceSquadFactionIndex(b, defender);
            for (int seed = 1; seed <= seeds; seed++) {
                unsigned battle_seed = BalanceSeed(attacker, defender, seed_base + seed);
                BattleSimRunRosters(&sim, &b->squads[attacker], &b->squads[defender], battle_seed);
                BalanceTallyAdd(&b->squad_tally[attacker], sim.player_won, 1);
                BalanceTallyAdd(&b->squad_tally[defender], !sim.player_won, 1);
                if (attacker_faction == BALANCE_NO_FACTION || defender_faction == BALANCE_NO_FACTION) continue;
                BalanceTallyAdd(&b->faction_tally[attacker_faction][defender_faction], sim.player_won, 1);
                BalanceTallyAdd(&b->faction_tally[defender_faction][attacker_faction], !sim.player_won, 1);
            }
        }
    }
}

static int BalanceByWinPct(const void *a, const void *b) {
    float delta = BalanceWinPct(*(const BalanceHero *const *)a) - BalanceWinPct(*(const BalanceHero *const *)b);
    return (delta > 0) - (delta < 0);
}

static int BalanceByInt(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int BalanceClampHandicap(float pct) {
    int rounded = (int)lroundf(pct);
    if (rounded < STAT_BUDGET_HERO_MIN_PCT) return STAT_BUDGET_HERO_MIN_PCT;
    if (rounded > STAT_BUDGET_HERO_MAX_PCT) return STAT_BUDGET_HERO_MAX_PCT;
    return rounded;
}

static bool BalanceHandicapAtLimit(int pct) {
    return pct == STAT_BUDGET_HERO_MIN_PCT || pct == STAT_BUDGET_HERO_MAX_PCT;
}

static void BalanceCentreHandicaps(Balance *b, bool giants) {
    static int sorted[BALANCE_HERO_CAPACITY];
    int count = 0;
    for (int i = 0; i < b->hero_count; i++)
        if (b->heroes[i].giant == giants) sorted[count++] = b->budget.heroes[i].budget_pct;
    if (!count) return;
    qsort(sorted, (size_t)count, sizeof(sorted[0]), BalanceByInt);
    float median = (float)sorted[count / 2];
    for (int i = 0; i < b->hero_count; i++) {
        int *pct = &b->budget.heroes[i].budget_pct;
        if (b->heroes[i].giant == giants)
            *pct = BalanceClampHandicap(*pct * (float)STAT_BUDGET_NEUTRAL_HERO_PCT / median);
    }
}

static int BalanceInBandCount(const Balance *b, const BalanceOptions *o) {
    int in_band = 0;
    for (int i = 0; i < b->hero_count; i++)
        in_band += BalanceIsGated(o, &b->heroes[i]) && BalanceFought(&b->heroes[i]) && BalanceInBand(o, &b->heroes[i]);
    return in_band;
}

static int BalanceFoughtCount(const Balance *b, const BalanceOptions *o) {
    int fought = 0;
    for (int i = 0; i < b->hero_count; i++) fought += BalanceIsGated(o, &b->heroes[i]) && BalanceFought(&b->heroes[i]);
    return fought;
}

static void BalanceWriteHandicaps(const Balance *b) {
    FILE *file = fopen(STAT_BUDGET_HEROES_PATH, "w");
    if (!file) TraceLog(LOG_FATAL, "Balance: could not write %s", STAT_BUDGET_HEROES_PATH);
    fprintf(file, "{\n \"version\": %d,\n", STAT_BUDGET_HEROES_VERSION);
    fprintf(file, " \"heroes\": {\n");
    for (int i = 0; i < b->hero_count; i++) {
        const StatBudgetHero *handicap = &b->budget.heroes[i];
        fprintf(file, "  \"%s\": %d%s\n", handicap->id, handicap->budget_pct, i + 1 < b->hero_count ? "," : "");
    }
    fprintf(file, " }\n}\n");
    fclose(file);
}

static float BalanceCalibrationStep(int round, float win_pct) {
    float limit = BALANCE_CALIBRATION_FIRST_STEP_LIMIT * powf(BALANCE_CALIBRATION_STEP_LIMIT_DECAY, (float)(round - 1));
    float step = BALANCE_CALIBRATION_GAIN * (win_pct - BALANCE_EVEN_WIN_PCT) / 100.0f;
    return step > limit ? limit : step < -limit ? -limit : step;
}

static void BalanceCalibrate(Balance *b, const BalanceOptions *o) {
    for (int round = 1; round <= o->calibrate_rounds; round++) {
        BalanceFightHeroes(b, o, BALANCE_CALIBRATION_SEED_BASE);
        int in_band = BalanceInBandCount(b, o), at_limit = 0;
        for (int i = 0; i < b->hero_count; i++) {
            const BalanceHero *hero = &b->heroes[i];
            if (!BalanceIsGated(o, hero) || !BalanceFought(hero)) continue;
            int *pct = &b->budget.heroes[i].budget_pct;
            *pct = BalanceClampHandicap(*pct * expf(-BalanceCalibrationStep(round, BalanceWinPct(hero))));
        }
        if (!o->gated_count) {
            BalanceCentreHandicaps(b, false);
            BalanceCentreHandicaps(b, true);
        }
        for (int i = 0; i < b->hero_count; i++)
            at_limit += BalanceIsGated(o, &b->heroes[i]) && BalanceHandicapAtLimit(b->budget.heroes[i].budget_pct);
        printf(
            "Calibration round %d: %d of %d heroes in band before the step, %d at a handicap limit after it\n",
            round,
            in_band,
            BalanceFoughtCount(b, o),
            at_limit
        );
        fflush(stdout);
    }
    BalanceWriteHandicaps(b);
    printf("Balance: handicaps written to %s\n\n", STAT_BUDGET_HEROES_PATH);
}

static const char *BalanceClassName(Rarity rarity, int stars, bool giant) {
    if (giant) return TextFormat("%s giant", BALANCE_RARITY_NAMES[rarity]);
    if (stars > 0) return TextFormat("%s %d-star", BALANCE_RARITY_NAMES[rarity], stars);
    return BALANCE_RARITY_NAMES[rarity];
}

static void BalancePrintClass(const Balance *b, const BalanceOptions *o, Rarity rarity, int stars, bool giant) {
    static const BalanceHero *members[BALANCE_HERO_CAPACITY];
    static int handicaps[BALANCE_HERO_CAPACITY];
    int count = 0, in_band = 0;
    for (int i = 0; i < b->hero_count; i++) {
        const BalanceHero *hero = &b->heroes[i];
        if (!BalanceFought(hero) || hero->rarity != rarity || hero->giant != giant || (!giant && hero->stars != stars))
            continue;
        handicaps[count] = BalanceHandicap(b, hero);
        members[count++] = hero;
        in_band += BalanceInBand(o, hero);
    }
    if (!count) return;
    qsort(members, (size_t)count, sizeof(members[0]), BalanceByWinPct);
    qsort(handicaps, (size_t)count, sizeof(handicaps[0]), BalanceByInt);
    printf(
        "%-20s %6d %7.1f %7.1f %7.1f %8d %6d %6d %6d\n",
        BalanceClassName(rarity, stars, giant),
        count,
        BalanceWinPct(members[0]),
        BalanceWinPct(members[count / 2]),
        BalanceWinPct(members[count - 1]),
        in_band,
        handicaps[0],
        handicaps[count / 2],
        handicaps[count - 1]
    );
}

static void BalancePrintHero(const Balance *b, const BalanceHero *hero) {
    printf(
        "  %5.1f%%  %4d%%  %-5s  %-28s %-34s %s\n",
        BalanceWinPct(hero),
        BalanceHandicap(b, hero),
        hero->giant                                ? "giant"
        : BalanceBestRow(hero) == BALANCE_BACK_ROW ? "back"
                                                   : "front",
        hero->id,
        hero->name,
        BalanceClassName(hero->rarity, hero->stars, hero->giant)
    );
}

static float BalanceLopsidedMatchupPct(const Balance *b) {
    int lopsided = 0, matchups = 0;
    for (int i = 0; i < b->hero_count; i++) {
        const BalanceHero *hero = &b->heroes[i];
        const BalanceTally *by_squad = hero->matchups[BalanceBestRow(hero)];
        for (int q = 0; q < b->squad_count; q++) {
            if (!by_squad[q].battles) continue;
            float pct = BalanceTallyPct(by_squad[q]);
            lopsided += pct <= BALANCE_LOPSIDED_LOW_PCT || pct >= BALANCE_LOPSIDED_HIGH_PCT;
            matchups++;
        }
    }
    return matchups ? 100.0f * lopsided / matchups : 0.0f;
}

static const char *BalanceStatsName(const BalanceOptions *o) {
    if (o->raw) return "catalog stats";
    return o->neutral && !o->calibrate_rounds ? "stat budget without handicaps" : "stat budget with handicaps";
}

static void BalancePrintHeader(const Balance *b, const BalanceOptions *o) {
    printf(
        "Balance: %d heroes, %d %s squads, %d seeds, %s%s\n",
        b->hero_count,
        b->squad_count,
        o->holdout ? "holdout" : "reference",
        o->seeds,
        BalanceStatsName(o),
        o->stacks ? ", four copies of each hero" : ""
    );
}

static void BalancePrintReport(const Balance *b, const BalanceOptions *o) {
    BalancePrintHeader(b, o);
    printf("Mirror battles won by the attacking side: %.1f%%\n", BalanceTallyPct(b->mirrors));
    printf(
        "Hero-squad matchups won under %.0f%% or over %.0f%%: %.1f%%\n",
        BALANCE_LOPSIDED_LOW_PCT,
        BALANCE_LOPSIDED_HIGH_PCT,
        BalanceLopsidedMatchupPct(b)
    );
    printf(
        "\n%-20s %6s %7s %7s %7s %8s %6s %6s %6s\n",
        "class",
        "heroes",
        "min%",
        "median%",
        "max%",
        "in band",
        "min hc",
        "med hc",
        "max hc"
    );
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC; rarity++) {
        for (int stars = 0; stars < BALANCE_STAR_CAPACITY; stars++) BalancePrintClass(b, o, rarity, stars, false);
        BalancePrintClass(b, o, rarity, 0, true);
    }

    static const BalanceHero *ranked[BALANCE_HERO_CAPACITY];
    int count = 0, at_limit = 0;
    for (int i = 0; i < b->hero_count; i++) {
        if (!BalanceFought(&b->heroes[i])) continue;
        ranked[count++] = &b->heroes[i];
        at_limit += BalanceHandicapAtLimit(BalanceHandicap(b, &b->heroes[i]));
    }
    qsort(ranked, (size_t)count, sizeof(ranked[0]), BalanceByWinPct);
    printf(
        "\n%d of %d heroes win %.0f%%-%.0f%%; %d sit at a handicap limit (%d%% or %d%%)\n",
        BalanceInBandCount(b, o),
        count,
        o->band_low_pct,
        o->band_high_pct,
        at_limit,
        STAT_BUDGET_HERO_MIN_PCT,
        STAT_BUDGET_HERO_MAX_PCT
    );
    if (o->list_all) {
        printf("\nEvery hero (win rate, handicap, better row):\n");
        for (int i = 0; i < count; i++) BalancePrintHero(b, ranked[i]);
        return;
    }
    int shown = count < BALANCE_EXTREMES_SHOWN ? count : BALANCE_EXTREMES_SHOWN;
    printf("\nStrongest (win rate, handicap, better row):\n");
    for (int i = 0; i < shown; i++) BalancePrintHero(b, ranked[count - 1 - i]);
    printf("\nWeakest:\n");
    for (int i = 0; i < shown; i++) BalancePrintHero(b, ranked[i]);
}

static void
BalancePrintFactionMatrix(const char *title, const BalanceTally tallies[BALANCE_FACTION_COUNT][BALANCE_FACTION_COUNT]) {
    printf("\n%s:\n%-8s", title, "");
    for (int column = 0; column < BALANCE_FACTION_COUNT; column++) printf("%8s", BALANCE_FACTION_NAMES[column]);
    printf("\n");
    for (int row = 0; row < BALANCE_FACTION_COUNT; row++) {
        printf("%-8s", BALANCE_FACTION_NAMES[row]);
        for (int column = 0; column < BALANCE_FACTION_COUNT; column++) {
            if (tallies[row][column].battles)
                printf("%7.1f%%", BalanceTallyPct(tallies[row][column]));
            else
                printf("%8s", "-");
        }
        printf("\n");
    }
}

static int BalanceSquadByWinPct(const void *a, const void *b) {
    const BalanceTally *left = *(const BalanceTally *const *)a, *right = *(const BalanceTally *const *)b;
    float delta = BalanceTallyPct(*left) - BalanceTallyPct(*right);
    return (delta > 0) - (delta < 0);
}

static int BalanceBudgetSharePct(const Balance *b, const Character *ch) {
    if (!b->stat_budget) return STAT_BUDGET_NEUTRAL_HERO_PCT;
    int aura_points = StatBudgetAuraPoints(b->stat_budget, PassiveManifestFind(CharacterDataId(ch)));
    return StatBudgetHeroPct(b->stat_budget, ch->id) * 100 / StatBudgetAuraChargePct(b->stat_budget, aura_points);
}

static int BalanceSquadMeanBudgetShare(const Balance *b, const BattleRoster *squad) {
    int total = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) total += BalanceBudgetSharePct(b, &squad->chars[slot]);
    return total / SQUAD_SLOT_COUNT;
}

static void BalancePrintSquad(const Balance *b, const BalanceTally *tally) {
    int squad = (int)(tally - b->squad_tally);
    int faction = BalanceSquadFactionIndex(b, squad);
    printf(
        "  %5.1f%%  %4d%%  %-6s",
        BalanceTallyPct(*tally),
        BalanceSquadMeanBudgetShare(b, &b->squads[squad]),
        faction == BALANCE_NO_FACTION ? "mixed" : BALANCE_FACTION_NAMES[faction]
    );
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        printf("%s%s", slot ? ", " : " ", b->squads[squad].chars[slot].name);
    printf("\n");
}

static void BalancePrintSquadReport(const Balance *b, const BalanceOptions *o) {
    BalancePrintHeader(b, o);
    static const BalanceTally *ranked[BALANCE_SQUAD_CAPACITY];
    int in_band = 0;
    for (int q = 0; q < b->squad_count; q++) {
        ranked[q] = &b->squad_tally[q];
        float pct = BalanceTallyPct(b->squad_tally[q]);
        in_band += pct >= BALANCE_SQUAD_BAND_LOW_PCT && pct <= BALANCE_SQUAD_BAND_HIGH_PCT;
    }
    qsort(ranked, (size_t)b->squad_count, sizeof(ranked[0]), BalanceSquadByWinPct);
    printf(
        "Squad win rates against every other squad: min %.1f%%, median %.1f%%, max %.1f%%; %d of %d win "
        "%.0f%%-%.0f%%\n",
        BalanceTallyPct(*ranked[0]),
        BalanceTallyPct(*ranked[b->squad_count / 2]),
        BalanceTallyPct(*ranked[b->squad_count - 1]),
        in_band,
        b->squad_count,
        BALANCE_SQUAD_BAND_LOW_PCT,
        BALANCE_SQUAD_BAND_HIGH_PCT
    );
    if (o->list_all) {
        printf("\nEvery squad (win rate, mean share of the stat budget after handicap and aura cost, faction):\n");
        for (int i = 0; i < b->squad_count; i++) BalancePrintSquad(b, ranked[i]);
    } else {
        int shown = b->squad_count < BALANCE_SQUAD_EXTREMES_SHOWN ? b->squad_count : BALANCE_SQUAD_EXTREMES_SHOWN;
        printf("\nStrongest squads (win rate, mean share of the stat budget after handicap and aura cost, faction):\n");
        for (int i = 0; i < shown; i++) BalancePrintSquad(b, ranked[b->squad_count - 1 - i]);
        printf("\nWeakest squads:\n");
        for (int i = 0; i < shown; i++) BalancePrintSquad(b, ranked[i]);
    }

    BalancePrintFactionMatrix("Faction win rate, row against column", b->faction_tally);
}

static float BalanceSquadScore(Balance *b, const BattleRoster *squad, int seed_base) {
    BalanceTally tally = {0};
    for (int q = 0; q < b->squad_count; q++) {
        for (int seed = 1; seed <= b->seeds; seed++) {
            unsigned battle_seed = BalanceSeed(q, SQUAD_SLOT_COUNT, seed_base + seed);
            BalanceTallyAdd(&tally, BalanceWinsFromBothSides(squad, &b->squads[q], battle_seed), BALANCE_SIDES);
        }
    }
    return BalanceTallyPct(tally);
}

static int BalanceSearchRandom(Balance *b, int count) {
    b->search_rng ^= b->search_rng << 13;
    b->search_rng ^= b->search_rng >> 17;
    b->search_rng ^= b->search_rng << 5;
    return (int)(b->search_rng % (unsigned)count);
}

static bool BalanceSquadFieldsHero(const BattleRoster *squad, const char *id, int except_slot) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (slot != except_slot && !strcmp(squad->chars[slot].id, id)) return true;
    return false;
}

static bool
BalanceSearchHeroFits(const BattleRoster *squad, const BalanceHero *hero, int slot, int faction, bool unique) {
    if (hero->giant != (slot == SQUAD_SLOT_LARGE)) return false;
    if (!hero->giant && !(hero->faction_mask & (1u << faction))) return false;
    return !unique || !BalanceSquadFieldsHero(squad, hero->id, slot);
}

static void BalanceSearchDropDuplicates(Balance *b, BattleRoster *squad, int faction) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        while (BalanceSquadFieldsHero(squad, squad->chars[slot].id, slot)) {
            const BalanceHero *hero = &b->heroes[BalanceSearchRandom(b, b->hero_count)];
            if (BalanceSearchHeroFits(squad, hero, slot, faction, true)) BalanceRosterPlaceHero(squad, hero, slot);
        }
    }
}

static int BalanceStrongestSquadOfFaction(Balance *b, int faction, float *score) {
    int strongest = BALANCE_NO_FACTION;
    for (int q = 0; q < b->squad_count; q++) {
        if (BalanceSquadFactionIndex(b, q) != faction) continue;
        float squad_score = BalanceSquadScore(b, &b->squads[q], BALANCE_SEARCH_SEED_BASE);
        if (strongest != BALANCE_NO_FACTION && squad_score <= *score) continue;
        strongest = q;
        *score = squad_score;
    }
    return strongest;
}

static void BalancePrintSearchedSquad(const Balance *b, const BattleRoster *squad) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const Character *ch = &squad->chars[slot];
        int handicap = b->stat_budget ? StatBudgetHeroPct(b->stat_budget, ch->id) : STAT_BUDGET_NEUTRAL_HERO_PCT;
        printf("    %-34s %-28s %4d%%\n", ch->name, ch->id, handicap);
    }
}

static bool BalanceSearchFaction(Balance *b, const BalanceOptions *o, int faction, BattleRoster *searched) {
    static BattleRoster best, trial;
    float pooled_score = 0.0f;
    int strongest = BalanceStrongestSquadOfFaction(b, faction, &pooled_score);
    if (strongest == BALANCE_NO_FACTION) return false;
    BalanceRosterCopy(&best, &b->squads[strongest]);
    if (o->unique) BalanceSearchDropDuplicates(b, &best, faction);
    float best_score = BalanceSquadScore(b, &best, BALANCE_SEARCH_SEED_BASE);
    for (int attempt = 0; attempt < o->search_tries; attempt++) {
        int slot = BalanceSearchRandom(b, SQUAD_SLOT_COUNT);
        const BalanceHero *hero = &b->heroes[BalanceSearchRandom(b, b->hero_count)];
        if (!BalanceSearchHeroFits(&best, hero, slot, faction, o->unique)) continue;
        BalanceRosterCopy(&trial, &best);
        BalanceRosterPlaceHero(&trial, hero, slot);
        float score = BalanceSquadScore(b, &trial, BALANCE_SEARCH_SEED_BASE);
        if (score <= best_score) continue;
        BalanceRosterCopy(&best, &trial);
        best_score = score;
    }
    printf(
        "\n%s: strongest pooled squad wins %.1f%%, searched squad wins %.1f%% (%.1f%% on fresh seeds)\n",
        BALANCE_FACTION_NAMES[faction],
        pooled_score,
        best_score,
        BalanceSquadScore(b, &best, BALANCE_HOLDOUT_SEED_BASE)
    );
    BalancePrintSearchedSquad(b, &best);
    BalanceRosterCopy(searched, &best);
    return true;
}

static void BalanceFightSearchedSquads(
    Balance *b,
    const BattleRoster *searched,
    const bool *found,
    BalanceTally tallies[BALANCE_FACTION_COUNT][BALANCE_FACTION_COUNT]
) {
    for (int row = 0; row < BALANCE_FACTION_COUNT; row++) {
        for (int column = row + 1; column < BALANCE_FACTION_COUNT; column++) {
            if (!found[row] || !found[column]) continue;
            for (int seed = 1; seed <= b->seeds * BALANCE_SEARCH_DUEL_SEED_FACTOR; seed++) {
                unsigned battle_seed = BalanceSeed(row, column, BALANCE_HOLDOUT_SEED_BASE + seed);
                int wins = BalanceWinsFromBothSides(&searched[row], &searched[column], battle_seed);
                BalanceTallyAdd(&tallies[row][column], wins, BALANCE_SIDES);
                BalanceTallyAdd(&tallies[column][row], BALANCE_SIDES - wins, BALANCE_SIDES);
            }
        }
    }
}

static void BalanceSearch(Balance *b, const BalanceOptions *o) {
    BalancePrintHeader(b, o);
    printf(
        "Squad search: %d swaps tried per faction, %s\n",
        o->search_tries,
        o->unique ? "one copy of a hero per squad" : "copies allowed"
    );
    b->search_rng = BALANCE_SEARCH_RNG_SEED;
    static BattleRoster searched[BALANCE_FACTION_COUNT];
    static BalanceTally duels[BALANCE_FACTION_COUNT][BALANCE_FACTION_COUNT];
    bool found[BALANCE_FACTION_COUNT];
    for (int faction = 0; faction < BALANCE_FACTION_COUNT; faction++)
        found[faction] = BalanceSearchFaction(b, o, faction, &searched[faction]);
    BalanceFightSearchedSquads(b, searched, found, duels);
    BalancePrintFactionMatrix("Searched squads against each other, row against column", duels);
}

static int BalanceGate(const Balance *b, const BalanceOptions *o) {
    int status = 0;
    for (int g = 0; g < o->gated_count; g++) {
        const BalanceHero *hero = NULL;
        for (int i = 0; i < b->hero_count; i++)
            if (!strcmp(b->heroes[i].id, o->gated[g])) hero = &b->heroes[i];
        if (!hero || !BalanceFought(hero)) {
            printf("Balance: %s is not a playable hero with a reference squad of its faction\n", o->gated[g]);
            status = BALANCE_EXIT_OUT_OF_BAND;
            continue;
        }
        bool in_band = BalanceInBand(o, hero);
        printf(
            "Balance: %s wins %.1f%% of %d battles at a %d%% handicap, %s the %.0f%%-%.0f%% band\n",
            hero->id,
            BalanceWinPct(hero),
            BalanceBestTally(hero).battles,
            BalanceHandicap(b, hero),
            in_band ? "inside" : "outside",
            o->band_low_pct,
            o->band_high_pct
        );
        if (!in_band) status = BALANCE_EXIT_OUT_OF_BAND;
    }
    return status;
}

static bool BalanceParseOptions(BalanceOptions *o, int argc, char **argv) {
    *o = (BalanceOptions){
        .seeds = BALANCE_DEFAULT_SEEDS,
        .squads = BALANCE_DEFAULT_SQUADS,
        .band_low_pct = BALANCE_DEFAULT_BAND_LOW_PCT,
        .band_high_pct = BALANCE_DEFAULT_BAND_HIGH_PCT,
    };
    for (int i = 1; i < argc; i++) {
        int values_left = argc - i - 1;
        if (!strcmp(argv[i], "--raw")) {
            o->raw = true;
        } else if (!strcmp(argv[i], "--neutral")) {
            o->neutral = true;
        } else if (!strcmp(argv[i], "--holdout")) {
            o->holdout = true;
        } else if (!strcmp(argv[i], "--all")) {
            o->list_all = true;
        } else if (!strcmp(argv[i], "--stacks")) {
            o->stacks = true;
        } else if (!strcmp(argv[i], "--squad-report")) {
            o->squad_report = true;
        } else if (!strcmp(argv[i], "--unique")) {
            o->unique = true;
        } else if (!strcmp(argv[i], "--search") && values_left >= 1) {
            o->search_tries = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--nudge") && values_left >= 1) {
            o->nudge_pct = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--calibrate") && values_left >= 1) {
            o->calibrate_rounds = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--seeds") && values_left >= 1) {
            o->seeds = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--squads") && values_left >= 1) {
            o->squads = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--band") && values_left >= 2) {
            o->band_low_pct = (float)atof(argv[++i]);
            o->band_high_pct = (float)atof(argv[++i]);
        } else if (!strcmp(argv[i], "--hero") && values_left >= 1 && o->gated_count < BALANCE_GATED_HERO_CAPACITY) {
            o->gated[o->gated_count++] = argv[++i];
        } else {
            return false;
        }
    }
    bool one_mode = o->stacks + o->squad_report + (o->search_tries > 0) <= 1;
    bool calibration_fits = !o->calibrate_rounds || (!o->raw && !o->holdout && one_mode && !o->stacks &&
                                                     !o->squad_report && !o->search_tries && !o->nudge_pct);
    return o->seeds > 0 && o->seeds <= BALANCE_SEED_CAPACITY && o->squads > 0 && o->squads <= BALANCE_SQUAD_CAPACITY &&
           o->band_low_pct <= o->band_high_pct && o->calibrate_rounds >= 0 && calibration_fits &&
           !(o->raw && o->neutral) && one_mode && o->search_tries >= 0 && o->nudge_pct > -100 &&
           !(o->raw && o->nudge_pct);
}

int main(int argc, char **argv) {
    SetTraceLogLevel(LOG_WARNING);
    BalanceOptions options;
    if (!BalanceParseOptions(&options, argc, argv)) {
        printf("%s\n", BALANCE_USAGE);
        return BALANCE_EXIT_USAGE;
    }
    cJSON *catalog_root = BattleReplayCatalogLoad();
    static Balance balance;
    Balance *b = &balance;
    b->catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    b->seeds = options.seeds;
    BalanceLoadHeroes(b);
    BalanceLoadBudget(b, &options);
    BalanceLoadSquads(b, &options);
    int seed_base = options.holdout ? BALANCE_HOLDOUT_SEED_BASE : BALANCE_EVALUATION_SEED_BASE;

    int status = 0;
    if (options.squad_report) {
        BalanceFightSquads(b, options.seeds, seed_base);
        BalancePrintSquadReport(b, &options);
    } else if (options.search_tries) {
        BalanceSearch(b, &options);
    } else {
        if (options.calibrate_rounds) BalanceCalibrate(b, &options);
        BalanceFightHeroes(b, &options, seed_base);
        if (options.gated_count) {
            status = BalanceGate(b, &options);
        } else {
            BalanceFightMirrors(b, options.seeds, seed_base);
            BalancePrintReport(b, &options);
        }
    }
    cJSON_Delete(catalog_root);
    return status;
}
