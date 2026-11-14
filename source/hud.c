typedef enum {
    HUD_NONE = 0,
    HUD_HOME,
    HUD_PROFILE,
    HUD_ENERGY,
    HUD_STAMINA,
    HUD_GOLD,
    HUD_GEMS,
    HUD_OPTIONS,
    HUD_TROPHIES,
    HUD_BANNERS,
} HudAction;

typedef struct {
    bool initialized;
    int value;
    int delta;
    float change_time;
} HudChangingValue;

#define HUD_PROFILE_ART EXTRA_ASSET_DIR "gui/hud/profile.png"
#define HUD_PROFILE_STEP 0.9f
#define HUD_TALLY_TIME 2.25f
#define HUD_TALLY_PULSE_COUNT 3
#define HUD_TALLY_PULSE_PERIOD 0.75f
#define HUD_TALLY_PULSE_TIME 0.5f
#define HUD_TALLY_PULSE_GROWTH 0.5f

typedef struct {
    bool initialized;
    bool held;
    bool ticking;
    double shown;
    double rate;
    float elapsed;
} HudCurrencyTally;

typedef struct {
    Texture2D background;
    Button home_button;
    Texture2D home_inactive;
    Texture2D xp_frame;
    Texture2D xp_bg;
    Texture2D xp_fill;
    Texture2D resource_frame;
    Texture2D energy;
    Texture2D stamina;
    Texture2D gold;
    Texture2D gold_selected;
    Texture2D gems;
    Texture2D gems_selected;
    Texture2D options;
    Texture2D profile;
    Font font;
    HudAction armed_action;
    HudAction store_selection;
    HudChangingValue energy_value;
    HudChangingValue stamina_value;
    HudCurrencyTally gold_tally;
    HudCurrencyTally gems_tally;
    HudCurrencyTally trophies_tally;
    bool show_trophies;
    bool show_banners;
} Hud;

void HudSetStoreSelection(Hud *hud, HudAction selection) {
    hud->store_selection = selection;
}

typedef struct {
    Rectangle background;
    Rectangle home;
    Rectangle xp;
    Rectangle xp_bar;
    Rectangle xp_level_label;
    Rectangle xp_amount_label;
    Rectangle energy;
    Rectangle energy_icon;
    Rectangle energy_numerator;
    Rectangle energy_denominator;
    Rectangle energy_hit;
    Rectangle stamina;
    Rectangle stamina_icon;
    Rectangle stamina_numerator;
    Rectangle stamina_denominator;
    Rectangle stamina_hit;
    Rectangle gold_slot;
    Rectangle gold_icon;
    Rectangle gold_label;
    Rectangle gems_slot;
    Rectangle gems_icon;
    Rectangle gems_label;
    Rectangle options;
    Rectangle profile;
} HudLayout;

#define HUD_TOP_HEIGHT 73.0f

static float HudRectValue(float parent, float input) {
    if (input <= -2.0f || input >= 2.0f) return ceilf(input);
    return ceilf(parent * input);
}

static Rectangle HudFitRect(Rectangle parent, float x, float y, float width, float height, float aspect) {
    float w = HudRectValue(parent.width, width);
    float h = HudRectValue(parent.height, height);
    if (aspect > 0.0f && w > 0.0f && h > 0.0f) {
        float aspect_width = h * aspect;
        if (aspect_width < w)
            w = aspect_width;
        else
            h = w / aspect;
    }
    return (Rectangle){
        parent.x + HudRectValue(parent.width - w, x),
        parent.y + HudRectValue(parent.height - h, y),
        w,
        h,
    };
}

static Rectangle HudOffsetRect(Rectangle rect, float x, float y) {
    rect.x += x;
    rect.y += y;
    return rect;
}

