#define BATTLE_REPLAY_CATALOG_PATH HOD_CACHE "characters.json"
#define BATTLE_REPLAY_DIGEST_HEX (SHA256_DIGEST_BYTES * 2 + 1)
#define BATTLE_REPLAY_ACTION_TEXT (64 + SIM_MAX_TARGETS * 96)

static const char *BattleReplayGearId(const Character *ch, int slot) {
    const GearDef *def = GearInSlot(ch, slot);
    return def ? def->id : "";
}

static cJSON *BattleReplayUnitJson(const Character *ch, int slot, bool player_side, bool hero_build) {
    cJSON *unit = cJSON_CreateObject();
    if (!unit) return NULL;
    cJSON_AddNumberToObject(unit, "slot", slot);
    if (player_side && ch->instance_id) cJSON_AddNumberToObject(unit, "instance_id", (double)ch->instance_id);
    cJSON_AddStringToObject(unit, "id", ch->id);
    cJSON_AddStringToObject(unit, "tier", TIER_NAMES[ch->tier]);
    cJSON_AddNumberToObject(unit, "level", ch->level);
    if (!hero_build) return unit;
    cJSON_AddNumberToObject(unit, "consume_xp", ch->consume_xp);
    cJSON_AddBoolToObject(unit, "ascended", ch->ascended);
    cJSON *gear = cJSON_AddArrayToObject(unit, "gear");
    for (int g = 0; gear && g < GEAR_SLOT_CAPACITY; g++)
        cJSON_AddItemToArray(gear, cJSON_CreateString(BattleReplayGearId(ch, g)));
    cJSON *fortify = FortifyChildrenJson(ch);
    if (fortify) cJSON_AddItemToObject(unit, "fortify", fortify);
    return unit;
}

static cJSON *BattleReplaySideJson(const BattleSimSide *side, bool player_side, bool hero_builds) {
    cJSON *json = cJSON_CreateObject();
    cJSON *units = cJSON_AddArrayToObject(json, "units");
    cJSON *runes = cJSON_AddArrayToObject(json, "runes");
    if (!units || !runes) {
        cJSON_Delete(json);
        return NULL;
    }
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const Character *ch = side->chars[slot];
        if (ch) cJSON_AddItemToArray(units, BattleReplayUnitJson(ch, slot, player_side, hero_builds));
    }
    for (int i = 0; i < side->rune_def_count; i++) {
        const RuneDef *def = RuneDefAt(side->rune_defs[i]);
        if (def) cJSON_AddItemToArray(runes, cJSON_CreateString(def->id));
    }
    if (hero_builds && !player_side) cJSON_AddBoolToObject(json, BATTLE_HERO_BUILDS_FIELD, true);
    return json;
}

cJSON *BattleReplayPlayerJson(const Player *player) {
    BattleSimSide side = SimPlayerSide(player);
    return BattleReplaySideJson(&side, true, true);
}

cJSON *BattleReplayRosterJson(const BattleRoster *roster, bool player_side) {
    BattleSimSide side = SimRosterSide(roster);
    return BattleReplaySideJson(&side, player_side, player_side || roster->hero_builds);
}

static cJSON *BattleReplayStrings(const char *first, int count, size_t stride) {
    cJSON *array = cJSON_CreateArray();
    for (int i = 0; array && i < count; i++)
        cJSON_AddItemToArray(array, cJSON_CreateString(first + (size_t)i * stride));
    return array;
}

cJSON *BattleReplayChangerJson(const BattleGameChanger *changer) {
    if (!changer) return cJSON_CreateNull();
    cJSON *json = cJSON_CreateObject();
    cJSON *effects = cJSON_AddArrayToObject(json, "effects");
    for (int e = 0; effects && e < changer->effect_count; e++) {
        const BattleChangerEffect *effect = &changer->effects[e];
        cJSON *row = cJSON_CreateObject();
        if (!row) break;
        cJSON_AddItemToObject(
            row, "groups", BattleReplayStrings(effect->groups[0], effect->group_count, sizeof(effect->groups[0]))
        );
        cJSON_AddItemToObject(
            row, "units", BattleReplayStrings(effect->units[0], effect->unit_count, sizeof(effect->units[0]))
        );
        cJSON_AddNumberToObject(row, "faction_mask", effect->faction_mask);
        cJSON_AddNumberToObject(row, "health_pct", effect->health_pct);
        cJSON_AddNumberToObject(row, "power_pct", effect->power_pct);
        cJSON_AddItemToArray(effects, row);
    }
    return json;
}

