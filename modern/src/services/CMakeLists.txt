# mxh_services: Phase 13.2 real service implementations (header-only)
# -----------------------------------------------------------------------------
# Real implementations of the IInventoryService / ISkillService /
# IPlayerStatsService / IQuestService / ITradeService / IMoveService
# interfaces under modern/include/mxh/services/.  The implementations
# are header-only (no .cpp files) because they are trivial adapters
# over wire-format structs (ItemTotalInfo, PlayerCombatStats,
# std::vector<LearnedSkill>, Player+QuestLog, commit-callback,
# std::vector<MovePoint>).  The library exists as an INTERFACE so
# consumers can link against a single target name; adding .cpp files
# later (e.g. when the binary loaders for SkillList.bin /
# character_mugong.bin land) requires only extending this file.
# -----------------------------------------------------------------------------
# src/ui is on the INTERFACE include path so TradeServiceImpl can
# #include "cdealdialog.hpp" for the full DealItem definition (the
# ITradeService header forward-declares mxh::ui::DealItem only; the
# implementation needs the complete POD to honor the const-ref vector
# parameter in completeTrade()).  This keeps services free of any
# extra source file while still being able to consume the UI-side
# data shapes it brokers.
# -----------------------------------------------------------------------------
add_library(mxh_services INTERFACE)

target_include_directories(mxh_services INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/ui
)

install(TARGETS mxh_services ARCHIVE DESTINATION lib)
