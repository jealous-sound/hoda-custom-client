#define OPTIONS_DESIGN_WIDTH 1280.0f
#define OPTIONS_SCRIM_COLOR ((Color){0, 0, 0, 180})
#define OPTIONS_TEXT_COLOR ((Color){230, 230, 230, 255})
#define OPTIONS_TITLE_FONT_SIZE 32.0f
#define OPTIONS_LABEL_FONT_SIZE 26.0f
#define OPTIONS_TOGGLE_FONT_SIZE 20.0f
#define OPTIONS_PLAYER_ID_FONT_SIZE 18.0f
#define OPTIONS_BUTTON_LABEL_SCALE 0.3f
#define OPTIONS_OK_LABEL_SCALE (20.0f / 76.0f)
#define OPTIONS_MORE_SLOT_COUNT 4
#define LANGUAGE_ROW_HEIGHT 77.0f
#define LANGUAGE_ROW_FILL_HEIGHT 0.85f
#define LANGUAGE_ROW_CONTENT_HEIGHT 0.8f
#define LANGUAGE_LABEL_X 0.1f
#define LANGUAGE_LABEL_WIDTH 0.75f
#define LANGUAGE_CHECK_X 0.95f
#define LANGUAGE_CHECK_HEIGHT 0.8f
#define LANGUAGE_LABEL_FONT_SIZE 26.0f
#define LANGUAGE_SCROLL_STEP 40.0f
#define LANGUAGE_SYSTEM_ROW 0
#define LANGUAGE_NAME_CAP 160

static const Rectangle OPTIONS_MORE_BACKGROUND = {447.0f, 72.0f, 388.0f, 653.0f};
static const Rectangle OPTIONS_MORE_PANEL = {447.0f, 72.0f, 388.0f, 584.0f};
static const Rectangle OPTIONS_MORE_TITLE = {447.0f, 72.0f, 388.0f, 65.0f};
static const Rectangle OPTIONS_MORE_CLOSE = {777.0f, 103.0f, 73.0f, 73.0f};
static const Rectangle OPTIONS_MORE_PLAYER_ID = {476.0f, 638.0f, 330.0f, 65.0f};
static const float OPTIONS_MORE_SLOT_Y[OPTIONS_MORE_SLOT_COUNT] = {176.0f, 248.0f, 320.0f, 392.0f};
static const Rectangle OPTIONS_MORE_SLOT = {505.0f, 0.0f, 272.0f, 68.0f};

static const Rectangle OPTIONS_AUDIO_PANEL = {447.0f, 195.0f, 388.0f, 402.0f};
static const Rectangle OPTIONS_AUDIO_TITLE = {447.0f, 195.0f, 388.0f, 66.0f};
static const Rectangle OPTIONS_AUDIO_CLOSE = {776.0f, 227.0f, 74.0f, 74.0f};
static const Rectangle OPTIONS_MUSIC_LABEL = {447.0f, 286.0f, 388.0f, 52.0f};
static const Rectangle OPTIONS_MUSIC_TOGGLE = {567.0f, 329.0f, 148.0f, 68.0f};
static const Rectangle OPTIONS_SOUND_LABEL = {447.0f, 419.0f, 388.0f, 52.0f};
static const Rectangle OPTIONS_SOUND_TOGGLE = {567.0f, 464.0f, 148.0f, 68.0f};
static const Rectangle OPTIONS_TOGGLE_ON_LABEL = {5.0f, 10.0f, 59.0f, 52.0f};
static const Rectangle OPTIONS_TOGGLE_OFF_LABEL = {84.0f, 10.0f, 59.0f, 52.0f};

static const Rectangle LANGUAGE_PANEL = {108.0f, 110.0f, 1064.0f, 500.0f};
static const Rectangle LANGUAGE_TITLE = {108.0f, 136.0f, 1065.0f, 76.0f};
static const Rectangle LANGUAGE_LIST = {196.0f, 200.0f, 904.0f, 275.0f};
static const Rectangle LANGUAGE_OK = {489.0f, 485.0f, 304.0f, 76.0f};

