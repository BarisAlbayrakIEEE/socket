#ifndef GET_LOCAL_ADDR_TO_BIND_H
#define GET_LOCAL_ADDR_TO_BIND_H

#include "get_local_addrs_uniform.h"
#include <string>

namespace ba_socket {
    inline std::string get_ip_of_interface(const std::string& ifname) {
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
        int socktype = SOCK_STREAM)
    {
        std::string node;
        if (bind_mode == "all" || bind_mode == "any") {
            node.clear(); // == nullptr
        }
        else if (bind_mode == "localhost") {
            node = "127.0.0.1";
            if (family == AF_INET6) node = "::1";
        }
        else {
            node = get_ip_of_interface(bind_mode); // e.g. "eth0"
            if (node.empty()) return nullptr;
        }

        struct addrinfo hints{};
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = family;
        hints.ai_socktype = socktype;
        hints.ai_flags    = AI_PASSIVE;

        std::string port_str(std::to_string(port));
        struct addrinfo* bind_address = nullptr;
        int status = getaddrinfo(
            node.empty() ? nullptr : node.c_str(),
            port_str.c_str(),
            &hints,
            &bind_address);
        if (status != 0) {
            SOCKET_ERROR__GETADDRINFO();
            return nullptr;
        }

        return bind_address;
    }
} // namespace ba_socket

#endif // GET_LOCAL_ADDR_TO_BIND_H
