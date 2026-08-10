// skill_caster.hpp - Phase 13.4 caster data plane.
//
// Phase 13.4 of Moxian-Reborn modernization: 1:1 port of the skill
// caster data plane that legacy [Server]Map/SkillManager.cpp +
// [Server]Map/CharacterCalcManager.cpp + [Server]Map/Object.cpp
// expose through CSkillObject / CObjectManager::UseSkill / the per
// skill cooldown check / the per skill MP cost guard / the dodge +
// crit + phy/attr damage formula.
//
// This header ships the *data plane* only: pure-function decision
// + damage math that takes the caster state, the target state, the
// skill template + an RNG and returns a typed decision (Ok /
// NotEnoughMp / OutOfRange / ...).  The orchestrator (MapHandler)
// applies the decision to its player state under its own critical
// section.  Splitting data plane from orchestrator mirrors
// npc_shop.hpp + quest_manager.hpp / quest_script_loader.hpp.
//
// 1:1 quirks preserved (matches MapHandler::calculate_damage 1:1):
//   * Dodge roll is `(rng() % 100) < defender.dodge_rate`.  Miss
//     returns hit_result = 0 and damage = 0 (caller treats the
//     SingleResult as 0 damage with a miss flag).
//   * Base phy damage = skill.phy_attack + attacker.phy_attack -
//     defender.phy_defence, clamped to a minimum of 1.
//   * Attr damage = skill.att_attack + attacker.att_attack -
//     defender.att_defence, clamped to 0.
//   * Crit roll is `(rng() % 100) < (skill.critical_rate +
//     attacker.critical_rate)`.  On crit total damage is multiplied
//     by 1.5 and hit_result = 2; otherwise hit_result = 1.
//   * OuterMugong heal amount = skill.phy_attack +
//     caster.level * 5 (matches the in-line calc in handle_skill).

#pragma once

#include "mxh/game/skill_types.hpp"

#include <cstdint>
#include <random>

namespace mxh::server {

// ---- Decision outcome for an attempted skill use ----
//
// Mirrors the legacy CSkillObject::UseSkill + CObjectManager guards.
// UnknownCaster is reported by the orchestrator before the request
// reaches this module; the module only returns it when the caster
// state is empty (level == 0 or all vitals == 0) so it stays useful
// for unit testing.
enum class SkillCasterStatus : std::uint8_t {
    Ok            = 0,
    UnknownSkill  = 1,  // skill_idx == 0 (orchestrator pre-check on find_skill)
    DeadCaster    = 2,  // caster.current_hp == 0
    NotEnoughMp   = 3,  // caster.current_mp < skill.need_nearyuk
    OutOfRange    = 4,  // distance > skill.skill_range
    WrongKind     = 5,  // skill_kind cannot be cast as attack/heal
};

// ---- One request (caster + target + skill) ----
struct SkillCasterRequest final {
    // Caster state.  level / phy_attack / phy_defence feed the
    // damage formula.  current_mp feeds the MP guard.
    mxh::game::PlayerCombatStats caster{};
    bool            caster_alive        = true;
    float           caster_pos_x        = 0.0f;
    float           caster_pos_z        = 0.0f;
    // Target state.  target_present == false skips the range /
    // alive guard (self / ground-targeted skills).  When present
    // and the skill is an attack, target_combat feeds the defender
    // half of the damage formula.
    bool            target_present      = true;
    bool            target_alive        = true;
    float           target_pos_x        = 0.0f;
    float           target_pos_z        = 0.0f;
    mxh::game::PlayerCombatStats target_combat{};
    // Skill template (simple view carries every field the
    // decision + damage formula needs).
    mxh::game::SkillInfoSimple skill{};
};

// ---- Decision returned by skill_caster_decide_use ----
struct SkillCasterDecision final {
    SkillCasterStatus status     = SkillCasterStatus::Ok;
    // Filled on Ok when the skill can hit a target.  damage is the
    // post-crit / post-dodge damage; on miss damage == 0 and
    // hit_result == 0.  On non-Ok the caller does not consume damage
    // and the wire-level StartNack reason is derived from status.
    std::int32_t      damage     = 0;
    std::uint8_t      hit_result = 1;  // 0=miss 1=hit 2=crit
};

// ---- Pure-function decision for a player-vs-target attack ----
//
// Returns the typed outcome of an attempted skill use.  The
// orchestrator maps the decision to MP_SKILL_START_ACK / _NACK on
// the wire and, on Ok, mutates the player state (HP / MP /
// cooldowns) under its critical section.
SkillCasterDecision skill_caster_decide_use(
    const SkillCasterRequest& req,
    std::mt19937& rng) noexcept;

// ---- Heal amount for OuterMugong self/ally heal skills ----
//
// 1:1 with the in-line heal = skill.phy_attack + caster.level * 5
// in handle_skill.  Caller is responsible for applying the result
// to the target's vitals (negative damage sent as a heal on the
// wire).
std::int32_t skill_caster_heal_amount(
    const mxh::game::PlayerCombatStats& caster,
    const mxh::game::SkillInfoSimple& skill) noexcept;

// ---- Damage formula 1:1 port (extractable from MapHandler) ----
//
// `attacker` is the caster combat stats; `defender` is the target
// (player or monster) combat stats; `skill` is the *full* SkillInfo
// (the simple view is used internally).  The function uses the
// caller-supplied RNG so unit tests can pin dodges / crits.
mxh::game::DamageResult skill_caster_calculate_damage(
    const mxh::game::PlayerCombatStats& attacker,
    const mxh::game::PlayerCombatStats& defender,
    const mxh::game::SkillInfo& skill,
    std::mt19937& rng) noexcept;

}  // namespace mxh::server
