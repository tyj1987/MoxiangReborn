add_executable(mxh_game_tests
    game_types_test.cpp
    skill_manager_test.cpp
    skill_list_parser_test.cpp
    item_list_parser_test.cpp
    item_manager_test.cpp
    battle_factory_test.cpp
    battle_runtime_test.cpp
    attack_loop_test.cpp
    experience_curve_test.cpp
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
