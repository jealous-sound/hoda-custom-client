#define PROFILE_DESIGN_WIDTH 1280.0f
#define PROFILE_SCRIM_COLOR ((Color){0, 0, 0, 180})
#define PROFILE_TEXT_COLOR ((Color){230, 230, 230, 255})
#define PROFILE_LABEL_COLOR ((Color){176, 168, 150, 255})
#define PROFILE_NOTICE_COLOR ((Color){236, 200, 120, 255})
#define PROFILE_ERROR_COLOR ((Color){235, 110, 90, 255})
#define PROFILE_DIVIDER_COLOR ((Color){120, 104, 76, 160})
#define PROFILE_TITLE_FONT_SIZE 32.0f
#define PROFILE_NAME_FONT_SIZE 30.0f
#define PROFILE_HEADING_FONT_SIZE 24.0f
#define PROFILE_ROW_FONT_SIZE 21.0f
#define PROFILE_HINT_FONT_SIZE 19.0f
#define PROFILE_ID_FONT_SIZE 18.0f
#define PROFILE_BUTTON_LABEL_SCALE 0.3f
#define PROFILE_SMALL_BUTTON_LABEL_SCALE 0.4f
#define PROFILE_CONFIRM_LABEL_SCALE 0.34f
#define PROFILE_ROW_HEIGHT 38.0f
#define PROFILE_ACCOUNT_BUTTON_STEP 72.0f
#define PROFILE_ACCOUNT_BUTTON_COUNT 3
#define PROFILE_COPIED_SECONDS 2.0f
#define PROFILE_TASK_START_TIMEOUT_SECONDS 12.0f
#define PROFILE_SPINNER_HEIGHT 0.8f
#define PROFILE_WORK_SPINNER_HEIGHT 0.2f
#define PROFILE_TEXT_CAP 160
#define PROFILE_EMPTY_VALUE "-"
#define PROFILE_DATE_FORMAT "%Y-%m-%d"

static const Rectangle PROFILE_PANEL = {108.0f, 110.0f, 1064.0f, 500.0f};
static const Rectangle PROFILE_TITLE = {108.0f, 132.0f, 1064.0f, 60.0f};
static const Rectangle PROFILE_CLOSE = {1116.0f, 92.0f, 73.0f, 73.0f};
static const Rectangle PROFILE_NAME = {206.0f, 208.0f, 300.0f, 46.0f};
static const Rectangle PROFILE_RENAME = {516.0f, 208.0f, 136.0f, 46.0f};
static const Rectangle PROFILE_ROW_LABEL = {206.0f, 270.0f, 170.0f, PROFILE_ROW_HEIGHT};
static const Rectangle PROFILE_ROW_VALUE = {376.0f, 270.0f, 280.0f, PROFILE_ROW_HEIGHT};
static const Rectangle PROFILE_PLAYER_ID = {206.0f, 508.0f, 300.0f, 46.0f};
static const Rectangle PROFILE_COPY = {516.0f, 508.0f, 136.0f, 46.0f};
static const Rectangle PROFILE_DIVIDER = {683.0f, 212.0f, 2.0f, 340.0f};
static const Rectangle PROFILE_ACCOUNT_HEADING = {712.0f, 208.0f, 384.0f, 40.0f};
static const Rectangle PROFILE_ACCOUNT_HINT = {712.0f, 250.0f, 384.0f, 82.0f};
static const Rectangle PROFILE_ACCOUNT_BUTTON = {768.0f, 340.0f, 272.0f, 68.0f};

typedef enum {
    PROFILE_ACTION_NONE,
    PROFILE_ACTION_ACCOUNT_CHANGED,
} ProfileAction;

typedef enum {
    PROFILE_TASK_NONE,
    PROFILE_TASK_SHOW_ACCOUNT,
    PROFILE_TASK_CREATE_LOGIN,
    PROFILE_TASK_SIGN_IN,
    PROFILE_TASK_CHANGE_PASSWORD,
    PROFILE_TASK_SIGN_OUT,
    PROFILE_TASK_DELETE,
} ProfileTask;

