// Server_Strategy__Single_Thread_Reactor.h

#ifndef SERVER_STRATEGY__SINGLE_THREAD_REACTOR_H
#define SERVER_STRATEGY__SINGLE_THREAD_REACTOR_H

#include "IServer_Strategy.hpp"
#include "IEvent_Loop.hpp"
#include "IWorker_Pool.hpp"

namespace ba_socket {
    class Server_Strategy__Single_Thread_Reactor : public IServer_Strategy {
    public:
        Server_Strategy__Single_Thread_Reactor(IEvent_Loop& loop, IWorker_Pool& pool)
            : _loop(loop), _pool(pool)
        {}

        void start() override { _loop.run(); }
        void stop()  override { _loop.stop();  _pool.shutdown(); }

        void on_accept(Socket client) override {
            // Accept happens on the I/O thread
            _pool.submit([this, client]() mutable {
                // parse / compute work
            });
        }

        void on_read_ready(Socket client) override {
            std::vector<uint8_t> data = client.read_nonblocking();

            _pool.submit([this, client, data]() mutable {
                // processing
            });
        }

        void on_write_ready(Socket client) override {
            client.flush_pending_writes();
        }

        void on_disconnect(Socket client) override {
            // cleanup
        }

    private:
        IEvent_Loop& _loop;
        IWorker_Pool& _pool;
    };
} // namespace ba_socket

#endif // SERVER_STRATEGY__SINGLE_THREAD_REACTOR_H
