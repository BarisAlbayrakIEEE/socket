// Event_Loop__Select__ST.hpp
// Single-threaded select event loop

#ifndef EVENT_LOOP__SELECT__ST_HPP
#define EVENT_LOOP__SELECT__ST_HPP

#include "IEvent_Loop.hpp"
#include <tuple>
#include <unordered_map>
#include <atomic>

namespace BA_Socket {
    class Event_Loop__Select__ST : public IEvent_Loop {
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
        Event_Loop__Select__ST(time_t sec = 0, suseconds_t usec = 0)
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

        inline void add_handler(int fd, handler_ptr_t&& handler, Enum_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                _handler_entrys[fd]._handler__read = std::move(handler);
            }
            else if (event_type == Enum_Event_Types::Write) {
                _handler_entrys[fd]._handler__write = std::move(handler);
            }
        }

        inline void run() override {
            _running.store(true);

            // main loop
            while (_running.load()) {
                if (_fd_max < 0) break;

                // perform select operation
                fd_set fd_set__read = _fd_set__read;
                fd_set fd_set__write = _fd_set__write;

                // windows only:
                //   Windows doesn't support fd_set for stdin.
                //   So, a timeout loop is required.
                struct timeval timeout;
                struct timeval* timeout_ptr = nullptr;
                if (_sec || _usec) {
                    timeout.tv_sec  = _sec;
                    timeout.tv_usec = _usec;
                    timeout_ptr     = &timeout;
                }
                if (::select(_fd_max + 1, &fd_set__read, &fd_set__write, nullptr, timeout_ptr) < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__SELECT();
                    break;
                }

                // execute _handler_entrys - read
                reactor_command_pack_t reactor_commands;
                for (auto& [fd, handler_entry] : _handler_entrys) {
                    // inspect the fd, the handler entry and the handler
                    if (!handler_entry._active) continue;
                    if (!IS_VALID_SOCKET(fd)) continue;
#if defined(_WIN32)
                    if (fd == 0) {
                        if (!_kbhit()) continue;
                    } else {
                        if (!FD_ISSET(fd, &fd_set__read)) continue;
                    }
#else
                    if (!FD_ISSET(fd, &fd_set__read)) continue;
#endif
                    auto handler_ptr = handler_entry._handler__read.get();
                    if (!handler_ptr) continue;

                    // execute the handler
                    auto reactor_command_pack = handler_ptr->apply(fd);
                    for (auto& reactor_command : reactor_command_pack) {
                        if (reactor_command._register_type == Enum_Register_Types::Unregister) {
                            handler_entry._active = false;
                        }
                        reactor_commands.push_back(std::move(reactor_command));
                    }
                }

                // execute _handler_entrys - write
                for (auto& [fd, handler_entry] : _handler_entrys) {
                    // inspect the fd, the handler entry and the handler
                    if (!handler_entry._active) continue;
                    if (!IS_VALID_SOCKET(fd)) continue;
                    if (!FD_ISSET(fd, &fd_set__write)) continue;
                    auto handler_ptr = handler_entry._handler__write.get();
                    if (!handler_ptr) continue;

                    // execute the handler
                    auto reactor_command_pack = handler_ptr->apply(fd);
                    for (auto& reactor_command : reactor_command_pack) {
                        if (reactor_command._register_type == Enum_Register_Types::Unregister) {
                            handler_entry._active = false;
                        }
                        reactor_commands.push_back(std::move(reactor_command));
                    }
                }

                // update the fd_sets and the handler maps.
                for (auto& [fd, register_type, event_type, handler_command_type, handler__new] : reactor_commands) {
                    if (register_type == Enum_Register_Types::Unregister) {
                        fd_unregister(fd);
                        _handler_entrys.erase(fd);
                    }
                    else if (register_type == Enum_Register_Types::Register) {
                        fd_register(fd, event_type);
                    }
                    if (
                        handler_command_type == Enum_Handler_Command_Types::Add ||
                        handler_command_type == Enum_Handler_Command_Types::Replace)
                    {
                        add_handler(fd, std::move(handler__new), event_type);
                    }
                }
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:
        std::unordered_map<int, Handler_Entry> _handler_entrys;
        fd_set _fd_set__read;
        fd_set _fd_set__write;
        time_t _sec{0};
        suseconds_t _usec{0};
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT__ST_HPP
