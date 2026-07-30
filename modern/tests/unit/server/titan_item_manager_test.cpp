#include "mxh/server/titan_item_manager.hpp"

#include <gtest/gtest.h>

namespace {
using namespace mxh::server;

TEST(TitanItemManagerParts, FindsPartsByLegacyIndex) {
    const std::vector<TitanPartsKind> table{{100u, 2u, 900u}, {101u, 3u, 901u}};
    const auto* entry = find_titan_parts_kind(table, 101u);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->dwPartsKind, 3u);
    EXPECT_EQ(entry->dwResultTitanIdx, 901u);
    EXPECT_EQ(find_titan_parts_kind(table, 999u), nullptr);
}

TEST(TitanItemManagerUpgrade, RejectsInsufficientMoneyWithoutMutation) {
    TitanUpgradeInventory inventory{99u, {{10u, 2u}}};
    const TitanUpgradeInfo info{1u, 2u, 100u, {{10u, 2u}}};
    EXPECT_FALSE(consume_titan_upgrade_cost(inventory, info));
    EXPECT_EQ(inventory.money, 99u);
    EXPECT_EQ(inventory.materials.front().count, 2u);
}

TEST(TitanItemManagerUpgrade, CountsSplitMaterialStacks) {
    const TitanUpgradeInventory inventory{100u, {{10u, 1u}, {10u, 2u}, {20u, 5u}}};
    const TitanUpgradeInfo info{1u, 2u, 100u, {{10u, 3u}, {20u, 5u}}};
    EXPECT_TRUE(can_afford_titan_upgrade(inventory, info));
}

TEST(TitanItemManagerUpgrade, ConsumesExactCostsAndRemovesEmptyStacks) {
    TitanUpgradeInventory inventory{500u, {{10u, 1u}, {10u, 2u}, {20u, 5u}}};
    const TitanUpgradeInfo info{1u, 2u, 120u, {{10u, 3u}, {20u, 2u}}};
    ASSERT_TRUE(consume_titan_upgrade_cost(inventory, info));
    EXPECT_EQ(inventory.money, 380u);
    ASSERT_EQ(inventory.materials.size(), 1u);
    EXPECT_EQ(inventory.materials.front().item_idx, 20u);
    EXPECT_EQ(inventory.materials.front().count, 3u);
}

TEST(TitanItemManagerUpgrade, AppliesNextTitanAndGrade) {
    TitanUpgradeInventory inventory{1000u, {{10u, 2u}}};
    TitanTotalInfo titan;
    titan.TitanKind = 100u;
    titan.TitanGrade = 1u;
    const TitanUpgradeInfo info{100u, 101u, 250u, {{10u, 2u}}};
    ASSERT_TRUE(apply_titan_upgrade(inventory, titan, info));
    EXPECT_EQ(titan.TitanKind, 101u);
    EXPECT_EQ(titan.TitanGrade, 2u);
    EXPECT_EQ(inventory.money, 750u);
}

TEST(TitanItemManagerUpgrade, RejectsMismatchedTitanWithoutCharging) {
    TitanUpgradeInventory inventory{1000u, {{10u, 2u}}};
    TitanTotalInfo titan;
    titan.TitanKind = 99u;
    const TitanUpgradeInfo info{100u, 101u, 250u, {{10u, 2u}}};
    EXPECT_FALSE(apply_titan_upgrade(inventory, titan, info));
    EXPECT_EQ(inventory.money, 1000u);
    EXPECT_EQ(titan.TitanKind, 99u);
}

TEST(TitanItemManagerBreak, PicksWithoutReplacement) {
    const TitanBreakInfo info{500u, 100u, 3u, 3u,
                              {{10u, 1u, 40u}, {20u, 2u, 30u}, {30u, 3u, 30u}}};
    const auto result = roll_titan_break(info, 100u, {1u, 1u, 1u});
    ASSERT_EQ(result.materials.size(), 3u);
    EXPECT_EQ(result.materials[0].item_idx, 10u);
    EXPECT_EQ(result.materials[1].item_idx, 20u);
    EXPECT_EQ(result.materials[2].item_idx, 30u);
}

TEST(TitanItemManagerBreak, HonorsCumulativeLegacyBoundary) {
    const TitanBreakInfo info{500u, 100u, 3u, 1u,
                              {{10u, 1u, 40u}, {20u, 2u, 30u}, {30u, 3u, 30u}}};
    const auto first = roll_titan_break(info, 100u, {40u});
    const auto second = roll_titan_break(info, 100u, {41u});
    ASSERT_EQ(first.materials.size(), 1u);
    ASSERT_EQ(second.materials.size(), 1u);
    EXPECT_EQ(first.materials.front().item_idx, 10u);
    EXPECT_EQ(second.materials.front().item_idx, 20u);
}

TEST(TitanItemManagerBreak, StopsAtConfiguredGetCount) {
    const TitanBreakInfo info{500u, 100u, 3u, 2u,
                              {{10u, 1u, 40u}, {20u, 2u, 30u}, {30u, 3u, 30u}}};
    const auto result = roll_titan_break(info, 100u, {90u, 1u, 1u});
    ASSERT_EQ(result.materials.size(), 2u);
    EXPECT_EQ(result.materials[0].item_idx, 30u);
    EXPECT_EQ(result.materials[1].item_idx, 10u);
}

TEST(TitanItemManagerBreak, EmptyOrZeroRateProducesNothing) {
    TitanBreakInfo info;
    EXPECT_TRUE(roll_titan_break(info, 100u, {1u}).materials.empty());
    info.wTotalCnt = 1u;
    info.wGetCnt = 1u;
    info.materials.push_back({10u, 1u, 100u});
    EXPECT_TRUE(roll_titan_break(info, 0u, {1u}).materials.empty());
}

}  // namespace