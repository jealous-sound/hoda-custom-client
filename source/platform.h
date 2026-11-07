#ifndef HOD_PLATFORM_H
#define HOD_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define PLATFORM_HTTP_HEADER_VALUE_CAP 64

typedef enum {
    PLATFORM_DOWNLOAD_OK,
    PLATFORM_DOWNLOAD_UNAVAILABLE,
    PLATFORM_DOWNLOAD_NETWORK_ERROR,
    PLATFORM_DOWNLOAD_SERVER_ERROR,
    PLATFORM_DOWNLOAD_FILE_ERROR,
    PLATFORM_DOWNLOAD_CANCELLED,
} PlatformDownloadResult;

typedef bool (*PlatformDownloadProgress)(void *context, long long file_bytes);

typedef struct {
    const char *method;
    const char *url;
    const char *const *headers;
    int header_count;
    const char *body;
    int timeout_seconds;
    const char *reply_header;
} PlatformHttpRequest;

typedef struct {
    int status;
    char *body;
    char reply_header_value[PLATFORM_HTTP_HEADER_VALUE_CAP];
} PlatformHttpResponse;

bool PlatformHttpSend(const PlatformHttpRequest *request, PlatformHttpResponse *response);
bool PlatformReadAt(FILE *file, long long offset, void *out, size_t size);
bool PlatformReplaceFile(const char *source, const char *target);
int PlatformProcessorCount(void);
time_t PlatformTimeFromUtc(struct tm *utc);
PlatformDownloadResult PlatformDownload(
    const char *url, const char *path, bool resume, PlatformDownloadProgress progress, void *context
);
bool PlatformUserDataDirectory(const char *app_name, char *out, size_t capacity);
bool PlatformMakeDirectory(const char *path);

#endif
