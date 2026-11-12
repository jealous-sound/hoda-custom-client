#include "platform.h"
#include "client_version.h"

#include <stdlib.h>
#include <string.h>

#define PLATFORM_DOWNLOAD_CONNECT_TIMEOUT_SECONDS 15L
#define PLATFORM_DOWNLOAD_STALL_BYTES_PER_SECOND 1L
#define PLATFORM_DOWNLOAD_STALL_SECONDS 20L
#define PLATFORM_DOWNLOAD_MAX_REDIRECTS 5L
#define PLATFORM_HTTP_BODY_INITIAL_BYTES 16384
#define PLATFORM_HTTP_STALL_BYTES_PER_SECOND 1L

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <direct.h>
#include <errno.h>
#include <io.h>

#define PLATFORM_MILLISECONDS_PER_SECOND 1000
#define PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS 30000
#define PLATFORM_DOWNLOAD_USER_AGENT L"HeroesOfDragonAge/" HOD_VERSION
#define PLATFORM_DOWNLOAD_BUFFER_BYTES 65536
#define PLATFORM_WIDE_URL_CAP 2048
#define PLATFORM_HOST_CAP 256
#define PLATFORM_RANGE_HEADER_CAP 64
#define PLATFORM_HTTP_METHOD_CAP 16
#define PLATFORM_HTTP_HEADER_NAME_CAP 64
#define PLATFORM_HTTP_HEADERS_CAP 2048
#define PLATFORM_HTTP_OK 200
#define PLATFORM_HTTP_PARTIAL_CONTENT 206
#define PLATFORM_HTTP_RANGE_NOT_SATISFIABLE 416
#define PLATFORM_APP_DATA_ENV "APPDATA"
#define PLATFORM_LOCAL_APP_DATA_ENV "LOCALAPPDATA"
#define PLATFORM_WINDOWS_DIR_ENV "WINDIR"
#define PLATFORM_WINDOWS_DIR_DEFAULT "C:\\Windows"
#define PLATFORM_PATH_CAP 1024
#define PLATFORM_COMMAND_CAP 1100

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

bool PlatformMoveEntry(const char *source, const char *target) {
    return MoveFileExA(source, target, MOVEFILE_COPY_ALLOWED) != 0;
}

bool PlatformDiskFull(void) {
    DWORD error = GetLastError();
    return errno == ENOSPC || error == ERROR_DISK_FULL || error == ERROR_HANDLE_DISK_FULL;
}

long PlatformLastFailure(void) {
    return (long)GetLastError();
}

