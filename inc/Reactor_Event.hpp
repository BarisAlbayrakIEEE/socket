// Reactor_Event.hpp

#ifndef REACTOR_EVENT_HPP
#define REACTOR_EVENT_HPP

#include <memory>
#include <vector>
#include "IEvent_Loop.hpp"

namespace BA_Socket {
    struct IEvent_Handler;
    using event_handler_ptr_t = std::unique_ptr<IEvent_Handler>;

    struct Reactor_Event{
        int _fd{-1};
        Enum_Register_Types _register_type{ Enum_Register_Types::None };
        Enum_IO_Event_Types _event_type{ Enum_IO_Event_Types::None };
        Enum_Handler_Command_Types _handler_command_type{ Enum_Handler_Command_Types::None };
        event_handler_ptr_t _handler_ptr{ nullptr };

        Reactor_Event() = default;
        Reactor_Event(
            int fd,
            Enum_Register_Types register_type,
            Enum_IO_Event_Types event_type,
            Enum_Handler_Command_Types handler_command_type,
            event_handler_ptr_t&& handler_ptr)
            :
            _fd(fd),
            _register_type(register_type),
            _event_type(event_type),
            _handler_command_type(handler_command_type),
            _handler_ptr(std::move(handler_ptr)) {};

        Reactor_Event(Reactor_Event&&) noexcept = default;
        Reactor_Event& operator=(Reactor_Event&&) noexcept = default;
        Reactor_Event(const Reactor_Event&) = delete;
        Reactor_Event& operator=(const Reactor_Event&) = delete;
        ~Reactor_Event() noexcept = default;
    };

    using reactor_event_pack_t = std::vector<Reactor_Event>;
} // namespace BA_Socket

#endif // REACTOR_EVENT_HPP
