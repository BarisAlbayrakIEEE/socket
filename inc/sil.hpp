#pragma once
#include "IWorkerPool.hpp"
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>
#include <optional>
#include <vector>
#include <functional>
#include <numeric>

class WorkStealingWorkerPool : public IWorkerPool {
public:
    explicit WorkStealingWorkerPool(size_t threads = std::thread::hardware_concurrency())
        : _running(true),
          _queues(threads)
    {
        for (size_t i = 0; i < threads; ++i)
            _workers.emplace_back([this, i] { worker_loop(i); });
    }

    ~WorkStealingWorkerPool() {
        shutdown();
    }

    // ============================================================
    //  submit(F, Args...) -> std::future<R>
    // ============================================================
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;

        // Bind user function + args → zero-arg callable
        auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        // Create packaged_task<R()>
        auto task = std::make_shared<std::packaged_task<R()>>(std::move(bound));
        std::future<R> fut = task->get_future();

        // Wrap into std::function<void()>
        std::function<void()> job = [task]() {
            (*task)();
        };

        // Push job into this thread's queue (round-robin)
        size_t idx = _next.fetch_add(1, std::memory_order_relaxed) % _queues.size();
        push_job(idx, std::move(job));

        return fut;
    }

    // ============================================================
    //  shutdown(): stop workers and join them
    // ============================================================
    void shutdown() override {
        bool expected = true;
        if (!_running.compare_exchange_strong(expected, false))
            return;

        // Wake all workers
        for (auto& q : _queues) {
            std::lock_guard lk(q._m);
            q._stopped = true;
        }

        for (auto& t : _workers)
            if (t.joinable())
                t.join();
    }

private:
    struct Queue {
        std::deque<std::function<void()>> _job_deque;
        std::mutex _m;
        bool _stopped = false;
    };

    // ============================================================
    //  Push job into worker's deque (back)
    // ============================================================
    void push_job(size_t idx, std::function<void()> job) {
        Queue& q = _queues[idx];
        {
            std::lock_guard lk(q._m);
            q._job_deque.push_back(std::move(job));
        }
    }

    // ============================================================
    //  Try to steal job from another worker (front)
    // ============================================================
    std::optional<std::function<void()>> steal(size_t thief_id) {
        size_t n = _queues.size();
        for (size_t k = 1; k < n; ++k) {
            size_t victim = (thief_id + k) % n;

            Queue& q = _queues[victim];
            std::lock_guard lk(q._m);

            if (!q._job_deque.empty()) {
                auto job = std::move(q._job_deque.front());
                q._job_deque.pop_front();
                return job;
            }
        }
        return std::nullopt;
    }

    // ============================================================
    //  Worker main loop
    // ============================================================
    void worker_loop(size_t id) {
        Queue& myq = _queues[id];

        while (_running.load(std::memory_order_relaxed)) {
            std::function<void()> job;

            // Try to pop from own deque
            {
                std::lock_guard lk(myq._m);
                if (!myq._job_deque.empty()) {
                    job = std::move(myq._job_deque.back());
                    myq._job_deque.pop_back();
                }
            }

            if (!job) {
                // Attempt to steal
                auto stolen = steal(id);
                if (stolen)
                    job = std::move(*stolen);
            }

            if (job) {
                job();         // execute job => fulfills future
            } else {
                std::this_thread::yield();
            }
        }
    }

    // ============================================================
    //  Members
    // ============================================================
    std::atomic<bool> _running;
    std::atomic<size_t> _next{0};

    std::vector<Queue> _queues;
    std::vector<std::thread> _workers;
};
