#define CONSUME_PANEL_WIDTH ROSTER_PANEL_WIDTH
#define CONSUME_DETAILS_RIGHT_MARGIN 134.0f
#define CONSUME_FLY_START_Y 418.0f
#define CONSUME_FLY_END_Y 148.0f
#define CONSUME_DRAG_THRESHOLD 12.0f
#define CONSUME_DOUBLE_CLICK_TIME 0.35f
#define CONSUME_EASING_TIME 1.5f
#define CONSUME_FLY_MOVE_TIME 0.5f
#define CONSUME_FLY_HOLD_TIME 2.0f
#define CONSUME_FLY_FADE_TIME 0.375f
#define CONSUME_MAX_FLY_TEXTS 16
#define CONSUME_FLY_TEXT_CAP 96
#define CONSUME_WINDOW_W 873.0f
#define CONSUME_WINDOW_H 610.0f
#define CONSUME_WINDOW_PREVIEW_X 764.0f
#define CONSUME_WINDOW_PREVIEW_Y 226.0f
#define CONSUME_WINDOW_RESULTS_X 1076.0f
#define CONSUME_WINDOW_RESULTS_Y 190.0f
#define CONSUME_LIST_X 84.0f
#define CONSUME_LIST_Y 170.0f
#define CONSUME_LIST_W 725.0f
#define CONSUME_LIST_H 397.0f
#define CONSUME_LIST_ROW_H 99.0f
#define CONSUME_LADDER_SCROLL_RATE 300.0f
#define CONSUME_LEVEL_BEAM_FX "fx_chartraining_lvlup_pre"
#define CONSUME_MESSAGE_CAP 256
#define CONSUME_MULTI_POPUP_W 0.6f
#define CONSUME_MULTI_POPUP_H 0.6f
#define CONSUME_MULTI_POPUP_ASPECT 1.56f
#define CONSUME_LEVEL_SOUND_COUNT 4
#define CONSUME_RARITY_EPIC_WARNING RARITY_EPIC

#define CONSUME_GEAR_WARNING "Снаряжение этого героя будет уничтожено. Вы уверены?"

typedef enum {
    CONSUME_LEAVE_NONE,
    CONSUME_LEAVE_SQUAD,
    CONSUME_LEAVE_HOME,
    CONSUME_LEAVE_STORE,
    CONSUME_LEAVE_RUNES,
    CONSUME_LEAVE_GALLERY,
    CONSUME_LEAVE_QUESTS
} ConsumeLeave;

typedef enum {
    CONSUME_DIALOG_NONE,
    CONSUME_DIALOG_MESSAGE,
    CONSUME_DIALOG_RARITY_WARNING,
    CONSUME_DIALOG_MULTI_TYPE,
    CONSUME_DIALOG_MULTI_CONFIRM
} ConsumeDialog;

typedef enum {
    CONSUME_MULTI_NONE,
    CONSUME_MULTI_IDENTICAL,
    CONSUME_MULTI_RARITY,
    CONSUME_MULTI_FACTION
} ConsumeMultiType;

typedef enum { CONSUME_ATTACK_NONE, CONSUME_ATTACK_LEVEL, CONSUME_ATTACK_TIER } ConsumeAttackClip;

typedef struct {
    char label[CONSUME_FLY_TEXT_CAP];
    char amount[16];
    Texture2D icon;
} ConsumeFlyText;

typedef struct {
    Roster roster;

    int target;
    int seeds[CONSUME_MAX_SEEDS];
    int seed_count;
    int multi_seeds[PLAYER_ROSTER_CAPACITY];
    int multi_count;
    int multi_counts[CONSUME_MAX_SEEDS];
    bool multi_ladder;
    ConsumeMultiType multi_type;

    int press_index;
    int press_spot;
    Vector2 press_pos;
    bool dragging;
    int last_click_index;
    double last_click_time;

    int details_index;
    int pending_details;
    bool details_armed;

    ConsumeDialog dialog;
    char message[CONSUME_MESSAGE_CAP];
    int pending_seed;
    bool warning_disable;
    bool warnings_disabled[RARITY_MYTHIC + 1];

    bool board_dirty;
    int placed_spot;
    bool ladder_open;
    float ladder_scroll;
    bool ladder_user_scrolled;
    bool ladder_snap;

    float shown_consume_frac, shown_xp_frac;
    int shown_consume_level, shown_level;

    ConsumeFlyText fly[CONSUME_MAX_FLY_TEXTS];
    int fly_count;
    int fly_index;
    float fly_time;

    bool results;
    Character before;
    int results_level;
    Tier results_tier;
    float results_time;
    float results_step_time;
    bool results_settled;
    const char *fx_request;
    int fx_spot;
    ConsumeAttackClip attack_request;
    bool model_request;

    ConsumeLeave leave_done;

    Button consume, multiple, cancel, store, view_all, info, ladder_close;
    Button again, heroes, pve, pvp;
    Button dialog_yes, dialog_no, dialog_ok, dialog_identical, dialog_rarity, dialog_faction;
} ConsumeScreen;

void ConsumeInitialize(ConsumeScreen *c, const Player *p) {
    memset(c, 0, sizeof(*c));
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    float scale = 0.33f, small = 0.24f;
    c->target = -1;
    RosterInitialize(&c->roster, p);
    c->consume = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("CONSUME_BUTTON"), font, anchor, scale);
    c->multiple = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "Поглотить несколько", font, anchor, 0.2f);
    c->cancel = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("COMBINE_BUTTON_CANCEL"), font, anchor, scale);
    c->store = ButtonLoad(GUI_CONSUME_SHOP, GUI_CONSUME_SHOP_ON);
    c->info = ButtonLoad(GUI_CONSUME_INFO, GUI_CONSUME_INFO_ON);
    c->ladder_close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    c->view_all = ButtonLoadText(
        GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, LocGet("CONSUME_UI_PROGRESS_BUTTON"), font, anchor, 0.3f
    );
    c->again = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("CONSUME_AGAIN_BUTTON"), font, anchor, small);
    c->heroes = ButtonLoad(GUI_CONSUME_RESULT_HEROES, GUI_CONSUME_RESULT_HEROES_ON);
    c->pve = ButtonLoad(GUI_CONSUME_RESULT_PVE, GUI_CONSUME_RESULT_PVE_ON);
    c->pvp = ButtonLoad(GUI_CONSUME_RESULT_PVP, GUI_CONSUME_RESULT_PVP_ON);
    c->dialog_yes = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("CONFIRM_DIALOG_YES"), font, anchor, scale);
    c->dialog_no = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("CONFIRM_DIALOG_NO"), font, anchor, scale);
    c->dialog_ok = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("RUNE_SINGLE_CONFIRMATION_OK_BUTTON"), font, anchor, scale
    );
    c->dialog_identical = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "Одинаковые", font, anchor, 0.27f);
    c->dialog_rarity = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "Та же редкость", font, anchor, 0.27f);
    c->dialog_faction = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "Та же фракция", font, anchor, 0.27f);
}

static Rectangle ConsumePxSquare(float x, float y, float size) {
    Rectangle box = RosterPx(x, y, size, size);
    float side = fminf(box.width, box.height);
    return (Rectangle){box.x + (box.width - side) * 0.5f, box.y + (box.height - side) * 0.5f, side, side};
}

static float ConsumeFontScale(void) {
    return (UiHeight() - ROSTER_PREFAB_TOP) / ROSTER_PREFAB_HEIGHT;
}

