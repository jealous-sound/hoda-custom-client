#define main hod_game_main
#include "../source/main.c"
#undef main

#define BALANCE_USAGE                                                                                                  \
    "usage: balance_check [--raw] [--seeds N] [--squads N] [--band LOW HIGH] [--all] [--hero UNIT_ID]...\n"            \
    "Swaps every hero into reference squads, fights the unchanged squads from both sides and reports win rates.\n"     \
    "--raw fights with catalog stats instead of the stat budget. --all lists every hero, weakest first.\n"             \
    "--hero fights only that hero and fails when it leaves the band."
#define BALANCE_HERO_CAPACITY 1024
#define BALANCE_SQUAD_CAPACITY 64
#define BALANCE_GATED_HERO_CAPACITY 32
#define BALANCE_DEFAULT_SQUADS 12
#define BALANCE_DEFAULT_SEEDS 4
#define BALANCE_DEFAULT_BAND_LOW_PCT 45.0f
#define BALANCE_DEFAULT_BAND_HIGH_PCT 55.0f
#define BALANCE_EXTREMES_SHOWN 10
#define BALANCE_STAR_CAPACITY 4
#define BALANCE_FODDER_GROUP "Wisp"
#define BALANCE_SEED_SALT 0x9E3779B9u
#define BALANCE_EXIT_OUT_OF_BAND 1
#define BALANCE_EXIT_USAGE 2

typedef struct {
    char id[64];
    char name[64];
    Rarity rarity;
    int stars;
    bool giant;
    const cJSON *def;
    int wins, battles;
} BalanceHero;

typedef struct {
    bool raw;
    bool list_all;
    int seeds;
    int squads;
    float band_low_pct, band_high_pct;
    const char *gated[BALANCE_GATED_HERO_CAPACITY];
    int gated_count;
} BalanceOptions;

typedef struct {
    const cJSON *catalog;
    const StatBudget *stat_budget;
    BalanceHero heroes[BALANCE_HERO_CAPACITY];
    int hero_count;
    BattleRoster squads[BALANCE_SQUAD_CAPACITY];
    int squad_count;
    int mirror_player_wins, mirror_battles;
} Balance;

static void BalanceMaxOut(Character *ch) {
    ch->tier = ch->max_tier;
    int max_level = ch->stats[ch->tier].max_level;
    ch->level = max_level > 0 ? max_level : 1;
}

static bool BalanceFieldsInBattle(const Character *ch) {
    return ch->has_tier[ch->max_tier] && ConsumeProfileFor(ch) && !SimPassiveHasGroup(ch, BALANCE_FODDER_GROUP);
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
        hero->def = row;
    }
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

static void BalanceLoadSquads(Balance *b, int wanted) {
    const PvpTable *table = PvpTableEnsure();
    int full[PVP_SQUAD_CAPACITY];
    int full_count = 0;
    for (int i = 0; i < table->squad_count; i++)
        if (BalanceSquadIsFull(&table->squads[i])) full[full_count++] = i;
    if (wanted > full_count) wanted = full_count;
    for (int i = 0; i < wanted; i++) {
        int evenly_spread = wanted > 1 ? i * (full_count - 1) / (wanted - 1) : full_count - 1;
        BalanceRosterFromSquad(b, &b->squads[b->squad_count++], &table->squads[full[evenly_spread]]);
    }
}

static void BalanceRosterWithHero(BattleRoster *out, const BattleRoster *squad, const BalanceHero *hero, int slot) {
    *out = *squad;
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) out->chars[s].ability = &out->abilities[s];
    CharacterParse(hero->def, &out->chars[slot], &out->abilities[slot]);
    BalanceMaxOut(&out->chars[slot]);
}

static unsigned BalanceSeed(int squad, int slot, int seed) {
    return BALANCE_SEED_SALT ^ (unsigned)(squad * 8191 + slot * 127 + seed);
}

