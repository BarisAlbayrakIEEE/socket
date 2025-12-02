// TCP__Client__Low__ST.hpp

#ifndef TCP__CLIENT__LOW__ST_HPP
#define TCP__CLIENT__LOW__ST_HPP

#include "Socket.hpp"
#include "Event_Loop__Low__ST.hpp"
#include "aux_functions.hpp"
#include <ctype.h>

namespace BA_Socket {
    int TCP__Client__Low__ST_helper(
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
        Event_Loop__Low__ST el{ 0, 100000 };
        el.fd_register(0, Enum_IO_Event_Types::Read);
        el.add_event_handler(
            0,
            std::make_unique<Handler_Read_Redirect>(std::vector<int>{ fd_peer }),
            Enum_IO_Event_Types::Read);

        el.fd_register(fd_peer, Enum_IO_Event_Types::Read);
        el.add_event_handler(
            fd_peer,
            std::make_unique<Handler_Read_Forward<string_forward_t>>(&write_to_stdout),
            Enum_IO_Event_Types::Read);
        el.run();

        return 0;
    }

    inline int TCP__Client__Low__ST(
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_INET6)
    {
        SOCKET_STARTUP();
        int status = TCP__Client__Low__ST_helper(hostname, port, family);
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP__CLIENT__LOW__ST_HPP
