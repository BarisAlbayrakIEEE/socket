// TCP_server.h

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "Socket.h"
#include "utility_addr.h"
#include <ctype.h>
#include <vector>

namespace ba_socket {
    int TCP_server() {
        SOCKET_STARTUP();

        // create the server socket and bind to the local address
        PRINTF1("[Server]: Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen{ create_socket_bind_to_local_addr("localhost") };
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("[Server]: Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // create the fd_set
        fd_set master;
        FD_ZERO(&master);
        FD_SET(fd_listen, &master);
        SOCKET fd_max = fd_listen;

        PRINTF1("[Server]: Waiting for connections...\n");
        while(1) {
            fd_set reads;
            reads = master;
            if (::select(fd_max + 1, &reads, nullptr, nullptr, nullptr) < 0) {
                SOCKET_ERROR__SELECT();
                return 1;
            }

            for(SOCKET i = 1; i <= fd_max; ++i) { // start from stdout (fd=1)
                if (FD_ISSET(i, &reads)) {
                    if (i == fd_listen) { // server socket
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
                            SOCKET_ERROR__ACCEPT();
                            return 1;
                        }
                        print_sockaddr<is_debug_mode>(client_addr, client_len);

                        // add the new client socket to the master set
                        FD_SET(fd_client, &master);
                        if (fd_client > fd_max) fd_max = fd_client;
                    } else { // client socket
                        // receive data from the client
                        char read[1024];
                        int bytes_received = ::recv(i, read, 1024, 0);
                        PRINTF1("[Server]: Receiving data from client...\n");
                        if (bytes_received < 1) {
                            FD_CLR(i, &master);
                            CLOSE_SOCKET(i);
                            continue;
                        }
                        PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

                        // convert to uppercase and send back
                        PRINTF1("[Server]: Updating data before ending to client...\n");
                        int j;
                        for (j = 0; j < bytes_received; ++j)
                            read[j] = toupper(read[j]);
                        ::send(i, read, bytes_received, 0);
                        PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);
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
        PRINTF1("[Server]: Closing the listening socket...\n");
        socket_listen.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // TCP_SERVER_H
