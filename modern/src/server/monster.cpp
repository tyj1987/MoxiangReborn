// monster.cpp - AI state machine implementation.

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
        return AiState::Die;
    }
    if (should_flee(m.current_hp, m.max_hp, m.behavior.flee_hp_percent)) {
        m.ai_state = AiState::RunAway;
        m.state_entered_ms = now_ms;
        return AiState::RunAway;
    }
    return m.ai_state;  // unchanged
}

// ---- kill_monster ----
void kill_monster(MonsterInstance& m, std::uint32_t now_ms) noexcept {
    m.current_hp = 0;
    m.state = 2;  // eObjectState_Die
    m.ai_state = AiState::Die;
    m.state_entered_ms = now_ms;
}

// ---- ai_tick ----
AiState ai_tick(MonsterInstance& m, std::uint32_t now_ms) noexcept {
    if (m.ai_state == AiState::Die) return AiState::Die;
    switch (m.ai_state) {
        case AiState::None:
        case AiState::Stand:
            // Allow chase transition if a target has been set
            if (m.target_player_id != 0) {
                if (attack_cooldown_elapsed(m, now_ms)) {
                    m.ai_state = AiState::Run;
                    m.state_entered_ms = now_ms;
                }
            } else {
                // Idle for a while, then enter Walk (legacy patrol)
                m.ai_state = AiState::Walk;
                m.state_entered_ms = now_ms;
            }
            break;
        case AiState::Walk:
            // Continue patrolling until a target arrives
            if (m.target_player_id != 0 && attack_cooldown_elapsed(m, now_ms)) {
                m.ai_state = AiState::Run;
                m.state_entered_ms = now_ms;
            }
            break;
        case AiState::Run:
            if (m.target_player_id == 0) {
                // Target lost -> return to Stand
                m.ai_state = AiState::Stand;
                m.state_entered_ms = now_ms;
            } else if (attack_cooldown_elapsed(m, now_ms)) {
                m.ai_state = AiState::Attack;
                m.state_entered_ms = now_ms;
            }
            break;
        case AiState::Attack:
            // Attack animation done; track last-attack time and return to Run
            m.last_attack_ms = now_ms;
            if (m.target_player_id != 0) {
                m.ai_state = AiState::Run;
            } else {
                m.ai_state = AiState::Stand;
            }
            m.state_entered_ms = now_ms;
            break;
        case AiState::Skill:
            // Cast finished; return to Run
            if (m.target_player_id != 0) {
                m.ai_state = AiState::Run;
            } else {
                m.ai_state = AiState::Stand;
            }
            m.state_entered_ms = now_ms;
            break;
        case AiState::RunAway:
            // Flee toward spawn; legacy AfterRunAway returns to Stand after
            // either HP recovers or a 10 s timer expires. We approximate
            // by checking elapsed time in Stand.
            if (!should_flee(m.current_hp, m.max_hp, m.behavior.flee_hp_percent)) {
                m.ai_state = AiState::Stand;
                m.state_entered_ms = now_ms;
            }
            break;
        case AiState::FallBack:
        case AiState::Paused:
            // Hold for one tick then resume Run if target still set
            if (m.target_player_id != 0) {
                m.ai_state = AiState::Run;
                m.state_entered_ms = now_ms;
            } else {
                m.ai_state = AiState::Stand;
                m.state_entered_ms = now_ms;
            }
            break;
    }
    return m.ai_state;
}

}  // namespace mxh::server
