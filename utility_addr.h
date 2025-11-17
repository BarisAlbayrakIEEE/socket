// utility_addr.h

#ifndef UTILITY_ADDR_H
#define UTILITY_ADDR_H

#include "socket_setup.h"
#include <cstring>

namespace ba_socket {
    inline struct addrinfo* get_addrinfo(
        int socktype = SOCK_STREAM,
        const std::string& hostname = "localhost",
        uint16_t port = 8080)
    {
        PRINTF1("Obtaining the peer address...\n");
        std::string port_str(std::to_string(port));
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = socktype;
        struct addrinfo *socket_addr;
        if (::getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &socket_addr)) {
            SOCKET_ERROR__GETADDRINFO();
            return nullptr;
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
} // namespace ba_socket

#endif // UTILITY_ADDR_H
