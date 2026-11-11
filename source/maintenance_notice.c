#define MAINTENANCE_TITLE_FALLBACK "Server Restart"
#define MAINTENANCE_MESSAGE_FALLBACK                                                                                   \
    "The server will restart soon for an update. New battles pause a few minutes before it, and you may need to "      \
    "download a new version afterwards."
#define MAINTENANCE_HOLD_MESSAGE_FALLBACK "The server is restarting for an update. Battles are paused until it is back."
#define MAINTENANCE_CLOCK_CAP 48
#define MAINTENANCE_STRIP_TEXT_CAP 160
#define MAINTENANCE_STRIP_HEIGHT 30.0f
#define MAINTENANCE_STRIP_FONT_SIZE 20.0f
#define MAINTENANCE_STRIP_PADDING 12.0f
#define MAINTENANCE_STRIP_MARGIN 8.0f
#define MAINTENANCE_STRIP_ROUNDNESS 0.5f
#define MAINTENANCE_STRIP_SEGMENTS 8
#define MAINTENANCE_STRIP_PLATE_OPACITY 0.72f
#define MAINTENANCE_POPUP_CLOCK_SIZE 46.0f
#define MAINTENANCE_SECONDS_PER_MINUTE 60
#define MAINTENANCE_ANNOUNCED_COLOR ((Color){255, 204, 92, 255})
#define MAINTENANCE_HOLDING_COLOR ((Color){255, 112, 96, 255})

typedef struct {
    Button ok;
    bool open;
    bool offline_preview;
    long long explained_restart;
} MaintenanceNotice;

void MaintenanceNoticeInitialize(MaintenanceNotice *notice) {
    memset(notice, 0, sizeof(*notice));
    notice->ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("MAIN_MESSAGE_DIALOG_OK"), GuiFont(FNT_COOL), (Vector2){0.5f, 0.5f}, 0.34f
    );
}

static ClientMaintenance MaintenanceNoticeWatch(const MaintenanceNotice *notice) {
    if (!BackendEnabled() && !notice->offline_preview) return (ClientMaintenance){.phase = CLIENT_MAINTENANCE_NONE};
    return ClientMaintenanceNow();
}

bool MaintenanceNoticeIsOpen(const MaintenanceNotice *notice) {
    return notice->open;
}

bool MaintenanceNoticeClose(MaintenanceNotice *notice) {
    if (!notice->open) return false;
    notice->open = false;
    return true;
}

bool MaintenanceNoticeHoldsBattles(const MaintenanceNotice *notice) {
    return MaintenanceNoticeWatch(notice).phase == CLIENT_MAINTENANCE_HOLDING_BATTLES;
}

void MaintenanceNoticeExplain(MaintenanceNotice *notice) {
    ClientMaintenance maintenance = MaintenanceNoticeWatch(notice);
    if (maintenance.phase == CLIENT_MAINTENANCE_NONE) return;
    notice->explained_restart = maintenance.at;
    notice->open = true;
}

void MaintenanceNoticeUpdate(MaintenanceNotice *notice, bool interruptible) {
    ClientMaintenance maintenance = MaintenanceNoticeWatch(notice);
    if (maintenance.phase == CLIENT_MAINTENANCE_NONE) {
        notice->open = false;
        return;
    }
    if (interruptible && notice->explained_restart != maintenance.at) MaintenanceNoticeExplain(notice);
}

static void MaintenanceClockText(long long seconds, char *out, size_t capacity) {
    snprintf(
        out,
        capacity,
        "%02lld:%02lld",
        seconds / MAINTENANCE_SECONDS_PER_MINUTE,
        seconds % MAINTENANCE_SECONDS_PER_MINUTE
    );
}

static Color MaintenanceColor(ClientMaintenancePhase phase) {
    return phase == CLIENT_MAINTENANCE_HOLDING_BATTLES ? MAINTENANCE_HOLDING_COLOR : MAINTENANCE_ANNOUNCED_COLOR;
}

static const char *MaintenanceTitle(void) {
    return LocGetOr("HODA_MAINTENANCE_TITLE", MAINTENANCE_TITLE_FALLBACK);
}

