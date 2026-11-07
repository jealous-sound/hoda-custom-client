#define EXPEDITION_REWARD_ITEM_CAPACITY (EXPEDITION_SLOT_CAPACITY * EXPEDITION_DROP_CAPACITY)
#define EXPEDITION_ACTION_START "expedition_start"
#define EXPEDITION_ACTION_CLAIM "expedition_claim"
#define EXPEDITION_ACTION_RECALL "expedition_recall"

typedef struct {
    int expeditions;
    int great_successes;
    int gold;
    int gems;
    int hero_xp;
    int gear[EXPEDITION_REWARD_ITEM_CAPACITY];
    int gear_count;
    int runes[EXPEDITION_REWARD_ITEM_CAPACITY];
    bool rune_new[EXPEDITION_REWARD_ITEM_CAPACITY];
    int rune_count;
    int heroes[EXPEDITION_REWARD_ITEM_CAPACITY];
    bool hero_new[EXPEDITION_REWARD_ITEM_CAPACITY];
    int hero_count;
} ExpeditionReward;

typedef enum {
    EXPEDITION_CLAIM_OK,
    EXPEDITION_CLAIM_NOTHING_READY,
    EXPEDITION_CLAIM_ROSTER_FULL,
} ExpeditionClaimCheck;

bool ExpeditionHeroAvailable(const Player *p, int roster_index) {
    return roster_index >= 0 && roster_index < p->roster_count && !PlayerSquadsContain(p, roster_index) &&
           !PlayerHeroAway(p, roster_index);
}

static int ExpeditionPartyCharacters(const Player *p, const int *party, int count, const Character **out) {
    int known = 0;
    for (int i = 0; i < count && i < EXPEDITION_PARTY_CAPACITY; i++)
        if (party[i] >= 0 && party[i] < p->roster_count) out[known++] = &p->roster[party[i]];
    return known;
}

bool ExpeditionPartyMeetsRequirements(const Player *p, int def_index, const int *party, int count) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    const Character *members[EXPEDITION_PARTY_CAPACITY];
    int known = ExpeditionPartyCharacters(p, party, count, members);
    return def && ExpeditionRequirementsMet(def, members, known);
}

bool ExpeditionPartyGreatSuccess(const Player *p, int def_index, const int *party, int count) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    const Character *members[EXPEDITION_PARTY_CAPACITY];
    int known = ExpeditionPartyCharacters(p, party, count, members);
    return def && ExpeditionGreatSuccess(def, members, known);
}

bool ExpeditionCanStart(const Player *p, int def_index) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    return ExpeditionUnlocked(def, p->level) && ExpeditionsFind(&p->expeditions, def_index) == EXPEDITION_NONE &&
           p->expeditions.count < ExpeditionSlotsUnlocked(p->level);
}

bool ExpeditionPartyValid(const Player *p, int def_index, const int *party, int count) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    if (!def || count != def->party) return false;
    for (int i = 0; i < count; i++) {
        if (!ExpeditionHeroAvailable(p, party[i])) return false;
        for (int other = 0; other < i; other++)
            if (party[other] == party[i]) return false;
    }
    return ExpeditionPartyMeetsRequirements(p, def_index, party, count);
}

static int ExpeditionHeroWorth(const Character *ch) {
    return ((int)ch->rarity * TIER_COUNT + (int)ch->tier) * 1000 + ch->level;
}

static int ExpeditionCheapestCandidate(const Player *p, const ExpeditionTrait *trait, const bool *taken) {
    int best = EXPEDITION_NONE;
    for (int i = 0; i < p->roster_count; i++) {
        if (taken[i] || !ExpeditionHeroAvailable(p, i)) continue;
        if (trait && !ExpeditionTraitMatches(trait, &p->roster[i])) continue;
        if (best < 0 || ExpeditionHeroWorth(&p->roster[i]) < ExpeditionHeroWorth(&p->roster[best])) best = i;
    }
    return best;
}

static void ExpeditionFillTrait(
    const Player *p, const ExpeditionTrait *trait, int party_size, int *party, int *count, bool *taken
) {
    const Character *members[EXPEDITION_PARTY_CAPACITY];
    while (*count < party_size) {
        int known = ExpeditionPartyCharacters(p, party, *count, members);
        if (ExpeditionTraitMet(trait, members, known)) return;
        int pick = ExpeditionCheapestCandidate(p, trait, taken);
        if (pick < 0) return;
        taken[pick] = true;
        party[(*count)++] = pick;
    }
}

