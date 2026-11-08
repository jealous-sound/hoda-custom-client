#define TIMED_QUEST_DAY_MIX 2654435761u
#define TIMED_QUEST_SLOT_MIX 40503u
#define TIMED_QUEST_ACTION_CLAIM "quest_claim"
#define TIMED_QUEST_ACTION_SWAP "quest_swap"
#define TIMED_QUEST_ACTION_CHEST "quest_chest"

typedef enum {
    TIMED_QUEST_CHEST_OK,
    TIMED_QUEST_CHEST_LOCKED,
    TIMED_QUEST_CHEST_OPENED,
    TIMED_QUEST_CHEST_ROSTER_FULL,
} TimedQuestChestCheck;

static int TimedQuestDailyBasePick(long long day, int slot, int pool_size) {
    uint32_t state = (uint32_t)day * TIMED_QUEST_DAY_MIX + ((uint32_t)slot + 1u) * TIMED_QUEST_SLOT_MIX;
    return StoreRngRange(&state, pool_size);
}

int TimedQuestDailyPool(const PlayerTimedQuests *q, int slot) {
    const TimedQuestSlot *def = TimedQuestSlotAt(slot);
    if (!def) return TIMED_QUEST_NONE;
    return (TimedQuestDailyBasePick(q->day, slot, def->quest_count) + q->daily_offsets[slot]) % def->quest_count;
}

const TimedQuestDef *TimedQuestDailyAt(const PlayerTimedQuests *q, int slot) {
    int pool = TimedQuestDailyPool(q, slot);
    return pool == TIMED_QUEST_NONE ? NULL : &TimedQuestSlotAt(slot)->quests[pool];
}

bool TimedQuestDailyClaimed(const PlayerTimedQuests *q, int slot) {
    int pool = TimedQuestDailyPool(q, slot);
    return pool != TIMED_QUEST_NONE && q->daily_claimed[slot][pool];
}

long long TimedQuestProgress(const PlayerTimedQuests *q, TimedQuestPeriod period, const TimedQuestDef *def) {
    long long count = q->counts[period][def->stat];
    return count < def->target ? count : def->target;
}

static bool TimedQuestDone(const PlayerTimedQuests *q, TimedQuestPeriod period, const TimedQuestDef *def) {
    return def && q->counts[period][def->stat] >= def->target;
}

bool TimedQuestDailyClaimable(const PlayerTimedQuests *q, int slot) {
    return TimedQuestDone(q, TIMED_QUEST_DAILY, TimedQuestDailyAt(q, slot)) && !TimedQuestDailyClaimed(q, slot);
}

bool TimedQuestWeeklyClaimable(const PlayerTimedQuests *q, int index) {
    return TimedQuestDone(q, TIMED_QUEST_WEEKLY, TimedQuestWeeklyAt(index)) && !q->weekly_claimed[index];
}

int TimedQuestPoints(const PlayerTimedQuests *q, TimedQuestPeriod period) {
    int points = 0;
    if (period == TIMED_QUEST_WEEKLY) {
        for (int i = 0; i < TimedQuestWeeklyCount(); i++)
            if (q->weekly_claimed[i]) points += TimedQuestWeeklyAt(i)->points;
        return points;
    }
    for (int slot = 0; slot < TimedQuestSlotCount(); slot++) {
        const TimedQuestSlot *def = TimedQuestSlotAt(slot);
        for (int i = 0; i < def->quest_count; i++)
            if (q->daily_claimed[slot][i]) points += def->quests[i].points;
    }
    return points;
}

bool TimedQuestChestReached(const PlayerTimedQuests *q, TimedQuestPeriod period, int index) {
    const TimedQuestChest *chest = TimedQuestChestAt(period, index);
    return chest && TimedQuestPoints(q, period) >= chest->points;
}

bool TimedQuestChestOpenable(const PlayerTimedQuests *q, TimedQuestPeriod period, int index) {
    return TimedQuestChestReached(q, period, index) && !q->chests_opened[period][index];
}

int TimedQuestsClaimableCount(const Player *p) {
    if (!TimedQuestsUnlocked(p->level)) return 0;
    const PlayerTimedQuests *q = &p->timed_quests;
    int count = 0;
    for (int slot = 0; slot < TimedQuestSlotCount(); slot++) count += TimedQuestDailyClaimable(q, slot);
    for (int i = 0; i < TimedQuestWeeklyCount(); i++) count += TimedQuestWeeklyClaimable(q, i);
    for (int period = 0; period < TIMED_QUEST_PERIOD_COUNT; period++)
        for (int i = 0; i < TimedQuestChestCount((TimedQuestPeriod)period); i++)
            count += TimedQuestChestOpenable(q, (TimedQuestPeriod)period, i);
    return count;
}

bool TimedQuestCanSwap(const PlayerTimedQuests *q, int slot) {
    const TimedQuestSlot *def = TimedQuestSlotAt(slot);
    return def && def->quest_count > 1 && q->swaps_used < TimedQuestDailySwaps() && !TimedQuestDailyClaimed(q, slot) &&
           !TimedQuestDailyClaimable(q, slot);
}

