#define main hod_game_main
#include "../source/main.c"
#undef main

#include "squad_meta.h"

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

#define BALANCE_USAGE                                                                                                  \
    "usage: balance_check [--raw | --neutral] [--holdout] [--seeds N] [--squads N] [--band LOW HIGH] [--all]\n"        \
    "                     [--stacks | --squad-report | --progress | --search TRIES | --adversarial ROUNDS "            \
    "[--swaps N]] [--unique]\n"                                                                                        \
    "                     [--nudge PCT] [--calibrate ROUNDS] [--hero UNIT_ID]...\n"                                    \
    "Seats every hero in reference squads of its faction, fights the same squad with a peer in that seat from both\n"  \
    "sides and reports the win rate in the hero's better row. SEEDS is the number of peers per seat.\n"                \
    "--raw fights with catalog stats, --neutral with the stat budget but without the per-hero handicaps.\n"            \
    "--holdout uses reference squads and peers the calibration never saw. --all lists every hero, weakest first.\n"    \
    "--stacks fields four copies of each hero against the unchanged squads.\n"                                         \
    "--squad-report fights the reference squads against each other and prints squad and faction win rates.\n"          \
    "--progress fights each reference squad against itself with one side's growth, rarity, fortify, rune or gear\n"    \
    "changed, and prints what that progress is worth: the strongest runes and items with the battles each fought,\n"   \
    "and the mean win rate of each rarity. --all lists every rune and item. Every other mode judges fully grown\n"     \
    "heroes at one rarity share.\n"                                                                                    \
    "--search climbs from each faction's strongest pooled squad by single swaps and prints the squad it ends on;\n"    \
    "--unique forbids two copies of a hero in it.\n"                                                                   \
    "--adversarial looks each round for the squads that best beat the current meta, adds them to the pool and\n"       \
    "solves the meta again: the mix of pooled squads that no pooled squad beats. Round 1's meta is the reference\n"    \
    "pool. Every faction and a squad across factions get one climb from the strongest pooled squad and one from a\n"   \
    "random squad, each of N swaps (default 2000). It prints the squads, factions and heroes the meta ends on.\n"      \
    "--nudge fights each hero with its own handicap raised by PCT percent, to show how steep the win rate is.\n"       \
    "--calibrate moves the handicaps toward an even win rate for ROUNDS rounds and writes them back.\n"                \
    "With --adversarial it instead runs ROUNDS passes of that search, keeping the squads found, and each pass "        \
    "lowers\n"                                                                                                         \
    "a hero's handicap by its share of the meta and lets it recover toward the handicap it started with.\n"            \
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
#define BALANCE_UNIT_ID_CAPACITY 64
#define BALANCE_NO_SQUAD -1
#define BALANCE_NO_SLOT -1
#define BALANCE_NO_WIN_SHARE -1.0f
#define BALANCE_REFERENCE_ROUND 0
#define BALANCE_DEFAULT_SWAPS 2000
#define BALANCE_ACROSS_FACTIONS BALANCE_FACTION_COUNT
#define BALANCE_CLIMB_TARGET_COUNT (BALANCE_FACTION_COUNT + 1)
#define BALANCE_CLIMBS_PER_TARGET 2
#define BALANCE_CLIMB_COUNT (BALANCE_CLIMB_TARGET_COUNT * BALANCE_CLIMBS_PER_TARGET)
#define BALANCE_KEPT_SQUAD_CAPACITY 96
#define BALANCE_JOB_CAPACITY 16
#define BALANCE_PAYOFF_JOB_COUNT 12
#define BALANCE_PAYOFF_ROWS_PER_JOB ((SQUAD_META_CAPACITY + BALANCE_PAYOFF_JOB_COUNT - 1) / BALANCE_PAYOFF_JOB_COUNT)
#define BALANCE_KEPT_ROUND -1
#define BALANCE_ADVERSARIAL_ROUND_LIMIT                                                                                \
    ((SQUAD_META_CAPACITY - BALANCE_SQUAD_CAPACITY - BALANCE_KEPT_SQUAD_CAPACITY) / BALANCE_CLIMB_COUNT)
#define BALANCE_DEMAND_GAIN 0.5
#define BALANCE_DEMAND_RECOVERY 0.1
#define BALANCE_ADVERSARIAL_SEED_BASE 900000
#define BALANCE_PAYOFF_SEED_BASE 1100000
#define BALANCE_PAYOFF_SEED_FACTOR 8
#define BALANCE_RIVAL_MIN_SHARE_OF_EVEN_WEIGHT 0.25
#define BALANCE_META_SHOWN_WEIGHT 0.01
#define BALANCE_META_HEROES_SHOWN 15
#define BALANCE_SEAT_SWAP_ONE_IN 5
#define BALANCE_PROGRESS_SEED_BASE 1300000
#define BALANCE_PROGRESS_LABEL_CAPACITY 96
#define BALANCE_PROGRESS_SHOWN 12
#define BALANCE_PROGRESS_STRONG_PCT 60.0f
#define BALANCE_PROGRESS_ROW_CAPACITY (GEAR_CATALOG_CAPACITY + RUNE_CATALOG_CAPACITY)
#define BALANCE_PROGRESS_RARITY_COUNT (RUNE_PRESTIGE_RARITY + 1)
#define BALANCE_GEAR_TRIAL_SLOT 0
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
    bool progress;
    bool unique;
    int search_tries;
    int adversarial_rounds;
    int swaps;
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

typedef struct {
    int squad;
    int seeds;
    double weight;
} BalanceRival;

typedef struct {
    SquadMeta meta;
    BattleRoster squads[SQUAD_META_CAPACITY];
    int found_in_round[SQUAD_META_CAPACITY];
    BalanceRival rivals[SQUAD_META_CAPACITY];
    int rival_count;
    int reference_count;
    int payoff_seeds;
} BalancePool;

typedef struct {
    int regular[BALANCE_HERO_CAPACITY], giants[BALANCE_HERO_CAPACITY];
    int regular_count, giant_count;
} BalanceBench;

typedef struct {
    char ids[SQUAD_SLOT_COUNT][BALANCE_UNIT_ID_CAPACITY];
    float win_share;
} BalanceClimbResult;

typedef struct {
    int index;
    double share;
} BalanceRanked;

typedef void (*BalanceJob)(void *context, int job, void *result);

typedef struct {
    Balance *b;
    const BalanceOptions *o;
    const BalancePool *pool;
    int round;
} BalanceClimbRun;

typedef struct {
    const BalancePool *pool;
    int first;
} BalancePayoffRun;

