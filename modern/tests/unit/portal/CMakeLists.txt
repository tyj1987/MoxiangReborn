# modern/tests/unit/portal/CMakeLists.txt
# M5.1: portal unit + integration tests

# 1. C++ unit tests (config loading)
add_executable(mxh_portal_config_tests
    portal_config_test.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/config.cpp
)
target_include_directories(mxh_portal_config_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal
    ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_features(mxh_portal_config_tests PRIVATE cxx_std_17)
target_link_libraries(mxh_portal_config_tests PRIVATE
    mxh_log
    gtest_main
)
if(MSVC)
    target_compile_definitions(mxh_portal_config_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

include(GoogleTest)
gtest_add_tests(TARGET mxh_portal_config_tests
    PortalConfig.DefaultsAreSensible
    PortalConfig.EnvOverridesDefaults
    PortalConfig.EnvDoesNotCrashOnMissingVars
)

# Python integration smoke
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(
        NAME PortalIntegrationSmoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/portal_integration_test.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    message(STATUS "Python3 not found — skipping PortalIntegrationSmoke")
endif()
