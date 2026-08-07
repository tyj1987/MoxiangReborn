// shop_item_seal_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_seal_side_effect_plan(). The data plane returns an empty
// plan (no player), a single BroadcastSealNack entry (one of 8 gate
// failures), or the 8-step success chain; this header walks the plan
// and dispatches each entry to a virtual ShopItemSealSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_SEAL_SYN from
// [Server]Map/ItemManager.cpp:5675-5775):
//   - 8 gates in order: FindUser / IsUseAbleShopItem(seal) /
//     IsUseAbleShopItem(target) / item pointers + ItemInfo /
//     seal kind == eIncantation_ItemSeal / target kind in
//     {MAKEUP, DECORATION, PET} / SellPrice == Forever /
//     not-already-sealed / DiscardItem(seal) success. Each failure
//     maps to its own NACK code (1, 2, 3, 3, 4, 5, 6, 7, 9).
//   - Success chain in legacy order: LogShopItemUse ->
//     SetItemParamSeal -> DeleteUsingShopItemInfo ->
//     ShopItemParamUpdateToDb -> ShopItemDeleteToDb ->
//     LogShopItemSeal -> BroadcastUseAck -> BroadcastSealAck.
//
// Pattern mirrors check_end_time_side_effect_runtime.hpp (multi-step
// chains) and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_seal_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemSeal side-effect chain.
class ShopItemSealSideEffectSink {
public:
    virtual ~ShopItemSealSideEffectSink() = default;

    // Legacy: LogItemMoney(eLog_ShopItemUse).
    virtual void log_shop_item_use() = 0;

    // Legacy: set ITEM_PARAM_SEAL on the target row.
    virtual void set_item_param_seal(std::uint32_t target_db_idx,
                                     std::uint32_t target_item_param) = 0;

    // Legacy: DeleteUsingShopItemInfo(...).
    virtual void delete_using_shop_item_info() = 0;

    // Legacy: ShopItemParamUpdateToDB(target, ITEM_PARAM_SEAL).
    virtual void shop_item_param_update_to_db(
        std::uint32_t target_db_idx, std::uint32_t target_item_param) = 0;

    // Legacy: ShopItemDeleteToDB(target).
    virtual void shop_item_delete_to_db(std::uint32_t target_db_idx) = 0;

    // Legacy: LogItemMoney(eLog_ShopItemSeal).
    virtual void log_shop_item_seal() = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_USE_ACK) with the seal item
    // payload.
    virtual void broadcast_use_ack(std::uint16_t seal_item_idx,
                                   std::uint16_t seal_item_pos) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_SEAL_ACK).
    virtual void broadcast_seal_ack() = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_SEAL_NACK) with the gate error
    // code.
    virtual void broadcast_seal_nack(std::uint16_t seal_item_idx,
                                     std::uint16_t seal_item_pos,
                                     std::uint32_t nack_code) = 0;
};

struct ShopItemSealRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t logs_use        = 0;
    std::size_t param_seals     = 0;
    std::size_t use_deletes     = 0;
    std::size_t db_param_updates = 0;
    std::size_t db_deletes      = 0;
    std::size_t logs_seal       = 0;
    std::size_t use_acks        = 0;
    std::size_t seal_acks       = 0;
    std::size_t seal_nacks      = 0;
    bool ack_flag_consumed      = false;
    bool nack_flag_consumed     = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ShopItemSealRuntimeOutcome apply_shop_item_seal_side_effects(
    const ShopItemSealSideEffectPlan& plan,
    ShopItemSealSideEffectSink& sink) {
    ShopItemSealRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemSealSideEffectKind::LogShopItemUse:
            sink.log_shop_item_use();
            ++out.logs_use;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::SetItemParamSeal:
            sink.set_item_param_seal(effect.target_db_idx,
                                     effect.target_item_param);
            ++out.param_seals;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::DeleteUsingShopItemInfo:
            sink.delete_using_shop_item_info();
            ++out.use_deletes;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::ShopItemParamUpdateToDb:
            sink.shop_item_param_update_to_db(
                effect.target_db_idx, effect.target_item_param);
            ++out.db_param_updates;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::ShopItemDeleteToDb:
            sink.shop_item_delete_to_db(effect.target_db_idx);
            ++out.db_deletes;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::LogShopItemSeal:
            sink.log_shop_item_seal();
            ++out.logs_seal;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::BroadcastUseAck:
            sink.broadcast_use_ack(effect.seal_item_idx,
                                   effect.seal_item_pos);
            ++out.use_acks;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::BroadcastSealAck:
            sink.broadcast_seal_ack();
            ++out.seal_acks;
            ++out.effects_applied;
            break;
        case ShopItemSealSideEffectKind::BroadcastSealNack:
            sink.broadcast_seal_nack(effect.seal_item_idx,
                                     effect.seal_item_pos,
                                     effect.nack_code);
            ++out.seal_nacks;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
