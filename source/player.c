#define SQUAD_SLOT_COUNT 5
#define SQUAD_SLOT_LARGE 4
#define PLAYER_REGEN_PERIOD 300.0f
#define ENERGY_REFILL_GEM_COST 3
#define STAMINA_REFILL_GEM_COST 3
#define PLAYER_ROSTER_CAPACITY 1024
#define PLAYER_STORE_PACK_COUNT 4
#define PLAYER_VOUCHER_CAPACITY 16
#define PLAYER_INSTANCE_ID_MAX 9007199254740991ULL
#define PLAYER_NAME_MAX_CHARACTERS 25
#define PLAYER_NAME_CAPACITY (PLAYER_NAME_MAX_CHARACTERS * 4 + 1)

int SquadSlotReferencePosition(int slot) {
    static const int reference_position[SQUAD_SLOT_COUNT] = {2, 0, 3, 1, -1};
    if (slot < 0 || slot >= SQUAD_SLOT_COUNT) return -1;
    return reference_position[slot];
}

int SquadSlotReferenceX(int slot) {
    int position = SquadSlotReferencePosition(slot);
    return position >= 0 ? position / 2 : -1;
}

int SquadSlotReferenceY(int slot) {
    int position = SquadSlotReferencePosition(slot);
    return position >= 0 ? position % 2 : -1;
}

bool SquadSlotIsFront(int slot) {
    return SquadSlotReferenceX(slot) == 0;
}

bool SquadSlotIsBack(int slot) {
    return SquadSlotReferenceX(slot) == 1;
}

typedef struct {
    bool active;
    bool unlock_next;
    int from_map;
    int from_node;
    int to_map;
    int to_node;
} PlayerCampaignAnimation;

typedef struct {
    int campaign_index;
    int node_index;
    int node_completions[CAMPAIGN_MAX_MAPS][CAMPAIGN_MAX_NODES];
    int node_losses[CAMPAIGN_MAX_MAPS][CAMPAIGN_MAX_NODES];
    PlayerCampaignAnimation pending_animation;
} PlayerCampaignProgress;

typedef struct {
    int wins;
    int losses;
    int previous_wins;
} PlayerNexusNode;

typedef struct {
    char map_id[80];
    int event_index;
    int difficulty_index;
    int current_node_step;
    int selected_node_step;
    bool committed;
    bool reset;
    PlayerNexusNode nodes[NEXUS_MAX_NODES];
    int escrow_gold, escrow_xp, escrow_potions;
    int escrow_units[7];
    bool pending_animation;
    int pending_from_step, pending_to_step;
} PlayerNexusRun;

typedef struct {
    int completions[NEXUS_MAX_EVENTS][NEXUS_MAX_DIFFICULTIES];
    long long unlocked_at[NEXUS_MAX_EVENTS];
    int remaining_runs[NEXUS_MAX_EVENTS];
} PlayerNexusEvents;

typedef struct {
    bool main_complete;
    bool consume_complete;
    int restore_step;
} PlayerTutorial;

typedef struct {
    int level;
    int experience;
    int experience_min;
    int experience_max;
    int energy;
    int energy_max;
    int stamina;
    int stamina_max;
    float energy_regen_timer, stamina_regen_timer;
    int gold;
    int gems;

    Character *roster;
    Ability *roster_abilities;
    int roster_count;
    int roster_capacity;
    int starter_roster_count;
    uint64_t next_instance_id;
    int squads[PLAYER_MAX_SQUADS][SQUAD_SLOT_COUNT];
    int current_squad;

    uint32_t store_rng_state;
    int store_opened_total;
    int store_opened_by_pack[PLAYER_STORE_PACK_COUNT];
    int champion_since_legendary;
    int pack_vouchers[PLAYER_VOUCHER_CAPACITY];

    PlayerCampaignProgress campaign_progress;

    int potions;
    int potion_max;
    int keys[NEXUS_KEY_COUNT];
    PlayerNexusRun nexus_run;
    PlayerNexusEvents nexus_events;
    long long news_seen_at;

    PlayerRunes runes;
    PlayerGear gear;
    PlayerGallery gallery;
    PlayerMissions missions;
    PlayerPvp pvp;

    char name[PLAYER_NAME_CAPACITY];
    PlayerTutorial tutorial;
    bool fast_battles;
} Player;

