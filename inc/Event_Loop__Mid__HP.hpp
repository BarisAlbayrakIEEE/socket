// Event_Loop__Mid__HP.hpp :
//   Platform                 : Cross-platform
//   Performance              : Mid (poll-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Parallel

#ifndef EVENT_LOOP__MID__HP_HPP
#define EVENT_LOOP__MID__HP_HPP

#include <thread>
#include <optional>
#include "Event_Loop__Mid__Base.hpp"

namespace BA_Socket {
    template <
        typename Concurrent_Queue_Type__Job,
        typename Concurrent_Queue_Type__job_result,
        typename Thread_Pool_Type>
            requires CEL<
                Concurrent_Queue_Type__Job,
                Concurrent_Queue_Type__job_result,
                Thread_Pool_Type>
    class Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::HP,
        Concurrent_Queue_Type__Job,
        Concurrent_Queue_Type__job_result,
        Thread_Pool_Type>
            : public IEvent_Loop, public Event_Loop__Mid__Base
    {
    public:
        Event_Loop(
            timeout_x msec = -1,
            size_t thread_count = std::thread::hardware_concurrency())
                : _msec(msec), _tp(thread_count == 0 ? 1 : thread_count) {}

        inline void fd_register(int fd, Enum_IO_Event_Types IO_event_type) override {
            Event_Loop__Mid__Base::fd_register(
                _pollfds,
                fd,
                IO_event_type);
        }

        inline void fd_unregister(int fd, Enum_IO_Event_Types IO_event_type) override {
            Event_Loop__Mid__Base::fd_unregister(
                _event_handler_entrys,
                _pollfds,
                fd,
                IO_event_type,
                false);
        }

        inline void add_event_handler(
            int fd,
            event_handler_ptr_t&& event_handler,
            Enum_IO_Event_Types IO_event_type) override
        {
            Event_Loop__Base::add_event_handler(
                _event_handler_entrys,
                fd,
                std::move(event_handler),
                IO_event_type);
        }

        inline void remove_event_handler(
            int fd,
            Enum_IO_Event_Types IO_event_type) override
        {
            Event_Loop__Base::remove_event_handler(
                _event_handler_entrys,
                fd,
                IO_event_type);
        }

        inline void run() override {
            _running.store(true);

            // main loop
            while (_running.load()) {
                if (_pollfds.empty()) break;

                // perform poll operation
                std::vector<pollfd_x> pollfds_copy = _pollfds;
                int status = poll_x(
                    pollfds_copy.data(),
                    static_cast<nfds_x>(pollfds_copy.size()),
                    _msec);
                if (status < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__MID();
                    break;
                }
                if (status == 0) continue; // timeout

                // create jobs from the current event handler entry map
                create_jobs(pollfds_copy);

                // submit the jobs via the thread pool - multiple threads
                submit_jobs();

                // apply the reactor events - single thread
                apply_reactor_events();
            }
        };

        inline void stop() noexcept override {
            _running.store(false);
        }

    private:

        // create jobs from the current event handler entry map
        void create_jobs(
            const std::vector<pollfd_x>& pollfds_copy)
        {
            for (const auto& pollfd_ : pollfds_copy) {
                // get the event handler entry from the pollfd
                auto event_handler_entry_ptr = Event_Loop__Mid__Base::get_event_handler_entry_ptr(
                    _event_handler_entrys,
                    pollfd_,
                    false);
                if (!event_handler_entry_ptr) continue;

                // add the new jobs into _jobs
                if ((pollfd_.revents & POLL_X_IN) && event_handler_entry_ptr->_event_handler__read) {
                    _jobs.push(Job(pollfd_.fd, event_handler_entry_ptr, Enum_IO_Event_Types::Read));
                }
                if ((pollfd_.revents & POLL_X_OUT) && event_handler_entry_ptr->_event_handler__write) {
                    _jobs.push(Job(pollfd_.fd, event_handler_entry_ptr, Enum_IO_Event_Types::Write));
                }
            }
        }

        // submit the jobs via the thread pool - multiple threads
        // notice that this function is executed in the main thread.
        // hence, operations on _jobs are single-threaded and safe (especially _job_results.empty()).
        void submit_jobs() {
            while (!_jobs.empty()) {
                auto job{ _jobs.pop() };
                if (!job.has_value()) continue;

                auto job_val = std::move(job.value());
                _tp.submit(
                    [job_val, this]()
                    { execute_job<Concurrent_Queue_Type__job_result>(job_val, _job_results); });
            }
        }

        // apply the reactor events.
        // notice that this function is executed in the main thread,
        // and starts with a call to wait_all_jobs on the thread pool.
        // hence, the operations on _job_results are single-threaded and safe (especially _job_results.empty()).
        void apply_reactor_events() {
            // wait till all workers finish
            _tp.wait_all_jobs();

            // apply the reactor events
            while (!_job_results.empty()) {
                auto job_result{ _job_results.pop() };
                if (!job_result.has_value()) continue;

                auto& [fd, register_type, IO_event_type, event_handler_action_type, handler__new] = job_result.value();
                if (register_type == Enum_Register_Types::Unregister) {
                    fd_unregister(fd, IO_event_type);
                }
                else if (register_type == Enum_Register_Types::Register) {
                    fd_register(fd, IO_event_type);
                }
                if (
                    event_handler_action_type == Enum_Event_Handler_Action_Types::Add ||
                    event_handler_action_type == Enum_Event_Handler_Action_Types::Replace)
                {
                    add_event_handler(fd, std::move(handler__new), IO_event_type);
                }
            }
        }

        std::unordered_map<int, Event_Handler_Entry> _event_handler_entrys;
        std::vector<pollfd_x> _pollfds;
        Concurrent_Queue_Type__Job _jobs;
        Concurrent_Queue_Type__job_result _job_results;
        Thread_Pool_Type _tp;
        timeout_x _msec{ -1 };
        std::atomic<bool> _running{ false };
    };

    template <
        template <typename> typename Concurrent_Queue_Type,
        typename Thread_Pool_Type>
            requires CEL<
                Concurrent_Queue_Type<Job>,
                Concurrent_Queue_Type<job_result_t>,
                Thread_Pool_Type>
    using Event_Loop__Mid__HP_t = Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::HP,
        Concurrent_Queue_Type<Job>,
        Concurrent_Queue_Type<job_result_t>,
        Thread_Pool_Type>;
} // namespace BA_Socket

#endif // EVENT_LOOP__MID__HP_HPP
