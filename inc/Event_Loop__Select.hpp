// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>

namespace BA_Socket {
    template <typename Handler_Type>
    class Event_Loop__Select : public IEvent_Loop {
    public:
        Event_Loop__Select(Handler_Type on_read, Handler_Type on_write)
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
                if (::select(_fd_max + 1, &fds_read, &fds_write, nullptr, nullptr) < 0) {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }
                for(SOCKET fd = 0; fd <= _fd_max; ++fd) {
                    if (FD_ISSET(fd, &fds_read)) {
                        auto fd_actions = _on_read(fd);
                        apply_fd_action(fd_actions);
                    }
                    if (FD_ISSET(fd, &fds_write)) {
                        auto fd_actions = _on_write(fd);
                        apply_fd_action(fd_actions);
                    }
                }
            }
        }

        inline void stop() override {
            _running.store(false);
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

        inline void apply_fd_action(const std::vector<fd_Action>& fd_actions) {
            for (const auto& fd_action : fd_actions) {
                switch (fd_action._type) {
                case Enum_fd_Action_Types::Register_Read:
                    fd_register(fd_action._fd, Enum_Event_Types::Read);
                    break;
                case Enum_fd_Action_Types::Register_Write:
                    fd_register(fd_action._fd, Enum_Event_Types::Write);
                    break;
                case Enum_fd_Action_Types::None:
                    break;
                default:
                    break;
                }
            }
        }

    private:

        Handler_Type _on_read;
        Handler_Type _on_write;
        fd_set _fds_read;
        fd_set _fds_write;
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
