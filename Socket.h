#ifndef SOCKET_H
#define SOCKET_H

#include "socket_setup.h"
#include <stdexcept>
#include <utility>
#include <cstring>
#include <stdio.h>

namespace ba_socket {
#define BA_SOCKET_DEBUG
#ifdef BA_SOCKET_DEBUG
    #define SOCKET_ERROR__RECV() \
        fprintf(stderr, "[recv() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SOCKET() \
        fprintf(stderr, "[socket() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SETSOCKOPT() \
        fprintf(stderr, "[setsockopt() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__BIND() \
        fprintf(stderr, "[bind() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__LISTEN() \
        fprintf(stderr, "[listen() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__ACCEPT() \
        fprintf(stderr, "[accept() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__SEND() \
        fprintf(stderr, "[send() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
    #define SOCKET_ERROR__CONNECT() \
        fprintf(stderr, "[connect() error] %s: %s (errno=%d)\n", strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
#else
    #define SOCKET_ERROR__RECV() \
        throw std::runtime_error(std::string("[recv() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
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
    #define SOCKET_ERROR__SEND() \
        throw std::runtime_error(std::string("[send() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
    #define SOCKET_ERROR__CONNECT() \
        throw std::runtime_error(std::string("[connect() error] ") + ": " + strerror(GET_SOCKET_ERRNO()))
#endif

    class Socket {
    public:
        Socket(int domain, int type, int protocol = 0) {
            _fd = ::socket(domain, type, protocol);
            if (!IS_VALID_SOCKET(_fd)) {
                SOCKET_ERROR__SOCKET();
            }
        }
        explicit Socket(SOCKET fd) noexcept : _fd(fd) {};
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

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        inline void socketopt(int option) {
            if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&option, sizeof(option))) {
                SOCKET_ERROR__SETSOCKOPT();
            }
        }

        // Bind to local address
        inline void bind(const struct sockaddr* addr, socklen_t addrlen) {
            if (::bind(_fd, addr, addrlen)) {
                SOCKET_ERROR__BIND();
            }
        }

        // Listen for connections
        inline void listen(int backlog = 10) {
            if (::listen(_fd, backlog) < 0) {
                SOCKET_ERROR__LISTEN();
            }
        }

        // Accept a new connection (returns a new RAII socket)
        inline Socket accept(struct sockaddr* addr = nullptr, socklen_t* addrlen = nullptr) {
            SOCKET client_fd = ::accept(_fd, addr, addrlen);
            if (!IS_VALID_SOCKET(client_fd)) {
                SOCKET_ERROR__ACCEPT();
            }
            return Socket(client_fd);
        }

        // Send data
        inline ssize_t send(const void* buffer, size_t length, int flags = 0) {
            return ::send(_fd, (const char*)buffer, static_cast<int>(length), flags);
        }

        // Safe send
        ssize_t send_all(const void* buffer, size_t length, int flags = 0) {
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
                }
                if (sent == 0) break; // shouldn't happen unless socket closed
                total_sent += sent;
            }
        }

        // Receive data
        inline ssize_t recv(void* buffer, size_t length, int flags = 0) {
            ssize_t bytes_received = ::recv(_fd, (char*)buffer, static_cast<int>(length), flags);
            if (bytes_received < 1) {
                SOCKET_ERROR__RECV();
            }
            return bytes_received;
        }

        // Connect to remote address
        inline void connect(const struct sockaddr* addr, socklen_t addrlen) {
            if (::connect(_fd, addr, addrlen) < 0) {
                SOCKET_ERROR__CONNECT();
            }
        }

        // Close the socket (safe to call multiple times)
        inline void close() noexcept {
            if (is_valid()) {
                CLOSE_SOCKET(_fd);
                _fd = INVALID_SOCKET;
            }
        }

        // Check if socket is valid
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
} // namespace ba_socket

#endif // SOCKET_H
