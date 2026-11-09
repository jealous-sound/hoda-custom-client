#define TIMED_QUESTS_PATH "data/timed_quests.json"
#define TIMED_QUESTS_VERSION 1
#define TIMED_QUEST_SLOT_CAPACITY 8
#define TIMED_QUEST_POOL_CAPACITY 8
#define TIMED_QUEST_WEEKLY_CAPACITY 8
#define TIMED_QUEST_CHEST_CAPACITY 4
#define TIMED_QUEST_CHEST_DROP_CAPACITY 3
#define TIMED_QUEST_ID_CAPACITY 32
#define TIMED_QUEST_KEY_CAPACITY 64
#define TIMED_QUEST_ART_CAPACITY 64
#define TIMED_QUEST_NONE (-1)
#define TIMED_QUEST_SECONDS_PER_DAY 86400LL
#define TIMED_QUEST_DAYS_PER_WEEK 7LL
#define TIMED_QUEST_EPOCH_DAYS_TO_MONDAY 3LL

typedef enum {
    TIMED_QUEST_STAT_BATTLES,
    TIMED_QUEST_STAT_BATTLE_WINS,
    TIMED_QUEST_STAT_CAMPAIGN_WINS,
    TIMED_QUEST_STAT_PVP_BATTLES,
    TIMED_QUEST_STAT_PVP_WINS,
    TIMED_QUEST_STAT_NEXUS_WINS,
    TIMED_QUEST_STAT_CRITS,
    TIMED_QUEST_STAT_FACTION_HITS,
    TIMED_QUEST_STAT_CONSUMED,
    TIMED_QUEST_STAT_SUMMONED,
    TIMED_QUEST_STAT_EXPEDITIONS_SENT,
    TIMED_QUEST_STAT_EXPEDITIONS_RETURNED,
    TIMED_QUEST_STAT_DAILY_QUESTS,
    TIMED_QUEST_STAT_ACTIVE_DAYS,
    TIMED_QUEST_STAT_COUNT
} TimedQuestStat;

static const char *const TIMED_QUEST_STAT_NAMES[TIMED_QUEST_STAT_COUNT] = {
    [TIMED_QUEST_STAT_BATTLES] = "battles",
    [TIMED_QUEST_STAT_BATTLE_WINS] = "battle_wins",
    [TIMED_QUEST_STAT_CAMPAIGN_WINS] = "campaign_wins",
    [TIMED_QUEST_STAT_PVP_BATTLES] = "pvp_battles",
    [TIMED_QUEST_STAT_PVP_WINS] = "pvp_wins",
    [TIMED_QUEST_STAT_NEXUS_WINS] = "nexus_wins",
    [TIMED_QUEST_STAT_CRITS] = "crits",
    [TIMED_QUEST_STAT_FACTION_HITS] = "faction_hits",
    [TIMED_QUEST_STAT_CONSUMED] = "consumed",
    [TIMED_QUEST_STAT_SUMMONED] = "summoned",
    [TIMED_QUEST_STAT_EXPEDITIONS_SENT] = "expeditions_sent",
    [TIMED_QUEST_STAT_EXPEDITIONS_RETURNED] = "expeditions_returned",
    [TIMED_QUEST_STAT_DAILY_QUESTS] = "daily_quests",
    [TIMED_QUEST_STAT_ACTIVE_DAYS] = "active_days",
};

typedef enum { TIMED_QUEST_DAILY, TIMED_QUEST_WEEKLY, TIMED_QUEST_PERIOD_COUNT } TimedQuestPeriod;

typedef struct {
    char id[TIMED_QUEST_ID_CAPACITY];
    TimedQuestStat stat;
    long long target;
    int points;
    int gold;
    int gems;
} TimedQuestDef;

typedef struct {
    char id[TIMED_QUEST_ID_CAPACITY];
    TimedQuestDef quests[TIMED_QUEST_POOL_CAPACITY];
    int quest_count;
} TimedQuestSlot;

typedef struct {
    char id[TIMED_QUEST_ID_CAPACITY];
    char art[TIMED_QUEST_ART_CAPACITY];
    char fallback[TIMED_QUEST_ART_CAPACITY];
    int points;
    int gold;
    int gems;
    ExpeditionDrop drops[TIMED_QUEST_CHEST_DROP_CAPACITY];
    int drop_count;
} TimedQuestChest;

