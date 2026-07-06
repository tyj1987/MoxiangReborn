add_executable(mxh_compat_tests
    mh_file_ex_test.cpp
    pack_file_test.cpp
    bsad_area_test.cpp
)

target_link_libraries(mxh_compat_tests PRIVATE mxh_compat gtest_main)

include(GoogleTest)
gtest_discover_tests(mxh_compat_tests)