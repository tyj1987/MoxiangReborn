# Server handler tests - Phase 10.17
# Smoke tests for the 3 game-server handlers (Login / Agent / Map).

add_executable(mxh_server_handler_tests
    server_handler_test.cpp
)

target_link_libraries(mxh_server_handler_tests PRIVATE
    mxh_server mxh_db gtest_main
)

target_include_directories(mxh_server_handler_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_add_tests(TARGET mxh_server_handler_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_character_calc_manager_tests
    character_calc_manager_test.cpp
)
target_link_libraries(mxh_character_calc_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_character_calc_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_character_calc_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_player_state_tests
    player_state_test.cpp
)
target_link_libraries(mxh_player_state_tests PRIVATE
    mxh_server mxh_game gtest_main
)
target_include_directories(mxh_player_state_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_player_state_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_player_tests
    player_test.cpp
)
target_link_libraries(mxh_player_tests PRIVATE
    mxh_server mxh_game gtest_main
)
target_include_directories(mxh_player_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_player_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_monster_tests
    monster_test.cpp
)
target_link_libraries(mxh_monster_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_monster_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_monster_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)


add_executable(mxh_monster_ext_tests
    monster_ext_test.cpp
)
target_link_libraries(mxh_monster_ext_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_monster_ext_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_monster_ext_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)














add_executable(mxh_money_manager_tests
    money_manager_test.cpp
)
target_link_libraries(mxh_money_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_money_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_money_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_pk_manager_tests
    pk_manager_test.cpp
)
target_link_libraries(mxh_pk_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_pk_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_pk_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_showdown_manager_tests
    showdown_manager_test.cpp
)
target_link_libraries(mxh_showdown_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_showdown_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_showdown_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_distribute_network_msg_parser_tests
    distribute_network_msg_parser_test.cpp
)
target_link_libraries(mxh_distribute_network_msg_parser_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_distribute_network_msg_parser_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_distribute_network_msg_parser_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_server_list_tests
    server_list_test.cpp
)
target_link_libraries(mxh_server_list_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_server_list_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_server_list_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_login_server_db_msg_parser_tests
    login_server_db_msg_parser_test.cpp
)
target_link_libraries(mxh_login_server_db_msg_parser_tests PRIVATE
    mxh_server mxh_db gtest_main
)
target_include_directories(mxh_login_server_db_msg_parser_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_login_server_db_msg_parser_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_mn_server_tests
    mn_server_test.cpp
)
target_link_libraries(mxh_mn_server_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_mn_server_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_mn_server_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_distribute_server_tests
    distribute_server_test.cpp
)
target_link_libraries(mxh_distribute_server_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_distribute_server_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_distribute_server_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_murimnet_channelling_manager_tests
    murimnet_channelling_manager_test.cpp
)
target_link_libraries(mxh_murimnet_channelling_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_murimnet_channelling_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_murimnet_channelling_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_wanted_manager_tests
    wanted_manager_test.cpp
)
target_link_libraries(mxh_wanted_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_wanted_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_wanted_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_note_manager_tests
    note_manager_test.cpp
)
target_link_libraries(mxh_note_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_note_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_note_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_ability_group_tests
    ability_group_test.cpp
)
target_link_libraries(mxh_ability_group_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_ability_group_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_ability_group_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_item_container_tests
    item_container_test.cpp
)
target_link_libraries(mxh_item_container_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_item_container_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_item_container_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_item_slot_tests
    item_slot_test.cpp
)
target_link_libraries(mxh_item_slot_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_item_slot_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_item_slot_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_skill_manager_tests
    skill_manager_test.cpp
)
target_link_libraries(mxh_skill_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_skill_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_skill_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_quest_manager_tests
    quest_manager_test.cpp
)
target_link_libraries(mxh_quest_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_quest_execute_tests
    quest_execute_test.cpp
)
target_link_libraries(mxh_quest_execute_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_execute_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_execute_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_quest_script_line_tests
    quest_script_line_test.cpp
)
target_link_libraries(mxh_quest_script_line_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_script_line_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_script_line_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_quest_subquest_block_tests
    quest_subquest_block_test.cpp
)
target_link_libraries(mxh_quest_subquest_block_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_subquest_block_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_subquest_block_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_quest_trigger_tests
    quest_trigger_test.cpp
)
target_link_libraries(mxh_quest_trigger_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_trigger_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_trigger_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)



add_executable(mxh_item_manager_tests
    item_manager_test.cpp
)
target_link_libraries(mxh_item_manager_tests PRIVATE
    mxh_server mxh_game gtest_main
)
target_include_directories(mxh_item_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_item_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_party_manager_tests
    party_manager_test.cpp
)
target_link_libraries(mxh_party_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_party_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_party_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_guild_manager_tests
    guild_manager_test.cpp
)
target_link_libraries(mxh_guild_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_guild_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_guild_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_pet_manager_tests
    pet_manager_test.cpp
)
target_link_libraries(mxh_pet_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_pet_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_pet_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_exchange_manager_tests
    exchange_manager_test.cpp
)
target_link_libraries(mxh_exchange_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_exchange_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_exchange_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_titan_item_manager_tests
    titan_item_manager_test.cpp
)
target_link_libraries(mxh_titan_item_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_titan_item_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_titan_item_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_titan_manager_tests
    titan_manager_test.cpp
)
target_link_libraries(mxh_titan_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_titan_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_titan_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_street_stall_manager_tests
    street_stall_manager_test.cpp
)
target_link_libraries(mxh_street_stall_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_street_stall_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_street_stall_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_pyo_guk_manager_tests
    pyo_guk_manager_test.cpp
)
target_link_libraries(mxh_pyo_guk_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_pyo_guk_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_pyo_guk_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_friend_manager_tests
    friend_manager_test.cpp
)
target_link_libraries(mxh_friend_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_friend_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_friend_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_bobusang_manager_tests
    bobusang_manager_test.cpp
)
target_link_libraries(mxh_bobusang_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_bobusang_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_bobusang_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_survival_mode_manager_tests
    survival_mode_manager_test.cpp
)
target_link_libraries(mxh_survival_mode_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_survival_mode_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_survival_mode_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_regen_manager_tests
    regen_manager_test.cpp
)
target_link_libraries(mxh_regen_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_regen_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_regen_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_party_war_mgr_tests
    party_war_mgr_test.cpp
)
target_link_libraries(mxh_party_war_mgr_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_party_war_mgr_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_party_war_mgr_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_distributer_tests
    distributer_test.cpp
)
target_link_libraries(mxh_distributer_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_distributer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_distributer_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_quest_regen_mgr_tests
    quest_regen_mgr_test.cpp
)
target_link_libraries(mxh_quest_regen_mgr_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_regen_mgr_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_regen_mgr_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_looting_manager_tests
    looting_manager_test.cpp
)
target_link_libraries(mxh_looting_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_looting_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_looting_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_distribute_strategy_tests
    distribute_strategy_test.cpp
)
target_link_libraries(mxh_distribute_strategy_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_distribute_strategy_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_distribute_strategy_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_item_drop_tests
    item_drop_test.cpp
)
target_link_libraries(mxh_item_drop_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_item_drop_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_item_drop_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_drop_item_tests
    drop_item_test.cpp
)
target_link_libraries(mxh_drop_item_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_drop_item_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_drop_item_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_item_limit_manager_tests
    item_limit_manager_test.cpp
)
target_link_libraries(mxh_item_limit_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_item_limit_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_item_limit_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_change_item_manager_tests
    change_item_manager_test.cpp
)
target_link_libraries(mxh_change_item_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_change_item_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_change_item_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_quest_map_mgr_tests
    quest_map_mgr_test.cpp
)
target_link_libraries(mxh_quest_map_mgr_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_map_mgr_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_map_mgr_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_quest_updater_tests
    quest_updater_test.cpp
)
target_link_libraries(mxh_quest_updater_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_updater_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_updater_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_cquest_base_tests
    cquest_base_test.cpp
)
target_link_libraries(mxh_cquest_base_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_cquest_base_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_cquest_base_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_quest_group_tests
    quest_group_test.cpp
)
target_link_libraries(mxh_quest_group_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_quest_group_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_quest_group_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_user_table_tests
    user_table_test.cpp
)
target_link_libraries(mxh_user_table_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_user_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_user_table_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_help_request_manager_tests help_request_manager_test.cpp)
target_link_libraries(mxh_help_request_manager_tests PRIVATE mxh_server gtest_main)
target_include_directories(mxh_help_request_manager_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
include(GoogleTest)
gtest_add_tests(TARGET mxh_help_request_manager_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit PRE_SHIP_BUILD)

add_executable(mxh_server_table_tests
    server_table_test.cpp
)
target_link_libraries(mxh_server_table_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_server_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_server_table_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)



add_executable(mxh_filtering_table_tests
    filtering_table_test.cpp
)
target_link_libraries(mxh_filtering_table_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_filtering_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_filtering_table_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_skill_delay_manager_tests
    skill_delay_manager_test.cpp
)
target_link_libraries(mxh_skill_delay_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_skill_delay_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_skill_delay_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_msg_table_tests
    msg_table_test.cpp
)
target_link_libraries(mxh_msg_table_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_msg_table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_msg_table_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_punish_manager_tests
    punish_manager_test.cpp
)
target_link_libraries(mxh_punish_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_punish_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_punish_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_shout_manager_tests
    shout_manager_test.cpp
)
target_link_libraries(mxh_shout_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_shout_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_shout_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_gm_power_list_tests
    gm_power_list_test.cpp
)
target_link_libraries(mxh_gm_power_list_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_gm_power_list_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_gm_power_list_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_hackshield_manager_tests
    hackshield_manager_test.cpp
)
target_link_libraries(mxh_hackshield_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_hackshield_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_hackshield_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_nprotect_manager_tests
    nprotect_manager_test.cpp
)
target_link_libraries(mxh_nprotect_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_nprotect_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_nprotect_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_bobusang_manager_agent_tests
    bobusang_manager_agent_test.cpp
)
target_link_libraries(mxh_bobusang_manager_agent_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_bobusang_manager_agent_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_bobusang_manager_agent_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_server_system_tests
    server_system_test.cpp
)
target_link_libraries(mxh_server_system_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_server_system_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_network_msg_parser_tests
    agent_network_msg_parser_test.cpp
)
target_link_libraries(mxh_agent_network_msg_parser_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_network_msg_parser_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_db_msg_parser_tests
    agent_db_msg_parser_test.cpp
)
target_link_libraries(mxh_agent_db_msg_parser_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_db_msg_parser_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_mh_time_manager_tests
    mh_time_manager_test.cpp
)
target_link_libraries(mxh_mh_time_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_mh_time_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_common_network_msg_parser_tests
    common_network_msg_parser_test.cpp
)
target_link_libraries(mxh_common_network_msg_parser_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_common_network_msg_parser_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_crypt_tests
    crypt_test.cpp
)
target_link_libraries(mxh_crypt_tests PRIVATE mxh_server mxh_crypto GTest::gtest_main)
gtest_add_tests(TARGET mxh_crypt_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_traffic_log_tests
    traffic_log_test.cpp
)
target_link_libraries(mxh_traffic_log_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_traffic_log_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_plustime_manager_tests
    plustime_manager_test.cpp
)
target_link_libraries(mxh_plustime_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_plustime_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_jackpot_manager_agent_tests
    jackpot_manager_agent_test.cpp
)
target_link_libraries(mxh_jackpot_manager_agent_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_jackpot_manager_agent_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_gm_tool_manager_tests
    gm_tool_manager_test.cpp
)
target_link_libraries(mxh_gm_tool_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_gm_tool_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_attack_manager_tests
    attack_manager_test.cpp
)
target_link_libraries(mxh_attack_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_attack_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_object_event_tests
    object_event_test.cpp
)
target_link_libraries(mxh_object_event_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_object_event_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_object_state_manager_tests
    object_state_manager_test.cpp
)
target_link_libraries(mxh_object_state_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_object_state_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_mh_file_reader_tests
    mh_file_reader_test.cpp
)
target_link_libraries(mxh_mh_file_reader_tests PRIVATE mxh_server mxh_compat GTest::gtest_main)
gtest_add_tests(TARGET mxh_mh_file_reader_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_object_factory_tests
    object_factory_test.cpp
)
target_link_libraries(mxh_object_factory_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_object_factory_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)




add_executable(mxh_ai_group_loader_tests
    ai_group_loader_test.cpp
)
target_link_libraries(mxh_ai_group_loader_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_ai_group_loader_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_ai_system_tests
    ai_system_test.cpp
)
target_link_libraries(mxh_ai_system_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_ai_system_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_map_object_tests
    map_object_test.cpp
)
target_link_libraries(mxh_map_object_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_map_object_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_object_tests
    object_test.cpp
)
target_link_libraries(mxh_object_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_object_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_ai_define_tests
    ai_define_test.cpp
)
target_link_libraries(mxh_ai_define_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_ai_define_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_ai_param_tests
    ai_param_test.cpp
)
target_link_libraries(mxh_ai_param_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_ai_param_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_chat_tests
    agent_chat_test.cpp
)
target_link_libraries(mxh_agent_chat_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_chat_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_skill_tests
    agent_skill_test.cpp
)
target_link_libraries(mxh_agent_skill_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_skill_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_ai_group_prototype_tests
    ai_group_prototype_test.cpp
)
target_link_libraries(mxh_ai_group_prototype_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_ai_group_prototype_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_guild_tests
    agent_guild_test.cpp
)
target_link_libraries(mxh_agent_guild_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_guild_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_murimnet_tests
    agent_murimnet_test.cpp
)
target_link_libraries(mxh_agent_murimnet_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_murimnet_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_protocol_tests
    murimnet_protocol_test.cpp
)
target_link_libraries(mxh_murimnet_protocol_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_protocol_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_economy_tests
    economy_test.cpp
)
target_link_libraries(mxh_economy_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_economy_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_mh_error_tests
    mh_error_test.cpp
)
target_link_libraries(mxh_mh_error_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_mh_error_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_fame_manager_tests
    fame_manager_test.cpp
)
target_link_libraries(mxh_fame_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_fame_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_party_tests
    agent_party_test.cpp
)
target_link_libraries(mxh_agent_party_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_party_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_purse_tests
    purse_test.cpp
)
target_link_libraries(mxh_purse_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_purse_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_item_tests
    agent_item_test.cpp
)
target_link_libraries(mxh_agent_item_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_item_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_regen_condition_info_tests
    regen_condition_info_test.cpp
)
target_link_libraries(mxh_regen_condition_info_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_regen_condition_info_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_regen_prototype_tests
    regen_prototype_test.cpp
)
target_link_libraries(mxh_regen_prototype_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_regen_prototype_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_state_param_tests
    state_param_test.cpp
)
target_link_libraries(mxh_state_param_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_state_param_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_packed_tests
    agent_packed_test.cpp
)
target_link_libraries(mxh_agent_packed_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_packed_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_summon_monster_tests
    summon_monster_test.cpp
)
target_link_libraries(mxh_summon_monster_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_summon_monster_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_note_tests
    agent_note_test.cpp
)
target_link_libraries(mxh_agent_note_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_note_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_friend_tests
    agent_friend_test.cpp
)
target_link_libraries(mxh_agent_friend_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_friend_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_fixed_tile_tests
    fixed_tile_test.cpp
)
target_link_libraries(mxh_fixed_tile_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_fixed_tile_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_autonote_tests
    agent_autonote_test.cpp
)
target_link_libraries(mxh_agent_autonote_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_autonote_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_fixed_tile_info_tests
    fixed_tile_info_test.cpp
)
target_link_libraries(mxh_fixed_tile_info_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_fixed_tile_info_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_tile_tests
    tile_test.cpp
)
target_link_libraries(mxh_tile_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_tile_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_weather_tests
    agent_weather_test.cpp
)
target_link_libraries(mxh_agent_weather_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_weather_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_tactic_ability_info_tests
    tactic_ability_info_test.cpp
)
target_link_libraries(mxh_tactic_ability_info_tests PRIVATE mxh_server GTest::gtest_main)

add_executable(mxh_agent_userconn_tests
    agent_userconn_test.cpp
)
target_link_libraries(mxh_agent_userconn_tests PRIVATE mxh_server GTest::gtest_main)
add_executable(mxh_agent_battle_tests
    agent_battle_test.cpp
)
target_link_libraries(mxh_agent_battle_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_battle_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_quest_tests
    agent_quest_test.cpp
)
target_link_libraries(mxh_agent_quest_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_quest_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_mugong_tests
    agent_mugong_test.cpp
)
target_link_libraries(mxh_agent_mugong_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_mugong_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_move_tests
    agent_move_test.cpp
)
target_link_libraries(mxh_agent_move_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_move_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_user_tests
    agent_user_test.cpp
)
target_link_libraries(mxh_agent_user_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_user_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
gtest_add_tests(TARGET mxh_agent_userconn_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_powerup_tests
    agent_powerup_test.cpp
)
target_link_libraries(mxh_agent_powerup_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_powerup_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_cheat_tests
    agent_cheat_test.cpp
)
target_link_libraries(mxh_agent_cheat_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_cheat_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
add_executable(mxh_agent_gtournament_server_tests
    agent_gtournament_server_test.cpp
)
target_link_libraries(mxh_agent_gtournament_server_tests PRIVATE mxh_server GTest::gtest_main)

add_executable(mxh_protocol_constants_sweep_tests
    protocol_constants_sweep_test.cpp
)
target_link_libraries(mxh_protocol_constants_sweep_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_protocol_constants_sweep_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_constants_sweep_tests
    agent_constants_sweep_test.cpp
)
target_link_libraries(mxh_agent_constants_sweep_tests PRIVATE mxh_server GTest::gtest_main)
add_executable(mxh_bonus_constants_sweep_tests
    bonus_constants_sweep_test.cpp
)
target_link_libraries(mxh_bonus_constants_sweep_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME Skill_types.BonusSKILLMAXNAMEIs17 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Skill_types.BonusSKILLMAXNAMEIs17)
add_test(NAME Skill_types.BonusSKILLMAXLEVELIs12 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Skill_types.BonusSKILLMAXLEVELIs12)
add_test(NAME Negotiate.BonusKHandshakeRequestSizeIs12 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Negotiate.BonusKHandshakeRequestSizeIs12)
add_test(NAME Negotiate.BonusKHandshakeResponseHeaderSizeIs5 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Negotiate.BonusKHandshakeResponseHeaderSizeIs5)
add_test(NAME Negotiate.BonusKMagicByte0Is77 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Negotiate.BonusKMagicByte0Is77)
add_test(NAME Negotiate.BonusKMagicByte1Is88 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Negotiate.BonusKMagicByte1Is88)
add_test(NAME Negotiate.BonusKMagicByte2Is72 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Negotiate.BonusKMagicByte2Is72)
add_test(NAME Negotiate.BonusKMagicByte3Is78 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=Negotiate.BonusKMagicByte3Is78)
add_test(NAME VersionCheck.MxhVersionMatches COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=VersionCheck.MxhVersionMatches)
add_test(NAME VersionCheck.MxhModernProtocolIsV1 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=VersionCheck.MxhModernProtocolIsV1)
add_test(NAME VersionCheck.MxhLegacyProtocolIsV0 COMMAND mxh_bonus_constants_sweep_tests --gtest_filter=VersionCheck.MxhLegacyProtocolIsV0)
add_executable(mxh_server_misc_constants_sweep_tests
    server_misc_constants_sweep_test.cpp
)
target_link_libraries(mxh_server_misc_constants_sweep_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME Negotiate.KProtocolVersionCurrentIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Negotiate.KProtocolVersionCurrentIs1)
add_test(NAME Negotiate.KProtocolVersionLegacyIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Negotiate.KProtocolVersionLegacyIs0)
add_test(NAME Negotiate.KAckSuccessIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Negotiate.KAckSuccessIs1)
add_test(NAME Negotiate.KAckFailIs255 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Negotiate.KAckFailIs255)
add_test(NAME Cheat.CheatCategoryIs11 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatCategoryIs11)
add_test(NAME Cheat.CheatUserLevelGmIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatUserLevelGmIs1)
add_test(NAME Cheat.CheatUserLevelProgrammerIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatUserLevelProgrammerIs2)
add_test(NAME Cheat.CheatUserLevelDeveloperIs3 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatUserLevelDeveloperIs3)
add_test(NAME Cheat.CheatUserLevelMonitorIs4 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatUserLevelMonitorIs4)
add_test(NAME Cheat.CheatUserLevelSuperuserIs5 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatUserLevelSuperuserIs5)
add_test(NAME Cheat.CheatGmLoginSynIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatGmLoginSynIs0)
add_test(NAME Cheat.CheatChangemapSynIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatChangemapSynIs1)
add_test(NAME Cheat.CheatChangemapNackIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatChangemapNackIs2)
add_test(NAME Cheat.CheatChangemapAckIs3 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatChangemapAckIs3)
add_test(NAME Cheat.CheatBancharacterSynIs4 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBancharacterSynIs4)
add_test(NAME Cheat.CheatBancharacterNackIs5 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBancharacterNackIs5)
add_test(NAME Cheat.CheatBlockcharacterSynIs6 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBlockcharacterSynIs6)
add_test(NAME Cheat.CheatWhereisSynIs7 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatWhereisSynIs7)
add_test(NAME Cheat.CheatEventMonsterRegenIs8 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventMonsterRegenIs8)
add_test(NAME Cheat.CheatEventMonsterDeleteIs9 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventMonsterDeleteIs9)
add_test(NAME Cheat.CheatBanmapSynIs10 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBanmapSynIs10)
add_test(NAME Cheat.CheatAgentcheckSynIs11 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatAgentcheckSynIs11)
add_test(NAME Cheat.CheatPkallowSynIs12 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatPkallowSynIs12)
add_test(NAME Cheat.CheatNoticeSynIs13 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatNoticeSynIs13)
add_test(NAME Cheat.CheatAbilityexpSynIs14 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatAbilityexpSynIs14)
add_test(NAME Cheat.CheatAddmugongSynIs15 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatAddmugongSynIs15)
add_test(NAME Cheat.CheatMugongsungSynIs16 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatMugongsungSynIs16)
add_test(NAME Cheat.CheatItemSynIs17 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatItemSynIs17)
add_test(NAME Cheat.CheatItemOptionSynIs18 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatItemOptionSynIs18)
add_test(NAME Cheat.CheatMoneySynIs19 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatMoneySynIs19)
add_test(NAME Cheat.CheatEventSynIs20 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventSynIs20)
add_test(NAME Cheat.CheatEventnotifyOnIs21 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventnotifyOnIs21)
add_test(NAME Cheat.CheatPlustimeOnIs22 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatPlustimeOnIs22)
add_test(NAME Cheat.CheatEventnotifyOffIs23 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventnotifyOffIs23)
add_test(NAME Cheat.CheatPlustimeAlloffIs24 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatPlustimeAlloffIs24)
add_test(NAME Cheat.CheatChangeEventmapSynIs25 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatChangeEventmapSynIs25)
add_test(NAME Cheat.CheatEventStartSynIs26 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventStartSynIs26)
add_test(NAME Cheat.CheatEventReadySynIs27 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatEventReadySynIs27)
add_test(NAME Cheat.CheatPetStaminaIs28 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatPetStaminaIs28)
add_test(NAME Cheat.CheatPetFriendshipSynIs29 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatPetFriendshipSynIs29)
add_test(NAME Cheat.CheatPetSelectedFriendshipSynIs30 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatPetSelectedFriendshipSynIs30)
add_test(NAME Cheat.CheatGuildpointSynIs31 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatGuildpointSynIs31)
add_test(NAME Cheat.CheatGuildhuntedMonstercountSynIs32 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatGuildhuntedMonstercountSynIs32)
add_test(NAME Cheat.CheatMussangReadyIs33 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatMussangReadyIs33)
add_test(NAME Cheat.CheatJackpotGetprizeIs34 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatJackpotGetprizeIs34)
add_test(NAME Cheat.CheatJackpotMoneypermonsterIs35 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatJackpotMoneypermonsterIs35)
add_test(NAME Cheat.CheatJackpotOnoffIs36 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatJackpotOnoffIs36)
add_test(NAME Cheat.CheatJackpotProbabilityIs37 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatJackpotProbabilityIs37)
add_test(NAME Cheat.CheatJackpotControlIs38 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatJackpotControlIs38)
add_test(NAME Cheat.CheatBobusanginfoRequestSynIs39 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBobusanginfoRequestSynIs39)
add_test(NAME Cheat.CheatBobusangLeaveSynIs40 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBobusangLeaveSynIs40)
add_test(NAME Cheat.CheatBobusanginfoChangeSynIs41 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBobusanginfoChangeSynIs41)
add_test(NAME Cheat.CheatItemlimitSynIs42 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatItemlimitSynIs42)
add_test(NAME Cheat.CheatAutonoteSettingSynIs43 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatAutonoteSettingSynIs43)
add_test(NAME Cheat.CheatBancharacterAckIs50 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatBancharacterAckIs50)
add_test(NAME Cheat.CheatWhereisAckIs52 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatWhereisAckIs52)
add_test(NAME Cheat.CheatDamageSynIs44 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatDamageSynIs44)
add_test(NAME Cheat.CheatDamageAckIs45 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatDamageAckIs45)
add_test(NAME Cheat.CheatDamageNackIs46 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatDamageNackIs46)
add_test(NAME Cheat.CheatMapConditionIs47 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatMapConditionIs47)
add_test(NAME Cheat.CheatAgentConditionIs48 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatAgentConditionIs48)
add_test(NAME Cheat.CheatTitanFuelSpellMaxSynIs49 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Cheat.CheatTitanFuelSpellMaxSynIs49)
add_test(NAME Gtournament_server.GtoournamentCategoryIs60 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtoournamentCategoryIs60)
add_test(NAME Gtournament_server.GtournamentMovetobattlemapSynIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentMovetobattlemapSynIs0)
add_test(NAME Gtournament_server.GtournamentMovetobattlemapNackIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentMovetobattlemapNackIs1)
add_test(NAME Gtournament_server.GtournamentStandinginfoSynIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentStandinginfoSynIs2)
add_test(NAME Gtournament_server.GtournamentStandinginfoNackIs3 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentStandinginfoNackIs3)
add_test(NAME Gtournament_server.GtournamentBattlejoinSynIs4 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentBattlejoinSynIs4)
add_test(NAME Gtournament_server.GtournamentObserverjoinSynIs5 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentObserverjoinSynIs5)
add_test(NAME Gtournament_server.GtournamentBattlejoinNackIs6 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentBattlejoinNackIs6)
add_test(NAME Gtournament_server.GtournamentLeaveSynIs7 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentLeaveSynIs7)
add_test(NAME Gtournament_server.GtournamentCheatIs8 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentCheatIs8)
add_test(NAME Gtournament_server.GtournamentEventStartIs9 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentEventStartIs9)
add_test(NAME Gtournament_server.GtournamentEventEndIs10 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentEventEndIs10)
add_test(NAME Gtournament_server.GtournamentStandinginfoRegistedIs11 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentStandinginfoRegistedIs11)
add_test(NAME Gtournament_server.GtournamentReturntomapIs12 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentReturntomapIs12)
add_test(NAME Gtournament_server.GtournamentNotifyWinloseIs13 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentNotifyWinloseIs13)
add_test(NAME Gtournament_server.GtournamentMapNumIs60 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentMapNumIs60)
add_test(NAME Gtournament_server.GtournamentUserLevelMaxForEventStartIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentUserLevelMaxForEventStartIs1)
add_test(NAME Gtournament_server.GtournamentErrorIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Gtournament_server.GtournamentErrorIs0)
add_test(NAME Powerup.PowerupCategoryIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupCategoryIs2)
add_test(NAME Powerup.PowerupBootingNotifyIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupBootingNotifyIs0)
add_test(NAME Powerup.PowerupBootlistSynIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupBootlistSynIs1)
add_test(NAME Powerup.PowerupBootlistAckIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupBootlistAckIs2)
add_test(NAME Powerup.PowerupConnectSynIs3 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupConnectSynIs3)
add_test(NAME Powerup.PowerupConnectAckIs4 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupConnectAckIs4)
add_test(NAME Powerup.PowerupMaxAgentServersIs100 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Powerup.PowerupMaxAgentServersIs100)
add_test(NAME Userconn.UserconnCategoryIs7 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCategoryIs7)
add_test(NAME Userconn.UserconnDistConnectsuccessIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDistConnectsuccessIs0)
add_test(NAME Userconn.UserconnLoginSynIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginSynIs1)
add_test(NAME Userconn.UserconnLoginAckIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginAckIs2)
add_test(NAME Userconn.UserconnLoginNackIs3 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginNackIs3)
add_test(NAME Userconn.UserconnNotifyUserloginSynIs4 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNotifyUserloginSynIs4)
add_test(NAME Userconn.UserconnNotifyUserloginAckIs5 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNotifyUserloginAckIs5)
add_test(NAME Userconn.UserconnNotifyUserloginNackIs6 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNotifyUserloginNackIs6)
add_test(NAME Userconn.UserconnNotifyOverlappedloginIs7 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNotifyOverlappedloginIs7)
add_test(NAME Userconn.UserconnAgentConnectsuccessIs8 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnAgentConnectsuccessIs8)
add_test(NAME Userconn.UserconnCharacterlistSynIs9 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterlistSynIs9)
add_test(NAME Userconn.UserconnDirectcharacterlistSynIs10 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDirectcharacterlistSynIs10)
add_test(NAME Userconn.UserconnCharacterlistNackIs11 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterlistNackIs11)
add_test(NAME Userconn.UserconnCharacterlistAckIs12 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterlistAckIs12)
add_test(NAME Userconn.UserconnDisconnectSynIs13 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDisconnectSynIs13)
add_test(NAME Userconn.UserconnDisconnectAckIs14 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDisconnectAckIs14)
add_test(NAME Userconn.UserconnDisconnectNackIs15 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDisconnectNackIs15)
add_test(NAME Userconn.UserconnCharacterselectSynIs16 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterselectSynIs16)
add_test(NAME Userconn.UserconnCharacterselectAckIs17 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterselectAckIs17)
add_test(NAME Userconn.UserconnCharacterselectNackIs18 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterselectNackIs18)
add_test(NAME Userconn.UserconnCharacterNamecheckSynIs19 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterNamecheckSynIs19)
add_test(NAME Userconn.UserconnCharacterNamecheckAckIs20 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterNamecheckAckIs20)
add_test(NAME Userconn.UserconnCharacterNamecheckNackIs21 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterNamecheckNackIs21)
add_test(NAME Userconn.UserconnCharacterMakeSynIs22 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterMakeSynIs22)
add_test(NAME Userconn.UserconnCharacterMakeAckIs23 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterMakeAckIs23)
add_test(NAME Userconn.UserconnCharacterMakeNackIs24 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterMakeNackIs24)
add_test(NAME Userconn.UserconnCharacterInfoSynIs25 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterInfoSynIs25)
add_test(NAME Userconn.UserconnCharacterInfoAckIs26 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterInfoAckIs26)
add_test(NAME Userconn.UserconnCharacterInfoNackIs27 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterInfoNackIs27)
add_test(NAME Userconn.UserconnGameinSynIs28 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinSynIs28)
add_test(NAME Userconn.UserconnGameinAckIs29 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinAckIs29)
add_test(NAME Userconn.UserconnGameinNackIs30 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinNackIs30)
add_test(NAME Userconn.UserconnGameoutSynIs31 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameoutSynIs31)
add_test(NAME Userconn.UserconnGameoutAckIs32 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameoutAckIs32)
add_test(NAME Userconn.UserconnGameoutNackIs33 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameoutNackIs33)
add_test(NAME Userconn.UserconnDisconnectedIs34 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDisconnectedIs34)
add_test(NAME Userconn.UserconnCharacterAddIs35 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterAddIs35)
add_test(NAME Userconn.UserconnPetAddIs36 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnPetAddIs36)
add_test(NAME Userconn.UserconnMonsterAddIs37 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMonsterAddIs37)
add_test(NAME Userconn.UserconnBossmonsterAddIs38 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBossmonsterAddIs38)
add_test(NAME Userconn.UserconnNpcAddIs39 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNpcAddIs39)
add_test(NAME Userconn.UserconnObjectRemoveIs40 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnObjectRemoveIs40)
add_test(NAME Userconn.UserconnCharacterDieIs41 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterDieIs41)
add_test(NAME Userconn.UserconnMonsterDieIs42 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMonsterDieIs42)
add_test(NAME Userconn.UserconnPetDieIs43 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnPetDieIs43)
add_test(NAME Userconn.UserconnCharacterReviveIs44 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterReviveIs44)
add_test(NAME Userconn.UserconnCharacterRemoveSynIs45 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterRemoveSynIs45)
add_test(NAME Userconn.UserconnCharacterRemoveAckIs46 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterRemoveAckIs46)
add_test(NAME Userconn.UserconnCharacterRemoveNackIs47 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterRemoveNackIs47)
add_test(NAME Userconn.UserconnChangemapSynIs48 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChangemapSynIs48)
add_test(NAME Userconn.UserconnChangemapAckIs49 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChangemapAckIs49)
add_test(NAME Userconn.UserconnChangemapNackIs50 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChangemapNackIs50)
add_test(NAME Userconn.UserconnMapOutIs51 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMapOutIs51)
add_test(NAME Userconn.UserconnMapOutWithmapnumIs52 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMapOutWithmapnumIs52)
add_test(NAME Userconn.UserconnCharacterTotalinfoIs53 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterTotalinfoIs53)
add_test(NAME Userconn.UserconnSavepointSynIs54 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnSavepointSynIs54)
add_test(NAME Userconn.UserconnSavepointAckIs55 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnSavepointAckIs55)
add_test(NAME Userconn.UserconnSavepointNackIs56 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnSavepointNackIs56)
add_test(NAME Userconn.UserconnBacktocharselSynIs57 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktocharselSynIs57)
add_test(NAME Userconn.UserconnBacktocharselAckIs58 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktocharselAckIs58)
add_test(NAME Userconn.UserconnBacktocharselNackIs59 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktocharselNackIs59)
add_test(NAME Userconn.UserconnGridinitIs60 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGridinitIs60)
add_test(NAME Userconn.UserconnSetvisibleIs61 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnSetvisibleIs61)
add_test(NAME Userconn.UserconnOtheruserConnecttryNotifyIs62 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnOtheruserConnecttryNotifyIs62)
add_test(NAME Userconn.UserconnConnectionCheckIs63 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnConnectionCheckIs63)
add_test(NAME Userconn.UserconnConnectionCheckOkIs64 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnConnectionCheckOkIs64)
add_test(NAME Userconn.UserconnChecksumerrorIs65 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChecksumerrorIs65)
add_test(NAME Userconn.UserconnForceDisconnectOverlaploginIs66 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnForceDisconnectOverlaploginIs66)
add_test(NAME Userconn.UserconnDisconnectedByOverlaploginIs67 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDisconnectedByOverlaploginIs67)
add_test(NAME Userconn.UserconnChannelinfoSynIs68 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChannelinfoSynIs68)
add_test(NAME Userconn.UserconnChannelinfoAckIs69 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChannelinfoAckIs69)
add_test(NAME Userconn.UserconnChannelinfoNackIs70 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChannelinfoNackIs70)
add_test(NAME Userconn.UserconnNotifytoagentAlreadyoutIs71 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNotifytoagentAlreadyoutIs71)
add_test(NAME Userconn.UserconnRequestDistoutIs72 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnRequestDistoutIs72)
add_test(NAME Userconn.UserconnDisconnectedOnLoginIs73 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnDisconnectedOnLoginIs73)
add_test(NAME Userconn.UserconnServerNotreadyIs74 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnServerNotreadyIs74)
add_test(NAME Userconn.UserconnMapdescIs75 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMapdescIs75)
add_test(NAME Userconn.UserconnCharacterReviveNackIs76 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterReviveNackIs76)
add_test(NAME Userconn.UserconnReadyToReviveIs77 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnReadyToReviveIs77)
add_test(NAME Userconn.UserconnCheatUsingIs78 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCheatUsingIs78)
add_test(NAME Userconn.UserconnCheatChangemapAckIs79 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCheatChangemapAckIs79)
add_test(NAME Userconn.UserconnUseDynamicSynIs80 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnUseDynamicSynIs80)
add_test(NAME Userconn.UserconnUseDynamicAckIs81 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnUseDynamicAckIs81)
add_test(NAME Userconn.UserconnUseDynamicNackIs82 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnUseDynamicNackIs82)
add_test(NAME Userconn.UserconnLoginDynamicSynIs83 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginDynamicSynIs83)
add_test(NAME Userconn.UserconnLoginDynamicAckIs84 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginDynamicAckIs84)
add_test(NAME Userconn.UserconnLoginDynamicNackIs85 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginDynamicNackIs85)
add_test(NAME Userconn.UserconnLogincheckDeleteIs86 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLogincheckDeleteIs86)
add_test(NAME Userconn.UserconnForceDisconnectOverlaploginAckIs87 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnForceDisconnectOverlaploginAckIs87)
add_test(NAME Userconn.UserconnMapOutToEventmapIs88 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMapOutToEventmapIs88)
add_test(NAME Userconn.UserconnMapOutToEventbeforemapIs89 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnMapOutToEventbeforemapIs89)
add_test(NAME Userconn.UserconnEnterEventmapSynIs90 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEnterEventmapSynIs90)
add_test(NAME Userconn.UserconnEventReadyIs91 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEventReadyIs91)
add_test(NAME Userconn.UserconnEventStartIs92 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEventStartIs92)
add_test(NAME Userconn.UserconnEventEndIs93 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEventEndIs93)
add_test(NAME Userconn.UserconnEventitemUseIs94 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEventitemUseIs94)
add_test(NAME Userconn.UserconnEventitemUse2Is95 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEventitemUse2Is95)
add_test(NAME Userconn.UserconnGameinposSynIs96 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinposSynIs96)
add_test(NAME Userconn.UserconnGameinposAckIs97 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinposAckIs97)
add_test(NAME Userconn.UserconnGameinposNackIs98 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinposNackIs98)
add_test(NAME Userconn.UserconnRemaintimeNotifyIs99 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnRemaintimeNotifyIs99)
add_test(NAME Userconn.UserconnBacktobeforemapTouserIs100 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktobeforemapTouserIs100)
add_test(NAME Userconn.UserconnBacktobeforemapSynIs101 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktobeforemapSynIs101)
add_test(NAME Userconn.UserconnBacktobeforemapAckIs102 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktobeforemapAckIs102)
add_test(NAME Userconn.UserconnBacktobeforemapNackIs103 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnBacktobeforemapNackIs103)
add_test(NAME Userconn.UserconnEnterGtournamentSynIs104 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnEnterGtournamentSynIs104)
add_test(NAME Userconn.UserconnCharacterslotIs105 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCharacterslotIs105)
add_test(NAME Userconn.UserconnCastlegateAddIs106 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCastlegateAddIs106)
add_test(NAME Userconn.UserconnGameinOthermapSynIs107 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnGameinOthermapSynIs107)
add_test(NAME Userconn.UserconnNowaitexitplayerIs108 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnNowaitexitplayerIs108)
add_test(NAME Userconn.UserconnFlagnpcOnoffIs109 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnFlagnpcOnoffIs109)
add_test(NAME Userconn.UserconnLoginSynBuddyIs110 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginSynBuddyIs110)
add_test(NAME Userconn.UserconnChangemapChannelinfoSynIs111 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChangemapChannelinfoSynIs111)
add_test(NAME Userconn.UserconnChangemapChannelinfoAckIs112 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChangemapChannelinfoAckIs112)
add_test(NAME Userconn.UserconnChangemapChannelinfoNackIs113 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnChangemapChannelinfoNackIs113)
add_test(NAME Userconn.UserconnCurrentmapChannelinfoIs114 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnCurrentmapChannelinfoIs114)
add_test(NAME Userconn.UserconnLoginErrNoAgentServerIs0 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginErrNoAgentServerIs0)
add_test(NAME Userconn.UserconnLoginErrDistAlreadyoutIs1 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginErrDistAlreadyoutIs1)
add_test(NAME Userconn.UserconnLoginErrOverlapIs2 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginErrOverlapIs2)
add_test(NAME Userconn.UserconnLoginErrNoUserIs3 COMMAND mxh_server_misc_constants_sweep_tests --gtest_filter=Userconn.UserconnLoginErrNoUserIs3)
add_test(NAME Autonote.AutonoteCategoryIs75 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteCategoryIs75)
add_test(NAME Autonote.AutonoteAsktoautoSynIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteAsktoautoSynIs0)
add_test(NAME Autonote.AutonoteAsktoautoIs3 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteAsktoautoIs3)
add_test(NAME Autonote.AutonoteAnswerSynIs4 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteAnswerSynIs4)
add_test(NAME Autonote.AutonoteAnswerFailIs7 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteAnswerFailIs7)
add_test(NAME Autonote.AutonoteNotautoIs10 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteNotautoIs10)
add_test(NAME Autonote.AutonoteKillautoIs11 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteKillautoIs11)
add_test(NAME Autonote.AutonoteListAddIs13 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteListAddIs13)
add_test(NAME Autonote.AutonotePunishIs15 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonotePunishIs15)
add_test(NAME Autonote.AutonoteAsktoautoImageIs16 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.AutonoteAsktoautoImageIs16)
add_test(NAME Autonote.UserLevelGmIs8 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Autonote.UserLevelGmIs8)
add_test(NAME Bobusang_user.BobusangUserCategoryIs73 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Bobusang_user.BobusangUserCategoryIs73)
add_test(NAME Chat.ChatAllIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Chat.ChatAllIs0)
add_test(NAME Fortwar.FortwarCategoryIs76 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Fortwar.FortwarCategoryIs76)
add_test(NAME Fortwar.FortwarInfoIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Fortwar.FortwarInfoIs0)
add_test(NAME Fortwar.FortwarStartBefore10minIs1 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Fortwar.FortwarStartBefore10minIs1)
add_test(NAME Fortwar.FortwarStartBefore10minToMapIs5 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Fortwar.FortwarStartBefore10minToMapIs5)
add_test(NAME Friend.FriendCategoryIs33 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendCategoryIs33)
add_test(NAME Friend.FriendAddSynIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddSynIs0)
add_test(NAME Friend.FriendAddInviteIs3 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddInviteIs3)
add_test(NAME Friend.FriendAddAcceptIs4 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddAcceptIs4)
add_test(NAME Friend.FriendAddDenyIs7 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddDenyIs7)
add_test(NAME Friend.FriendDelSynIs8 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendDelSynIs8)
add_test(NAME Friend.FriendDelidSynIs11 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendDelidSynIs11)
add_test(NAME Friend.FriendShowListSynIs13 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendShowListSynIs13)
add_test(NAME Friend.FriendLoginIs16 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendLoginIs16)
add_test(NAME Friend.FriendLogoutNotifyIs19 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendLogoutNotifyIs19)
add_test(NAME Friend.FriendLogoutNotifyToAgentIs20 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendLogoutNotifyToAgentIs20)
add_test(NAME Friend.FriendLogoutNotifyToClientIs21 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendLogoutNotifyToClientIs21)
add_test(NAME Friend.FriendLogoutNotifyAgentToAgentIs22 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendLogoutNotifyAgentToAgentIs22)
add_test(NAME Friend.FriendAddidSynIs23 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddidSynIs23)
add_test(NAME Friend.FriendListSynIs26 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendListSynIs26)
add_test(NAME Friend.FriendAddAcceptToAgentIs29 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddAcceptToAgentIs29)
add_test(NAME Friend.FriendLoginNotifyToAgentIs30 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendLoginNotifyToAgentIs30)
add_test(NAME Friend.FriendAddInviteToAgentIs31 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddInviteToAgentIs31)
add_test(NAME Friend.FriendAddAckToAgentIs32 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddAckToAgentIs32)
add_test(NAME Friend.FriendAddNackToAgentIs33 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddNackToAgentIs33)
add_test(NAME Friend.FriendAddAcceptNackToAgentIs34 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Friend.FriendAddAcceptNackToAgentIs34)
add_test(NAME Guild.GuildCategoryIs63 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Guild.GuildCategoryIs63)
add_test(NAME Guild.GuildCreateSynIs2 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Guild.GuildCreateSynIs2)
add_test(NAME Guild.GuildGivenicknameNackIs21 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Guild.GuildGivenicknameNackIs21)
add_test(NAME Guild.GuildErrCreateNameIs4 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Guild.GuildErrCreateNameIs4)
add_test(NAME Guild_union.GuildUnionCategoryIs61 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Guild_union.GuildUnionCategoryIs61)
add_test(NAME Guild_union.GuildUnionCreateSynIs2 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Guild_union.GuildUnionCreateSynIs2)
add_test(NAME Hackcheck.HackcheckCategoryIs41 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Hackcheck.HackcheckCategoryIs41)
add_test(NAME Hackcheck.HackcheckSpeedhackIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Hackcheck.HackcheckSpeedhackIs0)
add_test(NAME Item.ItemCategoryIs5 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Item.ItemCategoryIs5)
add_test(NAME Item.ItemShopitemChangemapSynIs116 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Item.ItemShopitemChangemapSynIs116)
add_test(NAME Item.ItemShopitemChaseSynIs154 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Item.ItemShopitemChaseSynIs154)
add_test(NAME Item.ItemShopitemNchangeSynIs161 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Item.ItemShopitemNchangeSynIs161)
add_test(NAME Item.ItemShopitemShoutAckIs189 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Item.ItemShopitemShoutAckIs189)
add_test(NAME Item.ItemShopitemShoutSendserverIs191 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Item.ItemShopitemShoutSendserverIs191)
add_test(NAME Itemlimit.ItemlimitCategoryIs74 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Itemlimit.ItemlimitCategoryIs74)
add_test(NAME Itemlimit.ItemlimitAddcountToMapIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Itemlimit.ItemlimitAddcountToMapIs0)
add_test(NAME Itemlimit.ItemlimitFullToClientIs1 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Itemlimit.ItemlimitFullToClientIs1)
add_test(NAME Murimnet.MurimnetCategoryIs38 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Murimnet.MurimnetCategoryIs38)
add_test(NAME Murimnet.MurimnetChangetomurimnetSynIs1 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Murimnet.MurimnetChangetomurimnetSynIs1)
add_test(NAME Murimnet.MurimnetChangetomurimnetAckIs4 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Murimnet.MurimnetChangetomurimnetAckIs4)
add_test(NAME Note.NoteCategoryIs58 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteCategoryIs58)
add_test(NAME Note.NoteSendnoteSynIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteSendnoteSynIs0)
add_test(NAME Note.NoteSendnoteidSynIs3 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteSendnoteidSynIs3)
add_test(NAME Note.NoteReceivenoteIs4 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteReceivenoteIs4)
add_test(NAME Note.NoteDelnoteSynIs5 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteDelnoteSynIs5)
add_test(NAME Note.NoteDelallnoteSynIs8 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteDelallnoteSynIs8)
add_test(NAME Note.NoteNotelistSynIs11 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteNotelistSynIs11)
add_test(NAME Note.NoteReadnoteSynIs14 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteReadnoteSynIs14)
add_test(NAME Note.NoteNewNoteIs17 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Note.NoteNewNoteIs17)
add_test(NAME Packed.PackedCategoryIs13 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Packed.PackedCategoryIs13)
add_test(NAME Packed.PackedNormalIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Packed.PackedNormalIs0)
add_test(NAME Packed.PackedToMapserverIs1 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Packed.PackedToMapserverIs1)
add_test(NAME Packed.PackedToBroadMapserverIs2 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Packed.PackedToBroadMapserverIs2)
add_test(NAME Party.PartyCategoryIs14 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyCategoryIs14)
add_test(NAME Party.PartyInfoIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyInfoIs0)
add_test(NAME Party.PartyCreateSynIs1 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyCreateSynIs1)
add_test(NAME Party.PartyAddSynIs4 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyAddSynIs4)
add_test(NAME Party.PartyAddInviteIs7 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyAddInviteIs7)
add_test(NAME Party.PartyInviteAcceptSynIs8 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyInviteAcceptSynIs8)
add_test(NAME Party.PartyInviteDenySynIs11 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyInviteDenySynIs11)
add_test(NAME Party.PartyNotifyAddToMapserverIs14 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyAddToMapserverIs14)
add_test(NAME Party.PartyDelSynIs15 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyDelSynIs15)
add_test(NAME Party.PartyNotifyDeleteToMapserverIs18 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyDeleteToMapserverIs18)
add_test(NAME Party.PartyBanSynIs20 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyBanSynIs20)
add_test(NAME Party.PartyNotifyBanToMapserverIs23 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyBanToMapserverIs23)
add_test(NAME Party.PartyChangemasterSynIs24 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyChangemasterSynIs24)
add_test(NAME Party.PartyNotifyChangemasterToMapserverIs27 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyChangemasterToMapserverIs27)
add_test(NAME Party.PartyBreakupSynIs28 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyBreakupSynIs28)
add_test(NAME Party.PartyNotifyBreakupToMapserverIs31 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyBreakupToMapserverIs31)
add_test(NAME Party.PartyMemberLoginIs32 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMemberLoginIs32)
add_test(NAME Party.PartyNotifyMemberLoginToMapserverIs33 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyMemberLoginToMapserverIs33)
add_test(NAME Party.PartyMemberLogoutIs34 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMemberLogoutIs34)
add_test(NAME Party.PartyNotifyMemberLogoutToMapserverIs35 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyMemberLogoutToMapserverIs35)
add_test(NAME Party.PartyMemberlifeIs36 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMemberlifeIs36)
add_test(NAME Party.PartyMemberlevelIs39 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMemberlevelIs39)
add_test(NAME Party.PartyNotifyChangesToMapserverIs42 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyChangesToMapserverIs42)
add_test(NAME Party.PartyClearIs43 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyClearIs43)
add_test(NAME Party.PartyNotifyCreateToMapserverIs44 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyCreateToMapserverIs44)
add_test(NAME Party.PartyMemberLoginmsgIs45 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMemberLoginmsgIs45)
add_test(NAME Party.PartyNotifyMemberLoginmsgIs46 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyMemberLoginmsgIs46)
add_test(NAME Party.PartyNotifyMemberLevelIs47 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyMemberLevelIs47)
add_test(NAME Party.PartyMonsterObtainNotifyIs48 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMonsterObtainNotifyIs48)
add_test(NAME Party.PartyErrorIs50 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyErrorIs50)
add_test(NAME Party.PartyMasterToRequestSynIs51 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyMasterToRequestSynIs51)
add_test(NAME Party.PartyRequestConsentSynIs53 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyRequestConsentSynIs53)
add_test(NAME Party.PartyRequestRefusalSynIs56 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyRequestRefusalSynIs56)
add_test(NAME Party.PartyNotifyInfoIs59 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Party.PartyNotifyInfoIs59)
add_test(NAME Siegewar.SiegewarCategoryIs62 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar.SiegewarCategoryIs62)
add_test(NAME Siegewar.SiegewarMoveinSynIs1 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar.SiegewarMoveinSynIs1)
add_test(NAME Siegewar.SiegewarBattlejoinSynIs7 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar.SiegewarBattlejoinSynIs7)
add_test(NAME Siegewar.SiegewarLeaveSynIs12 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar.SiegewarLeaveSynIs12)
add_test(NAME Siegewar.SiegewarCheatIs61 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar.SiegewarCheatIs61)
add_test(NAME Siegewarprofit.SiegewarprofitCategoryIs63 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewarprofit.SiegewarprofitCategoryIs63)
add_test(NAME Siegewarprofit.SiegewarprofitChangeTexrateNotifyToMapIs7 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewarprofit.SiegewarprofitChangeTexrateNotifyToMapIs7)
add_test(NAME Siegewarprofit.SiegewarprofitChangeGuildNotifyToMapIs11 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewarprofit.SiegewarprofitChangeGuildNotifyToMapIs11)
add_test(NAME Siegewar_server.SiegewarTaxrateIs60 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar_server.SiegewarTaxrateIs60)
add_test(NAME Siegewar_server.SiegewarReturntomapIs50 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar_server.SiegewarReturntomapIs50)
add_test(NAME Siegewar_server.SiegewarFlagchangeIs62 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Siegewar_server.SiegewarFlagchangeIs62)
add_test(NAME Simple_auth_forward.StreetstallCategoryIs29 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Simple_auth_forward.StreetstallCategoryIs29)
add_test(NAME Simple_auth_forward.ExchangeCategoryIs28 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Simple_auth_forward.ExchangeCategoryIs28)
add_test(NAME Survival.SurvivalCategoryIs70 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Survival.SurvivalCategoryIs70)
add_test(NAME Survival.SurvivalInfoIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Survival.SurvivalInfoIs0)
add_test(NAME Survival.SurvivalReadySynIs7 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Survival.SurvivalReadySynIs7)
add_test(NAME Wanted.WantedCategoryIs51 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Wanted.WantedCategoryIs51)
add_test(NAME Wanted.WantedNotifyDeleteToMapIs9 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Wanted.WantedNotifyDeleteToMapIs9)
add_test(NAME Wanted.WantedNotifyNotcompleteToMapIs18 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Wanted.WantedNotifyNotcompleteToMapIs18)
add_test(NAME Wanted.WantedNotcompleteToAgentIs23 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Wanted.WantedNotcompleteToAgentIs23)
add_test(NAME Weather.WeatherCategoryIs64 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Weather.WeatherCategoryIs64)
add_test(NAME Weather.WeatherSetIs0 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Weather.WeatherSetIs0)
add_test(NAME Weather.UserLevelGmWeatherIs8 COMMAND mxh_agent_constants_sweep_tests --gtest_filter=Weather.UserLevelGmWeatherIs8)
add_executable(mxh_category_byte_sweep_tests
    category_byte_sweep_test.cpp
)
target_link_libraries(mxh_category_byte_sweep_tests PRIVATE mxh_server GTest::gtest_main)

add_executable(mxh_byte_match_sweep_tests
    byte_match_sweep_test.cpp
)
target_link_libraries(mxh_byte_match_sweep_tests PRIVATE mxh_server GTest::gtest_main)
add_executable(mxh_cheat_byte_sweep_tests
    cheat_byte_sweep_test.cpp
)
target_link_libraries(mxh_cheat_byte_sweep_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME CheatByte.GmLoginSynIs0 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.GmLoginSynIs0)
add_test(NAME CheatByte.ChangemapSynIs1 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ChangemapSynIs1)
add_test(NAME CheatByte.ChangemapNackIs2 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ChangemapNackIs2)
add_test(NAME CheatByte.ChangemapAckIs3 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ChangemapAckIs3)
add_test(NAME CheatByte.BancharacterSynIs4 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BancharacterSynIs4)
add_test(NAME CheatByte.BancharacterNackIs5 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BancharacterNackIs5)
add_test(NAME CheatByte.BlockcharacterSynIs6 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BlockcharacterSynIs6)
add_test(NAME CheatByte.WhereisSynIs7 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.WhereisSynIs7)
add_test(NAME CheatByte.EventMonsterRegenIs8 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventMonsterRegenIs8)
add_test(NAME CheatByte.EventMonsterDeleteIs9 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventMonsterDeleteIs9)
add_test(NAME CheatByte.BanmapSynIs10 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BanmapSynIs10)
add_test(NAME CheatByte.AgentcheckSynIs11 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.AgentcheckSynIs11)
add_test(NAME CheatByte.PkallowSynIs12 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.PkallowSynIs12)
add_test(NAME CheatByte.NoticeSynIs13 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.NoticeSynIs13)
add_test(NAME CheatByte.AbilityexpSynIs14 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.AbilityexpSynIs14)
add_test(NAME CheatByte.AddmugongSynIs15 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.AddmugongSynIs15)
add_test(NAME CheatByte.MugongsungSynIs16 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.MugongsungSynIs16)
add_test(NAME CheatByte.ItemSynIs17 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ItemSynIs17)
add_test(NAME CheatByte.ItemOptionSynIs18 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ItemOptionSynIs18)
add_test(NAME CheatByte.MoneySynIs19 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.MoneySynIs19)
add_test(NAME CheatByte.EventSynIs20 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventSynIs20)
add_test(NAME CheatByte.EventnotifyOnIs21 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventnotifyOnIs21)
add_test(NAME CheatByte.PlustimeOnIs22 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.PlustimeOnIs22)
add_test(NAME CheatByte.EventnotifyOffIs23 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventnotifyOffIs23)
add_test(NAME CheatByte.PlustimeAlloffIs24 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.PlustimeAlloffIs24)
add_test(NAME CheatByte.ChangeEventmapSynIs25 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ChangeEventmapSynIs25)
add_test(NAME CheatByte.EventStartSynIs26 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventStartSynIs26)
add_test(NAME CheatByte.EventReadySynIs27 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.EventReadySynIs27)
add_test(NAME CheatByte.PetStaminaIs28 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.PetStaminaIs28)
add_test(NAME CheatByte.PetFriendshipSynIs29 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.PetFriendshipSynIs29)
add_test(NAME CheatByte.PetSelectedFriendshipSynIs30 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.PetSelectedFriendshipSynIs30)
add_test(NAME CheatByte.GuildpointSynIs31 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.GuildpointSynIs31)
add_test(NAME CheatByte.GuildhuntedMonstercountSynIs32 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.GuildhuntedMonstercountSynIs32)
add_test(NAME CheatByte.MussangReadyIs33 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.MussangReadyIs33)
add_test(NAME CheatByte.JackpotGetprizeIs34 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.JackpotGetprizeIs34)
add_test(NAME CheatByte.JackpotMoneypermonsterIs35 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.JackpotMoneypermonsterIs35)
add_test(NAME CheatByte.JackpotOnoffIs36 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.JackpotOnoffIs36)
add_test(NAME CheatByte.JackpotProbabilityIs37 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.JackpotProbabilityIs37)
add_test(NAME CheatByte.JackpotControlIs38 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.JackpotControlIs38)
add_test(NAME CheatByte.BobusanginfoRequestSynIs39 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BobusanginfoRequestSynIs39)
add_test(NAME CheatByte.BobusangLeaveSynIs40 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BobusangLeaveSynIs40)
add_test(NAME CheatByte.BobusanginfoChangeSynIs41 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BobusanginfoChangeSynIs41)
add_test(NAME CheatByte.ItemlimitSynIs42 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.ItemlimitSynIs42)
add_test(NAME CheatByte.AutonoteSettingSynIs43 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.AutonoteSettingSynIs43)
add_test(NAME CheatByte.DamageSynIs44 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.DamageSynIs44)
add_test(NAME CheatByte.DamageAckIs45 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.DamageAckIs45)
add_test(NAME CheatByte.DamageNackIs46 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.DamageNackIs46)
add_test(NAME CheatByte.MapConditionIs47 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.MapConditionIs47)
add_test(NAME CheatByte.AgentConditionIs48 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.AgentConditionIs48)
add_test(NAME CheatByte.TitanFuelSpellMaxSynIs49 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.TitanFuelSpellMaxSynIs49)
add_test(NAME CheatByte.BancharacterAckIs50 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.BancharacterAckIs50)
add_test(NAME CheatByte.WhereisAckIs52 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatByte.WhereisAckIs52)
add_test(NAME PowerUpByte.BootingNotifyIs0 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=PowerUpByte.BootingNotifyIs0)
add_test(NAME PowerUpByte.BootlistSynIs1 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=PowerUpByte.BootlistSynIs1)
add_test(NAME PowerUpByte.BootlistAckIs2 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=PowerUpByte.BootlistAckIs2)
add_test(NAME PowerUpByte.ConnectSynIs3 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=PowerUpByte.ConnectSynIs3)
add_test(NAME PowerUpByte.ConnectAckIs4 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=PowerUpByte.ConnectAckIs4)
add_test(NAME GTournamentByte.MovetobattlemapSynIs0 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.MovetobattlemapSynIs0)
add_test(NAME GTournamentByte.MovetobattlemapNackIs1 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.MovetobattlemapNackIs1)
add_test(NAME GTournamentByte.StandinginfoSynIs2 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.StandinginfoSynIs2)
add_test(NAME GTournamentByte.StandinginfoNackIs3 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.StandinginfoNackIs3)
add_test(NAME GTournamentByte.BattlejoinSynIs4 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.BattlejoinSynIs4)
add_test(NAME GTournamentByte.ObserverjoinSynIs5 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.ObserverjoinSynIs5)
add_test(NAME GTournamentByte.BattlejoinNackIs6 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.BattlejoinNackIs6)
add_test(NAME GTournamentByte.LeaveSynIs7 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.LeaveSynIs7)
add_test(NAME GTournamentByte.CheatIs8 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.CheatIs8)
add_test(NAME GTournamentByte.EventStartIs9 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.EventStartIs9)
add_test(NAME GTournamentByte.EventEndIs10 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.EventEndIs10)
add_test(NAME GTournamentByte.StandinginfoRegistedIs11 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.StandinginfoRegistedIs11)
add_test(NAME GTournamentByte.ReturntomapIs12 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.ReturntomapIs12)
add_test(NAME GTournamentByte.NotifyWinloseIs13 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=GTournamentByte.NotifyWinloseIs13)
add_test(NAME CheatConstants.UserLevelGmIs1 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatConstants.UserLevelGmIs1)
add_test(NAME CheatConstants.UserLevelProgrammerIs2 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatConstants.UserLevelProgrammerIs2)
add_test(NAME CheatConstants.UserLevelDeveloperIs3 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CheatConstants.UserLevelDeveloperIs3)
add_test(NAME UserConnConstants.LoginErrNoAgentServerIs0 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=UserConnConstants.LoginErrNoAgentServerIs0)
add_test(NAME UserConnConstants.LoginErrDistAlreadyoutIs1 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=UserConnConstants.LoginErrDistAlreadyoutIs1)
add_test(NAME CategoryInvariant.UserConnIs7 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CategoryInvariant.UserConnIs7)
add_test(NAME CategoryInvariant.PowerUpIs2 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CategoryInvariant.PowerUpIs2)
add_test(NAME CategoryInvariant.CheatIs11 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CategoryInvariant.CheatIs11)
add_test(NAME CategoryInvariant.GTournamentIs60 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CategoryInvariant.GTournamentIs60)
add_test(NAME MapConstants.GTournamentMapNumIs60 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=MapConstants.GTournamentMapNumIs60)
add_test(NAME PowerUpConstants.MaxAgentServersAtLeast50 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=PowerUpConstants.MaxAgentServersAtLeast50)
add_test(NAME UserConnByte.DistConnectSuccessIs0 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DistConnectSuccessIs0)
add_test(NAME UserConnByte.LoginSynIs1 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginSynIs1)
add_test(NAME UserConnByte.LoginAckIs2 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginAckIs2)
add_test(NAME UserConnByte.LoginNackIs3 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginNackIs3)
add_test(NAME UserConnByte.NotifyUserloginSynIs4 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NotifyUserloginSynIs4)
add_test(NAME UserConnByte.NotifyUserloginAckIs5 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NotifyUserloginAckIs5)
add_test(NAME UserConnByte.NotifyUserloginNackIs6 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NotifyUserloginNackIs6)
add_test(NAME UserConnByte.NotifyOverlappedloginIs7 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NotifyOverlappedloginIs7)
add_test(NAME UserConnByte.AgentConnectsuccessIs8 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.AgentConnectsuccessIs8)
add_test(NAME UserConnByte.CharacterlistSynIs9 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterlistSynIs9)
add_test(NAME UserConnByte.DirectcharacterlistSynIs10 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DirectcharacterlistSynIs10)
add_test(NAME UserConnByte.CharacterlistNackIs11 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterlistNackIs11)
add_test(NAME UserConnByte.CharacterlistAckIs12 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterlistAckIs12)
add_test(NAME UserConnByte.DisconnectSynIs13 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DisconnectSynIs13)
add_test(NAME UserConnByte.DisconnectAckIs14 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DisconnectAckIs14)
add_test(NAME UserConnByte.DisconnectNackIs15 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DisconnectNackIs15)
add_test(NAME UserConnByte.CharacterselectSynIs16 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterselectSynIs16)
add_test(NAME UserConnByte.CharacterselectAckIs17 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterselectAckIs17)
add_test(NAME UserConnByte.CharacterselectNackIs18 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterselectNackIs18)
add_test(NAME UserConnByte.CharacterNamecheckSynIs19 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterNamecheckSynIs19)
add_test(NAME UserConnByte.CharacterNamecheckAckIs20 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterNamecheckAckIs20)
add_test(NAME UserConnByte.CharacterNamecheckNackIs21 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterNamecheckNackIs21)
add_test(NAME UserConnByte.CharacterMakeSynIs22 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterMakeSynIs22)
add_test(NAME UserConnByte.CharacterMakeAckIs23 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterMakeAckIs23)
add_test(NAME UserConnByte.CharacterMakeNackIs24 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterMakeNackIs24)
add_test(NAME UserConnByte.CharacterInfoSynIs25 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterInfoSynIs25)
add_test(NAME UserConnByte.CharacterInfoAckIs26 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterInfoAckIs26)
add_test(NAME UserConnByte.CharacterInfoNackIs27 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterInfoNackIs27)
add_test(NAME UserConnByte.GameinSynIs28 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinSynIs28)
add_test(NAME UserConnByte.GameinAckIs29 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinAckIs29)
add_test(NAME UserConnByte.GameinNackIs30 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinNackIs30)
add_test(NAME UserConnByte.GameoutSynIs31 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameoutSynIs31)
add_test(NAME UserConnByte.GameoutAckIs32 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameoutAckIs32)
add_test(NAME UserConnByte.GameoutNackIs33 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameoutNackIs33)
add_test(NAME UserConnByte.DisconnectedIs34 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DisconnectedIs34)
add_test(NAME UserConnByte.CharacterAddIs35 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterAddIs35)
add_test(NAME UserConnByte.PetAddIs36 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.PetAddIs36)
add_test(NAME UserConnByte.MonsterAddIs37 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MonsterAddIs37)
add_test(NAME UserConnByte.BossmonsterAddIs38 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BossmonsterAddIs38)
add_test(NAME UserConnByte.NpcAddIs39 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NpcAddIs39)
add_test(NAME UserConnByte.ObjectRemoveIs40 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ObjectRemoveIs40)
add_test(NAME UserConnByte.CharacterDieIs41 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterDieIs41)
add_test(NAME UserConnByte.MonsterDieIs42 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MonsterDieIs42)
add_test(NAME UserConnByte.PetDieIs43 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.PetDieIs43)
add_test(NAME UserConnByte.CharacterReviveIs44 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterReviveIs44)
add_test(NAME UserConnByte.CharacterRemoveSynIs45 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterRemoveSynIs45)
add_test(NAME UserConnByte.CharacterRemoveAckIs46 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterRemoveAckIs46)
add_test(NAME UserConnByte.CharacterRemoveNackIs47 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterRemoveNackIs47)
add_test(NAME UserConnByte.ChangemapSynIs48 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChangemapSynIs48)
add_test(NAME UserConnByte.ChangemapAckIs49 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChangemapAckIs49)
add_test(NAME UserConnByte.ChangemapNackIs50 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChangemapNackIs50)
add_test(NAME UserConnByte.MapOutIs51 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MapOutIs51)
add_test(NAME UserConnByte.MapOutWithmapnumIs52 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MapOutWithmapnumIs52)
add_test(NAME UserConnByte.CharacterTotalinfoIs53 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterTotalinfoIs53)
add_test(NAME UserConnByte.SavepointSynIs54 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.SavepointSynIs54)
add_test(NAME UserConnByte.SavepointAckIs55 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.SavepointAckIs55)
add_test(NAME UserConnByte.SavepointNackIs56 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.SavepointNackIs56)
add_test(NAME UserConnByte.BacktocharselSynIs57 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktocharselSynIs57)
add_test(NAME UserConnByte.BacktocharselAckIs58 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktocharselAckIs58)
add_test(NAME UserConnByte.BacktocharselNackIs59 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktocharselNackIs59)
add_test(NAME UserConnByte.GridinitIs60 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GridinitIs60)
add_test(NAME UserConnByte.SetvisibleIs61 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.SetvisibleIs61)
add_test(NAME UserConnByte.OtheruserConnecttryNotifyIs62 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.OtheruserConnecttryNotifyIs62)
add_test(NAME UserConnByte.ConnectionCheckIs63 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ConnectionCheckIs63)
add_test(NAME UserConnByte.ConnectionCheckOkIs64 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ConnectionCheckOkIs64)
add_test(NAME UserConnByte.ChecksumerrorIs65 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChecksumerrorIs65)
add_test(NAME UserConnByte.ForceDisconnectOverlaploginIs66 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ForceDisconnectOverlaploginIs66)
add_test(NAME UserConnByte.DisconnectedByOverlaploginIs67 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DisconnectedByOverlaploginIs67)
add_test(NAME UserConnByte.ChannelinfoSynIs68 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChannelinfoSynIs68)
add_test(NAME UserConnByte.ChannelinfoAckIs69 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChannelinfoAckIs69)
add_test(NAME UserConnByte.ChannelinfoNackIs70 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChannelinfoNackIs70)
add_test(NAME UserConnByte.NotifytoagentAlreadyoutIs71 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NotifytoagentAlreadyoutIs71)
add_test(NAME UserConnByte.RequestDistoutIs72 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.RequestDistoutIs72)
add_test(NAME UserConnByte.DisconnectedOnLoginIs73 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.DisconnectedOnLoginIs73)
add_test(NAME UserConnByte.ServerNotreadyIs74 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ServerNotreadyIs74)
add_test(NAME UserConnByte.MapdescIs75 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MapdescIs75)
add_test(NAME UserConnByte.CharacterReviveNackIs76 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterReviveNackIs76)
add_test(NAME UserConnByte.ReadyToReviveIs77 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ReadyToReviveIs77)
add_test(NAME UserConnByte.CheatUsingIs78 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CheatUsingIs78)
add_test(NAME UserConnByte.CheatChangemapAckIs79 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CheatChangemapAckIs79)
add_test(NAME UserConnByte.UseDynamicSynIs80 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.UseDynamicSynIs80)
add_test(NAME UserConnByte.UseDynamicAckIs81 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.UseDynamicAckIs81)
add_test(NAME UserConnByte.UseDynamicNackIs82 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.UseDynamicNackIs82)
add_test(NAME UserConnByte.LoginDynamicSynIs83 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginDynamicSynIs83)
add_test(NAME UserConnByte.LoginDynamicAckIs84 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginDynamicAckIs84)
add_test(NAME UserConnByte.LoginDynamicNackIs85 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginDynamicNackIs85)
add_test(NAME UserConnByte.LogincheckDeleteIs86 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LogincheckDeleteIs86)
add_test(NAME UserConnByte.ForceDisconnectOverlaploginAckIs87 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ForceDisconnectOverlaploginAckIs87)
add_test(NAME UserConnByte.MapOutToEventmapIs88 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MapOutToEventmapIs88)
add_test(NAME UserConnByte.MapOutToEventbeforemapIs89 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.MapOutToEventbeforemapIs89)
add_test(NAME UserConnByte.EnterEventmapSynIs90 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EnterEventmapSynIs90)
add_test(NAME UserConnByte.EventReadyIs91 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EventReadyIs91)
add_test(NAME UserConnByte.EventStartIs92 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EventStartIs92)
add_test(NAME UserConnByte.EventEndIs93 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EventEndIs93)
add_test(NAME UserConnByte.EventitemUseIs94 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EventitemUseIs94)
add_test(NAME UserConnByte.EventitemUse2Is95 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EventitemUse2Is95)
add_test(NAME UserConnByte.GameinposSynIs96 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinposSynIs96)
add_test(NAME UserConnByte.GameinposAckIs97 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinposAckIs97)
add_test(NAME UserConnByte.GameinposNackIs98 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinposNackIs98)
add_test(NAME UserConnByte.RemaintimeNotifyIs99 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.RemaintimeNotifyIs99)
add_test(NAME UserConnByte.BacktobeforemapTouserIs100 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktobeforemapTouserIs100)
add_test(NAME UserConnByte.BacktobeforemapSynIs101 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktobeforemapSynIs101)
add_test(NAME UserConnByte.BacktobeforemapAckIs102 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktobeforemapAckIs102)
add_test(NAME UserConnByte.BacktobeforemapNackIs103 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.BacktobeforemapNackIs103)
add_test(NAME UserConnByte.EnterGtournamentSynIs104 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.EnterGtournamentSynIs104)
add_test(NAME UserConnByte.CharacterslotIs105 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CharacterslotIs105)
add_test(NAME UserConnByte.CastlegateAddIs106 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CastlegateAddIs106)
add_test(NAME UserConnByte.GameinOthermapSynIs107 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.GameinOthermapSynIs107)
add_test(NAME UserConnByte.NowaitexitplayerIs108 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.NowaitexitplayerIs108)
add_test(NAME UserConnByte.FlagnpcOnoffIs109 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.FlagnpcOnoffIs109)
add_test(NAME UserConnByte.LoginSynBuddyIs110 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.LoginSynBuddyIs110)
add_test(NAME UserConnByte.ChangemapChannelinfoSynIs111 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChangemapChannelinfoSynIs111)
add_test(NAME UserConnByte.ChangemapChannelinfoAckIs112 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChangemapChannelinfoAckIs112)
add_test(NAME UserConnByte.ChangemapChannelinfoNackIs113 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.ChangemapChannelinfoNackIs113)
add_test(NAME UserConnByte.CurrentmapChannelinfoIs114 COMMAND mxh_byte_match_sweep_tests --gtest_filter=UserConnByte.CurrentmapChannelinfoIs114)
add_test(NAME CategoryByteSweep.UserConn_IsByte7 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.UserConn_IsByte7)
add_test(NAME CategoryByteSweep.PowerUp_IsByte2 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.PowerUp_IsByte2)
add_test(NAME CategoryByteSweep.Cheat_IsByte11 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Cheat_IsByte11)
add_test(NAME CategoryByteSweep.Friend_IsByte33 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Friend_IsByte33)
add_test(NAME CategoryByteSweep.Weather_IsByte64 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Weather_IsByte64)
add_test(NAME CategoryByteSweep.Wanted_IsByte51 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Wanted_IsByte51)
add_test(NAME CategoryByteSweep.Survival_IsByte70 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Survival_IsByte70)
add_test(NAME CategoryByteSweep.ItemLimit_IsByte74 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.ItemLimit_IsByte74)
add_test(NAME CategoryByteSweep.FortWar_IsByte76 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.FortWar_IsByte76)
add_test(NAME CategoryByteSweep.Bobusang_IsByte73 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Bobusang_IsByte73)
add_test(NAME CategoryByteSweep.AutoNote_IsByte75 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.AutoNote_IsByte75)
add_test(NAME CategoryByteSweep.SiegeWarProfit_IsByte63 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.SiegeWarProfit_IsByte63)
add_test(NAME CategoryByteSweep.GuildUnion_IsByte61 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.GuildUnion_IsByte61)
add_test(NAME CategoryByteSweep.SiegeWar_IsByte62 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.SiegeWar_IsByte62)
add_test(NAME CategoryByteSweep.HackCheck_IsByte41 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.HackCheck_IsByte41)
add_test(NAME CategoryByteSweep.PackedData_IsByte13 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.PackedData_IsByte13)
add_test(NAME CategoryByteSweep.Note_IsByte58 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Note_IsByte58)
add_test(NAME CategoryByteSweep.Item_IsByte5 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Item_IsByte5)
add_test(NAME CategoryByteSweep.Party_IsByte14 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Party_IsByte14)
add_test(NAME CategoryByteSweep.MurimNet_IsByte38 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.MurimNet_IsByte38)
add_test(NAME CategoryByteSweep.Exchange_IsByte28 COMMAND mxh_category_byte_sweep_tests --gtest_filter=CategoryByteSweep.Exchange_IsByte28)


gtest_add_tests(TARGET mxh_tactic_ability_info_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_itemlimit_tests
    agent_itemlimit_test.cpp
)
target_link_libraries(mxh_agent_itemlimit_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_itemlimit_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_fortwar_tests
    agent_fortwar_test.cpp
)
target_link_libraries(mxh_agent_fortwar_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_fortwar_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_survival_tests
    agent_survival_test.cpp
)
target_link_libraries(mxh_agent_survival_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_survival_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_wanted_tests
    agent_wanted_test.cpp
)
target_link_libraries(mxh_agent_wanted_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_wanted_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_bobusang_user_tests
    agent_bobusang_user_test.cpp
)
target_link_libraries(mxh_agent_bobusang_user_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_bobusang_user_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_gtournament_tests
    agent_gtournament_test.cpp
)
target_link_libraries(mxh_agent_gtournament_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_gtournament_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_siegewarprofit_tests
    agent_siegewarprofit_test.cpp
)
target_link_libraries(mxh_agent_siegewarprofit_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_siegewarprofit_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_guild_union_tests
    agent_guild_union_test.cpp
)
target_link_libraries(mxh_agent_guild_union_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_guild_union_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_siegewar_tests
    agent_siegewar_test.cpp
)
target_link_libraries(mxh_agent_siegewar_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_siegewar_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_siegewar_server_tests
    agent_siegewar_server_test.cpp
)
target_link_libraries(mxh_agent_siegewar_server_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_siegewar_server_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_simple_auth_forward_tests
    agent_simple_auth_forward_test.cpp
)
target_link_libraries(mxh_agent_simple_auth_forward_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_simple_auth_forward_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_agent_hackcheck_tests
    agent_hackcheck_test.cpp
)
target_link_libraries(mxh_agent_hackcheck_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_agent_hackcheck_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)
















# Phase 6.4 unblock: regression coverage for tools/MoxianSideBySide/packet.cpp
# Packet::wire_bytes() -- locks the 4.5 wire-format fix (commit 60110a7).
# packet.cpp is compiled directly into the test because MoxianSideBySide is
# built as an executable (not a library) -- including it as a source avoids
# needing to refactor packet.cpp into mxh_side_by_side_core.
add_executable(mxh_wire_format_tests
    wire_format_test.cpp
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide/packet.cpp
)
target_include_directories(mxh_wire_format_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/MoxianSideBySide
)
target_link_libraries(mxh_wire_format_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_wire_format_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# Phase 6.4 E2E: in-process LoginServer wire-format test (M2 step 1 scaffold).
# Spins up LoginHandler + TcpServer(legacy_framing=true) inside the test,
# connects a TcpClient, and verifies DistConnectSuccess / NotifyUserLoginAck
# byte layouts round-trip correctly across the wire.
add_executable(mxh_login_e2e_tests login_e2e_test.cpp)
target_link_libraries(mxh_login_e2e_tests PRIVATE mxh_server mxh_db GTest::gtest_main)
if(WIN32)
    target_link_libraries(mxh_login_e2e_tests PRIVATE Ws2_32)
endif()
gtest_add_tests(TARGET mxh_login_e2e_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

# M18 wire-format coverage audit (C 协议扩展 foundation).
# Pure header-only test that reads golden/ files and asserts the floor.
add_executable(mxh_wire_format_coverage_tests wire_format_coverage_test.cpp)
target_link_libraries(mxh_wire_format_coverage_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_wire_format_coverage_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_play_room_tests murimnet_play_room_test.cpp)
target_link_libraries(mxh_murimnet_play_room_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_play_room_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_play_room_manager_tests murimnet_play_room_manager_test.cpp)
target_link_libraries(mxh_murimnet_play_room_manager_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_play_room_manager_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_player_tests murimnet_player_test.cpp)
target_link_libraries(mxh_murimnet_player_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_player_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_runtime_tests murimnet_runtime_test.cpp)
target_link_libraries(mxh_murimnet_runtime_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_runtime_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_channel_tests murimnet_channel_test.cpp)
target_link_libraries(mxh_murimnet_channel_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_channel_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_wire_tests murimnet_wire_test.cpp)
target_link_libraries(mxh_murimnet_wire_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_wire_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_wire_serializer_tests murimnet_wire_serializer_test.cpp)
target_link_libraries(mxh_murimnet_wire_serializer_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_wire_serializer_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_murimnet_crypt_tests murimnet_crypt_test.cpp)
target_link_libraries(mxh_murimnet_crypt_tests PRIVATE mxh_server mxh_crypto GTest::gtest_main)
gtest_add_tests(TARGET mxh_murimnet_crypt_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit)

add_executable(mxh_player_monster_point_tests
    player_monster_point_test.cpp
)
target_link_libraries(mxh_player_monster_point_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_player_monster_point_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_player_monster_point_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)

add_executable(mxh_boss_monster_tests
    boss_monster_test.cpp
)
target_link_libraries(mxh_boss_monster_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_boss_monster_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_boss_monster_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_field_boss_monster_tests
    field_boss_monster_test.cpp
)
target_link_libraries(mxh_field_boss_monster_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_field_boss_monster_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_field_boss_monster_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_boss_monster_manager_tests
    boss_monster_manager_test.cpp
)
target_link_libraries(mxh_boss_monster_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_boss_monster_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_boss_monster_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_c_monster_speech_manager_tests
    c_monster_speech_manager_test.cpp
)
target_link_libraries(mxh_c_monster_speech_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_c_monster_speech_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_c_monster_speech_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)
add_executable(mxh_shop_item_manager_tests
    shop_item_manager_test.cpp
    shop_item_update_sql_test.cpp
    calc_shop_item_option_test.cpp
    update_logout_to_db_test.cpp
    dup_param_test.cpp
    is_dup_item_test.cpp
    item_kind_predicates_test.cpp
    get_item_kind_type_test.cpp
    playtime_decrement_test.cpp
    discard_avatar_item_test.cpp
    calc_plus_time_test.cpp
    extra_slot_count_test.cpp
    looting_thresholds_test.cpp
    avatar_equip_transition_test.cpp
    skin_select_transition_test.cpp
    skin_discard_transition_test.cpp
    add_using_shop_item_test.cpp
    check_end_time_side_effect_test.cpp
    put_on_avatar_side_effect_test.cpp
    take_off_avatar_side_effect_test.cpp
    skin_select_side_effect_test.cpp
    skin_discard_side_effect_test.cpp
    save_point_add_side_effect_test.cpp
    save_point_mutate_side_effect_test.cpp
    item_move_side_effect_test.cpp
    pyoguk_money_side_effect_test.cpp
    buy_pyoguk_side_effect_test.cpp
    item_discard_side_effect_test.cpp
    item_sell_side_effect_test.cpp
    item_use_side_effect_test.cpp
    item_divide_side_effect_test.cpp
    item_combine_side_effect_test.cpp
    item_buy_side_effect_test.cpp
    item_mix_side_effect_test.cpp
    item_dissolution_side_effect_test.cpp
    item_mix_add_item_side_effect_test.cpp
    item_mix_release_side_effect_test.cpp
    use_for_quest_start_side_effect_test.cpp
    shop_item_change_map_side_effect_test.cpp
    use_change_item_side_effect_test.cpp
    dealer_open_side_effect_test.cpp
    pyoguk_item_info_side_effect_test.cpp
    pet_inven_info_side_effect_test.cpp
    shop_item_info_side_effect_test.cpp
    shop_item_mpinfo_side_effect_test.cpp
    guild_warehouse_info_side_effect_test.cpp
    guild_warehouse_leave_side_effect_test.cpp
    guild_move_side_effect_test.cpp
    avatar_takeoff_side_effect_test.cpp
    avatar_change_side_effect_test.cpp
    chase_use_side_effect_test.cpp
    shop_item_seal_side_effect_test.cpp
    shop_item_shout_side_effect_test.cpp
    item_upgrade_side_effect_test.cpp
    item_reinforce_side_effect_test.cpp
    shop_item_name_change_side_effect_test.cpp
    shop_item_chase_side_effect_test.cpp
    item_unseal_side_effect_test.cpp
    revive_other_side_effect_test.cpp
    revive_other_ack_side_effect_test.cpp
    avatar_use_side_effect_test.cpp
    job_change_side_effect_test.cpp
    reinforce_reset_side_effect_test.cpp
    rare_create_side_effect_test.cpp
    shout_pass_through_side_effect_test.cpp
    char_change_side_effect_test.cpp
    reinforce_with_shop_item_side_effect_test.cpp
    titan_delegate_side_effect_test.cpp
    revive_other_nack_side_effect_test.cpp
    curse_cancellation_side_effect_test.cpp
    additem_slot_lock_side_effect_test.cpp
    release_slot_lock_side_effect_test.cpp
    unique_item_mix_side_effect_test.cpp
)
target_link_libraries(mxh_shop_item_manager_tests PRIVATE
    mxh_server gtest_main
)
target_include_directories(mxh_shop_item_manager_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
include(GoogleTest)
gtest_add_tests(TARGET mxh_shop_item_manager_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/unit
    PRE_SHIP_BUILD
)


































































