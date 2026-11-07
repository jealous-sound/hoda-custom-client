#define main hod_game_main
#include "../source/main.c"
#undef main

#define GOLDEN_DEFAULT_DIR "tests/golden"
#define GOLDEN_REPLAYS_FILE "battle_replays.json"
#define GOLDEN_RECIPES_FILE "battle_recipes.json"
#define GOLDEN_RNG_SEED 0x5EEDB417u
#define GOLDEN_HERO_APPEARANCES 2
#define GOLDEN_CHARACTER_CAPACITY 1024
#define GOLDEN_CHANGER_CAPACITY 96
#define GOLDEN_EMPTY_SLOT_PCT 10
#define GOLDEN_GIANT_PCT 50
#define GOLDEN_ASCENDED_PCT 15
#define GOLDEN_GEAR_SLOT_PCT 35
#define GOLDEN_CHANGER_PCT 30
#define GOLDEN_STANDARD_SLOTS 4
#define GOLDEN_CONSUME_XP_JITTER 9
#define GOLDEN_CAMPAIGN_RANDOM_NODES 1
#define GOLDEN_NEXUS_BATTLES 24
#define GOLDEN_PVP_BATTLES 24
#define GOLDEN_PVP_OPPONENTS 40
#define GOLDEN_PVP_CYCLES 24
#define GOLDEN_FIRST_NOW 1767225600LL
#define GOLDEN_BUFFED_BATTLES 4
#define GOLDEN_NOW_SPAN (86400LL * 400)

typedef struct {
    char id[64];
    bool giant;
    bool consumable;
    unsigned faction_mask;
    bool has_tier[TIER_COUNT];
    int max_level[TIER_COUNT];
    const ConsumeXpTable *consume_table;
    int uses;
} GoldenCharacter;

typedef struct {
    unsigned rng;
    const cJSON *catalog;
    GoldenCharacter characters[GOLDEN_CHARACTER_CAPACITY];
    int character_count;
    BattleGameChanger changers[GOLDEN_CHANGER_CAPACITY];
    int changer_count;
    cJSON *battles;
} Golden;

static unsigned GoldenRand(Golden *g) {
    unsigned x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g->rng = x;
}

static int GoldenRange(Golden *g, int lo, int hi) {
    return lo + (int)(GoldenRand(g) % (unsigned)(hi - lo + 1));
}

static bool GoldenChance(Golden *g, int pct) {
    return GoldenRange(g, 1, 100) <= pct;
}

static const ConsumeProfile *GoldenConsumeProfile(const Character *ch) {
    ConsumeCatalogEnsure();
    for (int i = 0; i < consume_catalog.profile_count; i++)
        if (!strcmp(consume_catalog.profiles[i].id, CharacterDataId(ch))) return &consume_catalog.profiles[i];
    return NULL;
}

static void GoldenLoadCharacters(Golden *g) {
    static Character ch;
    static Ability ability;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, g->catalog) {
        CharacterParse(row, &ch, &ability);
        bool any_tier = false;
        for (int t = 0; t < TIER_COUNT; t++) any_tier |= ch.has_tier[t];
        if (!any_tier || g->character_count == GOLDEN_CHARACTER_CAPACITY) continue;
        GoldenCharacter *entry = &g->characters[g->character_count++];
        StrCopy(entry->id, sizeof(entry->id), ch.id);
        entry->giant = ch.giant;
        entry->faction_mask = SimFactionMask(ch.faction);
        for (int t = 0; t < TIER_COUNT; t++) {
            entry->has_tier[t] = ch.has_tier[t];
            entry->max_level[t] = ch.stats[t].max_level > 0 ? ch.stats[t].max_level : 1;
        }
        const ConsumeProfile *profile = GoldenConsumeProfile(&ch);
        entry->consumable = profile != NULL;
        if (profile) entry->consume_table = profile->consume_table;
    }
}

