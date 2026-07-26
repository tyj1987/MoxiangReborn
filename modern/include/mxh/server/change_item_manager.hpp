#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mxh::server {

inline constexpr std::uint32_t MAX_CHANGE_RATE = 30001u;
inline constexpr std::uint32_t MAX_YOUNGYAKITEM_DUPNUM = 20u;
inline constexpr std::uint16_t CHANGE_REWARD_MONEY = 7999u;
inline constexpr std::uint16_t CHANGE_REWARD_EVENT = 7998u;
inline constexpr std::uint16_t CHANGE_REWARD_ABILITY = 7997u;
inline constexpr std::uint16_t CHANGE_REWARD_EVENT2 = 7996u;

struct ChangeItemUnit {
    std::uint16_t wToItemIdx = 0;
    std::uint32_t dwToItemDur = 0;
    std::uint32_t dwPercent = 0;
    bool stackable = false;
};

struct ChangeItemSet {
    std::uint16_t wItemIdx = 0;
    std::vector<ChangeItemUnit> pItemUnit;
};

struct MultiChangeItem {
    std::uint16_t wItemIdx = 0;
    std::uint16_t wLimitLevel = 0;
    std::vector<ChangeItemSet> pChangeItem;
    std::uint32_t nMaxItemSpace = 0;
};

struct ChangeItemManagerState {
    std::unordered_map<std::uint16_t, ChangeItemSet> m_ChangeItemList;
    std::unordered_map<std::uint16_t, MultiChangeItem> m_MultiChangeItemList;
};

enum class ChangeRewardKind : std::uint8_t {
    Item,
    Money,
    Event,
    Ability,
    Event2,
};

inline ChangeRewardKind change_reward_kind(std::uint16_t itemIdx) {
    if (itemIdx == CHANGE_REWARD_MONEY) return ChangeRewardKind::Money;
    if (itemIdx == CHANGE_REWARD_EVENT) return ChangeRewardKind::Event;
    if (itemIdx == CHANGE_REWARD_ABILITY) return ChangeRewardKind::Ability;
    if (itemIdx == CHANGE_REWARD_EVENT2) return ChangeRewardKind::Event2;
    return ChangeRewardKind::Item;
}

inline bool register_change_item(ChangeItemManagerState& state, ChangeItemSet set) {
    if (set.wItemIdx == 0u) return false;
    state.m_ChangeItemList.insert_or_assign(set.wItemIdx, std::move(set));
    return true;
}

inline bool register_multi_change_item(ChangeItemManagerState& state, MultiChangeItem item) {
    if (item.wItemIdx == 0u) return false;
    state.m_MultiChangeItemList.insert_or_assign(item.wItemIdx, std::move(item));
    return true;
}

inline const ChangeItemUnit* select_change_item_unit(const ChangeItemSet& set,
                                                     std::uint32_t randomRate) {
    std::uint32_t upper = 0;
    for (const auto& unit : set.pItemUnit) {
        const auto lower = upper;
        upper += unit.dwPercent;
        if (lower <= randomRate && randomRate < upper) return &unit;
    }
    return nullptr;
}

inline std::uint32_t make_hk_change_random(std::uint32_t high,
                                           std::uint32_t low) {
    return (high % 1000u) * 1000u + (low % 1000u);
}

inline std::uint32_t make_default_change_random(std::uint32_t randomValue) {
    return randomValue % MAX_CHANGE_RATE;
}

struct MaxSpaceSelection {
    const ChangeItemUnit* unit = nullptr;
    std::uint32_t maxNonStackableDuration = 0;
};

inline MaxSpaceSelection get_max_space_item_ref(const ChangeItemSet& set) {
    MaxSpaceSelection result;
    std::uint32_t maxDuration = 0;
    for (const auto& unit : set.pItemUnit) {
        if (maxDuration < unit.dwToItemDur) {
            maxDuration = unit.dwToItemDur;
            result.unit = &unit;
        }
        if (!unit.stackable && result.maxNonStackableDuration < unit.dwToItemDur)
            result.maxNonStackableDuration = unit.dwToItemDur;
    }
    return result;
}

inline std::uint32_t required_slots_for_reward(const ChangeItemUnit& unit) {
    const auto kind = change_reward_kind(unit.wToItemIdx);
    if (kind == ChangeRewardKind::Money || kind == ChangeRewardKind::Ability)
        return 0u;
    if (unit.stackable) {
        const auto fullStacks = unit.dwToItemDur / MAX_YOUNGYAKITEM_DUPNUM;
        return fullStacks + (unit.dwToItemDur % MAX_YOUNGYAKITEM_DUPNUM != 0u ? 1u : 0u);
    }
    return unit.dwToItemDur;
}

inline std::uint32_t changed_total_item_num_legacy(const MultiChangeItem& multi) {
    std::uint32_t total = 0;
    const ChangeItemUnit* staleUnit = nullptr;
    std::uint32_t staleMin = 0;
    for (const auto& set : multi.pChangeItem) {
        std::uint32_t nMin = 0;
        if (set.pItemUnit.size() == 1u) {
            ++total;
        } else {
            const auto selection = get_max_space_item_ref(set);
            staleUnit = selection.unit;
            staleMin = selection.maxNonStackableDuration;
            nMin = staleMin;
        }

        if (staleUnit != nullptr) {
            const auto kind = change_reward_kind(staleUnit->wToItemIdx);
            if (kind == ChangeRewardKind::Money || kind == ChangeRewardKind::Ability)
                continue;
            if (staleUnit->stackable) {
                const auto stackSlots = required_slots_for_reward(*staleUnit);
                total += nMin > staleUnit->dwToItemDur / MAX_YOUNGYAKITEM_DUPNUM
                    ? nMin : stackSlots;
            } else {
                total += staleUnit->dwToItemDur;
            }
        }
    }
    return total;
}

inline bool has_change_item_space(std::uint32_t emptySlots,
                                  std::uint32_t requiredSlots) {
    return emptySlots + 1u >= requiredSlots;
}

inline const ChangeItemSet* find_change_item(const ChangeItemManagerState& state,
                                             std::uint16_t itemIdx) {
    const auto it = state.m_ChangeItemList.find(itemIdx);
    return it == state.m_ChangeItemList.end() ? nullptr : &it->second;
}

inline const MultiChangeItem* find_multi_change_item(const ChangeItemManagerState& state,
                                                     std::uint16_t itemIdx) {
    const auto it = state.m_MultiChangeItemList.find(itemIdx);
    return it == state.m_MultiChangeItemList.end() ? nullptr : &it->second;
}

}
