// shop_item_shout_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_shout_side_effect_plan(). The data plane returns an
// empty plan (no player), a single NACK (not usable / discard fail),
// or a broadcast chain (discard + use-ACK for the once variant, then
// the agent-server shout forward); this header walks the plan and
// dispatches each entry to a virtual ShopItemShoutSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_SHOUT_SYN from
// [Server]Map/ItemManager.cpp:6030-6070):
//   - FindUser null -> return (empty plan).
//   - !IsUseAbleShopItem -> SHOUT_NACK(92) + return.
//   - Once variant: DiscardItem != EI_TRUE -> SHOUT_NACK + return;
//     else send MP_ITEM_SHOPITEM_USE_ACK(93).
//   - Always: forward SEND_SHOUTBASE {MP_ITEM, SHOUT_ACK(91),
//     characterIdx, msg} to the agent server for world broadcast.
//
// Pattern mirrors item_discard_side_effect_runtime.hpp (D4.46) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_shout_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemShout side-effect chain.
class ShopItemShoutSideEffectSink {
public:
    virtual ~ShopItemShoutSideEffectSink() = default;

    // Legacy: DiscardItem(...) -- consumes the once-use shout item.
    virtual void discard_shout_item(std::uint16_t item_idx,
                                    std::uint16_t item_pos,
                                    bool is_once_variant) = 0;

    // Legacy: SendAckMsg(MP_ITEM_SHOPITEM_USE_ACK).
    virtual void broadcast_use_ack(std::uint16_t item_idx,
                                   std::uint16_t item_pos) = 0;

    // Legacy: Send2AgentServer(SEND_SHOUTBASE {MP_ITEM, SHOUT_ACK,
    // characterIdx, msg}) -- the world-broadcast forward.
    virtual void forward_shout_ack(std::uint32_t character_idx,
                                   std::uint16_t item_idx,
                                   std::uint16_t item_pos) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_SHOUT_NACK).
    virtual void broadcast_shout_nack(std::uint16_t item_idx,
                                      std::uint16_t item_pos,
                                      bool is_once_variant) = 0;
};

struct ShopItemShoutRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t discards        = 0;
    std::size_t use_acks_sent   = 0;
    std::size_t forwards_sent   = 0;
    std::size_t nacks_sent      = 0;
    bool nack_flag_consumed   = false;
    bool forward_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ShopItemShoutRuntimeOutcome apply_shop_item_shout_side_effects(
    const ShopItemShoutSideEffectPlan& plan,
    ShopItemShoutSideEffectSink& sink) {
    ShopItemShoutRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemShoutSideEffectKind::DiscardShoutItem:
            sink.discard_shout_item(effect.item_idx, effect.item_pos,
                                    effect.is_once_variant);
            ++out.discards;
            ++out.effects_applied;
            break;
        case ShopItemShoutSideEffectKind::BroadcastUseAck:
            sink.broadcast_use_ack(effect.item_idx, effect.item_pos);
            ++out.use_acks_sent;
            ++out.effects_applied;
            break;
        case ShopItemShoutSideEffectKind::ForwardShoutAck:
            sink.forward_shout_ack(effect.character_idx,
                                   effect.item_idx, effect.item_pos);
            ++out.forwards_sent;
            ++out.effects_applied;
            break;
        case ShopItemShoutSideEffectKind::BroadcastShoutNack:
            sink.broadcast_shout_nack(effect.item_idx, effect.item_pos,
                                      effect.is_once_variant);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.nack_flag_consumed = plan.send_nack;
    out.forward_flag_consumed = plan.forward_shout;
    return out;
}

}  // namespace mxh::server
