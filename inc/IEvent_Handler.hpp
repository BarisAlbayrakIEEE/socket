// IEvent_Handler.hpp

#ifndef IEVENT_HANDLER_HPP
#define IEVENT_HANDLER_HPP

#include "Reactor_Event.hpp"
#include "aux_type_traits.hpp"

#define READ_LEN 4096
#define HANDLER_RETURN_PACK__NONE()                            \
    reactor_event_pack_t rep{};                                \
    rep.emplace_back();                                        \
    return rep
#define HANDLER_RETURN_PACK__UNREGISTER(fd__)                  \
    reactor_event_pack_t rep{};                                \
    rep.emplace_back(                                          \
        (fd__),                                                \
        Enum_Register_Types::Unregister,                       \
        Enum_IO_Event_Types::Read_Write,                        \
        Enum_Handler_Command_Types::Remove,                    \
        nullptr);                                              \
    return rep

namespace BA_Socket {
    // Handler interface
    struct IEvent_Handler {
        virtual ~IEvent_Handler() = default;
        virtual inline reactor_event_pack_t apply(int fd) const {
            HANDLER_RETURN_PACK__NONE();
        };
    };

    struct Event_Handler_Entry {
        event_handler_ptr_t _event_handler__read{ nullptr };
        event_handler_ptr_t _event_handler__write{ nullptr };
        bool _active{ true };

        Event_Handler_Entry(
            event_handler_ptr_t event_handler__read = nullptr,
            event_handler_ptr_t event_handler__write = nullptr,
            bool active = true)
            :
            _event_handler__read(std::move(event_handler__read)),
            _event_handler__write(std::move(event_handler__write)),
            _active(active) {}                
    };

    struct Job {
        int _fd{-1};
        IEvent_Handler* _event_handler{};
        Job(int fd, IEvent_Handler *event_handler) : _fd(fd), _event_handler(event_handler) {};
    };
    using job_result_t = Reactor_Event;
    using job_result_pack_t = std::vector<job_result_t>;

    template <template <typename> typename Concurrent_Queue_Type>
    void execute_job(
        const Job& job,
        Concurrent_Queue_Type<job_result_t>& job_results)
    {
        auto rep = std::move(job._event_handler->apply(job._fd));
        for (auto& rc : rep) {
            job_results.push(std::move(rc));
        }
    };

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
        requires std::is_base_of_v<IEvent_Handler, Next_Handler_Type>
    struct Handler_Accept;
} // namespace BA_Socket

#endif // IEVENT_HANDLER_HPP
