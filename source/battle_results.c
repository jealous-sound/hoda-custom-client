#define BATTLE_RESULTS_MAX_PRIZES 2
#define BATTLE_RESULTS_FULL_SIZE_REWARDS 3
#define BATTLE_RESULTS_MAX_REWARDS (5 + BATTLE_RESULTS_MAX_PRIZES)
#define BATTLE_RESULTS_CLICK_GUARD_SECONDS 0.22f
#define BATTLE_RESULTS_ESCAPED_NEWLINE "\\n"
#define BATTLE_RESULTS_VICTORIES_TOKEN "[numberOfVictories]"
#define HERO_LEVEL_DESIGN_WIDTH 1280.0f
#define HERO_LEVEL_DESIGN_HEIGHT 720.0f
#define HERO_LEVEL_XP_EASE_SECONDS 2.0f
#define HERO_LEVEL_FADE_SECONDS 0.2f
#define TEXT_ALIGN_START 0.0f
#define TEXT_ALIGN_CENTER 0.5f
#define TEXT_ALIGN_END 1.0f

typedef struct {
    bool victory;
    bool first_win;
    bool mastered;
    bool grind;
    int completions_pre;
    int completions_post;
    int wins_to_master;
    int player_level_pre;
    int player_level_post;
    int energy_max_delta;
    int xp;
    int gold;
    int completion_gold;
    int completion_mastery_gems;
    int gems;
    bool escrowed;
    bool pvp;
    int trophy_delta;
    int banner_delta;
    PvpPayout event_payout;
    BattleSquadLeveling squad;
} BattleResultsPresentation;

typedef enum {
    BATTLE_RESULTS_OFF,
    BATTLE_RESULTS_PLAYER_LEVEL,
    BATTLE_RESULTS_HERO_LEVEL,
    BATTLE_RESULTS_NODE_COMPLETION,
    BATTLE_RESULTS_REWARDS,
    BATTLE_RESULTS_DONE,
} BattleResultsStage;

typedef enum {
    BATTLE_RESULTS_ACTION_NONE,
    BATTLE_RESULTS_ACTION_CONTINUE,
    BATTLE_RESULTS_ACTION_HEROES,
    BATTLE_RESULTS_ACTION_MAP,
    BATTLE_RESULTS_ACTION_STORE,
} BattleResultsAction;

typedef struct {
    float from, to, duration, time;
    bool running;
} HeroLevelEasing;

typedef struct {
    int hero;
    int next_hero;
    float min, max, value;
    HeroLevelEasing easing;
    bool crossed_level;
} HeroLevelScreen;

typedef struct {
    BattleResultsPresentation presentation;
    BattleResultsStage stage;
    float stage_time;
    int reward_sounds_played;
    BattleResultsAction armed_action;
    HeroLevelScreen hero_level;
} BattleResultsFlow;

typedef struct {
    GuiId icon;
    int amount;
    bool animate_quantity;
    bool cool;
    Texture2D prize;
} BattleResultsReward;

static float BattleResultsClamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float BattleResultsEaseInCubic(float x) {
    x = BattleResultsClamp01(x);
    return x * x * x;
}

static Color BattleResultsAlpha(Color color, float alpha) {
    color.a = (unsigned char)((float)color.a * BattleResultsClamp01(alpha));
    return color;
}

static void BattleResultsDrawTextureRotated(Texture2D texture, Rectangle dst, float rotation, Color tint) {
    DrawTexturePro(
        texture,
        (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height},
        (Rectangle){dst.x + dst.width * 0.5f, dst.y + dst.height * 0.5f, dst.width, dst.height},
        (Vector2){dst.width * 0.5f, dst.height * 0.5f},
        rotation,
        tint
    );
}

static void
BattleResultsDrawTextInRect(Font font, const char *text, Rectangle rect, float size, Color tint, bool shadow) {
    if (shadow) {
        Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
        Vector2 at = {
            rect.x + (rect.width - measured.x) * 0.5f + 1.0f,
            rect.y + (rect.height - measured.y) * 0.5f + 2.0f,
        };
        DrawTextEx(font, text, at, size, 1.0f, BattleResultsAlpha(BLACK, (float)tint.a / 255.0f * 0.8f));
    }
    UiDrawTextCentered(font, text, rect, size, tint);
}

static void BattleResultsDrawScrim(float opacity) {
    DrawRectangleRec(UiRoot(), BattleResultsAlpha((Color){4, 5, 7, 224}, opacity));
}

static bool BattleResultsHasCompletion(const BattleResultsPresentation *p) {
    return !p->grind && (p->first_win || p->mastered);
}

static bool HeroLeveled(const BattleHeroLeveling *hero) {
    return hero->post.level != hero->pre.level;
}

static int HeroLevelNextFrom(const BattleResultsPresentation *p, int from) {
    for (int i = from; i < p->squad.count; i++)
        if (HeroLeveled(&p->squad.heroes[i])) return i;
    return -1;
}

static bool BattleResultsHasHeroLevel(const BattleResultsPresentation *p) {
    return HeroLevelNextFrom(p, 0) >= 0;
}

static float HeroLevelQuarticEase(float t) {
    if (t < 0.5f) return 8.0f * t * t * t * t;
    t -= 1.0f;
    return 1.0f - 8.0f * t * t * t * t;
}

static HeroLevelEasing HeroLevelEasingStart(float from, float to) {
    return (HeroLevelEasing){.from = from, .to = to, .duration = HERO_LEVEL_XP_EASE_SECONDS, .running = true};
}

static float HeroLevelEasingStep(HeroLevelEasing *easing, float dt) {
    easing->time += dt;
    float t = easing->duration > 0.0f ? fminf(easing->time / easing->duration, 1.0f) : 1.0f;
    if (t >= 1.0f) easing->running = false;
    return easing->from + (easing->to - easing->from) * HeroLevelQuarticEase(t);
}

static SoundId HeroLevelSound(int level) {
    static const struct {
        int above;
        SoundId sound;
    } bands[] = {{30, SND_CONSUME_LEVEL_4}, {20, SND_CONSUME_LEVEL_3}, {10, SND_CONSUME_LEVEL_2}};
    for (size_t i = 0; i < sizeof(bands) / sizeof(bands[0]); i++)
        if (level > bands[i].above) return bands[i].sound;
    return SND_CONSUME_LEVEL_1;
}

