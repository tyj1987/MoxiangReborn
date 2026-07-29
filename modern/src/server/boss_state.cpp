// boss_state.cpp - Phase 6.2 boss stage transitions.

#include "mxh/server/boss_state.hpp"

namespace mxh::server {

BossPhase boss_phase_from_hp(std::uint32_t current_hp, std::uint32_t max_hp) noexcept {
    if (max_hp == 0) return BossPhase::Sealed;
    if (current_hp == 0) return BossPhase::Dead;
    const std::uint32_t pct = (current_hp * 100u) / max_hp;
    if (pct >= 76) return BossPhase::Combat;
    if (pct >= 51) return BossPhase::Enraged;
    if (pct >= 26) return BossPhase::Phase2;
    return BossPhase::Rage;
}

BossPhase boss_phase_transition(BossPhase current,
                                 std::uint32_t current_hp,
                                 std::uint32_t max_hp) noexcept {
    if (current == BossPhase::Dead) return BossPhase::Dead;
    if (current == BossPhase::Sealed) {
        // First time we see HP > 0, enter Intro.
        if (current_hp > 0) return BossPhase::Intro;
        return BossPhase::Sealed;
    }
    auto desired = boss_phase_from_hp(current_hp, max_hp);
    if (current_hp == 0) return BossPhase::Dying;
    if (static_cast<std::uint8_t>(desired) > static_cast<std::uint8_t>(current) ||
        (static_cast<std::uint8_t>(desired) != static_cast<std::uint8_t>(current) &&
         desired != BossPhase::Combat && desired != BossPhase::Sealed))
        return desired;
    return current;
}

bool boss_phase_is_terminal(BossPhase p) noexcept {
    return p == BossPhase::Dead;
}

}  // namespace mxh::server