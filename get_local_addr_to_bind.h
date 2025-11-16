#ifndef GET_LOCAL_ADDR_TO_BIND_H
#define GET_LOCAL_ADDR_TO_BIND_H

#include "socket_setup.h"
#include <cstring>
#include <string>
#include <stdexcept>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace ba_socket {
    inline std::string get_ip_of_interface(const std::string& ifname) {
        struct ifaddrs* ifaddr;
        if (getifaddrs(&ifaddr) == -1) {
            SOCKET_ERROR__GETIFADDRS();
            return "";
        }

        std::string ip;
        for (auto* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) continue;
            if (ifa->ifa_addr->sa_family == AF_INET && ifname == ifa->ifa_name) {
                char host[NI_MAXHOST];
                if (
                    getnameinfo(
                        ifa->ifa_addr,
                        sizeof(struct sockaddr_in),
                        host,
                        NI_MAXHOST,
                        nullptr,
                        0,
                        NI_NUMERICHOST) == 0)
                {
                    ip = host;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
        if (ip.empty())
            throw std::runtime_error("Interface " + ifname + " has no IPv4/IPv6 address or does not exist");
        return ip;
    }

    inline struct addrinfo* get_local_addr_to_bind(
        const std::string& bind_mode,
        uint16_t port = 8080,
        int family = AF_INET6,
        int socktype = SOCK_STREAM)
    {
        struct addrinfo hints{};
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = family;
        hints.ai_socktype = socktype;
        hints.ai_flags    = AI_PASSIVE;

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
        }

        struct addrinfo* bind_address = nullptr;
        std::string port_str(std::to_string(port));
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
