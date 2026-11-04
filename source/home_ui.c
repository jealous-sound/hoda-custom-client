typedef enum {
    HOME_NONE = 0,
    HOME_STORE,
    HOME_HISTORY,
    HOME_HEROES,
    HOME_QUESTS,
    HOME_PVP,
} HomeButton;

typedef struct {
    Button store;
    Button history;
    Button heroes;
    Button quests;
    Button pvp;
} HomeUI;

void HomeUIInitialize(HomeUI *hui) {
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.82f};
    float scale = 0.17f;
    hui->store = ButtonLoadText(GUI_BTN_STORE, GUI_BTN_STORE_ON, "Магазин", font, anchor, scale);
    hui->history = ButtonLoadText(GUI_BTN_HISTORY, GUI_BTN_HISTORY_ON, "Цели", font, anchor, scale);
    hui->heroes = ButtonLoadText(GUI_BTN_HEROES, GUI_BTN_HEROES_ON, "Герои", font, anchor, scale);
    hui->quests = ButtonLoadText(GUI_BTN_HOMEQUEST, GUI_BTN_HOMEQUEST_ON, "Задание", font, anchor, scale);
    hui->pvp = ButtonLoadText(GUI_BTN_PVP, GUI_BTN_PVP_ON, "PvP", font, anchor, scale);
    ButtonSetClickSound(&hui->store, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->history, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->heroes, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->quests, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->pvp, SND_UI_BIG_BUTTON);
}

HomeButton HomeUIDraw(HomeUI *hui, bool interactive, int heroes_badge, int missions_badge) {
    float size = 88.0f;
    float pad = 12.0f;
    float gap = 6.0f;
    float y = UiHeight() - size - pad;
    float screen_w = UiWidth();

    HomeButton clicked = HOME_NONE;

    Rectangle store = {pad, y, size, size};
    TutorialAnchorSet(TUTORIAL_ANCHOR_HOME_STORE, store);
    if (ButtonDrawEx(&hui->store, store, interactive)) clicked = HOME_STORE;
    Rectangle history = {pad + size + gap, y, size, size};
    if (ButtonDrawEx(&hui->history, history, interactive)) clicked = HOME_HISTORY;
    ButtonDrawBadge(history, missions_badge);

    Rectangle heroes = {(screen_w - size) * 0.5f, y, size, size};
    if (ButtonDrawEx(&hui->heroes, heroes, interactive)) clicked = HOME_HEROES;
    ButtonDrawBadge(heroes, heroes_badge);

    float pvp_x = screen_w - pad - size;
    float quests_x = pvp_x - gap - size;
    Rectangle quests = {quests_x, y, size, size};
    TutorialAnchorSet(TUTORIAL_ANCHOR_HOME_QUESTS, quests);
    if (ButtonDrawEx(&hui->quests, quests, interactive)) clicked = HOME_QUESTS;
    if (ButtonDrawEx(&hui->pvp, (Rectangle){pvp_x, y, size, size}, interactive)) clicked = HOME_PVP;

    return clicked;
}