typedef struct {
    char icon[TIMED_QUEST_ART_CAPACITY];
    char art[TIMED_QUEST_ART_CAPACITY];
    char title_key[TIMED_QUEST_KEY_CAPACITY];
    char description_key[TIMED_QUEST_KEY_CAPACITY];
} TimedQuestStatText;

typedef struct {
    TimedQuestChest items[TIMED_QUEST_CHEST_CAPACITY];
    int count;
} TimedQuestChests;

typedef struct {
    bool loaded;
    int unlock_level;
    int daily_swaps;
    TimedQuestSlot slots[TIMED_QUEST_SLOT_CAPACITY];
    int slot_count;
    TimedQuestDef weekly[TIMED_QUEST_WEEKLY_CAPACITY];
    int weekly_count;
    TimedQuestChests chests[TIMED_QUEST_PERIOD_COUNT];
    TimedQuestStatText text[TIMED_QUEST_STAT_COUNT];
} TimedQuestCatalog;

typedef struct {
    long long day;
    long long week;
    long long counts[TIMED_QUEST_PERIOD_COUNT][TIMED_QUEST_STAT_COUNT];
    bool daily_claimed[TIMED_QUEST_SLOT_CAPACITY][TIMED_QUEST_POOL_CAPACITY];
    int daily_offsets[TIMED_QUEST_SLOT_CAPACITY];
    int swaps_used;
    bool weekly_claimed[TIMED_QUEST_WEEKLY_CAPACITY];
    bool chests_opened[TIMED_QUEST_PERIOD_COUNT][TIMED_QUEST_CHEST_CAPACITY];
} PlayerTimedQuests;

static TimedQuestCatalog timed_quest_catalog;

static const char *const TIMED_QUEST_PERIOD_CHEST_KEYS[TIMED_QUEST_PERIOD_COUNT] = {"daily_chests", "weekly_chests"};

static TimedQuestStat TimedQuestStatFromName(const char *name) {
    for (int stat = 0; stat < TIMED_QUEST_STAT_COUNT; stat++)
        if (!strcmp(name, TIMED_QUEST_STAT_NAMES[stat])) return (TimedQuestStat)stat;
    return TIMED_QUEST_STAT_COUNT;
}

static bool TimedQuestDefParse(const cJSON *json, TimedQuestDef *def) {
    *def = (TimedQuestDef){
        .stat = TimedQuestStatFromName(JsonStr(json, "stat")),
        .target = JsonInt(json, "target"),
        .points = JsonInt(json, "points"),
        .gold = JsonInt(json, "gold"),
        .gems = JsonInt(json, "gems"),
    };
    StrCopy(def->id, sizeof(def->id), JsonStr(json, "id"));
    return def->id[0] && def->stat != TIMED_QUEST_STAT_COUNT && def->target > 0 && def->points >= 0 && def->gold >= 0 &&
           def->gems >= 0;
}

static bool TimedQuestChestParse(const cJSON *json, TimedQuestChest *chest) {
    *chest = (TimedQuestChest){
        .points = JsonInt(json, "points"),
        .gold = JsonInt(json, "gold"),
        .gems = JsonInt(json, "gems"),
    };
    StrCopy(chest->id, sizeof(chest->id), JsonStr(json, "id"));
    StrCopy(chest->art, sizeof(chest->art), JsonStr(json, "art"));
    StrCopy(chest->fallback, sizeof(chest->fallback), JsonStr(json, "fallback"));
    bool ok = chest->id[0] && chest->fallback[0] && chest->points > 0 && chest->gold >= 0 && chest->gems >= 0;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(json, "drops")) {
        if (chest->drop_count == TIMED_QUEST_CHEST_DROP_CAPACITY) return false;
        ok = ExpeditionDropParse(row, &chest->drops[chest->drop_count++]) && ok;
    }
    return ok;
}

static bool TimedQuestChestsParse(const cJSON *json, TimedQuestChests *chests) {
    bool ok = true;
    int last_points = 0;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(json, "chests")) {
        if (chests->count == TIMED_QUEST_CHEST_CAPACITY) return false;
        TimedQuestChest *chest = &chests->items[chests->count++];
        ok = TimedQuestChestParse(row, chest) && chest->points > last_points && ok;
        last_points = chest->points;
    }
    return ok;
}

