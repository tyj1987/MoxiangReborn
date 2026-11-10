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
    sound_list_test.cpp
    audio/bgm_player_test.cpp
    audio/sfx_player_test.cpp
    hfl_height_field_test.cpp
    stm_static_model_test.cpp
    map_change_catalog_test.cpp
    gx_map_script_test.cpp
)

target_link_libraries(mxh_compat_tests PRIVATE mxh_compat mxh_audio mxh_game gtest_main)

include(GoogleTest)

# MXH_PLAYDH_ROOT contract. Header-only; no game data and no Win32.
add_executable(mxh_playdh_env_tests
    playdh_env_test.cpp
)
target_link_libraries(mxh_playdh_env_tests PRIVATE gtest_main)
target_compile_features(mxh_playdh_env_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_playdh_env_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# CInGameState PlayDH resolution. Header-only; the walk under test is the
# same function CInGameState::Init calls. No game bytes and no Win32.
add_executable(mxh_ingame_playdh_root_tests
    ingame_playdh_root_test.cpp
)
target_include_directories(mxh_ingame_playdh_root_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client)
target_link_libraries(mxh_ingame_playdh_root_tests PRIVATE gtest_main)
target_compile_features(mxh_ingame_playdh_root_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_ingame_playdh_root_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)

# MoxianClientE2E port base. Header-only; no servers and no Win32.
add_executable(mxh_e2e_ports_tests
    e2e_ports_test.cpp
)
target_link_libraries(mxh_e2e_ports_tests PRIVATE gtest_main)
target_compile_features(mxh_e2e_ports_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_e2e_ports_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# mxh_client_e2e soak flags. Parser, JSON writer, and the RST linger seam.
# No client library, no server, no original game bytes. Live login / peer /
# socket-drop checks stay on Windows.
add_executable(mxh_e2e_soak_tests
    e2e_soak_test.cpp
)
target_include_directories(mxh_e2e_soak_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/MoxianClientE2E
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(mxh_e2e_soak_tests PRIVATE gtest_main)
target_compile_features(mxh_e2e_soak_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_definitions(mxh_e2e_soak_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_e2e_soak_tests PRIVATE /W3 /utf-8)
endif()
gtest_add_tests(TARGET mxh_e2e_soak_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# DPI math for the fixed game client. No window, no renderer, no game bytes.
add_executable(mxh_dpi_scale_tests dpi_scale_test.cpp)
target_include_directories(mxh_dpi_scale_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mxh_dpi_scale_tests PRIVATE gtest_main)
target_compile_features(mxh_dpi_scale_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_dpi_scale_tests)

# Item wire numbers against the generated original profile. Header-only.
add_executable(mxh_item_protocol_tests item_protocol_test.cpp)
target_include_directories(mxh_item_protocol_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mxh_item_protocol_tests PRIVATE gtest_main)
target_compile_features(mxh_item_protocol_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_item_protocol_tests)

# Canonical Item protocol hash. The host tool and CMake script are the only
# hash writers. Synthetic cases use temp lists; the real-header case does
# not assert wire numbers.
find_program(MXH_PYTHON NAMES python3 python)
if(NOT MXH_PYTHON)
  set(MXH_PYTHON "python3")
endif()
add_executable(mxh_protocol_hash_tests protocol_hash_test.cpp)
target_include_directories(mxh_protocol_hash_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/generated
  ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mxh_protocol_hash_tests PRIVATE gtest_main)
target_compile_features(mxh_protocol_hash_tests PRIVATE cxx_std_20)
add_dependencies(mxh_protocol_hash_tests mxh_item_protocol_hash)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/protocol_hash_test_paths.hpp" CONTENT
"#pragma once
inline constexpr char kMxhCmake[] = R\"p(${CMAKE_COMMAND})p\";
inline constexpr char kMxhHashScript[] = R\"p(${MXH_ITEM_PROTOCOL_HASH_SCRIPT})p\";
inline constexpr char kMxhCanonList[] = R\"p(${MXH_ITEM_CANON_LIST})p\";
inline constexpr char kMxhGatePy[] = R\"p(${CMAKE_SOURCE_DIR}/../scripts/protocol_manifest_gate.py)p\";
inline constexpr char kMxhPython[] = R\"p(${MXH_PYTHON})p\";
")
gtest_add_tests(TARGET mxh_protocol_hash_tests)

# Login Item-protocol gate. Header-only decision and nack text. Synthetic
# hashes only; the generated protocol hash is not an input.
add_executable(mxh_login_protocol_gate_tests login_protocol_gate_test.cpp)
target_include_directories(mxh_login_protocol_gate_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mxh_login_protocol_gate_tests PRIVATE gtest_main)
target_compile_features(mxh_login_protocol_gate_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_login_protocol_gate_tests)

# Audio decisions shared with the Windows playback path. Header-only.
# Synthetic inputs only: no PlayDH, no pak, no SoundList.bin.
add_executable(mxh_audio_contract_tests audio_contract_test.cpp)
target_include_directories(mxh_audio_contract_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mxh_audio_contract_tests PRIVATE gtest_main)
target_compile_features(mxh_audio_contract_tests PRIVATE cxx_std_20)
gtest_add_tests(TARGET mxh_audio_contract_tests)

# See tests/unit/render/CMakeLists.txt for why we don't use gtest_discover_tests.
# gtest_add_tests auto-discovers TEST macros; do NOT list names here.
# chx / chr tests picked up automatically.
gtest_add_tests(TARGET mxh_compat_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
# Real PlayDH cases. gtest_add_tests is the only registration. There is no
# whole-binary add_test for mxh_compat_tests, so these names are not listed twice.
set_tests_properties(
    HflHeightField.ParsesRealMap12Terrain
    HflHeightField.ParsesRecoveredMap17LooseTerrain
    StmStaticModel.ParsesRealMap12Scene
    StmStaticModel.ParsesRealMap12SkyModel
    StmStaticModel.RealL001BonesAndPhysique
    StmStaticModel.ParsesEveryDefaultMaleCharacterPart
    SoundList.LoadsRealPlayDhSoundListWhenAvailable
    BgmPlayer.ResolvesOriginalLoginMusic
    BgmPlayer.PlaybackLoopReportsCurrentId
    BgmPlayer.PlaybackReplacesCurrentBgm
    BmhmMap.RealMap0
    BmhmMap.RealMap17
    BmhmMap.RealMap101
    PROPERTIES LABELS "rog")

# STM .vcl vertex-color contract (DX-free). RealMap10VclMatchesStm reads
# original data read-only from $MXH_ORIGINAL_MAP_DIR and skips otherwise.
add_executable(mxh_stm_vertex_color_tests stm_vertex_color_test.cpp)
target_link_libraries(mxh_stm_vertex_color_tests PRIVATE mxh_compat gtest_main)
gtest_add_tests(TARGET mxh_stm_vertex_color_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# 96-byte original pak header. Compiles pack_file.cpp directly so the
# target does not pull the rest of mxh_compat (DX11 / Win32).
add_executable(mxh_pack_file_header_tests
    pack_file_header_test.cpp
    ${CMAKE_SOURCE_DIR}/src/pack_file.cpp
)
target_include_directories(mxh_pack_file_header_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mxh_pack_file_header_tests PRIVATE gtest_main)
target_compile_features(mxh_pack_file_header_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_definitions(mxh_pack_file_header_tests PRIVATE
        _CRT_SECURE_NO_WARNINGS)
endif()
gtest_add_tests(TARGET mxh_pack_file_header_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    TEST_LIST mxh_pack_file_header_test_list)
set_tests_properties(${mxh_pack_file_header_test_list} PROPERTIES
    TIMEOUT 60)

# modern/tools/pak_writer.py output read back by PackFile. The fixture pak
# is generated from LCG bytes at build time; no original data is used.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(MXH_PAK_WRITER_FIXTURE ${CMAKE_CURRENT_BINARY_DIR}/pak_writer_fixture.pak)
add_custom_command(
    OUTPUT ${MXH_PAK_WRITER_FIXTURE}
    COMMAND Python3::Interpreter -B
            ${CMAKE_SOURCE_DIR}/tests/unit/tools/make_pak_writer_fixture.py
            ${MXH_PAK_WRITER_FIXTURE}
    DEPENDS ${CMAKE_SOURCE_DIR}/tests/unit/tools/make_pak_writer_fixture.py
            ${CMAKE_SOURCE_DIR}/tools/pak_writer.py
            ${CMAKE_SOURCE_DIR}/tools/unpack_pak.py
    VERBATIM)
add_custom_target(mxh_pak_writer_fixture DEPENDS ${MXH_PAK_WRITER_FIXTURE})
add_executable(mxh_pak_writer_packfile_tests
    pak_writer_packfile_test.cpp
    ${CMAKE_SOURCE_DIR}/src/pack_file.cpp
)
add_dependencies(mxh_pak_writer_packfile_tests mxh_pak_writer_fixture)
target_include_directories(mxh_pak_writer_packfile_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(mxh_pak_writer_packfile_tests PRIVATE
    MXH_PAK_WRITER_FIXTURE="${MXH_PAK_WRITER_FIXTURE}")
target_link_libraries(mxh_pak_writer_packfile_tests PRIVATE gtest_main)
target_compile_features(mxh_pak_writer_packfile_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_definitions(mxh_pak_writer_packfile_tests PRIVATE
        _CRT_SECURE_NO_WARNINGS)
endif()
gtest_add_tests(TARGET mxh_pak_writer_packfile_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    TEST_LIST mxh_pak_writer_packfile_test_list)
set_tests_properties(${mxh_pak_writer_packfile_test_list} PROPERTIES
    TIMEOUT 60)

# Index-only pak open. Compiles pack_file.cpp and filesystem_file_storage.cpp
# directly so the target does not link DirectX. Linux uses a tiny COM/Win32
# stub; that stub is not on the include path of any other target.
add_executable(mxh_pak_lazy_read_tests
    pak_lazy_read_test.cpp
    ${CMAKE_SOURCE_DIR}/src/pack_file.cpp
    ${CMAKE_SOURCE_DIR}/src/render/filesystem_file_storage.cpp
    ${CMAKE_SOURCE_DIR}/src/log/mlog.cpp
)
target_include_directories(mxh_pak_lazy_read_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_include_directories(mxh_pak_lazy_read_tests BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support/win32_stub)
endif()
target_link_libraries(mxh_pak_lazy_read_tests PRIVATE gtest_main)
target_compile_features(mxh_pak_lazy_read_tests PRIVATE cxx_std_20)
if(WIN32)
    target_compile_definitions(mxh_pak_lazy_read_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(mxh_pak_lazy_read_tests PRIVATE psapi)
endif()
gtest_add_tests(TARGET mxh_pak_lazy_read_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    TEST_LIST mxh_pak_lazy_read_test_list)
set_tests_properties(${mxh_pak_lazy_read_test_list} PROPERTIES
    TIMEOUT 180)


# -----------------------------------------------------------------------------
# mxh_crypto tests ????HSEL stream cipher (Phase 3.2)
# -----------------------------------------------------------------------------
add_executable(mxh_crypto_tests
    hsel_stream_test.cpp
    hsel_class_test.cpp
    hsel_encryptor_test.cpp
    hackshield_stub_test.cpp
    aes_gcm_test.cpp
)

target_link_libraries(mxh_crypto_tests PRIVATE mxh_crypto mxh_net gtest_main)

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
    mxh_services mxh_server gtest_main)
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
target_compile_definitions(mxh_tools_side_by_side_tests PRIVATE
    MXH_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
include(GoogleTest)
gtest_add_tests(TARGET mxh_tools_side_by_side_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_gm_security_tests gm_security_test.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianGMTool/gm_repository.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianGMTool/gm_item_catalog.cpp)
target_link_libraries(mxh_gm_security_tests PRIVATE gtest_main mxh_db mxh_game)
target_include_directories(mxh_gm_security_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/tools/MoxianGMTool)
target_compile_definitions(mxh_gm_security_tests PRIVATE
    MXH_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
gtest_add_tests(TARGET mxh_gm_security_tests)

add_executable(mxh_patch_security_tests patch_security_test.cpp)
target_link_libraries(mxh_patch_security_tests PRIVATE gtest_main)
if(WIN32)
    target_link_libraries(mxh_patch_security_tests PRIVATE bcrypt)
endif()
gtest_add_tests(TARGET mxh_patch_security_tests)

add_executable(mxh_sqlite_backup_script_tests sqlite_backup_script_test.cpp)
target_link_libraries(mxh_sqlite_backup_script_tests PRIVATE gtest_main)
target_compile_definitions(mxh_sqlite_backup_script_tests PRIVATE MXH_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
gtest_add_tests(TARGET mxh_sqlite_backup_script_tests)

# mxh_login_sbs_e2e_tests end-to-end smoke for side-by-side login path
add_executable(mxh_login_sbs_e2e_tests
    login_sbs_e2e_test.cpp
)
target_link_libraries(mxh_login_sbs_e2e_tests PRIVATE gtest_main)
target_include_directories(mxh_login_sbs_e2e_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
include(GoogleTest)
gtest_add_tests(TARGET mxh_login_sbs_e2e_tests)


# Phase 13.4 / M4 resource-coverage gate regression test.
# Pins the is_plaintext_ini() heuristic in
# modern/tools/audit_resource_coverage.py so the T1 coverage gate stays at
# 100% even when legacy ships additional plaintext-config .bin files.
add_test(NAME AuditResourceCoveragePlaintextDetection
    COMMAND python
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/audit_resource_coverage_plaintext_test.py
)
set_tests_properties(AuditResourceCoveragePlaintextDetection PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# -----------------------------------------------------------------------------
# M-R3: cDialogLoader — 装载 132 dialog .bin 链 (独立 target, 避免与
# cResourceManager_test 的 hand-rolled main 在 mxh_ui_tests 里冲突)
# -----------------------------------------------------------------------------
add_executable(mxh_dialog_loader_tests
    ui/cDialogLoader_test.cpp
)
target_link_libraries(mxh_dialog_loader_tests PRIVATE
    mxh_ui mxh_compat mxh_log)
if(MSVC)
    target_compile_options(mxh_dialog_loader_tests PRIVATE /FS /Z7 /MP1)
endif()
# hand-rolled main: 用 add_test 而不是 gtest_add_tests (后者需 TEST macro + main)
add_test(NAME mxh_dialog_loader_tests
    COMMAND $<TARGET_FILE:mxh_dialog_loader_tests>)
set_tests_properties(mxh_dialog_loader_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS "rog")

# -----------------------------------------------------------------------------
# M-R7 (G3): resolution_mode 头less 单测 — 独立 target, 跟 cDialogLoader
# 拆开因为 1:1 verify 不需要 LoadAll 全部 dialog (15.bin 一个 file 就够).
# 验证 7 个 sub-test (apply_legacy_layout 3/4 参数 + LoadOne 接受 mode
# + wm 模式切换 + detect/mode_size/mode_name 1:1). 42/42 PASS.
# -----------------------------------------------------------------------------
add_executable(mxh_resolution_mode_tests
    ui/resolution_mode_test.cpp
)
target_link_libraries(mxh_resolution_mode_tests PRIVATE
    mxh_ui mxh_compat mxh_log)
if(MSVC)
    target_compile_options(mxh_resolution_mode_tests PRIVATE /FS /Z7 /MP1)
endif()
add_test(NAME mxh_resolution_mode_tests
    COMMAND $<TARGET_FILE:mxh_resolution_mode_tests>)
set_tests_properties(mxh_resolution_mode_tests PROPERTIES
    LABELS "rog"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------------------------------------------------
# M-R6.2: cWindowManager focus chain 1:1 验证 (独立 target, 同样避免
# 跟 mxh_ui_tests 双 main 冲突)
# -----------------------------------------------------------------------------
add_executable(mxh_window_manager_focus_tests
    ui/cWindowManager_focus_test.cpp
)
target_link_libraries(mxh_window_manager_focus_tests PRIVATE
    mxh_ui mxh_compat mxh_log)
if(MSVC)
    target_compile_options(mxh_window_manager_focus_tests PRIVATE /FS /Z7 /MP1)
endif()
add_test(NAME mxh_window_manager_focus_tests
    COMMAND $<TARGET_FILE:mxh_window_manager_focus_tests>)
set_tests_properties(mxh_window_manager_focus_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------------------------------------------------
# 2026-09-07 visual polish — pak_extract (unpack_pak.py) Python unit tests.
# Self-contained: synthesizes a minimal .pak in a temp directory and exercises
# the parser end-to-end, so it does not depend on any real PlayDH resource.
# -----------------------------------------------------------------------------
add_subdirectory(tools)