static void BalanceFightHero(Balance *b, BalanceHero *hero, int seeds) {
    static BattleSim sim;
    static BattleRoster with_hero;
    int first_slot = hero->giant ? SQUAD_SLOT_LARGE : 0;
    int last_slot = hero->giant ? SQUAD_SLOT_LARGE : SQUAD_SLOT_LARGE - 1;
    for (int q = 0; q < b->squad_count; q++) {
        const BattleRoster *squad = &b->squads[q];
        for (int slot = first_slot; slot <= last_slot; slot++) {
            BalanceRosterWithHero(&with_hero, squad, hero, slot);
            for (int seed = 1; seed <= seeds; seed++) {
                BattleSimRunRosters(&sim, &with_hero, squad, BalanceSeed(q, slot, seed));
                hero->wins += sim.player_won;
                BattleSimRunRosters(&sim, squad, &with_hero, BalanceSeed(q, slot, seed));
                hero->wins += !sim.player_won;
                hero->battles += 2;
            }
        }
    }
}

static void BalanceFightMirrors(Balance *b, int seeds) {
    static BattleSim sim;
    for (int q = 0; q < b->squad_count; q++) {
        for (int seed = 1; seed <= seeds * SQUAD_SLOT_COUNT; seed++) {
            BattleSimRunRosters(&sim, &b->squads[q], &b->squads[q], BalanceSeed(q, SQUAD_SLOT_COUNT, seed));
            b->mirror_player_wins += sim.player_won;
            b->mirror_battles++;
        }
    }
}

static float BalanceWinPct(const BalanceHero *hero) {
    return hero->battles ? 100.0f * hero->wins / hero->battles : 0.0f;
}

static int BalanceByWinPct(const void *a, const void *b) {
    float delta = BalanceWinPct(*(const BalanceHero *const *)a) - BalanceWinPct(*(const BalanceHero *const *)b);
    return (delta > 0) - (delta < 0);
}

static bool BalanceInBand(const BalanceOptions *o, const BalanceHero *hero) {
    float pct = BalanceWinPct(hero);
    return pct >= o->band_low_pct && pct <= o->band_high_pct;
}

static const char *BalanceClassName(Rarity rarity, int stars, bool giant) {
    static const char *const RARITY_LABELS[] = {"", "Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic"};
    if (giant) return TextFormat("%s giant", RARITY_LABELS[rarity]);
    if (stars > 0) return TextFormat("%s %d-star", RARITY_LABELS[rarity], stars);
    return RARITY_LABELS[rarity];
}

static void BalancePrintClass(const Balance *b, const BalanceOptions *o, Rarity rarity, int stars, bool giant) {
    static const BalanceHero *members[BALANCE_HERO_CAPACITY];
    int count = 0, in_band = 0;
    for (int i = 0; i < b->hero_count; i++) {
        const BalanceHero *hero = &b->heroes[i];
        if (!hero->battles || hero->rarity != rarity || hero->giant != giant || (!giant && hero->stars != stars))
            continue;
        members[count++] = hero;
        in_band += BalanceInBand(o, hero);
    }
    if (!count) return;
    qsort(members, (size_t)count, sizeof(members[0]), BalanceByWinPct);
    printf(
        "%-20s %6d %7.1f %7.1f %7.1f %8d\n",
        BalanceClassName(rarity, stars, giant),
        count,
        BalanceWinPct(members[0]),
        BalanceWinPct(members[count / 2]),
        BalanceWinPct(members[count - 1]),
        in_band
    );
}

static void BalancePrintHero(const BalanceHero *hero) {
    printf(
        "  %5.1f%%  %-28s %-34s %s\n",
        BalanceWinPct(hero),
        hero->id,
        hero->name,
        BalanceClassName(hero->rarity, hero->stars, hero->giant)
    );
}

