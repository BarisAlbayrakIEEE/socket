#ifndef SERVER_WITH_TIMER_H
#define SERVER_WITH_TIMER_H

#include "get_local_addr_to_bind.h"
#include <stdio.h>
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
    void handle_client(SOCKET socket_client, struct sockaddr_storage client_address, socklen_t client_len) {
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
        int bytes_received = recv(socket_client, request, sizeof(request), 0);
        if (bytes_received < 1) {
            fprintf(stderr, "recv() failed. (%d)\n", GET_SOCKET_ERRNO());
            CLOSE_SOCKET(socket_client);
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

        send(socket_client, header, strlen(header), 0);
        send(socket_client, body, strlen(body), 0);
        printf("[Response sent to %s]\n", address_buffer);

        // close client socket (connection)
        printf("Closing client socket (connection)...\n");
        CLOSE_SOCKET(socket_client);
        printf("[Client socket (connection) closed] %s\n", address_buffer);

        --active_clients;
    }

    int server_with_timer(void) {
        SOCKET_STARTUP();

        // configure local address that the web server should bind to
        printf("Configuring local address...\n");
        struct addrinfo *bind_address = get_local_addr_to_bind();

        // create a socket with IPv6 stack to listen for connections
        printf("Creating listening socket...\n");
        SOCKET socket_listen;
        socket_listen = socket(
            bind_address->ai_family,
            bind_address->ai_socktype,
            bind_address->ai_protocol);
        if (!IS_VALID_SOCKET(socket_listen)) {
            fprintf(stderr, "socket() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }

        // convert IPV6_V6ONLY socket to dual stack.
        // disable IPV6_V6ONLY to accept both IPv4 and IPv6
        printf("Converting IPV6_V6ONLY socket to dual stack...\n");
        int option = 1; // reuse address option
        if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, (void*)&option, sizeof(option))) {
            fprintf(stderr, "setsockopt() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }

        // bind listening socket to local address
        printf("Binding listening socket to local address...\n");
        if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
            fprintf(stderr, "bind() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }
        freeaddrinfo(bind_address);

        // listen for connections
        printf("Listening for connections...\n");
        if (listen(socket_listen, 10) < 0) {
            fprintf(stderr, "listen() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }
        printf("Server is listening... (Ctrl+C to stop)\n");

        // Accept loop
        signal(SIGINT, signal_handler);
        while (running) {
            // accept a connection
            printf("Accepting a connection...\n");
            fflush(stdout);

            struct sockaddr_storage client_address;
            socklen_t client_len = sizeof(client_address);
            SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
            if (!IS_VALID_SOCKET(socket_client)) {
                fprintf(stderr, "accept() failed. (%d)\n", GET_SOCKET_ERRNO());
                continue; // try again
            }

            // Spawn a detached thread per client
            if (!running) {
                CLOSE_SOCKET(socket_client);
                break;
            }
            std::thread(handle_client, socket_client, client_address, client_len).detach();
        }

        // close listening socket
        printf("Waiting for clients to finish...\n");
        while (active_clients.load() > 0) {
            printf("[Active clients: %d]\n", active_clients.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        printf("All clients finished.\n");

        printf("Closing listening socket...\n");
        CLOSE_SOCKET(socket_listen);
        printf("Closed listening socket...\n");

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER_H
