#pragma once
#include "mxh/server/revive_shop_plan.hpp"
#include "mxh/server/shop_item_manager.hpp"
namespace mxh::server {
// Call on the same fenced snapshot as the penalty plan; publish only after commit.
inline std::optional<ShopItemManager> prepare_revive_shop_manager(
    const ShopItemManager& original,const mxh::db::LegacyShopAppearanceRows& before,
    const ReviveShopPlan& plan) {
    if(original.using_item_count()!=before.used_items.size()) return std::nullopt;
    std::unordered_set<std::uint16_t> seen;
    ShopItemManager result=original;
    for(const auto& row:before.used_items) {
        if(!seen.insert(row.item_id).second) return std::nullopt;
        const auto* entry=original.find_using_item(row.item_id);
        if(!entry || entry->Data.ShopItem.ItemBase.wIconIdx!=row.item_id ||
            entry->Data.ShopItem.ItemBase.dwDBIdx!=row.database_id ||
            entry->Data.ShopItem.Param!=row.parameter) return std::nullopt;
        const auto next=std::find_if(plan.rows.used_items.begin(),plan.rows.used_items.end(),
            [&](const auto& item) { return item.item_id==row.item_id; });
        if(next==plan.rows.used_items.end()) {
            if(!result.delete_using_item(row.item_id)) return std::nullopt;
        } else {
            if(next->database_id!=row.database_id) return std::nullopt;
            auto* mutable_entry=result.find_using_item_by_icon_idx_mutable(row.item_id);
            if(!mutable_entry) return std::nullopt;
            mutable_entry->Data.ShopItem.Param=next->parameter;
        }
    }
    if(result.using_item_count()!=plan.rows.used_items.size()) return std::nullopt;
    if(plan.protection.end_combined) result.set_protect_item_idx(0);
    return result;
}
} // namespace mxh::server
