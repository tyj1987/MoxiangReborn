# Memory pool benchmark

add_executable(memory_benchmark memory_benchmark.cpp)

target_link_libraries(memory_benchmark PRIVATE mxh_memory)

target_include_directories(memory_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Memory pool tests — Phase 12.4 core coverage
add_executable(memory_pool_tests memory_pool_test.cpp)

target_link_libraries(memory_pool_tests PRIVATE mxh_memory gtest_main)

target_include_directories(memory_pool_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET memory_pool_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
