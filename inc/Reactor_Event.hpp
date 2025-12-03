// Reactor_Event.hpp

#ifndef REACTOR_EVENT_HPP
#define REACTOR_EVENT_HPP

#include <vector>
#include "IEvent_Loop.hpp"

namespace BA_Socket {
    struct Reactor_Event{
        int _fd{-1};
        Enum_Register_Types _register_type{ Enum_Register_Types::None };
        Enum_IO_Event_Types _IO_event_type{ Enum_IO_Event_Types::None };
        Enum_Event_Handler_Action_Types _event_handler_action_type{ Enum_Event_Handler_Action_Types::None };
        event_handler_ptr_t _event_handler_ptr{ nullptr };

        Reactor_Event() = default;
        Reactor_Event(
            int fd,
            Enum_Register_Types register_type,
            Enum_IO_Event_Types event_type,
            Enum_Event_Handler_Action_Types event_handler_action_type,
            event_handler_ptr_t&& event_handler_ptr)
            :
            _fd(fd),
            _register_type(register_type),
            _IO_event_type(event_type),
            _event_handler_action_type(event_handler_action_type),
            _event_handler_ptr(std::move(event_handler_ptr)) {};

        Reactor_Event(Reactor_Event&&) noexcept = default;
        Reactor_Event& operator=(Reactor_Event&&) noexcept = default;
        Reactor_Event(const Reactor_Event&) = delete;
        Reactor_Event& operator=(const Reactor_Event&) = delete;
        ~Reactor_Event() noexcept = default;
    };

    using reactor_event_pack_t = std::vector<Reactor_Event>;
} // namespace BA_Socket

#endif // REACTOR_EVENT_HPP