static int PlayerNameNextCodepoint(const unsigned char **cursor) {
    const unsigned char *c = *cursor;
    int length = c[0] < 0x80             ? 1
                 : (c[0] & 0xE0) == 0xC0 ? 2
                 : (c[0] & 0xF0) == 0xE0 ? 3
                 : (c[0] & 0xF8) == 0xF0 ? 4
                                         : 0;
    if (length == 0) return -1;
    int codepoint = length == 1 ? c[0] : c[0] & (0x7F >> length);
    for (int i = 1; i < length; i++) {
        if ((c[i] & 0xC0) != 0x80) return -1;
        codepoint = (codepoint << 6) | (c[i] & 0x3F);
    }
    *cursor = c + length;
    return codepoint;
}

static bool PlayerNameCodepointAllowed(int c) {
    bool ascii = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ' || c == '-';
    bool latin = c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7;
    bool greek = c >= 0x386 && c <= 0x3FF;
    bool cyrillic = c >= 0x400 && c <= 0x52F;
    return ascii || latin || greek || cyrillic;
}

bool PlayerNameValid(const char *name) {
    if (!name || !name[0] || name[0] == ' ') return false;
    const unsigned char *cursor = (const unsigned char *)name;
    int count = 0, last = 0;
    while (*cursor) {
        last = PlayerNameNextCodepoint(&cursor);
        if (last < 0 || !PlayerNameCodepointAllowed(last) || ++count > PLAYER_NAME_MAX_CHARACTERS) return false;
    }
    return last != ' ';
}

static cJSON *CatalogFindCharacter(const cJSON *characters, const char *id) {
    cJSON *c = NULL;
    cJSON_ArrayForEach(c, (cJSON *)characters) {
        const char *cid = JsonStr(c, "id");
        if (cid[0] && !strcmp(cid, id)) return c;
    }
    return NULL;
}

static int PlayerRosterIndex(const Player *p, const char *id) {
    for (int i = 0; i < p->roster_count; i++) {
        if (!strcmp(p->roster[i].id, id)) return i;
    }
    return -1;
}

int PlayerRosterDefinitionCount(const Player *p, const char *id) {
    if (!p || !p->roster || !id || !id[0]) return 0;
    int count = 0;
    for (int i = 0; i < p->roster_count; i++) count += strcmp(p->roster[i].id, id) == 0;
    return count;
}

bool PlayerRosterHasCapacity(const Player *p, int additions) {
    if (!p || additions < 0 || !p->roster || !p->roster_abilities) return false;
    if (p->roster_count < 0 || p->roster_capacity < p->roster_count) return false;
    return additions <= p->roster_capacity - p->roster_count;
}

static uint64_t PlayerRosterNextAvailableInstanceId(const Player *p) {
    uint64_t next = p->next_instance_id ? p->next_instance_id : 1;
    for (int i = 0; i < p->roster_count; i++) {
        if (p->roster[i].instance_id >= next) {
            if (p->roster[i].instance_id == UINT64_MAX) return 0;
            next = p->roster[i].instance_id + 1;
        }
    }
    return next;
}

