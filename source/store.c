
bool ProgressionSave(const Player *player);
Color GearRarityColor(int rarity);
Texture2D GearRarityFrame(int rarity);
void GearDrawTile(const GearDef *def, Rectangle r, bool selected);
static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
);

#define STORE_PACK_COUNT 4
#define STORE_PACK_CAPACITY 72
#define STORE_MAX_RESULTS 10
#define STORE_GEM_SKU_COUNT 7
#define STORE_PACK_BULLET_MAX 4
#define STORE_VOUCHER_BULLETS 2
#define STORE_VOUCHER_WIN_DROP_PCT 5
#define STORE_RUNE_PACK_SLOTS 3
#define STORE_RUNE_SLOT_FAMILIES 4
#define STORE_MAX_ITEM_RESULTS                                                                                         \
    (GEAR_PACK_MAX_ROLLS > STORE_RUNE_PACK_SLOTS ? GEAR_PACK_MAX_ROLLS : STORE_RUNE_PACK_SLOTS)
#define STORE_PACK_UNAVAILABLE (-1LL)
#define STORE_PACK_ALWAYS 0LL
#define STORE_SECONDS_PER_DAY 86400LL
#define STORE_BULLET_DISPLAY_SECONDS 3.0f
#define STORE_BULLET_FADE_SECONDS 1.0f
#define STORE_BULLET_CYCLE_SECONDS 5.0f
#define STORE_DIALOG_TEXT_CAPACITY 512
#define STORE_ITEMS_TITLE "Предметы"
#define STORE_VAULT_TITLE "Хранилище"
#define STORE_FEATURED_TITLE "Популярное"
#define STORE_COMING_SOON_TEXT "Скоро!"
#define STORE_CONFIRM_GEMS_FORMAT "Потратить самоцветы: %d?"
#define STORE_PLATE_TOP 76.0f
#define STORE_PLATE_HEIGHT 633.0f
#define STORE_PLATE_ASPECT 1.482f
#define STORE_CELL_ASPECT 0.582f
#define STORE_SCROLL_STEP_OF_LIST 0.7645f
#define STORE_ARROW_ASPECT 0.928f

static const Rectangle STORE_CELL_HEADER = {0.0244f, 0.0229f, 0.9385f, 0.1013f};
static const Rectangle STORE_CELL_CARD = {0.0393f, 0.1468f, 0.9193f, 0.661f};
static const Rectangle STORE_CELL_ICON = {0.0993f, 0.1918f, 0.7989f, 0.464f};
static const Rectangle STORE_CELL_INFO = {0.795f, 0.1667f, 0.1333f, 0.0779f};
static const Rectangle STORE_CELL_BULLET = {0.06f, 0.705f, 0.88f, 0.06f};
static const Rectangle STORE_CELL_BUY = {0.0244f, 0.8219f, 0.953f, 0.1013f};
static const Rectangle STORE_CELL_TAP_AREA = {0.0f, 0.0f, 1.0f, 0.81f};
static const Rectangle STORE_CELL_TIMER = {0.2f, 0.135f, 0.6f, 0.065f};
#define STORE_TIMER_BANNER_COLOR ((Color){176, 28, 24, 255})
#define STORE_SCROLL_LINE_GAP 4.0f

typedef enum {
    STORE_TILE_FEATURED,
    STORE_TILE_HEROES,
    STORE_TILE_ITEMS,
    STORE_TILE_GEMS,
    STORE_TILE_VAULT,
    STORE_TILE_COUNT,
} StoreTile;

static const Rectangle STORE_LANDING_TILES[STORE_TILE_COUNT] = {
    [STORE_TILE_FEATURED] = {0.064f, 0.039f, 0.347f, 0.922f},
    [STORE_TILE_HEROES] = {0.426f, 0.039f, 0.245f, 0.429f},
    [STORE_TILE_ITEMS] = {0.698f, 0.039f, 0.245f, 0.429f},
    [STORE_TILE_GEMS] = {0.426f, 0.532f, 0.245f, 0.429f},
    [STORE_TILE_VAULT] = {0.698f, 0.532f, 0.245f, 0.429f},
};
#define STORE_TILE_ART_INSET_X 0.05f
#define STORE_TILE_ART_HEIGHT 0.76f
#define STORE_TILE_LABEL_HEIGHT 28.0f
#define STORE_TILE_LABEL_BOTTOM_OF_FEATURED_TILE 0.065f
#define STORE_TILE_LABEL_BOTTOM_OF_SQUARE_TILE 0.045f
#define STORE_TILE_LABEL_SIZE 26.0f
#define STORE_FEATURED_FRAME_TINT ((Color){255, 196, 92, 255})

#define STORE_REVEAL_STEP 0.6f
#define STORE_REVEAL_BURST_SECONDS 0.9f
#define STORE_REVEAL_BURST_SCALE 2.4f
#define STORE_REVEAL_CARD_WIDTH 140.0f
#define STORE_REVEAL_CARD_GAP 12.0f
#define STORE_REVEAL_CARD_ASPECT 0.985f
#define STORE_REVEAL_LEFT 256.0f
#define STORE_REVEAL_AREA_WIDTH 780.0f
#define STORE_REVEAL_CENTER_Y 383.0f
#define STORE_REVEAL_AGAIN_RECT ((Rectangle){520.0f, 606.0f, 259.0f, 58.0f})
#define STORE_REVEAL_DONE_RECT ((Rectangle){789.0f, 606.0f, 259.0f, 58.0f})
#define STORE_HEADER_TEXT_OF_CELL 0.052f
#define STORE_BULLET_TEXT_OF_CELL 0.04f
#define STORE_BULLET_ICON_GAP_OF_ICON 0.3f
#define STORE_PRICE_TEXT_OF_BAR 0.66f
#define STORE_PRICE_RIGHT_OF_BAR 0.5f
#define STORE_CURRENCY_CENTER_OF_BAR 0.609f
#define STORE_CURRENCY_SIZE_OF_BAR 0.8f
#define STORE_TITLE_TEXT_OF_PLATE 0.088f
#define STORE_GEM_SUBTEXT_HEIGHT 21.0f

typedef enum {
    STORE_PACK_RECRUIT,
    STORE_PACK_SOLDIER,
    STORE_PACK_CHAMPION,
    STORE_PACK_GRAND_CHAMPION,
} StorePackId;

typedef enum {
    STORE_VIEW_LANDING,
    STORE_VIEW_HEROES,
    STORE_VIEW_ITEMS,
    STORE_VIEW_FEATURED,
    STORE_VIEW_VAULT,
    STORE_VIEW_GEMS,
    STORE_VIEW_OPENING,
    STORE_VIEW_RESULT,
    STORE_VIEW_ITEM_REVEAL,
} StoreView;

typedef enum {
    STORE_ACTION_NONE,
    STORE_ACTION_CLOSE,
    STORE_ACTION_PURCHASED,
    STORE_ACTION_GOTO_MAP,
    STORE_ACTION_GOTO_SQUAD,
    STORE_ACTION_GOTO_CONSUME,
} StoreAction;

typedef enum {
    STORE_DIALOG_NONE,
    STORE_DIALOG_CANT_AFFORD,
    STORE_DIALOG_ROSTER_FULL,
    STORE_DIALOG_VOUCHER,
    STORE_DIALOG_CONFIRM_GEMS,
} StoreDialog;

typedef enum {
    STORE_KIND_HERO,
    STORE_KIND_RUNE,
    STORE_KIND_GEAR,
} StorePackKind;

typedef struct {
    const char *text;
    int rarity;
} StoreBullet;

typedef struct {
    const char *families[STORE_RUNE_SLOT_FAMILIES];
    int family_count;
    int min_rarity;
    int max_rarity;
} StoreRuneSlot;

typedef struct {
    int voucher;
    bool gems;
    int price;
} StoreOffer;

typedef struct {
    char id[32];
    const char *name;
    const char *info;
    const char *header;
    const char *icon;
    StoreBullet bullets[STORE_PACK_BULLET_MAX];
    int bullet_count;
    StorePackKind kind;
    StoreView page;
    StoreOffer paid;
    bool has_paid_offer;
    int voucher;
    int schedule_start;
    int schedule_end;
    int flash_index;
    int opened_counter;
    int hero_count;
    Rarity minimum_rarity;
    int guaranteed_legendary;
    int guaranteed_epic;
    int legendary_pity;
    int weights[RARITY_LEGENDARY + 1];
    int rune_chance_pct;
    int rune_count;
    int rune_min_rarity;
    int rune_max_rarity;
    int gear_chance_pct;
    int gear_min_rarity;
    int gear_max_rarity;
    StoreRuneSlot slots[STORE_RUNE_PACK_SLOTS];
    int slot_count;
    int gear_pack;
} StorePackDef;

typedef struct {
    const char *cost;
    const char *banner;
    int gems;
    int icon;
} StoreGemSku;

typedef struct {
    cJSON *config_root;
    cJSON *catalog_root;
    const cJSON *catalog;
    const cJSON **rarity_pool[RARITY_LEGENDARY + 1];
    int rarity_pool_count[RARITY_LEGENDARY + 1];
    char balance_note_ru[256];
    StorePackDef packs[STORE_PACK_CAPACITY];
    int pack_count;
    const char *voucher_icons[PLAYER_VOUCHER_CAPACITY];
    int voucher_count;
    int flash_days;
    int flash_count;
    StoreGemSku gem_skus[STORE_GEM_SKU_COUNT];
    PackScene opening_scene;
    PackFx opening_fx;
    PackHero opening_hero;

    Texture2D card;
    Texture2D popup;
    Texture2D plate;
    Texture2D category_frame;
    Texture2D category_art[STORE_TILE_COUNT];
    Texture2D timer_banner;
    Texture2D timer_icon;
    Texture2D scroll_arrow;
    Texture2D currency_gold;
    Texture2D currency_gems;
    Texture2D gem_card;
    Texture2D gem_coin;
    Texture2D gem_quest;
    Texture2D gem_info_frame;
    Texture2D banner_tex;
    Texture2D gem_icons[STORE_GEM_SKU_COUNT];
    Texture2D info_icon;
    Texture2D info_icon_on;
    Font title_font;
    Font body_font;

    Button close;
    Button back;
    Button buy[STORE_PACK_CAPACITY];
    Button next;
    Button skip;
    Button done;
    Button buy_again;
    Button item_done;
    Button item_again;
    Button dialog_ok;
    Button dialog_cancel;
    Button nav_train, nav_runes, nav_squad;
    bool nav_consume_clicked;
    Camera3D opening_camera;

    StoreView view;
    StoreView return_view;
    int visible[STORE_PACK_CAPACITY];
    int visible_count;
    long long clock_override;
    int selected_pack;
    int pending_pack;
    int info_pack;
    float scroll;
    bool dragging;
    bool drag_scrolled;
    float drag_start_x;
    float scroll_start;
    float bullet_time;
    StoreDialog dialog;
    bool dialog_press_armed;
    int item_results[STORE_MAX_ITEM_RESULTS];
    int item_result_count;
    StorePackKind item_kind;
    int rune_details;
    float item_reveal_time;
    int pack_rune_def;
    int pack_gear_def;
    int result_roster_indices[STORE_MAX_RESULTS];
    int result_count;
    int reveal_index;
    int loaded_reveal_index;
    float phase_time;
    float result_time;
    bool sequence_sound_played;
    bool rarity_sound_played;
    bool melee_played;
    int reveal_hitch_guard_frames;
    float status_time;
    char status[192];
} Store;

#define STORE_OPEN_ROULETTE_END 3.06f
#define STORE_OPEN_CAMERA_END 4.06f
#define STORE_OPEN_SEQUENCE_START 4.80f
#define STORE_OPEN_COMPLETE 8.0503f
#define STORE_PACK_RUNE_RECT ((Rectangle){1074.0f, 88.0f, 206.0f, 206.0f})
#define STORE_PACK_RUNE_ICON_SCALE 0.78f
#define STORE_PACK_RUNE_BURST_SCALE 1.2f
#define STORE_PACK_RUNE_FADE_IN 0.8f
#define STORE_PACK_RUNE_BURST_DELAY 1.8f
#define STORE_PACK_RUNE_BURST_DURATION 2.0f
#define STORE_PACK_RUNE_BURST_SWELL_DURATION 1.0f
#define STORE_PACK_RUNE_BURST_SWELL_RETURN_DELAY 2.8f
#define STORE_PACK_RUNE_BURST_SPIN_DEGREES 90.0f
#define STORE_PACK_RUNE_BURST_PEAK_BASE 1.6f
#define STORE_PACK_RUNE_BURST_PEAK_PER_RARITY 0.1f

