// monster.hpp - 1:1 port of legacy [Server]Map/Monster.h (CMonster) and
// AISystem.h (CAISystem) into pure data structs + pure functions.
//
// Source: legacy [Server]Map/Monster.h + AISystem.h + AIParam.h
// The legacy engine uses deep inheritance (CMonster : CObject : CAction)
// with 200+ members; the modern port models just the data fields and the
// state machine transitions as pure POD structs + free functions. The
// map_handler uses these to populate ServerSideMonsterInstance and drive
// the per-tick AI loop.

#pragma once

#include "mxh/game/monster_types.hpp"
#include <cstdint>
#include <vector>

namespace mxh::server {

// ---- AI state machine (legacy eAIState) ----
// 1:1 port of CAISystem::eAIState enum (AISystem.h L88). The order matches
// legacy; do NOT renumber without updating the wire format.
enum class AiState : std::uint8_t {
    None      = 0,   // not yet ticked
    Stand     = 1,   // idle, ready
    Walk      = 2,   // patrolling
    Run       = 3,   // chasing target
    Attack    = 4,   // executing an attack
    Skill     = 5,   // casting a skill
    Die       = 6,   // dead
    FallBack  = 7,   // retreat (used by some boss patterns)
    RunAway   = 8,   // flee
    Paused    = 9,   // held by buff / trap
};

// ---- AI behavior template (legacy BASEINDEX / AIParam fields) ----
// 1:1 port of CAISystem::AIParam union / boss-variant flags. Stores the
// static behavior table (chase radius, search radius, attack interval,
// flee HP threshold) loaded from a .bin file by the resource manager.
struct AiBehavior final {
    std::uint32_t monster_kind        = 0;  // legacy MonsterKind (enum)
    std::uint16_t chase_radius        = 0;  // chase chase radius (cells)
    std::uint16_t search_radius       = 0;  // aggro / detection radius (cells)
    std::uint16_t attack_interval_ms  = 0;  // min ms between attacks
    std::uint8_t  flee_hp_percent     = 0;  // 0..100; below this enter RunAway
    std::uint8_t  skill_use_chance    = 0;  // 0..100; chance to use skill vs basic
    std::uint8_t  is_boss             = 0;  // 0/1 flag
    std::uint8_t  reserved0           = 0;
};

// ---- Monster instance runtime state ----
// 1:1 port of CMonster m_Hero / m_HP / m_AI / m_AISub / m_pSUnit fields
// (a subset of the 80+ legacy members, only those driving server-side
// AI ticks + combat resolution).
struct MonsterInstance final {
    std::uint32_t object_id      = 0;     // unique across the map
    std::uint32_t monster_kind   = 0;     // legacy MonsterKind enum
    std::uint16_t map_num        = 0;

    // Position (cells, legacy uses VECTOR3 in 4Dyuchi coords; we use
    // XZ + Y separate ints for portability)
    std::int32_t  pos_x          = 0;
    std::int32_t  pos_y          = 0;
    std::int32_t  pos_z          = 0;

    // Vitals (legacy GetLife/GetMaxLife)
    std::uint32_t current_hp     = 0;
    std::uint32_t max_hp         = 1;
    std::uint8_t  state          = 0;     // eObjectState enum (Die=2)

    // AI state
    AiState  ai_state            = AiState::None;
    std::uint32_t target_player_id = 0;  // 0 = no target
    std::uint32_t last_attack_ms   = 0;  // legacy m_dwLastAttackTime
    std::uint32_t state_entered_ms = 0;  // legacy m_dwStateEnteredTime

    // Static behavior table
    AiBehavior behavior;

    // Exp drop on kill (legacy m_exp / MonsterList.bin)
    std::uint32_t exp_reward     = 0;

    // Group ID for aggro / leash; same group monsters assist each
    // other (legacy m_dwGroup)
    std::uint32_t group_id       = 0;
};

// ---- Boss variant (legacy CBossMonster extends CMonster) ----
// Adds field-boss-only behavior: stage transitions, shout, special drop.
struct BossMonsterInstance final {
    MonsterInstance base;
    std::uint32_t stage          = 0;     // boss stage (1..3, legacy)
    std::uint32_t rage_target_id = 0;     // boss targets this in rage
    std::uint32_t rage_until_ms  = 0;     // rage timer end
    std::uint8_t  is_field_boss  = 0;     // 0=map boss, 1=world boss
    std::uint8_t  reserved0      = 0;
    std::uint16_t reserved1      = 0;
};

// ---- AI state transitions (pure functions) ----

// Should the monster enter RunAway given hp/max and the flee threshold?
bool should_flee(std::uint32_t current_hp, std::uint32_t max_hp, std::uint8_t flee_pct) noexcept;

// Transition Idle -> Run (chase) when a player enters chase_radius.
AiState transition_idle_to_chase(const MonsterInstance& m,
                                  std::uint32_t now_ms,
                                  std::uint32_t player_id) noexcept;

// Transition Run -> Attack when target is within attack radius (cell<=1).
AiState transition_chase_to_attack(const MonsterInstance& m,
                                     std::uint32_t now_ms) noexcept;

// Tick down attack cooldown; returns true if the monster can attack now.
bool attack_cooldown_elapsed(const MonsterInstance& m, std::uint32_t now_ms) noexcept;

// Compute the next state after receiving a damage hit. If the monster
// dies it transitions to Die; if its HP falls below the flee threshold
// it transitions to RunAway (low-HP flee); otherwise it stays in its
// current state.
AiState transition_on_damage(MonsterInstance& m,
                               std::uint32_t damage,
                               std::uint32_t now_ms) noexcept;

// Kill the monster (HP=0, state=Die).
void kill_monster(MonsterInstance& m, std::uint32_t now_ms) noexcept;

// Advance the AI state machine by one tick. Pure function; caller is
// responsible for broadcasting the resulting delta.
AiState ai_tick(MonsterInstance& m, std::uint32_t now_ms) noexcept;

}  // namespace mxh::server