static void GoldenLoadChangers(Golden *g) {
    cJSON *root = JsonLoadFile(NEXUS_GAME_CHANGERS_PATH);
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        if (g->changer_count == GOLDEN_CHANGER_CAPACITY) break;
        BattleGameChanger *changer = &g->changers[g->changer_count++];
        memset(changer, 0, sizeof(*changer));
        const cJSON *effect = NULL;
        cJSON_ArrayForEach(effect, cJSON_GetObjectItemCaseSensitive(item, "effects")) {
            if (changer->effect_count < BATTLE_CHANGER_MAX_EFFECTS)
                BattleChangerEffectLoad(&changer->effects[changer->effect_count++], effect);
        }
    }
    cJSON_Delete(root);
    const PvpEventRules *events = &PvpTableEnsure()->events;
    for (int i = 0; i < events->catalog_count && g->changer_count < GOLDEN_CHANGER_CAPACITY; i++)
        g->changers[g->changer_count++] = events->catalog[i].changer;
    for (int i = 0; i < events->guild_catalog_count && g->changer_count < GOLDEN_CHANGER_CAPACITY; i++)
        g->changers[g->changer_count++] = events->guild_catalog[i].changer;
}

static Tier GoldenTier(Golden *g, const GoldenCharacter *ch) {
    Tier tiers[TIER_COUNT];
    int count = 0;
    for (int t = 0; t < TIER_COUNT; t++)
        if (ch->has_tier[t]) tiers[count++] = (Tier)t;
    return tiers[GoldenRange(g, 0, count - 1)];
}

static GoldenCharacter *GoldenPick(Golden *g, bool giant, bool player_side, const bool *taken) {
    GoldenCharacter *best = NULL;
    int ties = 0;
    for (int i = 0; i < g->character_count; i++) {
        GoldenCharacter *ch = &g->characters[i];
        if (ch->giant != giant || taken[i] || (player_side && !ch->consumable)) continue;
        if (!best || ch->uses < best->uses) {
            best = ch;
            ties = 1;
        } else if (ch->uses == best->uses && GoldenRange(g, 1, ++ties) == 1) {
            best = ch;
        }
    }
    return best;
}

static cJSON *GoldenUnit(Golden *g, GoldenCharacter *ch, int slot, bool player_side) {
    ch->uses++;
    Tier tier = GoldenTier(g, ch);
    cJSON *unit = cJSON_CreateObject();
    cJSON_AddNumberToObject(unit, "slot", slot);
    cJSON_AddStringToObject(unit, "id", ch->id);
    cJSON_AddStringToObject(unit, "tier", TIER_NAMES[tier]);
    cJSON_AddNumberToObject(unit, "level", GoldenRange(g, 1, ch->max_level[tier]));
    if (!player_side) return unit;
    int consume_level = GoldenRange(g, 1, ch->consume_table->count);
    int consume_xp = ConsumeXpForLevel(ch->consume_table, consume_level) + GoldenRange(g, 0, GOLDEN_CONSUME_XP_JITTER);
    cJSON_AddNumberToObject(unit, "consume_xp", consume_xp);
    cJSON_AddBoolToObject(unit, "ascended", GoldenChance(g, GOLDEN_ASCENDED_PCT));
    cJSON *gear = cJSON_AddArrayToObject(unit, "gear");
    for (int s = 0; s < GEAR_SLOT_CAPACITY; s++) {
        const GearDef *def =
            GoldenChance(g, GOLDEN_GEAR_SLOT_PCT) ? GearDefAt(GoldenRange(g, 0, GearCatalogCount() - 1)) : NULL;
        cJSON_AddItemToArray(gear, cJSON_CreateString(def ? def->id : ""));
    }
    return unit;
}

static cJSON *GoldenRunes(Golden *g) {
    cJSON *runes = cJSON_CreateArray();
    int count = GoldenRange(g, -2, RUNE_SLOT_COUNT);
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(runes, cJSON_CreateString(RuneDefAt(GoldenRange(g, 0, RuneCatalogCount() - 1))->id));
    return runes;
}

