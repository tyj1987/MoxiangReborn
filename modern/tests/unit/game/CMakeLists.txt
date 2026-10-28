add_executable(mxh_game_tests
    game_types_test.cpp
    skill_manager_test.cpp
    skill_list_parser_test.cpp
    item_list_parser_test.cpp
    item_manager_test.cpp
    avatar_calc_test.cpp
    battle_factory_test.cpp
    base_attack_power_test.cpp
    battle_runtime_test.cpp
    attack_loop_test.cpp
    experience_curve_test.cpp
    hero_total_layout_test.cpp
    effect_catalog_test.cpp
    effect_timeline_test.cpp
)
target_include_directories(mxh_game_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(mxh_game_tests
    PRIVATE
    mxh_game
    gtest_main
)
if(MSVC)
    target_compile_options(mxh_game_tests PRIVATE /W3 /utf-8)
endif()

include(GoogleTest)
gtest_add_tests(TARGET mxh_game_tests)

# Linux-buildable half of the TYJ-156 Map10 repro. mxh_server (MapHandler)
# does not compile on Linux: crypto.cpp includes windows.h and the net
# library is IOCP. This target links the damage formula plus the same
# shield-before-life step MapHandler::apply_monster_damage uses.
add_executable(mxh_map10_first_combat_tests
    map10_first_combat_damage_test.cpp
    ${CMAKE_SOURCE_DIR}/src/server/skill_caster.cpp
)
target_include_directories(mxh_map10_first_combat_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(mxh_map10_first_combat_tests
    PRIVATE
    mxh_game
    gtest_main
)
if(MSVC)
    target_compile_options(mxh_map10_first_combat_tests PRIVATE /W3 /utf-8)
endif()
gtest_add_tests(TARGET mxh_map10_first_combat_tests)



add_executable(mxh_attack_calc_tests
    attack_calc_test.cpp
)
target_include_directories(mxh_attack_calc_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(mxh_attack_calc_tests
    PRIVATE
    mxh_game
    gtest_main
)
if(MSVC)
    target_compile_options(mxh_attack_calc_tests PRIVATE /W3 /utf-8)
endif()

include(GoogleTest)
gtest_add_tests(TARGET mxh_attack_calc_tests)
