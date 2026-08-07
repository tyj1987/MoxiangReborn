// guild_warehouse_leave_side_effect_runtime_test.cpp
//
// Verifies apply_guild_warehouse_leave_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_GUILD_WAREHOUSE_LEAVE
// side-effect chain) walks the data-plane plan and dispatches the
// GUILDMGR->LeaveWareHouse call when the player exists, and stays a
// no-op otherwise (pure server-side state, no ACK/NACK).

#include <mxh/server/guild_warehouse_leave_side_effect.hpp>
#include <mxh/server/guild_warehouse_leave_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::GuildWarehouseLeaveSideEffectKind;
using mxh::server::GuildWarehouseLeaveSideEffectSink;
using mxh::server::apply_guild_warehouse_leave_side_effects;
using mxh::server::guild_warehouse_leave_side_effect_plan;

class RecordingSink final : public GuildWarehouseLeaveSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_object_id = 0;
    std::uint8_t last_request_type = 0;
    std::size_t fire_count = 0;

    void fire_guild_leave_warehouse(
        std::uint32_t object_id, std::uint8_t request_type) override {
        last_call = "leave";
        last_object_id = object_id;
        last_request_type = request_type;
        ++fire_count;
    }
};

}  // namespace

TEST(ApplyGuildWarehouseLeaveSideEffects, PlayerFoundEmitsLeave) {
    mxh::server::GuildWarehouseLeaveValidationInput in;
    in.player_found = true;
    auto plan = guild_warehouse_leave_side_effect_plan(
        in, /*object_id=*/0x00010002u, /*request_type=*/1);
    EXPECT_TRUE(plan.fired);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildWarehouseLeaveSideEffectKind::FireGuildLeaveWarehouse);
    EXPECT_EQ(plan.effects[0].object_id, 0x00010002u);
    EXPECT_EQ(plan.effects[0].request_type, 1u);

    RecordingSink sink;
    auto out = apply_guild_warehouse_leave_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.fires, 1u);
    EXPECT_TRUE(out.fired_flag_consumed);
    EXPECT_EQ(sink.last_call, "leave");
    EXPECT_EQ(sink.last_object_id, 0x00010002u);
    EXPECT_EQ(sink.last_request_type, 1u);
    EXPECT_EQ(sink.fire_count, 1u);
}

TEST(ApplyGuildWarehouseLeaveSideEffects, NoPlayerIsNoOp) {
    mxh::server::GuildWarehouseLeaveValidationInput in;
    in.player_found = false;
    auto plan = guild_warehouse_leave_side_effect_plan(in, 7, 1);
    EXPECT_FALSE(plan.fired);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_guild_warehouse_leave_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.fires, 0u);
    EXPECT_FALSE(out.fired_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.fire_count, 0u);
}

TEST(ApplyGuildWarehouseLeaveSideEffects, EmptyPlanIsNoOp) {
    mxh::server::GuildWarehouseLeaveSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_guild_warehouse_leave_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.fires, 0u);
    EXPECT_FALSE(out.fired_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.fire_count, 0u);
}

TEST(ApplyGuildWarehouseLeaveSideEffects, MaxObjectIdStillDispatches) {
    mxh::server::GuildWarehouseLeaveValidationInput in;
    in.player_found = true;
    auto plan = guild_warehouse_leave_side_effect_plan(
        in, 0xFFFFFFFFu, 255);
    EXPECT_TRUE(plan.fired);

    RecordingSink sink;
    (void)apply_guild_warehouse_leave_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "leave");
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_request_type, 255u);
    EXPECT_EQ(sink.fire_count, 1u);
}

TEST(ApplyGuildWarehouseLeaveSideEffects, ZeroRequestTypeStillDispatches) {
    mxh::server::GuildWarehouseLeaveValidationInput in;
    in.player_found = true;
    auto plan = guild_warehouse_leave_side_effect_plan(in, 7, 0);
    EXPECT_TRUE(plan.fired);

    RecordingSink sink;
    (void)apply_guild_warehouse_leave_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "leave");
    EXPECT_EQ(sink.last_object_id, 7u);
    EXPECT_EQ(sink.last_request_type, 0u);
    EXPECT_EQ(sink.fire_count, 1u);
}
