// shop_item_mpinfo_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_mpinfo_side_effect_plan(). The data plane returns an
// empty plan (no player) or a single FireSavedMovePointDbQuery
// entry; this header walks the plan and dispatches the entry to a
// virtual ShopItemMpInfoSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_MPINFO from
// [Server]Map/ItemManager.cpp:5095-5103):
//   - FindUser returns null: handler returns (empty plan).
//   - Player found: handler fires SavedMovePointInfo(pPlayer->GetID())
//     -- the DB query that loads the player's move-point
//     (saved-location) info.
//   - The handler sends NO ACK/NACK to the client; the data arrives
//     later via MP_ITEM broadcasts from the DB callback.
//
// Pattern mirrors shop_item_info_side_effect_runtime.hpp (D4.64) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_mpinfo_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemMpInfo side-effect chain.
class ShopItemMpInfoSideEffectSink {
public:
    virtual ~ShopItemMpInfoSideEffectSink() = default;

    // Legacy: SavedMovePointInfo(pPlayer->GetID()) -- fires the DB
    // query that loads the player's move-point info.
    virtual void fire_saved_move_point_db_query(
        std::uint32_t object_id) = 0;
};

struct ShopItemMpInfoRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t db_queries      = 0;
    bool trigger_db_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ShopItemMpInfoRuntimeOutcome apply_shop_item_mpinfo_side_effects(
    const ShopItemMpInfoSideEffectPlan& plan,
    ShopItemMpInfoSideEffectSink& sink) {
    ShopItemMpInfoRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemMpInfoSideEffectKind::FireSavedMovePointDbQuery:
            sink.fire_saved_move_point_db_query(effect.object_id);
            ++out.db_queries;
            ++out.effects_applied;
            break;
        }
    }
    out.trigger_db_flag_consumed = plan.trigger_db;
    return out;
}

}  // namespace mxh::server
