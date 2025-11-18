// timer_server__accept.h

#ifndef SERVER_WITH_TIMER__ACCEPT_H
#define SERVER_WITH_TIMER__ACCEPT_H

#include "timer_server__handle_client.h"
#include <thread>
#include <vector>
#include <csignal>

namespace ba_socket {
    std::atomic<bool> running{true};
    void signal_handler(int) {
        running = false;
    }

    int timer_server__accept_helper(void) {
        // sigaction to handle Ctrl + C
        struct sigaction sa{};
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0; // disable SA_RESTART to allow Ctrl + C to interrupt accept()
        sigaction(SIGINT, &sa, nullptr);

        // create the server socket and bind to the local address
        PRINTF1("Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen = create_socket_bind_to_local_addr("all");
        if (!socket_listen.is_valid()) return 1;

        // listen for connections
        PRINTF1("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // Accept loop
        {
            std::vector<std::jthread> client_threads;
            while (running) {
                // accept a connection
                PRINTF1("Accepting a connection...\n");
                fflush(stdout);

                struct sockaddr_storage client_address;
                socklen_t client_len = sizeof(client_address);
                Socket socket_client = socket_listen.accept(
                    (struct sockaddr*)&client_address,
                    &client_len);
                if (!socket_client.is_valid()) {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;   // accept interrupted by Ctrl+C
                    SOCKET_ERROR__ACCEPT();
                    return 1;
                }

                // Spawn a thread per client
                if (!running) {
                    socket_client.close();
                    break;
                }
                client_threads.emplace_back(
                    handle_client,
                    std::move(socket_client),
                    client_address,
                    client_len);
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
    
    inline int timer_server__accept() {
        SOCKET_STARTUP();
        int status = timer_server__accept_helper();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER__ACCEPT_H
