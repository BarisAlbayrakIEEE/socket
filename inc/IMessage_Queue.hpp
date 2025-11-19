// IMessage_Queue.h

#ifndef IMESSAGE_QUEUE_H
#define IMESSAGE_QUEUE_H

namespace ba_socket {
    template <typename T>
    class IMessage_Queue {
    public:
        virtual ~IMessage_Queue() = default;

        virtual void push(const T&) = 0;
        virtual bool try_pop(T&) = 0;
    };
} // namespace ba_socket

#endif // IMESSAGE_QUEUE_H
