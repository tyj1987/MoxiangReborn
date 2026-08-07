// save_point_add_side_effect_runtime_test.cpp
//
// Verifies apply_save_point_add_success_side_effects() /
// apply_save_point_add_nack_side_effects() (the runtime orchestrators
// for the CItemManager::MP_ITEM_SHOPITEM_SAVEPOINT_ADD_SYN side-effect
// chains) walk the data-plane plans and dispatch each entry: USE_ACK
// broadcast -> SavedMovePointInsert DB in legacy order / single
// USE_NACK on failure.

#include <mxh/server/save_point_add_side_effect.hpp>
#include <mxh/server/save_point_add_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

using mxh::server::SavePointAddSideEffectSink;
using mxh::server::apply_save_point_add_nack_side_effects;
using mxh::server::apply_save_point_add_success_side_effects;
using mxh::server::save_point_add_nack_side_effect_plan;
using mxh::server::save_point_add_success_side_effect_plan;

class RecordingSink final : public SavePointAddSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    mxh::game::ShopItemBase last_base{};
    std::uint16_t last_shop_item_pos = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::array<char, 21u> last_move_name{};
    std::uint16_t last_map_num = 0;
    std::uint32_t last_point_value = 0;
    std::uint8_t last_e_code = 0;
    std::size_t ack_count = 0;
    std::size_t insert_count = 0;
    std::size_t nack_count = 0;

    void broadcast_use_ack(std::uint32_t player_id,
                           const mxh::game::ShopItemBase& shop_item_base,
                           std::uint16_t shop_item_pos,
                           std::uint16_t shop_item_idx) override {
        calls.push_back("ack");
        last_player_id = player_id;
        last_base = shop_item_base;
        last_shop_item_pos = shop_item_pos;
        last_shop_item_idx = shop_item_idx;
        ++ack_count;
    }
    void insert_saved_move_point(
        std::uint32_t player_id,
        const std::array<char, 21u>& move_name,
        std::uint16_t map_num, std::uint32_t point_value) override {
        calls.push_back("insert");
        last_player_id = player_id;
        last_move_name = move_name;
        last_map_num = map_num;
        last_point_value = point_value;
        ++insert_count;
    }
    void broadcast_use_nack(std::uint32_t player_id,
                            std::uint8_t e_code) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_e_code = e_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplySavePointAddSideEffects, SuccessEmitsBroadcastThenDbInOrder) {
    mxh::game::ShopItemBase shop_item{};
    shop_item.ItemBase.wIconIdx = 55365;
    shop_item.Remaintime = 60000;
    std::array<char, 21u> name{};
    std::strcpy(name.data(), "TestPoint");
    auto plan = save_point_add_success_side_effect_plan(
        shop_item, /*shop_item_pos=*/42, /*shop_item_idx=*/55365,
        name, /*map_num=*/3, /*point_value=*/777);
    EXPECT_TRUE(plan.send_use_ack);
    ASSERT_EQ(plan.effects.size(), 2u);

    RecordingSink sink;
    auto out = apply_save_point_add_success_side_effects(
        /*player_id=*/0x001C001Du, plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.use_acks_sent, 1u);
    EXPECT_EQ(out.db_inserts, 1u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"ack", "insert"}));
    EXPECT_EQ(sink.last_player_id, 0x001C001Du);
    EXPECT_EQ(sink.last_base.ItemBase.wIconIdx, 55365u);
    EXPECT_EQ(sink.last_base.Remaintime, 60000u);
    EXPECT_EQ(sink.last_shop_item_pos, 42u);
    EXPECT_EQ(sink.last_shop_item_idx, 55365u);
    EXPECT_EQ(sink.last_map_num, 3u);
    EXPECT_EQ(sink.last_point_value, 777u);
    EXPECT_EQ(std::string(sink.last_move_name.data()), "TestPoint");
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.insert_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplySavePointAddSideEffects, FailureEmitsSingleUseNack) {
    auto plan = save_point_add_nack_side_effect_plan(/*e_code=*/9);
    EXPECT_TRUE(plan.send_use_nack);
    ASSERT_EQ(plan.steps.size(), 1u);

    RecordingSink sink;
    auto out = apply_save_point_add_nack_side_effects(7u, plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.use_nacks_sent, 1u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.db_inserts, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_player_id, 7u);
    EXPECT_EQ(sink.last_e_code, 9u);
}

TEST(ApplySavePointAddSideEffects, NackECodeBoundary) {
    auto plan = save_point_add_nack_side_effect_plan(0xFFu);
    RecordingSink sink;
    (void)apply_save_point_add_nack_side_effects(1u, plan, sink);
    EXPECT_EQ(sink.last_e_code, 0xFFu);
}

TEST(ApplySavePointAddSideEffects, ZeroValuesStillDispatch) {
    mxh::game::ShopItemBase shop_item{};
    std::array<char, 21u> name{};
    auto plan = save_point_add_success_side_effect_plan(
        shop_item, 0, 0, name, 0, 0);
    RecordingSink sink;
    auto out = apply_save_point_add_success_side_effects(3u, plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(sink.last_shop_item_pos, 0u);
    EXPECT_EQ(sink.last_shop_item_idx, 0u);
    EXPECT_EQ(sink.last_map_num, 0u);
    EXPECT_EQ(sink.last_point_value, 0u);
}

TEST(ApplySavePointAddSideEffects, SuccessEmptyPlanIsNoOp) {
    mxh::server::SavePointAddSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_save_point_add_success_side_effects(3u, plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.db_inserts, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplySavePointAddSideEffects, NackEmptyPlanIsNoOp) {
    mxh::server::SavePointAddNackPlan plan;
    RecordingSink sink;
    auto out = apply_save_point_add_nack_side_effects(3u, plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.use_nacks_sent, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplySavePointAddSideEffects, NackDoesNotTouchAckState) {
    auto ack_plan = save_point_add_success_side_effect_plan(
        mxh::game::ShopItemBase{}, 1, 2, {}, 3, 4);
    RecordingSink ack_sink;
    auto ack_out =
        apply_save_point_add_success_side_effects(1u, ack_plan, ack_sink);
    EXPECT_EQ(ack_out.use_nacks_sent, 0u);
    EXPECT_EQ(ack_sink.nack_count, 0u);

    auto nack_plan = save_point_add_nack_side_effect_plan(5);
    RecordingSink nack_sink;
    auto nack_out =
        apply_save_point_add_nack_side_effects(2u, nack_plan, nack_sink);
    EXPECT_EQ(nack_out.use_acks_sent, 0u);
    EXPECT_EQ(nack_sink.ack_count, 0u);
}
