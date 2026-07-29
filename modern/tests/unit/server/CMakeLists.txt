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



add_executable(mxh_map_object_tests
    map_object_test.cpp
)
target_link_libraries(mxh_map_object_tests PRIVATE mxh_server GTest::gtest_main)
gtest_add_tests(TARGET mxh_map_object_tests
    WORKING_DIRECTORY \/tests/unit)
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

add_executable(mxh_byte_match_sweep_tests
    byte_match_sweep_test.cpp
)
target_link_libraries(mxh_byte_match_sweep_tests PRIVATE mxh_server GTest::gtest_main)
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