static bool TimedQuestSlotParse(const cJSON *json, TimedQuestSlot *slot) {
    *slot = (TimedQuestSlot){0};
    StrCopy(slot->id, sizeof(slot->id), JsonStr(json, "id"));
    bool ok = slot->id[0] != '\0';
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(json, "quests")) {
        if (slot->quest_count == TIMED_QUEST_POOL_CAPACITY) return false;
        ok = TimedQuestDefParse(row, &slot->quests[slot->quest_count++]) && ok;
    }
    return ok && slot->quest_count > 0;
}

static bool TimedQuestStatTextParse(const cJSON *json, TimedQuestCatalog *c) {
    const cJSON *entry = NULL;
    bool ok = true;
    cJSON_ArrayForEach(entry, json) {
        TimedQuestStat stat = TimedQuestStatFromName(entry->string);
        if (stat == TIMED_QUEST_STAT_COUNT) return false;
        TimedQuestStatText *text = &c->text[stat];
        StrCopy(text->icon, sizeof(text->icon), JsonStr(entry, "icon"));
        StrCopy(text->art, sizeof(text->art), JsonStr(entry, "art"));
        StrCopy(text->title_key, sizeof(text->title_key), JsonStr(entry, "title_key"));
        StrCopy(text->description_key, sizeof(text->description_key), JsonStr(entry, "description_key"));
        ok = text->icon[0] && text->title_key[0] && text->description_key[0] && ok;
    }
    return ok;
}

static bool TimedQuestEveryQuestHasText(const TimedQuestCatalog *c) {
    for (int slot = 0; slot < c->slot_count; slot++)
        for (int i = 0; i < c->slots[slot].quest_count; i++)
            if (!c->text[c->slots[slot].quests[i].stat].title_key[0]) return false;
    for (int i = 0; i < c->weekly_count; i++)
        if (!c->text[c->weekly[i].stat].title_key[0]) return false;
    return true;
}

bool TimedQuestCatalogLoad(const char *path) {
    TimedQuestCatalog *c = &timed_quest_catalog;
    memset(c, 0, sizeof(*c));
    char *text = FileExists(path) ? LoadFileText(path) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    if (text) UnloadFileText(text);
    const cJSON *daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
    const cJSON *weekly = cJSON_GetObjectItemCaseSensitive(root, "weekly");
    c->unlock_level = JsonInt(root, "unlock_level");
    c->daily_swaps = JsonInt(daily, "swaps");
    bool ok = root && JsonInt(root, "version") == TIMED_QUESTS_VERSION && daily && weekly && c->unlock_level >= 1 &&
              c->daily_swaps >= 0;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(daily, "slots")) {
        if (c->slot_count == TIMED_QUEST_SLOT_CAPACITY) {
            ok = false;
            break;
        }
        ok = TimedQuestSlotParse(row, &c->slots[c->slot_count++]) && ok;
    }
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive(weekly, "quests")) {
        if (c->weekly_count == TIMED_QUEST_WEEKLY_CAPACITY) {
            ok = false;
            break;
        }
        ok = TimedQuestDefParse(row, &c->weekly[c->weekly_count++]) && ok;
    }
    ok = TimedQuestChestsParse(daily, &c->chests[TIMED_QUEST_DAILY]) && ok;
    ok = TimedQuestChestsParse(weekly, &c->chests[TIMED_QUEST_WEEKLY]) && ok;
    ok = TimedQuestStatTextParse(cJSON_GetObjectItemCaseSensitive(root, "stats"), c) && ok;
    ok = ok && c->slot_count > 0 && TimedQuestEveryQuestHasText(c);
    cJSON_Delete(root);
    if (!ok) {
        TraceLog(LOG_WARNING, "Timed quests: invalid table %s", path);
        memset(c, 0, sizeof(*c));
    }
    c->loaded = ok;
    return ok;
}

bool TimedQuestsAvailable(void) {
    return timed_quest_catalog.loaded;
}

int TimedQuestUnlockLevel(void) {
    return timed_quest_catalog.unlock_level;
}

bool TimedQuestsUnlocked(int player_level) {
    return TimedQuestsAvailable() && player_level >= timed_quest_catalog.unlock_level;
}

int TimedQuestSlotCount(void) {
    return timed_quest_catalog.slot_count;
}

