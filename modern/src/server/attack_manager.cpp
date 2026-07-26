// attack_manager.cpp - Phase D6 AttackManager 1:1 port implementations.
//
// Source-of-truth: legacy [Server]Map/AttackManager.cpp (lines marked
// per-function below).  All formulas are 1:1 copies of the legacy
// expression; modernization only removes the CObject singleton
// coupling and exposes them as pure functions over POD inputs.

#include "mxh/server/attack_manager.hpp"

#include <algorithm>

namespace mxh::server {

std::uint32_t get_jinbub_damage(std::uint32_t attack_power,
                                float decrease_damage_rate,
                                std::uint8_t attacker_kind,
                                std::uint8_t target_kind,
                                bool is_japan_local) {
    // Legacy AttackManager.cpp GetJinbubDamage body (excerpted):
    //   DWORD attackPhyDamage = (DWORD)(AttackPower * fDecreaseDamageRate);
    //   if (PVP) { ... apply 50% PvP reduction (JP variant) ... }
    //   return attackPhyDamage;
    auto damage = static_cast<std::uint32_t>(
        static_cast<float>(attack_power) * decrease_damage_rate);
    const bool pvp = (attacker_kind == eObjectKind_Player) &&
                     (target_kind == eObjectKind_Player);
    if (pvp) {
        // Legacy: PvP is halved for jinbub true-damage.
        // Japan local variant keeps full damage; CN/KR halve it.
        if (!is_japan_local) {
            damage = damage / 2u;
        }
    }
    return damage;
}

std::uint32_t get_penalty_damage(std::uint32_t damage,
                                 std::uint8_t attacker_kind,
                                 std::uint8_t target_kind,
                                 bool attacker_in_titan) {
    // Legacy AttackManager.cpp GetPenaltyDemege body (excerpted):
    //   if (target is monster) {
    //     if (attacker is player) {
    //       if (target is titan monster && !attacker_in_titan)
    //         return damage * 0.05f
    //       else if (!target is titan monster && attacker_in_titan)
    //         return damage * 0.5f
    //     }
    //   }
    //   return damage unchanged
    const bool target_is_monster = (target_kind & eObjectKind_Monster) != 0;
    if (!target_is_monster) return damage;
    if (attacker_kind != eObjectKind_Player) return damage;

    const bool target_is_titan_monster = (target_kind == eObjectKind_TitanMonster);
    if (target_is_titan_monster && !attacker_in_titan) {
        return static_cast<std::uint32_t>(static_cast<float>(damage) * 0.05f);
    }
    if (!target_is_titan_monster && attacker_in_titan) {
        return static_cast<std::uint32_t>(static_cast<float>(damage) * 0.5f);
    }
    return damage;
}

std::uint32_t recover_life(std::uint32_t recover_val,
                           std::uint32_t current_life,
                           std::uint32_t max_life) {
    // Legacy AddLife body:
    //   realAddVal = (current + recover <= max) ? recover : (max - current)
    if (current_life >= max_life) return 0u;
    const std::uint32_t room = max_life - current_life;
    return std::min(recover_val, room);
}

std::uint32_t recover_shield(std::uint32_t recover_val,
                             std::uint32_t current_shield,
                             std::uint32_t max_shield) {
    if (current_shield >= max_shield) return 0u;
    const std::uint32_t room = max_shield - current_shield;
    return std::min(recover_val, room);
}

std::uint32_t recover_naeryuk(std::uint32_t recover_val,
                               std::uint32_t current_naeryuk,
                               std::uint32_t max_naeryuk) {
    if (current_naeryuk >= max_naeryuk) return 0u;
    const std::uint32_t room = max_naeryuk - current_naeryuk;
    return std::min(recover_val, room);
}

DamageSplit apply_shield_cap(std::uint32_t damage, std::uint32_t target_shield) {
    // Legacy: damage is first absorbed by shield up to remaining shield.
    //   shield_chunk = min(damage, target_shield)
    //   life_chunk = damage - shield_chunk
    DamageSplit split{};
    split.shield_chunk = std::min(damage, target_shield);
    split.life_chunk = damage - split.shield_chunk;
    return split;
}

DamageSplit apply_combo_shield_split(std::uint32_t damage) {
    // Legacy: SHIELD_COMBO_DAMAGE = 0.5 -> 50% of damage hits shield.
    DamageSplit split{};
    split.shield_chunk = static_cast<std::uint32_t>(
        static_cast<float>(damage) * SHIELD_COMBO_DAMAGE);
    split.life_chunk = damage - split.shield_chunk;
    return split;
}

DamageSplit apply_mugong_shield_split(std::uint32_t damage) {
    // Legacy: SHIELD_OUT_MUGONG_DAMAGE = 0.7 -> 70% hits shield.
    DamageSplit split{};
    split.shield_chunk = static_cast<std::uint32_t>(
        static_cast<float>(damage) * SHIELD_OUT_MUGONG_DAMAGE);
    split.life_chunk = damage - split.shield_chunk;
    return split;
}

ResolvedAttack resolve_attack(std::uint32_t raw_damage,
                              float damage_rate,
                              const AttackCalcParams& params) {
    // Convenience pipeline: rate -> penalty -> shield cap.
    ResolvedAttack out{};
    std::uint32_t d = apply_damage_rate(raw_damage, damage_rate);
    d = get_penalty_damage(d, params.attacker_kind, params.target_kind,
                           params.attacker_in_titan);
    const auto split = apply_shield_cap(d, params.target_shield);
    out.shield_chunk = split.shield_chunk;
    out.life_chunk = split.life_chunk;
    out.real_damage = d;
    return out;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int attack_manager_translation_unit_anchor = 0;
}
