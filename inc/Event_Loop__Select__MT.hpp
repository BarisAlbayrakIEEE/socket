// Event_Loop__Select__MT.hpp
// Multi-threaded select event loop

#ifndef EVENT_LOOP__SELECT__MT_HPP
#define EVENT_LOOP__SELECT__MT_HPP

#include "IEvent_Loop.hpp"
#include "IConcurrent_Queue.hpp"
#include "IThread_Pool.hpp"
#include <tuple>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <optional>
#include <future>

namespace BA_Socket {
    struct Job {
        IHandler* _handler{};
    };
    using job_pack_t = std::vector<Job>;
    using job_result_t = Reactor_Command;
    using job_result_pack_t = std::vector<job_result_t>;

    template <template <typename> typename Concurrent_Queue_Type>
    size_t execute_job_pack(
        const job_pack_t& job_pack,
        Concurrent_Queue_Type<job_result_t>& job_results)
    {
        size_t job_result_count{};
        for (auto& job : job_pack) {
            auto reactor_command_pack = std::move(job._handler->apply());
            for (auto& reactor_command : reactor_command_pack) {
                ++job_result_count;
                job_results.push(std::move(reactor_command));
            }
        }
        return job_result_count;
    };

    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type>
        requires (
            std::is_base_of_v<BA_Concurrency::IThread_Pool, Thread_Pool_Type> &&
            std::is_base_of_v<BA_Concurrency::IConcurrent_Queue<Job>, Concurrent_Queue_Type<Job>> &&
            std::is_base_of_v<BA_Concurrency::IConcurrent_Queue<job_result_t>, Concurrent_Queue_Type<job_result_t>>)
    class Event_Loop__Select__MT : public IEvent_Loop {
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

    public:
        Event_Loop__Select__MT(
            time_t sec = 0,
            suseconds_t usec = 0,
            size_t thread_count = std::thread::hardware_concurrency())
                : _sec(sec), _usec(usec), _thread_pool(thread_count == 0 ? 1 : thread_count)
        {
            FD_ZERO(&_fd_set__read);
            FD_ZERO(&_fd_set__write);
        }

        inline void fd_register(int fd, Enum_Event_Types event_type) override {
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                FD_SET(fd, &_fd_set__read);
            }
            else if (event_type == Enum_Event_Types::Write) {
                FD_SET(fd, &_fd_set__write);
            }
            else { // if (event_type == Enum_Event_Types::Read_Write) {
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
        }

        inline void add_stdin_to_reads() {
            FD_SET(0, &_fd_set__read);
        }

        inline void add_stdout_to_writes() {
            FD_SET(1, &_fd_set__write);
        }

        inline void add_handler(handler_ptr_t&& handler, Enum_Event_Types event_type) {
            if (!handler) return;
            if (event_type == Enum_Event_Types::None) return;

            if (event_type == Enum_Event_Types::Read) {
                _handler_entrys[handler->get_fd()]._handler__read = std::move(handler);
            }
            else if (event_type == Enum_Event_Types::Write) {
                _handler_entrys[handler->get_fd()]._handler__write = std::move(handler);
            }
        }

