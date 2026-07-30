// wanted_manager.cpp

#include "mxh/server/wanted_manager.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

bool WantedManager::register_target(std::uint32_t target_id, const std::string& target_name,
                                       std::uint32_t money, std::uint32_t now_ms) noexcept {
    if (target_id == 0) return false;
    WantedEntry e{};
    e.wanted_idx  = next_idx_++;
    e.target_id   = target_id;
    e.money       = money;
    e.register_ms = now_ms;
    e.end_ms      = now_ms + 7ULL * 24ULL * 3600000ULL;  // 7 days
    std::size_t k = std::min<std::size_t>(target_name.size(), 16);
    std::memcpy(e.target_name, target_name.data(), k);
    e.target_name[k] = '\0';
    entries_.push_back(e);
    return true;
}

bool WantedManager::claim(std::uint32_t wanted_idx, std::uint32_t killer_id) noexcept {
    auto* w = find(wanted_idx);
    if (!w) return false;
    if (w->killer_id != 0) return false;  // already claimed
    const_cast<WantedEntry*>(w)->killer_id = killer_id;
    return true;
}

bool WantedManager::remove(std::uint32_t wanted_idx) noexcept {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->wanted_idx == wanted_idx) { entries_.erase(it); return true; }
    }
    return false;
}

const WantedEntry* WantedManager::find(std::uint32_t wanted_idx) const noexcept {
    for (const auto& e : entries_) if (e.wanted_idx == wanted_idx) return &e;
    return nullptr;
}

std::vector<WantedEntry> WantedManager::snapshot() const noexcept {
    return entries_;
}

// -------- WantNpcManager --------

bool WantNpcManager::register_bounty(std::uint32_t player_id, std::uint32_t npc_unique,
                                       std::uint16_t npc_job, std::uint32_t now_ms) noexcept {
    if (player_id == 0 || npc_unique == 0) return false;
    WantNpcEntry e{};
    e.player_id   = player_id;
    e.npc_unique  = npc_unique;
    e.npc_job     = npc_job;
    e.state       = WantNpcState::Progress;
    e.register_ms = now_ms;
    entries_.push_back(e);
    return true;
}

bool WantNpcManager::set_state(std::uint32_t player_id, std::uint32_t npc_unique,
                                  WantNpcState new_state, std::uint32_t now_ms) noexcept {
    for (auto& e : entries_) {
        if (e.player_id == player_id && e.npc_unique == npc_unique) {
            e.state = new_state;
            if (new_state == WantNpcState::Complete) e.complete_ms = now_ms;
            return true;
        }
    }
    return false;
}

std::vector<WantNpcEntry> WantNpcManager::list_for_player(std::uint32_t player_id) const noexcept {
    std::vector<WantNpcEntry> out;
    for (const auto& e : entries_) if (e.player_id == player_id) out.push_back(e);
    return out;
}

}  // namespace mxh::server
