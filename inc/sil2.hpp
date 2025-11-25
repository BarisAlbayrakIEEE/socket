
    /*
    template <typename F> struct Handler_Read_Forward
    struct Handler_Read_Redirect
    template <typename F, typename Next_Handler_Type> struct Handler_Read_Transform
    template <typename F> struct Handler_Read_Transform
    template <typename F> struct Handler_Read_Transform_Write
    template <typename F> struct Handler_Read_Transform_Redirect
        explicit Handler_Accept(F const* forwarder) : _forwarder(forwarder) {};
        explicit Handler_Read_Redirect(const std::vector<int>& fds) : _fds(fds) {};
        explicit Handler_Read_Redirect(std::vector<int>&& fds) : _fds(std::move(fds)) {};
        explicit Handler_Read_Transform(F const* transformer) : _transformer(transformer) {};
        Handler_Read_Transform(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};
        explicit Handler_Read_Transform_Write(F const* transformer) : _transformer(transformer) {};
        Handler_Read_Transform_Redirect(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform_Redirect(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};
    */

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Forward<F>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Read_Forward<F>.
    template <typename F>
        requires CString_Forward<F>
    struct Handler_Accept<Handler_Read_Forward<F>> : public IHandler {
        F const* _forwarder{};

        explicit Handler_Accept(F const* forwarder) : _forwarder(forwarder) {};

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
                    std::make_unique<Handler_Read_Forward>(_forwarder)));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Redirect.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Read_Redirect.
    struct Handler_Accept<Handler_Read_Redirect> : public IHandler {
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
                    std::make_unique<Handler_Read_Redirect>()));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by Handler_Read_Transform<F, Handler_Write>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Read_Transform<F, Handler_Write>.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform<F, Handler_Write>> : public IHandler {
        F const* _transformer{};

        explicit Handler_Accept(F const* transformer) : _transformer(transformer) {};
        Handler_Read_Transform(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Read_Transform(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

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
                    std::make_unique<Handler_Read_Transform<F, Handler_Write>>(_transformer)));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by Handler_Read_Transform<F, Handler_Redirect>.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Read_Transform<F, Handler_Redirect>.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform<F, Handler_Redirect>> : public IHandler {
        F const* _transformer{};
        std::vector<int> _fds;

        Handler_Accept(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Accept(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

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
                    std::make_unique<Handler_Read_Transform<F, Handler_Redirect>>(_transformer, _fds)));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Transform_Write.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Read_Transform_Write.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform_Write<F>> : public IHandler {
        F const* _transformer{};

        explicit Handler_Accept(F const* transformer) : _transformer(transformer) {};

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
                    std::make_unique<Handler_Read_Transform_Write<F>>(_transformer)));
        };
    };

    // Accept handler
    //
    // Specialization for: Followed by a Handler_Read_Transform_Redirect.
    //
    // fd_set actions:
    //   Registers the client fd to the read fd_set.
    //
    // Handler action:
    //   Adds a new Handler_Read_Transform_Redirect.
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Accept<Handler_Read_Transform_Redirect<F>> : public IHandler {
        F const* _transformer{};
        std::vector<int> _fds;

        Handler_Accept(F const* transformer, const std::vector<int>& fds)
            : _transformer(transformer), _fds(fds) {};
        Handler_Accept(F const* transformer, std::vector<int>&& fds)
            : _transformer(transformer), _fds(std::move(fds)) {};

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
                    std::make_unique<Handler_Read_Transform_Redirect<F>>(_transformer, _fds)));
        };
    };
