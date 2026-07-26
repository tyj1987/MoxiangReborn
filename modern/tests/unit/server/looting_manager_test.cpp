#include <mxh/server/looting_manager.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(LootingConstants, MatchLegacy) {
    EXPECT_EQ(PKLOOTING_ITEM_NUM, 20u);
    EXPECT_EQ(PKLOOTING_LIMIT_TIME, 10000u);
    EXPECT_EQ(PKLOOTING_DLG_DELAY_TIME, 2000u);
    EXPECT_FLOAT_EQ(PK_LOOTING_DISTANCE, 1000.0f);
}

TEST(LootingChance, LocksAllBoundaries) {
    EXPECT_EQ(get_looting_chance(0), 3);
    EXPECT_EQ(get_looting_chance(99999), 3);
    EXPECT_EQ(get_looting_chance(100000), 4);
    EXPECT_EQ(get_looting_chance(500000), 5);
    EXPECT_EQ(get_looting_chance(1000000), 6);
    EXPECT_EQ(get_looting_chance(5000000), 7);
    EXPECT_EQ(get_looting_chance(10000000), 8);
    EXPECT_EQ(get_looting_chance(50000000), 9);
    EXPECT_EQ(get_looting_chance(100000000), 10);
}

TEST(LootingItemNum, LocksAllBoundaries) {
    EXPECT_EQ(get_looting_item_num(49), 0);
    EXPECT_EQ(get_looting_item_num(50), 1);
    EXPECT_EQ(get_looting_item_num(100000000), 2);
    EXPECT_EQ(get_looting_item_num(400000000), 3);
    EXPECT_EQ(get_looting_item_num(700000000), 4);
    EXPECT_EQ(get_looting_item_num(1000000000), 5);
}

TEST(LootingWearRatio, LocksAllBoundaries) {
    EXPECT_EQ(get_wear_item_looting_ratio(0), 0);
    EXPECT_EQ(get_wear_item_looting_ratio(1), 1);
    EXPECT_EQ(get_wear_item_looting_ratio(50), 10);
    EXPECT_EQ(get_wear_item_looting_ratio(4000), 20);
    EXPECT_EQ(get_wear_item_looting_ratio(20000), 30);
    EXPECT_EQ(get_wear_item_looting_ratio(80000), 40);
    EXPECT_EQ(get_wear_item_looting_ratio(400000), 50);
    EXPECT_EQ(get_wear_item_looting_ratio(1600000), 60);
    EXPECT_EQ(get_wear_item_looting_ratio(8000000), 70);
    EXPECT_EQ(get_wear_item_looting_ratio(32000000), 85);
    EXPECT_EQ(get_wear_item_looting_ratio(100000000), 100);
}

TEST(LootingSituation, RejectsNonPlayerAndDeadAttacker) {
    EXPECT_FALSE(is_looting_situation(false, false, true, false, false));
    EXPECT_FALSE(is_looting_situation(true, true, true, false, false));
}

TEST(LootingSituation, AcceptsEachLegacyCondition) {
    EXPECT_TRUE(is_looting_situation(true, false, true, false, false));
    EXPECT_TRUE(is_looting_situation(true, false, false, true, false));
    EXPECT_TRUE(is_looting_situation(true, false, false, false, true));
    EXPECT_FALSE(is_looting_situation(true, false, false, false, false));
}

TEST(LootingRoom, InitializesFromAttackerBadFame) {
    auto room = make_looting_room(10, 20, 500000, 1234);
    EXPECT_EQ(room.m_dwDiePlayer, 10u);
    EXPECT_EQ(room.m_dwAttacker, 20u);
    EXPECT_EQ(room.m_nChance, 5);
    EXPECT_EQ(room.m_nItemLootCount, 1);
    EXPECT_EQ(room.m_dwLootingStartTime, 1234u);
}

TEST(LootingMoney, UsesLegacyThreePercentIntegerMath) {
    EXPECT_EQ(calculate_loot_money(0), 0u);
    EXPECT_EQ(calculate_loot_money(33), 0u);
    EXPECT_EQ(calculate_loot_money(100), 3u);
    EXPECT_EQ(calculate_loot_money(999), 29u);
}

TEST(LootingTimeout, UsesStrictGreaterThanFifteenSeconds) {
    auto room = make_looting_room(1, 2, 50, 1000);
    EXPECT_FALSE(is_looting_room_timeout(room, 16000));
    EXPECT_TRUE(is_looting_room_timeout(room, 16001));
}

