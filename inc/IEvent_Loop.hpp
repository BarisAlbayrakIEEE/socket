// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <unordered_map>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_Event_Types { Read, Write };
    enum class Enum_Reactor_Command_Types { None, Register, Unregister, Close, WriteData };

    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(const Socket& s, Enum_Event_Types type) = 0;
        virtual void fd_unregister(const Socket& s) = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
