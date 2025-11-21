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
        using Callback = std::function<void(const Socket&)>;
        using sockmap_t = std::unordered_map<SOCKET, Socket>;
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

        inline void fd_register(const Socket& s, Enum_Event_Types type) override {
            std::scoped_lock lk(_m);

            int fd = s.native_handle();
            if (type == Enum_Event_Types::Read) _sockmap_read[fd] = s;
            else _sockmap_write[fd] = s;
            update_fd_sets();
        }

        inline void fd_unregister(const Socket& s) override {
            std::scoped_lock lk(_m);

            int fd = s.native_handle();
            _sockmap_read.erase(fd);
            _sockmap_write.erase(fd);
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

                for (auto& [fd, sock] : _sockmap_read) if (FD_ISSET(fd, &rcopy)) _on_read(sock);
                for (auto& [fd, sock] : _sockmap_write) if (FD_ISSET(fd, &wcopy)) _on_write(sock);
            }
        }

        inline void stop() override { _running.store(false); }

    private:
        void update_fd_sets() {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
            _max_fd = 0;
            for (auto& [fd, sock] : _sockmap_read) {
                FD_SET(fd, &_fds_read);
                if (fd > _max_fd) _max_fd = fd;
            }
            for (auto& [fd, sock] : _sockmap_write) {
                FD_SET(fd, &_fds_write);
                if (fd > _max_fd) _max_fd = fd;
            }
        }

        sockmap_t _sockmap_read;
        sockmap_t _sockmap_write;
        Callback _on_read, _on_write, _on_disconnect;
        fd_set _fds_read;
        fd_set _fds_write;
        int _max_fd = 0;
        std::mutex _m;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
