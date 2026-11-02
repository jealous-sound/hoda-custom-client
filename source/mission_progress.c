#define MISSION_ORIGINS_NAME_COUNT 4

static const char *const MISSION_ORIGINS_NAMES[MISSION_ORIGINS_NAME_COUNT] = {
    "Morrigan",
    "Leliana",
    "Alistair",
    "Oghren",
};

typedef bool (*MissionFactionTest)(Faction faction);

static const MissionFactionTest MISSION_SQUAD_FACTIONS[] = {
    FactionIsRed, FactionIsBlue, FactionIsWhite, FactionIsBlack
};

static void MissionAddCounter(PlayerMissions *m, MissionStat stat, long long amount) {
    if (amount > 0) m->counters[stat] += amount;
}

static void MissionTrackEarnedGold(Player *p) {
    PlayerMissions *m = &p->missions;
    if (m->gold_baseline_set) MissionAddCounter(m, MISSION_STAT_GOLD_EARNED, (long long)p->gold - m->gold_baseline);
    m->gold_baseline = p->gold;
    m->gold_baseline_set = true;
}

static bool MissionNodeMastered(const CampaignNode *node, int wins) {
    return !node->grind && node->wins_to_master > 0 && wins >= node->wins_to_master;
}

static long long MissionAnyNodeMastered(const Campaign *campaign, const PlayerCampaignProgress *progress) {
    for (int map = 0; map < campaign->map_count; map++) {
        for (int node = 0; node < campaign->maps[map].node_count; node++) {
            if (MissionNodeMastered(&campaign->maps[map].nodes[node], progress->node_completions[map][node])) return 1;
        }
    }
    return 0;
}

static long long MissionMapMastered(const Campaign *campaign, const PlayerCampaignProgress *progress, int map) {
    if (map < 0 || map >= campaign->map_count) return 0;
    bool any = false;
    for (int node = 0; node < campaign->maps[map].node_count; node++) {
        const CampaignNode *def = &campaign->maps[map].nodes[node];
        if (def->grind || def->wins_to_master <= 0) continue;
        if (!MissionNodeMastered(def, progress->node_completions[map][node])) return 0;
        any = true;
    }
    return any ? 1 : 0;
}

static long long MissionMapsUnlocked(const Campaign *campaign, const PlayerCampaignProgress *progress) {
    long long unlocked = 0;
    for (int map = 0; map < campaign->map_count; map++) {
        if (CampaignMapUnlocked(campaign, progress, map)) unlocked = map + 1;
    }
    return unlocked;
}

static long long MissionHighestHeroLevel(const Player *p, const MissionDef *tier_iv_of_rarity) {
    long long highest = 0;
    for (int i = 0; i < p->roster_count; i++) {
        const Character *ch = &p->roster[i];
        if (tier_iv_of_rarity && (ch->tier != TIER_IV || (int)ch->rarity != tier_iv_of_rarity->rarity)) continue;
        if (ch->level > highest) highest = ch->level;
    }
    return highest;
}

static long long MissionCollected(const Player *p, int rarity, int tier) {
    long long unique = 0;
    for (int i = 0; i < p->roster_count; i++) {
        const Character *ch = &p->roster[i];
        if ((int)ch->rarity != rarity || (int)ch->tier != tier) continue;
        bool repeated = false;
        for (int j = 0; j < i && !repeated; j++) {
            repeated =
                p->roster[j].rarity == ch->rarity && p->roster[j].tier == ch->tier && !strcmp(p->roster[j].id, ch->id);
        }
        if (!repeated) unique++;
    }
    return unique;
}

static const Character *MissionSquadUnit(const Player *p, int slot) {
    int index = p->squads[p->current_squad][slot];
    return index >= 0 && index < p->roster_count ? &p->roster[index] : NULL;
}

static long long MissionSquadAllFaction(const Player *p, int faction) {
    int count = (int)(sizeof(MISSION_SQUAD_FACTIONS) / sizeof(MISSION_SQUAD_FACTIONS[0]));
    if (faction < 0 || faction >= count) return 0;
    int members = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const Character *ch = MissionSquadUnit(p, slot);
        if (!ch) continue;
        if (!MISSION_SQUAD_FACTIONS[faction](ch->faction)) return 0;
        members++;
    }
    return members == SQUAD_SLOT_COUNT ? 1 : 0;
}

static long long MissionOriginsSquadMembers(const Player *p) {
    long long members = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const Character *ch = MissionSquadUnit(p, slot);
        for (int name = 0; ch && name < MISSION_ORIGINS_NAME_COUNT; name++) {
            if (!strstr(ch->name, MISSION_ORIGINS_NAMES[name])) continue;
            members++;
            break;
        }
    }
    return members;
}