typedef struct {
    BackendAccountCall call;
    bool waits_for_saved_progress;
    bool leaves_account;
    const char *done_notice_key;
} ProfileTaskSpec;

static const ProfileTaskSpec PROFILE_TASKS[] = {
    [PROFILE_TASK_SHOW_ACCOUNT] = {BACKEND_ACCOUNT_SHOW},
    [PROFILE_TASK_CREATE_LOGIN] = {BACKEND_ACCOUNT_CHOOSE_LOGIN, .done_notice_key = "HODA_ACCOUNT_LOGIN_CREATED"},
    [PROFILE_TASK_SIGN_IN] = {BACKEND_ACCOUNT_SIGN_IN, .leaves_account = true},
    [PROFILE_TASK_CHANGE_PASSWORD] =
        {BACKEND_ACCOUNT_CHANGE_PASSWORD, .done_notice_key = "HODA_ACCOUNT_PASSWORD_CHANGED"},
    [PROFILE_TASK_SIGN_OUT] = {BACKEND_ACCOUNT_SIGN_OUT, .waits_for_saved_progress = true, .leaves_account = true},
    [PROFILE_TASK_DELETE] = {BACKEND_ACCOUNT_DELETE, .leaves_account = true},
};

typedef enum {
    PROFILE_BUTTON_CREATE_LOGIN,
    PROFILE_BUTTON_SIGN_IN,
    PROFILE_BUTTON_CHANGE_PASSWORD,
    PROFILE_BUTTON_SIGN_OUT,
    PROFILE_BUTTON_DELETE,
    PROFILE_BUTTON_COUNT,
} ProfileButton;

static const char *const PROFILE_BUTTON_LABEL_KEYS[PROFILE_BUTTON_COUNT] = {
    [PROFILE_BUTTON_CREATE_LOGIN] = "HODA_ACCOUNT_CREATE_LOGIN",
    [PROFILE_BUTTON_SIGN_IN] = "LOGIN_GAME_SERVICES",
    [PROFILE_BUTTON_CHANGE_PASSWORD] = "HODA_ACCOUNT_CHANGE_PASSWORD",
    [PROFILE_BUTTON_SIGN_OUT] = "OPTIONS_GPGS_LOGOUT_BUTTON_TEXT",
    [PROFILE_BUTTON_DELETE] = "HODA_ACCOUNT_DELETE",
};

static const ProfileButton PROFILE_GUEST_BUTTONS[PROFILE_ACCOUNT_BUTTON_COUNT] = {
    PROFILE_BUTTON_CREATE_LOGIN, PROFILE_BUTTON_SIGN_IN, PROFILE_BUTTON_DELETE
};
static const ProfileButton PROFILE_MEMBER_BUTTONS[PROFILE_ACCOUNT_BUTTON_COUNT] = {
    PROFILE_BUTTON_CHANGE_PASSWORD, PROFILE_BUTTON_SIGN_OUT, PROFILE_BUTTON_DELETE
};

typedef struct {
    bool open;
    bool scrim_armed;
    bool confirming_sign_out;
    ProfileTask task;
    bool task_sent;
    float task_seconds;
    cJSON *task_fields;
    bool account_asked;
    bool account_known;
    BackendAccountAnswer account;
    const char *notice_key;
    bool notice_is_error;
    float copied_seconds;
    AccountForm form;
    Button close;
    Button rename;
    Button copy;
    Button account_buttons[PROFILE_BUTTON_COUNT];
    Button confirm_decline;
    Button confirm_accept;
} ProfilePopup;

