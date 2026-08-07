// put_on_avatar_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// put_on_avatar_side_effect_plan(). The data plane returns an
// ordered list of 0-2 PutOnAvatarSideEffect entries (BroadcastAvatarInfo
// + RecomputeAvatarOption, gated on the transition flags); this header
// walks the list and dispatches each entry to its respective subsystem
// via virtual callback interfaces.
//
// 1:1 invariants (1:1 with legacy CShopItemManager::PutOnAvatarItem tail):
//   - BroadcastAvatarInfo: legacy SEND_AVATARITEM_INFO broadcast.
//     The data plane only emits this when transition.send_avatar_info
//     is true (legacy: gated on ItemPos != 0).
//   - RecomputeAvatarOption: legacy CalcAvatarOption(bCalcStats) recompute.
//     The data plane emits this when transition.recalculate_avatar_option
//     is true.
//   - Steps are applied in the legacy order: broadcast FIRST, then
//     recompute (legacy: send then stat recalc).
//
// Pattern mirrors check_end_time_side_effect_runtime.hpp (D4.35) and
// the agent_*_side_effect_plan_runtime dispatchers.

#pragma once

#include <cstdint>

#include <mxh/game/avatar_item_option.hpp>
#include <mxh/server/avatar_calc.hpp>
#include <mxh/server/avatar_equip_transition.hpp>
#include <mxh/server/put_on_avatar_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the PutOnAvatarItem side-effect chain.
// Production wires each method to the live subsystem (Player broadcast,
// avatar stat recompute). Tests wire them to recording stubs.
class PutOnAvatarSideEffectSink {
public:
    virtual ~PutOnAvatarSideEffectSink() = default;

    // Legacy: SEND_AVATARITEM_INFO broadcast to the player + observers.
    virtual void broadcast_avatar_info(std::uint16_t w_icon_idx,
                                       std::uint16_t item_pos) = 0;

    // Legacy: CalcAvatarOption(bCalcStats) -- walks avatar[] and folds
    // per-slot ITEM_INFO deltas into the AVATARITEMOPTION aggregate.
    // Returns the recomputed option so tests can assert against it.
    virtual mxh::game::AvatarItemOption recompute_avatar_option(
        bool calc_stats) = 0;
};

// Outcome counters returned by the runtime.
struct PutOnAvatarRuntimeOutcome {
    std::size_t effects_applied     = 0;
    std::size_t broadcasts          = 0;
    std::size_t recomputes          = 0;
    mxh::game::AvatarItemOption last_avatar_option{};
    bool last_recompute_invoked = false;
};

// Runtime: walks the side-effect plan and dispatches each entry in
// legacy order (broadcast, then recompute). Returns the outcome.
inline PutOnAvatarRuntimeOutcome apply_put_on_avatar_side_effects(
    const PutOnAvatarSideEffectPlan& plan,
    PutOnAvatarSideEffectSink& sink) {
    PutOnAvatarRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case PutOnAvatarSideEffectKind::BroadcastAvatarInfo:
            sink.broadcast_avatar_info(
                effect.w_icon_idx, effect.item_pos);
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        case PutOnAvatarSideEffectKind::RecomputeAvatarOption:
            out.last_avatar_option =
                sink.recompute_avatar_option(effect.calc_stats);
            out.last_recompute_invoked = true;
            ++out.recomputes;
            ++out.effects_applied;
            break;
        }
    }
    return out;
}

}  // namespace mxh::server
