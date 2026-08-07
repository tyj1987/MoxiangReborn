// save_point_mutate_side_effect_runtime_test.cpp
//
// Verifies apply_save_point_update_side_effects() /
// apply_save_point_del_side_effects() (the runtime orchestrators for
// the CItemManager SAVEPOINT update/delete side-effect chains) walk
// the data-plane plans and dispatch each entry: DB mutate then ACK
// echo on success / NACK echo on failure.

#include <mxh/server/save_point_mutate_side_effect.hpp>
#include <mxh/server/save_point_mutate_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

using mxh::server::LEGACY_MP_SAVEPOINT_DEL_ACK;
using mxh::server::LEGACY_MP_SAVEPOINT_DEL_NACK;
using mxh::server::LEGACY_MP_SAVEPOINT_UPDATE_ACK;
using mxh::server::LEGACY_MP_SAVEPOINT_UPDATE_NACK;
using mxh::server::SavePointMutateSideEffectSink;
using mxh::server::apply_save_point_del_side_effects;
using mxh::server::apply_save_point_update_side_effects;
using mxh::server::save_point_del_side_effect_plan;
using mxh::server::save_point_update_side_effect_plan;

class RecordingSink final : public SavePointMutateSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_db_idx = 0;
    std::array<char, 21u> last_new_name{};
    std::size_t rename_count = 0;
    std::size_t update_ack_count = 0;
    std::size_t update_nack_count = 0;
    std::size_t delete_count = 0;
    std::size_t del_ack_count = 0;
    std::size_t del_nack_count = 0;

    void rename_saved_move_point(
        std::uint32_t db_idx,
        const std::array<char, 21u>& new_name) override {
        calls.push_back("rename");
        last_db_idx = db_idx;
        last_new_name = new_name;
        ++rename_count;
    }
    void broadcast_update_ack(
        std::uint32_t db_idx,
        const std::array<char, 21u>& new_name) override {
        calls.push_back("uack");
        last_db_idx = db_idx;
        last_new_name = new_name;
        ++update_ack_count;
    }
    void broadcast_update_nack(
        std::uint32_t db_idx,
        const std::array<char, 21u>& new_name) override {
        calls.push_back("unack");
        last_db_idx = db_idx;
        last_new_name = new_name;
        ++update_nack_count;
    }
    void delete_saved_move_point(std::uint32_t db_idx) override {
        calls.push_back("del");
        last_db_idx = db_idx;
        ++delete_count;
    }
    void broadcast_del_ack(std::uint32_t db_idx) override {
        calls.push_back("dack");
        last_db_idx = db_idx;
        ++del_ack_count;
    }
    void broadcast_del_nack(std::uint32_t db_idx) override {
        calls.push_back("dnack");
        last_db_idx = db_idx;
        ++del_nack_count;
    }
};

}  // namespace

TEST(ApplySavePointMutateSideEffects, UpdateSuccessEmitsRenameThenAckInOrder) {
    std::array<char, 21u> name{};
    std::strcpy(name.data(), "NewName");
    auto plan = save_point_update_side_effect_plan(
        /*db_idx=*/0x001E001Fu, name, /*has_match=*/true);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.ack_protocol, LEGACY_MP_SAVEPOINT_UPDATE_ACK);
    ASSERT_EQ(plan.effects.size(), 2u);

    RecordingSink sink;
    auto out = apply_save_point_update_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.renames, 1u);
    EXPECT_EQ(out.update_acks_sent, 1u);
    EXPECT_EQ(out.update_nacks_sent, 0u);
    EXPECT_TRUE(out.update_ack_flag_consumed);
    EXPECT_FALSE(out.update_nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"rename", "uack"}));
    EXPECT_EQ(sink.last_db_idx, 0x001E001Fu);
    EXPECT_EQ(std::string(sink.last_new_name.data()), "NewName");
    EXPECT_EQ(sink.rename_count, 1u);
    EXPECT_EQ(sink.update_ack_count, 1u);
    EXPECT_EQ(sink.update_nack_count, 0u);
}

