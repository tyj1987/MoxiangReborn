// rare_create_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// rare_create_side_effect_plan(). The data plane returns a single
// NACK (11 gate categories) or a 5-step success chain (NO
// RARECREATE_ACK -- only USE_ACK); this header walks the plan and
// dispatches each entry to a virtual RareCreateSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_RARECREATE_SYN from
// [Server]Map/ItemManager.cpp:5924-6029):
//   - 11 gates in order with NACK codes 1..11: usable / both items
//     exist / both infos exist / rare-create icon {50,51,52,53} /
//     equip kind / no existing option / no suffixed icon / level in
//     range / IsRareItemAble / GetRare ok / discard ok.
//   - Success chain in legacy order: GenerateRareOption ->
//     DiscardShopItem -> ShopItemRareInsertToDB -> LogItemMoney ->
//     USE_ACK. No RARECREATE_ACK is sent on success.
//
// Pattern mirrors item_upgrade_side_effect_runtime.hpp (D4.74) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/rare_create_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the RareCreate side-effect chain.
class RareCreateSideEffectSink {
public:
    virtual ~RareCreateSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_RARECREATE_NACK, nack_code).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint32_t nack_code) = 0;

    // Legacy: SEND_SHOPITEM_BASEINFO {USE_ACK, ShopItemPos,
    // ShopItemIdx} -- the only success wire message.
    virtual void send_use_ack_to_player(
        std::uint32_t player_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;

    // Legacy: RAREITEMMGR->GetRare(target.wIconIdx, &rare, player,
    // TRUE) -- rolls the rare option.
    virtual void generate_rare_option(std::uint32_t player_id,
                                      std::uint32_t target_w_icon_idx) = 0;

    // Legacy: DiscardItem(...) -- consumes the rare-create item.
    virtual void discard_shop_item(std::uint32_t player_id,
                                   std::uint16_t shop_item_idx,
                                   std::uint16_t shop_item_pos) = 0;

    // Legacy: ShopItemRareInsertToDB(player, wIconIdx, Position,
    // dwDBIdx, &rareoption).
    virtual void shop_item_rare_insert_to_db(
        std::uint32_t player_id, std::uint32_t target_w_icon_idx,
        std::uint32_t target_position, std::uint32_t target_db_idx) = 0;

    // Legacy: LogItemMoney(eLog_ShopItemUse).
    virtual void log_item_money(std::uint32_t player_id) = 0;
};

struct RareCreateRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_sent      = 0;
    std::size_t use_acks_sent   = 0;
    std::size_t generated       = 0;
    std::size_t discards        = 0;
    std::size_t db_inserts      = 0;
    std::size_t money_logs      = 0;
    bool nack_flag_consumed     = false;
    bool use_ack_flag_consumed  = false;
    bool generate_flag_consumed = false;
    bool discard_flag_consumed  = false;
    bool db_flag_consumed       = false;
    bool log_flag_consumed      = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline RareCreateRuntimeOutcome apply_rare_create_side_effects(
    const RareCreateSideEffectPlan& plan,
    RareCreateSideEffectSink& sink) {
    RareCreateRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case RareCreateSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id, effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case RareCreateSideEffectKind::SendUseAckToPlayer:
            sink.send_use_ack_to_player(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.use_acks_sent;
            ++out.effects_applied;
            break;
        case RareCreateSideEffectKind::GenerateRareOption:
            sink.generate_rare_option(effect.player_id,
                                      effect.target_w_icon_idx);
            ++out.generated;
            ++out.effects_applied;
            break;
        case RareCreateSideEffectKind::DiscardShopItem:
            sink.discard_shop_item(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.discards;
            ++out.effects_applied;
            break;
        case RareCreateSideEffectKind::ShopItemRareInsertToDB:
            sink.shop_item_rare_insert_to_db(
                effect.player_id, effect.target_w_icon_idx,
                effect.target_position, effect.target_db_idx);
            ++out.db_inserts;
            ++out.effects_applied;
            break;
        case RareCreateSideEffectKind::LogItemMoney:
            sink.log_item_money(effect.player_id);
            ++out.money_logs;
            ++out.effects_applied;
            break;
        }
    }
    out.nack_flag_consumed = plan.send_nack;
    out.use_ack_flag_consumed = plan.send_use_ack;
    out.generate_flag_consumed = plan.generate_rare_option;
    out.discard_flag_consumed = plan.discard_shop_item;
    out.db_flag_consumed = plan.db_rare_insert;
    out.log_flag_consumed = plan.log_item_money;
    return out;
}

}  // namespace mxh::server
