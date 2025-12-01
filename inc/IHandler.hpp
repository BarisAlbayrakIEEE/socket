// IHandler.hpp

#ifndef IHANDLER_HPP
#define IHANDLER_HPP

#include "Reactor_Command.hpp"
#include "aux_type_traits.hpp"

#define READ_LEN 4096
#define HANDLER_RETURN_PACK__NONE()                            \
    reactor_command_pack_t rcp{};                              \
    rcp.emplace_back();                                        \
    return rcp
#define HANDLER_RETURN_PACK__UNREGISTER(fd__)                  \
    reactor_command_pack_t rcp{};                              \
    rcp.emplace_back(                                          \
        (fd__),                                                \
        Enum_Register_Types::Unregister,                       \
        Enum_Event_Types::Read_Write,                          \
        Enum_Handler_Command_Types::Remove,                    \
        nullptr);                                              \
    return rcp

namespace BA_Socket {
    // Handler interface
    struct IHandler {
        virtual ~IHandler() = default;
        virtual inline reactor_command_pack_t apply(int fd) const {
            HANDLER_RETURN_PACK__NONE();
        };
    };

    struct Handler_Entry {
        handler_ptr_t _handler__read{ nullptr };
        handler_ptr_t _handler__write{ nullptr };
        bool _active{ true };

        Handler_Entry(
            handler_ptr_t handler__read = nullptr,
            handler_ptr_t handler__write = nullptr,
            bool active = true)
            :
            _handler__read(std::move(handler__read)),
            _handler__write(std::move(handler__write)),
            _active(active) {}                
    };

    struct Job {
        int _fd{-1};
        IHandler* _handler{};
        Job(int fd, IHandler *handler) : _fd(fd), _handler(handler) {};
    };
    using job_result_t = Reactor_Command;
    using job_result_pack_t = std::vector<job_result_t>;

    // forward declerations for the handlers
    struct Handler_Write;
    struct Handler_Redirect;
    template <typename F>
        requires CString_Forward<F>
    struct Handler_Read_Forward;
    struct Handler_Read_Redirect;
    template <typename F, typename Next_Handler_Type>
        requires
            CString_Transform<F> &&
            (
                std::is_same_v<Next_Handler_Type, Handler_Write> ||
                std::is_same_v<Next_Handler_Type, Handler_Redirect>)
    struct Handler_Read_Transform;
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Write;
    template <typename F>
        requires CString_Transform<F>
    struct Handler_Read_Transform_Redirect;
    template <typename Next_Handler_Type>
        requires std::is_base_of_v<IHandler, Next_Handler_Type>
    struct Handler_Accept;
} // namespace BA_Socket

#endif // IHANDLER_HPP
