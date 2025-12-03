// Event_Handler.hpp

#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <functional>
#include <utility>
#include "utility_addr.hpp"
#include "IEvent_Handler.hpp"

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
    template <typename Event_Handler_Type>
    inline void write_helper(
        int fd,
        const std::string& buffer,
        reactor_event_pack_t& rep)
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
                rep.emplace_back(
                    fd,
                    Enum_Register_Types::Register,
                    Enum_IO_Event_Types::Write,
                    Enum_Event_Handler_Action_Types::Add,
                    std::make_unique<Event_Handler_Type>(std::move(remaining)));
                return;
            }

            // Hard disconnection
            if (bytes_sent == 0) {
                PRINTF1("[Handler]: Peer closed the connection.\n");
                CLOSE_SOCKET(fd);
                rep.emplace_back(
                    fd,
                    Enum_Register_Types::Unregister,
                    Enum_IO_Event_Types::Read_Write,
                    Enum_Event_Handler_Action_Types::Remove,
                    nullptr);
                return;
            }

            // get errno
            auto status = GET_SOCKET_ERRNO();

            // transient errors: retry later
            if (status == ERROR_INTERRUPTED || status == ERROR_BLOCKED) {
                rep.emplace_back(
                    fd,
                    Enum_Register_Types::Register,
                    Enum_IO_Event_Types::Write,
                    Enum_Event_Handler_Action_Types::Add,
                    std::make_unique<Event_Handler_Type>(buffer));
                return;
            }

            // deal with separate EWOULDBLOCK
#if !defined(_WIN32)
            if (status == EWOULDBLOCK) {
                rep.emplace_back(
                    fd,
                    Enum_Register_Types::Register,
                    Enum_IO_Event_Types::Write,
                    Enum_Event_Handler_Action_Types::Add,
                    std::make_unique<Event_Handler_Type>(buffer));
                return;
            }
