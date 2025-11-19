// TCP_server.hpp

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "Socket.hpp"
#include "utility_addr.hpp"
#include <atomic>
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

        // create the fd_set
        fd_set master;
        FD_ZERO(&master);
        FD_SET(fd_listen, &master);
        SOCKET fd_max = fd_listen;

        PRINTF1("[Server]: Waiting for connections...\n");
        while(1) {
            // call select to register running file descriptors to the fd_set
            fd_set reads = master;
            if (::select(fd_max + 1, &reads, nullptr, nullptr, nullptr) < 0) {
                if (GET_SOCKET_ERRNO() == EINTR) break; // Ctrl+C pressed
                SOCKET_ERROR__SELECT();
                return 1;
            }

            // loop through the file descriptors registered in the fd_set
            for(SOCKET fd = 1; fd <= fd_max; ++fd) { // start from stdout (fd=1)
                if (!FD_ISSET(fd, &reads))
                    continue;

                if (fd == fd_listen) { // server socket
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
                        if (GET_SOCKET_ERRNO() == EINTR) break; // Ctrl+C pressed
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
                    int bytes_received = ::recv(fd, read, 1024, 0);
                    PRINTF1("[Server]: Receiving data from client...\n");
                    if (bytes_received < 1) {
                        FD_CLR(fd, &master);
                        CLOSE_SOCKET(fd);
                        if (GET_SOCKET_ERRNO() == EINTR) break; // Ctrl+C pressed
                        continue;
                    }
                    PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

                    // convert to uppercase and send back
                    PRINTF1("[Server]: Updating data before ending to client...\n");
                    int j;
                    for (j = 0; j < bytes_received; ++j)
                        read[j] = toupper(read[j]);
                    ::send(fd, read, bytes_received, 0);
                    PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);
                }
            }
        }

        // close all remaining sockets
        for (SOCKET fd = 0; fd <= fd_max; ++fd) {
            if (FD_ISSET(fd, &master)) {
                CLOSE_SOCKET(fd);
            }
        }

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
