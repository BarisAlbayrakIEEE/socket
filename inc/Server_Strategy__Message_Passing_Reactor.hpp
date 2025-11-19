// Server_Strategy__Message_Passing_Reactor.h

#ifndef SERVER_STRATEGY__MESSAGE_PASSING_REACTOR_H
#define SERVER_STRATEGY__MESSAGE_PASSING_REACTOR_H

#include "IServer_Strategy.hpp"
#include "IEvent_Loop.hpp"
#include "IWorker_Pool.hpp"
#include "IMessage_Queue.hpp"

namespace ba_socket {
    struct WorkerJob {
        Socket client;
        std::vector<uint8_t> data;
    };

    struct IoJob {
        Socket client;
        std::vector<uint8_t> response;
    };

    class Server_Strategy__Message_Passing_Reactor : public IServer_Strategy {
    public:
        Server_Strategy__Message_Passing_Reactor(
            IEvent_Loop& loop,
            IWorker_Pool& pool,
            IMessage_Queue<WorkerJob>& toWorkers,
            IMessage_Queue<IoJob>& toIo)
            : _loop(loop), _pool(pool),
            _toWorkers(toWorkers), _toIo(toIo)
        {}

        void start() override {
            // Start worker poller thread
            _worker_poller = std::jthread([this](std::stop_token st) {
                IoJob job;
                while (!st.stop_requested()) {
                    if (_toIo.try_pop(job)) {
                        job.client.write(job.response);
                    }
                }
            });

            _loop.run();
        }

        void stop() override {
            _loop.stop();
            _pool.shutdown();
            _worker_poller.request_stop();
        }

        void on_accept(Socket client) override {
            // nothing special here
        }

        void on_read_ready(Socket client) override {
            auto data = client.read_nonblocking();
            _toWorkers.push({client, data});
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

        IMessage_Queue<WorkerJob>& _toWorkers;
        IMessage_Queue<IoJob>& _toIo;

        std::jthread _worker_poller;
    };
} // namespace ba_socket

#endif // SERVER_STRATEGY__MESSAGE_PASSING_REACTOR_H
