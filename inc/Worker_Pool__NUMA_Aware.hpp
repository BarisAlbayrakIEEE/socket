// Worker_Pool__NUMA_Awareh

#ifndef WORKER_POOL__NUMA_AWARE_HPP
#define WORKER_POOL__NUMA_AWARE_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_Blocking.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <sched.h>
#include <unistd.h>

using namespace BA_Concurrency;

namespace BA_Socket {
    class Worker_Pool__NUMA_Aware : public IWorker_Pool {
    public:
        Worker_Pool__NUMA_Aware(size_t thread_count_per_node = 2)
        {
            size_t nodes = std::thread::hardware_concurrency() / thread_count_per_node;
            if (nodes == 0) nodes = 1;

            _queues.resize(nodes);

            for (size_t n = 0; n < nodes; ++n) {
                for (size_t t = 0; t < thread_count_per_node; ++t) {
                    _workers.emplace_back([this, n] {
                        pin_to_numa_node(n);
                        auto& q = _queues[n];

                        while (_running) {
                            auto job = q.pop();
                            if (job) job();
                        }
                    });
                }
            }
        }

        void submit(std::function<void()> job) override {
            size_t idx = _next++ % _queues.size();
            _queues[idx].push(std::move(job));
        }

        void shutdown() override {
            _running = false;
            for (auto& q : _queues) q.stop();
            for (auto& t : _workers) t.join();
        }

    private:
        static void pin_to_numa_node(size_t node) {
            cpu_set_t set;
            CPU_ZERO(&set);
            CPU_SET(node, &set);
            sched_setaffinity(0, sizeof(set), &set);
        }

        std::atomic<bool> _running{true};
        std::atomic<size_t> _next{0};

        std::vector<Concurrent_Queue_Blocking<std::function<void()>>> _queues;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__NUMA_AWARE_HPP
