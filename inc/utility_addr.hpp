// utility_addr.hpp

#ifndef UTILITY_ADDR_HPP
#define UTILITY_ADDR_HPP

#include "socket_setup.hpp"
#include <cstring>
#ifdef _WIN32
    #include <iphlpapi.h>
#else
    #include <ifaddrs.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <string>

namespace BA_Socket {
    inline struct addrinfo* get_addrinfo(
        int socktype = SOCK_STREAM,
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_UNSPEC,
        int flags = AI_PASSIVE)
    {
        PRINTF1("Obtaining the address info...\n");
        std::string port_str(std::to_string(port));
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = socktype;
        hints.ai_family = family;
        hints.ai_flags = flags;
        struct addrinfo *socket_addr;
        if (hostname.empty()) {
            if (::getaddrinfo(nullptr, port_str.c_str(), &hints, &socket_addr)) {
                SOCKET_ERROR__GETADDRINFO();
                return nullptr;
            }
        }
        else {
            if (::getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &socket_addr)) {
                SOCKET_ERROR__GETADDRINFO();
                return nullptr;
            }
        }
        return socket_addr;
    }

    template <bool Is_Debug_Mode>
    inline void print_sockaddr(struct sockaddr_storage sockaddr_, socklen_t socklen) {
        // print the peer address
        PRINTF1("Printing the socket address...\n");
        char buffer_addr[256];
        char buffer_service[256];
        ::getnameinfo(
            (struct sockaddr*)&sockaddr_,
            socklen,
            buffer_addr,
            sizeof(buffer_addr),
            buffer_service,
            sizeof(buffer_service),
            NI_NUMERICHOST);
        PRINTF3("%s %s\n", buffer_addr, buffer_service);
    }
    template <>
    inline void print_sockaddr<false>(struct sockaddr_storage sockaddr_, socklen_t socklen) {
        ;
    }

    template <bool Is_Debug_Mode>
    inline void print_addrinfo(struct addrinfo* addrinfo_) {
        // print the peer address
        PRINTF1("Printing the address info...\n");
        char buffer_addr[256];
        char buffer_service[256];
        ::getnameinfo(
            addrinfo_->ai_addr,
            addrinfo_->ai_addrlen,
            buffer_addr,
            sizeof(buffer_addr),
            buffer_service,
            sizeof(buffer_service),
            NI_NUMERICHOST);
        PRINTF3("%s %s\n", buffer_addr, buffer_service);
    }
    template <>
    inline void print_addrinfo<false>(struct addrinfo* addrinfo_) {
        ;
    }

    // A uniform interface for network address information
    typedef struct Uniform_Addr {
        char name[128];
        unsigned int flags;
        struct sockaddr *sock_addr;
        socklen_t addr_len;
    } Uniform_Addr;

#ifdef _WIN32
    // get uniform address - windows
    size_t get_local_addrs_uniform(Uniform_Addr *uniform_addrs, size_t max) {
        DWORD asize = 15000;
        PIP_ADAPTER_ADDRESSES adapter_addrs = NULL;
        while (1) {
            adapter_addrs = (PIP_ADAPTER_ADDRESSES)malloc(asize);
            if (!adapter_addrs) {
                SOCKET_ERROR__ALLOC();
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
                SOCKET_ERROR__GETADAPTERSADDRESSES();
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
    size_t get_local_addrs_uniform(Uniform_Addr *uniform_addrs, size_t max) {
        PRINTF1("Obtaining all addresses...\n");

        struct ifaddrs *ifaddrs_;
        if (getifaddrs(&ifaddrs_) == -1) {
            SOCKET_ERROR__GETIFADDRS();
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

    inline std::string get_ip_of_interface(const std::string& ifname) {
        PRINTF1("Obtaining the address matched by name...\n");

        Uniform_Addr uniform_addrs[100];
        size_t count = get_local_addrs_uniform(uniform_addrs, 100);
        for (size_t i = 0; i < count; ++i) {
            if (ifname == uniform_addrs[i].name) {
                char host[NI_MAXHOST];
                if (
                    getnameinfo(
                        uniform_addrs[i].sock_addr,
                        uniform_addrs[i].addr_len,
                        host,
                        NI_MAXHOST,
                        nullptr,
                        0,
                        NI_NUMERICHOST) == 0)
                {
                    return std::string(host);
                }
            }
        }
        SOCKET_ERROR__GETIFADDRS();
        return std::string();
    }

    inline struct addrinfo* get_local_addr_to_bind(
        const std::string& bind_mode,
        uint16_t port = 8080,
        int family = AF_INET6,
        int socktype = SOCK_STREAM,
        int flags = AI_PASSIVE)
    {
        PRINTF1("Obtaining the local address to bind...\n");

        std::string node;
        if (bind_mode == "all" || bind_mode == "any") {
            node.clear(); // == nullptr
        }
        else if (bind_mode == "localhost") {
            node = "127.0.0.1";
            if (family == AF_INET6) node = "::1";
        }
        else {
            node = get_ip_of_interface(bind_mode); // e.g. "ech0"
            if (node.empty()) return nullptr;
        }

        struct addrinfo hints{};
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = family;
        hints.ai_socktype = socktype;
        hints.ai_flags    = flags;

        // obtain the peer address
        struct addrinfo *bind_addr = get_addrinfo(socktype, node, port, family, flags);
        return bind_addr;
    }
} // namespace BA_Socket

#endif // UTILITY_ADDR_HPP