void ProfilePopupInitialize(ProfilePopup *p) {
    *p = (ProfilePopup){0};
    Font font = GuiFont(FNT_COOL);
    Vector2 centre = {0.5f, 0.5f};
    p->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&p->close, SND_UI_POPUP_CLOSE);
    p->rename = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_PROFILE_RENAME"), font, centre, PROFILE_SMALL_BUTTON_LABEL_SCALE
    );
    p->copy = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, NULL, font, centre, PROFILE_SMALL_BUTTON_LABEL_SCALE);
    for (int i = 0; i < PROFILE_BUTTON_COUNT; i++)
        p->account_buttons[i] = ButtonLoadText(
            GUI_BTN_GREY,
            GUI_BTN_GREY_ON,
            LocGet(PROFILE_BUTTON_LABEL_KEYS[i]),
            font,
            centre,
            PROFILE_BUTTON_LABEL_SCALE
        );
    p->confirm_decline = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("COMBINE_BUTTON_CANCEL"), font, centre, PROFILE_CONFIRM_LABEL_SCALE
    );
    p->confirm_accept = ButtonLoadText(
        GUI_BTN_GOLD,
        GUI_BTN_GOLD_ON,
        LocGet("OPTIONS_GPGS_LOGOUT_BUTTON_TEXT"),
        font,
        centre,
        PROFILE_CONFIRM_LABEL_SCALE
    );
    ButtonSetLabelColor(&p->confirm_accept, WHITE);
    AccountFormInitialize(&p->form);
}

bool ProfilePopupIsOpen(const ProfilePopup *p) {
    return p->open;
}

static bool ProfileWorking(const ProfilePopup *p) {
    return p->task != PROFILE_TASK_NONE && p->task != PROFILE_TASK_SHOW_ACCOUNT;
}

bool ProfilePopupModal(const ProfilePopup *p) {
    return p->open && (AccountFormIsOpen(&p->form) || p->confirming_sign_out || ProfileWorking(p));
}

static void ProfileBeginTask(ProfilePopup *p, ProfileTask task, cJSON *fields) {
    cJSON_Delete(p->task_fields);
    p->task = task;
    p->task_fields = fields;
    p->task_sent = false;
    p->task_seconds = 0.0f;
    p->notice_key = NULL;
}

static void ProfileEndTask(ProfilePopup *p) {
    cJSON_Delete(p->task_fields);
    p->task_fields = NULL;
    p->task = PROFILE_TASK_NONE;
}

void ProfilePopupOpen(ProfilePopup *p) {
    p->open = true;
    p->scrim_armed = false;
    p->confirming_sign_out = false;
    p->account_asked = false;
    p->account_known = false;
    p->notice_key = NULL;
    p->copied_seconds = 0.0f;
    AccountFormClose(&p->form);
}

void ProfilePopupClose(ProfilePopup *p) {
    if (ProfileWorking(p)) return;
    ProfileEndTask(p);
    AccountFormClose(&p->form);
    p->confirming_sign_out = false;
    p->open = false;
}

bool ProfilePopupBack(ProfilePopup *p) {
    if (!p->open) return false;
    if (ProfileWorking(p)) return true;
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    if (p->confirming_sign_out)
        p->confirming_sign_out = false;
    else if (AccountFormIsOpen(&p->form))
        AccountFormClose(&p->form);
    else
        ProfilePopupClose(p);
    return true;
}

static Rectangle ProfilePlace(Rectangle design) {
    float offset = (UiWidth() - PROFILE_DESIGN_WIDTH) * 0.5f;
    return (Rectangle){design.x + offset, design.y, design.width, design.height};
}

static void ProfileShowNotice(ProfilePopup *p, const char *key, bool is_error) {
    p->notice_key = key;
    p->notice_is_error = is_error;
    if (is_error) GuiSoundPlay(SND_STORE_ERROR);
}

static void ProfileFailTask(ProfilePopup *p, const char *error_key) {
    bool asked_by_form = AccountFormIsOpen(&p->form) && p->task != PROFILE_TASK_SHOW_ACCOUNT;
    ProfileEndTask(p);
    if (asked_by_form)
        AccountFormFail(&p->form, error_key);
    else
        ProfileShowNotice(p, error_key, true);
}