void PlatformKeepLog(const char *path) {
    if (freopen(path, "w", stdout)) setvbuf(stdout, NULL, _IONBF, 0);
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
            result = PlatformDiskFull() ? PLATFORM_DOWNLOAD_DISK_FULL : PLATFORM_DOWNLOAD_FILE_ERROR;
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

typedef struct {
    HINTERNET session;
    HINTERNET connection;
    HINTERNET request;
} PlatformWinHttp;

static void PlatformWinHttpClose(PlatformWinHttp *handles) {
    if (handles->request) WinHttpCloseHandle(handles->request);
    if (handles->connection) WinHttpCloseHandle(handles->connection);
    if (handles->session) WinHttpCloseHandle(handles->session);
    *handles = (PlatformWinHttp){0};
}

static bool
PlatformWinHttpOpen(const wchar_t *url, const wchar_t *method, DWORD timeout_milliseconds, PlatformWinHttp *handles) {
    *handles = (PlatformWinHttp){0};
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
    if (!WinHttpCrackUrl(url, 0, 0, &parts)) return false;
    if (wcslen(request_path) + wcslen(extra) >= PLATFORM_WIDE_URL_CAP) return false;
    wcscat(request_path, extra);

    handles->session = WinHttpOpen(
        PLATFORM_DOWNLOAD_USER_AGENT,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (handles->session)
        WinHttpSetTimeouts(
            handles->session, timeout_milliseconds, timeout_milliseconds, timeout_milliseconds, timeout_milliseconds
        );
    handles->connection = handles->session ? WinHttpConnect(handles->session, host, parts.nPort, 0) : NULL;
    DWORD secure = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    handles->request = handles->connection ? WinHttpOpenRequest(
                                                 handles->connection,
                                                 method,
                                                 request_path,
                                                 NULL,
                                                 WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 secure
                                             )
                                           : NULL;
    if (handles->request) return true;
    PlatformWinHttpClose(handles);
    return false;
}

static bool PlatformWinHttpStatus(HINTERNET request, DWORD *status) {
    DWORD status_size = sizeof(*status);
    return WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        status,
        &status_size,
        WINHTTP_NO_HEADER_INDEX
    );
}

static PlatformDownloadResult PlatformTransfer(
    const wchar_t *url,
    const char *path,
    long long offset,
    PlatformDownloadProgress progress,
    void *context,
    bool *resume_rejected
) {
    wchar_t range[PLATFORM_RANGE_HEADER_CAP] = L"";
    if (offset > 0) {
        char narrow[PLATFORM_RANGE_HEADER_CAP];
        snprintf(narrow, sizeof(narrow), "Range: bytes=%lld-", offset);
        PlatformWiden(narrow, range, PLATFORM_RANGE_HEADER_CAP);
    }

    PlatformWinHttp handles;
    if (!PlatformWinHttpOpen(url, L"GET", PLATFORM_DOWNLOAD_TIMEOUT_MILLISECONDS, &handles))
        return PLATFORM_DOWNLOAD_NETWORK_ERROR;
    DWORD status = 0;
    bool answered = WinHttpSendRequest(
                        handles.request,
                        offset > 0 ? range : WINHTTP_NO_ADDITIONAL_HEADERS,
                        offset > 0 ? (DWORD)-1L : 0,
                        WINHTTP_NO_REQUEST_DATA,
                        0,
                        0,
                        0
                    ) &&
                    WinHttpReceiveResponse(handles.request, NULL) && PlatformWinHttpStatus(handles.request, &status);

    PlatformDownloadResult result = PLATFORM_DOWNLOAD_NETWORK_ERROR;
    bool resumed = offset > 0 && status == PLATFORM_HTTP_PARTIAL_CONTENT;
    if (!answered) {
        result = PLATFORM_DOWNLOAD_NETWORK_ERROR;
    } else if (offset > 0 && status == PLATFORM_HTTP_RANGE_NOT_SATISFIABLE) {
        *resume_rejected = true;
        result = PLATFORM_DOWNLOAD_SERVER_ERROR;
    } else if (status == PLATFORM_HTTP_OK || resumed) {
        FILE *file = fopen(path, resumed ? "ab" : "wb");
        result = file ? PlatformReceiveBody(handles.request, file, resumed ? offset : 0, progress, context)
                      : PLATFORM_DOWNLOAD_FILE_ERROR;
        if (file && fclose(file) != 0 && result == PLATFORM_DOWNLOAD_OK) result = PLATFORM_DOWNLOAD_FILE_ERROR;
    } else {
        result = PLATFORM_DOWNLOAD_SERVER_ERROR;
    }
    PlatformWinHttpClose(&handles);
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

static bool PlatformWinHttpHeaders(const PlatformHttpRequest *request, wchar_t *out, int capacity) {
    char joined[PLATFORM_HTTP_HEADERS_CAP] = "";
    size_t used = 0;
    for (int i = 0; i < request->header_count; i++) {
        int written = snprintf(joined + used, sizeof(joined) - used, "%s\r\n", request->headers[i]);
        if (written < 0 || (size_t)written >= sizeof(joined) - used) return false;
        used += (size_t)written;
    }
    return PlatformWiden(joined, out, capacity);
}

static void PlatformWinHttpReplyHeader(HINTERNET request, const char *name, char *out, int capacity) {
    out[0] = '\0';
    wchar_t wide_name[PLATFORM_HTTP_HEADER_NAME_CAP], value[PLATFORM_HTTP_HEADER_VALUE_CAP];
    DWORD value_size = sizeof(value);
    if (!name || !PlatformWiden(name, wide_name, PLATFORM_HTTP_HEADER_NAME_CAP)) return;
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, wide_name, value, &value_size, WINHTTP_NO_HEADER_INDEX))
        return;
    if (WideCharToMultiByte(CP_UTF8, 0, value, -1, out, capacity, NULL, NULL) == 0) out[0] = '\0';
}