#endif

            // unhandled send failure
            PRINTF1("[Handler]: Unhandled send failure.\n");
            CLOSE_SOCKET(fd);
            rep.emplace_back(
                fd,
                Enum_Register_Types::Unregister,
                Enum_IO_Event_Types::Read_Write,
                Enum_Event_Handler_Action_Types::Remove,
                nullptr);
        }
    }

    // Write handler - Once (only)
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    struct Event_Handler_Write__Once : public IEvent_Handler {
        std::string _buffer{};

        explicit Event_Handler_Write__Once(const std::string& buffer)
            : _buffer(buffer) {};
        explicit Event_Handler_Write__Once(std::string&& buffer)
            : _buffer(std::move(buffer)) {};

        reactor_event_pack_t apply(int fd) const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            reactor_event_pack_t rep{};
            write_helper<Event_Handler_Write__Once>(fd, _buffer, rep);

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Write);
            }
            return rep;
        };
    };

    // Write handler - Loop (infinite)
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    struct Event_Handler_Write__Loop : public IEvent_Handler {
        std::string _buffer{};

        explicit Event_Handler_Write__Loop(const std::string& buffer)
            : _buffer(buffer) {};
        explicit Event_Handler_Write__Loop(std::string&& buffer)
            : _buffer(std::move(buffer)) {};

        reactor_event_pack_t apply(int fd) const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            reactor_event_pack_t rep{};
            write_helper<Event_Handler_Write__Once>(fd, _buffer, rep);

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rep;
        };
    };
    
    // Redirect handler - Once (only)
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    struct Event_Handler_Redirect__Once : public IEvent_Handler {
        std::string _buffer{};
        std::vector<int> _fds{};

        Event_Handler_Redirect__Once(const std::string& buffer, const std::vector<int>& fds)
            : _buffer(buffer), _fds(fds) {};
        Event_Handler_Redirect__Once(std::string&& buffer, std::vector<int>&& fds)
            : _buffer(std::move(buffer)), _fds(std::move(fds)) {};

        inline reactor_event_pack_t apply(int fd) const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            reactor_event_pack_t rep{};
            for (int fd__redirect: _fds) {
                write_helper<Event_Handler_Write__Once>(fd__redirect, _buffer, rep);
            }

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Write);
            }
            return rep;
        };
    };
    
    // Redirect handler - Loop (infinite)
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    struct Event_Handler_Redirect__Loop : public IEvent_Handler {
        std::string _buffer{};
        std::vector<int> _fds{};

        Event_Handler_Redirect__Loop(const std::string& buffer, const std::vector<int>& fds)
            : _buffer(buffer), _fds(fds) {};
        Event_Handler_Redirect__Loop(std::string&& buffer, std::vector<int>&& fds)
            : _buffer(std::move(buffer)), _fds(std::move(fds)) {};

        inline reactor_event_pack_t apply(int fd) const override {
            // send the data to the peer
            PRINTF1("[Handler]: Sending the data to the peer...\n");
            reactor_event_pack_t rep{};
            for (int fd__redirect: _fds) {
                write_helper<Event_Handler_Write__Once>(fd__redirect, _buffer, rep);
            }

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rep;
        };
    };

    // Read-Forward handler:
    //   Recieves the data from the peer
    //   and sends it to a function (for processing the peer data internally).
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    template <typename F>
        requires CString_Forward<F>
    struct Event_Handler_Read_Forward : public IEvent_Handler {
        F const* _forwarder{};

        explicit Event_Handler_Read_Forward(F const* forwarder)
            : _forwarder(forwarder) {};
        
        reactor_event_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Read_Write);
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
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    struct Event_Handler_Read_Redirect : public IEvent_Handler {
        std::vector<int> _fds{};

        explicit Event_Handler_Read_Redirect(const std::vector<int>& fds)
            : _fds(fds) {};
        explicit Event_Handler_Read_Redirect(std::vector<int>&& fds)
            : _fds(std::move(fds)) {};

        reactor_event_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Read_Write);
            }

            // redirect the data to the contained fds
            PRINTF1("[Handler]: Redirecting the data to the ...\n");
            reactor_event_pack_t rep{};
            for (int fd__redirect: _fds) {
                write_helper<Event_Handler_Write__Once>(fd__redirect, buffer, rep);
            }

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rep;
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next command (write or redirect).
    //
    // Base template: Followed by a Event_Handler_Write__Once.
    // Will be specialized for the case that is followed by a Event_Handler_Redirect__Once.
    //
    // fd_set actions:
    //   Registers the fd to the write fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Write__Once.
    template <typename F, typename Next_Event_Handler_Type>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform : public IEvent_Handler {
        F const* _transformer{};

        explicit Event_Handler_Read_Transform(F const* transformer)
            : _transformer(transformer) {};

        reactor_event_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Read_Write);
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Write,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Write__Once>(std::move(buffer)));
            return rep;
        };
    };

    // Read-Transform handler:
    //   Recieves the data from the peer
    //   and transforms it for the next command (write or redirect).
    //
    // Specialization for: Followed by a Event_Handler_Redirect__Once.
    //
    // fd_set actions:
    //   Registers the fd to the write fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Redirect__Once.
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform<F, Event_Handler_Redirect__Once> : public IEvent_Handler {
        F const* _transformer{};
        std::vector<int> _fds{};

        Event_Handler_Read_Transform(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Event_Handler_Read_Transform(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_event_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Read_Write);
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Write,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Redirect__Once>(std::move(buffer), _fds));
            return rep;
        };
    };

    // Read-Transform-Write handler:
    //   Recieves the data from the peer,
    //   transforms it and writes back to the peer.
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform_Write : public IEvent_Handler {
        F const* _transformer{};

        explicit Event_Handler_Read_Transform_Write(F const* transformer)
            : _transformer(transformer) {};

        reactor_event_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Read_Write);
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
            reactor_event_pack_t rep{};
            write_helper<Event_Handler_Write__Once>(fd, buffer, rep);

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rep;
        };
    };

    // Read-Transform-Redirect handler:
    //   Recieves the data from the peer,
    //   transforms it and redirects to the sockets defined as a member.
    //
    // fd_set actions:
    //   None
    //
    // Event handler actions:
    //   None
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform_Redirect : public IEvent_Handler {
        F const* _transformer{};
        std::vector<int> _fds{};

        Event_Handler_Read_Transform_Redirect(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Event_Handler_Read_Transform_Redirect(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_event_pack_t apply(int fd) const override {
            std::string buffer;
            if (!read_helper(fd, buffer)) {
                HANDLER_RETURN_PACK__UNREGISTER(fd, Enum_IO_Event_Types::Read_Write);
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
            reactor_event_pack_t rep{};
            for (int fd__redirect: _fds) {
                write_helper<Event_Handler_Write__Once>(fd__redirect, buffer, rep);
            }

            // return the handler pack
            if (rep.empty()) {
                HANDLER_RETURN_PACK__NONE();
            }
            return rep;
        };
    };

    // Accept handler
    //
    // Base template: Followed by a Event_Handler_Write__Once.
    // Will be specialized for the read handlers.
    //
    // fd_set actions:
    //   Registers the client fd to the write fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Write__Once.
    template <typename Next_Event_Handler_Type>
        requires std::is_base_of_v<IEvent_Handler, Next_Event_Handler_Type>
    struct Event_Handler_Accept : public IEvent_Handler {
        std::string _buffer{};

        explicit Event_Handler_Accept(const std::string& buffer)
            : _buffer(buffer) {};
        explicit Event_Handler_Accept(std::string&& buffer)
            : _buffer(std::move(buffer)) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Write,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Write__Once>(_buffer));
            return rep;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Event_Handler_Read_Forward<F>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Read_Forward<F>.
    template <typename F>
        requires CString_Forward<F>
    struct Event_Handler_Accept<Event_Handler_Read_Forward<F>> : public IEvent_Handler {
        F const* _forwarder{};

        explicit Event_Handler_Accept(F const* forwarder)
            : _forwarder(forwarder) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Read,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Read_Forward<F>>(_forwarder));
            return rep;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Event_Handler_Read_Redirect.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Read_Redirect.
    template <>
    struct Event_Handler_Accept<Event_Handler_Read_Redirect> : public IEvent_Handler {
        std::vector<int> _fds{};

        explicit Event_Handler_Accept(const std::vector<int>& fds)
            : _fds(fds) {};
        explicit Event_Handler_Accept(std::vector<int>&& fds)
            : _fds(std::move(fds)) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Read,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Read_Redirect>(_fds));
            return rep;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by Event_Handler_Read_Transform<F, Event_Handler_Write__Once>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Read_Transform<F, Event_Handler_Write__Once>.
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Accept<Event_Handler_Read_Transform<F, Event_Handler_Write__Once>> : public IEvent_Handler {
        F const* _transformer{};

        explicit Event_Handler_Accept(F const* transformer)
            : _transformer(transformer) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Read,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Read_Transform<F, Event_Handler_Write__Once>>(_transformer));
            return rep;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by Event_Handler_Read_Transform<F, Event_Handler_Redirect__Once>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Read_Transform<F, Event_Handler_Redirect__Once>.
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Accept<Event_Handler_Read_Transform<F, Event_Handler_Redirect__Once>> : public IEvent_Handler {
        F const* _transformer{};
        std::vector<int> _fds{};

        Event_Handler_Accept(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Event_Handler_Accept(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Read,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Read_Transform<F, Event_Handler_Redirect__Once>>(_transformer, _fds));
            return rep;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Event_Handler_Read_Transform_Write<F>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Read_Transform_Write<F>.
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Accept<Event_Handler_Read_Transform_Write<F>> : public IEvent_Handler {
        F const* _transformer{};

        explicit Event_Handler_Accept(F const* transformer)
            : _transformer(transformer) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Read,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Read_Transform_Write<F>>(_transformer));
            return rep;
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Event_Handler_Read_Transform_Redirect<F>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Event handler actions:
    //   Adds a new Event_Handler_Read_Transform_Redirect<F>.
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Accept<Event_Handler_Read_Transform_Redirect<F>> : public IEvent_Handler {
        F const* _transformer{};
        std::vector<int> _fds{};

        Event_Handler_Accept(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Event_Handler_Accept(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

        reactor_event_pack_t apply(int fd) const override {
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
            reactor_event_pack_t rep{};
            rep.emplace_back(
                fd_client,
                Enum_Register_Types::Register,
                Enum_IO_Event_Types::Read,
                Enum_Event_Handler_Action_Types::Add,
                std::make_unique<Event_Handler_Read_Transform_Redirect<F>>(_transformer, _fds));
            return rep;
        };
    };
} // namespace BA_Socket

#endif // EVENT_HANDLER_HPP
