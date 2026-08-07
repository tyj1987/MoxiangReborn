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

# Phase 10.11: IOCP server + connection unit tests
add_executable(mxh_iocp_tests
    iocp_test.cpp
)

# mxh_net already links Ws2_32 + Mswsock on Windows; just add gtest.
target_link_libraries(mxh_iocp_tests PRIVATE mxh_net gtest_main)

target_include_directories(mxh_iocp_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_iocp_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
# Phase 10.16: net message + connection types tests
add_executable(mxh_message_tests
    message_test.cpp
)

target_link_libraries(mxh_message_tests PRIVATE mxh_net gtest_main)
target_include_directories(mxh_message_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_message_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
# E2 T2 capture handler -- SHA-256 fingerprint + on_message recorder
# used by wire SHA-256 replay (E2 T2) and side-by-side behavior diff (E3 T3).
add_executable(mxh_capture_handler_tests
    capture_handler_test.cpp
)
target_link_libraries(mxh_capture_handler_tests PRIVATE mxh_net gtest_main)
target_include_directories(mxh_capture_handler_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_capture_handler_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
