// 1:1 side-effect-dispatcher port of the legacy
// CShopItemManager::PutOnAvatarItem tail-side-effects (after the avatar
// mutation) from [Server]Map/ShopItemManager.cpp:1792-1922.
//
// After PutOnAvatarItem successfully mutates avatar[], the legacy code
// applies these side effects in order:
//   1. SEND_AVATARITEM_INFO broadcast (only when ItemPos != 0).
//   2. CalcAvatarOption(bCalcStats) recompute via the legacy helper.
//
// The data plane below captures both effects in a structured payload so
// the orchestrator can route them to the right subsystems.

#pragma once

#include <array>
#include <cstdint>

#include <mxh/game/avatar_item_option.hpp>
#include <mxh/game/shop_item_option.hpp>
#include <mxh/server/avatar_calc.hpp>
#include <mxh/server/avatar_equip_transition.hpp>

namespace mxh::server {

enum class PutOnAvatarSideEffectKind : std::uint8_t {
    BroadcastAvatarInfo = 0,
    RecomputeAvatarOption = 1,
};

struct PutOnAvatarSideEffect final {
    PutOnAvatarSideEffectKind kind = PutOnAvatarSideEffectKind::BroadcastAvatarInfo;
    std::uint16_t w_icon_idx = 0;
    std::uint16_t item_pos   = 0;
    bool calc_stats = true;
};

struct PutOnAvatarSideEffectPlan final {
    std::vector<PutOnAvatarSideEffect> effects;
};

// 1:1 with legacy PutOnAvatarItem tail. The transition already holds
// the new avatar[24] state + send_avatar_info + recalculate_avatar_option
// flags, so this helper maps those flags into the ordered side-effect
// list the orchestrator applies.
inline PutOnAvatarSideEffectPlan put_on_avatar_side_effect_plan(
    const AvatarEquipTransition& transition,
    std::uint16_t dw_item_index) {
    PutOnAvatarSideEffectPlan plan;
    plan.effects.reserve(2u);
    if (transition.send_avatar_info) {
        PutOnAvatarSideEffect broadcast{};
        broadcast.kind = PutOnAvatarSideEffectKind::BroadcastAvatarInfo;
        broadcast.w_icon_idx = dw_item_index;
        broadcast.item_pos = 0;  // filled in by the orchestrator from runtime
        broadcast.calc_stats = transition.calc_stats;
        plan.effects.push_back(broadcast);
    }
    if (transition.recalculate_avatar_option) {
        PutOnAvatarSideEffect recompute{};
        recompute.kind = PutOnAvatarSideEffectKind::RecomputeAvatarOption;
        recompute.w_icon_idx = dw_item_index;
        recompute.calc_stats = transition.calc_stats;
        plan.effects.push_back(recompute);
    }
    return plan;
}

}  // namespace mxh::server
