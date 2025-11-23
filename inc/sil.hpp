// Event_Loop__Select.hpp

#ifndef EVENT_LOOP__SELECT_HPP
#define EVENT_LOOP__SELECT_HPP

#include "IEvent_Loop.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace BA_Socket {
    struct Reactor_Command {
        Enum_Reactor_Command_Types type = Enum_Reactor_Command_Types::None;
        int fd = -1;
        Enum_Event_Types interest = Enum_Event_Types::Read;
        std::string write_data;

        static Reactor_Command Register(int fd, Enum_Event_Types interest)
        {
            return {Enum_Reactor_Command_Types::Register, fd, interest, {}};
        }
        static Reactor_Command Unregister(int fd)
        {
            return {Enum_Reactor_Command_Types::Unregister, fd, Enum_Event_Types::Read, {}};
        }
        static Reactor_Command Close(int fd)
        {
            return {Enum_Reactor_Command_Types::Close, fd, Enum_Event_Types::Read, {}};
        }
        static Reactor_Command Write(int fd, std::string data)
        {
            return {Enum_Reactor_Command_Types::WriteData, fd, Enum_Event_Types::Write, std::move(data)};
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
        Event_Loop__Select(
            Callback on_read,
            Callback on_write,
            Callback on_disconnect)
            :
            _on_read(on_read),
            _on_write(on_write),
            _on_disconnect(on_disconnect)
        {
            FD_ZERO(&_fds_read);
            FD_ZERO(&_fds_write);
        }

        inline void fd_register(const Socket& s, Enum_Event_Types type) override {
            std::scoped_lock lk(_m);

            int fd = s.native_handle();
            if (type == Enum_Event_Types::Read) {
                _sockmap_read[fd] = s;
                FD_SET(fd, &_fds_read);
            } else {
                _sockmap_write[fd] = s;
                FD_SET(fd, &_fds_write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(const Socket& s) override {
            std::scoped_lock lk(_m);

            int fd = s.native_handle();
            _sockmap_read.erase(fd);
            _sockmap_write.erase(fd);
            FD_CLR(fd, &_fds_read);
            FD_CLR(fd, &_fds_write);
            if (fd == _fd_max) {
                _fd_max = 0;
                for (const auto& [fd_, _] : _sockmap_read)
                    if (fd_ > _fd_max) _fd_max = fd_;
                for (const auto& [fd_, _] : _sockmap_write)
                    if (fd_ > _fd_max) _fd_max = fd_;
            }
        }

        inline void run() override {
            _running.store(true);
            while (_running.load()) {
                if (_fd_max < 0) break;

                fd_set rcopy = _fds_read;
                fd_set wcopy = _fds_write;
                if (::select(
                    _fd_max + 1,
                    _sockmap_read.empty()  ? nullptr : &rcopy,
                    _sockmap_write.empty()  ? nullptr : &wcopy,
                    nullptr,
                    nullptr) < 0)
                {
                    if (GET_SOCKET_ERRNO() == EINTR) continue;
                    SOCKET_ERROR__SELECT();
                }

                for (auto& [fd, sock] : _sockmap_read) if (FD_ISSET(fd, &rcopy)) _on_read(sock);
                for (auto& [fd, sock] : _sockmap_write) if (FD_ISSET(fd, &wcopy)) _on_write(sock);
            }
            stop();
        }

        inline void stop() override { _running.store(false); }

    private:

        sockmap_t _sockmap_read;
        sockmap_t _sockmap_write;
        Callback _on_read, _on_write, _on_disconnect;
        fd_set _fds_read;
        fd_set _fds_write;
        int _fd_max = -1;
        std::mutex _m;
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT_HPP
