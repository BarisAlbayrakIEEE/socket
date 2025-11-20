// Worker_Pool__Actor.h

#ifndef WORKER_POOL__ACTOR_HPP
#define WORKER_POOL__ACTOR_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_LF_Ring_MPSC.hpp"
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
            for (size_t i = 0; i < thread_count; ++i)
                _threads.emplace_back([this, i] { worker_loop(i); });
        }

        ~Worker_Pool__Actor() {
            if (_running) shutdown();
        }

        inline void submit(func_t job) override {
            size_t id = _next.fetch_add(1, std::memory_order_relaxed) % _jobs.size();
            _jobs[id].push(std::move(job));
        }

        inline void shutdown() override {
            if (bool expected{true}; !_running.compare_exchange_strong(expected, false))
                return;
            for (auto& t : _threads) t.join();
        }

    private:

        inline void worker_loop(size_t id) {
            auto& jobs = _jobs[id];
            while (_running.load(std::memory_order_relaxed)) {
                auto job{ jobs.try_pop() };
                if (job) job.value()();
                else std::this_thread::yield();
            }
        }

        std::vector<queue_LF_ring_MPSC<func_t, Capacity_As_Pow2>> _jobs;
        std::vector<std::thread> _threads;
        std::atomic<size_t> _next{0};
        std::atomic<bool> _running{true};
    };
} // namespace BA_Socket

#endif // WORKER_POOL__ACTOR_HPP
