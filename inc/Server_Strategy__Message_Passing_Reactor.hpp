// Server_Strategy__Message_Passing_Reactor.hpp

#ifndef SERVER_STRATEGY__MESSAGE_PASSING_REACTOR_HPP
#define SERVER_STRATEGY__MESSAGE_PASSING_REACTOR_HPP

#include "IServer_Strategy.hpp"
#include "IEvent_Loop.hpp"
#include "IThread_Pool.hpp"
#include "IMessage_Queue.hpp"
#include <stop_token>
#include <thread>

namespace BA_Socket {

    class Server_Strategy__Message_Passing_Reactor : public IServer_Strategy {

        struct WorkerJob {
            Socket client{ INVALID_SOCKET };
            std::vector<uint8_t> data{};
            WorkerJob() = default;
        };
        struct IoJob {
            Socket client{ INVALID_SOCKET };
            std::vector<uint8_t> response{};
            IoJob() = default;
        };

    public:
        Server_Strategy__Message_Passing_Reactor(
            IEvent_Loop& loop,
            IThread_Pool& pool,
            IMessage_Queue<WorkerJob>& to_workers,
            IMessage_Queue<IoJob>& to_Io)
            : _loop(loop), _pool(pool), _to_workers(to_workers), _to_Io(to_Io) {}

        void start() override {
            // Start worker poller thread
            _worker_poller = std::jthread([this](std::stop_token st) {
                IoJob job;
                while (!st.stop_requested()) {
                    if (_to_Io.try_pop(job)) {
                        job.client.write(job.response);
                    }
                }
            });
            _loop.run();
        }

        inline void stop() override {
            _loop.stop();
            _pool.shutdown();
            _worker_poller.request_stop();
        }

        inline void on_accept(Socket client) override {
            // nohing specialhere
        }

        inline void on_read_ready(Socket client) override {
            auto data = client.read_nonblocking();
            _to_workers.push({client, data});
        }

        inline void on_write_ready(Socket client) override {
            client.flush_pending_writes();
        }

        inline void on_disconnect(Socket client) override {
            TODO: cleanup;
        }

    private:
        IEvent_Loop& _loop;
        IThread_Pool& _pool;
        IMessage_Queue<WorkerJob>& _to_workers;
        IMessage_Queue<IoJob>& _to_Io;
        std::jthread _worker_poller;
    };
} // namespace BA_Socket

#endif // SERVER_STRATEGY__MESSAGE_PASSING_REACTOR_HPP