void ConsumeRequestLeave(ConsumeScreen *c, ConsumeLeave leave) {
    if (c->dialog != CONSUME_DIALOG_NONE) return;
    c->leave_done = leave;
}

static void ConsumeClearFly(ConsumeScreen *c) {
    c->fly_count = 0;
    c->fly_index = 0;
    c->fly_time = 0.0f;
}

static void ConsumeResetPress(ConsumeScreen *c) {
    c->press_index = -1;
    c->press_spot = -1;
    c->dragging = false;
}

static void ConsumeClearBoard(ConsumeScreen *c) {
    c->target = -1;
    c->seed_count = 0;
    c->multi_type = CONSUME_MULTI_NONE;
    c->multi_count = 0;
    c->multi_ladder = false;
    c->ladder_open = false;
    c->board_dirty = true;
    c->results = false;
    c->fx_request = NULL;
    ConsumeClearFly(c);
}

void ConsumeOpen(ConsumeScreen *c, const Player *p) {
    ConsumeClearBoard(c);
    ConsumeResetPress(c);
    c->roster.scroll = 0.0f;
    c->dialog = CONSUME_DIALOG_NONE;
    c->details_index = -1;
    c->pending_details = -1;
    c->placed_spot = -1;
    c->last_click_index = -1;
    c->leave_done = CONSUME_LEAVE_NONE;
    RosterRefreshOrder(&c->roster, p);
}

static bool ConsumeIsPlaced(const ConsumeScreen *c, int index) {
    if (index == c->target) return true;
    for (int i = 0; i < c->seed_count; i++)
        if (c->seeds[i] == index) return true;
    return false;
}

static int ConsumeActiveSeeds(const ConsumeScreen *c, const Player *p, const Character **out) {
    const int *indices = c->multi_type != CONSUME_MULTI_NONE ? c->multi_seeds : c->seeds;
    int count = c->multi_type != CONSUME_MULTI_NONE ? c->multi_count : c->seed_count;
    for (int i = 0; i < count; i++) out[i] = &p->roster[indices[i]];
    return count;
}

static ConsumeOutcome ConsumePreview(const ConsumeScreen *c, const Player *p) {
    const Character **seeds = (const Character **)MemAlloc(PLAYER_ROSTER_CAPACITY * sizeof(*seeds));
    int count = ConsumeActiveSeeds(c, p, seeds);
    ConsumeOutcome outcome = ConsumeCompute(&p->roster[c->target], seeds, count);
    MemFree(seeds);
    return outcome;
}

static void ConsumeSyncShownBars(ConsumeScreen *c, const Character *target) {
    const ConsumeProfile *profile = ConsumeProfileFor(target);
    c->shown_consume_level = target->consume_level;
    c->shown_level = target->level;
    c->shown_consume_frac = ConsumeBandFraction(profile->consume_table, target->consume_level, target->consume_xp);
    c->shown_xp_frac = ConsumeBandFraction(profile->level_table, target->level, target->xp);
}

static void ConsumeShowMessage(ConsumeScreen *c, const char *text) {
    StrCopy(c->message, sizeof(c->message), text);
    c->dialog = CONSUME_DIALOG_MESSAGE;
    GuiSoundPlay(SND_STORE_ERROR);
}

static void ConsumeQueueFly(ConsumeScreen *c, const char *label, const char *amount, Texture2D icon) {
    if (c->fly_count >= CONSUME_MAX_FLY_TEXTS) return;
    ConsumeFlyText *fly = &c->fly[c->fly_count++];
    StrCopy(fly->label, sizeof(fly->label), label);
    StrCopy(fly->amount, sizeof(fly->amount), amount);
    fly->icon = icon;
}

static void ConsumeQueueSeedFly(ConsumeScreen *c, const Character *target, const Character *seed) {
    ConsumeSeedPreview preview = ConsumePreviewSeed(target, seed);
    char amount[16];
    snprintf(amount, sizeof(amount), "+%d", preview.xp_gain);
    ConsumeQueueFly(c, LocGet("CONSUME_FLY_TEXT_XP"), amount, GuiTex(GUI_CONSUME_ICON_XP));
    if (preview.tier_bonus_pct > 0) {
        snprintf(amount, sizeof(amount), "+%d%%", preview.tier_bonus_pct);
        ConsumeQueueFly(c, LocGet("CONSUME_FLY_TEXT_TIER_BONUS"), amount, GuiTex(GUI_CONSUME_ICON_TIER));
    }
    if (preview.shares_faction) {
        snprintf(amount, sizeof(amount), "+%d%%", preview.faction_bonus_pct);
        Texture2D coin = GuiTex(
            target->giant || target->faction == FACTION_ALL ? GUI_COIN_LARGE : GUI_COIN_LARGE + (int)target->faction
        );
        ConsumeQueueFly(c, LocGet("CONSUME_FLY_TEXT_FACTION_BONUS"), amount, coin);
    }
    if (preview.combine_bonus_pct > 0) {
        snprintf(amount, sizeof(amount), "+%d%%", preview.combine_bonus_pct);
        ConsumeQueueFly(c, LocGet("CONSUME_FLY_TEXT_COMBINE_BONUS"), amount, GuiTex(GUI_CONSUME_ICON_COMBINE));
    }
}

static void ConsumeSeedAdd(ConsumeScreen *c, const Player *p, int index) {
    c->seeds[c->seed_count++] = index;
    c->placed_spot = c->seed_count;
    c->board_dirty = true;
    ConsumeQueueSeedFly(c, &p->roster[c->target], &p->roster[index]);
}

static bool ConsumeRarityNeedsWarning(const ConsumeScreen *c, const Character *ch) {
    return ch->rarity >= CONSUME_RARITY_EPIC_WARNING && !c->warnings_disabled[ch->rarity];
}

static bool ConsumeSeedNeedsWarning(const ConsumeScreen *c, const Character *ch) {
    return GearHasEquipped(ch) || ConsumeRarityNeedsWarning(c, ch);
}

static const char *ConsumeSeedWarning(const Character *ch) {
    if (GearHasEquipped(ch)) return CONSUME_GEAR_WARNING;
    return LocGet(
        ch->rarity == RARITY_EPIC ? "CONSUME_CONFIRMATION_EPIC_WARNING" : "CONSUME_CONFIRMATION_LEGENDARY_WARNING"
    );
}

static void ConsumePlaceUnit(ConsumeScreen *c, HeroesBackground *scene, const Player *p, int index) {
    if (ConsumeIsPlaced(c, index)) return;
    const Character *ch = &p->roster[index];
    if (c->target < 0) {
        if (ConsumeCharacterMaxed(ch)) {
            HeroesBackgroundShake(scene);
            return;
        }
        c->target = index;
        c->placed_spot = 0;
        c->board_dirty = true;
        ConsumeSyncShownBars(c, ch);
        HeroesBackgroundPlayPlace(scene);
        return;
    }
    if (c->seed_count >= CONSUME_MAX_SEEDS) {
        HeroesBackgroundShake(scene);
        return;
    }
    if (PlayerSquadsContain(p, index)) {
        ConsumeShowMessage(c, LocGet("CONSUME_CONFIRMATION_SQUAD_WARNING"));
        return;
    }
    if (ConsumeSeedNeedsWarning(c, ch)) {
        c->pending_seed = index;
        c->warning_disable = false;
        c->dialog = CONSUME_DIALOG_RARITY_WARNING;
        GuiSoundPlay(SND_STORE_ERROR);
        return;
    }
    ConsumeSeedAdd(c, p, index);
    HeroesBackgroundPlayPlace(scene);
}

