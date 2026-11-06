#include "platform.h"

#include <stdlib.h>
#include <string.h>

#define PLATFORM_DOWNLOAD_CONNECT_TIMEOUT_SECONDS 15L
#define PLATFORM_DOWNLOAD_STALL_BYTES_PER_SECOND 1L
#define PLATFORM_DOWNLOAD_STALL_SECONDS 20L
#define PLATFORM_DOWNLOAD_MAX_REDIRECTS 5L

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <direct.h>
#include <errno.h>
#include <io.h>

#define PLATFORM_WINSOCK_VERSION MAKEWORD(2, 2)
#define PLATFORM_MILLISECONDS_PER_SECOND 1000
#define PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS 30000
#define PLATFORM_DOWNLOAD_USER_AGENT L"HeroesOfDragonAge"
#define PLATFORM_DOWNLOAD_BUFFER_BYTES 65536
#define PLATFORM_WIDE_URL_CAP 2048
#define PLATFORM_HOST_CAP 256
#define PLATFORM_RANGE_HEADER_CAP 64
#define PLATFORM_HTTP_OK 200
#define PLATFORM_HTTP_PARTIAL_CONTENT 206
#define PLATFORM_HTTP_RANGE_NOT_SATISFIABLE 416
#define PLATFORM_APP_DATA_ENV "APPDATA"

static bool PlatformNetworkReady(void) {
    static bool started = false;
    if (started) return true;
    WSADATA data;
    started = WSAStartup(PLATFORM_WINSOCK_VERSION, &data) == 0;
    return started;
}

intptr_t PlatformSocketConnect(const char *host, const char *port, int timeout_seconds) {
    if (!PlatformNetworkReady()) return PLATFORM_SOCKET_INVALID;
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    struct addrinfo *addresses = NULL;
    if (getaddrinfo(host, port, &hints, &addresses) != 0) return PLATFORM_SOCKET_INVALID;
    SOCKET connected = INVALID_SOCKET;
    DWORD timeout = (DWORD)timeout_seconds * PLATFORM_MILLISECONDS_PER_SECOND;
    for (struct addrinfo *address = addresses; address; address = address->ai_next) {
        connected = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (connected == INVALID_SOCKET) continue;
        setsockopt(connected, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
        setsockopt(connected, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
        if (connect(connected, address->ai_addr, (int)address->ai_addrlen) == 0) break;
        closesocket(connected);
        connected = INVALID_SOCKET;
    }
    freeaddrinfo(addresses);
    return connected == INVALID_SOCKET ? PLATFORM_SOCKET_INVALID : (intptr_t)connected;
}

long PlatformSocketSend(intptr_t socket_handle, const char *data, size_t length) {
    return send((SOCKET)socket_handle, data, (int)length, 0);
}

long PlatformSocketReceive(intptr_t socket_handle, char *buffer, size_t capacity) {
    return recv((SOCKET)socket_handle, buffer, (int)capacity, 0);
}

void PlatformSocketClose(intptr_t socket_handle) {
    closesocket((SOCKET)socket_handle);
}

bool PlatformReadAt(FILE *file, long long offset, void *out, size_t size) {
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file));
    if (handle == INVALID_HANDLE_VALUE) return false;
    OVERLAPPED position = {.Offset = (DWORD)offset, .OffsetHigh = (DWORD)((unsigned long long)offset >> 32)};
    DWORD read = 0;
    return ReadFile(handle, out, (DWORD)size, &read, &position) && read == (DWORD)size;
}

