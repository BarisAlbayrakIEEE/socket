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
    enum class Enum_Event_Types { None, Read, Write, Read_Write }; // Read_Write is for unregistering from both read and write
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

    template <typename F>
    concept CString_Forward = 
        requires (F f, const std::string& s) { { f(s) } -> std::same_as<bool>; };

    template <typename F>
    concept CString_Transform = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<bool>; };

    // Handler interface
    struct IHandler {
        virtual ~IHandler() = default;
        virtual inline handler_return_t on_read(int) const {
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
        virtual inline handler_return_t on_write(int) const {
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Write handler
    //
    // fd_set actions:
    //   Unregisters the fd from the write fd_set.
    //   New read will register the fd again.
    //
    // Handler action:
    //   Removes the handler from the handler map.
    struct Handler_Write : public IHandler {
        std::string _buffer{};

        explicit Handler_Write(const std::string& buffer) : _buffer(buffer) {};
        explicit Handler_Write(std::string&& buffer) : _buffer(std::move(buffer)) {};

        handler_return_t on_write(int fd) const override {
            // send the data to the peer
            PRINTF1("[Server]: Sending the data to the peer...\n");
            ::send(fd, _buffer.c_str(), _buffer.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", _buffer.c_str(), _buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Accept handler
    //
    // Base template: Followed by a one of read handlers.
    // Will be specialized for the case that is followed by a write handler.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new handler of type Next_Handler_Type.
    //   Next handler type shall be one of read handlers (e.g. read-forward).
    template <typename Next_Handler_Type>
        requires std::is_base_of_v<IHandler, Next_Handler_Type>
    struct Handler_Accept : public IHandler {
        handler_return_t on_read(int fd) const override {
            // accept a new connection
            PRINTF1("[Server]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
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

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd_client,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Read) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Next_Handler_Type>()));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a write handler.
    // Will be specialized for the case that is followed by a write handler.
    //
    // fd_set actions:
    //   Registers the client fd to the write fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Write.
    template <>
    struct Handler_Accept<Handler_Write> : public IHandler {
        std::string _buffer{};

        explicit Handler_Accept(const std::string& buffer) : _buffer(buffer) {};
        explicit Handler_Accept(std::string&& buffer) : _buffer(std::move(buffer)) {};

        handler_return_t on_read(int fd) const override {
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

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        fd_client,
                        Enum_Register_Types::Register,
                        Enum_Event_Types::Write) },
                handler_action_t(
                    Enum_Handler_Action_Types::Add,
                    std::make_unique<Handler_Write>(_buffer)));
        };
    };

    // Redirect handler
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    struct Handler_Redirect : public IHandler {
        std::string _buffer{};
        std::vector<int> _fds;

        Handler_Redirect(const std::string& buffer, const std::vector<int>& fds)
            : _buffer(buffer), _fds(fds) {};
        Handler_Redirect(std::string&& buffer, std::vector<int>&& fds)
            : _buffer(std::move(buffer)), _fds(std::move(fds)) {};

        inline handler_return_t on_write(int fd) const override {
            // send the data to the peer
            PRINTF1("[Server]: Sending the data to the peer...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, _buffer.c_str(), _buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Forward handler:
    //   Recieves the data from the peer
    //   and sends it to a function (for processing the peer data internally).
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    template <typename F>
        requires CString_Forward<F>
    struct Handler_Read_Forward : public IHandler {
        handler_return_t n_read(int fd) const override {
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
            std::string buffer{ read, static_cast<size_t>(bytes_received) };
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

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Redirect handler:
    //   Recieves the data from the peer
    //   and sends it to the sockets with the contained file descriptors.
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    struct Handler_Read_Redirect : public IHandler {
        std::vector<int> _fds;

        explicit Handler_Read_Redirect(const std::vector<int>& fds) : _fds(fds) {};
        explicit Handler_Read_Redirect(std::vector<int>&& fds) : _fds(std::move(fds)) {};

        handler_return_t on_read(int fd) const override {
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
            std::string buffer{ read, static_cast<size_t>(bytes_received) };
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next action (write or redirect).
    //
    // Base template: Followed by a write handler.
    // Will be specialized for the case that is followed by a redirect handler.
    //
    // fd_set actions:
    //   Registers the fd to the write fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Write.
    template <typename F, typename Next_Handler_Type>
        requires
            CString_Transform<F> &&
            (
                std::is_same_v<Next_Handler_Type, Handler_Write> ||
                std::is_same_v<Next_Handler_Type, Handler_Redirect>)
    struct Handler_Read_Transform : public IHandler {
        handler_return_t on_read(int fd) const override {
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
            std::string buffer{ read, static_cast<size_t>(bytes_received) };
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

            // return the fd_set actions and the handler action
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

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next action (write or redirect).
    //
    // fd_set actions:
    //   Registers the fd to the write fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Write.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform<F, Handler_Redirect> : public IHandler {
        std::vector<int> _fds;

        explicit Handler_Read_Transform(const std::vector<int>& fds) : _fds(fds) {};
        explicit Handler_Read_Transform(std::vector<int>&& fds) : _fds(std::move(fds)) {};

        handler_return_t on_read(int fd) const override {
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
            std::string buffer{ read, static_cast<size_t>(bytes_received) };
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

            // return the fd_set actions and the handler action
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

    // Read-Transform-Write handler:
    //   Recieves the data from the peer,
    //   transforms it
    //   and writes back to the peer.
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Write : public IHandler {
        handler_return_t on_read(int fd) const override {
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
            std::string buffer{ read, static_cast<size_t>(bytes_received) };
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
            ::send(fd, buffer.c_str(), buffer.size(), 0);
            PRINTF4("[Server]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), read);

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Read-Transform-Redirect handler:
    //   Recieves the data from the peer,
    //   transforms it
    //   and redirects to the sockets defined as a member.
    //
    // Specialization for: Followed by a redirect handler.
    //
    // fd_set actions:
    //   None
    //
    // Handler action:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Redirect : public IHandler {
        std::vector<int> _fds;

        explicit Handler_Read_Transform_Redirect(const std::vector<int>& fds) : _fds(fds) {};
        explicit Handler_Read_Transform_Redirect(std::vector<int>&& fds) : _fds(std::move(fds)) {};

        handler_return_t on_read(int fd) const override {
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
            std::string buffer{ read, static_cast<size_t>(bytes_received) };
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

            // return the fd_set actions and the handler action
            return handler_return_t(
                fd_set_actions_t{
                    fd_set_Action(
                        -1,
                        Enum_Register_Types::None,
                        Enum_Event_Types::None) },
                handler_action_t(Enum_Handler_Action_Types::None, nullptr));
        };
    };

    // Event loop interface
    class IEvent_Loop {
    public:
        virtual ~IEvent_Loop() = default;

        virtual void fd_register(int, Enum_Event_Types) = 0;
        virtual void fd_unregister(int, Enum_Event_Types) = 0;
        virtual void close_sockets() = 0;

        virtual void run() = 0; // blocking
        virtual void stop() = 0;
    };
} // namespace BA_Socket

#endif // IEVENT_LOOP_HPP