static void ConsumeRemoveSpot(ConsumeScreen *c, int spot) {
    if (spot == 0) {
        ConsumeClearBoard(c);
        return;
    }
    int seed = spot - 1;
    if (seed >= c->seed_count) return;
    for (int i = seed; i + 1 < c->seed_count; i++) c->seeds[i] = c->seeds[i + 1];
    c->seed_count--;
    c->multi_type = CONSUME_MULTI_NONE;
    c->board_dirty = true;
    ConsumeClearFly(c);
    GuiSoundPlay(SND_RUNE_PICKUP);
}

static bool ConsumeMultiMatches(ConsumeMultiType type, const Character *unit, const Character *seed) {
    switch (type) {
        case CONSUME_MULTI_IDENTICAL:
            return !strcmp(unit->id, seed->id) && unit->tier == seed->tier;
        case CONSUME_MULTI_RARITY:
            return unit->rarity == seed->rarity;
        case CONSUME_MULTI_FACTION:
            return unit->rarity == seed->rarity && unit->faction == seed->faction;
        default:
            return false;
    }
}

static void ConsumeMultiSelect(ConsumeScreen *c, const Player *p, ConsumeMultiType type) {
    c->multi_type = type;
    c->multi_count = 0;
    memset(c->multi_counts, 0, sizeof(c->multi_counts));
    for (int i = 0; i < p->roster_count; i++) {
        if (!PlayerConsumeSeedAllowed(p, c->target, i)) continue;
        if (GearHasEquipped(&p->roster[i]) && !ConsumeIsPlaced(c, i)) continue;
        for (int s = 0; s < c->seed_count; s++) {
            if (!ConsumeMultiMatches(type, &p->roster[i], &p->roster[c->seeds[s]])) continue;
            c->multi_seeds[c->multi_count++] = i;
            c->multi_counts[s]++;
            break;
        }
    }
    c->dialog = CONSUME_DIALOG_MULTI_CONFIRM;
}

static const char *ConsumeLadderLabel(const ConsumeProfile *profile, int level, char *out, int cap, Texture2D *icon) {
    ConsumeUnlock kind = ConsumeUnlockKindAt(profile, level);
    int value = ConsumeUnlockValueAt(profile, level, kind);
    char number[16];
    snprintf(number, sizeof(number), "%d", value);
    *icon = (Texture2D){0};
    if (kind == CONSUME_UNLOCK_CRIT) {
        *icon = GuiTex(GUI_DETAILS_CRIT);
        LocFormat2(out, (size_t)cap, "CONSUME_UNLOCK_CRITICAL_HIT_SHORT", number, "");
    } else if (kind == CONSUME_UNLOCK_PROMOTION) {
        *icon = GuiTex(GUI_CONSUME_TIER_UP);
        LocFormat2(
            out,
            (size_t)cap,
            "CONSUME_UNLOCK_TIER_SHORT",
            TIER_NAMES[(int)Clamp((float)(value - 1), 0.0f, (float)(TIER_COUNT - 1))],
            ""
        );
    } else if (kind == CONSUME_UNLOCK_FORTIFY) {
        bool health = ConsumeFortifySlotIsHealth(profile, value);
        *icon = GuiTex(health ? GUI_DETAILS_HEALTH : GUI_CONSUME_POWER_SLOT);
        StrCopy(out, cap, LocGet(health ? "CONSUME_UNLOCK_HEALTH_SHORT" : "CONSUME_UNLOCK_POWER_SHORT"));
    } else {
        StrCopy(out, cap, LocGet("CONSUME_PROGRESS_BAR_MAXED"));
    }
    return out;
}

static void ConsumeDrawBarFill(GuiId fill_id, Rectangle bar, float frac) {
    if (frac <= 0.0f) return;
    Texture2D fill = GuiTex(fill_id);
    Rectangle src = {0, 0, (float)fill.width, (float)fill.height};
    DrawTexturePro(fill, src, (Rectangle){bar.x, bar.y, bar.width * frac, bar.height}, (Vector2){0, 0}, 0.0f, WHITE);
}

static void
ConsumeDrawBar(Rectangle frame, Rectangle bar, GuiId old_fill, GuiId new_fill, float base_frac, float gain_frac) {
    UiDrawTexture(GuiTex(GUI_CONSUME_METER), frame, WHITE);
    ConsumeDrawBarFill(new_fill, bar, gain_frac);
    ConsumeDrawBarFill(old_fill, bar, base_frac);
}

typedef struct {
    const ConsumeScreen *screen;
    const Player *player;
} ConsumeCardContext;

static void ConsumeDrawRosterCard(const HeroCard *card, Rectangle cell, int roster_index, const void *context) {
    const ConsumeCardContext *c = (const ConsumeCardContext *)context;
    const Character *ch = card->ch;
    const ConsumeProfile *profile = ConsumeProfileFor(ch);
    RosterCardState state = {
        .maxed = ch->consume_level >= profile->consume_table->count,
        .grey_bar = true,
        .in_squad = PlayerSquadsContain(c->player, roster_index),
        .in_use = ConsumeIsPlaced(c->screen, roster_index),
    };
    if (!state.maxed) {
        state.progress = ConsumeBandFraction(profile->consume_table, ch->consume_level, ch->consume_xp);
        char label[64];
        ConsumeLadderLabel(profile, ch->consume_level, label, sizeof(label), &state.unlock_icon);
    }
    RosterDrawCard(card, cell, &state);
}

static void ConsumeDrawRoster(ConsumeScreen *c, const Heroes *h, const Player *p, bool input) {
    ConsumeCardContext context = {c, p};
    int hovered = RosterDraw(&c->roster, h->cards, p, input, ConsumeDrawRosterCard, &context);
    if (hovered >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        c->press_index = hovered;
        c->press_spot = -1;
        c->press_pos = UiMouse();
        c->dragging = false;
    }

    if (ButtonDrawEx(&c->store, RosterPx(13.0f, 828.0f, 81.0f, 61.0f), input))
        ConsumeRequestLeave(c, CONSUME_LEAVE_STORE);
    UiDrawTextLeftMiddle(
        GuiFont(FNT_MENU),
        LocGet("TAP_PORTRAIT_FOR_INFO"),
        RosterPx(105.0f, 831.0f, 287.0f, 66.0f),
        15.0f,
        (Color){220, 216, 204, 255}
    );
}

static void ConsumeHandleClick(ConsumeScreen *c, HeroesBackground *scene, const Player *p, int index) {
    double now = GetTime();
    bool second = c->last_click_index == index && now - c->last_click_time <= CONSUME_DOUBLE_CLICK_TIME;
    c->last_click_index = second ? -1 : index;
    c->last_click_time = now;
    if (second) {
        c->pending_details = -1;
        ConsumePlaceUnit(c, scene, p, index);
        return;
    }
    c->pending_details = index;
    GuiClickSoundPlayDuplicate();
}

static void ConsumeOpenPendingDetails(ConsumeScreen *c) {
    if (c->pending_details < 0 || GetTime() - c->last_click_time <= CONSUME_DOUBLE_CLICK_TIME) return;
    c->details_index = c->pending_details;
    c->details_armed = false;
    c->pending_details = -1;
}

