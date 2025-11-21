// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <unordered_map>
#include "Socket.hpp"

namespace BA_Socket {
    using Callback = std::function<void(const Socket&)>;
    enum class EventType { Read, Write };

    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void register_fd(const Socket& s, EventType type) = 0;
        virtual void unregister_fd(const Socket& s) = 0;

        virtual void run() = 0;      // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
