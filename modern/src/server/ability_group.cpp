// ability_group.cpp

#include "mxh/server/ability_group.hpp"

namespace mxh::server {

bool AbilityGroup::push(const EffectEntry& e) noexcept {
    effects_.push_back(e);
    return true;
}

bool AbilityGroup::remove(std::uint32_t effect_idx) noexcept {
    for (auto it = effects_.begin(); it != effects_.end(); ++it) {
        if (it->effect_idx == effect_idx) {
            effects_.erase(it);
            return true;
        }
    }
    return false;
}

void AbilityGroup::tick(std::uint32_t now_ms) noexcept {
    std::vector<EffectEntry> kept;
    kept.reserve(effects_.size());
    for (const auto& e : effects_) {
        if (now_ms >= e.start_ms + e.duration_ms) continue;
        kept.push_back(e);
    }
    effects_.swap(kept);
}

std::uint32_t AbilityGroup::total_value(std::uint8_t kind) const noexcept {
    std::uint32_t sum = 0;
    for (const auto& e : effects_) if (e.kind == kind) sum += e.value;
    return sum;
}

void DelayGroup::activate(std::uint32_t skill_idx, std::uint32_t now_ms) noexcept {
    std::uint32_t i = skill_idx % 100u;
    last_idx_[i]    = skill_idx;
    last_used_ms_[i] = now_ms;
}

void DelayGroup::clear(std::uint32_t skill_idx) noexcept {
    std::uint32_t i = skill_idx % 100u;
    if (last_idx_[i] == skill_idx) {
        last_used_ms_[i] = 0;
        last_idx_[i] = 0;
    }
}

std::uint32_t DelayGroup::remaining_ms(std::uint32_t skill_idx, std::uint32_t now_ms,
                                         std::uint32_t delay_ms) const noexcept {
    std::uint32_t i = skill_idx % 100u;
    if (last_idx_[i] != skill_idx) return 0;
    if (last_used_ms_[i] == 0) return 0;
    std::uint32_t elapsed = now_ms - last_used_ms_[i];
    if (elapsed >= delay_ms) return 0;
    return delay_ms - elapsed;
}

}  // namespace mxh::server
