// 1:1 side-effect-dispatcher port of the legacy
// ItemManager::MP_ITEMEXT_SKINTITEM_SELECT handler in
// [Server]Map/ItemManager.cpp:6525-6575 (the skin select + remove
// branches).
//
// After PutSkinSelectItem / RemoveEquipSkin succeeds, the legacy code
// applies the same side effects:
//   1. InitSkinDelay / StartSkinDelay (legacy: set delay gate so the
//      same skin cannot be re-applied within the cooldown window).
//   2. CharacterSkinInfoUpdate (DB write of the new wSkinItem[]).
//   3. SEND_SKIN_INFO broadcast (Category = MP_ITEMEXT,
//      Protocol = MP_ITEMEXT_SKINITEM_SELECT_ACK).
//
// The data plane below captures those effects in a structured payload
// so the orchestrator can route them to the runtime player + DBThread
// + PACKEDDATA_OBJ subsystems without re-reading the legacy body.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <mxh/server/skin_select_transition.hpp>

namespace mxh::server {

enum class SkinSelectSideEffectKind : std::uint8_t {
    StartSkinDelay = 0,        // legacy InitSkinDelay + StartSkinDelay
    CharacterSkinInfoUpdate = 1, // legacy DB write
    BroadcastSkinInfo = 2,       // legacy SEND_SKIN_INFO broadcast
};

struct SkinSelectSideEffect final {
    SkinSelectSideEffectKind kind = SkinSelectSideEffectKind::StartSkinDelay;
};

struct SkinSelectSideEffectPlan final {
    std::vector<SkinSelectSideEffect> effects;
    bool send_broadcast = false;
};

// 1:1 with legacy ItemManager::MP_ITEMEXT_SKINTITEM_SELECT success path.
// Both PutSkinSelectItem (Success) and RemoveEquipSkin (legacy: no error
// return) flow into the same 3-step chain.
inline SkinSelectSideEffectPlan skin_select_success_side_effect_plan() {
    SkinSelectSideEffectPlan plan;
    plan.effects.reserve(3u);
    plan.send_broadcast = true;

    SkinSelectSideEffect delay{};
    delay.kind = SkinSelectSideEffectKind::StartSkinDelay;
    plan.effects.push_back(delay);

    SkinSelectSideEffect db{};
    db.kind = SkinSelectSideEffectKind::CharacterSkinInfoUpdate;
    plan.effects.push_back(db);

    SkinSelectSideEffect broadcast{};
    broadcast.kind = SkinSelectSideEffectKind::BroadcastSkinInfo;
    plan.effects.push_back(broadcast);

    return plan;
}

}  // namespace mxh::server
