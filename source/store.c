
bool ProgressionSave(const Player *player);
static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
);

#define STORE_PACK_COUNT 4
#define STORE_MAX_RESULTS 10
#define STORE_GEM_SKU_COUNT 7
#define STORE_PACK_BULLET_MAX 4
#define STORE_RUNE_PACK_COUNT 5
#define STORE_RUNE_PACK_SLOTS 3
#define STORE_RUNE_SLOT_FAMILIES 4
#define STORE_MAX_PACK_RUNES (STORE_MAX_RESULTS + STORE_RUNE_PACK_SLOTS)
#define STORE_RUNE_REVEAL_STEP 0.5f

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
static const Rectangle STORE_CELL_BULLET = {0.06f, 0.724f, 0.88f, 0.06f};
static const Rectangle STORE_CELL_BUY = {0.0244f, 0.8219f, 0.953f, 0.1013f};
static const Rectangle STORE_CELL_TAP_AREA = {0.0f, 0.0f, 1.0f, 0.81f};
#define STORE_HEADER_TEXT_OF_CELL 0.044f
#define STORE_BULLET_TEXT_OF_CELL 0.04f
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
    STORE_VIEW_GEMS,
    STORE_VIEW_OPENING,
    STORE_VIEW_RESULT,
    STORE_VIEW_RUNES,
    STORE_VIEW_RUNE_REVEAL,
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
} StoreDialog;

typedef struct {
    char id[32];
    const char *name;
    const char *info;
    const char *bullets[STORE_PACK_BULLET_MAX];
    int bullet_count;
    bool gems;
    int price;
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
} StorePackDef;

typedef struct {
    const char *families[STORE_RUNE_SLOT_FAMILIES];
    int family_count;
    int min_rarity;
    int max_rarity;
} StoreRuneSlot;

typedef struct {
    const char *name;
    const char *info;
    const char *description;
    const char *icon;
    bool gems;
    int price;
    StoreRuneSlot slots[STORE_RUNE_PACK_SLOTS];
    int slot_count;
} StoreRunePackDef;

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
    StorePackDef packs[STORE_PACK_COUNT];
    StoreRunePackDef rune_packs[STORE_RUNE_PACK_COUNT];
    StoreGemSku gem_skus[STORE_GEM_SKU_COUNT];
    PackScene opening_scene;
    PackFx opening_fx;
    PackHero opening_hero;

    Texture2D headers[STORE_PACK_COUNT];
    Texture2D icons[STORE_PACK_COUNT];
    Texture2D card;
    Texture2D popup;
    Texture2D plate;
    Texture2D category_frame;
    Texture2D category_art[3];
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
    Button buy[STORE_PACK_COUNT];
    Button rune_buy[STORE_RUNE_PACK_COUNT];
    Button next;
    Button skip;
    Button done;
    Button buy_again;
    Button dialog_ok;
    Button nav_train, nav_runes, nav_squad;
    bool nav_consume_clicked;
    Camera3D opening_camera;

    StoreView view;
    int selected_pack;
    int info_pack;
    float scroll;
    bool dragging;
    bool drag_scrolled;
    float drag_start_x;
    float scroll_start;
    float bullet_time;
    StoreDialog dialog;
    bool dialog_press_armed;
    int rune_info_pack;
    int rune_selected_pack;
    int rune_results[STORE_MAX_PACK_RUNES];
    int rune_result_count;
    int rune_details;
    float rune_reveal_time;
    int pack_rune_def;
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

static const char *const STORE_EXPECTED_PACK_IDS[STORE_PACK_COUNT] = {
    "recruit", "soldier", "champion", "grand_champion"
};

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

static bool StorePackAffordable(const StorePackDef *pack, const Player *player) {
    return pack->gems ? player->gems >= pack->price : player->gold >= pack->price;
}

static const char *StorePackLocText(const cJSON *json, const char *suffix, const char *fallback_key) {
    char key[96];
    snprintf(key, sizeof(key), "%s%s", JsonStr(json, "loc_id"), suffix);
    const char *text = LocGet(key);
    if (!text[0] && fallback_key) text = JsonStr(json, fallback_key);
    return text;
}

