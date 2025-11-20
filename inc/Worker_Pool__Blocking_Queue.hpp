// Worker_Pool__Concurrent_Queue_Blocking.hpp

#ifndef WORKER_POOL__BLOCKING_QUEUE_HPP
#define WORKER_POOL__BLOCKING_QUEUE_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_Blocking.hpp"
#include <vector>
#include <thread>

using namespace BA_Concurrency;

namespace BA_Socket {
    class Worker_Pool__Concurrent_Queue_Blocking : public IWorker_Pool {
    public:
        explicit Worker_Pool__Concurrent_Queue_Blocking(
            size_t thread_count = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < thread_count; ++i) {
                _workers.emplace_back([this] {
                    while (true) {
                        auto job = _queue.pop();
                        if (!_running) break;
                        if (job) job();
                    }
                });
            }
        }

        ~Worker_Pool__Concurrent_Queue_Blocking() {
            if (_running.load())
                shutdown();
        }

        inline void submit(std::function<void()> job) override {
            _queue.push(std::move(job));
        }

        inline void shutdown() override {
            _running.store(false);
            _queue.stop();
            for (auto& t : _workers) t.join();
        }

    private:
        std::atomic<bool> _running{true};
        Concurrent_Queue_Blocking<std::function<void()>> _queue;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__BLOCKING_QUEUE_HPP
