// Handler.hpp

#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <functional>
#include <utility>
#include "Socket.hpp"
#include "aux_type_traits.hpp"
#include "IHandler.hpp"

namespace BA_Socket {
    // read helper function
    inline bool read_helper(int fd, std::string& buffer) {
        buffer.resize(READ_LEN);
        if (fd == 0) {
            // receive data from stdin
            PRINTF1("[Handler]: Reading user input...\n");
            if (!std::fgets(buffer.data(), READ_LEN, stdin)) {
                PRINTF1("[Handler]: EOF for stdin.\n");
                return false;
            }
            buffer.resize(std::strlen(buffer.data()));
            PRINTF4("[Handler]: Read (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.data());
        } else {
            // receive data from the peer
            PRINTF1("[Handler]: Receiving data from peer...\n");
            int bytes_received = ::recv(fd, buffer.data(), READ_LEN, 0);
            if (bytes_received == 0) {
                PRINTF1("[Handler]: Peer closed the connection.\n");
                CLOSE_SOCKET(fd);
                return false;
            }
            else if (bytes_received < 0) {
                if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) return false; // Ctrl+C
                CLOSE_SOCKET(fd);
                SOCKET_ERROR__RECV();
                return false;
            }
            buffer.resize(bytes_received);
            PRINTF4("[Handler]: Received (%d bytes): %.*s", bytes_received, bytes_received, buffer.data());
        }
        return true;
    }

    // Write handler
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    struct Handler_Write : public IHandler {
        int _fd{-1};
        std::string _buffer{};

        Handler_Write(int fd, const std::string& buffer)
            : _fd(fd), _buffer(buffer) {};
        Handler_Write(int fd, std::string&& buffer)
            : _fd(fd), _buffer(std::move(buffer)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            ::send(_fd, _buffer.c_str(), _buffer.size(), 0);
            PRINTF4("[Handler]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), _buffer.c_str());

            // return the handler pack
            HANDLER_RETURN_PACK__NONE();
        };
    };
    
    // Redirect handler
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    struct Handler_Redirect : public IHandler {
        int _fd{-1};
        std::string _buffer{};
        std::vector<int> _fds{};

        Handler_Redirect(int fd, const std::string& buffer, const std::vector<int>& fds)
            : _fd(fd), _buffer(buffer), _fds(fds) {};
        Handler_Redirect(int fd, std::string&& buffer, std::vector<int>&& fds)
            : _fd(fd), _buffer(std::move(buffer)), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        inline reactor_command_pack_t apply() const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, _buffer.c_str(), _buffer.size(), 0);
            }
            PRINTF4("[Handler]: Sent (%d bytes): %.*s", _buffer.size(), _buffer.size(), _buffer.c_str());

