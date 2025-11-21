// Event_Loop__Epoll.hpp

#ifndef EVENT_LOOP__EPOLL_HPP
#define EVENT_LOOP__EPOLL_HPP

#include "IEvent_Loop.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace BA_Socket {
    class Event_Loop__Epoll : public IEvent_Loop {
    public:
        Event_Loop__Epoll(
            Callback on_read,
            Callback on_write,
            Callback on_disconnect)
            :
            _on_read(on_read),
            _on_write(on_write),
            _on_disconnect(on_disconnect)
        {
            _epfd = ::epoll_create1(0);
            if (_epfd < 0)
                throw std::runtime_error("epoll_create1 failed");
        }

        ~Event_Loop__Epoll() {
            ::close(_epfd);
        }

        void register_fd(const Socket& s, EventType type) override {
            std::lock_guard lock(_mtx);

            int fd = s.native_handle();
            epoll_event ev{};
            ev.data.fd = fd;
            ev.events = (type == EventType::Read ? EPOLLIN : EPOLLOUT);
            if (::epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
                throw std::runtime_error("epoll_ctl ADD failed");

            _socket_map[fd] = s;
        }

        void unregister_fd(const Socket& s) override {
            std::lock_guard lock(_mtx);
            int fd = s.native_handle();
            ::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
            _socket_map.erase(fd);
        }

        void run() override {
            _running.store(true);

            constexpr int MAX_EVENTS = 64;
            epoll_event events[MAX_EVENTS];
            while (_running.load()) {
                int n = ::epoll_wait(_epfd, events, MAX_EVENTS, -1);
                if (n < 0) continue;

                for (int i = 0; i < n; ++i) {
                    int fd = events[i].data.fd;

                    if (events[i].events & EPOLLIN)
                        _on_read(_socket_map.at(fd));

                    if (events[i].events & EPOLLOUT)
                        _on_write(_socket_map.at(fd));

                    if (events[i].events & (EPOLLERR | EPOLLHUP))
                        _on_disconnect(_socket_map.at(fd));
                }
            }
        }

        void stop() override { _running.store(false); }

    private:
        std::unordered_map<SOCKET, Socket> _socket_map;
        Callback _on_read, _on_write, _on_disconnect;
        int _epfd;
        std::mutex _mtx;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__EPOLL_HPP
