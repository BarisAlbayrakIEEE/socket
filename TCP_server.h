// TCP_server.h

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "Socket.h"
#include "utility_addr.h"
#include <ctype.h>

namespace ba_socket {
    int TCP_server() {
        SOCKET_STARTUP();

        // create the server socket and bind to the local address
        PRINTF1("Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen{ "localhost" };
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        PRINTF1("Converting IPV6_V6ONLY socket to dual stack...\n");
        if (!socket_listen.socketopt(0)) return 1;

        // listen for connections
        PRINTF1("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // create the fd_set
        fd_set master;
        FD_ZERO(&master);
        FD_SET(fd_listen, &master);
        SOCKET fd_max = fd_listen;

        PRINTF1("Waiting for connections...\n");
        while(1) {
            fd_set reads;
            reads = master;
            if (::select(fd_max + 1, &reads, 0, 0, 0) < 0) {
                SOCKET_ERROR__SELECT();
                return 1;
            }

            for(SOCKET i = 1; i <= fd_max; ++i) { // start from stdout (fd=1)
                if (FD_ISSET(i, &reads)) {
                    if (i == fd_listen) { // server socket
                        // accept a new connection
                        PRINTF1("Accepting a new connection...\n");
                        fflush(stdout);
                        struct sockaddr_storage client_addr;
                        socklen_t client_len = sizeof(client_addr);
                        Socket socket_client = socket_listen.accept(
                            (struct sockaddr*) &client_addr,
                            &client_len);
                        if (!socket_client.is_valid()) {
                            SOCKET_ERROR__ACCEPT();
                            return 1;
                        }
                        print_sockaddr<is_debug_mode>(client_addr, client_len);

                        // add the new client socket to the master set
                        SOCKET fd_client = socket_client.native_handle();
                        FD_SET(fd_client, &master);
                        if (fd_client > fd_max) fd_max = fd_client;
                    } else { // client socket
                        // receive data from the client
                        char read[1024];
                        int bytes_received = ::recv(i, read, 1024, 0);
                        if (bytes_received < 1) {
                            FD_CLR(i, &master);
                            CLOSE_SOCKET(i);
                            continue;
                        }

                        // convert to uppercase and send back
                        int j;
                        for (j = 0; j < bytes_received; ++j)
                            read[j] = toupper(read[j]);
                        ::send(i, read, bytes_received, 0);
                    }
                }
            }
        }

        // close all remaining sockets
        for (SOCKET i = 0; i <= fd_max; ++i) {
            if (FD_ISSET(i, &master)) {
                CLOSE_SOCKET(i);
            }
        }

        // close the listening socket
        PRINTF1("Closing the listening socket...\n");
        socket_listen.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // TCP_SERVER_H
