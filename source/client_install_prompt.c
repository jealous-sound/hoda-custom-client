#define CLIENT_INSTALL_PROMPT_TEXT_CAP 512
#define CLIENT_INSTALL_PROMPT_NUMBER_CAP 16
#define CLIENT_INSTALL_PERCENT_MAX 100
#define CLIENT_INSTALL_RESTART_NOTICE_SECONDS 1.5
#define CLIENT_INSTALL_BUTTON_FALLBACK "Update"
#define CLIENT_INSTALL_CANCEL_FALLBACK "Cancel"
#define CLIENT_INSTALL_AVAILABLE_FALLBACK                                                                              \
    "Heroes of Dragon Age {0} is out. Update now to get the latest heroes and fixes."
#define CLIENT_INSTALL_REQUIRED_FALLBACK                                                                               \
    "This version can no longer play online. Update to Heroes of Dragon Age {0} to keep playing online."
#define CLIENT_INSTALL_REQUIRED_FOR_ONLINE_FALLBACK                                                                    \
    "Online play needs a newer version. Update to Heroes of Dragon Age {0} to play online, or start a local game."
#define CLIENT_INSTALL_WORKING_FALLBACK "Downloading Heroes of Dragon Age {0}... {1}%"
#define CLIENT_INSTALL_RESTARTING_FALLBACK "Heroes of Dragon Age {0} is installed. Restarting..."
#define CLIENT_INSTALL_FAILED_FALLBACK                                                                                 \
    "The update could not be installed. Download Heroes of Dragon Age {0} from the website instead."

typedef enum {
    CLIENT_INSTALL_OCCASION_AVAILABLE,
    CLIENT_INSTALL_OCCASION_REQUIRED,
    CLIENT_INSTALL_OCCASION_REQUIRED_FOR_ONLINE,
    CLIENT_INSTALL_OCCASION_COUNT,
} ClientInstallOccasion;

typedef enum {
    CLIENT_INSTALL_PROMPT_OPEN,
    CLIENT_INSTALL_PROMPT_DECLINED,
    CLIENT_INSTALL_PROMPT_WEBSITE,
    CLIENT_INSTALL_PROMPT_ABANDONED,
    CLIENT_INSTALL_PROMPT_RESTART,
} ClientInstallPromptResult;

typedef struct {
    const char *website_key;
    const char *install_key;
    const char *install_fallback;
} ClientInstallWording;

typedef struct {
    Texture2D frame;
    Font title_font;
    Font body_font;
    const char *title;
    ClientInstallOccasion occasion;
    const char *fixed_message;
    const char *decline_label;
    Button *decline;
    Button *accept;
} ClientInstallPrompt;

typedef struct {
    bool showing;
    double since;
} ClientInstallRestartNotice;

static ClientInstallRestartNotice client_install_restart_notice;

static const ClientInstallWording client_install_wording[CLIENT_INSTALL_OCCASION_COUNT] = {
    [CLIENT_INSTALL_OCCASION_AVAILABLE] =
        {"HODA_UPDATE_AVAILABLE_MESSAGE", "HODA_UPDATE_AVAILABLE_INSTALL_MESSAGE", CLIENT_INSTALL_AVAILABLE_FALLBACK},
    [CLIENT_INSTALL_OCCASION_REQUIRED] =
        {"HODA_UPDATE_REQUIRED_MESSAGE", "HODA_UPDATE_REQUIRED_INSTALL_MESSAGE", CLIENT_INSTALL_REQUIRED_FALLBACK},
    [CLIENT_INSTALL_OCCASION_REQUIRED_FOR_ONLINE] = {
        "HODA_UPDATE_REQUIRED_ONLINE_MESSAGE",
        "HODA_UPDATE_REQUIRED_ONLINE_INSTALL_MESSAGE",
        CLIENT_INSTALL_REQUIRED_FOR_ONLINE_FALLBACK
    },
};

static bool ClientInstallPromptAnswered(ClientInstallPromptResult result) {
    return result == CLIENT_INSTALL_PROMPT_DECLINED || result == CLIENT_INSTALL_PROMPT_WEBSITE ||
           result == CLIENT_INSTALL_PROMPT_ABANDONED;
}

static bool ClientInstallRestartNoticeRead(void) {
    ClientInstallRestartNotice *notice = &client_install_restart_notice;
    if (!notice->showing) *notice = (ClientInstallRestartNotice){.showing = true, .since = GetTime()};
    return GetTime() - notice->since >= CLIENT_INSTALL_RESTART_NOTICE_SECONDS;
}

static int ClientInstallPercent(const ClientInstallProgress *progress) {
    if (progress->total_bytes <= 0) return 0;
    long long percent = progress->received_bytes * CLIENT_INSTALL_PERCENT_MAX / progress->total_bytes;
    return (int)(percent < 0 ? 0 : percent > CLIENT_INSTALL_PERCENT_MAX ? CLIENT_INSTALL_PERCENT_MAX : percent);
}

