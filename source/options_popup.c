#define OPTIONS_DESIGN_WIDTH 1280.0f
#define OPTIONS_SCRIM_COLOR ((Color){0, 0, 0, 180})
#define OPTIONS_TEXT_COLOR ((Color){230, 230, 230, 255})
#define OPTIONS_HEADING_COLOR ((Color){236, 200, 120, 255})
#define OPTIONS_VALUE_COLOR ((Color){206, 198, 180, 255})
#define OPTIONS_NOTICE_COLOR ((Color){236, 200, 120, 255})
#define OPTIONS_DIVIDER_COLOR ((Color){120, 104, 76, 160})
#define OPTIONS_ROW_SHADE ((Color){255, 255, 255, 10})
#define OPTIONS_ROW_HOVER_SHADE ((Color){255, 220, 150, 22})
#define OPTIONS_SLIDER_FILL_TOP ((Color){226, 176, 64, 255})
#define OPTIONS_SLIDER_FILL_BOTTOM ((Color){150, 96, 22, 255})
#define OPTIONS_COUNTER_SHADE ((Color){0, 0, 0, 150})
#define OPTIONS_TITLE_FONT_SIZE 32.0f
#define OPTIONS_HEADING_FONT_SIZE 24.0f
#define OPTIONS_ROW_FONT_SIZE 21.0f
#define OPTIONS_VALUE_FONT_SIZE 19.0f
#define OPTIONS_TOGGLE_FONT_SIZE 16.0f
#define OPTIONS_COUNTER_FONT_SIZE 16.0f
#define OPTIONS_BUTTON_LABEL_SCALE 0.4f
#define OPTIONS_OK_LABEL_SCALE (20.0f / 76.0f)
#define OPTIONS_SECTION_ROW_COUNT 4
#define OPTIONS_ROW_STEP 56.0f
#define OPTIONS_ROW_HEIGHT 48.0f
#define OPTIONS_ROW_PADDING 12.0f
#define OPTIONS_CONTROL_WIDTH 210.0f
#define OPTIONS_VERSION_WIDTH 270.0f
#define OPTIONS_FIT_ATTEMPTS 3
#define OPTIONS_SLIDER_BAR_HEIGHT 34.0f
#define OPTIONS_SLIDER_VALUE_WIDTH 54.0f
#define OPTIONS_SLIDER_TROUGH_WIDTH 0.79f
#define OPTIONS_SLIDER_TROUGH_HEIGHT 0.4f
#define OPTIONS_SLIDER_KNOB_SIZE 30.0f
#define OPTIONS_SLIDER_WHEEL_STEP 0.05f
#define OPTIONS_PERCENT 100.0f
#define OPTIONS_TOGGLE_WIDTH 104.0f
#define OPTIONS_TOGGLE_LABEL_HEIGHT 0.76f
#define OPTIONS_ARROW_SIZE 32.0f
#define OPTIONS_COUNTER_PADDING 6.0f
#define OPTIONS_NO_SLIDER (-1)
#define OPTIONS_TEXT_CAP 160
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

static const Rectangle OPTIONS_PANEL = {108.0f, 110.0f, 1064.0f, 500.0f};
static const Rectangle OPTIONS_TITLE = {108.0f, 132.0f, 1064.0f, 60.0f};
static const Rectangle OPTIONS_CLOSE = {1085.0f, 92.0f, 73.0f, 73.0f};
static const float OPTIONS_COLUMN_X[] = {196.0f, 660.0f};
static const float OPTIONS_COLUMN_WIDTH = 424.0f;
static const float OPTIONS_HEADING_Y = 200.0f;
static const float OPTIONS_HEADING_HEIGHT = 34.0f;
static const float OPTIONS_HEADING_RULE_HEIGHT = 6.0f;
static const float OPTIONS_FIRST_ROW_Y = 246.0f;
static const Rectangle OPTIONS_DIVIDER = {639.0f, 208.0f, 2.0f, 254.0f};
static const Rectangle OPTIONS_FOOTER_RULE = {196.0f, 478.0f, 888.0f, 6.0f};
static const float OPTIONS_FOOTER_ROW_Y = 496.0f;

