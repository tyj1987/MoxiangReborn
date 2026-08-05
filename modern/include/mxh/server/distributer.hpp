#pragma once

#include "mxh/server/player_monster_point.hpp"

#include <cstdint>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

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
    // Legacy [Server]Map/Distributer.cpp::SetPlusTotalDamage uses +=, not =.
    // 1:1 with `m_TotalDamage += Damage; m_PlusDamage = Damage;` from
    // the original. Modern port had been assigning m_TotalDamage directly,
    // which dropped cumulative plus damage (e.g. back-to-back crit
    // SetPlusTotalDamage calls in the legacy boss/field-boss reward path).
    s.m_TotalDamage += damage;
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
                                               bool in_party,
                                               int rand_0_1 = 0) {
    const auto& table = in_party ? s.m_DamageObjectTableParty : s.m_DamageObjectTableSolo;
    std::uint32_t big_damage = 0u;
    std::uint32_t big_id = 0u;
    for (const auto& kv : table) {
        const auto damage = kv.second.dwData;
        const auto id = kv.second.dwID;
        if (damage > big_damage) {
            big_damage = damage;
            big_id = id;
        } else if (damage == big_damage && big_id != 0u) {
            // Legacy 1:1 tie-break: when two attackers deal identical
            // damage, legacy ChooseOne flips a `rand()%2 == 1` coin to
            // decide whether the later entry replaces the current big.
            // `rand_0_1` is the integer sample in [0, 2); pass 1 to
            // force a replacement, 0 to keep the first winner.
            if ((rand_0_1 & 1) == 1) {
                big_id = id;
            }
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

// ---- Recipient decision (Distributer.cpp L195-220, 1:1 lock) ----
//
// Legacy CDistributer::Distribute picks the recipient of the post-kill
// ability / item drop from BigPlayerID (winner of the solo table) and
// BigPartyID (winner of the party table). The decision tree is:
//
//   if (BigPartyDamage <  BigPlayerDamage) -> Personal
//   else if (BigPartyDamage == BigPlayerDamage) {
//       if (pParty && pParty->IsPartyMember(BigPlayerID))
//           -> Party    // 1:1 quirk: tie + member -> Party
//       else if (rand_0_1 == 0) -> Personal
//       else                    -> Party
//   }
//   else    // BigPartyDamage > BigPlayerDamage
//       -> Party
//
// `party_contains_big_player` is the host-resolved `IsPartyMember(...)`
// bit (party tables are userdata outside this header). Null means pParty is absent.
// `rand_0_1` is the integer sample in [0, 2).
enum class DistributeRecipient { None, Personal, Party };
struct RecipientDecision {
    DistributeRecipient kind = DistributeRecipient::None;
    std::uint32_t party_id;
    std::uint32_t player_id;
};
inline RecipientDecision distribute_decide_recipient(
    std::uint32_t big_player_id, std::uint32_t big_player_damage,
    std::uint32_t big_party_id, std::uint32_t big_party_damage,
    std::optional<bool> party_contains_big_player, int rand_0_1 = 0) {
    RecipientDecision r{};
    if (big_party_damage < big_player_damage) {
        r = {DistributeRecipient::Personal, 0u, big_player_id};
    } else if (big_party_damage == big_player_damage) {
        if (!party_contains_big_player.has_value()) {
            // Legacy outer if (pParty) has no else: a missing party
            // sends nothing in the tie branch, even after the coin is drawn.
            r = {DistributeRecipient::None, 0u, 0u};
        } else if (*party_contains_big_player) {
            r = {DistributeRecipient::Party, big_party_id, 0u};
        } else if ((rand_0_1 & 1) == 0) {
            r = {DistributeRecipient::Personal, 0u, big_player_id};
        } else {
            r = {DistributeRecipient::Party, big_party_id, 0u};
        }
    } else {
        r = {DistributeRecipient::Party, big_party_id, 0u};
    }
    return r;
}

// ---- Party experience allocation (Distributer.cpp L257-298, 1:1 lock) ----
//
// Legacy CalcAndSendPartyExp does two passes:
//   (1) walk party members, collect per-member level + max level +
//       online count + level average;
//   (2) for each online member, allocate
//         exp = (partyexp * curlevel * (10 + online) / 9 / levelavg)
//               / online
//       with the singular `online == 1` quirk: the whole partyexp goes
//       to that one member unchanged.
//
// The legacy `(curlevel * (10 + online) / 9.f / levelavg) / online` step
// runs in float on the legacy build; the modern port keeps the same float
// operations and truncates back to uint32. The host-side `gEventRate[eEvent_PartyExpRate]`
// multiplier is intentionally NOT applied here -- callers fold it in
// after this pure function returns so the rate can be host-driven.
struct PartyMemberExp {
    std::uint32_t member_id = 0;
    std::uint32_t exp = 0;
};
struct PartyExpAllocation {
    std::uint32_t online_count = 0;
    std::uint32_t max_level = 0;
    float level_average = 0.0f;
    std::vector<PartyMemberExp> members;
};
inline PartyExpAllocation allocate_party_exp_per_member(
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>& online_members,
    std::uint32_t partyexp) {
    PartyExpAllocation out{};
    if (partyexp == 0u) return out;
    float level_sum = 0.0f;
    std::uint32_t max_level = 0u;
    for (const auto& m : online_members) {
        if (m.first == 0u) continue;
        ++out.online_count;
        level_sum += static_cast<float>(m.second);
        if (max_level < m.second) max_level = m.second;
    }
    out.max_level = max_level;
    out.level_average = out.online_count > 0u
        ? level_sum / static_cast<float>(out.online_count)
        : 0.0f;
    out.members.reserve(out.online_count);
    for (const auto& m : online_members) {
        if (m.first == 0u) continue;
        std::uint32_t exp = 0u;
        if (out.online_count != 1u) {
            const auto product = m.second * (10u + out.online_count);
            const float scaled = static_cast<float>(product) / 9.0f /
                                 out.level_average /
                                 static_cast<float>(out.online_count);
            exp = static_cast<std::uint32_t>(static_cast<float>(partyexp) * scaled);
        } else {
            exp = partyexp;
        }
        if (exp != 0u) out.members.push_back({m.first, exp});
    }
    return out;
}

// Legacy MP_CHAR_EXPPOINT_ACK wire: MSGBASE + EXPTYPE + BYTE. EXPTYPE is
// INT64 in CommonStruct.h; the explicit offsets preserve the packed layout.
inline std::vector<std::uint8_t> serialize_exp_point_wire(
    std::uint32_t object_id, std::int64_t exp_point, std::uint8_t exp_kind,
    std::uint8_t category = 3u, std::uint8_t protocol = 13u) {
    std::vector<std::uint8_t> wire(17u, 0u);
    wire[2] = category;
    wire[3] = protocol;
    std::memcpy(wire.data() + 4u, &object_id, sizeof(object_id));
    std::memcpy(wire.data() + 8u, &exp_point, sizeof(exp_point));
    wire[16] = exp_kind;
    return wire;
}

// Legacy MP_CHAR_ABILITYEXPPOINT_ACK wire: MSGBASE + DWORD + BYTE.
inline std::vector<std::uint8_t> serialize_ability_exp_point_wire(
    std::uint32_t object_id, std::uint32_t exp_point, std::uint8_t exp_kind,
    std::uint8_t category = 3u, std::uint8_t protocol = 29u) {
    std::vector<std::uint8_t> wire(13u, 0u);
    wire[2] = category;
    wire[3] = protocol;
    std::memcpy(wire.data() + 4u, &object_id, sizeof(object_id));
    std::memcpy(wire.data() + 8u, &exp_point, sizeof(exp_point));
    wire[12] = exp_kind;
    return wire;
}

}  // namespace mxh::server