typedef enum {
    OPTIONS_PAGE_MORE,
    OPTIONS_PAGE_AUDIO,
    OPTIONS_PAGE_LANGUAGE,
} OptionsPage;

typedef enum {
    OPTIONS_MORE_AUDIO,
    OPTIONS_MORE_LANGUAGE,
    OPTIONS_MORE_FULLSCREEN,
    OPTIONS_MORE_BUTTON_COUNT,
} OptionsMoreButton;

typedef enum {
    OPTIONS_TOGGLE_NONE,
    OPTIONS_TOGGLE_MUSIC,
    OPTIONS_TOGGLE_SOUND,
} OptionsToggle;

typedef enum {
    OPTIONS_ACTION_NONE,
    OPTIONS_ACTION_RESTART,
} OptionsAction;

typedef struct {
    bool open;
    bool scrim_armed;
    bool row_armed;
    int armed_row;
    Vector2 armed_row_press;
    OptionsToggle armed_toggle;
    OptionsPage page;
    int selected_row;
    float scroll;
    Font title_font;
    Font menu_font;
    Button close;
    Button more[OPTIONS_MORE_BUTTON_COUNT];
    Button ok;
} OptionsPopup;

static const char *const OPTIONS_MORE_LABEL_KEYS[OPTIONS_MORE_BUTTON_COUNT] = {
    [OPTIONS_MORE_AUDIO] = "OPTIONS_AUDIO_SETTINGS",
    [OPTIONS_MORE_LANGUAGE] = "HODA_OPTIONS_LANGUAGE",
    [OPTIONS_MORE_FULLSCREEN] = "OPTIONS_VIDEO_SETTINGS",
};

void OptionsPopupInitialize(OptionsPopup *o) {
    *o = (OptionsPopup){0};
    o->title_font = GuiFont(FNT_COOL);
    o->menu_font = GuiFont(FNT_MENU);
    o->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&o->close, SND_UI_POPUP_CLOSE);
    Vector2 centre = {0.5f, 0.5f};
    for (int i = 0; i < OPTIONS_MORE_BUTTON_COUNT; i++)
        o->more[i] = ButtonLoadText(
            GUI_BTN_GREY,
            GUI_BTN_GREY_ON,
            LocGet(OPTIONS_MORE_LABEL_KEYS[i]),
            o->title_font,
            centre,
            OPTIONS_BUTTON_LABEL_SCALE
        );
    o->ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("ENTER_NAME_OK_BUTTON"), o->title_font, centre, OPTIONS_OK_LABEL_SCALE
    );
    ButtonSetLabelColor(&o->ok, WHITE);
}

bool OptionsPopupIsOpen(const OptionsPopup *o) {
    return o->open;
}

void OptionsPopupOpen(OptionsPopup *o) {
    o->open = true;
    o->page = OPTIONS_PAGE_MORE;
    o->scrim_armed = false;
    o->row_armed = false;
}

void OptionsPopupClose(OptionsPopup *o) {
    o->open = false;
}

static int OptionsLanguageRowCount(void) {
    return LocLanguageCount() + 1;
}

static int OptionsSavedLanguageRow(void) {
    const char *saved = SettingsLanguage();
    for (int i = 0; i < LocLanguageCount(); i++)
        if (!strcmp(LocLanguageAt(i)->code, saved)) return i + 1;
    return LANGUAGE_SYSTEM_ROW;
}

static const char *OptionsLanguageRowCode(int row) {
    return row == LANGUAGE_SYSTEM_ROW ? "" : LocLanguageAt(row - 1)->code;
}