static const Rectangle LANGUAGE_PANEL = {108.0f, 110.0f, 1064.0f, 500.0f};
static const Rectangle LANGUAGE_TITLE = {108.0f, 136.0f, 1065.0f, 76.0f};
static const Rectangle LANGUAGE_LIST = {196.0f, 200.0f, 904.0f, 275.0f};
static const Rectangle LANGUAGE_OK = {489.0f, 485.0f, 304.0f, 76.0f};

static const int OPTIONS_FRAME_RATES[] = {30, 60, 120, 144, 240, SETTINGS_UNLIMITED_FRAME_RATE};
#define OPTIONS_FRAME_RATE_COUNT ((int)(sizeof(OPTIONS_FRAME_RATES) / sizeof(OPTIONS_FRAME_RATES[0])))

typedef enum {
    OPTIONS_PAGE_SETTINGS,
    OPTIONS_PAGE_LANGUAGE,
} OptionsPage;

typedef enum {
    OPTIONS_ACTION_NONE,
    OPTIONS_ACTION_RESTART,
} OptionsAction;

typedef enum {
    OPTIONS_COLUMN_AUDIO,
    OPTIONS_COLUMN_DISPLAY,
    OPTIONS_COLUMN_COUNT,
} OptionsColumn;

typedef enum {
    OPTIONS_CONTROL_SLIDER,
    OPTIONS_CONTROL_SWITCH,
    OPTIONS_CONTROL_CHOICE,
    OPTIONS_CONTROL_LANGUAGE,
    OPTIONS_CONTROL_VERSION,
} OptionsControl;

typedef enum {
    OPTIONS_CHOICE_DISPLAY_MODE,
    OPTIONS_CHOICE_FRAME_RATE,
    OPTIONS_CHOICE_COUNT,
} OptionsChoice;

typedef enum {
    OPTIONS_ALIGN_LEFT,
    OPTIONS_ALIGN_CENTRE,
    OPTIONS_ALIGN_RIGHT,
} OptionsAlign;

typedef enum {
    OPTIONS_STEP_BACK,
    OPTIONS_STEP_FORWARD,
    OPTIONS_STEP_COUNT,
} OptionsStep;

typedef enum {
    OPTIONS_ROW_MASTER_VOLUME,
    OPTIONS_ROW_MUSIC_VOLUME,
    OPTIONS_ROW_SOUND_VOLUME,
    OPTIONS_ROW_MUTE_IN_BACKGROUND,
    OPTIONS_ROW_DISPLAY_MODE,
    OPTIONS_ROW_FRAME_RATE,
    OPTIONS_ROW_VSYNC,
    OPTIONS_ROW_SHOW_FPS,
    OPTIONS_ROW_LANGUAGE,
    OPTIONS_ROW_VERSION,
    OPTIONS_ROW_COUNT,
} OptionsRow;

#define OPTIONS_FOOTER_SLOT OPTIONS_SECTION_ROW_COUNT

typedef struct {
    const char *label_key;
    OptionsControl control;
    OptionsColumn column;
    int slot;
    SettingsVolume volume;
    SettingsSwitch setting;
    OptionsChoice choice;
} OptionsRowSpec;

