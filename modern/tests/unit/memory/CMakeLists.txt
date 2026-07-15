# Memory pool benchmark

add_executable(memory_benchmark memory_benchmark.cpp)

target_link_libraries(memory_benchmark PRIVATE mxh_memory)

target_include_directories(memory_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)