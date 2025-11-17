#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "Socket.h"
#include "utility_addr.h"
#ifdef _WIN32
    #include <conio.h>
#endif

namespace ba_socket {
    int create_TCP_client(const std::string& hostname = "localhost", uint16_t port = 8080) {
        SOCKET_STARTUP();

        // obtain the peer address
        struct addrinfo *peer_addr = get_addrinfo(SOCK_STREAM, hostname, port);
        if (!peer_addr) return 1;

        // print the peer address
        print_addrinfo<is_debug_mode>(peer_addr);

        // create the peer socket
        PRINTF1("Creating the peer socket...\n");
        Socket socket_peer{
            peer_addr->ai_family,
            peer_addr->ai_socktype,
            peer_addr->ai_protocol};
        if (!socket_peer.is_valid()) {
            SOCKET_ERROR__SOCKET();
            freeaddrinfo(peer_addr);
            return 1;
        }
        SOCKET fd_peer = socket_peer.native_handle();

        // connect to the remote server
        PRINTF1("Connecting to the remote server...\n");
        if (
            connect(
                fd_peer,
                peer_addr->ai_addr,
                peer_addr->ai_addrlen))
        {
            freeaddrinfo(peer_addr);
            return 1;
        }
        freeaddrinfo(peer_addr);

        // let the user enter data to send via stdin
        PRINTF1("Connected to the remote server.\n");
        PRINTF1("To send data, enter text followed by enter.\n");

        // loop for the data transfer: terminal -> server OR server -> terminal
        while(1) {
            // create the fd_set
            fd_set reads;
            FD_ZERO(&reads);
            FD_SET(fd_peer, &reads);
#if !defined(_WIN32)
            FD_SET(0, &reads); // on unix/linux, stdin fd = 0
#endif

            // wait for data on either socket
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            if (select(fd_peer + 1, &reads, 0, 0, &timeout) < 0) {
                SOCKET_ERROR__SELECT();
                return 1;
            }

            // check if data from peer
            if (FD_ISSET(fd_peer, &reads)) {
                char read[4096];
                int bytes_received = socket_peer.recv(read, 4096, 0);
                if (bytes_received < 1) {
                    PRINTF1("Connection closed by peer.\n");
                    break;
                }
                PRINTF4("Received (%d bytes): %.*s", bytes_received, bytes_received, read);
            }

            // check if data from stdin
#if defined(_WIN32)
            if(_kbhit()) {
#else
            if(FD_ISSET(0, &reads)) {
#endif
                char read[4096];
                if (!fgets(read, 4096, stdin)) break;
                PRINTF2("Sending: %s", read);
                int bytes_sent = socket_peer.send(read, strlen(read), 0);
                PRINTF2("Sent %d bytes.\n", bytes_sent);
            }
        }

        // close the peer socket
        PRINTF1("Closing the peer socket...\n");
        socket_peer.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // TCP_CLIENT_H
