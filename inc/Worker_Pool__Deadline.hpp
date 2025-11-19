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
    class Worker_Pool__Deadline : public IWorker_Pool {
        struct Deadline_Job {
            std::chrono::steady_clock::time_point deadline;
            std::function<void()> fn;

            inline bool operator>(const Deadline_Job& other) const {
                return deadline > other.deadline;
            }
        };
    public:
        explicit Worker_Pool__Deadline(
            size_t thread_count = std::thread::hardware_concurrency())
        {
            for (size_t i=0; i<thread_count; ++i) {
                _workers.emplace_back([this] {
                    worker_loop();
                });
            }
        }

        inline void submit(std::function<void()> job) override {
            Deadline_Job dj{
                std::chrono::steady_clock::now(),
                std::move(job)
            };
            {
                std::lock_guard lock(_mtx);
                _pq.push(std::move(dj));
            }
            _cv.notify_one();
        }

        inline void shutdown() override {
            _running = false;
            _cv.notify_all();
            for (auto& t : _workers) t.join();
        }

    private:
        void worker_loop() {
            while (_running) {
                Deadline_Job job;
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
        std::priority_queue<Deadline_Job, std::vector<Deadline_Job>, std::greater<>> _pq;
        std::mutex _mtx;
        std::condition_variable _cv;
        std::vector<std::thread> _workers;
    };
} // namespace BA_Socket

#endif // WORKER_POOL__DEADLINE_HPP
