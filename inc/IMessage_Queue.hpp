// IMessage_Queue.hpp

#ifndef IMESSAGE_QUEUE_HPP
#define IMESSAGE_QUEUE_HPP

namespace BA_Socket {
    template <typename T>
    class IMessage_Queue {
    public:
        virtual ~IMessage_Queue() = default;

        virtual void push(const T&) = 0;
        virtual bool try_pop(T&) = 0;
    };
} // namespace BA_Socket

#endif // IMESSAGE_QUEUE_HPP