int ExpeditionAutoFill(const Player *p, int def_index, int *party) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    if (!def || p->roster_count <= 0) return 0;
    bool *taken = (bool *)MemAlloc((unsigned int)p->roster_count);
    int count = 0;
    for (int i = 0; i < def->requirement_count; i++)
        ExpeditionFillTrait(p, &def->requirements[i], def->party, party, &count, taken);
    ExpeditionFillTrait(p, &def->bonus, def->party, party, &count, taken);
    while (count < def->party) {
        int pick = ExpeditionCheapestCandidate(p, NULL, taken);
        if (pick < 0) break;
        taken[pick] = true;
        party[count++] = pick;
    }
    MemFree(taken);
    return count;
}

bool ExpeditionStart(Player *p, int def_index, const int *party, int count, long long now) {
    if (!ExpeditionCanStart(p, def_index) || !ExpeditionPartyValid(p, def_index, party, count)) return false;
    PlayerExpedition *expedition = &p->expeditions.active[p->expeditions.count++];
    *expedition = (PlayerExpedition){
        .def = def_index,
        .hero_count = count,
        .started_at = now,
        .ends_at = now + ExpeditionDurationSeconds(ExpeditionAt(def_index)),
    };
    for (int i = 0; i < count; i++) expedition->heroes[i] = p->roster[party[i]].instance_id;
    return true;
}

cJSON *ExpeditionStartAction(const Player *p, int def_index, const int *party, int count) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    cJSON *fields = def ? ServerActionCreate(EXPEDITION_ACTION_START) : NULL;
    cJSON *heroes = fields ? cJSON_AddArrayToObject(fields, "heroes") : NULL;
    if (!heroes || !cJSON_AddStringToObject(fields, "expedition", def->id)) {
        cJSON_Delete(fields);
        return NULL;
    }
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(heroes, cJSON_CreateNumber((double)p->roster[party[i]].instance_id));
    return fields;
}

bool ExpeditionRecall(Player *p, int def_index) {
    int active = ExpeditionsFind(&p->expeditions, def_index);
    if (active < 0) return false;
    ExpeditionsRemoveAt(&p->expeditions, active);
    return true;
}

cJSON *ExpeditionRecallAction(int def_index) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    cJSON *fields = def ? ServerActionCreate(EXPEDITION_ACTION_RECALL) : NULL;
    if (fields) cJSON_AddStringToObject(fields, "expedition", def->id);
    return fields;
}

static int ExpeditionRosterIndexOf(const Player *p, uint64_t instance) {
    for (int i = 0; i < p->roster_count; i++)
        if (p->roster[i].instance_id == instance) return i;
    return EXPEDITION_NONE;
}

static int ExpeditionActivePartyIndices(const Player *p, const PlayerExpedition *expedition, int *party) {
    int count = 0;
    for (int i = 0; i < expedition->hero_count; i++) {
        int index = ExpeditionRosterIndexOf(p, expedition->heroes[i]);
        if (index >= 0) party[count++] = index;
    }
    return count;
}

bool ExpeditionActiveGreatSuccess(const Player *p, const PlayerExpedition *expedition) {
    int party[EXPEDITION_PARTY_CAPACITY];
    int count = ExpeditionActivePartyIndices(p, expedition, party);
    return ExpeditionPartyGreatSuccess(p, expedition->def, party, count);
}

int ExpeditionReadyDefs(const Player *p, long long now, int *defs) {
    int count = 0;
    for (int i = 0; i < p->expeditions.count; i++)
        if (ExpeditionReady(&p->expeditions.active[i], now)) defs[count++] = p->expeditions.active[i].def;
    return count;
}

ExpeditionClaimCheck ExpeditionCheckClaim(const Player *p, const int *defs, int count, long long now) {
    int ready = 0, hero_room = 0;
    for (int i = 0; i < count; i++) {
        int active = ExpeditionsFind(&p->expeditions, defs[i]);
        if (active < 0 || !ExpeditionReady(&p->expeditions.active[active], now)) continue;
        ready++;
        hero_room += ExpeditionHeroDropLimit(ExpeditionAt(defs[i]));
    }
    if (ready == 0) return EXPEDITION_CLAIM_NOTHING_READY;
    return PlayerRosterHasCapacity(p, hero_room) ? EXPEDITION_CLAIM_OK : EXPEDITION_CLAIM_ROSTER_FULL;
}

