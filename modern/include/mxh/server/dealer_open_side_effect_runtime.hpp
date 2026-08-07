// dealer_open_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// dealer_open_side_effect_plan(). The data plane returns an empty
// plan (no player / hack NPC) or a single BroadcastDealerAck entry;
// this header walks the plan and dispatches the entry to a virtual
// DealerOpenSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_DEALER_SYN
// from [Server]Map/ItemManager.cpp:4851-4871):
//   - FindUser returns null: handler returns (silent drop).
//   - CheckHackNpc returns false: handler returns (silent drop, NO
//     NACK).
//   - Both pass: handler builds MSG_WORD {Category=MP_ITEM,
//     Protocol=MP_ITEM_DEALER_ACK (75), wData=pmsg->wData} and sends
//     it (opens the street-stall dealer UI).
//
// Pattern mirrors shop_item_change_map_side_effect_runtime.hpp
// (D4.59) and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/dealer_open_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the DealerOpen side-effect chain.
class DealerOpenSideEffectSink {
public:
    virtual ~DealerOpenSideEffectSink() = default;

    // Legacy: SendMsg(MSG_WORD, MP_ITEM_DEALER_ACK, wData=npc_pos) --
    // opens the street-stall dealer UI for the player.
    virtual void broadcast_dealer_ack(std::uint16_t npc_pos) = 0;
};

struct DealerOpenRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    bool ack_flag_consumed      = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline DealerOpenRuntimeOutcome apply_dealer_open_side_effects(
    const DealerOpenSideEffectPlan& plan,
    DealerOpenSideEffectSink& sink) {
    DealerOpenRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case DealerOpenSideEffectKind::BroadcastDealerAck:
            sink.broadcast_dealer_ack(effect.npc_pos);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    return out;
}

}  // namespace mxh::server