static void ClientInstallProgressText(const ClientInstallProgress *progress, char *out, size_t capacity) {
    char percent[CLIENT_INSTALL_PROMPT_NUMBER_CAP];
    snprintf(percent, sizeof(percent), "%d", ClientInstallPercent(progress));
    const char *arguments[] = {progress->version, percent};
    const char *format = progress->phase == CLIENT_INSTALL_READY
                             ? LocGetOr("HODA_UPDATE_RESTARTING_MESSAGE", CLIENT_INSTALL_RESTARTING_FALLBACK)
                             : LocGetOr("HODA_UPDATE_INSTALLING_MESSAGE", CLIENT_INSTALL_WORKING_FALLBACK);
    LocFormatTextArgs(out, capacity, format, arguments, 2);
}

static void ClientInstallOfferText(const ClientInstallPrompt *prompt, bool installable, char *out, size_t capacity) {
    if (prompt->fixed_message) {
        StrCopy(out, (int)capacity, prompt->fixed_message);
        return;
    }
    const ClientInstallWording *wording = &client_install_wording[prompt->occasion];
    const char *arguments[] = {ClientUpdateCurrentRelease().latest};
    const char *format =
        installable ? LocGetOr(wording->install_key, wording->install_fallback) : LocGet(wording->website_key);
    LocFormatTextArgs(out, capacity, format, arguments, 1);
}

static ClientInstallPromptResult ClientInstallPromptDrawOffer(const ClientInstallPrompt *prompt) {
    bool installable = ClientInstallOffered();
    char message[CLIENT_INSTALL_PROMPT_TEXT_CAP];
    ClientInstallOfferText(prompt, installable, message, sizeof(message));
    prompt->decline->label = prompt->decline_label;
    prompt->accept->label =
        installable ? LocGetOr("HODA_UPDATE_INSTALL", CLIENT_INSTALL_BUTTON_FALLBACK) : LocGet("HODA_UPDATE_DOWNLOAD");
    MessagePopupChoice choice = MessagePopupDrawChoice(
        prompt->frame, prompt->title_font, prompt->body_font, prompt->title, message, prompt->decline, prompt->accept
    );
    if (choice == MESSAGE_POPUP_CHOICE_DECLINE) return CLIENT_INSTALL_PROMPT_DECLINED;
    if (choice != MESSAGE_POPUP_CHOICE_ACCEPT) return CLIENT_INSTALL_PROMPT_OPEN;
    if (installable) {
        ClientInstallStart();
        return CLIENT_INSTALL_PROMPT_OPEN;
    }
    ClientUpdateOpenDownload();
    return CLIENT_INSTALL_PROMPT_WEBSITE;
}

static ClientInstallPromptResult
ClientInstallPromptDrawWorking(const ClientInstallPrompt *prompt, const ClientInstallProgress *progress) {
    char message[CLIENT_INSTALL_PROMPT_TEXT_CAP];
    ClientInstallProgressText(progress, message, sizeof(message));
    Rectangle popup =
        MessagePopupDrawMessage(prompt->frame, prompt->title_font, prompt->body_font, prompt->title, message);
    if (progress->phase == CLIENT_INSTALL_READY)
        return ClientInstallRestartNoticeRead() ? CLIENT_INSTALL_PROMPT_RESTART : CLIENT_INSTALL_PROMPT_OPEN;
    prompt->decline->label = LocGetOr("FORTIFY_CANCEL_BUTTON", CLIENT_INSTALL_CANCEL_FALLBACK);
    if (ButtonDraw(prompt->decline, MessagePopupOkButton(popup))) ClientInstallCancel();
    return CLIENT_INSTALL_PROMPT_OPEN;
}

static ClientInstallPromptResult
ClientInstallPromptDrawFailed(const ClientInstallPrompt *prompt, const ClientInstallProgress *progress) {
    char message[CLIENT_INSTALL_PROMPT_TEXT_CAP];
    const char *arguments[] = {progress->version};
    LocFormatTextArgs(
        message, sizeof(message), LocGetOr("HODA_UPDATE_FAILED_MESSAGE", CLIENT_INSTALL_FAILED_FALLBACK), arguments, 1
    );
    prompt->decline->label = prompt->decline_label;
    prompt->accept->label = LocGet("HODA_UPDATE_DOWNLOAD");
    MessagePopupChoice choice = MessagePopupDrawChoice(
        prompt->frame, prompt->title_font, prompt->body_font, prompt->title, message, prompt->decline, prompt->accept
    );
    if (choice == MESSAGE_POPUP_CHOICE_NONE) return CLIENT_INSTALL_PROMPT_OPEN;
    ClientInstallAcknowledgeFailure();
    if (choice == MESSAGE_POPUP_CHOICE_DECLINE) return CLIENT_INSTALL_PROMPT_ABANDONED;
    ClientUpdateOpenDownload();
    return CLIENT_INSTALL_PROMPT_WEBSITE;
}

static ClientInstallPromptResult ClientInstallPromptDraw(const ClientInstallPrompt *prompt) {
    ClientInstallProgress progress = ClientInstallPoll();
    switch (progress.phase) {
        case CLIENT_INSTALL_CHECKING:
        case CLIENT_INSTALL_DOWNLOADING:
        case CLIENT_INSTALL_UNPACKING:
        case CLIENT_INSTALL_READY:
            return ClientInstallPromptDrawWorking(prompt, &progress);
        case CLIENT_INSTALL_FAILED:
            return ClientInstallPromptDrawFailed(prompt, &progress);
        case CLIENT_INSTALL_IDLE:
            break;
    }
    return ClientInstallPromptDrawOffer(prompt);
}
