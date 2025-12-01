// core.hpp

#ifndef CORE_HPP
#define CORE_HPP

#include <string>

namespace BA_Socket {
    enum class Enum_Register_Types { None, Register, Unregister };
    enum class Enum_Handler_Command_Types { None, Add, Remove, Replace };
    enum class Enum_Event_Types { None, Read, Write, Read_Write }; // Read_Write is for unregister op
    
    const std::string INFO_WRONG_DATA = "Wrong data for the request";
} // namespace BA_Socket

#endif // CORE_HPP