static bool PlayerRosterParseDefinition(const cJSON *def, Character *ch, Ability *ability) {
    if (!cJSON_IsObject(def)) return false;
    const char *id = JsonStr(def, "id");
    if (!id[0] || strlen(id) >= sizeof(ch->id)) return false;

    cJSON *groups = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "groups");
    cJSON *specialties = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "specialties");
    cJSON *ability_json = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "ability");
    cJSON *mechanics = cJSON_GetObjectItemCaseSensitive(ability_json, "mechanics");
    cJSON *tiers = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "tiers");
    if ((groups && !cJSON_IsArray(groups)) || (specialties && !cJSON_IsArray(specialties)) ||
        (mechanics && !cJSON_IsArray(mechanics)) || !cJSON_IsObject(tiers))
        return false;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, groups) if (!cJSON_IsString(item)) return false;
    cJSON_ArrayForEach(item, specialties) if (!cJSON_IsString(item)) return false;
    cJSON_ArrayForEach(item, mechanics) if (!cJSON_IsString(item)) return false;
    cJSON_ArrayForEach(item, tiers) {
        if (!item->string) return false;
        Tier tier = TierFromString(item->string);
        if (strcmp(item->string, TIER_NAMES[tier]) != 0) return false;
    }

    CharacterParse(def, ch, ability);
    for (int tier = 0; tier < TIER_COUNT; tier++) {
        if (!ch->has_tier[tier] || ch->stats[tier].max_level < 1) continue;
        ch->tier = (Tier)tier;
        ch->level = 1;
        return true;
    }
    return false;
}

static void PlayerRosterClearRange(Player *p, int first, int end) {
    for (int i = first; i < end; i++) {
        memset(&p->roster[i], 0, sizeof(*p->roster));
        memset(&p->roster_abilities[i], 0, sizeof(*p->roster_abilities));
    }
}

bool PlayerRosterAppendDefinitions(Player *p, const cJSON *const *defs, int count, int *out_first_index) {
    if (out_first_index) *out_first_index = -1;
    if (!p || count <= 0 || !defs || !PlayerRosterHasCapacity(p, count)) return false;

    uint64_t first_id = PlayerRosterNextAvailableInstanceId(p);
    if (!first_id || first_id > PLAYER_INSTANCE_ID_MAX || (uint64_t)count > PLAYER_INSTANCE_ID_MAX - first_id)
        return false;
    int first = p->roster_count;
    for (int n = 0; n < count; n++) {
        int i = first + n;
        if (!defs[n] || !PlayerRosterParseDefinition(defs[n], &p->roster[i], &p->roster_abilities[i])) {
            PlayerRosterClearRange(p, first, i + 1);
            return false;
        }
        p->roster[i].instance_id = first_id + (uint64_t)n;
        p->roster[i].ability = &p->roster_abilities[i];
    }

    for (int n = 0; n < count; n++) ConsumeNormalizeCharacter(&p->roster[first + n]);
    p->roster_count += count;
    p->next_instance_id = first_id + (uint64_t)count;
    if (out_first_index) *out_first_index = first;
    return true;
}

static void PlayerRosterReset(Player *p) {
    p->roster = NULL;
    p->roster_abilities = NULL;
    p->roster_count = 0;
    p->roster_capacity = 0;
    p->starter_roster_count = 0;
    p->next_instance_id = 1;
    p->current_squad = 0;
    for (int q = 0; q < PLAYER_MAX_SQUADS; q++) {
        for (int s = 0; s < SQUAD_SLOT_COUNT; s++) p->squads[q][s] = -1;
    }
}

