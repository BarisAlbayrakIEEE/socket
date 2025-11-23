// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <vector>
#include <concepts>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_fd_Action_Types { None, Register_Read, Register_Write, Register_Read_Write, Unregister };
    struct fd_Action {
        Enum_fd_Action_Types _type{Enum_fd_Action_Types::None};
        int _fd{-1};
    };

    class IHandler {
    public:
        virtual ~IHandler() = default;
        virtual std::vector<fd_Action> apply(int fd) const = 0;
    };

    class Handler_Accept {
    public:
        std::vector<fd_Action> operator()(int fd) const {
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
                if (GET_SOCKET_ERRNO() == EINTR)
                    return { { Enum_fd_Action_Types::None, -1 } }; // Ctrl+C pressed
                SOCKET_ERROR__ACCEPT();
                return { { Enum_fd_Action_Types::None, -1 } };
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // add the new client socket to fd_set
            return { { Enum_fd_Action_Types::Register_Read, fd_client } };
        };
    };

    class Handler_Read {
    public:
        std::vector<fd_Action> operator()(int fd) const {
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from client...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return { { Enum_fd_Action_Types::Unregister, fd } };
            }

            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);
            return { { Enum_fd_Action_Types::None, -1 } };
        };
    };

    class Handler_Write {
        std::string buf;
    public:
        std::vector<fd_Action> operator()(int fd) const {
            PRINTF1("[Server]: Sending data to client...\n");
            ::send(fd, buf.c_str(), buf.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buf.size(), buf.size(), read);
            return { { Enum_fd_Action_Types::None, -1 } };
        };
    };
    
    template <typename F>
    concept CString_Func = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<void>; };
    
    template <typename F>
        requires CString_Func<F>
    class Handler_Read_Write {
    public:
        std::vector<fd_Action> apply(int fd) const {
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from client...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return { { Enum_fd_Action_Types::Unregister, fd } };
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            PRINTF1("[Server]: Updating data before sending to client...\n");
            std::string str{ read };
            F(str);

            PRINTF1("[Server]: Sending updated data to client...\n");
            ::send(fd, str.c_str(), bytes_received, 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);
            return { { Enum_fd_Action_Types::None, -1 } };
        };
    };

    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int) = 0;
        virtual void fd_unregister(int) = 0;
        virtual void close_sockets() = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
