#ifndef GET_LOCAL_ADDR_TO_BIND_H
#define GET_LOCAL_ADDR_TO_BIND_H

#include "socket_setup.h"
#include <cstring>
#include <stdio.h>

namespace ba_socket {
    struct addrinfo* get_local_addr_to_bind(void) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo *bind_address;
        int status = getaddrinfo(NULL, "8080", &hints, &bind_address);
        if(status) {
            fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(status));
            return nullptr;
        }
        return bind_address;
    }
} // namespace ba_socket

#endif // GET_LOCAL_ADDR_TO_BIND_H
