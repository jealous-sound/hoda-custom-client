#define ACCOUNT_GATE_MESSAGE_Y 0.62f
#define ACCOUNT_GATE_MESSAGE_WIDTH 0.7f
#define ACCOUNT_GATE_MESSAGE_HEIGHT 0.12f
#define ACCOUNT_GATE_MESSAGE_FONT_SIZE 22.0f
#define ACCOUNT_GATE_MESSAGE_COLOR ((Color){236, 200, 120, 255})
#define ACCOUNT_GATE_MESSAGE_BACKING ((Color){0, 0, 0, 170})
#define ACCOUNT_GATE_MESSAGE_ROUNDNESS 0.3f
#define ACCOUNT_GATE_MESSAGE_SEGMENTS 8

typedef enum {
    ACCOUNT_GATE_UNDECIDED,
    ACCOUNT_GATE_GUEST,
    ACCOUNT_GATE_SIGNED_IN,
    ACCOUNT_GATE_QUIT,
} AccountGateChoice;

typedef struct {
    Texture2D splash;
    const char *message_key;
    bool signing_in;
    Button guest;
    Button sign_in;
    AccountForm form;
} AccountGate;

void AccountGateInitialize(AccountGate *gate, Texture2D splash, const char *message_key) {
    *gate = (AccountGate){.splash = splash, .message_key = message_key};
    Font font = GuiFont(FNT_COOL);
    Vector2 centre = {0.5f, 0.5f};
    gate->guest = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("LOGIN_PLAY_AS_GUEST"), font, centre, PLAY_MODE_BUTTON_LABEL_SCALE
    );
    gate->sign_in = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("LOGIN_GAME_SERVICES"), font, centre, PLAY_MODE_BUTTON_LABEL_SCALE
    );
    AccountFormInitialize(&gate->form);
}

static void AccountGateSubmit(AccountGate *gate) {
    cJSON *fields = cJSON_CreateObject();
    if (fields) {
        cJSON_AddStringToObject(fields, "login", AccountFormText(&gate->form, 0));
        cJSON_AddStringToObject(fields, "password", AccountFormText(&gate->form, 1));
    }
    AccountFormBeginWork(&gate->form);
    gate->signing_in = fields && BackendBeginAccountCall(BACKEND_ACCOUNT_SIGN_IN, fields);
    if (!gate->signing_in) AccountFormFail(&gate->form, "HODA_ACCOUNT_UNREACHABLE");
}

static bool AccountGateSignedIn(AccountGate *gate) {
    if (!gate->signing_in) return false;
    BackendAccountAnswer answer;
    BackendAccountState state = BackendPollAccountCall(&answer);
    if (state == BACKEND_ACCOUNT_PENDING) return false;
    gate->signing_in = false;
    if (state == BACKEND_ACCOUNT_DONE) return true;
    AccountFormFail(&gate->form, AccountRefusalKey(state, &answer));
    return false;
}

static void AccountGateDrawMessage(const AccountGate *gate) {
    if (!gate->message_key) return;
    Rectangle box =
        UiRect(UiRoot(), 0.5f, ACCOUNT_GATE_MESSAGE_Y, ACCOUNT_GATE_MESSAGE_WIDTH, ACCOUNT_GATE_MESSAGE_HEIGHT);
    float size = ACCOUNT_GATE_MESSAGE_FONT_SIZE * ContentCanvasScale();
    DrawRectangleRounded(
        box, ACCOUNT_GATE_MESSAGE_ROUNDNESS, ACCOUNT_GATE_MESSAGE_SEGMENTS, ACCOUNT_GATE_MESSAGE_BACKING
    );
    MessagePopupDrawText(GuiFont(FNT_MENU), LocGet(gate->message_key), box, size, ACCOUNT_GATE_MESSAGE_COLOR);
}

static void AccountGateDrawForm(AccountGate *gate) {
    AccountFormEvent event = AccountFormDraw(&gate->form);
    if (event == ACCOUNT_FORM_SUBMITTED) AccountGateSubmit(gate);
    bool left = event == ACCOUNT_FORM_CANCELLED || (!gate->signing_in && IsKeyPressed(KEY_ESCAPE));
    if (left) AccountFormClose(&gate->form);
}

AccountGateChoice AccountGateDraw(AccountGate *gate) {
    if (AccountGateSignedIn(gate)) return ACCOUNT_GATE_SIGNED_IN;
    PlayModeDrawBackdrop(gate->splash);
    if (AccountFormIsOpen(&gate->form)) {
        AccountGateDrawForm(gate);
        return ACCOUNT_GATE_UNDECIDED;
    }
    PlayModeButtonRow row = PlayModeLayOutButtons(gate->guest.idle);
    Font hint_font = GuiFont(FNT_MENU);
    PlayModeDrawHint(hint_font, LocGet("HODA_ACCOUNT_GUEST_START_HINT"), row.first);
    PlayModeDrawHint(hint_font, LocGet("HODA_ACCOUNT_LOG_IN_HINT"), row.second);
    AccountGateDrawMessage(gate);
    if (ButtonDraw(&gate->guest, row.first)) return ACCOUNT_GATE_GUEST;
    if (ButtonDraw(&gate->sign_in, row.second)) AccountFormOpen(&gate->form, ACCOUNT_FORM_SIGN_IN);
    return IsKeyPressed(KEY_ESCAPE) ? ACCOUNT_GATE_QUIT : ACCOUNT_GATE_UNDECIDED;
}