static void OptionsShowLanguages(OptionsPopup *o) {
    o->page = OPTIONS_PAGE_LANGUAGE;
    o->selected_row = OptionsSavedLanguageRow();
    float content = OptionsLanguageRowCount() * LANGUAGE_ROW_HEIGHT;
    float wanted = o->selected_row * LANGUAGE_ROW_HEIGHT;
    o->scroll = Clamp(wanted, 0.0f, fmaxf(0.0f, content - LANGUAGE_LIST.height));
    o->row_armed = false;
}

bool OptionsPopupBack(OptionsPopup *o) {
    if (!o->open) return false;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    if (o->page == OPTIONS_PAGE_MORE)
        o->open = false;
    else
        o->page = OPTIONS_PAGE_MORE;
    return true;
}

static Rectangle OptionsPlace(Rectangle design) {
    float offset = (UiWidth() - OPTIONS_DESIGN_WIDTH) * 0.5f;
    return (Rectangle){design.x + offset, design.y, design.width, design.height};
}

static Rectangle OptionsWithin(Rectangle parent, Rectangle local) {
    return (Rectangle){parent.x + local.x, parent.y + local.y, local.width, local.height};
}

static bool OptionsScrimClicked(OptionsPopup *o, Rectangle panel) {
    Vector2 mouse = UiMouse();
    bool outside = !CheckCollisionPointRec(mouse, panel);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) o->scrim_armed = outside;
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return false;
    bool clicked = o->scrim_armed && outside;
    o->scrim_armed = false;
    return clicked;
}

static void OptionsDrawMore(OptionsPopup *o, const char *player_id) {
    UiDrawTexture(GuiTex(GUI_OPTIONS_MORE_BG), OptionsPlace(OPTIONS_MORE_BACKGROUND), WHITE);
    UiDrawTextCentered(
        o->title_font,
        LocGet("OPTIONS_MORE_TITLE"),
        OptionsPlace(OPTIONS_MORE_TITLE),
        OPTIONS_TITLE_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    for (int i = 0; i < OPTIONS_MORE_BUTTON_COUNT; i++) {
        Rectangle slot = OPTIONS_MORE_SLOT;
        slot.y = OPTIONS_MORE_SLOT_Y[i];
        if (!ButtonDraw(&o->more[i], OptionsPlace(slot))) continue;
        if (i == OPTIONS_MORE_AUDIO) o->page = OPTIONS_PAGE_AUDIO;
        if (i == OPTIONS_MORE_LANGUAGE) OptionsShowLanguages(o);
        if (i == OPTIONS_MORE_FULLSCREEN) ToggleBorderlessWindowed();
    }
    if (player_id && player_id[0]) {
        char text[LANGUAGE_NAME_CAP];
        LocFormat1(text, sizeof(text), "OPTIONS_PLAYER_ID", player_id);
        UiDrawTextCentered(
            o->menu_font, text, OptionsPlace(OPTIONS_MORE_PLAYER_ID), OPTIONS_PLAYER_ID_FONT_SIZE, OPTIONS_TEXT_COLOR
        );
    }
    if (ButtonDraw(&o->close, OptionsPlace(OPTIONS_MORE_CLOSE)) ||
        OptionsScrimClicked(o, OptionsPlace(OPTIONS_MORE_PANEL)))
        o->open = false;
}

static bool OptionsToggleClicked(OptionsPopup *o, Rectangle box, OptionsToggle toggle) {
    bool inside = CheckCollisionPointRec(UiMouse(), box);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inside) o->armed_toggle = toggle;
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || o->armed_toggle != toggle) return false;
    o->armed_toggle = OPTIONS_TOGGLE_NONE;
    return inside;
}