void PlayerLoadRoster(Player *p, const char *catalog_path, const char *player_path) {
    PlayerRosterReset(p);

    cJSON *player_root = JsonLoadFile(player_path);
    cJSON *catalog_root = CharacterCatalogAddCustomHeroes(JsonLoadFile(catalog_path));

    cJSON *entries = cJSON_GetObjectItemCaseSensitive(player_root, "characters");
    cJSON *catalog = cJSON_GetObjectItemCaseSensitive(catalog_root, "characters");
    int total = cJSON_GetArraySize(entries);
    if (total <= 0) {
        cJSON_Delete(catalog_root);
        cJSON_Delete(player_root);
        return;
    }

    if (total > PLAYER_ROSTER_CAPACITY)
        TraceLog(LOG_FATAL, "Roster: %d starters exceed fixed capacity %d", total, PLAYER_ROSTER_CAPACITY);

    p->roster_capacity = PLAYER_ROSTER_CAPACITY;
    p->roster = (Character *)MemAlloc((unsigned int)((size_t)p->roster_capacity * sizeof(Character)));
    p->roster_abilities = (Ability *)MemAlloc((unsigned int)((size_t)p->roster_capacity * sizeof(Ability)));
    if (!p->roster || !p->roster_abilities) TraceLog(LOG_FATAL, "Roster: failed to reserve the fixed pool");
    memset(p->roster, 0, (size_t)p->roster_capacity * sizeof(Character));
    memset(p->roster_abilities, 0, (size_t)p->roster_capacity * sizeof(Ability));

    int entry_index = 0;
    cJSON *e = NULL;
    cJSON_ArrayForEach(e, entries) {
        int index = entry_index++;
        const char *id = JsonStr(e, "id");
        if (!id[0]) TraceLog(LOG_FATAL, "Roster: entry %d in %s has no id", index, player_path);
        cJSON *def = CatalogFindCharacter(catalog, id);
        if (!def) TraceLog(LOG_FATAL, "Roster: '%s' not found in %s", id, catalog_path);

        int i = p->roster_count;
        CharacterParse(def, &p->roster[i], &p->roster_abilities[i]);
        p->roster[i].instance_id = p->next_instance_id++;

        Character *ch = &p->roster[i];
        const char *tier_name = JsonStr(e, "tier");
        ch->tier = TIER_I;
        if (tier_name[0]) {
            ch->tier = TierFromString(tier_name);
            if (strcmp(tier_name, TIER_NAMES[ch->tier]) != 0)
                TraceLog(LOG_FATAL, "Roster: %s has unknown tier '%s'", id, tier_name);
        }
        if (!ch->has_tier[ch->tier])
            TraceLog(LOG_FATAL, "Roster: %s has no tier %s in the catalog", id, TIER_NAMES[ch->tier]);

        cJSON *level_item = cJSON_GetObjectItemCaseSensitive(e, "level");
        ch->level = cJSON_IsNumber(level_item) ? level_item->valueint : 1;
        int max_level = ch->stats[ch->tier].max_level;
        if (ch->level < 1 || ch->level > max_level)
            TraceLog(
                LOG_FATAL,
                "Roster: %s level %d outside 1..%d for tier %s",
                id,
                ch->level,
                max_level,
                TIER_NAMES[ch->tier]
            );

        p->roster_count++;
        TraceLog(LOG_INFO, "  owned: %s (%s)", p->roster[i].name, p->roster[i].id);
    }
    p->starter_roster_count = p->roster_count;
    for (int i = 0; i < p->roster_count; i++) ConsumeNormalizeCharacter(&p->roster[i]);

    cJSON *squads = cJSON_GetObjectItemCaseSensitive(player_root, "squads");
    int squad_index = 0;
    cJSON *sq = NULL;
    cJSON_ArrayForEach(sq, squads) {
        int q = squad_index++;
        if (q >= PLAYER_MAX_SQUADS)
            TraceLog(LOG_FATAL, "Roster: more than %d squads in %s", PLAYER_MAX_SQUADS, player_path);
        if (cJSON_GetArraySize(sq) > SQUAD_SLOT_COUNT)
            TraceLog(LOG_FATAL, "Roster: squad %d has more than %d slots", q + 1, SQUAD_SLOT_COUNT);

        int slot = 0;
        cJSON *cell = NULL;
        cJSON_ArrayForEach(cell, sq) {
            int s = slot++;
            if (!cJSON_IsString(cell)) continue;
            const char *id = cell->valuestring;
            int i = PlayerRosterIndex(p, id);
            if (i < 0) TraceLog(LOG_FATAL, "Roster: squad %d slot %d refers to unowned %s", q + 1, s, id);
            for (int prev = 0; prev < s; prev++) {
                if (p->squads[q][prev] == i) TraceLog(LOG_FATAL, "Roster: %s twice in squad %d", id, q + 1);
            }
            if (p->roster[i].giant != (s == SQUAD_SLOT_LARGE))
                TraceLog(
                    LOG_FATAL,
                    "Roster: %s is %s but stands on the %s pad of squad %d",
                    id,
                    p->roster[i].giant ? "giant" : "not giant",
                    s == SQUAD_SLOT_LARGE ? "large" : "small",
                    q + 1
                );
            p->squads[q][s] = i;
            TraceLog(LOG_INFO, "  squad %d slot %d: %s", q + 1, s, id);
        }
    }

    cJSON *current = cJSON_GetObjectItemCaseSensitive(player_root, "current_squad");
    p->current_squad = cJSON_IsNumber(current) ? current->valueint : 0;
    if (p->current_squad < 0 || p->current_squad >= PLAYER_MAX_SQUADS)
        TraceLog(LOG_FATAL, "Roster: current_squad %d outside 0..%d", p->current_squad, PLAYER_MAX_SQUADS - 1);

    cJSON_Delete(catalog_root);
    cJSON_Delete(player_root);

    p->campaign_progress = (PlayerCampaignProgress){0};
}