static void HeroLevelScreenShow(HeroLevelScreen *screen, const BattleHeroLeveling *hero, int index) {
    screen->hero = index;
    screen->crossed_level = false;
    screen->min = (float)hero->pre.band_low;
    screen->max = (float)hero->pre.band_high;
    screen->value = (float)hero->pre.xp;
    float target = hero->pre.level < hero->post.level ? screen->max : (float)hero->post.xp;
    screen->easing = HeroLevelEasingStart(screen->value, target);
    GuiSoundPlay(HeroLevelSound(hero->post.level));
}

static void HeroLevelScreenUpdate(HeroLevelScreen *screen, const BattleHeroLeveling *hero, float dt) {
    if (!screen->easing.running) return;
    screen->value = HeroLevelEasingStep(&screen->easing, dt);
    if (screen->easing.running) return;
    if (hero->pre.level >= hero->post.level || screen->crossed_level) return;
    screen->crossed_level = true;
    if (hero->post.maxed) {
        screen->min = 0.0f;
        screen->max = screen->value = 1.0f;
        return;
    }
    screen->min = screen->value = (float)hero->post.band_low;
    screen->max = (float)hero->post.band_high;
    screen->easing = HeroLevelEasingStart(screen->value, (float)hero->post.xp);
}

static float HeroLevelBarRatio(float min, float max, float value) {
    return max > min ? BattleResultsClamp01((value - min) / (max - min)) : 1.0f;
}

static int BattleResultsRewardCount(const BattleResultsPresentation *p) {
    return (p->gems > 0) + (p->xp > 0) + (p->gold > 0) + (p->trophy_delta != 0) + (p->banner_delta != 0) +
           (p->event_payout.prize_count < BATTLE_RESULTS_MAX_PRIZES ? p->event_payout.prize_count
                                                                    : BATTLE_RESULTS_MAX_PRIZES);
}

static float BattleResultsPopupScale(void) {
    return fminf(UiHeight() / 639.0f, UiWidth() / 1061.0f);
}

static Rectangle BattleResultsPopupRect(float x, float y, float width, float height) {
    float scale = BattleResultsPopupScale();
    float root_width = 1061.0f * scale;
    float root_height = 639.0f * scale;
    float origin_x = (UiWidth() - root_width) * 0.5f;
    float origin_y = (UiHeight() - root_height) * 0.5f;
    return (Rectangle){origin_x + x * scale, origin_y + y * scale, width * scale, height * scale};
}

static Rectangle BattleResultsRewardsContinueRect(void) {
    return BattleResultsPopupRect(659.0f, 535.0f, 240.0f, 60.0f);
}

static Rectangle BattleResultsRewardsStoreRect(void) {
    return BattleResultsPopupRect(164.0f, 535.0f, 120.0f, 60.0f);
}

static Rectangle BattleResultsRewardsHeroesRect(void) {
    return BattleResultsPopupRect(284.0f, 535.0f, 120.0f, 60.0f);
}

static Rectangle BattleResultsRewardsMapRect(void) {
    return BattleResultsPopupRect(404.0f, 535.0f, 120.0f, 60.0f);
}

static void BattleResultsEnterStage(BattleResultsFlow *flow, BattleResultsStage stage) {
    flow->stage = stage;
    flow->stage_time = 0.0f;
    flow->reward_sounds_played = 0;
    flow->armed_action = BATTLE_RESULTS_ACTION_NONE;

    if (stage == BATTLE_RESULTS_PLAYER_LEVEL || stage == BATTLE_RESULTS_NODE_COMPLETION) GuiSoundPlay(SND_LEVEL_UP);
    if (stage == BATTLE_RESULTS_HERO_LEVEL) {
        int first = HeroLevelNextFrom(&flow->presentation, 0);
        HeroLevelScreenShow(&flow->hero_level, &flow->presentation.squad.heroes[first], first);
        flow->hero_level.next_hero = HeroLevelNextFrom(&flow->presentation, first + 1);
    }
}

static BattleResultsStage BattleResultsStageAfterHeroLevels(const BattleResultsPresentation *p) {
    return BattleResultsHasCompletion(p) ? BATTLE_RESULTS_NODE_COMPLETION : BATTLE_RESULTS_REWARDS;
}

static BattleResultsStage BattleResultsStageAfterPlayerLevel(const BattleResultsPresentation *p) {
    return BattleResultsHasHeroLevel(p) ? BATTLE_RESULTS_HERO_LEVEL : BattleResultsStageAfterHeroLevels(p);
}

void BattleResultsFlowInitialize(BattleResultsFlow *flow) {
    *flow = (BattleResultsFlow){0};
}

void BattleResultsFlowStart(BattleResultsFlow *flow, BattleResultsPresentation presentation) {
    if (presentation.xp < 0) presentation.xp = 0;
    if (presentation.gold < 0) presentation.gold = 0;
    if (presentation.completion_gold < 0) presentation.completion_gold = 0;
    if (presentation.completion_mastery_gems < 0) presentation.completion_mastery_gems = 0;
    if (presentation.gems < 0) presentation.gems = 0;
    flow->presentation = presentation;

    if (presentation.player_level_post > presentation.player_level_pre)
        BattleResultsEnterStage(flow, BATTLE_RESULTS_PLAYER_LEVEL);
    else
        BattleResultsEnterStage(flow, BattleResultsStageAfterPlayerLevel(&presentation));
}

const BattleHeroLeveling *BattleResultsFlowLevelUpHero(const BattleResultsFlow *flow) {
    if (flow->stage != BATTLE_RESULTS_HERO_LEVEL) return NULL;
    return &flow->presentation.squad.heroes[flow->hero_level.hero];
}

void BattleResultsFlowReset(BattleResultsFlow *flow) {
    *flow = (BattleResultsFlow){0};
}

bool BattleResultsFlowActive(const BattleResultsFlow *flow) {
    return flow->stage > BATTLE_RESULTS_OFF && flow->stage < BATTLE_RESULTS_DONE;
}

bool BattleResultsFlowTopUiVisible(const BattleResultsFlow *flow) {
    return BattleResultsFlowActive(flow) && flow->stage == BATTLE_RESULTS_REWARDS;
}

