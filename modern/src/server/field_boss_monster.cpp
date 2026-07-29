// field_boss_monster.cpp

#include "mxh/server/field_boss_monster.hpp"

namespace mxh::server {

void FieldBossMonsterManager::configure_channel(std::size_t idx,
                            std::uint32_t monster_kind,
                            std::uint32_t respawn_interval_ms,
                            std::uint32_t first_spawn_ms,
                            std::int32_t spawn_x, std::int32_t spawn_y, std::int32_t spawn_z,
                            std::uint16_t map_num) noexcept {
    if (idx >= channels_.size()) channels_.resize(idx + 1);
    auto& ch = channels_[idx];
    ch.monster_kind        = monster_kind;
    ch.respawn_interval_ms = respawn_interval_ms;
    ch.next_spawn_ms       = first_spawn_ms;
    ch.spawn_x             = spawn_x;
    ch.spawn_y             = spawn_y;
    ch.spawn_z             = spawn_z;
    ch.map_num             = map_num;
    ch.active              = 0;
    ch.current_object_id   = 0;
}

bool FieldBossMonsterManager::tick(std::uint32_t now_ms,
                const std::vector<mxh::game::MonsterTemplate>& templates,
                std::uint32_t next_object_id) noexcept {
    bool spawned = false;
    for (auto& ch : channels_) {
        if (ch.monster_kind == 0) continue;
        if (ch.active) continue;             // alive, no spawn
        if (ch.respawn_interval_ms == 0) continue;  // no respawn configured
        if (now_ms < ch.next_spawn_ms) continue;   // not yet
        // Find template.
        const mxh::game::MonsterTemplate* tpl = nullptr;
        for (const auto& t : templates) {
            if (static_cast<std::uint32_t>(t.MonsterKind) == ch.monster_kind) {
                tpl = &t; break;
            }
        }
        if (!tpl) continue;
        ch.active = 1;
        ch.current_object_id = next_object_id;
        ch.next_spawn_ms = now_ms + ch.respawn_interval_ms;  // schedule next
        last_spawn_object_id_ = next_object_id;
        spawned = true;
        break;  // one per tick (legacy uses multiple, but the api returns 1)
    }
    return spawned;
}

}  // namespace mxh::server