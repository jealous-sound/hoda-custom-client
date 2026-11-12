#define CONTENT_TRACK_TEXTURE APK_RESOURCE("Bar_ProgBarBg_v4")
#define CONTENT_FILL_TEXTURE APK_RESOURCE("Meter_LoadingFill")
#define CONTENT_ALERT_FRAME APK_RESOURCE("Bg_Overlay_Short")
#define CONTENT_ALERT_BUTTON APK_RESOURCE("Btn_CTA_gold")
#define CONTENT_ALERT_BUTTON_ACTIVE APK_RESOURCE("Btn_CTA_gold_active")
#define CONTENT_ALERT_DECLINE APK_RESOURCE("Btn_Default_grey")
#define CONTENT_ALERT_DECLINE_ACTIVE APK_RESOURCE("Btn_Default_grey_active")
#define CONTENT_ALERT_BUTTON_LABEL_SCALE 0.34f
#define CONTENT_PROGRESS_CONTAINER_X 0.5f
#define CONTENT_PROGRESS_CONTAINER_Y 0.25f
#define CONTENT_PROGRESS_CONTAINER_WIDTH 0.7f
#define CONTENT_PROGRESS_CONTAINER_HEIGHT 0.88f
#define CONTENT_BAR_CONTAINER_X 0.5f
#define CONTENT_BAR_CONTAINER_Y 1.0f
#define CONTENT_BAR_CONTAINER_WIDTH 0.76f
#define CONTENT_BAR_CONTAINER_HEIGHT 0.05f
#define CONTENT_BAR_X 0.5f
#define CONTENT_BAR_Y 0.5f
#define CONTENT_BAR_WIDTH 0.965f
#define CONTENT_BAR_HEIGHT 0.35f
#define CONTENT_BAR_MINIMUM_FILL_PIXELS 10.0f
#define CONTENT_TRACK_BORDER_PIXELS 7.0f
#define CONTENT_PERCENT_LABEL_X 0.5f
#define CONTENT_PERCENT_LABEL_Y 0.99f
#define CONTENT_PERCENT_LABEL_WIDTH 0.5f
#define CONTENT_PERCENT_LABEL_HEIGHT 0.1f
#define CONTENT_PERCENT_FONT_SIZE 26.0f
#define CONTENT_DETAIL_LABEL_X 0.5f
#define CONTENT_DETAIL_LABEL_Y 0.85f
#define CONTENT_DETAIL_LABEL_WIDTH 0.9f
#define CONTENT_DETAIL_LABEL_HEIGHT 0.1f
#define CONTENT_DETAIL_FONT_SIZE 24.0f
#define CONTENT_MESSAGE_MINIMUM_FONT_SIZE 10.0f
#define CONTENT_MESSAGE_FONT_STEP 1.0f
#define CONTENT_PERCENT_MAX 100
#define CONTENT_BYTES_PER_MEGABYTE 1048576.0
#define CONTENT_DEFAULT_DECIMAL_SEPARATOR '.'
#define CONTENT_LABEL_CAP 64
#define CONTENT_SCREEN_TEXT_CAP 512
#define CONTENT_BLANK_BACKGROUND BLACK
#define CONTENT_BLANK_RETRY_SECONDS 5.0

typedef enum {
    CONTENT_SCREEN_WAIT,
    CONTENT_SCREEN_RETRY,
    CONTENT_SCREEN_QUIT,
} ContentScreenChoice;

typedef struct {
    const char *title_key;
    const char *message_key;
    const char *button_key;
    bool quits;
    bool offers_download;
} ContentAlert;

typedef struct {
    bool attempted;
    bool loaded;
    Texture2D splash;
    Texture2D plate;
    Texture2D ring;
    Texture2D track;
    Texture2D fill;
    Texture2D alert_frame;
    Font menu_font;
    Font title_font;
    Button alert_button;
    Button alert_decline;
} ContentArt;

typedef struct {
    char directory[CONTENT_PATH_CAP];
    bool entered;
    bool failure_seen;
    double failure_time;
    ContentArt art;
} ContentScreen;