static char *PlatformWinHttpReadBody(HINTERNET request) {
    size_t length = 0, capacity = PLATFORM_HTTP_BODY_INITIAL_BYTES;
    char *body = malloc(capacity + 1);
    for (;;) {
        DWORD available = 0, read = 0;
        if (!body || !WinHttpQueryDataAvailable(request, &available)) break;
        if (available == 0) {
            body[length] = '\0';
            return body;
        }
        if (length + available > capacity) {
            while (length + available > capacity) capacity *= 2;
            char *grown = realloc(body, capacity + 1);
            if (!grown) break;
            body = grown;
        }
        if (!WinHttpReadData(request, body + length, available, &read)) break;
        length += read;
    }
    free(body);
    return NULL;
}

bool PlatformHttpSend(const PlatformHttpRequest *request, PlatformHttpResponse *response) {
    *response = (PlatformHttpResponse){0};
    wchar_t url[PLATFORM_WIDE_URL_CAP], method[PLATFORM_HTTP_METHOD_CAP], headers[PLATFORM_HTTP_HEADERS_CAP];
    if (!PlatformWiden(request->url, url, PLATFORM_WIDE_URL_CAP) ||
        !PlatformWiden(request->method, method, PLATFORM_HTTP_METHOD_CAP) ||
        !PlatformWinHttpHeaders(request, headers, PLATFORM_HTTP_HEADERS_CAP))
        return false;
    PlatformWinHttp handles;
    DWORD timeout = (DWORD)request->timeout_seconds * PLATFORM_MILLISECONDS_PER_SECOND;
    if (!PlatformWinHttpOpen(url, method, timeout, &handles)) return false;
    DWORD body_length = request->body ? (DWORD)strlen(request->body) : 0;
    DWORD status = 0;
    bool answered = WinHttpSendRequest(
                        handles.request,
                        headers[0] ? headers : WINHTTP_NO_ADDITIONAL_HEADERS,
                        headers[0] ? (DWORD)-1L : 0,
                        body_length ? (LPVOID)request->body : WINHTTP_NO_REQUEST_DATA,
                        body_length,
                        body_length,
                        0
                    ) &&
                    WinHttpReceiveResponse(handles.request, NULL) && PlatformWinHttpStatus(handles.request, &status);
    if (answered) {
        PlatformWinHttpReplyHeader(
            handles.request, request->reply_header, response->reply_header_value, PLATFORM_HTTP_HEADER_VALUE_CAP
        );
        response->body = PlatformWinHttpReadBody(handles.request);
        response->status = (int)status;
    }
    PlatformWinHttpClose(&handles);
    return response->body != NULL;
}

bool PlatformUserDataDirectory(const char *app_name, char *out, size_t capacity) {
    const char *base = getenv(PLATFORM_APP_DATA_ENV);
    if (!base || !base[0]) return false;
    int length = snprintf(out, capacity, "%s/%s", base, app_name);
    return length > 0 && (size_t)length < capacity;
}

bool PlatformCompactDataDirectory(const char *compact_name, char *out, size_t capacity) {
    const char *base = getenv(PLATFORM_LOCAL_APP_DATA_ENV);
    if (!base || !base[0]) return false;
    int length = snprintf(out, capacity, "%s/%s", base, compact_name);
    return length > 0 && (size_t)length < capacity;
}

bool PlatformMakeDirectory(const char *path) {
    return _mkdir(path) == 0 || errno == EEXIST;
}

static const char *const platform_fallback_fonts[] = {
    "msyh.ttc",
    "msyh.ttf",
    "simhei.ttf",
    "msjh.ttc",
    "msjh.ttf",
    "simsun.ttc",
    "mingliu.ttc",
    "malgun.ttf",
    "meiryo.ttc",
};

int PlatformFallbackFonts(char paths[][PLATFORM_FONT_PATH_CAP], int capacity) {
    const char *windows = getenv(PLATFORM_WINDOWS_DIR_ENV);
    if (!windows || !windows[0]) windows = PLATFORM_WINDOWS_DIR_DEFAULT;
    int count = 0;
    for (size_t i = 0; i < sizeof(platform_fallback_fonts) / sizeof(platform_fallback_fonts[0]) && count < capacity;
         i++)
        snprintf(paths[count++], PLATFORM_FONT_PATH_CAP, "%s\\Fonts\\%s", windows, platform_fallback_fonts[i]);
    return count;
}

