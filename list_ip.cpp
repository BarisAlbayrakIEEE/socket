// ip_list.c — Portable IPv4/IPv6 interface listing
#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #if defined(WINVER) && (WINVER >= 0x0A00)
            #define _WIN32_WINNT 0x0A00   // Windows 10 or later
        #elif defined(WINVER) && (WINVER >= 0x0603)
            #define _WIN32_WINNT 0x0603   // Windows 8.1
        #elif defined(WINVER) && (WINVER >= 0x0602)
            #define _WIN32_WINNT 0x0602   // Windows 8
        #elif defined(WINVER) && (WINVER >= 0x0601)
            #define _WIN32_WINNT 0x0601   // Windows 7
        #elif defined(WINVER) && (WINVER >= 0x0600)
            #define _WIN32_WINNT 0x0600   // Windows Vista
        #elif defined(WINVER) && (WINVER >= 0x0501)
            #define _WIN32_WINNT 0x0501   // Windows XP
        #else
            #define _WIN32_WINNT 0x0600   // Default: Vista
        #endif
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>   // must come after winsock2.h
    #include <iphlpapi.h>

    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <ifaddrs.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif

#include <stdio.h>
#include <stdlib.h>

int main(void) {

#ifdef _WIN32
    // ---------- Windows Implementation ----------
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }

    DWORD asize = 15000;
    PIP_ADAPTER_ADDRESSES adapters = NULL;
    while (1) {
        adapters = (PIP_ADAPTER_ADDRESSES)malloc(asize);
        if (!adapters) {
            fprintf(stderr, "Allocation failure.\n");
            WSACleanup();
            return 1;
        }

        DWORD result = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_INCLUDE_PREFIX,
            NULL,
            adapters,
            &asize);
        if (result == ERROR_BUFFER_OVERFLOW) {
            free(adapters);
            adapters = NULL;
            continue;
        } else if (result != ERROR_SUCCESS) {
            fprintf(stderr, "GetAdaptersAddresses failed: %lu\n", result);
            free(adapters);
            WSACleanup();
            return 1;
        }
        break;
    }

    for (
        PIP_ADAPTER_ADDRESSES adapter = adapters;
        adapter != NULL;
        adapter = adapter->Next)
    {
        wprintf(L"\nAdapter: %s\n", adapter->FriendlyName);
        for (
            PIP_ADAPTER_UNICAST_ADDRESS ua = adapter->FirstUnicastAddress;
            ua != NULL;
            ua = ua->Next)
        {
            int family = ua->Address.lpSockaddr->sa_family;
            char ip[100];

            getnameinfo(
                ua->Address.lpSockaddr,
                ua->Address.iSockaddrLength,
                ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
            printf("  %-5s : %s\n", (family == AF_INET ? "IPv4" : "IPv6"), ip);
        }
    }
    free(adapters);
    WSACleanup();
#else // ---------- Linux / Unix Implementation ----------
    struct ifaddrs *addresses;
    if (getifaddrs(&addresses) == -1) {
        perror("getifaddrs");
        return 1;
    }

    for (
        struct ifaddrs *addr = addresses;
        addr != NULL;
        addr = addr->ifa_next)
    {
        if (!addr->ifa_addr) continue;

        int family = addr->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            char ip[100];
            getnameinfo(
                addr->ifa_addr,
                (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
            printf(
                "%s\t%-5s\t%s\n",
                addr->ifa_name,
                (family == AF_INET ? "IPv4" : "IPv6"), ip);
        }
    }
    freeifaddrs(addresses);
#endif

    return 0;
}