static void ConsumeUpdateDrag(ConsumeScreen *c, HeroesBackground *scene, const Player *p, bool input) {
    Vector2 mouse = UiMouse();
    if (input && c->press_index < 0 && mouse.x > CONSUME_PANEL_WIDTH && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int spot = HeroesBackgroundConsumeSpotAtMouse(scene);
        int index = spot == 0 ? c->target : spot > 0 && spot <= c->seed_count ? c->seeds[spot - 1] : -1;
        if (index >= 0) {
            c->press_index = index;
            c->press_spot = spot;
            c->press_pos = mouse;
            c->dragging = false;
        }
    }
    if (c->press_index < 0) return;
    if (!c->dragging && Vector2Distance(mouse, c->press_pos) >= CONSUME_DRAG_THRESHOLD) {
        c->dragging = true;
        c->details_index = -1;
        HeroesBackgroundConsumeDragStart(scene, &p->roster[c->press_index], c->press_index, c->press_spot);
    }
    bool over_tray = mouse.x < CONSUME_PANEL_WIDTH;
    if (c->dragging) HeroesBackgroundConsumeDragMove(scene, over_tray);
    if (!input || !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;

    int index = c->press_index, spot = c->press_spot;
    bool dragged = c->dragging;
    ConsumeResetPress(c);
    if (dragged) HeroesBackgroundConsumeDragEnd(scene);
    if (spot >= 0) {
        if (dragged && over_tray)
            ConsumeRemoveSpot(c, spot);
        else if (dragged)
            HeroesBackgroundPlayPlace(scene);
    } else if (!dragged) {
        ConsumeHandleClick(c, scene, p, index);
    } else if (!over_tray) {
        ConsumePlaceUnit(c, scene, p, index);
    }
}

static void ConsumeDrawDragged(const ConsumeScreen *c, const Heroes *h) {
    Vector2 m = UiMouse();
    if (!c->dragging || c->press_index < 0 || m.x >= CONSUME_PANEL_WIDTH) return;
    float size = 82.0f * 0.85f * 1.9f;
    UiDrawTexture(
        h->cards[c->press_index].portrait, (Rectangle){m.x - size * 0.5f, m.y - size * 0.5f, size, size}, WHITE
    );
}

static void ConsumeDrawCallToAction(ConsumeScreen *c, bool input) {
    UiDrawTexture(GuiTex(GUI_COMBAT_PREBATTLE_BG), RosterPx(734.0f, 25.0f, 814.0f, 92.0f), WHITE);
    Rectangle text_box = RosterPx(780.0f, 35.0f, 610.0f, 71.0f);
    const char *text = LocGet(c->target < 0 ? "CONSUME_CTA_TARGET" : "CONSUME_CTA_SEEDS");
    Font font = GuiFont(FNT_MENU);
    float size = 19.0f, width = MeasureTextEx(font, text, size, 1.0f).x;
    if (width > text_box.width) size *= text_box.width / width;
    UiDrawTextCentered(font, text, text_box, size, WHITE);
    if (ButtonDrawEx(&c->info, ConsumePxSquare(1393.0f, 24.0f, 90.0f), input)) {
        ConsumeShowMessage(c, LocGet("CONSUME_FTUE_CONSUME_HOW_TO"));
    }
}

static void ConsumeDrawProgressPanel(
    ConsumeScreen *c,
    const Character *shown,
    const ConsumeProfile *profile,
    int consume_level,
    int level,
    float consume_frac,
    float gain_frac,
    float xp_frac,
    float xp_gain_frac,
    bool input
) {
    Font title = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    float fs = ConsumeFontScale();
    UiDrawTexture(GuiTex(GUI_CONSUME_HEADER), RosterPx(691.0f, 29.0f, 924.0f, 187.0f), WHITE);

    char key[96];
    snprintf(key, sizeof(key), "%s%s", shown->id, TIER_NAMES[shown->tier]);
    const char *name = LocGet(key);
    if (!name[0]) name = shown->name_ru[0] ? shown->name_ru : shown->name;
    UiDrawTextCentered(title, name, RosterPx(785.0f, 18.0f, 700.0f, 62.0f), 32.0f * fs, WHITE);

    UiDrawTexture(GuiTex(GUI_CONSUME_ICON_XP), ConsumePxSquare(798.0f, 56.0f, 70.0f), WHITE);
    char number[16];
    snprintf(number, sizeof(number), "%d", level);
    UiDrawTextCentered(title, number, RosterPx(858.0f, 56.0f, 72.0f, 70.0f), 36.0f * fs, WHITE);
    ConsumeDrawBar(
        RosterPx(929.0f, 70.0f, 448.0f, 46.0f),
        RosterPx(936.0f, 73.0f, 434.0f, 38.0f),
        GUI_HUD_XP_METER,
        GUI_CONSUME_BAR_XP_NEW,
        xp_frac,
        xp_gain_frac
    );
    if (level > c->shown_level)
        UiDrawTextCentered(
            body,
            LocGet("CONSUME_PROGRESS_BAR_INCREASED"),
            RosterPx(936.0f, 73.0f, 434.0f, 38.0f),
            26.0f * fs,
            (Color){120, 255, 120, 255}
        );

    UiDrawTextLeftMiddle(
        body,
        LocGet("CONSUME_UI_CONSUME_XP_TITLE"),
        RosterPx(778.0f, 138.0f, 146.0f, 51.0f),
        24.0f * fs,
        (Color){220, 216, 204, 255}
    );
    ConsumeDrawBar(
        RosterPx(928.0f, 145.0f, 383.0f, 46.0f),
        RosterPx(933.0f, 148.0f, 376.0f, 39.0f),
        GUI_CONSUME_BAR_OLD,
        GUI_CONSUME_BAR_NEW,
        consume_frac,
        gain_frac
    );

    char unlock[64];
    Texture2D icon;
    ConsumeLadderLabel(profile, consume_level, unlock, sizeof(unlock), &icon);
    UiDrawTextCentered(body, unlock, RosterPx(934.0f, 148.0f, 371.0f, 40.0f), 22.0f * fs, WHITE);
    if (icon.id) UiDrawTexture(icon, ConsumePxSquare(1289.0f, 126.0f, 75.0f), WHITE);
    if (ButtonDrawEx(&c->view_all, RosterPx(1372.0f, 135.0f, 170.0f, 62.0f), input)) {
        c->ladder_open = !c->ladder_open;
        c->ladder_user_scrolled = false;
        GuiSoundPlay(SND_UI_TAB);
    }
}

static void ConsumeDrawPreview(ConsumeScreen *c, const Player *p, bool input, float dt) {
    const Character *target = &p->roster[c->target];
    const ConsumeProfile *profile = ConsumeProfileFor(target);
    ConsumeOutcome outcome = ConsumePreview(c, p);
    int consume_level = outcome.consume_level;
    float base_consume = consume_level == target->consume_level
                             ? ConsumeBandFraction(profile->consume_table, target->consume_level, target->consume_xp)
                             : 0.0f;
    float base_xp =
        outcome.level == target->level ? ConsumeBandFraction(profile->level_table, target->level, target->xp) : 0.0f;
    float goal_consume = ConsumeBandFraction(profile->consume_table, consume_level, outcome.consume_xp);
    float goal_xp = ConsumeBandFraction(profile->level_table, outcome.level, outcome.xp);
    float blend = 1.0f - expf(-dt * 4.0f / CONSUME_EASING_TIME);
    if (c->shown_consume_level != consume_level) c->shown_consume_frac = base_consume;
    c->shown_consume_level = consume_level;
    c->shown_consume_frac = Lerp(c->shown_consume_frac, goal_consume, blend);
    c->shown_xp_frac = Lerp(c->shown_xp_frac, goal_xp, blend);

    Character shown = *target;
    shown.tier = outcome.tier;
    c->shown_level = target->level;
    ConsumeDrawProgressPanel(
        c,
        &shown,
        profile,
        consume_level,
        outcome.level,
        base_consume,
        c->shown_consume_frac,
        base_xp,
        c->shown_xp_frac,
        input
    );
}

static void ConsumeDrawFlyText(ConsumeScreen *c, float dt) {
    if (c->fly_index >= c->fly_count) return;
    const ConsumeFlyText *fly = &c->fly[c->fly_index];
    float t = c->fly_time;
    float rise = fminf(t / CONSUME_FLY_MOVE_TIME, 1.0f);
    float fade_start = CONSUME_FLY_MOVE_TIME + CONSUME_FLY_HOLD_TIME;
    float alpha = t < CONSUME_FLY_MOVE_TIME ? rise : 1.0f - Clamp((t - fade_start) / CONSUME_FLY_FADE_TIME, 0.0f, 1.0f);
    float y = Lerp(CONSUME_FLY_START_Y, CONSUME_FLY_END_Y, 1.0f - (1.0f - rise) * (1.0f - rise));
    Color tint = Fade(WHITE, alpha);
    Font font = GuiFont(FNT_COOL);
    float fs = ConsumeFontScale();
    UiDrawTexture(GuiTex(GUI_CONSUME_FLY), RosterPx(978.0f, y - 3.0f, 526.0f, 269.0f), tint);
    if (fly->icon.id) UiDrawTexture(fly->icon, ConsumePxSquare(1331.0f, y + 96.0f, 108.0f), tint);
    UiDrawTextCentered(font, fly->label, RosterPx(1040.0f, y + 100.0f, 290.0f, 56.0f), 26.0f * fs, tint);
    UiDrawTextCentered(
        font,
        fly->amount,
        RosterPx(1040.0f, y + 140.0f, 290.0f, 56.0f),
        34.0f * fs,
        Fade((Color){120, 255, 120, 255}, alpha)
    );

    c->fly_time += dt;
    if (c->fly_time >= fade_start + CONSUME_FLY_FADE_TIME) {
        c->fly_time = 0.0f;
        c->fly_index++;
    }
}

static void ConsumeDrawLadderRow(const ConsumeProfile *profile, int level, Rectangle row, int base_xp, int shown_xp) {
    Font body = GuiFont(FNT_MENU);
    float fs = ConsumeFontScale();
    UiDrawTexture(GuiTex(GUI_RESULTS_ROSTER_BG), (Rectangle){row.x, row.y, row.width, row.height * 0.9f}, WHITE);
    char label[64], number[32];
    Texture2D icon;
    ConsumeLadderLabel(profile, level, label, sizeof(label), &icon);
    float icon_size = row.height * 0.62f;
    Rectangle icon_box = {row.x + row.width * 0.02f, row.y + row.height * 0.14f, icon_size, icon_size};
    if (icon.id) UiDrawTexture(icon, icon_box, WHITE);
    Rectangle name = {icon_box.x + icon_size + row.width * 0.03f, row.y, row.width * 0.572f, row.height * 0.9f};
    UiDrawTextLeftMiddle(body, label, name, 30.0f * fs, WHITE);
    int threshold = ConsumeXpForLevel(profile->consume_table, level + 1);
    if (base_xp >= threshold) {
        float check = row.height * 0.74f;
        UiDrawTexture(
            GuiTex(GUI_CONSUME_IN_USE),
            (Rectangle){row.x + row.width * 0.96f - check, row.y + row.height * 0.08f, check, check},
            WHITE
        );
        return;
    }
    snprintf(number, sizeof(number), "%d", threshold);
    Color tone = shown_xp >= threshold ? (Color){0, 255, 0, 255} : WHITE;
    UiDrawTextRight(
        body, number, (Rectangle){0.0f, row.y + row.height * 0.45f, row.x + row.width * 0.97f, 0.0f}, 30.0f * fs, tone
    );
}

static void ConsumeDrawLadder(ConsumeScreen *c, const Character *ch, int base_xp, int shown_xp, bool input, float dt) {
    const ConsumeProfile *profile = ConsumeProfileFor(ch);
    Font title = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    float fs = ConsumeFontScale();
    float wx = c->results ? CONSUME_WINDOW_RESULTS_X : CONSUME_WINDOW_PREVIEW_X;
    float wy = c->results ? CONSUME_WINDOW_RESULTS_Y : CONSUME_WINDOW_PREVIEW_Y;
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), RosterPx(wx, wy, CONSUME_WINDOW_W, CONSUME_WINDOW_H), WHITE);
    UiDrawTextCentered(
        title, LocGet("CONSUME_PROGRESS_UI_TITLE"), RosterPx(wx + 130.0f, wy + 48.0f, 612.0f, 66.0f), 34.0f * fs, WHITE
    );

    char number[32], amount[96], extra[32] = "";
    snprintf(number, sizeof(number), "%d", base_xp);
    if (shown_xp > base_xp) {
        snprintf(extra, sizeof(extra), "(+%d)", shown_xp - base_xp);
        LocFormat2(amount, sizeof(amount), "CONSUME_PROGRESS_UI_XP_VALUE_NEW", number, extra);
    } else {
        LocFormat2(amount, sizeof(amount), "CONSUME_PROGRESS_UI_XP_VALUE", number, "");
    }
    Rectangle amount_box = RosterPx(wx + 200.0f, wy + 110.0f, 473.0f, 60.0f);
    float amount_size = 30.0f * fs;
    float amount_w = MeasureTextEx(body, TextFormat("%s %s", number, extra), amount_size, 1.0f).x + 90.0f * fs;
    HeroesDrawRich(
        body,
        amount,
        (Rectangle){
            amount_box.x + (amount_box.width - amount_w) * 0.5f, amount_box.y, amount_w + 40.0f, amount_box.height
        },
        amount_size,
        WHITE
    );

    Rectangle list = RosterPx(wx + CONSUME_LIST_X, wy + CONSUME_LIST_Y, CONSUME_LIST_W, CONSUME_LIST_H);
    float row_h = RosterPx(0.0f, 0.0f, 0.0f, CONSUME_LIST_ROW_H).height;
    int rows = 0, next_row = -1;
    for (int level = 1; level < profile->progression->count; level++) {
        if (next_row < 0 && ConsumeXpForLevel(profile->consume_table, level + 1) > base_xp) next_row = rows;
        rows++;
    }
    float max_scroll = fmaxf(rows * row_h - list.height, 0.0f);
    bool over = input && CheckCollisionPointRec(UiMouse(), list);
    float wheel = over ? GetMouseWheelMove() : 0.0f;
    if (wheel != 0.0f) {
        c->ladder_user_scrolled = true;
        c->ladder_scroll -= wheel * ROSTER_SCROLL_STEP;
    } else if (!c->ladder_user_scrolled && next_row >= 0) {
        float goal = next_row * row_h - list.height * 0.5f;
        float step = c->ladder_snap ? fabsf(goal - c->ladder_scroll) : CONSUME_LADDER_SCROLL_RATE * dt;
        c->ladder_scroll += Clamp(goal - c->ladder_scroll, -step, step);
        c->ladder_snap = false;
    }
    c->ladder_scroll = Clamp(c->ladder_scroll, 0.0f, max_scroll);

    float ui = UiScale();
    BeginScissorMode((int)(list.x * ui), (int)(list.y * ui), (int)(list.width * ui), (int)(list.height * ui));
    for (int row = 0; row < rows; row++) {
        float y = list.y + row * row_h - c->ladder_scroll;
        if (y + row_h < list.y || y > list.y + list.height) continue;
        ConsumeDrawLadderRow(profile, row + 1, (Rectangle){list.x, y, list.width, row_h}, base_xp, shown_xp);
    }
    EndScissorMode();

    Rectangle close = ConsumePxSquare(wx + CONSUME_WINDOW_W - 62.0f, wy - 18.0f, 80.0f);
    if (ButtonDrawEx(&c->ladder_close, close, input)) c->ladder_open = false;
}