bool BattleReplayChangerLoad(BattleGameChanger *changer, const cJSON *json) {
    memset(changer, 0, sizeof(*changer));
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(json, "effects")) {
        if (changer->effect_count == BATTLE_CHANGER_MAX_EFFECTS) return false;
        BattleChangerEffect *effect = &changer->effects[changer->effect_count++];
        const cJSON *item = NULL;
        cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(row, "groups")) {
            if (effect->group_count == BATTLE_CHANGER_MAX_GROUPS || !cJSON_IsString(item)) return false;
            StrCopy(effect->groups[effect->group_count++], sizeof(effect->groups[0]), item->valuestring);
        }
        cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(row, "units")) {
            if (effect->unit_count == BATTLE_CHANGER_MAX_UNITS || !cJSON_IsString(item)) return false;
            StrCopy(effect->units[effect->unit_count++], sizeof(effect->units[0]), item->valuestring);
        }
        effect->faction_mask = (unsigned)JsonInt(row, "faction_mask");
        effect->health_pct = JsonInt(row, "health_pct");
        effect->power_pct = JsonInt(row, "power_pct");
    }
    return true;
}

static bool
BattleReplayUnitLoad(BattleRoster *roster, const cJSON *catalog, const cJSON *unit, bool player_side, bool hero_build) {
    int slot = JsonInt(unit, "slot");
    if (slot < 0 || slot >= SQUAD_SLOT_COUNT || roster->present[slot]) return false;
    const cJSON *def = CatalogFindCharacter(catalog, JsonStr(unit, "id"));
    if (!def) return false;
    Character *ch = &roster->chars[slot];
    CharacterParse(def, ch, &roster->abilities[slot]);
    ch->tier = TierFromString(JsonStr(unit, "tier"));
    if (!ch->has_tier[ch->tier] || ch->giant != (slot == SQUAD_SLOT_LARGE)) return false;
    ch->level = JsonInt(unit, "level") > 0 ? JsonInt(unit, "level") : 1;
    if (player_side) {
        const cJSON *instance_id = cJSON_GetObjectItemCaseSensitive(unit, "instance_id");
        ch->instance_id =
            cJSON_IsNumber(instance_id) && instance_id->valuedouble > 0 ? (uint64_t)instance_id->valuedouble : 0;
    }
    if (hero_build) {
        BattleHeroBuild build;
        if (!BattleHeroBuildLoad(&build, unit) || !BattleHeroBuildApply(ch, &build)) return false;
    }
    roster->present[slot] = true;
    return true;
}

bool BattleReplayRosterLoad(BattleRoster *roster, const cJSON *catalog, const cJSON *side, bool player_side) {
    memset(roster, 0, sizeof(*roster));
    roster->hero_builds = player_side || JsonBool(side, BATTLE_HERO_BUILDS_FIELD);
    const cJSON *unit = NULL;
    cJSON_ArrayForEach(unit, cJSON_GetObjectItemCaseSensitive(side, "units")) {
        if (!BattleReplayUnitLoad(roster, catalog, unit, player_side, roster->hero_builds)) return false;
    }
    const cJSON *rune = NULL;
    cJSON_ArrayForEach(rune, cJSON_GetObjectItemCaseSensitive(side, "runes")) {
        int def = cJSON_IsString(rune) ? RuneDefIndex(rune->valuestring) : -1;
        if (def < 0 || roster->rune_def_count == RUNE_SLOT_COUNT) return false;
        roster->rune_defs[roster->rune_def_count++] = def;
    }
    return true;
}

cJSON *BattleReplayCatalogLoad(void) {
    RuneCatalogEnsure();
    GearCatalogEnsure();
    return CharacterCatalogAddCustomHeroes(JsonLoadFile(BATTLE_REPLAY_CATALOG_PATH));
}

