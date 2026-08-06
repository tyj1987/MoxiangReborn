// D4.62 PyogukItemInfo (MP_ITEM_PYOGUK_ITEM_INFO_SYN) side-effect
// dispatcher tests.

#include <mxh/server/pyoguk_item_info_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

PyogukItemInfoValidationInput ok() {
    PyogukItemInfoValidationInput in{};
    in.player_found = true;
    in.npc_check_ok = true;
    in.got_warehouse_items = false;
    return in;
}

TEST(PyogukItemInfoOutcome, AllGatesPassIsTriggered) {
    auto in = ok();
    EXPECT_EQ(classify_pyoguk_item_info_outcome(in),
              PyogukItemInfoOutcome::Triggered);
}

TEST(PyogukItemInfoOutcome, NoPlayerIsNoPlayer) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_pyoguk_item_info_outcome(in),
              PyogukItemInfoOutcome::NoPlayer);
}

TEST(PyogukItemInfoOutcome, NpcCheckFailIsHackNpc) {
    auto in = ok();
    in.npc_check_ok = false;
    EXPECT_EQ(classify_pyoguk_item_info_outcome(in),
              PyogukItemInfoOutcome::HackNpc);
}

TEST(PyogukItemInfoOutcome, AlreadyLoadingIsDedup) {
    auto in = ok();
    in.got_warehouse_items = true;
    EXPECT_EQ(classify_pyoguk_item_info_outcome(in),
              PyogukItemInfoOutcome::AlreadyLoading);
}

TEST(PyogukItemInfoOutcome, PrecedenceIsNoPlayerThenHackNpcThenAlreadyLoading) {
    auto in = ok();
    in.player_found = false;
    in.npc_check_ok = false;
    in.got_warehouse_items = true;
    EXPECT_EQ(classify_pyoguk_item_info_outcome(in),
              PyogukItemInfoOutcome::NoPlayer);

    in.player_found = true;
    in.npc_check_ok = false;
    in.got_warehouse_items = true;
    EXPECT_EQ(classify_pyoguk_item_info_outcome(in),
              PyogukItemInfoOutcome::HackNpc);
}

TEST(PyogukItemInfoPlan, TriggeredEmitsTwoSteps) {
    auto in = ok();
    auto plan = pyoguk_item_info_side_effect_plan(
        in, /*object_id=*/100, /*user_id=*/200);
    EXPECT_TRUE(plan.mark_loading);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukItemInfoSideEffectKind::SetGotWarehouseItems);
    EXPECT_EQ(plan.effects[0].object_id, 100u);
    EXPECT_EQ(plan.effects[1].kind,
              PyogukItemInfoSideEffectKind::FirePyogukDbQuery);
    EXPECT_EQ(plan.effects[1].object_id, 100u);
    EXPECT_EQ(plan.effects[1].user_id, 200u);
    EXPECT_EQ(plan.effects[1].start_db_idx, 0u);
}

TEST(PyogukItemInfoPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = pyoguk_item_info_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.mark_loading);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(PyogukItemInfoPlan, HackNpcEmitsEmptyPlan) {
    auto in = ok();
    in.npc_check_ok = false;
    auto plan = pyoguk_item_info_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.mark_loading);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(PyogukItemInfoPlan, AlreadyLoadingEmitsEmptyPlan) {
    auto in = ok();
    in.got_warehouse_items = true;
    auto plan = pyoguk_item_info_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.mark_loading);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(PyogukItemInfoPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = pyoguk_item_info_side_effect_plan(in, 1, 2);
    auto b = pyoguk_item_info_side_effect_plan(in, 1, 2);
    EXPECT_EQ(a.mark_loading, b.mark_loading);
    EXPECT_EQ(a.trigger_db, b.trigger_db);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
        EXPECT_EQ(a.effects[i].user_id, b.effects[i].user_id);
    }
}