typedef struct {
    float win_share[BALANCE_PAYOFF_ROWS_PER_JOB][SQUAD_META_CAPACITY];
} BalancePayoffRows;

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
    for (int rarity = RARITY_COMMON; rarity < STAT_BUDGET_RARITY_COUNT && !o->progress; rarity++)
        b->budget.rarity_budget_pct[rarity] = STAT_BUDGET_NEUTRAL_RARITY_PCT;
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

static void BalanceRosterSeatUnit(const Balance *b, BattleRoster *roster, int slot, const char *id) {
    const cJSON *def = CatalogFindCharacter((cJSON *)b->catalog, id);
    if (!def) TraceLog(LOG_FATAL, "Balance: unit %s is not in the catalog", id);
    CharacterParse(def, &roster->chars[slot], &roster->abilities[slot]);
    BalanceMaxOut(&roster->chars[slot]);
    roster->present[slot] = true;
}

static void BalanceRosterFromSquad(Balance *b, BattleRoster *roster, const PvpSquad *squad) {
    memset(roster, 0, sizeof(*roster));
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) BalanceRosterSeatUnit(b, roster, slot, squad->cells[slot].id);
    roster->stat_budget = b->stat_budget;
}

static void BalanceRosterFromClimb(const Balance *b, BattleRoster *roster, const BalanceClimbResult *found) {
    memset(roster, 0, sizeof(*roster));
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) BalanceRosterSeatUnit(b, roster, slot, found->ids[slot]);
    roster->stat_budget = b->stat_budget;
}

static unsigned BalanceFormationFaction(const BattleRoster *roster) {
    unsigned shared = BALANCE_ANY_FACTION;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (roster->present[slot] && !roster->chars[slot].giant) shared &= SimFactionMask(roster->chars[slot].faction);
    return shared ? shared : BALANCE_ANY_FACTION;
}

static int BalanceFactionIndex(unsigned shared) {
    if (shared == BALANCE_ANY_FACTION) return BALANCE_NO_FACTION;
    for (int faction = 0; faction < BALANCE_FACTION_COUNT; faction++)
        if (shared & (1u << faction)) return faction;
    return BALANCE_NO_FACTION;
}

static int BalanceSquadFactionIndex(const Balance *b, int squad) {
    return BalanceFactionIndex(b->squad_faction[squad]);
}

static const char *BalanceFactionName(int faction) {
    return faction == BALANCE_NO_FACTION ? "mixed" : BALANCE_FACTION_NAMES[faction];
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

static void BalancePrintSquadNames(const BattleRoster *squad) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) printf("%s%s", slot ? "; " : " ", squad->chars[slot].name);
    printf("\n");
}

static void BalancePrintSquad(const Balance *b, const BalanceTally *tally) {
    int squad = (int)(tally - b->squad_tally);
    printf(
        "  %5.1f%%  %4d%%  %-6s",
        BalanceTallyPct(*tally),
        BalanceSquadMeanBudgetShare(b, &b->squads[squad]),
        BalanceFactionName(BalanceSquadFactionIndex(b, squad))
    );
    BalancePrintSquadNames(&b->squads[squad]);
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

#ifdef _WIN32
static void BalanceRunJobs(BalanceJob run, void *context, int jobs, void *results, size_t result_size) {
    for (int job = 0; job < jobs; job++) run(context, job, (char *)results + job * result_size);
}
#else
static bool BalanceReadWhole(int reader, void *out, size_t size) {
    size_t received = 0;
    while (received < size) {
        ssize_t part = read(reader, (char *)out + received, size - received);
        if (part <= 0) return false;
        received += (size_t)part;
    }
    return true;
}

static void BalanceRunJobs(BalanceJob run, void *context, int jobs, void *results, size_t result_size) {
    int readers[BALANCE_JOB_CAPACITY];
    pid_t workers[BALANCE_JOB_CAPACITY];
    fflush(stdout);
    for (int job = 0; job < jobs; job++) {
        void *result = (char *)results + job * result_size;
        int ends[2];
        if (pipe(ends) != 0) TraceLog(LOG_FATAL, "Balance: no pipe for job %d", job);
        workers[job] = fork();
        if (workers[job] < 0) TraceLog(LOG_FATAL, "Balance: could not start job %d", job);
        if (workers[job] == 0) {
            close(ends[0]);
            run(context, job, result);
            _exit(write(ends[1], result, result_size) == (ssize_t)result_size ? 0 : 1);
        }
        close(ends[1]);
        readers[job] = ends[0];
    }
    for (int job = 0; job < jobs; job++) {
        bool reported = BalanceReadWhole(readers[job], (char *)results + job * result_size, result_size);
        close(readers[job]);
        waitpid(workers[job], NULL, 0);
        if (!reported) TraceLog(LOG_FATAL, "Balance: job %d did not report", job);
    }
}
#endif

static float BalancePoolFight(const BalancePool *pool, int squad, int rival) {
    int wins = 0;
    for (int seed = 1; seed <= pool->payoff_seeds; seed++) {
        unsigned battle_seed = BalanceSeed(squad, rival, BALANCE_PAYOFF_SEED_BASE + seed);
        wins += BalanceWinsFromBothSides(&pool->squads[squad], &pool->squads[rival], battle_seed);
    }
    return (float)wins / (float)(BALANCE_SIDES * pool->payoff_seeds);
}

static void BalancePayoffJob(void *context, int job, void *result) {
    const BalancePayoffRun *run = context;
    BalancePayoffRows *rows = result;
    int row = 0;
    for (int squad = run->first + job; squad < run->pool->meta.count; squad += BALANCE_PAYOFF_JOB_COUNT, row++)
        for (int rival = 0; rival < squad; rival++)
            rows->win_share[row][rival] = BalancePoolFight(run->pool, squad, rival);
}

static void BalancePoolFightFrom(BalancePool *pool, int first) {
    static BalancePayoffRows rows[BALANCE_PAYOFF_JOB_COUNT];
    BalancePayoffRun run = {.pool = pool, .first = first};
    BalanceRunJobs(BalancePayoffJob, &run, BALANCE_PAYOFF_JOB_COUNT, rows, sizeof(rows[0]));
    for (int job = 0; job < BALANCE_PAYOFF_JOB_COUNT; job++) {
        int row = 0;
        for (int squad = first + job; squad < pool->meta.count; squad += BALANCE_PAYOFF_JOB_COUNT, row++)
            for (int rival = 0; rival < squad; rival++)
                SquadMetaRecord(&pool->meta, squad, rival, rows[job].win_share[row][rival]);
    }
}

static int BalancePoolAdd(BalancePool *pool, const BattleRoster *roster, int round) {
    int squad = SquadMetaAdd(&pool->meta);
    BalanceRosterCopy(&pool->squads[squad], roster);
    pool->found_in_round[squad] = round;
    return squad;
}

static bool BalanceRostersMatch(const BattleRoster *left, const BattleRoster *right) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (strcmp(left->chars[slot].id, right->chars[slot].id)) return false;
    return true;
}

