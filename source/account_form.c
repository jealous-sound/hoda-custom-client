#define ACCOUNT_FORM_FIELD_CAP 3
#define ACCOUNT_LOGIN_MIN_LENGTH 3
#define ACCOUNT_LOGIN_MAX_LENGTH 24
#define ACCOUNT_PASSWORD_MIN_RUNES 8
#define ACCOUNT_PASSWORD_MAX_RUNES 64
#define ACCOUNT_LOGIN_PUNCTUATION "._-"
#define ACCOUNT_DELETE_WORD "DELETE"
#define ACCOUNT_FORM_TITLE_SIZE 30.0f
#define ACCOUNT_FORM_NOTE_SIZE 19.0f
#define ACCOUNT_FORM_LABEL_SIZE 20.0f
#define ACCOUNT_FORM_FIELD_TEXT_SIZE 22.0f
#define ACCOUNT_FORM_TITLE_HEIGHT 0.13f
#define ACCOUNT_FORM_NOTE_Y 0.18f
#define ACCOUNT_FORM_NOTE_HEIGHT 0.22f
#define ACCOUNT_FORM_ROWS_Y 0.38f
#define ACCOUNT_FORM_ROW_HEIGHT 0.115f
#define ACCOUNT_FORM_ROW_STEP 0.135f
#define ACCOUNT_FORM_FIELD_BORDER 1.5f
#define ACCOUNT_FORM_LABEL_WIDTH 0.34f
#define ACCOUNT_FORM_FIELD_X 0.37f
#define ACCOUNT_FORM_FIELD_WIDTH 0.55f
#define ACCOUNT_FORM_LONE_FIELD_WIDTH 0.8f
#define ACCOUNT_FORM_SPINNER_HEIGHT 0.8f
#define ACCOUNT_FORM_BUTTON_LABEL_SCALE 0.34f
#define ACCOUNT_FORM_TEXT_COLOR ((Color){242, 236, 218, 255})
#define ACCOUNT_FORM_NOTE_COLOR ((Color){200, 192, 172, 255})
#define ACCOUNT_FORM_ERROR_COLOR ((Color){235, 110, 90, 255})
#define ACCOUNT_FORM_FIELD_BORDER_COLOR ((Color){120, 104, 76, 255})
#define ACCOUNT_FORM_FIELD_FOCUS_COLOR ((Color){226, 184, 96, 255})

typedef enum {
    ACCOUNT_FORM_SIGN_IN,
    ACCOUNT_FORM_CREATE_LOGIN,
    ACCOUNT_FORM_CHANGE_PASSWORD,
    ACCOUNT_FORM_DELETE,
    ACCOUNT_FORM_DELETE_GUEST,
    ACCOUNT_FORM_RENAME,
} AccountFormKind;

typedef enum {
    ACCOUNT_FORM_OPEN,
    ACCOUNT_FORM_CANCELLED,
    ACCOUNT_FORM_SUBMITTED,
} AccountFormEvent;

typedef struct {
    const char *label_key;
    int max_runes;
    bool masked;
} AccountFormFieldSpec;

typedef struct {
    const char *title_key;
    const char *submit_key;
    const char *note_key;
    const char *note_argument;
    int field_count;
    AccountFormFieldSpec fields[ACCOUNT_FORM_FIELD_CAP];
} AccountFormSpec;

#define ACCOUNT_FIELD_LOGIN {"HODA_ACCOUNT_LOGIN", ACCOUNT_LOGIN_MAX_LENGTH, false}
#define ACCOUNT_FIELD_PASSWORD(label) {label, ACCOUNT_PASSWORD_MAX_RUNES, true}