            // return the handler pack
            HANDLER_RETURN_PACK__NONE();
        };
    };

    // Read-Forward handler:
    //   Recieves the data from the peer
    //   and sends it to a function (for processing the peer data internally).
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    template <typename F>
        requires CString_Forward<F>
    struct Handler_Read_Forward : public IHandler {
        int _fd{-1};
        F const* _forwarder{};

        Handler_Read_Forward(int fd, F const* forwarder)
            : _fd(fd), _forwarder(forwarder) {};

        inline int get_fd() const override { return _fd; };
        
        reactor_command_pack_t apply() const override {
            std::string buffer;
            if (!read_helper(_fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(_fd);
            }

            // forward the recieved data to function F
            PRINTF1("[Handler]: Forwarding the recieved data to function F...\n");
            if (!(*_forwarder)(buffer)) {
                // send the info for the failed forwarding (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed forwarding (wrong input data) to the peer...\n");
                ::send(_fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // return the handler pack
            HANDLER_RETURN_PACK__NONE();
        };
    };

    // Read-Redirect handler:
    //   Recieves the data from the peer
    //   and sends it to the sockets with the contained file descriptors.
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    struct Handler_Read_Redirect : public IHandler {
        int _fd{-1};
        std::vector<int> _fds{};

        Handler_Read_Redirect(int fd, const std::vector<int>& fds)
            : _fd(fd), _fds(fds) {};
        Handler_Read_Redirect(int fd, std::vector<int>&& fds)
            : _fd(fd), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            std::string buffer;
            if (!read_helper(_fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(_fd);
            }

            // redirect the data to the contained fds
            PRINTF1("[Handler]: Redirecting the data to the ...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Handler]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.c_str());

            // return the handler pack
            HANDLER_RETURN_PACK__NONE();
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next command (write or redirect).
    //
    // Base template: Followed by a Handler_Write.
    // Will be specialized for the case that is followed by a Handler_Redirect.
    //
    // fd_set commands:
    //   Registers the fd to the write fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Write.
    template <typename F, typename Next_Handler_Type>
        requires
            CString_Transform<F> &&
            (
                std::is_same_v<Next_Handler_Type, Handler_Write> ||
                std::is_same_v<Next_Handler_Type, Handler_Redirect>)
    struct Handler_Read_Transform : public IHandler {
        int _fd{-1};
        F const* _transformer{};

        Handler_Read_Transform(int fd, F const* transformer)
            : _fd(fd), _transformer(transformer) {};

        inline int get_fd() const override { return _fd; };
        
        reactor_command_pack_t apply() const override {
            std::string buffer;
            if (!read_helper(_fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(_fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(_fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                _fd,
                Enum_Register_Types::Register,
                Enum_Event_Types::Write,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Write>(_fd, std::move(buffer)));
            return rcp;
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next command (write or redirect).
    //
    // Specialization for: Followed by a Handler_Redirect.
    //
    // fd_set commands:
    //   Registers the fd to the write fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Redirect.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform<F, Handler_Redirect> : public IHandler {
        int _fd{-1};
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Read_Transform(int fd, F const* transformer, const std::vector<int>& fds)
            : _fd(fd), _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform(int fd, F const* transformer, std::vector<int>&& fds)
            : _fd(fd), _transformer(transformer), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            std::string buffer;
            if (!read_helper(_fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(_fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(_fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                _fd,
                Enum_Register_Types::Register,
                Enum_Event_Types::Write,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Redirect>(_fd, std::move(buffer), _fds));
            return rcp;
        };
    };

    // Read-Transform-Write handler:
    //   Recieves the data from the peer,
    //   transforms it and writes back to the peer.
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Write : public IHandler {
        int _fd{-1};
        F const* _transformer{};

        Handler_Read_Transform_Write(int fd, F const* transformer)
            : _fd(fd), _transformer(transformer) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            std::string buffer;
            if (!read_helper(_fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(_fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(_fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // send the transformed data back to the peer
            PRINTF1("[Handler]: Sending the transformed data back to the peer...\n");
            ::send(_fd, buffer.c_str(), buffer.size(), 0);
            PRINTF4("[Handler]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.c_str());

            // return the handler pack
            HANDLER_RETURN_PACK__NONE();
        };
    };

    // Read-Transform-Redirect handler:
    //   Recieves the data from the peer,
    //   transforms it and redirects to the sockets defined as a member.
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Redirect : public IHandler {
        int _fd{-1};
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Read_Transform_Redirect(int fd, F const* transformer, const std::vector<int>& fds)
            : _fd(fd), _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform_Redirect(int fd, F const* transformer, std::vector<int>&& fds)
            : _fd(fd), _transformer(transformer), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            std::string buffer;
            if (!read_helper(_fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(_fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(_fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // redirect the data to the contained fds
            PRINTF1("[Handler]: Redirecting the data to the ...\n");
            for (const auto& fd_: _fds) {
                ::send(fd_, buffer.c_str(), buffer.size(), 0);
            }
            PRINTF4("[Handler]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.c_str());

            // return the handler pack
            HANDLER_RETURN_PACK__NONE();
        };
    };

    // Accept handler
    //
    // Base template: Followed by a Handler_Write.
    // Will be specialized for the read handlers.
    //
    // fd_set commands:
    //   Registers the client fd to the write fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Write.
    template <typename Next_Handler_Type>
        requires std::is_base_of_v<IHandler, Next_Handler_Type>
    struct Handler_Accept : public IHandler {
        int _fd{-1};
        std::string _buffer{};

        Handler_Accept(int fd, const std::string& buffer)
            : _fd(fd), _buffer(buffer) {};
        Handler_Accept(int fd, std::string&& buffer)
            : _fd(fd), _buffer(std::move(buffer)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Write,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Write>(fd_client, _buffer));
            return rcp;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Forward<F>.
    //
    // fd_set commands:
    //   Registers the client fd to the read fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Read_Forward<F>.
    template <typename F>
        requires CString_Forward<F>
    struct Handler_Accept<Handler_Read_Forward<F>> : public IHandler {
        int _fd{-1};
        F const* _forwarder{};

        Handler_Accept(int fd, F const* forwarder)
            : _fd(fd), _forwarder(forwarder) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Read,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Read_Forward<F>>(fd_client, _forwarder));
            return rcp;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Redirect.
    //
    // fd_set commands:
    //   Registers the client fd to the read fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Read_Redirect.
    template <>
    struct Handler_Accept<Handler_Read_Redirect> : public IHandler {
        int _fd{-1};
        std::vector<int> _fds{};

        Handler_Accept(int fd, const std::vector<int>& fds)
            : _fd(fd), _fds(fds) {};
        Handler_Accept(int fd, std::vector<int>&& fds)
            : _fd(fd), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Read,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Read_Redirect>(fd_client, _fds));
            return rcp;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by Handler_Read_Transform<F, Handler_Write>.
    //
    // fd_set commands:
    //   Registers the client fd to the read fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Read_Transform<F, Handler_Write>.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform<F, Handler_Write>> : public IHandler {
        int _fd{-1};
        F const* _transformer{};

        Handler_Accept(int fd, F const* transformer)
            : _fd(fd), _transformer(transformer) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Read,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Read_Transform<F, Handler_Write>>(fd_client, _transformer));
            return rcp;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by Handler_Read_Transform<F, Handler_Redirect>.
    //
    // fd_set commands:
    //   Registers the client fd to the read fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Read_Transform<F, Handler_Redirect>.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform<F, Handler_Redirect>> : public IHandler {
        int _fd{-1};
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Accept(int fd, F const* transformer, const std::vector<int>& fds)
            : _fd(fd), _transformer(transformer), _fds(fds) {};
        Handler_Accept(int fd, F const* transformer, std::vector<int>&& fds)
            : _fd(fd), _transformer(transformer), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Read,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Read_Transform<F, Handler_Redirect>>(fd_client, _transformer, _fds));
            return rcp;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Transform_Write<F>.
    //
    // fd_set commands:
    //   Registers the client fd to the read fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Read_Transform_Write<F>.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform_Write<F>> : public IHandler {
        int _fd{-1};
        F const* _transformer{};

        Handler_Accept(int fd, F const* transformer)
            : _fd(fd), _transformer(transformer) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Read,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Read_Transform_Write<F>>(fd_client, _transformer));
            return rcp;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Transform_Redirect<F>.
    //
    // fd_set commands:
    //   Registers the client fd to the read fd_set.
    //
    // Handler command:
    //   Adds a new Handler_Read_Transform_Redirect<F>.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform_Redirect<F>> : public IHandler {
        int _fd{-1};
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Accept(int fd, F const* transformer, const std::vector<int>& fds)
            : _fd(fd), _transformer(transformer), _fds(fds) {};
        Handler_Accept(int fd, F const* transformer, std::vector<int>&& fds)
            : _fd(fd), _transformer(transformer), _fds(std::move(fds)) {};

        inline int get_fd() const override { return _fd; };

        reactor_command_pack_t apply() const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                _fd,
                (struct sockaddr*) &client_addr,
                &client_len);
            if (!IS_VALID_SOCKET(fd_client)) {
                if (GET_SOCKET_ERRNO() != ERROR_INTERRUPTED) SOCKET_ERROR__ACCEPT();
                HANDLER_RETURN_PACK__NONE();
            }
            print_sockaddr<is_debug_mode>(client_addr, client_len);

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_Event_Types::Read,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Read_Transform_Redirect<F>>(fd_client, _transformer, _fds));
            return rcp;
        };
    };
} // namespace BA_Socket

#endif // HANDLER_HPP