static int BalancePoolFind(const BalancePool *pool, const BattleRoster *roster) {
    for (int squad = 0; squad < pool->meta.count; squad++)
        if (BalanceRostersMatch(&pool->squads[squad], roster)) return squad;
    return BALANCE_NO_SQUAD;
}

static int BalancePoolFactionIndex(const BalancePool *pool, int squad) {
    return BalanceFactionIndex(BalanceFormationFaction(&pool->squads[squad]));
}

static double BalancePoolReferenceShare(const BalancePool *pool, int squad) {
    double share = 0.0;
    for (int reference = 0; reference < pool->reference_count; reference++)
        share += pool->meta.win_share[squad][reference];
    return share / pool->reference_count;
}

static double BalanceMetaReferenceShare(const BalancePool *pool) {
    double share = 0.0;
    for (int squad = 0; squad < pool->meta.count; squad++)
        share += pool->meta.weight[squad] * BalancePoolReferenceShare(pool, squad);
    return share;
}

static int BalanceRankedByShare(const void *a, const void *b) {
    const BalanceRanked *left = a, *right = b;
    if (left->share != right->share) return left->share < right->share ? 1 : -1;
    return left->index - right->index;
}

static void BalancePoolPickRivals(BalancePool *pool, int seed_budget) {
    static BalanceRanked ranked[SQUAD_META_CAPACITY];
    double least = BALANCE_RIVAL_MIN_SHARE_OF_EVEN_WEIGHT / pool->meta.count, carried = 0.0;
    pool->rival_count = 0;
    for (int squad = 0; squad < pool->meta.count; squad++) {
        if (pool->meta.weight[squad] < least) continue;
        ranked[pool->rival_count++] = (BalanceRanked){.index = squad, .share = pool->meta.weight[squad]};
        carried += pool->meta.weight[squad];
    }
    qsort(ranked, (size_t)pool->rival_count, sizeof(ranked[0]), BalanceRankedByShare);
    for (int i = 0; i < pool->rival_count; i++) {
        double weight = ranked[i].share / carried;
        int seeds = (int)lround(weight * seed_budget);
        pool->rivals[i] = (BalanceRival){.squad = ranked[i].index, .seeds = seeds > 0 ? seeds : 1, .weight = weight};
    }
}

static float BalanceShareAgainstRivals(const BalancePool *pool, const BattleRoster *squad, float share_to_beat) {
    double lost = 0.0;
    for (int i = 0; i < pool->rival_count; i++) {
        const BalanceRival *rival = &pool->rivals[i];
        for (int seed = 1; seed <= rival->seeds; seed++) {
            unsigned battle_seed = BalanceSeed(rival->squad, SQUAD_SLOT_COUNT, BALANCE_ADVERSARIAL_SEED_BASE + seed);
            int wins = BalanceWinsFromBothSides(squad, &pool->squads[rival->squad], battle_seed);
            lost += rival->weight * (BALANCE_SIDES - wins) / (BALANCE_SIDES * rival->seeds);
            if (1.0 - lost <= share_to_beat) return (float)(1.0 - lost);
        }
    }
    return (float)(1.0 - lost);
}

static unsigned BalanceTargetFactions(int target) {
    return target == BALANCE_ACROSS_FACTIONS ? BALANCE_ANY_FACTION : 1u << target;
}

static const char *BalanceTargetName(int target) {
    return target == BALANCE_ACROSS_FACTIONS ? "any" : BALANCE_FACTION_NAMES[target];
}

static void BalanceBenchFill(const Balance *b, unsigned factions, BalanceBench *bench) {
    bench->regular_count = bench->giant_count = 0;
    for (int i = 0; i < b->hero_count; i++) {
        const BalanceHero *hero = &b->heroes[i];
        if (hero->giant)
            bench->giants[bench->giant_count++] = i;
        else if (hero->faction_mask & factions)
            bench->regular[bench->regular_count++] = i;
    }
}

static bool
BalanceBenchSeat(Balance *b, const BalanceOptions *o, const BalanceBench *bench, BattleRoster *squad, int slot) {
    bool large = slot == SQUAD_SLOT_LARGE;
    int pick = BalanceSearchRandom(b, large ? bench->giant_count : bench->regular_count);
    const BalanceHero *hero = &b->heroes[large ? bench->giants[pick] : bench->regular[pick]];
    if (!strcmp(squad->chars[slot].id, hero->id)) return false;
    if (o->unique && BalanceSquadFieldsHero(squad, hero->id, slot)) return false;
    BalanceRosterPlaceHero(squad, hero, slot);
    return true;
}

static void BalanceRosterSwapSeats(BattleRoster *roster, int first, int second) {
    static Character ch;
    static Ability ability;
    ch = roster->chars[first];
    ability = roster->abilities[first];
    roster->chars[first] = roster->chars[second];
    roster->abilities[first] = roster->abilities[second];
    roster->chars[second] = ch;
    roster->abilities[second] = ability;
    roster->chars[first].ability = &roster->abilities[first];
    roster->chars[second].ability = &roster->abilities[second];
}

static bool BalanceClimbMove(Balance *b, const BalanceOptions *o, const BalanceBench *bench, BattleRoster *squad) {
    if (BalanceSearchRandom(b, BALANCE_SEAT_SWAP_ONE_IN))
        return BalanceBenchSeat(b, o, bench, squad, BalanceSearchRandom(b, SQUAD_SLOT_COUNT));
    int first = BalanceSearchRandom(b, SQUAD_SLOT_LARGE);
    int second = (first + 1 + BalanceSearchRandom(b, SQUAD_SLOT_LARGE - 1)) % SQUAD_SLOT_LARGE;
    if (!strcmp(squad->chars[first].id, squad->chars[second].id)) return false;
    BalanceRosterSwapSeats(squad, first, second);
    return true;
}

static int BalancePoolStrongest(const BalancePool *pool, int target) {
    int strongest = BALANCE_NO_SQUAD;
    double strongest_share = 0.0;
    for (int squad = 0; squad < pool->meta.count; squad++) {
        if (target != BALANCE_ACROSS_FACTIONS && BalancePoolFactionIndex(pool, squad) != target) continue;
        double share = SquadMetaWinShare(&pool->meta, squad);
        if (strongest != BALANCE_NO_SQUAD && share <= strongest_share) continue;
        strongest = squad;
        strongest_share = share;
    }
    return strongest;
}