bool PlatformReplaceFile(const char *source, const char *target) {
    return MoveFileExA(source, target, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

int PlatformProcessorCount(void) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
}

time_t PlatformTimeFromUtc(struct tm *utc) {
    return _mkgmtime(utc);
}

static long long PlatformFileBytes(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    long long size = _fseeki64(file, 0, SEEK_END) == 0 ? _ftelli64(file) : 0;
    fclose(file);
    return size > 0 ? size : 0;
}

static bool PlatformWiden(const char *text, wchar_t *out, int capacity) {
    return MultiByteToWideChar(CP_UTF8, 0, text, -1, out, capacity) > 0;
}

static PlatformDownloadResult PlatformReceiveBody(
    HINTERNET request, FILE *file, long long written, PlatformDownloadProgress progress, void *context
) {
    char *buffer = malloc(PLATFORM_DOWNLOAD_BUFFER_BYTES);
    if (!buffer) return PLATFORM_DOWNLOAD_FILE_ERROR;
    PlatformDownloadResult result = PLATFORM_DOWNLOAD_OK;
    for (;;) {
        DWORD available = 0, read = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            result = PLATFORM_DOWNLOAD_NETWORK_ERROR;
            break;
        }
        if (available == 0) break;
        DWORD wanted = available < PLATFORM_DOWNLOAD_BUFFER_BYTES ? available : PLATFORM_DOWNLOAD_BUFFER_BYTES;
        if (!WinHttpReadData(request, buffer, wanted, &read)) {
            result = PLATFORM_DOWNLOAD_NETWORK_ERROR;
            break;
        }
        if (fwrite(buffer, 1, read, file) != read) {
            result = PLATFORM_DOWNLOAD_FILE_ERROR;
            break;
        }
        written += read;
        if (progress && !progress(context, written)) {
            result = PLATFORM_DOWNLOAD_CANCELLED;
            break;
        }
    }
    free(buffer);
    return result;
}

