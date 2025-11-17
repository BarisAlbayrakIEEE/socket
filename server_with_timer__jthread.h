#ifndef SERVER_WITH_TIMER__JTHREAD_H
#define SERVER_WITH_TIMER__JTHREAD_H

#include "server_with_timer__handle_client.h"
#include <thread>
#include <vector>
#include <csignal>

namespace ba_socket {
    std::atomic<bool> running{true};
    void signal_handler(int) {
        running = false;
    }

    int server_with_timer__jthread(void) {
        SOCKET_STARTUP();

        // create the server socket and bind to the local address
        PRINTF1("Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen{ "localhost" };
        if (!socket_listen.is_valid()) return 1;

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        PRINTF1("Converting IPV6_V6ONLY socket to dual stack...\n");
        if (!socket_listen.socketopt(0)) return 1;

        // listen for connections
        PRINTF1("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // Accept loop
        {
            std::vector<std::jthread> client_threads;
            ::signal(SIGINT, signal_handler);
            while (running) {
                // accept a connection
                PRINTF1("Accepting a connection...\n");
                fflush(stdout);

                struct sockaddr_storage client_address;
                socklen_t client_len = sizeof(client_address);
                Socket socket_client = socket_listen.accept((struct sockaddr*)&client_address, &client_len);
                if (!socket_client.is_valid()) return 1;

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

        // close listening socket
        PRINTF1("Closing listening socket...\n");
        socket_listen.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER__JTHREAD_H