        inline void run() override {
            _running.store(true);

            // main loop
            while (_running.load()) {
                if (_fd_max < 0) break;

                // perform select operation
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
                if (::select(_fd_max + 1, &fd_set__read, &fd_set__write, nullptr, timeout_ptr) < 0) {
                    if (GET_SOCKET_ERRNO() == ERROR_INTERRUPTED) continue;
                    SOCKET_ERROR__SELECT();
                    break;
                }

                // execute _handler_entrys - read
                _job_count = 0;
                for (auto& [fd, handler_entry] : _handler_entrys) {
                    // inspect the fd, the handler entry and the handler
                    if (!handler_entry._active) continue;
                    if (!IS_VALID_SOCKET(fd)) continue;
#if defined(_WIN32)
                    if (fd == 0) {
                        if (!_kbhit()) continue;
                    } else {
                        if (!FD_ISSET(fd, &fd_set__read)) continue;
                    }
#else
                    if (!FD_ISSET(fd, &fd_set__read)) continue;
#endif
                    auto handler_ptr = handler_entry._handler__read.get();
                    if (!handler_ptr) continue;

                    // push the job into the job queue
                    _jobs.push(Job(handler_ptr));
                    ++_job_count;
                }

                // execute _handler_entrys - write
                for (auto& [fd, handler_entry] : _handler_entrys) {
                    // inspect the fd, the handler entry and the handler
                    if (!handler_entry._active) continue;
                    if (!IS_VALID_SOCKET(fd)) continue;
                    if (!FD_ISSET(fd, &fd_set__write)) continue;
                    auto handler_ptr = handler_entry._handler__write.get();
                    if (!handler_ptr) continue;

                    // push the job into the job queue
                    _jobs.push(Job(handler_ptr));
                    ++_job_count;
                }

                // submit the jobs via the thread pool - multiple threads
                submit_jobs();

                // update the fd_sets and handler map by applying the reactor commands returned as the job results - single thread
                apply_reactor_commands();
            }
        };

        // submit the jobs via the thread pool - multiple threads
        void submit_jobs() {
            auto thread_count = _thread_pool.get_thread_count();
            if (_job_count == 0 || thread_count == 0) return;
            size_t job_count_per_thread = (_job_count + thread_count - 1) / thread_count;
            
            // worker loop
            size_t job_count{};
            _job_result_count = 0;
            std::vector<std::future<size_t>> job_result_counts;
            futures.reserve(thread_count);
            for (auto i = 0; i < thread_count; ++i) {
                // prepare the job inputs
                job_pack_t job_pack{};
                for (auto j = 0; j < job_count_per_thread && job_count < _job_count; ++j) {
                    auto job{ _jobs.pop() };
                    if (!job.has_value()) break;

                    ++job_count;
                    job_pack.push_back(std::move(job.value()));
                }

                // submit the job
                if (!job_pack.empty()) {
                    job_result_counts.push_back(
                        _thread_pool.submit_any(
                            &execute_job_pack<Concurrent_Queue_Type>,
                            std::move(job_pack),
                            std::ref(_job_results)));
                }
            }

            // Collect job result counts from the workers
            for (auto& job_result_count : job_result_counts) {
                _job_result_count += job_result_count.get();
            }
        }

        // update the fd_sets and handler map by applying the reactor commands returned as the job results - single thread
        void apply_reactor_commands() {
            // apply the reactor commands
            for (auto i = 0; i < _job_result_count; ++i) {
                auto job_result{ _job_results.pop() };
                auto& [fd, register_type, event_type, handler_command_type, handler__new] = job_result.value();
                if (register_type == Enum_Register_Types::Unregister) {
                    fd_unregister(fd);
                    _handler_entrys.erase(fd);
                }
                else if (register_type == Enum_Register_Types::Register) {
                    fd_register(fd, event_type);
                }
                if (
                    handler_command_type == Enum_Handler_Command_Types::Add ||
                    handler_command_type == Enum_Handler_Command_Types::Replace)
                {
                    add_handler(std::move(handler__new), event_type);
                }
            }
        }

        inline void stop() override {
            _running.store(false);
        }

    private:
        std::unordered_map<int, Handler_Entry> _handler_entrys;
        Concurrent_Queue_Type<Job> _jobs;
        Concurrent_Queue_Type<job_result_t> _job_results;
        Thread_Pool_Type _thread_pool;
        fd_set _fd_set__read;
        fd_set _fd_set__write;
        time_t _sec{0};
        suseconds_t _usec{0};
        int _fd_max = -1;
        size_t _job_count{0};
        size_t _job_result_count{0};
        std::atomic<bool> _running{false};
    };
} // namespace BA_Socket

#endif // EVENT_LOOP__SELECT__MT_HPP