static void TimedQuestPay(Player *p, int gold, int gems) {
    p->gold = ProgressionSaturatingAdd(p->gold, gold);
    p->gems = ProgressionSaturatingAdd(p->gems, gems);
    p->missions.gold_baseline = p->gold;
}

bool TimedQuestSwap(Player *p, int slot, long long now) {
    PlayerTimedQuests *q = &p->timed_quests;
    TimedQuestsRoll(q, now);
    if (!TimedQuestsUnlocked(p->level) || !TimedQuestCanSwap(q, slot)) return false;
    q->daily_offsets[slot]++;
    q->swaps_used++;
    return true;
}

cJSON *TimedQuestSwapAction(int slot) {
    const TimedQuestSlot *def = TimedQuestSlotAt(slot);
    cJSON *fields = def ? ServerActionCreate(TIMED_QUEST_ACTION_SWAP) : NULL;
    if (fields && !cJSON_AddStringToObject(fields, "slot", def->id)) {
        cJSON_Delete(fields);
        return NULL;
    }
    return fields;
}

bool TimedQuestClaimDaily(Player *p, int slot, long long now) {
    PlayerTimedQuests *q = &p->timed_quests;
    TimedQuestsRoll(q, now);
    if (!TimedQuestsUnlocked(p->level) || !TimedQuestDailyClaimable(q, slot)) return false;
    const TimedQuestDef *def = TimedQuestDailyAt(q, slot);
    q->daily_claimed[slot][TimedQuestDailyPool(q, slot)] = true;
    TimedQuestPay(p, def->gold, def->gems);
    TimedQuestsCount(q, TIMED_QUEST_STAT_DAILY_QUESTS, 1, now);
    return true;
}

bool TimedQuestClaimWeekly(Player *p, int index, long long now) {
    PlayerTimedQuests *q = &p->timed_quests;
    TimedQuestsRoll(q, now);
    if (!TimedQuestsUnlocked(p->level) || !TimedQuestWeeklyClaimable(q, index)) return false;
    const TimedQuestDef *def = TimedQuestWeeklyAt(index);
    q->weekly_claimed[index] = true;
    TimedQuestPay(p, def->gold, def->gems);
    return true;
}

cJSON *TimedQuestClaimAction(const TimedQuestDef *def) {
    cJSON *fields = def ? ServerActionCreate(TIMED_QUEST_ACTION_CLAIM) : NULL;
    if (fields && !cJSON_AddStringToObject(fields, "quest", def->id)) {
        cJSON_Delete(fields);
        return NULL;
    }
    return fields;
}

static int TimedQuestChestHeroDrops(const TimedQuestChest *chest) {
    int heroes = 0;
    for (int i = 0; i < chest->drop_count; i++) heroes += chest->drops[i].kind == EXPEDITION_DROP_HERO;
    return heroes;
}

TimedQuestChestCheck TimedQuestCheckChest(const Player *p, TimedQuestPeriod period, int index) {
    const PlayerTimedQuests *q = &p->timed_quests;
    if (!TimedQuestsUnlocked(p->level) || !TimedQuestChestReached(q, period, index)) return TIMED_QUEST_CHEST_LOCKED;
    if (q->chests_opened[period][index]) return TIMED_QUEST_CHEST_OPENED;
    int heroes = TimedQuestChestHeroDrops(TimedQuestChestAt(period, index));
    return PlayerRosterHasCapacity(p, heroes) ? TIMED_QUEST_CHEST_OK : TIMED_QUEST_CHEST_ROSTER_FULL;
}

ExpeditionReward TimedQuestOpenChest(Player *p, TimedQuestPeriod period, int index, long long now) {
    ExpeditionReward reward = {0};
    TimedQuestsRoll(&p->timed_quests, now);
    if (TimedQuestCheckChest(p, period, index) != TIMED_QUEST_CHEST_OK) return reward;
    const TimedQuestChest *chest = TimedQuestChestAt(period, index);
    p->timed_quests.chests_opened[period][index] = true;
    TimedQuestPay(p, chest->gold, chest->gems);
    reward.gold = chest->gold;
    reward.gems = chest->gems;
    cJSON *root = ProgressionLoadCharacterCatalog();
    const cJSON *catalog = cJSON_GetObjectItemCaseSensitive(root, "characters");
    for (int i = 0; i < chest->drop_count; i++) {
        const ExpeditionDrop *drop = &chest->drops[i];
        if (StoreRngRange(&p->store_rng_state, EXPEDITION_PERCENT) < drop->chance)
            ExpeditionRollDrop(p, drop, catalog, now, &reward);
    }
    cJSON_Delete(root);
    return reward;
}

cJSON *TimedQuestChestAction(TimedQuestPeriod period, int index) {
    const TimedQuestChest *chest = TimedQuestChestAt(period, index);
    cJSON *fields = chest ? ServerActionCreate(TIMED_QUEST_ACTION_CHEST) : NULL;
    if (fields && !cJSON_AddStringToObject(fields, "chest", chest->id)) {
        cJSON_Delete(fields);
        return NULL;
    }
    return fields;
}
