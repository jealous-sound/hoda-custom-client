
#define NEXUS_RECAP_DESIGN_WIDTH 1280.0f
#define NEXUS_RECAP_DESIGN_HEIGHT 720.0f
#define NEXUS_RECAP_FADE_TIME 0.2f
#define NEXUS_RECAP_CURRENCY_TIME 1.0f
#define NEXUS_RECAP_REVEAL_GAP 0.5f
#define NEXUS_RECAP_REVEALS_PER_LINE 5
#define NEXUS_RECAP_NAVIGATION_FADE_TIME 1.0f
#define NEXUS_RECAP_REWARDS_X 240.0f
#define NEXUS_RECAP_REWARDS_Y 172.0f
#define NEXUS_RECAP_REWARDS_WIDTH 802.0f
#define NEXUS_RECAP_REWARD_WIDTH 218.0f
#define NEXUS_RECAP_REWARD_HEIGHT 87.0f
#define NEXUS_RECAP_GOLD_LABEL_SHARE 0.61f
#define NEXUS_RECAP_POTION_LABEL_SHARE 0.7f
#define NEXUS_RECAP_AMOUNT_PADDING 5.0f
#define NEXUS_RECAP_AMOUNT_FONT_SIZE 34.0f
#define NEXUS_RECAP_TILES_X 249.0f
#define NEXUS_RECAP_TILES_Y 262.0f
#define NEXUS_RECAP_TILES_WIDTH 747.0f
#define NEXUS_RECAP_TILES_HEIGHT 294.0f
#define NEXUS_RECAP_TILE_SIZE 147.0f
#define NEXUS_RECAP_TILE_IMAGE_SCALE 0.9f

typedef struct {
    int gold, potions;
    int unit_count;
    int unit_roster[NEXUS_MAX_NODES];
    bool unit_newly_owned[NEXUS_MAX_NODES];
} NexusAward;

typedef enum {
    NEXUS_RECAP_NONE,
    NEXUS_RECAP_REPLAY,
    NEXUS_RECAP_RETURN,
} NexusRecapAction;

typedef struct {
    float image_down, image_up;
    float burst_wait, burst_up, burst_hold, burst_down;
    float burst_degrees_per_cycle;
} NexusRecapFanfare;

static const NexusRecapFanfare NEXUS_RECAP_NORMAL_FANFARE = {0.3f, 0.3f, 0.0f, 0.2f, 0.0f, 0.4f, 80.0f};
static const NexusRecapFanfare NEXUS_RECAP_NEWLY_OWNED_FANFARE = {0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 100.0f};

typedef struct {
    bool open;
    NexusAward award;
    float time;
    int currency_landed;
    bool currency_released;
    int tiles_started;
    int tiles_finished;
    float next_tile_time;
    float tile_start[NEXUS_MAX_NODES];
    bool tile_finished[NEXUS_MAX_NODES];
    int detail_card;
    bool navigation_visible;
    float navigation_time;
    Button replay, leave;
} NexusRecap;

static float NexusRecapScale(void) {
    return fminf(UiWidth() / NEXUS_RECAP_DESIGN_WIDTH, UiHeight() / NEXUS_RECAP_DESIGN_HEIGHT);
}

static Rectangle NexusRecapRect(float x, float y, float width, float height) {
    float scale = NexusRecapScale();
    float origin_x = (UiWidth() - NEXUS_RECAP_DESIGN_WIDTH * scale) * 0.5f;
    float origin_y = (UiHeight() - NEXUS_RECAP_DESIGN_HEIGHT * scale) * 0.5f;
    return (Rectangle){origin_x + x * scale, origin_y + y * scale, width * scale, height * scale};
}

static Color NexusRecapFade(Color color, float alpha) {
    color.a = (unsigned char)((float)color.a * Clamp(alpha, 0.0f, 1.0f));
    return color;
}

