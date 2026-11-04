#define NAME_PROMPT_TITLE_SIZE 30.0f
#define NAME_PROMPT_FIELD_TEXT_SIZE 28.0f
#define NAME_PROMPT_ERROR_SIZE 20.0f
#define NAME_PROMPT_CARET_PERIOD 1.0f
#define NAME_PROMPT_TEXT_COLOR ((Color){242, 236, 218, 255})
#define NAME_PROMPT_ERROR_COLOR ((Color){235, 110, 90, 255})

static const char *const NAME_PROMPT_SUGGESTIONS[] = {
    "Grey Warden",
    "Mabari War Hound",
    "Alistair",
    "Morrigan",
    "Leliana",
    "Sten",
    "Wynne",
    "Oghren",
    "Zevran",
    "Shale",
    "Duncan",
    "Cailan Theirin",
    "Flemeth",
    "Queen Anora Theirin",
    "Zathrian",
    "Bhelen Aeducan",
    "Lord Pyral Harrowmont",
    "Arl Eamon",
    "Riordan",
    "Loghain Mac Tir",
    "Arl Howe",
    "Urthemiel",
    "The Warden Commander",
    "Mhairi",
    "Nathaniel Howe",
    "Sigrun",
    "Velanna",
    "Justice",
    "The Architect",
    "The Mother",
    "Hawke",
    "Varric Tethras",
    "Bethany Hawke",
    "Carver Hawke",
    "Aveline Vallen",
    "Isabela",
    "Merrill",
    "Anders",
    "Fenris",
    "Sebastian Vael",
    "Tallis",
    "Knight-Commander Meredith",
    "First Enchanter Orsino",
    "The Arishok",
};

typedef struct {
    bool open;
    bool invalid;
    char text[PLAYER_NAME_CAPACITY];
    float time;
    Button ok;
} NamePrompt;

void NamePromptInitialize(NamePrompt *prompt) {
    *prompt = (NamePrompt){0};
    prompt->ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("ENTER_NAME_OK_BUTTON"), GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.34f
    );
}

void NamePromptOpen(NamePrompt *prompt) {
    int count = (int)(sizeof(NAME_PROMPT_SUGGESTIONS) / sizeof(NAME_PROMPT_SUGGESTIONS[0]));
    StrCopy(prompt->text, sizeof(prompt->text), NAME_PROMPT_SUGGESTIONS[GetRandomValue(0, count - 1)]);
    prompt->open = true;
    prompt->invalid = false;
    prompt->time = 0.0f;
}

bool NamePromptIsOpen(const NamePrompt *prompt) {
    return prompt->open;
}

static int NamePromptCharacterCount(const char *text) {
    int count = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c; c++) count += (*c & 0xC0) != 0x80;
    return count;
}

static void NamePromptErase(NamePrompt *prompt) {
    int length = (int)strlen(prompt->text);
    while (length > 0 && ((unsigned char)prompt->text[length - 1] & 0xC0) == 0x80) length--;
    if (length > 0) length--;
    prompt->text[length] = '\0';
}

static void NamePromptType(NamePrompt *prompt) {
    for (int codepoint = GetCharPressed(); codepoint > 0; codepoint = GetCharPressed()) {
        int bytes = 0;
        const char *encoded = CodepointToUTF8(codepoint, &bytes);
        int length = (int)strlen(prompt->text);
        if (NamePromptCharacterCount(prompt->text) >= PLAYER_NAME_MAX_CHARACTERS ||
            length + bytes >= (int)sizeof(prompt->text))
            continue;
        memcpy(prompt->text + length, encoded, (size_t)bytes);
        prompt->text[length + bytes] = '\0';
        prompt->invalid = false;
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        NamePromptErase(prompt);
        prompt->invalid = false;
    }
}

static bool NamePromptSubmit(NamePrompt *prompt, Player *player) {
    if (!PlayerNameValid(prompt->text)) {
        prompt->invalid = true;
        GuiSoundPlay(SND_STORE_ERROR);
        return false;
    }
    StrCopy(player->name, sizeof(player->name), prompt->text);
    prompt->open = false;
    return true;
}

bool NamePromptDraw(NamePrompt *prompt, Player *player, float dt) {
    if (!prompt->open) return false;
    prompt->time += dt;
    NamePromptType(prompt);

    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    Font title_font = GuiFont(FNT_COOL), body_font = GuiFont(FNT_MENU);
    UiDrawTextCentered(
        title_font,
        LocGet("ENTER_NAME_PROMPT"),
        UiRect(content, 0.5f, 0.0f, 1.0f, 0.22f),
        NAME_PROMPT_TITLE_SIZE,
        NAME_PROMPT_TEXT_COLOR
    );

    Rectangle field = UiRect(content, 0.5f, 0.4f, 0.9f, 0.24f);
    DrawRectangleRec(field, (Color){18, 16, 14, 230});
    DrawRectangleLinesEx(field, 2.0f, (Color){170, 140, 90, 255});
    bool caret = fmodf(prompt->time, NAME_PROMPT_CARET_PERIOD) < NAME_PROMPT_CARET_PERIOD * 0.5f;
    UiDrawTextCentered(
        body_font, TextFormat("%s%s", prompt->text, caret ? "|" : " "), field, NAME_PROMPT_FIELD_TEXT_SIZE, WHITE
    );
    if (prompt->invalid)
        MessagePopupDrawText(
            body_font,
            LocGet("ERROR_INVALID_CHARACTER_IN_NAME_MESSAGE"),
            UiRect(content, 0.5f, 0.78f, 1.0f, 0.3f),
            NAME_PROMPT_ERROR_SIZE,
            NAME_PROMPT_ERROR_COLOR
        );

    bool ready = prompt->text[0] != '\0';
    bool pressed = ButtonDrawEx(&prompt->ok, MessagePopupOkButton(popup), ready);
    if (ready && (pressed || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)))
        return NamePromptSubmit(prompt, player);
    return false;
}