static PlatformDownloadResult PlatformTransfer(
    const wchar_t *url,
    const char *path,
    long long offset,
    PlatformDownloadProgress progress,
    void *context,
    bool *resume_rejected
) {
    wchar_t host[PLATFORM_HOST_CAP], request_path[PLATFORM_WIDE_URL_CAP], extra[PLATFORM_WIDE_URL_CAP];
    URL_COMPONENTS parts = {
        .dwStructSize = sizeof(parts),
        .lpszHostName = host,
        .dwHostNameLength = PLATFORM_HOST_CAP,
        .lpszUrlPath = request_path,
        .dwUrlPathLength = PLATFORM_WIDE_URL_CAP,
        .lpszExtraInfo = extra,
        .dwExtraInfoLength = PLATFORM_WIDE_URL_CAP,
    };
    if (!WinHttpCrackUrl(url, 0, 0, &parts)) return PLATFORM_DOWNLOAD_NETWORK_ERROR;
    if (wcslen(request_path) + wcslen(extra) >= PLATFORM_WIDE_URL_CAP) return PLATFORM_DOWNLOAD_NETWORK_ERROR;
    wcscat(request_path, extra);

    wchar_t range[PLATFORM_RANGE_HEADER_CAP] = L"";
    if (offset > 0) {
        char narrow[PLATFORM_RANGE_HEADER_CAP];
        snprintf(narrow, sizeof(narrow), "Range: bytes=%lld-", offset);
        PlatformWiden(narrow, range, PLATFORM_RANGE_HEADER_CAP);
    }

    HINTERNET session = WinHttpOpen(
        PLATFORM_DOWNLOAD_USER_AGENT,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    HINTERNET connection = session ? WinHttpConnect(session, host, parts.nPort, 0) : NULL;
    DWORD secure = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request =
        connection
            ? WinHttpOpenRequest(
                  connection, L"GET", request_path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure
              )
            : NULL;
    if (session) {
        WinHttpSetTimeouts(
            session,
            PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS,
            PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS,
            PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS,
            PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS
        );
    }
    DWORD status = 0, status_size = sizeof(status);
    bool answered = request &&
                    WinHttpSendRequest(
                        request,
                        offset > 0 ? range : WINHTTP_NO_ADDITIONAL_HEADERS,
                        offset > 0 ? (DWORD)-1L : 0,
                        WINHTTP_NO_REQUEST_DATA,
                        0,
                        0,
                        0
                    ) &&
                    WinHttpReceiveResponse(request, NULL) &&
                    WinHttpQueryHeaders(
                        request,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &status,
                        &status_size,
                        WINHTTP_NO_HEADER_INDEX
                    );

    PlatformDownloadResult result = PLATFORM_DOWNLOAD_NETWORK_ERROR;
    bool resumed = offset > 0 && status == PLATFORM_HTTP_PARTIAL_CONTENT;
    if (!answered) {
        result = PLATFORM_DOWNLOAD_NETWORK_ERROR;
    } else if (offset > 0 && status == PLATFORM_HTTP_RANGE_NOT_SATISFIABLE) {
        *resume_rejected = true;
        result = PLATFORM_DOWNLOAD_SERVER_ERROR;
    } else if (status == PLATFORM_HTTP_OK || resumed) {
        FILE *file = fopen(path, resumed ? "ab" : "wb");
        result = file ? PlatformReceiveBody(request, file, resumed ? offset : 0, progress, context)
                      : PLATFORM_DOWNLOAD_FILE_ERROR;
        if (file && fclose(file) != 0 && result == PLATFORM_DOWNLOAD_OK) result = PLATFORM_DOWNLOAD_FILE_ERROR;
    } else {
        result = PLATFORM_DOWNLOAD_SERVER_ERROR;
    }
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    if (session) WinHttpCloseHandle(session);
    return result;
}

PlatformDownloadResult
PlatformDownload(const char *url, const char *path, bool resume, PlatformDownloadProgress progress, void *context) {
    wchar_t wide_url[PLATFORM_WIDE_URL_CAP];
    if (!PlatformWiden(url, wide_url, PLATFORM_WIDE_URL_CAP)) return PLATFORM_DOWNLOAD_NETWORK_ERROR;
    bool resume_rejected = false;
    long long offset = resume ? PlatformFileBytes(path) : 0;
    PlatformDownloadResult result = PlatformTransfer(wide_url, path, offset, progress, context, &resume_rejected);
    if (resume_rejected) result = PlatformTransfer(wide_url, path, 0, progress, context, &resume_rejected);
    return result;
}

bool PlatformUserDataDirectory(const char *app_name, char *out, size_t capacity) {
    const char *base = getenv(PLATFORM_APP_DATA_ENV);
    if (!base || !base[0]) return false;
    int length = snprintf(out, capacity, "%s/%s", base, app_name);
    return length > 0 && (size_t)length < capacity;
}

bool PlatformMakeDirectory(const char *path) {
    return _mkdir(path) == 0 || errno == EEXIST;
}

#else
#include <curl/curl.h>
#include <dlfcn.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define PLATFORM_DOWNLOAD_USER_AGENT "HeroesOfDragonAge"
#define PLATFORM_HTTP_RANGE_NOT_SATISFIABLE 416
#define PLATFORM_DIRECTORY_MODE 0755
#define PLATFORM_HOME_ENV "HOME"
#define PLATFORM_XDG_DATA_ENV "XDG_DATA_HOME"

#ifdef __APPLE__
#define PLATFORM_USER_DATA_SUFFIX "Library/Application Support"
static const char *const platform_curl_libraries[] = {"/usr/lib/libcurl.4.dylib", "libcurl.4.dylib"};
#else
#define PLATFORM_USER_DATA_SUFFIX ".local/share"
static const char *const platform_curl_libraries[] = {"libcurl.so.4", "libcurl-gnutls.so.4", "libcurl.so"};
#endif

typedef struct {
    CURLcode (*global_init)(long flags);
    CURL *(*easy_init)(void);
    CURLcode (*easy_setopt)(CURL *handle, CURLoption option, ...);
    CURLcode (*easy_perform)(CURL *handle);
    CURLcode (*easy_getinfo)(CURL *handle, CURLINFO info, ...);
    void (*easy_cleanup)(CURL *handle);
    bool ready;
} PlatformCurl;

typedef struct {
    FILE *file;
    long long offset;
    PlatformDownloadProgress progress;
    void *context;
    bool cancelled;
    bool write_failed;
} PlatformTransferState;

static PlatformCurl platform_curl;
static pthread_once_t platform_curl_once = PTHREAD_ONCE_INIT;

static void *PlatformCurlSymbol(void *library, const char *name) {
    return dlsym(library, name);
}

static void PlatformCurlLoad(void) {
    void *library = NULL;
    size_t candidates = sizeof(platform_curl_libraries) / sizeof(platform_curl_libraries[0]);
    for (size_t i = 0; !library && i < candidates; i++)
        library = dlopen(platform_curl_libraries[i], RTLD_NOW | RTLD_LOCAL);
    if (!library) return;
    *(void **)&platform_curl.global_init = PlatformCurlSymbol(library, "curl_global_init");
    *(void **)&platform_curl.easy_init = PlatformCurlSymbol(library, "curl_easy_init");
    *(void **)&platform_curl.easy_setopt = PlatformCurlSymbol(library, "curl_easy_setopt");
    *(void **)&platform_curl.easy_perform = PlatformCurlSymbol(library, "curl_easy_perform");
    *(void **)&platform_curl.easy_getinfo = PlatformCurlSymbol(library, "curl_easy_getinfo");
    *(void **)&platform_curl.easy_cleanup = PlatformCurlSymbol(library, "curl_easy_cleanup");
    bool resolved = platform_curl.global_init && platform_curl.easy_init && platform_curl.easy_setopt &&
                    platform_curl.easy_perform && platform_curl.easy_getinfo && platform_curl.easy_cleanup;
    platform_curl.ready = resolved && platform_curl.global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

static size_t PlatformTransferWrite(char *data, size_t size, size_t count, void *user) {
    PlatformTransferState *transfer = user;
    size_t length = size * count;
    if (fwrite(data, 1, length, transfer->file) == length) return length;
    transfer->write_failed = true;
    return 0;
}

static int PlatformTransferProgress(
    void *user, curl_off_t expected, curl_off_t received, curl_off_t upload_expected, curl_off_t uploaded
) {
    (void)expected;
    (void)upload_expected;
    (void)uploaded;
    PlatformTransferState *transfer = user;
    if (!transfer->progress || transfer->progress(transfer->context, transfer->offset + (long long)received)) return 0;
    transfer->cancelled = true;
    return 1;
}

static long long PlatformFileBytes(const char *path) {
    struct stat info;
    return stat(path, &info) == 0 ? (long long)info.st_size : 0;
}

static bool PlatformResumeRejected(CURLcode code, long status) {
    return code == CURLE_RANGE_ERROR || code == CURLE_BAD_DOWNLOAD_RESUME ||
           (code == CURLE_HTTP_RETURNED_ERROR && status == PLATFORM_HTTP_RANGE_NOT_SATISFIABLE);
}

static PlatformDownloadResult PlatformTransfer(
    const char *url,
    const char *path,
    long long offset,
    PlatformDownloadProgress progress,
    void *context,
    bool *resume_rejected
) {
    PlatformTransferState transfer = {.offset = offset, .progress = progress, .context = context};
    transfer.file = fopen(path, offset > 0 ? "ab" : "wb");
    if (!transfer.file) return PLATFORM_DOWNLOAD_FILE_ERROR;
    CURL *curl = platform_curl.easy_init();
    if (!curl) {
        fclose(transfer.file);
        return PLATFORM_DOWNLOAD_UNAVAILABLE;
    }
    platform_curl.easy_setopt(curl, CURLOPT_URL, url);
    platform_curl.easy_setopt(curl, CURLOPT_USERAGENT, PLATFORM_DOWNLOAD_USER_AGENT);
    platform_curl.easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    platform_curl.easy_setopt(curl, CURLOPT_MAXREDIRS, PLATFORM_DOWNLOAD_MAX_REDIRECTS);
    platform_curl.easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    platform_curl.easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    platform_curl.easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, PLATFORM_DOWNLOAD_CONNECT_TIMEOUT_SECONDS);
    platform_curl.easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, PLATFORM_DOWNLOAD_STALL_BYTES_PER_SECOND);
    platform_curl.easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, PLATFORM_DOWNLOAD_STALL_SECONDS);
    platform_curl.easy_setopt(curl, CURLOPT_WRITEFUNCTION, PlatformTransferWrite);
    platform_curl.easy_setopt(curl, CURLOPT_WRITEDATA, &transfer);
    platform_curl.easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    platform_curl.easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, PlatformTransferProgress);
    platform_curl.easy_setopt(curl, CURLOPT_XFERINFODATA, &transfer);
    if (offset > 0) platform_curl.easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)offset);
    CURLcode code = platform_curl.easy_perform(curl);
    long status = 0;
    platform_curl.easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    platform_curl.easy_cleanup(curl);
    if (fclose(transfer.file) != 0) transfer.write_failed = true;

    if (transfer.cancelled) return PLATFORM_DOWNLOAD_CANCELLED;
    if (transfer.write_failed) return PLATFORM_DOWNLOAD_FILE_ERROR;
    if (code == CURLE_OK) return PLATFORM_DOWNLOAD_OK;
    if (offset > 0 && PlatformResumeRejected(code, status)) *resume_rejected = true;
    return code == CURLE_HTTP_RETURNED_ERROR ? PLATFORM_DOWNLOAD_SERVER_ERROR : PLATFORM_DOWNLOAD_NETWORK_ERROR;
}

