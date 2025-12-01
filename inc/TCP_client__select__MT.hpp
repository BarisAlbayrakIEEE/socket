// TCP_client__select__MT.hpp

#ifndef TCP_CLIENT__SELECT__MT_HPP
#define TCP_CLIENT__SELECT__MT_HPP

#include "Socket.hpp"
#include "Event_Loop__Select__MT.hpp"
#include <ctype.h>

using namespace BA_Concurrency;

namespace BA_Socket {
    inline bool write_to_stdout(const std::string& buffer) {
        printf("[Client]: Received (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.c_str());
        return true;
    }

    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires CEL<Concurrent_Queue_Type, Thread_Pool_Type, Job, job_result_t>
    int TCP_client__select__MT_helper(
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_INET6)
    {
        using EL_t = Event_Loop__Select__MT<
            Concurrent_Queue_Type,
            Thread_Pool_Type>;

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
        EL_t el{ 0, 100000, std::thread::hardware_concurrency() };
        el.fd_register(0, Enum_Event_Types::Read);
        el.add_handler(
            0,
            std::make_unique<Handler_Read_Redirect>(std::vector<int>{ fd_peer }),
            Enum_Event_Types::Read);

        el.fd_register(fd_peer, Enum_Event_Types::Read);
        el.add_handler(
            fd_peer,
            std::make_unique<Handler_Read_Forward<string_forward_t>>(&write_to_stdout),
            Enum_Event_Types::Read);
        el.run();

        return 0;
    }
    
    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires CEL<Concurrent_Queue_Type, Thread_Pool_Type, Job, job_result_t>
    inline int TCP_client__select__MT(
        const std::string& hostname = "localhost",
        uint16_t port = 8080,
        int family = AF_INET6)
    {
        SOCKET_STARTUP();
        int status = TCP_client__select__MT_helper<Concurrent_Queue_Type, Thread_Pool_Type>(hostname, port, family);
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP_CLIENT__SELECT__MT_HPP
