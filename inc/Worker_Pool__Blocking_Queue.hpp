// Worker_Pool__Concurrent_Queue_Blocking.hpp

#ifndef WORKER_POOL__BLOCKING_QUEUE_HPP
#define WORKER_POOL__BLOCKING_QUEUE_HPP

#include "IWorker_Pool.hpp"
#include "Concurrent_Queue_Blocking.hpp"
#include <vector>
#include <thread>
#include <memory>
#include <future>
#include <type_traits>

using namespace BA_Concurrency;

namespace BA_Socket {
    class Worker_Pool__Concurrent_Queue_Blocking : public IWorker_Pool {
    public:
        explicit Worker_Pool__Concurrent_Queue_Blocking(
            size_t thread_count = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < thread_count; ++i) {
                _workers.emplace_back([this] {
                    worker_loop();
                });
            }
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
            using Task = std::packaged_task<void()>;
            _queue.push(std::move(job));
        }

        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args)
        {
            using R = std::invoke_result_t<F, Args...>;
            using Task = std::packaged_task<R()>;

            auto task = std::make_shared<std::packaged_task<R()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            auto fut = task.get_future();
            _queue.push([task]() { (*task)(); });
            return fut;
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
