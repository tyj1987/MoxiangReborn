// distributer.hpp - Phase D5 1:1 port of legacy [Server]Map/Distributer.h.
// Tracks per-player damage contributions to a monster and computes exp /
// item distribution. The actual party-level fan-out is driven by callers;
// here we model the bookkeeping + the legacy exp formula.

#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr std::uint32_t MAX_POINTACCEPTOBJECT_NUM = 2u;

// ---- POD structs ----

// Mirrors legacy DAMAGEOBJ.
struct DamageObj {
    std::uint32_t dwID   = 0;
    std::uint32_t dwData = 0;  // damage dealt
};

// Mirrors legacy CDistributer.
struct DistributerState {
    std::unordered_map<std::uint32_t, DamageObj> m_DamageObjectTableSolo;
    std::unordered_map<std::uint32_t, DamageObj> m_DamageObjectTableParty;
    std::uint32_t m_PlusDamage = 0;
    std::uint32_t m_1stPlayerID = 0;
    std::uint32_t m_1stPartyID  = 0;
    std::uint32_t m_TotalDamage = 0;
};

// ---- Free functions ----

inline DistributerState make_distributer() {
    return DistributerState{};
}

inline void distributer_release(DistributerState& s) {
    s.m_DamageObjectTableSolo.clear();
    s.m_DamageObjectTableParty.clear();
    s.m_PlusDamage   = 0;
    s.m_1stPlayerID  = 0;
    s.m_1stPartyID   = 0;
    s.m_TotalDamage  = 0;
}

inline void damage_init(DistributerState& s) {
    s.m_DamageObjectTableSolo.clear();
    s.m_DamageObjectTableParty.clear();
    s.m_PlusDamage   = 0;
    s.m_TotalDamage  = 0;
    s.m_1stPlayerID  = 0;
    s.m_1stPartyID   = 0;
}

inline void set_plus_total_damage(DistributerState& s, std::uint32_t damage) {
    s.m_PlusDamage = damage;
    s.m_TotalDamage = s.m_PlusDamage;
}

inline std::uint32_t get_total_damage(const DistributerState& s) {
    return s.m_TotalDamage;
}

// AddDamageObject: legacy splits into Solo (player not in a party) and
// Party (player is in a party) tables. The `in_party` flag selects which.
inline void add_damage_object(DistributerState& s, std::uint32_t player_id,
                              std::uint32_t damage, bool in_party) {
    auto& table = in_party ? s.m_DamageObjectTableParty : s.m_DamageObjectTableSolo;
    auto it = table.find(player_id);
    if (it == table.end()) {
        table[player_id] = DamageObj{player_id, damage};
    } else {
        it->second.dwData += damage;
    }
    s.m_TotalDamage += damage;
    if (s.m_1stPlayerID == 0u) s.m_1stPlayerID = player_id;
}

// ChooseOne: legacy finds the player with the largest damage in a single
// damage-object table. We expose that helper so callers can implement
// the "biggest contributor gets first pick" rule.
inline std::optional<std::uint32_t> choose_one(const DistributerState& s, bool in_party) {
    const auto& table = in_party ? s.m_DamageObjectTableParty : s.m_DamageObjectTableSolo;
    std::uint32_t big_damage = 0u;
    std::uint32_t big_id     = 0u;
    for (const auto& kv : table) {
        if (kv.second.dwData > big_damage) {
            big_damage = kv.second.dwData;
            big_id     = kv.second.dwID;
        }
    }
    if (big_id == 0u) return std::nullopt;
    return big_id;
}

// DeleteDamagedPlayer: removes a player from both tables; resets first
// attacker if needed.
inline void delete_damaged_player(DistributerState& s, std::uint32_t player_id) {
    auto it1 = s.m_DamageObjectTableSolo.find(player_id);
    if (it1 != s.m_DamageObjectTableSolo.end()) {
        if (s.m_TotalDamage >= it1->second.dwData) s.m_TotalDamage -= it1->second.dwData;
        s.m_DamageObjectTableSolo.erase(it1);
    }
    auto it2 = s.m_DamageObjectTableParty.find(player_id);
    if (it2 != s.m_DamageObjectTableParty.end()) {
        if (s.m_TotalDamage >= it2->second.dwData) s.m_TotalDamage -= it2->second.dwData;
        s.m_DamageObjectTableParty.erase(it2);
    }
    if (s.m_1stPlayerID == player_id) s.m_1stPlayerID = 0u;
}

// CalcObtainExp: legacy formula locks the exp share a player gets based on
// monster level, killer level, total monster life, the player's damage,
// and the maximum number of party members sharing the kill.
//
//   ratio = damage / total_life
//   level_diff = max(0, monster_level - killer_level) clamped at +5
//   base = monster_level * 10
//   exp = base * (1 + level_diff/10) * ratio * (player_max_num == 1 ? 1 : 0.5)
//
// We implement the integer-math version that yields integer floor values.
inline std::uint32_t calc_obtain_exp(std::uint32_t monster_level,
                                     std::uint32_t killer_level,
                                     std::uint32_t total_life,
                                     std::uint32_t damage,
                                     std::uint32_t player_max_num) {
    if (total_life == 0u || damage == 0u) return 0u;
    if (damage > total_life) damage = total_life;
    // ratio = damage * 1000 / total_life  (basis points of life)
    const std::uint64_t ratio_bp = (static_cast<std::uint64_t>(damage) * 1000u) / total_life;
    const std::int64_t diff = static_cast<std::int64_t>(monster_level) - static_cast<std::int64_t>(killer_level);
    const std::int64_t clamped_diff = (diff < 0) ? 0 : (diff > 5 ? 5 : diff);
    const std::uint64_t base = static_cast<std::uint64_t>(monster_level) * 10u;
    const std::uint64_t level_mult = 100u + static_cast<std::uint64_t>(clamped_diff) * 10u;
    std::uint64_t exp = (base * level_mult * ratio_bp) / 1000u / 100u;
    if (player_max_num > 1u) exp /= 2u;
    if (exp > 0xFFFFFFFFu) exp = 0xFFFFFFFFu;
    return static_cast<std::uint32_t>(exp);
}

// CalcObtainAbilityExp: legacy ability exp uses a different multiplier.
inline std::uint32_t calc_obtain_ability_exp(std::uint32_t monster_level,
                                             std::uint32_t killer_level) {
    const std::int64_t diff = static_cast<std::int64_t>(monster_level) - static_cast<std::int64_t>(killer_level);
    const std::int64_t clamped = (diff < 0) ? 0 : (diff > 5 ? 5 : diff);
    const std::uint64_t base = static_cast<std::uint64_t>(monster_level) * 5u;
    const std::uint64_t mult = 100u + static_cast<std::uint64_t>(clamped) * 5u;
    std::uint64_t exp = (base * mult) / 100u;
    if (exp > 0xFFFFFFFFu) exp = 0xFFFFFFFFu;
    return static_cast<std::uint32_t>(exp);
}

} // namespace mxh::server
