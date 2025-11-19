// Event_Loop__Select.h

#ifndef EVENT_LOOP__SELECT_H
#define EVENT_LOOP__SELECT_H

#include "IEventLoop.h"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace ba_socket {
    class Event_Loop__Select : public IEventLoop {
    public:
        using Callback = std::function<void(const Socket&)>;

        Event_Loop__Select(
            Callback on_read,
            Callback on_write,
            Callback on_disconnect)
            : _on_read(on_read),
            _on_write(on_write),
            _on_disconnect(on_disconnect)
        {
            FD_ZERO(&_readfds);
            FD_ZERO(&_writefds);
        }

        void register_fd(const Socket& s, EventType type) override {
            std::lock_guard lock(_mtx);
            int fd = s.fd();

            if (type == EventType::Read)
                _read_map[fd] = s;
            else
                _write_map[fd] = s;

            update_fd_sets();
        }

        void unregister_fd(const Socket& s) override {
            std::lock_guard lock(_mtx);
            int fd = s.fd();
            _read_map.erase(fd);
            _write_map.erase(fd);
            update_fd_sets();
        }

        void run() override {
            _running.store(true);

            while (_running.load()) {
                fd_set rcopy = _readfds;
                fd_set wcopy = _writefds;

                int nfds = _max_fd + 1;

                int ready = ::select(nfds, &rcopy, &wcopy, nullptr, nullptr);
                if (ready < 0) continue;

                // Process readable
                for (auto& [fd, sock] : _read_map) {
                    if (FD_ISSET(fd, &rcopy))
                        _on_read(sock);
                }

                // Process writable
                for (auto& [fd, sock] : _write_map) {
                    if (FD_ISSET(fd, &wcopy))
                        _on_write(sock);
                }
            }
        }

        void stop() override { _running.store(false); }

    private:
        void update_fd_sets() {
            FD_ZERO(&_readfds);
            FD_ZERO(&_writefds);

            _max_fd = 0;

            for (auto& [fd, sock] : _read_map) {
                FD_SET(fd, &_readfds);
                if (fd > _max_fd) _max_fd = fd;
            }

            for (auto& [fd, sock] : _write_map) {
                FD_SET(fd, &_writefds);
                if (fd > _max_fd) _max_fd = fd;
            }
        }

        std::mutex _mtx;
        std::atomic<bool> _running{false};

        fd_set _readfds;
        fd_set _writefds;

        int _max_fd = 0;

        std::unordered_map<int, Socket> _read_map;
        std::unordered_map<int, Socket> _write_map;

        Callback _on_read, _on_write, _on_disconnect;
    };
} // namespace ba_socket

#endif // EVENT_LOOP__SELECT_H