static int store_voucher_kinds;

static Rectangle StoreScaledAround(Rectangle rect, float scale);

static void StoreDrawCenteredFit(Font font, const char *text, Rectangle rect, float size, Color color) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    if (measured.x > rect.width && measured.x > 0.0f) size *= rect.width / measured.x;
    UiDrawTextCentered(font, text, rect, size, color);
}

static uint32_t StoreRngNext(uint32_t *state) {
    uint32_t x = *state;
    if (x == 0) x = 0x6D2B79F5u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static int StoreRngRange(uint32_t *state, int count) {
    return count > 0 ? (int)(StoreRngNext(state) % (uint32_t)count) : 0;
}

static Rarity StoreRollRarity(const StorePackDef *pack, uint32_t *rng) {
    int total = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) total += pack->weights[rarity];
    if (total <= 0) return pack->minimum_rarity;
    int roll = StoreRngRange(rng, total);
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) {
        if (roll < pack->weights[rarity]) return (Rarity)rarity;
        roll -= pack->weights[rarity];
    }
    return RARITY_LEGENDARY;
}

static SoundId StoreRevealSound(Rarity rarity) {
    if (rarity >= RARITY_LEGENDARY) return SND_PACK_LEGENDARY;
    if (rarity == RARITY_EPIC) return SND_PACK_EPIC;
    if (rarity == RARITY_RARE) return SND_PACK_RARE;
    if (rarity == RARITY_UNCOMMON) return SND_PACK_UNCOMMON;
    return SND_PACK_COMMON;
}

static HeroCard *StoreCurrentCard(const Store *store, Heroes *heroes) {
    if (store->reveal_index < 0 || store->reveal_index >= store->result_count) return NULL;
    int roster_index = store->result_roster_indices[store->reveal_index];
    if (roster_index < 0 || roster_index >= heroes->card_count) return NULL;
    return &heroes->cards[roster_index];
}

static PackReveal StoreCurrentReveal(const Store *store, Heroes *heroes) {
    PackReveal reveal = {.rarity = RARITY_COMMON, .faction = FACTION_ALL, .base_scale = PackSceneBaseScale()};
    const HeroCard *card = StoreCurrentCard(store, heroes);
    if (!card) return reveal;
    const Character *ch = card->ch;
    reveal.rarity = ch->rarity;
    reveal.faction = ch->faction;
    reveal.stars = ch->rarity == RARITY_LEGENDARY ? ch->stars : 0;
    reveal.base_scale = PackSceneBaseScale();
    reveal.tall = PackHeroIsTall(&store->opening_hero);
    return reveal;
}

static void StorePlayCurrentRaritySound(const Store *store, Heroes *heroes) {
    const HeroCard *card = StoreCurrentCard(store, heroes);
    if (card) GuiSoundPlay(StoreRevealSound(card->ch->rarity));
}

static void StoreSetStatus(Store *store, const char *message) {
    StrCopy(store->status, sizeof(store->status), message);
    store->status_time = 3.5f;
}

static long long StoreNow(const Store *store) {
    return store->clock_override ? store->clock_override : RuneNow();
}

static int StoreParseMonthDay(const char *text) {
    int month = 0, day = 0;
    return sscanf(text, "%d-%d", &month, &day) == 2 ? month * 100 + day : 0;
}

long long StorePackSecondsLeft(const StorePackDef *pack, int flash_days, int flash_count, long long now) {
    if (pack->flash_index >= 0) {
        if (flash_days <= 0 || flash_count <= 0) return STORE_PACK_UNAVAILABLE;
        long long span = flash_days * STORE_SECONDS_PER_DAY;
        long long period = now / span;
        if ((int)(period % flash_count) != pack->flash_index) return STORE_PACK_UNAVAILABLE;
        return (period + 1) * span - now;
    }
    if (!pack->schedule_start) return STORE_PACK_ALWAYS;
    time_t stamp = (time_t)now;
    struct tm date = *localtime(&stamp);
    int today = (date.tm_mon + 1) * 100 + date.tm_mday;
    bool wraps = pack->schedule_start > pack->schedule_end;
    bool active = wraps ? today >= pack->schedule_start || today <= pack->schedule_end
                        : today >= pack->schedule_start && today <= pack->schedule_end;
    if (!active) return STORE_PACK_UNAVAILABLE;
    struct tm end = date;
    end.tm_year += wraps && today >= pack->schedule_start ? 1 : 0;
    end.tm_mon = pack->schedule_end / 100 - 1;
    end.tm_mday = pack->schedule_end % 100 + 1;
    end.tm_hour = end.tm_min = end.tm_sec = 0;
    end.tm_isdst = -1;
    return (long long)mktime(&end) - now;
}

static StoreOffer StoreActiveOffer(const StorePackDef *pack, const Player *player) {
    if (pack->voucher >= 0 && (player->pack_vouchers[pack->voucher] > 0 || !pack->has_paid_offer))
        return (StoreOffer){.voucher = pack->voucher, .price = 1};
    return pack->paid;
}

static bool StoreOfferAffordable(StoreOffer offer, const Player *player) {
    if (offer.voucher >= 0) return player->pack_vouchers[offer.voucher] >= offer.price;
    return offer.gems ? player->gems >= offer.price : player->gold >= offer.price;
}

static void StoreOfferPay(StoreOffer offer, Player *player) {
    bool spent = true;
    if (offer.voucher >= 0)
        player->pack_vouchers[offer.voucher] -= offer.price;
    else
        spent = offer.gems ? PlayerSpendGems(player, offer.price) : PlayerSpendGold(player, offer.price);
    if (!spent) TraceLog(LOG_FATAL, "Store: validated currency spend failed");
}

static const char *StorePackLocText(const cJSON *json, const char *suffix, const char *fallback_key) {
    char key[96];
    snprintf(key, sizeof(key), "%s%s", JsonStr(json, "loc_id"), suffix);
    const char *text = LocGet(key);
    if (!text[0] && fallback_key) text = JsonStr(json, fallback_key);
    return text;
}

static int StoreVoucherIndex(const Store *store, const cJSON *config, const char *id) {
    int index = 0;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive((cJSON *)config, "vouchers")) {
        if (!strcmp(JsonStr(row, "id"), id)) return index;
        index++;
    }
    TraceLog(LOG_FATAL, "Store: unknown voucher %s", id);
    return -1;
}

static void StoreLoadOffers(Store *store, StorePackDef *pack, const cJSON *json) {
    pack->voucher = -1;
    pack->paid.voucher = -1;
    const cJSON *row = NULL;
    cJSON_ArrayForEach(row, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "offers")) {
        const char *currency = JsonStr(row, "currency");
        if (!strcmp(currency, "voucher")) {
            pack->voucher = StoreVoucherIndex(store, store->config_root, JsonStr(row, "voucher"));
            continue;
        }
        pack->paid.gems = !strcmp(currency, "gems");
        pack->paid.price = JsonInt(row, "price");
        pack->has_paid_offer = true;
    }
    if (!pack->has_paid_offer && pack->voucher < 0) TraceLog(LOG_FATAL, "Store: pack %s has no offer", pack->id);
}

static void StoreLoadHeroContents(StorePackDef *pack, const cJSON *json) {
    pack->opened_counter = JsonInt(json, "opened_counter");
    pack->hero_count = JsonInt(json, "hero_count");
    pack->minimum_rarity = RarityFromString(JsonStr(json, "minimum_rarity"));
    pack->guaranteed_legendary = JsonInt(json, "guaranteed_legendary");
    pack->guaranteed_epic = JsonInt(json, "guaranteed_epic");
    pack->legendary_pity = JsonInt(json, "legendary_pity");
    pack->rune_chance_pct = JsonInt(json, "rune_chance_pct");
    pack->rune_count = JsonInt(json, "rune_count");
    pack->rune_min_rarity = JsonInt(json, "rune_min_rarity");
    pack->rune_max_rarity = JsonInt(json, "rune_max_rarity");
    pack->gear_chance_pct = JsonInt(json, "gear_chance_pct");
    pack->gear_min_rarity = JsonInt(json, "gear_min_rarity");
    pack->gear_max_rarity = JsonInt(json, "gear_max_rarity");

    const cJSON *weights = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "weights");
    static const char *const keys[] = {"", "Common", "Uncommon", "Rare", "Epic", "Legendary"};
    int total = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) {
        pack->weights[rarity] = JsonInt(weights, keys[rarity]);
        total += pack->weights[rarity];
        if (rarity < pack->minimum_rarity && pack->weights[rarity] != 0)
            TraceLog(LOG_FATAL, "Store: %s assigns weight below its minimum rarity", pack->id);
    }
    if (pack->hero_count <= 0 || pack->hero_count > STORE_MAX_RESULTS || pack->guaranteed_legendary < 0 ||
        pack->guaranteed_epic < 0 || pack->guaranteed_legendary + pack->guaranteed_epic > pack->hero_count ||
        pack->opened_counter < 0 || pack->opened_counter >= PLAYER_STORE_PACK_COUNT || total != 10000)
        TraceLog(LOG_FATAL, "Store: invalid hero pack %s", pack->id);
}

static void StoreLoadRuneContents(StorePackDef *pack, const cJSON *json) {
    const cJSON *slot = NULL;
    cJSON_ArrayForEach(slot, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "slots")) {
        if (pack->slot_count >= STORE_RUNE_PACK_SLOTS)
            TraceLog(LOG_FATAL, "Store: pack %s has too many slots", pack->id);
        StoreRuneSlot *rune_slot = &pack->slots[pack->slot_count++];
        rune_slot->min_rarity = JsonInt(slot, "min_rarity");
        rune_slot->max_rarity = JsonInt(slot, "max_rarity");
        const cJSON *family = NULL;
        cJSON_ArrayForEach(family, cJSON_GetObjectItemCaseSensitive((cJSON *)slot, "families")) {
            if (rune_slot->family_count < STORE_RUNE_SLOT_FAMILIES)
                rune_slot->families[rune_slot->family_count++] = family->valuestring;
        }
    }
    if (pack->slot_count == 0) TraceLog(LOG_FATAL, "Store: rune pack %s has no slots", pack->id);
}

static void StoreLoadGearContents(StorePackDef *pack) {
    for (int i = 0; i < GearPackCount(); i++)
        if (!strcmp(GearPackAt(i)->id, pack->id)) pack->gear_pack = i;
    if (pack->gear_pack < 0) TraceLog(LOG_FATAL, "Store: gear pack %s did not load", pack->id);
}

static StoreView StorePageFromCategory(const char *category) {
    if (!strcmp(category, "ITEMS")) return STORE_VIEW_ITEMS;
    if (!strcmp(category, "FEATURED")) return STORE_VIEW_FEATURED;
    return STORE_VIEW_HEROES;
}

static void StoreLoadPack(Store *store, StorePackDef *pack, const cJSON *json) {
    StrCopy(pack->id, sizeof(pack->id), JsonStr(json, "id"));
    pack->name = StorePackLocText(json, "_PACK_NAME", "name_ru");
    pack->info = StorePackLocText(json, "_PACK_INFO", "info_ru");
    pack->header = JsonStr(json, "header");
    pack->icon = JsonStr(json, "icon");
    pack->page = StorePageFromCategory(JsonStr(json, "category"));
    StoreLoadOffers(store, pack, json);

    pack->flash_index = -1;
    pack->gear_pack = -1;
    const cJSON *schedule = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "schedule");
    if (cJSON_IsObject(schedule)) {
        pack->schedule_start = StoreParseMonthDay(JsonStr(schedule, "start"));
        pack->schedule_end = StoreParseMonthDay(JsonStr(schedule, "end"));
    }
    const cJSON *rotation = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "rotation");
    if (cJSON_IsObject(rotation)) {
        pack->flash_index = JsonInt(rotation, "index");
        store->flash_count++;
    }

    const char *description = StorePackLocText(json, "_PACK_DESCRIPTION", NULL);
    if (description[0]) pack->bullets[pack->bullet_count++] = (StoreBullet){description, 0};
    const cJSON *bullet = NULL;
    cJSON_ArrayForEach(bullet, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "bullets")) {
        if (pack->bullet_count < STORE_PACK_BULLET_MAX)
            pack->bullets[pack->bullet_count++] = (StoreBullet){JsonStr(bullet, "text_ru"), JsonInt(bullet, "rarity")};
    }

    const char *kind = JsonStr(json, "kind");
    if (!strcmp(kind, "rune")) {
        pack->kind = STORE_KIND_RUNE;
        StoreLoadRuneContents(pack, json);
    } else if (!strcmp(kind, "gear")) {
        pack->kind = STORE_KIND_GEAR;
        StoreLoadGearContents(pack);
    } else {
        pack->kind = STORE_KIND_HERO;
        StoreLoadHeroContents(pack, json);
    }
    if (!pack->name[0] || !pack->info[0] || pack->bullet_count == 0)
        TraceLog(LOG_FATAL, "Store: invalid pack definition %s", pack->id);
}

