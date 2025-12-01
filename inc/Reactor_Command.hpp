// Reactor_Command.hpp

#ifndef REACTOR_COMMAND_HPP
#define REACTOR_COMMAND_HPP

#include <memory>
#include <vector>
#include "core.hpp"

namespace BA_Socket {
    struct IHandler;
    using handler_ptr_t = std::unique_ptr<IHandler>;

    struct Reactor_Command{
        int _fd{-1};
        Enum_Register_Types _register_type{ Enum_Register_Types::None };
        Enum_Event_Types _event_type{ Enum_Event_Types::None };
        Enum_Handler_Command_Types _handler_command_type{ Enum_Handler_Command_Types::None };
        handler_ptr_t _handler_ptr{ nullptr };

        Reactor_Command() = default;
        Reactor_Command(
            int fd,
            Enum_Register_Types register_type,
            Enum_Event_Types event_type,
            Enum_Handler_Command_Types handler_command_type,
            handler_ptr_t&& handler_ptr)
            :
            _fd(fd),
            _register_type(register_type),
            _event_type(event_type),
            _handler_command_type(handler_command_type),
            _handler_ptr(std::move(handler_ptr)) {};

        Reactor_Command(Reactor_Command&&) noexcept = default;
        Reactor_Command& operator=(Reactor_Command&&) noexcept = default;
        Reactor_Command(const Reactor_Command&) = delete;
        Reactor_Command& operator=(const Reactor_Command&) = delete;
        ~Reactor_Command() noexcept = default;
    };

    using reactor_command_pack_t = std::vector<Reactor_Command>;
} // namespace BA_Socket

#endif // REACTOR_COMMAND_HPP
