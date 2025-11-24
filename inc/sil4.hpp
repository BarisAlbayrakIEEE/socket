    enum class Enum_Register_Types { None, Register, Unregister };
    enum class Enum_Event_Types { None, Read, Write };
    enum class Enum_Reactor_Command_Types {
        Register,
        Register_Read,
        Register_Read_Forward,
        Register_Read_Transform,
        Register_Read_Transform_Write,
        Register_Read_Write, // echo
        Register_Write,
        Read,
        Read_Forward,
        Read_Transform,
        Read_Transform_Write,
        Read_Write, // echo
        Forward,
        Transform,
        Transform_Write,
        Write,
        Unregister
    };

    class Abstract_Reactor_Command;

    class IHandler {
    public:
        virtual std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply(Abstract_Reactor_Command const*, int) const = 0;
    };

    struct Abstract_Reactor_Command {
        // move only
        Abstract_Reactor_Command(Abstract_Reactor_Command&& rhs) = default;
        Abstract_Reactor_Command& operator=(Abstract_Reactor_Command&& rhs) = default;
        virtual ~Abstract_Reactor_Command() = default;

        virtual std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const = 0;
        virtual Enum_Register_Types get_register_type() const = 0;
        virtual Enum_Event_Types get_event_type() const = 0;
        virtual Enum_Reactor_Command_Types get_reactor_command_type() const = 0;
        virtual int get_fd() const = 0;
    };

    struct Reactor_Command__Register : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::Register;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Register;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Register_Read : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::Register;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Register_Read;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Register_Read_Forward : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::Register;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Register_Read_Forward;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Register_Read_Transform : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::Register;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Register_Read_Transform;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Register_Read_Transform_Write : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::Register;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Register_Read_Transform_Write;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Register_Read_Write : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::Register;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Register_Read_Write;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Read : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::None;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Read;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Read_Forward : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::None;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Read_Forward;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Read_Transform : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::None;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Read_Transform;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Read_Transform_Write : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::None;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Read_Transform_Write;
        };
        inline int get_fd() const { return fd; };
    };

    struct Reactor_Command__Read_Write : public Abstract_Reactor_Command {
        std::unique_ptr<IHandler> _handler{};
        int fd{};

        inline std::vector<std::unique_ptr<Abstract_Reactor_Command>> apply() const {
            return _handler->apply(this, fd);
        };
        inline Enum_Register_Types get_register_type() const {
            return Enum_Register_Types::None;
        };
        inline Enum_Event_Types get_event_type() const {
            return Enum_Event_Types::Read;
        };
        inline Enum_Reactor_Command_Types get_reactor_command_type() const {
            return Enum_Reactor_Command_Types::Read_Write;
        };
        inline int get_fd() const { return fd; };
    };