static ProfileAction ProfileFinishTask(ProfilePopup *p, const BackendAccountAnswer *answer) {
    const ProfileTaskSpec *spec = &PROFILE_TASKS[p->task];
    bool asked_by_form = p->task != PROFILE_TASK_SHOW_ACCOUNT;
    ProfileEndTask(p);
    if (asked_by_form) AccountFormClose(&p->form);
    if (spec->leaves_account) {
        p->open = false;
        return PROFILE_ACTION_ACCOUNT_CHANGED;
    }
    p->account = *answer;
    p->account_known = true;
    if (spec->done_notice_key) ProfileShowNotice(p, spec->done_notice_key, false);
    return PROFILE_ACTION_NONE;
}

static bool ProfileSendTask(ProfilePopup *p) {
    const ProfileTaskSpec *spec = &PROFILE_TASKS[p->task];
    if (spec->waits_for_saved_progress && BackendUploadPending()) return false;
    cJSON *fields = p->task_fields ? cJSON_Duplicate(p->task_fields, true) : NULL;
    if (p->task_fields && !fields) return false;
    return BackendBeginAccountCall(spec->call, fields);
}

static ProfileAction ProfileUpdateTask(ProfilePopup *p, float dt) {
    if (p->task == PROFILE_TASK_NONE && !p->account_asked && BackendAccountManaged()) {
        p->account_asked = true;
        ProfileBeginTask(p, PROFILE_TASK_SHOW_ACCOUNT, NULL);
    }
    if (p->task == PROFILE_TASK_NONE) return PROFILE_ACTION_NONE;
    if (!p->task_sent) {
        p->task_sent = ProfileSendTask(p);
        p->task_seconds += dt;
        if (!p->task_sent && p->task_seconds > PROFILE_TASK_START_TIMEOUT_SECONDS)
            ProfileFailTask(p, "HODA_ACCOUNT_UNREACHABLE");
        return PROFILE_ACTION_NONE;
    }
    BackendAccountAnswer answer;
    BackendAccountState state = BackendPollAccountCall(&answer);
    if (state == BACKEND_ACCOUNT_PENDING) return PROFILE_ACTION_NONE;
    if (state == BACKEND_ACCOUNT_DONE) return ProfileFinishTask(p, &answer);
    ProfileFailTask(p, AccountRefusalKey(state, &answer));
    return PROFILE_ACTION_NONE;
}

static cJSON *ProfileFields(const char *first_key, const char *first, const char *second_key, const char *second) {
    cJSON *fields = cJSON_CreateObject();
    if (!fields) return NULL;
    cJSON_AddStringToObject(fields, first_key, first);
    if (second_key) cJSON_AddStringToObject(fields, second_key, second);
    return fields;
}

static void ProfileSubmitForm(ProfilePopup *p, Player *player) {
    AccountForm *form = &p->form;
    const char *first = AccountFormText(form, 0), *second = AccountFormText(form, 1);
    switch (form->kind) {
        case ACCOUNT_FORM_RENAME:
            StrCopy(player->name, sizeof(player->name), first);
            ProgressionSave(player);
            AccountFormClose(form);
            return;
        case ACCOUNT_FORM_SIGN_IN:
            ProfileBeginTask(p, PROFILE_TASK_SIGN_IN, ProfileFields("login", first, "password", second));
            break;
        case ACCOUNT_FORM_CREATE_LOGIN:
            ProfileBeginTask(p, PROFILE_TASK_CREATE_LOGIN, ProfileFields("login", first, "password", second));
            break;
        case ACCOUNT_FORM_CHANGE_PASSWORD:
            ProfileBeginTask(
                p, PROFILE_TASK_CHANGE_PASSWORD, ProfileFields("current_password", first, "new_password", second)
            );
            break;
        case ACCOUNT_FORM_DELETE:
            ProfileBeginTask(p, PROFILE_TASK_DELETE, ProfileFields("password", first, NULL, NULL));
            break;
        case ACCOUNT_FORM_DELETE_GUEST:
            ProfileBeginTask(p, PROFILE_TASK_DELETE, ProfileFields("password", "", NULL, NULL));
            break;
    }
    AccountFormBeginWork(form);
}

