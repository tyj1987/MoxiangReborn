# mxh_services: Phase 13.2 real service implementations (header-only)
# -----------------------------------------------------------------------------
# Real implementations of the IInventoryService / ISkillService /
# IPlayerStatsService interfaces under modern/include/mxh/services/.
# The implementations are header-only (no .cpp files) because they are
# trivial adapters over wire-format structs (ItemTotalInfo,
# PlayerCombatStats, std::vector<LearnedSkill>). The library exists
# as a stub so consumers (e.g. the future MapHandler dialog dispatch)
# can link against a single target name; adding .cpp files later
# (e.g. when the binary loaders for SkillList.bin / character_mugong.bin
# land) requires only extending this file.
# -----------------------------------------------------------------------------
add_library(mxh_services INTERFACE)

target_include_directories(mxh_services INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS mxh_services ARCHIVE DESTINATION lib)