static void ConsumeBeginResults(ConsumeScreen *c, Heroes *h, Player *p) {
    bool multi = c->multi_type != CONSUME_MULTI_NONE;
    const int *seeds = multi ? c->multi_seeds : c->seeds;
    int count = multi ? c->multi_count : c->seed_count;

    c->before = p->roster[c->target];
    int target = PlayerConsumeUnits(p, c->target, seeds, count);
    if (target < 0) {
        ConsumeShowMessage(c, LocGet("CONSUME_CONFIRMATION_SQUAD_WARNING"));
        return;
    }
    ProgressionSave(p);
    HeroesApplyConsume(h, p, seeds, count, target);
    RosterRefreshOrder(&c->roster, p);
    c->target = target;
    c->seed_count = 0;
    c->multi_type = CONSUME_MULTI_NONE;
    c->multi_count = 0;
    c->board_dirty = true;
    c->results = true;
    c->results_settled = false;
    c->results_level = c->before.consume_level;
    c->results_tier = c->before.tier;
    c->results_time = 0.0f;
    int steps = p->roster[target].consume_level - c->before.consume_level;
    c->results_step_time = CONSUME_EASING_TIME / sqrtf((float)steps + 1.0f);
    c->ladder_open = true;
    c->ladder_user_scrolled = false;
    c->ladder_snap = true;
    c->details_index = -1;
    ConsumeClearFly(c);
    GuiSoundPlay(SND_CONSUME_COMBINE);
}

