// Handler.hpp

#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <functional>
#include <utility>
#include "utility_addr.hpp"
#include "IHandler.hpp"

namespace BA_Socket {
    // read helper function: convinience function to wrap fgets and ::recv
    inline bool read_helper(int fd, std::string& buffer) {
        buffer.resize(READ_LEN);
        if (fd == 0) {
            PRINTF1("[Handler]: Reading from the stdin...\n");
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

    // write helper function: convinience function to wrap ::send
    inline void write_helper(
        int fd,
        const std::string& buffer,
        reactor_command_pack_t& rcp)
    {
        if (fd == 1) {
            printf(buffer.c_str());
        } else if (fd == 2) {
            fprintf(stderr, buffer.c_str());
        } else {
            int bytes_sent = ::send(fd, buffer.c_str(), buffer.size(), 0);
            if (bytes_sent == static_cast<int>(buffer.size())) {
                PRINTF4("[Handler]: Sent (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.c_str());
                return;
            }

            // partial send: schedule write handler to continue later
            if (bytes_sent > 0) {
                std::string remaining = buffer.substr(bytes_sent);
                rcp.emplace_back(
                    fd,
                    Enum_Register_Types::Register,
                    Enum_Event_Types::Write,
                    Enum_Handler_Command_Types::Add,
                    std::make_unique<Handler_Write>(std::move(remaining)));
                return;
            }

            // Hard disconnection
            if (bytes_sent == 0) {
                PRINTF1("[Handler]: Peer closed the connection.\n");
                CLOSE_SOCKET(fd);
                rcp.emplace_back(
                    fd,
                    Enum_Register_Types::Unregister,
                    Enum_Event_Types::Read_Write,
                    Enum_Handler_Command_Types::Remove,
                    nullptr);
                return;
            }

            // get errno
            auto status = GET_SOCKET_ERRNO();

            // transient errors: retry later
            if (status == ERROR_INTERRUPTED || status == ERROR_BLOCKED) {
                rcp.emplace_back(
                    fd,
                    Enum_Register_Types::Register,
                    Enum_Event_Types::Write,
                    Enum_Handler_Command_Types::Add,
                    std::make_unique<Handler_Write>(buffer));
                return;
            }

            // deal with separate EWOULDBLOCK
#if !defined(_WIN32)
            if (status == EWOULDBLOCK) {
                rcp.emplace_back(
                    fd,
                    Enum_Register_Types::Register,
                    Enum_Event_Types::Write,
                    Enum_Handler_Command_Types::Add,
                    std::make_unique<Handler_Write>(buffer));
                return;
            }
#endif

            // unhandled send failure
            PRINTF1("[Handler]: Unhandled send failure.\n");
            CLOSE_SOCKET(fd);
            rcp.emplace_back(
                fd,
                Enum_Register_Types::Unregister,
                Enum_Event_Types::Read_Write,
                Enum_Handler_Command_Types::Remove,
                nullptr);
        }
    }

    // Write handler
    //
    // fd_set commands:
    //   None
    //
    // Handler command:
    //   None
    struct Handler_Write : public IHandler {
        std::string _buffer{};

        explicit Handler_Write(const std::string& buffer)
            : _buffer(buffer) {};
        explicit Handler_Write(std::string&& buffer)
            : _buffer(std::move(buffer)) {};

        reactor_command_pack_t apply(int fd) const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            reactor_command_pack_t rcp{};
            write_helper(fd, _buffer, rcp);

            // return the handler pack
            if (rcp.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rcp;
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
        std::string _buffer{};
        std::vector<int> _fds{};

        Handler_Redirect(const std::string& buffer, const std::vector<int>& fds)
            : _buffer(buffer), _fds(fds) {};
        Handler_Redirect(std::string&& buffer, std::vector<int>&& fds)
            : _buffer(std::move(buffer)), _fds(std::move(fds)) {};

        inline reactor_command_pack_t apply(int fd) const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            reactor_command_pack_t rcp{};
            for (int fd__redirect: _fds) {
                write_helper(fd__redirect, _buffer, rcp);
            }

            // return the handler pack
            if (rcp.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rcp;
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
        F const* _forwarder{};

        explicit Handler_Read_Forward(F const* forwarder)
            : _forwarder(forwarder) {};
        
        reactor_command_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd);
            }

            // forward the recieved data to function F
            PRINTF1("[Handler]: Forwarding the recieved data to function F...\n");
            if (!(*_forwarder)(buffer)) {
                // send the info for the failed forwarding (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed forwarding (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
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
        std::vector<int> _fds{};

        explicit Handler_Read_Redirect(const std::vector<int>& fds)
            : _fds(fds) {};
        explicit Handler_Read_Redirect(std::vector<int>&& fds)
            : _fds(std::move(fds)) {};

        reactor_command_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd);
            }

            // redirect the data to the contained fds
            PRINTF1("[Handler]: Redirecting the data to the ...\n");
            reactor_command_pack_t rcp{};
            for (int fd__redirect: _fds) {
                write_helper(fd__redirect, buffer, rcp);
            }

            // return the handler pack
            if (rcp.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rcp;
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
        F const* _transformer{};

        explicit Handler_Read_Transform(F const* transformer)
            : _transformer(transformer) {};
        
        reactor_command_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd,
                Enum_Register_Types::Register,
                Enum_Event_Types::Write,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Write>(std::move(buffer)));
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
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Read_Transform(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_command_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // return the handler pack
            reactor_command_pack_t rcp{};
            rcp.emplace_back(
                fd,
                Enum_Register_Types::Register,
                Enum_Event_Types::Write,
                Enum_Handler_Command_Types::Add,
                std::make_unique<Handler_Redirect>(std::move(buffer), _fds));
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
        F const* _transformer{};

        explicit Handler_Read_Transform_Write(F const* transformer)
            : _transformer(transformer) {};

        reactor_command_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // send the transformed data back to the peer
            PRINTF1("[Handler]: Sending the transformed data back to the peer...\n");
            reactor_command_pack_t rcp{};
            write_helper(fd, buffer, rcp);

            // return the handler pack
            if (rcp.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rcp;
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
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Read_Transform_Redirect(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform_Redirect(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_command_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd);
            }

            // transform the recieved data by function F
            PRINTF1("[Handler]: Transforming the recieved data by function F...\n");
            if (!(*_transformer)(buffer)) {
                // send the info for the failed transformation (wrong input data) to the peer
                PRINTF1("[Handler]: Sending the info for the failed transformation (wrong input data) to the peer...\n");
                ::send(fd, INFO_WRONG_DATA.c_str(), INFO_WRONG_DATA.size(), 0);
                HANDLER_RETURN_PACK__NONE();
            }

            // redirect the data to the contained fds
            PRINTF1("[Handler]: Redirecting the data to the ...\n");
            reactor_command_pack_t rcp{};
            for (int fd__redirect: _fds) {
                write_helper(fd__redirect, buffer, rcp);
            }

            // return the handler pack
            if (rcp.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rcp;
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
        std::string _buffer{};

        explicit Handler_Accept(const std::string& buffer)
            : _buffer(buffer) {};
        explicit Handler_Accept(std::string&& buffer)
            : _buffer(std::move(buffer)) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Write>(_buffer));
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
        F const* _forwarder{};

        explicit Handler_Accept(F const* forwarder)
            : _forwarder(forwarder) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Read_Forward<F>>(_forwarder));
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
        std::vector<int> _fds{};

        explicit Handler_Accept(const std::vector<int>& fds)
            : _fds(fds) {};
        explicit Handler_Accept(std::vector<int>&& fds)
            : _fds(std::move(fds)) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Read_Redirect>(_fds));
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
        F const* _transformer{};

        explicit Handler_Accept(F const* transformer)
            : _transformer(transformer) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Read_Transform<F, Handler_Write>>(_transformer));
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
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Accept(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Accept(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Read_Transform<F, Handler_Redirect>>(_transformer, _fds));
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
        F const* _transformer{};

        explicit Handler_Accept(F const* transformer)
            : _transformer(transformer) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Read_Transform_Write<F>>(_transformer));
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
        F const* _transformer{};
        std::vector<int> _fds{};

        Handler_Accept(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Accept(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_command_pack_t apply(int fd) const override {
            // accept a new connection
            PRINTF1("[Handler]: Accepting a new connection...\n");
            fflush(stdout);
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            SOCKET fd_client = ::accept(
                fd,
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
                std::make_unique<Handler_Read_Transform_Redirect<F>>(_transformer, _fds));
            return rcp;
        };
    };
} // namespace BA_Socket

#endif // HANDLER_HPP
