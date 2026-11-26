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
#include <GLES3/gl3.h>
#include <emscripten/em_js.h>
#include <emscripten/html5.h>

#define WINDOW_RESIZE_FLAGS 0
#define WINDOW_SAMPLING_FLAGS 0
#define WINDOW_PAGE_DENSITY_LIMIT 2.0
#define WINDOW_FRAME_SAMPLES 4

typedef struct {
    GLuint framebuffer;
    GLuint color;
    GLuint depth;
    int width;
    int height;
} WindowFrame;

static WindowFrame window_frame;

// clang-format off
EM_JS(int, WindowPageTurned, (void), { return Module.hodPageTurned ? 1 : 0; });
EM_JS(int, WindowPageWidth, (void), { return Module.canvas.clientWidth; });
EM_JS(int, WindowPageHeight, (void), { return Module.canvas.clientHeight; });
EM_JS(double, WindowPageDensity, (void), { return window.devicePixelRatio || 1; });
// clang-format on

static int WindowDevicePixels(int page_pixels) {
    double density = fmin(fmax(WindowPageDensity(), 1.0), WINDOW_PAGE_DENSITY_LIMIT);
    return (int)lround(page_pixels * density);
}

static EM_BOOL WindowFillPage(int event_type, const EmscriptenUiEvent *event, void *user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;
    int width = WindowPageWidth(), height = WindowPageHeight();
    if (width >= 1 && height >= 1) SetWindowSize(WindowDevicePixels(width), WindowDevicePixels(height));
    return EM_TRUE;
}

static void WindowFrameRelease(WindowFrame *frame) {
    if (frame->framebuffer) glDeleteFramebuffers(1, &frame->framebuffer);
    if (frame->color) glDeleteRenderbuffers(1, &frame->color);
    if (frame->depth) glDeleteRenderbuffers(1, &frame->depth);
    *frame = (WindowFrame){0};
}

static GLuint WindowFrameStorage(GLenum format, GLsizei samples, int width, int height) {
    GLuint storage = 0;
    glGenRenderbuffers(1, &storage);
    glBindRenderbuffer(GL_RENDERBUFFER, storage);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return storage;
}

static bool WindowFrameFit(WindowFrame *frame, int width, int height) {
    if (frame->framebuffer && frame->width == width && frame->height == height) return true;
    WindowFrameRelease(frame);
    if (width < 1 || height < 1) return false;
    GLint most_samples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &most_samples);
    GLsizei samples = most_samples < WINDOW_FRAME_SAMPLES ? most_samples : WINDOW_FRAME_SAMPLES;
    frame->color = WindowFrameStorage(GL_RGBA8, samples, width, height);
    frame->depth = WindowFrameStorage(GL_DEPTH24_STENCIL8, samples, width, height);
    glGenFramebuffers(1, &frame->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frame->framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, frame->color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frame->depth);
    bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) {
        WindowFrameRelease(frame);
        return false;
    }
    frame->width = width;
    frame->height = height;
    return true;
}

static void WindowBeginFrame(void) {
    BeginDrawing();
    if (WindowFrameFit(&window_frame, GetRenderWidth(), GetRenderHeight()))
        glBindFramebuffer(GL_FRAMEBUFFER, window_frame.framebuffer);
}

static void WindowEndFrame(void) {
    if (window_frame.framebuffer) {
        rlDrawRenderBatchActive();
        rlDisableScissorTest();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, window_frame.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0,
            0,
            window_frame.width,
            window_frame.height,
            0,
            0,
            window_frame.width,
            window_frame.height,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    EndDrawing();
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
#define WINDOW_SAMPLING_FLAGS FLAG_MSAA_4X_HINT

static int WindowPageTurned(void) {
    return 0;
}

static void WindowBeginFrame(void) {
    BeginDrawing();
}

static void WindowEndFrame(void) {
    EndDrawing();
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
