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
add_test(NAME mxh_server_system_tests COMMAND mxh_server_system_tests)

add_executable(mxh_agent_network_msg_parser_tests
    agent_network_msg_parser_test.cpp
)
target_link_libraries(mxh_agent_network_msg_parser_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_agent_network_msg_parser_tests COMMAND mxh_agent_network_msg_parser_tests)

add_executable(mxh_agent_db_msg_parser_tests
    agent_db_msg_parser_test.cpp
)
target_link_libraries(mxh_agent_db_msg_parser_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_agent_db_msg_parser_tests COMMAND mxh_agent_db_msg_parser_tests)

add_executable(mxh_mh_time_manager_tests
    mh_time_manager_test.cpp
)
target_link_libraries(mxh_mh_time_manager_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_mh_time_manager_tests COMMAND mxh_mh_time_manager_tests)

add_executable(mxh_common_network_msg_parser_tests
    common_network_msg_parser_test.cpp
)
target_link_libraries(mxh_common_network_msg_parser_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_common_network_msg_parser_tests COMMAND mxh_common_network_msg_parser_tests)

add_executable(mxh_crypt_tests
    crypt_test.cpp
)
target_link_libraries(mxh_crypt_tests PRIVATE mxh_server mxh_crypto GTest::gtest_main)
add_test(NAME mxh_crypt_tests COMMAND mxh_crypt_tests)

add_executable(mxh_traffic_log_tests
    traffic_log_test.cpp
)
target_link_libraries(mxh_traffic_log_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_traffic_log_tests COMMAND mxh_traffic_log_tests)

add_executable(mxh_plustime_manager_tests
    plustime_manager_test.cpp
)
target_link_libraries(mxh_plustime_manager_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_plustime_manager_tests COMMAND mxh_plustime_manager_tests)

add_executable(mxh_jackpot_manager_agent_tests
    jackpot_manager_agent_test.cpp
)
target_link_libraries(mxh_jackpot_manager_agent_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_jackpot_manager_agent_tests COMMAND mxh_jackpot_manager_agent_tests)

add_executable(mxh_gm_tool_manager_tests
    gm_tool_manager_test.cpp
)
target_link_libraries(mxh_gm_tool_manager_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_gm_tool_manager_tests COMMAND mxh_gm_tool_manager_tests)

add_executable(mxh_attack_manager_tests
    attack_manager_test.cpp
)
target_link_libraries(mxh_attack_manager_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_attack_manager_tests COMMAND mxh_attack_manager_tests)

add_executable(mxh_object_event_tests
    object_event_test.cpp
)
target_link_libraries(mxh_object_event_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_object_event_tests COMMAND mxh_object_event_tests)

add_executable(mxh_object_state_manager_tests
    object_state_manager_test.cpp
)
target_link_libraries(mxh_object_state_manager_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_object_state_manager_tests COMMAND mxh_object_state_manager_tests)

add_executable(mxh_mh_file_reader_tests
    mh_file_reader_test.cpp
)
target_link_libraries(mxh_mh_file_reader_tests PRIVATE mxh_server mxh_compat GTest::gtest_main)
add_test(NAME mxh_mh_file_reader_tests COMMAND mxh_mh_file_reader_tests)

add_executable(mxh_object_factory_tests
    object_factory_test.cpp
)
target_link_libraries(mxh_object_factory_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_object_factory_tests COMMAND mxh_object_factory_tests)

add_executable(mxh_ai_define_tests
    ai_define_test.cpp
)
target_link_libraries(mxh_ai_define_tests PRIVATE mxh_server GTest::gtest_main)
add_test(NAME mxh_ai_define_tests COMMAND mxh_ai_define_tests)