static bool OptionsDrawToggle(OptionsPopup *o, Rectangle design, OptionsToggle toggle, bool enabled) {
    Rectangle box = OptionsPlace(design);
    UiDrawTexture(GuiTex(enabled ? GUI_OPTIONS_TOGGLE_ON : GUI_OPTIONS_TOGGLE_OFF), box, WHITE);
    UiDrawTextCentered(
        o->title_font,
        LocGet("OPTIONS_ON"),
        OptionsWithin(box, OPTIONS_TOGGLE_ON_LABEL),
        OPTIONS_TOGGLE_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    UiDrawTextCentered(
        o->title_font,
        LocGet("OPTIONS_OFF"),
        OptionsWithin(box, OPTIONS_TOGGLE_OFF_LABEL),
        OPTIONS_TOGGLE_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    bool clicked = OptionsToggleClicked(o, box, toggle);
    if (clicked) GuiClickSoundPlay();
    return clicked;
}

static void OptionsDrawAudio(OptionsPopup *o) {
    Rectangle panel = OptionsPlace(OPTIONS_AUDIO_PANEL);
    UiDrawTexture(GuiTex(GUI_OPTIONS_AUDIO_BG), panel, WHITE);
    UiDrawTextCentered(
        o->title_font,
        LocGet("OPTIONS_AUDIO_SETTINGS_TITLE"),
        OptionsPlace(OPTIONS_AUDIO_TITLE),
        OPTIONS_TITLE_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    UiDrawTextCentered(
        o->menu_font,
        LocGet("OPTIONS_MUSIC"),
        OptionsPlace(OPTIONS_MUSIC_LABEL),
        OPTIONS_LABEL_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    UiDrawTextCentered(
        o->menu_font,
        LocGet("OPTIONS_SOUND_FX"),
        OptionsPlace(OPTIONS_SOUND_LABEL),
        OPTIONS_LABEL_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    if (OptionsDrawToggle(o, OPTIONS_MUSIC_TOGGLE, OPTIONS_TOGGLE_MUSIC, SettingsMusicEnabled()))
        SettingsSetMusic(!SettingsMusicEnabled());
    if (OptionsDrawToggle(o, OPTIONS_SOUND_TOGGLE, OPTIONS_TOGGLE_SOUND, SettingsSoundEnabled()))
        SettingsSetSound(!SettingsSoundEnabled());
    if (ButtonDraw(&o->close, OptionsPlace(OPTIONS_AUDIO_CLOSE)) || OptionsScrimClicked(o, panel))
        o->page = OPTIONS_PAGE_MORE;
}

static void OptionsLanguageRowName(int row, char *out, size_t capacity) {
    if (row == LANGUAGE_SYSTEM_ROW) {
        snprintf(out, capacity, "%s", LocGet("HODA_LANGUAGE_SYSTEM"));
        return;
    }
    const LocLanguage *language = LocLanguageAt(row - 1);
    const char *localized = LocGet(language->name_key);
    if (!localized[0] || !strcmp(localized, language->native_name))
        snprintf(out, capacity, "%s", language->native_name);
    else
        snprintf(out, capacity, "%s (%s)", language->native_name, localized);
}

static Rectangle OptionsLanguageRowRect(const OptionsPopup *o, Rectangle list, int row) {
    return (Rectangle){list.x, list.y + row * LANGUAGE_ROW_HEIGHT - o->scroll, list.width, LANGUAGE_ROW_HEIGHT};
}

static void OptionsDrawLanguageRow(OptionsPopup *o, Rectangle row, int index) {
    Rectangle fill = UiRect(row, 0.0f, 0.5f, 1.0f, LANGUAGE_ROW_FILL_HEIGHT);
    UiDrawTexture(GuiTex(GUI_LANGUAGE_ROW_BG), fill, WHITE);
    Rectangle content = UiRect(row, 0.5f, 0.5f, 1.0f, LANGUAGE_ROW_CONTENT_HEIGHT);
    char name[LANGUAGE_NAME_CAP];
    OptionsLanguageRowName(index, name, sizeof(name));
    UiDrawTextLeftMiddle(
        o->menu_font,
        name,
        UiRect(content, LANGUAGE_LABEL_X, 0.5f, LANGUAGE_LABEL_WIDTH, 1.0f),
        LANGUAGE_LABEL_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    float check = content.height * LANGUAGE_CHECK_HEIGHT;
    Rectangle box = UiRect(content, LANGUAGE_CHECK_X, 0.5f, check, check);
    UiDrawTexture(GuiTex(index == o->selected_row ? GUI_CONSUME_CHECK_ON : GUI_CONSUME_CHECK_OFF), box, WHITE);
}

static void OptionsUpdateLanguageInput(OptionsPopup *o, Rectangle list) {
    Vector2 mouse = UiMouse();
    bool inside = CheckCollisionPointRec(mouse, list);
    float content = OptionsLanguageRowCount() * LANGUAGE_ROW_HEIGHT;
    if (inside) o->scroll -= GetMouseWheelMove() * LANGUAGE_SCROLL_STEP;
    if (o->row_armed && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) o->scroll -= UiMouseDelta().y;
    o->scroll = Clamp(o->scroll, 0.0f, fmaxf(0.0f, content - list.height));
    int row = inside ? (int)floorf((mouse.y - list.y + o->scroll) / LANGUAGE_ROW_HEIGHT) : -1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        o->row_armed = row >= 0 && row < OptionsLanguageRowCount();
        o->armed_row = row;
        o->armed_row_press = mouse;
    }
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || !o->row_armed) return;
    o->row_armed = false;
    if (row != o->armed_row || Vector2Distance(mouse, o->armed_row_press) > LANGUAGE_SCROLL_STEP * 0.25f) return;
    o->selected_row = row;
    GuiClickSoundPlay();
}

static OptionsAction OptionsDrawLanguages(OptionsPopup *o) {
    Rectangle panel = OptionsPlace(LANGUAGE_PANEL);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), panel, WHITE);
    UiDrawTextCentered(
        o->title_font,
        LocGet("LANGUAGE_SELECT_TITLE"),
        OptionsPlace(LANGUAGE_TITLE),
        OPTIONS_TITLE_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    Rectangle list = OptionsPlace(LANGUAGE_LIST);
    OptionsUpdateLanguageInput(o, list);
    float ui = UiScale();
    BeginScissorMode((int)(list.x * ui), (int)(list.y * ui), (int)(list.width * ui), (int)(list.height * ui));
    for (int i = 0; i < OptionsLanguageRowCount(); i++) {
        Rectangle row = OptionsLanguageRowRect(o, list, i);
        if (row.y + row.height < list.y || row.y > list.y + list.height) continue;
        OptionsDrawLanguageRow(o, row, i);
    }
    EndScissorMode();
    if (ButtonDraw(&o->ok, OptionsPlace(LANGUAGE_OK))) {
        bool changed = o->selected_row != OptionsSavedLanguageRow();
        o->page = OPTIONS_PAGE_MORE;
        if (!changed) return OPTIONS_ACTION_NONE;
        SettingsSetLanguage(OptionsLanguageRowCode(o->selected_row));
        o->open = false;
        return OPTIONS_ACTION_RESTART;
    }
    if (OptionsScrimClicked(o, panel)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        o->page = OPTIONS_PAGE_MORE;
    }
    return OPTIONS_ACTION_NONE;
}

OptionsAction OptionsPopupDraw(OptionsPopup *o, const char *player_id) {
    if (!o->open) return OPTIONS_ACTION_NONE;
    Rectangle root = UiRoot();
    DrawRectangleRec(root, OPTIONS_SCRIM_COLOR);
    switch (o->page) {
        case OPTIONS_PAGE_MORE:
            OptionsDrawMore(o, player_id);
            break;
        case OPTIONS_PAGE_AUDIO:
            OptionsDrawAudio(o);
            break;
        case OPTIONS_PAGE_LANGUAGE:
            return OptionsDrawLanguages(o);
    }
    return OPTIONS_ACTION_NONE;
}
