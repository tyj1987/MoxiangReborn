// dealer_open_side_effect_runtime_test.cpp
//
// Verifies apply_dealer_open_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_DEALER_SYN side-effect chain) walks
// the data-plane plan and dispatches the DealerAck entry when the
// player + NPC gates pass, and stays a silent no-op otherwise (no
// NACK on failure).

#include <mxh/server/dealer_open_side_effect.hpp>
#include <mxh/server/dealer_open_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::DealerOpenSideEffectKind;
using mxh::server::DealerOpenSideEffectSink;
using mxh::server::apply_dealer_open_side_effects;
using mxh::server::dealer_open_side_effect_plan;

class RecordingSink final : public DealerOpenSideEffectSink {
public:
    std::string last_call;
    std::uint16_t last_npc_pos = 0;
    std::size_t ack_count = 0;

    void broadcast_dealer_ack(std::uint16_t npc_pos) override {
        last_call = "ack";
        last_npc_pos = npc_pos;
        ++ack_count;
    }
};

}  // namespace

TEST(ApplyDealerOpenSideEffects, GatesPassEmitsDealerAck) {
    mxh::server::DealerOpenValidationInput in;
    in.player_found = true;
    in.npc_check_ok = true;
    auto plan = dealer_open_side_effect_plan(in, /*npc_pos=*/33);
    EXPECT_TRUE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              DealerOpenSideEffectKind::BroadcastDealerAck);
    EXPECT_EQ(plan.effects[0].npc_pos, 33u);

    RecordingSink sink;
    auto out = apply_dealer_open_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_npc_pos, 33u);
    EXPECT_EQ(sink.ack_count, 1u);
}

TEST(ApplyDealerOpenSideEffects, HackNpcIsSilentNoOp) {
    // Legacy: CheckHackNpc false -> silent drop, NO NACK.
    mxh::server::DealerOpenValidationInput in;
    in.player_found = true;
    in.npc_check_ok = false;
    auto plan = dealer_open_side_effect_plan(in, 33);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_dealer_open_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
}

TEST(ApplyDealerOpenSideEffects, NoPlayerIsSilentNoOp) {
    mxh::server::DealerOpenValidationInput in;
    in.player_found = false;
    in.npc_check_ok = true;
    auto plan = dealer_open_side_effect_plan(in, 33);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_dealer_open_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
}

TEST(ApplyDealerOpenSideEffects, EmptyPlanIsNoOp) {
    mxh::server::DealerOpenSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_dealer_open_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
}

TEST(ApplyDealerOpenSideEffects, NoPlayerOverridesNpcCheck) {
    // classify_dealer_open_outcome: NoPlayer wins over HackNpc.
    mxh::server::DealerOpenValidationInput in;
    in.player_found = false;
    in.npc_check_ok = false;
    auto plan = dealer_open_side_effect_plan(in, 33);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_dealer_open_side_effects(plan, sink);
    EXPECT_EQ(sink.ack_count, 0u);
}

TEST(ApplyDealerOpenSideEffects, ZeroNpcPosStillDispatches) {
    mxh::server::DealerOpenValidationInput in;
    in.player_found = true;
    in.npc_check_ok = true;
    auto plan = dealer_open_side_effect_plan(in, 0);
    EXPECT_TRUE(plan.send_ack);

    RecordingSink sink;
    (void)apply_dealer_open_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_npc_pos, 0u);
    EXPECT_EQ(sink.ack_count, 1u);
}
