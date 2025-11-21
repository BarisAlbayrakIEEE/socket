// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>

namespace BA_Socket {
    struct Reactor_Command {
        Enum_Reactor_Command_Types type = Enum_Reactor_Command_Types::None;
        int fd = -1;
        Enum_Event_Types event_type = Enum_Event_Types::Read;
        std::string write_data;

        inline static Reactor_Command Register(int fd, Enum_Event_Types event_type)
        {
            return { Enum_Reactor_Command_Types::Register, fd, event_type, {} };
        }
        inline static Reactor_Command Unregister(int fd)
        {
            return { Enum_Reactor_Command_Types::Unregister, fd, Enum_Event_Types::Read, {} };
        }
        inline static Reactor_Command Close(int fd)
        {
            return { Enum_Reactor_Command_Types::Close, fd, Enum_Event_Types::Read, {} };
        }
        inline static Reactor_Command Write(int fd, std::string data)
        {
            return { Enum_Reactor_Command_Types::WriteData, fd, Enum_Event_Types::Write, std::move(data) };
        }
    };

    class IHandler {
    public:
        virtual ~IHandler() = default;

        // Called when fd is readable
        virtual std::vector<Reactor_Command> on_read(int fd) {
            return {};
        }

        // Called when fd is writable
        virtual std::vector<Reactor_Command> on_write(int fd) {
            return {};
        }

        // Called when fd should be closed or error occurs
        virtual std::vector<Reactor_Command> on_error(int fd) {
            return { Reactor_Command::Close(fd) };
        }
    };

    class Event_Loop__Select : public IEvent_Loop {
        using Callback = std::function<void(const Socket&)>;
        using sockmap_t = std::unordered_map<SOCKET, Socket>;
    public:
        Event_Loop__Select() {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        void add_handler(
            int fd,
            std::unique_ptr<IHandler> handler,
            Enum_Event_Types event_type)
        {
            if (fd < 0) return;
            _handlers[fd] = std::move(handler);
            update_fd_set(fd, event_type);
            _fd_max = std::max(_fd_max, fd);
        }

        void remove_handler(int fd) {
            if (fd < 0) return;

            _handlers.erase(fd);
            FD_CLR(fd, &_fds_read);
            FD_CLR(fd, &_fds_write);
            if (fd == _fd_max) recompute_max_fd();
        }

        void run() {
            _running.store(true);
            while (_running.load()) {
                if (_handlers.empty()) break;

                fd_set fds_read = _fds_read;
                fd_set fds_write = _fds_write;
                if (::select(_fd_max + 1, &fds_read, &fds_write, nullptr, nullptr) < 0) {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }

                // Snapshot handlers because they may change after commands
                auto handlers = _handlers;
                for (auto& [fd, handler] : handlers) {
                    if (!handler) continue;

                    std::vector<Reactor_Command> cmds;
                    if (FD_ISSET(fd, &fds_read)) {
                        cmds = handler->on_read(fd);
                    }
                    if (FD_ISSET(fd, &fds_write)) {
                        auto wcmds = handler->on_write(fd);
                        cmds.insert(cmds.end(), wcmds.begin(), wcmds.end());
                    }
                    apply_commands(cmds);
                }
            }
            stop();
        }

        void stop() { _running.store(false); }

    private:

        void update_fd_set(int fd, Enum_Event_Types event_type) {
            if (event_type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fds_read);
                FD_CLR(fd, &_fds_write);
            } else {
                FD_SET(fd, &_fds_write);
                FD_CLR(fd, &_fds_read);
            }
        }

        void apply_commands(const std::vector<Reactor_Command>& commands) {
            for (auto& cmd : commands) {
                switch (cmd.type) {
                case Enum_Reactor_Command_Types::Register:
                    update_fd_set(cmd.fd, cmd.event_type);
                    _fd_max = std::max(_fd_max, cmd.fd);
                    break;
                case Enum_Reactor_Command_Types::Unregister:
                    remove_handler(cmd.fd);
                    break;
                case Enum_Reactor_Command_Types::Close:
                    ::close(cmd.fd);
                    remove_handler(cmd.fd);
                    break;
                case Enum_Reactor_Command_Types::WriteData:
                    ::send(cmd.fd, cmd.write_data.data(), cmd.write_data.size(), 0);
                    break;
                case Enum_Reactor_Command_Types::None:
                default:
                    break;
                }
            }
        }

        void recompute_max_fd() {
            _fd_max = -1;
            for (const auto& kv : _handlers)
                _fd_max = std::max(_fd_max, kv.first);
        }

    private:

        std::unordered_map<int, std::unique_ptr<IHandler>> _handlers;
        fd_set _fds_read{};
        fd_set _fds_write{};
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
