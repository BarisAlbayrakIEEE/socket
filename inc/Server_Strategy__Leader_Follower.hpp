// Server_Strategy__Leader_Follower.hpp

#ifndef SERVER_STRATEGY__LEADER_FOLLOWER_HPP
#define SERVER_STRATEGY__LEADER_FOLLOWER_HPP

#include "IServer_Strategy.hpp"
#include "IEvent_Loop.hpp"
#include "IWorker_Pool.hpp"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace BA_Socket {
    class Server_Strategy__Leader_Follower : public IServer_Strategy {
    public:
        Server_Strategy__Leader_Follower(IEvent_Loop& loop, IWorker_Pool& pool)
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
            dispath([client, this]() mutable {
                //handle accept
            });
        }

        void on_read_ready(Socket client) override {
            dispath([client, this]() mutable {
                //handle read
            });
        }

        void on_write_ready(Socket client) override {
            dispath([client, this]() mutable {
                //handle write
            });
        }

        void on_disconnect(Socket client) override {
            dispath([client, this]() mutable {
                //handle disconnect
            });
        }

    private:
        void dispath(std::function<void()> job) {
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

        IEvent_Loop& _loop;
        IWorker_Pool& _pool;

        std::mutex _leader_mtx;
        std::atomic<bool> _running;
        bool _is_leader = false;
    };
} // namespace BA_Socket

#endif // SERVER_STRATEGY__LEADER_FOLLOWER_HPP
