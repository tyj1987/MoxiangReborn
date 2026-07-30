// pk_manager.cpp

#include "mxh/server/pk_manager.hpp"

namespace mxh::server {

bool PKManager::set_peace(std::uint32_t player_id) noexcept {
    auto* s = find(player_id);
    if (!s) { PkState n{}; n.player_id=player_id; n.mode = PkMode::Peace; states_.push_back(n); s = &states_.back(); return true; }
    s->mode = PkMode::Peace;
    s->penalty_until_ms = 0;
    return true;
}

bool PKManager::allow_pk(std::uint32_t player_id) noexcept {
    if (find(player_id) == nullptr) set_peace(player_id);
    auto* s = find(player_id);
    if (!s) return false;
    s->mode = PkMode::PkAllow;
    return true;
}

bool PKManager::on_kill(std::uint32_t player_id, std::uint32_t now_ms) noexcept {
    if (find(player_id) == nullptr) set_peace(player_id);
    auto* s = find(player_id);
    if (!s) return false;
    s->kill_count += 1;
    if (s->kill_count >= MXH_PK_KILLER_LIMIT) {
        s->mode = PkMode::KillerPenalty;
        s->penalty_until_ms = now_ms + MXH_PK_PENALTY_MS;
    }
    return true;
}

bool PKManager::tick(std::uint32_t now_ms) noexcept {
    bool changed = false;
    for (auto& s : states_) {
        if (s.mode == PkMode::KillerPenalty && s.penalty_until_ms != 0 &&
            now_ms >= s.penalty_until_ms) {
            s.mode = PkMode::Peace;
            s.penalty_until_ms = 0;
            s.kill_count = 0;
            changed = true;
        }
    }
    return changed;
}

PkState* PKManager::find(std::uint32_t player_id) noexcept {
    for (auto& s : states_) if (s.player_id == player_id) return &s;
    return nullptr;
}

const PkState* PKManager::find(std::uint32_t player_id) const noexcept {
    for (const auto& s : states_) if (s.player_id == player_id) return &s;
    return nullptr;
}

// -------- PKLootingManager --------

bool PKLootingManager::set_drops(std::uint32_t victim_id, const std::vector<LootDrop>& drops) noexcept {
    if (drops.empty()) return false;
    for (auto& t : tables_) {
        if (t.victim_id == victim_id) { t.drops = drops; return true; }
    }
    Table t{};
    t.victim_id = victim_id;
    t.drops = drops;
    tables_.push_back(std::move(t));
    return true;
}

std::uint32_t PKLootingManager::roll(std::uint32_t victim_id, std::uint32_t rng_value) const noexcept {
    const Table* t = nullptr;
    for (const auto& x : tables_) if (x.victim_id == victim_id) { t = &x; break; }
    if (!t) return 0;
    std::uint32_t total = 0;
    for (const auto& d : t->drops) total += d.ratio;
    if (total == 0) return 0;
    std::uint32_t pick = rng_value % total;
    std::uint32_t accum = 0;
    for (const auto& d : t->drops) {
        accum += d.ratio;
        if (pick < accum) return d.item_idx;
    }
    return t->drops.back().item_idx;
}

}  // namespace mxh::server
