// services_real_test.cpp - Phase 13.2 real service implementation
// contract test (vs. the mock test in services_test.cpp).
//
// Covers the three real service impls under
// modern/src/services/:
//   - InventoryServiceImpl   (backed by mxh::game::ItemTotalInfo)
//   - PlayerStatsServiceImpl (backed by mxh::game::PlayerCombatStats)
//   - SkillServiceImpl       (backed by std::vector<LearnedSkill>)
//
// The point of this test file is to verify that the real impls
// behave IDENTICALLY to the mocks for the read paths the Tier 2/3
// dialogs need. The dialog code can therefore be written against
// the interface and work with both the mock (in-process dialog
// tests) and the real impl (server-backed dialog tests in a
// later phase).
//
// What's tested:
//   - InventoryServiceImpl: getItem / getWearedItem / occupied /
//     totalCapacity / findItemByIconIdx / hasItem — all backed by
//     the same wire-format ItemTotalInfo the MapHandler uses.
//   - PlayerStatsServiceImpl: HP / MP / level / fraction — backed
//     by PlayerCombatStats. Includes the div-by-zero guard for
//     max_hp == 0 (the freshly-loaded player case).
//   - SkillServiceImpl: enumeration, level lookup, quick-slot
//     binding lookup.
//   - Cross-service: a dialog-style "CharacterDialog refresh"
//     scenario that pulls from all three real services at once.

#include "mxh/services/IInventoryService.hpp"
#include "mxh/services/ISkillService.hpp"
#include "mxh/services/IPlayerStatsService.hpp"

#include "InventoryServiceImpl.hpp"
#include "PlayerStatsServiceImpl.hpp"
#include "SkillServiceImpl.hpp"

#include "mxh/game/item_types.hpp"
#include "mxh/game/skill_types.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::services::real_test {

// ===========================================================================
// InventoryServiceImpl
// ===========================================================================

TEST(RealInventoryServiceTest, EmptyItemTotalInfoIsAllEmpty) {
    mxh::game::ItemTotalInfo items{};
    InventoryServiceImpl svc(items);
    EXPECT_EQ(svc.occupiedSlotCount(), 0u);
    EXPECT_EQ(svc.totalCapacity(), mxh::game::SLOT_INVENTORY_NUM);
    for (std::uint16_t i = 0; i < mxh::game::SLOT_INVENTORY_NUM; ++i) {
        EXPECT_EQ(svc.getItem(i), nullptr);
    }
    for (std::uint8_t i = 0; i < mxh::game::WEARED_ITEM_MAX; ++i) {
        EXPECT_EQ(svc.getWearedItem(i), nullptr);
        EXPECT_FALSE(svc.isWearedSlotOccupied(i));
    }
}

TEST(RealInventoryServiceTest, OccupiedSlotReturnsPointerToBacking) {
    mxh::game::ItemTotalInfo items{};
    // Set item 5 to a real (non-empty) item.
    items.Inventory[5].dwDBIdx = 42;
    items.Inventory[5].wIconIdx = 1001;
    items.Inventory[5].Durability = 100;
    InventoryServiceImpl svc(items);
    EXPECT_EQ(svc.occupiedSlotCount(), 1u);
    const auto* p = svc.getItem(5);
    ASSERT_NE(p, nullptr);
    // The returned pointer must address the same backing
    // storage (mutations through the pointer must be visible
    // to the service via the same reference).
    EXPECT_EQ(p->dwDBIdx, 42u);
    EXPECT_EQ(p->wIconIdx, 1001u);
    EXPECT_EQ(p->Durability, 100u);
    // Address must be inside the backing ItemTotalInfo.
    const auto& expected = items.Inventory[5];
    EXPECT_EQ(p, &expected);
}

TEST(RealInventoryServiceTest, OutOfRangeReturnsNull) {
    mxh::game::ItemTotalInfo items{};
    InventoryServiceImpl svc(items);
    EXPECT_EQ(svc.getItem(mxh::game::SLOT_INVENTORY_NUM), nullptr);
    EXPECT_EQ(svc.getItem(1000), nullptr);
    EXPECT_EQ(svc.getWearedItem(mxh::game::WEARED_ITEM_MAX), nullptr);
    EXPECT_EQ(svc.getWearedItem(99), nullptr);
}

