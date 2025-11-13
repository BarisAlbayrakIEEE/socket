#if defined(_WIN32)
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    #define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSE_SOCKET(s)    (closesocket(s))
    #define GET_SOCKET_ERRNO() (WSAGetLastError())

    #define SOCKET_STARTUP()                                \
        do {                                                \
            WSADATA d;                                      \
            if (WSAStartup(MAKEWORD(2, 2), &d)) {           \
                fprintf(stderr, "Failed to initialize.\n"); \
                exit(1);                                    \
            }                                               \
        } while (0)
    #define SOCKET_CLEANUP()   (WSACleanup())
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>

    typedef int SOCKET;
    #define INVALID_SOCKET     (-1)
    #define SOCKET_ERROR       (-1)
    #define IS_VALID_SOCKET(s) ((s) >= 0)
    #define CLOSE_SOCKET(s)    (close(s))
    #define GET_SOCKET_ERRNO() (errno)

    #define SOCKET_STARTUP()
    #define SOCKET_CLEANUP()
#endif

#include <stdio.h>

int main() {
    SOCKET_STARTUP();
    printf("Ready to use socket API.\n");
    SOCKET_CLEANUP();
    return 0;
}
