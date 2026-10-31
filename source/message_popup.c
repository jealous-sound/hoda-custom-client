#define MESSAGE_POPUP_TEXT_CAP 256
#define MESSAGE_POPUP_LINES 8
#define MESSAGE_POPUP_HEIGHT 0.625f
#define MESSAGE_POPUP_ASPECT 2.13f

static void MessagePopupDrawLine(Font font, const char *line, Rectangle rect, float y, float size, Color color) {
    float width = MeasureTextEx(font, line, size, 1.0f).x;
    DrawTextEx(font, line, (Vector2){rect.x + (rect.width - width) * 0.5f, y}, size, 1.0f, color);
}

static int
MessagePopupWrap(Font font, const char *text, float width, float size, char lines[][MESSAGE_POPUP_TEXT_CAP], int cap) {
    int count = 0, used = 0;
    char line[MESSAGE_POPUP_TEXT_CAP] = "";
    for (const char *c = text;;) {
        bool hard_break = c[0] == '\n' || (c[0] == '\\' && c[1] == 'n');
        if (c[0] == '\0' || hard_break) {
            if (count < cap) StrCopy(lines[count++], MESSAGE_POPUP_TEXT_CAP, line);
            line[0] = '\0';
            used = 0;
            if (c[0] == '\0') break;
            c += c[0] == '\n' ? 1 : 2;
            continue;
        }
        const char *end = c;
        while (*end && *end != ' ' && *end != '\n' && !(end[0] == '\\' && end[1] == 'n')) end++;
        char candidate[MESSAGE_POPUP_TEXT_CAP];
        snprintf(candidate, sizeof(candidate), "%s%s%.*s", line, used ? " " : "", (int)(end - c), c);
        if (used && MeasureTextEx(font, candidate, size, 1.0f).x > width) {
            if (count < cap) StrCopy(lines[count++], MESSAGE_POPUP_TEXT_CAP, line);
            snprintf(candidate, sizeof(candidate), "%.*s", (int)(end - c), c);
        }
        StrCopy(line, sizeof(line), candidate);
        used = 1;
        c = *end == ' ' ? end + 1 : end;
    }
    return count;
}

static void MessagePopupDrawText(Font font, const char *text, Rectangle rect, float size, Color color) {
    char lines[MESSAGE_POPUP_LINES][MESSAGE_POPUP_TEXT_CAP];
    int count = MessagePopupWrap(font, text, rect.width, size, lines, MESSAGE_POPUP_LINES);
    float line_h = size * 1.25f;
    float y = rect.y + (rect.height - count * line_h) * 0.5f;
    for (int i = 0; i < count; i++) MessagePopupDrawLine(font, lines[i], rect, y + i * line_h, size, color);
}

static void MessagePopupDrawScrim(void) {
    DrawRectangle(0, 0, (int)UiWidth() + 1, (int)UiHeight() + 1, (Color){0, 0, 0, 150});
}

static Rectangle MessagePopupRect(void) {
    float height = UiHeight() * MESSAGE_POPUP_HEIGHT;
    float width = fminf(height * MESSAGE_POPUP_ASPECT, UiWidth());
    return (Rectangle){(UiWidth() - width) * 0.5f, (UiHeight() - height) * 0.5f, width, height};
}

static Rectangle MessagePopupChoiceRow(Rectangle popup) {
    return UiRect(UiRect(popup, 0.0f, 0.9f, 1.0f, 0.187f), 0.5f, 0.5f, 0.56f, 0.8f);
}

static Rectangle MessagePopupOkButton(Rectangle popup) {
    return UiRect(UiRect(popup, 0.0f, 0.9f, 1.0f, 0.187f), 0.5f, 0.5f, 0.28f, 0.8f);
}

static Rectangle MessagePopupContent(Rectangle popup) {
    return UiRect(popup, 0.5f, 0.5f, 0.8f, 0.8f);
}

static Rectangle MessagePopupDrawFrame(void) {
    Rectangle popup = MessagePopupRect();
    MessagePopupDrawScrim();
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), popup, WHITE);
    return popup;
}
