// item_divide_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_divide_side_effect_plan(). The data plane returns either an
// empty plan + silent_success flag (DivideItem rt == 0) or a single
// BroadcastErrorNack entry (rt != 0); this header walks the plan and
// dispatches the entry to a virtual ItemDivideSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_DIVIDE_SYN
// from [Server]Map/ItemManager.cpp:4092-4114):
//   - DivideItem returns 0: legacy has an empty body; the DivideItem
//     -> ObtainItemEx path emits its own ITEMOBTAINARRAYINFO ACK
//     through Alloc's DBResult chain. The runtime reports the silent
//     success via the sink so callers skip the broadcast step.
//   - DivideItem returns non-zero: legacy sends MSG_ITEM_ERROR with
//     Protocol = MP_ITEM_ERROR_NACK, ECode = eItemUseErr_Divide (= 4),
//     and the original rt as the SendErrorMsg auxiliary code. The
//     runtime's BroadcastErrorNack carries ecode = 4 + original_rt.
//
// Pattern mirrors item_move_side_effect_runtime.hpp (D4.43) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_divide_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemDivide side-effect chain.
class ItemDivideSideEffectSink {
public:
    virtual ~ItemDivideSideEffectSink() = default;

    // Legacy: SendErrorMsg(MP_ITEM_ERROR_NACK, ECode=eItemUseErr_Divide,
    // aux=rt) -- sends the fixed divide error code plus the original rt
    // as the auxiliary code.
    virtual void broadcast_error_nack(std::uint16_t from_pos,
                                      std::uint16_t to_pos,
                                      std::uint16_t item_idx,
                                      std::uint16_t from_dur,
                                      std::uint16_t to_dur,
                                      int original_rt,
                                      int ecode) = 0;

    // Legacy: rt == 0 -> empty body; the ObtainItemEx path emits its
    // own ACK. The runtime reports the silent success so callers skip
    // the broadcast step without double-acking.
    virtual void silent_success() = 0;
};

struct ItemDivideRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_sent      = 0;
    std::size_t silent_successes = 0;
    bool nack_flag_consumed    = false;
    bool silent_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry, then
// reports the silent-success flag when the data plane emitted an
// empty plan (1:1 with the empty-body success branch).
inline ItemDivideRuntimeOutcome apply_item_divide_side_effects(
    const ItemDivideSideEffectPlan& plan,
    ItemDivideSideEffectSink& sink) {
    ItemDivideRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemDivideSideEffectKind::BroadcastErrorNack:
            sink.broadcast_error_nack(
                effect.from_pos, effect.to_pos, effect.item_idx,
                effect.from_dur, effect.to_dur,
                effect.original_rt, LEGACY_EITEMUSE_DIVIDE);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    if (plan.silent_success && out.silent_successes == 0u) {
        sink.silent_success();
        ++out.silent_successes;
    }
    out.nack_flag_consumed = plan.send_nack;
    out.silent_flag_consumed = plan.silent_success;
    return out;
}

}  // namespace mxh::server
