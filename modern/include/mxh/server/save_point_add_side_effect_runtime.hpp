// save_point_add_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plans emitted by
// save_point_add_success_side_effect_plan() /
// save_point_add_nack_side_effect_plan(). The data plane returns a
// 2-step success chain (USE_ACK broadcast -> SavedMovePointInsert DB)
// or a single USE_NACK error step; this header walks both plan
// shapes and dispatches each entry to a virtual
// SavePointAddSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_SAVEPOINT_ADD_SYN from
// [Server]Map/ItemManager.cpp:5104-5146):
//   - Success (UseShopItem rt == 0): SEND_SHOPITEM_BASEINFO
//     {MP_ITEM_SHOPITEM_USE_ACK, ShopItemBase + ShopItemPos/Idx} then
//     SavedMovePointInsert (PlayerID + Name + MapNum + Point) in
//     legacy order.
//   - Failure (Validsavenum gate or rt != 0): single MSG_ITEM_ERROR
//     {MP_ITEM_SHOPITEM_USE_NACK, ECode}.
//
// Pattern mirrors save_point_mutate_side_effect_runtime.hpp (D4.42)
// and the rest of the runtime orchestrator family.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <mxh/server/save_point_add_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the SavePointAdd side-effect chain.
class SavePointAddSideEffectSink {
public:
    virtual ~SavePointAddSideEffectSink() = default;

    // Legacy: SEND_SHOPITEM_BASEINFO broadcast {MP_ITEM,
    // MP_ITEM_SHOPITEM_USE_ACK, ShopItemBase, ShopItemPos, ShopItemIdx}.
    virtual void broadcast_use_ack(
        std::uint32_t player_id,
        const game::ShopItemBase& shop_item_base,
        std::uint16_t shop_item_pos, std::uint16_t shop_item_idx) = 0;

    // Legacy: SavedMovePointInsert(player_id, name, map_num, point).
    virtual void insert_saved_move_point(
        std::uint32_t player_id,
        const std::array<char, 21u>& move_name,
        std::uint16_t map_num, std::uint32_t point_value) = 0;

    // Legacy: MSG_ITEM_ERROR {MP_ITEM, MP_ITEM_SHOPITEM_USE_NACK,
    // ECode}.
    virtual void broadcast_use_nack(std::uint32_t player_id,
                                    std::uint8_t e_code) = 0;
};

struct SavePointAddRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t use_acks_sent   = 0;
    std::size_t db_inserts      = 0;
    std::size_t use_nacks_sent  = 0;
    bool ack_flag_consumed  = false;
    bool nack_flag_consumed = false;
};

// Runtime: walks the success plan and dispatches each entry in legacy
// order. player_id is threaded from the handler (the data plane
// carries the move-point payload but not the player identity).
inline SavePointAddRuntimeOutcome apply_save_point_add_success_side_effects(
    std::uint32_t player_id, const SavePointAddSideEffectPlan& plan,
    SavePointAddSideEffectSink& sink) {
    SavePointAddRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case SavePointAddSideEffectKind::BroadcastUseAck:
            sink.broadcast_use_ack(
                player_id, effect.shop_item_base,
                effect.shop_item_pos, effect.shop_item_idx);
            ++out.use_acks_sent;
            ++out.effects_applied;
            break;
        case SavePointAddSideEffectKind::InsertSavedMovePoint:
            sink.insert_saved_move_point(
                player_id, effect.move_name, effect.map_num,
                effect.point_value);
            ++out.db_inserts;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_use_ack;
    return out;
}

// Runtime: walks the failure plan and dispatches the single NACK step.
inline SavePointAddRuntimeOutcome apply_save_point_add_nack_side_effects(
    std::uint32_t player_id, const SavePointAddNackPlan& plan,
    SavePointAddSideEffectSink& sink) {
    SavePointAddRuntimeOutcome out;
    for (const auto& step : plan.steps) {
        switch (step.kind) {
        case SavePointAddNackKind::BroadcastUseNack:
            sink.broadcast_use_nack(player_id, step.e_code);
            ++out.use_nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.nack_flag_consumed = plan.send_use_nack;
    return out;
}

}  // namespace mxh::server