static long long MissionValue(const Player *p, const Campaign *campaign, const MissionDef *def) {
    const PlayerCampaignProgress *progress = &p->campaign_progress;
    switch (def->stat) {
        case MISSION_STAT_FIRST_WIN:
            return progress->node_completions[0][0] > 0 ? 1 : 0;
        case MISSION_STAT_FIRST_BOSS: {
            int boss = CampaignBossNodeIndex(campaign, 0);
            return boss >= 0 && progress->node_completions[0][boss] > 0 ? 1 : 0;
        }
        case MISSION_STAT_NODE_MASTERED:
            return MissionAnyNodeMastered(campaign, progress);
        case MISSION_STAT_MAP_MASTERED:
            return MissionMapMastered(campaign, progress, def->map);
        case MISSION_STAT_MAPS_UNLOCKED:
            return MissionMapsUnlocked(campaign, progress);
        case MISSION_STAT_PLAYER_LEVEL:
            return p->level;
        case MISSION_STAT_HERO_LEVEL:
            return MissionHighestHeroLevel(p, NULL);
        case MISSION_STAT_TIER_IV_LEVEL:
            return MissionHighestHeroLevel(p, def);
        case MISSION_STAT_COLLECTED:
            return MissionCollected(p, def->rarity, def->hero_tier);
        case MISSION_STAT_SQUAD_FACTION:
            return MissionSquadAllFaction(p, def->faction);
        case MISSION_STAT_ORIGINS_SQUAD:
            return MissionOriginsSquadMembers(p);
        case MISSION_STAT_PVP_BATTLES:
            return (long long)p->pvp.wins + p->pvp.losses;
        case MISSION_STAT_PVP_WINS:
            return p->pvp.wins;
        case MISSION_STAT_PVP_WIN_STREAK:
            return p->pvp.best_win_streak;
        case MISSION_STAT_PVP_TROPHIES_WON:
            return p->pvp.trophies_won;
        case MISSION_STAT_PVP_TROPHIES_HELD:
            return PvpTrophiesHeld(&p->pvp, RuneNow());
        default:
            return p->missions.counters[def->stat];
    }
}

void MissionsSync(Player *p, const Campaign *campaign) {
    MissionTrackEarnedGold(p);
    for (int i = 0; i < MissionCount(); i++) {
        MissionRaiseProgress(&p->missions, i, MissionValue(p, campaign, MissionAt(i)));
    }
}

static long long MissionBattleDamage(const BattleSim *sim, bool *faction_hit) {
    long long damage = 0;
    for (int a = 0; a < sim->action_count; a++) {
        const BattleSimAction *action = &sim->actions[a];
        if (action->actor < 0 || action->actor / SQUAD_SLOT_COUNT != 0) continue;
        for (int t = 0; t < action->target_count; t++) {
            const BattleSimTarget *target = &action->targets[t];
            if (target->unit / SQUAD_SLOT_COUNT == 0) continue;
            int dealt = target->health_delta + target->health_faction_delta;
            if (dealt < 0) damage -= dealt;
            if (target->health_faction_delta < 0) *faction_hit = true;
        }
    }
    return damage;
}

static int MissionLocalDay(long long now) {
    time_t moment = (time_t)now;
    struct tm *local = localtime(&moment);
    return local ? local->tm_year * 1000 + local->tm_yday : 0;
}

void MissionsRecordBattle(Player *p, const BattleSim *sim, int crits, bool challenge, long long now) {
    PlayerMissions *m = &p->missions;
    bool faction_hit = false;
    long long damage = MissionBattleDamage(sim, &faction_hit);
    MissionAddCounter(m, MISSION_STAT_CRITS, crits);
    if (damage > m->counters[MISSION_STAT_BATTLE_DAMAGE]) m->counters[MISSION_STAT_BATTLE_DAMAGE] = damage;
    if (challenge) MissionAddCounter(m, MISSION_STAT_CHALLENGE_ATTEMPTS, 1);
    if (faction_hit) MissionAddCounter(m, MISSION_STAT_FACTION_HITS, 1);
    int day = MissionLocalDay(now);
    if (day != m->last_battle_day) {
        m->last_battle_day = day;
        MissionAddCounter(m, MISSION_STAT_BATTLE_DAYS, 1);
    }
}

static bool MissionSquadFullyBuffed(const BattleSim *sim) {
    bool any = false;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const SimUnit *unit = &sim->units[slot];
        if (!unit->present) continue;
        if (!unit->game_changer_applied) return false;
        any = true;
    }
    return any;
}

void MissionsRecordEventBattle(Player *p, const BattleSim *sim, int banners) {
    MissionAddCounter(&p->missions, MISSION_STAT_EVENT_BANNERS, banners);
    if (p->pvp.event.buffed || !MissionSquadFullyBuffed(sim)) return;
    p->pvp.event.buffed = true;
    MissionAddCounter(&p->missions, MISSION_STAT_EVENT_BUFFED_SQUADS, 1);
}

bool MissionsClaim(Player *p, int index) {
    PlayerMissions *m = &p->missions;
    const MissionDef *def = MissionAt(index);
    if (!def || !MissionClaimable(m, index)) return false;
    m->claimed[index] = true;
    p->gold = ProgressionSaturatingAdd(p->gold, def->gold);
    p->gems = ProgressionSaturatingAdd(p->gems, def->gems);
    m->gold_baseline = p->gold;
    return true;
}