static void BalanceClimbStart(
    Balance *b,
    const BalanceOptions *o,
    const BalancePool *pool,
    const BalanceBench *bench,
    int pooled,
    BattleRoster *squad
) {
    if (pooled == BALANCE_NO_SQUAD) {
        memset(squad, 0, sizeof(*squad));
        squad->stat_budget = b->stat_budget;
    } else {
        BalanceRosterCopy(squad, &pool->squads[pooled]);
    }
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        squad->present[slot] = true;
        bool seated = pooled != BALANCE_NO_SQUAD;
        bool copied = o->unique && BalanceSquadFieldsHero(squad, squad->chars[slot].id, slot);
        while (!seated || copied) {
            seated = BalanceBenchSeat(b, o, bench, squad, slot);
            copied = false;
        }
    }
}

static unsigned BalanceClimbRngSeed(int round, int climb) {
    unsigned seed =
        BALANCE_SEARCH_RNG_SEED + (unsigned)(round * BALANCE_CLIMB_COUNT + climb) * BALANCE_PEER_PICK_MULTIPLIER;
    return seed ? seed : BALANCE_SEARCH_RNG_SEED;
}

static void BalanceClimb(
    Balance *b, const BalanceOptions *o, const BalancePool *pool, int round, int climb, BalanceClimbResult *found
) {
    static BalanceBench bench;
    static BattleRoster best, trial;
    int target = climb / BALANCE_CLIMBS_PER_TARGET;
    bool from_pool = climb % BALANCE_CLIMBS_PER_TARGET == 0;
    memset(found, 0, sizeof(*found));
    found->win_share = BALANCE_NO_WIN_SHARE;
    BalanceBenchFill(b, BalanceTargetFactions(target), &bench);
    if (bench.regular_count < SQUAD_SLOT_LARGE || !bench.giant_count) return;
    b->search_rng = BalanceClimbRngSeed(round, climb);
    BalanceClimbStart(b, o, pool, &bench, from_pool ? BalancePoolStrongest(pool, target) : BALANCE_NO_SQUAD, &best);
    float best_share = BalanceShareAgainstRivals(pool, &best, BALANCE_NO_WIN_SHARE);
    for (int swap = 0; swap < o->swaps; swap++) {
        BalanceRosterCopy(&trial, &best);
        if (!BalanceClimbMove(b, o, &bench, &trial)) continue;
        float share = BalanceShareAgainstRivals(pool, &trial, best_share);
        if (share <= best_share) continue;
        BalanceRosterCopy(&best, &trial);
        best_share = share;
    }
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        StrCopy(found->ids[slot], sizeof(found->ids[slot]), best.chars[slot].id);
    found->win_share = best_share;
}

static void BalanceClimbJob(void *context, int job, void *result) {
    const BalanceClimbRun *run = context;
    BalanceClimb(run->b, run->o, run->pool, run->round, job, result);
}

static int BalanceMetaShownSquads(const BalancePool *pool) {
    int shown = 0;
    for (int squad = 0; squad < pool->meta.count; squad++)
        shown += pool->meta.weight[squad] >= BALANCE_META_SHOWN_WEIGHT;
    return shown;
}

static void BalancePoolSearch(Balance *b, const BalanceOptions *o, BalancePool *pool, int round, double *best) {
    static BalanceClimbResult found[BALANCE_CLIMB_COUNT];
    static BattleRoster admitted;
    int admitted_as[BALANCE_CLIMB_COUNT], pooled = pool->meta.count;
    BalanceClimbRun run = {.b = b, .o = o, .pool = pool, .round = round};
    BalancePoolPickRivals(pool, o->seeds * pool->reference_count);
    BalanceRunJobs(BalanceClimbJob, &run, BALANCE_CLIMB_COUNT, found, sizeof(found[0]));
    for (int climb = 0; climb < BALANCE_CLIMB_COUNT; climb++) {
        admitted_as[climb] = BALANCE_NO_SQUAD;
        if (found[climb].win_share < 0.0f) continue;
        BalanceRosterFromClimb(b, &admitted, &found[climb]);
        admitted_as[climb] = BalancePoolFind(pool, &admitted);
        if (admitted_as[climb] == BALANCE_NO_SQUAD) admitted_as[climb] = BalancePoolAdd(pool, &admitted, round);
    }
    BalancePoolFightFrom(pool, pooled);
    for (int target = 0; target < BALANCE_CLIMB_TARGET_COUNT; target++) best[target] = BALANCE_NO_WIN_SHARE;
    for (int climb = 0; climb < BALANCE_CLIMB_COUNT; climb++) {
        if (admitted_as[climb] == BALANCE_NO_SQUAD) continue;
        double share = SquadMetaWinShare(&pool->meta, admitted_as[climb]);
        double *target_best = &best[climb / BALANCE_CLIMBS_PER_TARGET];
        if (share > *target_best) *target_best = share;
    }
}

static void BalanceAdversarialRound(Balance *b, const BalanceOptions *o, BalancePool *pool, int round) {
    double best[BALANCE_CLIMB_TARGET_COUNT];
    int meta_squads = BalanceMetaShownSquads(pool);
    BalancePoolSearch(b, o, pool, round, best);
    printf("%5d %12d", round, meta_squads);
    for (int target = 0; target < BALANCE_CLIMB_TARGET_COUNT; target++) {
        if (best[target] < 0.0)
            printf("%8s", "-");
        else
            printf("%7.1f%%", 100.0 * best[target]);
    }
    printf("\n");
    fflush(stdout);
}

static void BalancePrintMetaSquad(const BalancePool *pool, int squad) {
    int round = pool->found_in_round[squad];
    printf(
        "  %5.1f%%  %-9s  %-6s  %5.1f%% ",
        100.0 * pool->meta.weight[squad],
        round == BALANCE_REFERENCE_ROUND ? "reference"
        : round == BALANCE_KEPT_ROUND    ? "kept"
                                         : TextFormat("round %d", round),
        BalanceFactionName(BalancePoolFactionIndex(pool, squad)),
        100.0 * BalancePoolReferenceShare(pool, squad)
    );
    BalancePrintSquadNames(&pool->squads[squad]);
}

