#define VAULT_PATH "data/vault.json"
#define VAULT_VERSION 1
#define VAULT_TIER_FIELD "vault_tier"
#define VAULT_BRACKET_CAPACITY 8
#define VAULT_MILLIGEMS_PER_GEM 1000LL
#define VAULT_BASIS_POINTS_PER_WHOLE 10000LL
#define VAULT_WHOLE_PERCENT 100LL
#define VAULT_SECONDS_PER_DAY 86400LL
#define VAULT_ACTION_DEPOSIT "vault_deposit"
#define VAULT_ACTION_WITHDRAW "vault_withdraw"
#define VAULT_ACTION_CLAIM "vault_claim"
#define VAULT_ACTION_GEMS_FIELD "gems"
#define VAULT_BADGE_PATH EXTRA_ASSET_DIR "gui/vault/badge_%s.png"

typedef enum {
    VAULT_TIER_NONE,
    VAULT_TIER_BRONZE,
    VAULT_TIER_SILVER,
    VAULT_TIER_GOLD,
    VAULT_TIER_COUNT,
} VaultTier;

typedef struct {
    long long up_to;
    long long basis_points_per_day;
} VaultBracket;

typedef struct {
    bool loaded;
    long long lock_days;
    long long unclaimed_days;
    long long early_withdraw_fee_pct;
    long long minimum_deposit;
    VaultBracket brackets[VAULT_BRACKET_CAPACITY];
    int bracket_count;
    long long tier_gems[VAULT_TIER_COUNT];
} VaultCatalog;

typedef struct {
    long long locked;
    long long unlocks_at;
    long long accrued_at;
    long long claimed_at;
    long long pending_milligems;
    long long interest_paid;
    long long fees_paid;
} PlayerVault;

typedef struct {
    const char *key;
    size_t offset;
} VaultField;

typedef struct {
    bool asked;
    Texture2D texture;
} VaultBadgeSlot;

static VaultCatalog vault_catalog;
static VaultBadgeSlot vault_badges[VAULT_TIER_COUNT];

static const char *const VAULT_TIER_IDS[VAULT_TIER_COUNT] = {
    [VAULT_TIER_NONE] = "",
    [VAULT_TIER_BRONZE] = "bronze",
    [VAULT_TIER_SILVER] = "silver",
    [VAULT_TIER_GOLD] = "gold",
};

static const VaultField VAULT_FIELDS[] = {
    {"locked", offsetof(PlayerVault, locked)},
    {"unlocks_at", offsetof(PlayerVault, unlocks_at)},
    {"accrued_at", offsetof(PlayerVault, accrued_at)},
    {"claimed_at", offsetof(PlayerVault, claimed_at)},
    {"pending_milligems", offsetof(PlayerVault, pending_milligems)},
    {"interest_paid", offsetof(PlayerVault, interest_paid)},
    {"fees_paid", offsetof(PlayerVault, fees_paid)},
};

static long long *VaultFieldOf(PlayerVault *vault, const VaultField *field) {
    return (long long *)((char *)vault + field->offset);
}

static bool VaultJsonCount(const cJSON *json, const char *key, long long low, long long *out) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < (double)low || item->valuedouble > (double)INT_MAX ||
        item->valuedouble != (double)(long long)item->valuedouble)
        return false;
    *out = (long long)item->valuedouble;
    return true;
}

static bool VaultLoadBrackets(VaultCatalog *c, const cJSON *brackets) {
    long long floor = 0;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, brackets) {
        if (c->bracket_count == VAULT_BRACKET_CAPACITY) return false;
        VaultBracket *bracket = &c->brackets[c->bracket_count++];
        if (!VaultJsonCount(entry, "up_to", floor + 1, &bracket->up_to) ||
            !VaultJsonCount(entry, "basis_points_per_day", 1, &bracket->basis_points_per_day))
            return false;
        floor = bracket->up_to;
    }
    return c->bracket_count > 0;
}

long long VaultCapacity(void) {
    return vault_catalog.bracket_count > 0 ? vault_catalog.brackets[vault_catalog.bracket_count - 1].up_to : 0;
}

static bool VaultLoadTiers(VaultCatalog *c, const cJSON *tiers) {
    for (int tier = VAULT_TIER_BRONZE; tier < VAULT_TIER_COUNT; tier++)
        if (!VaultJsonCount(tiers, VAULT_TIER_IDS[tier], c->tier_gems[tier - 1] + 1, &c->tier_gems[tier])) return false;
    return cJSON_GetArraySize(tiers) == VAULT_TIER_COUNT - 1;
}