static void ConsumeResultsStep(ConsumeScreen *c, const Character *final) {
    const ConsumeProfile *profile = ConsumeProfileFor(final);
    ConsumeUnlock kind = ConsumeUnlockKindAt(profile, c->results_level);
    int promoted = ConsumeUnlockValueAt(profile, c->results_level, CONSUME_UNLOCK_PROMOTION) - 1;
    c->results_level++;
    if (kind == CONSUME_UNLOCK_PROMOTION && promoted > (int)c->results_tier && promoted <= (int) final->tier) {
        static const char *const promote_fx[RARITY_MYTHIC + 1] = {
            "",
            "fx_tierup_common",
            "fx_tierup_uncommon",
            "fx_tierup_rare",
            "fx_tierup_epic",
            "fx_tierup_legendary",
            "fx_tierup_legendary",
        };
        c->results_tier = (Tier)promoted;
        c->model_request = true;
        c->attack_request = CONSUME_ATTACK_TIER;
        c->fx_request = promote_fx[final->rarity];
        GuiSoundPlay((SoundId)(SND_CONSUME_LEVEL_1 + Clamp(promoted, 0, CONSUME_LEVEL_SOUND_COUNT - 1)));
        return;
    }
    c->fx_request = "fx_levelup_2p0_base";
    c->attack_request = CONSUME_ATTACK_LEVEL;
    GuiSoundPlay((SoundId)(SND_CONSUME_LEVEL_1 + Clamp((int)c->results_tier, 0, CONSUME_LEVEL_SOUND_COUNT - 1)));
}

static void ConsumeUpdateResults(ConsumeScreen *c, const Character *final, float dt) {
    if (c->results_settled) return;
    c->results_time += dt;
    if (c->results_time < c->results_step_time) return;
    if (c->results_level >= final->consume_level) {
        c->results_settled = true;
        return;
    }
    c->results_time = 0.0f;
    ConsumeResultsStep(c, final);
}

static ConsumeLeave ConsumeDrawResultsActions(ConsumeScreen *c, const Player *p, bool input) {
    ButtonDrawEx(&c->pvp, RosterPx(0.0f, 828.0f, 196.0f, 98.0f), false);
    bool squad = ButtonDrawEx(&c->heroes, RosterPx(196.0f, 828.0f, 196.0f, 98.0f), input);
    bool quests = ButtonDrawEx(&c->pve, RosterPx(392.0f, 828.0f, 196.0f, 98.0f), input);
    bool again = ButtonDrawEx(&c->again, RosterPx(1638.0f, 828.0f, 392.0f, 98.0f), input);
    if (again) {
        int target = c->target;
        ConsumeClearBoard(c);
        if (ConsumeCharacterMaxed(&p->roster[target])) return CONSUME_LEAVE_NONE;
        c->target = target;
        ConsumeSyncShownBars(c, &p->roster[target]);
        return CONSUME_LEAVE_NONE;
    }
    return squad ? CONSUME_LEAVE_SQUAD : quests ? CONSUME_LEAVE_QUESTS : CONSUME_LEAVE_NONE;
}

static void ConsumeDrawResults(ConsumeScreen *c, Heroes *h, const Player *p, bool input, float dt) {
    const Character *final = &p->roster[c->target];
    const ConsumeProfile *profile = ConsumeProfileFor(final);
    ConsumeUpdateResults(c, final, dt);

    float progress = c->results_settled ? 1.0f : Clamp(c->results_time / c->results_step_time, 0.0f, 1.0f);
    int level = c->results_level;
    int low = ConsumeXpForLevel(profile->consume_table, level);
    int high = ConsumeXpForLevel(profile->consume_table, level + 1);
    int start = level == c->before.consume_level ? c->before.consume_xp : low;
    int end = level >= final->consume_level ? final->consume_xp : high;
    int xp_now = (int)Lerp((float)start, (float)end, progress);
    float base = level == c->before.consume_level
                     ? ConsumeBandFraction(profile->consume_table, level, c->before.consume_xp)
                     : 0.0f;
    float gain = ConsumeBandFraction(profile->consume_table, level, xp_now);

    float fs = ConsumeFontScale();
    UiDrawTexture(GuiTex(GUI_CONSUME_RESULT_PANEL), RosterPx(79.0f, 632.0f, 453.0f, 130.0f), WHITE);
    char unlock[64];
    Texture2D icon;
    ConsumeLadderLabel(profile, level, unlock, sizeof(unlock), &icon);
    if (icon.id) UiDrawTexture(icon, ConsumePxSquare(107.0f, 636.0f, 54.0f), WHITE);
    UiDrawTextLeftMiddle(GuiFont(FNT_MENU), unlock, RosterPx(169.0f, 649.0f, 363.0f, 46.0f), 26.0f * fs, WHITE);
    ConsumeDrawBar(
        RosterPx(109.0f, 697.0f, 296.0f, 40.0f),
        RosterPx(112.0f, 701.0f, 290.0f, 32.0f),
        GUI_CONSUME_BAR_OLD,
        GUI_CONSUME_BAR_NEW,
        base,
        gain
    );
    if (final->consume_level >= profile->consume_table->count && c->results_settled)
        UiDrawTextCentered(
            GuiFont(FNT_COOL),
            LocGet("CONSUME_PROGRESS_BAR_MAX"),
            RosterPx(112.0f, 691.0f, 290.0f, 52.0f),
            28.0f * fs,
            WHITE
        );
    if (ButtonDrawEx(&c->view_all, RosterPx(415.0f, 692.0f, 110.0f, 52.0f), input)) {
        c->ladder_open = !c->ladder_open;
        c->ladder_user_scrolled = false;
        GuiSoundPlay(SND_UI_TAB);
    }

    if (!c->ladder_open) HeroesDrawDetailsAt(&h->cards[c->target], RosterPx(1092.0f, 38.0f, 585.0f, 750.0f));
    if (c->ladder_open) ConsumeDrawLadder(c, final, xp_now, final->consume_xp, input, dt);
    ConsumeLeave leave = ConsumeDrawResultsActions(c, p, input);
    if (leave != CONSUME_LEAVE_NONE) c->leave_done = leave;
}