void NexusRecapInitialize(NexusRecap *recap) {
    *recap = (NexusRecap){.detail_card = -1};
    Font font = GuiFont(FNT_COOL);
    recap->replay = ButtonLoadText(
        GUI_BTN_GOLD,
        GUI_BTN_GOLD_ON,
        LocGetOr("NEXUS_REWARD_REPLAY_MAP", "Пройти заново"),
        font,
        (Vector2){0.5f, 0.5f},
        0.32f
    );
    recap->leave = ButtonLoadText(
        GUI_BTN_GOLD,
        GUI_BTN_GOLD_ON,
        LocGetOr("NEXUS_BUTTON_LOC_STRING", "Задание"),
        font,
        (Vector2){0.5f, 0.5f},
        0.32f
    );
}

static int NexusRecapCurrencyCount(const NexusRecap *recap) {
    return (recap->award.gold > 0) + (recap->award.potions > 0);
}

void NexusRecapOpen(NexusRecap *recap, const NexusAward *award) {
    recap->open = true;
    recap->award = *award;
    recap->time = 0.0f;
    recap->currency_landed = 0;
    recap->currency_released = false;
    recap->tiles_started = 0;
    recap->tiles_finished = 0;
    recap->next_tile_time = 0.0f;
    recap->detail_card = -1;
    recap->navigation_visible = false;
    recap->navigation_time = 0.0f;
    memset(recap->tile_finished, 0, sizeof(recap->tile_finished));
}

bool NexusRecapOpened(const NexusRecap *recap) {
    return recap->open;
}

static void NexusRecapReleaseCurrency(NexusRecap *recap, Hud *hud, const Player *player) {
    if (recap->currency_released) return;
    recap->currency_released = true;
    HudCurrencyRelease(hud, player, false);
}

static void NexusRecapClose(NexusRecap *recap, Hud *hud, const Player *player) {
    NexusRecapReleaseCurrency(recap, hud, player);
    recap->open = false;
}

bool NexusRecapHandleBack(NexusRecap *recap, Hud *hud, const Player *player) {
    if (!recap->open) return false;
    if (recap->detail_card >= 0) {
        recap->detail_card = -1;
    } else if (recap->navigation_visible) {
        NexusRecapClose(recap, hud, player);
        GuiClickSoundPlay();
    }
    return true;
}

static const NexusRecapFanfare *NexusRecapTileFanfare(const NexusRecap *recap, int index) {
    return recap->award.unit_newly_owned[index] ? &NEXUS_RECAP_NEWLY_OWNED_FANFARE : &NEXUS_RECAP_NORMAL_FANFARE;
}

static float NexusRecapBurstTime(const NexusRecapFanfare *fanfare) {
    return fanfare->burst_wait + fanfare->burst_up + fanfare->burst_hold + fanfare->burst_down;
}

static float NexusRecapTileTime(const NexusRecapFanfare *fanfare) {
    return fmaxf(fanfare->image_down + fanfare->image_up, NexusRecapBurstTime(fanfare));
}

static Rarity NexusRecapTileRarity(const NexusRecap *recap, const Player *player, int index) {
    return player->roster[recap->award.unit_roster[index]].rarity;
}

static void NexusRecapStartTile(NexusRecap *recap, const Player *player) {
    int index = recap->tiles_started++;
    recap->tile_start[index] = recap->time;
    recap->next_tile_time = recap->time + NEXUS_RECAP_REVEAL_GAP;
    GuiSoundPlay(StoreRevealSound(NexusRecapTileRarity(recap, player, index)));
}

static void NexusRecapFinishTiles(NexusRecap *recap) {
    for (int i = 0; i < recap->tiles_started; i++) {
        if (recap->tile_finished[i]) continue;
        float age = recap->time - recap->tile_start[i];
        if (age < NexusRecapTileTime(NexusRecapTileFanfare(recap, i))) continue;
        recap->tile_finished[i] = true;
        recap->tiles_finished++;
        if (recap->award.unit_newly_owned[i]) recap->detail_card = recap->award.unit_roster[i];
    }
}

static bool NexusRecapDetailPending(const NexusRecap *recap) {
    if (recap->detail_card >= 0) return true;
    for (int i = 0; i < recap->tiles_started; i++)
        if (recap->award.unit_newly_owned[i] && !recap->tile_finished[i]) return true;
    return false;
}

