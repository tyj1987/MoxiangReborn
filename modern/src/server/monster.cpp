// monster.cpp - AI state machine implementation + Phase 6.2 boss stage helpers.

#include "mxh/server/monster.hpp"

namespace mxh::server {

// ---- Helpers ----
// ---- should_flee ----
bool should_flee(std::uint32_t current_hp, std::uint32_t max_hp, std::uint8_t flee_pct) noexcept {
    if (flee_pct == 0) return false;
    if (max_hp == 0) return false;
    // hp_pct = current*100 / max; flee if hp_pct < flee_pct
    // Using integer math to match legacy (legacy: (life*100/maxlife) < flee)
    return (current_hp * 100u) / max_hp < flee_pct;
}

// ---- transition_idle_to_chase ----
AiState transition_idle_to_chase(const MonsterInstance& m,
                                  std::uint32_t now_ms,
                                  std::uint32_t player_id) noexcept {
    (void)now_ms;
    if (m.ai_state != AiState::Stand && m.ai_state != AiState::Walk &&
        m.ai_state != AiState::None)
        return m.ai_state;
    if (player_id == 0) return m.ai_state;
    if (m.behavior.search_radius == 0) return m.ai_state;
    // Approximate aggro: by policy we always chase once a non-zero player_id
    // arrives within the AI tick (real geometry check is the responsibility
    // of the map handler; this layer only owns the transition logic).
    return AiState::Run;
}

// ---- transition_chase_to_attack ----
AiState transition_chase_to_attack(const MonsterInstance& m, std::uint32_t now_ms) noexcept {
    if (m.ai_state != AiState::Run) return m.ai_state;
    if (m.target_player_id == 0) return m.ai_state;
    if (!attack_cooldown_elapsed(m, now_ms)) return m.ai_state;
    return AiState::Attack;
}

bool attack_cooldown_elapsed(const MonsterInstance& m, std::uint32_t now_ms) noexcept {
    std::uint32_t cooldown = m.behavior.attack_interval_ms == 0
        ? 2000u : m.behavior.attack_interval_ms;  // legacy default 2 s
    if (m.last_attack_ms == 0) return true;
    return now_ms >= m.last_attack_ms + cooldown;
}

// ---- transition_on_damage ----
AiState transition_on_damage(MonsterInstance& m,
                               std::uint32_t damage, std::uint32_t now_ms) noexcept {
    // Apply damage
    if (damage >= m.current_hp) {
        m.current_hp = 0;
    } else {
        m.current_hp -= damage;
    }
    if (m.current_hp == 0) {
        m.ai_state = AiState::Die;
        m.state_entered_ms = now_ms;
        m.state = 2;  // eObjectState::Die
        return AiState::Die;
    }
    if (m.behavior.flee_hp_percent > 0 && should_flee(m.current_hp, m.max_hp, m.behavior.flee_hp_percent)) {
        m.ai_state = AiState::RunAway;
        m.state_entered_ms = now_ms;
        return AiState::RunAway;
    }
    // Otherwise stay in current state. A chasing monster keeps Run;
    // an attacking monster keeps Attack; idle stays idle.
    return m.ai_state;
}

// ---- kill_monster ----
void kill_monster(MonsterInstance& m, std::uint32_t now_ms) noexcept {
    m.current_hp = 0;
    m.ai_state = AiState::Die;
    m.state = 2;  // eObjectState::Die
    m.state_entered_ms = now_ms;
}

// ---- ai_tick ----
AiState ai_tick(MonsterInstance& m, std::uint32_t now_ms) noexcept {
    // Die is terminal: stays Die forever.
    if (m.ai_state == AiState::Die) {
        return AiState::Die;
    }
    // RunAway has a chance to recover to Stand once HP exceeds flee pct.
    if (m.ai_state == AiState::RunAway) {
        if (!should_flee(m.current_hp, m.max_hp, m.behavior.flee_hp_percent)) {
            m.ai_state = AiState::Stand;
            m.state_entered_ms = now_ms;
        }
        return m.ai_state;
    }
    // On the very first tick from None, transition to Walk (legacy CAISystem path: just spawned, start patrolling).
    if (m.ai_state == AiState::None) {
        m.ai_state = AiState::Walk;
        m.state_entered_ms = now_ms;
        return AiState::Walk;
    }
    // If we have a target, attempt chase -> attack transition.
    if (m.target_player_id != 0) {
        if (m.ai_state == AiState::Stand || m.ai_state == AiState::Walk) {
            m.ai_state = transition_idle_to_chase(m, now_ms, m.target_player_id);
            m.state_entered_ms = now_ms;
            return m.ai_state;
        }
        if (m.ai_state == AiState::Run) {
            auto next = transition_chase_to_attack(m, now_ms);
            if (next != m.ai_state) {
                m.ai_state = next;
                m.state_entered_ms = now_ms;
                if (next == AiState::Attack) m.last_attack_ms = now_ms;
            }
            return m.ai_state;
        }
        if (m.ai_state == AiState::Attack) {
            // After a hit, fall back to Run to chase the target.
            m.ai_state = AiState::Run;
            m.state_entered_ms = now_ms;
            m.last_attack_ms = now_ms;
            return AiState::Run;
        }
        if (m.ai_state == AiState::Skill) {
            // After a skill, fall back to Run to chase.
            m.ai_state = AiState::Run;
            m.state_entered_ms = now_ms;
            return AiState::Run;
        }
    }
    // No target: any active state falls back to Stand (legacy CAISystem leash).
    if (m.target_player_id == 0) {
        if (m.ai_state == AiState::Run || m.ai_state == AiState::Attack ||
            m.ai_state == AiState::Skill) {
            m.ai_state = AiState::Stand;
            m.state_entered_ms = now_ms;
            return AiState::Stand;
        }
    }
    return m.ai_state;
}

// ---- boss_stage_from_hp ----
// 1:1 with the legacy CBossMonster stage table.
//   HP% 100..76  -> 0 normal
//   HP%  75..51  -> 1 enraged
//   HP%  50..26  -> 2 phase2
//   HP%  25..1   -> 3 rage
//   HP%        0 -> 4 dead (caller checks)
std::uint32_t boss_stage_from_hp(std::uint32_t current_hp, std::uint32_t max_hp) noexcept {
    if (max_hp == 0) return 0;
    const std::uint32_t pct = (current_hp * 100u) / max_hp;
    if (pct == 0)            return 4;  // dead stage (caller handles die)
    else if (pct >= 76)      return 0;
    else if (pct >= 51)      return 1;
    else if (pct >= 26)      return 2;
    else /* pct in 1..25 */  return 3;
}

// ---- update_boss_stage ----
void update_boss_stage(BossMonsterInstance& b,
                       std::uint32_t now_ms,
                       std::uint32_t shout_interval_ms) noexcept {
    std::uint32_t target_stage = boss_stage_from_hp(b.base.current_hp, b.base.max_hp);
    if (target_stage == 4) {
        // HP went to 0: ensure boss is in Die state.
        if (b.base.ai_state != AiState::Die) {
            b.base.current_hp = 0;
            b.base.ai_state = AiState::Die;
            b.base.state = 2;
            b.base.state_entered_ms = now_ms;
            b.stage = 4;
        }
        return;
    }
    // Stage transition? (only update on change to avoid spamming)
    if (target_stage != b.stage) {
        b.stage = target_stage;
        // Each stage transition resets the attack timer so the boss
        // immediately uses its new skill set.
        b.base.last_attack_ms = now_ms;
        // Reset shout cooldown so the boss can speak on entry.
        b.last_shout_ms = 0;
    }
    // Speak periodically.
    if (shout_interval_ms > 0 && b.speech_id != 0 &&
        (now_ms - b.last_shout_ms) >= shout_interval_ms) {
        // In real engine, cMonsterSpeechManager broadcasts to map channel.
        // Here we simply bump the counter; the caller observes the change.
        b.last_shout_ms = now_ms;
    }
}

}  // namespace mxh::server

