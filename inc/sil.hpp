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
#include <memory>
#include <utility>
#include <concepts>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_Register_Types { None, Register, Unregister };
    enum class Enum_Event_Types { None, Read, Write, Read_Write }; // Read_Write is for unregistering from both read and write
    enum class Enum_Handler_Action_Types { None, Add, Remove, Replace };

    const std::string INFO_WRONG_DATA = "Wrong data for the request";

    struct fd_set_Action{
        int _fd{-1};
        Enum_Register_Types _register_type{ Enum_Register_Types::None };
        Enum_Event_Types _event_type{ Enum_Event_Types::None };
    };

    struct IHandler;
    using fd_set_actions_t = std::vector<fd_set_Action>;
    using handler_action_t = std::pair<Enum_Handler_Action_Types, std::unique_ptr<IHandler>>;
    using handler_return_t = std::pair<fd_set_actions_t, handler_action_t>;

    template <typename F>
    concept CString_Forward = 
        requires (F f, const std::string& s) { { f(s) } -> std::same_as<bool>; };

    template <typename F>
    concept CString_Transform = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<bool>; };

    // Handler interface
    struct IHandler {
        virtual handler_return_t apply(int) const = 0;
    };

    // Write handler
    //
    // fd_set actions:
    //   Unregisters the fd from the write fd_set.
    //   New read will register the fd again.
    //
    // Handler action:
    //   Removes the handler from the handler map.
    struct Handler_Write : public IHandler {
        std::string _buffer{};

        handler_return_t apply(int fd) const {
            // send the data to the peer
            PRINTF1("[Server]: Sending the data to the peer...\n");
            ::send(fd, _buffer.c_str(), _buffer.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Accept handler
    //
    // Base template: Followed by a one of read handlers.
    // Will be specialized for the case that is followed by a write handler.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new handler of type Next_Handler_Type.
    //   Next handler type shall be one of read handlers (e.g. read-forward).
    template <typename Next_Handler_Type>
        requires std::is_base_of_v<IHandler, Next_Handler_Type>
    struct Handler_Accept : IHandler {
        handler_return_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Server]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd)) {
                if (GET_SOCKET_ERRNO() != EINTR) SOCKET_ERROR__ACCEPT();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd_client,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Read) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Next_Handler_Type>()));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a write handler.
    // Will be specialized for the case that is followed by a write handler.
    //
    // fd_set actions:
    //   Registers the client fd to the write fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Write.
    template <>
    struct Handler_Accept<Handler_Write> : IHandler {
        std::string _buffer{};

        handler_return_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Server]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd)) {
                if (GET_SOCKET_ERRNO() != EINTR) SOCKET_ERROR__ACCEPT();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd_client,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Write) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Handler_Write>(_buffer)));
        };
    };

    // Redirect handler
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    struct Handler_Redirect {
        std::string _buffer{};
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // send the data to the peer
            PRINTF1("[Server]: Sending the data to the peer...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, _buffer.c_str(), _buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Forward handler:
    //   Recieves the data from the peer
    //   and sends it to a function (for processing the peer data internally).
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    template <typename F>
        requires CString_Forward<F>
    struct Handler_Read_Forward {
        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // forward the recieved data to function F
            PRINTF1("[Server]: Forwarding the recieved data to function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed forwarding (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed forwarding (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Redirect handler:
    //   Recieves the data from the peer
    //   and sends it to the sockets with the contained file descriptors.
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    struct Handler_Read_Redirect {
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // redirect the data to the contained fds
            PRINTF1("[Server]: Redirecting the data to the ...\n");
            std::string buffer{ read };
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next action (write or redirect).
    //
    // Base template: Followed by a write handler.
    // Will be specialized for the case that is followed by a redirect handler.
    //
    // fd_set actions:
    //   Registers the fd to the write fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Write.
    template <typename F, typename Next_Handler_Type>
        requires
            CString_Transform<F> &&
            (
                std::is_same_v<Next_Handler_Type, Handler_Write> ||
                std::is_same_v<Next_Handler_Type, Handler_Redirect>)
    struct Handler_Read_Transform {
        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Write) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Handler_Write>(std::move(buffer))));
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next action (write or redirect).
    //
    // fd_set actions:
    //   Registers the fd to the write fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Write.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform<F, Handler_Redirect> {
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Write) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Handler_Redirect>(std::move(buffer), _fds)));
        };
    };

    // Read-Transform-Write handler:
    //   Recieves the data from the peer,
    //   transforms it
    //   and writes back to the peer.
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Write {
        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // send the transformed data back to the peer
            PRINTF1("[Server]: Sending the transformed data back to the peer...\n");
            ::send(fd, buffer.c_str(), bytes_received, 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Transform-Redirect handler:
    //   Recieves the data from the peer,
    //   transforms it
    //   and redirects to the sockets defined as a member.
    //
    // Specialization for: Followed by a redirect handler.
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Redirect {
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // redirect the data to the contained fds
            PRINTF1("[Server]: Redirecting the data to the ...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Event loop interface
    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int, Enum_Event_Types) = 0;
        virtual void fd_unregister(int, Enum_Event_Types) = 0;
        virtual void close_sockets() = 0;

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
        Event_Loop__Select() {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        inline void fd_register(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fds_read);
            }
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Write) {
                FD_SET(fd, &_fds_write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Read) {
                FD_CLR(fd, &_fds_read);
            }
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Write) {
                FD_CLR(fd, &_fds_read);
            }
            if (fd == _fd_max) {
                if (
                    (event_type == Enum_Event_Types::Read && FD_ISSET(fd, &_fds_write)) ||
                    (event_type == Enum_Event_Types::Write && FD_ISSET(fd, &_fds_read)))
                {
                    return;
                }

                auto fd_max = _fd_max;
                _fd_max = -1;
                for(SOCKET fd = 0; fd <= fd_max; ++fd) {
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

                // perform select operation
                fd_set fds_read = _fds_read;
                fd_set fds_write = _fds_write;
                if (::select(_fd_max + 1, &fds_read, &fds_write, nullptr, nullptr) < 0) {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }

                // initialize a new map to collect the new handlers
                // those should be added to or removed from _handlers
                //   true: handler to be added
                //   false: handler to be removed
                std::unordered_map<int, std::pair<bool, std::unique_ptr<IHandler>>> new_handler_actions;

                // execute _handlers
                for (auto& [fd, handler] : _handlers) {
                    if (!handler) continue;

                    // execute the handler
                    auto handler_return = handler->apply(fd);
                    auto fd_set_actions = std::move(handler_return.first);
                    auto handler_action = std::move(handler_return.second);

                    // loop through the fd_set actions reulted from the handler
                    for (const auto& fd_set_action : fd_set_actions) {
                        if (fd_set_action._register_type == Enum_Register_Types::Register) {
                            fd_register(fd_set_action._fd, fd_set_action._event_type);
                        }
                        else if (fd_set_action._register_type == Enum_Register_Types::Unregister) {
                            fd_unregister(fd_set_action._fd, fd_set_action._event_type);
                        }
                    }
                    
                    // perform the handler action resulted from the handler:
                    //   collect the handlers those should be added to or removed from _handlers
                    if(handler_action.first == Enum_Handler_Action_Types::None) continue;
                    if(handler_action.first == Enum_Handler_Action_Types::Add) {
                        new_handler_actions[fd] = { true, std::move(handler_action.second) };
                    }
                    else if(handler_action.first == Enum_Handler_Action_Types::Remove) {
                        new_handler_actions[fd] = { false, std::move(handler_action.second) };
                    }
                    else { // if(handler_action.first == Enum_Handler_Action_Types::Replace) {
                        new_handler_actions[fd] = { true, std::move(handler_action.second) };
                    }
                }

                // update _handlers by the collected handlers
                // those should be added to or removed from _handlers
                for (auto& [fd, handler_action_pair] : new_handler_actions) {
                    if(handler_action_pair.first) {
                        _handlers[fd] = std::move(handler_action_pair.second);
                    }
                    else {
                        _handlers.erase(fd);
                    }
                }
            }
        }

        inline void stop() override {
            _running.store(false);
        }

        inline void close_sockets() override {
            for (SOCKET fd = 0; fd <= _fd_max; ++fd) {
                if (FD_ISSET(fd, &_fds_read)) {
                    CLOSE_SOCKET(fd);
                }
                if (FD_ISSET(fd, &_fds_write)) {
                    CLOSE_SOCKET(fd);
                }
            }
        }

    private:
        std::unordered_map<int, std::unique_ptr<IHandler>> _handlers;
        fd_set _fds_read;
        fd_set _fds_write;
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
