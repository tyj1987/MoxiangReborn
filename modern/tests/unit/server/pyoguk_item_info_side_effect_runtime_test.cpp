// pyoguk_item_info_side_effect_runtime_test.cpp
//
// Verifies apply_pyoguk_item_info_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_PYOGUK_ITEM_INFO_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// 2-step chain (mark loading -> DB query) in legacy order, and stays
// a no-op for the three failure gates.

#include <mxh/server/pyoguk_item_info_side_effect.hpp>
#include <mxh/server/pyoguk_item_info_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::PyogukItemInfoSideEffectKind;
using mxh::server::PyogukItemInfoSideEffectSink;
using mxh::server::apply_pyoguk_item_info_side_effects;
using mxh::server::pyoguk_item_info_side_effect_plan;

class RecordingSink final : public PyogukItemInfoSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_user_id = 0;
    std::uint32_t last_start_db_idx = 0;

    void set_got_warehouse_items(std::uint32_t object_id) override {
        calls.push_back("mark");
        last_object_id = object_id;
    }
    void fire_pyoguk_db_query(std::uint32_t object_id,
                              std::uint32_t user_id,
                              std::uint32_t start_db_idx) override {
        calls.push_back("db");
        last_object_id = object_id;
        last_user_id = user_id;
        last_start_db_idx = start_db_idx;
    }
};

}  // namespace

TEST(ApplyPyogukItemInfoSideEffects, AllGatesPassEmitsMarkThenDbInOrder) {
    mxh::server::PyogukItemInfoValidationInput in;
    in.player_found = true;
    in.npc_check_ok = true;
    in.got_warehouse_items = false;
    auto plan = pyoguk_item_info_side_effect_plan(
        in, /*object_id=*/0x00010002u, /*user_id=*/0x00000003u);
    EXPECT_TRUE(plan.mark_loading);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukItemInfoSideEffectKind::SetGotWarehouseItems);
    EXPECT_EQ(plan.effects[1].kind,
              PyogukItemInfoSideEffectKind::FirePyogukDbQuery);

    RecordingSink sink;
    auto out = apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.marks_loading, 1u);
    EXPECT_EQ(out.db_queries, 1u);
    EXPECT_TRUE(out.mark_loading_flag_consumed);
    EXPECT_TRUE(out.trigger_db_flag_consumed);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0], "mark");
    EXPECT_EQ(sink.calls[1], "db");
    EXPECT_EQ(sink.last_object_id, 0x00010002u);
    EXPECT_EQ(sink.last_user_id, 0x00000003u);
    EXPECT_EQ(sink.last_start_db_idx, 0u);  // legacy PyogukItemOptionInfo(0)
}

TEST(ApplyPyogukItemInfoSideEffects, NoPlayerIsNoOp) {
    mxh::server::PyogukItemInfoValidationInput in;
    in.player_found = false;
    in.npc_check_ok = true;
    in.got_warehouse_items = false;
    auto plan = pyoguk_item_info_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.mark_loading);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.marks_loading, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.mark_loading_flag_consumed);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyPyogukItemInfoSideEffects, HackNpcIsNoOp) {
    mxh::server::PyogukItemInfoValidationInput in;
    in.player_found = true;
    in.npc_check_ok = false;
    in.got_warehouse_items = false;
    auto plan = pyoguk_item_info_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.mark_loading);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyPyogukItemInfoSideEffects, AlreadyLoadingIsNoOp) {
    // Legacy dedup: IsGotWarehouseItems() == TRUE -> return.
    mxh::server::PyogukItemInfoValidationInput in;
    in.player_found = true;
    in.npc_check_ok = true;
    in.got_warehouse_items = true;
    auto plan = pyoguk_item_info_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.mark_loading);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyPyogukItemInfoSideEffects, EmptyPlanIsNoOp) {
    mxh::server::PyogukItemInfoSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.marks_loading, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.mark_loading_flag_consumed);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyPyogukItemInfoSideEffects, GatePrecedenceNoPlayerOverOthers) {
    // classify: NoPlayer wins over HackNpc and AlreadyLoading.
    mxh::server::PyogukItemInfoValidationInput in;
    in.player_found = false;
    in.npc_check_ok = false;
    in.got_warehouse_items = true;
    auto plan = pyoguk_item_info_side_effect_plan(in, 7, 8);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyPyogukItemInfoSideEffects, MaxIdsStillDispatches) {
    mxh::server::PyogukItemInfoValidationInput in;
    in.player_found = true;
    in.npc_check_ok = true;
    in.got_warehouse_items = false;
    auto plan = pyoguk_item_info_side_effect_plan(
        in, 0xFFFFFFFFu, 0xFFFFFFFEu);
    EXPECT_TRUE(plan.mark_loading);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_pyoguk_item_info_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"mark", "db"}));
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_user_id, 0xFFFFFFFEu);
}
