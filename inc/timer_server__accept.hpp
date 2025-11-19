// timer_server__accept.hpp

#ifndef SERVER_WITH_TIMER__ACCEPT_HPP
#define SERVER_WITH_TIMER__ACCEPT_HPP

#include "timer_server__handle_client.hpp"
#include <thread>
#include <vector>
#include <csignal>

namespace BA_Socket {
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
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // Accept loop
        {
            std::vector<std::jthread> client_thread_count;
            while (running) {
                // accept a connection
                PRINTF1("Accepting a connection...\n");
                fflush(stdout);

                struct sockaddr_storage client_addr;
                socklen_t client_len = sizeof(client_addr);
                SOCKET fd_client = ::accept(
                    fd_listen,
                    (struct sockaddr*)&client_addr,
                    &client_len);
                if (!IS_VALID_SOCKET(fd_client)) {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;   // accept interrupted by Ctrl+C
                    SOCKET_ERROR__ACCEPT();
                    return 1;
                }

                // Spawn a thread per client
                if (!running) {
                    CLOSE_SOCKET(fd_client);
                    break;
                }
                client_thread_count.emplace_back(
                    handle_client,
                    fd_client,
                    client_addr,
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
} // namespace BA_Socket

#endif // SERVER_WITH_TIMER__ACCEPT_HPP
