// Event_Loop__Mid__HP.hpp :
//   Platform                 : Cross-platform
//   Performance              : Mid (poll-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Parallel

#ifndef EVENT_LOOP__MID__HP_HPP
#define EVENT_LOOP__MID__HP_HPP

#include <unordered_map>
#include <vector>
#include <atomic>
#include <thread>
#include <optional>
#include "poll_setup.hpp"
#include "IEvent_Loop.hpp"
#include "Event_Handler.hpp"

namespace BA_Socket {
    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires CEL<Concurrent_Queue_Type, Thread_Pool_Type, Job, job_result_t>
    class Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::HP,
        Thread_Pool_Type,
        Job,
        job_result_t>
            : public IEvent_Loop
    {
    public:
        Event_Loop(
            timeout_x msec = -1,
            size_t thread_count = std::thread::hardware_concurrency())
                : _msec(msec), _tp(thread_count == 0 ? 1 : thread_count) {}

        inline void fd_register(int fd, Enum_IO_Event_Types event_type) override {
            if (event_type == Enum_IO_Event_Types::None) return;

            short events = 0;
            if (event_type == Enum_IO_Event_Types::Read) {
                events |= POLL_X_IN;
            } else if (event_type == Enum_IO_Event_Types::Write) {
                events |= POLL_X_OUT;
            } else { // Read_Write
                events |= POLL_X_IN | POLL_X_OUT;
            }

            // update or add pollfd
            auto it = std::find_if(
                _pollfds.begin(),
                _pollfds.end(),
                [fd](const auto& pollfd_){ return pollfd_.fd == fd; });
            if (it != _pollfds.end()) {
                it->events |= events;
            } else {
                pollfd_t pollfd_{};
                pollfd_.fd = fd;
                pollfd_.events = events;
                pollfd_.revents = 0;
                _pollfds.push_back(pollfd_);
            }
        }

        inline void fd_unregister(int fd) override {
            _pollfds.erase(
                std::remove_if(
                    _pollfds.begin(),
                    _pollfds.end(),
                    [fd](const pollfd_t& pollfd_){ return pollfd_.fd == fd; }),
                _pollfds.end());
            _event_handler_entrys.erase(fd);
        }

        inline void add_event_handler(int fd, event_handler_ptr_t&& handler, Enum_IO_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_IO_Event_Types::None) return;

            auto& event_handler_entry = _event_handler_entrys[fd];
            if (event_type == Enum_IO_Event_Types::Read) {
                event_handler_entry._event_handler__read = std::move(handler);
            } else if (event_type == Enum_IO_Event_Types::Write) {
                event_handler_entry._event_handler__write = std::move(handler);
            }
        }

        inline void run() override {
            _running.store(true);

            // main loop
            while (_running.load()) {
                if (_pollfds.empty()) break;

                // perform poll operation
                std::vector<pollfd_t> pollfds_copy = _pollfds;
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

                // create jobs from the current handler entry map
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

        // get the event handler entry from the pollfd
        inline Event_Handler_Entry* get_event_handler_entry_ptr(const pollfd_t & pollfd_) {
            if (pollfd_.revents == 0) return nullptr;
            if (!IS_VALID_SOCKET(pollfd_.fd)) return nullptr;
#if defined(_WIN32)
            if (pollfd_.fd == 0 && !_kbhit()) return nullptr;
#endif

            auto it = _event_handler_entrys.find(pollfd_.fd);
            if (it == _event_handler_entrys.end()) return nullptr;
            auto& event_handler_entry = it->second;
            if (!event_handler_entry._active) return nullptr;
            return &event_handler_entry;
        }

        // create jobs from the current handler entry map
        void create_jobs(
            const std::vector<pollfd_t>& pollfds_copy)
        {
            for (const auto& pollfd_ : pollfds_copy) {
                // get the event handler entry from the pollfd
                auto event_handler_entry_ptr = get_event_handler_entry_ptr(pollfd_);
                if (!event_handler_entry_ptr) continue;

                // add the new jobs into _jobs
                if ((pollfd_.revents & POLL_X_IN) && event_handler_entry_ptr->_event_handler__read) {
                    _jobs.push(Job(pollfd_.fd, event_handler_entry_ptr->_event_handler__read));
                }
                if ((pollfd_.revents & POLL_X_OUT) && event_handler_entry_ptr->_event_handler__write) {
                    _jobs.push(Job(pollfd_.fd, event_handler_entry_ptr->_event_handler__write));
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
                    { execute_job<Concurrent_Queue_Type>(job_val, _job_results); });
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

                auto& [fd, register_type, event_type, handler_command_type, handler__new] = job_result.value();
                if (register_type == Enum_Register_Types::Unregister) {
                    fd_unregister(fd);
                }
                else if (register_type == Enum_Register_Types::Register) {
                    fd_register(fd, event_type);
                }
                if (
                    handler_command_type == Enum_Handler_Command_Types::Add ||
                    handler_command_type == Enum_Handler_Command_Types::Replace)
                {
                    add_event_handler(fd, std::move(handler__new), event_type);
                }
            }
        }

        std::unordered_map<int, Event_Handler_Entry> _event_handler_entrys;
        std::vector<pollfd_t> _pollfds;
        Concurrent_Queue_Type<Job> _jobs;
        Concurrent_Queue_Type<job_result_t> _job_results;
        Thread_Pool_Type _tp;
        timeout_x _msec{ -1 };
        std::atomic<bool> _running{ false };
    };

    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires CEL<Concurrent_Queue_Type, Thread_Pool_Type, Job, job_result_t>
    using Event_Loop__Mid__HP = Event_Loop<
        Enum_Event_Loop_Types::Mid,
        Enum_Concurrency_Types::HP,
        Thread_Pool_Type,
        Job,
        job_result_t>;
} // namespace BA_Socket

#endif // EVENT_LOOP__MID__HP_HPP
