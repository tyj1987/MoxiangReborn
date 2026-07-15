# mlog tests — Phase 10.13
# Header-only module + standalone cpp; tests link against the cpp
# via mxh_log (which is the small lib in src/CMakeLists.txt that
# wraps log/mlog.cpp).

add_executable(mxh_mlog_tests
    mlog_test.cpp
)

target_link_libraries(mxh_mlog_tests PRIVATE mxh_log gtest_main)
target_include_directories(mxh_mlog_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_mlog_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
