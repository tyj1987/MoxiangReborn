cmake_minimum_required(VERSION 3.24)
project(MoxiangPickupWireTests LANGUAGES C CXX)
enable_testing()
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(../../../third_party/googletest-src googletest EXCLUDE_FROM_ALL)
add_executable(mxh_pickup_wire_tests ../pickup_wire_test.cpp ../../../tests/unit/server/item_id_test.cpp)
target_compile_features(mxh_pickup_wire_tests PRIVATE cxx_std_20)
target_include_directories(mxh_pickup_wire_tests PRIVATE ../../../src/unity_core ../../../include)
target_link_libraries(mxh_pickup_wire_tests PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(mxh_pickup_wire_tests)

add_executable(mxh_synthetic_server_resources_tests
    ../../../tests/unit/server/synthetic_server_resources_test.cpp
    ../../../src/server/ai_group_loader.cpp
    ../../../src/mh_file_ex.cpp)
target_compile_features(mxh_synthetic_server_resources_tests PRIVATE cxx_std_20)
target_include_directories(mxh_synthetic_server_resources_tests PRIVATE ../../../include)
target_link_libraries(mxh_synthetic_server_resources_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(mxh_synthetic_server_resources_tests)

add_executable(mxh_base_attack_power_tests
    ../../../tests/unit/game/base_attack_power_test.cpp)
target_compile_features(mxh_base_attack_power_tests PRIVATE cxx_std_20)
target_include_directories(mxh_base_attack_power_tests PRIVATE ../../../include)
target_link_libraries(mxh_base_attack_power_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(mxh_base_attack_power_tests)

add_library(mxh_audit_sqlite STATIC ../../../third_party/sqlite/sqlite3.c)
target_include_directories(mxh_audit_sqlite PUBLIC ../../../third_party/sqlite)
add_executable(mxh_equipment_stats_tests
    ../../../tests/unit/server/player_equipment_stats_test.cpp
    ../../../tests/unit/server/player_state_test.cpp
    ../../../tests/unit/server/character_calc_manager_test.cpp
    ../../../tests/unit/server/skill_caster_test.cpp
    ../../../tests/unit/db/schema_migration_test.cpp
    ../../../src/server/player_state.cpp
    ../../../src/server/character_calc_manager.cpp
    ../../../src/server/skill_caster.cpp
    ../../../src/game/item_list_parser.cpp
    ../../../src/mh_file_ex.cpp
    ../../../src/schema_migration.cpp
    ../../../src/sqlite_adapter.cpp
    ../../../src/db_adapter.cpp)
target_compile_features(mxh_equipment_stats_tests PRIVATE cxx_std_20)
target_compile_definitions(mxh_equipment_stats_tests PRIVATE MXH_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../..")
target_include_directories(mxh_equipment_stats_tests PRIVATE ../../../include)
target_link_libraries(mxh_equipment_stats_tests PRIVATE GTest::gtest_main mxh_audit_sqlite)
gtest_discover_tests(mxh_equipment_stats_tests)