static void ConsumeDrawActions(ConsumeScreen *c, Player *p, Heroes *h, bool input) {
    if (c->seed_count == 0) return;
    if (ButtonDrawEx(&c->cancel, RosterPx(552.0f, 828.0f, 392.0f, 98.0f), input)) {
        ConsumeClearBoard(c);
        return;
    }
    if (ButtonDrawEx(&c->multiple, RosterPx(944.0f, 828.0f, 392.0f, 98.0f), input))
        c->dialog = CONSUME_DIALOG_MULTI_TYPE;
    if (ButtonDrawEx(&c->consume, RosterPx(1336.0f, 828.0f, 392.0f, 98.0f), input)) ConsumeBeginResults(c, h, p);
}

static Rectangle ConsumeMultiPopup(void) {
    float height = UiHeight() * CONSUME_MULTI_POPUP_H;
    float width = fminf(height * CONSUME_MULTI_POPUP_ASPECT, UiWidth() * CONSUME_MULTI_POPUP_W);
    return UiRect(UiRoot(), 0.5f, 0.75f, width, height);
}

static void ConsumeDrawMessage(ConsumeScreen *c) {
    Rectangle popup = MessagePopupRect();
    MessagePopupDrawScrim();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), popup, WHITE);
    Rectangle content = UiRect(popup, 0.5f, 0.5f, 0.8f, 0.8f);
    MessagePopupDrawText(GuiFont(FNT_MENU), c->message, UiRect(content, 0.0f, 0.0f, 1.0f, 0.5f), 26.0f, WHITE);
    Rectangle row = UiRect(popup, 0.0f, 0.9f, 1.0f, 0.187f);
    if (ButtonDraw(&c->dialog_ok, UiRect(row, 0.5f, 0.5f, 0.28f, 0.8f))) c->dialog = CONSUME_DIALOG_NONE;
}

static void ConsumeDrawRarityWarning(ConsumeScreen *c, HeroesBackground *scene, const Player *p) {
    Rectangle popup = MessagePopupRect();
    const Character *seed = &p->roster[c->pending_seed];
    MessagePopupDrawScrim();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), popup, WHITE);
    Rectangle content = UiRect(popup, 0.5f, 0.5f, 0.8f, 0.8f);
    Rectangle body = UiRect(content, 0.0f, 0.0f, 1.0f, 0.5f);
    MessagePopupDrawText(GuiFont(FNT_MENU), ConsumeSeedWarning(seed), body, 26.0f, WHITE);

    Rectangle check_row = {content.x, body.y + body.height, content.width, content.height * 0.187f};
    float box_size = check_row.height * 0.8f;
    Rectangle box = UiRect(check_row, 0.14f, 0.5f, box_size, box_size);
    UiDrawTexture(GuiTex(c->warning_disable ? GUI_CONSUME_CHECK_ON : GUI_CONSUME_CHECK_OFF), box, WHITE);
    Rectangle label = UiRect(check_row, 1.0f, 0.5f, 0.75f, 1.0f);
    UiDrawTextLeftMiddle(GuiFont(FNT_MENU), LocGet("CONSUME_CONFIRMATION_DISABLE_WARNING"), label, 20.0f, WHITE);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), check_row))
        c->warning_disable = !c->warning_disable;

    Rectangle row = MessagePopupChoiceRow(popup);
    bool yes = ButtonDraw(&c->dialog_yes, UiRect(row, 0.0f, 0.0f, 0.5f, 1.0f));
    bool no = ButtonDraw(&c->dialog_no, UiRect(row, 1.0f, 0.0f, 0.5f, 1.0f));
    if (!yes && !no) return;
    c->dialog = CONSUME_DIALOG_NONE;
    if (!yes) return;
    c->warnings_disabled[seed->rarity] = c->warning_disable;
    ConsumeSeedAdd(c, p, c->pending_seed);
    HeroesBackgroundPlayPlace(scene);
}

static void ConsumeDrawMultiType(ConsumeScreen *c, const Player *p) {
    Rectangle popup = ConsumeMultiPopup();
    Font body = GuiFont(FNT_MENU);
    MessagePopupDrawScrim();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), popup, WHITE);
    MessagePopupDrawText(
        body, "Выберите, каких героев поглотить:", UiRect(popup, 0.5f, 0.07f, 0.83f, 0.15f), 20.0f, WHITE
    );
    Rectangle choice = UiRect(popup, 0.5f, 0.5f, 0.8f, 0.6f);
    const struct {
        const char *text;
        Button *button;
        ConsumeMultiType type;
        float y;
    } rows[] = {
        {"Все копии выбранных героев.", &c->dialog_identical, CONSUME_MULTI_IDENTICAL, 0.05f},
        {"Все герои той же редкости.", &c->dialog_rarity, CONSUME_MULTI_RARITY, 0.45f},
        {"Все герои той же редкости и фракции.", &c->dialog_faction, CONSUME_MULTI_FACTION, 0.85f},
    };
    for (int i = 0; i < 3; i++) {
        if (ButtonDraw(rows[i].button, UiRect(choice, 0.0f, rows[i].y, 0.45f, 0.2f)))
            ConsumeMultiSelect(c, p, rows[i].type);
        MessagePopupDrawText(body, rows[i].text, UiRect(choice, 1.0f, rows[i].y, 0.53f, 0.2f), 17.0f, WHITE);
        if (i < 2) UiDrawTexture(GuiTex(GUI_CONSUME_LINE), UiRect(choice, 0.5f, rows[i].y + 0.25f, 1.0f, 0.01f), WHITE);
    }
    MessagePopupDrawText(
        body,
        "Герои в отрядах не будут поглощены.",
        UiRect(popup, 0.5f, 0.9f, 0.85f, 0.1f),
        16.0f,
        (Color){200, 196, 184, 255}
    );
    float close_size = popup.height * 0.15f;
    if (ButtonDraw(&c->ladder_close, UiRect(popup, 0.99f, -0.04f, close_size, close_size)))
        c->dialog = CONSUME_DIALOG_NONE;
}