static void StoreLoadGemSku(StoreGemSku *sku, const cJSON *json, int index) {
    char cost_key[64];
    snprintf(cost_key, sizeof(cost_key), "%s_cost", JsonStr(json, "id"));
    sku->cost = LocGet(cost_key);
    sku->gems = JsonInt(json, "gems");
    sku->icon = JsonInt(json, "icon");
    const char *banner_key = JsonStr(json, "banner");
    sku->banner = banner_key[0] ? LocGet(banner_key) : NULL;
    if (!sku->cost[0] || sku->gems <= 0 || sku->icon < 0 || sku->icon >= STORE_GEM_SKU_COUNT ||
        (sku->banner && !sku->banner[0]))
        TraceLog(LOG_FATAL, "Store: invalid gem sku at index %d", index);
}

static bool StoreHeroDefinitionEligible(const cJSON *entry) {
    if (!JsonBool(entry, "obtainable")) return false;
    const char *id = JsonStr(entry, "id");
    if (strstr(id, "WISP") || strstr(id, "BANNER")) return false;
    Rarity rarity = RarityFromString(JsonStr(entry, "rarity"));
    return rarity >= RARITY_COMMON && rarity <= RARITY_LEGENDARY;
}

static void StoreBuildRarityPools(Store *store) {
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, (cJSON *)store->catalog) {
        if (!StoreHeroDefinitionEligible(entry)) continue;
        Rarity rarity = RarityFromString(JsonStr(entry, "rarity"));
        store->rarity_pool_count[rarity]++;
    }
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) {
        int count = store->rarity_pool_count[rarity];
        if (count <= 0) TraceLog(LOG_FATAL, "Store: no obtainable heroes of rarity %d", rarity);
        store->rarity_pool[rarity] = (const cJSON **)MemAlloc((size_t)count * sizeof(cJSON *));
        store->rarity_pool_count[rarity] = 0;
    }
    cJSON_ArrayForEach(entry, (cJSON *)store->catalog) {
        if (!StoreHeroDefinitionEligible(entry)) continue;
        Rarity rarity = RarityFromString(JsonStr(entry, "rarity"));
        store->rarity_pool[rarity][store->rarity_pool_count[rarity]++] = entry;
    }
}

void StoreInitialize(Store *store, const char *config_path, const char *catalog_path) {
    memset(store, 0, sizeof(*store));
    store->config_root = JsonLoadFile(config_path);
    if (JsonInt(store->config_root, "version") != 3) TraceLog(LOG_FATAL, "Store: unsupported config version");
    StrCopy(store->balance_note_ru, sizeof(store->balance_note_ru), JsonStr(store->config_root, "balance_note_ru"));
    store->flash_days = JsonInt(store->config_root, "flash_days");
    const cJSON *voucher = NULL;
    cJSON_ArrayForEach(voucher, cJSON_GetObjectItemCaseSensitive(store->config_root, "vouchers")) {
        if (store->voucher_count >= PLAYER_VOUCHER_CAPACITY) TraceLog(LOG_FATAL, "Store: too many vouchers");
        store->voucher_icons[store->voucher_count++] = JsonStr(voucher, "icon");
    }
    store_voucher_kinds = store->voucher_count;
    const cJSON *pack = NULL;
    cJSON_ArrayForEach(pack, cJSON_GetObjectItemCaseSensitive(store->config_root, "packs")) {
        if (store->pack_count >= STORE_PACK_CAPACITY) TraceLog(LOG_FATAL, "Store: too many packs in %s", config_path);
        StoreLoadPack(store, &store->packs[store->pack_count++], pack);
    }
    const cJSON *gem_skus = cJSON_GetObjectItemCaseSensitive(store->config_root, "gem_skus");
    if (!cJSON_IsArray(gem_skus) || cJSON_GetArraySize(gem_skus) != STORE_GEM_SKU_COUNT)
        TraceLog(LOG_FATAL, "Store: expected %d gem skus in %s", STORE_GEM_SKU_COUNT, config_path);
    for (int i = 0; i < STORE_GEM_SKU_COUNT; i++)
        StoreLoadGemSku(&store->gem_skus[i], cJSON_GetArrayItem(gem_skus, i), i);

    store->catalog_root = JsonLoadFile(catalog_path);
    store->catalog = cJSON_GetObjectItemCaseSensitive(store->catalog_root, "characters");
    if (!cJSON_IsArray(store->catalog)) TraceLog(LOG_FATAL, "Store: catalog has no characters array");
    StoreBuildRarityPools(store);
    PackSceneInitialize(&store->opening_scene);
    PackFxInitialize(&store->opening_fx);
    PackHeroInitialize(&store->opening_hero, HOD_CACHE "char_pack");

    store->card = GuiTex(GUI_STORE_CARD);
    store->popup = GuiTex(GUI_STORE_POPUP);
    store->plate = GuiTex(GUI_STORE_PLATE);
    store->category_frame = GuiTex(GUI_STORE_CATEGORY_FRAME);
    store->category_art[STORE_TILE_FEATURED] = GuiTex(GUI_STORE_CATEGORY_FEATURED);
    store->category_art[STORE_TILE_HEROES] = GuiTex(GUI_STORE_CATEGORY_HEROES);
    store->category_art[STORE_TILE_ITEMS] = GuiTex(GUI_STORE_CATEGORY_ITEMS);
    store->category_art[STORE_TILE_GEMS] = GuiTex(GUI_STORE_CATEGORY_GEMS);
    store->category_art[STORE_TILE_VAULT] = GuiTex(GUI_STORE_CATEGORY_VAULT);
    store->timer_banner = GuiTex(GUI_STORE_TIMER_BANNER);
    store->timer_icon = GuiTex(GUI_STORE_TIMER_ICON);
    store->scroll_arrow = GuiTex(GUI_STORE_SCROLL_ARROW);
    store->currency_gold = GuiTex(GUI_HUD_GOLD);
    store->currency_gems = GuiTex(GUI_STORE_CURRENCY_GEMS);
    store->gem_card = GuiTex(GUI_STORE_GEM_CARD);
    store->gem_coin = GuiTex(GUI_STORE_GEM_COIN);
    store->gem_quest = GuiTex(GUI_STORE_GEM_QUEST);
    store->gem_info_frame = GuiTex(GUI_STORE_GEM_INFO_FRAME);
    store->banner_tex = GuiTex(GUI_STORE_BANNER);
    for (int i = 0; i < STORE_GEM_SKU_COUNT; i++) store->gem_icons[i] = GuiTex(GUI_STORE_GEM_ICON_0 + i);
    store->info_icon = GuiTex(GUI_STORE_INFO_ICON);
    store->info_icon_on = GuiTex(GUI_STORE_INFO_ICON_ON);
    store->title_font = GuiFont(FNT_COOL);
    store->body_font = GuiFont(FNT_MENU);

    store->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&store->close, SND_UI_POPUP_CLOSE);
    store->back = ButtonLoad(GUI_STORE_BACK, GUI_STORE_BACK);
    ButtonSetClickSound(&store->back, SND_UI_TAB);
    for (int i = 0; i < store->pack_count; i++) {
        store->buy[i] = ButtonLoad(GUI_STORE_BUY, GUI_STORE_BUY_ON);
        ButtonDisableClickSound(&store->buy[i]);
    }
    store->rune_details = -1;
    store->pack_rune_def = -1;
    store->pack_gear_def = -1;
    store->next =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ДАЛЕЕ", store->title_font, (Vector2){0.5f, 0.5f}, 0.34f);
    store->skip =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ПРОПУСТИТЬ", store->title_font, (Vector2){0.5f, 0.5f}, 0.29f);
    store->done =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ГОТОВО", store->title_font, (Vector2){0.5f, 0.5f}, 0.34f);
    store->buy_again =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "КУПИТЬ ЕЩЁ", store->title_font, (Vector2){0.5f, 0.5f}, 0.27f);
    store->item_done =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "ГОТОВО", store->title_font, (Vector2){0.5f, 0.5f}, 0.4f);
    store->item_again =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "КУПИТЬ ЕЩЁ", store->title_font, (Vector2){0.5f, 0.5f}, 0.36f);
    store->nav_train = ButtonLoad(GUI_BTN_PACK_TRAIN, GUI_BTN_PACK_TRAIN_ON);
    store->nav_runes = ButtonLoad(GUI_BTN_PACK_RUNES, GUI_BTN_PACK_RUNES_ON);
    store->nav_squad = ButtonLoad(GUI_BTN_PACK_SQUAD, GUI_BTN_PACK_SQUAD_ON);
    store->dialog_ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), store->title_font, (Vector2){0.5f, 0.5f}, 0.34f
    );
    store->dialog_cancel =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ОТМЕНА", store->title_font, (Vector2){0.5f, 0.5f}, 0.3f);
    store->view = STORE_VIEW_LANDING;
    store->return_view = STORE_VIEW_HEROES;
    store->info_pack = -1;
}

void StoreDestroy(Store *store) {
    PackHeroDestroy(&store->opening_hero);
    PackFxDestroy(&store->opening_fx);
    PackSceneDestroy(&store->opening_scene);
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) {
        if (store->rarity_pool[rarity]) MemFree((void *)store->rarity_pool[rarity]);
        store->rarity_pool[rarity] = NULL;
        store->rarity_pool_count[rarity] = 0;
    }
    cJSON_Delete(store->catalog_root);
    cJSON_Delete(store->config_root);
    store->catalog_root = NULL;
    store->config_root = NULL;
}

void StoreOpen(Store *store) {
    store->view = STORE_VIEW_LANDING;
    store->scroll = 0.0f;
    store->dragging = false;
    store->drag_scrolled = false;
    store->bullet_time = 0.0f;
    store->dialog = STORE_DIALOG_NONE;
    store->info_pack = -1;
    PackHeroClear(&store->opening_hero);
    store->phase_time = 0.0f;
    store->reveal_hitch_guard_frames = 0;
    store->status_time = 0.0f;
    store->status[0] = '\0';
    GuiSoundPlay(SND_STORE_OPEN);
}

static void StoreEnterPage(Store *store, StoreView target) {
    if (store->view == STORE_VIEW_OPENING || store->view == STORE_VIEW_RESULT) PackHeroClear(&store->opening_hero);
    store->view = target;
    store->scroll = 0.0f;
    store->info_pack = -1;
    store->dialog = STORE_DIALOG_NONE;
    store->status_time = 0.0f;
    store->rune_details = -1;
}

void StoreShowPage(Store *store, bool gems_page) {
    StoreView target = gems_page ? STORE_VIEW_GEMS : STORE_VIEW_HEROES;
    if (store->view != target) StoreEnterPage(store, target);
}

void StoreShowItems(Store *store) {
    StoreEnterPage(store, STORE_VIEW_ITEMS);
}

bool StoreDialogOpen(const Store *store) {
    return store->dialog != STORE_DIALOG_NONE;
}

bool StoreIsOnGemsPage(const Store *store) {
    return store->view == STORE_VIEW_GEMS;
}

bool StoreTopHudVisible(const Store *store) {
    (void)store;
    return true;
}

bool StoreTopHudHomeActive(const Store *store) {
    if (store->view != STORE_VIEW_OPENING && store->view != STORE_VIEW_RESULT) return true;
    return store->view == STORE_VIEW_RESULT && store->reveal_index + 1 >= store->result_count;
}

bool StoreUsesHomeBackground(const Store *store) {
    return store->view != STORE_VIEW_OPENING && store->view != STORE_VIEW_RESULT;
}

