#include "platform.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>

#define PLATFORM_WINSOCK_VERSION MAKEWORD(2, 2)
#define PLATFORM_MILLISECONDS_PER_SECOND 1000

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

#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

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
