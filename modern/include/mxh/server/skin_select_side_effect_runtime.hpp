// skin_select_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plans emitted by
// skin_select_success_side_effect_plan() (3-step success chain) and
// skin_select_nack_side_effect_plan() (NACK step on failure). The data
// planes return the ordered step lists; this header walks them and
// dispatches each entry to its respective subsystem via virtual
// callback interfaces.
//
// 1:1 invariants (1:1 with legacy [Server]Map/ItemManager.cpp:6525-6575):
//   Success path (3 steps, in legacy order):
//     1. StartSkinDelay  (legacy InitSkinDelay + StartSkinDelay).
//     2. CharacterSkinInfoUpdate (legacy DB write of new wSkinItem[]).
//     3. BroadcastSkinInfo (legacy SEND_SKIN_INFO broadcast).
//   Failure path (single NACK step):
//     - dwData1 = result_code (legacy eSkinResult_* value).
//     - dwData2 = skin_delay_remaining (legacy GetSkinDelayTime()).
//     - dwData3 = dw_limit_level (legacy pSkinInfo->dwLimitLevel).
//
// Pattern mirrors put_on_avatar_side_effect_runtime.hpp (D4.36):
// data plane in matching header, runtime orchestrator also inline
// here, tests verify behavior through the public surface.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/skin_select_side_effect.hpp>
#include <mxh/server/skin_select_transition.hpp>

namespace mxh::server {

// Subsystem callbacks for the skin-select side-effect chain.
// Production wires each method to the live subsystem (Player delay
// gate, DB thread, PACKEDDATA broadcast). Tests wire them to recording
// stubs so each test starts with empty call lists.
class SkinSelectSideEffectSink {
public:
    virtual ~SkinSelectSideEffectSink() = default;

    // Legacy: InitSkinDelay + StartSkinDelay. Sets the cooldown gate
    // so the same skin cannot be re-applied within the cooldown window.
    virtual void start_skin_delay() = 0;

    // Legacy: CharacterSkinInfoUpdate -- DB write of new wSkinItem[].
    virtual void character_skin_info_update() = 0;

    // Legacy: SEND_SKIN_INFO broadcast (MP_ITEMEXT / MP_ITEMEXT_SKINITEM_SELECT_ACK).
    virtual void broadcast_skin_info() = 0;

    // Legacy: MSG_DWORD3 NACK to the originating player on Fail /
    // DelayFail / LevelFail. The runtime passes the same 3 DWORDs the
    // legacy code would have sent:
    //   dwData1 = result_code (eSkinResult_* value)
    //   dwData2 = skin_delay_remaining (GetSkinDelayTime())
    //   dwData3 = dw_limit_level (pSkinInfo->dwLimitLevel)
    virtual void send_nack(std::uint32_t result_code,
                           std::uint32_t skin_delay_remaining,
                           std::uint32_t dw_limit_level) = 0;
};

// Outcome counters returned by the runtime so tests can assert which
// subsystems fired.
struct SkinSelectRuntimeOutcome {
    std::size_t effects_applied    = 0;
    std::size_t delays_started     = 0;
    std::size_t db_updates         = 0;
    std::size_t broadcasts         = 0;
    std::size_t nacks_sent         = 0;
    bool broadcast_flag_consumed = false;
    bool nack_flag_consumed     = false;
};

// Runtime: walks the success side-effect plan (3 steps) and dispatches
// each entry in legacy order. Returns the outcome counters.
inline SkinSelectRuntimeOutcome apply_skin_select_success_side_effects(
    const SkinSelectSideEffectPlan& plan,
    SkinSelectSideEffectSink& sink) {
    SkinSelectRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case SkinSelectSideEffectKind::StartSkinDelay:
            sink.start_skin_delay();
            ++out.delays_started;
            ++out.effects_applied;
            break;
        case SkinSelectSideEffectKind::CharacterSkinInfoUpdate:
            sink.character_skin_info_update();
            ++out.db_updates;
            ++out.effects_applied;
            break;
        case SkinSelectSideEffectKind::BroadcastSkinInfo:
            sink.broadcast_skin_info();
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        }
    }
    out.broadcast_flag_consumed = plan.send_broadcast;
    return out;
}

// Runtime: walks the NACK plan (0 or 1 step) and dispatches the
// single NACK payload to the sink. Returns the outcome counters.
inline SkinSelectRuntimeOutcome apply_skin_select_nack_side_effects(
    const SkinSelectNackPlan& plan,
    SkinSelectSideEffectSink& sink) {
    SkinSelectRuntimeOutcome out;
    for (const auto& step : plan.steps) {
        sink.send_nack(
            step.result_code,
            step.skin_delay_remaining,
            step.dw_limit_level);
        ++out.nacks_sent;
        ++out.effects_applied;
    }
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
