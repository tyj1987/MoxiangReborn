// save_point_mutate_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plans emitted by
// save_point_update_side_effect_plan() / save_point_del_side_effect_
// plan(). The data plane returns a 2-step success chain (DB mutate ->
// ACK echo) or a single NACK echo; this header walks both plan shapes
// and dispatches each entry to a virtual
// SavePointMutateSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager SAVEPOINT update/delete
// handlers from [Server]Map/ItemManager.cpp:5148-5188):
//   - Update success: SavedMovePointUpdate DB call then echo pmsg with
//     Protocol = UPDATE_ACK(154); failure: echo with UPDATE_NACK(155).
//   - Del success: SavedMovePointDelete DB call then echo pmsg with
//     Protocol = DEL_ACK(156); failure: echo with DEL_NACK(157).
//
// Pattern mirrors shop_item_name_change_side_effect_runtime.hpp
// (D4.76) and the rest of the runtime orchestrator family.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <mxh/server/save_point_mutate_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the SavePoint update/delete side-effect
// chains. The sink is bound to the originating player; the plans
// carry the DBIdx and rename payload only.
class SavePointMutateSideEffectSink {
public:
    virtual ~SavePointMutateSideEffectSink() = default;

    // Legacy: SavedMovePointUpdate(db_idx, new_name) DB call.
    virtual void rename_saved_move_point(
        std::uint32_t db_idx,
        const std::array<char, LEGACY_MAX_SAVED_MOVE_NAME>& new_name) = 0;

    // Legacy: SendMsg(pmsg) with Protocol = UPDATE_ACK(154).
    virtual void broadcast_update_ack(
        std::uint32_t db_idx,
        const std::array<char, LEGACY_MAX_SAVED_MOVE_NAME>& new_name) = 0;

    // Legacy: SendMsg(pmsg) with Protocol = UPDATE_NACK(155).
    virtual void broadcast_update_nack(
        std::uint32_t db_idx,
        const std::array<char, LEGACY_MAX_SAVED_MOVE_NAME>& new_name) = 0;

    // Legacy: SavedMovePointDelete(db_idx) DB call.
    virtual void delete_saved_move_point(std::uint32_t db_idx) = 0;

    // Legacy: SendMsg(pmsg) with Protocol = DEL_ACK(156).
    virtual void broadcast_del_ack(std::uint32_t db_idx) = 0;

    // Legacy: SendMsg(pmsg) with Protocol = DEL_NACK(157).
    virtual void broadcast_del_nack(std::uint32_t db_idx) = 0;
};

struct SavePointMutateRuntimeOutcome {
    std::size_t effects_applied  = 0;
    std::size_t renames          = 0;
    std::size_t update_acks_sent = 0;
    std::size_t update_nacks_sent = 0;
    std::size_t deletes          = 0;
    std::size_t del_acks_sent    = 0;
    std::size_t del_nacks_sent   = 0;
    bool update_ack_flag_consumed  = false;
    bool update_nack_flag_consumed = false;
    bool del_ack_flag_consumed     = false;
    bool del_nack_flag_consumed    = false;
};

// Runtime: walks the update plan and dispatches each entry in legacy
// order.
inline SavePointMutateRuntimeOutcome apply_save_point_update_side_effects(
    const SavePointUpdateSideEffectPlan& plan,
    SavePointMutateSideEffectSink& sink) {
    SavePointMutateRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case SavePointUpdateSideEffectKind::RenameSavedMovePoint:
            sink.rename_saved_move_point(effect.db_idx, effect.new_name);
            ++out.renames;
            ++out.effects_applied;
            break;
        case SavePointUpdateSideEffectKind::BroadcastUpdateAck:
            sink.broadcast_update_ack(effect.db_idx, effect.new_name);
            ++out.update_acks_sent;
            ++out.effects_applied;
            break;
        case SavePointUpdateSideEffectKind::BroadcastUpdateNack:
            sink.broadcast_update_nack(effect.db_idx, effect.new_name);
            ++out.update_nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.update_ack_flag_consumed = plan.send_ack;
    out.update_nack_flag_consumed = plan.send_nack;
    return out;
}

// Runtime: walks the del plan and dispatches each entry in legacy
// order.
inline SavePointMutateRuntimeOutcome apply_save_point_del_side_effects(
    const SavePointDelSideEffectPlan& plan,
    SavePointMutateSideEffectSink& sink) {
    SavePointMutateRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case SavePointDelSideEffectKind::DeleteSavedMovePoint:
            sink.delete_saved_move_point(effect.db_idx);
            ++out.deletes;
            ++out.effects_applied;
            break;
        case SavePointDelSideEffectKind::BroadcastDelAck:
            sink.broadcast_del_ack(effect.db_idx);
            ++out.del_acks_sent;
            ++out.effects_applied;
            break;
        case SavePointDelSideEffectKind::BroadcastDelNack:
            sink.broadcast_del_nack(effect.db_idx);
            ++out.del_nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.del_ack_flag_consumed = plan.send_ack;
    out.del_nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