static cJSON *GoldenSide(Golden *g, bool player_side) {
    bool taken[GOLDEN_CHARACTER_CAPACITY] = {false};
    cJSON *side = cJSON_CreateObject();
    cJSON *units = cJSON_AddArrayToObject(side, "units");
    for (int slot = 0; slot < GOLDEN_STANDARD_SLOTS; slot++) {
        if (cJSON_GetArraySize(units) > 0 && GoldenChance(g, GOLDEN_EMPTY_SLOT_PCT)) continue;
        GoldenCharacter *ch = GoldenPick(g, false, player_side, taken);
        if (!ch) continue;
        taken[ch - g->characters] = true;
        cJSON_AddItemToArray(units, GoldenUnit(g, ch, slot, player_side));
    }
    if (GoldenChance(g, GOLDEN_GIANT_PCT)) {
        GoldenCharacter *giant = GoldenPick(g, true, player_side, taken);
        if (giant) cJSON_AddItemToArray(units, GoldenUnit(g, giant, SQUAD_SLOT_LARGE, player_side));
    }
    cJSON_AddItemToObject(side, "runes", GoldenRunes(g));
    return side;
}

static cJSON *GoldenChanger(Golden *g) {
    if (!GoldenChance(g, GOLDEN_CHANGER_PCT)) return cJSON_CreateNull();
    return BattleReplayChangerJson(&g->changers[GoldenRange(g, 0, g->changer_count - 1)]);
}

static void GoldenAddBattle(Golden *g, const char *name, cJSON *player, cJSON *enemy, cJSON *changer) {
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddStringToObject(entry, "name", name);
    cJSON_AddNumberToObject(entry, "seed", GoldenRand(g));
    cJSON_AddItemToObject(entry, "player", player);
    cJSON_AddItemToObject(entry, "enemy", enemy);
    cJSON_AddItemToObject(entry, "changer", changer);
    BattleReplayOutcome outcome;
    if (!BattleReplayRun(g->catalog, entry, &outcome)) TraceLog(LOG_FATAL, "Golden: battle %s does not load", name);
    BattleReplayOutcomeAdd(entry, &outcome);
    cJSON_AddItemToArray(g->battles, entry);
}

static bool GoldenHeroesCovered(const Golden *g) {
    for (int i = 0; i < g->character_count; i++)
        if (g->characters[i].uses < GOLDEN_HERO_APPEARANCES) return false;
    return true;
}

static void GoldenHeroBattles(Golden *g) {
    for (int battle = 0; !GoldenHeroesCovered(g); battle++) {
        cJSON *player = GoldenSide(g, true);
        cJSON *enemy = GoldenSide(g, false);
        GoldenAddBattle(g, TextFormat("heroes-%03d", battle), player, enemy, GoldenChanger(g));
    }
}

static bool GoldenChangerBuffsFaction(const BattleGameChanger *changer, unsigned *faction) {
    if (changer->effect_count == 0) return false;
    const BattleChangerEffect *effect = &changer->effects[0];
    *faction = effect->faction_mask;
    return effect->faction_mask && effect->group_count == 0 && effect->unit_count == 0;
}

static void GoldenBuffedBattles(Golden *g) {
    int written = 0;
    for (int c = 0; c < g->changer_count && written < GOLDEN_BUFFED_BATTLES; c++) {
        unsigned faction = 0;
        if (!GoldenChangerBuffsFaction(&g->changers[c], &faction)) continue;
        cJSON *player = cJSON_CreateObject();
        cJSON *units = cJSON_AddArrayToObject(player, "units");
        int slot = 0;
        for (int i = 0; i < g->character_count && slot < GOLDEN_STANDARD_SLOTS; i++) {
            GoldenCharacter *ch = &g->characters[GoldenRange(g, 0, g->character_count - 1)];
            if (ch->giant || !ch->consumable || !(ch->faction_mask & faction)) continue;
            cJSON_AddItemToArray(units, GoldenUnit(g, ch, slot++, true));
        }
        cJSON_AddItemToObject(player, "runes", cJSON_CreateArray());
        if (slot == 0) {
            cJSON_Delete(player);
            continue;
        }
        GoldenAddBattle(
            g,
            TextFormat("buffed-%d", written++),
            player,
            GoldenSide(g, false),
            BattleReplayChangerJson(&g->changers[c])
        );
    }
}