static Rectangle MaintenanceStripPlate(const HudLayout *hud, float text_width) {
    float width = text_width + 2.0f * MAINTENANCE_STRIP_PADDING;
    float gap_left = hud->stamina.x + hud->stamina.width + MAINTENANCE_STRIP_MARGIN;
    float gap_right = hud->gold_icon.x - MAINTENANCE_STRIP_MARGIN;
    if (gap_right - gap_left >= width)
        return (Rectangle){
            (gap_left + gap_right - width) * 0.5f,
            hud->stamina.y + (hud->stamina.height - MAINTENANCE_STRIP_HEIGHT) * 0.5f,
            width,
            MAINTENANCE_STRIP_HEIGHT,
        };
    return (Rectangle){
        (UiWidth() - width) * 0.5f,
        hud->background.y + hud->background.height - MAINTENANCE_STRIP_HEIGHT,
        width,
        MAINTENANCE_STRIP_HEIGHT,
    };
}

static void MaintenanceNoticeDrawStrip(ClientMaintenance maintenance) {
    char clock[MAINTENANCE_CLOCK_CAP];
    MaintenanceClockText(maintenance.seconds_left, clock, sizeof(clock));
    char text[MAINTENANCE_STRIP_TEXT_CAP];
    snprintf(text, sizeof(text), "%s  %s", MaintenanceTitle(), clock);
    Font font = GuiFont(FNT_MENU);
    HudLayout hud = HudGetLayout();
    float text_width = MeasureTextEx(font, text, MAINTENANCE_STRIP_FONT_SIZE, 1.0f).x;
    Rectangle plate = MaintenanceStripPlate(&hud, text_width);
    DrawRectangleRounded(
        plate, MAINTENANCE_STRIP_ROUNDNESS, MAINTENANCE_STRIP_SEGMENTS, Fade(BLACK, MAINTENANCE_STRIP_PLATE_OPACITY)
    );
    UiDrawTextCentered(font, text, plate, MAINTENANCE_STRIP_FONT_SIZE, MaintenanceColor(maintenance.phase));
}

static void MaintenanceNoticeDrawPopup(MaintenanceNotice *notice, ClientMaintenance maintenance) {
    bool holding = maintenance.phase == CLIENT_MAINTENANCE_HOLDING_BATTLES;
    const char *message = holding ? LocGetOr("HODA_MAINTENANCE_HOLD_MESSAGE", MAINTENANCE_HOLD_MESSAGE_FALLBACK)
                                  : LocGetOr("HODA_MAINTENANCE_MESSAGE", MAINTENANCE_MESSAGE_FALLBACK);
    char clock[MAINTENANCE_CLOCK_CAP];
    MaintenanceClockText(maintenance.seconds_left, clock, sizeof(clock));
    Rectangle frame = MessagePopupDrawFrame();
    Rectangle content = MessagePopupContent(frame);
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        MaintenanceTitle(),
        UiRect(content, 0.5f, 0.0f, 1.0f, 0.2f),
        MESSAGE_POPUP_TITLE_SIZE,
        MESSAGE_POPUP_TEXT_COLOR
    );
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        clock,
        UiRect(content, 0.5f, 0.28f, 1.0f, 0.2f),
        MAINTENANCE_POPUP_CLOCK_SIZE,
        MaintenanceColor(maintenance.phase)
    );
    MessagePopupDrawText(
        GuiFont(FNT_MENU),
        message,
        UiRect(content, 0.5f, 0.8f, 1.0f, 0.42f),
        MESSAGE_POPUP_MESSAGE_SIZE,
        MESSAGE_POPUP_TEXT_COLOR
    );
    if (ButtonDraw(&notice->ok, MessagePopupOkButton(frame))) notice->open = false;
}

void MaintenanceNoticeDraw(MaintenanceNotice *notice) {
    ClientMaintenance maintenance = MaintenanceNoticeWatch(notice);
    if (maintenance.phase == CLIENT_MAINTENANCE_NONE) return;
    if (notice->open) {
        UiSetInputBlocked(false);
        UiSetInputWindow(false, (Rectangle){0});
    }
    UiFrameBegin();
    MaintenanceNoticeDrawStrip(maintenance);
    if (notice->open) MaintenanceNoticeDrawPopup(notice, maintenance);
    UiFrameEnd();
}