static const AccountFormSpec ACCOUNT_FORM_SPECS[] = {
    [ACCOUNT_FORM_SIGN_IN] =
        {
            .title_key = "LOGIN_GAME_SERVICES",
            .submit_key = "LOGIN_GAME_SERVICES",
            .field_count = 2,
            .fields = {ACCOUNT_FIELD_LOGIN, ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_PASSWORD")},
        },
    [ACCOUNT_FORM_CREATE_LOGIN] =
        {
            .title_key = "HODA_ACCOUNT_CREATE_LOGIN",
            .submit_key = "MAIN_MESSAGE_DIALOG_CONFIRM",
            .note_key = "HODA_ACCOUNT_CREATE_LOGIN_NOTE",
            .field_count = 3,
            .fields =
                {
                    ACCOUNT_FIELD_LOGIN,
                    ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_PASSWORD"),
                    ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_REPEAT_PASSWORD"),
                },
        },
    [ACCOUNT_FORM_CHANGE_PASSWORD] =
        {
            .title_key = "HODA_ACCOUNT_CHANGE_PASSWORD",
            .submit_key = "MAIN_MESSAGE_DIALOG_CONFIRM",
            .note_key = "HODA_ACCOUNT_CHANGE_PASSWORD_NOTE",
            .field_count = 3,
            .fields =
                {
                    ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_CURRENT_PASSWORD"),
                    ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_NEW_PASSWORD"),
                    ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_REPEAT_PASSWORD"),
                },
        },
    [ACCOUNT_FORM_DELETE] =
        {
            .title_key = "HODA_ACCOUNT_DELETE",
            .submit_key = "HODA_ACCOUNT_DELETE",
            .note_key = "HODA_ACCOUNT_DELETE_NOTE",
            .field_count = 1,
            .fields = {ACCOUNT_FIELD_PASSWORD("HODA_ACCOUNT_PASSWORD")},
        },
    [ACCOUNT_FORM_DELETE_GUEST] =
        {
            .title_key = "HODA_ACCOUNT_DELETE",
            .submit_key = "HODA_ACCOUNT_DELETE",
            .note_key = "HODA_ACCOUNT_DELETE_GUEST_NOTE",
            .note_argument = ACCOUNT_DELETE_WORD,
            .field_count = 1,
            .fields = {{NULL, (int)sizeof(ACCOUNT_DELETE_WORD) - 1, false}},
        },
    [ACCOUNT_FORM_RENAME] = {
        .title_key = "ENTER_NAME_PROMPT",
        .submit_key = "ENTER_NAME_OK_BUTTON",
        .field_count = 1,
        .fields = {{NULL, PLAYER_NAME_MAX_CHARACTERS, false}},
    },
};

typedef struct {
    bool open;
    bool busy;
    int field_at_work;
    AccountFormKind kind;
    TextField fields[ACCOUNT_FORM_FIELD_CAP];
    const char *note_key;
    const char *error_key;
    Button cancel;
    Button submit;
} AccountForm;

bool AccountLoginValid(const char *login) {
    size_t length = strlen(login);
    if (length < ACCOUNT_LOGIN_MIN_LENGTH || length > ACCOUNT_LOGIN_MAX_LENGTH || !isalnum((unsigned char)login[0]))
        return false;
    for (const unsigned char *c = (const unsigned char *)login; *c; c++)
        if (*c >= 0x80 || (!isalnum(*c) && !strchr(ACCOUNT_LOGIN_PUNCTUATION, *c))) return false;
    return true;
}

bool AccountPasswordValid(const char *password) {
    int runes = TextFieldRuneCount(password);
    return runes >= ACCOUNT_PASSWORD_MIN_RUNES && runes <= ACCOUNT_PASSWORD_MAX_RUNES;
}

static const AccountFormSpec *AccountFormSpecOf(const AccountForm *form) {
    return &ACCOUNT_FORM_SPECS[form->kind];
}

static void AccountFormWipe(AccountForm *form) {
    for (int i = 0; i < ACCOUNT_FORM_FIELD_CAP; i++) memset(form->fields[i].text, 0, sizeof(form->fields[i].text));
}

void AccountFormInitialize(AccountForm *form) {
    *form = (AccountForm){0};
    Vector2 centre = {0.5f, 0.5f};
    Font font = GuiFont(FNT_COOL);
    form->cancel = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("COMBINE_BUTTON_CANCEL"), font, centre, ACCOUNT_FORM_BUTTON_LABEL_SCALE
    );
    form->submit = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, NULL, font, centre, ACCOUNT_FORM_BUTTON_LABEL_SCALE);
    ButtonSetLabelColor(&form->submit, WHITE);
}

void AccountFormOpen(AccountForm *form, AccountFormKind kind) {
    const AccountFormSpec *spec = &ACCOUNT_FORM_SPECS[kind];
    form->open = true;
    form->busy = false;
    form->kind = kind;
    form->note_key = spec->note_key;
    form->error_key = NULL;
    form->submit.label = LocGet(spec->submit_key);
    for (int i = 0; i < spec->field_count; i++) {
        TextFieldInit(&form->fields[i], spec->fields[i].max_runes, false);
        form->fields[i].masked = spec->fields[i].masked;
    }
    TextFieldFocus(&form->fields[0]);
}

