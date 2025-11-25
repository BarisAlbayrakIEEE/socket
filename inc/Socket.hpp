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
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
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
                    if (errno_ == ERROR_INTERRUPTED) continue; // Interrupted -> retry
                    if (errno_ == ERROR_BLOCKED || errno_ == EWOULDBLOCK) {
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
        inline int recv(void* buffer, size_t length, int flags = 0) {
            int bytes_received = ::recv(_fd, (char*)buffer, static_cast<int>(length), flags);
            if (bytes_received == 0) {
                PRINTF1("Peer closed the connection.\n");
                CLOSE_SOCKET(_fd);
            }
            else if (bytes_received < 0) {
                if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) { // Ctrl+C
                    return bytes_received;
                }
                CLOSE_SOCKET(_fd);
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
        if (!socket_.bind(bind_addr->ai_addr, bind_addr->ai_addrlen)) {
            ::freeaddrinfo(bind_addr);
            return Socket(INVALID_SOCKET);
        }

        // free the address info
        ::freeaddrinfo(bind_addr);

        return socket_;
    }
} // namespace BA_Socket

#endif // SOCKET_HPP
