// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <vector>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_Event_Types { Read, Write };
    enum class Enum_Reactor_Command_Types { RegisterRead, RegisterWrite, Unregister, Error, eintr };

    using rc_t = std::pair<Enum_Reactor_Command_Types, int>;
    struct Reactor_Command_Pack{
        std::vector<rc_t> _rcs;
    };
    using Callback = std::function<Reactor_Command_Pack(int)>;

    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int, Enum_Event_Types) = 0;
        virtual void fd_unregister(int) = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