static void BalancePrintReport(const Balance *b, const BalanceOptions *o) {
    printf(
        "Balance: %d heroes, %d reference squads, %d seeds, %s\n",
        b->hero_count,
        b->squad_count,
        o->seeds,
        o->raw ? "catalog stats" : "stat budget"
    );
    printf(
        "Mirror battles won by the attacking side: %.1f%%\n",
        b->mirror_battles ? 100.0f * b->mirror_player_wins / b->mirror_battles : 0.0f
    );
    printf("\n%-20s %6s %7s %7s %7s %8s\n", "class", "heroes", "min%", "median%", "max%", "in band");
    for (int rarity = RARITY_COMMON; rarity <= RARITY_MYTHIC; rarity++) {
        for (int stars = 0; stars < BALANCE_STAR_CAPACITY; stars++) BalancePrintClass(b, o, rarity, stars, false);
        BalancePrintClass(b, o, rarity, 0, true);
    }

    static const BalanceHero *ranked[BALANCE_HERO_CAPACITY];
    int count = 0, in_band = 0;
    for (int i = 0; i < b->hero_count; i++) {
        if (!b->heroes[i].battles) continue;
        ranked[count++] = &b->heroes[i];
        in_band += BalanceInBand(o, &b->heroes[i]);
    }
    qsort(ranked, (size_t)count, sizeof(ranked[0]), BalanceByWinPct);
    printf("\n%d of %d heroes win %.0f%%-%.0f%%\n", in_band, count, o->band_low_pct, o->band_high_pct);
    if (o->list_all) {
        printf("\nEvery hero:\n");
        for (int i = 0; i < count; i++) BalancePrintHero(ranked[i]);
        return;
    }
    int shown = count < BALANCE_EXTREMES_SHOWN ? count : BALANCE_EXTREMES_SHOWN;
    printf("\nStrongest:\n");
    for (int i = 0; i < shown; i++) BalancePrintHero(ranked[count - 1 - i]);
    printf("\nWeakest:\n");
    for (int i = 0; i < shown; i++) BalancePrintHero(ranked[i]);
}

static int BalanceGate(const Balance *b, const BalanceOptions *o) {
    int status = 0;
    for (int g = 0; g < o->gated_count; g++) {
        const BalanceHero *hero = NULL;
        for (int i = 0; i < b->hero_count; i++)
            if (!strcmp(b->heroes[i].id, o->gated[g])) hero = &b->heroes[i];
        if (!hero || !hero->battles) {
            printf("Balance: %s is not a playable hero\n", o->gated[g]);
            status = BALANCE_EXIT_OUT_OF_BAND;
            continue;
        }
        bool in_band = BalanceInBand(o, hero);
        printf(
            "Balance: %s wins %.1f%% of %d battles, %s the %.0f%%-%.0f%% band\n",
            hero->id,
            BalanceWinPct(hero),
            hero->battles,
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
        } else if (!strcmp(argv[i], "--all")) {
            o->list_all = true;
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
    return o->seeds > 0 && o->squads > 0 && o->squads <= BALANCE_SQUAD_CAPACITY && o->band_low_pct <= o->band_high_pct;
}

static bool BalanceIsGated(const BalanceOptions *o, const BalanceHero *hero) {
    if (!o->gated_count) return true;
    for (int g = 0; g < o->gated_count; g++)
        if (!strcmp(o->gated[g], hero->id)) return true;
    return false;
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
    b->stat_budget = options.raw ? NULL : StatBudgetEnsure();
    BalanceLoadHeroes(b);
    BalanceLoadSquads(b, options.squads);

    for (int i = 0; i < b->hero_count; i++)
        if (BalanceIsGated(&options, &b->heroes[i])) BalanceFightHero(b, &b->heroes[i], options.seeds);

    int status = 0;
    if (options.gated_count) {
        status = BalanceGate(b, &options);
    } else {
        BalanceFightMirrors(b, options.seeds);
        BalancePrintReport(b, &options);
    }
    cJSON_Delete(catalog_root);
    return status;
}
