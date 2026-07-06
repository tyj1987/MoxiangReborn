add_executable(mxh_compat_tests
    mh_file_ex_test.cpp
    pack_file_test.cpp
    bsad_area_test.cpp
    chx_real_resource_test.cpp
)

target_link_libraries(mxh_compat_tests PRIVATE mxh_compat gtest_main)

# Skip gtest_discover_tests — Chinese path prevents JSON output in this build env.
# Run tests manually: build/tests/unit/Debug/mxh_compat_tests.exe
