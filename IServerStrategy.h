// IServerStrategy.h

#ifndef ISERVER_STRATEGY_H
#define ISERVER_STRATEGY_H

#include <functional>
#include "Socket.h"

namespace ba_socket {
    class IServerStrategy {
    public:
        virtual ~IServerStrategy() = default;

        virtual void start() = 0;
        virtual void stop() = 0;

        // Called by event loop (or leader)
        virtual void on_accept(Socket client) = 0;
        virtual void on_read_ready(Socket client) = 0;
        virtual void on_write_ready(Socket client) = 0;
        virtual void on_disconnect(Socket client) = 0;
    };
} // namespace ba_socket

#endif // ISERVER_STRATEGY_H