static bool ProfileSignedIn(const ProfilePopup *p) {
    return p->account_known && p->account.login[0];
}

static void ProfilePressAccountButton(ProfilePopup *p, Player *player, ProfileButton button) {
    switch (button) {
        case PROFILE_BUTTON_CREATE_LOGIN:
            AccountFormOpen(&p->form, ACCOUNT_FORM_CREATE_LOGIN);
            break;
        case PROFILE_BUTTON_SIGN_IN:
            AccountFormOpen(&p->form, ACCOUNT_FORM_SIGN_IN);
            AccountFormSetNote(&p->form, "HODA_ACCOUNT_SWITCH_WARNING");
            break;
        case PROFILE_BUTTON_CHANGE_PASSWORD:
            AccountFormOpen(&p->form, ACCOUNT_FORM_CHANGE_PASSWORD);
            break;
        case PROFILE_BUTTON_SIGN_OUT:
            ProgressionSave(player);
            p->confirming_sign_out = true;
            break;
        case PROFILE_BUTTON_DELETE:
            AccountFormOpen(&p->form, ProfileSignedIn(p) ? ACCOUNT_FORM_DELETE : ACCOUNT_FORM_DELETE_GUEST);
            break;
        default:
            break;
    }
}

static void ProfileDrawRow(Font font, int row, const char *label_key, const char *value) {
    Rectangle label = PROFILE_ROW_LABEL, text = PROFILE_ROW_VALUE;
    label.y += row * PROFILE_ROW_HEIGHT;
    text.y += row * PROFILE_ROW_HEIGHT;
    UiDrawTextLeftMiddle(font, LocGet(label_key), ProfilePlace(label), PROFILE_ROW_FONT_SIZE, PROFILE_LABEL_COLOR);
    UiDrawTextLeftMiddle(
        font,
        value && value[0] ? value : PROFILE_EMPTY_VALUE,
        ProfilePlace(text),
        PROFILE_ROW_FONT_SIZE,
        PROFILE_TEXT_COLOR
    );
}

static void ProfileFormatJoined(const ProfilePopup *p, char *out, size_t capacity) {
    out[0] = '\0';
    if (!p->account_known || p->account.created_at <= 0) return;
    time_t created = (time_t)p->account.created_at;
    struct tm *local = localtime(&created);
    if (local) strftime(out, capacity, PROFILE_DATE_FORMAT, local);
}

static void ProfileDrawCard(ProfilePopup *p, Player *player, const char *guild_name, bool interactive) {
    Font font = GuiFont(FNT_MENU);
    UiDrawTextLeftMiddle(
        font,
        player->name[0] ? player->name : PROFILE_EMPTY_VALUE,
        ProfilePlace(PROFILE_NAME),
        PROFILE_NAME_FONT_SIZE,
        PROFILE_TEXT_COLOR
    );
    bool renamable = player->name[0] && ProgressionPersistenceEnabled();
    if (renamable && ButtonDrawEx(&p->rename, ProfilePlace(PROFILE_RENAME), interactive)) {
        AccountFormOpen(&p->form, ACCOUNT_FORM_RENAME);
        TextFieldSet(&p->form.fields[0], player->name);
    }
    char value[PROFILE_TEXT_CAP];
    int row = 0;
    ProfileDrawRow(font, row++, "LEADERBOARD_LEVEL", TextFormat("%d", player->level));
    LocFormatInt2(value, sizeof(value), "HODA_PROFILE_TROPHIES_VALUE", player->pvp.trophies, player->pvp.best_trophies);
    ProfileDrawRow(font, row++, "LEADERBOARD_TROPHIES", value);
    LocFormatInt2(value, sizeof(value), "HODA_PROFILE_PVP_VALUE", player->pvp.wins, player->pvp.losses);
    ProfileDrawRow(font, row++, "HODA_PROFILE_PVP", value);
    ProfileDrawRow(font, row++, "BUTTON_MAINSCREEN_HEROES", TextFormat("%d", player->roster_count));
    ProfileDrawRow(font, row++, "GUILD_HOME_TITLE", guild_name);
    if (BackendAccountManaged()) {
        ProfileFormatJoined(p, value, sizeof(value));
        ProfileDrawRow(font, row++, "HODA_PROFILE_PLAYING_SINCE", value);
    }
    const char *player_id = BackendCurrentPlayerId();
    if (!player_id || !player_id[0]) return;
    LocFormat1(value, sizeof(value), "OPTIONS_PLAYER_ID", player_id);
    UiDrawTextLeftMiddle(font, value, ProfilePlace(PROFILE_PLAYER_ID), PROFILE_ID_FONT_SIZE, PROFILE_LABEL_COLOR);
    p->copied_seconds = fmaxf(0.0f, p->copied_seconds - GetFrameTime());
    p->copy.label = LocGet(p->copied_seconds > 0.0f ? "HODA_PROFILE_COPIED" : "HODA_PROFILE_COPY_ID");
    if (ButtonDrawEx(&p->copy, ProfilePlace(PROFILE_COPY), interactive)) {
        SetClipboardText(player_id);
        p->copied_seconds = PROFILE_COPIED_SECONDS;
    }
}

