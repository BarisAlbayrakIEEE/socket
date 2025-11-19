// Worker_Pool__LF.h

#ifndef WORKER_POOL__LF_H
#define WORKER_POOL__LF_H

#include "IWorker_Pool.h"
#include "Concurrent_Queue.h"
#include <vector>
#include <thread>
#include <atomic>

namespace ba_socket {
    class Worker_Pool__LF : public IWorker_Pool {
    public:
        Worker_Pool__LF(size_t threads = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < threads; ++i) {
                _workers.emplace_back([this] {
                    while (_running.load(std::memory_order_relaxed)) {
                        std::function<void()> job;
                        if (_queue.try_pop(job)) {
                            job();
                        } else {
                            std::this_thread::yield(); // or sleep_for(0)
                        }
                    }
                });
            }
        }

        void submit(std::function<void()> job) override {
            _queue.push(std::move(job));
        }

        void shutdown() override {
            _running.store(false);
            for (auto& t : _workers) t.join();
        }

    private:
        std::atomic<bool> _running{true};
        Concurrent_Queue<std::function<void()>> _queue;
        std::vector<std::thread> _workers;
    };
} // namespace ba_socket

#endif // WORKER_POOL__LF_H
