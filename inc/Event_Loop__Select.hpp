// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace BA_Socket {
    class Event_Loop__Select : public IEvent_Loop {
    public:
        Event_Loop__Select(
            Callback on_read,
            Callback on_write,
            Callback on_disconnect)
            :
            _on_read(on_read),
            _on_write(on_write),
            _on_disconnect(on_disconnect)
        {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        inline void register_fd(const Socket& s, EventType type) override {
            std::lock_guard lock(_mtx);
            int fd = s.native_handle();
            if (type == EventType::Read)
                _read_map[fd] = s;
            else
                _write_map[fd] = s;

            update_fd_sets();
        }

        inline void unregister_fd(const Socket& s) override {
            std::lock_guard lock(_mtx);
            int fd = s.native_handle();
            _read_map.erase(fd);
            _write_map.erase(fd);
            update_fd_sets();
        }

        void run() override {
            _running.store(true);
            while (_running.load()) {
                fd_set rcopy = _fds_read;
                fd_set wcopy = _fds_write;

                int nfds = _max_fd + 1;
                int ready = ::select(nfds, &rcopy, &wcopy, nullptr, nullptr);
                if (ready < 0) continue;

                for (auto& [fd, sock] : _read_map)
                    if (FD_ISSET(fd, &rcopy))
                        _on_read(sock);
                for (auto& [fd, sock] : _write_map)
                    if (FD_ISSET(fd, &wcopy))
                        _on_write(sock);
            }
        }

        inline void stop() override { _running.store(false); }

    private:
        void update_fd_sets() {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
            _max_fd = 0;
            for (auto& [fd, sock] : _read_map) {
                FD_SET(fd, &_fds_read);
                if (fd > _max_fd) _max_fd = fd;
            }
            for (auto& [fd, sock] : _write_map) {
                FD_SET(fd, &_fds_write);
                if (fd > _max_fd) _max_fd = fd;
            }
        }

        std::unordered_map<SOCKET, Socket> _read_map;
        std::unordered_map<SOCKET, Socket> _write_map;
        Callback _on_read, _on_write, _on_disconnect;
        fd_set _fds_read;
        fd_set _fds_write;
        int _max_fd = 0;
        std::mutex _mtx;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