TEST(RealInventoryServiceTest, WearedSlotOccupancy) {
    mxh::game::ItemTotalInfo items{};
    items.WearedItem[mxh::game::WEARED_HAT].dwDBIdx = 7;
    items.WearedItem[mxh::game::WEARED_HAT].wIconIdx = 200;
    InventoryServiceImpl svc(items);
    EXPECT_TRUE(svc.isWearedSlotOccupied(mxh::game::WEARED_HAT));
    EXPECT_FALSE(svc.isWearedSlotOccupied(mxh::game::WEARED_WEAPON));
    const auto* hat = svc.getWearedItem(mxh::game::WEARED_HAT);
    ASSERT_NE(hat, nullptr);
    EXPECT_EQ(hat->wIconIdx, 200u);
    EXPECT_EQ(hat, &items.WearedItem[mxh::game::WEARED_HAT]);
}

TEST(RealInventoryServiceTest, FindItemByIconIdxFirstMatchWins) {
    mxh::game::ItemTotalInfo items{};
    items.Inventory[10].dwDBIdx = 1; items.Inventory[10].wIconIdx = 555;
    items.Inventory[20].dwDBIdx = 2; items.Inventory[20].wIconIdx = 666;
    items.Inventory[30].dwDBIdx = 3; items.Inventory[30].wIconIdx = 555;  // duplicate
    InventoryServiceImpl svc(items);
    auto pos = svc.findItemByIconIdx(555);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, 10u);  // lowest position wins
    EXPECT_TRUE(svc.hasItem(555));
    EXPECT_TRUE(svc.hasItem(666));
    EXPECT_FALSE(svc.hasItem(999));
}

TEST(RealInventoryServiceTest, OccupiedCountCountsAllSlots) {
    mxh::game::ItemTotalInfo items{};
    items.Inventory[0].dwDBIdx = 1;
    items.Inventory[5].dwDBIdx = 2;
    items.Inventory[79].dwDBIdx = 3;     // last inventory slot
    items.WearedItem[0].dwDBIdx = 4;     // weared counts separately
    InventoryServiceImpl svc(items);
    EXPECT_EQ(svc.occupiedSlotCount(), 3u);  // weared is not in inventory count
}

// ===========================================================================
// PlayerStatsServiceImpl
// ===========================================================================

TEST(RealPlayerStatsServiceTest, DefaultsAreSafe) {
    mxh::game::PlayerCombatStats stats{};
    PlayerStatsServiceImpl svc(stats);
    EXPECT_EQ(svc.getLevel(), 1u);
    EXPECT_EQ(svc.getCurrentHp(), 100u);
    EXPECT_EQ(svc.getMaxHp(), 100u);
    EXPECT_FLOAT_EQ(svc.getHpFraction(), 1.0f);
    EXPECT_FLOAT_EQ(svc.getMpFraction(), 1.0f);
}

TEST(RealPlayerStatsServiceTest, MaxZeroDoesNotDivide) {
    mxh::game::PlayerCombatStats stats{};
    stats.max_hp = 0;
    stats.max_mp = 0;
    PlayerStatsServiceImpl svc(stats);
    EXPECT_FLOAT_EQ(svc.getHpFraction(), 0.0f);
    EXPECT_FLOAT_EQ(svc.getMpFraction(), 0.0f);
}

TEST(RealPlayerStatsServiceTest, PartialHPReducesFraction) {
    mxh::game::PlayerCombatStats stats{};
    stats.max_hp = 1000; stats.current_hp = 750;
    PlayerStatsServiceImpl svc(stats);
    EXPECT_FLOAT_EQ(svc.getHpFraction(), 0.75f);
}

TEST(RealPlayerStatsServiceTest, LevelExpIsHardcodedBaseline) {
    // 1:1 quirk: exp_for_next_level returns 100 * level
    // (placeholder for the legacy character_exp.bin table that
    // has not been ported yet).
    mxh::game::PlayerCombatStats stats{};
    stats.level = 25;
    PlayerStatsServiceImpl svc(stats);
    EXPECT_EQ(svc.getLevel(), 25u);
    EXPECT_EQ(svc.getExpForNextLevel(), 2500u);
}

TEST(RealPlayerStatsServiceTest, CoreAttributesReturnZeroForNow) {
    // 1:1 quirk: legacy StatsCalcManager computes str/agi/int/
    // wis/dex from equipped items + skills + level points. The
    // modern port has no equivalent yet, so the service
    // returns 0 (the "freshly spawned" baseline).
    mxh::game::PlayerCombatStats stats{};
    PlayerStatsServiceImpl svc(stats);
    EXPECT_EQ(svc.getStr(), 0u);
    EXPECT_EQ(svc.getAgi(), 0u);
    EXPECT_EQ(svc.getInt(), 0u);
    EXPECT_EQ(svc.getWis(), 0u);
    EXPECT_EQ(svc.getDex(), 0u);
}

