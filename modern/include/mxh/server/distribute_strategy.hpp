#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

enum class DistributeStrategy : std::uint8_t {
    Damage = 0,
    Random = 1,
};

struct PartyReceiveMember {
    std::uint32_t playerId = 0;
    std::uint32_t damage = 0;
    bool available = true;
};

struct PartyMoneyShare {
    std::uint32_t perMember = 0;
    std::uint32_t distributed = 0;
    std::uint32_t remainder = 0;
};

inline std::optional<std::size_t> select_random_receiver(
    const std::vector<PartyReceiveMember>& members, std::uint32_t randomValue) {
    if (members.empty()) return std::nullopt;
    const auto index = static_cast<std::size_t>(randomValue % members.size());
    if (!members[index].available) return std::nullopt;
    return index;
}

inline std::optional<std::size_t> select_damage_receiver_legacy(
    const std::vector<PartyReceiveMember>& members,
    const std::vector<std::uint32_t>& tieRandomValues = {}) {
    if (members.empty()) return std::nullopt;

    std::uint32_t bigDamage = 0;
    std::size_t selected = 0;
    std::size_t tieIndex = 0;
    for (std::size_t index = 0; index < members.size(); ++index) {
        if (bigDamage < members[index].damage) {
            selected = index;
        } else if (bigDamage == members[index].damage) {
            const auto randomValue = tieIndex < tieRandomValues.size()
                ? tieRandomValues[tieIndex] : 0u;
            ++tieIndex;
            if (randomValue % 2u == 1u) selected = index;
        }
    }
    if (!members[selected].available) return std::nullopt;
    return selected;
}

inline PartyMoneyShare split_party_money(std::uint32_t money, std::size_t memberCount) {
    if (memberCount == 0u) return {};
    const auto count = static_cast<std::uint32_t>(memberCount);
    const auto perMember = money / count;
    return {perMember, perMember * count, money - perMember * count};
}

inline bool legacy_drop_ratio_hit(std::uint32_t dropItemRatio,
                                  std::uint32_t randomValue) {
    if (dropItemRatio == 0u || dropItemRatio > 100u) return false;
    const auto divisor = 100u / dropItemRatio;
    if (divisor == 0u) return false;
    return (randomValue % 100u) % divisor == 0u;
}

inline bool can_distribute_item(std::size_t memberCount, bool levelAllowed) {
    return memberCount != 0u && levelAllowed;
}

}
