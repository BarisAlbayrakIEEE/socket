// Message_Passing_Reactor_Strategy.h

#ifndef MESSAGE_PASSING_REACTOR_STRATEGY_H
#define MESSAGE_PASSING_REACTOR_STRATEGY_H

#include "IServerStrategy.h"
#include "IEventLoop.h"
#include "IWorkerPool.h"
#include "IMessageQueue.h"

namespace ba_socket {
    struct WorkerJob {
        Socket client;
        std::vector<uint8_t> data;
    };

    struct IoJob {
        Socket client;
        std::vector<uint8_t> response;
    };

    class Message_Passing_Reactor_Strategy : public IServerStrategy {
    public:
        Message_Passing_Reactor_Strategy(
            IEventLoop& loop,
            IWorkerPool& pool,
            IMessageQueue<WorkerJob>& toWorkers,
            IMessageQueue<IoJob>& toIo)
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
        IEventLoop& _loop;
        IWorkerPool& _pool;

        IMessageQueue<WorkerJob>& _toWorkers;
        IMessageQueue<IoJob>& _toIo;

        std::jthread _worker_poller;
    };
} // namespace ba_socket

#endif // MESSAGE_PASSING_REACTOR_STRATEGY_H