bool PlatformPreferredLanguage(char *out, size_t capacity) {
    wchar_t name[LOCALE_NAME_MAX_LENGTH];
    LCID interface_language = MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT);
    if (!LCIDToLocaleName(interface_language, name, LOCALE_NAME_MAX_LENGTH, 0)) return false;
    return WideCharToMultiByte(CP_UTF8, 0, name, -1, out, (int)capacity, NULL, NULL) > 0;
}

bool PlatformMakeExecutable(const char *path) {
    (void)path;
    return true;
}

static bool PlatformRemoveEntry(const char *path, DWORD attributes) {
    if (attributes & FILE_ATTRIBUTE_READONLY) SetFileAttributesA(path, attributes & ~(DWORD)FILE_ATTRIBUTE_READONLY);
    return attributes & FILE_ATTRIBUTE_DIRECTORY ? RemoveDirectoryA(path) != 0 : DeleteFileA(path) != 0;
}

bool PlatformRemoveTree(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    bool walkable = (attributes & FILE_ATTRIBUTE_DIRECTORY) && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
    if (!walkable) return PlatformRemoveEntry(path, attributes);
    char pattern[PLATFORM_PATH_CAP];
    if ((size_t)snprintf(pattern, sizeof(pattern), "%s\\*", path) >= sizeof(pattern)) return false;
    bool emptied = true;
    WIN32_FIND_DATAA found;
    HANDLE search = FindFirstFileA(pattern, &found);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if (!strcmp(found.cFileName, ".") || !strcmp(found.cFileName, "..")) continue;
            char child[PLATFORM_PATH_CAP];
            bool named = (size_t)snprintf(child, sizeof(child), "%s\\%s", path, found.cFileName) < sizeof(child);
            emptied = named && PlatformRemoveTree(child) && emptied;
        } while (FindNextFileA(search, &found));
        FindClose(search);
    }
    return PlatformRemoveEntry(path, attributes) && emptied;
}

