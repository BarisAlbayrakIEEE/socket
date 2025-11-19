// IServer_Strategy.h

#ifndef ISERVER_STRATEGY_H
#define ISERVER_STRATEGY_H

#include <functional>
#include "Socket.hpp"

namespace ba_socket {
    class IServer_Strategy {
    public:
        virtual ~IServer_Strategy() = default;

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
