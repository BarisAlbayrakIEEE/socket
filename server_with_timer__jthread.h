#ifndef SERVER_WITH_TIMER__JTHREAD_H
#define SERVER_WITH_TIMER__JTHREAD_H

#include "handle_client.h"
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
        printf("Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen{ Socket::create_bind_socket() };
        if (!socket_listen.is_valid()) return 1;

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        printf("Converting IPV6_V6ONLY socket to dual stack...\n");
        if (!socket_listen.socketopt(1)) return 1;

        // listen for connections
        printf("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // Accept loop
        {
            std::vector<std::jthread> client_threads;
            ::signal(SIGINT, signal_handler);
            while (running) {
                // accept a connection
                printf("Accepting a connection...\n");
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

        /*// wait for clients to finish
        printf("Waiting for clients to finish...\n");
        while (active_clients.load() > 0) {
            printf("[Active clients: %d]\n", active_clients.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        printf("All clients finished.\n");*/

        // close listening socket
        printf("Closing listening socket...\n");
        socket_listen.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER__JTHREAD_H
