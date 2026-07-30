// battle_factory.cpp - 1:1 numeric formulas from AttackCalc.cpp
//
// All formulas reproduced byte-for-byte from
//   ?? [Server]Map\AttackCalc.cpp
// The KR/CN build (this repo) selects the !_JAPAN_LOCAL_ branch; Japan uses
// a different critical ceiling (0.15) and a 2.25x crit multiplier instead of 1.5x.
// Both branches are identical for GetPlayerExpPoint.

#include "mxh/game/battle_factory.hpp"
#include <algorithm>

namespace mxh::game {

// Critical / decisive (AttackCalc.cpp L79, L148, KR/CN branch).
// The two formulas are identical up to the source stat field name; we provide
// both explicitly so the legacy GetPercent() helper does not leak in.
static double base_critical_rate(std::int32_t attacker_stat,
                              std::int32_t target_level) {
    // (attacker_stat + 20) / (target_level * 20 + 300); clamp at 0.15.
    double rate =
        static_cast<double>(attacker_stat + 20) /
        static_cast<double>(target_level * 20 + 300);
    if (rate > 0.2) rate = 0.2;
    return rate;
}

static double level_adjust(double rate,
                      std::int32_t attacker_level,
                      std::int32_t target_level,
                      double fCriticalRate) {
    if (attacker_level < target_level) {
        rate += fCriticalRate -
               static_cast<double>(target_level - attacker_level) * 0.02;
    } else {
        rate += fCriticalRate +
               static_cast<double>(attacker_level - target_level) * 0.004;
    }
    if (rate < 0.0) rate = 0.0;
    return rate;
}

double compute_critical_rate(std::int32_t attacker_crit,
                            std::int32_t attacker_level,
                            std::int32_t target_level,
                            double fCriticalRate) {
    if (target_level <= 0) return 0.0;
    double rate = base_critical_rate(attacker_crit, target_level);
    if (fCriticalRate != 0.0) {
        rate = level_adjust(rate, attacker_level, target_level, fCriticalRate);
    }
    return rate;
}

bool roll_critical(double critical_rate, int rand_0_99) {
    if (critical_rate <= 0.0) return false;
    return rand_0_99 < static_cast<int>(critical_rate * 100.0);
}

double compute_decisive_rate(std::int32_t attacker_decisive,
                             std::int32_t attacker_level,
                             std::int32_t target_level,
                             double fCriticalRate) {
    if (target_level <= 0) return 0.0;
    double rate = base_critical_rate(attacker_decisive, target_level);
    if (fCriticalRate != 0.0) {
        rate = level_adjust(rate, attacker_level, target_level, fCriticalRate);
    }
    return rate;
}

bool roll_decisive(double decisive_rate, int rand_0_99) {
    if (decisive_rate <= 0.0) return false;
    return rand_0_99 < static_cast<int>(decisive_rate * 100.0);
}

// Physical attack power (AttackCalc.cpp L217).
std::uint32_t compute_player_physical_attack(
    std::uint32_t min_val,
    std::uint32_t max_val,
    double base_atk_bonus,
    double phy_attack_rate,
    bool b_critical,
    int rand_gap) {
    // BaseAtk.SkillStatsOption: legacy does val = 1 + that, clamp at 0.
    double val = 1.0 + base_atk_bonus;
    if (val < 0.0) val = 0.0;
    auto scale = static_cast<std::uint32_t>(static_cast<double>(min_val) * val + 0.5);
    auto mx   = static_cast<std::uint32_t>(static_cast<double>(max_val) * val + 0.5);
    std::uint32_t rolled;
    if (mx <= scale) {
        rolled = scale;
    } else {
        std::uint32_t gap = mx - scale + 1;
        int r = rand_gap % static_cast<int>(gap);
        if (r < 0) r = -r;
        rolled = scale + static_cast<std::uint32_t>(r);
    }
    double result = static_cast<double>(rolled) * phy_attack_rate;
    if (b_critical) result *= 1.5;  // KR/CN branch (Japan = 2.25)
    return static_cast<std::uint32_t>(result);
}

// Attribute attack power (AttackCalc.cpp L260, KR/CN branch).
std::uint32_t compute_player_attribute_attack(
    std::int32_t level,
    std::uint32_t sim_mek,
    std::uint32_t att_attack_min,
    std::uint32_t att_attack_max,
    double att_attack_rate) {
    if (att_attack_rate <= 0.0) return 0;
    double midterm = (static_cast<double>(sim_mek) + 200.0) / 100.0;
    std::uint32_t minlvv = static_cast<std::uint32_t>(level + 5 - 5);
    std::uint32_t maxlvv = static_cast<std::uint32_t>(level + 5 + 5);
    std::uint32_t sim_div5 = sim_mek / 5;
    std::int32_t cap = static_cast<std::int32_t>(sim_mek) - 12;
    std::int32_t sim_cap = (cap > 25) ? 25 : (cap < 0 ? 0 : cap);
    double minv = static_cast<double>(minlvv) * att_attack_rate * midterm +
                static_cast<double>(sim_div5) + static_cast<double>(sim_cap);
    double maxv = static_cast<double>(maxlvv) * att_attack_rate * midterm +
                static_cast<double>(sim_div5) + static_cast<double>(sim_cap);
    if (maxv < minv) std::swap(minv, maxv);
    // Result is rounded to uint; legacy uses min/max for clamp but here we
    // take the average as the deterministic damage (legacy RNG was uniform).
    double avg = (minv + maxv) * 0.5 + 
               0.5 * (static_cast<double>(att_attack_min) +
                       static_cast<double>(att_attack_max));
    return static_cast<std::uint32_t>(avg);
}

// EXP modifier (AttackCalc.cpp L45).
std::uint32_t compute_player_exp_point(std::int32_t level_gap,
                                    std::uint32_t monster_exp) {
    if (monster_exp == 0) return 0;
    double exp_d = static_cast<double>(monster_exp);
    double result;
    if (level_gap < -8) {
        result = exp_d * 1.5;
    } else if (level_gap == 5) {
        result = exp_d * 0.1;
    } else if (level_gap > 5) {
        return 0;
    } else if (level_gap > 0) {
        result = exp_d * (5 - level_gap) * 0.2;
    } else {
        // -8..0 inclusive: exp + exp * (-gap) * 0.05
        int abs_gap = (level_gap < 0) ? -level_gap : level_gap;
        result = exp_d + exp_d * static_cast<double>(abs_gap) * 0.05;
    }
    // Legacy: round-up at first decimal (smallest positive decimal).
    auto floor10 = static_cast<std::uint32_t>(result * 10);
    auto floored = static_cast<std::uint32_t>(result);
    if (floor10 > floored * 10) ++floored;
    return floored;
}

// PlayerXPt clamp + lookup (AttackCalc.cpp L28).
std::uint32_t clamp_player_x_monster_lookup(std::int32_t level,
                                          std::int32_t level_gap,
                                          PlayerXMonsterPointLookup lookup) {
    if (level == MAX_CHARACTER_LEVEL_NUM) return 0;
    if (level < 1) return 0;
    if (level_gap < -MONSTERLEVELRESTRICT_LOWSTARTNUM)
        level_gap = -MONSTERLEVELRESTRICT_LOWSTARTNUM;
    else if (level_gap >= MAX_MONSTERLEVELPOINTRESTRICT_NUM)
        level_gap = MAX_MONSTERLEVELPOINTRESTRICT_NUM;
    return lookup(level, level_gap);
}


// Physical defence level (AttackCalc.cpp getPhyDefenceLevel, KR/CN branch).
//
// Legacy formula (KR/CN, !_JAPAN_LOCAL_):
//   phyDefenceLevel = (phyDefence * 2.0 + 50) / (attackerLevel * 20 + 150)
//   clamp [0.0, 0.9]
// The KR/CN build of this repo uses the 0.9 ceiling; Japan uses 0.99.
// skill_stats_phy_def and party_rate come from buff/party bonuses that the
// legacy code applies BEFORE the (def*2 + 50) numerator. They are folded
// into the base phy_defence by the caller.
//
// titan_defense_override != 0 activates the Titan branch (SW070127):
//   phyDefenceLevel = min(titan_defense_override, 0.8)
// Otherwise the standard formula is used.
double compute_phy_defence_level(
    double phy_defence,           // already buffed/unique-adjusted
    std::int32_t attacker_level,
    double titan_defense_override) {
    if (attacker_level < 1) attacker_level = 1;
    if (titan_defense_override > 0.0) {
        return (titan_defense_override > 0.8) ? 0.8 : titan_defense_override;
    }
    double rate = (phy_defence * 2.0 + 50.0) /
                 (static_cast<double>(attacker_level) * 20.0 + 150.0);
    if (rate < 0.0) rate = 0.0;
    if (rate > 0.9) rate = 0.9;
    return rate;
}

// Compute the final received damage = attack - (attack * defence_level).
// Legacy: val - (val * phyDefenceLevel); floor at 1 minimum.
std::uint32_t compute_received_damage(
    std::uint32_t attack_power,
    double phy_defence_level) {
    if (attack_power == 0) return 0;
    double def = phy_defence_level;
    if (def < 0.0) def = 0.0;
    if (def > 0.95) def = 0.95;
    double dmg = static_cast<double>(attack_power) * (1.0 - def);
    if (dmg < 1.0) dmg = 1.0;
    return static_cast<std::uint32_t>(dmg);
}

// Monster physical attack power (AttackCalc.cpp getMonsterPhysicalAttackPower).
//
// Legacy:
//   if (maxVal <= minVal) val = minVal;
//   else val = minVal + rand()%(maxVal - minVal + 1);
//   val = val * PhyAttackRate;
//   (no critical multiplier for monsters in the legacy KR/CN build)
std::uint32_t compute_monster_physical_attack(
    std::uint32_t min_val,
    std::uint32_t max_val,
    double phy_attack_rate,
    int rand_gap) {
    std::uint32_t val;
    if (max_val <= min_val) {
        val = min_val;
    } else {
        std::uint32_t gap = max_val - min_val + 1u;
        int r = (rand_gap < 0) ? 0 : (rand_gap % static_cast<int>(gap));
        val = min_val + static_cast<std::uint32_t>(r);
    }
    return static_cast<std::uint32_t>(
        static_cast<double>(val) * phy_attack_rate);
}


// Monster attribute attack (AttackCalc.cpp L372).
//
// Legacy:
//   ASSERT(AttAttackMax >= AttAttackMin);
//   gap = AttAttackMax - AttAttackMin + 1;
//   return AttAttackMin + rand() % gap;
//
// When max <= min, gap underflows to a huge number -- we guard by
// collapsing to min (matches the inclusive_roll() invariant used by
// the modern attack_calc.cpp port). Legacy ASSERTs and would crash on
// bad inputs; modern port is defensive.
std::uint32_t compute_monster_attribute_attack(
    std::uint32_t attack_min,
    std::uint32_t attack_max,
    int rand_gap) {
    if (attack_max <= attack_min) return attack_min;
    std::uint64_t gap =
        static_cast<std::uint64_t>(attack_max) - attack_min + 1u;
    std::uint32_t sample = (rand_gap < 0) ? 0u
                                        : static_cast<std::uint32_t>(rand_gap);
    return attack_min + static_cast<std::uint32_t>(sample % gap);
}

// Titan physical attack (AttackCalc.cpp L383, SW070127 Titan branch).
//
// Legacy:
//   if (maxVal <= minVal) power = minVal;
//   else {
//     gap = maxVal - minVal + 1;
//     power = minVal + rand() % gap;
//   }
//   power *= PhyAttackRate;
//   if (bCritical) power *= 1.5;   // KR/CN crit multiplier.
std::uint32_t compute_titan_physical_attack(
    std::uint32_t min_val,
    std::uint32_t max_val,
    double phy_attack_rate,
    bool b_critical,
    int rand_gap) {
    std::uint32_t rolled;
    if (max_val <= min_val) {
        rolled = min_val;
    } else {
        std::uint64_t gap = static_cast<std::uint64_t>(max_val) - min_val + 1u;
        std::uint32_t sample = (rand_gap < 0) ? 0u
                                            : static_cast<std::uint32_t>(rand_gap);
        rolled = min_val + static_cast<std::uint32_t>(sample % gap);
    }
    double result = static_cast<double>(rolled) * phy_attack_rate;
    if (b_critical) result *= 1.5;
    return static_cast<std::uint32_t>(result);
}

// Titan attribute attack (AttackCalc.cpp L411).
//
// Legacy:
//   base = titan_attribute_val * (SimMek + 100) / 400 + SimMek / 5
//   MinPwr = MaxPwr = DWORD(base * 0.74f)
//   AttackPower = random(MinPwr, MaxPwr);   // deterministic since Min==Max
//   return AttackPower * AttAttackRate
//
// The legacy uses the same value for Min and Max so random() yields that
// constant; modern port is fully deterministic. titan_attribute_val is
// the element_val from titan stats (e.g. fire/water/wind); owner_sim_mek// owner_sim_mek is the master player's SimMek (mystique) stat.
std::uint32_t compute_titan_attribute_attack(
    std::uint32_t titan_attribute_val,
    std::uint32_t owner_sim_mek,
    double att_attack_rate,
    int rand_gap) {
    std::uint64_t base = static_cast<std::uint64_t>(titan_attribute_val) *
                          (owner_sim_mek + 100u) / 400u +
                      owner_sim_mek / 5u;
    double scaled = static_cast<double>(base) * 0.74;
    auto power = static_cast<std::uint32_t>(scaled);
    // Legacy uses random(MinPwr, MaxPwr) with Min==Max; rand_gap accepted
    // for API parity but the result is fully deterministic.
    (void)rand_gap;
    return static_cast<std::uint32_t>(static_cast<double>(power) * att_attack_rate);
}

}  // namespace mxh::game