TEST(ApplySavePointMutateSideEffects, UpdateFailureEmitsNackOnly) {
    std::array<char, 21u> name{};
    auto plan = save_point_update_side_effect_plan(5u, name, false);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_EQ(plan.nack_protocol, LEGACY_MP_SAVEPOINT_UPDATE_NACK);
    ASSERT_EQ(plan.effects.size(), 1u);

    RecordingSink sink;
    auto out = apply_save_point_update_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.update_nacks_sent, 1u);
    EXPECT_EQ(out.renames, 0u);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"unack"}));
    EXPECT_EQ(sink.last_db_idx, 5u);
}

TEST(ApplySavePointMutateSideEffects, DelSuccessEmitsDeleteThenAckInOrder) {
    auto plan = save_point_del_side_effect_plan(
        /*db_idx=*/0x00200021u, /*has_match=*/true);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.ack_protocol, LEGACY_MP_SAVEPOINT_DEL_ACK);
    ASSERT_EQ(plan.effects.size(), 2u);

    RecordingSink sink;
    auto out = apply_save_point_del_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.deletes, 1u);
    EXPECT_EQ(out.del_acks_sent, 1u);
    EXPECT_EQ(out.del_nacks_sent, 0u);
    EXPECT_TRUE(out.del_ack_flag_consumed);
    EXPECT_FALSE(out.del_nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"del", "dack"}));
    EXPECT_EQ(sink.last_db_idx, 0x00200021u);
    EXPECT_EQ(sink.delete_count, 1u);
    EXPECT_EQ(sink.del_ack_count, 1u);
    EXPECT_EQ(sink.del_nack_count, 0u);
}

TEST(ApplySavePointMutateSideEffects, DelFailureEmitsNackOnly) {
    auto plan = save_point_del_side_effect_plan(9u, false);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_EQ(plan.nack_protocol, LEGACY_MP_SAVEPOINT_DEL_NACK);
    ASSERT_EQ(plan.effects.size(), 1u);

    RecordingSink sink;
    auto out = apply_save_point_del_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.del_nacks_sent, 1u);
    EXPECT_EQ(out.deletes, 0u);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"dnack"}));
    EXPECT_EQ(sink.last_db_idx, 9u);
}

TEST(ApplySavePointMutateSideEffects, DbIdxBoundaryAndNamePassthrough) {
    std::array<char, 21u> name{};
    std::strcpy(name.data(), "01234567890123456789");  // 20 chars + nul
    auto plan = save_point_update_side_effect_plan(
        0xFFFFFFFFu, name, true);
    RecordingSink sink;
    (void)apply_save_point_update_side_effects(plan, sink);
    EXPECT_EQ(sink.last_db_idx, 0xFFFFFFFFu);
    EXPECT_EQ(std::string(sink.last_new_name.data()),
              "01234567890123456789");

    auto del_plan = save_point_del_side_effect_plan(0xFFFFFFFFu, true);
    RecordingSink del_sink;
    (void)apply_save_point_del_side_effects(del_plan, del_sink);
    EXPECT_EQ(del_sink.last_db_idx, 0xFFFFFFFFu);
}

TEST(ApplySavePointMutateSideEffects, UpdateEmptyPlanIsNoOp) {
    mxh::server::SavePointUpdateSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_save_point_update_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.renames, 0u);
    EXPECT_EQ(out.update_acks_sent, 0u);
    EXPECT_EQ(out.update_nacks_sent, 0u);
    EXPECT_FALSE(out.update_ack_flag_consumed);
    EXPECT_FALSE(out.update_nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplySavePointMutateSideEffects, DelEmptyPlanIsNoOp) {
    mxh::server::SavePointDelSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_save_point_del_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.deletes, 0u);
    EXPECT_EQ(out.del_acks_sent, 0u);
    EXPECT_EQ(out.del_nacks_sent, 0u);
    EXPECT_FALSE(out.del_ack_flag_consumed);
    EXPECT_FALSE(out.del_nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
