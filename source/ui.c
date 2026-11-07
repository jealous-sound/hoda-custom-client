#define UI_DESIGN_W 1280.0f
#define UI_DESIGN_H 720.0f

float UiScale(void) {
    return GetScreenHeight() / UI_DESIGN_H;
}

float UiWidth(void) {
    return GetScreenWidth() / UiScale();
}

float UiHeight(void) {
    return GetScreenHeight() / UiScale();
}

Rectangle UiRoot(void) {
    return (Rectangle){0.0f, 0.0f, UiWidth(), UiHeight()};
}

static bool ui_input_blocked = false;
static bool ui_input_suspended = false;
static bool ui_input_window_active = false;
static Rectangle ui_input_window = {0};

void UiSetInputBlocked(bool blocked) {
    ui_input_blocked = blocked;
}

void UiSetInputSuspended(bool suspended) {
    ui_input_suspended = suspended;
}

bool UiInputBlocked(void) {
    return ui_input_blocked || ui_input_suspended;
}

void UiSetInputWindow(bool active, Rectangle window) {
    ui_input_window_active = active;
    ui_input_window = window;
}

Vector2 UiPointer(void) {
    Vector2 m = GetMousePosition();
    float s = UiScale();
    return (Vector2){m.x / s, m.y / s};
}

static bool UiPointerOutsideWindow(void) {
    return ui_input_window_active && !CheckCollisionPointRec(UiPointer(), ui_input_window);
}

bool UiClicksReachScreen(void) {
    return !UiInputBlocked() && !UiPointerOutsideWindow();
}

Vector2 UiMouse(void) {
    if (UiInputBlocked() || UiPointerOutsideWindow()) return (Vector2){-1.0e6f, -1.0e6f};
    return UiPointer();
}

Vector2 UiMouseDelta(void) {
    if (UiInputBlocked() || UiPointerOutsideWindow()) return (Vector2){0, 0};
    Vector2 d = GetMouseDelta();
    float s = UiScale();
    return (Vector2){d.x / s, d.y / s};
}

void UiFrameBegin(void) {
    BeginMode2D((Camera2D){.zoom = UiScale()});
}

void UiFrameEnd(void) {
    EndMode2D();
}

void UiDrawTexture(Texture2D texture, Rectangle dst, Color tint) {
    Rectangle src = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
    DrawTexturePro(texture, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

void UiDrawTextCentered(Font font, const char *text, Rectangle rect, float size, Color color) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    Vector2 at = {rect.x + (rect.width - measured.x) * 0.5f, rect.y + (rect.height - measured.y) * 0.5f};
    DrawTextEx(font, text, at, size, 1.0f, color);
}

void UiDrawTextLeftMiddle(Font font, const char *text, Rectangle rect, float size, Color color) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    DrawTextEx(font, text, (Vector2){rect.x, rect.y + (rect.height - measured.y) * 0.5f}, size, 1.0f, color);
}

void UiDrawTextRight(Font font, const char *text, Rectangle rect, float size, Color color) {
    Vector2 measured = MeasureTextEx(font, text, size, 1.0f);
    Vector2 at = {rect.x + rect.width - measured.x, rect.y + (rect.height - measured.y) * 0.5f};
    DrawTextEx(font, text, at, size, 1.0f, color);
}

static float UiRectValue(float parent, float v) {
    return (v > -2.0f && v < 2.0f) ? parent * v : v;
}

Rectangle UiRect(Rectangle parent, float x, float y, float w, float h) {
    Rectangle r;
    r.width = UiRectValue(parent.width, w);
    r.height = UiRectValue(parent.height, h);
    r.x = parent.x + UiRectValue(parent.width - r.width, x);
    r.y = parent.y + UiRectValue(parent.height - r.height, y);
    return r;
}

typedef struct {
    int first;
    int last;
} UiCodepointRange;

static const UiCodepointRange UI_IDEOGRAPHIC_RANGES[] = {
    {0x2E80, 0x2FFF},
    {0x3000, 0x30FF},
    {0x3100, 0x31FF},
    {0x3400, 0x4DBF},
    {0x4E00, 0x9FFF},
    {0xF900, 0xFAFF},
    {0xFF00, 0xFFEF},
};

static const int UI_NO_BREAK_BEFORE[] = {
    0x3001,
    0x3002,
    0x300D,
    0x300F,
    0x3011,
    0x3015,
    0x30FC,
    0xFF01,
    0xFF09,
    0xFF0C,
    0xFF0E,
    0xFF1A,
    0xFF1B,
    0xFF1F,
    0x2026,
    0x30FB,
    0x309D,
    0x309E,
};

bool UiIdeographic(int codepoint) {
    for (size_t i = 0; i < sizeof(UI_IDEOGRAPHIC_RANGES) / sizeof(UI_IDEOGRAPHIC_RANGES[0]); i++)
        if (codepoint >= UI_IDEOGRAPHIC_RANGES[i].first && codepoint <= UI_IDEOGRAPHIC_RANGES[i].last) return true;
    return false;
}

static bool UiNoBreakBefore(int codepoint) {
    for (size_t i = 0; i < sizeof(UI_NO_BREAK_BEFORE) / sizeof(UI_NO_BREAK_BEFORE[0]); i++)
        if (codepoint == UI_NO_BREAK_BEFORE[i]) return true;
    return false;
}

static int UiCodepointAt(const char *text, int *bytes) {
    int size = 0;
    int codepoint = GetCodepointNext(text, &size);
    *bytes = size > 0 ? size : 1;
    return codepoint;
}

const char *UiBreakUnitEnd(const char *text) {
    if (!*text) return text;
    int bytes = 0;
    int codepoint = UiCodepointAt(text, &bytes);
    const char *end = text + bytes;
    if (UiIdeographic(codepoint)) {
        while (*end && UiNoBreakBefore(UiCodepointAt(end, &bytes))) end += bytes;
        return end;
    }
    while (*end && *end != ' ' && *end != '\n' && !(end[0] == '\\' && end[1] == 'n') &&
           !UiIdeographic(UiCodepointAt(end, &bytes)))
        end += bytes;
    return end;
}
