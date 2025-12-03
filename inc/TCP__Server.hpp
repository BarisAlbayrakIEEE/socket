// TCP__Server.hpp

#ifndef TCP__SERVER_HPP
#define TCP__SERVER_HPP

#include "Socket.hpp"
#include "Event_Loop__Factory.hpp"
#include "aux_functions.hpp"
#include <ctype.h>
#include <algorithm>

using namespace BA_Concurrency;

namespace BA_Socket {
    template <typename Event_Loop_Type>
    int TCP__Server__helper() {
        // create the server socket and bind to the local address
        PRINTF1("[Server]: Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen = create_socket_bind_to_local_addr("all");
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("[Server]: Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // create the event looop
        Event_Loop_Type event_loop{ Event_Loop__Factory<Event_Loop_Type>::create() };
        event_loop.fd_register(fd_listen, Enum_IO_Event_Types::Read);
#ifdef SEPARATE_READ_WRITE
        event_loop.add_event_handler(
            fd_listen,
            std::make_unique<Event_Handler_Accept<Event_Handler_Read_Transform<string_transform_t, Event_Handler_Write__Once>>>(&to_up),
            Enum_IO_Event_Types::Read);
#else
        event_loop.add_event_handler(
            fd_listen,
            std::make_unique<Event_Handler_Accept<Event_Handler_Read_Transform_Write<string_transform_t>>>(&to_up),
            Enum_IO_Event_Types::Read);
#endif
        event_loop.run();

        return 0;
    }
    
    template <typename Event_Loop_Type>
    inline int TCP__Server() {
        SOCKET_STARTUP();
        int status = TCP__Server__helper<Event_Loop_Type>();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP__SERVER_HPP