const TimedQuestSlot *TimedQuestSlotAt(int slot) {
    return slot >= 0 && slot < timed_quest_catalog.slot_count ? &timed_quest_catalog.slots[slot] : NULL;
}

int TimedQuestDailySwaps(void) {
    return timed_quest_catalog.daily_swaps;
}

int TimedQuestWeeklyCount(void) {
    return timed_quest_catalog.weekly_count;
}

const TimedQuestDef *TimedQuestWeeklyAt(int index) {
    return index >= 0 && index < timed_quest_catalog.weekly_count ? &timed_quest_catalog.weekly[index] : NULL;
}

int TimedQuestChestCount(TimedQuestPeriod period) {
    return timed_quest_catalog.chests[period].count;
}

const TimedQuestChest *TimedQuestChestAt(TimedQuestPeriod period, int index) {
    const TimedQuestChests *chests = &timed_quest_catalog.chests[period];
    return index >= 0 && index < chests->count ? &chests->items[index] : NULL;
}

const TimedQuestStatText *TimedQuestText(TimedQuestStat stat) {
    return &timed_quest_catalog.text[stat];
}

int TimedQuestMaxPoints(TimedQuestPeriod period) {
    const TimedQuestChests *chests = &timed_quest_catalog.chests[period];
    return chests->count > 0 ? chests->items[chests->count - 1].points : 0;
}

long long TimedQuestDay(long long now) {
    return now >= 0 ? now / TIMED_QUEST_SECONDS_PER_DAY : (now + 1) / TIMED_QUEST_SECONDS_PER_DAY - 1;
}

long long TimedQuestWeek(long long now) {
    return (TimedQuestDay(now) + TIMED_QUEST_EPOCH_DAYS_TO_MONDAY) / TIMED_QUEST_DAYS_PER_WEEK;
}

long long TimedQuestResetIn(TimedQuestPeriod period, long long now) {
    long long day = TimedQuestDay(now);
    long long next_day = day + 1;
    if (period == TIMED_QUEST_WEEKLY)
        next_day = (TimedQuestWeek(now) + 1) * TIMED_QUEST_DAYS_PER_WEEK - TIMED_QUEST_EPOCH_DAYS_TO_MONDAY;
    return next_day * TIMED_QUEST_SECONDS_PER_DAY - now;
}

static void TimedQuestsClearDaily(PlayerTimedQuests *q, long long day) {
    memset(q->counts[TIMED_QUEST_DAILY], 0, sizeof(q->counts[TIMED_QUEST_DAILY]));
    memset(q->daily_claimed, 0, sizeof(q->daily_claimed));
    memset(q->daily_offsets, 0, sizeof(q->daily_offsets));
    memset(q->chests_opened[TIMED_QUEST_DAILY], 0, sizeof(q->chests_opened[TIMED_QUEST_DAILY]));
    q->swaps_used = 0;
    q->day = day;
}

static void TimedQuestsClearWeekly(PlayerTimedQuests *q, long long week) {
    memset(q->counts[TIMED_QUEST_WEEKLY], 0, sizeof(q->counts[TIMED_QUEST_WEEKLY]));
    memset(q->weekly_claimed, 0, sizeof(q->weekly_claimed));
    memset(q->chests_opened[TIMED_QUEST_WEEKLY], 0, sizeof(q->chests_opened[TIMED_QUEST_WEEKLY]));
    q->week = week;
}

void TimedQuestsRoll(PlayerTimedQuests *q, long long now) {
    long long day = TimedQuestDay(now);
    long long week = TimedQuestWeek(now);
    if (q->day != day) TimedQuestsClearDaily(q, day);
    if (q->week != week) TimedQuestsClearWeekly(q, week);
}

static void TimedQuestsAdd(PlayerTimedQuests *q, TimedQuestStat stat, long long amount) {
    for (int period = 0; period < TIMED_QUEST_PERIOD_COUNT; period++) {
        long long *count = &q->counts[period][stat];
        *count = amount > LLONG_MAX - *count ? LLONG_MAX : *count + amount;
    }
}

void TimedQuestsCount(PlayerTimedQuests *q, TimedQuestStat stat, long long amount, long long now) {
    if (amount <= 0 || stat < 0 || stat >= TIMED_QUEST_STAT_COUNT) return;
    TimedQuestsRoll(q, now);
    bool first_battle_today = stat == TIMED_QUEST_STAT_BATTLES && q->counts[TIMED_QUEST_DAILY][stat] == 0;
    TimedQuestsAdd(q, stat, amount);
    if (first_battle_today) TimedQuestsAdd(q, TIMED_QUEST_STAT_ACTIVE_DAYS, 1);
}