static void BalancePrintStrongestFound(const BalancePool *pool) {
    int strongest = BALANCE_NO_SQUAD;
    for (int squad = pool->reference_count; squad < pool->meta.count; squad++) {
        bool stronger = strongest == BALANCE_NO_SQUAD ||
                        BalancePoolReferenceShare(pool, squad) > BalancePoolReferenceShare(pool, strongest);
        if (stronger) strongest = squad;
    }
    if (strongest == BALANCE_NO_SQUAD) return;
    printf(
        "\nStrongest squad found against the reference pool: %.1f%%  %s ",
        100.0 * BalancePoolReferenceShare(pool, strongest),
        BalanceFactionName(BalancePoolFactionIndex(pool, strongest))
    );
    BalancePrintSquadNames(&pool->squads[strongest]);
}

static void BalancePrintMetaSquads(const BalancePool *pool, const BalanceOptions *o) {
    static BalanceRanked ranked[SQUAD_META_CAPACITY];
    double least = o->list_all ? 0.0 : BALANCE_META_SHOWN_WEIGHT;
    for (int squad = 0; squad < pool->meta.count; squad++)
        ranked[squad] = (BalanceRanked){.index = squad, .share = pool->meta.weight[squad]};
    qsort(ranked, (size_t)pool->meta.count, sizeof(ranked[0]), BalanceRankedByShare);
    printf(
        "\nMeta after round %d: %d of %d pooled squads carry %.0f%% or more. It wins %.1f%% against the reference "
        "pool, and no pooled squad wins more than %.1f%% against it.\n",
        o->adversarial_rounds,
        BalanceMetaShownSquads(pool),
        pool->meta.count,
        100.0 * BALANCE_META_SHOWN_WEIGHT,
        100.0 * BalanceMetaReferenceShare(pool),
        100.0 * SquadMetaBestWinShare(&pool->meta)
    );
    printf("\nMeta squads (weight, found in, faction, win rate against the reference pool):\n");
    for (int i = 0; i < pool->meta.count; i++)
        if (ranked[i].share > 0.0 && ranked[i].share >= least) BalancePrintMetaSquad(pool, ranked[i].index);
}

static void BalancePrintMetaFactions(const BalancePool *pool) {
    double share[BALANCE_FACTION_COUNT] = {0}, mixed = 0.0;
    for (int squad = 0; squad < pool->meta.count; squad++) {
        int faction = BalancePoolFactionIndex(pool, squad);
        if (faction == BALANCE_NO_FACTION)
            mixed += pool->meta.weight[squad];
        else
            share[faction] += pool->meta.weight[squad];
    }
    printf("\nFaction share of the meta:");
    for (int faction = 0; faction < BALANCE_FACTION_COUNT; faction++)
        printf(" %s %.1f%%,", BALANCE_FACTION_NAMES[faction], 100.0 * share[faction]);
    printf(" mixed %.1f%%\n", 100.0 * mixed);
}

static double BalanceMetaHeroShare(const BalancePool *pool, const char *id) {
    double share = 0.0;
    for (int squad = 0; squad < pool->meta.count; squad++)
        if (BalanceSquadFieldsHero(&pool->squads[squad], id, BALANCE_NO_SLOT)) share += pool->meta.weight[squad];
    return share;
}

static void BalancePrintMetaHeroes(const Balance *b, const BalanceOptions *o, const BalancePool *pool) {
    static BalanceRanked ranked[BALANCE_HERO_CAPACITY];
    int fielded = 0;
    for (int i = 0; i < b->hero_count; i++) {
        ranked[i] = (BalanceRanked){.index = i, .share = BalanceMetaHeroShare(pool, b->heroes[i].id)};
        fielded += ranked[i].share >= BALANCE_META_SHOWN_WEIGHT;
    }
    qsort(ranked, (size_t)b->hero_count, sizeof(ranked[0]), BalanceRankedByShare);
    int shown = o->list_all || fielded < BALANCE_META_HEROES_SHOWN ? fielded : BALANCE_META_HEROES_SHOWN;
    printf(
        "\n%d of %d heroes are in %.0f%% or more of the meta. Most fielded (share of the meta, handicap):\n",
        fielded,
        b->hero_count,
        100.0 * BALANCE_META_SHOWN_WEIGHT
    );
    for (int i = 0; i < shown; i++) {
        const BalanceHero *hero = &b->heroes[ranked[i].index];
        printf(
            "  %5.1f%%  %4d%%  %-28s %-34s %s\n",
            100.0 * ranked[i].share,
            BalanceHandicap(b, hero),
            hero->id,
            hero->name,
            BalanceClassName(hero->rarity, hero->stars, hero->giant)
        );
    }
}

static void BalancePoolOpen(
    const Balance *b, const BalanceOptions *o, BalancePool *pool, const BattleRoster *kept, int kept_count
) {
    pool->meta.count = 0;
    pool->payoff_seeds = o->seeds * BALANCE_PAYOFF_SEED_FACTOR;
    pool->reference_count = b->squad_count;
    for (int q = 0; q < b->squad_count; q++) BalancePoolAdd(pool, &b->squads[q], BALANCE_REFERENCE_ROUND);
    for (int i = 0; i < kept_count; i++) BalancePoolAdd(pool, &kept[i], BALANCE_KEPT_ROUND);
    BalancePoolFightFrom(pool, 0);
    SquadMetaWeighEvenly(&pool->meta, pool->reference_count);
    if (kept_count) SquadMetaSolve(&pool->meta);
}

static int BalancePoolKeepFound(const BalancePool *pool, BattleRoster *kept) {
    static BalanceRanked ranked[SQUAD_META_CAPACITY];
    int found = 0, count = 0;
    for (int squad = pool->reference_count; squad < pool->meta.count; squad++)
        ranked[found++] = (BalanceRanked){.index = squad, .share = pool->meta.weight[squad]};
    qsort(ranked, (size_t)found, sizeof(ranked[0]), BalanceRankedByShare);
    while (count < found && count < BALANCE_KEPT_SQUAD_CAPACITY && ranked[count].share > 0.0) {
        BalanceRosterCopy(&kept[count], &pool->squads[ranked[count].index]);
        count++;
    }
    return count;
}

static int BalanceDemandStep(Balance *b, const BalancePool *pool, const int *settled, int *fielded, double *most) {
    int lowered = 0;
    *fielded = 0;
    *most = 0.0;
    for (int i = 0; i < b->hero_count; i++) {
        double share = BalanceMetaHeroShare(pool, b->heroes[i].id);
        int *pct = &b->budget.heroes[i].budget_pct;
        double recovered = *pct * pow((double)settled[i] / *pct, BALANCE_DEMAND_RECOVERY);
        int stepped = BalanceClampHandicap((float)(recovered * exp(-BALANCE_DEMAND_GAIN * share)));
        lowered += stepped < *pct;
        *fielded += share >= BALANCE_META_SHOWN_WEIGHT;
        if (share > *most) *most = share;
        *pct = stepped;
    }
    return lowered;
}

