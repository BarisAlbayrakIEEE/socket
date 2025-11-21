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
    class Actor_Ref;
    using func_t = std::function<void(Actor_Ref)>;
    using MPSC_queue_t = queue_LF_ring_MPSC<func_t, Capacity_As_Pow2>;
    using MPSC_queues_t = std::vector<MPSC_queue_t>;

    class Actor_Ref {
    public:
        Actor_Ref() = default;
        Actor_Ref(size_t id, MPSC_queues_t* mailboxes)
            : _id(id), _mailboxes(mailboxes) {}

        template <typename F>
        void send(F&& f) const {
            (*_mailboxes)[_id].push(Job(std::forward<F>(f)));
        }

    private:
        size_t _id;
        MPSC_queues_t* _mailboxes;
    };

    class Worker_Pool__Actor : public IWorker_Pool {
    public:
        Worker_Pool__Actor(size_t thread_count = std::thread::hardware_concurrency()) {
            _messages.reserve(thread_count);
            _actor_refs.reserve(thread_count);
            for (size_t i = 0; i < thread_count; ++i) {
                _messages.emplace_back();
                _actor_refs.emplace_back(i, &_messages);
            }
            for (size_t i = 0; i < thread_count; ++i)
                _threads.emplace_back([this, i] { worker_loop(i); });
        }

        ~Worker_Pool__Actor() {
            if (_running) shutdown();
        }

        inline void submit(func_t message) override {
            size_t id = _next.fetch_add(1, std::memory_order_relaxed) % _messages.size();
            _messages[id].push(std::move(message));
        }

        inline void shutdown() override {
            if (bool expected{true}; !_running.compare_exchange_strong(expected, false))
                return;
            for (auto& t : _threads) t.join();
        }

    private:

        inline void worker_loop(size_t id) {
            Actor_Ref self = _actor_refs[id];
            while (_running.load()) {
                auto msg = _messages[id].try_pop();
                if (msg) msg.value()(self);
                else std::this_thread::yield();
            }
        }
        inline void worker_loop(size_t id) {
            auto& messages = _messages[id];
            while (_running.load(std::memory_order_relaxed)) {
                auto message{ messages.try_pop() };
                if (message) message.value()();
                else std::this_thread::yield();
            }
        }

        std::vector<Actor_Ref> _actor_refs;
        MPSC_queues_t _messages;
        std::vector<std::thread> _threads;
        std::atomic<size_t> _next{0};
        std::atomic<bool> _running{true};
    };
} // namespace BA_Socket

#endif // WORKER_POOL__ACTOR_HPP
