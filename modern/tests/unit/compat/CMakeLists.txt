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

# .chr motion parser tests — Phase 10.14
add_executable(mxh_chr_motion_tests
    chr_motion_test.cpp
)

target_link_libraries(mxh_chr_motion_tests PRIVATE mxh_compat gtest_main)
target_include_directories(mxh_chr_motion_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_chr_motion_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
# Cross-platform abstraction tests — Phase 10.15
add_executable(mxh_platform_tests
    platform_test.cpp
)

target_link_libraries(mxh_platform_tests PRIVATE mxh_compat gtest_main)
target_include_directories(mxh_platform_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_platform_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
# T1 resource byte-level verification - Phase 6.1
add_executable(mxh_resource_byte_level_tests
    resource_byte_level_test.cpp
)
target_link_libraries(mxh_resource_byte_level_tests PRIVATE mxh_compat gtest_main)
target_include_directories(mxh_resource_byte_level_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_resource_byte_level_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_resource_reference_tests resource_reference_test.cpp)
target_link_libraries(mxh_resource_reference_tests PRIVATE gtest_main bcrypt)
target_include_directories(mxh_resource_reference_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
include(GoogleTest)
gtest_add_tests(TARGET mxh_resource_reference_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit PRE_SHIP_BUILD)


# T1 mechanical expansion - Phase E1: read_mh_bin coverage for all 59
# deploy/server/Distribute/Resource/*.bin files. Each .bin is opened via
# read_mh_bin, header parsed, payload size asserted vs header.file_size.
add_executable(mxh_resource_parse_all_bin_tests
    resource_parse_all_bin_test.cpp
)
target_link_libraries(mxh_resource_parse_all_bin_tests PRIVATE mxh_compat gtest_main)
target_include_directories(mxh_resource_parse_all_bin_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_resource_parse_all_bin_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)


# T1 payload SHA-256 byte-level verification - Phase E1.
# Verifies modern read_mh_bin() produces payload bytes matching manifest JSON SHA-256s.
add_executable(mxh_resource_payload_sha256_tests resource_payload_sha256_test.cpp)
target_link_libraries(mxh_resource_payload_sha256_tests PRIVATE mxh_compat gtest_main bcrypt)
target_include_directories(mxh_resource_payload_sha256_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
include(GoogleTest)
gtest_add_tests(TARGET mxh_resource_payload_sha256_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit PRE_SHIP_BUILD)
