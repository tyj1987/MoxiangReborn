// monster.hpp - 1:1 port of legacy [Server]Map/Monster.h (CMonster) and
// AISystem.h (CAISystem) into pure data structs + pure functions.
//
// Source: legacy [Server]Map/Monster.h + AISystem.h + AIParam.h
// The legacy engine uses deep inheritance (CMonster : CObject : CAction)
// with 200+ members; the modern port models just the data fields and the
// state machine transitions as pure POD structs + free functions. The
// map_handler uses these to populate ServerSideMonsterInstance and drive
// the per-tick AI loop.
//
// Phase 6.2 extension: adds full legacy 1:1 fields (SubID, RegenNum,
// DropItemId, SuryunGroup, EventMob, Group from MONSTER_TOTALINFO) and
// the BossMonsterInstance stage / field-boss variants.

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
// 1:1 port of CMonster m_Hero / m_HP / m_AI / m_AISub / m_pSUnit fields,
// extended in Phase 6.2 with the full set of legacy 1:1 fields so that
// the wire format / server-side bookkeeping is byte-equivalent to the
// original engine.
struct MonsterInstance final {
    // ---- object identity ----
    std::uint32_t object_id      = 0;     // unique across the map
    std::uint32_t monster_kind   = 0;     // legacy MonsterKind enum
    std::uint8_t  object_kind    = mxh::game::OBJECTKIND_MONSTER;
    char          name[mxh::game::MAX_MONSTER_NAME_LENGTH+1] = {};
    std::uint16_t map_num        = 0;
    std::uint16_t group          = 0;     // MONSTER_TOTALINFO.Group (legacy aggro group)

    // ---- position ----
    std::int32_t  pos_x          = 0;
    std::int32_t  pos_y          = 0;
    std::int32_t  pos_z          = 0;
    std::int32_t  spawn_x        = 0;     // legacy regen point
    std::int32_t  spawn_y        = 0;
    std::int32_t  spawn_z        = 0;

    // ---- vitals ----
    std::uint32_t current_hp     = 0;
    std::uint32_t max_hp         = 1;
    std::uint32_t current_shield = 0;     // legacy Shield (hostile-shield, not MP)
    std::uint32_t max_shield     = 0;
    std::uint8_t  state          = 0;     // eObjectState enum (Die=2)

    // ---- AI state ----
    AiState  ai_state            = AiState::None;
    std::uint32_t target_player_id = 0;  // 0 = no target
    std::uint32_t last_attack_ms   = 0;  // legacy m_dwLastAttackTime
    std::uint32_t state_entered_ms = 0;  // legacy m_dwStateEnteredTime

    // ---- static behavior table (loaded from BASE_MONSTER_LIST) ----
    AiBehavior behavior;

    // ---- legacy CMonster fields (1:1) ----
    std::uint16_t drop_item_id      = 0;  // legacy m_DropItemId (0..MonsterDropItemList.bin size)
    std::uint32_t drop_item_ratio   = 100;// legacy m_dwDropItemRatio (percent; 100 = always)
    std::uint32_t sub_id            = 0;  // legacy m_SubID (object_id of the regen point)
    std::uint16_t regen_num         = 0;  // legacy m_RegenNum (spawn index within the regen list)
    int           suryun_group      = 0;  // legacy m_SuryunGroup (training area group)
    bool          event_mob         = false;  // legacy m_bEventMob (event-flagged)
    std::uint32_t killer_player_id  = 0;  // legacy m_KillerPlayer (last killing blow)
    std::uint32_t last_attacker_id  = 0;  // legacy m_pLastAttackPlayer

    // ---- exp / aggro / leash ----
    std::uint32_t exp_reward     = 0;
    std::uint32_t group_id       = 0;     // legacy m_dwGroup (assist / leash group)

    // ---- helpers ----
    // Wire-format dump into MONSTER_TOTALINFO (14 bytes).
    mxh::game::MonsterTotalInfo make_totalinfo() const noexcept {
        mxh::game::MonsterTotalInfo t{};
        t.Life        = current_hp;
        t.Shield      = current_shield;
        t.MonsterKind = static_cast<std::uint16_t>(monster_kind & 0xFFFFu);
        t.Group       = group;
        t.MapNum      = map_num;
        return t;
    }
};

// ---- Boss variant (legacy CBossMonster extends CMonster) ----
// Adds field-boss-only behavior: stage transitions, shout, special drop.
//
// Phase 6.2 extension: stage is now a 1:1 port of the legacy 4-stage
// boss combat model: 0=normal, 1=enraged (HP<75%), 2=phase2 (HP<50%),
// 3=rage (HP<25%). Each stage can swap attack patterns, AI param and
// shout trigger.
struct BossMonsterInstance final {
    MonsterInstance base;
    std::uint32_t stage          = 0;     // boss stage (0..3, legacy)
    std::uint32_t rage_target_id = 0;     // boss targets this in rage
    std::uint32_t rage_until_ms  = 0;     // rage timer end
    std::uint8_t  is_field_boss  = 0;     // 0=map boss, 1=world boss
    std::uint8_t  reserved0      = 0;
    std::uint16_t reserved1      = 0;
    std::uint32_t speech_id      = 0;     // cMonsterSpeechManager cue
    std::uint32_t last_shout_ms  = 0;
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

// ---- Boss stage helpers ----

// Compute the boss stage from HP percent (0..100). Stages:
//   100..76 -> stage 0 (normal)
//   75..51  -> stage 1 (enraged)
//   50..26  -> stage 2 (phase2)
//   25..1   -> stage 3 (rage)
//   0       -> die (caller decides)
std::uint32_t boss_stage_from_hp(std::uint32_t current_hp, std::uint32_t max_hp) noexcept;

// Update a boss instance: re-evaluate stage based on HP and reset
// attack interval / shout counters when the stage changes.
void update_boss_stage(BossMonsterInstance& b,
                       std::uint32_t now_ms,
                       std::uint32_t shout_interval_ms = 5000) noexcept;

}  // namespace mxh::server