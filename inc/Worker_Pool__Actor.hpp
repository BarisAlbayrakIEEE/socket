// Worker_Pool__Actor.h

#ifndef WORKER_POOL__ACTOR_H
#define WORKER_POOL__ACTOR_H

#include "IWorker_Pool.h"
#include "Concurrent_Queue.h"   // MPSC-specialized
#include <vector>
#include <thread>
#include <atomic>

namespace ba_socket {
    class Worker_Pool__Actor : public IWorker_Pool {
    public:
        Worker_Pool__Actor(size_t threads = std::thread::hardware_concurrency())
            : _queues(threads)
        {
            for (size_t i = 0; i < threads; ++i) {
                _workers.emplace_back([this, i] {
                    auto& q = _queues[i];
                    while (_running.load(std::memory_order_relaxed)) {
                        std::function<void()> job;
                        if (q.try_pop(job)) {
                            job();
                        } else {
                            std::this_thread::yield();
                        }
                    }
                });
            }
        }

        void submit(std::function<void()> job) override {
            // Simple round-robin dispatching
            size_t idx = _next.fetch_add(1, std::memory_order_relaxed) % _queues.size();
            _queues[idx].push(std::move(job));
        }

        void shutdown() override {
            _running.store(false);
            for (auto& t : _workers) t.join();
        }

    private:
        std::atomic<bool> _running{true};
        std::atomic<size_t> _next{0};
        std::vector<Concurrent_Queue<std::function<void()>>> _queues;
        std::vector<std::thread> _workers;
    };
} // namespace ba_socket

#endif // WORKER_POOL__ACTOR_H
