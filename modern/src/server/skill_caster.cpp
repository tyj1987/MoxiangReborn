// skill_caster.cpp - Phase 13.4 caster data plane implementation.
//
// The current skill/defence damage path remains simplified. Hydrated ordinary
// weapon endpoints are sampled inclusively without changing the wire shape.

#include "mxh/server/skill_caster.hpp"
#include "mxh/game/battle_factory.hpp"

#include <cmath>
#include <algorithm>
#include <limits>
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
    std::uint32_t physical = attacker.phy_attack;
    if (attacker.has_physical_range) {
        if (attacker.phy_attack_max < attacker.phy_attack_min) return result;
        const auto span = std::uint64_t(attacker.phy_attack_max) - attacker.phy_attack_min + 1u;
        physical = attacker.phy_attack_min + static_cast<std::uint32_t>(rng() % span);
    }
    std::int64_t base_damage = static_cast<std::int64_t>(simple.phy_attack)
                             + physical - static_cast<std::int64_t>(defender.phy_defence);
    if (base_damage < 1) base_damage = 1;

    std::int32_t attr_damage = static_cast<std::int32_t>(simple.att_attack)
                             + static_cast<std::int32_t>(attacker.att_attack)
                             - static_cast<std::int32_t>(defender.att_defence);
    if (attr_damage < 0) attr_damage = 0;

    std::int64_t total_damage = base_damage + attr_damage;

    std::uint8_t crit_roll = static_cast<std::uint8_t>(rng() % 100);
    if (crit_roll < (simple.critical_rate + attacker.critical_rate)) {
        result.is_critical = true;
        total_damage = static_cast<std::int64_t>(total_damage * 1.5);
        result.hit_result = 2;
    } else {
        result.hit_result = 1;
    }

    result.damage = static_cast<std::int32_t>(std::min<std::int64_t>(
        total_damage, std::numeric_limits<std::int32_t>::max()));
    return result;
}

mxh::game::DamageResult skill_caster_calculate_monster_damage(
    const mxh::game::PlayerCombatStats& attacker,
    const mxh::game::PlayerCombatStats& defender,
    const mxh::game::SkillInfo& skill,
    std::mt19937& rng) noexcept {
    mxh::game::DamageResult result;
    const auto simple = mxh::game::to_simple(skill);

    const auto dodge_roll = static_cast<std::uint8_t>(rng() % 100);
    if (dodge_roll < defender.dodge_rate) {
        result.is_miss = true;
        result.hit_result = 0;
        return result;
    }

    // AttackRate is the legacy multiplier.  A zero field means the skill
    // carries no override and therefore uses the normal 1.0 multiplier.
    const double skill_rate = skill.AttackRate[0] > 0.0f
        ? static_cast<double>(skill.AttackRate[0]) : 1.0;
    std::uint32_t min_attack = attacker.phy_attack;
    std::uint32_t max_attack = attacker.phy_attack;
    if (attacker.has_physical_range) {
        min_attack = attacker.phy_attack_min;
        max_attack = attacker.phy_attack_max;
    }
    const auto gap = max_attack >= min_attack
        ? static_cast<std::uint64_t>(max_attack) - min_attack + 1u : 1u;
    const auto roll = static_cast<int>(rng() % gap);
    auto physical = mxh::game::compute_player_physical_attack(
        min_attack, max_attack, 0.0, skill_rate, false, roll);
    // UpPhyAttack is a flat skill contribution in the modern combat view;
    // retain it before applying the original defence curve.
    physical = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        static_cast<std::uint64_t>(physical) + simple.phy_attack,
        std::numeric_limits<std::uint32_t>::max()));
    const auto defence_level = mxh::game::compute_phy_defence_level(
        defender.phy_defence, std::max<std::uint16_t>(1u, attacker.level));
    std::int64_t total_damage = mxh::game::compute_received_damage(
        physical, defence_level);

    std::int32_t attr_damage = static_cast<std::int32_t>(simple.att_attack)
                             + static_cast<std::int32_t>(attacker.att_attack)
                             - static_cast<std::int32_t>(defender.att_defence);
    if (attr_damage > 0) total_damage += attr_damage;

    const auto crit_roll = static_cast<std::uint8_t>(rng() % 100);
    if (crit_roll < (simple.critical_rate + attacker.critical_rate)) {
        result.is_critical = true;
        total_damage = static_cast<std::int64_t>(total_damage * 1.5);
        result.hit_result = 2;
    } else {
        result.hit_result = 1;
    }
    result.damage = static_cast<std::int32_t>(std::min<std::int64_t>(
        total_damage, std::numeric_limits<std::int32_t>::max()));
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