static float StoreCameraMix(const Store *store) {
    if (store->view == STORE_VIEW_RESULT) return 1.0f;
    return Clamp(
        (store->phase_time - STORE_OPEN_ROULETTE_END) / (STORE_OPEN_CAMERA_END - STORE_OPEN_ROULETTE_END), 0.0f, 1.0f
    );
}

void StoreDrawWorld(Store *store, Heroes *heroes) {
    bool final = store->view == STORE_VIEW_RESULT;
    PackHeroSetVisible(&store->opening_hero, final);

    float camera_mix = StoreCameraMix(store);
    PackReveal reveal = StoreCurrentReveal(store, heroes);
    PackSceneDraw(&store->opening_scene, camera_mix, store->phase_time, reveal);

    Camera3D camera = PackSceneCamera(camera_mix, reveal.tall);
    float fx_time = final ? STORE_OPEN_COMPLETE + store->result_time : store->phase_time;
    PackFxDraw(&store->opening_fx, camera, fx_time, final, reveal.rarity);
    store->opening_camera = camera;
    PackHeroDraw(&store->opening_hero, camera);
}

static void StoreBeginCurrentResult(Store *store);
static void StoreFinishCurrentResult(Store *store, Heroes *heroes, bool skipped);

StoreAction StoreHandleBack(Store *store, Heroes *heroes) {
    if (store->dialog != STORE_DIALOG_NONE) {
        store->dialog = STORE_DIALOG_NONE;
        store->dialog_press_armed = false;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return STORE_ACTION_NONE;
    }
    switch (store->view) {
        case STORE_VIEW_HEROES:
        case STORE_VIEW_ITEMS:
        case STORE_VIEW_FEATURED:
        case STORE_VIEW_VAULT:
        case STORE_VIEW_GEMS:
            store->view = STORE_VIEW_LANDING;
            store->info_pack = -1;
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            return STORE_ACTION_NONE;
        case STORE_VIEW_ITEM_REVEAL:
            if (store->rune_details >= 0)
                store->rune_details = -1;
            else
                store->view = store->packs[store->selected_pack].page;
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            return STORE_ACTION_NONE;
        case STORE_VIEW_OPENING:
            StoreFinishCurrentResult(store, heroes, true);
            return STORE_ACTION_NONE;
        case STORE_VIEW_RESULT:
            if (store->reveal_index + 1 < store->result_count) {
                store->reveal_index++;
                store->loaded_reveal_index = -1;
                StoreBeginCurrentResult(store);
            } else {
                store->view = store->return_view;
                PackHeroClear(&store->opening_hero);
                GuiSoundPlay(SND_UI_POPUP_CLOSE);
            }
            return STORE_ACTION_NONE;
        case STORE_VIEW_LANDING:
        default:
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            return STORE_ACTION_CLOSE;
    }
}

static const cJSON *StoreRandomDefinition(Store *store, Rarity rarity, uint32_t *rng) {
    int count = store->rarity_pool_count[rarity];
    return count > 0 ? store->rarity_pool[rarity][StoreRngRange(rng, count)] : NULL;
}

static bool StoreRuneFamilyMatches(const RuneDef *def, const StoreRuneSlot *slot) {
    for (int i = 0; i < slot->family_count; i++)
        if (!strcmp(def->family, slot->families[i])) return true;
    return slot->family_count == 0;
}

int StoreRollRune(const StoreRuneSlot *slot, uint32_t *rng) {
    int candidates[RUNE_CATALOG_CAPACITY];
    int count = 0;
    for (int i = 0; i < RuneCatalogCount(); i++) {
        const RuneDef *def = RuneDefAt(i);
        if (RuneDefRollable(def) && def->rarity >= slot->min_rarity && def->rarity <= slot->max_rarity &&
            StoreRuneFamilyMatches(def, slot))
            candidates[count++] = i;
    }
    return count > 0 ? candidates[StoreRngRange(rng, count)] : -1;
}

static int StoreRollPackRunes(const StorePackDef *pack, uint32_t *rng, PlayerRunes *runes, long long now) {
    StoreRuneSlot slot = {.min_rarity = pack->rune_min_rarity, .max_rarity = pack->rune_max_rarity};
    int first = -1;
    for (int i = 0; i < pack->rune_count; i++) {
        if (StoreRngRange(rng, 100) >= pack->rune_chance_pct) continue;
        int def = StoreRollRune(&slot, rng);
        if (def >= 0 && RuneGrant(runes, def, now) && first < 0) first = def;
    }
    return first;
}

static int StoreRollBonusGear(const StorePackDef *pack, uint32_t *rng) {
    if (StoreRngRange(rng, 100) >= pack->gear_chance_pct) return -1;
    GearPackDef roll = {.min_rarity = pack->gear_min_rarity, .max_rarity = pack->gear_max_rarity};
    return GearRollFromPack(&roll, rng);
}

void StoreAwardVoucherForWin(Player *player) {
    if (store_voucher_kinds <= 0) return;
    if (StoreRngRange(&player->store_rng_state, 100) >= STORE_VOUCHER_WIN_DROP_PCT) return;
    int voucher = StoreRngRange(&player->store_rng_state, store_voucher_kinds);
    if (player->pack_vouchers[voucher] < INT_MAX) player->pack_vouchers[voucher]++;
}

static void StoreOpenDialog(Store *store, StoreDialog dialog) {
    store->dialog = dialog;
    store->dialog_press_armed = false;
    GuiSoundPlay(SND_STORE_ERROR);
}

static void StoreBeginItemReveal(Store *store, int pack_index, StorePackKind kind, int count) {
    store->selected_pack = pack_index;
    store->item_kind = kind;
    store->item_result_count = count;
    store->item_reveal_time = 0.0f;
    store->rune_details = -1;
    store->view = STORE_VIEW_ITEM_REVEAL;
    GuiSoundPlay(SND_PACK_RARE);
}

static void StorePurchaseSaveFailed(Store *store) {
    StoreSetStatus(store, "Не удалось сохранить покупку");
    GuiSoundPlay(SND_STORE_ERROR);
}

static bool StoreTryBuyRunePack(Store *store, Player *player, int pack_index) {
    const StorePackDef *pack = &store->packs[pack_index];
    StoreOffer offer = StoreActiveOffer(pack, player);
    if (RUNE_INVENTORY_CAPACITY - player->runes.count < pack->slot_count) {
        StoreOpenDialog(store, STORE_DIALOG_ROSTER_FULL);
        return false;
    }
    if (!StoreOfferAffordable(offer, player)) {
        StoreOpenDialog(store, STORE_DIALOG_CANT_AFFORD);
        return false;
    }
    static PlayerRunes old_runes;
    static Player old_wallet;
    old_runes = player->runes;
    old_wallet = *player;
    uint32_t rng = player->store_rng_state;
    long long now = RuneNow();
    int defs[STORE_RUNE_PACK_SLOTS];
    for (int i = 0; i < pack->slot_count; i++) {
        defs[i] = StoreRollRune(&pack->slots[i], &rng);
        if (defs[i] < 0 || !RuneGrant(&player->runes, defs[i], now)) {
            player->runes = old_runes;
            StoreSetStatus(store, "Этот пак временно недоступен");
            GuiSoundPlay(SND_STORE_ERROR);
            return false;
        }
    }
    StoreOfferPay(offer, player);
    player->store_rng_state = rng;
    if (!ProgressionSave(player)) {
        *player = old_wallet;
        StorePurchaseSaveFailed(store);
        return false;
    }
    memcpy(store->item_results, defs, (size_t)pack->slot_count * sizeof(int));
    StoreBeginItemReveal(store, pack_index, STORE_KIND_RUNE, pack->slot_count);
    return true;
}

static int StoreCopyGearResults(Store *store, const GearPackDef *pack, const int *rolled) {
    int count = 0;
    for (int i = 0; i < GEAR_PACK_MAX_ROLLS && i <= pack->count; i++)
        if (rolled[i] != GEAR_ROLL_NONE) store->item_results[count++] = rolled[i];
    return count;
}

static bool StoreTryBuyGearPack(Store *store, Player *player, int pack_index) {
    const GearPackDef *pack = GearPackAt(store->packs[pack_index].gear_pack);
    int old_gold = player->gold, old_gems = player->gems, old_tomes = player->gear.tomes;
    uint32_t old_rng = player->store_rng_state;
    int rolled[GEAR_PACK_MAX_ROLLS];
    if (!PlayerBuyGearPack(player, pack, rolled)) {
        StoreOpenDialog(store, STORE_DIALOG_CANT_AFFORD);
        return false;
    }
    if (!ProgressionSave(player)) {
        for (int i = 0; i < pack->count; i++) player->gear.counts[rolled[i]]--;
        player->gear.tomes = old_tomes;
        player->gold = old_gold;
        player->gems = old_gems;
        player->store_rng_state = old_rng;
        StorePurchaseSaveFailed(store);
        return false;
    }
    StoreBeginItemReveal(store, pack_index, STORE_KIND_GEAR, StoreCopyGearResults(store, pack, rolled));
    return true;
}

static void StoreSortResults(const cJSON **definitions, Rarity *rarities, int count) {
    for (int i = 1; i < count; i++) {
        const cJSON *definition = definitions[i];
        Rarity rarity = rarities[i];
        int j = i - 1;
        while (j >= 0 && rarities[j] > rarity) {
            definitions[j + 1] = definitions[j];
            rarities[j + 1] = rarities[j];
            j--;
        }
        definitions[j + 1] = definition;
        rarities[j + 1] = rarity;
    }
}

static bool StoreTryBuyHeroPack(Store *store, Player *player, Heroes *heroes, int pack_index) {
    const StorePackDef *pack = &store->packs[pack_index];
    StoreOffer offer = StoreActiveOffer(pack, player);
    int counter = pack->opened_counter;
    if (!PlayerRosterHasCapacity(player, pack->hero_count)) {
        StoreOpenDialog(store, STORE_DIALOG_ROSTER_FULL);
        return false;
    }
    if (!StoreOfferAffordable(offer, player)) {
        StoreOpenDialog(store, STORE_DIALOG_CANT_AFFORD);
        return false;
    }
    if (player->store_opened_total == INT_MAX || player->store_opened_by_pack[counter] == INT_MAX) {
        StoreSetStatus(store, "Достигнут предел счётчика покупок");
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }

    uint32_t rng = player->store_rng_state;
    int old_roster_count = player->roster_count;
    uint64_t old_next_instance_id = player->next_instance_id;
    int old_gold = player->gold;
    int old_gems = player->gems;
    uint32_t old_rng = player->store_rng_state;
    int old_opened_total = player->store_opened_total;
    int old_opened_pack = player->store_opened_by_pack[counter];
    int old_champion_since = player->champion_since_legendary;
    int old_vouchers[PLAYER_VOUCHER_CAPACITY];
    memcpy(old_vouchers, player->pack_vouchers, sizeof(old_vouchers));
    const cJSON *definitions[STORE_MAX_RESULTS] = {0};
    Rarity rarities[STORE_MAX_RESULTS] = {0};
    bool tracks_pity = pack->legendary_pity > 0;
    int next_pity = player->champion_since_legendary;
    for (int i = 0; i < pack->hero_count; i++) {
        bool guaranteed_legendary = i < pack->guaranteed_legendary;
        bool guaranteed_epic =
            i >= pack->guaranteed_legendary && i < pack->guaranteed_legendary + pack->guaranteed_epic;
        bool pity = tracks_pity && next_pity >= pack->legendary_pity - 1;
        rarities[i] = (guaranteed_legendary || pity) ? RARITY_LEGENDARY
                                                     : (guaranteed_epic ? RARITY_EPIC : StoreRollRarity(pack, &rng));
        definitions[i] = StoreRandomDefinition(store, rarities[i], &rng);
        if (!definitions[i]) {
            StoreSetStatus(store, "Этот пак временно недоступен");
            GuiSoundPlay(SND_STORE_ERROR);
            return false;
        }
        if (tracks_pity) next_pity = rarities[i] == RARITY_LEGENDARY ? 0 : next_pity + 1;
    }
    if (pack->hero_count > 1) StoreSortResults(definitions, rarities, pack->hero_count);

    int first_index = -1;
    if (!PlayerRosterAppendDefinitions(player, definitions, pack->hero_count, &first_index)) {
        StoreSetStatus(store, "Не удалось добавить героев в ростер");
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }
    StoreOfferPay(offer, player);
    static PlayerRunes old_runes;
    old_runes = player->runes;
    int pack_rune = StoreRollPackRunes(pack, &rng, &player->runes, RuneNow());
    int pack_gear = StoreRollBonusGear(pack, &rng);
    if (pack_gear >= 0) GearGrant(&player->gear, pack_gear);

    player->store_rng_state = rng;
    player->store_opened_total++;
    player->missions.counters[MISSION_STAT_SUMMONED] += pack->hero_count;
    player->store_opened_by_pack[counter]++;
    if (tracks_pity) player->champion_since_legendary = next_pity;

    if (!ProgressionSave(player)) {
        PlayerRosterClearRange(player, old_roster_count, player->roster_count);
        player->runes = old_runes;
        if (pack_gear >= 0) player->gear.counts[pack_gear]--;
        player->roster_count = old_roster_count;
        player->next_instance_id = old_next_instance_id;
        player->gold = old_gold;
        player->gems = old_gems;
        memcpy(player->pack_vouchers, old_vouchers, sizeof(old_vouchers));
        player->store_rng_state = old_rng;
        player->store_opened_total = old_opened_total;
        player->store_opened_by_pack[counter] = old_opened_pack;
        player->champion_since_legendary = old_champion_since;
        StorePurchaseSaveFailed(store);
        return false;
    }

    if (!HeroesSyncRoster(heroes, player)) TraceLog(LOG_FATAL, "Store: persisted roster cannot sync to Heroes");
    store->selected_pack = pack_index;
    store->return_view = pack->page;
    store->pack_rune_def = pack_rune;
    store->pack_gear_def = pack_gear;
    store->result_count = pack->hero_count;
    for (int i = 0; i < pack->hero_count; i++) store->result_roster_indices[i] = first_index + i;
    store->reveal_index = 0;
    store->loaded_reveal_index = -1;
    store->status_time = 0.0f;
    StoreBeginCurrentResult(store);
    return true;
}

