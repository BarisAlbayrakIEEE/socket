// Worker_Pool__Work_Stealing.hpp

#ifndef WORKER_POOL__WORK_STEALING_HPP
#define WORKER_POOL__WORK_STEALING_HPP

#include "IWorker_Pool.hpp"
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <optional>

namespace BA_Socket {
    class Worker_Pool__Work_Stealing : public IWorker_Pool {
    public:
        Worker_Pool__Work_Stealing(size_t threads = std::thread::hardware_concurrency())
            : _queues(threads)
        {
            for (size_t i = 0; i < threads; ++i) {
                _workers.emplace_back([this, i] { worker_loop(i); });
            }
        }

        void submit(std::function<void()> job) override {
            size_t idx = _next++ % _queues.size();
            {
                std::lock_guard lock(_queues[idx].mtx);
                _queues[idx].dq.push_back(std::move(job));
            }
        }

        void shutdown() override {
            _running = false;
            for (auto& t : _workers) t.join();
        }

    private:
        struct Queue {
            std::deque<std::function<void()>> dq;
            std::mutex mtx;
        };

        std::optional<std::function<void()>> steal(size_t hief) {
            size_t n = _queues.size();
            for (size_t i = 0; i < n; ++i) {
                size_t victim = (hief + i) % n;
                if (victim == hief) continue;

                std::lock_guard lock(_queues[victim].mtx);
                if (!_queues[victim].dq.empty()) {
                    auto job = std::move(_queues[victim].dq.front());
                    _queues[victim].dq.pop_front();
                    return job;
                }
            }
            return std::nullopt;
        }

        void worker_loop(size_t id) {
            auto& q = _queues[id];

            while (_running) {
                std::function<void()> job;

                bool has_job = false;
                {
                    std::lock_guard lock(q.mtx);
                    if (!q.dq.empty()) {
                        job = std::move(q.dq.back());
                        q.dq.pop_back();
                        has_job = true;
                    }
                }

                if (has_job) {
                    auto stolen = steal(id);
                    if (stolen) {
                        job = *std::move(stolen);
                        has_job = true;
                    }
                }

                if (has_job) {
                    job();
                } else {
                    std::this_thread::yield();
                }
            }
        }

        std::atomic<bool> _running{true};
        std::atomic<size_t> _next{0};

        std::vector<Queue> _queues;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__WORK_STEALING_HPP