static const OptionsRowSpec OPTIONS_ROWS[OPTIONS_ROW_COUNT] = {
    [OPTIONS_ROW_MASTER_VOLUME] =
        {"HODA_SETTINGS_MASTER_VOLUME",
         OPTIONS_CONTROL_SLIDER,
         OPTIONS_COLUMN_AUDIO,
         0,
         .volume = SETTINGS_VOLUME_MASTER},
    [OPTIONS_ROW_MUSIC_VOLUME] =
        {"OPTIONS_MUSIC", OPTIONS_CONTROL_SLIDER, OPTIONS_COLUMN_AUDIO, 1, .volume = SETTINGS_VOLUME_MUSIC},
    [OPTIONS_ROW_SOUND_VOLUME] =
        {"OPTIONS_SOUND_FX", OPTIONS_CONTROL_SLIDER, OPTIONS_COLUMN_AUDIO, 2, .volume = SETTINGS_VOLUME_SOUND},
    [OPTIONS_ROW_MUTE_IN_BACKGROUND] =
        {"HODA_SETTINGS_MUTE_IN_BACKGROUND",
         OPTIONS_CONTROL_SWITCH,
         OPTIONS_COLUMN_AUDIO,
         3,
         .setting = SETTINGS_SWITCH_MUTE_IN_BACKGROUND},
    [OPTIONS_ROW_DISPLAY_MODE] =
        {"HODA_SETTINGS_DISPLAY_MODE",
         OPTIONS_CONTROL_CHOICE,
         OPTIONS_COLUMN_DISPLAY,
         0,
         .choice = OPTIONS_CHOICE_DISPLAY_MODE},
    [OPTIONS_ROW_FRAME_RATE] =
        {"HODA_SETTINGS_FRAME_RATE",
         OPTIONS_CONTROL_CHOICE,
         OPTIONS_COLUMN_DISPLAY,
         1,
         .choice = OPTIONS_CHOICE_FRAME_RATE},
    [OPTIONS_ROW_VSYNC] =
        {"HODA_SETTINGS_VSYNC", OPTIONS_CONTROL_SWITCH, OPTIONS_COLUMN_DISPLAY, 2, .setting = SETTINGS_SWITCH_VSYNC},
    [OPTIONS_ROW_SHOW_FPS] =
        {"HODA_SETTINGS_SHOW_FPS",
         OPTIONS_CONTROL_SWITCH,
         OPTIONS_COLUMN_DISPLAY,
         3,
         .setting = SETTINGS_SWITCH_SHOW_FPS},
    [OPTIONS_ROW_LANGUAGE] =
        {"HODA_OPTIONS_LANGUAGE", OPTIONS_CONTROL_LANGUAGE, OPTIONS_COLUMN_AUDIO, OPTIONS_FOOTER_SLOT},
    [OPTIONS_ROW_VERSION] = {
        "HODA_SETTINGS_VERSION", OPTIONS_CONTROL_VERSION, OPTIONS_COLUMN_DISPLAY, OPTIONS_FOOTER_SLOT
    },
};

static const char *const OPTIONS_COLUMN_HEADING_KEYS[OPTIONS_COLUMN_COUNT] = {
    [OPTIONS_COLUMN_AUDIO] = "HODA_SETTINGS_AUDIO",
    [OPTIONS_COLUMN_DISPLAY] = "HODA_SETTINGS_DISPLAY",
};

typedef struct {
    bool open;
    bool scrim_armed;
    bool row_armed;
    int armed_row;
    Vector2 armed_row_press;
    OptionsPage page;
    int selected_row;
    float scroll;
    int dragged_slider;
    Font title_font;
    Font menu_font;
    char language_label[OPTIONS_TEXT_CAP];
    Button close;
    Button ok;
    Button language;
    Button switches[OPTIONS_ROW_COUNT];
    Button steps[OPTIONS_ROW_COUNT][OPTIONS_STEP_COUNT];
} OptionsPopup;

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

void OptionsPopupInitialize(OptionsPopup *o) {
    *o = (OptionsPopup){.dragged_slider = OPTIONS_NO_SLIDER};
    o->title_font = GuiFont(FNT_COOL);
    o->menu_font = GuiFont(FNT_MENU);
    o->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&o->close, SND_UI_POPUP_CLOSE);
    Vector2 centre = {0.5f, 0.5f};
    o->ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("ENTER_NAME_OK_BUTTON"), o->title_font, centre, OPTIONS_OK_LABEL_SCALE
    );
    ButtonSetLabelColor(&o->ok, WHITE);
    OptionsLanguageRowName(OptionsSavedLanguageRow(), o->language_label, sizeof(o->language_label));
    o->language = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, o->language_label, o->menu_font, centre, OPTIONS_BUTTON_LABEL_SCALE
    );
    for (int row = 0; row < OPTIONS_ROW_COUNT; row++)
        for (int step = 0; step < OPTIONS_STEP_COUNT; step++)
            o->steps[row][step] = ButtonLoad(GUI_OPTIONS_ARROW, GUI_OPTIONS_ARROW_ON);
}

bool OptionsPopupIsOpen(const OptionsPopup *o) {
    return o->open;
}

