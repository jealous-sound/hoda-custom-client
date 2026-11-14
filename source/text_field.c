#define TEXT_FIELD_CAPACITY 1024
#define TEXT_FIELD_CARET_PERIOD 1.0f
#define TEXT_FIELD_PADDING 10.0f
#define TEXT_FIELD_SLICE 10
#define TEXT_FIELD_FORBIDDEN "<>"
#define TEXT_FIELD_MASK_GLYPH '*'

static const Color TEXT_FIELD_BACKGROUND = {24, 20, 16, 235};
static const Color TEXT_FIELD_BACKGROUND_FOCUSED = {40, 33, 25, 245};
static const Color TEXT_FIELD_TEXT = {242, 236, 218, 255};
static const Color TEXT_FIELD_PLACEHOLDER = {150, 142, 128, 255};

typedef struct {
    char text[TEXT_FIELD_CAPACITY];
    int max_runes;
    bool digits_only;
    bool focused;
    bool read_only;
    bool masked;
    bool stripped;
    float time;
} TextField;

void TextFieldInit(TextField *field, int max_runes, bool digits_only) {
    *field = (TextField){.max_runes = max_runes, .digits_only = digits_only};
}

int TextFieldRuneCount(const char *text) {
    int count = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c; c++) count += (*c & 0xC0) != 0x80;
    return count;
}

void TextFieldSet(TextField *field, const char *text) {
    StrCopy(field->text, sizeof(field->text), text ? text : "");
    field->stripped = false;
}

void TextFieldClear(TextField *field) {
    field->text[0] = '\0';
    field->stripped = false;
}

static bool TextFieldAccepts(const TextField *field, int codepoint) {
    if (codepoint < ' ') return false;
    if (field->digits_only) return codepoint >= '0' && codepoint <= '9';
    if (codepoint < 0x80 && strchr(TEXT_FIELD_FORBIDDEN, codepoint)) return false;
    return codepoint <= 0xFFFF;
}

static void TextFieldInsert(TextField *field, int codepoint) {
    if (!TextFieldAccepts(field, codepoint)) {
        field->stripped = true;
        return;
    }
    if (field->max_runes > 0 && TextFieldRuneCount(field->text) >= field->max_runes) return;
    int bytes = 0;
    const char *encoded = CodepointToUTF8(codepoint, &bytes);
    int length = (int)strlen(field->text);
    if (length + bytes >= (int)sizeof(field->text)) return;
    memcpy(field->text + length, encoded, (size_t)bytes);
    field->text[length + bytes] = '\0';
}

static void TextFieldErase(TextField *field) {
    int length = (int)strlen(field->text);
    while (length > 0 && ((unsigned char)field->text[length - 1] & 0xC0) == 0x80) length--;
    if (length > 0) length--;
    field->text[length] = '\0';
}

static void TextFieldPaste(TextField *field) {
    const char *clip = GetClipboardText();
    if (!clip) return;
    for (const char *c = clip; *c;) {
        int bytes = 0;
        int codepoint = GetCodepointNext(c, &bytes);
        if (bytes <= 0) break;
        TextFieldInsert(field, codepoint == '\n' || codepoint == '\r' || codepoint == '\t' ? ' ' : codepoint);
        c += bytes;
    }
}

static bool TextFieldPasteChord(void) {
    bool modifier = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER) || IsKeyDown(KEY_LEFT_CONTROL) ||
                    IsKeyDown(KEY_RIGHT_CONTROL);
    return modifier && IsKeyPressed(KEY_V);
}

static void TextFieldType(TextField *field) {
    for (int codepoint = GetCharPressed(); codepoint > 0; codepoint = GetCharPressed())
        TextFieldInsert(field, codepoint);
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) TextFieldErase(field);
    if (TextFieldPasteChord()) TextFieldPaste(field);
}

void TextFieldBlur(TextField *field) {
    field->focused = false;
}

void TextFieldFocus(TextField *field) {
    field->focused = true;
    field->time = 0.0f;
    while (GetCharPressed() > 0) {}
}

static const char *TextFieldShownText(const TextField *field) {
    if (!field->masked) return field->text;
    static char mask[TEXT_FIELD_CAPACITY];
    int runes = TextFieldRuneCount(field->text);
    memset(mask, TEXT_FIELD_MASK_GLYPH, (size_t)runes);
    mask[runes] = '\0';
    return mask;
}

typedef enum { TEXT_FIELD_IDLE, TEXT_FIELD_SUBMITTED, TEXT_FIELD_BLURRED } TextFieldEvent;

static void TextFieldDrawBox(Rectangle rect, bool focused) {
    Texture2D box = GuiTex(GUI_GUILD_TEXTBOX);
    NPatchInfo slices = {
        {0.0f, 0.0f, (float)box.width, (float)box.height},
        TEXT_FIELD_SLICE,
        TEXT_FIELD_SLICE,
        TEXT_FIELD_SLICE,
        TEXT_FIELD_SLICE,
        NPATCH_NINE_PATCH,
    };
    DrawTextureNPatch(
        box, slices, rect, (Vector2){0.0f, 0.0f}, 0.0f, focused ? TEXT_FIELD_BACKGROUND_FOCUSED : TEXT_FIELD_BACKGROUND
    );
}

static void TextFieldDrawText(const TextField *field, Rectangle inner, Font font, float size, const char *placeholder) {
    bool caret = field->focused && fmodf(field->time, TEXT_FIELD_CARET_PERIOD) < TEXT_FIELD_CARET_PERIOD * 0.5f;
    if (!field->text[0] && !field->focused) {
        UiDrawTextLeftMiddle(font, placeholder ? placeholder : "", inner, size, TEXT_FIELD_PLACEHOLDER);
        return;
    }
    const char *shown = TextFormat("%s%s", TextFieldShownText(field), caret ? "|" : "");
    float width = MeasureTextEx(font, shown, size, 1.0f).x;
    float shift = width > inner.width ? width - inner.width : 0.0f;
    float scale = UiScale();
    BeginScissorMode(
        (int)(inner.x * scale), (int)(inner.y * scale), (int)(inner.width * scale) + 1, (int)(inner.height * scale)
    );
    UiDrawTextLeftMiddle(
        font, shown, (Rectangle){inner.x - shift, inner.y, inner.width + shift, inner.height}, size, TEXT_FIELD_TEXT
    );
    EndScissorMode();
}

TextFieldEvent
TextFieldDraw(TextField *field, Rectangle rect, Font font, float size, const char *placeholder, bool interactive) {
    TextFieldEvent event = TEXT_FIELD_IDLE;
    field->time += GetFrameTime();
    if (interactive && !field->read_only && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool inside = CheckCollisionPointRec(UiMouse(), rect);
        if (field->focused && !inside) event = TEXT_FIELD_BLURRED;
        if (!field->focused && inside) {
            field->time = 0.0f;
            while (GetCharPressed() > 0) {}
        }
        field->focused = inside;
    }
    if (!interactive || field->read_only) field->focused = false;
    if (field->focused) {
        TextFieldType(field);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) event = TEXT_FIELD_SUBMITTED;
    }
    TextFieldDrawBox(rect, field->focused);
    Rectangle inner = {
        rect.x + TEXT_FIELD_PADDING, rect.y, fmaxf(rect.width - TEXT_FIELD_PADDING * 2.0f, 0.0f), rect.height
    };
    TextFieldDrawText(field, inner, font, size, placeholder);
    return event;
}

bool TextFieldTakeStripped(TextField *field) {
    bool stripped = field->stripped;
    field->stripped = false;
    return stripped;
}
