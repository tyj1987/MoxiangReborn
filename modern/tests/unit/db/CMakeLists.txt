add_executable(mxh_db_tests
    db_adapter_test.cpp
    sqlite_adapter_test.cpp
    real_resource_test.cpp
)

target_link_libraries(mxh_db_tests PRIVATE mxh_db mxh_compat gtest_main)

include(GoogleTest)
gtest_discover_tests(mxh_db_tests)