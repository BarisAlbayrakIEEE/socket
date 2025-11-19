// timer_server__select.hpp

#ifndef SERVER_WITH_TIMER__SELECT_HPP
#define SERVER_WITH_TIMER__SELECT_HPP

#include "timer_server__handle_client.hpp"
#include <thread>
#include <vector>
#include <csignal>

namespace BA_Socket {
    int timer_server__select_helper(void) {
        // create the server socket and bind to the local address
        PRINTF1("Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen = create_socket_bind_to_local_addr("all");
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // Prepare fd_set for select()
        fd_set master;
        FD_ZERO(&master);
        FD_SET(fd_listen, &master);
        SOCKET fd_max = fd_listen;

        // Accept loop
        {
            std::vector<std::jthread> client_threads;
            while (1) {
                fd_set reads = master;

                // call select to register running file descriptors to the fd_set.
                struct timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 100000;
                if (::select(fd_max + 1, &reads, nullptr, nullptr, &timeout) < 0) {
                    if (GET_SOCKET_ERRNO() == EINTR) break; // Ctrl+C pressed
                    SOCKET_ERROR__SELECT();
                    return 1;
                }

                // loop through the file descriptors registered in the fd_set
                for (SOCKET fd = 0; fd <= fd_max; ++fd) {
                    if (!FD_ISSET(fd, &reads))
                        continue;

                    if (fd == fd_listen) {
                        // Listening socket ready → incoming connection
                        PRINTF1("Accepting a connection...\n");

                        // accept a new connection
                        struct sockaddr_storage client_addr;
                        socklen_t client_len = sizeof(client_addr);
                        SOCKET fd_client = ::accept(
                            fd_listen,
                            (struct sockaddr*)&client_addr,
                            &client_len);
                        if (!IS_VALID_SOCKET(fd_client)) {
                            if (GET_SOCKET_ERRNO() == EINTR) break; // Ctrl+C pressed
                            SOCKET_ERROR__ACCEPT();
                            return 1;
                        }
                        print_sockaddr<is_debug_mode>(client_addr, client_len);

                        // Spawnhandler thread
                        client_threads.emplace_back(
                            handle_client,
                            fd_client,
                            client_addr,
                            client_len
                        );
                    }
                }
            }
        }

        // wait for clients to finish
        PRINTF1("Waiting for clients to finish...\n");
        while (active_clients.load() > 0) {
            PRINTF2("[Active clients: %d]\n", active_clients.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        PRINTF1("All clients finished.\n");

        return 0;
    }
    
    inline int timer_server__select() {
        SOCKET_STARTUP();
        int status = timer_server__select_helper();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // SERVER_WITH_TIMER__SELECT_HPP
