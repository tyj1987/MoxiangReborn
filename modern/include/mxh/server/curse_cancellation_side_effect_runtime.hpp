// curse_cancellation_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// curse_cancellation_side_effect_plan(). The data plane returns a
// single NACK (3 gate categories), a 6-step chain (no inventory
// space), or a 7-step chain (full cancel); this header walks the plan
// and dispatches each entry to a virtual
// CurseCancellationSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_SYN from
// [Server]Map/ItemManager.cpp:6245-6333):
//   - 3 gates in order with NACK codes 1/2/3: CHKRT ItemOf / unique
//     option list exists + dwCurseCancellation != 0 / discard shop ok.
//   - Success chain: DiscardShopItem -> LogItemMoney(use) -> USE_ACK
//     -> DiscardCursedItem -> DELETEITEM ACK -> LogItemMoney(discard)
//     -> ObtainItemEx (omitted when no inventory space).
//
// Pattern mirrors char_change_side_effect_runtime.hpp (D4.84) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/curse_cancellation_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the CurseCancellation side-effect chain.
class CurseCancellationSideEffectSink {
public:
    virtual ~CurseCancellationSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_NACK,
    // nack_code).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint32_t nack_code) = 0;

    // Legacy: SEND_SHOPITEM_BASEINFO {MP_ITEM, USE_ACK, ShopItemPos,
    // ShopItemIdx}.
    virtual void send_use_ack_to_player(
        std::uint32_t player_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;

    // Legacy: DiscardItem(shop item) -- consumes the cancellation
    // scroll.
    virtual void discard_shop_item(std::uint32_t player_id,
                                   std::uint16_t shop_item_idx,
                                   std::uint16_t shop_item_pos) = 0;

    // Legacy: LogItemMoney(eLog_ShopItemUse).
    virtual void log_item_money_use(std::uint32_t player_id) = 0;

    // Legacy: DiscardItem(cursed unique item).
    virtual void discard_cursed_item(std::uint32_t player_id) = 0;

    // Legacy: MSG_ITEM_DISCARD_ACK {MP_ITEMEXT,
    // CURSE_CANCELLATION_DELETEITEM, TargetPos, wItemIdx, ItemNum}.
    virtual void send_delete_item_ack(std::uint32_t player_id) = 0;

    // Legacy: LogItemMoney(eLog_ItemDiscard).
    virtual void log_item_money_discard(std::uint32_t player_id) = 0;

    // Legacy: ObtainItemEx(...) -- restores the uncursed unique item
    // and emits the CURSE_CANCELLATION_ACK.
    virtual void obtain_item_ex(std::uint32_t player_id,
                                std::uint32_t curse_cancellation_count) = 0;
};

struct CurseCancellationRuntimeOutcome {
    std::size_t effects_applied   = 0;
    std::size_t nacks_sent        = 0;
    std::size_t use_acks_sent     = 0;
    std::size_t shop_discards     = 0;
    std::size_t use_logs          = 0;
    std::size_t cursed_discards   = 0;
    std::size_t delete_acks_sent  = 0;
    std::size_t discard_logs      = 0;
    std::size_t obtains           = 0;
    bool nack_flag_consumed     = false;
    bool use_ack_flag_consumed  = false;
    bool shop_flag_consumed     = false;
    bool use_log_flag_consumed  = false;
    bool cursed_flag_consumed   = false;
    bool delete_flag_consumed   = false;
    bool discard_log_flag_consumed = false;
    bool obtain_flag_consumed   = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline CurseCancellationRuntimeOutcome apply_curse_cancellation_side_effects(
    const CurseCancellationSideEffectPlan& plan,
    CurseCancellationSideEffectSink& sink) {
    CurseCancellationRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case CurseCancellationSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id, effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::SendUseAckToPlayer:
            sink.send_use_ack_to_player(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.use_acks_sent;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::DiscardShopItem:
            sink.discard_shop_item(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.shop_discards;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::LogItemMoneyUse:
            sink.log_item_money_use(effect.player_id);
            ++out.use_logs;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::DiscardCursedItem:
            sink.discard_cursed_item(effect.player_id);
            ++out.cursed_discards;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::SendDeleteItemAck:
            sink.send_delete_item_ack(effect.player_id);
            ++out.delete_acks_sent;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::LogItemMoneyDiscard:
            sink.log_item_money_discard(effect.player_id);
            ++out.discard_logs;
            ++out.effects_applied;
            break;
        case CurseCancellationSideEffectKind::ObtainItemEx:
            sink.obtain_item_ex(effect.player_id,
                                effect.curse_cancellation_count);
            ++out.obtains;
            ++out.effects_applied;
            break;
        }
    }
    out.nack_flag_consumed = plan.send_nack;
    out.use_ack_flag_consumed = plan.send_use_ack;
    out.shop_flag_consumed = plan.discard_shop;
    out.use_log_flag_consumed = plan.log_use;
    out.cursed_flag_consumed = plan.discard_cursed;
    out.delete_flag_consumed = plan.send_delete_ack;
    out.discard_log_flag_consumed = plan.log_discard;
    out.obtain_flag_consumed = plan.obtain_ex;
    return out;
}

}  // namespace mxh::server