static cJSON *GoldenCampaignRosters(Golden *g, const char *rosters_path, const char *mode) {
    cJSON *rosters = cJSON_CreateArray();
    cJSON *root = JsonLoadFile(rosters_path);
    static BattleRoster roster;
    const cJSON *campaign = NULL;
    cJSON_ArrayForEach(campaign, cJSON_GetObjectItemCaseSensitive(root, "campaigns")) {
        const char *map_id = JsonStr(campaign, "campaign");
        int node_count = cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(campaign, "nodes"));
        int random_step = node_count > 0 ? GoldenRange(g, 1, node_count) : 0;
        const cJSON *node = NULL;
        cJSON_ArrayForEach(node, cJSON_GetObjectItemCaseSensitive(campaign, "nodes")) {
            int step = JsonInt(node, "step");
            BattleRosterLoad(&roster, rosters_path, BATTLE_REPLAY_CATALOG_PATH, map_id, step);
            cJSON *entry = cJSON_CreateObject();
            cJSON_AddStringToObject(entry, "mode", mode);
            cJSON_AddStringToObject(entry, "map", map_id);
            cJSON_AddNumberToObject(entry, "step", step);
            cJSON_AddItemToObject(entry, "enemy", BattleReplayRosterJson(&roster, false));
            cJSON_AddItemToArray(rosters, entry);
            bool campaign_battle = !strcmp(mode, "campaign") && (step == node_count || step == random_step);
            if (campaign_battle)
                GoldenAddBattle(
                    g,
                    TextFormat("%s-%s-%d", mode, map_id, step),
                    GoldenSide(g, true),
                    BattleReplayRosterJson(&roster, false),
                    cJSON_CreateNull()
                );
        }
    }
    cJSON_Delete(root);
    return rosters;
}

static void GoldenNexusBattles(Golden *g, const cJSON *rosters) {
    int count = 0;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, rosters) count++;
    for (int i = 0; i < GOLDEN_NEXUS_BATTLES; i++) {
        const cJSON *pick = cJSON_GetArrayItem(rosters, GoldenRange(g, 0, count - 1));
        GoldenAddBattle(
            g,
            TextFormat("nexus-%s-%d", JsonStr(pick, "map"), JsonInt(pick, "step")),
            GoldenSide(g, true),
            cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(pick, "enemy"), true),
            BattleReplayChangerJson(&g->changers[GoldenRange(g, 0, g->changer_count - 1)])
        );
    }
}

static cJSON *GoldenNexusChangers(void) {
    cJSON *changers = cJSON_CreateArray();
    cJSON *root = JsonLoadFile(NEXUS_GAME_CHANGERS_PATH);
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        BattleGameChanger changer = {0};
        const cJSON *effect = NULL;
        cJSON_ArrayForEach(effect, cJSON_GetObjectItemCaseSensitive(item, "effects")) {
            if (changer.effect_count < BATTLE_CHANGER_MAX_EFFECTS)
                BattleChangerEffectLoad(&changer.effects[changer.effect_count++], effect);
        }
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "event", JsonStr(item, "event"));
        cJSON_AddItemToObject(entry, "changer", BattleReplayChangerJson(&changer));
        cJSON_AddItemToArray(changers, entry);
    }
    cJSON_Delete(root);
    return changers;
}

static cJSON *GoldenPvpChangers(Golden *g) {
    cJSON *changers = cJSON_CreateArray();
    long long first = PvpEventCycle(GOLDEN_FIRST_NOW);
    for (int i = 0; i < GOLDEN_PVP_CYCLES; i++) {
        long long cycle = first + i + GoldenRange(g, 0, 3);
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(entry, "cycle", (double)cycle);
        cJSON_AddItemToObject(entry, "changer", BattleReplayChangerJson(&PvpEventForCycle(cycle)->changer));
        cJSON_AddItemToArray(changers, entry);
    }
    return changers;
}

static const char *const GOLDEN_PVP_KINDS[] = {"ladder", "event", "bot", "revenge"};

