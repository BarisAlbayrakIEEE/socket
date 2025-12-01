// socket_setup.hpp

#ifndef SOCKET_SETUP_HPP
#define SOCKET_SETUP_HPP

#include <stdexcept>

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
    #include <conio.h>
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

    #define ERROR_INTERRUPTED  (WSAEINTR)
    #define ERROR_BLOCKED      (WSAEWOULDBLOCK)

    #define SOCKET_ERROR__POLL() \
        fprintf(stderr, "[WSAPoll() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
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

    #define ERROR_INTERRUPTED  (EINTR)
    #define ERROR_BLOCKED      (EAGAIN)

    #define SOCKET_ERROR__POLL() \
        fprintf(stderr, "[poll() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
#endif
#if !defined(IPV6_V6ONLY)
    #define IPV6_V6ONLY 27
#endif

#define BA_SOCKET_DEBUG
#ifdef BA_SOCKET_DEBUG
    static const bool is_debug_mode = true;
    #define PRINTF8(a, b, c, d, e, f, g, h) printf(a, b, c, d, e, f, g, h)
    #define PRINTF7(a, b, c, d, e, f, g) printf(a, b, c, d, e, f, g)
    #define PRINTF6(a, b, c, d, e, f) printf(a, b, c, d, e, f)
    #define PRINTF5(a, b, c, d, e) printf(a, b, c, d, e)
    #define PRINTF4(a, b, c, d) printf(a, b, c, d)
    #define PRINTF3(a, b, c) printf(a, b, c)
    #define PRINTF2(a, b) printf(a, b)
    #define PRINTF1(a) printf(a)

    #define SOCKET_ERROR__ALLOC() \
        fprintf(stderr, "[malloc() error]\n")
    #if defined(_WIN32)
        #define SOCKET_ERROR__GETADDRS() \
            fprintf(stderr, "[GetAdaptersAddresses() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #else
        #define SOCKET_ERROR__GETADDRS() \
            fprintf(stderr, "[getifaddrs() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #endif
    #define SOCKET_ERROR__GETADDRINFO() \
        fprintf(stderr, "[getaddrinfo() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SOCKET() \
        fprintf(stderr, "[socket() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SETSOCKOPT() \
        fprintf(stderr, "[setsockopt() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__BIND() \
        fprintf(stderr, "[bind() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__LISTEN() \
        fprintf(stderr, "[listen() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__ACCEPT() \
        fprintf(stderr, "[accept() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__RECV() \
        fprintf(stderr, "[recv() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SEND() \
        fprintf(stderr, "[send() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__CONNECT() \
        fprintf(stderr, "[connect() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SELECT() \
        fprintf(stderr, "[select() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
#else
    static const bool is_debug_mode = false;
    #define PRINTF8(a, b, c, d, e, f, g, h)
    #define PRINTF7(a, b, c, d, e, f, g)
    #define PRINTF6(a, b, c, d, e, f)
    #define PRINTF5(a, b, c, d, e)
    #define PRINTF4(a, b, c, d)
    #define PRINTF3(a, b, c)
    #define PRINTF2(a, b)
    #define PRINTF1(a)

    #define SOCKET_ERROR__ALLOC() \
        throw std::runtime_error(std::string("[malloc() error] "))
    #if defined(_WIN32)
        #define SOCKET_ERROR__GETADDRS() \
            throw std::runtime_error(std::string("[GetAdaptersAddresses() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #else
        #define SOCKET_ERROR__GETADDRS() \
            throw std::runtime_error(std::string("[getifaddrs() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #endif
    #define SOCKET_ERROR__GETADDRINFO() \
        throw std::runtime_error(std::string("[getaddrinfo() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__SOCKET() \
        throw std::runtime_error(std::string("[socket() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__SETSOCKOPT() \
        throw std::runtime_error(std::string("[setsockopt() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__BIND() \
        throw std::runtime_error(std::string("[bind() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__LISTEN() \
        throw std::runtime_error(std::string("[listen() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__ACCEPT() \
        throw std::runtime_error(std::string("[accept() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__RECV() \
        throw std::runtime_error(std::string("[recv() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__SEND() \
        throw std::runtime_error(std::string("[send() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__CONNECT() \
        throw std::runtime_error(std::string("[connect() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__SELECT() \
        throw std::runtime_error(std::string("[select() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
#endif

// separate reads and writes:
#define SEPARATE_READ_WRITE

#endif // SOCKET_SETUP_HPP
