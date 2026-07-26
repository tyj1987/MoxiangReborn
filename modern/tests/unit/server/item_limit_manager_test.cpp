#include <mxh/server/item_limit_manager.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemLimitInit, DefaultAndInitAreEmpty) {
    auto state = make_item_limit_manager();
    EXPECT_EQ(item_limit_record_count(state), 0u);
    load_item_limit_record(state, 100, 5);
    item_limit_manager_init(state);
    EXPECT_EQ(item_limit_record_count(state), 0u);
}

TEST(ItemLimitRelease, ClearsAllRecords) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 100, 5);
    item_limit_manager_release(state);
    EXPECT_EQ(item_limit_record_count(state), 0u);
}

TEST(ItemLimitLoad, RejectsZeroItemIndex) {
    auto state = make_item_limit_manager();
    EXPECT_FALSE(load_item_limit_record(state, 0, 5));
    EXPECT_EQ(item_limit_record_count(state), 0u);
}

TEST(ItemLimitLoad, AddsRecordWithZeroCurrentCount) {
    auto state = make_item_limit_manager();
    EXPECT_TRUE(load_item_limit_record(state, 77, 12));
    const auto* info = get_item_limit_info(state, 77);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->dwItmeIdx, 77u);
    EXPECT_EQ(info->nItemLimitCount, 12);
    EXPECT_EQ(info->nItemCurrentCount, 0);
}

TEST(ItemLimitLoad, DuplicateRecordResetsCurrentCount) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 77, 12);
    add_current_item_count(state, 77, 5);
    load_item_limit_record(state, 77, 20);
    EXPECT_EQ(item_limit_record_count(state), 1u);
    EXPECT_EQ(get_item_limit_info(state, 77)->nItemLimitCount, 20);
    EXPECT_EQ(get_item_limit_info(state, 77)->nItemCurrentCount, 0);
}

TEST(ItemLimitCheck, UnknownItemReturnsOne) {
    auto state = make_item_limit_manager();
    EXPECT_EQ(check_item_limit_info(state, 999), 1);
}

TEST(ItemLimitCheck, ReturnsRemainingCount) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    add_current_item_count(state, 1, 3);
    EXPECT_EQ(check_item_limit_info(state, 1), 7);
}

TEST(ItemLimitCheck, AtLimitReturnsZero) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    add_current_item_count(state, 1, 10);
    EXPECT_EQ(check_item_limit_info(state, 1), 0);
}

TEST(ItemLimitCheck, OverLimitReturnsZero) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    add_current_item_count(state, 1, 15);
    EXPECT_EQ(check_item_limit_info(state, 1), 0);
}

TEST(ItemLimitDbSync, UpdatesOnlyFileRegisteredItems) {
    auto state = make_item_limit_manager();
    EXPECT_FALSE(set_item_limit_info_from_db(state, 1, 10, 3));
    load_item_limit_record(state, 1, 5);
    EXPECT_TRUE(set_item_limit_info_from_db(state, 1, 10, 3));
    EXPECT_EQ(check_item_limit_info(state, 1), 7);
}

TEST(ItemLimitCount, AddIsCumulative) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    add_current_item_count(state, 1, 2);
    add_current_item_count(state, 1, 3);
    EXPECT_EQ(get_item_limit_info(state, 1)->nItemCurrentCount, 5);
}

TEST(ItemLimitCount, NegativeAddMatchesLegacyUnclampedBehavior) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    add_current_item_count(state, 1, -3);
    EXPECT_EQ(get_item_limit_info(state, 1)->nItemCurrentCount, -3);
    EXPECT_EQ(check_item_limit_info(state, 1), 13);
}

TEST(ItemLimitCount, UnknownAddIsNoOp) {
    auto state = make_item_limit_manager();
    EXPECT_FALSE(add_current_item_count(state, 1, 5));
}

TEST(ItemLimitNetworkSync, ReplacesCurrentCount) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 100);
    add_current_item_count(state, 1, 7);
    EXPECT_TRUE(sync_current_item_count(state, 1, 42));
    EXPECT_EQ(get_item_limit_info(state, 1)->nItemCurrentCount, 42);
}

TEST(ItemLimitNetworkSync, UnknownItemIsIgnored) {
    auto state = make_item_limit_manager();
    EXPECT_FALSE(sync_current_item_count(state, 1, 42));
}

TEST(ItemLimitSetLimit, UpdatesExistingRecord) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    EXPECT_TRUE(set_item_limit_count(state, 1, 25));
    EXPECT_EQ(get_item_limit_info(state, 1)->nItemLimitCount, 25);
}

TEST(ItemLimitSetLimit, UnknownItemReturnsFalse) {
    auto state = make_item_limit_manager();
    EXPECT_FALSE(set_item_limit_count(state, 1, 25));
}

TEST(ItemLimitLookup, ConstAndMutableLookupMatch) {
    auto state = make_item_limit_manager();
    load_item_limit_record(state, 1, 10);
    const auto& constState = state;
    EXPECT_EQ(get_item_limit_info(state, 1)->dwItmeIdx,
              get_item_limit_info(constState, 1)->dwItmeIdx);
    EXPECT_EQ(get_item_limit_info(state, 2), nullptr);
}
