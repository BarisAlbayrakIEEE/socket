// TCP_server.hpp

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "Event_Loop__Select.hpp"
#include <ctype.h>

namespace BA_Socket {
    int TCP_server_helper() {
        // create the server socket and bind to the local address
        PRINTF1("[Server]: Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen{ create_socket_bind_to_local_addr("all") };
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("[Server]: Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // on_read
        auto on_read = Callback(
            [&](int fd_listen){
                // accept a new connection
                PRINTF1("[Server]: Accepting a new connection...\n");
                fflush(stdout);
                struct sockaddr_storage client_addr;
                socklen_t client_len = sizeof(client_addr);
                SOCKET fd_client = ::accept(
                    fd_listen,
                    (struct sockaddr*) &client_addr,
                    &client_len);
                if (!IS_VALID_SOCKET(fd_client)) {
                    if (GET_SOCKET_ERRNO() == EINTR)
                        return Reactor_Command_Pack(
                            std::vector<rc_t>{
                                rc_t(Enum_Reactor_Command_Types::eintr, -1)}); // Ctrl+C pressed
                    SOCKET_ERROR__ACCEPT();
                    return Reactor_Command_Pack(
                        std::vector<rc_t>{
                            rc_t(Enum_Reactor_Command_Types::Error, -1)});
                }
                print_sockaddr<is_debug_mode>(client_addr, client_len);

                // add the new client socket to the master set
                return Reactor_Command_Pack(
                    std::vector<rc_t>{
                        rc_t(Enum_Reactor_Command_Types::RegisterWrite, fd_client)});
        });

        // on_read
        auto on_write = Callback(
            [&](int fd_client){
                // receive data from the client
                char read[1024];
                int bytes_received = ::recv(fd_client, read, 1024, 0);
                PRINTF1("[Server]: Receiving data from client...\n");
                if (bytes_received < 1) {
                    CLOSE_SOCKET(fd_client);
                    auto rcp = Reactor_Command_Pack(
                        std::vector<rc_t>{
                            rc_t(Enum_Reactor_Command_Types::Unregister, fd_client)});
                    if (GET_SOCKET_ERRNO() == EINTR)
                        rcp._rcs.push_back(rc_t(Enum_Reactor_Command_Types::Error, -1));
                    rcp._rcs.push_back(rc_t(Enum_Reactor_Command_Types::eintr, -1));
                    return rcp;
                }
                PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

                // convert to uppercase and send back
                PRINTF1("[Server]: Updating data before ending to client...\n");
                int j;
                for (j = 0; j < bytes_received; ++j)
                    read[j] = toupper(read[j]);
                ::send(fd_client, read, bytes_received, 0);
                PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);

                return Reactor_Command_Pack({});
        });

        // create the event looop
        Event_Loop__Select el{ on_read, on_write };
        el.fd_register(socket_listen.native_handle(), Enum_Event_Types::Read);
        el.run();

        return 0;
    }
    
    inline int TCP_server() {
        SOCKET_STARTUP();
        int status = TCP_server_helper();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP_SERVER_HPP
