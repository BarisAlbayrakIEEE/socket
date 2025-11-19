// Leader_Follower_Strategy.h

#ifndef LEADER_FOLLOWER_STRATEGY_H
#define LEADER_FOLLOWER_STRATEGY_H

#include "IServerStrategy.h"
#include "IEventLoop.h"
#include "IWorkerPool.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace ba_socket {
    class Leader_Follower_Strategy : public IServerStrategy {
    public:
        Leader_Follower_Strategy(IEventLoop& loop, IWorkerPool& pool)
            : _loop(loop), _pool(pool), _running(false)
        {}

        void start() override {
            _running.store(true);
            become_leader();          // first leader
        }

        void stop() override {
            _running.store(false);
            _loop.stop();
            _pool.shutdown();
        }

        // Called by leader thread (event loop)
        void on_accept(Socket client) override {
            dispatch([client, this]() mutable {
                // handle accept
            });
        }

        void on_read_ready(Socket client) override {
            dispatch([client, this]() mutable {
                // handle read
            });
        }

        void on_write_ready(Socket client) override {
            dispatch([client, this]() mutable {
                // handle write
            });
        }

        void on_disconnect(Socket client) override {
            dispatch([client, this]() mutable {
                // handle disconnect
            });
        }

    private:
        void dispatch(std::function<void()> job) {
            _pool.submit([this, job]() {
                job();
                become_leader();
            });
        }

        void become_leader() {
            if (!_running.load()) return;

            // Only one thread should run this at a time
            std::unique_lock lock(_leader_mtx);
            if (_is_leader) return;  
            _is_leader = true;

            // Run event loop
            _loop.run();

            _is_leader = false;
        }

        IEventLoop& _loop;
        IWorkerPool& _pool;

        std::mutex _leader_mtx;
        std::atomic<bool> _running;
        bool _is_leader = false;
    };
} // namespace ba_socket

#endif // LEADER_FOLLOWER_STRATEGY_H
