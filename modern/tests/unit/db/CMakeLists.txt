add_executable(mxh_db_tests
    db_adapter_test.cpp
    sqlite_adapter_test.cpp
    real_resource_test.cpp
)

target_link_libraries(mxh_db_tests PRIVATE mxh_db mxh_compat gtest_main)

# Skip gtest_discover_tests — Chinese path prevents JSON output in this build env.
# Run tests manually: build/tests/unit/db/Debug/mxh_db_tests.exe