static bool StoreTryPurchase(Store *store, Player *player, Heroes *heroes, int pack_index) {
    if (pack_index < 0 || pack_index >= store->pack_count) return false;
    switch (store->packs[pack_index].kind) {
        case STORE_KIND_RUNE:
            return StoreTryBuyRunePack(store, player, pack_index);
        case STORE_KIND_GEAR:
            return StoreTryBuyGearPack(store, player, pack_index);
        default:
            return StoreTryBuyHeroPack(store, player, heroes, pack_index);
    }
}

static bool StoreRequestPurchase(Store *store, Player *player, Heroes *heroes, int pack_index) {
    StoreOffer offer = StoreActiveOffer(&store->packs[pack_index], player);
    store->pending_pack = pack_index;
    if (offer.voucher >= 0 && !StoreOfferAffordable(offer, player)) {
        StoreOpenDialog(store, STORE_DIALOG_VOUCHER);
        return false;
    }
    if (offer.voucher < 0 && offer.gems && StoreOfferAffordable(offer, player)) {
        store->dialog = STORE_DIALOG_CONFIRM_GEMS;
        store->dialog_press_armed = false;
        GuiSoundPlay(SND_STORE_INFO);
        return false;
    }
    return StoreTryPurchase(store, player, heroes, pack_index);
}

static bool StoreInputBlocked(const Store *store) {
    return store->dialog != STORE_DIALOG_NONE || store->drag_scrolled;
}

static bool StoreReleasedOn(const Store *store, Rectangle rect) {
    return !StoreInputBlocked(store) && CheckCollisionPointRec(UiMouse(), rect) &&
           IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void StoreDrawNineSlice(Texture2D texture, Rectangle dst, float border, Color tint) {
    float sx[4] = {0.0f, border, (float)texture.width - border, (float)texture.width};
    float sy[4] = {0.0f, border, (float)texture.height - border, (float)texture.height};
    float dx[4] = {dst.x, dst.x + border, dst.x + dst.width - border, dst.x + dst.width};
    float dy[4] = {dst.y, dst.y + border, dst.y + dst.height - border, dst.y + dst.height};
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            Rectangle src = {sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]};
            Rectangle out = {dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row]};
            if (src.width > 0.0f && src.height > 0.0f && out.width > 0.0f && out.height > 0.0f)
                DrawTexturePro(texture, src, out, (Vector2){0}, 0.0f, tint);
        }
    }
}

static void StoreDrawTextureCover(Texture2D texture, Rectangle dst, Color tint) {
    float src_w = (float)texture.width;
    float src_h = (float)texture.height;
    float src_aspect = src_w / src_h;
    float dst_aspect = dst.width / dst.height;
    Rectangle src = {0.0f, 0.0f, src_w, src_h};
    if (src_aspect > dst_aspect) {
        src.width = src_h * dst_aspect;
        src.x = (src_w - src.width) * 0.5f;
    } else {
        src.height = src_w / dst_aspect;
        src.y = (src_h - src.height) * 0.5f;
    }
    DrawTexturePro(texture, src, dst, (Vector2){0}, 0.0f, tint);
}

static Rectangle StorePlateRect(void) {
    float width = STORE_PLATE_HEIGHT * STORE_PLATE_ASPECT;
    return (Rectangle){(UiWidth() - width) * 0.5f, STORE_PLATE_TOP, width, STORE_PLATE_HEIGHT};
}

static Rectangle StoreListRect(void) {
    return UiRect(StorePlateRect(), 0.51f, 0.63f, 0.87f, 0.73f);
}

static float StoreCellStride(void) {
    return StoreListRect().height * STORE_CELL_ASPECT;
}

static Rectangle StoreArrowRect(bool left) {
    Rectangle plate = StorePlateRect();
    float height = plate.height * 0.15f;
    return UiRect(plate, left ? 0.0f : 1.0f, 0.5f, height * STORE_ARROW_ASPECT, height);
}

static Rectangle StoreCellPart(Rectangle cell, Rectangle part) {
    return (Rectangle){
        cell.x + part.x * cell.width, cell.y + part.y * cell.height, part.width * cell.width, part.height * cell.height
    };
}

static int StoreDrawPopupChrome(Store *store, const char *title, bool show_back) {
    DrawRectangle(0, 72, (int)UiWidth(), (int)UiHeight() - 72, (Color){0, 0, 0, 132});
    Rectangle popup = StorePlateRect();
    UiDrawTexture(store->plate, popup, WHITE);

    Rectangle title_rect = UiRect(popup, 0.51f, 0.02f, 0.37f, 0.1f);
    StoreDrawCenteredFit(
        store->title_font, title, title_rect, popup.height * STORE_TITLE_TEXT_OF_PLATE, (Color){242, 240, 228, 255}
    );

    float back_size = popup.height * 0.1f;
    float close_size = popup.height * 0.113f;
    int action = 0;
    bool interactive = !StoreInputBlocked(store);
    if (show_back && ButtonDrawEx(&store->back, UiRect(popup, 0.02f, 0.075f, back_size, back_size), interactive))
        action = 2;
    if (ButtonDrawEx(&store->close, UiRect(popup, 0.985f, 0.065f, close_size, close_size), interactive)) action = 1;
    if (!action && UiMouse().y > 72.0f && !CheckCollisionPointRec(UiMouse(), popup) &&
        StoreReleasedOn(store, (Rectangle){0.0f, 72.0f, UiWidth(), UiHeight() - 72.0f})) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        action = 1;
    }
    return action;
}

static void StoreDrawToast(Store *store) {
    if (store->status_time <= 0.0f || !store->status[0]) return;
    float y = store->view == STORE_VIEW_RESULT ? 596.0f : 660.0f;
    Rectangle toast = {390.0f, y, 500.0f, 42.0f};
    DrawRectangleRounded(toast, 0.24f, 6, (Color){70, 19, 17, 242});
    DrawRectangleLinesEx(toast, 2.0f, (Color){196, 92, 66, 255});
    StoreDrawCenteredFit(store->body_font, store->status, toast, 19.0f, WHITE);
}

static bool StoreDrawCategoryTile(Store *store, Rectangle rect, Texture2D art, const char *label, Color frame, float label_bottom) {
    bool hover = !StoreInputBlocked(store) && CheckCollisionPointRec(UiMouse(), rect);
    Rectangle inner = {
        rect.x + rect.width * STORE_TILE_ART_INSET_X,
        rect.y + rect.width * STORE_TILE_ART_INSET_X,
        rect.width * (1.0f - 2.0f * STORE_TILE_ART_INSET_X),
        rect.height * STORE_TILE_ART_HEIGHT
    };
    StoreDrawTextureCover(art, inner, WHITE);
    UiDrawTexture(store->category_frame, rect, hover ? ColorBrightness(frame, 0.12f) : frame);
    StoreDrawCenteredFit(
        store->title_font,
        label,
        (Rectangle){
            rect.x + 0.105f * rect.width,
            rect.y + rect.height * (1.0f - label_bottom) - STORE_TILE_LABEL_HEIGHT,
            0.79f * rect.width,
            STORE_TILE_LABEL_HEIGHT
        },
        STORE_TILE_LABEL_SIZE,
        (Color){244, 241, 225, 255}
    );
    return StoreReleasedOn(store, rect);
}

static Rectangle StoreLandingTileRect(int tile) {
    Rectangle list = StoreListRect();
    Rectangle part = STORE_LANDING_TILES[tile];
    return (Rectangle){
        list.x + part.x * list.width, list.y + part.y * list.height, part.width * list.width, part.height * list.height
    };
}