static void BalanceDemandPass(Balance *b, const BalanceOptions *o, BalancePool *pool, int pass, const int *settled) {
    static BattleRoster kept[BALANCE_KEPT_SQUAD_CAPACITY];
    static int kept_count;
    double best[BALANCE_CLIMB_TARGET_COUNT], most = 0.0;
    int fielded = 0;
    BalancePoolOpen(b, o, pool, kept, kept_count);
    for (int round = 1; round <= o->adversarial_rounds; round++) {
        BalancePoolSearch(b, o, pool, round, best);
        SquadMetaSolve(&pool->meta);
    }
    kept_count = BalancePoolKeepFound(pool, kept);
    double against_reference = BalanceMetaReferenceShare(pool);
    int meta_squads = BalanceMetaShownSquads(pool);
    int lowered = BalanceDemandStep(b, pool, settled, &fielded, &most);
    printf(
        "%4d %12.1f%% %11d %11d %10.1f%% %7d\n",
        pass,
        100.0 * against_reference,
        meta_squads,
        fielded,
        100.0 * most,
        lowered
    );
    fflush(stdout);
}

static void BalanceDemandCalibrate(Balance *b, const BalanceOptions *o, BalancePool *pool) {
    static int settled[BALANCE_HERO_CAPACITY];
    for (int i = 0; i < b->hero_count; i++) settled[i] = b->budget.heroes[i].budget_pct;
    printf(
        "Calibration against the meta: %d passes of %d search rounds. Each pass lowers a hero's handicap by its "
        "share of the meta\nand lets it recover toward the handicap it started with. The table written is the "
        "mean of the last %d passes.\n",
        o->calibrate_rounds,
        o->adversarial_rounds,
        o->calibrate_rounds - o->calibrate_rounds / 2
    );
    printf(
        "%4s %13s %11s %11s %11s %7s\n", "pass", "vs reference", "meta squads", "meta heroes", "top hero", "lowered"
    );
    static double log_sum[BALANCE_HERO_CAPACITY];
    int averaged_passes = o->calibrate_rounds - o->calibrate_rounds / 2;
    for (int i = 0; i < b->hero_count; i++) log_sum[i] = 0.0;
    for (int pass = 1; pass <= o->calibrate_rounds; pass++) {
        BalanceDemandPass(b, o, pool, pass, settled);
        if (pass <= o->calibrate_rounds - averaged_passes) continue;
        for (int i = 0; i < b->hero_count; i++) log_sum[i] += log((double)b->budget.heroes[i].budget_pct);
    }
    for (int i = 0; i < b->hero_count; i++)
        b->budget.heroes[i].budget_pct = BalanceClampHandicap((float)exp(log_sum[i] / averaged_passes));
    BalanceWriteHandicaps(b);
    printf("Balance: handicaps written to %s\n\n", STAT_BUDGET_HEROES_PATH);
}

static void BalanceAdversarial(Balance *b, const BalanceOptions *o, BalancePool *pool) {
    BalancePrintHeader(b, o);
    printf(
        "Adversarial search: %d rounds of %d climbs, %d swaps a climb, %s\n",
        o->adversarial_rounds,
        BALANCE_CLIMB_COUNT,
        o->swaps,
        o->unique ? "one copy of a hero per squad" : "copies allowed"
    );
    BalancePoolOpen(b, o, pool, NULL, 0);
    printf(
        "\nBest squad found each round against that round's meta, on fresh seeds. Round 1's meta is the reference "
        "pool.\n"
    );
    printf("%5s %12s", "round", "meta squads");
    for (int target = 0; target < BALANCE_CLIMB_TARGET_COUNT; target++) printf("%8s", BalanceTargetName(target));
    printf("\n");
    for (int round = 1; round <= o->adversarial_rounds; round++) {
        BalanceAdversarialRound(b, o, pool, round);
        SquadMetaSolve(&pool->meta);
    }
    BalancePrintStrongestFound(pool);
    BalancePrintMetaSquads(pool, o);
    BalancePrintMetaFactions(pool);
    BalancePrintMetaHeroes(b, o, pool);
}

typedef struct {
    char label[BALANCE_PROGRESS_LABEL_CAPACITY];
    int rarity;
    BalanceTally tally;
    long long changed_cost, unchanged_cost;
} BalanceProgressRow;

typedef bool (*BalanceProgressChange)(
    const Balance *b, BattleRoster *changed, BattleRoster *unchanged, const void *detail
);

typedef struct {
    const char *label;
    bool top_tier;
    int level_pct;
    bool ascended;
} BalanceGrowthStage;

typedef struct {
    Rarity rarity;
    bool top_tier;
    int slots;
} BalanceFortifyStage;

typedef struct {
    int def;
    bool every_hero;
} BalanceGearTrial;

static const BalanceGrowthStage BALANCE_GROWTH_STAGES[] = {
    {"first tier, level 1", false, 0, false},
    {"first tier, level cap", false, 100, false},
    {"top tier, level 1", true, 0, false},
    {"top tier, half its levels", true, 50, false},
    {"top tier, level cap", true, 100, false},
    {"top tier, ascended to the raised cap", true, 100, true},
};

static const BalanceFortifyStage BALANCE_FORTIFY_STAGES[] = {
    {RARITY_COMMON, false, FORTIFY_SLOT_COUNT / 2},
    {RARITY_COMMON, false, FORTIFY_SLOT_COUNT},
    {RARITY_RARE, true, FORTIFY_SLOT_COUNT / 2},
    {RARITY_RARE, true, FORTIFY_SLOT_COUNT},
    {RARITY_LEGENDARY, true, FORTIFY_SLOT_COUNT / 2},
    {RARITY_LEGENDARY, true, FORTIFY_SLOT_COUNT},
};

static long long BalanceRosterCost(const Balance *b, const BattleRoster *roster) {
    long long cost = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        if (!roster->present[slot]) continue;
        const Character *ch = &roster->chars[slot];
        int power = CharacterStatPower(ch), health = CharacterStatHealth(ch);
        if (b->stat_budget) StatBudgetApply(b->stat_budget, ch, &power, &health);
        cost += (long long)power * StatBudgetEnsure()->health_per_power + health;
    }
    return cost;
}

