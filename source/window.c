#define WINDOW_MONITOR_SHARE_LIMIT 0.8f

#if defined(__APPLE__)
#define WINDOW_PIXEL_DENSITY_FLAGS FLAG_WINDOW_HIGHDPI

static void WindowFitToDisplay(void) {
}

static bool WindowFullscreen(void) {
    return PlatformNativeFullscreen(GetWindowHandle());
}

static void WindowSetFullscreen(bool fullscreen) {
    if (WindowFullscreen() != fullscreen) PlatformToggleNativeFullscreen(GetWindowHandle());
}

static void WindowSendToBack(void) {
    PlatformHideApplication();
}
#else
#define WINDOW_PIXEL_DENSITY_FLAGS 0

#ifdef __EMSCRIPTEN__
#include <emscripten/em_js.h>
#include <emscripten/html5.h>

#define WINDOW_RESIZE_FLAGS 0

// clang-format off
EM_JS(int, WindowPageTurned, (void), { return Module.hodPageTurned ? 1 : 0; });
EM_JS(int, WindowPageWidth, (void), { return Module.canvas.clientWidth; });
EM_JS(int, WindowPageHeight, (void), { return Module.canvas.clientHeight; });
// clang-format on

static EM_BOOL WindowFillPage(int event_type, const EmscriptenUiEvent *event, void *user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;
    int width = WindowPageWidth(), height = WindowPageHeight();
    if (width >= 1 && height >= 1) SetWindowSize(width, height);
    return EM_TRUE;
}

static void WindowFitToDisplay(void) {
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, WindowFillPage);
    WindowFillPage(EMSCRIPTEN_EVENT_RESIZE, NULL, NULL);
}
#else
static float WindowGrowthForDensity(Vector2 density, Vector2 window, Vector2 monitor) {
    float wanted = fminf(density.x, density.y);
    float room = fminf(monitor.x / window.x, monitor.y / window.y) * WINDOW_MONITOR_SHARE_LIMIT;
    return fminf(wanted, room);
}

static void WindowFitToDisplay(void) {
    int monitor = GetCurrentMonitor();
    Vector2 window = {(float)GetScreenWidth(), (float)GetScreenHeight()};
    Vector2 monitor_size = {(float)GetMonitorWidth(monitor), (float)GetMonitorHeight(monitor)};
    float growth = WindowGrowthForDensity(GetWindowScaleDPI(), window, monitor_size);
    if (growth <= 1.0f) return;
    int width = (int)(window.x * growth), height = (int)(window.y * growth);
    Vector2 origin = GetMonitorPosition(monitor);
    SetWindowSize(width, height);
    SetWindowPosition(
        (int)origin.x + ((int)monitor_size.x - width) / 2, (int)origin.y + ((int)monitor_size.y - height) / 2
    );
}
#endif

static bool WindowFullscreen(void) {
    return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
}

static void WindowSetFullscreen(bool fullscreen) {
    if (WindowFullscreen() != fullscreen) ToggleBorderlessWindowed();
    if (fullscreen) ClearWindowState(FLAG_WINDOW_TOPMOST);
}

static void WindowSendToBack(void) {
    MinimizeWindow();
}
#endif

#ifndef __EMSCRIPTEN__
#define WINDOW_RESIZE_FLAGS FLAG_WINDOW_RESIZABLE

static int WindowPageTurned(void) {
    return 0;
}
#endif

static Vector2 WindowPointer(void) {
    Vector2 page = GetMousePosition();
    if (!WindowPageTurned()) return page;
    float width = (float)GetScreenWidth(), height = (float)GetScreenHeight();
    return (Vector2){page.y * width / height, height - page.x * height / width};
}

static Vector2 WindowPointerDelta(void) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return (Vector2){0.0f, 0.0f};
    Vector2 page = GetMouseDelta();
    if (!WindowPageTurned()) return page;
    float width = (float)GetScreenWidth(), height = (float)GetScreenHeight();
    return (Vector2){page.y * width / height, -page.x * height / width};
}
