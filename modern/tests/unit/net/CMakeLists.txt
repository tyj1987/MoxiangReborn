add_executable(mxh_net_tests
    net_test.cpp
    socket_test.cpp
)

# mxh_net already links Ws2_32 on Windows; just add gtest.
target_link_libraries(mxh_net_tests PRIVATE mxh_net gtest_main)

target_include_directories(mxh_net_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_net_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

# Phase 8.4: TCP throughput & latency benchmark
add_executable(net_benchmark net_benchmark.cpp)
target_link_libraries(net_benchmark PRIVATE mxh_net)
target_include_directories(net_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)