PlatformDownloadResult
PlatformDownload(const char *url, const char *path, bool resume, PlatformDownloadProgress progress, void *context) {
    pthread_once(&platform_curl_once, PlatformCurlLoad);
    if (!platform_curl.ready) return PLATFORM_DOWNLOAD_UNAVAILABLE;
    bool resume_rejected = false;
    long long offset = resume ? PlatformFileBytes(path) : 0;
    PlatformDownloadResult result = PlatformTransfer(url, path, offset, progress, context, &resume_rejected);
    if (resume_rejected) result = PlatformTransfer(url, path, 0, progress, context, &resume_rejected);
    return result;
}

bool PlatformUserDataDirectory(const char *app_name, char *out, size_t capacity) {
    const char *xdg = getenv(PLATFORM_XDG_DATA_ENV);
    const char *home = getenv(PLATFORM_HOME_ENV);
    int length = -1;
#ifndef __APPLE__
    if (xdg && xdg[0] == '/') length = snprintf(out, capacity, "%s/%s", xdg, app_name);
#else
    (void)xdg;
#endif
    if (length < 0 && home && home[0])
        length = snprintf(out, capacity, "%s/%s/%s", home, PLATFORM_USER_DATA_SUFFIX, app_name);
    return length > 0 && (size_t)length < capacity;
}