Player BattleResultsFlowHudPlayer(const BattleResultsFlow *flow, const Player *player) {
    Player copy = *player;
    if (flow->stage != BATTLE_RESULTS_REWARDS || flow->presentation.escrowed) return copy;

    int reward_count = BattleResultsRewardCount(&flow->presentation);
    float released = flow->stage_time - (float)reward_count;
    float progress = BattleResultsClamp01(released / 2.25f);
    if (flow->presentation.gold > 0) {
        int pre = copy.gold >= flow->presentation.gold ? copy.gold - flow->presentation.gold : 0;
        copy.gold = (int)roundf(Lerp((float)pre, (float)copy.gold, progress));
    }
    if (flow->presentation.gems > 0) {
        int pre = copy.gems >= flow->presentation.gems ? copy.gems - flow->presentation.gems : 0;
        copy.gems = (int)roundf(Lerp((float)pre, (float)copy.gems, progress));
    }
    return copy;
}

static void BattleResultsAdvance(BattleResultsFlow *flow) {
    if (flow->stage == BATTLE_RESULTS_PLAYER_LEVEL) {
        BattleResultsEnterStage(flow, BattleResultsStageAfterPlayerLevel(&flow->presentation));
    } else if (flow->stage == BATTLE_RESULTS_HERO_LEVEL) {
        HeroLevelScreen *screen = &flow->hero_level;
        if (screen->next_hero < 0) {
            BattleResultsEnterStage(flow, BattleResultsStageAfterHeroLevels(&flow->presentation));
            return;
        }
        int shown = screen->next_hero;
        HeroLevelScreenShow(screen, &flow->presentation.squad.heroes[shown], shown);
        screen->next_hero = HeroLevelNextFrom(&flow->presentation, shown + 1);
        flow->armed_action = BATTLE_RESULTS_ACTION_NONE;
    } else if (flow->stage == BATTLE_RESULTS_NODE_COMPLETION) {
        BattleResultsEnterStage(flow, BATTLE_RESULTS_REWARDS);
    } else if (flow->stage == BATTLE_RESULTS_REWARDS) {
        BattleResultsEnterStage(flow, BATTLE_RESULTS_DONE);
    }
}

BattleResultsAction BattleResultsFlowUpdate(BattleResultsFlow *flow, float dt, bool interactive) {
    if (!BattleResultsFlowActive(flow)) return BATTLE_RESULTS_ACTION_NONE;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;
    flow->stage_time += dt;
    if (flow->stage == BATTLE_RESULTS_HERO_LEVEL)
        HeroLevelScreenUpdate(&flow->hero_level, &flow->presentation.squad.heroes[flow->hero_level.hero], dt);

    if (flow->stage == BATTLE_RESULTS_REWARDS) {
        int reward_count = BattleResultsRewardCount(&flow->presentation);
        while (flow->reward_sounds_played < reward_count && flow->stage_time >= (float)flow->reward_sounds_played) {
            int index = flow->reward_sounds_played++;
            SoundId sound_id = SND_TALLY_HIT;
            if (index == 0)
                sound_id = flow->presentation.victory ? SND_TALLY_START : SND_TALLY_LOSS_START;
            else if (index == reward_count - 1)
                sound_id = flow->presentation.victory ? SND_TALLY_END : SND_TALLY_LOSS_END;
            GuiSoundPlay(sound_id);
        }
    }

    bool keyboard = interactive && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));
    BattleResultsAction under_mouse = BATTLE_RESULTS_ACTION_NONE;
    if (flow->stage == BATTLE_RESULTS_REWARDS) {
        if (CheckCollisionPointRec(UiMouse(), BattleResultsRewardsContinueRect()))
            under_mouse = BATTLE_RESULTS_ACTION_CONTINUE;
        else if (CheckCollisionPointRec(UiMouse(), BattleResultsRewardsHeroesRect()))
            under_mouse = BATTLE_RESULTS_ACTION_HEROES;
        else if (CheckCollisionPointRec(UiMouse(), BattleResultsRewardsMapRect()))
            under_mouse = BATTLE_RESULTS_ACTION_MAP;
        else if (CheckCollisionPointRec(UiMouse(), BattleResultsRewardsStoreRect()))
            under_mouse = BATTLE_RESULTS_ACTION_STORE;
    } else {
        under_mouse = BATTLE_RESULTS_ACTION_CONTINUE;
    }

    if (!interactive) flow->armed_action = BATTLE_RESULTS_ACTION_NONE;
    if (interactive && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) flow->armed_action = under_mouse;

    BattleResultsAction action = keyboard ? BATTLE_RESULTS_ACTION_CONTINUE : BATTLE_RESULTS_ACTION_NONE;
    if (interactive && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (flow->armed_action != BATTLE_RESULTS_ACTION_NONE && flow->armed_action == under_mouse)
            action = flow->armed_action;
        flow->armed_action = BATTLE_RESULTS_ACTION_NONE;
    }
    if (action == BATTLE_RESULTS_ACTION_NONE || flow->stage_time < BATTLE_RESULTS_CLICK_GUARD_SECONDS)
        return BATTLE_RESULTS_ACTION_NONE;

    if (flow->stage == BATTLE_RESULTS_REWARDS) {
        if (action == BATTLE_RESULTS_ACTION_HEROES || action == BATTLE_RESULTS_ACTION_MAP ||
            action == BATTLE_RESULTS_ACTION_STORE)
            GuiSoundPlay(SND_UI_BIG_BUTTON);
        else
            GuiClickSoundPlay();
    }
    BattleResultsAdvance(flow);
    return flow->stage == BATTLE_RESULTS_DONE ? action : BATTLE_RESULTS_ACTION_NONE;
}

static Rectangle BattleResultsLevelRect(Rectangle panel, float x, float y, float width, float height) {
    float scale = panel.height / 265.0f;
    return (Rectangle){
        panel.x + (x - 46.0f) * scale,
        panel.y + (y - 80.0f) * scale,
        width * scale,
        height * scale,
    };
}

