# Protocol layer tests — Phase 10.12 wire-format pinning for the
# 12 enum classes in modern/include/mxh/proto/protocol.hpp.
# Header-only library, no compiled sources; test links directly
# against the header.

add_executable(mxh_protocol_tests
    protocol_test.cpp
)

target_link_libraries(mxh_protocol_tests PRIVATE gtest_main)
target_include_directories(mxh_protocol_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_protocol_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
