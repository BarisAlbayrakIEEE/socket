// IEvent_Handler.hpp

#ifndef IEVENT_HANDLER_HPP
#define IEVENT_HANDLER_HPP

#include <type_traits>
#include <concepts>
#include "IConcurrent_Queue.hpp"
#include "IThread_Pool.hpp"
#include "Reactor_Event.hpp"

using namespace BA_Concurrency;

#define READ_LEN 4096
#define HANDLER_RETURN_PACK__NONE()                            \
    reactor_event_pack_t rep{};                                \
    rep.emplace_back();                                        \
    return rep
#define HANDLER_RETURN_PACK__UNREGISTER(fd__, IO_event_type)   \
    reactor_event_pack_t rep{};                                \
    rep.emplace_back(                                          \
        (fd__),                                                \
        Enum_Register_Types::Unregister,                       \
        (IO_event_type),                                       \
        Enum_Event_Handler_Action_Types::Remove,               \
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
        Event_Handler_Entry *_event_handler_entry{};
        Enum_IO_Event_Types _IO_event_type{Enum_IO_Event_Types::Read};
        Job(
            int fd,
            Event_Handler_Entry *event_handler_entry,
            Enum_IO_Event_Types IO_event_type)
                :
                _fd(fd),
                _event_handler_entry(event_handler_entry),
                _IO_event_type(IO_event_type) {};
    };
    using job_result_t = Reactor_Event;
    using job_result_pack_t = std::vector<job_result_t>;

    template <
        typename Concurrent_Queue_Type__Job,
        typename Concurrent_Queue_Type__job_result,
        typename Thread_Pool_Type>
    concept CEL = (
        std::is_base_of_v<IConcurrent_Queue<Job>, Concurrent_Queue_Type__Job> &&
        std::is_base_of_v<IConcurrent_Queue<job_result_t>, Concurrent_Queue_Type__job_result> &&
        std::is_base_of_v<IThread_Pool, Thread_Pool_Type>);

    template <typename F>
    concept CString_Forward = 
        requires (F f, const std::string& s) { { f(s) } -> std::same_as<bool>; };

    template <typename F>
    concept CString_Transform = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<bool>; };
    
    using string_forward_t = bool(const std::string&);
    using string_transform_t = bool(std::string&);

    template <typename Concurrent_Queue_Type__job_result>
    void execute_job(
        const Job& job,
        Concurrent_Queue_Type__job_result& job_results)
    {
        reactor_event_pack_t rep;
        if (job._IO_event_type == Enum_IO_Event_Types::Read) {
            rep = std::move(job._event_handler_entry->_event_handler__read->apply(job._fd));
        } else {
            rep = std::move(job._event_handler_entry->_event_handler__write->apply(job._fd));
        }
        for (auto& re : rep) {
            if (re._register_type == Enum_Register_Types::Unregister) {
                job._event_handler_entry->_active = false;
            }
            job_results.push(std::move(re));
        }
    };

    // forward declerations for the handlers
    struct Event_Handler_Write;
    struct Event_Handler_Redirect;
    template <typename F>
        requires CString_Forward<F>
    struct Event_Handler_Read_Forward;
    struct Event_Handler_Read_Redirect;
    template <typename F, typename Next_Event_Handler_Type>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform;
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform_Write;
    template <typename F>
        requires CString_Transform<F>
    struct Event_Handler_Read_Transform_Redirect;
    template <typename Next_Event_Handler_Type>
        requires std::is_base_of_v<IEvent_Handler, Next_Event_Handler_Type>
    struct Event_Handler_Accept;
} // namespace BA_Socket

#endif // IEVENT_HANDLER_HPP