static PvpOpponent GoldenPvpOpponent(Golden *g, cJSON *entry) {
    const PvpTable *table = PvpTableEnsure();
    const char *kind = GOLDEN_PVP_KINDS[GoldenRange(g, 0, 3)];
    int level = GoldenRange(g, 1, PVP_LOCAL_MAX_LEVEL);
    int trophies = GoldenRange(g, 0, 4000);
    int banners = GoldenRange(g, 0, table->events.ladder_banners);
    long long now = GOLDEN_FIRST_NOW + (long long)GoldenRange(g, 0, (int)(GOLDEN_NOW_SPAN / 60)) * 60;
    unsigned seed = (unsigned)GoldenRange(g, 1, 0x7FFFFFFF);
    cJSON_AddStringToObject(entry, "kind", kind);
    cJSON_AddNumberToObject(entry, "level", level);
    cJSON_AddNumberToObject(entry, "trophies", trophies);
    cJSON_AddNumberToObject(entry, "now", (double)now);
    cJSON_AddNumberToObject(entry, "seed", seed);
    if (!strcmp(kind, "event")) {
        cJSON_AddNumberToObject(entry, "banners", banners);
        return PvpEventOpponentGenerate(level, trophies, banners, now, seed);
    }
    if (!strcmp(kind, "bot")) {
        int bot = GoldenRange(g, 0, table->bot_count - 1);
        cJSON_AddNumberToObject(entry, "bot", bot);
        return PvpOpponentFromBot(bot, level, trophies, PvpDay(now), seed);
    }
    if (!strcmp(kind, "revenge")) {
        PlayerPvpBattle battle = {
            .bot = GoldenRange(g, 0, table->bot_count - 1),
            .squad = GoldenRange(g, 0, table->squad_count - 1),
            .level = level,
            .opponent_trophies = GoldenRange(g, 0, 4000),
            .seed = seed,
        };
        cJSON_AddNumberToObject(entry, "bot", battle.bot);
        cJSON_AddNumberToObject(entry, "squad", battle.squad);
        cJSON_AddNumberToObject(entry, "opponent_trophies", battle.opponent_trophies);
        return PvpOpponentFromBattle(&battle, trophies);
    }
    return PvpOpponentGenerate(level, trophies, PvpDay(now), seed);
}

static cJSON *GoldenPvpOpponents(Golden *g) {
    static BattleRoster roster;
    cJSON *opponents = cJSON_CreateArray();
    for (int i = 0; i < GOLDEN_PVP_OPPONENTS; i++) {
        cJSON *entry = cJSON_CreateObject();
        PvpOpponent opponent = GoldenPvpOpponent(g, entry);
        PvpOpponentRoster(&roster, &opponent);
        cJSON *result = cJSON_AddObjectToObject(entry, "opponent");
        cJSON_AddNumberToObject(result, "bot", opponent.bot);
        cJSON_AddNumberToObject(result, "squad", opponent.squad);
        cJSON_AddNumberToObject(result, "level", opponent.level);
        cJSON_AddNumberToObject(result, "trophies", opponent.trophies);
        cJSON_AddItemToObject(entry, "enemy", BattleReplayRosterJson(&roster, false));
        cJSON_AddItemToArray(opponents, entry);
        if (i < GOLDEN_PVP_BATTLES)
            GoldenAddBattle(
                g,
                TextFormat("pvp-%s-%d", JsonStr(entry, "kind"), i),
                GoldenSide(g, true),
                BattleReplayRosterJson(&roster, false),
                GoldenChanger(g)
            );
    }
    return opponents;
}

static const char *const GOLDEN_RECIPE_TARGETS[] = {
    "ability", "one", "row", "column", "all", "one_ally", "back_row", "any"
};

static void GoldenAddShort(cJSON *json, const char *name, short value) {
    if (value) cJSON_AddNumberToObject(json, name, value);
}

