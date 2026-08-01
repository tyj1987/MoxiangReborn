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

TEST(TitanItemManagerMaterial, MaterialCountSumsAcrossStacks) {
    const TitanUpgradeInventory inventory{100u, {{10u, 3u}, {10u, 5u}, {20u, 2u}}};
    EXPECT_EQ(titan_material_count(inventory, 10u), 8u);
    EXPECT_EQ(titan_material_count(inventory, 20u), 2u);
    EXPECT_EQ(titan_material_count(inventory, 999u), 0u);  // missing
}

TEST(TitanItemManagerMaterial, MaterialCountEmptyInventoryIsZero) {
    const TitanUpgradeInventory inventory{};
    EXPECT_EQ(titan_material_count(inventory, 0u), 0u);
    EXPECT_EQ(titan_material_count(inventory, 1u), 0u);
}

TEST(TitanItemManagerMaterial, MaterialCountIncludesZeroIndexStacks) {
    // 1:1 with legacy: titan_material_count does exact-match on item_idx,
    // so a stack with idx=0 contributes to a query for idx=0.
    const TitanUpgradeInventory inventory{0u, {{0u, 50u}, {10u, 5u}}};
    EXPECT_EQ(titan_material_count(inventory, 0u), 50u);
    EXPECT_EQ(titan_material_count(inventory, 10u), 5u);
}

TEST(TitanItemManagerMaterial, MaterialCountSaturatesAtUint32Max) {
    // 1:1 with legacy clamping: if the total exceeds UINT32_MAX, the
    // helper saturates instead of overflowing.
    TitanUpgradeInventory inventory{0u, {{10u, 0xFFFFFFFFu}, {10u, 1u}}};
    // 0xFFFFFFFF + 1 = 0x100000000 -- should clamp to UINT32_MAX.
    EXPECT_EQ(titan_material_count(inventory, 10u), 0xFFFFFFFFu);
}

TEST(TitanItemManagerUpgrade, RejectsZeroTitanIdx) {
    // 1:1: info with dwTitanIdx=0 is rejected outright (legacy sentinel).
    TitanUpgradeInventory inventory{10000u, {{10u, 999u}}};
    const TitanUpgradeInfo info{0u, 2u, 100u, {{10u, 1u}}};
    EXPECT_FALSE(can_afford_titan_upgrade(inventory, info));
}

TEST(TitanItemManagerUpgrade, RejectsZeroNextTitanIdx) {
    const TitanUpgradeInventory inventory{10000u, {{10u, 999u}}};
    const TitanUpgradeInfo info{1u, 0u, 100u, {{10u, 1u}}};
    EXPECT_FALSE(can_afford_titan_upgrade(inventory, info));
}

TEST(TitanItemManagerUpgrade, RejectsZeroRequiredMaterial) {
    // Material with dwIndex=0 OR dwCount=0 means "missing".
    TitanUpgradeInventory inventory{10000u, {{10u, 999u}}};
    const TitanUpgradeInfo info{1u, 2u, 100u, {{0u, 5u}}};  // idx=0
    EXPECT_FALSE(can_afford_titan_upgrade(inventory, info));
    const TitanUpgradeInfo info2{1u, 2u, 100u, {{10u, 0u}}};  // count=0
    EXPECT_FALSE(can_afford_titan_upgrade(inventory, info2));
}

TEST(TitanItemManagerUpgrade, RejectsPartialMaterialMatch) {
    // Need 5 of idx 10, only have 3.
    TitanUpgradeInventory inventory{10000u, {{10u, 3u}}};
    const TitanUpgradeInfo info{1u, 2u, 100u, {{10u, 5u}}};
    EXPECT_FALSE(can_afford_titan_upgrade(inventory, info));
}

TEST(TitanItemManagerUpgrade, ApplyRejectsInsufficientFunds) {
    // apply_titan_upgrade forwards to consume; verify it does not
    // bump TitanKind or TitanGrade when can_afford returns false.
    TitanUpgradeInventory inventory{50u, {}};  // not enough money
    TitanTotalInfo titan;
    titan.TitanKind = 100u;
    titan.TitanGrade = 1u;
    const TitanUpgradeInfo info{100u, 101u, 100u, {}};
    EXPECT_FALSE(apply_titan_upgrade(inventory, titan, info));
    EXPECT_EQ(titan.TitanKind, 100u);
    EXPECT_EQ(titan.TitanGrade, 1u);
}

TEST(TitanItemManagerUpgrade, ApplyCapsTitanGradeAtMax) {
    // Once TitanGrade reaches MAX_TITANGRADE, further upgrades do not bump it.
    TitanUpgradeInventory inventory{10000u, {}};
    TitanTotalInfo titan;
    titan.TitanKind = 100u;
    titan.TitanGrade = MAX_TITANGRADE;
    const TitanUpgradeInfo info{100u, 101u, 100u, {}};
    EXPECT_TRUE(apply_titan_upgrade(inventory, titan, info));
    EXPECT_EQ(titan.TitanKind, 101u);
    EXPECT_EQ(titan.TitanGrade, MAX_TITANGRADE);  // still capped
}

TEST(TitanItemManagerBreak, TotalRateGreaterThan1kDistributesAcrossMultiplePicks) {
    // 1:1: wTotalCnt and the total_rate interact; with two materials each at
    // rate 1000 and wGetCnt=2, both will be selected and "rate subtract" works.
    const TitanBreakInfo info{500u, 100u, 2u, 2u, {{10u, 1u, 1000u}, {20u, 2u, 1000u}}};
    const auto result = roll_titan_break(info, 2000u, {999u, 1u});
    EXPECT_EQ(result.materials.size(), 2u);
    EXPECT_EQ(result.materials[0].item_idx, 10u);
    EXPECT_EQ(result.materials[1].item_idx, 20u);
}

TEST(TitanItemManagerBreak, RollsBeyondGetCountAreIgnored) {
    // Only the first wGetCnt rolls are used.
    const TitanBreakInfo info{500u, 100u, 2u, 1u, {{10u, 1u, 50u}, {20u, 2u, 50u}}};
    const auto result = roll_titan_break(info, 100u, {0u, 99u});
    ASSERT_EQ(result.materials.size(), 1u);
    EXPECT_EQ(result.materials[0].item_idx, 10u);  // roll[0]=0 lands here
}

TEST(TitanItemManagerParts, FindsPartsAtFrontAndBack) {
    const std::vector<TitanPartsKind> table{{1u, 1u, 100u}, {2u, 2u, 200u}, {3u, 3u, 300u}};
    const auto* a = find_titan_parts_kind(table, 1u);
    const auto* b = find_titan_parts_kind(table, 3u);
    ASSERT_NE(a, nullptr); ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->dwResultTitanIdx, 100u);
    EXPECT_EQ(b->dwResultTitanIdx, 300u);
}

TEST(TitanItemManagerParts, EmptyTableReturnsNull) {
    const std::vector<TitanPartsKind> table{};
    EXPECT_EQ(find_titan_parts_kind(table, 0u), nullptr);
    EXPECT_EQ(find_titan_parts_kind(table, 1u), nullptr);
}
}  // namespace