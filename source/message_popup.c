#define MESSAGE_POPUP_TEXT_CAP 256
#define MESSAGE_POPUP_LINES 8
#define MESSAGE_POPUP_HEIGHT 0.625f
#define MESSAGE_POPUP_ASPECT 2.13f

#define MESSAGE_POPUP_SPACING 1.0f
#define MESSAGE_POPUP_LINE_SPACING 1.25f
#define MESSAGE_POPUP_TITLE_SIZE 30.0f
#define MESSAGE_POPUP_MESSAGE_SIZE 22.0f
#define MESSAGE_POPUP_TEXT_COLOR ((Color){242, 236, 218, 255})

static float
MessagePopupRichRun(Font font, const char *line, Vector2 at, float size, Color base, Color *color, bool draw) {
    float x = 0.0f;
    for (const char *c = line;;) {
        HeroesRichToken token = HeroesRichNextToken(c, base);
        if (token.kind == HEROES_RICH_END) break;
        if (token.kind == HEROES_RICH_COLOR_OPEN) *color = token.color;
        if (token.kind == HEROES_RICH_COLOR_CLOSE) *color = base;
        if (token.kind == HEROES_RICH_SPACE || token.kind == HEROES_RICH_TEXT) {
            char piece[MESSAGE_POPUP_TEXT_CAP];
            snprintf(piece, sizeof(piece), "%.*s", token.bytes, c);
            if (draw) DrawTextEx(font, piece, (Vector2){at.x + x, at.y}, size, MESSAGE_POPUP_SPACING, *color);
            x += MeasureTextEx(font, piece, size, MESSAGE_POPUP_SPACING).x + MESSAGE_POPUP_SPACING;
        }
        c += token.bytes;
    }
    return x > 0.0f ? x - MESSAGE_POPUP_SPACING : 0.0f;
}

static float MessagePopupRichWidth(Font font, const char *line, float size) {
    Color unused = WHITE;
    return MessagePopupRichRun(font, line, (Vector2){0.0f, 0.0f}, size, WHITE, &unused, false);
}

static void
MessagePopupDrawLine(Font font, const char *line, Rectangle rect, float y, float size, Color base, Color *color) {
    float width = MessagePopupRichWidth(font, line, size);
    MessagePopupRichRun(font, line, (Vector2){rect.x + (rect.width - width) * 0.5f, y}, size, base, color, true);
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
        if (used && MessagePopupRichWidth(font, candidate, size) > width) {
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
    float line_h = size * MESSAGE_POPUP_LINE_SPACING;
    float y = rect.y + (rect.height - count * line_h) * 0.5f;
    Color current = color;
    for (int i = 0; i < count; i++) MessagePopupDrawLine(font, lines[i], rect, y + i * line_h, size, color, &current);
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

static Rectangle MessagePopupDrawFrameWith(Texture2D frame) {
    Rectangle popup = MessagePopupRect();
    MessagePopupDrawScrim();
    UiDrawTexture(frame, popup, WHITE);
    return popup;
}

static Rectangle MessagePopupDrawFrame(void) {
    return MessagePopupDrawFrameWith(GuiTex(GUI_STORE_OVERLAY));
}

static bool MessagePopupDrawAlert(
    Texture2D frame, Font title_font, Font body_font, const char *title, const char *message, Button *button
) {
    Rectangle popup = MessagePopupDrawFrameWith(frame);
    Rectangle content = MessagePopupContent(popup);
    UiDrawTextCentered(
        title_font, title, UiRect(content, 0.5f, 0.0f, 1.0f, 0.22f), MESSAGE_POPUP_TITLE_SIZE, MESSAGE_POPUP_TEXT_COLOR
    );
    MessagePopupDrawText(
        body_font,
        message,
        UiRect(content, 0.5f, 0.55f, 1.0f, 0.6f),
        MESSAGE_POPUP_MESSAGE_SIZE,
        MESSAGE_POPUP_TEXT_COLOR
    );
    bool pressed = ButtonDraw(button, MessagePopupOkButton(popup));
    return pressed || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
}
