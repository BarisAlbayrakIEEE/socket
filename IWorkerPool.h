// IWorkerPool.h

#ifndef IWORKER_POOL_H
#define IWORKER_POOL_H

#include <functional>

namespace ba_socket {
    class IWorkerPool {
    public:
        virtual ~IWorkerPool() = default;

        virtual void submit(std::function<void()> job) = 0;
        virtual void shutdown() = 0;
    };
} // namespace ba_socket

#endif // IWORKER_POOL_H