static StoreAction StoreDrawLanding(Store *store) {
    int chrome = StoreDrawPopupChrome(store, LocGet("STORE_TITLE"), false);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    static const StoreView targets[STORE_TILE_COUNT] = {
        STORE_VIEW_FEATURED, STORE_VIEW_HEROES, STORE_VIEW_ITEMS, STORE_VIEW_GEMS, STORE_VIEW_VAULT
    };
    const char *labels[STORE_TILE_COUNT] = {
        STORE_FEATURED_TITLE,
        LocGet("BUTTON_STORE_HERO_TAB"),
        STORE_ITEMS_TITLE,
        LocGet("BUTTON_STORE_GEMS_TAB"),
        STORE_VAULT_TITLE,
    };
    for (int tile = 0; tile < STORE_TILE_COUNT; tile++) {
        Color frame = tile == STORE_TILE_FEATURED ? STORE_FEATURED_FRAME_TINT : WHITE;
        float label_bottom =
            tile == STORE_TILE_FEATURED ? STORE_TILE_LABEL_BOTTOM_OF_FEATURED_TILE : STORE_TILE_LABEL_BOTTOM_OF_SQUARE_TILE;
        if (StoreDrawCategoryTile(store, StoreLandingTileRect(tile), store->category_art[tile], labels[tile], frame, label_bottom)) {
            StoreEnterPage(store, targets[tile]);
            GuiSoundPlay(SND_UI_TAB);
            break;
        }
    }
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawBuyPrice(Store *store, StoreOffer offer, const Player *player, Rectangle bar) {
    Color color = {235, 232, 220, 255};
    if (offer.voucher >= 0 || offer.price == 0) {
        const char *key = offer.voucher < 0                          ? "STORE_PACK_FREE"
                          : player->pack_vouchers[offer.voucher] > 0 ? "ALLY_CODE_INCENTIVE_CLAIM_BUTTON"
                                                                     : "STORE_FIND_BUTTON";
        StoreDrawCenteredFit(store->title_font, LocGet(key), bar, bar.height * STORE_PRICE_TEXT_OF_BAR, color);
        return;
    }
    char price[16];
    snprintf(price, sizeof(price), "%d", offer.price);
    UiDrawTextRight(
        store->title_font,
        price,
        (Rectangle){bar.x, bar.y, bar.width * STORE_PRICE_RIGHT_OF_BAR, bar.height},
        bar.height * STORE_PRICE_TEXT_OF_BAR,
        color
    );
    Texture2D icon = offer.gems ? store->currency_gems : store->currency_gold;
    float icon_size = bar.height * STORE_CURRENCY_SIZE_OF_BAR;
    UiDrawTexture(
        icon,
        (Rectangle){
            bar.x + bar.width * STORE_CURRENCY_CENTER_OF_BAR - icon_size * 0.5f,
            bar.y + (bar.height - icon_size) * 0.5f,
            icon_size,
            icon_size
        },
        WHITE
    );
}

static Texture2D StoreRarityBackdrop(StorePackKind kind, int rarity) {
    int tier = rarity < RARITY_COMMON ? 0 : (rarity > RARITY_LEGENDARY ? RARITY_LEGENDARY : rarity) - RARITY_COMMON;
    if (kind == STORE_KIND_HERO) return GuiTex(GUI_STORE_UNIT_RARITY_COMMON + tier);
    return GuiTex(GUI_STORE_RUNE_RARITY_COMMON + tier);
}

static void StoreDrawRarityIcon(StorePackKind kind, Rectangle rect, int rarity, Color tint) {
    if (kind == STORE_KIND_GEAR) {
        DrawRectangleRec(rect, ColorAlpha(BLACK, tint.a / 255.0f));
        UiDrawTexture(GearRarityFrame(rarity), rect, tint);
        return;
    }
    UiDrawTexture(StoreRarityBackdrop(kind, rarity), rect, tint);
}

static void StoreDrawRarityCard(Store *store, StorePackKind kind, Rectangle rect, int rarity, float text_size) {
    StoreDrawRarityIcon(kind, rect, rarity, WHITE);
    StoreDrawCenteredFit(store->title_font, "?", rect, text_size, (Color){236, 232, 220, 255});
}

static int StoreBulletCount(const StorePackDef *pack, const Player *player) {
    bool claimable = pack->voucher >= 0 && player->pack_vouchers[pack->voucher] > 0;
    return pack->bullet_count + (claimable ? STORE_VOUCHER_BULLETS : 0);
}

static StoreBullet StoreBulletAt(const StorePackDef *pack, const Player *player, int index) {
    int extra = StoreBulletCount(pack, player) - pack->bullet_count;
    if (index >= extra) return pack->bullets[index - extra];
    if (index == 1) return (StoreBullet){LocGet("FREE!"), 0};
    return (StoreBullet){TextFormat("%d %s", player->pack_vouchers[pack->voucher], LocGet("AVAILABLE")), 0};
}

static void
StoreDrawPackBullets(Store *store, const StorePackDef *pack, const Player *player, Rectangle rect, float size) {
    int index = 0;
    float alpha = 1.0f;
    int count = StoreBulletCount(pack, player);
    if (count > 1) {
        int step = (int)(store->bullet_time / STORE_BULLET_CYCLE_SECONDS);
        float phase = store->bullet_time - step * STORE_BULLET_CYCLE_SECONDS;
        index = step % count;
        if (phase >= STORE_BULLET_DISPLAY_SECONDS + STORE_BULLET_FADE_SECONDS) {
            index = (index + 1) % count;
            alpha = phase - STORE_BULLET_DISPLAY_SECONDS - STORE_BULLET_FADE_SECONDS;
        } else if (phase >= STORE_BULLET_DISPLAY_SECONDS) {
            alpha = 1.0f - (phase - STORE_BULLET_DISPLAY_SECONDS);
        }
    }
    StoreBullet bullet = StoreBulletAt(pack, player, index);
    Color color = {239, 234, 218, (unsigned char)(alpha * 255.0f)};
    if (bullet.rarity <= 0) {
        StoreDrawCenteredFit(store->title_font, bullet.text, rect, size, color);
        return;
    }
    float icon = rect.height;
    float text_width = MeasureTextEx(store->title_font, bullet.text, size, 1.0f).x;
    float gap = icon * STORE_BULLET_ICON_GAP_OF_ICON;
    float group = fminf(text_width + gap + icon, rect.width);
    float left = rect.x + (rect.width - group) * 0.5f;
    Rectangle text = {left, rect.y, group - gap - icon, rect.height};
    StoreDrawCenteredFit(store->title_font, bullet.text, text, size, color);
    Rectangle card = {left + group - icon, rect.y, icon, icon};
    StoreDrawRarityIcon(pack->kind, card, bullet.rarity, Fade(WHITE, alpha));
}

static const char *StoreShortTime(long long seconds) {
    long long minutes = seconds / 60;
    long long hours = minutes / 60;
    long long days = hours / 24;
    if (days > 0) return TextFormat("%dд %dч", (int)days, (int)(hours % 24));
    if (hours > 0) return TextFormat("%dч %dм", (int)hours, (int)(minutes % 60));
    return TextFormat("%dм", (int)fmaxf((float)minutes, 1.0f));
}

static void StoreDrawTimerBanner(Store *store, Rectangle cell, long long seconds) {
    Rectangle banner = StoreCellPart(cell, STORE_CELL_TIMER);
    DrawRectangleRec(banner, STORE_TIMER_BANNER_COLOR);
    float icon = banner.height * 0.72f;
    UiDrawTexture(
        store->timer_icon, (Rectangle){banner.x + 5.0f, banner.y + (banner.height - icon) * 0.5f, icon, icon}, WHITE
    );
    Rectangle text = {banner.x + icon + 7.0f, banner.y, banner.width - icon - 11.0f, banner.height};
    StoreDrawCenteredFit(store->body_font, StoreShortTime(seconds), text, banner.height * 0.62f, WHITE);
}

static bool StoreDrawPackCard(
    Store *store, Player *player, Heroes *heroes, int pack_index, Rectangle cell, long long seconds_left
) {
    const StorePackDef *pack = &store->packs[pack_index];
    bool show_info = store->info_pack == pack_index;
    Vector2 mouse = UiMouse();
    bool in_viewport = CheckCollisionPointRec(mouse, StoreListRect()) &&
                       !CheckCollisionPointRec(mouse, StoreArrowRect(true)) &&
                       !CheckCollisionPointRec(mouse, StoreArrowRect(false));
    Rectangle art = StoreCellPart(cell, STORE_CELL_ICON);
    DrawRectangleRec(art, BLACK);
    Texture2D icon = RuneIconLoad(pack->icon);
    if (!show_info && icon.id != 0) StoreDrawTextureCover(icon, art, WHITE);
    UiDrawTexture(store->card, StoreCellPart(cell, STORE_CELL_CARD), WHITE);

    Rectangle top = StoreCellPart(cell, STORE_CELL_HEADER);
    Texture2D header = RuneIconLoad(pack->header);
    if (header.id != 0) UiDrawTexture(header, top, WHITE);
    StoreDrawCenteredFit(
        store->title_font,
        pack->name,
        (Rectangle){top.x + top.width * 0.06f, top.y, top.width * 0.88f, top.height},
        cell.height * STORE_HEADER_TEXT_OF_CELL,
        WHITE
    );

    if (show_info)
        CampaignDrawWrapped(
            store->body_font,
            pack->info,
            (Vector2){art.x + 6.0f, art.y + 12.0f},
            18.0f,
            1.0f,
            5.0f,
            art.width - 12.0f,
            (Color){236, 232, 220, 255}
        );

    if (seconds_left > 0) StoreDrawTimerBanner(store, cell, seconds_left);

    Rectangle info = StoreCellPart(cell, STORE_CELL_INFO);
    bool info_hover = !StoreInputBlocked(store) && CheckCollisionPointRec(mouse, info);
    UiDrawTexture(info_hover || show_info ? store->info_icon_on : store->info_icon, info, WHITE);

    StoreDrawPackBullets(
        store, pack, player, StoreCellPart(cell, STORE_CELL_BULLET), cell.height * STORE_BULLET_TEXT_OF_CELL
    );

    Rectangle buy = StoreCellPart(cell, STORE_CELL_BUY);
    bool clicked = ButtonDrawEx(&store->buy[pack_index], buy, !StoreInputBlocked(store) && in_viewport);
    StoreDrawBuyPrice(store, StoreActiveOffer(pack, player), player, buy);

    if (in_viewport && StoreReleasedOn(store, StoreCellPart(cell, STORE_CELL_TAP_AREA))) {
        store->info_pack = show_info ? -1 : pack_index;
        GuiSoundPlay(SND_STORE_INFO);
    }
    if (!clicked) return false;
    if (pack->kind == STORE_KIND_HERO) store->return_view = pack->page;
    return StoreRequestPurchase(store, player, heroes, pack_index);
}

static Rectangle StoreRevealCardRect(const Store *store, int index) {
    float width =
        fminf(STORE_REVEAL_CARD_WIDTH, (STORE_REVEAL_AREA_WIDTH / store->item_result_count) - STORE_REVEAL_CARD_GAP);
    float height = width * STORE_REVEAL_CARD_ASPECT;
    return (Rectangle){
        STORE_REVEAL_LEFT + index * (width + STORE_REVEAL_CARD_GAP),
        STORE_REVEAL_CENTER_Y - height * 0.5f,
        width,
        height
    };
}

static int StoreItemRarity(const Store *store, int result) {
    int def = store->item_results[result];
    if (store->item_kind == STORE_KIND_RUNE) return RuneDefAt(def)->rarity;
    return def == GEAR_ROLL_TOME ? GEAR_RARITY_COUNT : GearDefAt(def)->rarity;
}

static void StoreDrawRevealBurst(Rectangle card, float age) {
    if (age >= STORE_REVEAL_BURST_SECONDS) return;
    float fade = age / STORE_REVEAL_BURST_SECONDS;
    Rectangle burst = StoreScaledAround(card, STORE_REVEAL_BURST_SCALE);
    Texture2D texture = GuiTex(GUI_RESULTS_STARBURST);
    DrawTexturePro(
        texture,
        (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height},
        (Rectangle){burst.x + burst.width * 0.5f, burst.y + burst.height * 0.5f, burst.width, burst.height},
        (Vector2){burst.width * 0.5f, burst.height * 0.5f},
        STORE_PACK_RUNE_BURST_SPIN_DEGREES * fade,
        Fade(WHITE, 1.0f - fade * fade)
    );
}

static void StoreDrawRevealItem(const Store *store, int result, Rectangle card) {
    int def = store->item_results[result];
    if (store->item_kind == STORE_KIND_RUNE)
        RunesDrawTile(RuneDefAt(def), card, false);
    else if (def == GEAR_ROLL_TOME)
        UiDrawTexture(GearTomeIconLoad(), card, WHITE);
    else
        GearDrawTile(GearDefAt(def), card, false);
}

static StoreAction StoreDrawItemReveal(Store *store, Player *player, Heroes *heroes, float dt) {
    bool runes = store->item_kind == STORE_KIND_RUNE;
    int chrome = StoreDrawPopupChrome(
        store, runes ? LocGet("RUNE_REVEAL_WINDOW_TITLE") : GearText("GEAR_REVEAL_WINDOW_TITLE"), false
    );
    if (chrome == 1) return STORE_ACTION_CLOSE;
    if (store->rune_details < 0) store->item_reveal_time += dt;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < store->item_result_count; i++) {
            Rectangle card = StoreRevealCardRect(store, i);
            float age = store->item_reveal_time - (i + 1) * STORE_REVEAL_STEP;
            if (pass == 0) {
                if (age >= 0.0f) StoreDrawRevealBurst(card, age);
                continue;
            }
            if (age < 0.0f) {
                StoreDrawRarityCard(store, store->item_kind, card, StoreItemRarity(store, i), card.height * 0.6f);
                continue;
            }
            StoreDrawRevealItem(store, i, card);
            if (runes && store->rune_details < 0 && StoreReleasedOn(store, card)) {
                store->rune_details = i;
                GuiSoundPlay(SND_STORE_INFO);
                return STORE_ACTION_NONE;
            }
        }
    }
    bool interactive = !StoreInputBlocked(store) && store->rune_details < 0;
    bool again = ButtonDrawEx(&store->item_again, STORE_REVEAL_AGAIN_RECT, interactive);
    bool done = ButtonDrawEx(&store->item_done, STORE_REVEAL_DONE_RECT, interactive);
    if (store->rune_details >= 0) {
        float height = 480.0f;
        float cx = StorePlateRect().x + StorePlateRect().width * 0.5f;
        const RuneDef *def = RuneDefAt(store->item_results[store->rune_details]);
        RunesDrawCard(def, NULL, (Rectangle){cx - height * 0.39f, 150.0f, height * 0.78f, height}, 0);
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) store->rune_details = -1;
    }
    if (done) store->view = store->packs[store->selected_pack].page;
    if (again && StoreRequestPurchase(store, player, heroes, store->selected_pack)) return STORE_ACTION_PURCHASED;
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawScrollArrow(Store *store, Rectangle rect, bool left) {
    Rectangle src = {0.0f, 0.0f, (float)store->scroll_arrow.width, (float)store->scroll_arrow.height};
    if (left) {
        src.x = (float)store->scroll_arrow.width;
        src.width = -(float)store->scroll_arrow.width;
    }
    Color tint = WHITE;
    if (!StoreInputBlocked(store) && CheckCollisionPointRec(UiMouse(), rect)) tint = (Color){255, 224, 133, 255};
    DrawTexturePro(store->scroll_arrow, src, rect, (Vector2){0}, 0.0f, tint);
}

