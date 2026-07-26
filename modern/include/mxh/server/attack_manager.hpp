// attack_manager.hpp - Phase D6 AttackManager 1:1 port.
//
// Source-of-truth: legacy [Server]Map/AttackManager.h + .cpp.
// Mirrors legacy CAttackManager singleton decomposition into pure
// functions over POD structs.  Modern server wires these into the
// handler dispatch; no Object/CObject dependency is exposed in the
// header.
//
// Conventions:
//   * 1:1 numeric: every formula is the legacy exact expression.
//   * PascalCase field names match legacy ATTACKINFO / RESULTINFO.
//   * Object kinds map to eObjectKind_* from legacy common header
//     (eObjectKind_Player=1, eObjectKind_Monster=32, eObjectKind_TitanMonster=41).

#pragma once

#include <array>
#include <cstdint>

namespace mxh::server {

// ---- Object kind constants (mirror legacy eObjectKind_* enum) ----
inline constexpr std::uint8_t eObjectKind_None       = 0;
inline constexpr std::uint8_t eObjectKind_Player     = 1;
inline constexpr std::uint8_t eObjectKind_Monster    = 32;
inline constexpr std::uint8_t eObjectKind_TitanMonster = 41;
inline constexpr std::uint8_t eObjectKind_MonsterMask = 0x20;

// ---- Attack kind bits (mirror legacy eATTACKABS_KIND) ----
inline constexpr std::uint32_t eAAK_LIFE    = 1u;
inline constexpr std::uint32_t eAAK_SHIELD  = 2u;
inline constexpr std::uint32_t eAAK_NAERYUK = 4u;

// ---- Damage rate constants (mirror legacy SHIELD_* macros) ----
inline constexpr float SHIELD_COMBO_DAMAGE      = 0.5f;
inline constexpr float SHIELD_OUT_MUGONG_DAMAGE  = 0.7f;
inline constexpr float SHIELD_IN_MUGONG_DAMAGE   = 0.7f;

// ---- Damage rate cap (mirror legacy m_nDamageRate default) ----
inline constexpr float DEFAULT_DAMAGE_RATE = 100.0f;

// ---- POD structs (mirror legacy ATTACKINFO / RESULTINFO subset) ----

// Mirrors legacy RESULTINFO (the minimal slice we need for formulas).
struct ResultInfo {
    std::uint32_t phy_damage        = 0;
    std::uint32_t attr_damage       = 0;
    std::uint32_t real_damage       = 0;  // final applied damage
    std::uint32_t shield_damage     = 0;  // shield portion of real_damage
    std::uint32_t counter_damage    = 0;  // counter-attack damage to attacker
    std::uint32_t jinbub_damage     = 0;  // true-damage slice
    std::uint32_t vampiric_life     = 0;  // life stolen back to attacker
    std::uint32_t vampiric_naeryuk  = 0;  // mana stolen back to attacker
    std::int32_t  life_change       = 0;  // + heal, - damage
    std::int32_t  shield_change     = 0;
    std::int32_t  naeryuk_change    = 0;
    std::uint16_t critical          = 0;  // bool flag stored as u16
    std::uint16_t bmiss             = 0;  // bool flag stored as u16
    std::uint16_t bcounter          = 0;
    std::uint16_t bdodge            = 0;
};

// Mirrors legacy AttackCalcParams subset used by AttackManager.
struct AttackCalcParams {
    std::uint32_t attacker_min_damage   = 0;
    std::uint32_t attacker_max_damage   = 0;
    std::uint32_t attacker_attack_rate  = 0;  // % rate, e.g. 100 = 100%
    std::uint32_t attacker_critical     = 0;  // % crit chance
    std::uint32_t amplified_power       = 0;
    std::uint16_t amplified_power_attrib = 0;
    std::uint32_t target_physics_defence = 0;
    std::uint32_t target_shield         = 0;
    std::uint32_t target_max_life       = 0;
    std::uint32_t target_max_shield     = 0;
    std::uint32_t target_level          = 0;
    std::uint32_t attacker_level        = 0;
    std::uint32_t attacker_max_life     = 0;
    std::uint32_t attacker_max_naeryuk  = 0;
    std::uint32_t attacker_vamp_life_rate   = 0;  // %
    std::uint32_t attacker_vamp_naeryuk_rate = 0;  // %
    std::uint8_t  attacker_kind         = eObjectKind_Player;
    std::uint8_t  target_kind           = eObjectKind_Monster;
    bool          attacker_in_titan     = false;
    bool          target_in_titan       = false;
    bool          is_japan_local        = false;  // _JAPAN_LOCAL_ build flag
    bool          is_hk_local           = false;  // _HK_LOCAL_ build flag
};

// ---- Pure formula functions (1:1 with legacy) ----

// GetJinbubDamage - true-damage formula.
// Legacy: DWORD attackPhyDamage = (DWORD)(AttackPower * fDecreaseDamageRate);
//   if (PVP) divide by 2 (CN: also apply 50% PvP reduction).
std::uint32_t get_jinbub_damage(std::uint32_t attack_power,
                                float decrease_damage_rate,
                                std::uint8_t attacker_kind,
                                std::uint8_t target_kind,
                                bool is_japan_local);

// GetPenaltyDemege - titan-vs-monster penalty.
// Legacy:
//   target=Monster, attacker=Player, target=TitanMonster, attacker not in titan:
//     dwResult = damage * 0.05f (5%)
//   target=Monster (not titan), attacker in titan:
//     dwResult = damage * 0.5f (50%)
//   else: dwResult = damage unchanged
std::uint32_t get_penalty_damage(std::uint32_t damage,
                                 std::uint8_t attacker_kind,
                                 std::uint8_t target_kind,
                                 bool attacker_in_titan);

// RecoverLife - heals target by RecoverLifeVal but never above max.
std::uint32_t recover_life(std::uint32_t recover_val,
                           std::uint32_t current_life,
                           std::uint32_t max_life);

std::uint32_t recover_shield(std::uint32_t recover_val,
                             std::uint32_t current_shield,
                             std::uint32_t max_shield);

std::uint32_t recover_naeryuk(std::uint32_t recover_val,
                               std::uint32_t current_naeryuk,
                               std::uint32_t max_naeryuk);

// ApplyShieldCap - splits damage into shield + life chunks.
struct DamageSplit {
    std::uint32_t shield_chunk;
    std::uint32_t life_chunk;
};
DamageSplit apply_shield_cap(std::uint32_t damage, std::uint32_t target_shield);

// ComboDamageCap - combo-attack shield multiplier (50%).
DamageSplit apply_combo_shield_split(std::uint32_t damage);

// MugongDamageCap - mugong shield multiplier (70%).
DamageSplit apply_mugong_shield_split(std::uint32_t damage);

// DamageRate wrapper.
inline std::uint32_t apply_damage_rate(std::uint32_t damage, float damage_rate) {
    if (damage_rate <= 0.0f) return 0u;
    return static_cast<std::uint32_t>(static_cast<float>(damage) * (damage_rate / 100.0f));
}

struct ResolvedAttack {
    std::uint32_t shield_chunk;
    std::uint32_t life_chunk;
    std::uint32_t real_damage;
};
ResolvedAttack resolve_attack(std::uint32_t raw_damage,
                              float damage_rate,
                              const AttackCalcParams& params);

}  // namespace mxh::server
