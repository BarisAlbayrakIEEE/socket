#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "Socket.h"
#ifdef _WIN32
    #include <conio.h>
#endif

namespace ba_socket {
    int create_TCP_client(const std::string& hostname = "localhost", uint16_t port = 8080) {
        SOCKET_STARTUP();

        std::string port_str(std::to_string(port));
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *peer_addr;
        if (getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &peer_addr)) {
            SOCKET_ERROR__GETADDRINFO();
            return 1;
        }

#if defined(BA_SOCKET_DEBUG)
        printf("Printing the remote address...\n");
        char buffer_addr[100];
        char buffer_service[100];
        getnameinfo(
            peer_addr->ai_addr,
            peer_addr->ai_addrlen,
            buffer_addr,
            sizeof(buffer_addr),
            buffer_service,
            sizeof(buffer_service),
            NI_NUMERICHOST);
        printf("%s %s\n", buffer_addr, buffer_service);
#endif

        printf("Creating the peer socket...\n");
        Socket socket_peer{
            peer_addr->ai_family,
            peer_addr->ai_socktype,
            peer_addr->ai_protocol};
        if (!socket_peer.is_valid()) {
            SOCKET_ERROR__SOCKET();
            return 1;
        }


        SOCKET_CLEANUP();
    }
} // namespace ba_socket

#endif // TCP_CLIENT_H
