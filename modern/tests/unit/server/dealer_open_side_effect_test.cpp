// D4.61 DealerOpen (MP_ITEM_DEALER_SYN) side-effect dispatcher tests.

#include <mxh/server/dealer_open_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

DealerOpenValidationInput ok() {
    DealerOpenValidationInput in{};
    in.player_found = true;
    in.npc_check_ok = true;
    return in;
}

TEST(DealerOpenOutcome, PlayerAndNpcOkIsOpened) {
    auto in = ok();
    EXPECT_EQ(classify_dealer_open_outcome(in),
              DealerOpenOutcome::Opened);
}

TEST(DealerOpenOutcome, NoPlayerIsNoPlayer) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_dealer_open_outcome(in),
              DealerOpenOutcome::NoPlayer);
}

TEST(DealerOpenOutcome, NpcCheckFailIsHackNpc) {
    auto in = ok();
    in.npc_check_ok = false;
    EXPECT_EQ(classify_dealer_open_outcome(in),
              DealerOpenOutcome::HackNpc);
}

TEST(DealerOpenOutcome, NoPlayerTakesPrecedenceOverHackNpc) {
    auto in = ok();
    in.player_found = false;
    in.npc_check_ok = false;
    EXPECT_EQ(classify_dealer_open_outcome(in),
              DealerOpenOutcome::NoPlayer);
}

TEST(DealerOpenPlan, OpenedEmitsDealerAck) {
    auto in = ok();
    auto plan = dealer_open_side_effect_plan(in, /*npc_pos=*/42);
    EXPECT_TRUE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              DealerOpenSideEffectKind::BroadcastDealerAck);
    EXPECT_EQ(plan.effects[0].npc_pos, 42u);
}

TEST(DealerOpenPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = dealer_open_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(DealerOpenPlan, HackNpcEmitsEmptyPlan) {
    auto in = ok();
    in.npc_check_ok = false;
    auto plan = dealer_open_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(DealerOpenPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = dealer_open_side_effect_plan(in, 7);
    auto b = dealer_open_side_effect_plan(in, 7);
    EXPECT_EQ(a.send_ack, b.send_ack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].npc_pos, b.effects[i].npc_pos);
    }
}
