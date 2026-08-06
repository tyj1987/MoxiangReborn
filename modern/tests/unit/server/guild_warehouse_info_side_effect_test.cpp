// D4.66 GuildWarehouseInfo (MP_ITEM_GUILD_WAREHOUSE_INFO_SYN) side-effect
// dispatcher tests.

#include <mxh/server/guild_warehouse_info_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(GuildWarehouseInfoOutcome, PlayerFoundIsTriggered) {
    GuildWarehouseInfoValidationInput in{};
    in.player_found = true;
    EXPECT_EQ(classify_guild_warehouse_info_outcome(in),
              GuildWarehouseInfoOutcome::Triggered);
}

TEST(GuildWarehouseInfoOutcome, NoPlayerIsNoPlayer) {
    GuildWarehouseInfoValidationInput in{};
    in.player_found = false;
    EXPECT_EQ(classify_guild_warehouse_info_outcome(in),
              GuildWarehouseInfoOutcome::NoPlayer);
}

TEST(GuildWarehouseInfoPlan, TriggeredEmitsDbQuery) {
    GuildWarehouseInfoValidationInput in{};
    in.player_found = true;
    auto plan = guild_warehouse_info_side_effect_plan(
        in, /*object_id=*/42, /*request_type=*/2);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildWarehouseInfoSideEffectKind::FireGuildWarehouseDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_EQ(plan.effects[0].request_type, 2u);
}

TEST(GuildWarehouseInfoPlan, NoPlayerEmitsEmptyPlan) {
    GuildWarehouseInfoValidationInput in{};
    in.player_found = false;
    auto plan = guild_warehouse_info_side_effect_plan(in, 1, 0);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(GuildWarehouseInfoPlan, PlanIsIdempotent) {
    GuildWarehouseInfoValidationInput in{};
    in.player_found = true;
    auto a = guild_warehouse_info_side_effect_plan(in, 1, 0);
    auto b = guild_warehouse_info_side_effect_plan(in, 1, 0);
    EXPECT_EQ(a.trigger_db, b.trigger_db);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
        EXPECT_EQ(a.effects[i].request_type, b.effects[i].request_type);
    }
}