static void ProfileDrawAccountText(const ProfilePopup *p, const char *heading, const char *hint_key) {
    UiDrawTextCentered(
        GuiFont(FNT_MENU), heading, ProfilePlace(PROFILE_ACCOUNT_HEADING), PROFILE_HEADING_FONT_SIZE, PROFILE_TEXT_COLOR
    );
    const char *key = p->notice_key ? p->notice_key : hint_key;
    if (!key) return;
    Color notice_color = p->notice_is_error ? PROFILE_ERROR_COLOR : PROFILE_NOTICE_COLOR;
    char text[MESSAGE_POPUP_TEXT_CAP];
    LocFormatInt(text, sizeof(text), key, p->account.devices);
    MessagePopupDrawText(
        GuiFont(FNT_MENU),
        text,
        ProfilePlace(PROFILE_ACCOUNT_HINT),
        PROFILE_HINT_FONT_SIZE,
        p->notice_key ? notice_color : PROFILE_LABEL_COLOR
    );
}

static void ProfileDrawAccount(ProfilePopup *p, Player *player, bool interactive) {
    if (!BackendEnabled()) {
        ProfileDrawAccountText(p, LocGet("HODA_PLAY_MODE_LOCAL"), "HODA_PLAY_MODE_LOCAL_HINT");
        return;
    }
    bool keyless_player = BackendCurrentPlayerId() && !BackendAccountManaged();
    bool asking = !p->account_asked || p->task == PROFILE_TASK_SHOW_ACCOUNT;
    if (!keyless_player && !p->account_known && asking) {
        Rectangle spinner = LoadingIndicatorBox(ProfilePlace(PROFILE_ACCOUNT_HINT), 0.5f, 0.5f, PROFILE_SPINNER_HEIGHT);
        LoadingIndicatorDraw(GuiTex(GUI_LOADING_RING), spinner);
        return;
    }
    if (keyless_player || !p->account_known) {
        ProfileDrawAccountText(p, LocGet("HODA_PLAY_MODE_ONLINE"), NULL);
        return;
    }
    bool signed_in = ProfileSignedIn(p);
    char heading[PROFILE_TEXT_CAP];
    LocFormat1(heading, sizeof(heading), "HODA_ACCOUNT_SIGNED_IN", p->account.login);
    ProfileDrawAccountText(
        p,
        signed_in ? heading : LocGet("HODA_ACCOUNT_GUEST"),
        signed_in ? "HODA_ACCOUNT_DEVICES" : "HODA_ACCOUNT_GUEST_HINT"
    );
    const ProfileButton *buttons = signed_in ? PROFILE_MEMBER_BUTTONS : PROFILE_GUEST_BUTTONS;
    for (int i = 0; i < PROFILE_ACCOUNT_BUTTON_COUNT; i++) {
        Rectangle slot = PROFILE_ACCOUNT_BUTTON;
        slot.y += i * PROFILE_ACCOUNT_BUTTON_STEP;
        if (ButtonDrawEx(&p->account_buttons[buttons[i]], ProfilePlace(slot), interactive))
            ProfilePressAccountButton(p, player, buttons[i]);
    }
}