static int ContentPercent(const ContentProgress *progress) {
    if (progress->total_bytes <= 0) return 0;
    long long percent = progress->received_bytes * CONTENT_PERCENT_MAX / progress->total_bytes;
    return (int)(percent < 0 ? 0 : percent > CONTENT_PERCENT_MAX ? CONTENT_PERCENT_MAX : percent);
}

static bool ContentBarVisible(int percent) {
    return percent > 0 && percent < CONTENT_PERCENT_MAX;
}

static void ContentFormatMegabytes(long long bytes, const char *separator, char *out, size_t capacity) {
    char plain[CONTENT_LABEL_CAP];
    snprintf(plain, sizeof(plain), "%.1f", (double)bytes / CONTENT_BYTES_PER_MEGABYTE);
    size_t written = 0;
    for (const char *c = plain; *c && written + 1 < capacity; c++) {
        if (*c != CONTENT_DEFAULT_DECIMAL_SEPARATOR) {
            out[written++] = *c;
            continue;
        }
        for (const char *s = separator; *s && written + 1 < capacity; s++) out[written++] = *s;
    }
    out[written] = '\0';
}

static const char *ContentLoadingMessageKey(const ContentProgress *progress) {
    if (progress->packs_total == 0) return "LOADING_MESSAGE_CHECKING_UPDATES";
    return progress->first_install ? "LOADING_MESSAGE_WELCOME" : "LOADING_MESSAGE_DOWNLOADING_UPDATES";
}

static ContentAlert ContentAlertFor(ContentError error) {
    switch (error) {
        case CONTENT_ERROR_OFFLINE:
        case CONTENT_ERROR_DOWNLOAD_UNAVAILABLE:
            return (ContentAlert){"CONNECTION_ERROR_TITLE", "CONNECTION_ERROR_MESSAGE", "CONNECTION_ERROR_TRY_AGAIN"};
        case CONTENT_ERROR_DISK_FULL:
            return (ContentAlert){
                "ERROR_INSUFFICIENT_DEVICE_STORAGE_TITLE",
                "ERROR_INSUFFICIENT_DEVICE_STORAGE",
                "ASSET_DOWNLOAD_ERROR_BUTTON_RETRY"
            };
        case CONTENT_ERROR_CLIENT_TOO_OLD:
            return (ContentAlert){
                "HODA_CONTENT_CLIENT_TOO_OLD_TITLE",
                "HODA_CONTENT_CLIENT_TOO_OLD_MESSAGE",
                "HODA_UPDATE_QUIT",
                true,
                true
            };
        case CONTENT_ERROR_NONE:
        case CONTENT_ERROR_NOT_CONFIGURED:
        case CONTENT_ERROR_SERVER:
        case CONTENT_ERROR_CORRUPT:
        case CONTENT_ERROR_DISK:
        case CONTENT_ERROR_CANCELLED:
            break;
    }
    return (ContentAlert){
        "ASSET_DOWNLOAD_ERROR_TITLE_TU5", "ASSET_DOWNLOAD_ERROR_MESSAGE_TU5", "ASSET_DOWNLOAD_ERROR_BUTTON_RETRY"
    };
}

static float ContentCanvasScale(void) {
    return UiHeight() / LOADING_PREFAB_CANVAS_HEIGHT;
}

static void ContentArtUnload(ContentArt *art) {
    Texture2D *textures[] = {
        &art->splash,
        &art->plate,
        &art->ring,
        &art->track,
        &art->fill,
        &art->alert_frame,
        &art->alert_button.idle,
        &art->alert_button.active,
        &art->alert_decline.idle,
        &art->alert_decline.active,
    };
    for (size_t i = 0; i < sizeof(textures) / sizeof(textures[0]); i++)
        if (textures[i]->id) UnloadTexture(*textures[i]);
    if (art->menu_font.texture.id) UnloadFont(art->menu_font);
    if (art->title_font.texture.id) UnloadFont(art->title_font);
    if (art->loaded) LocUnload();
    *art = (ContentArt){.attempted = art->attempted};
}