void OptionsPopupOpen(OptionsPopup *o) {
    o->open = true;
    o->page = OPTIONS_PAGE_SETTINGS;
    o->scrim_armed = false;
    o->row_armed = false;
    o->dragged_slider = OPTIONS_NO_SLIDER;
}

void OptionsPopupClose(OptionsPopup *o) {
    o->open = false;
    o->dragged_slider = OPTIONS_NO_SLIDER;
    SettingsSaveAdjustments();
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
    if (o->page == OPTIONS_PAGE_SETTINGS)
        OptionsPopupClose(o);
    else
        o->page = OPTIONS_PAGE_SETTINGS;
    return true;
}

static Rectangle OptionsPlace(Rectangle design) {
    float offset = (UiWidth() - OPTIONS_DESIGN_WIDTH) * 0.5f;
    return (Rectangle){design.x + offset, design.y, design.width, design.height};
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

static void OptionsVersionLine(char *out, size_t capacity) {
    if (ClientUpdateStatusNow() == CLIENT_UPDATE_CURRENT) {
        StrCopy(out, (int)capacity, HOD_VERSION);
        return;
    }
    LocFormat2(out, capacity, "HODA_OPTIONS_VERSION_UPDATE", HOD_VERSION, ClientUpdateCurrentRelease().latest);
}

static float OptionsFrameRateOrder(int frame_rate) {
    return frame_rate == SETTINGS_UNLIMITED_FRAME_RATE ? INFINITY : (float)frame_rate;
}

int OptionsSteppedFrameRate(int current, OptionsStep step) {
    int at = 0;
    while (OptionsFrameRateOrder(OPTIONS_FRAME_RATES[at]) < OptionsFrameRateOrder(current)) at++;
    bool listed = OPTIONS_FRAME_RATES[at] == current;
    int index = step == OPTIONS_STEP_FORWARD ? (listed ? at + 1 : at) : at - 1;
    return OPTIONS_FRAME_RATES[(index + OPTIONS_FRAME_RATE_COUNT) % OPTIONS_FRAME_RATE_COUNT];
}

static bool OptionsFullscreen(void) {
    return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
}

static void OptionsChoiceText(OptionsChoice choice, char *out, size_t capacity) {
    if (choice == OPTIONS_CHOICE_DISPLAY_MODE) {
        StrCopy(
            out, (int)capacity, LocGet(OptionsFullscreen() ? "HODA_SETTINGS_FULLSCREEN" : "HODA_SETTINGS_WINDOWED")
        );
        return;
    }
    int frame_rate = SettingsFrameRateLimit();
    if (frame_rate == SETTINGS_UNLIMITED_FRAME_RATE) {
        StrCopy(out, (int)capacity, LocGet("HODA_SETTINGS_UNLIMITED"));
        return;
    }
    LocFormatInt(out, capacity, "HODA_SETTINGS_FPS_VALUE", frame_rate);
}

static void OptionsStepChoice(OptionsChoice choice, OptionsStep step) {
    if (choice == OPTIONS_CHOICE_DISPLAY_MODE) {
        ToggleBorderlessWindowed();
        SettingsSetSwitch(SETTINGS_SWITCH_FULLSCREEN, OptionsFullscreen());
        return;
    }
    SettingsSetFrameRateLimit(OptionsSteppedFrameRate(SettingsFrameRateLimit(), step));
    SetTargetFPS(SettingsFrameRateLimit());
}

static void OptionsApplySwitch(SettingsSwitch setting, bool on) {
    SettingsSetSwitch(setting, on);
    if (setting != SETTINGS_SWITCH_VSYNC) return;
    if (on)
        SetWindowState(FLAG_VSYNC_HINT);
    else
        ClearWindowState(FLAG_VSYNC_HINT);
}

static Rectangle OptionsRowRect(const OptionsRowSpec *spec) {
    float y =
        spec->slot == OPTIONS_FOOTER_SLOT ? OPTIONS_FOOTER_ROW_Y : OPTIONS_FIRST_ROW_Y + spec->slot * OPTIONS_ROW_STEP;
    return OptionsPlace((Rectangle){OPTIONS_COLUMN_X[spec->column], y, OPTIONS_COLUMN_WIDTH, OPTIONS_ROW_HEIGHT});
}

static Rectangle OptionsInset(Rectangle area) {
    return UiRect(area, 0.5f, 0.5f, area.width - OPTIONS_ROW_PADDING * 2.0f, 1.0f);
}

static float OptionsControlWidth(OptionsControl control) {
    if (control == OPTIONS_CONTROL_SWITCH) return OPTIONS_TOGGLE_WIDTH;
    return control == OPTIONS_CONTROL_VERSION ? OPTIONS_VERSION_WIDTH : OPTIONS_CONTROL_WIDTH;
}

static Rectangle OptionsControlRect(Rectangle row, OptionsControl control) {
    return UiRect(OptionsInset(row), 1.0f, 0.5f, OptionsControlWidth(control), 1.0f);
}

static Rectangle OptionsLabelRect(Rectangle row, OptionsControl control) {
    Rectangle inner = OptionsInset(row);
    return UiRect(inner, 0.0f, 0.5f, inner.width - OptionsControlWidth(control) - OPTIONS_ROW_PADDING, 1.0f);
}

static float OptionsFittedSize(Font font, const char *text, float width, float size) {
    for (int attempt = 0; attempt < OPTIONS_FIT_ATTEMPTS; attempt++) {
        float measured = MeasureTextEx(font, text, size, 1.0f).x;
        if (measured <= width) break;
        size *= width / measured;
    }
    return size;
}

static void
OptionsDrawFittedText(Font font, const char *text, Rectangle area, float size, Color color, OptionsAlign align) {
    size = OptionsFittedSize(font, text, area.width, size);
    if (align == OPTIONS_ALIGN_CENTRE)
        UiDrawTextCentered(font, text, area, size, color);
    else if (align == OPTIONS_ALIGN_RIGHT)
        UiDrawTextRight(font, text, area, size, color);
    else
        UiDrawTextLeftMiddle(font, text, area, size, color);
}

static Rectangle OptionsSliderBar(Rectangle control) {
    return UiRect(control, 0.0f, 0.5f, control.width - OPTIONS_SLIDER_VALUE_WIDTH, OPTIONS_SLIDER_BAR_HEIGHT);
}

static Rectangle OptionsSliderTrough(Rectangle bar) {
    return UiRect(bar, 0.5f, 0.5f, bar.width * OPTIONS_SLIDER_TROUGH_WIDTH, bar.height * OPTIONS_SLIDER_TROUGH_HEIGHT);
}

float OptionsSliderLevelAt(Rectangle trough, float x) {
    return Clamp((x - trough.x) / trough.width, 0.0f, 1.0f);
}

static void OptionsSliderSettle(int row) {
    SettingsSaveAdjustments();
    if (OPTIONS_ROWS[row].volume != SETTINGS_VOLUME_MUSIC) GuiClickSoundPlay();
}

static Rectangle OptionsSliderGrab(Rectangle control, Rectangle trough) {
    return (Rectangle){
        trough.x - OPTIONS_SLIDER_KNOB_SIZE * 0.5f,
        control.y,
        trough.width + OPTIONS_SLIDER_KNOB_SIZE,
        control.height,
    };
}

static void OptionsUpdateSlider(OptionsPopup *o, int row, Rectangle area, Rectangle control, Rectangle trough) {
    SettingsVolume volume = OPTIONS_ROWS[row].volume;
    Vector2 mouse = UiMouse();
    bool grabbed = CheckCollisionPointRec(mouse, OptionsSliderGrab(control, trough));
    if (grabbed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) o->dragged_slider = row;
    bool over = CheckCollisionPointRec(mouse, area);
    float wheel = over && o->dragged_slider == OPTIONS_NO_SLIDER ? GetMouseWheelMove() : 0.0f;
    if (wheel != 0.0f) {
        SettingsAdjustVolume(volume, SettingsVolumeLevel(volume) + wheel * OPTIONS_SLIDER_WHEEL_STEP);
        OptionsSliderSettle(row);
    }
    if (o->dragged_slider != row) return;
    SettingsAdjustVolume(volume, OptionsSliderLevelAt(trough, mouse.x));
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;
    o->dragged_slider = OPTIONS_NO_SLIDER;
    OptionsSliderSettle(row);
}

static void OptionsDrawSlider(OptionsPopup *o, int row, Rectangle area, Rectangle control) {
    Rectangle bar = OptionsSliderBar(control);
    Rectangle trough = OptionsSliderTrough(bar);
    OptionsUpdateSlider(o, row, area, control, trough);
    float level = SettingsVolumeLevel(OPTIONS_ROWS[row].volume);
    UiDrawTexture(GuiTex(GUI_MISSION_PROGRESS), bar, WHITE);
    Rectangle fill = trough;
    fill.width *= level;
    DrawRectangleGradientV(
        (int)fill.x, (int)fill.y, (int)fill.width, (int)fill.height, OPTIONS_SLIDER_FILL_TOP, OPTIONS_SLIDER_FILL_BOTTOM
    );
    Rectangle knob = {
        trough.x + trough.width * level - OPTIONS_SLIDER_KNOB_SIZE * 0.5f,
        trough.y + trough.height * 0.5f - OPTIONS_SLIDER_KNOB_SIZE * 0.5f,
        OPTIONS_SLIDER_KNOB_SIZE,
        OPTIONS_SLIDER_KNOB_SIZE,
    };
    UiDrawTexture(GuiTex(GUI_OPTIONS_SLIDER_KNOB), knob, WHITE);
    Rectangle value = UiRect(control, 1.0f, 0.5f, OPTIONS_SLIDER_VALUE_WIDTH, 1.0f);
    UiDrawTextRight(
        o->menu_font,
        TextFormat("%d%%", (int)roundf(level * OPTIONS_PERCENT)),
        value,
        OPTIONS_VALUE_FONT_SIZE,
        OPTIONS_VALUE_COLOR
    );
}

static void OptionsDrawSwitch(OptionsPopup *o, int row, Rectangle area, Rectangle control) {
    SettingsSwitch setting = OPTIONS_ROWS[row].setting;
    bool on = SettingsSwitchOn(setting);
    UiDrawTexture(GuiTex(on ? GUI_OPTIONS_TOGGLE_ON : GUI_OPTIONS_TOGGLE_OFF), control, WHITE);
    Rectangle on_half = UiRect(control, 0.0f, 0.5f, 0.5f, 1.0f);
    Rectangle off_half = UiRect(control, 1.0f, 0.5f, 0.5f, 1.0f);
    Rectangle on_label = UiRect(on_half, 0.5f, 0.5f, 1.0f, OPTIONS_TOGGLE_LABEL_HEIGHT);
    Rectangle off_label = UiRect(off_half, 0.5f, 0.5f, 1.0f, OPTIONS_TOGGLE_LABEL_HEIGHT);
    UiDrawTextCentered(o->title_font, LocGet("OPTIONS_ON"), on_label, OPTIONS_TOGGLE_FONT_SIZE, OPTIONS_TEXT_COLOR);
    UiDrawTextCentered(o->title_font, LocGet("OPTIONS_OFF"), off_label, OPTIONS_TOGGLE_FONT_SIZE, OPTIONS_TEXT_COLOR);
    if (!ButtonTouchAt(&o->switches[row], area, true, ButtonPointerNow()).clicked) return;
    GuiClickSoundPlay();
    OptionsApplySwitch(setting, !on);
}

static bool OptionsArrowClicked(Button *arrow, Rectangle area, bool backwards) {
    ButtonTouch touch = ButtonTouchAt(arrow, area, true, ButtonPointerNow());
    bool lit = touch.held || CheckCollisionPointRec(UiMouse(), area);
    Texture2D texture = lit ? arrow->active : arrow->idle;
    Rectangle source = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
    if (backwards) source.width = -source.width;
    DrawTexturePro(texture, source, area, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    return touch.clicked;
}

static void OptionsDrawChoice(OptionsPopup *o, int row, Rectangle control) {
    OptionsChoice choice = OPTIONS_ROWS[row].choice;
    Rectangle back = UiRect(control, 0.0f, 0.5f, OPTIONS_ARROW_SIZE, OPTIONS_ARROW_SIZE);
    Rectangle forward = UiRect(control, 1.0f, 0.5f, OPTIONS_ARROW_SIZE, OPTIONS_ARROW_SIZE);
    Rectangle value = UiRect(control, 0.5f, 0.5f, control.width - OPTIONS_ARROW_SIZE * 2.0f, 1.0f);
    char text[OPTIONS_TEXT_CAP];
    OptionsChoiceText(choice, text, sizeof(text));
    OptionsDrawFittedText(o->menu_font, text, value, OPTIONS_ROW_FONT_SIZE, OPTIONS_TEXT_COLOR, OPTIONS_ALIGN_CENTRE);
    bool stepped_back = OptionsArrowClicked(&o->steps[row][OPTIONS_STEP_BACK], back, true);
    bool stepped_forward = OptionsArrowClicked(&o->steps[row][OPTIONS_STEP_FORWARD], forward, false);
    if (!stepped_back && !stepped_forward) return;
    GuiClickSoundPlay();
    OptionsStepChoice(choice, stepped_back ? OPTIONS_STEP_BACK : OPTIONS_STEP_FORWARD);
}

static void OptionsDrawVersion(OptionsPopup *o, Rectangle control) {
    char version[OPTIONS_TEXT_CAP];
    OptionsVersionLine(version, sizeof(version));
    Color color = ClientUpdateStatusNow() == CLIENT_UPDATE_CURRENT ? OPTIONS_VALUE_COLOR : OPTIONS_NOTICE_COLOR;
    OptionsDrawFittedText(o->menu_font, version, control, OPTIONS_VALUE_FONT_SIZE, color, OPTIONS_ALIGN_RIGHT);
}

static void OptionsDrawRow(OptionsPopup *o, int row) {
    const OptionsRowSpec *spec = &OPTIONS_ROWS[row];
    Rectangle area = OptionsRowRect(spec);
    bool live = spec->control != OPTIONS_CONTROL_VERSION;
    bool hover = live && (CheckCollisionPointRec(UiMouse(), area) || o->dragged_slider == row);
    DrawRectangleRec(area, hover ? OPTIONS_ROW_HOVER_SHADE : OPTIONS_ROW_SHADE);
    Rectangle label = OptionsLabelRect(area, spec->control);
    OptionsDrawFittedText(
        o->menu_font, LocGet(spec->label_key), label, OPTIONS_ROW_FONT_SIZE, OPTIONS_TEXT_COLOR, OPTIONS_ALIGN_LEFT
    );
    Rectangle control = OptionsControlRect(area, spec->control);
    switch (spec->control) {
        case OPTIONS_CONTROL_SLIDER:
            OptionsDrawSlider(o, row, area, control);
            break;
        case OPTIONS_CONTROL_SWITCH:
            OptionsDrawSwitch(o, row, area, control);
            break;
        case OPTIONS_CONTROL_CHOICE:
            OptionsDrawChoice(o, row, control);
            break;
        case OPTIONS_CONTROL_LANGUAGE:
            if (ButtonDraw(&o->language, control)) OptionsShowLanguages(o);
            break;
        case OPTIONS_CONTROL_VERSION:
            OptionsDrawVersion(o, control);
            break;
    }
}

static void OptionsDrawHeading(const OptionsPopup *o, OptionsColumn column) {
    Rectangle heading = OptionsPlace(
        (Rectangle){OPTIONS_COLUMN_X[column], OPTIONS_HEADING_Y, OPTIONS_COLUMN_WIDTH, OPTIONS_HEADING_HEIGHT}
    );
    UiDrawTextCentered(
        o->title_font,
        LocGet(OPTIONS_COLUMN_HEADING_KEYS[column]),
        heading,
        OPTIONS_HEADING_FONT_SIZE,
        OPTIONS_HEADING_COLOR
    );
    Rectangle rule = {heading.x, heading.y + heading.height, heading.width, OPTIONS_HEADING_RULE_HEIGHT};
    UiDrawTexture(GuiTex(GUI_OPTIONS_RULE), rule, WHITE);
}

static void OptionsDrawSettings(OptionsPopup *o) {
    Rectangle panel = OptionsPlace(OPTIONS_PANEL);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), panel, WHITE);
    UiDrawTextCentered(
        o->title_font,
        LocGet("OPTIONS_GAME_SETTINGS_TITLE"),
        OptionsPlace(OPTIONS_TITLE),
        OPTIONS_TITLE_FONT_SIZE,
        OPTIONS_TEXT_COLOR
    );
    for (int column = 0; column < OPTIONS_COLUMN_COUNT; column++) OptionsDrawHeading(o, column);
    DrawRectangleRec(OptionsPlace(OPTIONS_DIVIDER), OPTIONS_DIVIDER_COLOR);
    UiDrawTexture(GuiTex(GUI_OPTIONS_RULE), OptionsPlace(OPTIONS_FOOTER_RULE), WHITE);
    for (int row = 0; row < OPTIONS_ROW_COUNT; row++) OptionsDrawRow(o, row);
    bool closed = ButtonDraw(&o->close, OptionsPlace(OPTIONS_CLOSE));
    if (OptionsScrimClicked(o, panel)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        closed = true;
    }
    if (closed) OptionsPopupClose(o);
}

