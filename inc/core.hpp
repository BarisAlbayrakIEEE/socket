// core.hpp

#ifndef CORE_HPP
#define CORE_HPP

#include <string>
#include <memory>

namespace BA_Socket {
    const std::string INFO_WRONG_DATA = "Wrong data for the request";

    enum class Enum_Register_Types { None, Register, Unregister };
    enum class Enum_IO_Event_Types { None, Read, Write, Read_Write }; // Read_Write is for unregister op
    enum class Enum_Event_Handler_Action_Types { None, Add, Remove, Replace };
    enum class Enum_Event_Loop_Types{ Low, Mid, High };
    enum class Enum_Concurrency_Types{
        ST,  // single-threaded (no concurrency)
        HP   // HP: handler parallelism
    };

    struct IEvent_Handler;
    using event_handler_ptr_t = std::unique_ptr<IEvent_Handler>;
} // namespace BA_Socket

#endif // CORE_HPP