static void ContentArtLoad(ContentArt *art) {
    art->attempted = true;
    LocLoadPreferred();
    art->splash = LoadingScreenTexture(LOADING_SCREEN_SPLASH);
    art->plate = LoadingScreenTexture(LOADING_SCREEN_PLATE);
    art->ring = LoadingScreenTexture(LOADING_INDICATOR_RING);
    art->track = LoadingScreenTexture(CONTENT_TRACK_TEXTURE);
    art->fill = LoadingScreenTexture(CONTENT_FILL_TEXTURE);
    art->alert_frame = LoadingScreenTexture(CONTENT_ALERT_FRAME);
    art->menu_font = LoadGameFont(FNT_MENU);
    art->title_font = LoadGameFont(FNT_COOL);
    art->alert_button = ButtonFromTextures(
        LoadingScreenTexture(CONTENT_ALERT_BUTTON),
        LoadingScreenTexture(CONTENT_ALERT_BUTTON_ACTIVE),
        NULL,
        art->title_font,
        (Vector2){0.5f, 0.5f},
        CONTENT_ALERT_BUTTON_LABEL_SCALE
    );
    art->alert_decline = ButtonFromTextures(
        LoadingScreenTexture(CONTENT_ALERT_DECLINE),
        LoadingScreenTexture(CONTENT_ALERT_DECLINE_ACTIVE),
        NULL,
        art->title_font,
        (Vector2){0.5f, 0.5f},
        CONTENT_ALERT_BUTTON_LABEL_SCALE
    );
    ResourceShutdown();
    bool complete = art->splash.id && art->plate.id && art->ring.id && art->track.id && art->fill.id &&
                    art->alert_frame.id && art->alert_button.idle.id && art->alert_button.active.id &&
                    art->alert_decline.idle.id && art->alert_decline.active.id && art->menu_font.texture.id &&
                    art->title_font.texture.id;
    if (!complete) {
        LocUnload();
        ContentArtUnload(art);
        return;
    }
    art->loaded = true;
}

static void ContentArtLoadWhenInstalled(ContentArt *art) {
    if (!art->attempted && FileExists(ORIGINAL_APK_PATH) && LocPreferredInstalled()) ContentArtLoad(art);
}

static float ContentFitMessageSize(Font font, const char *text, Rectangle box, float size) {
    char lines[MESSAGE_POPUP_LINES][MESSAGE_POPUP_TEXT_CAP];
    while (size > CONTENT_MESSAGE_MINIMUM_FONT_SIZE) {
        int count = MessagePopupWrap(font, text, box.width, size, lines, MESSAGE_POPUP_LINES);
        if (count < MESSAGE_POPUP_LINES && count * size * MESSAGE_POPUP_LINE_SPACING <= box.height) break;
        size -= CONTENT_MESSAGE_FONT_STEP;
    }
    return size;
}

static void ContentDrawLoadingMessage(const ContentArt *art, const ContentProgress *progress, Rectangle root) {
    Rectangle plate = LoadingFitAspect(
        root,
        LOADING_SCREEN_MESSAGE_X,
        LOADING_SCREEN_MESSAGE_Y,
        LOADING_SCREEN_MESSAGE_WIDTH,
        LOADING_SCREEN_MESSAGE_HEIGHT,
        LOADING_SCREEN_MESSAGE_ASPECT
    );
    UiDrawTexture(art->plate, plate, WHITE);
    Rectangle label = UiRect(plate, 0.5f, 0.5f, LOADING_SCREEN_LABEL_SIZE, LOADING_SCREEN_LABEL_SIZE);
    const char *message = LocGet(ContentLoadingMessageKey(progress));
    float size =
        ContentFitMessageSize(art->menu_font, message, label, LOADING_SCREEN_LABEL_FONT_SIZE * ContentCanvasScale());
    MessagePopupDrawText(art->menu_font, message, label, size, LOADING_LABEL_COLOR);
}