static void NexusRecapUpdate(NexusRecap *recap, Hud *hud, const Player *player, float dt) {
    if (recap->detail_card >= 0) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            recap->detail_card = -1;
            recap->next_tile_time = recap->time + NEXUS_RECAP_REVEAL_GAP;
        }
        return;
    }
    recap->time += fminf(dt, 0.1f);

    int currency_count = NexusRecapCurrencyCount(recap);
    while (recap->currency_landed < currency_count &&
           recap->time >= (float)(recap->currency_landed + 1) * NEXUS_RECAP_CURRENCY_TIME) {
        recap->currency_landed++;
        GuiSoundPlay(SND_TALLY_HIT);
    }
    if (recap->currency_landed == currency_count) NexusRecapReleaseCurrency(recap, hud, player);

    if (recap->tiles_started < recap->award.unit_count && recap->time >= recap->next_tile_time &&
        !NexusRecapDetailPending(recap))
        NexusRecapStartTile(recap, player);
    NexusRecapFinishTiles(recap);

    bool revealed = recap->tiles_finished == recap->award.unit_count;
    if (revealed && recap->currency_released && recap->detail_card < 0) {
        recap->navigation_visible = true;
        recap->navigation_time += dt;
    }
}

static float NexusRecapRewardPosition(int index, int count) {
    if (count <= 1) return 0.5f;
    return index == 0 ? 0.15f : 0.85f;
}

static void NexusRecapDrawCurrency(
    const NexusRecap *recap, Texture2D icon, int amount, int index, int count, Rectangle icon_rect, float label_share
) {
    float progress = (recap->time - (float)index * NEXUS_RECAP_CURRENCY_TIME) / NEXUS_RECAP_CURRENCY_TIME;
    if (progress <= 0.0f) return;
    progress = Clamp(progress, 0.0f, 1.0f);

    float span = NEXUS_RECAP_REWARDS_WIDTH - NEXUS_RECAP_REWARD_WIDTH;
    float x = NEXUS_RECAP_REWARDS_X + span * NexusRecapRewardPosition(index, count) * progress;
    float y = NEXUS_RECAP_REWARDS_Y;
    Color tint = NexusRecapFade(WHITE, progress);

    UiDrawTexture(
        GuiTex(GUI_NEXUS_RECAP_BRUSH), NexusRecapRect(x, y, NEXUS_RECAP_REWARD_WIDTH, NEXUS_RECAP_REWARD_HEIGHT), tint
    );
    UiDrawTexture(icon, NexusRecapRect(x + icon_rect.x, y + icon_rect.y, icon_rect.width, icon_rect.height), tint);
    float scale = NexusRecapScale();
    float label_x = x + NEXUS_RECAP_REWARD_WIDTH * (1.0f - label_share) + NEXUS_RECAP_AMOUNT_PADDING;
    Rectangle row = NexusRecapRect(label_x, y, 0.0f, NEXUS_RECAP_REWARD_HEIGHT);
    Font font = GuiFont(FNT_COOL);
    const char *text = TextFormat("+%d", amount);
    float size = NEXUS_RECAP_AMOUNT_FONT_SIZE * scale;
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    Vector2 at = {row.x, row.y + (row.height - measured.y) * 0.5f};
    DrawTextEx(font, text, at, size, 1.0f, NexusRecapFade((Color){230, 230, 230, 255}, progress));
}

static GuiId NexusRecapHiddenPortrait(Rarity rarity) {
    switch (rarity) {
        case RARITY_COMMON:
            return GUI_GALLERY_SHADOW_COMMON;
        case RARITY_UNCOMMON:
            return GUI_GALLERY_SHADOW_UNCOMMON;
        case RARITY_RARE:
            return GUI_GALLERY_SHADOW_RARE;
        case RARITY_EPIC:
            return GUI_GALLERY_SHADOW_EPIC;
        default:
            return GUI_GALLERY_SHADOW_LEGENDARY;
    }
}

