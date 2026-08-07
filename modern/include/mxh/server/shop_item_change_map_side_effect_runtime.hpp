// shop_item_change_map_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_change_map_side_effect_plan(). The data plane returns an
// empty plan (no player) or a single SilentConsume entry; this header
// walks the plan and dispatches the entry to a virtual
// ShopItemChangeMapSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_CHANGEMAP_SYN from
// [Server]Map/ItemManager.cpp:5087-5093):
//   - The handler is a no-op acknowledgement: it validates the player
//     exists and returns WITHOUT any network response (the actual
//     map-change logic runs on the map server via the separate
//     MP_MOVE/change-map request).
//   - FindUser returns null: handler returns immediately (empty
//     plan).
//   - Player found: handler consumes the SYN silently (SilentConsume,
//     no ACK/NACK).
//
// Pattern mirrors item_mix_release_side_effect_runtime.hpp (D4.57)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_change_map_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemChangeMap side-effect chain.
class ShopItemChangeMapSideEffectSink {
public:
    virtual ~ShopItemChangeMapSideEffectSink() = default;

    // Legacy: handler returns with no network I/O. The runtime reports
    // the silent consume so callers can trace the receipt (object_id
    // = pmsg->dwObjectID).
    virtual void silent_consume(std::uint32_t object_id) = 0;
};

struct ShopItemChangeMapRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t consumes        = 0;
    bool consume_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ShopItemChangeMapRuntimeOutcome
apply_shop_item_change_map_side_effects(
    const ShopItemChangeMapSideEffectPlan& plan,
    ShopItemChangeMapSideEffectSink& sink) {
    ShopItemChangeMapRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemChangeMapSideEffectKind::SilentConsume:
            sink.silent_consume(effect.object_id);
            ++out.consumes;
            ++out.effects_applied;
            break;
        }
    }
    out.consume_flag_consumed = plan.consume;
    return out;
}

}  // namespace mxh::server
