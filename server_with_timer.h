#ifndef SERVER_WITH_TIMER_H
#define SERVER_WITH_TIMER_H

#include "Socket.h"
#include <string.h>
#include <time.h>
#include <thread>
#include <atomic>
#include <csignal>

namespace ba_socket {
    std::atomic<bool> running{true};
    std::atomic<int> active_clients{0};
    void signal_handler(int) {
        running = false;
    }

    // Worker thread: handles a single client connection
    void handle_client(Socket&& socket_client, struct sockaddr_storage client_address, socklen_t client_len) {
        ++active_clients;
        printf("[Active clients: %d]\n", active_clients.load());
        
        // print client address
        printf("Printing client address... ");

        char address_buffer[100];
        getnameinfo(
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
        int bytes_received = static_cast<int>(socket_client.recv(request, sizeof(request)));
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

        socket_client.send(header, strlen(header), 0);
        socket_client.send(body, strlen(body), 0);

        // close client socket (connection)
        printf("Closing client socket (connection)...\n");
        socket_client.close();

        --active_clients;
    }

    int server_with_timer(void) {
        SOCKET_STARTUP();

        // create the server socket and bind to the local address
        Socket socket_listen{ Socket::create_bind_socket(8080) };
        if (!socket_listen.is_valid()) return 1;

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        printf("Converting IPV6_V6ONLY socket to dual stack...\n");
        if (!socket_listen.socketopt(1)) return 1;

        // listen for connections
        printf("Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // Accept loop
        signal(SIGINT, signal_handler);
        while (running) {
            // accept a connection
            printf("Accepting a connection...\n");
            fflush(stdout);

            struct sockaddr_storage client_address;
            socklen_t client_len = sizeof(client_address);
            Socket socket_client = socket_listen.accept((struct sockaddr*)&client_address, &client_len);
            if (!socket_client.is_valid()) return 1;

            // Spawn a detached thread per client
            if (!running) {
                socket_client.close();
                break;
            }
            std::thread(handle_client, std::move(socket_client), client_address, client_len).detach();
        }

        // close listening socket
        printf("Waiting for clients to finish...\n");
        while (active_clients.load() > 0) {
            printf("[Active clients: %d]\n", active_clients.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        printf("All clients finished.\n");

        printf("Closing listening socket...\n");
        socket_listen.close();

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER_H
