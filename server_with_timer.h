#ifndef SERVER_WITH_TIMER_H
#define SERVER_WITH_TIMER_H

#include "Socket.h"
#include <string.h>
#include <time.h>
#include <thread>
#include <atomic>
#include <vector>
#include <csignal>

namespace ba_socket {
    std::atomic<bool> running{true};
    std::atomic<int> active_clients{0};
    void signal_handler(int) {
        running = false;
    }

    // Worker thread: handles a single client connection
    void handle_client(Socket&& client_socket, struct sockaddr_storage client_address, socklen_t client_len) {
        ++active_clients;
        printf("[Active clients: %d]\n", active_clients.load());
        
        // print client address
        printf("Printing client address... ");

        char address_buffer[100];
        ::getnameinfo(
            (struct sockaddr*)&client_address,
            client_len,
            address_buffer, sizeof(address_buffer),
            NULL, 0,
            NI_NUMERICHOST);
        printf("[Client connected] %s\n", address_buffer);

        // Read the request
        printf("Reading the request...\n");
        fflush(stdout);

        char request[1024];
        int bytes_received = static_cast<int>(client_socket.recv(request, sizeof(request)));
        if (bytes_received < 1) {
            --active_clients;
            return;
        }
        printf("[Request from %s]\n%.*s\n", address_buffer, bytes_received, request);
        fflush(stdout);

        // send the response
        printf("Sending the response...\n");

        time_t timer;
        time(&timer);
        char *time_msg = ctime(&timer);
        time_msg[strcspn(time_msg, "\n")] = '\0';

        char body[256];
        snprintf(body, sizeof(body), "Local time is: %s", time_msg);
        char header[256];
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n\r\n",
            strlen(body));

        client_socket.send(header, strlen(header), 0);
        client_socket.send(body, strlen(body), 0);

        // close client socket (connection)
        printf("Closing client socket (connection)...\n");
        client_socket.close();

        --active_clients;
    }

    int server_with_timer(void) {
        SOCKET_STARTUP();

        // create the server socket and bind to the local address
        printf("Creating socket for the server and binding it to the local address...\n");
        Socket server_socket{ Socket::create_bind_socket(8080) };
        if (!server_socket.is_valid()) return 1;

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        printf("Converting IPV6_V6ONLY socket to dual stack...\n");
        if (!server_socket.socketopt(1)) return 1;

        // listen for connections
        printf("Listening for connections...(Ctrl+C to stop)\n");
        if (!server_socket.listen(10)) return 1;

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
                Socket client_socket = server_socket.accept((struct sockaddr*)&client_address, &client_len);
                if (!client_socket.is_valid()) return 1;

                // Spawn a thread per client
                if (!running) {
                    client_socket.close();
                    break;
                }
                client_threads.emplace_back(
                    handle_client,
                    std::move(client_socket),
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
        server_socket.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER_H