bool PlatformStartProgram(const char *executable) {
    char command[PLATFORM_COMMAND_CAP];
    if ((size_t)snprintf(command, sizeof(command), "\"%s\"", executable) >= sizeof(command)) return false;
    STARTUPINFOA startup = {.cb = sizeof(startup)};
    PROCESS_INFORMATION process;
    if (!CreateProcessA(executable, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) return false;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

bool PlatformContinueAs(const char *executable) {
    return PlatformStartProgram(executable);
}

#else
#include <ctype.h>
#include <curl/curl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <spawn.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

extern char **environ;

#define PLATFORM_DOWNLOAD_USER_AGENT "HeroesOfDragonAge/" HOD_VERSION
#define PLATFORM_HTTP_RANGE_NOT_SATISFIABLE 416
#define PLATFORM_DIRECTORY_MODE 0755
#define PLATFORM_EXECUTABLE_MODE 0755
#define PLATFORM_PATH_CAP 1024
#define PLATFORM_HOME_ENV "HOME"
#define PLATFORM_XDG_DATA_ENV "XDG_DATA_HOME"
#define PLATFORM_HTTP_NO_EXPECT_HEADER "Expect:"
#define PLATFORM_HTTP_GET "GET"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
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
    struct curl_slist *(*slist_append)(struct curl_slist *list, const char *text);
    void (*slist_free_all)(struct curl_slist *list);
    bool ready;
} PlatformCurl;

typedef struct {
    FILE *file;
    long long offset;
    PlatformDownloadProgress progress;
    void *context;
    bool cancelled;
    bool write_failed;
    bool disk_full;
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
    *(void **)&platform_curl.slist_append = PlatformCurlSymbol(library, "curl_slist_append");
    *(void **)&platform_curl.slist_free_all = PlatformCurlSymbol(library, "curl_slist_free_all");
    bool resolved = platform_curl.global_init && platform_curl.easy_init && platform_curl.easy_setopt &&
                    platform_curl.easy_perform && platform_curl.easy_getinfo && platform_curl.easy_cleanup &&
                    platform_curl.slist_append && platform_curl.slist_free_all;
    platform_curl.ready = resolved && platform_curl.global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

static size_t PlatformTransferWrite(char *data, size_t size, size_t count, void *user) {
    PlatformTransferState *transfer = user;
    size_t length = size * count;
    if (fwrite(data, 1, length, transfer->file) == length) return length;
    transfer->write_failed = true;
    transfer->disk_full = PlatformDiskFull();
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
    if (transfer.disk_full) return PLATFORM_DOWNLOAD_DISK_FULL;
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

typedef struct {
    char *body;
    size_t length;
    size_t capacity;
    bool failed;
    const char *header_name;
    char header_value[PLATFORM_HTTP_HEADER_VALUE_CAP];
} PlatformHttpReply;

static size_t PlatformHttpCollectBody(char *data, size_t size, size_t count, void *user) {
    PlatformHttpReply *reply = user;
    size_t length = size * count;
    size_t needed = reply->length + length + 1;
    if (needed > reply->capacity) {
        size_t capacity = reply->capacity ? reply->capacity : PLATFORM_HTTP_BODY_INITIAL_BYTES;
        while (capacity < needed) capacity *= 2;
        char *grown = realloc(reply->body, capacity);
        if (!grown) {
            reply->failed = true;
            return 0;
        }
        reply->body = grown;
        reply->capacity = capacity;
    }
    memcpy(reply->body + reply->length, data, length);
    reply->length += length;
    reply->body[reply->length] = '\0';
    return length;
}

static size_t PlatformHttpCollectHeader(char *data, size_t size, size_t count, void *user) {
    PlatformHttpReply *reply = user;
    size_t length = size * count;
    size_t name_length = reply->header_name ? strlen(reply->header_name) : 0;
    bool wanted = name_length > 0 && length > name_length && data[name_length] == ':' &&
                  strncasecmp(data, reply->header_name, name_length) == 0;
    if (!wanted) return length;
    const char *value = data + name_length + 1, *end = data + length;
    while (value < end && isspace((unsigned char)*value)) value++;
    while (end > value && isspace((unsigned char)end[-1])) end--;
    size_t value_length = (size_t)(end - value);
    if (value_length >= sizeof(reply->header_value)) value_length = sizeof(reply->header_value) - 1;
    memcpy(reply->header_value, value, value_length);
    reply->header_value[value_length] = '\0';
    return length;
}

static struct curl_slist *PlatformHttpHeaderList(const PlatformHttpRequest *request) {
    struct curl_slist *headers = platform_curl.slist_append(NULL, PLATFORM_HTTP_NO_EXPECT_HEADER);
    for (int i = 0; headers && i < request->header_count; i++) {
        struct curl_slist *appended = platform_curl.slist_append(headers, request->headers[i]);
        if (!appended) platform_curl.slist_free_all(headers);
        headers = appended;
    }
    return headers;
}

bool PlatformHttpSend(const PlatformHttpRequest *request, PlatformHttpResponse *response) {
    *response = (PlatformHttpResponse){0};
    pthread_once(&platform_curl_once, PlatformCurlLoad);
    if (!platform_curl.ready) return false;
    struct curl_slist *headers = PlatformHttpHeaderList(request);
    CURL *curl = headers ? platform_curl.easy_init() : NULL;
    if (!curl) {
        if (headers) platform_curl.slist_free_all(headers);
        return false;
    }
    PlatformHttpReply reply = {.header_name = request->reply_header};
    long timeout = (long)request->timeout_seconds;
    platform_curl.easy_setopt(curl, CURLOPT_URL, request->url);
    platform_curl.easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request->method);
    platform_curl.easy_setopt(curl, CURLOPT_USERAGENT, PLATFORM_DOWNLOAD_USER_AGENT);
    platform_curl.easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    platform_curl.easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);
    platform_curl.easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, PLATFORM_HTTP_STALL_BYTES_PER_SECOND);
    platform_curl.easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, timeout);
    platform_curl.easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    platform_curl.easy_setopt(curl, CURLOPT_WRITEFUNCTION, PlatformHttpCollectBody);
    platform_curl.easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
    platform_curl.easy_setopt(curl, CURLOPT_HEADERFUNCTION, PlatformHttpCollectHeader);
    platform_curl.easy_setopt(curl, CURLOPT_HEADERDATA, &reply);
    if (request->body || strcmp(request->method, PLATFORM_HTTP_GET) != 0) {
        const char *body = request->body ? request->body : "";
        platform_curl.easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        platform_curl.easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strlen(body));
    }
    CURLcode code = platform_curl.easy_perform(curl);
    long status = 0;
    platform_curl.easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    platform_curl.easy_cleanup(curl);
    platform_curl.slist_free_all(headers);
    if (code != CURLE_OK || reply.failed) {
        free(reply.body);
        return false;
    }
    response->body = reply.body ? reply.body : calloc(1, 1);
    response->status = (int)status;
    memcpy(response->reply_header_value, reply.header_value, sizeof(reply.header_value));
    return response->body != NULL;
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

