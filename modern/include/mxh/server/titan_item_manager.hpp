#pragma once

#include "mxh/server/player.hpp"
#include "mxh/server/titan_manager.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

struct TitanPartsKind final {
    std::uint32_t dwPartsIdx = 0;
    std::uint32_t dwPartsKind = 0;
    std::uint32_t dwResultTitanIdx = 0;
};

struct TitanUpgradeMaterial final {
    std::uint32_t dwIndex = 0;
    std::uint32_t dwCount = 0;
};

struct TitanUpgradeInfo final {
    std::uint32_t dwTitanIdx = 0;
    std::uint32_t dwNextTitanIdx = 0;
    std::uint32_t dwMoney = 0;
    std::vector<TitanUpgradeMaterial> materials;
};

struct TitanBreakMaterial final {
    std::uint32_t dwMaterialIdx = 0;
    std::uint16_t wMaterCnt = 0;
    std::uint16_t wRate = 0;
};

struct TitanBreakInfo final {
    std::uint32_t dwIdx = 0;
    std::uint32_t dwMoney = 0;
    std::uint16_t wTotalCnt = 0;
    std::uint16_t wGetCnt = 0;
    std::vector<TitanBreakMaterial> materials;
};

struct TitanMaterialStack final {
    std::uint32_t item_idx = 0;
    std::uint32_t count = 0;
};

struct TitanBreakResult final {
    std::vector<TitanMaterialStack> materials;
};

struct TitanUpgradeInventory final {
    std::uint32_t money = 0;
    std::vector<TitanMaterialStack> materials;
};

inline const TitanPartsKind* find_titan_parts_kind(
    const std::vector<TitanPartsKind>& table,
    std::uint32_t parts_idx) noexcept {
    const auto it = std::find_if(table.begin(), table.end(), [parts_idx](const auto& entry) {
        return entry.dwPartsIdx == parts_idx;
    });
    return it == table.end() ? nullptr : &*it;
}

inline std::uint32_t titan_material_count(const TitanUpgradeInventory& inventory,
                                          std::uint32_t item_idx) noexcept {
    std::uint64_t total = 0;
    for (const auto& stack : inventory.materials) {
        if (stack.item_idx == item_idx) {
            total += stack.count;
        }
    }
    return total > UINT32_MAX ? UINT32_MAX : static_cast<std::uint32_t>(total);
}

inline bool can_afford_titan_upgrade(const TitanUpgradeInventory& inventory,
                                     const TitanUpgradeInfo& info) noexcept {
    if (info.dwTitanIdx == 0 || info.dwNextTitanIdx == 0 || inventory.money < info.dwMoney) {
        return false;
    }
    for (const auto& required : info.materials) {
        if (required.dwIndex == 0 || required.dwCount == 0 ||
            titan_material_count(inventory, required.dwIndex) < required.dwCount) {
            return false;
        }
    }
    return true;
}

inline bool consume_titan_upgrade_cost(TitanUpgradeInventory& inventory,
                                       const TitanUpgradeInfo& info) noexcept {
    if (!can_afford_titan_upgrade(inventory, info)) {
        return false;
    }
    inventory.money -= info.dwMoney;
    for (const auto& required : info.materials) {
        std::uint32_t remaining = required.dwCount;
        for (auto& stack : inventory.materials) {
            if (stack.item_idx != required.dwIndex) {
                continue;
            }
            const std::uint32_t consumed = std::min(stack.count, remaining);
            stack.count -= consumed;
            remaining -= consumed;
            if (remaining == 0) {
                break;
            }
        }
    }
    inventory.materials.erase(
        std::remove_if(inventory.materials.begin(), inventory.materials.end(),
                       [](const auto& stack) { return stack.count == 0; }),
        inventory.materials.end());
    return true;
}

inline bool apply_titan_upgrade(TitanUpgradeInventory& inventory,
                                TitanTotalInfo& titan,
                                const TitanUpgradeInfo& info) noexcept {
    if (titan.TitanKind != info.dwTitanIdx || !consume_titan_upgrade_cost(inventory, info)) {
        return false;
    }
    titan.TitanKind = static_cast<std::uint16_t>(info.dwNextTitanIdx);
    if (titan.TitanGrade < MAX_TITANGRADE) {
        ++titan.TitanGrade;
    }
    return true;
}

inline TitanBreakResult roll_titan_break(const TitanBreakInfo& info,
                                         std::uint32_t total_rate,
                                         const std::vector<std::uint32_t>& rolls) {
    TitanBreakResult result;
    if (total_rate == 0 || info.materials.empty()) {
        return result;
    }

    const std::size_t table_count = std::min<std::size_t>(info.wTotalCnt, info.materials.size());
    const std::size_t get_count = std::min<std::size_t>({info.wGetCnt, table_count, rolls.size()});
    std::vector<bool> selected(table_count, false);
    std::uint32_t remaining_rate = total_rate;

    for (std::size_t pick = 0; pick < get_count && remaining_rate != 0; ++pick) {
        const std::uint32_t legacy_roll = rolls[pick] % (remaining_rate + 1u);
        std::uint32_t cumulative = 0;
        for (std::size_t index = 0; index < table_count; ++index) {
            if (selected[index]) {
                continue;
            }
            cumulative += info.materials[index].wRate;
            if (cumulative >= legacy_roll) {
                const auto& material = info.materials[index];
                selected[index] = true;
                remaining_rate = remaining_rate >= material.wRate
                    ? remaining_rate - material.wRate
                    : 0;
                result.materials.push_back({material.dwMaterialIdx, material.wMaterCnt});
                break;
            }
        }
    }
    return result;
}

}  // namespace mxh::server