static Rectangle NexusRecapTileRect(int index, int count) {
    int rows = (count + NEXUS_RECAP_REVEALS_PER_LINE - 1) / NEXUS_RECAP_REVEALS_PER_LINE;
    float size = fminf(NEXUS_RECAP_TILE_SIZE, NEXUS_RECAP_TILES_HEIGHT / (float)rows);
    int column = index % NEXUS_RECAP_REVEALS_PER_LINE;
    int row = index / NEXUS_RECAP_REVEALS_PER_LINE;
    float x = NEXUS_RECAP_TILES_X + (NEXUS_RECAP_TILES_WIDTH - size) * (float)column * 0.25f;
    float y = NEXUS_RECAP_TILES_Y + (NEXUS_RECAP_TILES_HEIGHT - size * (float)rows) * 0.5f + size * (float)row;
    return NexusRecapRect(x, y, size, size);
}

static Rectangle NexusRecapScaled(Rectangle rect, float scale) {
    float width = rect.width * scale, height = rect.height * scale;
    return (Rectangle){
        rect.x + (rect.width - width) * 0.5f,
        rect.y + (rect.height - height) * 0.5f,
        width,
        height,
    };
}

static void NexusRecapDrawBurst(const NexusRecapFanfare *fanfare, Rectangle tile, float age) {
    float total = NexusRecapBurstTime(fanfare);
    float t = age - fanfare->burst_wait;
    if (t < 0.0f || age >= total) return;

    float scale = 1.0f;
    if (t < fanfare->burst_up)
        scale = t / fanfare->burst_up;
    else if (t > fanfare->burst_up + fanfare->burst_hold)
        scale = (total - age) / fanfare->burst_down;

    Rectangle burst = NexusRecapScaled(tile, scale);
    Texture2D texture = GuiTex(GUI_RESULTS_STARBURST);
    DrawTexturePro(
        texture,
        (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height},
        (Rectangle){burst.x + burst.width * 0.5f, burst.y + burst.height * 0.5f, burst.width, burst.height},
        (Vector2){burst.width * 0.5f, burst.height * 0.5f},
        (total - age) / total * fanfare->burst_degrees_per_cycle,
        WHITE
    );
}

static void NexusRecapDrawTile(const NexusRecap *recap, const Heroes *heroes, const Player *player, int index) {
    int card = recap->award.unit_roster[index];
    if (card < 0 || card >= heroes->card_count) return;

    const NexusRecapFanfare *fanfare = NexusRecapTileFanfare(recap, index);
    bool started = index < recap->tiles_started;
    float age = started ? recap->time - recap->tile_start[index] : 0.0f;
    Rectangle tile = NexusRecapTileRect(index, recap->award.unit_count);
    if (started) NexusRecapDrawBurst(fanfare, tile, age);

    bool revealed = started && age >= fanfare->image_down;
    float scale = 1.0f;
    if (started) {
        scale = revealed ? Clamp((age - fanfare->image_down) / fanfare->image_up, 0.0f, 1.0f)
                         : 1.0f - age / fanfare->image_down;
    }
    Texture2D texture = revealed ? heroes->cards[card].portrait
                                 : GuiTex(NexusRecapHiddenPortrait(NexusRecapTileRarity(recap, player, index)));
    UiDrawTexture(texture, NexusRecapScaled(tile, NEXUS_RECAP_TILE_IMAGE_SCALE * scale), WHITE);
}