static Rectangle OptionsLanguageRowRect(const OptionsPopup *o, Rectangle list, int row) {
    return (Rectangle){list.x, list.y + row * LANGUAGE_ROW_HEIGHT - o->scroll, list.width, LANGUAGE_ROW_HEIGHT};
}

static void OptionsDrawLanguageRow(OptionsPopup *o, Rectangle row, int index) {
    Rectangle fill = UiRect(row, 0.0f, 0.5f, 1.0f, LANGUAGE_ROW_FILL_HEIGHT);
    UiDrawTexture(GuiTex(GUI_LANGUAGE_ROW_BG), fill, WHITE);
    Rectangle content = UiRect(row, 0.5f, 0.5f, 1.0f, LANGUAGE_ROW_CONTENT_HEIGHT);
    char name[OPTIONS_TEXT_CAP];
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
        o->page = OPTIONS_PAGE_SETTINGS;
        if (!changed) return OPTIONS_ACTION_NONE;
        SettingsSetLanguage(OptionsLanguageRowCode(o->selected_row));
        OptionsPopupClose(o);
        return OPTIONS_ACTION_RESTART;
    }
    if (ButtonDraw(&o->close, OptionsPlace(OPTIONS_CLOSE))) o->page = OPTIONS_PAGE_SETTINGS;
    if (OptionsScrimClicked(o, panel)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        o->page = OPTIONS_PAGE_SETTINGS;
    }
    return OPTIONS_ACTION_NONE;
}