static void ConsumeDrawMultiConfirm(ConsumeScreen *c, Heroes *h, Player *p) {
    static const char *const titles[] = {"", "Поглотить копии", "Поглотить по редкости", "Поглотить по фракции"};
    static const float unit_x[CONSUME_MAX_SEEDS] = {0.14f, 0.32f, 0.5f, 0.67f, 0.84f};
    Rectangle popup = ConsumeMultiPopup();
    Font title = GuiFont(FNT_COOL), body = GuiFont(FNT_MENU);
    MessagePopupDrawScrim();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), popup, WHITE);
    MessagePopupDrawText(title, titles[c->multi_type], UiRect(popup, 0.5f, 0.1f, 0.85f, 0.15f), 24.0f, WHITE);

    Rectangle units = UiRect(popup, 0.5f, 0.3f, 0.86f, 0.3f);
    for (int s = 0; s < c->seed_count; s++) {
        Rectangle slot = UiRect(units, unit_x[s], 0.0f, 0.12f, 1.0f);
        float icon_size = fminf(slot.width, slot.height * 0.7f);
        Rectangle icon = UiRect(slot, 0.5f, 0.0f, icon_size, icon_size);
        UiDrawTexture(h->cards[c->seeds[s]].portrait, icon, WHITE);
        char count[16];
        snprintf(count, sizeof(count), "x%d", c->multi_counts[s]);
        Rectangle label = {slot.x, icon.y + icon.height, slot.width, slot.height - icon.height};
        UiDrawTextCentered(body, count, label, 18.0f, WHITE);
    }
    char text[CONSUME_MESSAGE_CAP];
    snprintf(text, sizeof(text), "Будет поглощено героев: %d. Вы уверены?", c->multi_count);
    MessagePopupDrawText(body, text, UiRect(popup, 0.5f, 0.54f, 0.85f, 0.16f), 19.0f, WHITE);

    Rectangle choice = UiRect(popup, 0.5f, 0.85f, 0.8f, 0.3f);
    bool view = ButtonDraw(&c->view_all, UiRect(choice, 0.5f, 0.25f, 0.35f, 0.35f));
    bool no = ButtonDraw(&c->cancel, UiRect(choice, 0.11f, 1.0f, 0.4f, 0.45f));
    bool yes = ButtonDraw(&c->consume, UiRect(choice, 0.86f, 1.0f, 0.4f, 0.45f));
    if (view) {
        c->dialog = CONSUME_DIALOG_NONE;
        c->ladder_open = true;
        c->ladder_user_scrolled = false;
        c->multi_ladder = true;
        return;
    }
    if (!yes && !no) return;
    c->dialog = CONSUME_DIALOG_NONE;
    if (yes && c->multi_count > 0) {
        ConsumeBeginResults(c, h, p);
    } else {
        c->multi_type = CONSUME_MULTI_NONE;
        c->multi_count = 0;
    }
}

static void ConsumeDrawDialog(ConsumeScreen *c, HeroesBackground *scene, Heroes *h, Player *p) {
    switch (c->dialog) {
        case CONSUME_DIALOG_MESSAGE:
            ConsumeDrawMessage(c);
            break;
        case CONSUME_DIALOG_RARITY_WARNING:
            ConsumeDrawRarityWarning(c, scene, p);
            break;
        case CONSUME_DIALOG_MULTI_TYPE:
            ConsumeDrawMultiType(c, p);
            break;
        case CONSUME_DIALOG_MULTI_CONFIRM:
            ConsumeDrawMultiConfirm(c, h, p);
            break;
        default:
            break;
    }
}

static void ConsumeDrawUnitDetails(ConsumeScreen *c, Heroes *h, bool input) {
    if (c->details_index < 0) return;
    HeroesDrawDetails(
        &h->cards[c->details_index], CONSUME_PANEL_WIDTH, UiWidth() - CONSUME_PANEL_WIDTH - CONSUME_DETAILS_RIGHT_MARGIN
    );
    if (!input) return;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) c->details_armed = true;
    if (c->details_armed && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        c->details_index = -1;
        c->details_armed = false;
    }
}

bool ConsumeHandleBack(ConsumeScreen *c) {
    if (c->dialog != CONSUME_DIALOG_NONE) {
        if (c->dialog == CONSUME_DIALOG_MULTI_CONFIRM) {
            c->multi_type = CONSUME_MULTI_NONE;
            c->multi_count = 0;
        }
        c->dialog = CONSUME_DIALOG_NONE;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (c->ladder_open) {
        c->ladder_open = false;
        return true;
    }
    if (c->details_index >= 0) {
        c->details_index = -1;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return true;
    }
    if (c->target >= 0 && !c->results) {
        ConsumeClearBoard(c);
        return true;
    }
    c->leave_done = CONSUME_LEAVE_SQUAD;
    return true;
}

bool ConsumeTakeBoardUpdate(ConsumeScreen *c, const Player *p, const Character **units, Character *target_copy) {
    bool model_changed = c->model_request;
    if (!c->board_dirty && !model_changed) return false;
    c->board_dirty = false;
    c->model_request = false;
    for (int i = 0; i <= CONSUME_MAX_SEEDS; i++) units[i] = NULL;
    if (c->target < 0) return true;
    *target_copy = p->roster[c->target];
    if (c->results) target_copy->tier = c->results_tier;
    units[0] = target_copy;
    for (int i = 0; i < c->seed_count; i++) units[i + 1] = &p->roster[c->seeds[i]];
    return true;
}

int ConsumeTakePlacedSpot(ConsumeScreen *c) {
    int spot = c->placed_spot;
    c->placed_spot = -1;
    return spot;
}

const char *ConsumeTakeFxRequest(ConsumeScreen *c) {
    const char *name = c->fx_request;
    c->fx_request = NULL;
    return name;
}

ConsumeAttackClip ConsumeTakeAttackRequest(ConsumeScreen *c) {
    ConsumeAttackClip clip = c->attack_request;
    c->attack_request = CONSUME_ATTACK_NONE;
    return clip;
}

ConsumeCamKey ConsumeWantedCamera(const ConsumeScreen *c, const Player *p) {
    if (c->results) return p->roster[c->target].giant ? CONSUME_CAM_COMPLETE_TALL : CONSUME_CAM_COMPLETE;
    return c->target >= 0 ? CONSUME_CAM_ALL : CONSUME_CAM_BOARD;
}

ConsumeLeave ConsumeDraw(ConsumeScreen *c, HeroesBackground *scene, Heroes *h, Player *p, float dt) {
    if (c->multi_ladder && !c->ladder_open) {
        c->multi_ladder = false;
        c->dialog = CONSUME_DIALOG_MULTI_CONFIRM;
    }
    bool blocked = UiInputBlocked() || c->dialog != CONSUME_DIALOG_NONE;
    bool input = !blocked && c->details_index < 0;

    if (c->results) {
        ConsumeDrawResults(c, h, p, input, dt);
    } else {
        ConsumeDrawRoster(c, h, p, input);
        if (c->target < 0)
            ConsumeDrawCallToAction(c, input);
        else
            ConsumeDrawPreview(c, p, input, dt);
        if (c->target >= 0 && c->ladder_open) {
            ConsumeOutcome outcome = ConsumePreview(c, p);
            ConsumeDrawLadder(c, &p->roster[c->target], p->roster[c->target].consume_xp, outcome.consume_xp, input, dt);
        }
        ConsumeDrawActions(c, p, h, input);
        if (c->seed_count == 0) {
            HeroesButton nav =
                HeroesDrawNav(h, HEROES_CONSUME, input, RuneUnseenCount(&p->runes, RuneNow()), p->gallery.badge);
            if (nav == HEROES_SQUAD) ConsumeRequestLeave(c, CONSUME_LEAVE_SQUAD);
            if (nav == HEROES_RUNES) ConsumeRequestLeave(c, CONSUME_LEAVE_RUNES);
            if (nav == HEROES_GALLERY) ConsumeRequestLeave(c, CONSUME_LEAVE_GALLERY);
        }
        if (!c->ladder_open) ConsumeDrawFlyText(c, dt);
        if (input || c->press_index >= 0) ConsumeUpdateDrag(c, scene, p, !blocked);
        ConsumeDrawDragged(c, h);
    }
    if (!blocked) ConsumeOpenPendingDetails(c);
    ConsumeDrawUnitDetails(c, h, !blocked);
    ConsumeDrawDialog(c, scene, h, p);

    ConsumeLeave done = c->leave_done;
    c->leave_done = CONSUME_LEAVE_NONE;
    return done;
}