const Character *PlayerSquadCharacter(const Player *p, int slot) {
    if (slot < 0 || slot >= SQUAD_SLOT_COUNT || p->squads[p->current_squad][slot] < 0) return NULL;
    return &p->roster[p->squads[p->current_squad][slot]];
}

int PlayerSquadUnitAt(const Player *p, int squad, int slot) {
    if (squad < 0 || squad >= PLAYER_MAX_SQUADS || slot < 0 || slot >= SQUAD_SLOT_COUNT) return -1;
    return p->squads[squad][slot];
}

int PlayerSquadCount(const Player *p, int squad) {
    int count = 0;
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) count += PlayerSquadUnitAt(p, squad, s) >= 0;
    return count;
}

void PlayerSquadSelect(Player *p, int squad) {
    if (squad < 0 || squad >= PLAYER_MAX_SQUADS) return;
    p->current_squad = squad;
}

int PlayerSquadFindUnit(const Player *p, int squad, int roster_index) {
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        if (p->squads[squad][s] == roster_index) return s;
    }
    return -1;
}

bool PlayerSquadPlace(Player *p, int squad, int slot, int roster_index) {
    if (squad < 0 || squad >= PLAYER_MAX_SQUADS) return false;
    if (slot < 0 || slot >= SQUAD_SLOT_COUNT) return false;
    if (roster_index < 0 || roster_index >= p->roster_count) return false;
    if (p->roster[roster_index].giant != (slot == SQUAD_SLOT_LARGE)) return false;

    int from = PlayerSquadFindUnit(p, squad, roster_index);
    if (from == slot) return true;
    if (from >= 0) p->squads[squad][from] = p->squads[squad][slot];
    p->squads[squad][slot] = roster_index;
    return true;
}

void PlayerSquadRemove(Player *p, int squad, int slot) {
    if (squad < 0 || squad >= PLAYER_MAX_SQUADS) return;
    if (slot < 0 || slot >= SQUAD_SLOT_COUNT) return;
    p->squads[squad][slot] = -1;
}

bool PlayerSquadWithdraw(Player *p, int squad, int slot) {
    if (PlayerSquadUnitAt(p, squad, slot) < 0) return false;
    if (squad <= 1 && PlayerSquadCount(p, squad) == 1) return false;
    PlayerSquadRemove(p, squad, slot);
    return true;
}

bool PlayerSquadsContain(const Player *p, int roster_index) {
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++)
        if (PlayerSquadFindUnit(p, squad, roster_index) >= 0) return true;
    return false;
}

static int PlayerRosterCompact(Player *p, const bool *remove, int keep_index) {
    int *remap = (int *)MemAlloc((unsigned int)((size_t)p->roster_count * sizeof(int)));
    int kept = 0, kept_starters = 0;
    for (int i = 0; i < p->roster_count; i++) {
        if (remove[i]) {
            remap[i] = -1;
            continue;
        }
        remap[i] = kept;
        if (i < p->starter_roster_count) kept_starters++;
        if (kept != i) {
            p->roster[kept] = p->roster[i];
            p->roster_abilities[kept] = p->roster_abilities[i];
            p->roster[kept].ability = &p->roster_abilities[kept];
        }
        kept++;
    }
    PlayerRosterClearRange(p, kept, p->roster_count);
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            int index = p->squads[squad][slot];
            if (index >= 0) p->squads[squad][slot] = remap[index];
        }
    }
    int new_keep_index = remap[keep_index];
    MemFree(remap);
    p->roster_count = kept;
    p->starter_roster_count = kept_starters;
    if (p->gallery.synced_roster_count > kept) p->gallery.synced_roster_count = kept;
    return new_keep_index;
}