static void BattleResultsDrawLevelUp(const BattleResultsFlow *flow) {
    const BattleResultsPresentation *p = &flow->presentation;
    float fade = BattleResultsClamp01(flow->stage_time / 0.2f);
    float aspect = 564.185f / 265.0f;
    float panel_height = UiHeight() * 0.625f;
    float panel_width = panel_height * aspect;
    if (panel_width > UiWidth() * 0.96f) {
        panel_width = UiWidth() * 0.96f;
        panel_height = panel_width / aspect;
    }
    Rectangle panel = {
        (UiWidth() - panel_width) * 0.5f,
        (UiHeight() - panel_height) * 0.5f,
        panel_width,
        panel_height,
    };
    float scale = panel.height / 265.0f;

    BattleResultsDrawScrim(fade);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), panel, BattleResultsAlpha(WHITE, fade));

    char escaped[160], heading[160];
    LocFormatInt(escaped, sizeof(escaped), "PLAYER_LEVEL_UP_STRING", p->player_level_post);
    StrReplaceToken(heading, sizeof(heading), escaped, BATTLE_RESULTS_ESCAPED_NEWLINE, "\n");
    BattleResultsDrawTextInRect(
        GuiFont(FNT_COOL),
        heading,
        BattleResultsLevelRect(panel, 75.0f, 107.0f, 508.0f, 53.0f),
        21.0f * scale,
        BattleResultsAlpha(WHITE, fade),
        false
    );

    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("PLAYER_LEVEL_ENERGY"),
        BattleResultsLevelRect(panel, 100.0f, 164.0f, 115.0f, 64.0f),
        16.0f * scale,
        BattleResultsAlpha(WHITE, fade),
        false
    );
    UiDrawTexture(
        GuiTex(GUI_HUD_ENERGY),
        BattleResultsLevelRect(panel, 215.0f, 173.0f, 46.0f, 46.0f),
        BattleResultsAlpha(WHITE, fade)
    );
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("PLAYER_LEVEL_STAMINA"),
        BattleResultsLevelRect(panel, 261.0f, 164.0f, 92.0f, 64.0f),
        16.0f * scale,
        BattleResultsAlpha(WHITE, fade),
        false
    );
    UiDrawTexture(
        GuiTex(GUI_HUD_STAMINA),
        BattleResultsLevelRect(panel, 353.0f, 173.0f, 46.0f, 46.0f),
        BattleResultsAlpha(WHITE, fade)
    );
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("PLAYER_LEVEL_REFRESH"),
        BattleResultsLevelRect(panel, 399.0f, 164.0f, 161.0f, 64.0f),
        16.0f * scale,
        BattleResultsAlpha(WHITE, fade),
        false
    );

    if (p->energy_max_delta != 0) {
        Rectangle brush = BattleResultsLevelRect(panel, 145.0f, 233.0f, 116.0f, 39.19f);
        UiDrawTexture(GuiTex(GUI_REWARD_BRUSH), brush, BattleResultsAlpha(WHITE, fade));
        UiDrawTexture(
            GuiTex(GUI_HUD_ENERGY),
            BattleResultsLevelRect(panel, 209.0f, 224.0f, 52.0f, 52.0f),
            BattleResultsAlpha(WHITE, fade)
        );

        char amount[24];
        snprintf(amount, sizeof(amount), "%+d", p->energy_max_delta);
        BattleResultsDrawTextInRect(
            GuiFont(FNT_COOL),
            amount,
            BattleResultsLevelRect(panel, 151.0f, 234.0f, 58.0f, 32.0f),
            21.0f * scale,
            BattleResultsAlpha((Color){242, 242, 242, 255}, fade),
            false
        );
        BattleResultsDrawTextInRect(
            GuiFont(FNT_COOL),
            LocGet("PLAYER_LEVEL_UP_MAX"),
            BattleResultsLevelRect(panel, 111.0f, 237.0f, 58.0f, 32.0f),
            14.0f * scale,
            BattleResultsAlpha((Color){242, 242, 242, 255}, fade),
            false
        );
    }

    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("PLAYER_LEVEL_UP_TAP"),
        BattleResultsLevelRect(panel, 75.0f, 287.0f, 508.0f, 32.0f),
        16.0f * scale,
        BattleResultsAlpha((Color){230, 230, 230, 255}, fade),
        false
    );
}

static Rectangle HeroLevelRect(float x, float y, float width, float height) {
    float sx = UiWidth() / HERO_LEVEL_DESIGN_WIDTH, sy = UiHeight() / HERO_LEVEL_DESIGN_HEIGHT;
    return (Rectangle){x * sx, y * sy, width * sx, height * sy};
}

static float HeroLevelFontSize(float dynamic_size, float element_height) {
    return dynamic_size * element_height * UiHeight() / HERO_LEVEL_DESIGN_HEIGHT;
}

static void BattleResultsDrawTextAligned(
    Font font, const char *text, Rectangle rect, float size, Color color, float align_x, float align_y
) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    Vector2 at = {
        rect.x + (rect.width - measured.x) * align_x,
        rect.y + (rect.height - measured.y) * align_y,
    };
    DrawTextEx(font, text, at, size, 1.0f, color);
}

static GuiId HeroLevelRarityHeader(Rarity rarity) {
    static const GuiId headers[RARITY_MYTHIC + 1] = {
        GUI_PLAQUE_COMMON,
        GUI_PLAQUE_COMMON,
        GUI_PLAQUE_UNCOMMON,
        GUI_PLAQUE_RARE,
        GUI_PLAQUE_EPIC,
        GUI_PLAQUE_LEGENDARY,
        GUI_PLAQUE_MYTHIC,
    };
    return headers[rarity >= RARITY_COMMON && rarity <= RARITY_MYTHIC ? rarity : RARITY_COMMON];
}

static void HeroLevelDrawStatRow(const char *name, int value, int gain, float y, float fade) {
    Color text = BattleResultsAlpha((Color){230, 230, 230, 255}, fade);
    BattleResultsDrawTextAligned(
        GuiFont(FNT_MENU),
        name,
        HeroLevelRect(848.0f, y, 146.0f, 54.0f),
        HeroLevelFontSize(0.6f, 54.0f),
        text,
        TEXT_ALIGN_START,
        TEXT_ALIGN_CENTER
    );
    char amount[48];
    snprintf(amount, sizeof(amount), "%d (+%d)", value, gain);
    BattleResultsDrawTextAligned(
        GuiFont(FNT_MENU),
        amount,
        HeroLevelRect(930.0f, y, 271.0f, 54.0f),
        HeroLevelFontSize(0.5f, 54.0f),
        text,
        TEXT_ALIGN_END,
        TEXT_ALIGN_CENTER
    );
}

