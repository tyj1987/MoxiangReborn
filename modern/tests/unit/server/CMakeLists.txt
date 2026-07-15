# Server handler tests — Phase 10.17
# Smoke tests for the 3 game-server handlers (Login / Agent / Map).
# They are linked against the mxh_server static lib (which contains
# the 3 handler .cpp files) plus gtest_main.

add_executable(mxh_server_handler_tests
    server_handler_test.cpp
)

target_link_libraries(mxh_server_handler_tests PRIVATE
    mxh_server mxh_db gtest_main
)

target_include_directories(mxh_server_handler_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_server_handler_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
