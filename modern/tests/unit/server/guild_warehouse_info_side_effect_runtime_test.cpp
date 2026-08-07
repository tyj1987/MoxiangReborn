// guild_warehouse_info_side_effect_runtime_test.cpp
//
// Verifies apply_guild_warehouse_info_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_GUILD_WAREHOUSE_INFO_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// GUILDMGR->GuildWarehouseInfo DB query when the player exists, and
// stays a no-op otherwise (no ACK/NACK; data arrives later via guild
// manager broadcasts).

#include <mxh/server/guild_warehouse_info_side_effect.hpp>
#include <mxh/server/guild_warehouse_info_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::GuildWarehouseInfoSideEffectKind;
using mxh::server::GuildWarehouseInfoSideEffectSink;
using mxh::server::apply_guild_warehouse_info_side_effects;
using mxh::server::guild_warehouse_info_side_effect_plan;

class RecordingSink final : public GuildWarehouseInfoSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_object_id = 0;
    std::uint8_t last_request_type = 0;
    std::size_t db_count = 0;

    void fire_guild_warehouse_db_query(
        std::uint32_t object_id, std::uint8_t request_type) override {
        last_call = "db";
        last_object_id = object_id;
        last_request_type = request_type;
        ++db_count;
    }
};

}  // namespace

TEST(ApplyGuildWarehouseInfoSideEffects, PlayerFoundEmitsDbQuery) {
    mxh::server::GuildWarehouseInfoValidationInput in;
    in.player_found = true;
    auto plan = guild_warehouse_info_side_effect_plan(
        in, /*object_id=*/0x00010002u, /*request_type=*/3);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildWarehouseInfoSideEffectKind::FireGuildWarehouseDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 0x00010002u);
    EXPECT_EQ(plan.effects[0].request_type, 3u);

    RecordingSink sink;
    auto out = apply_guild_warehouse_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.db_queries, 1u);
    EXPECT_TRUE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0x00010002u);
    EXPECT_EQ(sink.last_request_type, 3u);
    EXPECT_EQ(sink.db_count, 1u);
}

TEST(ApplyGuildWarehouseInfoSideEffects, NoPlayerIsNoOp) {
    mxh::server::GuildWarehouseInfoValidationInput in;
    in.player_found = false;
    auto plan = guild_warehouse_info_side_effect_plan(in, 7, 1);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_guild_warehouse_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyGuildWarehouseInfoSideEffects, EmptyPlanIsNoOp) {
    mxh::server::GuildWarehouseInfoSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_guild_warehouse_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyGuildWarehouseInfoSideEffects, MaxObjectIdStillDispatches) {
    mxh::server::GuildWarehouseInfoValidationInput in;
    in.player_found = true;
    auto plan = guild_warehouse_info_side_effect_plan(
        in, 0xFFFFFFFFu, 255);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_guild_warehouse_info_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_request_type, 255u);
    EXPECT_EQ(sink.db_count, 1u);
}

TEST(ApplyGuildWarehouseInfoSideEffects, ZeroRequestTypeStillDispatches) {
    mxh::server::GuildWarehouseInfoValidationInput in;
    in.player_found = true;
    auto plan = guild_warehouse_info_side_effect_plan(in, 7, 0);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_guild_warehouse_info_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 7u);
    EXPECT_EQ(sink.last_request_type, 0u);
    EXPECT_EQ(sink.db_count, 1u);
}
