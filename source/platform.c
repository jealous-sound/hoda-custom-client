#ifdef __EMSCRIPTEN__
#define _GNU_SOURCE
#endif
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

static bool PlatformReadLocalRanges(const char *path, PlatformFileRange *ranges, int count) {
    FILE *file = fopen(path, "rb");
    bool all_read = file != NULL;
    for (int i = 0; i < count; i++) {
        ranges[i].read = file && PlatformReadAt(file, ranges[i].offset, ranges[i].out, ranges[i].size);
        all_read = all_read && ranges[i].read;
    }
    if (file) fclose(file);
    return all_read;
}

static int PlatformCompareSpans(const void *a, const void *b) {
    const PlatformByteSpan *left = a, *right = b;
    if (left->start != right->start) return left->start < right->start ? -1 : 1;
    return (left->end > right->end) - (left->end < right->end);
}

int PlatformCoalesceRanges(const PlatformFileRange *ranges, int count, long long gap, PlatformByteSpan *spans) {
    for (int i = 0; i < count; i++)
        spans[i] = (PlatformByteSpan){ranges[i].offset, ranges[i].offset + (long long)ranges[i].size};
    qsort(spans, (size_t)count, sizeof(*spans), PlatformCompareSpans);
    int merged = 0;
    for (int i = 0; i < count; i++) {
        if (merged > 0 && spans[i].start <= spans[merged - 1].end + gap) {
            if (spans[i].end > spans[merged - 1].end) spans[merged - 1].end = spans[i].end;
        } else {
            spans[merged++] = spans[i];
        }
    }
    return merged;
}

#ifndef __EMSCRIPTEN__
bool PlatformReadFileRanges(const char *path, PlatformFileRange *ranges, int count) {
    return PlatformReadLocalRanges(path, ranges, count);
}
#endif

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

bool PlatformPrepareGameFiles(void) {
    return true;
}

