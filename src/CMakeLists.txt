file(GLOB SRC_FILES *.cpp)

add_executable(BA_Socket ${SRC_FILES})
target_include_directories(
  BA_Socket
  INTERFACE ${_CONCURRENT_INC_DIR}
  INTERFACE ${_INC_DIR}
)
set_target_properties(
  BA_Socket
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
)
target_compile_options(BA_Socket PRIVATE -Wall -Wextra -Wpedantic)
