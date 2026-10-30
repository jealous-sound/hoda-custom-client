
bool ProgressionSave(const Player *player);
static void CampaignDrawWrapped(
    Font font, const char *text, Vector2 pos, float size, float spacing, float line_gap, float max_width, Color tint
);

#define STORE_PACK_COUNT 4
#define STORE_MAX_RESULTS 10
#define STORE_GEM_SKU_COUNT 7
#define STORE_PACK_BULLET_MAX 4

#define STORE_LIST_X 224.0f
#define STORE_LIST_Y 182.0f
#define STORE_LIST_W 836.4f
#define STORE_LIST_H 474.4f
#define STORE_CELL_STRIDE (STORE_LIST_H * 0.6f)
#define STORE_SCROLL_STEP 639.5f

static const Rectangle STORE_ARROW_LEFT = {160.0f, 347.0f, 91.0f, 98.0f};
static const Rectangle STORE_ARROW_RIGHT = {1030.0f, 347.0f, 91.0f, 98.0f};

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
} StoreView;

typedef enum {
    STORE_ACTION_NONE,
    STORE_ACTION_CLOSE,
    STORE_ACTION_PURCHASED,
    STORE_ACTION_GOTO_MAP,
    STORE_ACTION_GOTO_SQUAD,
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
    StorePackDef packs[STORE_PACK_COUNT];
    StoreGemSku gem_skus[STORE_GEM_SKU_COUNT];
    PackScene opening_scene;
    PackFx opening_fx;
    PackHero opening_hero;

    Texture2D headers[STORE_PACK_COUNT];
    Texture2D icons[STORE_PACK_COUNT];
    Texture2D card;
    Texture2D popup;
    Texture2D title_bg;
    Texture2D overlay;
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
    Button next;
    Button skip;
    Button done;
    Button buy_again;
    Button dialog_ok;
    Button nav_train, nav_runes, nav_squad;

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
    store->title_bg = GuiTex(GUI_STORE_TITLE);
    store->overlay = GuiTex(GUI_STORE_OVERLAY);
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
    return store->view == STORE_VIEW_LANDING || store->view == STORE_VIEW_HEROES || store->view == STORE_VIEW_GEMS;
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
            store->view = STORE_VIEW_LANDING;
            store->info_pack = -1;
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

    player->store_rng_state = rng;
    player->store_opened_total++;
    player->store_opened_by_pack[pack_index]++;
    if (pack_index == STORE_PACK_CHAMPION) player->champion_since_legendary = next_pity;

    if (!ProgressionSave(player)) {
        PlayerRosterClearRange(player, old_roster_count, player->roster_count);
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

static int StoreDrawPopupChrome(Store *store, const char *title, bool show_back) {
    DrawRectangle(0, 72, (int)UiWidth(), (int)UiHeight() - 72, (Color){0, 0, 0, 132});
    Rectangle popup = {160.0f, 72.0f, 961.0f, 655.0f};
    StoreDrawNineSlice(store->overlay, popup, 25.0f, (Color){255, 255, 255, 248});
    DrawRectangleLinesEx(
        (Rectangle){popup.x + 6, popup.y + 6, popup.width - 12, popup.height - 12}, 2.0f, (Color){88, 91, 88, 255}
    );

    Rectangle title_rect = {469.0f, 84.0f, 356.0f, 65.0f};
    UiDrawTexture(store->title_bg, title_rect, WHITE);
    StoreDrawCenteredFit(store->title_font, title, title_rect, 36.0f, (Color){242, 240, 228, 255});

    int action = 0;
    bool interactive = !StoreInputBlocked(store);
    if (show_back && ButtonDrawEx(&store->back, (Rectangle){178.0f, 116.0f, 65.0f, 65.0f}, interactive)) action = 2;
    if (ButtonDrawEx(&store->close, (Rectangle){1034.0f, 110.0f, 74.0f, 74.0f}, interactive)) action = 1;
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
    float tile_w = STORE_LIST_W / 3.0f;
    for (int i = 0; i < 3; i++) {
        Rectangle tile = {STORE_LIST_X + i * tile_w, STORE_LIST_Y, tile_w, STORE_LIST_H};
        if (!StoreDrawCategoryTile(store, tile, store->category_art[i], LocGet(label_keys[i]))) continue;
        if (i == 2) {
            StoreSetStatus(store, "Руны временно недоступны");
            GuiSoundPlay(SND_STORE_INFO);
        } else {
            StoreShowPage(store, i == 1);
            GuiSoundPlay(SND_UI_TAB);
        }
    }
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawBuyPrice(Store *store, const StorePackDef *pack, Rectangle bar) {
    char price[16];
    snprintf(price, sizeof(price), "%d", pack->price);
    float size = bar.height * 0.72f;
    UiDrawTextRight(
        store->title_font,
        price,
        (Rectangle){bar.x, bar.y, bar.width * 0.5f, bar.height},
        size,
        (Color){235, 232, 220, 255}
    );
    Texture2D icon = pack->gems ? store->currency_gems : store->currency_gold;
    float icon_size = bar.height * 0.62f;
    UiDrawTexture(
        icon,
        (Rectangle){
            bar.x + bar.width * 0.775f - icon_size * 0.5f, bar.y + (bar.height - icon_size) * 0.5f, icon_size, icon_size
        },
        WHITE
    );
}

static void StoreDrawPackBullets(Store *store, const StorePackDef *pack, Rectangle rect) {
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
    StoreDrawCenteredFit(store->body_font, pack->bullets[index], rect, 19.0f, color);
}

static bool StoreDrawPackCard(Store *store, Player *player, Heroes *heroes, int pack_index, Rectangle rect) {
    const StorePackDef *pack = &store->packs[pack_index];
    bool show_info = store->info_pack == pack_index;
    bool in_viewport =
        CheckCollisionPointRec(UiMouse(), (Rectangle){STORE_LIST_X, STORE_LIST_Y, STORE_LIST_W, STORE_LIST_H}) &&
        !CheckCollisionPointRec(UiMouse(), STORE_ARROW_LEFT) && !CheckCollisionPointRec(UiMouse(), STORE_ARROW_RIGHT);
    Rectangle art = {rect.x + 24.0f, rect.y + 89.0f, rect.width - 48.0f, 237.0f};
    if (!show_info) StoreDrawTextureCover(store->icons[pack_index], art, WHITE);
    UiDrawTexture(store->card, rect, WHITE);

    float header_h = (rect.width + 14.0f) / 4.0f;
    Rectangle top = {rect.x - 7.0f, rect.y, rect.width + 14.0f, header_h};
    UiDrawTexture(store->headers[pack_index], top, WHITE);
    StoreDrawCenteredFit(
        store->title_font,
        pack->name,
        (Rectangle){top.x + 12.0f, top.y + header_h * 0.24f, top.width - 24.0f, header_h * 0.52f},
        34.0f,
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

    Rectangle info = {rect.x + rect.width - 60.0f, rect.y + 74.0f, 49.0f, 53.0f};
    bool info_hover = !StoreInputBlocked(store) && CheckCollisionPointRec(UiMouse(), info);
    UiDrawTexture(info_hover || show_info ? store->info_icon_on : store->info_icon, info, WHITE);

    StoreDrawPackBullets(store, pack, (Rectangle){rect.x + 14.0f, rect.y + 343.0f, rect.width - 28.0f, 46.0f});

    Rectangle buy = {rect.x - 5.5f, rect.y + 408.0f, rect.width + 11.0f, 62.0f};
    bool clicked = ButtonDrawEx(&store->buy[pack_index], buy, !StoreInputBlocked(store) && in_viewport);
    StoreDrawBuyPrice(store, pack, buy);

    if (in_viewport && StoreReleasedOn(store, info)) {
        store->info_pack = show_info ? -1 : pack_index;
        GuiSoundPlay(SND_STORE_INFO);
    } else if (in_viewport && StoreReleasedOn(store, (Rectangle){rect.x, rect.y, rect.width, 394.0f})) {
        store->info_pack = show_info ? -1 : pack_index;
        GuiSoundPlay(SND_STORE_INFO);
    }
    return clicked && StoreTryPurchase(store, player, heroes, pack_index);
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
    float content = STORE_PACK_COUNT * STORE_CELL_STRIDE;
    return content > STORE_LIST_W ? content - STORE_LIST_W : 0.0f;
}

static void StoreUpdateScrollInput(Store *store) {
    Rectangle viewport = {STORE_LIST_X, STORE_LIST_Y, STORE_LIST_W, STORE_LIST_H};
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
    StoreDrawScrollArrow(store, STORE_ARROW_LEFT, true, can_left);
    StoreDrawScrollArrow(store, STORE_ARROW_RIGHT, false, can_right);
    if (can_left && StoreReleasedOn(store, STORE_ARROW_LEFT)) {
        store->scroll -= STORE_SCROLL_STEP;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (can_right && StoreReleasedOn(store, STORE_ARROW_RIGHT)) {
        store->scroll += STORE_SCROLL_STEP;
        GuiSoundPlay(SND_UI_TAB);
    }
    store->scroll = Clamp(store->scroll, 0.0f, StoreMaxScroll());

    StoreAction action = STORE_ACTION_NONE;
    float scale = UiScale();
    BeginScissorMode(
        (int)(STORE_LIST_X * scale),
        (int)(STORE_LIST_Y * scale),
        (int)(STORE_LIST_W * scale),
        (int)(STORE_LIST_H * scale)
    );
    for (int pack_index = 0; pack_index < STORE_PACK_COUNT; pack_index++) {
        float cell_x = STORE_LIST_X + pack_index * STORE_CELL_STRIDE - store->scroll;
        if (cell_x + STORE_CELL_STRIDE < STORE_LIST_X || cell_x > STORE_LIST_X + STORE_LIST_W) continue;
        Rectangle card = {cell_x + STORE_CELL_STRIDE * 0.025f, STORE_LIST_Y, STORE_CELL_STRIDE * 0.95f, STORE_LIST_H};
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
    float cell_w = STORE_LIST_W * 0.245f;
    float cell_h = STORE_LIST_H * 0.5f;
    for (int i = 0; i < STORE_GEM_SKU_COUNT + 1; i++) {
        Rectangle cell = {STORE_LIST_X + (i % 4) * cell_w, STORE_LIST_Y + (i / 4) * cell_h, cell_w, cell_h};
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
        (Rectangle){222.0f, 655.0f, 836.0f, 26.0f},
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
        store->body_font, display, (Rectangle){222.0f, 686.0f, 836.0f, 26.0f}, 19.0f, (Color){255, 0, 255, 255}
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
    ButtonDrawEx(&store->nav_train, (Rectangle){4.0f, 662.0f, 106.0f, 49.5f}, false);
    ButtonDrawEx(&store->nav_runes, (Rectangle){120.0f, 662.0f, 106.0f, 49.5f}, false);
    return ButtonDrawEx(&store->nav_squad, (Rectangle){236.0f, 662.0f, 106.0f, 49.5f}, !StoreInputBlocked(store));
}

static StoreAction StoreDrawResult(Store *store, Player *player, Heroes *heroes) {
    HeroCard *card = StoreCurrentCard(store, heroes);
    bool last = store->reveal_index + 1 >= store->result_count;
    if (card && store->result_time >= 0.08f) HeroesDrawDetailsAt(card, (Rectangle){17.0f, 91.0f, 433.0f, 555.0f});
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
    if (again && StoreTryPurchase(store, player, heroes, store->selected_pack)) return STORE_ACTION_PURCHASED;
    StoreDrawToast(store);
    return STORE_ACTION_NONE;
}

static void StoreDrawDialog(Store *store) {
    if (store->dialog == STORE_DIALOG_NONE) return;
    const char *header =
        LocGet(store->dialog == STORE_DIALOG_CANT_AFFORD ? "MESSAGE_CANT_AFFORD_HEADER" : "MESSAGE_TOO_MANY_HEADER");
    const char *body = LocGet(
        store->dialog == STORE_DIALOG_CANT_AFFORD ? "MESSAGE_CANT_AFFORD_BODY" : "MESSAGE_TOO_MANY_BODY_NO_CONSUME"
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
    }
    StoreDrawDialog(store);
    return action;
}