static void BattleResultsDrawHeroLevel(const BattleResultsFlow *flow, const Heroes *heroes) {
    const HeroLevelScreen *screen = &flow->hero_level;
    const BattleHeroLeveling *hero = &flow->presentation.squad.heroes[screen->hero];
    if (!heroes || hero->roster_index < 0 || hero->roster_index >= heroes->card_count) return;
    const HeroCard *card = &heroes->cards[hero->roster_index];
    float fade = BattleResultsClamp01(flow->stage_time / HERO_LEVEL_FADE_SECONDS);
    Color tint = BattleResultsAlpha(WHITE, fade);
    Color text = BattleResultsAlpha((Color){230, 230, 230, 255}, fade);

    UiDrawTexture(GuiTex(GUI_LEVEL_UP_TITLE), HeroLevelRect(426.0f, 0.0f, 430.0f, 126.0f), tint);
    BattleResultsDrawTextAligned(
        GuiFont(FNT_COOL),
        LocGet("LEVEL_UP_TITLE"),
        HeroLevelRect(426.0f, 13.0f, 430.0f, 91.0f),
        HeroLevelFontSize(0.4f, 91.0f),
        text,
        TEXT_ALIGN_CENTER,
        TEXT_ALIGN_CENTER
    );

    Rectangle level_badge = HeroLevelRect(604.0f, 98.0f, 73.0f, 73.0f);
    UiDrawTexture(GuiTex(GUI_HUD_XP), level_badge, tint);
    char level[16];
    snprintf(level, sizeof(level), "%d", hero->post.level);
    BattleResultsDrawTextAligned(
        GuiFont(FNT_COOL),
        level,
        level_badge,
        HeroLevelFontSize(0.4f, 73.0f),
        text,
        TEXT_ALIGN_CENTER,
        TEXT_ALIGN_CENTER
    );

    UiDrawTexture(GuiTex(GUI_LEVEL_UP_OVERLAY), HeroLevelRect(735.0f, 202.0f, 513.0f, 389.0f), tint);
    UiDrawTexture(
        GuiTex(HeroLevelRarityHeader(card->ch->original_rarity)), HeroLevelRect(778.0f, 236.0f, 427.0f, 97.0f), tint
    );
    BattleResultsDrawTextAligned(
        GuiFont(FNT_COOL),
        card->name,
        HeroLevelRect(778.0f, 236.0f, 427.0f, 51.0f),
        HeroLevelFontSize(0.365f, 51.0f),
        text,
        TEXT_ALIGN_CENTER,
        TEXT_ALIGN_END
    );
    BattleResultsDrawTextAligned(
        GuiFont(FNT_COOL),
        card->rarity,
        HeroLevelRect(778.0f, 286.0f, 427.0f, 47.0f),
        HeroLevelFontSize(0.325f, 47.0f),
        BattleResultsAlpha(card->rarity_color, fade),
        TEXT_ALIGN_CENTER,
        TEXT_ALIGN_START
    );

    Rectangle bar = HeroLevelRect(922.0f, 352.0f, 279.0f, 33.0f);
    bar.width *= HeroLevelBarRatio(screen->min, screen->max, screen->value);
    UiDrawTexture(GuiTex(GUI_CARD_BAR_FILL), bar, tint);

    const BattleHeroStats *pre = &hero->pre, *post = &hero->post;
    HeroLevelDrawStatRow(LocGet("UNIT_DETAIL_XP"), post->xp, post->xp - pre->xp, 345.0f, fade);
    HeroLevelDrawStatRow(LocGet("UNIT_DETAIL_POWER"), post->power, post->power - pre->power, 417.0f, fade);
    HeroLevelDrawStatRow(LocGet("UNIT_DETAIL_HEALTH"), post->health, post->health - pre->health, 486.0f, fade);

    BattleResultsDrawTextAligned(
        GuiFont(FNT_MENU),
        LocGet("TAP_TO_CONTINUE"),
        HeroLevelRect(0.0f, 662.0f, 1280.0f, 58.0f),
        HeroLevelFontSize(0.4f, 58.0f),
        text,
        TEXT_ALIGN_CENTER,
        TEXT_ALIGN_CENTER
    );
}

static Rectangle BattleResultsCompletionRect(Rectangle base, float x, float y, float width, float height) {
    float scale = base.height / 523.0f;
    return (Rectangle){
        base.x + (x - 191.0f) * scale,
        base.y + (y - 29.0f) * scale,
        width * scale,
        height * scale,
    };
}

static void BattleResultsDrawNodeCompletion(const BattleResultsFlow *flow) {
    const BattleResultsPresentation *p = &flow->presentation;
    float base_height = UiHeight() * 0.90f;
    float base_width = base_height * (756.0f / 523.0f);
    if (base_width > UiWidth() * 0.96f) {
        base_width = UiWidth() * 0.96f;
        base_height = base_width * (523.0f / 756.0f);
    }
    Rectangle base = {
        (UiWidth() - base_width) * 0.5f,
        (UiHeight() - base_height) * 0.5f,
        base_width,
        base_height,
    };
    float scale = base.height / 523.0f;

    UiDrawTexture(GuiTex(GUI_RESULTS_MASTERY_BASE), base, WHITE);
    if (p->mastered) {
        UiDrawTexture(
            GuiTex(GUI_RESULTS_MASTERY_HIGHLIGHT),
            BattleResultsCompletionRect(base, 643.0f, 160.0f, 257.36f, 354.0f),
            WHITE
        );
    } else {
        UiDrawTexture(
            GuiTex(GUI_RESULTS_MASTERY_PROGRESS),
            BattleResultsCompletionRect(base, 428.0f, 160.0f, 292.05f, 354.0f),
            WHITE
        );
    }

    BattleResultsDrawTextInRect(
        GuiFont(FNT_COOL),
        LocGet(p->mastered ? "PVE_QUEST_MASTERED" : "PVE_QUEST_COMPLETED"),
        BattleResultsCompletionRect(base, 380.0f, 95.0f, 378.0f, 53.0f),
        29.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("PVE_UNLOCKED"),
        BattleResultsCompletionRect(base, 270.0f, 462.0f, 152.0f, 37.0f),
        22.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("PVE_ONE_VICTORY"),
        BattleResultsCompletionRect(base, 493.0f, 421.0f, 152.0f, 37.0f),
        22.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );

    char gold[32];
    snprintf(gold, sizeof(gold), "+%d", p->completion_gold);
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        gold,
        BattleResultsCompletionRect(base, 480.0f, 462.0f, 99.0f, 37.0f),
        22.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );

    char wins[16], victories[96];
    snprintf(wins, sizeof(wins), "%d", p->wins_to_master);
    StrReplaceToken(victories, sizeof(victories), LocGet("PVE_VICTORIES"), BATTLE_RESULTS_VICTORIES_TOKEN, wins);
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        victories,
        BattleResultsCompletionRect(base, 717.0f, 421.0f, 152.0f, 37.0f),
        22.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );

    char gems[32] = "?";
    if (p->mastered) snprintf(gems, sizeof(gems), "+%d", p->completion_mastery_gems);
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        gems,
        BattleResultsCompletionRect(base, 697.0f, 462.0f, 99.0f, 37.0f),
        22.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );
    BattleResultsDrawTextInRect(
        GuiFont(FNT_MENU),
        LocGet("TAP_TO_CONTINUE"),
        BattleResultsCompletionRect(base, 191.0f, 535.0f, 756.0f, 42.0f),
        17.0f * scale,
        (Color){230, 230, 230, 255},
        false
    );
}

