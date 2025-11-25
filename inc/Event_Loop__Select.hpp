// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <tuple>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace BA_Socket {
    class Event_Loop__Select : public IEvent_Loop {
    public:
        Event_Loop__Select() {
            FD_ZERO(&_fd_set__read);
            FD_ZERO(&_fd_set__write);
        }

        inline void fd_register(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fd_set__read);
            }
            else if (event_type == Enum_Event_Types::Write) {
                FD_SET(fd, &_fd_set__write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::None) return;
            
            if (event_type == Enum_Event_Types::Read) {
                FD_CLR(fd, &_fd_set__read);
            }
            else if (event_type == Enum_Event_Types::Write) {
                FD_CLR(fd, &_fd_set__write);
            }
            if (fd == _fd_max) {
                if (
                    (event_type == Enum_Event_Types::Read && FD_ISSET(fd, &_fd_set__write)) ||
                    (event_type == Enum_Event_Types::Write && FD_ISSET(fd, &_fd_set__read)))
                {
                    return;
                }

                auto fd_max = _fd_max;
                _fd_max = -1;
                for(SOCKET fd = 0; fd <= fd_max; ++fd) {
                    if (FD_ISSET(fd, &_fd_set__read)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                    if (FD_ISSET(fd, &_fd_set__write)) {
                        if (fd > _fd_max) _fd_max = fd;
                    }
                }
            }
        }

        inline void add_handler(handler_ptr_t&& handler, Enum_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                _handlers__read[handler->get_fd()] = std::move(handler);
            }
            else if (event_type == Enum_Event_Types::Write) {
                _handlers__write[handler->get_fd()] = std::move(handler);
            }
        }

        inline void remove_handler(int fd, Enum_Event_Types event_type) {
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                _handlers__read.erase(fd);
            }
            else if (event_type == Enum_Event_Types::Write) {
                _handlers__write.erase(fd);
            }
        }

        inline void run() override {
            _running.store(true);
            while (_running.load()) {
                if (_fd_max < 0) break;

                // perform select operation
                fd_set fd_set__read = _fd_set__read;
                fd_set fd_set__write = _fd_set__write;
                if (::select(_fd_max + 1, &fd_set__read, &fd_set__write, nullptr, nullptr) < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__SELECT();
                }

                // collect the fd_set actions and the handler actions
                // to apply after the loop of current handlers.
                // the 1st bool parameter:
                //   true: register fd / add handler
                //   false: unregister fd / remove handler
                using new_action_t = std::tuple<
                    Enum_Register_Types,
                    Enum_Handler_Action_Types,
                    Enum_Event_Types,
                    int,
                    handler_ptr_t>;
                std::vector<new_action_t> new_actions;

                // execute _handlers - read
                for (auto& [fd, handler_ptr] : _handlers__read) {
                    // execute the handler
                    handler_return_pack_t handler_return_pack;
                    if (FD_ISSET(fd, &fd_set__read)) {
                        if (!handler_ptr) continue;
                        handler_return_pack = handler_ptr->apply();
                    }
                    else continue;
                    
                    // loop through the handler return pack:
                    for (auto& handler_return : handler_return_pack) {
                        new_actions.push_back({
                            handler_return.first._register_type,
                            handler_return.first._handler_action_type,
                            handler_return.first._event_type,
                            handler_return.first._fd,
                            std::move(handler_return.second) });
                    }
                }

                // execute _handlers - write
                for (auto& [fd, handler_ptr] : _handlers__write) {
                    // execute the handler
                    handler_return_pack_t handler_return_pack;
                    if (FD_ISSET(fd, &fd_set__write)) {
                        if (!handler_ptr) continue;
                        handler_return_pack = handler_ptr->apply();
                    }
                    else continue;

                    // loop through the handler return pack:
                    for (auto& handler_return : handler_return_pack) {
                        new_actions.push_back({
                            handler_return.first._register_type,
                            handler_return.first._handler_action_type,
                            handler_return.first._event_type,
                            handler_return.first._fd,
                            std::move(handler_return.second) });
                    }
                }

                // update the fd_sets and the handler maps.
                Enum_Register_Types register_type;
                Enum_Handler_Action_Types handler_action_type;
                Enum_Event_Types event_type;
                handler_ptr_t handler__new;
                for (auto& [register_type, handler_action_type, event_type, fd, handler__new] : new_actions) {
                    if (register_type == Enum_Register_Types::Unregister) {
                        fd_unregister(fd, event_type);
                    }
                    else if (register_type == Enum_Register_Types::Register) {
                        fd_register(fd, event_type);
                    }
                    if (
                        handler_action_type == Enum_Handler_Action_Types::Add ||
                        handler_action_type == Enum_Handler_Action_Types::Replace)
                    {
                        add_handler(std::move(handler__new), event_type);
                    }
                    else if (handler_action_type == Enum_Handler_Action_Types::Remove)
                    {
                        remove_handler(fd, event_type);
                    }
                }
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:
        std::unordered_map<int, handler_ptr_t> _handlers__read;
        std::unordered_map<int, handler_ptr_t> _handlers__write;
        fd_set _fd_set__read;
        fd_set _fd_set__write;
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
