typedef enum { ENERGY_DIALOG_ACTION_NONE = 0, ENERGY_DIALOG_ACTION_QUESTS, ENERGY_DIALOG_ACTION_PVP } EnergyDialogAction;

typedef struct {
    bool open;
    bool stamina;
    bool opening_release;
    bool scrim_armed;
    Font font;
    Texture2D bg;
    Texture2D brush;
    Texture2D resource_icon;
    Texture2D gem;
    Button refill;
    Button navigate;
    Button close;
} EnergyDialog;

static void EnergyDialogInitializeKind(EnergyDialog *d, bool stamina) {
    d->open = false;
    d->stamina = stamina;
    d->opening_release = false;
    d->scrim_armed = false;
    d->font = GuiFont(FNT_COOL);
    d->bg = GuiTex(GUI_STORE_OVERLAY);
    d->brush = GuiTex(GUI_REWARD_BRUSH);
    d->resource_icon = GuiTex(stamina ? GUI_HUD_STAMINA : GUI_HUD_ENERGY);
    d->gem = GuiTex(GUI_DIALOG_GEM);
    d->refill = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("REFRESH_REFILL"), d->font, (Vector2){0.5f, 0.5f}, 20.0f / 76.0f
    );
    d->navigate = ButtonLoadText(
        GUI_BTN_GREY,
        GUI_BTN_GREY_ON,
        LocGet(stamina ? "NEXUS_BUTTON_LOC_STRING" : "REFRESH_BATTLE"),
        d->font,
        (Vector2){0.5f, 0.5f},
        20.0f / 76.0f
    );
    d->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetLabelColor(&d->refill, WHITE);
    ButtonSetLabelColor(&d->navigate, WHITE);
    ButtonDisableClickSound(&d->refill);
    ButtonSetClickSound(&d->close, SND_UI_POPUP_CLOSE);
}

void EnergyDialogInitialize(EnergyDialog *d) {
    EnergyDialogInitializeKind(d, false);
}

void StaminaDialogInitialize(EnergyDialog *d) {
    EnergyDialogInitializeKind(d, true);
}

void EnergyDialogOpen(EnergyDialog *d) {
    d->open = true;
    d->scrim_armed = false;
    d->opening_release = true;
}

void EnergyDialogClose(EnergyDialog *d) {
    d->open = false;
    d->opening_release = false;
    d->scrim_armed = false;
}

bool EnergyDialogIsOpen(const EnergyDialog *d) {
    return d->open;
}

