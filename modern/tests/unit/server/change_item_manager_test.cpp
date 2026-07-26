#include <mxh/server/change_item_manager.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ChangeItemConstants, MatchLegacy) {
    EXPECT_EQ(MAX_CHANGE_RATE, 30001u);
    EXPECT_EQ(MAX_YOUNGYAKITEM_DUPNUM, 20u);
    EXPECT_EQ(CHANGE_REWARD_MONEY, 7999u);
    EXPECT_EQ(CHANGE_REWARD_ABILITY, 7997u);
}

TEST(ChangeItemRewardKind, MapsSpecialIds) {
    EXPECT_EQ(change_reward_kind(7999), ChangeRewardKind::Money);
    EXPECT_EQ(change_reward_kind(7998), ChangeRewardKind::Event);
    EXPECT_EQ(change_reward_kind(7997), ChangeRewardKind::Ability);
    EXPECT_EQ(change_reward_kind(7996), ChangeRewardKind::Event2);
    EXPECT_EQ(change_reward_kind(100), ChangeRewardKind::Item);
}

TEST(ChangeItemRegistration, RejectsZeroAndReplacesDuplicate) {
    ChangeItemManagerState state;
    EXPECT_FALSE(register_change_item(state, {}));
    register_change_item(state, {10, {{100, 1, 10, false}}});
    register_change_item(state, {10, {{200, 1, 10, false}}});
    ASSERT_NE(find_change_item(state, 10), nullptr);
    EXPECT_EQ(find_change_item(state, 10)->pItemUnit[0].wToItemIdx, 200u);
}

TEST(ChangeItemRegistration, MultiLookupRoundTrip) {
    ChangeItemManagerState state;
    EXPECT_FALSE(register_multi_change_item(state, {}));
    register_multi_change_item(state, {20, 30, {}, 0});
    ASSERT_NE(find_multi_change_item(state, 20), nullptr);
    EXPECT_EQ(find_multi_change_item(state, 20)->wLimitLevel, 30u);
}

TEST(ChangeItemSelection, UsesZeroBasedHalfOpenIntervals) {
    ChangeItemSet set{1, {{10, 1, 3, false}, {20, 1, 2, false}}};
    EXPECT_EQ(select_change_item_unit(set, 0)->wToItemIdx, 10u);
    EXPECT_EQ(select_change_item_unit(set, 2)->wToItemIdx, 10u);
    EXPECT_EQ(select_change_item_unit(set, 3)->wToItemIdx, 20u);
    EXPECT_EQ(select_change_item_unit(set, 4)->wToItemIdx, 20u);
    EXPECT_EQ(select_change_item_unit(set, 5), nullptr);
}

TEST(ChangeItemSelection, ZeroWeightEntryIsSkipped) {
    ChangeItemSet set{1, {{10, 1, 0, false}, {20, 1, 2, false}}};
    EXPECT_EQ(select_change_item_unit(set, 0)->wToItemIdx, 20u);
}

TEST(ChangeItemRandom, DefaultUsesModulo30001) {
    EXPECT_EQ(make_default_change_random(30000), 30000u);
    EXPECT_EQ(make_default_change_random(30001), 0u);
    EXPECT_EQ(make_default_change_random(60003), 1u);
}

TEST(ChangeItemRandom, HongKongCombinesTwoModulo1000Values) {
    EXPECT_EQ(make_hk_change_random(123, 456), 123456u);
    EXPECT_EQ(make_hk_change_random(1123, 2456), 123456u);
    EXPECT_EQ(make_hk_change_random(999, 999), 999999u);
}

TEST(ChangeItemMaxSpace, PicksFirstStrictMaximumDuration) {
    ChangeItemSet set{1, {{10, 5, 1, false}, {20, 9, 1, false}, {30, 9, 1, false}}};
    const auto selection = get_max_space_item_ref(set);
    ASSERT_NE(selection.unit, nullptr);
    EXPECT_EQ(selection.unit->wToItemIdx, 20u);
}

TEST(ChangeItemMaxSpace, TracksLargestNonStackableDurationSeparately) {
    ChangeItemSet set{1, {{10, 100, 1, true}, {20, 30, 1, false}, {30, 40, 1, false}}};
    const auto selection = get_max_space_item_ref(set);
    EXPECT_EQ(selection.unit->wToItemIdx, 10u);
    EXPECT_EQ(selection.maxNonStackableDuration, 40u);
}

TEST(ChangeItemSlots, MoneyAndAbilityRequireNoSlots) {
    EXPECT_EQ(required_slots_for_reward({7999, 999, 1, false}), 0u);
    EXPECT_EQ(required_slots_for_reward({7997, 999, 1, false}), 0u);
}

TEST(ChangeItemSlots, EventIdsFollowOrdinaryItemPath) {
    EXPECT_EQ(required_slots_for_reward({7998, 1, 1, false}), 1u);
    EXPECT_EQ(required_slots_for_reward({7996, 2, 1, false}), 2u);
}

TEST(ChangeItemSlots, StackableItemsUseCeilingDivisionByTwenty) {
    EXPECT_EQ(required_slots_for_reward({10, 0, 1, true}), 0u);
    EXPECT_EQ(required_slots_for_reward({10, 1, 1, true}), 1u);
    EXPECT_EQ(required_slots_for_reward({10, 20, 1, true}), 1u);
    EXPECT_EQ(required_slots_for_reward({10, 21, 1, true}), 2u);
}

TEST(ChangeItemSlots, NonStackableItemsUseDurationAsCount) {
    EXPECT_EQ(required_slots_for_reward({10, 7, 1, false}), 7u);
}

TEST(ChangeItemMultiSpace, SingleResultSetAddsOneConservativeSlot) {
    MultiChangeItem multi{1, 0, {{1, {{10, 99, 1, false}}}}, 0};
    EXPECT_EQ(changed_total_item_num_legacy(multi), 1u);
}

TEST(ChangeItemMultiSpace, MultiResultUsesWorstDuration) {
    MultiChangeItem multi{1, 0, {{1, {{10, 3, 1, false}, {20, 7, 1, false}}}}, 0};
    EXPECT_EQ(changed_total_item_num_legacy(multi), 7u);
}

TEST(ChangeItemMultiSpace, StackableWorstCaseAccountsForNonStackableAlternative) {
    MultiChangeItem multi{1, 0, {{1, {{10, 100, 1, true}, {20, 8, 1, false}}}}, 0};
    EXPECT_EQ(changed_total_item_num_legacy(multi), 8u);
}

TEST(ChangeItemMultiSpace, PreservesStalePointerAfterSingletonSetBug) {
    MultiChangeItem multi{1, 0, {
        {1, {{10, 4, 1, false}, {20, 2, 1, false}}},
        {2, {{30, 1, 1, false}}}
    }, 0};
    EXPECT_EQ(changed_total_item_num_legacy(multi), 9u);
}

TEST(ChangeItemSpaceGate, ConsumedSourceItemContributesOneSlot) {
    EXPECT_TRUE(has_change_item_space(0, 1));
    EXPECT_TRUE(has_change_item_space(2, 3));
    EXPECT_FALSE(has_change_item_space(2, 4));
}
