// reinforce_with_shop_item_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// reinforce_with_shop_item_side_effect_plan(). The data plane returns
// an empty silent plan (rt == EI_TRUE), a single failed-ACK (rt ==
// 99), or a single NACK (any other rt); this header walks the plan
// and dispatches the entry to a virtual
// ReinforceWithShopItemSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_REINFORCE_WITHSHOPITEM_SYN from
// [Server]Map/ItemManager.cpp:4814-4850):
//   - rt == 0: NO message sent (silent success).
//   - rt == 99: rewrite Protocol to MP_ITEM_REINFORCE_FAILED_ACK and
//     SendAckMsg.
//   - else: MSG_ITEM_ERROR {REINFORCE_WITHSHOPITEM_NACK, dwObjectID,
//     rt} SendErrorMsg.
//
// Pattern mirrors item_reinforce_side_effect_runtime.hpp (D4.75) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/reinforce_with_shop_item_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ReinforceWithShopItem side-effect chain.
class ReinforceWithShopItemSideEffectSink {
public:
    virtual ~ReinforceWithShopItemSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_REINFORCE_FAILED_ACK) -- protocol
    // rewritten from the SYN in place.
    virtual void send_failed_ack_to_player(
        std::uint32_t player_id) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_REINFORCE_WITHSHOPITEM_NACK, rt).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     int nack_error_code) = 0;
};

struct ReinforceWithShopItemRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t failed_acks_sent = 0;
    std::size_t nacks_sent       = 0;
    bool failed_ack_flag_consumed = false;
    bool nack_flag_consumed       = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ReinforceWithShopItemRuntimeOutcome
apply_reinforce_with_shop_item_side_effects(
    const ReinforceWithShopItemSideEffectPlan& plan,
    ReinforceWithShopItemSideEffectSink& sink) {
    ReinforceWithShopItemRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ReinforceWithShopItemSideEffectKind::SendFailedAckToPlayer:
            sink.send_failed_ack_to_player(effect.player_id);
            ++out.failed_acks_sent;
            ++out.effects_applied;
            break;
        case ReinforceWithShopItemSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id,
                                     effect.nack_error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.failed_ack_flag_consumed = plan.send_failed_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
