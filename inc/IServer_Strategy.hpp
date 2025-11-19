// IServer_Strategy.hpp

#ifndef ISERVER_STRATEGY_HPP
#define ISERVER_STRATEGY_HPP

#include <functional>
#include "Socket.hpp"

namespace BA_Socket {
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
} // namespace BA_Socket

#endif // ISERVER_STRATEGY_HPP