static uint32_t BattleReplayFloatBits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static int BattleReplayActionText(const BattleSimAction *a, char text[BATTLE_REPLAY_ACTION_TEXT]) {
    int length = snprintf(
        text,
        BATTLE_REPLAY_ACTION_TEXT,
        "%d %d%d%d%d %d %d",
        a->actor,
        a->newround,
        a->crit,
        a->replay_attack,
        a->same_swing,
        (int)a->ailments,
        a->target_count
    );
    for (int t = 0; t < a->target_count; t++) {
        const BattleSimTarget *target = &a->targets[t];
        length += snprintf(
            text + length,
            BATTLE_REPLAY_ACTION_TEXT - (size_t)length,
            " %d %d %d %d %d %08x %08x",
            target->unit,
            target->health_delta,
            target->power_delta,
            target->initiative_delta,
            target->health_faction_delta,
            BattleReplayFloatBits(target->crit_delta),
            BattleReplayFloatBits(target->damage_bonus_delta)
        );
    }
    length += snprintf(text + length, BATTLE_REPLAY_ACTION_TEXT - (size_t)length, "\n");
    return length;
}

void BattleReplayDigest(const BattleSim *s, char hex[BATTLE_REPLAY_DIGEST_HEX]) {
    Sha256 hash;
    Sha256Begin(&hash);
    char text[BATTLE_REPLAY_ACTION_TEXT];
    for (int i = 0; i < s->action_count; i++)
        Sha256Update(&hash, text, (size_t)BattleReplayActionText(&s->actions[i], text));
    Sha256FinishHex(&hash, hex);
}

void BattleReplayTimelineWrite(const BattleSim *s, FILE *file) {
    char text[BATTLE_REPLAY_ACTION_TEXT];
    for (int i = 0; i < s->action_count; i++) {
        BattleReplayActionText(&s->actions[i], text);
        fprintf(file, "%d: %s", i, text);
    }
}

typedef struct {
    bool won;
    int crits;
    int rounds;
    int actions;
    char timeline[BATTLE_REPLAY_DIGEST_HEX];
    MissionBattleFacts facts;
} BattleReplayOutcome;

bool BattleReplaySimulate(const cJSON *catalog, const cJSON *replay, BattleSim *sim) {
    static BattleRoster player, enemy;
    static BattleGameChanger changer;
    const cJSON *changer_json = cJSON_GetObjectItemCaseSensitive(replay, "changer");
    if (!BattleReplayRosterLoad(&player, catalog, cJSON_GetObjectItemCaseSensitive(replay, "player"), true) ||
        !BattleReplayRosterLoad(&enemy, catalog, cJSON_GetObjectItemCaseSensitive(replay, "enemy"), false) ||
        (cJSON_IsObject(changer_json) && !BattleReplayChangerLoad(&changer, changer_json)))
        return false;
    enemy.game_changer = cJSON_IsObject(changer_json) ? &changer : NULL;
    enemy.stat_budget = JsonBool(replay, "stat_budget") ? StatBudgetEnsure() : NULL;
    const cJSON *seed = cJSON_GetObjectItemCaseSensitive(replay, "seed");
    if (!cJSON_IsNumber(seed) || seed->valuedouble < 0 || seed->valuedouble > UINT32_MAX) return false;
    BattleSimRunRosters(sim, &player, &enemy, (unsigned)seed->valuedouble);
    return true;
}

bool BattleReplayRun(const cJSON *catalog, const cJSON *replay, BattleReplayOutcome *out) {
    static BattlePlay play;
    if (!BattleReplaySimulate(catalog, replay, &play.sim)) return false;
    out->won = play.sim.player_won;
    out->crits = BattlePlayCriticalGold(&play) / BP_CRIT_GOLD_REWARD;
    out->rounds = play.sim.rounds;
    out->actions = play.sim.action_count;
    BattleReplayDigest(&play.sim, out->timeline);
    out->facts = MissionsBattleFacts(&play.sim);
    return true;
}

void BattleReplayOutcomeAdd(cJSON *entry, const BattleReplayOutcome *outcome) {
    cJSON_AddBoolToObject(entry, "won", outcome->won);
    cJSON_AddNumberToObject(entry, "crits", outcome->crits);
    cJSON_AddNumberToObject(entry, "rounds", outcome->rounds);
    cJSON_AddNumberToObject(entry, "actions", outcome->actions);
    cJSON_AddStringToObject(entry, "timeline", outcome->timeline);
    cJSON_AddNumberToObject(entry, "damage", (double)outcome->facts.damage);
    cJSON_AddBoolToObject(entry, "faction_hit", outcome->facts.faction_hit);
    cJSON_AddBoolToObject(entry, "squad_buffed", outcome->facts.squad_buffed);
}
