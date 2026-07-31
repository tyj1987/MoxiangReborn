#pragma once

#include "mxh/server/player_monster_point.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace mxh::server {

inline constexpr std::uint32_t MAX_POINTACCEPTOBJECT_NUM = 2u;

struct DamageObj {
    std::uint32_t dwID = 0;
    std::uint32_t dwData = 0;
};

struct DistributerState {
    std::unordered_map<std::uint32_t, DamageObj> m_DamageObjectTableSolo;
    std::unordered_map<std::uint32_t, DamageObj> m_DamageObjectTableParty;
    std::uint32_t m_PlusDamage = 0;
    std::uint32_t m_1stPlayerID = 0;
    std::uint32_t m_1stPartyID = 0;
    std::uint32_t m_TotalDamage = 0;
};

inline DistributerState make_distributer() {
    return DistributerState{};
}

inline void distributer_release(DistributerState& s) {
    s.m_DamageObjectTableSolo.clear();
    s.m_DamageObjectTableParty.clear();
    s.m_PlusDamage = 0;
    s.m_1stPlayerID = 0;
    s.m_1stPartyID = 0;
    s.m_TotalDamage = 0;
}

inline void damage_init(DistributerState& s) {
    s.m_DamageObjectTableSolo.clear();
    s.m_DamageObjectTableParty.clear();
    s.m_PlusDamage = 0;
    s.m_TotalDamage = 0;
    s.m_1stPlayerID = 0;
    s.m_1stPartyID = 0;
}

inline void set_plus_total_damage(DistributerState& s, std::uint32_t damage) {
    s.m_PlusDamage = damage;
    s.m_TotalDamage = s.m_PlusDamage;
}

inline std::uint32_t get_total_damage(const DistributerState& s) {
    return s.m_TotalDamage;
}

inline void add_damage_object(DistributerState& s, std::uint32_t player_id,
                              std::uint32_t damage, bool in_party) {
    auto& table = in_party ? s.m_DamageObjectTableParty : s.m_DamageObjectTableSolo;
    const auto it = table.find(player_id);
    if (it == table.end())
        table[player_id] = DamageObj{player_id, damage};
    else
        it->second.dwData += damage;
    s.m_TotalDamage += damage;
    if (s.m_1stPlayerID == 0u) s.m_1stPlayerID = player_id;
}

inline std::optional<std::uint32_t> choose_one(const DistributerState& s,
                                               bool in_party) {
    const auto& table = in_party ? s.m_DamageObjectTableParty : s.m_DamageObjectTableSolo;
    std::uint32_t big_damage = 0u;
    std::uint32_t big_id = 0u;
    for (const auto& kv : table) {
        if (kv.second.dwData > big_damage) {
            big_damage = kv.second.dwData;
            big_id = kv.second.dwID;
        }
    }
    if (big_id == 0u) return std::nullopt;
    return big_id;
}

inline void delete_damaged_player(DistributerState& s, std::uint32_t player_id) {
    const auto it1 = s.m_DamageObjectTableSolo.find(player_id);
    if (it1 != s.m_DamageObjectTableSolo.end()) {
        if (s.m_TotalDamage >= it1->second.dwData) s.m_TotalDamage -= it1->second.dwData;
        s.m_DamageObjectTableSolo.erase(it1);
    }
    const auto it2 = s.m_DamageObjectTableParty.find(player_id);
    if (it2 != s.m_DamageObjectTableParty.end()) {
        if (s.m_TotalDamage >= it2->second.dwData) s.m_TotalDamage -= it2->second.dwData;
        s.m_DamageObjectTableParty.erase(it2);
    }
    if (s.m_1stPlayerID == player_id) s.m_1stPlayerID = 0u;
}

inline std::uint32_t scaled_damage_reward(std::uint32_t obtain_point,
                                          std::uint32_t damage,
                                          std::uint32_t total_life) {
    if (total_life == 0u) return 0u;
    const auto percent = static_cast<std::uint64_t>(damage) * 100u / total_life;
    if (percent >= 100u) return obtain_point;
    if (percent >= 80u) return static_cast<std::uint32_t>(obtain_point * 0.8);
    if (percent >= 60u) return static_cast<std::uint32_t>(obtain_point * 0.6);
    if (percent >= 40u) return static_cast<std::uint32_t>(obtain_point * 0.4);
    if (percent >= 20u) return static_cast<std::uint32_t>(obtain_point * 0.2);
    return 0u;
}

inline std::uint32_t calc_obtain_exp(const PlayerMonsterPointTable& table,
                                     std::uint32_t monster_level,
                                     std::uint32_t killer_level,
                                     std::uint32_t total_life,
                                     std::uint32_t damage,
                                     std::uint32_t player_max_num = 0u) {
    (void)player_max_num;
    if (total_life == 0u) return 0u;
    const auto monster = static_cast<std::int64_t>(monster_level);
    const auto killer = static_cast<std::int64_t>(killer_level);
    if (killer >= monster + 6) return 0u;
    if (killer <= monster - 9) monster_level = killer_level + 9u;
    const auto level = static_cast<std::uint16_t>(monster_level);
    const auto gap = static_cast<std::int32_t>(monster_level) -
                     static_cast<std::int32_t>(killer_level);
    return scaled_damage_reward(table.get_player_point(level, gap), damage, total_life);
}

inline std::uint32_t calc_obtain_ability_exp(std::uint32_t monster_level,
                                             std::uint32_t killer_level) {
    if (monster_level + 5u < killer_level) return 0u;
    if (killer_level + 9u < monster_level) monster_level = killer_level + 9u;
    return (monster_level - killer_level + 5u) * 10u;
}

}  // namespace mxh::server