bool PlatformCompactDataDirectory(const char *compact_name, char *out, size_t capacity) {
    (void)compact_name;
    (void)out;
    (void)capacity;
    return false;
}

bool PlatformMakeDirectory(const char *path) {
    return mkdir(path, PLATFORM_DIRECTORY_MODE) == 0 || errno == EEXIST;
}

bool PlatformMoveEntry(const char *source, const char *target) {
    return rename(source, target) == 0;
}

bool PlatformDiskFull(void) {
    return errno == ENOSPC || errno == EDQUOT;
}

long PlatformLastFailure(void) {
    return (long)errno;
}

void PlatformKeepLog(const char *path) {
    (void)path;
}

#ifdef __APPLE__
static const char *const platform_fallback_fonts[] = {
    "/System/Library/Fonts/Hiragino Sans GB.ttc",
    "/System/Library/Fonts/STHeiti Medium.ttc",
    "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
    "/Library/Fonts/Arial Unicode.ttf",
    "/System/Library/Fonts/Supplemental/Songti.ttc",
    "/System/Library/Fonts/AppleSDGothicNeo.ttc",
};
#else
static const char *const platform_fallback_fonts[] = {
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
    "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
    "/usr/share/fonts/wenquanyi/wqy-microhei/wqy-microhei.ttc",
    "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
    "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
    "/usr/share/fonts/truetype/arphic/uming.ttc",
};
#endif

int PlatformFallbackFonts(char paths[][PLATFORM_FONT_PATH_CAP], int capacity) {
    int count = 0;
    for (size_t i = 0; i < sizeof(platform_fallback_fonts) / sizeof(platform_fallback_fonts[0]) && count < capacity;
         i++)
        snprintf(paths[count++], PLATFORM_FONT_PATH_CAP, "%s", platform_fallback_fonts[i]);
    return count;
}

#ifdef __APPLE__
bool PlatformPreferredLanguage(char *out, size_t capacity) {
    CFArrayRef languages = CFLocaleCopyPreferredLanguages();
    if (!languages) return false;
    bool found =
        CFArrayGetCount(languages) > 0 &&
        CFStringGetCString(CFArrayGetValueAtIndex(languages, 0), out, (CFIndex)capacity, kCFStringEncodingUTF8);
    CFRelease(languages);
    return found;
}
#else
static const char *const platform_language_variables[] = {"LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"};

bool PlatformPreferredLanguage(char *out, size_t capacity) {
    for (size_t i = 0; i < sizeof(platform_language_variables) / sizeof(platform_language_variables[0]); i++) {
        const char *value = getenv(platform_language_variables[i]);
        if (!value || !value[0] || !strcmp(value, "C") || !strcmp(value, "POSIX")) continue;
        int length = snprintf(out, capacity, "%s", value);
        return length > 0 && (size_t)length < capacity;
    }
    return false;
}
#endif

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

bool PlatformMakeExecutable(const char *path) {
    return chmod(path, PLATFORM_EXECUTABLE_MODE) == 0;
}

bool PlatformRemoveTree(const char *path) {
    struct stat info;
    if (lstat(path, &info) != 0) return errno == ENOENT;
    if (!S_ISDIR(info.st_mode)) return unlink(path) == 0;
    DIR *directory = opendir(path);
    if (!directory) return false;
    bool emptied = true;
    for (struct dirent *entry = readdir(directory); entry; entry = readdir(directory)) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        char child[PLATFORM_PATH_CAP];
        bool named = (size_t)snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) < sizeof(child);
        emptied = named && PlatformRemoveTree(child) && emptied;
    }
    closedir(directory);
    return rmdir(path) == 0 && emptied;
}

bool PlatformStartProgram(const char *executable) {
    char *arguments[] = {(char *)executable, NULL};
    pid_t child;
    return posix_spawn(&child, executable, NULL, NULL, arguments, environ) == 0;
}

bool PlatformContinueAs(const char *executable) {
    char *arguments[] = {(char *)executable, NULL};
    execv(executable, arguments);
    return false;
}
#endif
