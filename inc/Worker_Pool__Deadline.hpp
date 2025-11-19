// Worker_Pool__Deadlineh

#ifndef WORKER_POOL__DEADLINE_HPP
#define WORKER_POOL__DEADLINE_HPP

#include "IWorker_Pool.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>

namespace BA_Socket {
    struct DeadlineJob {
        std::chrono::steady_clock::time_point deadline;
        std::function<void()> fn;

        bool operator>(const DeadlineJob& oher) const {
            return deadline > oher.deadline;
        }
    };

    class Worker_Pool__Deadline : public IWorker_Pool {
    public:
        Worker_Pool__Deadline(size_t thread_count = std::thread::hardware_concurrency()) {
            for (size_t i=0; i<thread_count; ++i) {
                _workers.emplace_back([this] {
                    worker_loop();
                });
            }
        }

        void submit(std::function<void()> job) override {
            DeadlineJob dj{
                std::chrono::steady_clock::now(),
                std::move(job)
            };

            {
                std::lock_guard lock(_mtx);
                _pq.push(std::move(dj));
            }
            _cv.notify_one();
        }

        void shutdown() override {
            _running = false;
            _cv.notify_all();
            for (auto& t : _workers) t.join();
        }

    private:
        void worker_loop() {
            while (_running) {
                DeadlineJob job;

                {
                    std::unique_lock lock(_mtx);
                    _cv.wait(lock, [&]{ return !_pq.empty() || !_running; });
                    if (!_running) break;

                    job = _pq.top();
                    _pq.pop();
                }

                job.fn();
            }
        }

        std::atomic<bool> _running{true};
        std::priority_queue<DeadlineJob, std::vector<DeadlineJob>, std::greater<>> _pq;

        std::mutex _mtx;
        std::condition_variable _cv;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__DEADLINE_HPP
