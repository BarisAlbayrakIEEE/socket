// Event_Loop__Poll.hpp

#ifndef EVENT_LOOP__POLL_HPP
#define EVENT_LOOP__POLL_HPP

#include "IEvent_Loop.hpp"
#include <poll.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <vector>
#include <unordered_map>

namespace BA_Socket {
    class Event_Loop__Poll : public IEvent_Loop {
        using Callback = std::function<void(const Socket&)>;
        using sockmap_t = std::unordered_map<SOCKET, Socket>;
    public:
        Event_Loop__Poll(
            Callback on_read,
            Callback on_write,
            Callback on_disconnect)
            :
            _on_read(on_read),
            _on_write(on_write),
            _on_disconnect(on_disconnect) {}

        inline void fd_register(int fd, Enum_Event_Types type) override {
            std::scoped_lock lk(_m);

            struct pollfd p{};
            p.fd = fd;
            p.events = (type == Enum_Event_Types::Read ? POLLIN : POLLOUT);
            _pollfds.push_back(p);
        }

        inline void fd_unregister(int fd) override {
            std::scoped_lock lk(_m);

            _pollfds.erase(
                std::remove_if(
                    _pollfds.begin(),
                    _pollfds.end(),
                    [fd](const pollfd& p){ return p.fd == fd; }),
                _pollfds.end());
        }

        void run() override {
            _running.store(true);
            while (_running.load()) {
                int ready = ::poll(_pollfds.data(), _pollfds.size(), -1);
                if (ready < 0) continue;

                for (auto& p : _pollfds) {
                    if (p.revents & POLLIN) _on_read(_sockmap[p.fd]);
                    if (p.revents & POLLOUT) _on_write(_sockmap[p.fd]);
                    if (p.revents & (POLLERR | POLLHUP)) _on_disconnect(_sockmap[p.fd]);
                }
            }
        }

        inline void stop() override { _running.store(false); }

    private:
        sockmap_t _sockmap;
        std::vector<pollfd> _pollfds;
        Callback _on_read, _on_write, _on_disconnect;
        std::mutex _m;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__POLL_HPP