bool VaultCatalogLoad(const char *path) {
    VaultCatalog *c = &vault_catalog;
    memset(c, 0, sizeof(*c));
    char *text = FileExists(path) ? LoadFileText(path) : NULL;
    cJSON *root = text ? cJSON_Parse(text) : NULL;
    if (text) UnloadFileText(text);
    bool ok = root && JsonInt(root, "version") == VAULT_VERSION &&
              VaultJsonCount(root, "lock_days", 1, &c->lock_days) &&
              VaultJsonCount(root, "unclaimed_days", 1, &c->unclaimed_days) &&
              VaultJsonCount(root, "minimum_deposit", 1, &c->minimum_deposit) &&
              VaultJsonCount(root, "early_withdraw_fee_pct", 0, &c->early_withdraw_fee_pct) &&
              c->early_withdraw_fee_pct <= VAULT_WHOLE_PERCENT &&
              VaultLoadBrackets(c, cJSON_GetObjectItemCaseSensitive(root, "brackets")) &&
              VaultLoadTiers(c, cJSON_GetObjectItemCaseSensitive(root, "tier_gems")) &&
              c->tier_gems[VAULT_TIER_GOLD] <= VaultCapacity();
    cJSON_Delete(root);
    if (!ok) {
        TraceLog(LOG_WARNING, "Vault: %s is missing or invalid, the vault is closed", path);
        memset(c, 0, sizeof(*c));
    }
    c->loaded = ok;
    return ok;
}

bool VaultAvailable(void) {
    return vault_catalog.loaded;
}

long long VaultLockDays(void) {
    return vault_catalog.lock_days;
}

long long VaultUnclaimedDays(void) {
    return vault_catalog.unclaimed_days;
}

long long VaultEarlyWithdrawFeePct(void) {
    return vault_catalog.early_withdraw_fee_pct;
}

long long VaultMinimumDeposit(void) {
    return vault_catalog.minimum_deposit;
}

long long VaultMilligemsPerDay(long long locked) {
    long long total = 0, floor = 0;
    for (int i = 0; i < vault_catalog.bracket_count && locked > floor; i++) {
        const VaultBracket *bracket = &vault_catalog.brackets[i];
        long long slice = (locked < bracket->up_to ? locked : bracket->up_to) - floor;
        total += slice * bracket->basis_points_per_day * VAULT_MILLIGEMS_PER_GEM / VAULT_BASIS_POINTS_PER_WHOLE;
        floor = bracket->up_to;
    }
    return total;
}

VaultTier VaultTierFor(long long locked) {
    VaultTier tier = VAULT_TIER_NONE;
    for (int next = VAULT_TIER_BRONZE; vault_catalog.loaded && next < VAULT_TIER_COUNT; next++)
        if (locked >= vault_catalog.tier_gems[next]) tier = (VaultTier)next;
    return tier;
}

long long VaultTierGems(VaultTier tier) {
    return tier > VAULT_TIER_NONE && tier < VAULT_TIER_COUNT ? vault_catalog.tier_gems[tier] : 0;
}

int VaultTierOfListedPlayer(const cJSON *entry) {
    int tier = JsonInt(entry, VAULT_TIER_FIELD);
    return tier > VAULT_TIER_NONE && tier < VAULT_TIER_COUNT ? tier : VAULT_TIER_NONE;
}

long long VaultEarlyWithdrawFee(long long gems) {
    if (gems <= 0) return 0;
    return (gems * vault_catalog.early_withdraw_fee_pct + VAULT_WHOLE_PERCENT - 1) / VAULT_WHOLE_PERCENT;
}

bool VaultDepositFits(const PlayerVault *vault, long long gems) {
    long long room = VaultCapacity() - vault->locked;
    return vault_catalog.loaded && gems > 0 && gems <= room && (gems >= vault_catalog.minimum_deposit || gems == room);
}

bool VaultUnlocked(const PlayerVault *vault, long long now) {
    return now >= vault->unlocks_at;
}

static void VaultSettle(PlayerVault *vault, long long now) {
    if (vault->locked <= 0) return;
    long long window_end = vault->claimed_at + vault_catalog.unclaimed_days * VAULT_SECONDS_PER_DAY;
    long long end = now < window_end ? now : window_end;
    if (end <= vault->accrued_at) return;
    vault->pending_milligems += VaultMilligemsPerDay(vault->locked) * (end - vault->accrued_at) / VAULT_SECONDS_PER_DAY;
    vault->accrued_at = end;
}

long long VaultPendingMilligems(const PlayerVault *vault, long long now) {
    PlayerVault settled = *vault;
    VaultSettle(&settled, now);
    return settled.pending_milligems;
}

long long VaultPendingGems(const PlayerVault *vault, long long now) {
    return VaultPendingMilligems(vault, now) / VAULT_MILLIGEMS_PER_GEM;
}

