// Event_Loop__Factory.hpp

#ifndef EVENT_LOOP__FACTORY_HPP
#define EVENT_LOOP__FACTORY_HPP

#include "Event_Loop__Low__ST.hpp"
#include "Event_Loop__Low__HP.hpp"
#include "Event_Loop__Mid__ST.hpp"
#include "Event_Loop__Mid__HP.hpp"

using namespace BA_Concurrency;

namespace BA_Socket {
    template <typename Event_Loop_Type>
    struct Event_Loop__Factory {
        static Event_Loop_Type create() {
            return Event_Loop_Type();
        };
    };

    template <>
    struct Event_Loop__Factory<Event_Loop__Low__ST_t> {
        static Event_Loop__Low__ST_t create() {
            return Event_Loop__Low__ST_t();
        };
    };

    template <>
    struct Event_Loop__Factory<Event_Loop__Mid__ST_t> {
        static Event_Loop__Mid__ST_t create() {
            return Event_Loop__Mid__ST_t();
        };
    };

    template <
        template <typename> typename Concurrent_Queue_Type,
        typename Thread_Pool_Type>
            requires CEL<
                Concurrent_Queue_Type<Job>,
                Concurrent_Queue_Type<job_result_t>,
                Thread_Pool_Type>
    struct Event_Loop__Factory<Event_Loop__Low__HP_t<Concurrent_Queue_Type, Thread_Pool_Type>> {
        static Event_Loop__Low__HP_t<Concurrent_Queue_Type, Thread_Pool_Type> create() {
            return Event_Loop__Low__HP_t<Concurrent_Queue_Type, Thread_Pool_Type>(
                0,
                100000,
                std::thread::hardware_concurrency());
        };
    };

    template <
        template <typename> typename Concurrent_Queue_Type,
        typename Thread_Pool_Type>
            requires CEL<
                Concurrent_Queue_Type<Job>,
                Concurrent_Queue_Type<job_result_t>,
                Thread_Pool_Type>
    struct Event_Loop__Factory<Event_Loop__Mid__HP_t<Concurrent_Queue_Type, Thread_Pool_Type>> {
        static Event_Loop__Mid__HP_t<Concurrent_Queue_Type, Thread_Pool_Type> create() {
            return Event_Loop__Mid__HP_t<Concurrent_Queue_Type, Thread_Pool_Type>(
                100,
                std::thread::hardware_concurrency());
        };
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__FACTORY_HPP
