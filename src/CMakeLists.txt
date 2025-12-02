# ./src/CMakeLists.txt

#file(GLOB SRC_FILES *.cpp)

add_executable(TCP__Server__Low__ST TCP__Server__Low__ST.cpp)
add_executable(TCP__Server__Low__HP TCP__Server__Low__HP.cpp)
add_executable(TCP__Server__Mid__ST TCP__Server__Mid__ST.cpp)
add_executable(TCP__Server__Mid__HP TCP__Server__Mid__HP.cpp)
add_executable(TCP__Client__Low__ST TCP__Client__Low__ST.cpp)
add_executable(TCP__Client__Low__HP TCP__Client__Low__HP.cpp)
add_executable(TCP__Client__Mid__ST TCP__Client__Mid__ST.cpp)
add_executable(TCP__Client__Mid__HP TCP__Client__Mid__HP.cpp)
target_include_directories(TCP__Server__Low__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Server__Low__HP PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Server__Mid__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Server__Mid__HP PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Client__Low__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Client__Low__HP PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Client__Mid__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP__Client__Mid__HP PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
#set_target_properties(
#  BA_Socket
#  PROPERTIES
#  ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
#  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
#)
#target_compile_options(BA_Socket PRIVATE -Wall -Wextra -Wpedantic)