static HudLayout HudGetLayout(void) {
    HudLayout l = {0};
    Rectangle screen = UiRoot();

    l.background = HudFitRect(screen, 0.5f, 0.0f, 1.9f, 0.12f, 16.0f);

    Rectangle sizing = {2.0f, 0.0f, fmaxf(0.0f, ceilf(UiWidth()) - 2.0f), HUD_TOP_HEIGHT};
    Rectangle left = HudFitRect(sizing, 0.0f, 0.0f, 1.0f, 0.9f, 8.0f);
    l.home = HudFitRect(left, 0.0f, 0.0f, 1.0f, 1.0f, 2.0f);

    Rectangle dense_parent = {0.0f, 0.0f, left.width, left.height};
    Rectangle local_xp = HudFitRect(dense_parent, 0.0f, 0.5f, 0.33f, 1.0f, 3.4762f);
    Rectangle local_energy = HudFitRect(dense_parent, -0.04f, 0.5f, 0.33f, 1.1f, 2.39520955f);
    Rectangle local_stamina = HudFitRect(dense_parent, -0.07f, 0.5f, 0.33f, 1.1f, 2.39520955f);
    l.xp = HudOffsetRect(local_xp, l.home.x + l.home.width, left.y);
    l.energy = HudOffsetRect(local_energy, l.xp.x + l.xp.width, left.y);
    l.stamina = HudOffsetRect(local_stamina, l.energy.x + l.energy.width, left.y);

    l.xp_bar = HudFitRect(l.xp, 0.7f, 0.5f, 0.63f, 0.3f, 0.0f);
    l.xp_level_label = HudFitRect(l.xp, 0.0f, 0.5f, 0.275f, 0.75f, 0.0f);
    l.xp_amount_label = HudFitRect(l.xp, 0.67233247f, 0.49542952f, 0.5538922f, 0.28f, 0.0f);

    l.energy_icon = HudFitRect(l.energy, 0.11f, 0.39f, 1.0f, 0.5f, 1.0f);
    l.energy_numerator = HudFitRect(l.energy, 0.47f, 0.83f, 1.99f, 1.06f, 0.45f);
    l.energy_denominator = HudFitRect(l.energy, 0.72f, 0.83f, 1.99f, 1.06f, 0.45f);
    l.energy_hit = HudFitRect(l.energy, 0.0f, 0.5f, 1.0f, 1.5f, 0.0f);

    l.stamina_icon = HudFitRect(l.stamina, 0.11f, 0.45f, 1.0f, 0.5f, 1.0f);
    l.stamina_numerator = HudFitRect(l.stamina, 0.47f, 0.83f, 1.99f, 1.06f, 0.45f);
    l.stamina_denominator = HudFitRect(l.stamina, 0.72f, 0.83f, 1.99f, 1.06f, 0.45f);
    l.stamina_hit = HudFitRect(l.stamina, 0.0f, 0.5f, 1.0f, 1.5f, 0.0f);

    Rectangle currency = HudFitRect(sizing, 0.910000026f, 0.0f, 0.639999986f, 0.899999976f, 4.539999962f);
    l.options = HudFitRect(sizing, 1.0f, 0.0f, 1.0f, 0.899999976f, 1.128000021f);
    float profile_step = ceilf(l.options.width * HUD_PROFILE_STEP);
    l.profile = HudOffsetRect(l.options, -profile_step, 0.0f);
    currency = HudOffsetRect(currency, -profile_step, 0.0f);

    Rectangle local_gold =
        HudFitRect((Rectangle){0.0f, 0.0f, currency.width, currency.height}, 0.0f, 0.5f, 0.55f, 1.0f, 0.0f);
    Rectangle local_gems =
        HudFitRect((Rectangle){0.0f, 0.0f, currency.width, currency.height}, 0.0f, 0.5f, 0.45f, 1.0f, 0.0f);
    l.gold_slot = HudOffsetRect(local_gold, currency.x, currency.y);
    l.gems_slot = HudOffsetRect(local_gems, l.gold_slot.x + l.gold_slot.width, currency.y);
    l.gold_icon = HudFitRect(l.gold_slot, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
    l.gold_label = HudFitRect(l.gold_slot, 1.0f, 0.5f, 0.58f, 1.0f, 0.0f);
    l.gems_icon = HudFitRect(l.gems_slot, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
    l.gems_label = HudFitRect(l.gems_slot, 1.0f, 0.5f, 0.55f, 1.0f, 0.0f);
    return l;
}

static int HudDisplayedXpMinimum(const Player *player) {
    return player->experience_min < player->experience_max ? player->experience_min : 0;
}

static float HudXpProgress(const Player *player) {
    int minimum = HudDisplayedXpMinimum(player);
    int range = player->experience_max - minimum;
    if (range <= 0) return 0.0f;
    return Clamp((float)(player->experience - minimum) / (float)range, 0.0f, 1.0f);
}

static Texture2D hud_profile_art;

static Texture2D HudProfileTexture(void) {
    if (!hud_profile_art.id && ResourceExists(HUD_PROFILE_ART)) hud_profile_art = ResourceLoadTexture(HUD_PROFILE_ART);
    return hud_profile_art.id ? hud_profile_art : GuiTex(GUI_HUD_SOCIAL);
}

void HudInitialize(Hud *hud) {
    hud->font = GuiFont(FNT_COOL);
    hud->background = GuiTex(GUI_HUD_GRADIENT);
    hud->home_button = ButtonLoad(GUI_BTN_CAMP, GUI_BTN_CAMP_ON);
    ButtonSetClickSound(&hud->home_button, SND_UI_HOME);
    hud->home_inactive = GuiTex(GUI_BTN_CAMP_INACTIVE);
    hud->xp_frame = GuiTex(GUI_HUD_XP_FRAME);
    hud->xp_bg = GuiTex(GUI_HUD_XP_BG);
    hud->xp_fill = GuiTex(GUI_HUD_XP_METER);
    hud->resource_frame = GuiTex(GUI_HUD_RESOURCE_FRAME);
    hud->energy = GuiTex(GUI_HUD_ENERGY);
    hud->stamina = GuiTex(GUI_HUD_STAMINA);
    hud->gold = GuiTex(GUI_HUD_GOLD);
    hud->gold_selected = GuiTex(GUI_HUD_GOLD_SELECTED);
    hud->gems = GuiTex(GUI_HUD_GEMS);
    hud->gems_selected = GuiTex(GUI_HUD_GEMS_SELECTED);
    hud->options = GuiTex(GUI_HUD_OPTIONS);
    hud->profile = HudProfileTexture();
    hud->armed_action = HUD_NONE;
    hud->energy_value = (HudChangingValue){0};
    hud->stamina_value = (HudChangingValue){0};
    hud->gold_tally = (HudCurrencyTally){0};
    hud->gems_tally = (HudCurrencyTally){0};
    hud->trophies_tally = (HudCurrencyTally){0};
}

static Rectangle HudActionRect(HudAction action, const HudLayout *layout) {
    switch (action) {
        case HUD_ENERGY:
            return layout->energy_hit;
        case HUD_STAMINA:
            return layout->stamina_hit;
        case HUD_GOLD:
            return layout->gold_slot;
        case HUD_GEMS:
            return layout->gems_slot;
        case HUD_OPTIONS:
            return layout->options;
        case HUD_PROFILE:
            return layout->profile;
        default:
            return (Rectangle){0};
    }
}

static HudAction HudActionAt(Vector2 point, const HudLayout *layout) {
    static const HudAction order[] = {HUD_STAMINA, HUD_ENERGY, HUD_GOLD, HUD_GEMS, HUD_OPTIONS, HUD_PROFILE};
    for (int i = 0; i < (int)(sizeof(order) / sizeof(order[0])); i++) {
        if (CheckCollisionPointRec(point, HudActionRect(order[i], layout))) return order[i];
    }
    return HUD_NONE;
}

static HudAction HudUpdateInput(Hud *hud, const HudLayout *layout, bool interactive) {
    if (!interactive) {
        hud->armed_action = HUD_NONE;
        return HUD_NONE;
    }

    Vector2 mouse = UiMouse();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) hud->armed_action = HudActionAt(mouse, layout);

    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return HUD_NONE;
    HudAction clicked = HUD_NONE;
    if (hud->armed_action != HUD_NONE && CheckCollisionPointRec(mouse, HudActionRect(hud->armed_action, layout)))
        clicked = hud->armed_action;
    hud->armed_action = HUD_NONE;
    return clicked;
}

static bool HudActionPressed(const Hud *hud, HudAction action, const HudLayout *layout, bool interactive) {
    return interactive && hud->armed_action == action && IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(UiMouse(), HudActionRect(action, layout));
}

static void HudChangingValueUpdate(HudChangingValue *state, int value, float dt) {
    if (!state->initialized) {
        state->initialized = true;
        state->value = value;
        return;
    }
    if (value != state->value) {
        state->delta = value - state->value;
        state->value = value;
        state->change_time = 2.0f;
    }
    if (state->change_time > 0.0f) state->change_time = fmaxf(0.0f, state->change_time - dt);
}

static Color HudChangingValueColor(const HudChangingValue *state) {
    if (state->change_time <= 0.0f) return WHITE;
    float elapsed = 2.0f - state->change_time;
    float pulse = fmodf(elapsed, 0.5f) / 0.5f;
    Color delta_color = state->delta >= 0 ? (Color){0, 255, 0, 255} : (Color){255, 0, 0, 255};
    return ColorAlpha(delta_color, Clamp(pulse, 0.0f, 1.0f));
}

static void HudDrawXp(const Hud *hud, const Player *player, const HudLayout *layout) {
    UiDrawTexture(hud->xp_frame, layout->xp, WHITE);
    UiDrawTexture(hud->xp_bg, layout->xp_bar, WHITE);
    float progress = HudXpProgress(player);
    if (progress > 0.0f) {
        Rectangle src = {0.0f, 0.0f, (float)hud->xp_fill.width * progress, (float)hud->xp_fill.height};
        Rectangle dst = layout->xp_bar;
        dst.width *= progress;
        DrawTexturePro(hud->xp_fill, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    }
    UiDrawTextCentered(hud->font, TextFormat("%d", player->level), layout->xp_level_label, 20.0f, WHITE);
    UiDrawTextCentered(
        hud->font,
        TextFormat("%d/%d", player->experience, player->experience_max),
        layout->xp_amount_label,
        15.0f,
        WHITE
    );
}

static void HudDrawResource(
    const Hud *hud,
    Texture2D icon,
    Rectangle frame,
    Rectangle icon_rect,
    Rectangle numerator_rect,
    Rectangle denominator_rect,
    const HudChangingValue *value,
    int maximum,
    Color frame_tint
) {
    UiDrawTexture(hud->resource_frame, frame, frame_tint);
    UiDrawTexture(icon, icon_rect, WHITE);

    const char *numerator =
        value->change_time > 0.0f ? TextFormat("%+d", value->delta) : TextFormat("%d", value->value);
    UiDrawTextRight(hud->font, numerator, numerator_rect, 21.0f, HudChangingValueColor(value));
    UiDrawTextLeftMiddle(hud->font, TextFormat("/%d", maximum), denominator_rect, 21.0f, WHITE);
}

static void HudCurrencyTallyUpdate(HudCurrencyTally *tally, int value, float dt) {
    if (!tally->initialized) {
        tally->initialized = true;
        tally->shown = value;
        return;
    }
    if (tally->held) return;
    if (!tally->ticking) {
        tally->shown = value;
        return;
    }
    tally->elapsed += dt;
    double step = tally->rate * dt;
    if (fabs(value - tally->shown) <= step) {
        tally->shown = value;
        tally->ticking = false;
        return;
    }
    tally->shown += value > tally->shown ? step : -step;
}

static void HudCurrencyTallyHold(HudCurrencyTally *tally) {
    tally->held = true;
    tally->ticking = false;
}

static bool HudCurrencyTallyRelease(HudCurrencyTally *tally, int value) {
    if (!tally->held) return false;
    tally->held = false;
    if (value <= tally->shown) return false;
    tally->rate = (value - tally->shown) / HUD_TALLY_TIME;
    tally->elapsed = 0.0f;
    tally->ticking = true;
    return true;
}

void HudCurrencyHold(Hud *hud, bool gems) {
    HudCurrencyTallyHold(gems ? &hud->gems_tally : &hud->gold_tally);
}

void HudCurrencyRelease(Hud *hud, const Player *player, bool gems) {
    HudCurrencyTally *tally = gems ? &hud->gems_tally : &hud->gold_tally;
    bool rising = HudCurrencyTallyRelease(tally, gems ? player->gems : player->gold);
    if (rising && gems) GuiSoundPlay(SND_TALLY_GEM);
}

static void HudDrawCurrencyPulses(Texture2D icon, Rectangle rect, float elapsed) {
    for (int pulse = 0; pulse < HUD_TALLY_PULSE_COUNT; pulse++) {
        float local = (elapsed - pulse * HUD_TALLY_PULSE_PERIOD) / HUD_TALLY_PULSE_TIME;
        if (local < 0.0f || local > 1.0f) continue;
        float scale = 1.0f + HUD_TALLY_PULSE_GROWTH * local;
        Rectangle grown = {
            rect.x + rect.width * (1.0f - scale) * 0.5f,
            rect.y + rect.height * (1.0f - scale) * 0.5f,
            rect.width * scale,
            rect.height * scale,
        };
        UiDrawTexture(icon, grown, Fade(WHITE, 1.0f - local));
    }
}

static void HudDrawCurrency(
    const Hud *hud, Texture2D icon, Rectangle icon_rect, Rectangle label_rect, const HudCurrencyTally *tally
) {
    UiDrawTexture(icon, icon_rect, WHITE);
    if (tally->ticking) HudDrawCurrencyPulses(icon, icon_rect, tally->elapsed);
    Color color = tally->ticking ? YELLOW : WHITE;
    UiDrawTextLeftMiddle(hud->font, TextFormat("%d", (int)round(tally->shown)), label_rect, 22.0f, color);
}

Rectangle HudXpSparkleRect(const Player *player) {
    HudLayout layout = HudGetLayout();
    float size = layout.xp.height;
    float x_position = HudXpProgress(player) * 0.875f + 0.16f;
    return (Rectangle){
        layout.xp.x + (layout.xp.width - size) * x_position,
        layout.xp.y,
        size,
        size,
    };
}

Vector2 HudCurrencyTarget(bool gems) {
    HudLayout layout = HudGetLayout();
    Rectangle icon = gems ? layout.gems_icon : layout.gold_icon;
    return (Vector2){icon.x + icon.width * 0.5f, icon.y + icon.height * 0.5f};
}

HudAction HudDraw(Hud *hud, Player *player, bool interactive, bool on_home) {
    HudLayout layout = HudGetLayout();
    float dt = GetFrameTime();
    HudChangingValueUpdate(&hud->energy_value, player->energy, dt);
    HudChangingValueUpdate(&hud->stamina_value, player->stamina, dt);
    HudCurrencyTallyUpdate(&hud->gold_tally, player->gold, dt);
    HudCurrencyTallyUpdate(&hud->gems_tally, player->gems, dt);
    int pvp_value = hud->show_banners ? player->pvp.event.banners : player->pvp.trophies;
    HudCurrencyTallyUpdate(&hud->trophies_tally, pvp_value, dt);

    HudAction action = HudUpdateInput(hud, &layout, interactive);
    if ((hud->show_trophies || hud->show_banners) && action == HUD_GEMS)
        action = hud->show_banners ? HUD_BANNERS : HUD_TROPHIES;
    bool energy_pressed = HudActionPressed(hud, HUD_ENERGY, &layout, interactive);
    bool stamina_pressed = HudActionPressed(hud, HUD_STAMINA, &layout, interactive);

    UiDrawTexture(hud->background, layout.background, WHITE);

    bool home_clicked = false;
    TutorialAnchorSet(TUTORIAL_ANCHOR_HUD_HOME, layout.home);
    TutorialAnchorSet(TUTORIAL_ANCHOR_HUD_ENERGY, layout.energy_hit);
    TutorialAnchorSet(TUTORIAL_ANCHOR_HUD_STAMINA, layout.stamina_hit);
    if (on_home)
        UiDrawTexture(hud->home_inactive, layout.home, WHITE);
    else
        home_clicked = ButtonDrawEx(&hud->home_button, layout.home, interactive);

    HudDrawXp(hud, player, &layout);
    HudDrawResource(
        hud,
        hud->energy,
        layout.energy,
        layout.energy_icon,
        layout.energy_numerator,
        layout.energy_denominator,
        &hud->energy_value,
        player->energy_max,
        energy_pressed ? (Color){255, 192, 0, 255} : WHITE
    );
    HudDrawResource(
        hud,
        hud->stamina,
        layout.stamina,
        layout.stamina_icon,
        layout.stamina_numerator,
        layout.stamina_denominator,
        &hud->stamina_value,
        player->stamina_max,
        stamina_pressed ? (Color){255, 89, 137, 255} : WHITE
    );

    HudDrawCurrency(
        hud,
        hud->store_selection == HUD_GOLD ? hud->gold_selected : hud->gold,
        layout.gold_icon,
        layout.gold_label,
        &hud->gold_tally
    );
    if (hud->show_trophies || hud->show_banners)
        HudDrawCurrency(
            hud,
            GuiTex(hud->show_banners ? GUI_PVP_BANNER : GUI_TAB_TROPHY),
            layout.gems_icon,
            layout.gems_label,
            &hud->trophies_tally
        );
    else
        HudDrawCurrency(
            hud,
            hud->store_selection == HUD_GEMS ? hud->gems_selected : hud->gems,
            layout.gems_icon,
            layout.gems_label,
            &hud->gems_tally
        );
    UiDrawTexture(hud->options, layout.options, WHITE);
    UiDrawTexture(hud->profile, layout.profile, WHITE);

    if (home_clicked) return HUD_HOME;
    return action;
}