bool PlatformMakeDirectory(const char *path) {
    return mkdir(path, PLATFORM_DIRECTORY_MODE) == 0 || errno == EEXIST;
}

intptr_t PlatformSocketConnect(const char *host, const char *port, int timeout_seconds) {
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    struct addrinfo *addresses = NULL;
    if (getaddrinfo(host, port, &hints, &addresses) != 0) return PLATFORM_SOCKET_INVALID;
    int connected = -1;
    struct timeval timeout = {.tv_sec = timeout_seconds};
    for (struct addrinfo *address = addresses; address; address = address->ai_next) {
        connected = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (connected < 0) continue;
        setsockopt(connected, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(connected, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        if (connect(connected, address->ai_addr, address->ai_addrlen) == 0) break;
        close(connected);
        connected = -1;
    }
    freeaddrinfo(addresses);
    return connected < 0 ? PLATFORM_SOCKET_INVALID : (intptr_t)connected;
}

long PlatformSocketSend(intptr_t socket_handle, const char *data, size_t length) {
    return (long)send((int)socket_handle, data, length, 0);
}

long PlatformSocketReceive(intptr_t socket_handle, char *buffer, size_t capacity) {
    return (long)recv((int)socket_handle, buffer, capacity, 0);
}

void PlatformSocketClose(intptr_t socket_handle) {
    close((int)socket_handle);
}

bool PlatformReadAt(FILE *file, long long offset, void *out, size_t size) {
    return pread(fileno(file), out, size, (off_t)offset) == (ssize_t)size;
}

bool PlatformReplaceFile(const char *source, const char *target) {
    return rename(source, target) == 0;
}

int PlatformProcessorCount(void) {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return count > 0 ? (int)count : 1;
}

time_t PlatformTimeFromUtc(struct tm *utc) {
    return timegm(utc);
}
#endif
