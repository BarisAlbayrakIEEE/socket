// Worker_Pool__LFh

#ifndef WORKER_POOL__LF_HPP
#define WORKER_POOL__LF_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_LF_Ring_MPMC.hpp"
#include <vector>
#include <thread>
#include <atomic>

using namespace BA_Concurrency;

namespace BA_Socket {
    class Worker_Pool__LF : public IWorker_Pool {
    public:
        Worker_Pool__LF(size_t thread_count = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < thread_count; ++i) {
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
        queue_LF_ring_MPMC<std::function<void()>, 8> _queue;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__LF_HPP
