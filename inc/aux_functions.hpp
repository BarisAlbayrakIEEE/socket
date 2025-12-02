// aux_functions.hpp

#ifndef AUX_FUNCTIONS_HPP
#define AUX_FUNCTIONS_HPP

#include <string>
#include <type_traits>
#include <concepts>
#include "IConcurrent_Queue.hpp"
#include "IThread_Pool.hpp"

using namespace BA_Concurrency;

namespace BA_Socket {
    inline bool write_to_stdout(const std::string& buffer) {
        printf("[Client]: Received (%d bytes): %.*s", buffer.size(), buffer.size(), buffer.c_str());
        return true;
    }

    inline bool to_up(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return true;
    }
} // namespace BA_Socket

#endif // AUX_FUNCTIONS_HPP
