// IWorker_Pool.hpp

#ifndef IWORKER_POOL_HPP
#define IWORKER_POOL_HPP

#include <functional>

namespace BA_Socket {
    class IWorker_Pool {
    public:
        virtual ~IWorker_Pool() = default;

        virtual void submit(std::function<void()> job) = 0;
        virtual void shutdown() = 0;
    };
} // namespace BA_Socket

#endif // IWORKER_POOL_HPP
