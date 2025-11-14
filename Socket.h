#ifndef SOCKET_H
#define SOCKET_H

#include "socket_setup.h"
#include <stdexcept>
#include <utility>
        fprintf(stderr, "recv() failed. (%d)\n", GET_SOCKET_ERRNO());
        fprintf(stderr, "socket() failed. (%d)\n", GET_SOCKET_ERRNO());
        fprintf(stderr, "setsockopt() failed. (%d)\n", GET_SOCKET_ERRNO());
        fprintf(stderr, "bind() failed. (%d)\n", GET_SOCKET_ERRNO());
        fprintf(stderr, "listen() failed. (%d)\n", GET_SOCKET_ERRNO());
        fprintf(stderr, "accept() failed. (%d)\n", GET_SOCKET_ERRNO());
        throw std::runtime_error("socket() failed: " + std::to_string(GET_SOCKET_ERRNO()));
        throw std::runtime_error("bind() failed: " + std::to_string(GET_SOCKET_ERRNO()));
        throw std::runtime_error("listen() failed: " + std::to_string(GET_SOCKET_ERRNO()));
        throw std::runtime_error("accept() failed: " + std::to_string(GET_SOCKET_ERRNO()));
        throw std::runtime_error("connect() failed: " + std::to_string(GET_SOCKET_ERRNO()));

namespace ba_socket {
#ifdef BA_SOCKET_DEBUG
    #define BA_SOCKET_ERROR(msg) \
        fprintf(stderr, "[Socket Error] %s: %s (errno=%d)\n", msg, strerror(GET_SOCKET_ERRNO()), GET_SOCKET_ERRNO())
#else
    #define BA_SOCKET_ERROR(msg) \
        throw std::runtime_error(std::string("[Socket Error] ") + msg + ": " + strerror(GET_SOCKET_ERRNO()))
#endif

    class Socket {
    public:
        Socket(int domain, int type, int protocol = 0) {
            _fd = ::socket(domain, type, protocol);
            if (!IS_VALID_SOCKET(_fd)) {
                throw std::runtime_error("socket() failed: " + std::to_string(GET_SOCKET_ERRNO()));
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

        // Bind to local address
        inline void bind(const struct sockaddr* addr, socklen_t addrlen) {
            if (::bind(_fd, addr, addrlen)) {
                fprintf(stderr, "bind() failed. (%d)\n", GET_SOCKET_ERRNO());
                throw std::runtime_error("bind() failed: " + std::to_string(GET_SOCKET_ERRNO()));
            }
        }

        // Listen for connections
        inline void listen(int backlog = 10) {
            if (::listen(_fd, backlog) < 0) {
                throw std::runtime_error("listen() failed: " + std::to_string(GET_SOCKET_ERRNO()));
            }
        }

        // Accept a new connection (returns a new RAII socket)
        inline Socket accept(struct sockaddr* addr = nullptr, socklen_t* addrlen = nullptr) {
            SOCKET client_fd = ::accept(_fd, addr, addrlen);
            if (!IS_VALID_SOCKET(client_fd)) {
                throw std::runtime_error("accept() failed: " + std::to_string(GET_SOCKET_ERRNO()));
            }
            return Socket(client_fd);
        }

        // Connect to remote address
        inline void connect(const struct sockaddr* addr, socklen_t addrlen) {
            if (::connect(_fd, addr, addrlen) < 0) {
                throw std::runtime_error("connect() failed: " + std::to_string(GET_SOCKET_ERRNO()));
            }
        }

        // Send data
        inline ssize_t send(const void* buffer, size_t length, int flags = 0) {
            return ::send(_fd, (const char*)buffer, static_cast<int>(length), flags);
        }

        // Receive data
        inline ssize_t recv(void* buffer, size_t length, int flags = 0) {
            return ::recv(_fd, (char*)buffer, static_cast<int>(length), flags);
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
