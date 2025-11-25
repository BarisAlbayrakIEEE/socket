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

        inline void add_handler(int fd, std::unique_ptr<IHandler>&& handler) {
            _handlers[fd] = std::move(handler);
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

                // collect the fd_set actions and the handler actions
                // to apply after the loop of current handlers.
                // the 1st bool parameter:
                //   true: register fd / add handler
                //   false: unregister fd / remove handler
                std::vector<std::tuple<bool, int, Enum_Event_Types>> new_actions__fd_set;
                std::vector<std::pair<bool, std::unique_ptr<IHandler>>> new_actions__handler;

                // execute _handlers
                int fd;
                std::unique_ptr<IHandler> handler__current;
                for (auto& [fd, handler__current] : _handlers) {
                    if (!handler__current) continue;

                    // execute the handler
                    handler_return_t handler_returns;
                    if (FD_ISSET(fd, &fd_set_read)) 
                        handler_returns = handler__current->on_read();
                    else if (FD_ISSET(fd, &fd_set_write)) 
                        handler_returns = handler__current->on_write();
                    else continue;
                    
                    // loop through the fd_set actions resulted from the handler:
                    //   collect the new actions for the fd_sets
                    //   collect the new actions for the handlers
                    for (auto& handler_return : handler_returns) {
                        auto reactor_command = std::move(handler_return.first);
                        auto handler__new = std::move(handler_return.second);
                        auto fd__new = handler__new->get_fd();

                        // collect the new actions for the fd_sets
                        if (reactor_command._register_type == Enum_Register_Types::Register) {
                            new_actions__fd_set.push_back({ true, fd__new, reactor_command._event_type });
                        }
                        else if (reactor_command._register_type == Enum_Register_Types::Unregister) {
                            new_actions__fd_set.push_back({ false, fd__new, reactor_command._event_type });
                        }

                        // collect the new actions for the handlers
                        if (
                            reactor_command._handler_action_type == Enum_Handler_Action_Types::Add ||
                            reactor_command._handler_action_type == Enum_Handler_Action_Types::Replace)
                        {
                            new_actions__handler.push_back({ true, std::move(handler__new) });
                        }
                        else if (reactor_command._handler_action_type == Enum_Handler_Action_Types::Remove) {
                            new_actions__handler.push_back({ false, std::move(handler__new) });
                        }
                    }
                }

                // update the fd_sets and the _handlers.
                bool add;
                Enum_Event_Types event_type;
                for (auto& [add, fd, event_type] : new_actions__fd_set) {
                    if (add) fd_register(fd, event_type);
                    else fd_unregister(fd, event_type);
                }
                for (auto& [add, handler__new] : new_actions__handler) {
                    if (add) _handlers[handler__new->get_fd()] = (std::move(handler__new));
                    else _handlers.erase(handler__new->get_fd());
                }
            }
        }

        inline void stop() override {
            _running.store(false);
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
