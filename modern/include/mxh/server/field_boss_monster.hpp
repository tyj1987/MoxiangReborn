// field_boss_monster.hpp - per-server registry of world bosses.
//
// 1:1 port of legacy [Server]Map/FieldBossMonster.h + FieldBossMonsterManager.h.
// Legacy has a single FieldBossMonsterManager that owns a flat array of
// "channels" with per-channel spawn timers. Modern port keeps the same
// shape via a vector of FieldBossChannel entries.

#pragma once

#include "mxh/server/boss_monster.hpp"
#include "mxh/server/boss_monster_info.hpp"
#include "mxh/game/monster_types.hpp"
#include <cstdint>
#include <vector>

namespace mxh::server {

// One world-boss channel (legacy CFieldBossMonster::CHANNELNUM).
struct FieldBossChannel final {
    std::uint32_t monster_kind       = 0;
    std::uint32_t next_spawn_ms      = 0;       // server-time of next spawn
    std::uint32_t respawn_interval_ms = 0;      // 0 = no auto respawn
    std::uint8_t  active             = 0;       // 1 if a boss is alive on this channel
    std::uint8_t  reserved0          = 0;
    std::uint16_t reserved1          = 0;
    std::uint32_t current_object_id  = 0;       // 0 = none
    std::int32_t  spawn_x            = 0;
    std::int32_t  spawn_y            = 0;
    std::int32_t  spawn_z            = 0;
    std::uint16_t map_num            = 0;
    std::uint16_t reserved2          = 0;
};

class FieldBossMonsterManager final {
public:
    // Configure a channel. If monster_kind == 0 the channel is disabled.
    void configure_channel(std::size_t idx,
                            std::uint32_t monster_kind,
                            std::uint32_t respawn_interval_ms,
                            std::uint32_t first_spawn_ms,
                            std::int32_t spawn_x, std::int32_t spawn_y, std::int32_t spawn_z,
                            std::uint16_t map_num) noexcept;

    // Schedule a spawn at the given server-time. Returns true if fired.
    bool tick(std::uint32_t now_ms,
                const std::vector<mxh::game::MonsterTemplate>& templates,
                std::uint32_t next_object_id) noexcept;

    std::size_t channel_count() const noexcept { return channels_.size(); }
    const FieldBossChannel* channel(std::size_t idx) const noexcept {
        return idx < channels_.size() ? &channels_[idx] : nullptr;
    }
    FieldBossChannel* channel(std::size_t idx) noexcept {
        return idx < channels_.size() ? &channels_[idx] : nullptr;
    }

private:
    std::vector<FieldBossChannel> channels_;
    // Reusable scratch space for spawn logging.
    std::uint32_t last_spawn_object_id_ = 0;

public:
    // Internal helper: returns the most recent spawned object_id (test-only).
    std::uint32_t last_spawn_object_id() const noexcept { return last_spawn_object_id_; }
};

}  // namespace mxh::server