bool VaultInterestStopped(const PlayerVault *vault, long long now) {
    return vault->locked > 0 && now >= vault->claimed_at + vault_catalog.unclaimed_days * VAULT_SECONDS_PER_DAY;
}

static long long VaultTakeInterest(PlayerVault *vault) {
    long long gems = vault->pending_milligems / VAULT_MILLIGEMS_PER_GEM;
    vault->pending_milligems %= VAULT_MILLIGEMS_PER_GEM;
    vault->interest_paid += gems;
    return gems;
}

static void VaultCredit(int *wallet, long long gems) {
    long long total = (long long)*wallet + gems;
    *wallet = total > INT_MAX ? INT_MAX : (int)total;
}

bool VaultDeposit(PlayerVault *vault, int *wallet, long long gems, long long now) {
    if (!VaultDepositFits(vault, gems) || gems > *wallet) return false;
    if (vault->locked == 0) {
        vault->accrued_at = now;
        vault->claimed_at = now;
    } else {
        VaultSettle(vault, now);
    }
    *wallet -= (int)gems;
    vault->locked += gems;
    vault->unlocks_at = now + vault_catalog.lock_days * VAULT_SECONDS_PER_DAY;
    return true;
}

long long VaultWithdraw(PlayerVault *vault, int *wallet, long long gems, long long now) {
    if (!vault_catalog.loaded || gems <= 0 || gems > vault->locked) return 0;
    VaultSettle(vault, now);
    long long returned = gems;
    if (!VaultUnlocked(vault, now)) {
        long long fee = VaultEarlyWithdrawFee(gems);
        returned -= fee;
        vault->fees_paid += fee;
        vault->pending_milligems = 0;
    }
    vault->locked -= gems;
    if (vault->locked == 0) {
        returned += VaultTakeInterest(vault);
        *vault = (PlayerVault){.interest_paid = vault->interest_paid, .fees_paid = vault->fees_paid};
    }
    VaultCredit(wallet, returned);
    return returned;
}

long long VaultClaim(PlayerVault *vault, int *wallet, long long now) {
    if (!vault_catalog.loaded) return 0;
    PlayerVault settled = *vault;
    VaultSettle(&settled, now);
    long long gems = VaultTakeInterest(&settled);
    if (gems <= 0) return 0;
    settled.accrued_at = now;
    settled.claimed_at = now;
    *vault = settled;
    VaultCredit(wallet, gems);
    return gems;
}

cJSON *VaultAction(const char *action, long long gems) {
    cJSON *fields = ServerActionCreate(action);
    if (fields && gems > 0) cJSON_AddNumberToObject(fields, VAULT_ACTION_GEMS_FIELD, (double)gems);
    return fields;
}

cJSON *VaultSave(const PlayerVault *vault) {
    cJSON *root = cJSON_CreateObject();
    bool ok = root != NULL;
    PlayerVault copy = *vault;
    for (size_t i = 0; ok && i < sizeof(VAULT_FIELDS) / sizeof(VAULT_FIELDS[0]); i++)
        ok = cJSON_AddNumberToObject(root, VAULT_FIELDS[i].key, (double)*VaultFieldOf(&copy, &VAULT_FIELDS[i])) != NULL;
    if (ok) return root;
    cJSON_Delete(root);
    return NULL;
}

void VaultLoad(const cJSON *json, PlayerVault *vault) {
    PlayerVault loaded = {0};
    for (size_t i = 0; i < sizeof(VAULT_FIELDS) / sizeof(VAULT_FIELDS[0]); i++) {
        const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, VAULT_FIELDS[i].key);
        if (!item) continue;
        if (!cJSON_IsNumber(item) || item->valuedouble < 0.0 || item->valuedouble > (double)LLONG_MAX / 2) {
            *vault = (PlayerVault){0};
            return;
        }
        *VaultFieldOf(&loaded, &VAULT_FIELDS[i]) = (long long)item->valuedouble;
    }
    *vault = loaded;
}

Texture2D VaultBadgeTexture(VaultTier tier) {
    if (tier <= VAULT_TIER_NONE || tier >= VAULT_TIER_COUNT) return (Texture2D){0};
    VaultBadgeSlot *slot = &vault_badges[tier];
    if (slot->asked) return slot->texture;
    slot->asked = true;
    const char *path = TextFormat(VAULT_BADGE_PATH, VAULT_TIER_IDS[tier]);
    if (!ResourceExists(path)) return slot->texture;
    slot->texture = ResourceLoadTexture(path);
    if (slot->texture.id) {
        GenTextureMipmaps(&slot->texture);
        SetTextureFilter(slot->texture, TEXTURE_FILTER_TRILINEAR);
    }
    return slot->texture;
}
