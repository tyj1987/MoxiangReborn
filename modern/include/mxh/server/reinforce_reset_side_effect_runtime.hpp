// reinforce_reset_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// reinforce_reset_side_effect_plan(). The data plane returns a single
// NACK (7 gate categories with legacy code gaps 7/8) or a 9-step
// success chain; this header walks the plan and dispatches each entry
// to a virtual ReinforceResetSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_REINFORCERESET_SYN from
// [Server]Map/ItemManager.cpp:5833-5923):
//   - 7 gates in order with NACK codes 1/2/3/4/5/6/9: usable /
//     both items exist / item info exists / reinforce-reset icon /
//     equip kind / target has option / discard ok.
//   - Success chain in data-plane order: DiscardShopItem ->
//     LogItemMoney(use) -> RemoveItemOption -> CharacterItemOption
//     Delete (DB) -> ItemUpdateToDB -> LogItemMoney(reset) ->
//     ClearTargetDurability -> USE_ACK -> REINFORCERESET_ACK.
//
// Pattern mirrors shop_item_seal_side_effect_runtime.hpp (D4.72) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/reinforce_reset_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ReinforceReset side-effect chain.
class ReinforceResetSideEffectSink {
public:
    virtual ~ReinforceResetSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_REINFORCERESET_ACK).
    virtual void send_ack_to_player(std::uint32_t player_id) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_REINFORCERESET_NACK,
    // nack_code).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint32_t nack_code) = 0;

    // Legacy: SEND_SHOPITEM_BASEINFO {USE_ACK, ShopItemPos,
    // ShopItemIdx}.
    virtual void send_use_ack_to_player(
        std::uint32_t player_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;

    // Legacy: DiscardItem(...) -- consumes the reinforce-reset
    // incantation.
    virtual void discard_shop_item(std::uint32_t player_id,
                                   std::uint16_t shop_item_idx,
                                   std::uint16_t shop_item_pos) = 0;

    // Legacy: pPlayer->RemoveItemOption(target.Durability).
    virtual void remove_item_option(std::uint32_t player_id,
                                    std::uint32_t target_db_idx,
                                    std::uint32_t target_durability) = 0;

    // Legacy: CharacterItemOptionDelete(target.Durability,
    // target.dwDBIdx) -- DB delete of the removed option.
    virtual void character_item_option_delete(
        std::uint32_t player_id, std::uint32_t target_db_idx,
        std::uint32_t target_durability) = 0;

    // Legacy: ItemUpdateToDB(player, target.dwDBIdx, ...) -- DB
    // update after the durability reset.
    virtual void item_update_to_db(std::uint32_t player_id,
                                   std::uint32_t target_db_idx) = 0;

    // Legacy: LogItemMoney(eLog_ShopItemUse).
    virtual void log_item_money_use(std::uint32_t player_id) = 0;

    // Legacy: LogItemMoney(eLog_ShopItem_ReinforceReset).
    virtual void log_item_money_reset(std::uint32_t player_id) = 0;

    // Legacy: target.Durability = 0.
    virtual void clear_target_durability(
        std::uint32_t player_id, std::uint32_t target_db_idx) = 0;
};

struct ReinforceResetRuntimeOutcome {
    std::size_t effects_applied   = 0;
    std::size_t acks_sent         = 0;
    std::size_t nacks_sent        = 0;
    std::size_t use_acks_sent     = 0;
    std::size_t discards          = 0;
    std::size_t option_removals   = 0;
    std::size_t db_option_deletes = 0;
    std::size_t db_item_updates   = 0;
    std::size_t use_logs          = 0;
    std::size_t reset_logs        = 0;
    std::size_t durability_clears = 0;
    bool ack_flag_consumed        = false;
    bool nack_flag_consumed       = false;
    bool use_ack_flag_consumed    = false;
    bool discard_flag_consumed    = false;
    bool remove_flag_consumed     = false;
    bool db_delete_flag_consumed  = false;
    bool db_update_flag_consumed  = false;
    bool use_log_flag_consumed    = false;
    bool reset_log_flag_consumed  = false;
    bool clear_flag_consumed      = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ReinforceResetRuntimeOutcome apply_reinforce_reset_side_effects(
    const ReinforceResetSideEffectPlan& plan,
    ReinforceResetSideEffectSink& sink) {
    ReinforceResetRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ReinforceResetSideEffectKind::SendAckToPlayer:
            sink.send_ack_to_player(effect.player_id);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id, effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::SendUseAckToPlayer:
            sink.send_use_ack_to_player(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.use_acks_sent;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::DiscardShopItem:
            sink.discard_shop_item(
                effect.player_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.discards;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::RemoveItemOption:
            sink.remove_item_option(
                effect.player_id, effect.target_db_idx,
                effect.target_durability);
            ++out.option_removals;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::CharacterItemOptionDelete:
            sink.character_item_option_delete(
                effect.player_id, effect.target_db_idx,
                effect.target_durability);
            ++out.db_option_deletes;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::ItemUpdateToDB:
            sink.item_update_to_db(effect.player_id,
                                   effect.target_db_idx);
            ++out.db_item_updates;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::LogItemMoneyUse:
            sink.log_item_money_use(effect.player_id);
            ++out.use_logs;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::LogItemMoneyReset:
            sink.log_item_money_reset(effect.player_id);
            ++out.reset_logs;
            ++out.effects_applied;
            break;
        case ReinforceResetSideEffectKind::ClearTargetDurability:
            sink.clear_target_durability(effect.player_id,
                                         effect.target_db_idx);
            ++out.durability_clears;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.use_ack_flag_consumed = plan.send_use_ack;
    out.discard_flag_consumed = plan.discard_shop_item;
    out.remove_flag_consumed = plan.remove_item_option;
    out.db_delete_flag_consumed = plan.db_item_option_delete;
    out.db_update_flag_consumed = plan.db_item_update;
    out.use_log_flag_consumed = plan.log_item_money_use;
    out.reset_log_flag_consumed = plan.log_item_money_reset;
    out.clear_flag_consumed = plan.clear_target_durability;
    return out;
}

}  // namespace mxh::server
