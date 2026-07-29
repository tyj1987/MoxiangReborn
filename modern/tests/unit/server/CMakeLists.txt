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

add_executable(mxh_category_byte_sweep_tests
    category_byte_sweep_test.cpp
)
target_link_libraries(mxh_category_byte_sweep_tests PRIVATE mxh_server GTest::gtest_main)
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