static int
BattleResultsBuildRewards(const BattleResultsPresentation *p, BattleResultsReward rewards[BATTLE_RESULTS_MAX_REWARDS]) {
    int count = 0;
    if (p->banner_delta != 0) rewards[count++] = (BattleResultsReward){GUI_PVP_BANNER, p->banner_delta, false, true};
    if (p->trophy_delta != 0)
        rewards[count++] = (BattleResultsReward){GUI_TAB_TROPHY, p->trophy_delta, false, p->trophy_delta > 0};
    for (int i = 0; i < p->event_payout.prize_count && i < BATTLE_RESULTS_MAX_PRIZES; i++)
        rewards[count++] = (BattleResultsReward){.cool = true, .prize = PvpPrizeTexture(p->event_payout.prizes[i])};
    if (p->gems > 0) rewards[count++] = (BattleResultsReward){GUI_STORE_CURRENCY_GEMS, p->gems, false, true};
    if (p->xp > 0) rewards[count++] = (BattleResultsReward){GUI_MM_XP, p->xp, true, false};
    if (p->gold > 0) rewards[count++] = (BattleResultsReward){GUI_HUD_GOLD, p->gold, true, false};
    return count;
}

static float BattleResultsRewardAnchor(int index, int count) {
    if (count <= 1) return 0.5f;
    if (count == 2) return index == 0 ? 0.75f : 0.25f;
    return 1.0f - (float)index / (float)(count - 1);
}

static Rectangle BattleResultsRewardItemRect(Rectangle rewards_rect, int index, int count) {
    float scale = BattleResultsPopupScale();
    float fit = count > BATTLE_RESULTS_FULL_SIZE_REWARDS ? (float)BATTLE_RESULTS_FULL_SIZE_REWARDS / count : 1.0f;
    float item_width = 220.0f * scale * fit;
    float item_height = 129.0f * scale * fit;
    float anchor = BattleResultsRewardAnchor(index, count);
    return (Rectangle){
        rewards_rect.x + anchor * (rewards_rect.width - item_width),
        rewards_rect.y + (rewards_rect.height - item_height) * 0.5f,
        item_width,
        item_height,
    };
}

static Rectangle BattleResultsRewardIconRect(Rectangle item) {
    float scale = item.height / 129.0f;
    return (Rectangle){item.x + 116.0f * scale, item.y + 13.0f * scale, 104.0f * scale, 104.0f * scale};
}

static void BattleResultsDrawRewardItem(
    const BattleResultsFlow *flow,
    BattleResultsReward reward,
    Rectangle rewards_rect,
    int index,
    int count,
    float popup_fade
) {
    float local = flow->stage_time - (float)index;
    float alpha = BattleResultsClamp01(local / 0.15f) * popup_fade;
    if (alpha <= 0.0f) return;

    Rectangle item = BattleResultsRewardItemRect(rewards_rect, index, count);
    float move = BattleResultsEaseInCubic(local / 0.75f);
    item.x -= 1000.0f * BattleResultsPopupScale() * (1.0f - move);
    float scale = item.height / 129.0f;
    Rectangle brush = {item.x, item.y + 33.0f * scale, 220.0f * scale, 74.32f * scale};
    Rectangle icon = BattleResultsRewardIconRect(item);

    UiDrawTexture(GuiTex(GUI_REWARD_BRUSH), brush, BattleResultsAlpha(WHITE, alpha));
    if (reward.cool && local >= 1.0f) {
        float effect_time = local - 1.0f;
        float burst_alpha = BattleResultsClamp01(effect_time / 1.0f) * alpha;
        BattleResultsDrawTextureRotated(
            GuiTex(GUI_RESULTS_STARBURST), icon, effect_time * 90.0f, BattleResultsAlpha(WHITE, burst_alpha)
        );
    }

    float brightness = 1.0f;
    if (reward.cool && local >= 1.0f) {
        float effect_time = local - 1.0f;
        brightness = effect_time < 2.0f ? Lerp(1.0f, 0.75f, BattleResultsClamp01(effect_time / 2.0f))
                                        : Lerp(0.75f, 1.0f, BattleResultsClamp01((effect_time - 2.0f) / 2.0f));
    }
    unsigned char channel = (unsigned char)(255.0f * brightness);
    Texture2D texture = reward.prize.id ? reward.prize : GuiTex(reward.icon);
    UiDrawTexture(texture, icon, BattleResultsAlpha((Color){channel, channel, channel, 255}, alpha));

    bool show_quantity = !reward.prize.id && (!reward.animate_quantity || local >= 1.0f);
    if (show_quantity) {
        int shown = reward.amount;
        if (reward.animate_quantity)
            shown = (int)roundf((float)reward.amount * BattleResultsClamp01((local - 1.0f) / 1.0f));
        char amount[32];
        snprintf(amount, sizeof(amount), "%s%d", shown > 0 ? "+" : "", shown);
        BattleResultsDrawTextInRect(
            GuiFont(FNT_COOL),
            amount,
            (Rectangle){item.x + 6.0f * scale, item.y + 32.0f * scale, 110.0f * scale, 65.0f * scale},
            42.0f * scale,
            BattleResultsAlpha(WHITE, alpha),
            false
        );
    }
}

static void
BattleResultsDrawCurrencyParticles(Texture2D texture, Vector2 start, Vector2 end, float elapsed, float popup_fade) {
    for (int pass = 0; pass < 3; pass++) {
        for (int particle = 0; particle < 5; particle++) {
            float local = (elapsed - (float)pass * 0.16f - (float)particle * 0.035f) / 0.8f;
            if (local < 0.0f || local > 1.0f) continue;
            float eased = local * local;
            float wobble = sinf((float)(particle * 7 + pass * 11)) * UiHeight() * 0.05f;
            Vector2 position = {
                Lerp(start.x, end.x, eased),
                Lerp(start.y + wobble, end.y + wobble * 0.15f, eased),
            };
            float sprite_scale = 1.2f + 1.3f * (float)((particle * 3 + pass * 2) % 5) / 4.0f;
            float size = 22.0f * sprite_scale;
            UiDrawTexture(
                texture,
                (Rectangle){position.x - size * 0.5f, position.y - size * 0.5f, size, size},
                BattleResultsAlpha(WHITE, popup_fade * (1.0f - BattleResultsClamp01((local - 0.82f) / 0.18f)))
            );
        }
    }
}

