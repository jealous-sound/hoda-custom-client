typedef enum {
    HOME_NONE = 0,
    HOME_STORE,
    HOME_HISTORY,
    HOME_HEROES,
    HOME_QUESTS,
    HOME_PVP,
    HOME_GUILD,
    HOME_GUILD_LOCKED,
    HOME_GUILD_CHAT,
    HOME_GUILD_CHAT_LOCKED,
    HOME_EXPEDITIONS,
} HomeButton;

#define HOME_GUILD_BUTTON_X 13.0f
#define HOME_GUILD_BUTTON_Y 324.0f
#define HOME_GUILD_CHAT_BUTTON_Y 421.0f
#define HOME_GUILD_BUTTON_SIZE 73.0f
#define HOME_GUILD_BADGE_SHOWN_MAX 3

typedef struct {
    Button store;
    Button history;
    Button heroes;
    Button quests;
    Button pvp;
    Button guild;
    Button guild_locked;
    Button guild_chat;
    Button guild_chat_locked;
    Button expeditions;
    bool expeditions_shown;
    int expeditions_badge;
} HomeUI;

void HomeUIInitialize(HomeUI *hui) {
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.82f};
    float scale = 0.17f;
    hui->store =
        ButtonLoadText(GUI_BTN_STORE, GUI_BTN_STORE_ON, LocGet("BUTTON_MAINSCREEN_STORE"), font, anchor, scale);
    hui->history =
        ButtonLoadText(GUI_BTN_HISTORY, GUI_BTN_HISTORY_ON, LocGet("BUTTON_MAINSCREEN_HISTORY"), font, anchor, scale);
    hui->heroes =
        ButtonLoadText(GUI_BTN_HEROES, GUI_BTN_HEROES_ON, LocGet("BUTTON_MAINSCREEN_HEROES"), font, anchor, scale);
    hui->quests =
        ButtonLoadText(GUI_BTN_HOMEQUEST, GUI_BTN_HOMEQUEST_ON, LocGet("NEXUS_BUTTON_LOC_STRING"), font, anchor, scale);
    hui->pvp = ButtonLoadText(GUI_BTN_PVP, GUI_BTN_PVP_ON, LocGet("BUTTON_MAINSCREEN_BATTLE"), font, anchor, scale);
    ButtonSetClickSound(&hui->store, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->history, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->heroes, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->quests, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->pvp, SND_UI_BIG_BUTTON);
    hui->guild = ButtonLoad(GUI_GUILD_BUTTON, GUI_GUILD_BUTTON);
    hui->guild_locked = ButtonLoad(GUI_GUILD_BUTTON_OFF, GUI_GUILD_BUTTON_OFF);
    hui->guild_chat = ButtonLoad(GUI_GUILD_CHAT_BUTTON, GUI_GUILD_CHAT_BUTTON);
    hui->guild_chat_locked = ButtonLoad(GUI_GUILD_CHAT_BUTTON_OFF, GUI_GUILD_CHAT_BUTTON_OFF);
    ButtonSetClickSound(&hui->guild, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&hui->guild_chat, SND_UI_BIG_BUTTON);
    Texture2D expeditions = ExpeditionTexture(EXPEDITIONS_HOME_ART);
    Texture2D expeditions_on = ExpeditionTexture(EXPEDITIONS_HOME_ART_ON);
    hui->expeditions =
        expeditions.id && expeditions_on.id
            ? ButtonFromTextures(expeditions, expeditions_on, LocGet("HODA_EXPEDITIONS_BUTTON"), font, anchor, scale)
            : ButtonFromTextures(
                  GuiTex(GUI_BTN_GREY),
                  GuiTex(GUI_BTN_GREY_ON),
                  LocGet("HODA_EXPEDITIONS_BUTTON"),
                  font,
                  (Vector2){0.5f, 0.5f},
                  0.2f
              );
    hui->expeditions.click = GuiSound(SND_UI_BIG_BUTTON);
}

static void HomeUIDrawGuildBadge(Rectangle button, int count) {
    if (count <= 0) return;
    Rectangle badge = {button.x + 48.0f, button.y - 13.0f, 33.0f, 33.0f};
    UiDrawTexture(GuiTex(GUI_GUILD_BADGE), badge, WHITE);
    const char *text =
        count > HOME_GUILD_BADGE_SHOWN_MAX ? TextFormat("%d+", HOME_GUILD_BADGE_SHOWN_MAX) : TextFormat("%d", count);
    UiDrawTextCentered(GuiFont(FNT_MENU), text, badge, 20.0f, WHITE);
}

HomeButton HomeUIDrawGuild(HomeUI *hui, bool interactive, bool unlocked, bool member, int chat_badge) {
    HomeButton clicked = HOME_NONE;
    Rectangle guild = {HOME_GUILD_BUTTON_X, HOME_GUILD_BUTTON_Y, HOME_GUILD_BUTTON_SIZE, HOME_GUILD_BUTTON_SIZE};
    Rectangle chat = {HOME_GUILD_BUTTON_X, HOME_GUILD_CHAT_BUTTON_Y, HOME_GUILD_BUTTON_SIZE, HOME_GUILD_BUTTON_SIZE};
    if (ButtonDrawEx(unlocked ? &hui->guild : &hui->guild_locked, guild, interactive))
        clicked = unlocked ? HOME_GUILD : HOME_GUILD_LOCKED;
    if (ButtonDrawEx(member ? &hui->guild_chat : &hui->guild_chat_locked, chat, interactive))
        clicked = member ? HOME_GUILD_CHAT : HOME_GUILD_CHAT_LOCKED;
    if (member) HomeUIDrawGuildBadge(chat, chat_badge);
    return clicked;
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
    if (hui->expeditions_shown) {
        Rectangle expeditions = {pad + (size + gap) * 2.0f, y, size, size};
        if (ButtonDrawEx(&hui->expeditions, expeditions, interactive)) clicked = HOME_EXPEDITIONS;
        ButtonDrawBadge(expeditions, hui->expeditions_badge);
    }

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
