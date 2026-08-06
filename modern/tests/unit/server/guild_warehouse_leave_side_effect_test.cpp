// D4.67 GuildWarehouseLeave (MP_ITEM_GUILD_WAREHOUSE_LEAVE) side-effect
// dispatcher tests.

#include <mxh/server/guild_warehouse_leave_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(GuildWarehouseLeaveOutcome, PlayerFoundIsLeft) {
    GuildWarehouseLeaveValidationInput in{};
    in.player_found = true;
    EXPECT_EQ(classify_guild_warehouse_leave_outcome(in),
              GuildWarehouseLeaveOutcome::Left);
}

TEST(GuildWarehouseLeaveOutcome, NoPlayerIsNoPlayer) {
    GuildWarehouseLeaveValidationInput in{};
    in.player_found = false;
    EXPECT_EQ(classify_guild_warehouse_leave_outcome(in),
              GuildWarehouseLeaveOutcome::NoPlayer);
}

TEST(GuildWarehouseLeavePlan, LeftEmitsLeave) {
    GuildWarehouseLeaveValidationInput in{};
    in.player_found = true;
    auto plan = guild_warehouse_leave_side_effect_plan(
        in, /*object_id=*/42, /*request_type=*/0);
    EXPECT_TRUE(plan.fired);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildWarehouseLeaveSideEffectKind::FireGuildLeaveWarehouse);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_EQ(plan.effects[0].request_type, 0u);
}

TEST(GuildWarehouseLeavePlan, NoPlayerEmitsEmptyPlan) {
    GuildWarehouseLeaveValidationInput in{};
    in.player_found = false;
    auto plan = guild_warehouse_leave_side_effect_plan(in, 1, 0);
    EXPECT_FALSE(plan.fired);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(GuildWarehouseLeavePlan, PlanIsIdempotent) {
    GuildWarehouseLeaveValidationInput in{};
    in.player_found = true;
    auto a = guild_warehouse_leave_side_effect_plan(in, 1, 0);
    auto b = guild_warehouse_leave_side_effect_plan(in, 1, 0);
    EXPECT_EQ(a.fired, b.fired);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
    }
}
