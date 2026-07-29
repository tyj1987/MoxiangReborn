// boss_monster.hpp - 1:1 port of legacy [Server]Map/BossMonster.h CBossMonster.
//
// Legacy CBossMonster extends CMonster and adds stage / rage / shout.
// Modern port keeps the runtime state in BossMonsterInstance (declared
// in monster.hpp) and provides pure-function helpers here for stage
// transitions, rage-target and shout scheduling.
//
// Builder helpers (create_boss_from_template / create_boss_instance)
// let callers spawn a boss from a BASE_MONSTER_LIST template without
// touching the manager.

#pragma once

#include "mxh/server/boss_monster_info.hpp"
#include "mxh/server/boss_state.hpp"
#include "mxh/server/monster.hpp"
#include "mxh/game/monster_types.hpp"
#include <cstdint>
#include <vector>

namespace mxh::server {

// Allocate a BossMonsterInstance by copying fields from a MonsterTemplate
// and applying the per-MonsterKind BossMonsterInfo. The returned object
// can be inserted into a BossMonsterManager.
BossMonsterInstance create_boss_from_template(std::uint32_t monster_kind,
                                                const mxh::game::MonsterTemplate& tpl,
                                                const BossMonsterInfo& info,
                                                std::uint32_t object_id,
                                                std::int32_t spawn_x,
                                                std::int32_t spawn_y,
                                                std::int32_t spawn_z,
                                                std::uint16_t map_num) noexcept;

// Apply damage to a boss: handles HP reduction, stage transitions,
// and rage-target capture. Returns the new BossPhase.
BossPhase apply_boss_damage(BossMonsterInstance& b,
                              std::uint32_t damage,
                              std::uint32_t attacker_player_id,
                              std::uint32_t now_ms) noexcept;

// Choose the rage target from a list of attacker contributions.
// Lexicographic: highest damage gets rage first; ties broken by smaller id.
std::uint32_t pick_rage_target(const std::vector<std::pair<std::uint32_t, std::uint32_t>>& damage_by_player) noexcept;

}  // namespace mxh::server