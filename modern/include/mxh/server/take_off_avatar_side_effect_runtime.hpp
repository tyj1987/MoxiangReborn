// take_off_avatar_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// take_off_avatar_side_effect_plan(). The data plane returns an
// ordered list of 0-2 TakeOffAvatarSideEffect entries (BroadcastAvatarInfo
// + RecomputeAvatarOption, gated on the transition flags); this header
// walks the list and dispatches each entry to its respective subsystem
// via virtual callback interfaces.
//
// 1:1 invariants (1:1 with legacy CShopItemManager::TakeOffAvatarItem tail):
//   - BroadcastAvatarInfo: legacy SEND_AVATARITEM_INFO broadcast.
//     Unlike PutOn, this is unconditional on a successful take-off
//     (legacy: send_avatar_info is always set after a successful
//     take-off; the data plane emits it whenever transition.send_avatar_info
//     is true).
//   - RecomputeAvatarOption: legacy CalcAvatarOption(bCalcStats) recompute.
//   - Steps are applied in legacy order: broadcast FIRST, then recompute.
//
// Pattern mirrors put_on_avatar_side_effect_runtime.hpp (D4.36) -- the
// data plane is symmetric; only the broadcast gate differs.

#pragma once

#include <cstdint>

#include <mxh/game/avatar_item_option.hpp>
#include <mxh/server/avatar_calc.hpp>
#include <mxh/server/avatar_equip_transition.hpp>
#include <mxh/server/take_off_avatar_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the TakeOffAvatarItem side-effect chain.
// Same shape as PutOnAvatarSideEffectSink (D4.36) so production can
// share the same player-broadcast + avatar-recompute hooks between
// PutOn and TakeOff sites.
class TakeOffAvatarSideEffectSink {
public:
    virtual ~TakeOffAvatarSideEffectSink() = default;
    virtual void broadcast_avatar_info(std::uint16_t w_icon_idx,
                                       std::uint16_t item_pos) = 0;
    virtual mxh::game::AvatarItemOption recompute_avatar_option(
        bool calc_stats) = 0;
};

struct TakeOffAvatarRuntimeOutcome {
    std::size_t effects_applied     = 0;
    std::size_t broadcasts          = 0;
    std::size_t recomputes          = 0;
    mxh::game::AvatarItemOption last_avatar_option{};
    bool last_recompute_invoked = false;
};

// Runtime: walks the side-effect plan and dispatches each entry in
// legacy order (broadcast, then recompute). Returns the outcome.
inline TakeOffAvatarRuntimeOutcome apply_take_off_avatar_side_effects(
    const TakeOffAvatarSideEffectPlan& plan,
    TakeOffAvatarSideEffectSink& sink) {
    TakeOffAvatarRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case TakeOffAvatarSideEffectKind::BroadcastAvatarInfo:
            sink.broadcast_avatar_info(
                effect.w_icon_idx, effect.item_pos);
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        case TakeOffAvatarSideEffectKind::RecomputeAvatarOption:
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