static cJSON *GoldenAttackJson(const AttackRecipe *a) {
    cJSON *json = cJSON_CreateObject();
    GoldenAddShort(json, "damage_pct", a->dmg_pct);
    GoldenAddShort(json, "stun_pct", a->stun_pct);
    GoldenAddShort(json, "slow_pct", a->slow_pct);
    GoldenAddShort(json, "slow_amt", a->slow_amt);
    GoldenAddShort(json, "drain_pct", a->drain_pct);
    GoldenAddShort(json, "drain_chance", a->drain_chance);
    GoldenAddShort(json, "curse_power_pct", a->curse_power_pct);
    GoldenAddShort(json, "curse_health_pct", a->curse_health_pct);
    GoldenAddShort(json, "curse_turns", a->curse_turns);
    GoldenAddShort(json, "levy_pct", a->levy_pct);
    GoldenAddShort(json, "assassinate_pct", a->assassinate_pct);
    GoldenAddShort(json, "mark_pct", a->mark_pct);
    GoldenAddShort(json, "self_heal_pct", a->self_heal_pct);
    GoldenAddShort(json, "self_crit_pct", a->self_crit_pct);
    if (a->target != RECIPE_TARGET_ABILITY) cJSON_AddStringToObject(json, "target", GOLDEN_RECIPE_TARGETS[a->target]);
    if (a->status_only) cJSON_AddBoolToObject(json, "status_only", true);
    return json;
}

static cJSON *GoldenEffectsJson(const SecondaryEffects *fx) {
    cJSON *json = cJSON_CreateObject();
    GoldenAddShort(json, "self_power_pct", fx->self_power_pct);
    GoldenAddShort(json, "self_power_chance", fx->self_power_chance);
    GoldenAddShort(json, "self_heal_pct", fx->self_heal_pct);
    GoldenAddShort(json, "self_heal_chance", fx->self_heal_chance);
    GoldenAddShort(json, "self_damage_bonus_pct", fx->self_damage_bonus_pct);
    GoldenAddShort(json, "self_damage_pct", fx->self_damage_pct);
    GoldenAddShort(json, "ally_heal_pct", fx->ally_heal_pct);
    GoldenAddShort(json, "ally_heal_chance", fx->ally_heal_chance);
    GoldenAddShort(json, "ally_power_pct", fx->ally_power_pct);
    GoldenAddShort(json, "ally_power_chance", fx->ally_power_chance);
    GoldenAddShort(json, "ally_power_from_actor_pct", fx->ally_power_from_actor_pct);
    GoldenAddShort(json, "ally_slow_amt", fx->ally_slow_amt);
    GoldenAddShort(json, "ally_crit_pct", fx->ally_crit_pct);
    GoldenAddShort(json, "ally_damage_bonus_pct", fx->ally_damage_bonus_pct);
    GoldenAddShort(json, "ally_damage_pct", fx->ally_damage_pct);
    return json;
}

static cJSON *GoldenBuiltinRecipes(void) {
    cJSON *recipes = cJSON_CreateArray();
    for (int i = 0; i < SIM_RECIPE_COUNT; i++) {
        const AbilityRecipe *r = &SIM_RECIPES[i];
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "key", r->key);
        cJSON_AddItemToObject(json, "hit", GoldenAttackJson(&r->hit));
        cJSON_AddItemToObject(json, "effects", GoldenEffectsJson(&r->fx));
        GoldenAddShort(json, "effect_pct", r->effect_pct);
        cJSON_AddItemToObject(json, "effect", GoldenAttackJson(&r->effect));
        GoldenAddShort(json, "followup_pct", r->followup_pct);
        GoldenAddShort(json, "followup_count", r->followup_count);
        GoldenAddShort(json, "followup_damage_step_pct", r->followup_damage_step_pct);
        GoldenAddShort(json, "followup_final_damage_pct", r->followup_final_damage_pct);
        cJSON_AddItemToObject(json, "followup", GoldenAttackJson(&r->followup));
        cJSON_AddItemToArray(recipes, json);
    }
    return recipes;
}

