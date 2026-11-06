#ifndef HOD_PLATFORM_H
#define HOD_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define PLATFORM_SOCKET_INVALID ((intptr_t)-1)

intptr_t PlatformSocketConnect(const char *host, const char *port, int timeout_seconds);
long PlatformSocketSend(intptr_t socket_handle, const char *data, size_t length);
long PlatformSocketReceive(intptr_t socket_handle, char *buffer, size_t capacity);
void PlatformSocketClose(intptr_t socket_handle);
bool PlatformReadAt(FILE *file, long long offset, void *out, size_t size);
bool PlatformReplaceFile(const char *source, const char *target);
int PlatformProcessorCount(void);
time_t PlatformTimeFromUtc(struct tm *utc);

#endif
