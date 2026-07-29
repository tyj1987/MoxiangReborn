// boss_monster_manager.cpp

#include "mxh/server/boss_monster_manager.hpp"

namespace mxh::server {

void BossMonsterManager::register_info(std::uint32_t monster_kind, const BossMonsterInfo& info) noexcept {
    infos_[monster_kind] = info;
}

std::uint32_t BossMonsterManager::spawn(std::uint32_t monster_kind,
                                          const mxh::game::MonsterTemplate& tpl,
                                          std::uint32_t object_id,
                                          std::int32_t spawn_x, std::int32_t spawn_y, std::int32_t spawn_z,
                                          std::uint16_t map_num) noexcept {
    auto it = infos_.find(monster_kind);
    if (it == infos_.end()) return 0;  // no static info: cannot spawn
    BossMonsterInstance inst = create_boss_from_template(monster_kind, tpl, it->second,
                                                          object_id, spawn_x, spawn_y, spawn_z, map_num);
    bosses_[object_id] = inst;
    return object_id;
}

BossPhase BossMonsterManager::damage(std::uint32_t object_id,
                                       std::uint32_t damage,
                                       std::uint32_t attacker_player_id,
                                       std::uint32_t now_ms) noexcept {
    auto it = bosses_.find(object_id);
    if (it == bosses_.end()) return BossPhase::Sealed;
    return apply_boss_damage(it->second, damage, attacker_player_id, now_ms);
}

bool BossMonsterManager::erase(std::uint32_t object_id) noexcept {
    return bosses_.erase(object_id) > 0;
}

const BossMonsterInstance* BossMonsterManager::find(std::uint32_t object_id) const noexcept {
    auto it = bosses_.find(object_id);
    return it == bosses_.end() ? nullptr : &it->second;
}

BossMonsterInstance* BossMonsterManager::find(std::uint32_t object_id) noexcept {
    auto it = bosses_.find(object_id);
    return it == bosses_.end() ? nullptr : &it->second;
}

const BossMonsterInfo* BossMonsterManager::info_for(std::uint32_t monster_kind) const noexcept {
    auto it = infos_.find(monster_kind);
    return it == infos_.end() ? nullptr : &it->second;
}

}  // namespace mxh::server