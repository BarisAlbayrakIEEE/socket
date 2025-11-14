#ifndef SERVER_WITH_TIMER_H
#define SERVER_WITH_TIMER_H

#include "get_local_addr_to_bind.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace ba_socket {
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
        int option = 0;
        if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&option, sizeof(option))) {
            fprintf(stderr, "setsockopt() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }

        // bind listening socket to local address
        printf("Binding listening socket to local address...\n");
        if (
            bind(
                socket_listen,
                bind_address->ai_addr,
                bind_address->ai_addrlen))
        {
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

        // accept a connection
        printf("Accepting a connection...\n");
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);
        SOCKET socket_client = accept(
            socket_listen,
            (struct sockaddr*) &client_address,
            &client_len);
        if (!IS_VALID_SOCKET(socket_client)) {
            fprintf(stderr, "accept() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }

        // print client address
        printf("Printing client address... ");
        char address_buffer[100];
        getnameinfo(
            (struct sockaddr*)&client_address,
            client_len,
            address_buffer,
            sizeof(address_buffer),
            NULL,
            0,
            NI_NUMERICHOST);
        printf("%s\n", address_buffer);

        // read a request
        printf("Reading a request...\n");
        char request[1024];
        int bytes_received = recv(socket_client, request, 1024, 0);
        if (bytes_received < 1) {
            fprintf(stderr, "recv() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }
        printf("%.*s", bytes_received, request);

        // send a response
        printf("Sending a response...\n");
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Local time is: ";
        int bytes_sent = send(socket_client, response, strlen(response), 0);
        if (bytes_sent != (int)strlen(response)) {
            fprintf(stderr, "send() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }

        // send time
        time_t timer;
        time(&timer);
        char *time_msg = ctime(&timer);
        bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
        if (bytes_sent != (int)strlen(time_msg)) {
            fprintf(stderr, "send() failed. (%d)\n", GET_SOCKET_ERRNO());
            return 1;
        }

        // close client socket (connection)
        printf("Closing client socket (connection)...\n");
        CLOSE_SOCKET(socket_client);

        // close listening socket
        printf("Closing listening socket...\n");
        CLOSE_SOCKET(socket_listen);

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER_H
