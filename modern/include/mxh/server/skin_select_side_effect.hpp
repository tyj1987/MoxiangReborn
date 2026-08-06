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
// When PutSkinSelectItem returns Fail / DelayFail / LevelFail, the
// legacy code emits a single MSG_DWORD3 NACK to the originating player:
//   dwData1 = dwResult (the legacy eSkinResult_* code)
//   dwData2 = pPlayer->GetSkinDelayTime() (remaining cooldown)
//   dwData3 = pSkinInfo->dwLimitLevel (only meaningful for normalclothes)
//
// The data plane below captures both outcomes in structured payloads
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

// 1:1 with legacy ItemManager.cpp:6546-6560 NACK payload.
//
// The legacy code calls GAMERESRCMNGR->GetNomalClothesSkinList(dwData1)
// and returns early if the lookup fails; in that case the orchestrator
// must not emit the NACK. The data plane mirrors that gate by setting
// `send_nack = false` only when the result is Success. For any other
// result, the orchestrator is expected to have already resolved
// dw_limit_level (or zero if the lookup returned nullptr) and to have
// decided whether to suppress the NACK.
//
// Result mapping (1:1 with legacy eSkinResult_* codes):
//   Fail      -> dwData1 = 1
//   DelayFail -> dwData1 = 2
//   LevelFail -> dwData1 = 3
struct SkinSelectNackStep final {
    std::uint32_t result_code = 0;            // legacy dwData1 (= dwResult)
    std::uint32_t skin_delay_remaining = 0;   // legacy dwData2 (= GetSkinDelayTime())
    std::uint32_t dw_limit_level = 0;         // legacy dwData3 (= pSkinInfo->dwLimitLevel)
};

struct SkinSelectNackPlan final {
    std::vector<SkinSelectNackStep> steps;
    bool send_nack = false;
};

inline SkinSelectNackPlan skin_select_nack_side_effect_plan(
    SkinSelectResult result,
    std::uint32_t skin_delay_remaining,
    std::uint32_t dw_limit_level) {
    SkinSelectNackPlan plan;
    if (result == SkinSelectResult::Success) {
        // Success path takes the broadcast plan; the NACK plan is empty.
        return plan;
    }
    SkinSelectNackStep step{};
    step.result_code = static_cast<std::uint32_t>(result);
    step.skin_delay_remaining = skin_delay_remaining;
    step.dw_limit_level = dw_limit_level;
    plan.steps.push_back(step);
    plan.send_nack = true;
    return plan;
}

}  // namespace mxh::server
