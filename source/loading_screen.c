#define LOADING_SCREEN_SPLASH APK_RESOURCE("Bg_DASplash")
#define LOADING_SCREEN_PLATE APK_RESOURCE("loader_message")
#define LOADING_INDICATOR_RING APK_RESOURCE("loadingSmooth")
#define LOADING_SCREEN_MESSAGE_KEY "LOADING_MESSAGE_LOGIN"
#define LOADING_PREFAB_CANVAS_HEIGHT 564.0f
#define LOADING_SCREEN_SPINNER_X 0.01f
#define LOADING_SCREEN_SPINNER_Y 0.02f
#define LOADING_SCREEN_SPINNER_HEIGHT 0.15f
#define LOADING_SCREEN_MESSAGE_X 0.5f
#define LOADING_SCREEN_MESSAGE_Y 0.75f
#define LOADING_SCREEN_MESSAGE_WIDTH 1.9f
#define LOADING_SCREEN_MESSAGE_HEIGHT 0.19f
#define LOADING_SCREEN_MESSAGE_ASPECT 5.535f
#define LOADING_SCREEN_LABEL_SIZE 0.8f
#define LOADING_SCREEN_LABEL_FONT_SIZE 22.0f
#define LOADING_LABEL_COLOR ((Color){230, 230, 230, 255})
#define LOADING_INDICATOR_TURNS_PER_SECOND 1.0f

typedef struct {
    Texture2D splash;
    Texture2D plate;
    Texture2D ring;
    const char *message;
} LoadingScreen;

static LoadingScreen loading_screen;

static Texture2D LoadingScreenTexture(const char *ref) {
    Image image = ResourceLoadImage(ref);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id == 0) TraceLog(LOG_WARNING, "LoadingScreen: failed to load %s", ref);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

static Rectangle LoadingFitAspect(Rectangle parent, float x, float y, float width, float height, float aspect) {
    float fitted_height = parent.height * height;
    float fitted_width = fminf(parent.width * width, fitted_height * aspect);
    return UiRect(parent, x, y, fitted_width, fitted_height);
}

void LoadingIndicatorDraw(Texture2D ring, Rectangle box) {
    float half_width = box.width * 0.5f, half_height = box.height * 0.5f;
    Rectangle src = {0.0f, 0.0f, (float)ring.width, (float)ring.height};
    Rectangle dst = {box.x + half_width, box.y + half_height, box.width, box.height};
    float counter_clockwise = -fmodf((float)GetTime() * LOADING_INDICATOR_TURNS_PER_SECOND * 360.0f, 360.0f);
    DrawTexturePro(ring, src, dst, (Vector2){half_width, half_height}, counter_clockwise, WHITE);
}

Rectangle LoadingIndicatorBox(Rectangle parent, float x, float y, float height) {
    return LoadingFitAspect(parent, x, y, 1.0f, height, 1.0f);
}

void LoadingScreenBegin(void) {
    loading_screen.splash = LoadingScreenTexture(LOADING_SCREEN_SPLASH);
    loading_screen.plate = LoadingScreenTexture(LOADING_SCREEN_PLATE);
    loading_screen.ring = LoadingScreenTexture(LOADING_INDICATOR_RING);
    loading_screen.message = NULL;
}

Texture2D LoadingScreenSplash(void) {
    return loading_screen.splash;
}

void LoadingScreenShowMessage(void) {
    loading_screen.message = LocGet(LOADING_SCREEN_MESSAGE_KEY);
}

static void LoadingScreenDraw(void) {
    Rectangle root = UiRoot();
    UiDrawTexture(loading_screen.splash, root, WHITE);
    LoadingIndicatorDraw(
        loading_screen.ring,
        LoadingIndicatorBox(root, LOADING_SCREEN_SPINNER_X, LOADING_SCREEN_SPINNER_Y, LOADING_SCREEN_SPINNER_HEIGHT)
    );
    if (!loading_screen.message) return;
    Rectangle container = LoadingFitAspect(
        root,
        LOADING_SCREEN_MESSAGE_X,
        LOADING_SCREEN_MESSAGE_Y,
        LOADING_SCREEN_MESSAGE_WIDTH,
        LOADING_SCREEN_MESSAGE_HEIGHT,
        LOADING_SCREEN_MESSAGE_ASPECT
    );
    UiDrawTexture(loading_screen.plate, container, WHITE);
    Rectangle label = UiRect(container, 0.5f, 0.5f, LOADING_SCREEN_LABEL_SIZE, LOADING_SCREEN_LABEL_SIZE);
    float font_size = LOADING_SCREEN_LABEL_FONT_SIZE * UiHeight() / LOADING_PREFAB_CANVAS_HEIGHT;
    UiDrawTextCentered(GuiFont(FNT_MENU), loading_screen.message, label, font_size, LOADING_LABEL_COLOR);
}

void LoadingScreenPresent(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    UiFrameBegin();
    LoadingScreenDraw();
    UiFrameEnd();
    EndDrawing();
}

void LoadingScreenEnd(void) {
    UnloadTexture(loading_screen.splash);
    UnloadTexture(loading_screen.plate);
    UnloadTexture(loading_screen.ring);
    loading_screen = (LoadingScreen){0};
}
