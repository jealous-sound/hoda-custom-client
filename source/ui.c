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
void UiSetInputBlocked(bool blocked) {
    ui_input_blocked = blocked;
}

bool UiInputBlocked(void) {
    return ui_input_blocked;
}

Vector2 UiMouse(void) {
    if (ui_input_blocked) return (Vector2){-1.0e6f, -1.0e6f};
    Vector2 m = GetMousePosition();
    float s = UiScale();
    return (Vector2){m.x / s, m.y / s};
}

Vector2 UiMouseDelta(void) {
    if (ui_input_blocked) return (Vector2){0, 0};
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