void AccountFormClose(AccountForm *form) {
    AccountFormWipe(form);
    form->open = false;
    form->busy = false;
}

bool AccountFormIsOpen(const AccountForm *form) {
    return form->open;
}

const char *AccountFormText(const AccountForm *form, int index) {
    return form->fields[index].text;
}

void AccountFormSetNote(AccountForm *form, const char *note_key) {
    form->note_key = note_key;
}

static int AccountFormFocusedField(const AccountForm *form) {
    for (int i = 0; i < ACCOUNT_FORM_SPECS[form->kind].field_count; i++)
        if (form->fields[i].focused) return i;
    return 0;
}

void AccountFormBeginWork(AccountForm *form) {
    form->field_at_work = AccountFormFocusedField(form);
    form->busy = true;
    form->error_key = NULL;
}

void AccountFormFail(AccountForm *form, const char *error_key) {
    if (form->busy) TextFieldFocus(&form->fields[form->field_at_work]);
    form->busy = false;
    form->error_key = error_key;
    GuiSoundPlay(SND_STORE_ERROR);
}

static const char *AccountFormPasswordPairProblem(const AccountForm *form, int first) {
    if (!AccountPasswordValid(AccountFormText(form, first))) return "HODA_ACCOUNT_PASSWORD_RULES";
    bool repeated = strcmp(AccountFormText(form, first), AccountFormText(form, first + 1)) == 0;
    return repeated ? NULL : "HODA_ACCOUNT_PASSWORD_MISMATCH";
}

const char *AccountFormProblem(const AccountForm *form) {
    switch (form->kind) {
        case ACCOUNT_FORM_CREATE_LOGIN:
            if (!AccountLoginValid(AccountFormText(form, 0))) return "HODA_ACCOUNT_LOGIN_RULES";
            return AccountFormPasswordPairProblem(form, 1);
        case ACCOUNT_FORM_CHANGE_PASSWORD:
            return AccountFormPasswordPairProblem(form, 1);
        case ACCOUNT_FORM_DELETE_GUEST:
            return strcmp(AccountFormText(form, 0), ACCOUNT_DELETE_WORD) == 0 ? NULL : "HODA_ACCOUNT_DELETE_MISTYPED";
        case ACCOUNT_FORM_RENAME:
            return PlayerNameValid(AccountFormText(form, 0)) ? NULL : "ERROR_INVALID_CHARACTER_IN_NAME_MESSAGE";
        default:
            return NULL;
    }
}

const char *AccountRefusalKey(BackendAccountState state, const BackendAccountAnswer *answer) {
    static const struct {
        const char *code;
        const char *key;
    } refusals[] = {
        {"wrong_credentials", "HODA_ACCOUNT_WRONG_CREDENTIALS"},
        {"wrong_password", "HODA_ACCOUNT_WRONG_PASSWORD"},
        {"login_taken", "HODA_ACCOUNT_LOGIN_TAKEN"},
        {"invalid_login", "HODA_ACCOUNT_LOGIN_RULES"},
        {"invalid_password", "HODA_ACCOUNT_PASSWORD_RULES"},
    };
    if (state == BACKEND_ACCOUNT_LIMITED) return "HODA_ACCOUNT_TOO_MANY_ATTEMPTS";
    for (size_t i = 0; state == BACKEND_ACCOUNT_REFUSED && i < sizeof(refusals) / sizeof(refusals[0]); i++)
        if (!strcmp(answer->refusal, refusals[i].code)) return refusals[i].key;
    return "HODA_ACCOUNT_UNREACHABLE";
}

static bool AccountFormFilled(const AccountForm *form) {
    for (int i = 0; i < AccountFormSpecOf(form)->field_count; i++)
        if (!form->fields[i].text[0]) return false;
    return true;
}

static void AccountFormFocusNext(AccountForm *form) {
    int count = AccountFormSpecOf(form)->field_count;
    int focused = AccountFormFocusedField(form);
    TextFieldBlur(&form->fields[focused]);
    TextFieldFocus(&form->fields[(focused + 1) % count]);
}