static bool ProfileScrimClicked(ProfilePopup *p, Rectangle panel) {
    Vector2 mouse = UiMouse();
    bool outside = !CheckCollisionPointRec(mouse, panel);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) p->scrim_armed = outside;
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return false;
    bool clicked = p->scrim_armed && outside;
    p->scrim_armed = false;
    return clicked;
}

static void ProfileDrawPanel(ProfilePopup *p, Player *player, const char *guild_name, bool interactive) {
    Rectangle panel = ProfilePlace(PROFILE_PANEL);
    UiDrawTexture(GuiTex(GUI_STORE_OVERLAY), panel, WHITE);
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        LocGet("HODA_OPTIONS_PROFILE"),
        ProfilePlace(PROFILE_TITLE),
        PROFILE_TITLE_FONT_SIZE,
        PROFILE_TEXT_COLOR
    );
    DrawRectangleRec(ProfilePlace(PROFILE_DIVIDER), PROFILE_DIVIDER_COLOR);
    ProfileDrawCard(p, player, guild_name, interactive);
    ProfileDrawAccount(p, player, interactive);
    bool closed = ButtonDrawEx(&p->close, ProfilePlace(PROFILE_CLOSE), interactive);
    if (interactive && (closed || ProfileScrimClicked(p, panel))) ProfilePopupClose(p);
}

static void ProfileDrawSignOutConfirmation(ProfilePopup *p) {
    MessagePopupChoice choice = MessagePopupDrawChoice(
        GuiTex(GUI_STORE_OVERLAY),
        GuiFont(FNT_COOL),
        GuiFont(FNT_MENU),
        LocGet("OPTIONS_GPGS_LOGOUT_BUTTON_TEXT"),
        LocGet("HODA_ACCOUNT_SIGN_OUT_MESSAGE"),
        &p->confirm_decline,
        &p->confirm_accept
    );
    if (choice == MESSAGE_POPUP_CHOICE_NONE) return;
    p->confirming_sign_out = false;
    if (choice == MESSAGE_POPUP_CHOICE_ACCEPT) ProfileBeginTask(p, PROFILE_TASK_SIGN_OUT, cJSON_CreateObject());
}

static void ProfileDrawWork(const ProfilePopup *p) {
    if (!ProfileWorking(p) || AccountFormIsOpen(&p->form)) return;
    Rectangle root = UiRoot();
    DrawRectangleRec(root, PROFILE_SCRIM_COLOR);
    LoadingIndicatorDraw(GuiTex(GUI_LOADING_RING), LoadingIndicatorBox(root, 0.5f, 0.5f, PROFILE_WORK_SPINNER_HEIGHT));
}

ProfileAction ProfilePopupDraw(ProfilePopup *p, Player *player, const char *guild_name) {
    if (!p->open) return PROFILE_ACTION_NONE;
    ProfileAction action = ProfileUpdateTask(p, GetFrameTime());
    if (action != PROFILE_ACTION_NONE) return action;
    DrawRectangleRec(UiRoot(), PROFILE_SCRIM_COLOR);
    ProfileDrawPanel(p, player, guild_name, !ProfilePopupModal(p));
    if (!p->open) return PROFILE_ACTION_NONE;
    if (p->confirming_sign_out) {
        ProfileDrawSignOutConfirmation(p);
    } else if (AccountFormIsOpen(&p->form)) {
        AccountFormEvent event = AccountFormDraw(&p->form);
        if (event == ACCOUNT_FORM_CANCELLED) AccountFormClose(&p->form);
        if (event == ACCOUNT_FORM_SUBMITTED) ProfileSubmitForm(p, player);
    }
    ProfileDrawWork(p);
    return PROFILE_ACTION_NONE;
}