static float StoreMaxScroll(const Store *store) {
    float content = store->visible_count * StoreCellStride();
    float visible = StoreListRect().width;
    return content > visible ? content - visible : 0.0f;
}

static void StoreUpdateScrollInput(Store *store) {
    Rectangle viewport = StoreListRect();
    Vector2 mouse = UiMouse();
    if (store->dialog == STORE_DIALOG_NONE) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, viewport)) {
            store->dragging = true;
            store->drag_start_x = mouse.x;
            store->scroll_start = store->scroll;
        }
        if (store->dragging) {
            float delta = mouse.x - store->drag_start_x;
            if (fabsf(delta) > 13.0f) store->drag_scrolled = true;
            if (store->drag_scrolled) store->scroll = store->scroll_start - delta;
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) store->dragging = false;
        } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            store->drag_scrolled = false;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && CheckCollisionPointRec(mouse, viewport)) store->scroll -= wheel * 160.0f;
    }
    store->scroll = Clamp(store->scroll, 0.0f, StoreMaxScroll(store));
}

static void StoreRefreshVisible(Store *store, StoreView page) {
    store->visible_count = 0;
    long long now = StoreNow(store);
    for (int i = 0; i < store->pack_count; i++) {
        const StorePackDef *pack = &store->packs[i];
        if (pack->page != page) continue;
        if (StorePackSecondsLeft(pack, store->flash_days, store->flash_count, now) == STORE_PACK_UNAVAILABLE) continue;
        store->visible[store->visible_count++] = i;
    }
}

static void StoreDrawScrollLine(const Store *store, Rectangle list) {
    float max_scroll = StoreMaxScroll(store);
    if (max_scroll <= 0.0f) return;
    float width = list.width * list.width / (list.width + max_scroll);
    float x = list.x + (list.width - width) * (store->scroll / max_scroll);
    float y = list.y + list.height + STORE_SCROLL_LINE_GAP;
    DrawRectangleRec((Rectangle){list.x, y, list.width, 1.0f}, (Color){70, 68, 62, 255});
    DrawRectangleRec((Rectangle){x, y, width, 1.0f}, (Color){176, 172, 158, 255});
}

