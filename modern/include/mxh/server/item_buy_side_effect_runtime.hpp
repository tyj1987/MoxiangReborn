// item_buy_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_buy_side_effect_plan(). The data plane returns either an empty
// plan + silent_success flag (BuyItem rt == 0) or a single NACK entry
// whose ecode encodes the failing gate; this header walks the plan
// and dispatches the entry to a virtual ItemBuySideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_BUY_SYN from
// [Server]Map/ItemManager.cpp:4162-4204):
//   - CheckHackNpc fails: legacy sends MP_ITEM_BUY_NACK with
//     ECode = NOT_EXIST (= 103) and does not run the remaining gates.
//   - CheckDemandItem fails: legacy sends MP_ITEM_BUY_NACK with
//     ECode = NO_DEMANDITEM (= 108).
//   - BuyItem returns 0: legacy has an empty body; the BuyItem ->
//     ObtainItemEx path emits its own ITEMOBTAINARRAYINFO ACK. The
//     runtime reports the silent success via the sink.
//   - BuyItem returns non-zero: legacy sends MP_ITEM_BUY_NACK with
//     ECode = rt.
//   - Gate precedence: NpcGate > Demand > BuyItem rt (the data plane
//     encodes this; the runtime dispatches whatever ecode the plan
//     carries).
//
// Pattern mirrors item_divide_side_effect_runtime.hpp (D4.49) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_buy_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemBuy side-effect chain.
class ItemBuySideEffectSink {
public:
    virtual ~ItemBuySideEffectSink() = default;

    // Legacy: SendErrorMsg(MP_ITEM_BUY_NACK, ECode=ecode, aux=rt) --
    // all three failure gates share the same wire protocol; the ecode
    // discriminates the gate (NOT_EXIST=103 / NO_DEMANDITEM=108 /
    // BuyItem rt).
    virtual void broadcast_buy_nack(std::uint16_t buy_item_idx,
                                    std::uint16_t buy_item_num,
                                    std::uint16_t dealer_idx,
                                    int original_rt,
                                    int ecode) = 0;

    // Legacy: BuyItem rt == 0 -> empty body; ObtainItemEx emits its
    // own ACK. The runtime reports the silent success so callers skip
    // the broadcast step without double-acking.
    virtual void silent_success() = 0;
};

struct ItemBuyRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_sent      = 0;
    std::size_t silent_successes = 0;
    bool nack_flag_consumed    = false;
    bool silent_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry, then
// reports the silent-success flag when the data plane emitted an
// empty plan (1:1 with the empty-body success branch).
inline ItemBuyRuntimeOutcome apply_item_buy_side_effects(
    const ItemBuySideEffectPlan& plan,
    ItemBuySideEffectSink& sink) {
    ItemBuyRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemBuySideEffectKind::BroadcastNackNpcGate:
        case ItemBuySideEffectKind::BroadcastNackDemand:
        case ItemBuySideEffectKind::BroadcastNackBuyFail:
            sink.broadcast_buy_nack(
                effect.buy_item_idx, effect.buy_item_num,
                effect.dealer_idx, effect.original_rt, effect.ecode);
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
