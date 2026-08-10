// skill_caster.cpp - Phase 13.4 caster data plane implementation.
//
// 1:1 port of the skill caster decision + damage math that legacy
// [Server]Map/SkillManager.cpp + [Server]Map/CharacterCalcManager.cpp
// expose.  The body of `skill_caster_calculate_damage` is a 1:1
// extraction of the inline `MapHandler::calculate_damage` so the
// 5/5 attack capture wire shape stays byte-for-byte stable when
// the orchestrator is later rewired to call this module.

#include "mxh/server/skill_caster.hpp"

#include <cmath>
#include <utility>

namespace mxh::server {

namespace {

inline bool is_attack_kind(mxh::game::SkillKind k) noexcept {
    return k == mxh::game::SkillKind::OuterMugong
        || k == mxh::game::SkillKind::InnerMugong
        || k == mxh::game::SkillKind::Combo;
}

inline bool is_heal_kind(mxh::game::SkillKind k) noexcept {
    return k == mxh::game::SkillKind::OuterMugong;
}

}  // namespace

std::int32_t skill_caster_heal_amount(
    const mxh::game::PlayerCombatStats& caster,
    const mxh::game::SkillInfoSimple& skill) noexcept {
    // 1:1 with handle_skill in-line: heal = skill.phy_attack +
    // caster.level * 5.  Caller sends a negative damage on the wire
    // to mark the SingleResult as a heal.
    return static_cast<std::int32_t>(skill.phy_attack)
         + static_cast<std::int32_t>(caster.level) * 5;
}

mxh::game::DamageResult skill_caster_calculate_damage(
    const mxh::game::PlayerCombatStats& attacker,
    const mxh::game::PlayerCombatStats& defender,
    const mxh::game::SkillInfo& skill,
    std::mt19937& rng) noexcept {
    mxh::game::DamageResult result;

    // Dodge check
    std::uint8_t dodge_roll = static_cast<std::uint8_t>(rng() % 100);
    if (dodge_roll < defender.dodge_rate) {
        result.is_miss = true;
        result.hit_result = 0;
        result.damage = 0;
        return result;
    }

    const auto simple = mxh::game::to_simple(skill);
    std::int32_t base_damage = static_cast<std::int32_t>(simple.phy_attack)
                             + static_cast<std::int32_t>(attacker.phy_attack)
                             - static_cast<std::int32_t>(defender.phy_defence);
    if (base_damage < 1) base_damage = 1;

    std::int32_t attr_damage = static_cast<std::int32_t>(simple.att_attack)
                             + static_cast<std::int32_t>(attacker.att_attack)
                             - static_cast<std::int32_t>(defender.att_defence);
    if (attr_damage < 0) attr_damage = 0;

    std::int32_t total_damage = base_damage + attr_damage;

    std::uint8_t crit_roll = static_cast<std::uint8_t>(rng() % 100);
    if (crit_roll < (simple.critical_rate + attacker.critical_rate)) {
        result.is_critical = true;
        total_damage = static_cast<std::int32_t>(total_damage * 1.5);
        result.hit_result = 2;
    } else {
        result.hit_result = 1;
    }

    result.damage = total_damage;
    return result;
}

SkillCasterDecision skill_caster_decide_use(
    const SkillCasterRequest& req,
    std::mt19937& rng) noexcept {
    SkillCasterDecision out{};

    // 1. Caster must be alive.  Matches legacy UseSkill that aborts
    //    on a dead caster.
    if (!req.caster_alive || req.caster.current_hp == 0) {
        out.status = SkillCasterStatus::DeadCaster;
        return out;
    }

    // 2. Skill template must have a non-zero index.  Orchestrator
    //    usually checks find_skill() first; the guard stays so the
    //    module is safe to call directly from tests.
    if (req.skill.skill_idx == 0) {
        out.status = SkillCasterStatus::UnknownSkill;
        return out;
    }

    // 3. Skill kind must be castable (attack or heal).  Mining /
    //    Collection / Hunt / Titan are not combat skills.
    if (!is_attack_kind(req.skill.skill_kind) && !is_heal_kind(req.skill.skill_kind)) {
        out.status = SkillCasterStatus::WrongKind;
        return out;
    }

    // 4. MP cost.
    if (req.caster.current_mp < req.skill.need_nearyuk) {
        out.status = SkillCasterStatus::NotEnoughMp;
        return out;
    }

    // 5. Target present + range check (skip for self / no-target
    //    heals).
    if (req.target_present) {
        if (!req.target_alive) {
            out.status = SkillCasterStatus::DeadCaster;
            return out;
        }
        if (req.skill.skill_range > 0) {
            const float dx = req.caster_pos_x - req.target_pos_x;
            const float dz = req.caster_pos_z - req.target_pos_z;
            const float dsq = dx * dx + dz * dz;
            const float range = static_cast<float>(req.skill.skill_range);
            if (dsq > range * range) {
                out.status = SkillCasterStatus::OutOfRange;
                return out;
            }
        }
    }

    // 6. Heal path skips the damage formula.  The caller applies
    //    the heal amount via `skill_caster_heal_amount`.
    if (is_heal_kind(req.skill.skill_kind)
        && req.skill.skill_kind == mxh::game::SkillKind::OuterMugong
        && req.target_present == false) {
        out.status = SkillCasterStatus::Ok;
        out.damage = 0;
        out.hit_result = 1;
        return out;
    }

    // 7. Attack path: synthesise a full SkillInfo for the damage
    //    function.  to_simple() round-trips all formula inputs.
    mxh::game::SkillInfo full{};
    full.SkillIdx = static_cast<std::uint16_t>(req.skill.skill_idx);
    full.UpPhyAttack[0] = req.skill.phy_attack;
    full.FirstAttAttack[0] = req.skill.att_attack;
    full.AttackSuccessRate[0] = static_cast<float>(req.skill.att_rate);
    full.CriticalRate[0] = static_cast<float>(req.skill.critical_rate);
    full.StunRate[0] = static_cast<float>(req.skill.stun_rate);
    full.NeedNaeRyuk[0] = req.skill.need_nearyuk;
    full.SkillKind = static_cast<std::uint16_t>(req.skill.skill_kind);
    full.SkillRange = req.skill.skill_range;
    full.TargetRange = req.skill.target_range;

    const auto dmg = skill_caster_calculate_damage(
        req.caster, req.target_combat, full, rng);
    out.status = SkillCasterStatus::Ok;
    out.damage = dmg.damage;
    out.hit_result = dmg.hit_result;
    return out;
}

}  // namespace mxh::server