static void BalanceProgressMeasure(
    Balance *b, BalanceProgressRow *row, int row_seed, BalanceProgressChange change, const void *detail
) {
    static BattleRoster changed, unchanged;
    for (int squad = 0; squad < b->squad_count; squad++) {
        BalanceRosterCopy(&changed, &b->squads[squad]);
        BalanceRosterCopy(&unchanged, &b->squads[squad]);
        if (!change(b, &changed, &unchanged, detail)) continue;
        row->changed_cost += BalanceRosterCost(b, &changed);
        row->unchanged_cost += BalanceRosterCost(b, &unchanged);
        for (int seed = 0; seed < b->seeds; seed++) {
            unsigned battle_seed = BalanceSeed(squad, row_seed, BALANCE_PROGRESS_SEED_BASE + seed);
            BalanceTallyAdd(&row->tally, BalanceWinsFromBothSides(&changed, &unchanged, battle_seed), BALANCE_SIDES);
        }
    }
}

static void BalancePrintProgressRow(const BalanceProgressRow *row) {
    int stat_share = row->unchanged_cost ? (int)(row->changed_cost * 100 / row->unchanged_cost) : 100;
    printf("  %-44s %5d%% %8.1f%%\n", row->label, stat_share, BalanceTallyPct(row->tally));
}

static Tier BalanceFirstTier(const Character *ch) {
    for (int tier = 0; tier < TIER_COUNT; tier++)
        if (ch->has_tier[tier]) return (Tier)tier;
    return ch->tier;
}

static bool BalanceGrowTo(const Balance *b, BattleRoster *changed, BattleRoster *unchanged, const void *detail) {
    (void)b;
    (void)unchanged;
    const BalanceGrowthStage *stage = detail;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        Character *ch = &changed->chars[slot];
        if (stage->ascended) GearApplyAscension(ch);
        ch->tier = stage->top_tier ? ch->max_tier : BalanceFirstTier(ch);
        ch->level = 1 + (ch->stats[ch->tier].max_level - 1) * stage->level_pct / 100;
    }
    return true;
}

static bool BalanceRankAs(const Balance *b, BattleRoster *changed, BattleRoster *unchanged, const void *detail) {
    (void)b;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        changed->chars[slot].rarity = *(const Rarity *)detail;
        unchanged->chars[slot].rarity = RARITY_LEGENDARY;
    }
    return true;
}

static FortifyChild BalanceFortifyChild(const Balance *b, const BalanceFortifyStage *stage) {
    static Character parsed;
    static Ability ability;
    FortifyChild child = {0};
    for (int i = 0; i < b->hero_count && !child.id[0]; i++) {
        const BalanceHero *hero = &b->heroes[i];
        if (hero->rarity != stage->rarity || hero->giant || !FortifyHeroFind(hero->id)) continue;
        CharacterParse(hero->def, &parsed, &ability);
        child.tier = stage->top_tier ? parsed.max_tier : BalanceFirstTier(&parsed);
        StrCopy(child.id, sizeof(child.id), hero->id);
    }
    return child;
}

static bool BalanceFortifyWith(const Balance *b, BattleRoster *changed, BattleRoster *unchanged, const void *detail) {
    (void)unchanged;
    const BalanceFortifyStage *stage = detail;
    FortifyChild child = BalanceFortifyChild(b, stage);
    if (!child.id[0]) return false;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        for (int filled = 0; filled < stage->slots; filled++) changed->chars[slot].fortify[filled] = child;
    return true;
}

static bool BalanceCarryRune(const Balance *b, BattleRoster *changed, BattleRoster *unchanged, const void *detail) {
    (void)b;
    (void)unchanged;
    changed->rune_defs[0] = *(const int *)detail;
    changed->rune_def_count = 1;
    return true;
}

static bool BalanceWearItem(const Balance *b, BattleRoster *changed, BattleRoster *unchanged, const void *detail) {
    (void)b;
    (void)unchanged;
    const BalanceGearTrial *trial = detail;
    int wearers = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT && (trial->every_hero || !wearers); slot++) {
        Character *ch = &changed->chars[slot];
        if (!changed->present[slot] || !GearUnitCanWear(ch, GearDefAt(trial->def))) continue;
        ch->gear.defs[BALANCE_GEAR_TRIAL_SLOT] = (short)(trial->def + 1);
        wearers++;
    }
    return wearers > 0;
}

static void BalancePrintProgressHeader(const char *title, const char *changed, const char *unchanged) {
    printf("\n%s: %s, against %s\n", title, changed, unchanged);
    printf("  %-44s %6s %9s\n", "", "stats", "win rate");
}

static void BalanceProgressGrowth(Balance *b) {
    BalancePrintProgressHeader("Growth", "every hero at that stage", "the same squad fully grown");
    for (size_t stage = 0; stage < sizeof(BALANCE_GROWTH_STAGES) / sizeof(BALANCE_GROWTH_STAGES[0]); stage++) {
        BalanceProgressRow row = {0};
        StrCopy(row.label, sizeof(row.label), BALANCE_GROWTH_STAGES[stage].label);
        BalanceProgressMeasure(b, &row, (int)stage, BalanceGrowTo, &BALANCE_GROWTH_STAGES[stage]);
        BalancePrintProgressRow(&row);
    }
}

static void BalanceProgressRarity(Balance *b) {
    BalancePrintProgressHeader("Rarity", "every hero counted as that rarity", "the same squad counted as Legendary");
    for (int rarity = RARITY_COMMON; rarity < STAT_BUDGET_RARITY_COUNT; rarity++) {
        BalanceProgressRow row = {0};
        Rarity counted_as = (Rarity)rarity;
        StrCopy(row.label, sizeof(row.label), BALANCE_RARITY_NAMES[rarity]);
        BalanceProgressMeasure(b, &row, rarity, BalanceRankAs, &counted_as);
        BalancePrintProgressRow(&row);
    }
}

static void BalanceProgressFortify(Balance *b) {
    BalancePrintProgressHeader("Fortify", "every hero fortified", "the same squad bare");
    for (size_t i = 0; i < sizeof(BALANCE_FORTIFY_STAGES) / sizeof(BALANCE_FORTIFY_STAGES[0]); i++) {
        const BalanceFortifyStage *stage = &BALANCE_FORTIFY_STAGES[i];
        BalanceProgressRow row = {0};
        snprintf(
            row.label,
            sizeof(row.label),
            "%d slots of a %s at its %s tier",
            stage->slots,
            BALANCE_RARITY_NAMES[stage->rarity],
            stage->top_tier ? "top" : "first"
        );
        BalanceProgressMeasure(b, &row, (int)i, BalanceFortifyWith, stage);
        BalancePrintProgressRow(&row);
    }
}

static int BalanceProgressByWinPct(const void *a, const void *b) {
    float left = BalanceTallyPct(((const BalanceProgressRow *)a)->tally);
    float right = BalanceTallyPct(((const BalanceProgressRow *)b)->tally);
    return (left < right) - (left > right);
}