// ===========================================================================
// SkillServiceImpl
// ===========================================================================

TEST(RealSkillServiceTest, EmptyLearnedList) {
    std::vector<mxh::services::LearnedSkill> learned;
    SkillServiceImpl svc(learned);
    EXPECT_EQ(svc.learnedSkillCount(), 0u);
    EXPECT_FALSE(svc.isLearned(1));
    EXPECT_FALSE(svc.getSkillLevel(1).has_value());
}

TEST(RealSkillServiceTest, LearnedSkillEnumeration) {
    std::vector<mxh::services::LearnedSkill> learned = {
        {1, 5, std::optional<std::uint8_t>(0)},
        {2, 3, std::nullopt},
        {3, 12, std::optional<std::uint8_t>(9)},
    };
    SkillServiceImpl svc(learned);
    EXPECT_EQ(svc.learnedSkillCount(), 3u);
    EXPECT_EQ(svc.getLearnedSkillAt(0), 1u);
    EXPECT_EQ(svc.getLearnedSkillAt(1), 2u);
    EXPECT_EQ(svc.getLearnedSkillAt(2), 3u);
    EXPECT_TRUE(svc.isLearned(2));
    EXPECT_FALSE(svc.isLearned(99));
    auto lvl = svc.getSkillLevel(2);
    ASSERT_TRUE(lvl.has_value());
    EXPECT_EQ(*lvl, 3u);
    auto qslot = svc.getQuickSlotBinding(3);
    ASSERT_TRUE(qslot.has_value());
    EXPECT_EQ(*qslot, 9u);
    // Skill 2 has no binding.
    EXPECT_FALSE(svc.getQuickSlotBinding(2).has_value());
}

TEST(RealSkillServiceTest, OutOfRangeIndexReturnsZero) {
    std::vector<mxh::services::LearnedSkill> learned = {{1, 1, std::nullopt}};
    SkillServiceImpl svc(learned);
    EXPECT_EQ(svc.getLearnedSkillAt(5), 0u);  // OOB
}

// ===========================================================================
// Cross-service: dialog-shaped scenario (CharacterDialog refresh)
// ===========================================================================

TEST(RealServiceCompositionTest, CharacterDialogShapedRefresh) {
    // Set up backing state.
    mxh::game::ItemTotalInfo items{};
    items.WearedItem[mxh::game::WEARED_WEAPON].dwDBIdx = 100;
    items.WearedItem[mxh::game::WEARED_WEAPON].wIconIdx = 7777;
    items.Inventory[3].dwDBIdx = 200; items.Inventory[3].wIconIdx = 555;

    mxh::game::PlayerCombatStats stats{};
    stats.level = 30; stats.current_hp = 800; stats.max_hp = 1000;
    stats.current_mp = 250; stats.max_mp = 500;

    std::vector<mxh::services::LearnedSkill> learned = {
        {42, 5, std::optional<std::uint8_t>(2)},
    };

    // Wire up the three real services.
    InventoryServiceImpl inv_svc(items);
    PlayerStatsServiceImpl stats_svc(stats);
    SkillServiceImpl skill_svc(learned);

    // Pull the dialog-style "refresh" data out of all three.
    const auto* weapon = inv_svc.getWearedItem(mxh::game::WEARED_WEAPON);
    ASSERT_NE(weapon, nullptr);
    EXPECT_EQ(weapon->wIconIdx, 7777u);

    auto pos = inv_svc.findItemByIconIdx(555);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, 3u);
    // 1:1 quirk: occupiedSlotCount only counts inventory slots
    // (0..79), not the 10 weared slots. The dialog code
    // distinguishes them via getWearedItem(slot) directly.
    EXPECT_EQ(inv_svc.occupiedSlotCount(), 1u);

    EXPECT_TRUE(skill_svc.isLearned(42));
    EXPECT_EQ(skill_svc.getSkillLevel(42).value_or(0), 5u);
    EXPECT_TRUE(skill_svc.getQuickSlotBinding(42).has_value());

    EXPECT_EQ(stats_svc.getLevel(), 30u);
    EXPECT_FLOAT_EQ(stats_svc.getHpFraction(), 0.8f);
    EXPECT_FLOAT_EQ(stats_svc.getMpFraction(), 0.5f);
}

}  // namespace mxh::services::real_test