static const char *BattleResultsContinueLabel(const BattleResultsPresentation *p) {
    if (p->pvp) return LocGet("BUTTON_MATCHMAKING_NEXT");
    if (!p->victory) return LocGet("BUTTON_BATTLERESULTS_RETRYQUEST");
    if (p->first_win || p->mastered) return LocGet("BUTTON_BATTLERESULTS_NEXTQUEST");
    if (p->wins_to_master > 0 && p->completions_post > p->wins_to_master)
        return LocGet("BUTTON_BATTLERESULTS_REPEATQUEST");
    return LocGet("BUTTON_BATTLERESULTS_CONTINUEQUEST");
}

static void BattleResultsDrawContinueButton(Rectangle button, const char *label, float fade) {
    bool hot = CheckCollisionPointRec(UiMouse(), button) && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Texture2D texture = GuiTex(hot ? GUI_BTN_GOLD_ON : GUI_BTN_GOLD);
    UiDrawTexture(texture, button, BattleResultsAlpha(WHITE, fade));
    BattleResultsDrawTextInRect(
        GuiFont(FNT_COOL),
        label,
        button,
        18.0f * BattleResultsPopupScale(),
        BattleResultsAlpha((Color){242, 239, 224, 255}, fade),
        true
    );
}

static void BattleResultsDrawLeftButton(GuiId idle, GuiId active, Rectangle rect, float fade, bool interactive) {
    bool hot = interactive && CheckCollisionPointRec(UiMouse(), rect) && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Color tint = interactive ? WHITE : (Color){165, 165, 165, 255};
    UiDrawTexture(GuiTex(hot ? active : idle), rect, BattleResultsAlpha(tint, fade));
}

typedef struct {
    int roster_index;
    int level;
    float xp_ratio;
    bool show_xp;
} BattleResultsRosterEntry;

static BattleResultsRosterEntry BattleResultsRosterEntryFromLeveling(const BattleHeroLeveling *hero) {
    const BattleHeroStats *post = &hero->post;
    return (BattleResultsRosterEntry){
        .roster_index = hero->roster_index,
        .level = post->level,
        .xp_ratio =
            post->maxed ? 1.0f : HeroLevelBarRatio((float)post->band_low, (float)post->band_high, (float)post->xp),
        .show_xp = post->maxed || post->xp > post->band_low,
    };
}

static BattleResultsRosterEntry BattleResultsRosterEntryFromCard(const HeroCard *card, int roster_index) {
    bool maxed = card->max_level > 0 && card->level >= card->max_level;
    return (BattleResultsRosterEntry){
        .roster_index = roster_index,
        .level = card->level,
        .xp_ratio = maxed ? 1.0f : card->xp_frac,
        .show_xp = maxed || card->xp_frac > 0.0f,
    };
}

static void BattleResultsDrawRosterCard(
    const HeroCard *card, const BattleResultsRosterEntry *entry, Rectangle cell, float popup_fade
) {
    Color tint = BattleResultsAlpha(WHITE, popup_fade);
    Rectangle inner = UiRect(cell, 0.5f, 0.5f, 0.9f, 0.9f);
    UiDrawTexture(card->portrait, inner, tint);

    Rectangle data = UiRect(inner, 0.5f, 0.975f, 0.98f, 0.2f);
    float bar_width = data.width * 0.8f;
    Rectangle bar = UiRect(data, 0.75f, 0.6f, bar_width, bar_width / 7.0f);
    UiDrawTexture(GuiTex(GUI_CARD_BAR_BG), (Rectangle){bar.x, bar.y, bar.width * 1.02f, bar.height}, tint);
    if (entry->show_xp) {
        Rectangle fill = {bar.x, bar.y + bar.height * 0.2f, bar.width * entry->xp_ratio, bar.height * 0.6f};
        UiDrawTexture(GuiTex(GUI_CARD_BAR_FILL), fill, tint);
    }

    float badge_size = fminf(data.width, data.height * 1.5f);
    Rectangle badge = UiRect(data, -0.05f, 0.5f, badge_size, badge_size);
    UiDrawTexture(GuiTex(GUI_HUD_XP), badge, tint);
    char level[16];
    snprintf(level, sizeof(level), "%d", entry->level);
    BattleResultsDrawTextInRect(GuiFont(FNT_COOL), level, badge, badge_size * 0.48f, tint, false);
}

static int BattleResultsRosterEntries(
    const BattleResultsPresentation *p,
    const Player *player,
    const Heroes *heroes,
    int squad_index,
    BattleResultsRosterEntry entries[SQUAD_SLOT_COUNT]
) {
    int count = 0;
    if (p->squad.count > 0) {
        for (int i = 0; i < p->squad.count; i++)
            if (p->squad.heroes[i].roster_index < heroes->card_count)
                entries[count++] = BattleResultsRosterEntryFromLeveling(&p->squad.heroes[i]);
        return count;
    }
    if (!player || squad_index < 0 || squad_index >= PLAYER_MAX_SQUADS) return 0;
    for (int giant_pass = 0; giant_pass < 2; giant_pass++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            int roster_index = PlayerSquadUnitAt(player, squad_index, slot);
            if (roster_index < 0 || roster_index >= player->roster_count || roster_index >= heroes->card_count)
                continue;
            if ((int)player->roster[roster_index].giant != giant_pass) continue;
            entries[count++] = BattleResultsRosterEntryFromCard(&heroes->cards[roster_index], roster_index);
        }
    }
    return count;
}

static void BattleResultsDrawRoster(
    Rectangle roster,
    const BattleResultsPresentation *p,
    const Player *player,
    const Heroes *heroes,
    int squad_index,
    float popup_fade
) {
    if (!heroes) return;
    BattleResultsRosterEntry entries[SQUAD_SLOT_COUNT];
    int count = BattleResultsRosterEntries(p, player, heroes, squad_index, entries);
    if (count <= 0) return;

    float item = roster.height;
    float x = roster.x + (roster.width - item * (float)count) * 0.5f;
    for (int i = 0; i < count; i++) {
        BattleResultsDrawRosterCard(
            &heroes->cards[entries[i].roster_index],
            &entries[i],
            (Rectangle){x + item * (float)i, roster.y, item, item},
            popup_fade
        );
    }
}

