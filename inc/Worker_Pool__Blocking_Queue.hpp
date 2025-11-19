// Worker_Pool__Concurrent_Queue_Blocking.h

#ifndef WORKER_POOL__BLOCKING_QUEUE_H
#define WORKER_POOL__BLOCKING_QUEUE_H

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_Blocking.hpp"
#include <vector>
#include <thread>

using namespace BA_Concurrency;

namespace ba_socket {
    class Worker_Pool__Concurrent_Queue_Blocking : public IWorker_Pool {
    public:
        explicit Worker_Pool__Concurrent_Queue_Blocking(
            size_t threads = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < threads; ++i) {
                _workers.emplace_back([this] {
                    while (true) {
                        auto job = _queue.pop();
                        if (!_running) break;
                        if (job) job();
                    }
                });
            }
        }

        void submit(std::function<void()> job) override {
            _queue.push(std::move(job));
        }

        void shutdown() override {
            _running = false;
            _queue.stop();
            for (auto& t : _workers) t.join();
        }

    private:
        std::atomic<bool> _running{true};
        Blocking_Queue<std::function<void()>> _queue;
        std::vector<std::thread> _workers;
    };
} // namespace ba_socket

#endif // WORKER_POOL__BLOCKING_QUEUE_H
