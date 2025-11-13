#include "get_uniform_addrs.h"

using namespace ba_socket;

int main(void) {
    SOCKET_STARTUP();

    Uniform_Addr uniform_addrs[100];
    size_t count = get_uniform_addrs(uniform_addrs, 100);
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
