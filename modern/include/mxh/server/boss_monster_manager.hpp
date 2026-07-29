// boss_monster_manager.hpp - per-map registry of live bosses.
//
// 1:1 port of legacy [Server]Map/BossMonsterManager.h CBossMonsterManager.
// Legacy class owns a std::map<BossMonsterID, CBossMonster*> plus a list
// of static BossMonsterInfo entries. Modern port uses flat vector with
// O(1) lookup via id-index.

#pragma once

#include "mxh/server/boss_monster.hpp"
#include "mxh/server/boss_monster_info.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace mxh::server {

// Per-map boss registry.
class BossMonsterManager final {
public:
    // Register a BossMonsterInfo for a MonsterKind (call once at server start).
    void register_info(std::uint32_t monster_kind, const BossMonsterInfo& info) noexcept;

    // Spawn a new live boss from a template. Returns the assigned object_id
    // (0 on failure). If a boss with the same MonsterKind already exists it
    // is replaced (legacy CBossMonsterManager does the same).
    std::uint32_t spawn(std::uint32_t monster_kind,
                          const mxh::game::MonsterTemplate& tpl,
                          std::uint32_t object_id,
                          std::int32_t spawn_x, std::int32_t spawn_y, std::int32_t spawn_z,
                          std::uint16_t map_num) noexcept;

    // Apply damage to a live boss by object_id.
    // Returns the BossPhase after the hit.  Returns BossPhase::Sealed if id not found.
    BossPhase damage(std::uint32_t object_id,
                      std::uint32_t damage,
                      std::uint32_t attacker_player_id,
                      std::uint32_t now_ms) noexcept;

    // Erase a dead boss (called by map handler at the end of dying animation).
    bool erase(std::uint32_t object_id) noexcept;

    // Iterate / query.
    std::size_t live_count() const noexcept { return bosses_.size(); }
    const BossMonsterInstance* find(std::uint32_t object_id) const noexcept;
    BossMonsterInstance*       find(std::uint32_t object_id) noexcept;
    const BossMonsterInfo*     info_for(std::uint32_t monster_kind) const noexcept;

private:
    std::unordered_map<std::uint32_t, BossMonsterInfo> infos_;
    std::unordered_map<std::uint32_t, BossMonsterInstance> bosses_;
};

}  // namespace mxh::server