// TCP__Server__Low__HP.hpp

#ifndef TCP__SERVER__LOW__HP_HPP
#define TCP__SERVER__LOW__HP_HPP

#include "Socket.hpp"
#include "Event_Loop__Low__HP.hpp"
#include "aux_functions.hpp"
#include <ctype.h>
#include <algorithm>

using namespace BA_Concurrency;

namespace BA_Socket {
    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires CEL<Concurrent_Queue_Type, Thread_Pool_Type, Job, job_result_t>
    int TCP__Server__Low__HP_helper() {
        using EL_t = Event_Loop__Low__HP<
            Concurrent_Queue_Type,
            Thread_Pool_Type>;

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
        el.fd_register(fd_listen, Enum_IO_Event_Types::Read);
#ifdef SEPARATE_READ_WRITE
        el.add_event_handler(
            fd_listen,
            std::make_unique<Handler_Accept<Handler_Read_Transform<string_transform_t, Handler_Write>>>(&to_up),
            Enum_IO_Event_Types::Read);
#else
        el.add_event_handler(
            fd_listen,
            std::make_unique<Handler_Accept<Handler_Read_Transform_Write<string_transform_t>>>(&to_up),
            Enum_IO_Event_Types::Read);
#endif
        el.run();

        return 0;
    }
    
    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires CEL<Concurrent_Queue_Type, Thread_Pool_Type, Job, job_result_t>
    inline int TCP__Server__Low__HP() {
        SOCKET_STARTUP();
        int status = TCP__Server__Low__HP_helper<Concurrent_Queue_Type, Thread_Pool_Type>();
        SOCKET_CLEANUP();
        return status;
    }
} // namespace BA_Socket

#endif // TCP__SERVER__LOW__HP_HPP
