// Worker_Pool__Actor.h

#ifndef WORKER_POOL__ACTOR_HPP
#define WORKER_POOL__ACTOR_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_LF_Ring_MPMC.hpp"
#include "socket_setup.hpp"
#include <vector>
#include <thread>
#include <atomic>

using namespace BA_Concurrency;

namespace BA_Socket {
    class Worker_Pool__Actor : public IWorker_Pool {
        using func_t = std::function<void()>;
    public:
        Worker_Pool__Actor(size_t thread_count = std::thread::hardware_concurrency())
            : _jobs(thread_count)
        {
            for (size_t i = 0; i < thread_count; ++i) {
                _threads.emplace_back([this, i] {
                    auto& q = _jobs[i];
                    while (_running.load(std::memory_order_relaxed)) {
                        auto job{ q.try_pop() };
                        if (job.has_value()) {
                            job.value()();
                        } else {
                            std::this_thread::yield();
                        }
                    }
                });
            }
        }

        ~Worker_Pool__Actor() {
            if (_running) shutdown();
        }

        inline void submit(func_t job) override {
            // Simple round-robin dispatching
            size_t idx = _next.fetch_add(1, std::memory_order_relaxed) % _jobs.size();
            _jobs[idx].push(std::move(job));
        }

        inline void shutdown() override {
            if (bool expected{true}; !_running.compare_exchange_strong(expected, false))
                return;
            for (auto& t : _threads) t.join();
        }

    private:
        std::vector<queue_LF_ring_MPMC<func_t, Capacity_As_Pow2>> _jobs;
        std::vector<std::thread> _threads;
        std::atomic<size_t> _next{0};
        std::atomic<bool> _running{true};
    };
} // namespace BA_Socket

#endif // WORKER_POOL__ACTOR_HPP
