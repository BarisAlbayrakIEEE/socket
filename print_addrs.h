#ifndef PRINT_ADDRS_H
#define PRINT_ADDRS_H

#include "get_uniform_addrs.h"

using namespace ba_socket;

int print_addrs(void) {
    SOCKET_STARTUP();

    // get the addrs
    Uniform_Addr uniform_addrs[100];
    size_t count = get_uniform_addrs(uniform_addrs, 100);
    if (!count) {
        printf("No addresses found.\n");
        SOCKET_CLEANUP();
        return 1;
    }

    // print the addrs
    for (size_t i = 0; i < count; ++i) {
        char ip[100];
        getnameinfo(
            uniform_addrs[i].sock_addr,
            uniform_addrs[i].addr_len,
            ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
        printf(
            "%s\t%-5s\t%s\n",
            uniform_addrs[i].name,
            uniform_addrs[i].sock_addr->sa_family == AF_INET ? "IPv4" : "IPv6",
            ip);
    }

    SOCKET_CLEANUP();
    return 0;
}

#endif // PRINT_ADDRS_H