OptionsAction OptionsPopupDraw(OptionsPopup *o) {
    if (!o->open) return OPTIONS_ACTION_NONE;
    Rectangle root = UiRoot();
    DrawRectangleRec(root, OPTIONS_SCRIM_COLOR);
    if (o->page == OPTIONS_PAGE_LANGUAGE) return OptionsDrawLanguages(o);
    OptionsDrawSettings(o);
    return OPTIONS_ACTION_NONE;
}

void OptionsFrameRateCounterDraw(void) {
    if (!SettingsSwitchOn(SETTINGS_SWITCH_SHOW_FPS)) return;
    Font font = GuiFont(FNT_MENU);
    char text[OPTIONS_TEXT_CAP];
    LocFormatInt(text, sizeof(text), "HODA_SETTINGS_FPS_VALUE", GetFPS());
    UiFrameBegin();
    Vector2 size = MeasureTextEx(font, text, OPTIONS_COUNTER_FONT_SIZE, 1.0f);
    Rectangle backing = {
        (UiWidth() - size.x) * 0.5f - OPTIONS_COUNTER_PADDING,
        0.0f,
        size.x + OPTIONS_COUNTER_PADDING * 2.0f,
        size.y + OPTIONS_COUNTER_PADDING,
    };
    DrawRectangleRec(backing, OPTIONS_COUNTER_SHADE);
    UiDrawTextCentered(font, text, backing, OPTIONS_COUNTER_FONT_SIZE, OPTIONS_TEXT_COLOR);
    UiFrameEnd();
}