typedef enum { TIMED_QUEST_BATTLE_CAMPAIGN, TIMED_QUEST_BATTLE_PVP, TIMED_QUEST_BATTLE_NEXUS } TimedQuestBattleMode;

void TimedQuestsRecordBattle(
    PlayerTimedQuests *q, TimedQuestBattleMode mode, bool won, int crits, bool faction_hit, long long now
) {
    TimedQuestsCount(q, TIMED_QUEST_STAT_BATTLES, 1, now);
    TimedQuestsCount(q, TIMED_QUEST_STAT_CRITS, crits, now);
    TimedQuestsCount(q, TIMED_QUEST_STAT_FACTION_HITS, faction_hit ? 1 : 0, now);
    if (mode == TIMED_QUEST_BATTLE_PVP) TimedQuestsCount(q, TIMED_QUEST_STAT_PVP_BATTLES, 1, now);
    if (!won) return;
    TimedQuestsCount(q, TIMED_QUEST_STAT_BATTLE_WINS, 1, now);
    static const TimedQuestStat wins[] = {
        [TIMED_QUEST_BATTLE_CAMPAIGN] = TIMED_QUEST_STAT_CAMPAIGN_WINS,
        [TIMED_QUEST_BATTLE_PVP] = TIMED_QUEST_STAT_PVP_WINS,
        [TIMED_QUEST_BATTLE_NEXUS] = TIMED_QUEST_STAT_NEXUS_WINS,
    };
    TimedQuestsCount(q, wins[mode], 1, now);
}

static int TimedQuestSlotIndexById(const char *id) {
    for (int slot = 0; slot < timed_quest_catalog.slot_count; slot++)
        if (!strcmp(timed_quest_catalog.slots[slot].id, id)) return slot;
    return TIMED_QUEST_NONE;
}

static int TimedQuestPoolIndexById(const TimedQuestSlot *slot, const char *id) {
    for (int i = 0; i < slot->quest_count; i++)
        if (!strcmp(slot->quests[i].id, id)) return i;
    return TIMED_QUEST_NONE;
}

int TimedQuestWeeklyIndexById(const char *id) {
    for (int i = 0; i < timed_quest_catalog.weekly_count; i++)
        if (!strcmp(timed_quest_catalog.weekly[i].id, id)) return i;
    return TIMED_QUEST_NONE;
}

int TimedQuestChestIndexById(TimedQuestPeriod period, const char *id) {
    const TimedQuestChests *chests = &timed_quest_catalog.chests[period];
    for (int i = 0; i < chests->count; i++)
        if (!strcmp(chests->items[i].id, id)) return i;
    return TIMED_QUEST_NONE;
}

static bool TimedQuestFindDaily(const char *id, int *slot, int *pool) {
    for (int s = 0; s < timed_quest_catalog.slot_count; s++) {
        int found = TimedQuestPoolIndexById(&timed_quest_catalog.slots[s], id);
        if (found == TIMED_QUEST_NONE) continue;
        *slot = s;
        *pool = found;
        return true;
    }
    return false;
}

static cJSON *TimedQuestsSaveCounts(const long long *counts) {
    cJSON *root = cJSON_CreateObject();
    for (int stat = 0; root && stat < TIMED_QUEST_STAT_COUNT; stat++) {
        if (counts[stat] <= 0) continue;
        if (!cJSON_AddNumberToObject(root, TIMED_QUEST_STAT_NAMES[stat], (double)counts[stat])) {
            cJSON_Delete(root);
            return NULL;
        }
    }
    return root;
}

static bool TimedQuestsAddId(cJSON *array, const char *id) {
    cJSON *item = cJSON_CreateString(id);
    return item && cJSON_AddItemToArray(array, item);
}

