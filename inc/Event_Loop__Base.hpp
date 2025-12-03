// Event_Loop__Low__Base.hpp:
//   Platform                 : Cross-platform

#ifndef EVENT_LOOP__BASE_HPP
#define EVENT_LOOP__BASE_HPP

#include <unordered_map>
#include <atomic>
#include "IEvent_Loop.hpp"
#include "Event_Handler.hpp"

namespace BA_Socket {
    class Event_Loop__Base {
    public:

        static inline void add_event_handler(
            std::unordered_map<int, Event_Handler_Entry>& event_handler_entrys,
            int fd,
            event_handler_ptr_t&& event_handler,
            Enum_IO_Event_Types IO_event_type)
        {
            if (!event_handler) return;
            if (IO_event_type == Enum_IO_Event_Types::None) return;

            if (event_handler_entrys.contains(fd)) {
                auto& event_handler_entry = event_handler_entrys[fd];
                if (IO_event_type == Enum_IO_Event_Types::Read) {
                    event_handler_entry._event_handler__read = std::move(event_handler);
                } else if (IO_event_type == Enum_IO_Event_Types::Write) {
                    event_handler_entry._event_handler__write = std::move(event_handler);
                }
            } else {
                Event_Handler_Entry event_handler_entry{ nullptr, nullptr, true };
                if (IO_event_type == Enum_IO_Event_Types::Read) {
                    event_handler_entry._event_handler__read = std::move(event_handler);
                } else if (IO_event_type == Enum_IO_Event_Types::Write) {
                    event_handler_entry._event_handler__write = std::move(event_handler);
                }
                event_handler_entrys[fd] = std::move(event_handler_entry);
            }
        }

        static inline void remove_event_handler(
            std::unordered_map<int, Event_Handler_Entry>& event_handler_entrys,
            int fd,
            Enum_IO_Event_Types IO_event_type)
        {
            auto& event_handler_entry = event_handler_entrys[fd];
            if (IO_event_type == Enum_IO_Event_Types::Read) {
                event_handler_entry._event_handler__read.reset();
            } else if (IO_event_type == Enum_IO_Event_Types::Write) {
                event_handler_entry._event_handler__write.reset();
            }
        }
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__BASE_HPP
