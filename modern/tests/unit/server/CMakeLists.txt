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

add_executable(mxh_agent_constants_sweep_tests
    agent_constants_sweep_test.cpp
)
target_link_libraries(mxh_agent_constants_sweep_tests PRIVATE mxh_server GTest::gtest_main)
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
add_test(NAME CategoryInvariant.GTournamentIs59 COMMAND mxh_cheat_byte_sweep_tests --gtest_filter=CategoryInvariant.GTournamentIs59)
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