static void StoreLoadPack(StorePackDef *pack, const cJSON *json, int index) {
    StrCopy(pack->id, sizeof(pack->id), JsonStr(json, "id"));
    pack->name = StorePackLocText(json, "_PACK_NAME", "name_ru");
    pack->info = StorePackLocText(json, "_PACK_INFO", "info_ru");
    pack->gems = !strcmp(JsonStr(json, "currency"), "gems");
    pack->price = JsonInt(json, "price");
    pack->hero_count = JsonInt(json, "hero_count");
    pack->minimum_rarity = RarityFromString(JsonStr(json, "minimum_rarity"));
    pack->guaranteed_legendary = JsonInt(json, "guaranteed_legendary");
    pack->guaranteed_epic = JsonInt(json, "guaranteed_epic");
    pack->legendary_pity = JsonInt(json, "legendary_pity");
    pack->rune_chance_pct = JsonInt(json, "rune_chance_pct");
    pack->rune_count = JsonInt(json, "rune_count");
    pack->rune_min_rarity = JsonInt(json, "rune_min_rarity");
    pack->rune_max_rarity = JsonInt(json, "rune_max_rarity");

    pack->bullet_count = 0;
    const char *description = StorePackLocText(json, "_PACK_DESCRIPTION", NULL);
    if (description[0]) pack->bullets[pack->bullet_count++] = description;
    const cJSON *bullets = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "bullets_ru");
    cJSON *bullet = NULL;
    cJSON_ArrayForEach(bullet, (cJSON *)bullets) {
        if (cJSON_IsString(bullet) && pack->bullet_count < STORE_PACK_BULLET_MAX)
            pack->bullets[pack->bullet_count++] = bullet->valuestring;
    }

    const cJSON *weights = cJSON_GetObjectItemCaseSensitive((cJSON *)json, "weights");
    static const char *const keys[] = {"", "Common", "Uncommon", "Rare", "Epic", "Legendary"};
    int total = 0;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++) {
        pack->weights[rarity] = JsonInt(weights, keys[rarity]);
        total += pack->weights[rarity];
        if (rarity < pack->minimum_rarity && pack->weights[rarity] != 0)
            TraceLog(LOG_FATAL, "Store: %s assigns weight below its minimum rarity", pack->id);
    }
    if (strcmp(pack->id, STORE_EXPECTED_PACK_IDS[index]) || !pack->name[0] || !pack->info[0] ||
        pack->bullet_count == 0 || pack->price <= 0 || pack->hero_count <= 0 || pack->hero_count > STORE_MAX_RESULTS ||
        pack->guaranteed_legendary < 0 || pack->guaranteed_epic < 0 ||
        pack->guaranteed_legendary + pack->guaranteed_epic > pack->hero_count || total != 10000)
        TraceLog(LOG_FATAL, "Store: invalid pack definition at index %d", index);
}

