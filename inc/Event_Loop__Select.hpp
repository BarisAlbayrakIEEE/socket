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
        Event_Loop__Select() {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        inline void fd_register(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fds_read);
            } else {
                FD_SET(fd, &_fds_write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Read) {
                FD_CLR(fd, &_fds_read);
            }
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Write) {
                FD_CLR(fd, &_fds_read);
            }
            if (fd == _fd_max) {
                if (
                    (event_type == Enum_Event_Types::Read && FD_ISSET(fd, &_fds_write)) ||
                    (event_type == Enum_Event_Types::Write && FD_ISSET(fd, &_fds_read)))
                {
                    return;
                }

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

                std::unordered_map<int, std::pair<bool, std::unique_ptr<IHandler>>> handler_actions;
                for (auto& [fd, handler] : _handlers) {
                    if (!handler) continue;

                    auto handler_return = handler->apply(fd);
                    auto fd_set_actions = std::move(handler_return.first);
                    auto handler_actions = std::move(handler_return.second);
                    for (const auto& fd_set_action : fd_set_actions) {
                        if (fd_set_action._register_type == Enum_Register_Types::Register) {
                            fd_register(fd_set_action._fd, fd_set_action._event_type);
                        }
                        else if (fd_set_action._register_type == Enum_Register_Types::Unregister) {
                            fd_unregister(fd_set_action._fd, fd_set_action._event_type);
                        }
                        if ()
                        
                    }
                }
            }
        }
    using fd_set_actions_t = std::vector<fd_set_Action>;
    using handler_action_t = std::pair<Enum_Handler_Action_Types, std::unique_ptr<IHandler>>;
    using handler_return_t = std::pair<fd_set_actions_t, handler_action_t>;

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
        std::unordered_map<int, std::unique_ptr<IHandler>> _handlers;
        fd_set _fds_read;
        fd_set _fds_write;
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
