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
            FD_ZERO(&_fd_set_read);
            FD_ZERO(&_fd_set_write);
        }

        inline void fd_register(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fd_set_read);
            }
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Write) {
                FD_SET(fd, &_fd_set_write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Read) {
                FD_CLR(fd, &_fd_set_read);
            }
            if (event_type == Enum_Event_Types::Read_Write || event_type == Enum_Event_Types::Write) {
                FD_CLR(fd, &_fd_set_write);
            }
            if (fd == _fd_max) {
                if (
                    (event_type == Enum_Event_Types::Read && FD_ISSET(fd, &_fd_set_write)) ||
                    (event_type == Enum_Event_Types::Write && FD_ISSET(fd, &_fd_set_read)))
                {
                    return;
                }

                auto fd_max = _fd_max;
                _fd_max = -1;
                for(SOCKET fd = 0; fd <= fd_max; ++fd) {
                    if (FD_ISSET(fd, &_fd_set_read)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                    if (FD_ISSET(fd, &_fd_set_write)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                }
            }
        }

        inline void run() override {
            _running.store(true);
            while (_running.load()) {
                if (_fd_max < 0) break;

                // perform select operation
                fd_set fd_set_read = _fd_set_read;
                fd_set fd_set_write = _fd_set_write;
                if (::select(_fd_max + 1, &fd_set_read, &fd_set_write, nullptr, nullptr) < 0) {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }

                // initialize a new map to collect the new handlers
                // those should be added to or removed from _handlers
                //   true: handler to be added
                //   false: handler to be removed
                std::unordered_map<int, std::pair<bool, std::unique_ptr<IHandler>>> new_handler_actions;

                // execute _handlers
                for (auto& [fd, handler] : _handlers) {
                    if (!handler) continue;

                    // execute the handler
                    handler_return_t handler_return;
                    if (FD_ISSET(fd, &_fd_set_read)) 
                        handler_return = handler->on_read(fd);
                    else if (FD_ISSET(fd, &_fd_set_write)) 
                        handler_return = handler->on_write(fd);
                    else continue;
                    auto fd_set_actions = std::move(handler_return.first);
                    auto handler_action = std::move(handler_return.second);

                    // loop through the fd_set actions reulted from the handler
                    for (const auto& fd_set_action : fd_set_actions) {
                        if (fd_set_action._register_type == Enum_Register_Types::Register) {
                            fd_register(fd_set_action._fd, fd_set_action._event_type);
                        }
                        else if (fd_set_action._register_type == Enum_Register_Types::Unregister) {
                            fd_unregister(fd_set_action._fd, fd_set_action._event_type);
                        }
                    }
                    
                    // perform the handler action resulted from the handler:
                    //   collect the handlers those should be added to or removed from _handlers
                    if(handler_action.first == Enum_Handler_Action_Types::None) continue;
                    if(handler_action.first == Enum_Handler_Action_Types::Add) {
                        new_handler_actions[fd] = { true, std::move(handler_action.second) };
                    }
                    else if(handler_action.first == Enum_Handler_Action_Types::Remove) {
                        new_handler_actions[fd] = { false, std::move(handler_action.second) };
                    }
                    else { // if(handler_action.first == Enum_Handler_Action_Types::Replace) {
                        new_handler_actions[fd] = { true, std::move(handler_action.second) };
                    }
                }

                // update _handlers by the collected handlers
                // those should be added to or removed from _handlers
                for (auto& [fd, handler_action_pair] : new_handler_actions) {
                    if(handler_action_pair.first) {
                        _handlers[fd] = std::move(handler_action_pair.second);
                    }
                    else {
                        _handlers.erase(fd);
                    }
                }
            }
        }

        inline void stop() override {
            _running.store(false);
        }

        inline void close_sockets() override {
            for (SOCKET fd = 0; fd <= _fd_max; ++fd) {
                if (FD_ISSET(fd, &_fd_set_read)) {
                    CLOSE_SOCKET(fd);
                }
                if (FD_ISSET(fd, &_fd_set_write)) {
                    CLOSE_SOCKET(fd);
                }
            }
        }

    private:
        std::unordered_map<int, std::unique_ptr<IHandler>> _handlers;
        fd_set _fd_set_read;
        fd_set _fd_set_write;
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