static int BalanceProgressStrongCount(const BalanceProgressRow *rows, int count) {
    int strong = 0;
    for (int i = 0; i < count; i++) strong += BalanceTallyPct(rows[i].tally) > BALANCE_PROGRESS_STRONG_PCT;
    return strong;
}

static void BalancePrintProgressByRarity(const BalanceProgressRow *rows, int count) {
    double win_pct[BALANCE_PROGRESS_RARITY_COUNT] = {0};
    int ranked[BALANCE_PROGRESS_RARITY_COUNT] = {0};
    for (int i = 0; i < count; i++) {
        win_pct[rows[i].rarity] += BalanceTallyPct(rows[i].tally);
        ranked[rows[i].rarity]++;
    }
    printf("  mean win rate by rarity:");
    for (int rarity = 0; rarity < BALANCE_PROGRESS_RARITY_COUNT; rarity++)
        if (ranked[rarity]) printf(" %d: %.1f%%", rarity, win_pct[rarity] / ranked[rarity]);
    printf("\n");
}

static void
BalancePrintStrongestProgress(const BalanceOptions *o, BalanceProgressRow *rows, int count, const char *kind) {
    qsort(rows, (size_t)count, sizeof(rows[0]), BalanceProgressByWinPct);
    int shown = o->list_all || count < BALANCE_PROGRESS_SHOWN ? count : BALANCE_PROGRESS_SHOWN;
    printf("  %-44s %9s %8s\n", "", "win rate", "battles");
    for (int i = 0; i < shown; i++)
        printf("  %-44s %8.1f%% %8d\n", rows[i].label, BalanceTallyPct(rows[i].tally), rows[i].tally.battles);
    BalancePrintProgressByRarity(rows, count);
    printf(
        "  %d of %d %s win over %.0f%%\n",
        BalanceProgressStrongCount(rows, count),
        count,
        kind,
        BALANCE_PROGRESS_STRONG_PCT
    );
}

static bool BalanceRuneActsInBattle(const RuneDef *def) {
    return def->buff != RUNE_BUFF_GOLD && def->buff != RUNE_BUFF_XP;
}

static void BalanceProgressRunes(Balance *b, const BalanceOptions *o) {
    static BalanceProgressRow rows[BALANCE_PROGRESS_ROW_CAPACITY];
    int count = 0;
    for (int def = 0; def < RuneCatalogCount(); def++) {
        if (!BalanceRuneActsInBattle(RuneDefAt(def))) continue;
        BalanceProgressRow *row = &rows[count++];
        *row = (BalanceProgressRow){0};
        StrCopy(row->label, sizeof(row->label), RuneDefAt(def)->id);
        row->rarity = RuneDefAt(def)->rarity;
        BalanceProgressMeasure(b, row, def, BalanceCarryRune, &def);
    }
    printf("\nRunes: one rune on the squad, against the same squad without it; the strongest first\n");
    BalancePrintStrongestProgress(o, rows, count, "runes");
}

static void BalanceProgressGear(Balance *b, const BalanceOptions *o, bool every_hero) {
    static BalanceProgressRow rows[BALANCE_PROGRESS_ROW_CAPACITY];
    int count = 0;
    for (int def = 0; def < GearCatalogCount(); def++) {
        BalanceGearTrial trial = {.def = def, .every_hero = every_hero};
        BalanceProgressRow *row = &rows[count];
        *row = (BalanceProgressRow){0};
        snprintf(row->label, sizeof(row->label), "%s (rarity %d)", GearDefAt(def)->id, GearDefAt(def)->rarity);
        row->rarity = GearDefAt(def)->rarity;
        BalanceProgressMeasure(b, row, def * BALANCE_SIDES + every_hero, BalanceWearItem, &trial);
        count += row->tally.battles > 0;
    }
    printf(
        "\nGear: one item on %s, against the same squad without it; the strongest first\n",
        every_hero ? "every hero that can wear it" : "the first hero that can wear it"
    );
    BalancePrintStrongestProgress(o, rows, count, "items");
}

static void BalanceProgress(Balance *b, const BalanceOptions *o) {
    printf(
        "Progress: %d reference squads, %d seeds, %s; each row fights a changed squad against itself from both sides\n",
        b->squad_count,
        o->seeds,
        BalanceStatsName(o)
    );
    BalanceProgressGrowth(b);
    BalanceProgressRarity(b);
    BalanceProgressFortify(b);
    BalanceProgressRunes(b, o);
    BalanceProgressGear(b, o, false);
    BalanceProgressGear(b, o, true);
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
        .swaps = BALANCE_DEFAULT_SWAPS,
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
        } else if (!strcmp(argv[i], "--progress")) {
            o->progress = true;
        } else if (!strcmp(argv[i], "--unique")) {
            o->unique = true;
        } else if (!strcmp(argv[i], "--search") && values_left >= 1) {
            o->search_tries = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--adversarial") && values_left >= 1) {
            o->adversarial_rounds = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--swaps") && values_left >= 1) {
            o->swaps = atoi(argv[++i]);
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
    bool one_mode =
        o->stacks + o->squad_report + o->progress + (o->search_tries > 0) + (o->adversarial_rounds > 0) <= 1;
    bool calibration_fits =
        !o->calibrate_rounds || (!o->raw && !o->holdout && one_mode && !o->stacks && !o->squad_report && !o->progress &&
                                 !o->search_tries && !o->nudge_pct);
    bool adversarial_fits = o->adversarial_rounds >= 0 && o->adversarial_rounds <= BALANCE_ADVERSARIAL_ROUND_LIMIT &&
                            o->swaps > 0 && !(o->adversarial_rounds && o->gated_count);
    return adversarial_fits && o->seeds > 0 && o->seeds <= BALANCE_SEED_CAPACITY && o->squads > 0 &&
           o->squads <= BALANCE_SQUAD_CAPACITY && o->band_low_pct <= o->band_high_pct && o->calibrate_rounds >= 0 &&
           calibration_fits && !(o->raw && o->neutral) && one_mode && o->search_tries >= 0 && o->nudge_pct > -100 &&
           !(o->raw && o->nudge_pct) && !(o->progress && (o->gated_count || o->nudge_pct));
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
    } else if (options.progress) {
        BalanceProgress(b, &options);
    } else if (options.search_tries) {
        BalanceSearch(b, &options);
    } else if (options.adversarial_rounds) {
        static BalancePool pool;
        if (options.calibrate_rounds) BalanceDemandCalibrate(b, &options, &pool);
        BalanceAdversarial(b, &options, &pool);
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