EnergyDialogAction EnergyDialogDraw(EnergyDialog *d, Player *player, bool allow_navigation) {
    Font menu_font = GuiFont(FNT_MENU);
    Font cool_font = d->font;
    Rectangle root = UiRoot();

    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 180});

    Rectangle panel = {
        ceilf((root.width - 1065.0f) * 0.5f),
        ceilf((root.height - 500.0f) * 0.5f),
        1065.0f,
        500.0f,
    };
    UiDrawTexture(d->bg, panel, WHITE);
    float cx = panel.x + panel.width * 0.5f;
    int cost = d->stamina ? STAMINA_REFILL_GEM_COST : ENERGY_REFILL_GEM_COST;
    bool can_afford = player->gems >= cost;
    d->refill.label = LocGet(can_afford ? "REFRESH_REFILL" : "REFRESH_BUY_GEMS");

    UiDrawTextCentered(
        menu_font,
        LocGet(d->stamina ? "REFRESH_TIME_NEXT_STAMINA" : "REFRESH_TIME_NEXT_ENERGY"),
        (Rectangle){cx, panel.y + 81.0f, 0.0f, 0.0f},
        26.0f,
        WHITE
    );

    {
        float remaining = d->stamina ? PlayerStaminaRegenRemaining(player) : PlayerEnergyRegenRemaining(player);
        int total = (int)lrintf(remaining);
        const char *time = TextFormat("%02d:%02d:%02d", total / 3600, (total % 3600) / 60, total % 60);
        Rectangle timer = {panel.x, panel.y + 107.0f, 985.0f, 91.0f};
        UiDrawTextCentered(
            menu_font,
            time,
            (Rectangle){timer.x + timer.width * 0.5f, timer.y + timer.height * 0.5f, 0.0f, 0.0f},
            64.0f,
            WHITE
        );
        Rectangle resource_icon = {panel.x + 633.0f, panel.y + 102.0f, 76.0f, 76.0f};
        UiDrawTexture(d->resource_icon, resource_icon, WHITE);
    }

    UiDrawTextCentered(
        menu_font,
        LocGet(
            !can_afford ? (d->stamina ? "REFRESH_MORE_GEMS_STAMINA" : "REFRESH_MORE_GEMS_ENERGY")
                        : (d->stamina ? "REFRESH_STAMINA_PROMPT" : "REFRESH_ENERGY_PROMPT")
        ),
        (Rectangle){cx, panel.y + 242.0f, 0.0f, 0.0f},
        26.0f,
        WHITE
    );

    {
        const char *label = LocGet("REFRESH_COST"), *num = TextFormat("%d", cost);
        Rectangle title = {panel.x + 356.0f, panel.y + 260.0f, 142.0f, 101.0f};
        Rectangle amount = {panel.x + 515.0f, panel.y + 283.0f, 88.0f, 46.0f};
        Rectangle icon = {panel.x + 609.0f, panel.y + 260.0f, 101.0f, 101.0f};
        Rectangle brush = {panel.x + 515.0f, panel.y + 278.0f, 195.0f, 66.1017f};
        UiDrawTextRight(menu_font, label, title, 26.0f, WHITE);
        UiDrawTexture(d->brush, brush, WHITE);
        UiDrawTextRight(cool_font, num, amount, 32.0f, WHITE);
        UiDrawTexture(d->gem, icon, WHITE);
    }

    bool pool_not_full = d->stamina ? player->stamina < player->stamina_max : player->energy < player->energy_max;
    bool can_refill = pool_not_full && can_afford;
    Rectangle refill = {panel.x + 229.0f, panel.y + 375.0f, 304.0f, 76.0f};
    if (ButtonDrawEx(&d->refill, refill, can_refill)) {
        bool refilled = d->stamina ? PlayerRefillStamina(player) : PlayerRefillEnergy(player);
        if (refilled) {
            if (d->stamina) {
                GuiClickSoundPlay();
                GuiSoundPlay(SND_UI_POPUP_CLOSE);
                GuiSoundPlay(SND_STAMINA_REFRESH);
                GuiSoundPlay(SND_UI_POPUP_CLOSE);
            } else {
                GuiSoundPlay(SND_UI_POPUP_CLOSE);
                GuiSoundPlay(SND_STAMINA_REFRESH);
            }
            d->open = false;
        }
    }

    Rectangle navigate = {panel.x + 533.0f, panel.y + 375.0f, 304.0f, 76.0f};
    if (ButtonDrawEx(&d->navigate, navigate, allow_navigation)) {
        d->open = false;
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        return d->stamina ? ENERGY_DIALOG_ACTION_QUESTS : ENERGY_DIALOG_ACTION_PVP;
    }

    Rectangle close = {panel.x + 970.0f, panel.y - 22.0f, 82.0f, 82.0f};
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    Vector2 mouse = UiMouse();
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close) &&
                       !CheckCollisionPointRec(mouse, top_ui);
    if (!d->opening_release && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) d->scrim_armed = scrim_hover;

    bool close_clicked = ButtonDraw(&d->close, close);
    if (close_clicked) {
        if (d->stamina) GuiSoundPlay(SND_UI_POPUP_CLOSE);
        d->open = false;
    } else if (!d->opening_release && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && d->scrim_armed && scrim_hover) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        if (d->stamina) GuiSoundPlay(SND_UI_POPUP_CLOSE);
        d->open = false;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) d->scrim_armed = false;
    d->opening_release = false;
    return ENERGY_DIALOG_ACTION_NONE;
}
