#ifndef PRINT_ADDRS_H
#define PRINT_ADDRS_H

#include "utility_addr.h"

namespace ba_socket {
    int print_addrs(void) {
        SOCKET_STARTUP();

        // get the addrs
        Uniform_Addr uniform_addrs[100];
        size_t count = get_local_addrs_uniform(uniform_addrs, 100);
        if (!count) {
            PRINTF1("No addresses found.\n");
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
            PRINTF4(
                "%s\t%-5s\t%s\n",
                uniform_addrs[i].name,
                uniform_addrs[i].sock_addr->sa_family == AF_INET ? "IPv4" : "IPv6",
                ip);
        }

        SOCKET_CLEANUP();
        return 0;
    }
} // namespace ba_socket

#endif // PRINT_ADDRS_H
