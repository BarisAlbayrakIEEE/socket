# ./src/CMakeLists.txt

#file(GLOB SRC_FILES *.cpp)

add_executable(TCP_server__select__ST TCP_server__select__ST.cpp)
add_executable(TCP_server__select__MT TCP_server__select__MT.cpp)
add_executable(TCP_client__select__ST TCP_client__select__ST.cpp)
add_executable(TCP_client__select__MT TCP_client__select__MT.cpp)
add_executable(TCP_server__poll__ST TCP_server__poll__ST.cpp)
add_executable(TCP_server__poll__MT TCP_server__poll__MT.cpp)
add_executable(TCP_client__poll__ST TCP_client__poll__ST.cpp)
add_executable(TCP_client__poll__MT TCP_client__poll__MT.cpp)
target_include_directories(TCP_server__select__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_server__select__MT PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_client__select__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_client__select__MT PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_server__poll__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_server__poll__MT PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_client__poll__ST PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
target_include_directories(TCP_client__poll__MT PRIVATE ${_CONCURRENT_INC_DIR} ${_INC_DIR})
#set_target_properties(
#  BA_Socket
#  PROPERTIES
#  ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
#  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
#)
#target_compile_options(BA_Socket PRIVATE -Wall -Wextra -Wpedantic)
