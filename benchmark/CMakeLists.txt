find_package(benhmark REQUIRED)

file(GLOB HEADER_BENCHMARK *h)
file(GLOB SORCE_BENCHMARK *.cpp)

add_executable(
  benhmark.out
  ${HEADER_BENCHMARK}
  ${SORCE_BENCHMARK}
)
target_include_directories(
  benhmark.out
  PRIVATE ${_CONCURRENT_INC_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${_BENCKMARK_INC_DIR}
)
link_directories(${_CONCURRENT_LIB_DIR})
target_link_libraries(
  benhmark.out
  PRIVATE BA_Socket
  benhmark
  benhmark_main
  pthread
)
