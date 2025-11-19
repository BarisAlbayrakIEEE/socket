// Worker_Pool__Actor.h

#ifndef WORKER_POOL__ACTOR_HPP
#define WORKER_POOL__ACTOR_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_LF_Ring_MPMC.hpp"
#include <vector>
#include <thread>
#include <atomic>

using namespace BA_Concurrency;

namespace BA_Socket {
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
        std::vector<queue_LF_ring_MPMC<std::function<void()>, 8>> _queues;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__ACTOR_HPP
