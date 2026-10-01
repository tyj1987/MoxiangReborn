#pragma once
#include "mxh/db/legacy_shop_appearance.hpp"
#include "mxh/game/exp_penalty.hpp"
#include "mxh/proto/protocol.hpp"
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace mxh::server {
struct ReviveShopNotice {
    mxh::proto::ItemProtocol protocol;
    std::uint32_t value;
};
struct ReviveShopPlan {
    mxh::game::ReviveProtectionPlan protection;
    mxh::db::LegacyShopAppearanceRows rows;
    // Ordered MSG_DWORD notices; dispatch only after the full revive commits.
    std::vector<ReviveShopNotice> notices;
    bool reduce_pet_friendship = false;
};

// Produces a candidate only. Caller commits this with money/experience/vitals
// before applying it to the live ShopItemManager or sending use-end notifications.
inline std::optional<ReviveShopPlan> prepare_revive_shop(
    const mxh::db::LegacyShopAppearanceRows& original, mxh::game::ReviveLoss loss,
    std::int8_t combined_count, std::uint32_t combined_item) {
    constexpr std::uint16_t money_item = 55311, experience_item = 55312;
    if (combined_item > 65535) return std::nullopt;
    std::unordered_set<std::uint16_t> icons;
    for (const auto& row : original.used_items)
        if (!row.item_id || !icons.insert(row.item_id).second) return std::nullopt;
    const auto protection = mxh::game::plan_revive_protection(loss, combined_count,
        combined_item != 0 && icons.count(static_cast<std::uint16_t>(combined_item)) != 0,
        icons.count(money_item) != 0, icons.count(experience_item) != 0);
    if (!protection) return std::nullopt;
    ReviveShopPlan result{*protection, original};
    auto& rows = result.rows.used_items;
    rows.erase(std::remove_if(rows.begin(), rows.end(), [&](auto& row) {
        if (protection->consume_combined && row.item_id == combined_item) {
            if (protection->end_combined) return true;
            row.parameter = static_cast<std::uint32_t>(protection->remaining_combined);
        }
        return (protection->consume_money && row.item_id == money_item) ||
               (protection->consume_experience && row.item_id == experience_item);
    }), rows.end());
    if (protection->consume_combined) {
        if (protection->end_combined)
            result.notices.push_back({mxh::proto::ItemProtocol::ShopItemUseEnd,combined_item});
        result.notices.push_back({mxh::proto::ItemProtocol::ShopItemProtectAll,
            static_cast<std::uint32_t>(protection->remaining_combined)});
    } else {
        if (protection->consume_money)
            result.notices.push_back({mxh::proto::ItemProtocol::ShopItemMoneyProtect,money_item});
        if (protection->consume_experience)
            result.notices.push_back({mxh::proto::ItemProtocol::ShopItemExpProtect,experience_item});
        else result.reduce_pet_friendship=true;
    }
    return result;
}
} // namespace mxh::server