void PlatformJoinThread(pthread_t thread) {
    pthread_join(thread, NULL);
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

bool PlatformRemoveFile(const char *path) {
    return remove(path) == 0;
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
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/threading.h>
#include <emscripten/wasmfs.h>
#include <fcntl.h>
#else
#include <curl/curl.h>
#include <dlfcn.h>
#include <spawn.h>
#endif

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
#elif defined(__EMSCRIPTEN__)
#define PLATFORM_USER_DATA_SUFFIX ".local/share"
#else
#define PLATFORM_USER_DATA_SUFFIX ".local/share"
static const char *const platform_curl_libraries[] = {"libcurl.so.4", "libcurl-gnutls.so.4", "libcurl.so"};
#endif

#ifdef __EMSCRIPTEN__
#ifndef HOD_GAME_FILES_URL
#define HOD_GAME_FILES_URL ""
#endif
#define PLATFORM_WEB_FILES_URL_ENV "HOD_GAME_FILES_URL"
#define PLATFORM_WEB_ROOT "/"
#define PLATFORM_WEB_NODE_ROOT "."
#define PLATFORM_WEB_FILE_LIST "game_files.txt"
#define PLATFORM_WEB_LIST_SEPARATOR "  "
#define PLATFORM_WEB_URL_CAP 2048
#define PLATFORM_WEB_PATH_CAP 1024
#define PLATFORM_WEB_SAVES_DIR "/saves"
#define PLATFORM_WEB_STAGED_SUFFIX ".staged"
#define PLATFORM_WEB_COPY_BYTES 65536
#define PLATFORM_WEB_SAVE_QUEUE_CAP 16
#define PLATFORM_WEB_PREPARE_POLL_MILLISECONDS 10
#define PLATFORM_WEB_JOIN_POLL_MILLISECONDS 1
#define PLATFORM_WEB_LIST_TIMEOUT_SECONDS 60
#define PLATFORM_WEB_DOWNLOAD_TIMEOUT_SECONDS 600
#define PLATFORM_WEB_DIRECTORY_MODE 0777
#define PLATFORM_WEB_FILE_MODE 0444
#define PLATFORM_WEB_HEADER_SLOTS 16
#define PLATFORM_WEB_HEADER_CAP 512
#define PLATFORM_WEB_PROCESSOR_CAP 16
#define PLATFORM_WEB_SIZE_UNKNOWN (-1LL)
#define PLATFORM_WEB_WHOLE_FILE_BYTES (64LL * 1024)
#define PLATFORM_WEB_WHOLE_FILE_PERCENT 75
#define PLATFORM_WEB_PIECEMEAL_READS_BEFORE_WHOLE 3
#define PLATFORM_WEB_PIECEMEAL_SHARE_PERCENT 50
#define PLATFORM_WEB_RANGE_GAP_BYTES (64LL * 1024)
#define PLATFORM_WEB_RANGE_HEADER_CAP 64
#define PLATFORM_PERCENT 100
#define PLATFORM_HTTP_OK 200
#define PLATFORM_HTTP_PARTIAL_CONTENT 206
#define PLATFORM_HTTP_GET "GET"

typedef struct {
    char files_url[PLATFORM_WEB_URL_CAP];
    bool prepared;
    int restored;
    volatile int finished;
} PlatformWebFiles;

// clang-format off
EM_JS(int, PlatformWebPageUrl, (char *out, int capacity), {
    var url = new URL(".", location.href).href;
    if (lengthBytesUTF8(url) >= capacity) return 0;
    stringToUTF8(url, out, capacity);
    return 1;
});

EM_JS(int, PlatformWebHasOpfs, (void), {
    var storage = typeof navigator !== "undefined" && navigator.storage;
    return storage && typeof storage.getDirectory === "function" ? 1 : 0;
});

EM_JS(int, PlatformWebRunsInNode, (void), {
    return typeof process === "object" && process.versions && process.versions.node ? 1 : 0;
});
// clang-format on

static bool PlatformWebSplitHeader(const char *header, char *name, char *value, size_t capacity) {
    const char *colon = strchr(header, ':');
    if (!colon || (size_t)(colon - header) >= capacity) return false;
    memcpy(name, header, (size_t)(colon - header));
    name[colon - header] = '\0';
    const char *start = colon + 1;
    while (*start && isspace((unsigned char)*start)) start++;
    return (size_t)snprintf(value, capacity, "%s", start) < capacity;
}

static void PlatformWebReplyHeader(emscripten_fetch_t *fetch, const char *name, char *out, size_t capacity) {
    out[0] = '\0';
    if (!name || !name[0]) return;
    size_t length = emscripten_fetch_get_response_headers_length(fetch);
    char *raw = malloc(length + 1);
    if (!raw) return;
    emscripten_fetch_get_response_headers(fetch, raw, length + 1);
    char **pairs = emscripten_fetch_unpack_response_headers(raw);
    for (int i = 0; pairs && pairs[i] && pairs[i + 1]; i += 2) {
        if (strcasecmp(pairs[i], name) != 0) continue;
        const char *value = pairs[i + 1];
        while (*value && isspace((unsigned char)*value)) value++;
        snprintf(out, capacity, "%s", value);
        size_t end = strlen(out);
        while (end > 0 && isspace((unsigned char)out[end - 1])) out[--end] = '\0';
        break;
    }
    if (pairs) emscripten_fetch_free_unpacked_response_headers(pairs);
    free(raw);
}

static emscripten_fetch_t *PlatformWebFetch(const PlatformHttpRequest *request) {
    if (emscripten_is_main_browser_thread()) return NULL;
    char names[PLATFORM_WEB_HEADER_SLOTS][PLATFORM_WEB_HEADER_CAP];
    char values[PLATFORM_WEB_HEADER_SLOTS][PLATFORM_WEB_HEADER_CAP];
    const char *fields[PLATFORM_WEB_HEADER_SLOTS * 2 + 1];
    int field_count = 0;
    for (int i = 0; i < request->header_count && i < PLATFORM_WEB_HEADER_SLOTS; i++) {
        if (!PlatformWebSplitHeader(request->headers[i], names[i], values[i], PLATFORM_WEB_HEADER_CAP)) continue;
        fields[field_count++] = names[i];
        fields[field_count++] = values[i];
    }
    fields[field_count] = NULL;
    emscripten_fetch_attr_t attributes;
    emscripten_fetch_attr_init(&attributes);
    snprintf(attributes.requestMethod, sizeof(attributes.requestMethod), "%s", request->method);
    attributes.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;
    attributes.timeoutMSecs = (uint32_t)request->timeout_seconds * 1000u;
    attributes.requestHeaders = fields;
    if (request->body || strcmp(request->method, PLATFORM_HTTP_GET) != 0) {
        attributes.requestData = request->body ? request->body : "";
        attributes.requestDataSize = strlen(attributes.requestData);
    }
    emscripten_fetch_t *fetch = emscripten_fetch(&attributes, request->url);
    if (fetch && fetch->status == 0) {
        emscripten_fetch_close(fetch);
        return NULL;
    }
    return fetch;
}

static bool PlatformWebSucceeded(const emscripten_fetch_t *fetch) {
    return fetch->status >= 200 && fetch->status < 300;
}

bool PlatformHttpSend(const PlatformHttpRequest *request, PlatformHttpResponse *response) {
    *response = (PlatformHttpResponse){0};
    emscripten_fetch_t *fetch = PlatformWebFetch(request);
    bool answered = fetch != NULL;
    if (answered) {
        response->status = fetch->status;
        response->body = malloc((size_t)fetch->numBytes + 1);
        if (response->body) {
            if (fetch->numBytes > 0) memcpy(response->body, fetch->data, (size_t)fetch->numBytes);
            response->body[fetch->numBytes] = '\0';
        }
        PlatformWebReplyHeader(
            fetch, request->reply_header, response->reply_header_value, sizeof(response->reply_header_value)
        );
    }
    if (fetch) emscripten_fetch_close(fetch);
    return answered && response->body;
}

PlatformDownloadResult
PlatformDownload(const char *url, const char *path, bool resume, PlatformDownloadProgress progress, void *context) {
    (void)resume;
    PlatformHttpRequest request = {
        .method = PLATFORM_HTTP_GET,
        .url = url,
        .timeout_seconds = PLATFORM_WEB_DOWNLOAD_TIMEOUT_SECONDS,
    };
    emscripten_fetch_t *fetch = PlatformWebFetch(&request);
    if (!fetch) return PLATFORM_DOWNLOAD_NETWORK_ERROR;
    PlatformDownloadResult result = PlatformWebSucceeded(fetch) ? PLATFORM_DOWNLOAD_OK : PLATFORM_DOWNLOAD_SERVER_ERROR;
    size_t length = (size_t)fetch->numBytes;
    FILE *file = result == PLATFORM_DOWNLOAD_OK ? fopen(path, "wb") : NULL;
    if (result == PLATFORM_DOWNLOAD_OK && (!file || fwrite(fetch->data, 1, length, file) != length))
        result = PlatformDiskFull() ? PLATFORM_DOWNLOAD_DISK_FULL : PLATFORM_DOWNLOAD_FILE_ERROR;
    if (file && fclose(file) != 0 && result == PLATFORM_DOWNLOAD_OK) result = PLATFORM_DOWNLOAD_FILE_ERROR;
    if (result == PLATFORM_DOWNLOAD_OK && progress && !progress(context, (long long)length))
        result = PLATFORM_DOWNLOAD_CANCELLED;
    emscripten_fetch_close(fetch);
    return result;
}

typedef enum {
    PLATFORM_WEB_FILE_MISSING,
    PLATFORM_WEB_FILE_LOADING,
    PLATFORM_WEB_FILE_READY,
} PlatformWebFileState;

typedef struct {
    const char *path;
    const char *location;
    long long size;
    int state;
    int piecemeal_reads;
    long long piecemeal_bytes;
} PlatformWebGameFile;

typedef struct {
    char files_url[PLATFORM_WEB_URL_CAP];
    char *list;
    PlatformWebGameFile *files;
    int count;
} PlatformWebGameFiles;

static PlatformWebGameFiles platform_web_game_files;
static unsigned int platform_web_downloaded_bytes;

EM_JS_DEPS(platform_web_downloads, "malloc");

// clang-format off
EM_JS(void, PlatformWebShareDownloadCount, (unsigned int *count), {
    Module.hodDownloadedBytes = function () { return Atomics.load(HEAPU32, count >> 2); };
});

EM_ASYNC_JS(int, PlatformWebDownloadYielding, (const char *url, const char *range, unsigned char **data, int *size), {
    try {
        var headers = range ? { Range: UTF8ToString(range) } : {};
        var response = await fetch(UTF8ToString(url), { headers: headers });
        if (!response.ok) return response.status;
        var bytes = new Uint8Array(await response.arrayBuffer());
        var pointer = _malloc(bytes.length || 1);
        HEAPU8.set(bytes, pointer);
        HEAPU32[data >> 2] = pointer;
        HEAP32[size >> 2] = bytes.length;
        return response.status;
    } catch (failure) {
        return 0;
    }
});

EM_JS(int, PlatformWebDownloadBlocking, (const char *url, const char *range, unsigned char **data, int *size), {
    var request = new XMLHttpRequest();
    request.open("GET", UTF8ToString(url), false);
    request.responseType = "arraybuffer";
    if (range) request.setRequestHeader("Range", UTF8ToString(range));
    try {
        request.send(null);
    } catch (failure) {
        return 0;
    }
    if (request.status < 200 || request.status >= 300) return request.status;
    var bytes = new Uint8Array(request.response);
    var pointer = _malloc(bytes.length || 1);
    HEAPU8.set(bytes, pointer);
    HEAPU32[data >> 2] = pointer;
    HEAP32[size >> 2] = bytes.length;
    return request.status;
});
// clang-format on

static int PlatformWebDownload(const char *url, const char *range, unsigned char **data, int *size) {
    int status = emscripten_is_main_browser_thread() ? PlatformWebDownloadYielding(url, range, data, size)
                                                     : PlatformWebDownloadBlocking(url, range, data, size);
    if (status >= 200 && status < 300)
        __atomic_fetch_add(&platform_web_downloaded_bytes, (unsigned int)*size, __ATOMIC_RELAXED);
    return status;
}

static bool PlatformWebMakeParents(const char *path, char *made, size_t capacity) {
    const char *slash = strrchr(path, '/');
    if (!slash || slash == path) return true;
    size_t parent_length = (size_t)(slash - path);
    if (parent_length >= capacity) return false;
    if (strlen(made) == parent_length && strncmp(made, path, parent_length) == 0) return true;
    char parent[PLATFORM_WEB_PATH_CAP];
    memcpy(parent, path, parent_length);
    parent[parent_length] = '\0';
    for (char *cursor = parent + 1; *cursor; cursor++) {
        if (*cursor != '/') continue;
        *cursor = '\0';
        if (mkdir(parent, PLATFORM_WEB_DIRECTORY_MODE) != 0 && errno != EEXIST) return false;
        *cursor = '/';
    }
    if (mkdir(parent, PLATFORM_WEB_DIRECTORY_MODE) != 0 && errno != EEXIST) return false;
    snprintf(made, capacity, "%s", parent);
    return true;
}

static bool PlatformWebAbsolutePath(const char *relative, char *out, size_t capacity) {
    return (size_t)snprintf(out, capacity, "%s%s", PLATFORM_WEB_ROOT, relative) < capacity;
}

static bool PlatformWebCreatePlaceholder(const char *relative, char *made, size_t capacity) {
    char path[PLATFORM_WEB_PATH_CAP];
    if (!PlatformWebAbsolutePath(relative, path, sizeof(path)) || !PlatformWebMakeParents(path, made, capacity))
        return false;
    int descriptor = open(path, O_CREAT | O_WRONLY, PLATFORM_WEB_FILE_MODE);
    if (descriptor < 0) return false;
    close(descriptor);
    return true;
}

static int PlatformWebCompareGameFiles(const void *a, const void *b) {
    return strcmp(((const PlatformWebGameFile *)a)->path, ((const PlatformWebGameFile *)b)->path);
}

static int PlatformWebFindGameFile(const void *relative, const void *file) {
    return strcmp(relative, ((const PlatformWebGameFile *)file)->path);
}

static char *PlatformWebNextListField(char *field) {
    char *separator = strstr(field, PLATFORM_WEB_LIST_SEPARATOR);
    if (!separator) return NULL;
    *separator = '\0';
    return separator + strlen(PLATFORM_WEB_LIST_SEPARATOR);
}

static long long PlatformWebListedSize(char **rest) {
    char *end = *rest;
    while (isdigit((unsigned char)*end)) end++;
    size_t separator = strlen(PLATFORM_WEB_LIST_SEPARATOR);
    if (end == *rest || strncmp(end, PLATFORM_WEB_LIST_SEPARATOR, separator) != 0) return PLATFORM_WEB_SIZE_UNKNOWN;
    long long size = strtoll(*rest, NULL, 10);
    *rest = end + separator;
    return size;
}

static bool PlatformWebListGameFiles(PlatformWebGameFiles *game, char *list) {
    int capacity = 1;
    for (const char *cursor = list; *cursor; cursor++) capacity += *cursor == '\n';
    game->list = list;
    game->files = calloc((size_t)capacity, sizeof(*game->files));
    if (!game->files) return false;
    char made[PLATFORM_WEB_PATH_CAP] = "";
    for (char *line = strtok(list, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        if (!line[0]) continue;
        char *path = PlatformWebNextListField(line);
        if (!path) {
            fprintf(stderr, "Platform: %s names no location for %s\n", PLATFORM_WEB_FILE_LIST, line);
            return false;
        }
        long long size = PlatformWebListedSize(&path);
        if (!PlatformWebCreatePlaceholder(path, made, sizeof(made))) {
            fprintf(stderr, "Platform: could not expose %s\n", path);
            return false;
        }
        game->files[game->count++] = (PlatformWebGameFile){.path = path, .location = line, .size = size};
    }
    qsort(game->files, (size_t)game->count, sizeof(*game->files), PlatformWebCompareGameFiles);
    return game->count > 0;
}

static bool PlatformWebRelativePath(const char *path, char *out, size_t capacity) {
    char joined[PLATFORM_WEB_PATH_CAP];
    if (path[0] == '/') {
        if ((size_t)snprintf(joined, sizeof(joined), "%s", path) >= sizeof(joined)) return false;
    } else {
        char directory[PLATFORM_WEB_PATH_CAP];
        if (!getcwd(directory, sizeof(directory))) return false;
        if ((size_t)snprintf(joined, sizeof(joined), "%s/%s", directory, path) >= sizeof(joined)) return false;
    }
    size_t length = 0;
    for (const char *segment = joined; *segment;) {
        const char *slash = strchr(segment, '/');
        size_t size = slash ? (size_t)(slash - segment) : strlen(segment);
        bool current = size == 1 && segment[0] == '.';
        bool parent = size == 2 && segment[0] == '.' && segment[1] == '.';
        if (parent) {
            while (length > 0 && out[length - 1] != '/') length--;
            if (length > 0) length--;
        } else if (size > 0 && !current) {
            if (length + size + 2 > capacity) return false;
            if (length > 0) out[length++] = '/';
            memcpy(out + length, segment, size);
            length += size;
        }
        segment += size;
        if (*segment == '/') segment++;
    }
    out[length] = '\0';
    return length > 0;
}

static PlatformWebGameFile *PlatformWebGameFileAt(const char *path) {
    PlatformWebGameFiles *game = &platform_web_game_files;
    char relative[PLATFORM_WEB_PATH_CAP];
    if (game->count == 0 || !PlatformWebRelativePath(path, relative, sizeof(relative))) return NULL;
    return bsearch(relative, game->files, (size_t)game->count, sizeof(*game->files), PlatformWebFindGameFile);
}

static bool PlatformWebStoreGameFile(const char *relative, const unsigned char *data, int size) {
    char path[PLATFORM_WEB_PATH_CAP], staged[PLATFORM_WEB_PATH_CAP];
    if (!PlatformWebAbsolutePath(relative, path, sizeof(path)) ||
        (size_t)snprintf(staged, sizeof(staged), "%s%s", path, PLATFORM_WEB_STAGED_SUFFIX) >= sizeof(staged))
        return false;
    FILE *file = fopen(staged, "wb");
    if (!file) return false;
    bool written = size == 0 || fwrite(data, 1, (size_t)size, file) == (size_t)size;
    written = fclose(file) == 0 && written;
    return written && rename(staged, path) == 0;
}

static bool PlatformWebGameFileUrl(const PlatformWebGameFile *file, char *url, size_t capacity) {
    const PlatformWebGameFiles *game = &platform_web_game_files;
    return (size_t)snprintf(url, capacity, "%s%s", game->files_url, file->location) < capacity;
}

static bool PlatformWebLoadGameFile(const PlatformWebGameFile *file) {
    char url[PLATFORM_WEB_URL_CAP];
    if (!PlatformWebGameFileUrl(file, url, sizeof(url))) return false;
    unsigned char *data = NULL;
    int size = 0;
    int status = PlatformWebDownload(url, NULL, &data, &size);
    bool loaded = status >= 200 && status < 300 && PlatformWebStoreGameFile(file->path, data, size);
    free(data);
    if (!loaded) fprintf(stderr, "Platform: could not download %s from %s (HTTP %d)\n", file->path, url, status);
    return loaded;
}

static void PlatformWebAwaitOtherDownload(void) {
    if (emscripten_is_main_browser_thread())
        emscripten_sleep(PLATFORM_WEB_JOIN_POLL_MILLISECONDS);
    else
        usleep(PLATFORM_WEB_JOIN_POLL_MILLISECONDS * 1000);
}

static bool PlatformWebGameFileReady(PlatformWebGameFile *file) {
    return __atomic_load_n(&file->state, __ATOMIC_ACQUIRE) == PLATFORM_WEB_FILE_READY;
}

static bool PlatformWebClaimGameFile(PlatformWebGameFile *file) {
    int missing = PLATFORM_WEB_FILE_MISSING;
    return __atomic_compare_exchange_n(
        &file->state, &missing, PLATFORM_WEB_FILE_LOADING, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
    );
}

static void PlatformWebSettleGameFile(PlatformWebGameFile *file, bool stored) {
    __atomic_store_n(&file->state, stored ? PLATFORM_WEB_FILE_READY : PLATFORM_WEB_FILE_MISSING, __ATOMIC_RELEASE);
}

static bool PlatformWebLoadGameFileOnce(PlatformWebGameFile *file) {
    while (!PlatformWebGameFileReady(file)) {
        if (!PlatformWebClaimGameFile(file)) {
            PlatformWebAwaitOtherDownload();
            continue;
        }
        bool loaded = PlatformWebLoadGameFile(file);
        PlatformWebSettleGameFile(file, loaded);
        return loaded;
    }
    return true;
}

static bool PlatformWebEnsureGameFile(const char *path) {
    PlatformWebGameFile *file = PlatformWebGameFileAt(path);
    return !file || PlatformWebLoadGameFileOnce(file);
}

static bool PlatformWebPrefersWholeFile(PlatformWebGameFile *file, long long span_bytes) {
    bool read_often =
        __atomic_load_n(&file->piecemeal_reads, __ATOMIC_RELAXED) >= PLATFORM_WEB_PIECEMEAL_READS_BEFORE_WHOLE;
    if (file->size == PLATFORM_WEB_SIZE_UNKNOWN) return read_often;
    long long piecemeal = __atomic_load_n(&file->piecemeal_bytes, __ATOMIC_RELAXED);
    return file->size <= PLATFORM_WEB_WHOLE_FILE_BYTES ||
           span_bytes * PLATFORM_PERCENT >= file->size * PLATFORM_WEB_WHOLE_FILE_PERCENT ||
           (read_often && piecemeal * PLATFORM_PERCENT < file->size * PLATFORM_WEB_PIECEMEAL_SHARE_PERCENT);
}

static void PlatformWebKeepWholeFile(PlatformWebGameFile *file, const unsigned char *data, int size) {
    if (PlatformWebClaimGameFile(file))
        PlatformWebSettleGameFile(file, PlatformWebStoreGameFile(file->path, data, size));
}

static void
PlatformWebFetchSpan(PlatformWebGameFile *file, PlatformByteSpan span, PlatformFileRange *ranges, int count) {
    char url[PLATFORM_WEB_URL_CAP], range[PLATFORM_WEB_RANGE_HEADER_CAP];
    if (!PlatformWebGameFileUrl(file, url, sizeof(url))) return;
    snprintf(range, sizeof(range), "bytes=%lld-%lld", span.start, span.end - 1);
    unsigned char *data = NULL;
    int size = 0;
    int status = PlatformWebDownload(url, range, &data, &size);
    bool whole = status == PLATFORM_HTTP_OK, partial = status == PLATFORM_HTTP_PARTIAL_CONTENT;
    if (whole) PlatformWebKeepWholeFile(file, data, size);
    long long covered_start = whole ? 0 : span.start;
    long long covered_end = whole || partial ? covered_start + size : covered_start;
    for (int i = 0; i < count; i++) {
        PlatformFileRange *wanted = &ranges[i];
        if (wanted->read || wanted->offset < covered_start || wanted->offset + (long long)wanted->size > covered_end)
            continue;
        memcpy(wanted->out, data + (wanted->offset - covered_start), wanted->size);
        wanted->read = true;
    }
    free(data);
    if (!whole && !partial)
        fprintf(stderr, "Platform: could not read %s %s from %s (HTTP %d)\n", file->path, range, url, status);
}

bool PlatformReadFileRanges(const char *path, PlatformFileRange *ranges, int count) {
    PlatformWebGameFile *file = PlatformWebGameFileAt(path);
    if (!file || PlatformWebGameFileReady(file)) return PlatformReadLocalRanges(path, ranges, count);
    if (count == 0) return true;
    PlatformByteSpan *spans = malloc(sizeof(*spans) * (size_t)count);
    if (!spans) return false;
    int span_count = PlatformCoalesceRanges(ranges, count, PLATFORM_WEB_RANGE_GAP_BYTES, spans);
    long long span_bytes = 0;
    for (int i = 0; i < span_count; i++) span_bytes += spans[i].end - spans[i].start;
    bool whole = PlatformWebPrefersWholeFile(file, span_bytes);
    if (!whole) {
        __atomic_fetch_add(&file->piecemeal_reads, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&file->piecemeal_bytes, span_bytes, __ATOMIC_RELAXED);
    }
    for (int i = 0; i < count; i++) ranges[i].read = false;
    for (int i = 0; !whole && i < span_count && !PlatformWebGameFileReady(file); i++)
        PlatformWebFetchSpan(file, spans[i], ranges, count);
    free(spans);
    if (whole || PlatformWebGameFileReady(file))
        return PlatformWebLoadGameFileOnce(file) && PlatformReadLocalRanges(path, ranges, count);
    bool all_read = true;
    for (int i = 0; i < count; i++) all_read = all_read && ranges[i].read;
    return all_read;
}

FILE *__real_fopen(const char *path, const char *mode);

FILE *__wrap_fopen(const char *path, const char *mode) {
    if (mode[0] == 'r' && !PlatformWebEnsureGameFile(path)) {
        errno = EIO;
        return NULL;
    }
    return __real_fopen(path, mode);
}

backend_t wasmfs_create_root_dir(void) {
    return PlatformWebRunsInNode() ? wasmfs_create_node_backend(PLATFORM_WEB_NODE_ROOT)
                                   : wasmfs_create_memory_backend();
}

static bool platform_web_saves_mounted;

static bool PlatformWebCopyFile(const char *source, const char *target) {
    FILE *from = fopen(source, "rb");
    FILE *to = from ? fopen(target, "wb") : NULL;
    bool copied = from && to;
    char buffer[PLATFORM_WEB_COPY_BYTES];
    for (size_t read = copied ? fread(buffer, 1, sizeof(buffer), from) : 0; copied && read > 0;
         read = fread(buffer, 1, sizeof(buffer), from))
        copied = fwrite(buffer, 1, read, to) == read;
    copied = copied && !ferror(from);
    if (to && fclose(to) != 0) copied = false;
    if (from) fclose(from);
    return copied;
}

static bool PlatformWebSavedPath(const char *path, char *out, size_t capacity) {
    if (!platform_web_saves_mounted || !path[0] || strchr(path, '/')) return false;
    return (size_t)snprintf(out, capacity, "%s/%s", PLATFORM_WEB_SAVES_DIR, path) < capacity;
}

typedef struct {
    char path[PLATFORM_WEB_PATH_CAP];
    bool removal;
} PlatformWebSaveChange;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t wake;
    PlatformWebSaveChange changes[PLATFORM_WEB_SAVE_QUEUE_CAP];
    int count;
    bool keeper_started;
} PlatformWebSaveQueue;

static PlatformWebSaveQueue platform_web_save_queue = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .wake = PTHREAD_COND_INITIALIZER,
};

static void PlatformWebApplySaveChange(const PlatformWebSaveChange *change) {
    char saved[PLATFORM_WEB_PATH_CAP], staged[PLATFORM_WEB_PATH_CAP];
    if (!PlatformWebSavedPath(change->path, saved, sizeof(saved))) return;
    if (change->removal) {
        remove(saved);
        return;
    }
    bool kept = (size_t)snprintf(staged, sizeof(staged), "%s%s", saved, PLATFORM_WEB_STAGED_SUFFIX) < sizeof(staged) &&
                PlatformWebCopyFile(change->path, staged) && rename(staged, saved) == 0;
    if (!kept) fprintf(stderr, "Platform: could not keep %s in browser storage\n", change->path);
}

static void *PlatformWebKeepSaves(void *argument) {
    PlatformWebSaveQueue *queue = argument;
    for (;;) {
        pthread_mutex_lock(&queue->lock);
        while (queue->count == 0) pthread_cond_wait(&queue->wake, &queue->lock);
        PlatformWebSaveChange change = queue->changes[0];
        queue->count--;
        memmove(queue->changes, queue->changes + 1, sizeof(*queue->changes) * (size_t)queue->count);
        pthread_mutex_unlock(&queue->lock);
        PlatformWebApplySaveChange(&change);
    }
    return NULL;
}

static bool PlatformWebSaveQueued(const PlatformWebSaveQueue *queue, const PlatformWebSaveChange *change) {
    for (int i = 0; i < queue->count; i++)
        if (queue->changes[i].removal == change->removal && strcmp(queue->changes[i].path, change->path) == 0)
            return true;
    return false;
}

static void PlatformWebQueueSaveChange(const char *path, bool removal) {
    char saved[PLATFORM_WEB_PATH_CAP];
    if (!PlatformWebSavedPath(path, saved, sizeof(saved))) return;
    PlatformWebSaveChange change = {.removal = removal};
    snprintf(change.path, sizeof(change.path), "%s", path);
    PlatformWebSaveQueue *queue = &platform_web_save_queue;
    pthread_mutex_lock(&queue->lock);
    if (!queue->keeper_started) {
        pthread_t keeper;
        queue->keeper_started = pthread_create(&keeper, NULL, PlatformWebKeepSaves, queue) == 0;
        if (queue->keeper_started) pthread_detach(keeper);
    }
    if (queue->keeper_started && !PlatformWebSaveQueued(queue, &change) && queue->count < PLATFORM_WEB_SAVE_QUEUE_CAP)
        queue->changes[queue->count++] = change;
    else if (!queue->keeper_started || queue->count == PLATFORM_WEB_SAVE_QUEUE_CAP)
        fprintf(stderr, "Platform: could not queue %s for browser storage\n", path);
    pthread_cond_signal(&queue->wake);
    pthread_mutex_unlock(&queue->lock);
}

static bool PlatformWebStagedName(const char *name) {
    size_t length = strlen(name), suffix = strlen(PLATFORM_WEB_STAGED_SUFFIX);
    return length >= suffix && strcmp(name + length - suffix, PLATFORM_WEB_STAGED_SUFFIX) == 0;
}

static int PlatformWebRestoreSaves(void) {
    DIR *directory = opendir(PLATFORM_WEB_SAVES_DIR);
    if (!directory) return 0;
    int restored = 0;
    for (struct dirent *entry = readdir(directory); entry; entry = readdir(directory)) {
        if (entry->d_type != DT_REG || PlatformWebStagedName(entry->d_name)) continue;
        char saved[PLATFORM_WEB_PATH_CAP];
        if ((size_t)snprintf(saved, sizeof(saved), "%s/%s", PLATFORM_WEB_SAVES_DIR, entry->d_name) >= sizeof(saved))
            continue;
        char restored_path[PLATFORM_WEB_PATH_CAP];
        if ((size_t)snprintf(restored_path, sizeof(restored_path), "%s%s", PLATFORM_WEB_ROOT, entry->d_name) >=
            sizeof(restored_path))
            continue;
        restored += PlatformWebCopyFile(saved, restored_path);
    }
    closedir(directory);
    return restored;
}

static void PlatformWebMountSaves(PlatformWebFiles *files) {
    if (!PlatformWebHasOpfs()) return;
    backend_t saves = wasmfs_create_opfs_backend();
    if (!saves || wasmfs_create_directory(PLATFORM_WEB_SAVES_DIR, PLATFORM_WEB_DIRECTORY_MODE, saves) != 0) return;
    platform_web_saves_mounted = true;
    files->restored = PlatformWebRestoreSaves();
}

static void *PlatformWebPrepare(void *argument) {
    PlatformWebFiles *files = argument;
    PlatformWebMountSaves(files);
    char url[PLATFORM_WEB_URL_CAP];
    snprintf(url, sizeof(url), "%s%s", files->files_url, PLATFORM_WEB_FILE_LIST);
    PlatformHttpRequest request = {
        .method = PLATFORM_HTTP_GET,
        .url = url,
        .timeout_seconds = PLATFORM_WEB_LIST_TIMEOUT_SECONDS,
    };
    PlatformHttpResponse response;
    if (PlatformHttpSend(&request, &response)) {
        snprintf(platform_web_game_files.files_url, sizeof(platform_web_game_files.files_url), "%s", files->files_url);
        files->prepared = response.status == 200 && PlatformWebListGameFiles(&platform_web_game_files, response.body);
        if (!files->prepared) free(response.body);
    }
    __atomic_store_n(&files->finished, 1, __ATOMIC_RELEASE);
    return NULL;
}

static bool PlatformWebFilesUrl(char *out, size_t capacity) {
    const char *chosen = getenv(PLATFORM_WEB_FILES_URL_ENV);
    const char *configured = chosen && chosen[0] ? chosen : HOD_GAME_FILES_URL;
    if (!configured[0]) return PlatformWebPageUrl(out, (int)capacity);
    const char *separator = configured[strlen(configured) - 1] == '/' ? "" : "/";
    return (size_t)snprintf(out, capacity, "%s%s", configured, separator) < capacity;
}

void PlatformJoinThread(pthread_t thread) {
    if (!emscripten_is_main_browser_thread()) {
        pthread_join(thread, NULL);
        return;
    }
    while (pthread_tryjoin_np(thread, NULL) == EBUSY) emscripten_sleep(PLATFORM_WEB_JOIN_POLL_MILLISECONDS);
}

bool PlatformPrepareGameFiles(void) {
    static PlatformWebFiles files;
    if (!PlatformWebFilesUrl(files.files_url, sizeof(files.files_url))) return false;
    PlatformWebShareDownloadCount(&platform_web_downloaded_bytes);
    pthread_t thread;
    if (pthread_create(&thread, NULL, PlatformWebPrepare, &files) != 0) return false;
    while (!__atomic_load_n(&files.finished, __ATOMIC_ACQUIRE))
        emscripten_sleep(PLATFORM_WEB_PREPARE_POLL_MILLISECONDS);
    pthread_join(thread, NULL);
    if (!files.prepared) {
        fprintf(stderr, "Platform: could not prepare the game files from %s\n", files.files_url);
        return false;
    }
    printf(
        "Platform: exposed %d game files listed in %s%s, restored %d saved files\n",
        platform_web_game_files.count,
        files.files_url,
        PLATFORM_WEB_FILE_LIST,
        files.restored
    );
    return chdir(PLATFORM_WEB_ROOT) == 0;
}

#else
bool PlatformPrepareGameFiles(void) {
    return true;
}

void PlatformJoinThread(pthread_t thread) {
    pthread_join(thread, NULL);
}

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
#endif

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
#elif !defined(__EMSCRIPTEN__)
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

#ifdef __EMSCRIPTEN__
int PlatformFallbackFonts(char paths[][PLATFORM_FONT_PATH_CAP], int capacity) {
    (void)paths;
    (void)capacity;
    return 0;
}
#else
int PlatformFallbackFonts(char paths[][PLATFORM_FONT_PATH_CAP], int capacity) {
    int count = 0;
    for (size_t i = 0; i < sizeof(platform_fallback_fonts) / sizeof(platform_fallback_fonts[0]) && count < capacity;
         i++)
        snprintf(paths[count++], PLATFORM_FONT_PATH_CAP, "%s", platform_fallback_fonts[i]);
    return count;
}
#endif

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

#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>

#define PLATFORM_COCOA_FULLSCREEN_STYLE_MASK (1ul << 14)

bool PlatformNativeFullscreen(void *window) {
    unsigned long style = ((unsigned long (*)(id, SEL))objc_msgSend)((id)window, sel_registerName("styleMask"));
    return (style & PLATFORM_COCOA_FULLSCREEN_STYLE_MASK) != 0;
}

void PlatformToggleNativeFullscreen(void *window) {
    ((void (*)(id, SEL, id))objc_msgSend)((id)window, sel_registerName("toggleFullScreen:"), nil);
}

void PlatformHideApplication(void) {
    id application =
        ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
    ((void (*)(id, SEL, id))objc_msgSend)(application, sel_registerName("hide:"), nil);
}
#endif

bool PlatformReadAt(FILE *file, long long offset, void *out, size_t size) {
    return pread(fileno(file), out, size, (off_t)offset) == (ssize_t)size;
}

#ifdef __EMSCRIPTEN__
bool PlatformReplaceFile(const char *source, const char *target) {
    if (rename(source, target) != 0) return false;
    PlatformWebQueueSaveChange(target, false);
    return true;
}

bool PlatformRemoveFile(const char *path) {
    PlatformWebQueueSaveChange(path, true);
    return remove(path) == 0;
}
#else
bool PlatformReplaceFile(const char *source, const char *target) {
    return rename(source, target) == 0;
}

bool PlatformRemoveFile(const char *path) {
    return remove(path) == 0;
}
#endif

#ifdef __EMSCRIPTEN__
int PlatformProcessorCount(void) {
    int count = emscripten_num_logical_cores();
    return count < 1 ? 1 : count < PLATFORM_WEB_PROCESSOR_CAP ? count : PLATFORM_WEB_PROCESSOR_CAP;
}
#else
int PlatformProcessorCount(void) {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return count > 0 ? (int)count : 1;
}
#endif

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

#ifdef __EMSCRIPTEN__
bool PlatformStartProgram(const char *executable) {
    (void)executable;
    return false;
}

bool PlatformContinueAs(const char *executable) {
    (void)executable;
    return false;
}
#else
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
#endif
