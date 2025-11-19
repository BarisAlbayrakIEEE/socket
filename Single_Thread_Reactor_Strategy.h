// Single_Thread_Reactor_Strategy.h

#ifndef SINGLE_THREAD_REACTOR_STRATEGY_H
#define SINGLE_THREAD_REACTOR_STRATEGY_H

#include "IServerStrategy.h"
#include "IEventLoop.h"
#include "IWorkerPool.h"

namespace ba_socket {
    class Single_Thread_Reactor_Strategy : public IServerStrategy {
    public:
        Single_Thread_Reactor_Strategy(IEventLoop& loop, IWorkerPool& pool)
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
        IEventLoop& _loop;
        IWorkerPool& _pool;
    };
} // namespace ba_socket

#endif // SINGLE_THREAD_REACTOR_STRATEGY_H