cJSON *TimedQuestsSave(const PlayerTimedQuests *q) {
    const TimedQuestCatalog *c = &timed_quest_catalog;
    cJSON *root = cJSON_CreateObject();
    cJSON *daily = TimedQuestsSaveCounts(q->counts[TIMED_QUEST_DAILY]);
    cJSON *weekly = TimedQuestsSaveCounts(q->counts[TIMED_QUEST_WEEKLY]);
    bool ok = root && daily && weekly && cJSON_AddItemToObject(root, "daily", daily);
    if (!ok) cJSON_Delete(daily);
    ok = ok && cJSON_AddItemToObject(root, "weekly", weekly);
    if (!ok) cJSON_Delete(weekly);
    cJSON *claimed = ok ? cJSON_AddArrayToObject(root, "daily_claimed") : NULL;
    cJSON *offsets = claimed ? cJSON_AddObjectToObject(root, "daily_swaps") : NULL;
    cJSON *weekly_claimed = offsets ? cJSON_AddArrayToObject(root, "weekly_claimed") : NULL;
    ok = weekly_claimed && cJSON_AddNumberToObject(root, "day", (double)q->day) &&
         cJSON_AddNumberToObject(root, "week", (double)q->week) &&
         cJSON_AddNumberToObject(root, "swaps_used", q->swaps_used);
    for (int slot = 0; ok && slot < c->slot_count; slot++) {
        const TimedQuestSlot *def = &c->slots[slot];
        if (q->daily_offsets[slot] > 0) ok = cJSON_AddNumberToObject(offsets, def->id, q->daily_offsets[slot]) != NULL;
        for (int i = 0; ok && i < def->quest_count; i++)
            if (q->daily_claimed[slot][i]) ok = TimedQuestsAddId(claimed, def->quests[i].id);
    }
    for (int i = 0; ok && i < c->weekly_count; i++)
        if (q->weekly_claimed[i]) ok = TimedQuestsAddId(weekly_claimed, c->weekly[i].id);
    for (int period = 0; ok && period < TIMED_QUEST_PERIOD_COUNT; period++) {
        cJSON *opened = cJSON_AddArrayToObject(root, TIMED_QUEST_PERIOD_CHEST_KEYS[period]);
        ok = opened != NULL;
        for (int i = 0; ok && i < c->chests[period].count; i++)
            if (q->chests_opened[period][i]) ok = TimedQuestsAddId(opened, c->chests[period].items[i].id);
    }
    if (!ok) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

static void TimedQuestsLoadCounts(const cJSON *json, long long *counts) {
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, json) {
        TimedQuestStat stat = TimedQuestStatFromName(entry->string);
        if (stat != TIMED_QUEST_STAT_COUNT && cJSON_IsNumber(entry) && entry->valuedouble > 0.0)
            counts[stat] = (long long)entry->valuedouble;
    }
}

void TimedQuestsLoad(const cJSON *json, PlayerTimedQuests *q) {
    memset(q, 0, sizeof(*q));
    q->day = (long long)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(json, "day"));
    q->week = (long long)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(json, "week"));
    q->swaps_used = JsonInt(json, "swaps_used");
    TimedQuestsLoadCounts(cJSON_GetObjectItemCaseSensitive(json, "daily"), q->counts[TIMED_QUEST_DAILY]);
    TimedQuestsLoadCounts(cJSON_GetObjectItemCaseSensitive(json, "weekly"), q->counts[TIMED_QUEST_WEEKLY]);
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "daily_swaps")) {
        int slot = TimedQuestSlotIndexById(entry->string);
        if (slot != TIMED_QUEST_NONE && cJSON_IsNumber(entry) && entry->valueint > 0)
            q->daily_offsets[slot] = entry->valueint;
    }
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "daily_claimed")) {
        int slot = TIMED_QUEST_NONE, pool = TIMED_QUEST_NONE;
        if (cJSON_IsString(entry) && TimedQuestFindDaily(entry->valuestring, &slot, &pool))
            q->daily_claimed[slot][pool] = true;
    }
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, "weekly_claimed")) {
        int index = cJSON_IsString(entry) ? TimedQuestWeeklyIndexById(entry->valuestring) : TIMED_QUEST_NONE;
        if (index != TIMED_QUEST_NONE) q->weekly_claimed[index] = true;
    }
    for (int period = 0; period < TIMED_QUEST_PERIOD_COUNT; period++) {
        cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(json, TIMED_QUEST_PERIOD_CHEST_KEYS[period])) {
            int index =
                cJSON_IsString(entry) ? TimedQuestChestIndexById((TimedQuestPeriod)period, entry->valuestring) : -1;
            if (index != TIMED_QUEST_NONE) q->chests_opened[period][index] = true;
        }
    }
}
