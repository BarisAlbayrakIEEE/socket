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
        #define SOCKET_ERROR__GETADAPTERSADDRESSES() \
            fprintf(stderr, "[GetAdaptersAddresses() error] %s: (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #else
        #define SOCKET_ERROR__GETIFADDRS() \
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
        #define SOCKET_ERROR__GETADAPTERSADDRESSES() \
            throw std::runtime_error(std::string("[GetAdaptersAddresses() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #else
        #define SOCKET_ERROR__GETIFADDRS() \
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

#endif // SOCKET_SETUP_HPP





// utility_addr.hpp

#ifndef UTILITY_ADDR_HPP
#define UTILITY_ADDR_HPP

#include "socket_setup.hpp"
#include <cstring>
#ifdef _WIN32
    #include <iphlpapi.h>
#else
    #include <ifaddrs.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <string>

namespace BA_Socket {
    inline struct addrinfo* get_addrinfo(
        int socktype = SOCK_STREAM,
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_UNSPEC,
        int flags = AI_PASSIVE)
    {
        PRINTF1("Obtaining the address info...\n");
        std::string port_str(std::to_string(port));
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = socktype;
        hints.ai_family = family;
        hints.ai_flags = flags;
        struct addrinfo *socket_addr;
        if (hostname.empty()) {
            if (::getaddrinfo(nullptr, port_str.c_str(), &hints, &socket_addr)) {
                SOCKET_ERROR__GETADDRINFO();
                return nullptr;
            }
        }
        else {
            if (::getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &socket_addr)) {
                SOCKET_ERROR__GETADDRINFO();
                return nullptr;
            }
        }
        return socket_addr;
    }

    template <bool Is_Debug_Mode>
    inline void print_sockaddr(struct sockaddr_storage sockaddr_, socklen_t socklen) {
        // print the peer address
        PRINTF1("Printing the socket address...\n");
        char buffer_addr[256];
        char buffer_service[256];
        ::getnameinfo(
            (struct sockaddr*)&sockaddr_,
            socklen,
            buffer_addr,
            sizeof(buffer_addr),
            buffer_service,
            sizeof(buffer_service),
            NI_NUMERICHOST);
        PRINTF3("%s %s\n", buffer_addr, buffer_service);
    }
    template <>
    inline void print_sockaddr<false>(struct sockaddr_storage sockaddr_, socklen_t socklen) {
        ;
    }

    template <bool Is_Debug_Mode>
    inline void print_addrinfo(struct addrinfo* addrinfo_) {
        // print the peer address
        PRINTF1("Printing the address info...\n");
        char buffer_addr[256];
        char buffer_service[256];
        ::getnameinfo(
            addrinfo_->ai_addr,
            addrinfo_->ai_addrlen,
            buffer_addr,
            sizeof(buffer_addr),
            buffer_service,
            sizeof(buffer_service),
            NI_NUMERICHOST);
        PRINTF3("%s %s\n", buffer_addr, buffer_service);
    }
    template <>
    inline void print_addrinfo<false>(struct addrinfo* addrinfo_) {
        ;
    }

    // A uniform interface for network address information
    typedef struct Uniform_Addr {
        char name[128];
        unsigned int flags;
        struct sockaddr *sock_addr;
        socklen_t addr_len;
    } Uniform_Addr;

#ifdef _WIN32
    // get uniform address - windows
    size_t get_local_addrs_uniform(Uniform_Addr *uniform_addrs, size_t max) {
        DWORD asize = 15000;
        PIP_ADAPTER_ADDRESSES adapter_addrs = NULL;
        while (1) {
            adapter_addrs = (PIP_ADAPTER_ADDRESSES)malloc(asize);
            if (!adapter_addrs) {
                SOCKET_ERROR__ALLOC();
                return 0;
            }

            DWORD result = GetAdaptersAddresses(
                AF_UNSPEC,
                GAA_FLAG_INCLUDE_PREFIX,
                NULL,
                adapter_addrs,
                &asize);
            if (result == ERROR_BUFFER_OVERFLOW) {
                free(adapter_addrs);
                adapter_addrs = NULL;
                continue;
            } else if (result != ERROR_SUCCESS) {
                SOCKET_ERROR__GETADAPTERSADDRESSES();
                free(adapter_addrs);
                return 0;
            }
            break;
        }

        size_t count = 0;
        for (
            PIP_ADAPTER_ADDRESSES it = adapter_addrs;
            it && count < max;
            it = it->Next)
        {
            for (
                PIP_ADAPTER_UNICAST_ADDRESS adapter_unicast_addr = it->FirstUnicastAddress;
                adapter_unicast_addr && count < max;
                adapter_unicast_addr = adapter_unicast_addr->Next)
            {
                Uniform_Addr *uniform_addr = &uniform_addrs[count++];
                memset(uniform_addr, 0, sizeof(*uniform_addr));
                wcstombs(
                    uniform_addr->name,
                    it->FriendlyName,
                    sizeof(uniform_addr->name));
                uniform_addr->flags = it->Flags;
                uniform_addr->sock_addr = adapter_unicast_addr->Address.lpSockaddr;
                uniform_addr->addr_len = adapter_unicast_addr->Address.iSockaddrLength;
            }
        }
        free(adapter_addrs);
        return count;
    }
#else
    // get uniform address - linux/unix
    size_t get_local_addrs_uniform(Uniform_Addr *uniform_addrs, size_t max) {
        PRINTF1("Obtaining all addresses...\n");

        struct ifaddrs *ifaddrs_;
        if (getifaddrs(&ifaddrs_) == -1) {
            SOCKET_ERROR__GETIFADDRS();
            return 0;
        }

        size_t count = 0;
        for (
            struct ifaddrs *it = ifaddrs_;
            it && count < max;
            it = it->ifa_next)
        {
            if (!it->ifa_addr) continue;
            int family = it->ifa_addr->sa_family;
            if (family != AF_INET && family != AF_INET6) continue;

            Uniform_Addr *uniform_addr = &uniform_addrs[count++];
            memset(uniform_addr, 0, sizeof(*uniform_addr));
            strncpy(uniform_addr->name, it->ifa_name, sizeof(uniform_addr->name));
            uniform_addr->flags = it->ifa_flags;
            uniform_addr->sock_addr = it->ifa_addr;
            uniform_addr->addr_len =
                (family == AF_INET) ?
                sizeof(struct sockaddr_in) :
                sizeof(struct sockaddr_in6);
        }
        freeifaddrs(ifaddrs_);
        return count;
    }
#endif

    inline std::string get_ip_of_interface(const std::string& ifname) {
        PRINTF1("Obtaining the address matched by name...\n");

        Uniform_Addr uniform_addrs[100];
        size_t count = get_local_addrs_uniform(uniform_addrs, 100);
        for (size_t i = 0; i < count; ++i) {
            if (ifname == uniform_addrs[i].name) {
                char host[NI_MAXHOST];
                if (
                    getnameinfo(
                        uniform_addrs[i].sock_addr,
                        uniform_addrs[i].addr_len,
                        host,
                        NI_MAXHOST,
                        nullptr,
                        0,
                        NI_NUMERICHOST) == 0)
                {
                    return std::string(host);
                }
            }
        }
        SOCKET_ERROR__GETIFADDRS();
        return std::string();
    }

    inline struct addrinfo* get_local_addr_to_bind(
        const std::string& bind_mode,
        uint16_t port = 8080,
        int family = AF_INET6,
        int socktype = SOCK_STREAM,
        int flags = AI_PASSIVE)
    {
        PRINTF1("Obtaining the local address to bind...\n");

        std::string node;
        if (bind_mode == "all" || bind_mode == "any") {
            node.clear(); // == nullptr
        }
        else if (bind_mode == "localhost") {
            node = "127.0.0.1";
            if (family == AF_INET6) node = "::1";
        }
        else {
            node = get_ip_of_interface(bind_mode); // e.g. "ech0"
            if (node.empty()) return nullptr;
        }

        struct addrinfo hints{};
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = family;
        hints.ai_socktype = socktype;
        hints.ai_flags    = flags;

        // obtain the peer address
        struct addrinfo *bind_addr = get_addrinfo(socktype, node, port, family, flags);
        return bind_addr;
    }
} // namespace BA_Socket

#endif // UTILITY_ADDR_HPP





// Socket.hpp

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "utility_addr.hpp"
#include <stdexcept>
#include <utility>
#include <cstring>
#include <stdio.h>

namespace BA_Socket {
    class Socket {
    public:
        explicit Socket(SOCKET fd) noexcept : _fd(fd) {};
        Socket(int domain, int type, int protocol) {
            _fd = ::socket(domain, type, protocol);
            if (!IS_VALID_SOCKET(_fd)) {
                SOCKET_ERROR__SOCKET();
            }
        }
        ~Socket() { close(); };

        // non-copyable but movable
        Socket(const Socket&) = default;
        Socket& operator=(const Socket&) = default;
        Socket(Socket&& rhs) noexcept : _fd(rhs._fd) {
            rhs._fd = INVALID_SOCKET;
        }
        Socket& operator=(Socket&& rhs) noexcept {
            if (this != &rhs) {
                close();
                _fd = rhs._fd;
                rhs._fd = INVALID_SOCKET;
            }
            return *this;
        };

        // get socket option
        // defaults:
        //   level = SOL_SOCKET
        //   optname = SO_DOMAIN
        inline int get_sockopt(int level = SOL_SOCKET, int optname = SO_DOMAIN) {
            int optval = 0;
            socklen_t optlen = sizeof(optval);
            if (::getsockopt(_fd, level, optname, (void*)&optval, &optlen)) {
                SOCKET_ERROR__SETSOCKOPT();
                return -1;
            }
            return optval;
        }

        // set socket option
        // defaults:
        //   convert IPV6_V6ONLY socket to dual stack.
        //   disable IPV6_V6ONLY to accept both IPv4 and IPv6
        inline bool set_sockopt(int level = IPPROTO_IPV6, int optname = IPV6_V6ONLY, int optval = 0) {
            if (::setsockopt(_fd, level, optname, (void*)&optval, sizeof(optval))) {
                SOCKET_ERROR__SETSOCKOPT();
                return false;
            }
            return true;
        }

        // Bind to local address
        inline bool bind(const struct sockaddr* addr, socklen_t addrlen) {
            if (::bind(_fd, addr, addrlen)) {
                SOCKET_ERROR__BIND();
                return false;
            }
            return true;
        }

        // Create the local bind address for any interface on given port
        bool bind_any(uint16_t port, int family = AF_INET6, int socktype = SOCK_STREAM) {
            // get the local address to bind to
            char port_str[8];
            snprintf(port_str, sizeof(port_str), "%u", port);

            struct addrinfo hints{};
            hints.ai_family = family;
            hints.ai_socktype = socktype;
            hints.ai_flags = AI_PASSIVE;
            struct addrinfo* bind_addr = nullptr;
            int status = ::getaddrinfo(nullptr, port_str, &hints, &bind_addr);
            if (status != 0) {
                SOCKET_ERROR__GETADDRINFO();
                return false;
            }

            // bind the socket to the local address
            if (::bind(_fd, bind_addr->ai_addr, bind_addr->ai_addrlen) < 0) {
                ::freeaddrinfo(bind_addr);
                SOCKET_ERROR__BIND();
                return false;
            }

            // free the address info
            ::freeaddrinfo(bind_addr);
            return true;
        }

        // Listen for connections
        inline bool listen(int backlog = 10) {
            if (::listen(_fd, backlog) < 0) {
                SOCKET_ERROR__LISTEN();
                return false;
            }
            return true;
        }

        // Accept a new connection (returns a new RAII socket)
        // inspect the returned Socket with is_valid() before use
        inline Socket accept(struct sockaddr* addr = nullptr, socklen_t* addrlen = nullptr) {
            SOCKET client_fd = ::accept(_fd, addr, addrlen);
            if (!IS_VALID_SOCKET(client_fd)) {
                SOCKET_ERROR__ACCEPT();
                client_fd = -1;
            }
            return Socket(client_fd);
        }

        // Send data
        inline ssize_t send(const void* buffer, size_t length, int flags = 0) {
            return ::send(_fd, (const char*)buffer, static_cast<int>(length), flags);
        }

        // Safe send
        void send_all(const void* buffer, size_t length, int flags = 0) {
            size_t total_sent = 0;
            const char* buf = static_cast<const char*>(buffer);
            while (total_sent < length) {
                ssize_t sent = ::send(
                    _fd, buf + total_sent,
                    static_cast<int>(length - total_sent),
                    flags);
                if (sent < 0) {
                    int errno_ = GET_SOCKET_ERRNO();
                    if (errno_ == EINTR) continue; // Interrupted -> retry
                    if (errno_ == EAGAIN || errno_ == EWOULDBLOCK) {
                        continue; // TODO: can wait with poll/select if needed
                    }
                    SOCKET_ERROR__SEND();
                    return;
                }
                if (sent == 0) break; // shouldn't happen unless socket closed
                total_sent += sent;
            }
        }

        // Receive data
        // inspect the return value for number of bytes received
        inline ssize_t recv(void* buffer, size_t length, int flags = 0) {
            ssize_t bytes_received = ::recv(_fd, (char*)buffer, static_cast<int>(length), flags);
            if (bytes_received < 1) {
                SOCKET_ERROR__RECV();
            }
            return bytes_received;
        }

        // Connect to remote address
        inline bool connect(const struct sockaddr* addr, socklen_t addrlen) {
            if (::connect(_fd, addr, addrlen) < 0) {
                SOCKET_ERROR__CONNECT();
                return false;
            }
            return true;
        }

        // Close the socket (safe to call multiple times)
        inline void close() noexcept {
            if (is_valid()) {
                CLOSE_SOCKET(_fd);
                _fd = INVALID_SOCKET;
            }
        }

        // check if socket is valid
        inline bool is_valid() const noexcept {
            return IS_VALID_SOCKET(_fd);
        }

        // Access native socket handle
        inline SOCKET native_handle() const noexcept {
            return _fd;
        }

    private:
        SOCKET _fd;
    };

    // convenience function to createe a socket bound to local address
    Socket create_socket_bind_to_local_addr(
        const std::string& bind_mode,
        uint16_t port = 8080,
        int domain = AF_INET6,
        int socktype = SOCK_STREAM,
        int flags = AI_PASSIVE)
    {
        // create the socket
        struct addrinfo* bind_addr = get_local_addr_to_bind(
            bind_mode,
            port,
            domain,
            socktype);
        if (!bind_addr) {
            SOCKET_ERROR__GETADDRINFO();
            return Socket(INVALID_SOCKET);
        }

        // create the socket
        SOCKET fd = ::socket(
            bind_addr->ai_family,
            bind_addr->ai_socktype,
            bind_addr->ai_protocol);
        if (!IS_VALID_SOCKET(fd)) {
            SOCKET_ERROR__SOCKET();
            return Socket(INVALID_SOCKET);
        }
        Socket socket_{fd};

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        int v6only = socket_.get_sockopt(IPPROTO_IPV6, IPV6_V6ONLY);
        if (
            socket_.get_sockopt(SOL_SOCKET, SO_DOMAIN) == AF_INET6 &&
            socket_.get_sockopt(IPPROTO_IPV6, IPV6_V6ONLY) &&
            !socket_.set_sockopt())
        {
            ::freeaddrinfo(bind_addr);
            SOCKET_ERROR__SETSOCKOPT();
            return Socket(INVALID_SOCKET);
        }

        // reuse the address and port
        int reuseaddr = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
            ::freeaddrinfo(bind_addr);
            SOCKET_ERROR__SETSOCKOPT();
            return Socket(INVALID_SOCKET);
        }
#if defined(SO_REUSEPORT)
        int reuseport = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport)) < 0) {
            ::freeaddrinfo(bind_addr);
            SOCKET_ERROR__SETSOCKOPT();
            return Socket(INVALID_SOCKET);
        }