static void AccountFormDrawNote(const AccountForm *form, Rectangle content, Font font) {
    Rectangle area = UiRect(content, 0.5f, ACCOUNT_FORM_NOTE_Y, 1.0f, ACCOUNT_FORM_NOTE_HEIGHT);
    if (form->busy) {
        Rectangle spinner = LoadingIndicatorBox(area, 0.5f, 0.5f, ACCOUNT_FORM_SPINNER_HEIGHT);
        LoadingIndicatorDraw(GuiTex(GUI_LOADING_RING), spinner);
        return;
    }
    const char *key = form->error_key ? form->error_key : form->note_key;
    if (!key) return;
    char text[MESSAGE_POPUP_TEXT_CAP];
    const char *argument = AccountFormSpecOf(form)->note_argument;
    LocFormat1(text, sizeof(text), key, argument ? argument : "");
    Color color = form->error_key ? ACCOUNT_FORM_ERROR_COLOR : ACCOUNT_FORM_NOTE_COLOR;
    MessagePopupDrawText(font, text, area, ACCOUNT_FORM_NOTE_SIZE, color);
}

static bool AccountFormDrawFields(AccountForm *form, Rectangle content, Font font) {
    const AccountFormSpec *spec = AccountFormSpecOf(form);
    float unused_rows = (float)(ACCOUNT_FORM_FIELD_CAP - spec->field_count);
    float top = ACCOUNT_FORM_ROWS_Y + unused_rows * ACCOUNT_FORM_ROW_STEP * 0.5f;
    bool entered = false;
    for (int i = 0; i < spec->field_count; i++) {
        float y = content.y + content.height * (top + i * ACCOUNT_FORM_ROW_STEP);
        Rectangle row = {content.x, y, content.width, content.height * ACCOUNT_FORM_ROW_HEIGHT};
        Rectangle box = UiRect(row, 0.5f, 0.0f, ACCOUNT_FORM_LONE_FIELD_WIDTH, 1.0f);
        const char *label_key = spec->fields[i].label_key;
        if (label_key) {
            Rectangle label = {row.x, row.y, row.width * ACCOUNT_FORM_LABEL_WIDTH, row.height};
            UiDrawTextRight(font, LocGet(label_key), label, ACCOUNT_FORM_LABEL_SIZE, ACCOUNT_FORM_NOTE_COLOR);
            box = (Rectangle){
                row.x + row.width * ACCOUNT_FORM_FIELD_X, row.y, row.width * ACCOUNT_FORM_FIELD_WIDTH, row.height
            };
        }
        TextFieldEvent event =
            TextFieldDraw(&form->fields[i], box, font, ACCOUNT_FORM_FIELD_TEXT_SIZE, "", !form->busy);
        Color border = form->fields[i].focused ? ACCOUNT_FORM_FIELD_FOCUS_COLOR : ACCOUNT_FORM_FIELD_BORDER_COLOR;
        DrawRectangleLinesEx(box, ACCOUNT_FORM_FIELD_BORDER, border);
        entered = entered || event == TEXT_FIELD_SUBMITTED;
    }
    return entered;
}

AccountFormEvent AccountFormDraw(AccountForm *form) {
    if (!form->open) return ACCOUNT_FORM_OPEN;
    Rectangle popup = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(popup);
    Font body_font = GuiFont(FNT_MENU);
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        LocGet(AccountFormSpecOf(form)->title_key),
        UiRect(content, 0.5f, 0.0f, 1.0f, ACCOUNT_FORM_TITLE_HEIGHT),
        ACCOUNT_FORM_TITLE_SIZE,
        ACCOUNT_FORM_TEXT_COLOR
    );
    AccountFormDrawNote(form, content, body_font);
    if (!form->busy && IsKeyPressed(KEY_TAB)) AccountFormFocusNext(form);
    bool entered = AccountFormDrawFields(form, content, body_font);

    Rectangle buttons = MessagePopupChoiceRow(popup);
    bool ready = !form->busy && AccountFormFilled(form);
    bool cancelled = ButtonDrawEx(&form->cancel, UiRect(buttons, 0.0f, 0.0f, 0.5f, 1.0f), !form->busy);
    bool submitted = ButtonDrawEx(&form->submit, UiRect(buttons, 1.0f, 0.0f, 0.5f, 1.0f), ready);
    if (cancelled) return ACCOUNT_FORM_CANCELLED;
    if (!ready || (!submitted && !entered)) return ACCOUNT_FORM_OPEN;
    const char *problem = AccountFormProblem(form);
    if (problem) {
        AccountFormFail(form, problem);
        return ACCOUNT_FORM_OPEN;
    }
    return ACCOUNT_FORM_SUBMITTED;
}
