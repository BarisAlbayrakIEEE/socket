#ifndef GET_UNIFORM_ADDRS_H
#define GET_UNIFORM_ADDRS_H

#include "socket_setup.h"
#ifdef _WIN32
    #include <iphlpapi.h>
#else
    #include <ifaddrs.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

namespace ba_socket {
    // A uniform interface for network address information
    typedef struct Uniform_Addr {
        char name[128];
        unsigned int flags;
        struct sockaddr *sock_addr;
        socklen_t addr_len;
    } Uniform_Addr;

    #ifdef _WIN32
        // get uniform address - windows
        size_t get_uniform_addrs(Uniform_Addr *uniform_addrs, size_t max) {
            DWORD asize = 15000;
            PIP_ADAPTER_ADDRESSES adapter_addrs = NULL;
            while (1) {
                adapter_addrs = (PIP_ADAPTER_ADDRESSES)malloc(asize);
                if (!adapter_addrs) {
                    fprintf(stderr, "Allocation failure.\n");
                    return 0;
                }

                DWORD result = GetAdaptersAddresses(
                    AF_UNSPEC,
                    GAA_FLAG_INCLUDE_PREFIX,
                    NULL,
                    adapter_addrs,
                    &asize);
                if (result == ERROR_BUFFER_OVERFLOW) {
                    free(adapter_addrs);
                    adapter_addrs = NULL;
                    continue;
                } else if (result != ERROR_SUCCESS) {
                    fprintf(stderr, "GetAdaptersAddresses failed: %lu\n", result);
                    free(adapter_addrs);
                    return 0;
                }
                break;
            }

            size_t count = 0;
            for (
                PIP_ADAPTER_ADDRESSES it = adapter_addrs;
                it && count < max;
                it = it->Next)
            {
                for (
                    PIP_ADAPTER_UNICAST_ADDRESS adapter_unicast_addr = it->FirstUnicastAddress;
                    adapter_unicast_addr && count < max;
                    adapter_unicast_addr = adapter_unicast_addr->Next)
                {
                    Uniform_Addr *uniform_addr = &uniform_addrs[count++];
                    memset(uniform_addr, 0, sizeof(*uniform_addr));

                    wcstombs(
                        uniform_addr->name,
                        it->FriendlyName,
                        sizeof(uniform_addr->name));
                    uniform_addr->flags = it->Flags;
                    uniform_addr->sock_addr = adapter_unicast_addr->Address.lpSockaddr;
                    uniform_addr->addr_len = adapter_unicast_addr->Address.iSockaddrLength;
                }
            }
            free(adapter_addrs);
            return count;
        }
    #else
        // get uniform address - linux/unix
        size_t get_uniform_addrs(Uniform_Addr *uniform_addrs, size_t max) {
            struct ifaddrs *ifaddrs_;
            if (getifaddrs(&ifaddrs_) == -1) {
                perror("getifaddrs");
                return 0;
            }

            size_t count = 0;
            for (
                struct ifaddrs *it = ifaddrs_;
                it && count < max;
                it = it->ifa_next)
            {
                if (!it->ifa_addr) continue;
                int family = it->ifa_addr->sa_family;
                if (family != AF_INET && family != AF_INET6) continue;

                Uniform_Addr *uniform_addr = &uniform_addrs[count++];
                memset(uniform_addr, 0, sizeof(*uniform_addr));

                strncpy(uniform_addr->name, it->ifa_name, sizeof(uniform_addr->name));
                uniform_addr->flags = it->ifa_flags;
                uniform_addr->sock_addr = it->ifa_addr;
                uniform_addr->addr_len =
                    (family == AF_INET) ?
                    sizeof(struct sockaddr_in) :
                    sizeof(struct sockaddr_in6);
            }
            freeifaddrs(ifaddrs_);
            return count;
        }
    #endif
} // namespace ba_socket

#endif // GET_UNIFORM_ADDRS_H
