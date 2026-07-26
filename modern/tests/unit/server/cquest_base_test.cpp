#include <mxh/server/cquest_base.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(CQuestBaseInit, ConstructorDefaultsMatchLegacy) {
    const auto state = make_cquest_base();
    EXPECT_EQ(state.m_dwQuestIdx, 0u);
    EXPECT_EQ(state.m_State.value, 0u);
    EXPECT_EQ(state.m_nValidNum, 0);
}

TEST(CQuestBaseInit, InitStoresIndexAndRawState) {
    auto state = make_cquest_base();
    cquest_base_init(state, 77, 0x80000001u);
    EXPECT_EQ(cquest_base_get_quest_idx(state), 77u);
    EXPECT_EQ(state.m_State.value, 0x80000001u);
}

TEST(CQuestBaseValidBits, StoresValueWithoutClamping) {
    auto state = make_cquest_base();
    cquest_base_set_valid_bit_num(state, -3);
    EXPECT_EQ(state.m_nValidNum, -3);
    cquest_base_set_valid_bit_num(state, 99);
    EXPECT_EQ(state.m_nValidNum, 99);
}

TEST(QuestFlagIsSet, BitOneIsMostSignificantBit) {
    QuestFlag flag{0x80000000u};
    EXPECT_TRUE(quest_flag_is_set(flag, 1));
    EXPECT_FALSE(quest_flag_is_set(flag, 32));
}

TEST(QuestFlagIsSet, BitThirtyTwoIsLeastSignificantBit) {
    QuestFlag flag{0x00000001u};
    EXPECT_FALSE(quest_flag_is_set(flag, 1));
    EXPECT_TRUE(quest_flag_is_set(flag, 32));
}

TEST(QuestFlagIsSet, InvalidBitReturnsTrue) {
    QuestFlag flag{};
    EXPECT_TRUE(quest_flag_is_set(flag, 0));
    EXPECT_TRUE(quest_flag_is_set(flag, 33));
}

TEST(QuestFlagSetField, DefaultFalsePreservesLegacyNoOpBug) {
    QuestFlag flag{};
    quest_flag_set_field(flag, 1);
    EXPECT_EQ(flag.value, 0u);
}

TEST(QuestFlagSetField, ExplicitTrueSetsRequestedBit) {
    QuestFlag flag{};
    quest_flag_set_field(flag, 1, true);
    EXPECT_EQ(flag.value, 0x80000000u);
    quest_flag_set_field(flag, 32, true);
    EXPECT_EQ(flag.value, 0x80000001u);
}

TEST(QuestFlagSetField, InvalidBitIsNoOp) {
    QuestFlag flag{123u};
    quest_flag_set_field(flag, 0, true);
    quest_flag_set_field(flag, 33, true);
    EXPECT_EQ(flag.value, 123u);
}

TEST(QuestFlagSetField, OperationCanSetButNeverClear) {
    QuestFlag flag{0x80000000u};
    quest_flag_set_field(flag, 1, false);
    EXPECT_EQ(flag.value, 0x80000000u);
}

TEST(CQuestBaseSetState, MapServerCallDoesNotChangeStateOrNotify) {
    auto state = make_cquest_base();
    cquest_base_init(state, 77, 0);
    const auto notification = cquest_base_set_state(state, 1, true, 9);
    EXPECT_EQ(state.m_State.value, 0u);
    EXPECT_FALSE(notification.has_value());
}

TEST(CQuestBaseSetState, ClientBuildNotifiesEvenThoughDefaultSetIsNoOp) {
    auto state = make_cquest_base();
    cquest_base_init(state, 77, 5);
    const auto notification = cquest_base_set_state(state, 2, false, 9);
    ASSERT_TRUE(notification.has_value());
    EXPECT_EQ(notification->objectId, 9u);
    EXPECT_EQ(notification->questIdx, 77u);
    EXPECT_EQ(notification->state, 5u);
}

TEST(CQuestBaseSetValue, MapServerStoresValueWithoutNotification) {
    auto state = make_cquest_base();
    cquest_base_init(state, 77, 0);
    EXPECT_FALSE(cquest_base_set_value(state, 0x1234u).has_value());
    EXPECT_EQ(state.m_State.value, 0x1234u);
}

TEST(CQuestBaseSetValue, ClientBuildReturnsChangeStatePayload) {
    auto state = make_cquest_base();
    cquest_base_init(state, 77, 0);
    const auto notification = cquest_base_set_value(state, 0x1234u, false, 9);
    ASSERT_TRUE(notification.has_value());
    EXPECT_EQ(notification->objectId, 9u);
    EXPECT_EQ(notification->questIdx, 77u);
    EXPECT_EQ(notification->state, 0x1234u);
}

TEST(CQuestBaseComplete, UsesOnlyMostSignificantBit) {
    auto state = make_cquest_base();
    cquest_base_init(state, 1, 0x7fffffffu);
    EXPECT_FALSE(cquest_base_is_complete(state));
    cquest_base_set_value(state, 0x80000000u);
    EXPECT_TRUE(cquest_base_is_complete(state));
}
