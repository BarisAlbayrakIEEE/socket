// IEventLoop.h

#ifndef IEVENT_LOOP_H
#define IEVENT_LOOP_H

#include <functional>
#include <unordered_map>
#include "Socket.h"

namespace ba_socket {
    enum class EventType { Read, Write };

    class IEventLoop {
    public:
        virtual ~IEventLoop() = default;

        virtual void register_fd(const Socket& s, EventType type) = 0;
        virtual void unregister_fd(const Socket& s) = 0;

        virtual void run() = 0;      // blocking
        virtual void stop() = 0;
    };
} // namespace ba_socket

#endif // IEVENT_LOOP_H