static void StoreLoadRunePack(StoreRunePackDef *pack, const cJSON *json) {
    pack->name = StorePackLocText(json, "_PACK_NAME", "name_ru");
    pack->info = StorePackLocText(json, "_PACK_INFO", "info_ru");
    pack->description = StorePackLocText(json, "_PACK_DESCRIPTION", "description_ru");
    pack->icon = JsonStr(json, "icon");
    pack->gems = !strcmp(JsonStr(json, "currency"), "gems");
    pack->price = JsonInt(json, "price");
    const cJSON *slot = NULL;
    cJSON_ArrayForEach(slot, cJSON_GetObjectItemCaseSensitive((cJSON *)json, "slots")) {
        if (pack->slot_count >= STORE_RUNE_PACK_SLOTS)
            TraceLog(LOG_FATAL, "Store: rune pack %s has too many slots", pack->name);
        StoreRuneSlot *rune_slot = &pack->slots[pack->slot_count++];
        rune_slot->min_rarity = JsonInt(slot, "min_rarity");
        rune_slot->max_rarity = JsonInt(slot, "max_rarity");
        const cJSON *family = NULL;
        cJSON_ArrayForEach(family, cJSON_GetObjectItemCaseSensitive((cJSON *)slot, "families")) {
            if (rune_slot->family_count < STORE_RUNE_SLOT_FAMILIES)
                rune_slot->families[rune_slot->family_count++] = family->valuestring;
        }
    }
    if (!pack->name[0] || !pack->info[0] || !pack->description[0] || pack->price <= 0 || pack->slot_count == 0)
        TraceLog(LOG_FATAL, "Store: invalid rune pack definition %s", JsonStr(json, "id"));
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
    if (JsonInt(store->config_root, "version") != 2) TraceLog(LOG_FATAL, "Store: unsupported config version");
    StrCopy(store->balance_note_ru, sizeof(store->balance_note_ru), JsonStr(store->config_root, "balance_note_ru"));
    const cJSON *packs = cJSON_GetObjectItemCaseSensitive(store->config_root, "packs");
    if (!cJSON_IsArray(packs) || cJSON_GetArraySize(packs) != STORE_PACK_COUNT)
        TraceLog(LOG_FATAL, "Store: expected %d packs in %s", STORE_PACK_COUNT, config_path);
    for (int i = 0; i < STORE_PACK_COUNT; i++) StoreLoadPack(&store->packs[i], cJSON_GetArrayItem(packs, i), i);
    const cJSON *rune_packs = cJSON_GetObjectItemCaseSensitive(store->config_root, "rune_packs");
    if (!cJSON_IsArray(rune_packs) || cJSON_GetArraySize(rune_packs) != STORE_RUNE_PACK_COUNT)
        TraceLog(LOG_FATAL, "Store: expected %d rune packs in %s", STORE_RUNE_PACK_COUNT, config_path);
    for (int i = 0; i < STORE_RUNE_PACK_COUNT; i++)
        StoreLoadRunePack(&store->rune_packs[i], cJSON_GetArrayItem(rune_packs, i));
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

    store->headers[0] = GuiTex(GUI_STORE_HEADER_RECRUIT);
    store->headers[1] = GuiTex(GUI_STORE_HEADER_SOLDIER);
    store->headers[2] = GuiTex(GUI_STORE_HEADER_CHAMPION);
    store->headers[3] = GuiTex(GUI_STORE_HEADER_GRAND);
    store->icons[0] = GuiTex(GUI_STORE_ICON_RECRUIT);
    store->icons[1] = GuiTex(GUI_STORE_ICON_SOLDIER);
    store->icons[2] = GuiTex(GUI_STORE_ICON_CHAMPION);
    store->icons[3] = GuiTex(GUI_STORE_ICON_GRAND);
    store->card = GuiTex(GUI_STORE_CARD);
    store->popup = GuiTex(GUI_STORE_POPUP);
    store->plate = GuiTex(GUI_STORE_PLATE);
    store->category_frame = GuiTex(GUI_STORE_CATEGORY_FRAME);
    store->category_art[0] = GuiTex(GUI_STORE_CATEGORY_UNITS);
    store->category_art[1] = GuiTex(GUI_STORE_CATEGORY_GEMS);
    store->category_art[2] = GuiTex(GUI_STORE_CATEGORY_RUNES);
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
    for (int i = 0; i < STORE_PACK_COUNT; i++) {
        store->buy[i] = ButtonLoad(GUI_STORE_BUY, GUI_STORE_BUY_ON);
        ButtonDisableClickSound(&store->buy[i]);
    }
    for (int i = 0; i < STORE_RUNE_PACK_COUNT; i++) {
        store->rune_buy[i] = ButtonLoad(GUI_STORE_BUY, GUI_STORE_BUY_ON);
        ButtonDisableClickSound(&store->rune_buy[i]);
    }
    store->rune_info_pack = -1;
    store->rune_details = -1;
    store->pack_rune_def = -1;
    store->next =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ДАЛЕЕ", store->title_font, (Vector2){0.5f, 0.5f}, 0.34f);
    store->skip =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ПРОПУСТИТЬ", store->title_font, (Vector2){0.5f, 0.5f}, 0.29f);
    store->done =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "ГОТОВО", store->title_font, (Vector2){0.5f, 0.5f}, 0.34f);
    store->buy_again =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "КУПИТЬ ЕЩЁ", store->title_font, (Vector2){0.5f, 0.5f}, 0.27f);
    store->nav_train = ButtonLoad(GUI_BTN_PACK_TRAIN, GUI_BTN_PACK_TRAIN_ON);
    store->nav_runes = ButtonLoad(GUI_BTN_PACK_RUNES, GUI_BTN_PACK_RUNES_ON);
    store->nav_squad = ButtonLoad(GUI_BTN_PACK_SQUAD, GUI_BTN_PACK_SQUAD_ON);
    store->dialog_ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), store->title_font, (Vector2){0.5f, 0.5f}, 0.34f
    );
    store->view = STORE_VIEW_LANDING;
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

void StoreShowPage(Store *store, bool gems_page) {
    StoreView target = gems_page ? STORE_VIEW_GEMS : STORE_VIEW_HEROES;
    if (store->view == target) return;
    if (store->view == STORE_VIEW_OPENING || store->view == STORE_VIEW_RESULT) PackHeroClear(&store->opening_hero);
    store->view = target;
    store->scroll = 0.0f;
    store->info_pack = -1;
    store->dialog = STORE_DIALOG_NONE;
    store->status_time = 0.0f;
}

