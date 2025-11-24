// IEvent_Loop.hpp

#ifndef IEVENT_LOOP_HPP
#define IEVENT_LOOP_HPP

#include <functional>
#include <vector>
#include <memory>
#include <utility>
#include <concepts>
#include "Socket.hpp"

namespace BA_Socket {
    enum class Enum_Register_Types { None, Register, Unregister };
    enum class Enum_Event_Types { None, Read, Write };
    enum class Enum_Handler_Action_Types { None, Add, Remove, Replace };

    const std::string INFO_WRONG_DATA = "Wrong data for the request";

    struct fd_set_Action{
        int _fd{-1};
        Enum_Register_Types _register_type{ Enum_Register_Types::None };
        Enum_Event_Types _event_type{ Enum_Event_Types::None };
    };

    struct IHandler;
    using fd_set_actions_t = std::vector<fd_set_Action>;
    using handler_action_t = std::pair<Enum_Handler_Action_Types, std::unique_ptr<IHandler>>;
    using handler_return_t = std::pair<fd_set_actions_t, handler_action_t>;

    struct IHandler {
        virtual handler_return_t apply(int) const = 0;
    };

    // accept handler
    // registers the client fd to the read fd_set
    template <typename New_Handler_Type>
        requires std::is_base_of_v<IHandler, New_Handler_Type>
    struct Handler_Accept : IHandler {
        handler_return_t apply(int fd) const override {
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
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd_client,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Read) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<New_Handler_Type>()));
        };
    };

    // write handler
    // unregisters the fd from the write fd_set.
    // new read will register the fd again.
    struct Handler_Write {
        std::string _buffer{};

        handler_return_t apply(int fd) const {
            // send the data to the peer
            PRINTF1("[Server]: Sending the data to the peer...\n");
            ::send(fd, _buffer.c_str(), _buffer.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), read);

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd,
                        Enum_Register_Types::Unregister,
                        Enum_Event_Types::Write) },
                handler_action_t(Enum_Handler_Action_Types::Remove, nullptr));
        };
    };

    // redirect handler
    // unregisters the fd from the write fd_set.
    // new read will register the fd again.
    struct Handler_Redirect {
        std::string _buffer{};
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // send the data to the peer
            PRINTF1("[Server]: Sending the data to the peer...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, _buffer.c_str(), _buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), read);

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd,
                        Enum_Register_Types::Unregister,
                        Enum_Event_Types::Write) },
                handler_action_t(Enum_Handler_Action_Types::Remove, nullptr));
        };
    };

    // transform handler: transforms the buffer recieved before to a function
    // registers the fd to the write fd_set.
    // returns a write handler.
    template <typename F>
    concept CString_Transform = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<bool>; };

    template <typename F>
        requires CString_Transform<F>
    struct Handler_Transform {
        std::string _buffer{};

        handler_return_t apply(int fd) const {
            // transform the data by function F
            PRINTF1("[Server]: Transforming the data by function F...\n");
            if(!F(_buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(
                    Enum_Handler_Action_Types::Replace,
                    std::make_unique<Handler_Write>(std::move(_buffer))));
        };
    };

    // read-and-forward handler
    // does not registers fds to the fd_sets.
    // does not return new handlers.
    template <typename F>
    concept CString_Forward = 
        requires (F f, const std::string& s) { { f(s) } -> std::same_as<bool>; };

    template <typename F>
        requires CString_Forward<F>
    struct Handler_Read_Forward {
        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // forward the recieved data to function F
            PRINTF1("[Server]: Forwarding the recieved data to function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed forwarding (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed forwarding (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // read-and-redirect handler: recieves the data from the peer
    // and sends to the sockets with the contained file descriptors.
    // does not registers fds to the fd_sets.
    // does not return new handlers.
    struct Handler_Read_Redirect {
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // redirect the data to the contained fds
            PRINTF1("[Server]: Redirecting the data to the ...\n");
            std::string buffer{ read };
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), read);

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // read-and-transform handler
    // base template when followed by a write handler.
    // will be specialized when followed by a redirection.
    // registers the fd to the write fd_set.
    // returns a write handler.
    template <typename F, typename Next_Handler_Type>
        requires CString_Transform<F>
    struct Handler_Read_Transform {
        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Write) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Handler_Write>(std::move(buffer))));
        };
    };

    // read-and-transform handler
    // specialization when followed by a redirection.
    // registers the fd to the write fd_set.
    // returns a redirect handler.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform<F, Handler_Redirect> {
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Write) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Handler_Redirect>(std::move(buffer), _fds)));
        };
    };

    // read-and-transform-redirect handler: recieves the data from the peer
    // transforms the recieved data
    // and sends to the sockets with the contained file descriptors.
    // does not registers fds to the fd_sets.
    // does not return new handlers.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Redirect {
        std::vector<int> _fds;

        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // redirect the data to the contained fds
            PRINTF1("[Server]: Redirecting the data to the ...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), read);

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // read-and-transform-and-write handler
    // does not registers fds to the fd_sets.
    // does not return new handlers.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Write {
        handler_return_t apply(int fd) const {
            // receive data from the peer
            char read[1024];
            int bytes_received = ::recv(fd, read, 1024, 0);
            PRINTF1("[Server]: Receiving data from peer...\n");
            if (bytes_received < 1) {
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }
            PRINTF4("[Server]: Received (%d bytes): %.*s", bytes_received, bytes_received, read);

            // transform the recieved data by function F
            PRINTF1("[Server]: Transforming the recieved data by function F...\n");
            std::string buffer{ read };
            if(!F(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Server]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                return handler_return_t(
                    fd_set_actions_t{
                        fd_set_Action(
                            -1,
                            Enum_Register_Types::None,
                            Enum_Event_Types::None) },
                    handler_action_t(Enum_Handler_Action_Types::None, nullptr));
            }

            // send the transformed data back to the peer
            PRINTF1("[Server]: Sending the transformed data back to the peer...\n");
            ::send(fd, buffer.c_str(), bytes_received, 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", bytes_received, bytes_received, read);

            // return the fd set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
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
