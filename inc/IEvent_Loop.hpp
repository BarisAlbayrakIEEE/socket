// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include "core.hpp"

namespace BA_Socket {
    // Event loop interface
    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int, Enum_IO_Event_Types) = 0;
        virtual void fd_unregister(int, Enum_IO_Event_Types) = 0;
        virtual void add_event_handler(int, event_handler_ptr_t&&, Enum_IO_Event_Types) = 0;
        virtual void remove_event_handler(int, Enum_IO_Event_Types) = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };

    template <
        Enum_Event_Loop_Types Event_Loop_Type,
        Enum_Concurrency_Types Concurrency_Type,
        typename... Args>
    class Event_Loop{};
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
