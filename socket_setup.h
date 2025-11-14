#ifndef SOCKET_SETUP_H
#define SOCKET_SETUP_H

#if defined(_WIN32)
    #ifndef _WIN32_WINNT
        #if defined(WINVER) && (WINVER >= 0x0A00)
            #define _WIN32_WINNT 0x0A00   // Windows 10 or later
        #elif defined(WINVER) && (WINVER >= 0x0603)
            #define _WIN32_WINNT 0x0603   // Windows 8.1
        #elif defined(WINVER) && (WINVER >= 0x0602)
            #define _WIN32_WINNT 0x0602   // Windows 8
        #elif defined(WINVER) && (WINVER >= 0x0601)
            #define _WIN32_WINNT 0x0601   // Windows 7
        #elif defined(WINVER) && (WINVER >= 0x0600)
            #define _WIN32_WINNT 0x0600   // Windows Vista
        #elif defined(WINVER) && (WINVER >= 0x0501)
            #define _WIN32_WINNT 0x0501   // Windows XP
        #else
            #define _WIN32_WINNT 0x0A00   // Windows 10 or later
        #endif
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
    #define CLOSE_SOCKET(s)    (::close(s))
    #define GET_SOCKET_ERRNO() (errno)

    #define SOCKET_STARTUP()
    #define SOCKET_CLEANUP()
#endif
#if !defined(IPV6_V6ONLY)
    #define IPV6_V6ONLY 27
#endif

#endif // SOCKET_SETUP_H
