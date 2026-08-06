// 1:1 side-effect-dispatcher port of the legacy
// CShopItemManager::TakeOffAvatarItem tail-side-effects (after the avatar
// mutation) from [Server]Map/ShopItemManager.cpp:1925-2021.
//
// After TakeOffAvatarItem successfully mutates avatar[], the legacy code
// applies these side effects unconditionally:
//   1. SEND_AVATARITEM_INFO broadcast (always; not gated on ItemPos).
//   2. CalcAvatarOption(bCalcStats) recompute.
//
// The data plane below captures both effects in a structured payload so
// the orchestrator can route them to the right subsystems.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/avatar_equip_transition.hpp>

namespace mxh::server {

enum class TakeOffAvatarSideEffectKind : std::uint8_t {
    BroadcastAvatarInfo = 0,
    RecomputeAvatarOption = 1,
};

struct TakeOffAvatarSideEffect final {
    TakeOffAvatarSideEffectKind kind =
        TakeOffAvatarSideEffectKind::BroadcastAvatarInfo;
    std::uint16_t w_icon_idx = 0;
    std::uint16_t item_pos   = 0;
    bool calc_stats = true;
};

struct TakeOffAvatarSideEffectPlan final {
    std::vector<TakeOffAvatarSideEffect> effects;
};

// 1:1 with legacy TakeOffAvatarItem tail. Unlike PutOn, the broadcast
// is unconditional (legacy: send_avatar_info is always set after a
// successful take-off), so the plan always contains both effects.
inline TakeOffAvatarSideEffectPlan take_off_avatar_side_effect_plan(
    const AvatarEquipTransition& transition,
    std::uint16_t dw_item_index) {
    TakeOffAvatarSideEffectPlan plan;
    plan.effects.reserve(2u);

    if (transition.send_avatar_info) {
        TakeOffAvatarSideEffect broadcast{};
        broadcast.kind = TakeOffAvatarSideEffectKind::BroadcastAvatarInfo;
        broadcast.w_icon_idx = dw_item_index;
        broadcast.calc_stats = transition.calc_stats;
        plan.effects.push_back(broadcast);
    }

    if (transition.recalculate_avatar_option) {
        TakeOffAvatarSideEffect recompute{};
        recompute.kind = TakeOffAvatarSideEffectKind::RecomputeAvatarOption;
        recompute.w_icon_idx = dw_item_index;
        recompute.calc_stats = transition.calc_stats;
        plan.effects.push_back(recompute);
    }

    return plan;
}

}  // namespace mxh::server
