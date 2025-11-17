#ifndef SERVER_WITH_TIMER__HANDLE_CLIENT_H
#define SERVER_WITH_TIMER__HANDLE_CLIENT_H

#include "Socket.h"
#include "utility_addr.h"
#include <atomic>
#include <time.h>

namespace ba_socket {

    // Worker thread: handles a single client connection
    std::atomic<int> active_clients{0};
    void handle_client(Socket&& socket_client, struct sockaddr_storage client_addr, socklen_t client_len) {
        ++active_clients;
        PRINTF2("[Active clients: %d]\n", active_clients.load());

        // print the client address
        print_sockaddr<is_debug_mode>(client_addr, client_len);

        // Read the request
        PRINTF1("Reading the request...\n");
        fflush(stdout);

        char request[1024];
        int bytes_received = static_cast<int>(socket_client.recv(request, sizeof(request)));
        if (bytes_received < 1) {
            --active_clients;
            return;
        }
        fflush(stdout);

        // send the response
        PRINTF1("Sending the response...\n");

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
        PRINTF1("Closing client socket (connection)...\n");
        socket_client.close();

        --active_clients;
    }
} // namespace ba_socket

#endif // SERVER_WITH_TIMER__HANDLE_CLIENT_H
