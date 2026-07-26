add_executable(mxh_compat_tests
    mh_file_ex_test.cpp
    mh_file_ex_utf8_test.cpp
    pack_file_test.cpp
    bsad_area_test.cpp
    bmhm_map_test.cpp
    chx_real_resource_test.cpp
    chx_model_test.cpp
    compat/chr_motion_test.cpp
    item_effects_test.cpp
)

target_link_libraries(mxh_compat_tests PRIVATE mxh_compat gtest_main)

include(GoogleTest)
# See tests/unit/render/CMakeLists.txt for why we don't use gtest_discover_tests.
# gtest_add_tests auto-discovers TEST macros; do NOT list names here.
# chx / chr tests picked up automatically.
gtest_add_tests(TARGET mxh_compat_tests)


# -----------------------------------------------------------------------------
# mxh_crypto tests ????HSEL stream cipher (Phase 3.2)
# -----------------------------------------------------------------------------
add_executable(mxh_crypto_tests
    hsel_stream_test.cpp
    hsel_class_test.cpp
    hackshield_stub_test.cpp
    aes_gcm_test.cpp
)

target_link_libraries(mxh_crypto_tests PRIVATE mxh_crypto gtest_main)

# Use add_test (not gtest_add_tests) to run ALL tests in the binary,
# including parameterized tests (AllTypesSizes/HselRoundTripTest.*).
# gtest_add_tests can't enumerate parameterized test names.
add_test(NAME mxh_crypto_all_tests
    COMMAND $<TARGET_FILE:mxh_crypto_tests>)

# ---------------------------------------------------------------------------------------------------------------------------------------------
# mxh_crypto_benchmark ????HSEL vs AES-256-GCM throughput & latency (Phase 3.5)
# ---------------------------------------------------------------------------------------------------------------------------------------------
add_executable(mxh_crypto_benchmark
    crypto_benchmark.cpp
)

target_link_libraries(mxh_crypto_benchmark PRIVATE mxh_crypto)

# ---------------------------------------------------------------------------------------------------------------------------------------------
# mxh_proto negotiation tests ????Phase 3.4: cipher negotiation wire protocol
# Header-only library (no compiled sources); test links against the header directly.
# ---------------------------------------------------------------------------------------------------------------------------------------------
add_executable(mxh_proto_tests
    negotiate_test.cpp
)

target_link_libraries(mxh_proto_tests PRIVATE gtest_main)
target_include_directories(mxh_proto_tests PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
)

add_test(NAME mxh_proto_negotiate_tests
    COMMAND $<TARGET_FILE:mxh_proto_tests>)
# -----------------------------------------------------------------------------
# mxh_util tests ??? thread pool / object pool / compression (Phase 8 helpers)
# Header-only util module, links gtest_main only.
# -----------------------------------------------------------------------------
add_executable(mxh_util_tests
    util_test.cpp
)

target_link_libraries(mxh_util_tests PRIVATE gtest_main)
target_include_directories(mxh_util_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_util_tests)

# -----------------------------------------------------------------------------
# mxh_version tests ??? protocol version negotiation (Phase 3.4 / Phase 7.5)
# Header-level checks against mxh::proto::protocol.hpp and mxh::net::MsgHeader.
# -----------------------------------------------------------------------------
add_executable(mxh_version_tests
    version_test.cpp
)

target_link_libraries(mxh_version_tests PRIVATE mxh_net gtest_main)
target_include_directories(mxh_version_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_version_tests)
# -----------------------------------------------------------------------------
# mxh_game tests ??? Phase 10d game data structures
# Header-only structs (item / monster / skill). Verifies wire-format byte
# sizes match the legacy CommonStruct.h / GameResourceStruct.h layout and
# exercises the small helper functions.
# -----------------------------------------------------------------------------
add_executable(mxh_game_types_tests
    game_types_test.cpp
)

target_link_libraries(mxh_game_types_tests PRIVATE gtest_main)
target_include_directories(mxh_game_types_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_game_types_tests)

# -----------------------------------------------------------------------------
# mxh_services tests ??? Phase 13 service interface mock + contract test
# Header-only interfaces (IInventoryService / ISkillService /
# IPlayerStatsService) under modern/include/mxh/services/. The real
# server-backed implementations will land in modern/src/services/ in
# later sessions; this test file provides in-memory mock impls and
# pins the contract that real impls must replicate.
# -----------------------------------------------------------------------------
add_executable(mxh_services_tests
    services/services_test.cpp
)

target_link_libraries(mxh_services_tests PRIVATE gtest_main)
target_include_directories(mxh_services_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_services_tests)

# -----------------------------------------------------------------------------
# mxh_services_real tests ??? Phase 13.2 real service implementation
# contract test (vs. the mock test in mxh_services_tests above).
# The real impls are header-only; the test executable links the
# mxh_services INTERFACE target so the headers are findable.
# -----------------------------------------------------------------------------
add_executable(mxh_services_real_tests
    services/real/services_real_test.cpp
)

target_link_libraries(mxh_services_real_tests PRIVATE
    mxh_services gtest_main)
target_include_directories(mxh_services_real_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/services
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_services_real_tests)

add_executable(mxh_tools_side_by_side_tests
    tools_side_by_side_test.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide/packet.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide/capture/packet_capture.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide/diff/packet_diff.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide/replay/replay.cpp
)
target_link_libraries(mxh_tools_side_by_side_tests PRIVATE gtest_main)
target_include_directories(mxh_tools_side_by_side_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_tools_side_by_side_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# mxh_login_sbs_e2e_tests end-to-end smoke for side-by-side login path
add_executable(mxh_login_sbs_e2e_tests
    login_sbs_e2e_test.cpp
)
target_link_libraries(mxh_login_sbs_e2e_tests PRIVATE gtest_main)
target_include_directories(mxh_login_sbs_e2e_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
include(GoogleTest)
gtest_add_tests(TARGET mxh_login_sbs_e2e_tests)