TEST(LootingAttempt, RejectsInvalidPreconditionsWithoutConsumingChance) {
    auto room = make_looting_room(1, 2, 50, 0);
    room.m_nChance = 0;
    EXPECT_EQ(try_loot(room, 0, 0).error, LootingError::NoMoreChance);
    room.m_nChance = 3; room.m_nItemLootCount = 0;
    EXPECT_EQ(try_loot(room, 0, 0).error, LootingError::NoMoreItemLootCount);
    room.m_nItemLootCount = 1;
    EXPECT_EQ(try_loot(room, -1, 0).error, LootingError::InvalidPosition);
    EXPECT_EQ(room.m_nChance, 3);
}

TEST(LootingAttempt, EmptySlotConsumesChanceOnly) {
    auto room = make_looting_room(1, 2, 50, 0);
    auto result = try_loot(room, 0, 0);
    EXPECT_TRUE(result.consumedChance);
    EXPECT_FALSE(result.looted);
    EXPECT_EQ(room.m_nChance, 2);
    EXPECT_EQ(room.m_nItemLootCount, 1);
}

TEST(LootingAttempt, ItemLootUpdatesAllCounters) {
    auto room = make_looting_room(1, 2, 50, 0);
    set_looting_item(room, 3, LootingItemKind::Item, 77);
    auto result = try_loot(room, 3, 100);
    EXPECT_TRUE(result.looted);
    EXPECT_EQ(result.item.nKind, LootingItemKind::Item);
    EXPECT_EQ(result.item.dwData, 77u);
    EXPECT_EQ(room.m_nChance, 2);
    EXPECT_EQ(room.m_nItemLootCount, 0);
    EXPECT_EQ(room.m_nLootedItemCount, 1);
    EXPECT_EQ(room.m_LootingItemArray[3].nKind, LootingItemKind::Selected);
}

TEST(LootingAttempt, SelectedSlotCannotBeLootedAgain) {
    auto room = make_looting_room(1, 2, 50, 0);
    set_looting_item(room, 0, LootingItemKind::Selected, 0);
    EXPECT_EQ(try_loot(room, 0, 0).error, LootingError::AlreadySelected);
}

TEST(LootingAttempt, DistanceBoundaryAndForceMatchLegacy) {
    auto room = make_looting_room(1, 2, 50, 0);
    set_looting_item(room, 0, LootingItemKind::Money, 3);
    EXPECT_EQ(try_loot(room, 0, 1500.0f).error, LootingError::Ok);
    room = make_looting_room(1, 2, 50, 0);
    set_looting_item(room, 0, LootingItemKind::Money, 3);
    EXPECT_EQ(try_loot(room, 0, 1500.1f).error, LootingError::OverDistance);
    EXPECT_EQ(try_loot(room, 0, 9000.0f, true).error, LootingError::Ok);
}

TEST(LootingManager, CreateReplacesExistingVictimRoom) {
    LootingManagerState state;
    create_looting_room(state, make_looting_room(1, 2, 50, 0));
    create_looting_room(state, make_looting_room(1, 3, 50, 0));
    ASSERT_EQ(state.m_LootingRooms.size(), 1u);
    EXPECT_EQ(get_looting_room(state, 1)->m_dwAttacker, 3u);
}

TEST(LootingManager, LookupCloseAndPresence) {
    LootingManagerState state;
    create_looting_room(state, make_looting_room(1, 2, 50, 0));
    EXPECT_TRUE(is_looted_player(state, 1));
    EXPECT_NE(get_looting_room(state, 1), nullptr);
    EXPECT_TRUE(close_looting_room(state, 1));
    EXPECT_FALSE(is_looted_player(state, 1));
    EXPECT_FALSE(close_looting_room(state, 1));
}

TEST(LootingManager, CancelRemovesAllRoomsForAttacker) {
    LootingManagerState state;
    create_looting_room(state, make_looting_room(1, 9, 50, 0));
    create_looting_room(state, make_looting_room(2, 9, 50, 0));
    create_looting_room(state, make_looting_room(3, 8, 50, 0));
    EXPECT_EQ(cancel_looting_by_attacker(state, 9), 2u);
    EXPECT_EQ(state.m_LootingRooms.size(), 1u);
}

TEST(LootingManager, TimeoutSweepRemovesOnlyExpiredRooms) {
    LootingManagerState state;
    create_looting_room(state, make_looting_room(1, 2, 50, 0));
    create_looting_room(state, make_looting_room(2, 3, 50, 10000));
    EXPECT_EQ(process_looting_timeouts(state, 15001), 1u);
    EXPECT_FALSE(is_looted_player(state, 1));
    EXPECT_TRUE(is_looted_player(state, 2));
}
