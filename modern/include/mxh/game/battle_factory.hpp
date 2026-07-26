// battle_factory.hpp - 1:1 port of legacy damage formulas.
//
// Source: ?? [Server]Map\AttackCalc.cpp (CBattleFactory_Default / CAttackCalc)
// Extracted from the legacy CObject/CMonster/CPlayer-coupled code into pure
// numeric functions so they can be unit-tested and run on the modern stack.
// All numerical formulas are 1:1 with the legacy source; the legacy #ifdef
// _JAPAN_LOCAL_ branch is selected as the default since it ships with the
// KR/CN build used by this repo (the J/K/CN branches are identical anyway).

#pragma once

#include <cstdint>

namespace mxh::game {

// ---- Constants from ?? [CC]Header\ClientGameDefine.h ----
inline constexpr std::int32_t MAX_CHARACTER_LEVEL_NUM = 99;
inline constexpr std::int32_t MONSTERLEVELRESTRICT_LOWSTARTNUM = 8;
inline constexpr std::int32_t MAX_MONSTERLEVELPOINTRESTRICT_NUM = 12;

// ---- Critical / Decisive hit probability (AttackCalc.cpp L79, L148) ----
//
// Legacy: getCritical(attacker_crit, target_level, attacker_level, fCriticalRate)
//   cri = (attacker_crit + 20) / (target_level * 5 + 100)
//   if (cri > 0.15) cri = 0.15
//   if (attacker_level < target_level) cri += fCriticalRate - (target-attacker)*0.02
//   else                       cri += fCriticalRate + (attacker-target)*0.004
//   if (cri < 0) cri = 0;  return (rand_0_99 < cri * 100)
//
// Returns the critical rate in [0, 1] (probability, before random roll).
double compute_critical_rate(std::int32_t attacker_crit,
                            std::int32_t attacker_level,
                            std::int32_t target_level,
                            double fCriticalRate);

// True iff the random roll succeeds given the computed critical rate.
// and_0_99 is the integer [0, 100) sample; legacy uses (rand()%100).
bool roll_critical(double critical_rate, int rand_0_99);

// Same formula as critical, replacing GetCritical with GetDecisive.
double compute_decisive_rate(std::int32_t attacker_decisive,
                             std::int32_t attacker_level,
                             std::int32_t target_level,
                             double fCriticalRate);
bool roll_decisive(double decisive_rate, int rand_0_99);

// ---- Physical attack power (AttackCalc.cpp L217, L333) ----
//
// Legacy: getPlayerPhysicalAttackPower(pPlayer, PhyAttackRate, bCritical)
//   base_atk = 1 + BaseAtk.skill_stats_option (clamped >= 0)
//   min = (uint)(min * base_atk + 0.5);  max = (uint)(max * base_atk + 0.5)
//   if (max <= min) val = min; else val = min + rand%(max - min + 1)
//   val = val * PhyAttackRate
//   if (bCritical) val = val * 1.5   (KR/CN branch; Japan keeps 2.25)
// Returns the unsigned integer physical damage ready to subtract from HP.
std::uint32_t compute_player_physical_attack(
    std::uint32_t min_val,
    std::uint32_t max_val,
    double base_atk_bonus,    // BaseAtk.SkillStatsOption
    double phy_attack_rate,   // skill buff multiplier (e.g. 1.0)
    bool b_critical,
    int rand_gap);   // rand()%gap, called by the caller (legacy uses rand%gap)

// Player attribute (fire/water/wind/...) damage. AttackCalc.cpp L260.
// KR/CN branch:
//   SimMek   = player.Stat.simmek
//   midterm  = (SimMek + 200) / 100
//   MinLVV = (level+5)-5 = MaxLVV = (level+5)+5
//   MinV = MinLVV * AttAttackRate * midterm + SimMek/5 + min(SimMek-12, 25)
//   MaxV = MaxLVV * AttAttackRate * midterm + SimMek/5 + min(SimMek-12, 25)
// Then rand_uniform over [MinV, MaxV] if AttAttackRate>0; otherwise 0.0.
std::uint32_t compute_player_attribute_attack(
    std::int32_t level,
    std::uint32_t sim_mek,
    std::uint32_t att_attack_min,
    std::uint32_t att_attack_max,
    double att_attack_rate);

// ---- Experience point gain (AttackCalc.cpp L45) ----
//
// Legacy GetPlayerExpPoint(level_gap, monster_exp):
//   gap <= -9:  Exp = monster_exp * 1.5
//   -9 < gap < 1: Exp = exp + exp * (-gap) * 0.05
//   0 < gap < 5: Exp = exp * (5 - gap) * 0.2
//   gap == 5:    Exp = exp * 0.1
//   gap >  5:    return 0
//   round-up at first decimal: if (floor(Exp*10) > floor(Exp)*10) +1
std::uint32_t compute_player_exp_point(std::int32_t level_gap,
                                    std::uint32_t monster_exp);

// Lookup-table slot: clamped level gap indexed into the player-x-monster point
// table (GameResourceManager.GetPLAYERxMONSTER_POINT). Modern port provides a
// pure-function hook that callers can plug their own data table into.
using PlayerXMonsterPointLookup = std::uint32_t (*)(std::int32_t,
                                               std::int32_t);
std::uint32_t clamp_player_x_monster_lookup(std::int32_t level,
                                          std::int32_t level_gap,
                                          PlayerXMonsterPointLookup lookup);

// Physical defence level (AttackCalc.cpp getPhyDefenceLevel, KR/CN branch).
// phy_defence is the buffed / unique-adjusted base; titan_defense_override > 0
// activates the SW070127 Titan branch (clamped at 0.8).
double compute_phy_defence_level(double phy_defence,
                                 std::int32_t attacker_level,
                                 double titan_defense_override = 0.0);

// Final received damage after applying phyDefenceLevel. Legacy floor at 1.
std::uint32_t compute_received_damage(std::uint32_t attack_power,
                                      double phy_defence_level);

// Monster physical attack power (AttackCalc.cpp getMonsterPhysicalAttackPower).
// rand_gap is the integer rand()%gap; deterministic in tests.
std::uint32_t compute_monster_physical_attack(std::uint32_t min_val,
                                             std::uint32_t max_val,
                                             double phy_attack_rate,
                                             int rand_gap);

}  // namespace mxh::game
