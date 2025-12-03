// Event_Loop__Mid__ST.hpp :
//   Platform                 : Cross-platform
//   Performance              : Mid (poll-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Single-threaded

#ifndef EVENT_LOOP__MID__ST_HPP
#define EVENT_LOOP__MID__ST_HPP

#include "Event_Loop__Mid__Base.hpp"

namespace BA_Socket {

    template <>
    class Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::ST>
            : public IEvent_Loop, public Event_Loop__Mid__Base
    {
    public:
        Event_Loop(timeout_x msec = -1) : _msec(msec) {}

        inline void fd_register(int fd, Enum_IO_Event_Types IO_event_type) override {
            Event_Loop__Mid__Base::fd_register(
                _pollfds,
                fd,
                IO_event_type);
        }

        inline void fd_unregister(int fd, Enum_IO_Event_Types IO_event_type) override {
            Event_Loop__Mid__Base::fd_unregister(
                _event_handler_entrys,
                _pollfds,
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
            while (_running.load()) {
                if (_pollfds.empty()) break;

                // perform poll operation
                std::vector<pollfd_x> pollfds_copy = _pollfds;
                int status = poll_x(
                    pollfds_copy.data(),
                    static_cast<nfds_x>(pollfds_copy.size()),
                    _msec);
                if (status < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__MID();
                    break;
                }
                if (status == 0) continue; // timeout

                // dispatch the events
                reactor_event_pack_t rep_next;
                dispatch_events(rep_next, pollfds_copy);

                // apply the reactor events
                apply_reactor_events(rep_next);
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:

        // dispatch the events
        void dispatch_events(
            reactor_event_pack_t& rep_next,
            const std::vector<pollfd_x>& pollfds_copy)
        {
            for (const auto& pollfd_ : pollfds_copy) {
                // get the event handler entry from the pollfd
                auto event_handler_entry_ptr = Event_Loop__Mid__Base::get_event_handler_entry_ptr(
                    _event_handler_entrys,
                    pollfd_,
                    true);
                if (!event_handler_entry_ptr) continue;

                // dispatch the read event
                if ((pollfd_.revents & POLL_X_IN) && event_handler_entry_ptr->_event_handler__read) {
                    auto rep = event_handler_entry_ptr->_event_handler__read->apply(pollfd_.fd);
                    for (auto& re : rep) {
                        /*
                        Multi-thread only
                        if (re._register_type == Enum_Register_Types::Unregister) {
                            event_handler_entry_ptr->_active = false;
                        }
                        */
                        rep_next.push_back(std::move(re));
                    }
                }

                // dispatch the write event
                if ((pollfd_.revents & POLL_X_OUT) && event_handler_entry_ptr->_event_handler__write) {
                    auto rep = event_handler_entry_ptr->_event_handler__write->apply(pollfd_.fd);
                    for (auto& re : rep) {
                        /*
                        Multi-thread only
                        if (re._register_type == Enum_Register_Types::Unregister) {
                            event_handler_entry_ptr->_active = false;
                        }
                        */
                        rep_next.push_back(std::move(re));
                    }
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
        std::vector<pollfd_x> _pollfds;
        timeout_x _msec{ -1 };
        std::atomic<bool> _running{ false };
    };

    using Event_Loop__Mid__ST_t = Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::ST>;
} // namespace BA_Socket

#endif // EVENT_LOOP__MID__ST_HPP