static void
BattleResultsDrawRewards(const BattleResultsFlow *flow, const Player *player, const Heroes *heroes, int squad_index) {
    const BattleResultsPresentation *p = &flow->presentation;
    float popup_fade = BattleResultsClamp01(flow->stage_time / 0.2f);
    Rectangle background = BattleResultsPopupRect(104.0f, 63.0f, 854.0f, 576.0f);
    Rectangle header = BattleResultsPopupRect(160.0f, 150.0f, 743.0f, 240.84f);
    Rectangle title = BattleResultsPopupRect(104.0f, 63.0f, 854.0f, 58.0f);
    Rectangle result_title = BattleResultsPopupRect(164.0f, 149.0f, 735.0f, 59.0f);
    Rectangle rewards_rect = BattleResultsPopupRect(215.0f, 221.0f, 633.0f, 147.0f);
    Rectangle roster = BattleResultsPopupRect(173.0f, 390.0f, 717.0f, 135.0f);

    UiDrawTexture(GuiTex(GUI_RESULTS_OVERLAY), background, BattleResultsAlpha(WHITE, popup_fade));
    UiDrawTexture(GuiTex(GUI_RESULTS_BG), header, BattleResultsAlpha(WHITE, popup_fade));
    UiDrawTexture(GuiTex(GUI_RESULTS_ROSTER_BG), roster, BattleResultsAlpha(WHITE, popup_fade));

    BattleResultsDrawTextAligned(
        GuiFont(FNT_COOL),
        LocGet("BATTLERESULTS_TITLE"),
        title,
        29.0f * BattleResultsPopupScale(),
        BattleResultsAlpha(WHITE, popup_fade),
        TEXT_ALIGN_CENTER,
        TEXT_ALIGN_END
    );
    BattleResultsDrawTextInRect(
        GuiFont(FNT_COOL),
        LocGet(p->victory ? "BUTTON_BATTLERESULTS_VICTORY" : "BUTTON_BATTLERESULTS_DEFEATED"),
        result_title,
        24.0f * BattleResultsPopupScale(),
        BattleResultsAlpha(p->victory ? WHITE : (Color){205, 53, 43, 255}, popup_fade),
        false
    );

    BattleResultsReward rewards[BATTLE_RESULTS_MAX_REWARDS];
    int count = BattleResultsBuildRewards(p, rewards);
    for (int i = 0; i < count; i++) BattleResultsDrawRewardItem(flow, rewards[i], rewards_rect, i, count, popup_fade);

    int xp_index = -1;
    int gold_index = -1;
    int gem_index = -1;
    for (int i = 0; i < count; i++) {
        if (rewards[i].icon == GUI_MM_XP) xp_index = i;
        if (rewards[i].icon == GUI_HUD_GOLD) gold_index = i;
        if (rewards[i].icon == GUI_STORE_CURRENCY_GEMS) gem_index = i;
    }
    if (xp_index >= 0) {
        float sparkle_time = flow->stage_time - (float)xp_index - 0.75f;
        if (sparkle_time >= 0.0f && sparkle_time <= 1.25f) {
            float alpha = 1.0f - BattleResultsClamp01((sparkle_time - 0.75f) / 0.5f);
            BattleResultsDrawTextureRotated(
                GuiTex(GUI_RESULTS_STARBURST),
                HudXpSparkleRect(player),
                sparkle_time * 180.0f,
                BattleResultsAlpha(WHITE, popup_fade * alpha)
            );
        }
    }

    float release_time = flow->stage_time - (float)count;
    if (release_time >= 0.0f) {
        if (gold_index >= 0) {
            Rectangle item = BattleResultsRewardItemRect(rewards_rect, gold_index, count);
            Rectangle icon = BattleResultsRewardIconRect(item);
            BattleResultsDrawCurrencyParticles(
                GuiTex(GUI_HUD_GOLD),
                (Vector2){icon.x + icon.width * 0.5f, icon.y + icon.height * 0.5f},
                HudCurrencyTarget(false),
                release_time,
                popup_fade
            );
        }
        if (gem_index >= 0) {
            Rectangle item = BattleResultsRewardItemRect(rewards_rect, gem_index, count);
            Rectangle icon = BattleResultsRewardIconRect(item);
            BattleResultsDrawCurrencyParticles(
                GuiTex(GUI_STORE_CURRENCY_GEMS),
                (Vector2){icon.x + icon.width * 0.5f, icon.y + icon.height * 0.5f},
                HudCurrencyTarget(true),
                release_time,
                popup_fade
            );
        }
    }

    BattleResultsDrawRoster(roster, p, player, heroes, squad_index, popup_fade);

    TutorialAnchorSet(TUTORIAL_ANCHOR_RESULTS_STORE, BattleResultsRewardsStoreRect());
    TutorialAnchorSet(TUTORIAL_ANCHOR_RESULTS_CONTINUE, BattleResultsRewardsContinueRect());
    BattleResultsDrawLeftButton(
        GUI_RESULTS_STORE, GUI_RESULTS_STORE_ON, BattleResultsRewardsStoreRect(), popup_fade, true
    );
    BattleResultsDrawLeftButton(
        GUI_RESULTS_HEROES, GUI_RESULTS_HEROES_ON, BattleResultsRewardsHeroesRect(), popup_fade, true
    );
    BattleResultsDrawLeftButton(GUI_RESULTS_MAP, GUI_RESULTS_MAP_ON, BattleResultsRewardsMapRect(), popup_fade, true);
    BattleResultsDrawContinueButton(BattleResultsRewardsContinueRect(), BattleResultsContinueLabel(p), popup_fade);
}

void BattleResultsFlowDraw(const BattleResultsFlow *flow, const Player *player, const Heroes *heroes, int squad_index) {
    switch (flow->stage) {
        case BATTLE_RESULTS_PLAYER_LEVEL:
            BattleResultsDrawLevelUp(flow);
            break;
        case BATTLE_RESULTS_HERO_LEVEL:
            BattleResultsDrawHeroLevel(flow, heroes);
            break;
        case BATTLE_RESULTS_NODE_COMPLETION:
            BattleResultsDrawNodeCompletion(flow);
            break;
        case BATTLE_RESULTS_REWARDS:
            BattleResultsDrawRewards(flow, player, heroes, squad_index);
            break;
        case BATTLE_RESULTS_OFF:
        case BATTLE_RESULTS_DONE:
            break;
    }
}
