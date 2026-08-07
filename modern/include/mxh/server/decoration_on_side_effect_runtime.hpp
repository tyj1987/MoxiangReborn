// decoration_on_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// decoration_on_side_effect_plan(). The data plane returns an empty
// plan (no player) or a single BroadcastToOthers entry; this header
// walks the plan and dispatches the entry to a virtual
// DecorationOnSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEMEXT_SHOPITEM_DECORATION_ON from
// [Server]Map/ItemManager.cpp:6578-6594):
//   - FindUser(dwObjectID) null -> return (empty plan).
//   - Player found -> MSG_DWORD2 {MP_ITEMEXT, DECORATION_ON,
//     dwObjectID, dwData1, dwData2} QuickSendExceptObjectSelf.
//
// Pattern mirrors avatar_change_side_effect_runtime.hpp (D4.70) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/decoration_on_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the DecorationOn side-effect chain.
class DecorationOnSideEffectSink {
public:
    virtual ~DecorationOnSideEffectSink() = default;

    // Legacy: QuickSendExceptObjectSelf(MSG_DWORD2 {DECORATION_ON,
    // dwObjectID, dwData1, dwData2}).
    virtual void broadcast_to_others(std::uint32_t player_id,
                                     std::uint32_t data1,
                                     std::uint32_t data2) = 0;
};

struct DecorationOnRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t broadcasts_sent = 0;
    bool broadcast_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline DecorationOnRuntimeOutcome apply_decoration_on_side_effects(
    const DecorationOnSideEffectPlan& plan,
    DecorationOnSideEffectSink& sink) {
    DecorationOnRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case DecorationOnSideEffectKind::BroadcastToOthers:
            sink.broadcast_to_others(effect.player_id, effect.data1,
                                     effect.data2);
            ++out.broadcasts_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.broadcast_flag_consumed = plan.broadcast;
    return out;
}

}  // namespace mxh::server
