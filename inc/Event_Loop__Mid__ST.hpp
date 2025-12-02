// Event_Loop__Mid__ST.hpp :
//   Platform                 : Cross-platform
//   Performance              : Mid (poll-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Single-threaded

#ifndef EVENT_LOOP__MID__ST_HPP
#define EVENT_LOOP__MID__ST_HPP

#include <unordered_map>
#include <vector>
#include <atomic>
#include "poll_setup.hpp"
#include "IEvent_Loop.hpp"
#include "Event_Handler.hpp"

namespace BA_Socket {

    template <>
    class Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::ST>
            : public IEvent_Loop
    {
    public:
        Event_Loop(timeout_x msec = -1) : _msec(msec) {}

        inline void fd_register(int fd, Enum_IO_Event_Types event_type) override {
            if (event_type == Enum_IO_Event_Types::None) return;

            short events = 0;
            if (event_type == Enum_IO_Event_Types::Read) {
                events |= POLL_X_IN;
            } else if (event_type == Enum_IO_Event_Types::Write) {
                events |= POLL_X_OUT;
            } else { // Read_Write
                events |= POLL_X_IN | POLL_X_OUT;
            }

            // update or add pollfd
            auto it = std::find_if(
                _pollfds.begin(),
                _pollfds.end(),
                [fd](const auto& pollfd_){ return pollfd_.fd == fd; });
            if (it != _pollfds.end()) {
                it->events |= events;
            } else {
                pollfd_x pollfd_{};
                pollfd_.fd = fd;
                pollfd_.events = events;
                pollfd_.revents = 0;
                _pollfds.push_back(pollfd_);
            }
        }

        inline void fd_unregister(int fd) override {
            _pollfds.erase(
                std::remove_if(
                    _pollfds.begin(),
                    _pollfds.end(),
                    [fd](const pollfd_x& pollfd_){ return pollfd_.fd == fd; }),
                _pollfds.end());
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

        // get the event handler entry from the pollfd
        inline Event_Handler_Entry* get_event_handler_entry_ptr(const pollfd_x & pollfd_) {
            if (pollfd_.revents == 0) return nullptr;
            if (!IS_VALID_SOCKET(pollfd_.fd)) return nullptr;
#if defined(_WIN32)
            if (pollfd_.fd == 0 && !_kbhit()) return nullptr;
#endif

            auto it = _event_handler_entrys.find(pollfd_.fd);
            if (it == _event_handler_entrys.end()) return nullptr;
            auto& event_handler_entry = it->second;
            if (!event_handler_entry._active) return nullptr;
            return &event_handler_entry;
        }

        // dispatch the events
        void dispatch_events(
            reactor_event_pack_t& rep_next,
            const std::vector<pollfd_x>& pollfds_copy)
        {
            for (const auto& pollfd_ : pollfds_copy) {
                // get the event handler entry from the pollfd
                auto event_handler_entry_ptr = get_event_handler_entry_ptr(pollfd_);
                if (!event_handler_entry_ptr) continue;

                // dispatch the read event
                if ((pollfd_.revents & POLL_X_IN) && event_handler_entry_ptr->_event_handler__read) {
                    auto rep = event_handler_entry_ptr->_event_handler__read->apply(pollfd_.fd);
                    for (auto& re : rep) {
                        if (re._register_type == Enum_Register_Types::Unregister) {
                            event_handler_entry_ptr->_active = false;
                        }
                        rep_next.push_back(std::move(re));
                    }
                }

                // dispatch the write event
                if ((pollfd_.revents & POLL_X_OUT) && event_handler_entry_ptr->_event_handler__write) {
                    auto rep = event_handler_entry_ptr->_event_handler__write->apply(pollfd_.fd);
                    for (auto& re : rep) {
                        if (re._register_type == Enum_Register_Types::Unregister) {
                            event_handler_entry_ptr->_active = false;
                        }
                        rep_next.push_back(std::move(re));
                    }
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
        std::vector<pollfd_x> _pollfds;
        timeout_x _msec{ -1 };
        std::atomic<bool> _running{ false };
    };

    using Event_Loop__Mid__ST = Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::ST>;
} // namespace BA_Socket

#endif // EVENT_LOOP__MID__ST_HPP