static int ExpeditionRollBetween(uint32_t *rng, int low, int high) {
    return high > low ? low + StoreRngRange(rng, high - low + 1) : low;
}

static Rarity ExpeditionRollHeroRarity(const ExpeditionDrop *drop, uint32_t *rng) {
    int total = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) total += drop->hero_weights[rarity];
    int roll = StoreRngRange(rng, total);
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) {
        if (roll < drop->hero_weights[rarity]) return (Rarity)rarity;
        roll -= drop->hero_weights[rarity];
    }
    return RARITY_LEGENDARY;
}

static const cJSON *ExpeditionRollHeroDefinition(const cJSON *catalog, Rarity rarity, uint32_t *rng) {
    int pool = 0;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)catalog) pool +=
        StoreHeroDefinitionEligible(entry) && RarityFromString(JsonStr(entry, "rarity")) == rarity;
    int pick = StoreRngRange(rng, pool);
    cJSON_ArrayForEach(entry, (cJSON *)catalog) {
        if (!StoreHeroDefinitionEligible(entry) || RarityFromString(JsonStr(entry, "rarity")) != rarity) continue;
        if (pick-- == 0) return entry;
    }
    return NULL;
}

static bool ExpeditionOwnedBefore(const Player *p, int index) {
    for (int other = 0; other < index; other++)
        if (!strcmp(p->roster[other].id, p->roster[index].id)) return true;
    return false;
}

static void ExpeditionRecordHero(ExpeditionReward *reward, const Player *p, int index) {
    if (index < 0 || reward->hero_count == EXPEDITION_REWARD_ITEM_CAPACITY) return;
    reward->heroes[reward->hero_count] = index;
    reward->hero_new[reward->hero_count++] = !ExpeditionOwnedBefore(p, index);
}

static void ExpeditionRollDrop(
    Player *p, const ExpeditionDrop *drop, const cJSON *catalog, long long now, ExpeditionReward *reward
) {
    uint32_t *rng = &p->store_rng_state;
    if (drop->kind == EXPEDITION_DROP_GEAR) {
        GearPackDef roll = {.min_rarity = drop->min_rarity, .max_rarity = drop->max_rarity};
        int gear = GearRollFromPack(&roll, rng);
        if (gear >= 0 && GearGrant(&p->gear, gear) && reward->gear_count < EXPEDITION_REWARD_ITEM_CAPACITY)
            reward->gear[reward->gear_count++] = gear;
    } else if (drop->kind == EXPEDITION_DROP_RUNE) {
        StoreRuneSlot slot = {.min_rarity = drop->min_rarity, .max_rarity = drop->max_rarity};
        int rune = StoreRollRune(&slot, rng);
        bool owned = rune >= 0 && StoreRuneDefOwned(&p->runes, rune);
        if (rune >= 0 && RuneGrant(&p->runes, rune, now) && reward->rune_count < EXPEDITION_REWARD_ITEM_CAPACITY) {
            reward->rune_new[reward->rune_count] = !owned;
            reward->runes[reward->rune_count++] = rune;
        }
    } else {
        const cJSON *definition = ExpeditionRollHeroDefinition(catalog, ExpeditionRollHeroRarity(drop, rng), rng);
        int first = EXPEDITION_NONE;
        if (definition && PlayerRosterAppendDefinitions(p, &definition, 1, &first))
            ExpeditionRecordHero(reward, p, first);
    }
}

static void ExpeditionPayOne(Player *p, int active, const cJSON *catalog, long long now, ExpeditionReward *reward) {
    PlayerExpedition expedition = p->expeditions.active[active];
    const ExpeditionDef *def = ExpeditionAt(expedition.def);
    int party[EXPEDITION_PARTY_CAPACITY];
    int party_count = ExpeditionActivePartyIndices(p, &expedition, party);
    bool great = ExpeditionPartyGreatSuccess(p, expedition.def, party, party_count);
    ExpeditionsRemoveAt(&p->expeditions, active);
    uint32_t *rng = &p->store_rng_state;
    int gold = ExpeditionGreatSuccessAmount(ExpeditionRollBetween(rng, def->gold_min, def->gold_max), great);
    int gems = ExpeditionGreatSuccessAmount(ExpeditionRollBetween(rng, def->gems_min, def->gems_max), great);
    int hero_xp = ExpeditionGreatSuccessAmount(def->hero_xp, great);
    p->gold = ProgressionSaturatingAdd(p->gold, gold);
    p->gems = ProgressionSaturatingAdd(p->gems, gems);
    for (int i = 0; i < party_count; i++) ConsumeAwardHeroXp(&p->roster[party[i]], hero_xp);
    for (int i = 0; i < def->drop_count; i++) {
        const ExpeditionDrop *drop = &def->drops[i];
        int chance = great ? EXPEDITION_PERCENT : drop->chance;
        if (StoreRngRange(rng, EXPEDITION_PERCENT) < chance) ExpeditionRollDrop(p, drop, catalog, now, reward);
    }
    reward->expeditions++;
    reward->great_successes += great;
    reward->gold = ProgressionSaturatingAdd(reward->gold, gold);
    reward->gems = ProgressionSaturatingAdd(reward->gems, gems);
    reward->hero_xp = hero_xp > reward->hero_xp ? hero_xp : reward->hero_xp;
}

