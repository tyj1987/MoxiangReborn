// unique_item_mix_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// unique_item_mix_side_effect_plan(). The data plane returns an empty
// silent plan (gate failure or no space), or a success chain with
// per-material discard/ACK/log triplets, the basic-item discard trio,
// the result roll, and the obtain step; this header walks the plan
// and dispatches each entry to a virtual UniqueItemMixSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEMEXT_UNIQUEITEM_MIX_SYN from
// [Server]Map/ItemManager.cpp:6397-6521):
//   - 4 gates (basic exists / materials exist / mix info exists /
//     enough material) fail silently (legacy bare break, NO NACK).
//   - Success chain: for each material (DiscardItem -> DELETEITEM
//     ACK -> LogItemMoney), then (DiscardItem basic -> DELETEITEM ACK
//     -> LogItemMoney), then RollRandomResultItem -> ObtainItemEx
//     (omitted when no inventory space).
//
// Pattern mirrors item_discard_side_effect_runtime.hpp (D4.46) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/unique_item_mix_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the UniqueItemMix side-effect chain.
class UniqueItemMixSideEffectSink {
public:
    virtual ~UniqueItemMixSideEffectSink() = default;

    // Legacy: DiscardItem(material).
    virtual void discard_material_item(
        std::uint32_t player_id,
        const UniqueItemMixMaterial& material) = 0;

    // Legacy: MSG_ITEM_DISCARD_ACK {MP_ITEMEXT, DELETEITEM, TargetPos,
    // wItemIdx, ItemNum} per material.
    virtual void send_material_delete_ack(
        std::uint32_t player_id,
        const UniqueItemMixMaterial& material) = 0;

    // Legacy: LogItemMoney(eLog_ItemDiscard) per material.
    virtual void log_material_discard(
        std::uint32_t player_id,
        const UniqueItemMixMaterial& material) = 0;

    // Legacy: DiscardItem(basic item).
    virtual void discard_basic_item(
        std::uint32_t player_id, std::uint32_t basic_pos,
        std::uint16_t basic_w_idx, std::uint32_t basic_db_idx) = 0;

    // Legacy: MSG_ITEM_DISCARD_ACK {MP_ITEMEXT, DELETEITEM, basicPos,
    // basicIdx, 1}.
    virtual void send_basic_delete_ack(
        std::uint32_t player_id, std::uint32_t basic_pos,
        std::uint16_t basic_w_idx) = 0;

    // Legacy: LogItemMoney(eLog_ItemDiscard) for the basic item.
    virtual void log_basic_discard(
        std::uint32_t player_id, std::uint32_t basic_pos,
        std::uint16_t basic_w_idx, std::uint32_t basic_db_idx) = 0;

    // Legacy: roll 1..100 and pick the weighted result index.
    virtual void roll_random_result_item(
        std::uint32_t player_id, std::uint32_t result_w_idx) = 0;

    // Legacy: ObtainItemEx(...) -- creates the result item and emits
    // the UNIQUEITEM_MIX_ACK.
    virtual void obtain_result_item(
        std::uint32_t player_id, std::uint32_t result_w_idx,
        std::uint16_t obtain_num) = 0;
};

struct UniqueItemMixRuntimeOutcome {
    std::size_t effects_applied   = 0;
    std::size_t material_discards = 0;
    std::size_t material_acks     = 0;
    std::size_t material_logs     = 0;
    std::size_t basic_discards    = 0;
    std::size_t basic_acks        = 0;
    std::size_t basic_logs        = 0;
    std::size_t rolls             = 0;
    std::size_t obtains           = 0;
    bool materials_flag_consumed = false;
    bool basic_flag_consumed     = false;
    bool roll_flag_consumed      = false;
    bool obtain_flag_consumed    = false;
    bool log_flag_consumed       = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline UniqueItemMixRuntimeOutcome apply_unique_item_mix_side_effects(
    const UniqueItemMixSideEffectPlan& plan,
    UniqueItemMixSideEffectSink& sink) {
    UniqueItemMixRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case UniqueItemMixSideEffectKind::DiscardMaterialItem:
            sink.discard_material_item(effect.player_id, effect.material);
            ++out.material_discards;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::SendMaterialDeleteAck:
            sink.send_material_delete_ack(effect.player_id,
                                          effect.material);
            ++out.material_acks;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::LogMaterialDiscard:
            sink.log_material_discard(effect.player_id, effect.material);
            ++out.material_logs;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::DiscardBasicItem:
            sink.discard_basic_item(
                effect.player_id, effect.basic_pos,
                effect.basic_w_idx, effect.basic_db_idx);
            ++out.basic_discards;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::SendBasicDeleteAck:
            sink.send_basic_delete_ack(
                effect.player_id, effect.basic_pos,
                effect.basic_w_idx);
            ++out.basic_acks;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::LogBasicDiscard:
            sink.log_basic_discard(
                effect.player_id, effect.basic_pos,
                effect.basic_w_idx, effect.basic_db_idx);
            ++out.basic_logs;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::RollRandomResultItem:
            sink.roll_random_result_item(effect.player_id,
                                         effect.result_w_idx);
            ++out.rolls;
            ++out.effects_applied;
            break;
        case UniqueItemMixSideEffectKind::ObtainResultItem:
            sink.obtain_result_item(effect.player_id,
                                    effect.result_w_idx,
                                    effect.obtain_num);
            ++out.obtains;
            ++out.effects_applied;
            break;
        }
    }
    out.materials_flag_consumed = plan.discard_materials;
    out.basic_flag_consumed = plan.discard_basic;
    out.roll_flag_consumed = plan.roll_result;
    out.obtain_flag_consumed = plan.obtain_result;
    out.log_flag_consumed = plan.any_log;
    return out;
}

}  // namespace mxh::server
