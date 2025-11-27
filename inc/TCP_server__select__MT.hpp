// TCP_server__select__MT.hpp

#ifndef TCP_SERVER__SELECT__MT_HPP
#define TCP_SERVER__SELECT__MT_HPP

#include "Event_Loop__Select__MT.hpp"
#include "Concurrent_Queue_Blocking.hpp"
#include "Thread_Pool__Blocking_Queue.hpp"
#include <ctype.h>
#include <algorithm>

using namespace BA_Concurrency;

namespace BA_Socket {
    inline bool to_up(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return true;
    }

    int TCP_server__select__MT_helper() {
        using EL_t = Event_Loop__Select__MT<
            Concurrent_Queue_Blocking,
            Thread_Pool__Concurrent_Queue_Blocking>;

        // create the server socket and bind to the local address
        PRINTF1("[Server]: Creating socket for the server and binding it to the local address...\n");
        Socket socket_listen = create_socket_bind_to_local_addr("all");
        if (!socket_listen.is_valid()) return 1;
        SOCKET fd_listen = socket_listen.native_handle();

        // listen for connections
        PRINTF1("[Server]: Listening for connections...(Ctrl+C to stop)\n");
        if (!socket_listen.listen(10)) return 1;

        // create the event looop
        EL_t el{};
        el.fd_register(fd_listen, Enum_Event_Types::Read);
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
    
    inline int TCP_server__select__MT() {
        SOCKET_STARTUP();
        int status = TCP_server__select__MT_helper();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP_SERVER__SELECT__MT_HPP
