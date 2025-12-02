// Event_Loop__Low__ST.hpp :
//   Platform                 : Cross-platform
//   Performance              : Low (select-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Single-threaded

#ifndef EVENT_LOOP__LOW__ST_HPP
#define EVENT_LOOP__LOW__ST_HPP

#include <unordered_map>
#include <atomic>
#include "IEvent_Loop.hpp"
#include "Event_Handler.hpp"

namespace BA_Socket {
    template <>
    class Event_Loop<
        Enum_Event_Loop_Types::Low,
        Enum_Concurrency_Types::ST>
            : public IEvent_Loop
    {
    public:
        Event_Loop(time_t sec = 0, suseconds_t usec = 0)
            : _sec(sec), _usec(usec)
        {
            FD_ZERO(&_fd_set__read);
            FD_ZERO(&_fd_set__write);
        }

        inline void fd_register(int fd, Enum_IO_Event_Types event_type) override {
            if (event_type == Enum_IO_Event_Types::None) return;

            if (event_type == Enum_IO_Event_Types::Read) {
                FD_SET(fd, &_fd_set__read);
            }
            else if (event_type == Enum_IO_Event_Types::Write) {
                FD_SET(fd, &_fd_set__write);
            }
            else { // if (event_type == Enum_IO_Event_Types::Read_Write) {
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
            _event_handler_entrys.erase(fd);
        }

        inline void add_event_handler(int fd, event_handler_ptr_t&& handler, Enum_IO_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_IO_Event_Types::None) return;

            auto& event_handler_entry = _event_handler_entrys[fd];
            if (event_type == Enum_IO_Event_Types::Read) {
                event_handler_entry._event_handler__read = std::move(handler);
            } else if (event_type == Enum_IO_Event_Types::Write) {
                event_handler_entry._event_handler__write = std::move(handler);
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

                // perform select operation
                if (::select(_fd_max + 1, &fd_set__read, &fd_set__write, nullptr, timeout_ptr) < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__LOW();
                    break;
                }

                // dispatch the events
                reactor_event_pack_t rep_next;
                dispatch_events(rep_next, fd_set__read, fd_set__write);

                // apply the reactor events
                apply_reactor_events(rep_next);
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:

        // get handler from the handler entry
        inline IEvent_Handler* get_event_handler_ptr(
            int fd,
            fd_set fd_set__read,
            fd_set fd_set__write,
            Event_Handler_Entry& event_handler_entry,
            Enum_IO_Event_Types event_type)
        {
            if (!IS_VALID_SOCKET(fd)) return nullptr;
            if (!event_handler_entry._active) return nullptr;
            IEvent_Handler *event_handler_ptr{ nullptr };
            if (event_type == Enum_IO_Event_Types::Read) {
#if defined(_WIN32)
                if (fd == 0) {
                    if (!_kbhit()) return nullptr;
                } else {
                    if (!FD_ISSET(fd, &fd_set__read)) return nullptr;
                }
#else
                if (!FD_ISSET(fd, &fd_set__read)) return nullptr;
#endif
                event_handler_ptr = event_handler_entry._event_handler__read.get();
            }
            else if (event_type == Enum_IO_Event_Types::Write) {
                if (!FD_ISSET(fd, &fd_set__write)) return nullptr;
                event_handler_ptr = event_handler_entry._event_handler__write.get();
            }
            return event_handler_ptr;
        }

        // dispatch the events
        inline void dispatch_events(
            reactor_event_pack_t& rep_next,
            fd_set fd_set__read,
            fd_set fd_set__write)
        {
            dispatch_events_helper(rep_next, fd_set__read, fd_set__write, Enum_IO_Event_Types::Read);
            dispatch_events_helper(rep_next, fd_set__read, fd_set__write, Enum_IO_Event_Types::Write);
        }

        // dispatch the events - helper
        void dispatch_events_helper(
            reactor_event_pack_t& rep_next,
            fd_set fd_set__read,
            fd_set fd_set__write,
            Enum_IO_Event_Types event_type)
        {
            for (auto& [fd, event_handler_entry] : _event_handler_entrys) {
                // get the event handler
                auto event_handler_ptr = get_event_handler_ptr(
                    fd,
                    fd_set__read,
                    fd_set__write,
                    event_handler_entry,
                    event_type);
                if (!event_handler_ptr) continue;

                // dispatch the event
                auto rep = event_handler_ptr->apply(fd);
                for (auto& re : rep) {
                    if (re._register_type == Enum_Register_Types::Unregister) {
                        event_handler_entry._active = false;
                    }
                    rep_next.push_back(std::move(re));
                }
            }
        }

        // apply the reactor events
        void apply_reactor_events(reactor_event_pack_t& rep_next) {
            for (auto& [fd, register_type, event_type, handler_command_type, handler__new] : rep_next) {
                if (register_type == Enum_Register_Types::Unregister) {
                    fd_unregister(fd);
                }
                else if (register_type == Enum_Register_Types::Register) {
                    fd_register(fd, event_type);
                }
                if (
                    handler_command_type == Enum_Handler_Command_Types::Add ||
                    handler_command_type == Enum_Handler_Command_Types::Replace)
                {
                    add_event_handler(fd, std::move(handler__new), event_type);
                }
            }
        }

        std::unordered_map<int, Event_Handler_Entry> _event_handler_entrys;
        fd_set _fd_set__read;
        fd_set _fd_set__write;
        time_t _sec{0};
        suseconds_t _usec{0};
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };

    using Event_Loop__Low__ST = Event_Loop<
        Enum_Event_Loop_Types::Low,
        Enum_Concurrency_Types::ST>;
} // namespace BA_Socket

#endif // EVENT_LOOP__LOW__ST_HPP
