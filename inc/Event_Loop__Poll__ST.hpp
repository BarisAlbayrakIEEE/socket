// Event_Loop__Poll.hpp

#ifndef EVENT_LOOP__POLL__ST_HPP
#define EVENT_LOOP__POLL__ST_HPP

#include <unordered_map>
#include <vector>
#include <atomic>
#include "poll_setup.hpp"
#include "IEvent_Loop.hpp"
#include "Handler.hpp"

namespace BA_Socket {

    class Event_Loop__Poll__ST : public IEvent_Loop {
    public:
        Event_Loop__Poll__ST(int msec = -1) : _msec(msec) {}

        inline void fd_register(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::None) return;

            short events = 0;
            if (event_type == Enum_Event_Types::Read) {
                events |= POLL_IN;
            } else if (event_type == Enum_Event_Types::Write) {
                events |= POLL_OUT;
            } else { // Read_Write
                events |= POLL_IN | POLL_OUT;
            }

            // update or add pollfd
            auto it = std::find_if(
                _pollfds.begin(),
                _pollfds.end(),
                [fd](const auto& pollfd_){ return pollfd_.fd == fd; });
            if (it != _pollfds.end()) {
                it->events |= events;
            } else {
                pollfd_t pollfd_{};
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
                    [fd](const pollfd_t& pollfd_){ return pollfd_.fd == fd; }),
                _pollfds.end());
            _handler_entrys.erase(fd);
        }

        inline void add_handler(int fd, handler_ptr_t&& handler, Enum_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_Event_Types::None) return;

            auto& handler_entry = _handler_entrys[fd];
            if (event_type == Enum_Event_Types::Read) {
                handler_entry._handler__read = std::move(handler);
            } else if (event_type == Enum_Event_Types::Write) {
                handler_entry._handler__write = std::move(handler);
            }
        }

        inline void run() override {
            _running.store(true);
            while (_running.load()) {
                if (_pollfds.empty()) break;

                // perform poll operation
                std::vector<pollfd_t> pollfds_copy = _pollfds;
                int status = poll_execute(
                    pollfds_copy.data(),
                    static_cast<nfds_t>(pollfds_copy.size()),
                    _msec);
                if (status < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__POLL();
                    break;
                }
                if (status == 0) continue; // timeout

                // execute the handlers in the handler entry map
                reactor_command_pack_t rcp_next;
                execute_handlers(rcp_next, pollfds_copy);

                // apply the next reactor commands returned from the current handlers in the map
                apply_reactor_commands(rcp_next);
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:

        // get the handler entry from the pollfd
        inline Handler_Entry* get_handler_entry_ptr(const pollfd_t & pollfd_) {
            if (pollfd_.revents == 0) return nullptr;
            if (!IS_VALID_SOCKET(pollfd_.fd)) return nullptr;
#if defined(_WIN32)
            if (pollfd_.fd == 0 && !_kbhit()) return nullptr;
#endif

            auto it = _handler_entrys.find(pollfd_.fd);
            if (it == _handler_entrys.end()) return nullptr;
            auto& handler_entry = it->second;
            if (!handler_entry._active) return nullptr;
            return &handler_entry;
        }

        // execute the handlers in the handler entry map
        void execute_handlers(
            reactor_command_pack_t& rcp_next,
            const std::vector<pollfd_t>& pollfds_copy)
        {
            for (const auto& pollfd_ : pollfds_copy) {
                // get the handler entry from the pollfd
                auto handler_entry_ptr = get_handler_entry_ptr(pollfd_);
                if (!handler_entry_ptr) continue;

                // execute the read handler
                if ((pollfd_.revents & POLL_IN) && handler_entry_ptr->_handler__read) {
                    auto rcp = handler_entry_ptr->_handler__read->apply(pollfd_.fd);
                    for (auto& rc : rcp) {
                        if (rc._register_type == Enum_Register_Types::Unregister) {
                            handler_entry_ptr->_active = false;
                        }
                        rcp_next.push_back(std::move(rc));
                    }
                }

                // execute the write handler
                if ((pollfd_.revents & POLL_OUT) && handler_entry_ptr->_handler__write) {
                    auto rcp = handler_entry_ptr->_handler__write->apply(pollfd_.fd);
                    for (auto& rc : rcp) {
                        if (rc._register_type == Enum_Register_Types::Unregister) {
                            handler_entry_ptr->_active = false;
                        }
                        rcp_next.push_back(std::move(rc));
                    }
                }
            }
        }

        // apply the next reactor commands returned from the current handlers in the map
        void apply_reactor_commands(reactor_command_pack_t& rcp_next) {
            // update the fd_sets and the handler maps.
            for (auto& [fd, register_type, event_type, handler_command_type, handler__new] : rcp_next) {
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
                    add_handler(fd, std::move(handler__new), event_type);
                }
            }
        }

        std::unordered_map<int, Handler_Entry> _handler_entrys;
        std::vector<pollfd_t> _pollfds;
        int _msec{ -1 };
        std::atomic<bool> _running{ false };
    };

} // namespace BA_Socket

#endif // EVENT_LOOP__POLL__ST_HPP
