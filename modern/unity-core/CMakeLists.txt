cmake_minimum_required(VERSION 3.24)
project(MoxianUnityCore VERSION 0.1.0 LANGUAGES C CXX)

option(MXH_UNITY_BUILD_TESTS "Build native Unity core tests" ON)

set(MXH_MODERN_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(MXH_SRC "${MXH_MODERN_ROOT}/src")

add_library(mxh_unity_core_impl STATIC
    ${MXH_SRC}/net/net.cpp
    ${MXH_SRC}/net/socket.cpp
    ${MXH_SRC}/crypto/crypto.cpp
    ${MXH_SRC}/crypto/hsel_stream.cpp
    ${MXH_SRC}/crypto/hsel_class.cpp
    ${MXH_SRC}/crypto/hsel_encryptor.cpp
    ${MXH_SRC}/log/mlog.cpp
    ${MXH_SRC}/client/NetworkEventQueue.cpp
    ${MXH_SRC}/client/AgentSession.cpp
    ${MXH_SRC}/client/ClientWire.cpp
    ${MXH_SRC}/unity_core/NativeClientCore.cpp
    ${MXH_SRC}/game/movement_timeline.cpp
    ${MXH_SRC}/game/player_move_speed.cpp
)

target_include_directories(mxh_unity_core_impl
    PUBLIC ${MXH_MODERN_ROOT}/include
    PRIVATE ${MXH_SRC} ${MXH_SRC}/client ${MXH_SRC}/unity_core)
target_compile_features(mxh_unity_core_impl PUBLIC cxx_std_20)
target_compile_definitions(mxh_unity_core_impl PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS
    _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
target_link_libraries(mxh_unity_core_impl PUBLIC Ws2_32)
set_target_properties(mxh_unity_core_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(mxh_unity_core SHARED ${MXH_SRC}/unity_core/UnityNativeApi.cpp)
target_include_directories(mxh_unity_core
    PUBLIC ${MXH_MODERN_ROOT}/include
    PRIVATE ${MXH_SRC} ${MXH_SRC}/client ${MXH_SRC}/unity_core)
target_compile_features(mxh_unity_core PUBLIC cxx_std_20)
target_compile_definitions(mxh_unity_core PRIVATE
    MXH_UNITY_CORE_BUILD WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_link_libraries(mxh_unity_core PRIVATE mxh_unity_core_impl)
set_target_properties(mxh_unity_core PROPERTIES
    OUTPUT_NAME "mxh_unity_core"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES)

if(MSVC)
    target_compile_options(mxh_unity_core_impl PRIVATE /W4 /utf-8 /EHsc /permissive-)
    target_compile_options(mxh_unity_core PRIVATE /W4 /utf-8 /EHsc /permissive-)
endif()

if(MXH_UNITY_BUILD_TESTS)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(${MXH_MODERN_ROOT}/third_party/googletest-src
                     ${CMAKE_BINARY_DIR}/_deps/googletest-build EXCLUDE_FROM_ALL)
    add_executable(mxh_unity_core_tests
        tests/unity_core_test.cpp
        ${MXH_MODERN_ROOT}/tests/unit/game/movement_timeline_test.cpp
        ${MXH_MODERN_ROOT}/tests/unit/game/player_move_speed_test.cpp
        ${MXH_MODERN_ROOT}/tests/unit/proto/movement_wire_test.cpp
        ${MXH_MODERN_ROOT}/tests/unit/hsel_encryptor_test.cpp
        ${MXH_SRC}/server/hsel_session.cpp)
    target_compile_features(mxh_unity_core_tests PRIVATE cxx_std_20)
    target_include_directories(mxh_unity_core_tests PRIVATE
        ${MXH_MODERN_ROOT}/include ${MXH_SRC} ${MXH_SRC}/client ${MXH_SRC}/unity_core)
    target_compile_definitions(mxh_unity_core_tests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(mxh_unity_core_tests PRIVATE
        mxh_unity_core mxh_unity_core_impl GTest::gtest_main Ws2_32)
    if(MSVC)
        target_compile_options(mxh_unity_core_tests PRIVATE /W4 /utf-8 /EHsc /permissive-)
    endif()
    include(GoogleTest)
    gtest_discover_tests(mxh_unity_core_tests DISCOVERY_TIMEOUT 30)
    add_executable(mxh_unity_abi_c_test tests/abi_c_test.c)
    target_include_directories(mxh_unity_abi_c_test PRIVATE ${MXH_MODERN_ROOT}/include)
    target_link_libraries(mxh_unity_abi_c_test PRIVATE mxh_unity_core)
    add_test(NAME UnityCoreAbi.CHeaderAndExports COMMAND mxh_unity_abi_c_test)
    add_executable(mxh_unity_legacy_header_test tests/legacy_header_test.cpp)
    target_compile_features(mxh_unity_legacy_header_test PRIVATE cxx_std_20)
    target_include_directories(mxh_unity_legacy_header_test PRIVATE
        ${MXH_MODERN_ROOT}/include ${MXH_SRC} ${MXH_SRC}/client)
    target_compile_definitions(mxh_unity_legacy_header_test PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(mxh_unity_legacy_header_test PRIVATE mxh_unity_core_impl)
    add_test(NAME UnityCoreWire.LegacyHeadersUseExtractedSymbols
             COMMAND mxh_unity_legacy_header_test)
endif()
