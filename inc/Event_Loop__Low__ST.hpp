// Event_Loop__Low__ST.hpp :
//   Platform                 : Cross-platform
//   Performance              : Low (select-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Single-threaded

#ifndef EVENT_LOOP__LOW__ST_HPP
#define EVENT_LOOP__LOW__ST_HPP

#include "Event_Loop__Low__Base.hpp"

namespace BA_Socket {
    template <>
    class Event_Loop<
        Enum_Event_Loop_Types::Low,
        Enum_Concurrency_Types::ST>
            : public IEvent_Loop, public Event_Loop__Low__Base
    {
    public:
        Event_Loop(time_t sec = 0, suseconds_t usec = 0)
            : _sec(sec), _usec(usec)
        {
            FD_ZERO(&_fd_set__read);
            FD_ZERO(&_fd_set__write);
        }

        inline void fd_register(int fd, Enum_IO_Event_Types IO_event_type) override {
            Event_Loop__Low__Base::fd_register(
                &_fd_set__read,
                &_fd_set__write,
                &_fd_max,
                fd,
                IO_event_type);
        }

        inline void fd_unregister(int fd, Enum_IO_Event_Types IO_event_type) override {
            Event_Loop__Low__Base::fd_unregister(
                _event_handler_entrys,
                &_fd_set__read,
                &_fd_set__write,
                &_fd_max,
                fd,
                IO_event_type,
                true);
        }

        inline void add_event_handler(
            int fd,
            event_handler_ptr_t&& event_handler,
            Enum_IO_Event_Types IO_event_type) override
        {
            Event_Loop__Base::add_event_handler(
                _event_handler_entrys,
                fd,
                std::move(event_handler),
                IO_event_type);
        }

        inline void remove_event_handler(
            int fd,
            Enum_IO_Event_Types IO_event_type) override
        {
            Event_Loop__Base::remove_event_handler(
                _event_handler_entrys,
                fd,
                IO_event_type);
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
            Enum_IO_Event_Types IO_event_type)
        {
            for (auto& [fd, event_handler_entry] : _event_handler_entrys) {
                // get the event handler
                auto event_handler_ptr = Event_Loop__Low__Base::get_event_handler_ptr(
                    fd,
                    fd_set__read,
                    fd_set__write,
                    event_handler_entry,
                    IO_event_type,
                    true);
                if (!event_handler_ptr) continue;

                // dispatch the event
                auto rep = event_handler_ptr->apply(fd);
                for (auto& re : rep) {
                    /*
                    Multi-thread only
                    if (re._register_type == Enum_Register_Types::Unregister) {
                        event_handler_entry._active = false;
                    }
                    */
                    rep_next.push_back(std::move(re));
                }
            }
        }

        // apply the reactor events
        void apply_reactor_events(reactor_event_pack_t& rep_next) {
            for (auto& [fd, register_type, IO_event_type, event_handler_action_type, event_handler__new] : rep_next) {
                if (register_type == Enum_Register_Types::Unregister) {
                    fd_unregister(fd, IO_event_type);
                }
                else if (register_type == Enum_Register_Types::Register) {
                    fd_register(fd, IO_event_type);
                }
                if (
                    event_handler_action_type == Enum_Event_Handler_Action_Types::Add ||
                    event_handler_action_type == Enum_Event_Handler_Action_Types::Replace)
                {
                    add_event_handler(fd, std::move(event_handler__new), IO_event_type);
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

    using Event_Loop__Low__ST_t = Event_Loop<
        Enum_Event_Loop_Types::Low,
        Enum_Concurrency_Types::ST>;
} // namespace BA_Socket

#endif // EVENT_LOOP__LOW__ST_HPP
