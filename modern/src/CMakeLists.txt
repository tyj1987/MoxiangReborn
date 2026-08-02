add_library(mxh_compat STATIC
    mh_file_ex.cpp
    pack_file.cpp
    bmhm_map.cpp
    ttb_tile_table.cpp
    chx_model.cpp
    chr_motion.cpp
    bsad_area.cpp
    item_effects.cpp
)

target_include_directories(mxh_compat PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_compat PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(mxh_compat PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_compat PRIVATE /FS /MP)
endif()

# 安装（可选）
install(TARGETS mxh_compat
    ARCHIVE DESTINATION lib
)

# -----------------------------------------------------------------------------
# mxh_db: Database abstraction layer (Phase 2)
# SQLite source: vcpkg (find_package) > vendored amalgamation
# -----------------------------------------------------------------------------

# Check if vcpkg/system provides SQLite3.
find_package(SQLite3 QUIET)

if(SQLite3_FOUND)
    message(STATUS "SQLite3: provided by vcpkg/system (${SQLite3_VERSION})")
    add_library(mxh_db STATIC
        db_adapter.cpp
        sqlite_adapter.cpp
        db_factory.cpp
        mssql_odbc_adapter.cpp
    )
    target_link_libraries(mxh_db PRIVATE SQLite::SQLite3)
    target_include_directories(mxh_db PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )
else()
    message(STATUS "SQLite3: vendored amalgamation (third_party/sqlite)")
    add_library(mxh_db STATIC
        db_adapter.cpp
        sqlite_adapter.cpp
        db_factory.cpp
        mssql_odbc_adapter.cpp
        ../third_party/sqlite/sqlite3.c
    )
    target_include_directories(mxh_db PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/sqlite
    )
endif()

# Phase 10.2: Microsoft SQL Server via native Windows ODBC (odbc32.lib).
# Mirrors the legacy [Lib]DBThread ODBC layer. Windows-only — Linux would
# use unixODBC + libiodbc; out of scope for now (the game is Win32-only).
if(WIN32)
    target_link_libraries(mxh_db PRIVATE odbc32)
endif()

target_compile_features(mxh_db PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(mxh_db PUBLIC
        _CRT_SECURE_NO_WARNINGS
        SQLITE_ENABLE_JSON1
        SQLITE_THREADSAFE=1
    )
    if(NOT SQLite3_FOUND)
        # SQLite amalgamation produces a wall of warnings under /W4.
        target_compile_options(mxh_db PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/W3>
        )
    endif()
endif()

install(TARGETS mxh_db
    ARCHIVE DESTINATION lib
)

# -----------------------------------------------------------------------------
# mxh_net: Networking layer (Phase 4 + Phase 9 cross-platform)
# -----------------------------------------------------------------------------
add_library(mxh_net STATIC
    net/net.cpp
    net/socket.cpp
    net/iocp/iocp.cpp
)

target_include_directories(mxh_net PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_net PUBLIC cxx_std_20)
# Phase 10.11: mxh_net now also links mxh_monitor because iocp.cpp
# records message received/sent/connected metrics via the
# PerformanceMonitor singleton. PUBLIC so downstream tools that link
# mxh_net (server tools, 5/5 matrix) also pick up the monitor dep.
target_link_libraries(mxh_net PUBLIC mxh_compat mxh_monitor)

if(WIN32)
    # Ws2_32 for sockets; Mswsock for AcceptEx / GetAcceptExSockaddrs
    # (used by mxh::net::iocp — Phase 10.11 fix for missing imports).
    target_link_libraries(mxh_net PRIVATE Ws2_32 Mswsock)
endif()

install(TARGETS mxh_net ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_crypto: HSEL replacement (Phase 3) — AES-256-GCM via Windows CNG
# -----------------------------------------------------------------------------
add_library(mxh_crypto STATIC
    crypto/crypto.cpp
    crypto/hsel_stream.cpp
    crypto/hsel_class.cpp
    crypto/hackshield_stub.cpp
    crypto/ggsrv25_vendor_stub.cpp
)

target_include_directories(mxh_crypto PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_crypto PUBLIC cxx_std_20)
target_link_libraries(mxh_crypto PUBLIC mxh_net)

# bcrypt.dll is loaded dynamically — no static .lib or SDK header needed.

install(TARGETS mxh_crypto ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_server: Minimal game server (Distribute + Agent + Map stubs)
# -----------------------------------------------------------------------------
add_library(mxh_server STATIC
    server/help_request_manager.cpp
    server/login_handler.cpp
    server/agent_handler.cpp
    server/map_handler.cpp
    server/character_calc_manager.cpp
    server/player.cpp
    server/player_state.cpp
    server/monster.cpp
    server/boss_state.cpp
    server/boss_monster.cpp
    server/boss_monster_manager.cpp
    server/field_boss_monster.cpp
    server/c_monster_speech_manager.cpp
    server/drop_item.cpp
    server/skill_manager.cpp
    server/item_container.cpp
    server/item_slot.cpp
    server/ability_group.cpp
    server/note_manager.cpp
    server/wanted_manager.cpp
    server/murimnet_channelling_manager.cpp
    server/murimnet_play_room.cpp
    server/murimnet_play_room_manager.cpp
    server/murimnet_player.cpp
    server/murimnet_runtime.cpp
    server/murimnet_channel.cpp
    server/murimnet_wire_serializer.cpp
    server/murimnet_crypt.cpp
    server/distribute_server.cpp
    server/mn_server.cpp
    server/login_server_db_msg_parser.cpp
    server/server_list.cpp
    server/distribute_network_msg_parser.cpp
    server/showdown_manager.cpp
    server/pk_manager.cpp
    server/money_manager.cpp
    server/quest_manager.cpp
    server/quest_execute.cpp
    server/item_manager.cpp
    server/party_manager.cpp
    server/guild_manager.cpp
    server/pet_manager.cpp
    server/exchange_manager.cpp
    server/titan_manager.cpp
    server/titan_item_manager.cpp
    server/street_stall_manager.cpp
    server/pyo_guk_manager.cpp
    server/friend_manager.cpp
    server/bobusang_manager.cpp
    server/server_system.cpp
    server/agent_network_msg_parser.cpp
    server/agent_db_msg_parser.cpp
    server/mh_time_manager.cpp
    server/common_network_msg_parser.cpp
    server/crypt.cpp
    server/traffic_log.cpp
    server/attack_manager.cpp
    server/object_event.cpp
    server/object.cpp
    server/ai_system.cpp
    server/map_object.cpp
    server/object_state_manager.cpp
    server/object_factory.cpp
    server/ai_param.cpp
    server/ai_group_prototype.cpp
    server/economy.cpp
    server/mh_error.cpp
    server/fame_manager.cpp
    server/purse.cpp
    server/regen_condition_info.cpp
    server/regen_prototype.cpp
    server/state_param.cpp
    server/summon_monster.cpp
    server/agent_note.cpp
    server/fixed_tile.cpp
    server/fixed_tile_info.cpp
    server/tile.cpp
    server/tactic_ability_info.cpp
    server/agent_userconn.cpp
    server/agent_powerup.cpp
    server/agent_cheat.cpp
    server/agent_gtournament_server.cpp
    server/agent_battle.cpp
    server/agent_quest.cpp
    server/agent_mugong.cpp
    server/agent_move.cpp
    server/agent_user.cpp
    server/agent_friend.cpp
    server/agent_autonote.cpp
    server/agent_weather.cpp
    server/agent_itemlimit.cpp
    server/agent_fortwar.cpp
    server/agent_survival.cpp
    server/agent_wanted.cpp
    server/agent_bobusang_user.cpp
    server/agent_gtournament.cpp
    server/agent_siegewarprofit.cpp
    server/agent_guild_union.cpp
    server/agent_siegewar.cpp
    server/agent_siegewar_server.cpp
    server/agent_simple_auth_forward.cpp
    server/agent_hackcheck.cpp
    server/plustime_manager.cpp
    server/jackpot_manager_agent.cpp
    server/gm_tool_manager.cpp
    server/mh_file_reader.cpp
    server/agent_chat.cpp
    server/agent_skill.cpp
    server/agent_guild.cpp
    server/agent_murimnet.cpp
    server/agent_party.cpp
    server/agent_item.cpp
    server/agent_packed.cpp
    server/agent_note.cpp
    server/fixed_tile.cpp
    server/fixed_tile_info.cpp
    server/tile.cpp
    server/tactic_ability_info.cpp
    server/agent_friend.cpp
    server/agent_autonote.cpp
    server/agent_weather.cpp
    server/agent_itemlimit.cpp
    server/agent_fortwar.cpp
    server/agent_survival.cpp
    server/agent_wanted.cpp
    server/agent_bobusang_user.cpp
    server/agent_gtournament.cpp
    server/agent_siegewarprofit.cpp
    server/agent_guild_union.cpp
    server/agent_siegewar.cpp
    server/agent_siegewar_server.cpp
    server/agent_simple_auth_forward.cpp
    server/agent_hackcheck.cpp
    server/bobusang_manager_agent.cpp
    server/survival_mode_manager.cpp
    server/regen_manager.cpp
    server/party_war_mgr.cpp
    server/distributer.cpp
    server/player_monster_point.cpp
    server/quest_regen_mgr.cpp
    server/looting_manager.cpp
    server/distribute_strategy.cpp
    server/item_drop.cpp
    server/item_limit_manager.cpp
    server/change_item_manager.cpp
    server/quest_map_mgr.cpp
    server/quest_updater.cpp
    server/cquest_base.cpp
    server/user_table.cpp
    server/server_table.cpp
    server/filtering_table.cpp
    server/skill_delay_manager.cpp
    server/msg_table.cpp
    server/punish_manager.cpp
    server/shout_manager.cpp
    server/shop_item_manager.cpp
    server/gm_power_list.cpp
    server/hackshield_manager.cpp
    server/nprotect_manager.cpp
    server/quest_group.cpp
)

target_include_directories(mxh_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_server PUBLIC cxx_std_20)
target_link_libraries(mxh_server PUBLIC mxh_net mxh_crypto mxh_db mxh_monitor)

install(TARGETS mxh_server ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_log: minimal logging facility (used by all modules)
# -----------------------------------------------------------------------------
add_library(mxh_log STATIC
    log/mlog.cpp
)
target_include_directories(mxh_log PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}              # so log/mlog.cpp finds mlog.hpp
)
target_compile_features(mxh_log PUBLIC cxx_std_20)

# -----------------------------------------------------------------------------
# mxh_memory: High-performance memory pool (Phase 12.4)
# -----------------------------------------------------------------------------
add_library(mxh_memory STATIC
    memory/memory_pool.cpp
)
target_include_directories(mxh_memory PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(mxh_memory PUBLIC cxx_std_20)
target_link_libraries(mxh_memory PUBLIC mxh_log)

install(TARGETS mxh_memory ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_monitor: Performance monitoring (Phase 12.4)
# -----------------------------------------------------------------------------
add_library(mxh_monitor STATIC
    monitor/performance_monitor.cpp
)
target_include_directories(mxh_monitor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(mxh_monitor PUBLIC cxx_std_20)
target_link_libraries(mxh_monitor PUBLIC mxh_log)

install(TARGETS mxh_monitor ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_render: DirectX 11 port of 4Dyuchi GX renderer (Phase 5)
# -----------------------------------------------------------------------------
add_subdirectory(render)

# -----------------------------------------------------------------------------
# mxh_ui: Modern C++ UI framework (Phase 6)
# -----------------------------------------------------------------------------
add_subdirectory(ui)

# -----------------------------------------------------------------------------
# mxh_services: Phase 13 service implementations (header-only)
# Real implementations of IInventoryService / ISkillService / IPlayerStatsService
# backed by ItemTotalInfo / PlayerCombatStats / std::vector<LearnedSkill>.
# See modern/src/services/*.hpp and the contract test in
# modern/tests/unit/services/real/.
# -----------------------------------------------------------------------------
add_subdirectory(services)

# Phase 6.3: AgentServer 1:1 port — UserTable (legacy CUserTable).



















