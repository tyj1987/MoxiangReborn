// char_change_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// char_change_side_effect_plan(). The data plane returns a single
// NACK (6 gate categories) or a 7-step success chain; this header
// walks the plan and dispatches each entry to a virtual
// CharChangeSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_CHARCHANGE_SYN from
// [Server]Map/ItemManager.cpp:5577-5674):
//   - 6 gates in order with NACK codes 6/1/2/3/4/5: no avatar effect
//     active / item exists + char-or-shape icon / height+width in
//     [0.9,1.1] / gender in [0,2] / hair+face in [0,4] / discard ok.
//   - Success chain in data-plane order: DiscardCharChangeItem ->
//     SetCharChangeInfo -> USE_ACK -> BroadcastCharChange (shape
//     change overrides saved CTInfo) -> CharacterChangeInfoToDB ->
//     CHARCHANGE_ACK -> LogItemMoney.
//
// Pattern mirrors rare_create_side_effect_runtime.hpp (D4.82) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/char_change_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the CharChange side-effect chain.
class CharChangeSideEffectSink {
public:
    virtual ~CharChangeSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_CHARCHANGE_NACK, nack_code).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint32_t nack_code) = 0;

    // Legacy: SEND_SHOPITEM_BASEINFO {USE_ACK, ShopItemPos,
    // ShopItemIdx}.
    virtual void send_use_ack_to_player(
        std::uint32_t player_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;

    // Legacy: DiscardItem(...) -- consumes the char-change item.
    virtual void discard_char_change_item(
        std::uint32_t player_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;

    // Legacy: pPlayer->SetCharChangeInfo(&pmsg->Info).
    virtual void set_char_change_info(
        std::uint32_t player_id,
        const CharChangeValidationFields& info) = 0;

    // Legacy: SEND_CHARACTERCHANGE_INFO {CHARCHANGE, player id, Info}
    // QuickSend broadcast; shape-change overrides Gender/Height/Width
    // with the saved CTInfo values.
    virtual void broadcast_char_change(
        std::uint32_t player_id, CharChangeIcon icon_kind,
        const CharChangeValidationFields& info,
        std::uint8_t saved_gender, float saved_height,
        float saved_width) = 0;

    // Legacy: CharacterChangeInfoToDB (varies by item kind).
    virtual void character_change_info_to_db(
        std::uint32_t player_id, CharChangeIcon icon_kind,
        const CharChangeValidationFields& info,
        std::uint8_t saved_gender, float saved_height,
        float saved_width) = 0;

    // Legacy: MSGBASE CHARCHANGE_ACK.
    virtual void send_char_change_ack(std::uint32_t player_id) = 0;

    // Legacy: LogItemMoney(eLog_ShopItemUse).
    virtual void log_item_money(std::uint32_t player_id,
                                std::uint16_t shop_item_idx,
                                std::uint16_t shop_item_pos) = 0;
};

struct CharChangeRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_sent      = 0;
    std::size_t use_acks_sent   = 0;
    std::size_t discards        = 0;
    std::size_t info_sets       = 0;
    std::size_t broadcasts      = 0;
    std::size_t db_calls        = 0;
    std::size_t char_acks_sent  = 0;
    std::size_t money_logs      = 0;
    bool nack_flag_consumed     = false;
    bool use_ack_flag_consumed  = false;
    bool discard_flag_consumed  = false;
    bool set_info_flag_consumed = false;
    bool broadcast_flag_consumed = false;
    bool db_flag_consumed       = false;
    bool char_ack_flag_consumed = false;
    bool log_flag_consumed      = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline CharChangeRuntimeOutcome apply_char_change_side_effects(
    const CharChangeSideEffectPlan& plan,
    CharChangeSideEffectSink& sink) {
    CharChangeRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case CharChangeSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id, effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::SendUseAckToPlayer:
            sink.send_use_ack_to_player(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.use_acks_sent;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::DiscardCharChangeItem:
            sink.discard_char_change_item(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.discards;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::SetCharChangeInfo:
            sink.set_char_change_info(effect.player_id, effect.info);
            ++out.info_sets;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::BroadcastCharChange:
            sink.broadcast_char_change(
                effect.player_id, effect.icon_kind, effect.info,
                effect.saved_gender, effect.saved_height,
                effect.saved_width);
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::CharacterChangeInfoToDB:
            sink.character_change_info_to_db(
                effect.player_id, effect.icon_kind, effect.info,
                effect.saved_gender, effect.saved_height,
                effect.saved_width);
            ++out.db_calls;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::SendCharChangeAck:
            sink.send_char_change_ack(effect.player_id);
            ++out.char_acks_sent;
            ++out.effects_applied;
            break;
        case CharChangeSideEffectKind::LogItemMoney:
            sink.log_item_money(effect.player_id, effect.shop_item_idx,
                                effect.shop_item_pos);
            ++out.money_logs;
            ++out.effects_applied;
            break;
        }
    }
    out.nack_flag_consumed = plan.send_nack;
    out.use_ack_flag_consumed = plan.send_use_ack;
    out.discard_flag_consumed = plan.discard_item;
    out.set_info_flag_consumed = plan.set_char_change_info;
    out.broadcast_flag_consumed = plan.broadcast;
    out.db_flag_consumed = plan.db_call;
    out.char_ack_flag_consumed = plan.send_char_change_ack;
    out.log_flag_consumed = plan.log_item_money;
    return out;
}

}  // namespace mxh::server