static StoreAction StoreDrawPackPage(Store *store, Player *player, Heroes *heroes, const char *title) {
    int chrome = StoreDrawPopupChrome(store, title, true);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    if (chrome == 2) {
        StoreEnterPage(store, STORE_VIEW_LANDING);
        return STORE_ACTION_NONE;
    }

    StoreView page = store->view;
    StoreRefreshVisible(store, page);
    StoreUpdateScrollInput(store);

    Rectangle list = StoreListRect();
    float scroll_step = list.width * STORE_SCROLL_STEP_OF_LIST;
    Rectangle arrow_left = StoreArrowRect(true);
    Rectangle arrow_right = StoreArrowRect(false);
    bool can_left = store->scroll > 0.0f;
    bool can_right = store->scroll < StoreMaxScroll(store);
    if (can_left && StoreReleasedOn(store, arrow_left)) {
        store->scroll -= scroll_step;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (can_right && StoreReleasedOn(store, arrow_right)) {
        store->scroll += scroll_step;
        GuiSoundPlay(SND_UI_TAB);
    }
    store->scroll = Clamp(store->scroll, 0.0f, StoreMaxScroll(store));

    StoreAction action = STORE_ACTION_NONE;
    long long now = StoreNow(store);
    float scale = UiScale();
    BeginScissorMode(
        (int)(list.x * scale), (int)(list.y * scale), (int)(list.width * scale), (int)(list.height * scale)
    );
    for (int slot = 0; slot < store->visible_count; slot++) {
        float stride = StoreCellStride();
        float cell_x = list.x + slot * stride - store->scroll;
        if (cell_x + stride < list.x || cell_x > list.x + list.width) continue;
        Rectangle card = {cell_x, list.y, stride, list.height};
        int pack = store->visible[slot];
        long long left = StorePackSecondsLeft(&store->packs[pack], store->flash_days, store->flash_count, now);
        if (StoreDrawPackCard(store, player, heroes, pack, card, left)) {
            action = STORE_ACTION_PURCHASED;
            break;
        }
        if (store->view != page) break;
    }
    EndScissorMode();
    if (store->view != page) return action;
    StoreDrawScrollLine(store, list);
    if (store->scroll > 0.0f) StoreDrawScrollArrow(store, arrow_left, true);
    if (store->scroll < StoreMaxScroll(store)) StoreDrawScrollArrow(store, arrow_right, false);
    StoreDrawToast(store);
    return action;
}

static StoreAction StoreDrawVaultPage(Store *store) {
    int chrome = StoreDrawPopupChrome(store, STORE_VAULT_TITLE, true);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    if (chrome == 2) {
        StoreEnterPage(store, STORE_VIEW_LANDING);
        return STORE_ACTION_NONE;
    }
    StoreDrawCenteredFit(
        store->title_font, STORE_COMING_SOON_TEXT, StoreListRect(), 44.0f, (Color){236, 232, 220, 255}
    );
    return STORE_ACTION_NONE;
}

static StoreAction StoreDrawGemsPage(Store *store) {
    int chrome = StoreDrawPopupChrome(store, LocGet("BUTTON_STORE_GEMS_TAB"), true);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    if (chrome == 2) {
        store->view = STORE_VIEW_LANDING;
        return STORE_ACTION_NONE;
    }

    StoreAction action = STORE_ACTION_NONE;
    Rectangle list = StoreListRect();
    float cell_w = list.width * 0.245f;
    float cell_h = list.height * 0.5f;
    for (int i = 0; i < STORE_GEM_SKU_COUNT + 1; i++) {
        Rectangle cell = {list.x + (i % 4) * cell_w, list.y + (i / 4) * cell_h, cell_w, cell_h};
        Rectangle card = {cell.x + cell.width * 0.05f, cell.y, cell.width * 0.9f, cell.height * 0.72f};
        UiDrawTexture(store->gem_card, card, WHITE);

        if (i == 0) {
            Rectangle window = {card.x + 14.0f, card.y + 12.0f, card.width - 28.0f, card.height - 24.0f};
            float fit = fminf(window.width / store->gem_quest.width, window.height / store->gem_quest.height);
            UiDrawTexture(
                store->gem_quest,
                (Rectangle){
                    window.x + (window.width - store->gem_quest.width * fit) * 0.5f,
                    window.y + (window.height - store->gem_quest.height * fit) * 0.5f,
                    store->gem_quest.width * fit,
                    store->gem_quest.height * fit
                },
                WHITE
            );
            Rectangle tip = {cell.x + 6.0f, card.y + card.height + 8.0f, cell.width - 12.0f, 52.0f};
            StoreDrawNineSlice(store->gem_info_frame, tip, 14.0f, WHITE);
            StoreDrawCenteredFit(
                store->body_font,
                LocGet("STORE_QUEST_FOR_GEMS"),
                (Rectangle){tip.x + 8.0f, tip.y + 6.0f, tip.width - 16.0f, tip.height - 12.0f},
                21.0f,
                (Color){235, 232, 220, 255}
            );
            if (StoreReleasedOn(store, cell)) {
                GuiSoundPlay(SND_UI_TAB);
                action = STORE_ACTION_GOTO_MAP;
            }
            continue;
        }

        const StoreGemSku *sku = &store->gem_skus[i - 1];
        StoreDrawTextureCover(
            store->gem_icons[sku->icon],
            (Rectangle){card.x + 16.0f, card.y + 10.0f, card.width - 32.0f, card.height - 52.0f},
            WHITE
        );
        char quantity[16];
        snprintf(quantity, sizeof(quantity), "%d", sku->gems);
        float qty_size = 27.0f;
        Vector2 measured = MeasureTextEx(store->title_font, quantity, qty_size, 1.0f);
        float coin_size = 28.0f;
        float qty_right = card.x + card.width * 0.61f;
        DrawTextEx(
            store->title_font,
            quantity,
            (Vector2){qty_right - measured.x, card.y + card.height - 38.0f},
            qty_size,
            1.0f,
            WHITE
        );
        UiDrawTexture(
            store->gem_coin, (Rectangle){qty_right + 4.0f, card.y + card.height - 40.0f, coin_size, coin_size}, WHITE
        );
        if (sku->banner) {
            Rectangle ribbon = {
                card.x + card.width * 0.075f, card.y + card.height * 0.56f, card.width * 0.85f, card.height * 0.28f
            };
            UiDrawTexture(store->banner_tex, ribbon, WHITE);
            StoreDrawCenteredFit(
                store->body_font,
                sku->banner,
                (Rectangle){
                    ribbon.x + 10.0f, ribbon.y + ribbon.height * 0.22f, ribbon.width - 20.0f, ribbon.height * 0.5f
                },
                19.0f,
                WHITE
            );
        }

        Rectangle bar = {cell.x + 4.0f, card.y + card.height + 10.0f, cell.width - 8.0f, 36.0f};
        UiDrawTexture(GuiTex(GUI_STORE_BUY), bar, WHITE);
        StoreDrawCenteredFit(store->title_font, sku->cost, bar, 24.0f, (Color){235, 232, 220, 255});
        if (StoreReleasedOn(store, cell)) {
            StoreSetStatus(store, "Покупки за реальные деньги недоступны в оффлайн-версии");
            GuiSoundPlay(SND_STORE_INFO);
        }
    }

    StoreDrawCenteredFit(
        store->body_font,
        LocGet("STORE_SUBTEXT_GEMS"),
        (Rectangle){list.x, list.y + list.height, list.width, STORE_GEM_SUBTEXT_HEIGHT},
        19.0f,
        (Color){206, 202, 190, 255}
    );
    const char *stamina_note = LocGet("GEM_STORE_STAMINA_REFRESH_DESC");
    char stripped[256];
    StrCopy(stripped, sizeof(stripped), stamina_note);
    char *tag_open = strstr(stripped, ">");
    char *tag_close = strstr(stripped, "</color>");
    const char *display = stamina_note;
    if (tag_open && tag_close) {
        *tag_close = '\0';
        display = tag_open + 1;
    }
    StoreDrawCenteredFit(
        store->body_font,
        display,
        (Rectangle){list.x, list.y + list.height + STORE_GEM_SUBTEXT_HEIGHT, list.width, STORE_GEM_SUBTEXT_HEIGHT},
        19.0f,
        (Color){255, 0, 255, 255}
    );

    StoreDrawToast(store);
    return action;
}

static void StoreEnsureCurrentHero(Store *store, Heroes *heroes) {
    if (store->loaded_reveal_index == store->reveal_index) return;
    HeroCard *card = StoreCurrentCard(store, heroes);
    if (card)
        PackHeroSetCharacter(&store->opening_hero, card->ch);
    else
        PackHeroClear(&store->opening_hero);
    store->loaded_reveal_index = store->reveal_index;
    if (store->view == STORE_VIEW_OPENING) store->reveal_hitch_guard_frames = 2;
}

static void StoreBeginCurrentResult(Store *store) {
    store->view = STORE_VIEW_OPENING;
    store->phase_time = 0.0f;
    store->result_time = 0.0f;
    store->sequence_sound_played = false;
    store->rarity_sound_played = false;
    store->melee_played = false;
    store->reveal_hitch_guard_frames = 0;
    GuiSoundPlay(SND_PACK_SHUFFLE);
}

static void StoreFinishCurrentResult(Store *store, Heroes *heroes, bool skipped) {
    if (skipped) {
        GuiSoundStop(SND_PACK_SHUFFLE);
        GuiSoundStop(SND_PACK_REVEAL);
    }
    store->view = STORE_VIEW_RESULT;
    store->phase_time = STORE_OPEN_COMPLETE;
    store->result_time = 0.0f;
    PackHeroSetVisible(&store->opening_hero, true);
    if (!store->rarity_sound_played) {
        StorePlayCurrentRaritySound(store, heroes);
        store->rarity_sound_played = true;
    }
    if (!store->melee_played) {
        PackHeroPlayMelee(&store->opening_hero);
        store->melee_played = true;
    }
}

static StoreAction StoreDrawOpening(Store *store, Heroes *heroes) {
    if (ButtonDraw(&store->skip, (Rectangle){1044.0f, 654.0f, 236.0f, 59.0f}))
        StoreFinishCurrentResult(store, heroes, true);
    return STORE_ACTION_NONE;
}

static bool StoreDrawResultNavigation(Store *store, bool last) {
    if (!last) return false;
    store->nav_consume_clicked =
        ButtonDrawEx(&store->nav_train, (Rectangle){4.0f, 662.0f, 106.0f, 49.5f}, !StoreInputBlocked(store));
    ButtonDrawEx(&store->nav_runes, (Rectangle){120.0f, 662.0f, 106.0f, 49.5f}, false);
    return ButtonDrawEx(&store->nav_squad, (Rectangle){236.0f, 662.0f, 106.0f, 49.5f}, !StoreInputBlocked(store));
}

static Rectangle StoreScaledAround(Rectangle rect, float scale) {
    float width = rect.width * scale, height = rect.height * scale;
    return (Rectangle){rect.x + (rect.width - width) * 0.5f, rect.y + (rect.height - height) * 0.5f, width, height};
}

static float StoreClamp01(float value) {
    return Clamp(value, 0.0f, 1.0f);
}

static void StoreDrawPackBonus(const Store *store, Texture2D icon, int rarity) {
    float time = store->result_time;
    Rectangle slot = STORE_PACK_RUNE_RECT;

    float burst_time = time - STORE_PACK_RUNE_BURST_DELAY;
    if (burst_time >= 0.0f && burst_time < STORE_PACK_RUNE_BURST_DURATION) {
        float fade = StoreClamp01(burst_time / STORE_PACK_RUNE_BURST_DURATION);
        float peak = STORE_PACK_RUNE_BURST_PEAK_BASE * (1.0f + STORE_PACK_RUNE_BURST_PEAK_PER_RARITY * (float)rarity);
        float swell =
            burst_time < STORE_PACK_RUNE_BURST_SWELL_DURATION
                ? Lerp(1.0f, peak, StoreClamp01(burst_time / STORE_PACK_RUNE_BURST_SWELL_DURATION))
                : Lerp(
                      peak,
                      1.0f,
                      StoreClamp01(
                          (burst_time - (STORE_PACK_RUNE_BURST_SWELL_RETURN_DELAY - STORE_PACK_RUNE_BURST_DELAY)) /
                          STORE_PACK_RUNE_BURST_SWELL_DURATION
                      )
                  );
        Rectangle burst = StoreScaledAround(slot, STORE_PACK_RUNE_BURST_SCALE * swell);
        Texture2D texture = GuiTex(GUI_RESULTS_STARBURST);
        DrawTexturePro(
            texture,
            (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height},
            (Rectangle){burst.x + burst.width * 0.5f, burst.y + burst.height * 0.5f, burst.width, burst.height},
            (Vector2){burst.width * 0.5f, burst.height * 0.5f},
            STORE_PACK_RUNE_BURST_SPIN_DEGREES * (burst_time / STORE_PACK_RUNE_BURST_DURATION),
            Fade(WHITE, 1.0f - fade * fade * fade)
        );
    }

    UiDrawTexture(
        icon,
        StoreScaledAround(slot, STORE_PACK_RUNE_ICON_SCALE),
        Fade(WHITE, StoreClamp01(time / STORE_PACK_RUNE_FADE_IN))
    );
}

static StoreAction StoreDrawResult(Store *store, Player *player, Heroes *heroes) {
    HeroCard *card = StoreCurrentCard(store, heroes);
    bool last = store->reveal_index + 1 >= store->result_count;
    if (card && store->result_time >= 0.08f) HeroesDrawDetailsAt(card, (Rectangle){17.0f, 91.0f, 433.0f, 555.0f});
    bool first_result = store->reveal_index == 0;
    if (store->pack_gear_def >= 0 && first_result) {
        const GearDef *gear = GearDefAt(store->pack_gear_def);
        StoreDrawPackBonus(store, GearIconLoad(gear), gear->rarity);
    } else if (store->pack_rune_def >= 0 && first_result) {
        const RuneDef *rune = RuneDefAt(store->pack_rune_def);
        StoreDrawPackBonus(store, RuneIconLoad(rune->icon), rune->rarity);
    }
    bool squad = StoreDrawResultNavigation(store, last);

    bool interactive = !StoreInputBlocked(store);
    if (!last) {
        if (ButtonDrawEx(&store->next, (Rectangle){1044.0f, 654.0f, 236.0f, 59.0f}, interactive)) {
            store->reveal_index++;
            store->loaded_reveal_index = -1;
            StoreBeginCurrentResult(store);
        }
        return STORE_ACTION_NONE;
    }

    bool again = ButtonDrawEx(&store->buy_again, (Rectangle){808.0f, 654.0f, 236.0f, 59.0f}, interactive);
    if (ButtonDrawEx(&store->done, (Rectangle){1044.0f, 654.0f, 236.0f, 59.0f}, interactive)) {
        store->view = store->return_view;
        store->info_pack = -1;
        PackHeroClear(&store->opening_hero);
        return STORE_ACTION_NONE;
    }
    if (squad) {
        PackHeroClear(&store->opening_hero);
        return STORE_ACTION_GOTO_SQUAD;
    }
    if (store->nav_consume_clicked) {
        PackHeroClear(&store->opening_hero);
        return STORE_ACTION_GOTO_CONSUME;
    }
    if (again && StoreRequestPurchase(store, player, heroes, store->selected_pack)) return STORE_ACTION_PURCHASED;
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreFormatPackName(char *out, size_t size, const char *format, const char *name) {
    const char *slot = strstr(format, "{0}");
    if (!slot) {
        StrCopy(out, size, format);
        return;
    }
    snprintf(out, size, "%.*s%s%s", (int)(slot - format), format, name, slot + strlen("{0}"));
}

static void StoreDialogText(const Store *store, const Player *player, char *header, char *body, size_t size) {
    const char *name = store->packs[store->pending_pack].name;
    if (store->dialog == STORE_DIALOG_VOUCHER) {
        StoreFormatPackName(header, size, LocGet("VOUCHER_PACK_UNAVAILABLE_TITLE"), name);
        StrCopy(body, size, LocGet("VOUCHER_PACK_UNAVAILABLE_BODY"));
    } else if (store->dialog == STORE_DIALOG_CONFIRM_GEMS) {
        StrCopy(header, size, name);
        StrCopy(
            body,
            size,
            TextFormat(STORE_CONFIRM_GEMS_FORMAT, StoreActiveOffer(&store->packs[store->pending_pack], player).price)
        );
    } else {
        bool afford = store->dialog == STORE_DIALOG_CANT_AFFORD;
        StrCopy(header, size, LocGet(afford ? "MESSAGE_CANT_AFFORD_HEADER" : "MESSAGE_TOO_MANY_HEADER"));
        StrCopy(body, size, LocGet(afford ? "MESSAGE_CANT_AFFORD_BODY" : "MESSAGE_TOO_MANY_BODY_NEW_CONSUME"));
    }
}

static bool StoreDrawDialog(Store *store, Player *player, Heroes *heroes) {
    if (store->dialog == STORE_DIALOG_NONE) return false;
    char header[STORE_DIALOG_TEXT_CAPACITY];
    char body[STORE_DIALOG_TEXT_CAPACITY];
    StoreDialogText(store, player, header, body, sizeof(header));

    DrawRectangle(0, 0, (int)UiWidth(), (int)UiHeight(), (Color){0, 0, 0, 150});
    Rectangle plate = {400.0f, 190.0f, 480.0f, 340.0f};
    StoreDrawNineSlice(store->popup, plate, 34.0f, WHITE);
    StoreDrawCenteredFit(
        store->title_font,
        header,
        (Rectangle){plate.x + 28.0f, plate.y + 22.0f, plate.width - 56.0f, 44.0f},
        30.0f,
        (Color){242, 236, 218, 255}
    );
    CampaignDrawWrapped(
        store->body_font,
        body,
        (Vector2){plate.x + 40.0f, plate.y + 84.0f},
        19.0f,
        1.0f,
        6.0f,
        plate.width - 80.0f,
        (Color){228, 224, 212, 255}
    );
    bool confirm = store->dialog == STORE_DIALOG_CONFIRM_GEMS;
    float button_y = plate.y + plate.height - 82.0f;
    Rectangle ok_rect = confirm ? (Rectangle){plate.x + 238.0f, button_y, 214.0f, 59.0f}
                                : (Rectangle){plate.x + (plate.width - 236.0f) * 0.5f, button_y, 236.0f, 59.0f};
    if (confirm && ButtonDraw(&store->dialog_cancel, (Rectangle){plate.x + 28.0f, button_y, 214.0f, 59.0f})) {
        store->dialog = STORE_DIALOG_NONE;
        return false;
    }
    if (ButtonDraw(&store->dialog_ok, ok_rect)) {
        StoreDialog dialog = store->dialog;
        store->dialog = STORE_DIALOG_NONE;
        store->dialog_press_armed = false;
        if (dialog == STORE_DIALOG_CANT_AFFORD) StoreShowPage(store, true);
        return dialog == STORE_DIALOG_CONFIRM_GEMS && StoreTryPurchase(store, player, heroes, store->pending_pack);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) store->dialog_press_armed = true;
    if (!store->dialog_press_armed || CheckCollisionPointRec(UiMouse(), plate) ||
        !IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        return false;
    store->dialog = STORE_DIALOG_NONE;
    store->dialog_press_armed = false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    return false;
}

StoreAction StoreDraw(Store *store, Player *player, Heroes *heroes, float dt) {
    if (dt < 0.0f) dt = 0.0f;
    if (store->status_time > 0.0f) store->status_time = fmaxf(0.0f, store->status_time - dt);
    if (store->view == STORE_VIEW_HEROES || store->view == STORE_VIEW_ITEMS || store->view == STORE_VIEW_FEATURED) {
        store->bullet_time += dt;
    } else {
        store->dragging = false;
        store->drag_scrolled = false;
    }

    if (store->view == STORE_VIEW_OPENING || store->view == STORE_VIEW_RESULT) {
        StoreEnsureCurrentHero(store, heroes);
        PackHeroUpdate(&store->opening_hero, dt);
    }
    if (store->view == STORE_VIEW_OPENING) {
        float phase_dt = dt;
        if (store->reveal_hitch_guard_frames > 0) {
            store->reveal_hitch_guard_frames--;
            if (phase_dt > 0.20f) phase_dt = 0.05f;
        }
        float previous = store->phase_time;
        store->phase_time += phase_dt;
        if (!store->sequence_sound_played && previous < STORE_OPEN_SEQUENCE_START &&
            store->phase_time >= STORE_OPEN_SEQUENCE_START) {
            GuiSoundPlay(SND_PACK_REVEAL);
            store->sequence_sound_played = true;
        }
        if (store->phase_time >= STORE_OPEN_COMPLETE) StoreFinishCurrentResult(store, heroes, false);
    } else if (store->view == STORE_VIEW_RESULT) {
        store->result_time += dt;
    }

    StoreAction action = STORE_ACTION_NONE;
    switch (store->view) {
        case STORE_VIEW_LANDING:
            action = StoreDrawLanding(store);
            break;
        case STORE_VIEW_HEROES:
            action = StoreDrawPackPage(store, player, heroes, LocGet("BUTTON_STORE_HERO_TAB"));
            break;
        case STORE_VIEW_ITEMS:
            action = StoreDrawPackPage(store, player, heroes, STORE_ITEMS_TITLE);
            break;
        case STORE_VIEW_FEATURED:
            action = StoreDrawPackPage(store, player, heroes, STORE_FEATURED_TITLE);
            break;
        case STORE_VIEW_VAULT:
            action = StoreDrawVaultPage(store);
            break;
        case STORE_VIEW_GEMS:
            action = StoreDrawGemsPage(store);
            break;
        case STORE_VIEW_OPENING:
            action = StoreDrawOpening(store, heroes);
            break;
        case STORE_VIEW_RESULT:
            action = StoreDrawResult(store, player, heroes);
            break;
        case STORE_VIEW_ITEM_REVEAL:
            action = StoreDrawItemReveal(store, player, heroes, dt);
            break;
    }
    if (StoreDrawDialog(store, player, heroes)) action = STORE_ACTION_PURCHASED;
    return action;
}
