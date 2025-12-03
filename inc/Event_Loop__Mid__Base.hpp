// Event_Loop__Mid__Base.hpp :
//   Platform                 : Cross-platform
//   Performance              : Mid (poll-based configuration)

#ifndef EVENT_LOOP__MID__BASE_HPP
#define EVENT_LOOP__MID__BASE_HPP

#include <vector>
#include "Event_Loop__Base.hpp"
#include "poll_setup.hpp"

namespace BA_Socket {

    class Event_Loop__Mid__Base : public Event_Loop__Base {
    public:

        static inline void fd_register(
            std::vector<pollfd_x>& pollfds,
            int fd,
            Enum_IO_Event_Types IO_event_type)
        {
            if (IO_event_type == Enum_IO_Event_Types::None) return;

            short events = 0;
            if (IO_event_type == Enum_IO_Event_Types::Read) {
                events |= POLL_X_IN;
            } else if (IO_event_type == Enum_IO_Event_Types::Write) {
                events |= POLL_X_OUT;
            } else { // Read_Write
                events |= POLL_X_IN | POLL_X_OUT;
            }

            // update or add pollfd
            auto it = std::find_if(
                pollfds.begin(),
                pollfds.end(),
                [fd](const auto& pollfd_){ return pollfd_.fd == fd; });
            if (it != pollfds.end()) {
                it->events |= events;
            } else {
                pollfd_x pollfd_{};
                pollfd_.fd = fd;
                pollfd_.events = events;
                pollfd_.revents = 0;
                pollfds.push_back(pollfd_);
            }
        }

        static inline void fd_unregister(
            std::unordered_map<int, Event_Handler_Entry>& event_handler_entrys,
            std::vector<pollfd_x>& pollfds,
            int fd,
            Enum_IO_Event_Types IO_event_type,
            bool is_ST) // is a call from single-threaded configuration
        {
            if (IO_event_type == Enum_IO_Event_Types::None)
            {
                throw std::runtime_error("IO event type cannot be None.");
            }

            auto it = std::find_if(
                pollfds.begin(),
                pollfds.end(),
                [fd](const pollfd_x& pollfd_){ return pollfd_.fd == fd; });
            if (IO_event_type == Enum_IO_Event_Types::Read_Write)
            {
                pollfds.erase(it);
                event_handler_entrys.erase(fd);
                return;
            }
            if (
                IO_event_type == Enum_IO_Event_Types::Read &&
                (it->revents & POLL_X_IN && !(it->revents & POLL_X_OUT)))
            {
                pollfds.erase(it);
                event_handler_entrys.erase(fd);
                return;
            }
            if (
                IO_event_type == Enum_IO_Event_Types::Write &&
                (it->revents & POLL_X_OUT && !(it->revents & POLL_X_IN)))
            {
                pollfds.erase(it);
                event_handler_entrys.erase(fd);
                return;
            }

            if (
                IO_event_type == Enum_IO_Event_Types::Read &&
                (it->revents & POLL_X_OUT && !(it->revents & POLL_X_IN)))
            {
                throw std::runtime_error("wrong IO event type tor fd_unregister function.");
            }
            if (
                IO_event_type == Enum_IO_Event_Types::Write &&
                (it->revents & POLL_X_IN && !(it->revents & POLL_X_OUT)))
            {
                throw std::runtime_error("wrong IO event type tor fd_unregister function.");
            }

            if (IO_event_type == Enum_IO_Event_Types::Read)
            {
                it->events = POLL_X_OUT;
                remove_event_handler(event_handler_entrys, fd, IO_event_type);
            } else {
                it->events = POLL_X_IN;
                remove_event_handler(event_handler_entrys, fd, IO_event_type);
            }

            if (!is_ST) {
                auto& event_handler_entry = event_handler_entrys[fd];
                event_handler_entry._active = true;
            }
        }

        // get the event handler entry from the pollfd
        static inline Event_Handler_Entry* get_event_handler_entry_ptr(
            std::unordered_map<int, Event_Handler_Entry>& event_handler_entrys,
            const pollfd_x & pollfd_,
            bool is_ST) // is a call from single-threaded configuration
        {
            if (pollfd_.revents == 0) return nullptr;
            if (!IS_VALID_SOCKET(pollfd_.fd)) return nullptr;
#if defined(_WIN32)
            if (pollfd_.fd == 0 && !_kbhit()) return nullptr;
#endif

            auto it = event_handler_entrys.find(pollfd_.fd);
            if (it == event_handler_entrys.end()) return nullptr;
            auto& event_handler_entry = it->second;
            if (!is_ST) {
                if (!event_handler_entry._active) return nullptr;
            }
            return &event_handler_entry;
        }
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__MID__BASE_HPP