bool PlayerConsumeSeedAllowed(const Player *p, int target, int seed) {
    return seed >= 0 && seed < p->roster_count && seed != target && !PlayerSquadsContain(p, seed);
}

int PlayerConsumeUnits(Player *p, int target, const int *seeds, int count) {
    if (!p || target < 0 || target >= p->roster_count || !seeds || count <= 0) return -1;
    bool *remove = (bool *)MemAlloc((unsigned int)p->roster_count);
    const Character **seed_units = (const Character **)MemAlloc((unsigned int)((size_t)count * sizeof(*seed_units)));
    bool valid = true;
    for (int i = 0; i < count && valid; i++) {
        valid = PlayerConsumeSeedAllowed(p, target, seeds[i]) && !remove[seeds[i]];
        if (!valid) break;
        remove[seeds[i]] = true;
        seed_units[i] = &p->roster[seeds[i]];
    }
    int result = -1;
    if (valid) {
        Character *ch = &p->roster[target];
        ConsumeOutcome outcome = ConsumeCompute(ch, seed_units, count);
        ch->xp = outcome.xp;
        ch->level = outcome.level;
        ch->consume_xp = outcome.consume_xp;
        ch->consume_level = outcome.consume_level;
        ch->tier = outcome.tier;
        ch->crit_chance = outcome.crit_chance;
        GalleryRecordOwned(&p->gallery, ch->id, ch->tier, false);
        p->missions.counters[MISSION_STAT_CONSUMED] += count;
        for (int i = 0; i < count; i++) {
            if (!strcmp(seed_units[i]->id, ch->id)) p->missions.counters[MISSION_STAT_COMBINED]++;
        }
        result = PlayerRosterCompact(p, remove, target);
    }
    MemFree(seed_units);
    MemFree(remove);
    return result;
}

void PlayerGallerySync(Player *p) {
    bool initial = !p->gallery.seeded;
    for (int i = p->gallery.synced_roster_count; i < p->roster_count; i++)
        GalleryRecordOwned(&p->gallery, p->roster[i].id, p->roster[i].tier, initial);
    p->gallery.seeded = true;
    p->gallery.synced_roster_count = p->roster_count;
}

void PlayerUnloadRoster(Player *p) {
    GalleryFree(&p->gallery);
    MemFree(p->roster);
    MemFree(p->roster_abilities);
    PlayerRosterReset(p);
}

static void PlayerRegenPool(int *value, int max, float *timer, float dt) {
    if (*value >= max) {
        *timer = 0.0f;
        return;
    }
    *timer += dt;
    while (*timer >= PLAYER_REGEN_PERIOD && *value < max) {
        *timer -= PLAYER_REGEN_PERIOD;
        (*value)++;
    }
    if (*value >= max) *timer = 0.0f;
}

void PlayerUpdate(Player *p, float dt) {
    PlayerRegenPool(&p->energy, p->energy_max, &p->energy_regen_timer, dt);
    PlayerRegenPool(&p->stamina, p->stamina_max, &p->stamina_regen_timer, dt);
    RuneExpire(&p->runes, RuneNow());
}

static float PlayerRegenRemaining(int value, int max, float timer) {
    if (value >= max) return 0.0f;
    return PLAYER_REGEN_PERIOD - timer;
}

float PlayerEnergyRegenRemaining(const Player *p) {
    return PlayerRegenRemaining(p->energy, p->energy_max, p->energy_regen_timer);
}

float PlayerStaminaRegenRemaining(const Player *p) {
    return PlayerRegenRemaining(p->stamina, p->stamina_max, p->stamina_regen_timer);
}

bool PlayerSpendEnergy(Player *p, int cost) {
    if (cost < 0 || p->energy < cost) return false;
    p->energy -= cost;
    return true;
}

