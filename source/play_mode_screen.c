#define PLAY_MODE_BUTTON APK_RESOURCE("Btn_CTA_gold")
#define PLAY_MODE_BUTTON_ACTIVE APK_RESOURCE("Btn_CTA_gold_active")
#define PLAY_MODE_LOCAL_LABEL "Локальная игра"
#define PLAY_MODE_ONLINE_LABEL "Онлайн-игра"
#define PLAY_MODE_LOCAL_HINT "Сохранения на этом компьютере"
#define PLAY_MODE_ONLINE_HINT "Сохранения на сервере, гильдии и чат"
#define PLAY_MODE_SCREENSHOT_ENV "HOD_SHOT"
#define PLAY_MODE_BUTTON_LABEL_SCALE 0.34f
#define PLAY_MODE_BUTTON_HEIGHT 0.12f
#define PLAY_MODE_BUTTON_FALLBACK_ASPECT 3.0f
#define PLAY_MODE_BUTTON_GAP_RATIO 0.6f
#define PLAY_MODE_BUTTONS_Y 0.8f
#define PLAY_MODE_HINT_GAP_RATIO 0.08f
#define PLAY_MODE_HINT_HEIGHT_RATIO 0.6f
#define PLAY_MODE_HINT_FONT_SIZE 19.0f
#define PLAY_MODE_HINT_COLOR ((Color){230, 230, 230, 255})
#define PLAY_MODE_SHADE_HEIGHT 0.4f
#define PLAY_MODE_SHADE_COLOR ((Color){0, 0, 0, 200})

typedef enum {
    PLAY_MODE_UNDECIDED,
    PLAY_MODE_LOCAL,
    PLAY_MODE_ONLINE,
    PLAY_MODE_QUIT,
} PlayModeChoice;

typedef struct {
    Texture2D splash;
    Font title_font;
    Font menu_font;
    Button local;
    Button online;
} PlayModeScreen;

static void PlayModeScreenLoad(PlayModeScreen *screen) {
    screen->splash = LoadingScreenTexture(LOADING_SCREEN_SPLASH);
    screen->title_font = ContentArtFont(FNT_COOL);
    screen->menu_font = ContentArtFont(FNT_MENU);
    Texture2D idle = LoadingScreenTexture(PLAY_MODE_BUTTON);
    Texture2D active = LoadingScreenTexture(PLAY_MODE_BUTTON_ACTIVE);
    Vector2 centre = {0.5f, 0.5f};
    screen->local = ButtonFromTextures(
        idle, active, PLAY_MODE_LOCAL_LABEL, screen->title_font, centre, PLAY_MODE_BUTTON_LABEL_SCALE
    );
    screen->online = ButtonFromTextures(
        idle, active, PLAY_MODE_ONLINE_LABEL, screen->title_font, centre, PLAY_MODE_BUTTON_LABEL_SCALE
    );
    ResourceShutdown();
}

static void PlayModeScreenUnload(PlayModeScreen *screen) {
    Texture2D textures[] = {screen->splash, screen->local.idle, screen->local.active};
    for (size_t i = 0; i < sizeof(textures) / sizeof(textures[0]); i++)
        if (textures[i].id) UnloadTexture(textures[i]);
    if (screen->title_font.texture.id) UnloadFont(screen->title_font);
    if (screen->menu_font.texture.id) UnloadFont(screen->menu_font);
    *screen = (PlayModeScreen){0};
}

static float PlayModeButtonAspect(const PlayModeScreen *screen) {
    Texture2D idle = screen->local.idle;
    return idle.height > 0 ? (float)idle.width / (float)idle.height : PLAY_MODE_BUTTON_FALLBACK_ASPECT;
}

static void PlayModeDrawHint(const PlayModeScreen *screen, const char *hint, Rectangle button) {
    float gap = button.height * PLAY_MODE_HINT_GAP_RATIO;
    Rectangle box = {
        button.x, button.y + button.height + gap, button.width, button.height * PLAY_MODE_HINT_HEIGHT_RATIO
    };
    float size = PLAY_MODE_HINT_FONT_SIZE * ContentCanvasScale();
    MessagePopupDrawText(screen->menu_font, hint, box, size, PLAY_MODE_HINT_COLOR);
}

static PlayModeChoice PlayModeScreenDraw(PlayModeScreen *screen) {
    Rectangle root = UiRoot();
    UiDrawTexture(screen->splash, root, WHITE);
    Rectangle shade = UiRect(root, 0.0f, 1.0f, 1.0f, PLAY_MODE_SHADE_HEIGHT);
    DrawRectangleGradientV(
        (int)shade.x, (int)shade.y, (int)ceilf(shade.width), (int)ceilf(shade.height), BLANK, PLAY_MODE_SHADE_COLOR
    );
    float height = root.height * PLAY_MODE_BUTTON_HEIGHT;
    float width = height * PlayModeButtonAspect(screen);
    float gap = height * PLAY_MODE_BUTTON_GAP_RATIO;
    Rectangle row = UiRect(root, 0.5f, PLAY_MODE_BUTTONS_Y, width * 2.0f + gap, height);
    Rectangle local = {row.x, row.y, width, height};
    Rectangle online = {row.x + width + gap, row.y, width, height};
    PlayModeDrawHint(screen, PLAY_MODE_LOCAL_HINT, local);
    PlayModeDrawHint(screen, PLAY_MODE_ONLINE_HINT, online);
    bool local_chosen = ButtonDraw(&screen->local, local);
    bool online_chosen = ButtonDraw(&screen->online, online);
    if (local_chosen) return PLAY_MODE_LOCAL;
    if (online_chosen) return PLAY_MODE_ONLINE;
    return IsKeyPressed(KEY_ESCAPE) ? PLAY_MODE_QUIT : PLAY_MODE_UNDECIDED;
}

static bool PlayModeScreenRun(void) {
    const char *forced = BackendUrlOverride();
    if (forced) {
        BackendConnect(forced);
        return true;
    }
    const char *server = BackendDefaultUrl();
    if (!server || EnvFlag(PLAY_MODE_SCREENSHOT_ENV)) return true;
    PlayModeScreen screen = {0};
    PlayModeScreenLoad(&screen);
    PlayModeChoice choice = PLAY_MODE_UNDECIDED;
    while (choice == PLAY_MODE_UNDECIDED && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        UiFrameBegin();
        choice = PlayModeScreenDraw(&screen);
        UiFrameEnd();
        EndDrawing();
    }
    PlayModeScreenUnload(&screen);
    if (choice == PLAY_MODE_ONLINE) BackendConnect(server);
    return choice == PLAY_MODE_LOCAL || choice == PLAY_MODE_ONLINE;
}
