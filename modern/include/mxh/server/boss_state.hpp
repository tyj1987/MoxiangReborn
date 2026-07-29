// boss_state.hpp - boss stage state machine helpers.
//
// 1:1 port of legacy [Server]Map/BossState.h CBossState transition table.
// The legacy CBossState is a per-boss enum + chat queue; modern port
// keeps the enum + helpers as pure functions and lets the boss manager
// own the speech queue.

#pragma once

#include "mxh/server/monster.hpp"
#include <cstdint>

namespace mxh::server {

// Phase of the boss combat (1:1 with legacy CBossState::ePhase).
enum class BossPhase : std::uint8_t {
    Sealed      = 0,   // not yet engaged
    Intro       = 1,   // entrance speech + entrance attack
    Combat      = 2,   // normal combat
    Enraged     = 3,   // HP < 75%
    Phase2      = 4,   // HP < 50%
    Rage        = 5,   // HP < 25%
    Dying       = 6,   // death animation in progress
    Dead        = 7,   // dead
    Recovering  = 8,   // post-death respawn cooldown
};

// Map hp% to phase. 1:1 with legacy CBossState::update().
BossPhase boss_phase_from_hp(std::uint32_t current_hp, std::uint32_t max_hp) noexcept;

// Should the boss enter a new stage given the current AI state and HP%?
// Returns the new BossPhase if a transition should fire, current otherwise.
BossPhase boss_phase_transition(BossPhase current,
                                 std::uint32_t current_hp,
                                 std::uint32_t max_hp) noexcept;

// Has the boss finished dying? Pure function (true when current=DeathComplete).
bool boss_phase_is_terminal(BossPhase p) noexcept;

}  // namespace mxh::server