ExpeditionReward ExpeditionClaim(Player *p, const int *defs, int count, long long now) {
    ExpeditionReward reward = {0};
    if (ExpeditionCheckClaim(p, defs, count, now) != EXPEDITION_CLAIM_OK) return reward;
    cJSON *root = ProgressionLoadCharacterCatalog();
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    for (int i = 0; i < count; i++) {
        int active = ExpeditionsFind(&p->expeditions, defs[i]);
        if (active >= 0 && ExpeditionReady(&p->expeditions.active[active], now))
            ExpeditionPayOne(p, active, catalog, now, &reward);
    }
    cJSON_Delete(root);
    return reward;
}

cJSON *ExpeditionClaimAction(const int *defs, int count) {
    cJSON *fields = ServerActionCreate(EXPEDITION_ACTION_CLAIM);
    cJSON *ids = fields ? cJSON_AddArrayToObject(fields, "expeditions") : NULL;
    if (!ids) {
        cJSON_Delete(fields);
        return NULL;
    }
    for (int i = 0; i < count; i++) {
        const ExpeditionDef *def = ExpeditionAt(defs[i]);
        if (def) cJSON_AddItemToArray(ids, cJSON_CreateString(def->id));
    }
    return fields;
}

static int ExpeditionRuneCopies(const PlayerRunes *runes, int def) {
    int copies = 0;
    for (int i = 0; i < runes->count; i++) copies += runes->items[i].def == def;
    return copies;
}

static int ExpeditionRewardRuneCopies(const ExpeditionReward *reward, int def) {
    int copies = 0;
    for (int i = 0; i < reward->rune_count; i++) copies += reward->runes[i] == def;
    return copies;
}

ExpeditionReward ExpeditionRewardFromReply(const Player *p, const cJSON *reply, int expeditions, int great) {
    ExpeditionReward reward = {.expeditions = expeditions, .great_successes = great};
    const cJSON *result = cJSON_GetObjectItemCaseSensitive(reply, "granted");
    reward.gold = JsonInt(result, "gold");
    reward.gems = JsonInt(result, "gems");
    reward.hero_xp = JsonInt(result, "xp");
    const cJSON *granted = cJSON_GetObjectItemCaseSensitive(result, "granted");
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(granted, "heroes")) {
        ExpeditionRecordHero(&reward, p, ExpeditionRosterIndexOf(p, (uint64_t)JsonFloat(item, "instance_id")));
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(granted, "gear")) {
        int gear = cJSON_IsString(item) ? GearDefIndex(item->valuestring) : -1;
        if (gear >= 0 && reward.gear_count < EXPEDITION_REWARD_ITEM_CAPACITY) reward.gear[reward.gear_count++] = gear;
    }
    cJSON_ArrayForEach(item, cJSON_GetObjectItemCaseSensitive(granted, "runes")) {
        int rune = cJSON_IsString(item) ? RuneDefIndex(item->valuestring) : -1;
        if (rune >= 0 && reward.rune_count < EXPEDITION_REWARD_ITEM_CAPACITY) reward.runes[reward.rune_count++] = rune;
    }
    for (int i = 0; i < reward.rune_count; i++)
        reward.rune_new[i] =
            ExpeditionRuneCopies(&p->runes, reward.runes[i]) <= ExpeditionRewardRuneCopies(&reward, reward.runes[i]);
    return reward;
}

bool ExpeditionRewardHasItems(const ExpeditionReward *reward) {
    return reward->hero_count + reward->gear_count + reward->rune_count > 0;
}
