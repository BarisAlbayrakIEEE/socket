// TCP_client__select.hpp

#ifndef TCP_CLIENT__SELECT_HPP
#define TCP_CLIENT__SELECT_HPP

#include "Event_Loop__Select.hpp"
#include <ctype.h>
#include <algorithm>

namespace BA_Socket {
    void write_to_stdout(const std::string& str) {
        ;
    }

    int TCP_client__select_helper(
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_INET6)
    {
        // obtain the peer address
        struct addrinfo *peer_addr = get_addrinfo(SOCK_STREAM, hostname, port, family);
        if (!peer_addr) return 1;

        // print the peer address
        print_addrinfo<is_debug_mode>(peer_addr);

        // create the peer socket
        PRINTF1("[Client]: Creating the peer socket...\n");
        Socket socket_peer {
            peer_addr->ai_family,
            peer_addr->ai_socktype,
            peer_addr->ai_protocol};
        if (!socket_peer.is_valid()) {
            SOCKET_ERROR__SOCKET();
            ::freeaddrinfo(peer_addr);
            return 1;
        }
        SOCKET fd_peer = socket_peer.native_handle();

        // connect to the remote server
        PRINTF1("[Client]: Connecting to the remote server...\n");
        if (socket_peer.connect(peer_addr->ai_addr, peer_addr->ai_addrlen) < 0) {
            ::freeaddrinfo(peer_addr);
            return 1;
        }
        ::freeaddrinfo(peer_addr);

        // inform the user
        PRINTF1("[Client]: Connected to the remote server.\n");
        PRINTF1("[Client]: To send data, enter text followed by enter.\n");

        // create the event looop
        Event_Loop__Select el{ 0, 100000 };
        el.add_stdin_to_reads();
        el.fd_register(fd_peer, Enum_Event_Types::Read);
        el.add_handler(
            std::make_unique<Handler_Read_Forward<string_forward_t>>(fd_peer, &write_to_stdout),
            Enum_Event_Types::Read);
        el.fd_register(fd_peer, Enum_Event_Types::Write);
        el.add_handler(
            std::make_unique<Handler_Read_Redirect>(0, std::vector<int>{ fd_peer }),
            Enum_Event_Types::Read);
        el.run();

        return 0;
    }
    
    inline int TCP_client__select(
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_INET6)
    {
        SOCKET_STARTUP();
        int status = TCP_client__select_helper(hostname, port, family);
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP_CLIENT__SELECT_HPP
