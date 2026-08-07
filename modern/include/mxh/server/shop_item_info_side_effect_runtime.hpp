// shop_item_info_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_info_side_effect_plan(). The data plane returns an empty
// plan (no player) or a 2-step chain (SetShopItemInit ->
// FireShopItemDbQuery); this header walks the plan and dispatches
// each entry to a virtual ShopItemInfoSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_INFO_SYN from
// [Server]Map/ItemManager.cpp:5021-5032):
//   - FindUser returns null: handler returns (empty plan).
//   - Player found: handler (1) pPlayer->SetShopItemInit(FALSE),
//     then (2) CharacterShopItemInfo(pPlayer->GetID(), 0) -- the DB
//     query that re-loads the shop item info.
//   - The handler sends NO ACK/NACK to the client; the data arrives
//     later via MP_ITEM_SHOPITEM broadcasts from the DB callback.
//
// Pattern mirrors check_end_time_side_effect_runtime.hpp (multi-step
// chains) and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_info_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemInfo side-effect chain.
class ShopItemInfoSideEffectSink {
public:
    virtual ~ShopItemInfoSideEffectSink() = default;

    // Legacy: pPlayer->SetShopItemInit(FALSE) -- marks the player as
    // needing to re-init shop item state.
    virtual void set_shop_item_init(std::uint32_t object_id) = 0;

    // Legacy: CharacterShopItemInfo(pPlayer->GetID(), 0) -- fires the
    // DB query that re-loads the shop item info.
    virtual void fire_shop_item_db_query(std::uint32_t object_id,
                                         std::uint32_t start_db_idx) = 0;
};

struct ShopItemInfoRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t init_resets      = 0;
    std::size_t db_queries       = 0;
    bool reset_init_flag_consumed  = false;
    bool trigger_db_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ShopItemInfoRuntimeOutcome apply_shop_item_info_side_effects(
    const ShopItemInfoSideEffectPlan& plan,
    ShopItemInfoSideEffectSink& sink) {
    ShopItemInfoRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemInfoSideEffectKind::SetShopItemInit:
            sink.set_shop_item_init(effect.object_id);
            ++out.init_resets;
            ++out.effects_applied;
            break;
        case ShopItemInfoSideEffectKind::FireShopItemDbQuery:
            sink.fire_shop_item_db_query(effect.object_id,
                                         effect.start_db_idx);
            ++out.db_queries;
            ++out.effects_applied;
            break;
        }
    }
    out.reset_init_flag_consumed = plan.reset_init;
    out.trigger_db_flag_consumed = plan.trigger_db;
    return out;
}

}  // namespace mxh::server
