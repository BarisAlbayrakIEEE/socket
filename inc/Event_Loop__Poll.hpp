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
    public:
        Event_Loop__Poll(
            Callback on_read,
            Callback on_write,
            Callback on_disconnect)
            : _on_read(on_read),
            _on_write(on_write),
            _on_disconnect(on_disconnect) {}

        void register_fd(const Socket& s, EventType type) override {
            std::lock_guard lock(_mtx);

            int fd = s.native_handle();
            struct pollfd p{};
            p.fd = fd;
            p.events = (type == EventType::Read ? POLLIN : POLLOUT);
            _fds_poll.push_back(p);
            _socket_map[fd] = s;
        }

        void unregister_fd(const Socket& s) override {
            std::lock_guard lock(_mtx);
            int fd = s.native_handle();

            // remove from pollfds
            _fds_poll.erase(
                std::remove_if(
                    _fds_poll.begin(),
                    _fds_poll.end(),
                    [fd](const pollfd& p){ return p.fd == fd; }),
                _fds_poll.end());

            _socket_map.erase(fd);
        }

        void run() override {
            _running.store(true);

            while (_running.load()) {
                int ready = ::poll(_fds_poll.data(), _fds_poll.size(), -1);
                if (ready < 0) continue;

                for (auto& p : _fds_poll) {
                    if (p.revents & POLLIN)
                        _on_read(_socket_map[p.fd]);
                    if (p.revents & POLLOUT)
                        _on_write(_socket_map[p.fd]);
                    if (p.revents & (POLLERR | POLLHUP))
                        _on_disconnect(_socket_map[p.fd]);
                }
            }
        }

        void stop() override { _running.store(false); }

    private:
        std::unordered_map<SOCKET, Socket> _socket_map;
        std::vector<pollfd> _fds_poll;
        Callback _on_read, _on_write, _on_disconnect;
        std::mutex _mtx;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__POLL_HPP