#endif

        // bind the socket to the local address
        if (socket_.bind(bind_addr->ai_addr, bind_addr->ai_addrlen) < 0) {
            ::freeaddrinfo(bind_addr);
            SOCKET_ERROR__BIND();
            return Socket(INVALID_SOCKET);
        }

        // free the address info
        ::freeaddrinfo(bind_addr);

        return socket_;
    }
} // namespace BA_Socket

#endif // SOCKET_HPP





// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <vector>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_Event_Types { Read, Write };
    enum class Enum_Reactor_Command_Types { RegisterRead, RegisterWrite, Unregister, Error, eintr };

    using rc_t = std::pair<Enum_Reactor_Command_Types, int>;
    struct Reactor_Command_Pack{
        std::vector<rc_t> _rcs;
    };
    using Callback = std::function<Reactor_Command_Pack(int)>;

    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int, Enum_Event_Types) = 0;
        virtual void fd_unregister(int) = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP





// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>

namespace BA_Socket {
    class Event_Loop__Select : public IEvent_Loop {
    public:
        Event_Loop__Select(Callback on_read, Callback on_write)
            : _on_read(on_read), _on_write(on_write)
        {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        inline void fd_register(int fd, Enum_Event_Types type) override {
            std::scoped_lock lk(_m);

            if (type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fds_read);
            } else {
                FD_SET(fd, &_fds_write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd) override {
            std::scoped_lock lk(_m);

            FD_CLR(fd, &_fds_read);
            FD_CLR(fd, &_fds_write);
            if (fd == _fd_max) {
                _fd_max = -1;
                for(SOCKET fd = 0; fd <= _fd_max; ++fd) {
                    if (FD_ISSET(fd, &_fds_read)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                    if (FD_ISSET(fd, &_fds_write)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                }
            }
        }

        inline void run() override {
            _running.store(true);
            while (_running.load()) {
                if (_fd_max < 0) break;

                fd_set fds_read = _fds_read;
                fd_set fds_write = _fds_write;
                if (::select(
                    _fd_max + 1,
                    &fds_read,
                    &fds_write,
                    nullptr,
                    nullptr) < 0)
                {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }
                for(SOCKET fd = 0; fd <= _fd_max; ++fd) {
                    if (FD_ISSET(fd, &fds_read)) {
                        auto rcp = _on_read(fd);
                        apply_rcp(rcp);
                    }
                    if (FD_ISSET(fd, &fds_write)) {
                        auto rcp = _on_write(fd);
                        apply_rcp(rcp);
                    }
                }
            }
            stop();
        }

        inline void stop() override {
            _running.store(false);
            for (SOCKET fd = 0; fd <= _fd_max; ++fd) {
                if (FD_ISSET(fd, &_fds_read)) {
                    CLOSE_SOCKET(fd);
                }
                if (FD_ISSET(fd, &_fds_write)) {
                    CLOSE_SOCKET(fd);
                }
            }
        }

        inline void apply_rcp(const Reactor_Command_Pack& rcp) {
            for (const auto& rc : std::move(rcp)._rcs) {
                switch (rc.first) {
                case Enum_Reactor_Command_Types::RegisterRead:
                    fd_register(rc.second, Enum_Event_Types::Read);
                    break;
                case Enum_Reactor_Command_Types::RegisterWrite:
                    fd_register(rc.second, Enum_Event_Types::Write);
                    break;
                case Enum_Reactor_Command_Types::Unregister:
                    fd_unregister(rc.second);
                    break;
                case Enum_Reactor_Command_Types::Error:
                    break;
                case Enum_Reactor_Command_Types::eintr:
                    break;
                default:
                    break;
                }
            }
        }

    private:

        Callback _on_read, _on_write;
        fd_set _fds_read;
        fd_set _fds_write;
        int _fd_max = -1;
        std::mutex _m;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP





// TCP_server.hpp

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "Event_Loop__Select.hpp"
#include <ctype.h>

namespace BA_Socket {
    int TCP_server_helper() {
        // create the server socket and bind to the local address
        PRINTF1("[Server]: Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen{ create_socket_bind_to_local_addr("all") };
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("[Server]: Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // on_read
        auto on_read = Callback(
            [&](int fd_listen){
                // accept a new connection
                PRINTF1("[Server]: Accepting a new connection...\n");
                fflush(stdout);
                struct sockaddr_storage client_addr;
                socklen_t client_len = sizeof(client_addr);
                SOCKET fd_client = ::accept(
                    fd_listen,
                    (struct sockaddr*) &client_addr,
                    &client_len);
                if (!IS_VALID_SOCKET(fd_client)) {
                    if (GET_SOCKET_ERRNO() == EINTR)
                        return Reactor_Command_Pack(
                            std::vector<rc_t>{
                                rc_t(Enum_Reactor_Command_Types::eintr, -1)}); // Ctrl+C pressed
                    SOCKET_ERROR__ACCEPT();
                    return Reactor_Command_Pack(
                        std::vector<rc_t>{
                            rc_t(Enum_Reactor_Command_Types::Error, -1)});
                }
                print_sockaddr<is_debug_mode>(client_addr, client_len);

                // add the new client socket to the master set
                return Reactor_Command_Pack(
                    std::vector<rc_t>{
                        rc_t(Enum_Reactor_Command_Types::RegisterWrite, fd_client)});
        });

        // on_read
        auto on_write = Callback(
            [&](int fd_client){
                // receive data from the client
                char read[1024];
                int bytes_received = ::recv(fd_client, read, 1024, 0);
                PRINTF1("[Server]: Receiving data from client...\n");
                if (bytes_received < 1) {
                    CLOSE_SOCKET(fd_client);
                    auto rcp = Reactor_Command_Pack(
                        std::vector<rc_t>{
                            rc_t(Enum_Reactor_Command_Types::Unregister, fd_client)});
                    if (GET_SOCKET_ERRNO() == EINTR)
                        rcp._rcs.push_back(rc_t(Enum_Reactor_Command_Types::Error, -1));
                    rcp._rcs.push_back(rc_t(Enum_Reactor_Command_Types::eintr, -1));
                    return rcp;
                }
                PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

                // convert to uppercase and send back
                PRINTF1("[Server]: Updating data before ending to client...\n");
                int j;
                for (j = 0; j < bytes_received; ++j)
                    read[j] = toupper(read[j]);
                ::send(fd_client, read, bytes_received, 0);
                PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);

                return Reactor_Command_Pack({});
        });

        // create the event looop
        Event_Loop__Select el{ on_read, on_write };
        el.fd_register(socket_listen.native_handle(), Enum_Event_Types::Read);
        el.run();

        return 0;
    }
    
    inline int TCP_server() {
        SOCKET_STARTUP();
        int status = TCP_server_helper();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP_SERVER_HPP
