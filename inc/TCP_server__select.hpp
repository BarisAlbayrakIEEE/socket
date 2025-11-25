// TCP_server__select.hpp

#ifndef TCP_SERVER__SELECT_HPP
#define TCP_SERVER__SELECT_HPP

#include "Event_Loop__Select.hpp"
#include <ctype.h>
#include <algorithm>

namespace BA_Socket {
    void to_up(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    }

    int TCP_server__select_helper() {
        // create the server socket and bind to the local address
        PRINTF1("[Server]: Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen = create_socket_bind_to_local_addr("all");
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("[Server]: Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // create the event looop
        Event_Loop__Select el{};
        el.fd_register(socket_listen.native_handle(), Enum_Event_Types::Read);
#ifdef SEPARATE_READ_WRITE
        el.add_handler(
            std::make_unique<Handler_Accept<Handler_Read_Transform<string_transform_t, Handler_Write>>>(fd_listen, &to_up),
            Enum_Event_Types::Read);
#else
        el.add_handler(
            std::make_unique<Handler_Accept<Handler_Read_Transform_Write<string_transform_t>>>(fd_listen, &to_up),
            Enum_Event_Types::Read);
#endif
        el.run();

        return 0;
    }
    
    inline int TCP_server() {
        SOCKET_STARTUP();
        int status = TCP_server__select_helper();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP_SERVER__SELECT_HPP
