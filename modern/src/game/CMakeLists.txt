# mxh_game: Modern Moxian gameplay types (Phase D foundation).
# -----------------------------------------------------------------------------
# Phase D6.1 - 1:1 port of CommonStruct.h gameplay data structures.  This
# library is currently header-only (the types live in include/mxh/game/)
# with one .cpp (item_effects.cpp) for the linear-scaling effect
# resolver.  Future Phase D sub-phases (D1 SkillManager, D2 BattleFactory,
# D3 QuestManager, D4 商城 / 物品 / 仓库 / 邮件 / 帮派 / 队伍, D5 MurimNet)
# will add their .cpp files here and link against this library.
#
# Numeric baseline lock: every size, constant, and enum value here is
# matched against the legacy CommonStruct.h / CommonGameDefine.h / etc.
# via modern/tests/unit/game/game_types_test.cpp.  Any future change
# to a numeric baseline must be a deliberate cross-platform port, not
# a silent drift.
# -----------------------------------------------------------------------------
add_library(mxh_game STATIC
    ${CMAKE_SOURCE_DIR}/src/item_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/skill_manager.cpp
)
target_include_directories(mxh_game PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
if(MSVC)
    target_compile_options(mxh_game PRIVATE /W3 /utf-8)
endif()
