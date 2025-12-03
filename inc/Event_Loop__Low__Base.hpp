// Event_Loop__Low__Base.hpp :
//   Platform                 : Cross-platform
//   Performance              : Low (select-based configuration)

#ifndef EVENT_LOOP__LOW__BASE_HPP
#define EVENT_LOOP__LOW__BASE_HPP

#include "Event_Loop__Base.hpp"

namespace BA_Socket {
    class Event_Loop__Low__Base : public Event_Loop__Base {
    public:

        static inline void fd_register(
            fd_set *fd_set__read,
            fd_set *fd_set__write,
            int *fd_max,
            int fd,
            Enum_IO_Event_Types IO_event_type)
        {
            if (IO_event_type == Enum_IO_Event_Types::None)
            {
                throw std::runtime_error("IO event type cannot be None.");
            }

            if (IO_event_type == Enum_IO_Event_Types::Read) {
                FD_SET(fd, fd_set__read);
            }
            else if (IO_event_type == Enum_IO_Event_Types::Write) {
                FD_SET(fd, fd_set__write);
            }
            else { // if (IO_event_type == Enum_IO_Event_Types::Read_Write) {
                FD_SET(fd, fd_set__read);
                FD_SET(fd, fd_set__write);
            }
            if (fd > *fd_max) *fd_max = fd;
        }

        static inline void fd_unregister(
            std::unordered_map<int, Event_Handler_Entry>& event_handler_entrys,
            fd_set *fd_set__read,
            fd_set *fd_set__write,
            int *fd_max,
            int fd,
            Enum_IO_Event_Types IO_event_type,
            bool is_ST) // is a call from single-threaded configuration
        {
            if (IO_event_type == Enum_IO_Event_Types::None)
            {
                throw std::runtime_error("IO event type cannot be None.");
            }

            bool check_clear{};
            if (
                IO_event_type == Enum_IO_Event_Types::Read ||
                IO_event_type == Enum_IO_Event_Types::Read_Write)
            {
                check_clear = true;
                FD_CLR(fd, fd_set__read);
            }
            if (
                IO_event_type == Enum_IO_Event_Types::Write ||
                IO_event_type == Enum_IO_Event_Types::Read_Write)
            {
                check_clear = true;
                FD_CLR(fd, fd_set__write);
            }
            if (
                check_clear &&
                (FD_ISSET(fd, fd_set__read) || FD_ISSET(fd, fd_set__write)))
            {
                remove_event_handler(event_handler_entrys, fd, IO_event_type);
                if (!is_ST) {
                    auto& event_handler_entry = event_handler_entrys[fd];
                    event_handler_entry._active = true;
                }
                return;
            }

            event_handler_entrys.erase(fd);
            if (fd == *fd_max) {
                auto fd_max__new = *fd_max;
                *fd_max = -1;
                for(SOCKET fdi = 0; fdi <= fd_max__new; ++fdi) {
                    if (FD_ISSET(fdi, fd_set__read)) {
                        if (fdi > *fd_max) *fd_max = fdi;
                    }
                    if (FD_ISSET(fdi, fd_set__write)) {
                        if (fdi > *fd_max) *fd_max = fdi;
                    }
                }
            }
        }

        // inspect and get event handler from the event handler entry
        static inline IEvent_Handler* get_event_handler_ptr(
            int fd,
            fd_set fd_set__read,
            fd_set fd_set__write,
            Event_Handler_Entry& event_handler_entry,
            Enum_IO_Event_Types IO_event_type,
            bool is_ST) // is a call from single-threaded configuration
        {
            if (!IS_VALID_SOCKET(fd)) return nullptr;
            if (!is_ST) {
                if (!event_handler_entry._active) return nullptr;
            }
            IEvent_Handler *event_handler_ptr{ nullptr };
            if (IO_event_type == Enum_IO_Event_Types::Read) {
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
            else if (IO_event_type == Enum_IO_Event_Types::Write) {
                if (!FD_ISSET(fd, &fd_set__write)) return nullptr;
                event_handler_ptr = event_handler_entry._event_handler__write.get();
            }
            return event_handler_ptr;
        }
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__LOW__BASE_HPP
