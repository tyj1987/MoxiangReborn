// put_on_avatar_side_effect_runtime_test.cpp
//
// Verifies apply_put_on_avatar_side_effects() (the runtime
// orchestrator for the CShopItemManager::PutOnAvatarItem
// side-effect chain) walks the data-plane plan and dispatches each
// entry to its respective subsystem in the legacy order.
//
// Locks:
//   - BroadcastAvatarInfo and RecomputeAvatarOption are dispatched
//     in the legacy order (broadcast first, then recompute)
//   - Empty plan is a no-op
//   - Single-effect plans dispatch only the matching subsystem
//   - RecomputeAvatarOption.calc_stats flag flows through to the sink

#include <mxh/server/put_on_avatar_side_effect.hpp>
#include <mxh/server/put_on_avatar_side_effect_runtime.hpp>
#include <mxh/server/avatar_equip_transition.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::AvatarEquipStatus;
using mxh::server::AvatarEquipTransition;
using mxh::server::PutOnAvatarSideEffectKind;
using mxh::server::PutOnAvatarSideEffectSink;
using mxh::server::apply_put_on_avatar_side_effects;

class RecordingSink final : public PutOnAvatarSideEffectSink {
public:
    std::vector<std::string> calls;
    bool recompute_returns_nonzero = true;

    void broadcast_avatar_info(std::uint16_t w_icon_idx,
                               std::uint16_t item_pos) override {
        calls.push_back("broadcast " + std::to_string(w_icon_idx) + "/" +
                        std::to_string(item_pos));
    }
    mxh::game::AvatarItemOption recompute_avatar_option(
        bool calc_stats) override {
        calls.push_back(std::string("recompute ") +
                        (calc_stats ? "stats" : "nostats"));
        mxh::game::AvatarItemOption out{};
        if (recompute_returns_nonzero) {
            out.Life = 100;
        }
        return out;
    }
};

AvatarEquipTransition make_transition(bool send_info, bool recalc,
                                      bool calc_stats = true) {
    AvatarEquipTransition t;
    t.status = AvatarEquipStatus::Ok;
    t.send_avatar_info = send_info;
    t.recalculate_avatar_option = recalc;
    t.calc_stats = calc_stats;
    return t;
}

}  // namespace

// ----- both effects -----

TEST(ApplyPutOnAvatarSideEffects, BothEffectsDispatchedInLegacyOrder) {
    auto plan = put_on_avatar_side_effect_plan(
        make_transition(/*send=*/true, /*recalc=*/true), /*dw=*/42);
    RecordingSink sink;
    auto out = apply_put_on_avatar_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_EQ(out.recomputes, 1u);
    EXPECT_TRUE(out.last_recompute_invoked);
    const std::vector<std::string> kExpected = {"broadcast 42/0", "recompute stats"};
    EXPECT_EQ(sink.calls, kExpected);
}

TEST(ApplyPutOnAvatarSideEffects, RecomputeValueFlowsIntoOutcome) {
    auto plan = put_on_avatar_side_effect_plan(
        make_transition(/*send=*/false, /*recalc=*/true), /*dw=*/10);
    RecordingSink sink;
    sink.recompute_returns_nonzero = true;
    auto out = apply_put_on_avatar_side_effects(plan, sink);
    EXPECT_EQ(out.recomputes, 1u);
    EXPECT_EQ(out.last_avatar_option.Life, 100u);
}

// ----- broadcast only -----

TEST(ApplyPutOnAvatarSideEffects, BroadcastOnly) {
    auto plan = put_on_avatar_side_effect_plan(
        make_transition(/*send=*/true, /*recalc=*/false), /*dw=*/100);
    RecordingSink sink;
    auto out = apply_put_on_avatar_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_EQ(out.recomputes, 0u);
    EXPECT_FALSE(out.last_recompute_invoked);
    const std::vector<std::string> kExpected = {"broadcast 100/0"};
    EXPECT_EQ(sink.calls, kExpected);
}

// ----- recompute only -----

TEST(ApplyPutOnAvatarSideEffects, RecomputeOnly) {
    auto plan = put_on_avatar_side_effect_plan(
        make_transition(/*send=*/false, /*recalc=*/true), /*dw=*/200);
    RecordingSink sink;
    auto out = apply_put_on_avatar_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.broadcasts, 0u);
    EXPECT_EQ(out.recomputes, 1u);
    EXPECT_TRUE(out.last_recompute_invoked);
    const std::vector<std::string> kExpected = {"recompute stats"};
    EXPECT_EQ(sink.calls, kExpected);
}

// ----- empty plan (transition status != Ok suppresses both flags) -----

TEST(ApplyPutOnAvatarSideEffects, EmptyPlanIsNoOp) {
    AvatarEquipTransition t;
    t.status = AvatarEquipStatus::AvatarMissing;  // suppresses both flags
    auto plan = put_on_avatar_side_effect_plan(t, /*dw=*/1);
    EXPECT_TRUE(plan.effects.empty());
    RecordingSink sink;
    auto out = apply_put_on_avatar_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

// ----- calc_stats flag -----

TEST(ApplyPutOnAvatarSideEffects, RecomputeCalcStatsFalsePropagates) {
    auto plan = put_on_avatar_side_effect_plan(
        make_transition(/*send=*/false, /*recalc=*/true,
                        /*calc_stats=*/false),
        /*dw=*/10);
    RecordingSink sink;
    auto out = apply_put_on_avatar_side_effects(plan, sink);
    EXPECT_EQ(out.recomputes, 1u);
    const std::vector<std::string> kExpected = {"recompute nostats"};
    EXPECT_EQ(sink.calls, kExpected);
}