static void NexusRecapDrawWindow(const NexusRecap *recap) {
    float fade = Clamp(recap->time / NEXUS_RECAP_FADE_TIME, 0.0f, 1.0f);
    float scale = NexusRecapScale();
    DrawRectangleRec(UiRoot(), NexusRecapFade((Color){4, 5, 7, 224}, fade));
    UiDrawTexture(
        GuiTex(GUI_RESULTS_OVERLAY), NexusRecapRect(160.0f, 71.0f, 960.0f, 648.0f), NexusRecapFade(WHITE, fade)
    );
    Texture2D header = GuiTex(GUI_MM_TROUGH);
    NPatchInfo header_slices = {{0, 0, (float)header.width, (float)header.height}, 21, 22, 19, 31, NPATCH_NINE_PATCH};
    DrawTextureNPatch(
        header,
        header_slices,
        NexusRecapRect(228.0f, 164.0f, 826.0f, 102.0f),
        (Vector2){0, 0},
        0.0f,
        NexusRecapFade(WHITE, fade * 0.2f)
    );
    UiDrawTexture(
        GuiTex(GUI_NEXUS_RECAP_BRUSH), NexusRecapRect(228.0f, 602.0f, 826.0f, 68.0f), NexusRecapFade(WHITE, fade)
    );
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        LocGetOr("NEXUS_REWARD_WINDOW_TITLE", "Карта пройдена!"),
        NexusRecapRect(160.0f, 71.0f, 961.0f, 65.0f),
        29.0f * scale,
        NexusRecapFade(WHITE, fade)
    );
}

static NexusRecapAction NexusRecapDrawNavigation(NexusRecap *recap) {
    if (!recap->navigation_visible) return NEXUS_RECAP_NONE;
    UiDrawTextCentered(
        GuiFont(FNT_MENU),
        LocGetOr("NEXUS_REWARD_WINDOW_INSTRUCTIONS", "Откройте свои награды, нажимая на значки!"),
        NexusRecapRect(228.0f, 556.0f, 826.0f, 51.0f),
        20.0f * NexusRecapScale(),
        NexusRecapFade((Color){230, 230, 230, 255}, recap->navigation_time / NEXUS_RECAP_NAVIGATION_FADE_TIME)
    );
    bool interactive = recap->navigation_time >= NEXUS_RECAP_NAVIGATION_FADE_TIME;
    if (ButtonDrawEx(&recap->replay, NexusRecapRect(782.0f, 602.0f, 272.0f, 68.0f), interactive))
        return NEXUS_RECAP_REPLAY;
    if (ButtonDrawEx(&recap->leave, NexusRecapRect(514.0f, 602.0f, 272.0f, 68.0f), interactive))
        return NEXUS_RECAP_RETURN;
    return NEXUS_RECAP_NONE;
}

NexusRecapAction NexusRecapDraw(NexusRecap *recap, Heroes *heroes, Hud *hud, const Player *player) {
    if (!recap->open) return NEXUS_RECAP_NONE;
    NexusRecapUpdate(recap, hud, player, GetFrameTime());
    NexusRecapDrawWindow(recap);

    int currency_count = NexusRecapCurrencyCount(recap);
    int currency_index = 0;
    if (recap->award.gold > 0) {
        NexusRecapDrawCurrency(
            recap,
            GuiTex(GUI_HUD_GOLD),
            recap->award.gold,
            currency_index++,
            currency_count,
            (Rectangle){0.0f, 0.0f, NEXUS_RECAP_REWARD_HEIGHT, NEXUS_RECAP_REWARD_HEIGHT},
            NEXUS_RECAP_GOLD_LABEL_SHARE
        );
    }
    if (recap->award.potions > 0) {
        NexusRecapDrawCurrency(
            recap,
            GuiTex(GUI_NEXUS_POTION),
            recap->award.potions,
            currency_index++,
            currency_count,
            (Rectangle){5.0f, 9.0f, 64.0f, 64.0f},
            NEXUS_RECAP_POTION_LABEL_SHARE
        );
    }
    for (int i = 0; i < recap->award.unit_count; i++) NexusRecapDrawTile(recap, heroes, player, i);

    NexusRecapAction action = NexusRecapDrawNavigation(recap);
    if (action != NEXUS_RECAP_NONE) NexusRecapClose(recap, hud, player);

    if (recap->detail_card >= 0 && recap->detail_card < heroes->card_count) {
        DrawRectangleRec(UiRoot(), (Color){0, 0, 0, 128});
        HeroesDrawDetailsAt(&heroes->cards[recap->detail_card], NexusRecapRect(397.0f, 91.0f, 487.0f, 613.0f));
    }
    return action;
}
