// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>

namespace BA_Socket {
    class Event_Loop__Select : public IEvent_Loop {
    public:
        Event_Loop__Select(Callback on_read, Callback on_write)
            : _on_read(on_read), _on_write(on_write)
        {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        inline void fd_register(int fd, Enum_Event_Types type) override {
            if (type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fds_read);
            } else {
                FD_SET(fd, &_fds_write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd) override {
            FD_CLR(fd, &_fds_read);
            FD_CLR(fd, &_fds_write);
            if (fd == _fd_max) {
                auto fd_max = _fd_max;
                _fd_max = -1;
                for(SOCKET fd = 0; fd <= fd_max; ++fd) {
                    if (FD_ISSET(fd, &_fds_read)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                    if (FD_ISSET(fd, &_fds_write)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                }
            }
        }

        inline void run() override {
            _running.store(true);
            while (_running.load()) {
                if (_fd_max < 0) break;

                fd_set fds_read = _fds_read;
                fd_set fds_write = _fds_write;
                if (::select(
                    _fd_max + 1,
                    &fds_read,
                    &fds_write,
                    nullptr,
                    nullptr) < 0)
                {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }
                for(SOCKET fd = 0; fd <= _fd_max; ++fd) {
                    if (FD_ISSET(fd, &fds_read)) {
                        auto rcp = _on_read(fd);
                        apply_rcp(rcp);
                    }
                    if (FD_ISSET(fd, &fds_write)) {
                        auto rcp = _on_write(fd);
                        apply_rcp(rcp);
                    }
                }
            }
            stop();
        }

        inline void stop() override {
            _running.store(false);
            for (SOCKET fd = 0; fd <= _fd_max; ++fd) {
                if (FD_ISSET(fd, &_fds_read)) {
                    CLOSE_SOCKET(fd);
                }
                if (FD_ISSET(fd, &_fds_write)) {
                    CLOSE_SOCKET(fd);
                }
            }
        }

        inline void close_sockets() override {
            for (SOCKET fd = 0; fd <= _fd_max; ++fd) {
                if (FD_ISSET(fd, &_fds_read)) {
                    CLOSE_SOCKET(fd);
                }
                if (FD_ISSET(fd, &_fds_write)) {
                    CLOSE_SOCKET(fd);
                }
            }
        }

        inline void apply_rcp(const Reactor_Command_Pack& rcp) {
            for (const auto& rc : rcp._rcs) {
                switch (rc.first) {
                case Enum_Reactor_Command_Types::RegisterRead:
                    fd_register(rc.second, Enum_Event_Types::Read);
                    break;
                case Enum_Reactor_Command_Types::RegisterWrite:
                    fd_register(rc.second, Enum_Event_Types::Write);
                    break;
                case Enum_Reactor_Command_Types::Unregister:
                    fd_unregister(rc.second);
                    break;
                case Enum_Reactor_Command_Types::Error:
                    break;
                case Enum_Reactor_Command_Types::eintr:
                    break;
                default:
                    break;
                }
            }
        }

    private:

        fd_set _fds_read;
        fd_set _fds_write;
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
