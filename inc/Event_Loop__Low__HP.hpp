// Event_Loop__Low__HP.hpp :
//   Platform                 : Cross-platform
//   Performance              : Low (select-based configuration)
//   Concurrency              :
//     Event loop             : Single-threaded
//     Handler execution      : Parallel

#ifndef EVENT_LOOP__LOW__HP_HPP
#define EVENT_LOOP__LOW__HP_HPP

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <optional>
#include "IEvent_Loop.hpp"
#include "Event_Handler.hpp"

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
        Enum_Event_Loop_Types::Low,
        Enum_Concurrency_Types::HP,
        Concurrent_Queue_Type__Job,
        Concurrent_Queue_Type__job_result,
        Thread_Pool_Type>
            : public IEvent_Loop
    {
    public:
        Event_Loop(
            time_t sec = 0,
            suseconds_t usec = 0,
            size_t thread_count = std::thread::hardware_concurrency())
                : _sec(sec), _usec(usec), _tp(thread_count == 0 ? 1 : thread_count)
        {
            FD_ZERO(&_fd_set__read);
            FD_ZERO(&_fd_set__write);
        }

        inline void fd_register(int fd, Enum_IO_Event_Types event_type) override {
            if (event_type == Enum_IO_Event_Types::None) return;

            if (event_type == Enum_IO_Event_Types::Read) {
                FD_SET(fd, &_fd_set__read);
            }
            else if (event_type == Enum_IO_Event_Types::Write) {
                FD_SET(fd, &_fd_set__write);
            }
            else { // if (event_type == Enum_IO_Event_Types::Read_Write) {
                FD_SET(fd, &_fd_set__read);
                FD_SET(fd, &_fd_set__write);
            }
            if (fd > _fd_max) _fd_max = fd;
        }

        inline void fd_unregister(int fd) override {
            FD_CLR(fd, &_fd_set__read);
            FD_CLR(fd, &_fd_set__write);
            if (fd == _fd_max) {
                auto fd_max = _fd_max;
                _fd_max = -1;
                for(SOCKET fdi = 0; fdi <= fd_max; ++fdi) {
                    if (FD_ISSET(fdi, &_fd_set__read)) {
                        if (fdi > _fd_max) _fd_max = fdi;
                    }
                    if (FD_ISSET(fdi, &_fd_set__write)) {
                        if (fdi > _fd_max) _fd_max = fdi;
                    }
                }
            }
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
                if (_fd_max < 0) break;

                // copy fd_sets
                fd_set fd_set__read = _fd_set__read;
                fd_set fd_set__write = _fd_set__write;

                // windows only:
                //   Windows doesn't support fd_set for stdin.
                //   So, a timeout loop is required.
                struct timeval timeout;
                struct timeval* timeout_ptr = nullptr;
                if (_sec || _usec) {
                    timeout.tv_sec  = _sec;
                    timeout.tv_usec = _usec;
                    timeout_ptr     = &timeout;
                }

                // perform select operation
                if (::select(_fd_max + 1, &fd_set__read, &fd_set__write, nullptr, timeout_ptr) < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__LOW();
                    break;
                }

                // create jobs from the current handler entry map
                create_jobs(fd_set__read, fd_set__write);

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

        // get handler from the handler entry
        inline IEvent_Handler* get_event_handler_ptr(
            int fd,
            fd_set fd_set__read,
            fd_set fd_set__write,
            Event_Handler_Entry& event_handler_entry,
            Enum_IO_Event_Types event_type)
        {
            if (!IS_VALID_SOCKET(fd)) return nullptr;
            if (!event_handler_entry._active) return nullptr;
            IEvent_Handler *event_handler_ptr{ nullptr };
            if (event_type == Enum_IO_Event_Types::Read) {
#if defined(_WIN32)
                if (fd == 0) {
                    if (!_kbhit()) return nullptr;
                } else {
                    if (!FD_ISSET(fd, &fd_set__read)) return nullptr;
                }
#else
                if (!FD_ISSET(fd, &fd_set__read)) return nullptr;
#endif
                event_handler_ptr = event_handler_entry._event_handler__read.get();
            }
            else if (event_type == Enum_IO_Event_Types::Write) {
                if (!FD_ISSET(fd, &fd_set__write)) return nullptr;
                event_handler_ptr = event_handler_entry._event_handler__write.get();
            }
            return event_handler_ptr;
        }

        // create jobs from the current handler entry map
        inline void create_jobs(fd_set fd_set__read, fd_set fd_set__write) {
            create_jobs_helper(fd_set__read, fd_set__write, Enum_IO_Event_Types::Read);
            create_jobs_helper(fd_set__read, fd_set__write, Enum_IO_Event_Types::Write);
        }

        // create jobs from the current handler entry map - helper
        void create_jobs_helper(
            fd_set fd_set__read,
            fd_set fd_set__write,
            Enum_IO_Event_Types event_type)
        {
            for (auto& [fd, event_handler_entry] : _event_handler_entrys) {
                auto event_handler_ptr = get_event_handler_ptr(
                    fd,
                    fd_set__read,
                    fd_set__write,
                    event_handler_entry,
                    event_type);
                if (!event_handler_ptr) continue;
                _jobs.push(Job(fd, event_handler_ptr));
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

            // apply the reactor commands
            while (!_job_results.empty()) {
                auto job_result{ _job_results.pop() };
                if (!job_result.has_value()) continue;

                auto& [fd, register_type, event_type, event_handler_action_type, handler__new] = job_result.value();
                if (register_type == Enum_Register_Types::Unregister) {
                    fd_unregister(fd);
                }
                else if (register_type == Enum_Register_Types::Register) {
                    fd_register(fd, event_type);
                }
                if (
                    event_handler_action_type == Enum_Event_Handler_Action_Types::Add ||
                    event_handler_action_type == Enum_Event_Handler_Action_Types::Replace)
                {
                    add_event_handler(fd, std::move(handler__new), event_type);
                }
            }
        }

        std::unordered_map<int, Event_Handler_Entry> _event_handler_entrys;
        Concurrent_Queue_Type__Job _jobs;
        Concurrent_Queue_Type__job_result _job_results;
        Thread_Pool_Type _tp;
        fd_set _fd_set__read;
        fd_set _fd_set__write;
        time_t _sec{0};
        suseconds_t _usec{0};
        int _fd_max = -1;
        std::atomic<bool> _running{false};
    };

    template <
        template <typename> typename Concurrent_Queue_Type,
        typename Thread_Pool_Type>
            requires CEL<
                Concurrent_Queue_Type<Job>,
                Concurrent_Queue_Type<job_result_t>,
                Thread_Pool_Type>
    using Event_Loop__Low__HP = Event_Loop<
        Enum_Event_Loop_Types::Low,
        Enum_Concurrency_Types::HP,
        Concurrent_Queue_Type<Job>,
        Concurrent_Queue_Type<job_result_t>,
        Thread_Pool_Type>;
} // namespace BA_Socket

#endif // EVENT_LOOP__LOW__HP_HPP
