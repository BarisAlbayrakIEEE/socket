// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <vector>
#include <concepts>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_Event_Types { None, Read, Write };
    enum class Enum_Reactor_Command_Types { None, Register, Unregister, Close };

    struct Reactor_Command {
        Enum_Event_Types interest{ Enum_Event_Types::Read };
        Enum_Reactor_Command_Types type{ Enum_Reactor_Command_Types::None };
        int fd{-1};

        static Reactor_Command None()
        {
            return { Enum_Event_Types::None, Enum_Reactor_Command_Types::None, -1 };
        }
        static Reactor_Command Register(int fd, Enum_Event_Types interest)
        {
            return { interest, Enum_Reactor_Command_Types::Register, fd };
        }
        static Reactor_Command Unregister(int fd)
        {
            return { Enum_Event_Types::Read, Enum_Reactor_Command_Types::Unregister, fd };
        }
    };

    class IHandler {
    public:
        virtual ~IHandler() = default;

        virtual std::vector<Reactor_Command> on_read(int fd) const {
            return {};
        }
        virtual std::vector<Reactor_Command> on_write(int fd) const {
            return {};
        }
    };

    class Handler_Accept : IHandler {
    public:
        std::vector<Reactor_Command> on_read(int fd) const override {
            // accept a new connection
            PRINTF1("[Server]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd)) {
                if (GET_SOCKET_ERRNO() != EINTR) SOCKET_ERROR__ACCEPT();
                return { Reactor_Command::None() };
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // add the new client socket to fd_set
            return { Reactor_Command::Register(fd_client, Enum_Event_Types::Read) };
        };
    };

    template <typename F>
    concept CString_Forward = 
        requires (F f, const std::string& s) { { f(s) } -> std::same_as<void>; };

    template <typename F>
        requires CString_Forward<F>
    class Handler_Read_Forward {
    public:
        std::vector<fd_Action> apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return { Reactor_Command::Unregister(fd) };
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // forward the recieved data to function F
            PRINTF1("[Server]: Forwarding the recieved data to function F...\n");
            std::string str{ read };
            F(str);
        };
    };

    template <typename F>
    concept CString_Transform = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<void>; };

    template <typename F>
        requires CString_Transform<F>
    class Handler_Read_Transform_Write {
    public:
        std::vector<fd_Action> apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return { Reactor_Command::Unregister(fd) };
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string str{ read };
            F(str);

            // send the transformed data back to the peer
            PRINTF1("[Server]: Sending the transformed data back to the peer...\n");
            ::send(fd, str.c_str(), bytes_received, 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);
            return { Reactor_Command::None() };
        };
    };

    template <typename F>
        requires CString_Forward<F>
    class Handler_Write_Read_Forward : IHandler {
        std::string buf;
    public:
        std::vector<Reactor_Command> on_write(int fd) const override {
            // send data to the peer
            PRINTF1("[Server]: Sending data to the peer...\n");
            ::send(fd, buf.c_str(), buf.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buf.size(), buf.size(), read);
            return { Reactor_Command::None() };

            // receive the response data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving the response data from the peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return { Reactor_Command::Unregister(fd) };
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // forward the recieved data to function F
            PRINTF1("[Server]: Updating data before sending to peer...\n");
            std::string str{ read };
            F(str);
        };
    };

    class Handler_Write_Read : IHandler {
        std::string buf;
    public:
        std::vector<Reactor_Command> on_write(int fd) const override {
            // send data to the peer
            PRINTF1("[Server]: Sending data to peer...\n");
            ::send(fd, buf.c_str(), buf.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buf.size(), buf.size(), read);
            return { Reactor_Command::None() };
        };
    };

    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int, Enum_Event_Types) = 0;
        virtual void fd_unregister(int) = 0;
        virtual void close_sockets() = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
