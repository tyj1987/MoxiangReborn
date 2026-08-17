# modern/tests/unit/portal/CMakeLists.txt
# M5.2: portal unit + integration tests

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

# 2. JWT token unit tests
# jwt_token.cpp defines create_jwt/verify_jwt/hmac_sha256 (real HMAC impl).
# jwt_token_test_util.cpp provides inline base64url helpers for manual JWT construction.
# Both TUs have inline base64url_encode/decode (ODR-safe via inline keyword).
add_executable(mxh_portal_jwt_tests
    jwt_token_test.cpp
    jwt_token_test_util.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/jwt_token.cpp
)
target_include_directories(mxh_portal_jwt_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal
    ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_features(mxh_portal_jwt_tests PRIVATE cxx_std_17)
target_link_libraries(mxh_portal_jwt_tests PRIVATE
    mxh_log
    gtest_main
)
if(MSVC)
    target_compile_definitions(mxh_portal_jwt_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(mxh_portal_jwt_tests PRIVATE bcrypt)
endif()

# 3. Rate limiter unit tests
add_executable(mxh_portal_ratelimit_tests
    rate_limiter_test.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/rate_limiter.cpp
)
target_include_directories(mxh_portal_ratelimit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal
    ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_features(mxh_portal_ratelimit_tests PRIVATE cxx_std_17)
target_link_libraries(mxh_portal_ratelimit_tests PRIVATE
    mxh_log
    gtest_main
)
if(MSVC)
    target_compile_definitions(mxh_portal_ratelimit_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()


# 4. Auth routes unit tests (M5.3)
# Links mxh_db + mxh_server for handle_register/handle_login/handle_me.
add_executable(mxh_portal_auth_route_tests
    auth_route_test.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/auth_routes.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/jwt_token.cpp
)
target_include_directories(mxh_portal_auth_route_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal
    ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_features(mxh_portal_auth_route_tests PRIVATE cxx_std_17)
target_link_libraries(mxh_portal_auth_route_tests PRIVATE
    mxh_log mxh_db mxh_server mxh_compat
    gtest_main
)
if(MSVC)
    target_compile_definitions(mxh_portal_auth_route_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(mxh_portal_auth_route_tests PRIVATE bcrypt)
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
    message(STATUS "Python3 not found â€” skipping PortalIntegrationSmoke")
endif()

gtest_add_tests(TARGET mxh_portal_auth_route_tests
    AuthRegisterValidation.NonObjectBodyReturns400
    AuthRegisterValidation.MissingFieldsReturns400
    AuthRegisterValidation.PasswordMismatchReturns400
    AuthRegisterValidation.InvalidAccountNameReturns400
    AuthRegisterValidation.WeakPasswordReturns400
    AuthLoginValidation.EmptySecretReturns500
    AuthLoginValidation.NonObjectBodyReturns401
    AuthLoginValidation.MissingFieldsReturns401
    AuthLogout.StatelessReturns204
    AuthRoutesDbTest.RegisterDuplicateAccountReturns409
    AuthRoutesDbTest.LoginUnknownAccountReturns401
    AuthRoutesDbTest.MeUnknownAccountReturns404
    AuthRoutesDbTest.RegisterValidAccountReturns201WithUserIdx
    AuthRoutesDbTest.LoginValidCredentialsReturnsTokenAndUserIdx
    AuthRoutesDbTest.MeAfterLoginReturnsAccountInfo
)

# M5.5 news content loader tests
add_executable(mxh_portal_news_route_tests
    news_route_test.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/content_loader.cpp
)
target_include_directories(mxh_portal_news_route_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal
)
target_compile_features(mxh_portal_news_route_tests PRIVATE cxx_std_17)
target_link_libraries(mxh_portal_news_route_tests PRIVATE
    mxh_log
    gtest_main
)
if(MSVC)
    target_compile_definitions(mxh_portal_news_route_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# M5.6 shop catalog tests
add_executable(mxh_portal_shop_route_tests
    shop_route_test.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/content_loader.cpp
)
target_include_directories(mxh_portal_shop_route_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal
)
target_compile_features(mxh_portal_shop_route_tests PRIVATE cxx_std_17)
target_link_libraries(mxh_portal_shop_route_tests PRIVATE
    mxh_log
    gtest_main
)
if(MSVC)
    target_compile_definitions(mxh_portal_shop_route_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
