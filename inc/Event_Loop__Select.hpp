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

        struct Handler_Entry {
            handler_ptr_t _handler__read{ nullptr };
            handler_ptr_t _handler__write{ nullptr };
            bool _active{ true };

            Handler_Entry(
                handler_ptr_t handler__read = nullptr,
                handler_ptr_t handler__write = nullptr,
                bool active = true)
                :
                _handler__read(std::move(handler__read)),
                _handler__write(std::move(handler__write)),
                _active(active) {}                
        };

    public:
        Event_Loop__Select(time_t sec = 0, suseconds_t usec = 0)
            : _sec(sec), _usec(usec)
        {
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
            else { // if (event_type == Enum_Event_Types::Read_Write) {
                FD_SET(fd, &_fd_set__read);
                FD_SET(fd, &_fd_set__write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd) override {
            FD_CLR(fd, &_fd_set__read);
            FD_CLR(fd, &_fd_set__write);
            if (fd == _fd_max) {
                auto fd_max = _fd_max;
                _fd_max = -1;
                for(SOCKET fdi = 0; fdi <= fd_max; ++fdi) {
                    if (FD_ISSET(fdi, &_fd_set__read)) {
                        if (fdi > _fd_max) _fd_max = fdi;
                    }
                    if (FD_ISSET(fdi, &_fd_set__write)) {
                        if (fdi > _fd_max) _fd_max = fdi;
                    }
                }
            }
        }

        inline void add_stdin_to_reads() {
            FD_SET(0, &_fd_set__read);
        }

        inline void add_stdout_to_writes() {
            FD_SET(1, &_fd_set__write);
        }

        inline void add_handler(handler_ptr_t&& handler, Enum_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                _handlers[handler->get_fd()]._handler__read = std::move(handler);
            }
            else if (event_type == Enum_Event_Types::Write) {
                _handlers[handler->get_fd()]._handler__write = std::move(handler);
            }
        }

        inline void run() override {
            _running.store(true);

            // call select to register running file descriptors to the fd_set.
            // select needs to wait for some time,
            // as windows doesn't support fd_set for stdin, we use a timeout loop.
            struct timeval timeout;
            timeval *timeout_ptr = nullptr;
            if (_sec || _usec) {
                timeout.tv_sec = _sec;
                timeout.tv_usec = _usec;
                timeout_ptr = &timeout;
            }

            while (_running.load()) {
                if (_fd_max < 0) break;

                // perform select operation
                fd_set fd_set__read = _fd_set__read;
                fd_set fd_set__write = _fd_set__write;
                if (::select(_fd_max + 1, &fd_set__read, &fd_set__write, nullptr, timeout_ptr) < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__SELECT();
                    break;
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
                for (auto& [fd, handler_entry] : _handlers) {
                    // inspect the fd, the handler entry and the handler
                    if (!handler_entry._active) continue;
                    if (!IS_VALID_SOCKET(fd)) continue;
                    if (FD_ISSET(fd, &fd_set__read)) continue;
#if defined(_WIN32)
                    if (fd == 0 && !_kbhit()) continue;
#else
                    if (!FD_ISSET(fd, &fd_set__read)) continue;
#endif
                    auto handler_ptr = handler_entry._handler__read.get();
                    if (!handler_ptr) continue;

                    // execute the handler
                    handler_return_pack_t handler_return_pack = handler_ptr->apply();
                    
                    // loop through the handler return pack:
                    for (auto& handler_return : handler_return_pack) {
                        new_actions.push_back({
                            handler_return.first._register_type,
                            handler_return.first._handler_action_type,
                            handler_return.first._event_type,
                            handler_return.first._fd,
                            std::move(handler_return.second) });
                        if (handler_return.first._register_type == Enum_Register_Types::Unregister) {
                            handler_entry._active = false;
                        }
                    }
                }

                // execute _handlers - write
                for (auto& [fd, handler_entry] : _handlers) {
                    // inspect the fd, the handler entry and the handler
                    if (!handler_entry._active) continue;
                    if (!IS_VALID_SOCKET(fd)) continue;
                    if (FD_ISSET(fd, &fd_set__write)) continue;
                    auto handler_ptr = handler_entry._handler__write.get();
                    if (!handler_ptr) continue;

                    // execute the handler
                    handler_return_pack_t handler_return_pack = handler_ptr->apply();

                    // loop through the handler return pack:
                    for (auto& handler_return : handler_return_pack) {
                        new_actions.push_back({
                            handler_return.first._register_type,
                            handler_return.first._handler_action_type,
                            handler_return.first._event_type,
                            handler_return.first._fd,
                            std::move(handler_return.second) });
                        if (handler_return.first._register_type == Enum_Register_Types::Unregister) {
                            handler_entry._active = false;
                        }
                    }
                }

                // update the fd_sets and the handler maps.
                Enum_Register_Types register_type;
                Enum_Handler_Action_Types handler_action_type;
                Enum_Event_Types event_type;
                handler_ptr_t handler__new;
                for (auto& [register_type, handler_action_type, event_type, fd, handler__new] : new_actions) {
                    if (register_type == Enum_Register_Types::Unregister) {
                        fd_unregister(fd);
                        _handlers.erase(fd);
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
                }
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:
        std::unordered_map<int, Handler_Entry> _handlers;
        fd_set _fd_set__read;
        fd_set _fd_set__write;
        time_t _sec{0};
        suseconds_t _usec{0};
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