void StoreShowRunes(Store *store) {
    store->view = STORE_VIEW_RUNES;
    store->rune_info_pack = -1;
    store->rune_details = -1;
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
    return store->view == STORE_VIEW_LANDING || store->view == STORE_VIEW_HEROES || store->view == STORE_VIEW_GEMS ||
           store->view == STORE_VIEW_RUNES || store->view == STORE_VIEW_RUNE_REVEAL;
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
        case STORE_VIEW_GEMS:
        case STORE_VIEW_RUNES:
            store->view = STORE_VIEW_LANDING;
            store->info_pack = -1;
            GuiSoundPlay(SND_UI_POPUP_CLOSE);
            return STORE_ACTION_NONE;
        case STORE_VIEW_RUNE_REVEAL:
            if (store->rune_details >= 0)
                store->rune_details = -1;
            else
                store->view = STORE_VIEW_RUNES;
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
                store->view = STORE_VIEW_HEROES;
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

static void StoreOpenDialog(Store *store, StoreDialog dialog) {
    store->dialog = dialog;
    store->dialog_press_armed = false;
    GuiSoundPlay(SND_STORE_ERROR);
}

static bool StoreTryBuyRunePack(Store *store, Player *player, int pack_index) {
    const StoreRunePackDef *pack = &store->rune_packs[pack_index];
    if (RUNE_INVENTORY_CAPACITY - player->runes.count < pack->slot_count) {
        StoreOpenDialog(store, STORE_DIALOG_ROSTER_FULL);
        return false;
    }
    if (player->gems < pack->price) {
        StoreOpenDialog(store, STORE_DIALOG_CANT_AFFORD);
        return false;
    }
    static PlayerRunes old_runes;
    old_runes = player->runes;
    uint32_t rng = player->store_rng_state;
    int old_gems = player->gems;
    uint32_t old_rng = rng;
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
    PlayerSpendGems(player, pack->price);
    player->store_rng_state = rng;
    if (!ProgressionSave(player)) {
        player->runes = old_runes;
        player->gems = old_gems;
        player->store_rng_state = old_rng;
        StoreSetStatus(store, "Не удалось сохранить покупку");
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }
    store->rune_selected_pack = pack_index;
    store->rune_result_count = pack->slot_count;
    memcpy(store->rune_results, defs, (size_t)pack->slot_count * sizeof(int));
    store->rune_reveal_time = 0.0f;
    store->rune_details = -1;
    store->view = STORE_VIEW_RUNE_REVEAL;
    GuiSoundPlay(SND_PACK_RARE);
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

static bool StoreTryPurchase(Store *store, Player *player, Heroes *heroes, int pack_index) {
    if (pack_index < 0 || pack_index >= STORE_PACK_COUNT) return false;
    const StorePackDef *pack = &store->packs[pack_index];
    if (!PlayerRosterHasCapacity(player, pack->hero_count)) {
        store->dialog = STORE_DIALOG_ROSTER_FULL;
        store->dialog_press_armed = false;
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }
    if (!StorePackAffordable(pack, player)) {
        store->dialog = STORE_DIALOG_CANT_AFFORD;
        store->dialog_press_armed = false;
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }
    if (player->store_opened_total == INT_MAX || player->store_opened_by_pack[pack_index] == INT_MAX) {
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
    int old_opened_pack = player->store_opened_by_pack[pack_index];
    int old_champion_since = player->champion_since_legendary;
    const cJSON *definitions[STORE_MAX_RESULTS] = {0};
    Rarity rarities[STORE_MAX_RESULTS] = {0};
    int next_pity = player->champion_since_legendary;
    for (int i = 0; i < pack->hero_count; i++) {
        bool guaranteed_legendary = i < pack->guaranteed_legendary;
        bool guaranteed_epic =
            i >= pack->guaranteed_legendary && i < pack->guaranteed_legendary + pack->guaranteed_epic;
        bool pity =
            pack_index == STORE_PACK_CHAMPION && pack->legendary_pity > 0 && next_pity >= pack->legendary_pity - 1;
        rarities[i] = (guaranteed_legendary || pity) ? RARITY_LEGENDARY
                                                     : (guaranteed_epic ? RARITY_EPIC : StoreRollRarity(pack, &rng));
        definitions[i] = StoreRandomDefinition(store, rarities[i], &rng);
        if (!definitions[i]) {
            StoreSetStatus(store, "Этот пак временно недоступен");
            GuiSoundPlay(SND_STORE_ERROR);
            return false;
        }
        if (pack_index == STORE_PACK_CHAMPION) next_pity = rarities[i] == RARITY_LEGENDARY ? 0 : next_pity + 1;
    }
    if (pack->hero_count > 1) StoreSortResults(definitions, rarities, pack->hero_count);

    int first_index = -1;
    if (!PlayerRosterAppendDefinitions(player, definitions, pack->hero_count, &first_index)) {
        StoreSetStatus(store, "Не удалось добавить героев в ростер");
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }
    bool spent = pack->gems ? PlayerSpendGems(player, pack->price) : PlayerSpendGold(player, pack->price);
    if (!spent) TraceLog(LOG_FATAL, "Store: validated currency spend failed");
    static PlayerRunes old_runes;
    old_runes = player->runes;
    int pack_rune = StoreRollPackRunes(pack, &rng, &player->runes, RuneNow());

    player->store_rng_state = rng;
    player->store_opened_total++;
    player->missions.counters[MISSION_STAT_SUMMONED] += pack->hero_count;
    player->store_opened_by_pack[pack_index]++;
    if (pack_index == STORE_PACK_CHAMPION) player->champion_since_legendary = next_pity;

    if (!ProgressionSave(player)) {
        PlayerRosterClearRange(player, old_roster_count, player->roster_count);
        player->runes = old_runes;
        player->roster_count = old_roster_count;
        player->next_instance_id = old_next_instance_id;
        player->gold = old_gold;
        player->gems = old_gems;
        player->store_rng_state = old_rng;
        player->store_opened_total = old_opened_total;
        player->store_opened_by_pack[pack_index] = old_opened_pack;
        player->champion_since_legendary = old_champion_since;
        StoreSetStatus(store, "Не удалось сохранить покупку");
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }

    if (!HeroesSyncRoster(heroes, player)) TraceLog(LOG_FATAL, "Store: persisted roster cannot sync to Heroes");
    store->selected_pack = pack_index;
    store->pack_rune_def = pack_rune;
    store->result_count = pack->hero_count;
    for (int i = 0; i < pack->hero_count; i++) store->result_roster_indices[i] = first_index + i;
    store->reveal_index = 0;
    store->loaded_reveal_index = -1;
    store->status_time = 0.0f;
    StoreBeginCurrentResult(store);
    return true;
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

static bool StoreDrawCategoryTile(Store *store, Rectangle rect, Texture2D art, const char *label) {
    bool hover = !StoreInputBlocked(store) && CheckCollisionPointRec(UiMouse(), rect);
    Rectangle inner = {rect.x + 12.0f, rect.y + 18.0f, rect.width - 24.0f, rect.height - 78.0f};
    StoreDrawTextureCover(art, inner, WHITE);
    UiDrawTexture(store->category_frame, rect, hover ? (Color){255, 255, 236, 255} : WHITE);
    StoreDrawCenteredFit(
        store->title_font,
        label,
        (Rectangle){
            rect.x + 0.105f * rect.width, rect.y + 0.87f * rect.height, 0.79f * rect.width, 0.09f * rect.height
        },
        28.0f,
        (Color){244, 241, 225, 255}
    );
    return StoreReleasedOn(store, rect);
}

static StoreAction StoreDrawLanding(Store *store) {
    int chrome = StoreDrawPopupChrome(store, LocGet("STORE_TITLE"), false);
    if (chrome == 1) return STORE_ACTION_CLOSE;

    static const char *const label_keys[3] = {"BUTTON_STORE_HERO_TAB", "BUTTON_STORE_GEMS_TAB", "RUNES_BUTTON_LABEL"};
    Rectangle list = StoreListRect();
    float tile_w = list.width / 3.0f;
    for (int i = 0; i < 3; i++) {
        Rectangle tile = {list.x + i * tile_w, list.y, tile_w, list.height};
        if (!StoreDrawCategoryTile(store, tile, store->category_art[i], LocGet(label_keys[i]))) continue;
        if (i == 2) {
            StoreShowRunes(store);
            GuiSoundPlay(SND_UI_TAB);
        } else {
            StoreShowPage(store, i == 1);
            GuiSoundPlay(SND_UI_TAB);
        }
    }
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawBuyPrice(Store *store, int cost, bool gems, Rectangle bar) {
    char price[16];
    snprintf(price, sizeof(price), "%d", cost);
    UiDrawTextRight(
        store->title_font,
        price,
        (Rectangle){bar.x, bar.y, bar.width * STORE_PRICE_RIGHT_OF_BAR, bar.height},
        bar.height * STORE_PRICE_TEXT_OF_BAR,
        (Color){235, 232, 220, 255}
    );
    Texture2D icon = gems ? store->currency_gems : store->currency_gold;
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

static void StoreDrawPackBullets(Store *store, const StorePackDef *pack, Rectangle rect, float size) {
    int index = 0;
    float alpha = 1.0f;
    if (pack->bullet_count > 1) {
        float cycle = 5.0f;
        int step = (int)(store->bullet_time / cycle);
        float phase = store->bullet_time - step * cycle;
        index = step % pack->bullet_count;
        if (phase >= 4.0f) {
            index = (index + 1) % pack->bullet_count;
            alpha = phase - 4.0f;
        } else if (phase >= 3.0f) {
            alpha = 1.0f - (phase - 3.0f);
        }
    }
    Color color = {239, 234, 218, (unsigned char)(alpha * 255.0f)};
    StoreDrawCenteredFit(store->title_font, pack->bullets[index], rect, size, color);
}

static bool StoreDrawPackCard(Store *store, Player *player, Heroes *heroes, int pack_index, Rectangle cell) {
    const StorePackDef *pack = &store->packs[pack_index];
    bool show_info = store->info_pack == pack_index;
    Vector2 mouse = UiMouse();
    bool in_viewport = CheckCollisionPointRec(mouse, StoreListRect()) &&
                       !CheckCollisionPointRec(mouse, StoreArrowRect(true)) &&
                       !CheckCollisionPointRec(mouse, StoreArrowRect(false));
    Rectangle art = StoreCellPart(cell, STORE_CELL_ICON);
    if (show_info)
        DrawRectangleRec(art, BLACK);
    else
        StoreDrawTextureCover(store->icons[pack_index], art, WHITE);
    UiDrawTexture(store->card, StoreCellPart(cell, STORE_CELL_CARD), WHITE);

    Rectangle top = StoreCellPart(cell, STORE_CELL_HEADER);
    UiDrawTexture(store->headers[pack_index], top, WHITE);
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

    Rectangle info = StoreCellPart(cell, STORE_CELL_INFO);
    bool info_hover = !StoreInputBlocked(store) && CheckCollisionPointRec(mouse, info);
    UiDrawTexture(info_hover || show_info ? store->info_icon_on : store->info_icon, info, WHITE);

    StoreDrawPackBullets(store, pack, StoreCellPart(cell, STORE_CELL_BULLET), cell.height * STORE_BULLET_TEXT_OF_CELL);

    Rectangle buy = StoreCellPart(cell, STORE_CELL_BUY);
    bool clicked = ButtonDrawEx(&store->buy[pack_index], buy, !StoreInputBlocked(store) && in_viewport);
    StoreDrawBuyPrice(store, pack->price, pack->gems, buy);

    if (in_viewport && StoreReleasedOn(store, info)) {
        store->info_pack = show_info ? -1 : pack_index;
        GuiSoundPlay(SND_STORE_INFO);
    } else if (in_viewport && StoreReleasedOn(store, StoreCellPart(cell, STORE_CELL_TAP_AREA))) {
        store->info_pack = show_info ? -1 : pack_index;
        GuiSoundPlay(SND_STORE_INFO);
    }
    return clicked && StoreTryPurchase(store, player, heroes, pack_index);
}

static void StoreDrawRunePackCard(Store *store, Player *player, int pack_index, Rectangle cell) {
    const StoreRunePackDef *pack = &store->rune_packs[pack_index];
    bool show_info = store->rune_info_pack == pack_index;
    Rectangle art = StoreCellPart(cell, STORE_CELL_ICON);
    DrawRectangleRec(art, BLACK);
    Texture2D icon = RuneIconLoad(pack->icon);
    if (!show_info && icon.id != 0) StoreDrawTextureCover(icon, art, WHITE);
    UiDrawTexture(store->card, StoreCellPart(cell, STORE_CELL_CARD), WHITE);

    Rectangle top = StoreCellPart(cell, STORE_CELL_HEADER);
    UiDrawTexture(store->headers[STORE_PACK_CHAMPION], top, WHITE);
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
            (Vector2){art.x + 6.0f, art.y + 8.0f},
            14.0f,
            1.0f,
            3.0f,
            art.width - 12.0f,
            (Color){236, 232, 220, 255}
        );
    StoreDrawCenteredFit(
        store->body_font,
        pack->description,
        StoreCellPart(cell, STORE_CELL_BULLET),
        cell.height * STORE_BULLET_TEXT_OF_CELL,
        (Color){239, 234, 218, 255}
    );

    Rectangle buy = StoreCellPart(cell, STORE_CELL_BUY);
    bool clicked = ButtonDrawEx(&store->rune_buy[pack_index], buy, !StoreInputBlocked(store));
    StoreDrawBuyPrice(store, pack->price, pack->gems, buy);
    if (StoreReleasedOn(store, StoreCellPart(cell, STORE_CELL_TAP_AREA))) {
        store->rune_info_pack = show_info ? -1 : pack_index;
        GuiSoundPlay(SND_STORE_INFO);
    }
    if (clicked) StoreTryBuyRunePack(store, player, pack_index);
}

static StoreAction StoreDrawRunesPage(Store *store, Player *player) {
    int chrome = StoreDrawPopupChrome(store, LocGet("RUNES_BUTTON_LABEL"), true);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    if (chrome == 2) {
        store->view = STORE_VIEW_LANDING;
        return STORE_ACTION_NONE;
    }
    Rectangle list = StoreListRect();
    float cell_w = list.width / STORE_RUNE_PACK_COUNT;
    float cell_h = cell_w / STORE_CELL_ASPECT;
    for (int i = 0; i < STORE_RUNE_PACK_COUNT; i++) {
        Rectangle cell = {list.x + i * cell_w, list.y + (list.height - cell_h) * 0.5f, cell_w, cell_h};
        StoreDrawRunePackCard(store, player, i, cell);
        if (store->view != STORE_VIEW_RUNES) return STORE_ACTION_NONE;
    }
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static StoreAction StoreDrawRuneReveal(Store *store, Player *player, float dt) {
    int chrome = StoreDrawPopupChrome(store, LocGet("RUNE_REVEAL_WINDOW_TITLE"), false);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    store->rune_reveal_time += dt;
    Rectangle list = StoreListRect();
    float cell = 150.0f, gap = 20.0f;
    float total = store->rune_result_count * cell + (store->rune_result_count - 1) * gap;
    for (int i = 0; i < store->rune_result_count; i++) {
        if (store->rune_reveal_time < i * STORE_RUNE_REVEAL_STEP) continue;
        Rectangle rect = {
            list.x + (list.width - total) * 0.5f + i * (cell + gap), list.y + (list.height - cell) * 0.4f, cell, cell
        };
        const RuneDef *def = RuneDefAt(store->rune_results[i]);
        RunesDrawTile(def, rect, false);
        StoreDrawCenteredFit(
            store->body_font,
            def->name_ru,
            (Rectangle){rect.x - 10.0f, rect.y + cell + 4.0f, cell + 20.0f, 24.0f},
            17.0f,
            RuneRarityColor(def->rarity)
        );
        if (store->rune_details < 0 && StoreReleasedOn(store, rect)) {
            store->rune_details = i;
            GuiSoundPlay(SND_STORE_INFO);
            return STORE_ACTION_NONE;
        }
    }
    bool interactive = !StoreInputBlocked(store) && store->rune_details < 0;
    Rectangle popup = StorePlateRect();
    float y = popup.y + popup.height - 90.0f, cx = popup.x + popup.width * 0.5f;
    bool again = ButtonDrawEx(&store->buy_again, (Rectangle){cx - 242.0f, y, 236.0f, 59.0f}, interactive);
    bool done = ButtonDrawEx(&store->done, (Rectangle){cx + 6.0f, y, 236.0f, 59.0f}, interactive);
    if (store->rune_details >= 0) {
        float height = 480.0f;
        const RuneDef *def = RuneDefAt(store->rune_results[store->rune_details]);
        RunesDrawCard(def, NULL, (Rectangle){cx - height * 0.39f, 150.0f, height * 0.78f, height}, 0);
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) store->rune_details = -1;
    }
    if (done) store->view = STORE_VIEW_RUNES;
    if (again) StoreTryBuyRunePack(store, player, store->rune_selected_pack);
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawScrollArrow(Store *store, Rectangle rect, bool left, bool enabled) {
    Rectangle src = {0.0f, 0.0f, (float)store->scroll_arrow.width, (float)store->scroll_arrow.height};
    if (left) {
        src.x = (float)store->scroll_arrow.width;
        src.width = -(float)store->scroll_arrow.width;
    }
    Color tint = enabled ? WHITE : (Color){105, 105, 105, 115};
    if (enabled && !StoreInputBlocked(store) && CheckCollisionPointRec(UiMouse(), rect))
        tint = (Color){255, 224, 133, 255};
    DrawTexturePro(store->scroll_arrow, src, rect, (Vector2){0}, 0.0f, tint);
}

static float StoreMaxScroll(void) {
    float content = STORE_PACK_COUNT * StoreCellStride();
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
    store->scroll = Clamp(store->scroll, 0.0f, StoreMaxScroll());
}

static StoreAction StoreDrawHeroesPage(Store *store, Player *player, Heroes *heroes) {
    int chrome = StoreDrawPopupChrome(store, LocGet("BUTTON_STORE_HERO_TAB"), true);
    if (chrome == 1) return STORE_ACTION_CLOSE;
    if (chrome == 2) {
        store->view = STORE_VIEW_LANDING;
        store->info_pack = -1;
        return STORE_ACTION_NONE;
    }

    StoreUpdateScrollInput(store);

    bool can_left = store->scroll > 0.0f;
    bool can_right = store->scroll < StoreMaxScroll();
    Rectangle list = StoreListRect();
    float scroll_step = list.width * STORE_SCROLL_STEP_OF_LIST;
    Rectangle arrow_left = StoreArrowRect(true);
    Rectangle arrow_right = StoreArrowRect(false);
    StoreDrawScrollArrow(store, arrow_left, true, can_left);
    StoreDrawScrollArrow(store, arrow_right, false, can_right);
    if (can_left && StoreReleasedOn(store, arrow_left)) {
        store->scroll -= scroll_step;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (can_right && StoreReleasedOn(store, arrow_right)) {
        store->scroll += scroll_step;
        GuiSoundPlay(SND_UI_TAB);
    }
    store->scroll = Clamp(store->scroll, 0.0f, StoreMaxScroll());

    StoreAction action = STORE_ACTION_NONE;
    float scale = UiScale();
    BeginScissorMode(
        (int)(list.x * scale), (int)(list.y * scale), (int)(list.width * scale), (int)(list.height * scale)
    );
    for (int pack_index = 0; pack_index < STORE_PACK_COUNT; pack_index++) {
        float stride = StoreCellStride();
        float cell_x = list.x + pack_index * stride - store->scroll;
        if (cell_x + stride < list.x || cell_x > list.x + list.width) continue;
        Rectangle card = {cell_x, list.y, stride, list.height};
        if (StoreDrawPackCard(store, player, heroes, pack_index, card)) {
            action = STORE_ACTION_PURCHASED;
            break;
        }
    }
    EndScissorMode();
    StoreDrawToast(store);
    return action;
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

static void StoreDrawPackRune(const Store *store) {
    const RuneDef *rune = RuneDefAt(store->pack_rune_def);
    float time = store->result_time;
    Rectangle slot = STORE_PACK_RUNE_RECT;

    float burst_time = time - STORE_PACK_RUNE_BURST_DELAY;
    if (burst_time >= 0.0f && burst_time < STORE_PACK_RUNE_BURST_DURATION) {
        float fade = StoreClamp01(burst_time / STORE_PACK_RUNE_BURST_DURATION);
        float peak =
            STORE_PACK_RUNE_BURST_PEAK_BASE * (1.0f + STORE_PACK_RUNE_BURST_PEAK_PER_RARITY * (float)rune->rarity);
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
        RuneIconLoad(rune->icon),
        StoreScaledAround(slot, STORE_PACK_RUNE_ICON_SCALE),
        Fade(WHITE, StoreClamp01(time / STORE_PACK_RUNE_FADE_IN))
    );
}

static StoreAction StoreDrawResult(Store *store, Player *player, Heroes *heroes) {
    HeroCard *card = StoreCurrentCard(store, heroes);
    bool last = store->reveal_index + 1 >= store->result_count;
    if (card && store->result_time >= 0.08f) HeroesDrawDetailsAt(card, (Rectangle){17.0f, 91.0f, 433.0f, 555.0f});
    if (store->pack_rune_def >= 0 && store->reveal_index == 0) StoreDrawPackRune(store);
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
        store->view = STORE_VIEW_HEROES;
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
    if (again && StoreTryPurchase(store, player, heroes, store->selected_pack)) return STORE_ACTION_PURCHASED;
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawDialog(Store *store) {
    if (store->dialog == STORE_DIALOG_NONE) return;
    const char *header =
        LocGet(store->dialog == STORE_DIALOG_CANT_AFFORD ? "MESSAGE_CANT_AFFORD_HEADER" : "MESSAGE_TOO_MANY_HEADER");
    const char *body = LocGet(
        store->dialog == STORE_DIALOG_CANT_AFFORD ? "MESSAGE_CANT_AFFORD_BODY" : "MESSAGE_TOO_MANY_BODY_NEW_CONSUME"
    );

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
    if (ButtonDraw(
            &store->dialog_ok,
            (Rectangle){plate.x + (plate.width - 236.0f) * 0.5f, plate.y + plate.height - 82.0f, 236.0f, 59.0f}
        )) {
        bool redirect = store->dialog == STORE_DIALOG_CANT_AFFORD;
        store->dialog = STORE_DIALOG_NONE;
        store->dialog_press_armed = false;
        if (redirect) StoreShowPage(store, true);
        return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) store->dialog_press_armed = true;
    if (!store->dialog_press_armed || CheckCollisionPointRec(UiMouse(), plate) ||
        !IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        return;
    store->dialog = STORE_DIALOG_NONE;
    store->dialog_press_armed = false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
}

StoreAction StoreDraw(Store *store, Player *player, Heroes *heroes, float dt) {
    if (dt < 0.0f) dt = 0.0f;
    if (store->status_time > 0.0f) store->status_time = fmaxf(0.0f, store->status_time - dt);
    if (store->view == STORE_VIEW_HEROES) {
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
            action = StoreDrawHeroesPage(store, player, heroes);
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
        case STORE_VIEW_RUNES:
            action = StoreDrawRunesPage(store, player);
            break;
        case STORE_VIEW_RUNE_REVEAL:
            action = StoreDrawRuneReveal(store, player, dt);
            break;
    }
    StoreDrawDialog(store);
    return action;
}