static void GoldenWriteArray(FILE *file, const char *name, const cJSON *array, bool last) {
    fprintf(file, "  \"%s\": [", name);
    const cJSON *item = NULL;
    bool first = true;
    cJSON_ArrayForEach(item, array) {
        char *text = cJSON_PrintUnformatted(item);
        fprintf(file, "%s\n    %s", first ? "" : ",", text);
        cJSON_free(text);
        first = false;
    }
    fprintf(file, "\n  ]%s\n", last ? "" : ",");
}

static bool GoldenSameContents(FILE *generated, const char *path) {
    FILE *tracked = fopen(path, "rb");
    if (!tracked) return false;
    rewind(generated);
    int a, b;
    do {
        a = fgetc(generated);
        b = fgetc(tracked);
    } while (a == b && a != EOF);
    fclose(tracked);
    return a == b;
}

static FILE *GoldenOpen(bool check, const char *path) {
    FILE *file = check ? tmpfile() : fopen(path, "wb");
    if (!file) TraceLog(LOG_FATAL, "Golden: cannot write %s", path);
    return file;
}

static bool GoldenClose(FILE *file, bool check, const char *path) {
    bool current = !check || GoldenSameContents(file, path);
    fclose(file);
    if (check && !current) printf("Golden: %s is stale; run make golden\n", path);
    return current;
}

int main(int argc, char **argv) {
    SetTraceLogLevel(LOG_WARNING);
    bool check = argc > 1 && !strcmp(argv[1], "--check");
    const char *dir = argc > (check ? 2 : 1) ? argv[check ? 2 : 1] : GOLDEN_DEFAULT_DIR;
    char replays_path[512], recipes_path[512];
    snprintf(replays_path, sizeof(replays_path), "%s/%s", dir, GOLDEN_REPLAYS_FILE);
    snprintf(recipes_path, sizeof(recipes_path), "%s/%s", dir, GOLDEN_RECIPES_FILE);
    static Golden golden;
    Golden *g = &golden;
    g->rng = GOLDEN_RNG_SEED;
    cJSON *catalog_root = BattleReplayCatalogLoad();
    g->catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    g->battles = cJSON_CreateArray();
    GoldenLoadCharacters(g);
    GoldenLoadChangers(g);

    GoldenHeroBattles(g);
    GoldenBuffedBattles(g);
    cJSON *campaign = GoldenCampaignRosters(g, "data/campaign_rosters.json", "campaign");
    cJSON *nexus = GoldenCampaignRosters(g, "data/nexus_rosters.json", "nexus");
    GoldenNexusBattles(g, nexus);
    cJSON *opponents = GoldenPvpOpponents(g);
    cJSON *nexus_changers = GoldenNexusChangers();
    cJSON *pvp_changers = GoldenPvpChangers(g);
    cJSON *recipes = GoldenBuiltinRecipes();

    FILE *file = GoldenOpen(check, replays_path);
    fprintf(file, "{\n  \"version\": 1,\n");
    GoldenWriteArray(file, "battles", g->battles, false);
    GoldenWriteArray(file, "campaign_rosters", campaign, false);
    GoldenWriteArray(file, "nexus_rosters", nexus, false);
    GoldenWriteArray(file, "nexus_changers", nexus_changers, false);
    GoldenWriteArray(file, "pvp_changers", pvp_changers, false);
    GoldenWriteArray(file, "pvp_opponents", opponents, true);
    fprintf(file, "}\n");
    bool current = GoldenClose(file, check, replays_path);

    file = GoldenOpen(check, recipes_path);
    fprintf(file, "{\n  \"version\": 1,\n");
    GoldenWriteArray(file, "recipes", recipes, true);
    fprintf(file, "}\n");
    current = GoldenClose(file, check, recipes_path) && current;

    if (check && current) printf("Golden: %s matches the sim\n", dir);
    if (!check) printf("Golden: %d battles written to %s\n", cJSON_GetArraySize(g->battles), dir);

    cJSON_Delete(g->battles);
    cJSON_Delete(campaign);
    cJSON_Delete(nexus);
    cJSON_Delete(opponents);
    cJSON_Delete(nexus_changers);
    cJSON_Delete(pvp_changers);
    cJSON_Delete(recipes);
    cJSON_Delete(catalog_root);
    return current ? 0 : 1;
}
