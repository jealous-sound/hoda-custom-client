#define WINDOW_MONITOR_SHARE_LIMIT 0.8f

#if defined(__APPLE__)
#define WINDOW_PIXEL_DENSITY_FLAGS FLAG_WINDOW_HIGHDPI

static void WindowGrowToMonitorDensity(void) {
}
#else
#define WINDOW_PIXEL_DENSITY_FLAGS 0

static float WindowGrowthForDensity(Vector2 density, Vector2 window, Vector2 monitor) {
    float wanted = fminf(density.x, density.y);
    float room = fminf(monitor.x / window.x, monitor.y / window.y) * WINDOW_MONITOR_SHARE_LIMIT;
    return fminf(wanted, room);
}

static void WindowGrowToMonitorDensity(void) {
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