static void ContentDrawHorizontalSlices(Texture2D texture, Rectangle box, float border_pixels) {
    float border = fminf(border_pixels * ContentCanvasScale(), box.width * 0.5f);
    float edge = fminf(border_pixels, texture.width * 0.5f);
    float height = (float)texture.height;
    Rectangle sources[] = {
        {0.0f, 0.0f, edge, height},
        {edge, 0.0f, texture.width - edge * 2.0f, height},
        {texture.width - edge, 0.0f, edge, height},
    };
    Rectangle targets[] = {
        {box.x, box.y, border, box.height},
        {box.x + border, box.y, box.width - border * 2.0f, box.height},
        {box.x + box.width - border, box.y, border, box.height},
    };
    for (int i = 0; i < 3; i++) DrawTexturePro(texture, sources[i], targets[i], (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
}

static void ContentDrawProgressBar(const ContentArt *art, Rectangle root, int percent) {
    Rectangle container = UiRect(
        root,
        CONTENT_PROGRESS_CONTAINER_X,
        CONTENT_PROGRESS_CONTAINER_Y,
        CONTENT_PROGRESS_CONTAINER_WIDTH,
        CONTENT_PROGRESS_CONTAINER_HEIGHT
    );
    Rectangle track = UiRect(
        container,
        CONTENT_BAR_CONTAINER_X,
        CONTENT_BAR_CONTAINER_Y,
        CONTENT_BAR_CONTAINER_WIDTH,
        CONTENT_BAR_CONTAINER_HEIGHT
    );
    ContentDrawHorizontalSlices(art->track, track, CONTENT_TRACK_BORDER_PIXELS);
    Rectangle bar = UiRect(track, CONTENT_BAR_X, CONTENT_BAR_Y, CONTENT_BAR_WIDTH, CONTENT_BAR_HEIGHT);
    float filled = bar.width * (float)percent / (float)CONTENT_PERCENT_MAX;
    bar.width = fmaxf(filled, CONTENT_BAR_MINIMUM_FILL_PIXELS * ContentCanvasScale());
    UiDrawTexture(art->fill, bar, WHITE);
}

static void
ContentDrawProgressLabels(const ContentArt *art, const ContentProgress *progress, Rectangle root, int percent) {
    char percent_text[CONTENT_LABEL_CAP];
    snprintf(percent_text, sizeof(percent_text), "%d%%", percent);
    Rectangle percent_box = UiRect(
        root,
        CONTENT_PERCENT_LABEL_X,
        CONTENT_PERCENT_LABEL_Y,
        CONTENT_PERCENT_LABEL_WIDTH,
        CONTENT_PERCENT_LABEL_HEIGHT
    );
    UiDrawTextCentered(
        art->menu_font, percent_text, percent_box, CONTENT_PERCENT_FONT_SIZE * ContentCanvasScale(), WHITE
    );
    const char *separator = LocGetOr("NUMBER_DECIMAL_SEPARATOR", ".");
    char received[CONTENT_LABEL_CAP], total[CONTENT_LABEL_CAP], detail[CONTENT_SCREEN_TEXT_CAP];
    ContentFormatMegabytes(progress->received_bytes, separator, received, sizeof(received));
    ContentFormatMegabytes(progress->total_bytes, separator, total, sizeof(total));
    LocFormat2(detail, sizeof(detail), "DOWNLOAD_PROGRESS_MB_MESSAGE", received, total);
    Rectangle detail_box = UiRect(
        root, CONTENT_DETAIL_LABEL_X, CONTENT_DETAIL_LABEL_Y, CONTENT_DETAIL_LABEL_WIDTH, CONTENT_DETAIL_LABEL_HEIGHT
    );
    UiDrawTextCentered(art->menu_font, detail, detail_box, CONTENT_DETAIL_FONT_SIZE * ContentCanvasScale(), WHITE);
}

static ContentScreenChoice ContentDrawDownloadAlert(ContentArt *art, const ContentAlert *alert) {
    ClientInstallPrompt prompt = {
        .frame = art->alert_frame,
        .title_font = art->title_font,
        .body_font = art->menu_font,
        .title = LocGet(alert->title_key),
        .fixed_message = LocGet(alert->message_key),
        .decline_label = LocGet(alert->button_key),
        .decline = &art->alert_decline,
        .accept = &art->alert_button,
    };
    ClientInstallPromptResult result = ClientInstallPromptDraw(&prompt);
    bool escaped = !ClientInstallBusy() && IsKeyPressed(KEY_ESCAPE);
    bool staying = result == CLIENT_INSTALL_PROMPT_OPEN || result == CLIENT_INSTALL_PROMPT_WEBSITE;
    return staying && !escaped ? CONTENT_SCREEN_WAIT : CONTENT_SCREEN_QUIT;
}

static ContentScreenChoice ContentDrawArt(ContentArt *art, const ContentProgress *progress) {
    Rectangle root = UiRoot();
    UiDrawTexture(art->splash, root, WHITE);
    LoadingIndicatorDraw(
        art->ring,
        LoadingIndicatorBox(root, LOADING_SCREEN_SPINNER_X, LOADING_SCREEN_SPINNER_Y, LOADING_SCREEN_SPINNER_HEIGHT)
    );
    ContentDrawLoadingMessage(art, progress, root);
    if (progress->packs_total > 0) {
        int percent = ContentPercent(progress);
        if (ContentBarVisible(percent)) ContentDrawProgressBar(art, root, percent);
        ContentDrawProgressLabels(art, progress, root, percent);
    }
    if (progress->phase != CONTENT_PHASE_FAILED) return CONTENT_SCREEN_WAIT;
    ContentAlert alert = ContentAlertFor(progress->error);
    if (alert.offers_download) return ContentDrawDownloadAlert(art, &alert);
    art->alert_button.label = LocGet(alert.button_key);
    bool pressed = MessagePopupDrawAlert(
        art->alert_frame,
        art->title_font,
        art->menu_font,
        LocGet(alert.title_key),
        LocGet(alert.message_key),
        &art->alert_button
    );
    if (pressed) return alert.quits ? CONTENT_SCREEN_QUIT : CONTENT_SCREEN_RETRY;
    return IsKeyPressed(KEY_ESCAPE) ? CONTENT_SCREEN_QUIT : CONTENT_SCREEN_WAIT;
}

static ContentScreenChoice ContentDrawBlank(ContentScreen *screen, const ContentProgress *progress) {
    if (IsKeyPressed(KEY_ESCAPE)) return CONTENT_SCREEN_QUIT;
    if (progress->phase != CONTENT_PHASE_FAILED) return CONTENT_SCREEN_WAIT;
    if (!screen->failure_seen) {
        screen->failure_seen = true;
        screen->failure_time = GetTime();
    }
    return GetTime() - screen->failure_time >= CONTENT_BLANK_RETRY_SECONDS ? CONTENT_SCREEN_RETRY : CONTENT_SCREEN_WAIT;
}

static ContentScreenChoice ContentScreenDraw(ContentScreen *screen, const ContentProgress *progress) {
    return screen->art.loaded ? ContentDrawArt(&screen->art, progress) : ContentDrawBlank(screen, progress);
}

static bool ContentScreenStart(ContentScreen *screen, ContentUpdate *update) {
    screen->failure_seen = false;
    if (!screen->entered) {
        screen->entered = ContentEnterInstallDirectory(screen->directory, sizeof(screen->directory));
        if (screen->entered) PlatformKeepLog(CONTENT_LOG_FILE);
    }
    if (!screen->entered) return false;
    ContentArtLoadWhenInstalled(&screen->art);
    ContentUpdateStart(update, ContentBaseUrl());
    return true;
}

static bool ContentScreenRun(void) {
    if (ContentUsesLocalFiles()) return true;
    ContentScreen screen = {0};
    ContentUpdate update = {0};
    bool running = ContentScreenStart(&screen, &update);
    bool ready = false;
    while (!WindowShouldClose()) {
        ContentProgress progress = running
                                       ? ContentUpdatePoll(&update)
                                       : (ContentProgress){.phase = CONTENT_PHASE_FAILED, .error = CONTENT_ERROR_DISK};
        if (progress.phase == CONTENT_PHASE_READY) {
            ready = true;
            break;
        }
        if (screen.entered) ContentArtLoadWhenInstalled(&screen.art);
        BeginDrawing();
        ClearBackground(CONTENT_BLANK_BACKGROUND);
        UiFrameBegin();
        ContentScreenChoice choice = ContentScreenDraw(&screen, &progress);
        UiFrameEnd();
        EndDrawing();
        if (choice == CONTENT_SCREEN_QUIT) break;
        if (choice != CONTENT_SCREEN_RETRY) continue;
        if (running) ContentUpdateStop(&update, false);
        running = ContentScreenStart(&screen, &update);
    }
    if (running) ContentUpdateStop(&update, !ready);
    ContentArtUnload(&screen.art);
    return ready;
}