bool PlayerSpendStamina(Player *p, int cost) {
    if (cost < 0 || p->stamina < cost) return false;
    p->stamina -= cost;
    return true;
}

bool PlayerSpendGold(Player *p, int cost) {
    if (!p || cost < 0 || p->gold < cost) return false;
    p->gold -= cost;
    return true;
}

bool PlayerSpendGems(Player *p, int cost) {
    if (!p || cost < 0 || p->gems < cost) return false;
    p->gems -= cost;
    return true;
}

bool PlayerUnlockPrestigeRuneSlot(Player *p, int squad) {
    if (!p || squad < 0 || squad >= PLAYER_MAX_SQUADS || p->runes.prestige_unlocked[squad]) return false;
    if (!PlayerSpendGems(p, RUNE_PRESTIGE_UNLOCK_GEMS)) return false;
    p->runes.prestige_unlocked[squad] = true;
    return true;
}

bool PlayerUnlockPrestigeGearSlot(Player *p, Character *ch) {
    if (!p || !ch || !GearHasPrestigeSlot(ch) || ch->gear.prestige_unlocked) return false;
    if (!PlayerSpendGems(p, GearPrestigeSlotGemCost())) return false;
    ch->gear.prestige_unlocked = true;
    return true;
}

bool PlayerSellGear(Player *p, int def_index) {
    const GearDef *def = GearDefAt(def_index);
    if (!p || !def || p->gear.counts[def_index] <= 0) return false;
    p->gear.counts[def_index]--;
    p->gold += GearSellValue(def);
    return true;
}

bool PlayerBuyGearPack(Player *p, const GearPackDef *pack, int rolled[GEAR_PACK_MAX_ROLLS]) {
    if (!p || !pack) return false;
    if (!(pack->gems ? PlayerSpendGems(p, pack->price) : PlayerSpendGold(p, pack->price))) return false;
    for (int i = 0; i < pack->count; i++) {
        rolled[i] = GearRollFromPack(pack, &p->store_rng_state);
        GearGrant(&p->gear, rolled[i]);
    }
    rolled[pack->count] = GEAR_ROLL_NONE;
    if (GearRollTome(pack, &p->store_rng_state)) {
        rolled[pack->count] = GEAR_ROLL_TOME;
        p->gear.tomes++;
    }
    return true;
}

bool PlayerRefillEnergy(Player *p) {
    if (p->energy >= p->energy_max || p->gems < ENERGY_REFILL_GEM_COST) return false;
    p->gems -= ENERGY_REFILL_GEM_COST;
    p->energy = p->energy_max;
    p->energy_regen_timer = 0.0f;
    return true;
}

bool PlayerSpendPotion(Player *p, int count) {
    if (!p || count < 0 || p->potions < count) return false;
    p->potions -= count;
    return true;
}

void PlayerAddPotions(Player *p, int count) {
    if (!p || count <= 0) return;
    p->potions = p->potion_max > 0 ? (int)fminf((float)(p->potions + count), (float)p->potion_max) : p->potions + count;
}

int PlayerKeyIndexForId(int inventory_id) {
    int index = inventory_id - NEXUS_KEY_FIRST_ID;
    return index >= 0 && index < NEXUS_KEY_COUNT ? index : -1;
}

int PlayerKeyCount(const Player *p, int inventory_id) {
    int index = PlayerKeyIndexForId(inventory_id);
    return p && index >= 0 ? p->keys[index] : 0;
}

bool PlayerSpendKeys(Player *p, int inventory_id, int count) {
    int index = PlayerKeyIndexForId(inventory_id);
    if (!p || index < 0 || count < 0 || p->keys[index] < count) return false;
    p->keys[index] -= count;
    return true;
}

bool PlayerRefillStamina(Player *p) {
    if (p->stamina >= p->stamina_max || p->gems < STAMINA_REFILL_GEM_COST) return false;
    p->gems -= STAMINA_REFILL_GEM_COST;
    p->stamina = p->stamina_max;
    p->stamina_regen_timer = 0.0f;
    return true;
}
