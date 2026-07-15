# .ttb tile table parser tests — Phase 10.13
# Header-only struct + standalone cpp; tests link against the cpp via
# mxh_compat (where it is already included).

add_executable(mxh_ttb_tile_table_tests
    ttb_tile_table_test.cpp
)

target_link_libraries(mxh_ttb_tile_table_tests PRIVATE mxh_compat gtest_main)
target_include_directories(mxh_ttb_tile_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_ttb_tile_table_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
