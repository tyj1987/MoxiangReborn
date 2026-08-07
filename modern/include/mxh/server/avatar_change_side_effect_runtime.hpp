// avatar_change_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// avatar_change_side_effect_plan(). The data plane returns an empty
// plan (no player) or a 2-step chain (RecalcShopItemOption ->
// BroadcastAvatarPuton); this header walks the plan and dispatches
// each entry to a virtual AvatarChangeSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_AVATAR_CHANGE from
// [Server]Map/ItemManager.cpp:5394-5409):
//   - FindUser returns null: handler returns (empty plan).
//   - Player found: (1) CalcShopItemOption(pmsg->wData2, TRUE)
//     recomputes the shop item option stats, then (2) broadcasts
//     MSG_DWORD2 {MP_ITEM, MP_ITEM_SHOPITEM_AVATAR_PUTON (85),
//     pPlayer->GetID(), pmsg->wData2} to all other map clients
//     (except self).
//   - The handler sends NO ACK/NACK to the originating client; the
//     broadcast is the response.
//
// Pattern mirrors shop_item_info_side_effect_runtime.hpp (D4.64) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/avatar_change_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AvatarChange side-effect chain.
class AvatarChangeSideEffectSink {
public:
    virtual ~AvatarChangeSideEffectSink() = default;

    // Legacy: CalcShopItemOption(item_pos, TRUE) -- recomputes the
    // shop item option stats based on the new avatar.
    virtual void recalc_shop_item_option(std::uint32_t object_id,
                                         std::uint16_t item_pos) = 0;

    // Legacy: QuickSendExceptObjectSelf(MSG_DWORD2,
    // MP_ITEM_SHOPITEM_AVATAR_PUTON, object_id, item_pos) -- broadcasts
    // the avatar put-on event to all other map clients.
    virtual void broadcast_avatar_puton(std::uint32_t object_id,
                                        std::uint16_t item_pos) = 0;
};

struct AvatarChangeRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t recalcs         = 0;
    std::size_t broadcasts      = 0;
    bool recalc_flag_consumed   = false;
    bool broadcast_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AvatarChangeRuntimeOutcome apply_avatar_change_side_effects(
    const AvatarChangeSideEffectPlan& plan,
    AvatarChangeSideEffectSink& sink) {
    AvatarChangeRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AvatarChangeSideEffectKind::RecalcShopItemOption:
            sink.recalc_shop_item_option(effect.object_id,
                                         effect.item_pos);
            ++out.recalcs;
            ++out.effects_applied;
            break;
        case AvatarChangeSideEffectKind::BroadcastAvatarPuton:
            sink.broadcast_avatar_puton(effect.object_id,
                                        effect.item_pos);
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        }
    }
    out.recalc_flag_consumed = plan.recalc;
    out.broadcast_flag_consumed = plan.broadcast;
    return out;
}

}  // namespace mxh::server
