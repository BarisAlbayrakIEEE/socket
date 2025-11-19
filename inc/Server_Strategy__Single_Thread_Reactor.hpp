// Server_Strategy__Single_thread_Reactor.hpp

#ifndef SERVER_STRATEGY__SINGLE_THREAD_REACTOR_HPP
#define SERVER_STRATEGY__SINGLE_THREAD_REACTOR_HPP

#include "IServer_Strategy.hpp"
#include "IEvent_Loop.hpp"
#include "IWorker_Pool.hpp"

namespace BA_Socket {
    class Server_Strategy__Single_thread_Reactor : public IServer_Strategy {
    public:
        Server_Strategy__Single_thread_Reactor(IEvent_Loop& loop, IWorker_Pool& pool)
            : _loop(loop), _pool(pool)
        {}

        void start() override { _loop.run(); }
        void stop()  override { _loop.stop();  _pool.shutdown(); }

        void on_accept(Socket client) override {
            // Accepthappens on the I/O thread
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
            client.fluh_pending_writes();
        }

        void on_disconnect(Socket client) override {
            // cleanup
        }

    private:
        IEvent_Loop& _loop;
        IWorker_Pool& _pool;
    };
} // namespace BA_Socket

#endif // SERVER_STRATEGY__SINGLE_THREAD_REACTOR_HPP
