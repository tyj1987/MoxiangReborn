// shop_item_name_change_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_name_change_side_effect_plan(). The data plane returns an
// empty plan (no player), a BroadcastNchangeNack entry (item not
// found), or a FireCharacterChangeNameDb entry; this header walks the
// plan and dispatches the single entry to a virtual
// ShopItemNameChangeSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_NCHANGE_SYN from
// [Server]Map/ItemManager.cpp:5546-5576):
//   - FindUser returns null: handler returns (empty plan).
//   - Linear search of the player's shop inventory for the item whose
//     dwDBIdx matches pmsg->DBIdx fails: handler sends
//     MP_ITEM_SHOPITEM_NCHANGE_NACK (97) with dwData = 6.
//   - Item found: handler fires CharacterChangeName (DB call).
//
// Pattern mirrors shop_item_mpinfo_side_effect_runtime.hpp (D4.65)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_name_change_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemNameChange side-effect chain.
class ShopItemNameChangeSideEffectSink {
public:
    virtual ~ShopItemNameChangeSideEffectSink() = default;

    // Legacy: CharacterChangeName(...) -- fires the DB call that
    // renames the character via the name-change shop item.
    virtual void fire_character_change_name_db(
        std::uint32_t object_id, std::uint32_t item_db_idx) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_NCHANGE_NACK, dwData=6) --
    // the name-change item was not found in the shop inventory.
    virtual void broadcast_nchange_nack(std::uint32_t object_id,
                                        std::uint32_t item_db_idx,
                                        std::uint32_t nack_code) = 0;
};

struct ShopItemNameChangeRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t db_fires        = 0;
    std::size_t nacks_sent      = 0;
    bool db_flag_consumed       = false;
    bool nack_flag_consumed     = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ShopItemNameChangeRuntimeOutcome
apply_shop_item_name_change_side_effects(
    const ShopItemNameChangeSideEffectPlan& plan,
    ShopItemNameChangeSideEffectSink& sink) {
    ShopItemNameChangeRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemNameChangeSideEffectKind::FireCharacterChangeNameDb:
            sink.fire_character_change_name_db(
                effect.object_id, effect.item_db_idx);
            ++out.db_fires;
            ++out.effects_applied;
            break;
        case ShopItemNameChangeSideEffectKind::BroadcastNchangeNack:
            sink.broadcast_nchange_nack(
                effect.object_id, effect.item_db_idx,
                effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.db_flag_consumed = plan.trigger_db;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
