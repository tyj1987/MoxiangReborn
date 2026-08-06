// D4.42 SavePointUpdate/Del (MP_ITEM_SHOPITEM_SAVEPOINT_UPDATE_SYN /
// MP_ITEM_SHOPITEM_SAVEPOINT_DEL_SYN) side-effect dispatcher tests.

#include <mxh/server/save_point_mutate_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

// ---------- SavePointUpdate ----------

TEST(SavePointUpdate, MatchEmitsDbThenAckBroadcast) {
    std::array<char, LEGACY_MAX_SAVED_MOVE_NAME> name{};
    const char* lit = "NewName";
    for (std::size_t i = 0; i < 7; ++i) name[i] = lit[i];

    auto plan = save_point_update_side_effect_plan(
        /*db_idx=*/42u, name, /*has_match=*/true);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.ack_protocol, LEGACY_MP_SAVEPOINT_UPDATE_ACK);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              SavePointUpdateSideEffectKind::RenameSavedMovePoint);
    EXPECT_EQ(plan.effects[0].db_idx, 42u);
    EXPECT_EQ(plan.effects[1].kind,
              SavePointUpdateSideEffectKind::BroadcastUpdateAck);
}

TEST(SavePointUpdate, MissEmitsSingleNackBroadcast) {
    std::array<char, LEGACY_MAX_SAVED_MOVE_NAME> name{};
    auto plan = save_point_update_side_effect_plan(
        /*db_idx=*/999u, name, /*has_match=*/false);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_protocol, LEGACY_MP_SAVEPOINT_UPDATE_NACK);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              SavePointUpdateSideEffectKind::BroadcastUpdateNack);
    EXPECT_EQ(plan.effects[0].db_idx, 999u);
}

TEST(SavePointUpdate, PlanIsIdempotent) {
    std::array<char, LEGACY_MAX_SAVED_MOVE_NAME> name{};
    auto a = save_point_update_side_effect_plan(1u, name, true);
    auto b = save_point_update_side_effect_plan(1u, name, true);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].db_idx, b.effects[i].db_idx);
    }
}

// ---------- SavePointDel ----------

TEST(SavePointDel, MatchEmitsDbThenAckBroadcast) {
    auto plan = save_point_del_side_effect_plan(
        /*db_idx=*/7u, /*has_match=*/true);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.ack_protocol, LEGACY_MP_SAVEPOINT_DEL_ACK);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              SavePointDelSideEffectKind::DeleteSavedMovePoint);
    EXPECT_EQ(plan.effects[0].db_idx, 7u);
    EXPECT_EQ(plan.effects[1].kind,
              SavePointDelSideEffectKind::BroadcastDelAck);
}

TEST(SavePointDel, MissEmitsSingleNackBroadcast) {
    auto plan = save_point_del_side_effect_plan(
        /*db_idx=*/123u, /*has_match=*/false);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_protocol, LEGACY_MP_SAVEPOINT_DEL_NACK);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              SavePointDelSideEffectKind::BroadcastDelNack);
    EXPECT_EQ(plan.effects[0].db_idx, 123u);
}

TEST(SavePointDel, PlanIsIdempotent) {
    auto a = save_point_del_side_effect_plan(5u, true);
    auto b = save_point_del_side_effect_plan(5u, true);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].db_idx, b.effects[i].db_idx);
    }
}

TEST(SavePointMutate, ProtocolConstantsMatchLegacyGuesses) {
    // Legacy Protocol.h assigns these in the MP_ITEM category; the
    // exact values may differ between server builds but the relative
    // ACK/NACK pairing is preserved (Update ACK = Update NACK - 1,
    // Del ACK = Del NACK - 1). The data plane encodes them as named
    // constants so a wire diff can spot the reassignment.
    EXPECT_EQ(LEGACY_MP_SAVEPOINT_UPDATE_ACK, 154u);
    EXPECT_EQ(LEGACY_MP_SAVEPOINT_UPDATE_NACK, 155u);
    EXPECT_EQ(LEGACY_MP_SAVEPOINT_DEL_ACK, 156u);
    EXPECT_EQ(LEGACY_MP_SAVEPOINT_DEL_NACK, 157u);
}
