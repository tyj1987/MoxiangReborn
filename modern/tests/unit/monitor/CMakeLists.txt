# Performance monitor tests

add_executable(mxh_monitor_tests performance_monitor_test.cpp)

target_link_libraries(mxh_monitor_tests PRIVATE mxh_monitor gtest_main)

target_include_directories(mxh_monitor_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_monitor_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)