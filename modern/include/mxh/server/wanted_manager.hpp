// wanted_manager.hpp - 1:1 port of legacy [Server]Map/WantedManager.h and WantNpcManager.h.
//
// Two distinct subsystems but they live on the same code path:
//   1. CWantedManager: player-bounty log (player_id, money, killer_id, end_ms).
//   2. CWantNpcManager: per-server NPC bounty log + 3-tier state machine
//      (CANCEL / PROGRESS / COMPLETE) per (npc_idx, player_id).
//
// Both are written to the wire in SEND_WANTED_INFO/SEND_WANTNPCINFO_MSG with
// very tight byte layout that modern must match 1:1.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

// Max wanted entries per player (legacy MAX_WANTED_NUM).
inline constexpr std::uint16_t MXH_MAX_WANTED_PER_PLAYER = 5;

// Max NPC bounty entries per player (legacy MAX_WANTNPC_NUM).
inline constexpr std::uint16_t MXH_MAX_WANTNPC_PER_PLAYER = 10;

// Player-bounty entry (legacy WANTED_INFO).
struct WantedEntry final {
    std::uint32_t wanted_idx   = 0;   // legacy PKListIDX
    std::uint32_t target_id    = 0;
    std::uint32_t money        = 0;
    std::uint32_t killer_id    = 0;   // 0 = unclaimed
    std::uint32_t register_ms  = 0;
    std::uint32_t end_ms       = 0;   // auto-expire timestamp
    char          target_name[17] = {};
};

// WantedManager — per-server player-bounty registry.
class WantedManager final {
public:
    bool register_target(std::uint32_t target_id, const std::string& target_name,
                          std::uint32_t money, std::uint32_t now_ms) noexcept;
    bool claim(std::uint32_t wanted_idx, std::uint32_t killer_id) noexcept;
    bool remove(std::uint32_t wanted_idx) noexcept;
    std::size_t size() const noexcept { return entries_.size(); }
    const WantedEntry* find(std::uint32_t wanted_idx) const noexcept;
    std::vector<WantedEntry> snapshot() const noexcept;
private:
    std::vector<WantedEntry> entries_;
    std::uint32_t next_idx_ = 1;
};

// NPC bounty state machine: per (player_id, npc_unique_index).
enum class WantNpcState : std::uint8_t {
    Cancel    = 0,
    Progress  = 1,
    Complete  = 2,
};

// WantNpc entry (legacy WANTNPC_INFO).
struct WantNpcEntry final {
    std::uint32_t player_id    = 0;
    std::uint32_t npc_unique   = 0;
    std::uint16_t npc_job      = 0;
    WantNpcState  state         = WantNpcState::Cancel;
    std::uint8_t  reserved0     = 0;
    std::uint32_t register_ms  = 0;
    std::uint32_t complete_ms  = 0;
};

// WantNpcManager — per-server NPC bounty registry.
class WantNpcManager final {
public:
    bool register_bounty(std::uint32_t player_id, std::uint32_t npc_unique,
                          std::uint16_t npc_job, std::uint32_t now_ms) noexcept;
    bool set_state(std::uint32_t player_id, std::uint32_t npc_unique,
                    WantNpcState new_state, std::uint32_t now_ms) noexcept;
    std::vector<WantNpcEntry> list_for_player(std::uint32_t player_id) const noexcept;
    std::size_t size() const noexcept { return entries_.size(); }
private:
    std::vector<WantNpcEntry> entries_;
};

}  // namespace mxh::server
