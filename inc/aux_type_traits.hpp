// aux_type_traits.hpp

#ifndef AUX_TYPE_TRAITS_HPP
#define AUX_TYPE_TRAITS_HPP

#include <string>
#include <type_traits>
#include <concepts>
#include "IConcurrent_Queue.hpp"
#include "IThread_Pool.hpp"

using namespace BA_Concurrency;

namespace BA_Socket {
    template <typename F>
    concept CString_Forward = 
        requires (F f, const std::string& s) { { f(s) } -> std::same_as<bool>; };

    template <typename F>
    concept CString_Transform = 
        requires (F f, std::string& s) { { f(s) } -> std::same_as<bool>; };
    
    using string_forward_t = bool(const std::string&);
    using string_transform_t = bool(std::string&);

    template <template <typename> typename Concurrent_Queue_Type, typename Thread_Pool_Type, typename T, typename U>
    concept CEL = (
        std::is_base_of_v<BA_Concurrency::IThread_Pool, Thread_Pool_Type> &&
        std::is_base_of_v<BA_Concurrency::IConcurrent_Queue<T>, Concurrent_Queue_Type<T>> &&
        std::is_base_of_v<BA_Concurrency::IConcurrent_Queue<U>, Concurrent_Queue_Type<U>>);
} // namespace BA_Socket

#endif // AUX_TYPE_TRAITS_HPP
