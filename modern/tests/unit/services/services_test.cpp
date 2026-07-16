// services_test.cpp - Phase 13 service interface mock + test
//
// Covers the 3 service interfaces in modern/include/mxh/services/:
//   - IInventoryService   (inventory slots, weared items, queries)
//   - ISkillService       (learned skills, levels, quick bindings)
//   - IPlayerStatsService (stats, level/exp, hp/mp)
//
// This test file provides minimal in-memory mock implementations
// of each interface, then exercises the interfaces through the
// mock to pin the contract that real (server-backed) impls
// must replicate.
//
// The real implementations (backed by ItemManager, the
// server's player state, etc.) will live in
// modern/src/services/ in later sessions; this file is the
// contract test that the real impls must satisfy.

#include "mxh/services/IInventoryService.hpp"
#include "mxh/services/ISkillService.hpp"
#include "mxh/services/IPlayerStatsService.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::services::test {

// ===========================================================================
// Mock IInventoryService 鈥?in-memory inventory of 80 slots + 10 weared slots
// ===========================================================================

class MockInventoryService final : public IInventoryService {
public:
    std::array<mxh::game::ItemBase, mxh::game::SLOT_INVENTORY_NUM> inventory{};
    std::array<mxh::game::ItemBase, mxh::game::WEARED_ITEM_MAX> weared{};
    std::uint16_t occupied_count = 0;
    std::uint16_t total_capacity = mxh::game::SLOT_INVENTORY_NUM;

    const mxh::game::ItemBase* getItem(std::uint16_t pos) const noexcept override {
        if (pos >= mxh::game::SLOT_INVENTORY_NUM) return nullptr;
        const auto& item = inventory[pos];
        return item.dwDBIdx == 0 ? nullptr : &inventory[pos];
    }

    std::uint16_t occupiedSlotCount() const noexcept override { return occupied_count; }
    std::uint16_t totalCapacity() const noexcept override { return total_capacity; }

    const mxh::game::ItemBase* getWearedItem(std::uint8_t slot) const noexcept override {
        if (slot >= mxh::game::WEARED_ITEM_MAX) return nullptr;
        const auto& item = weared[slot];
        return item.dwDBIdx == 0 ? nullptr : &weared[slot];
    }

    bool isWearedSlotOccupied(std::uint8_t slot) const noexcept override {
        if (slot >= mxh::game::WEARED_ITEM_MAX) return false;
        return weared[slot].dwDBIdx != 0;
    }

    std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t wIconIdx) const noexcept override {
        for (std::uint16_t pos = 0; pos < mxh::game::SLOT_INVENTORY_NUM; ++pos) {
            if (inventory[pos].dwDBIdx != 0 && inventory[pos].wIconIdx == wIconIdx) {
                return pos;
            }
        }
        return std::nullopt;
    }

    bool hasItem(std::uint16_t wIconIdx) const noexcept override {
        return findItemByIconIdx(wIconIdx).has_value();
    }
};

// ===========================================================================
// Mock ISkillService 鈥?in-memory learned-skill table
// ===========================================================================

struct MockSkillEntry {
    std::uint32_t idx;
    std::uint8_t level;
    std::optional<std::uint8_t> quick_slot;
};

class MockSkillService final : public ISkillService {
public:
    std::vector<MockSkillEntry> learned;

    std::uint32_t learnedSkillCount() const noexcept override {
        return static_cast<std::uint32_t>(learned.size());
    }
    std::uint32_t getLearnedSkillAt(std::uint32_t i) const noexcept override {
        return i < learned.size() ? learned[i].idx : 0;
    }
    bool isLearned(std::uint32_t skillIdx) const noexcept override {
        for (const auto& e : learned) if (e.idx == skillIdx) return true;
        return false;
    }
    std::optional<std::uint8_t> getSkillLevel(std::uint32_t skillIdx) const noexcept override {
        for (const auto& e : learned) if (e.idx == skillIdx) return e.level;
        return std::nullopt;
    }
    std::optional<std::uint8_t> getQuickSlotBinding(std::uint32_t skillIdx) const noexcept override {
        for (const auto& e : learned) if (e.idx == skillIdx) return e.quick_slot;
        return std::nullopt;
    }
};

// ===========================================================================
// Mock IPlayerStatsService 鈥?set all values directly
// ===========================================================================

class MockPlayerStatsService final : public IPlayerStatsService {
public:
    std::uint16_t str = 0, agi = 0, intl = 0, wis = 0, dex = 0;
    std::uint16_t level = 1;
    std::uint32_t level_exp = 0;
    std::uint32_t exp_for_next = 100;
    std::uint32_t current_hp = 100, max_hp = 100;
    std::uint32_t current_mp = 50, max_mp = 50;

    std::uint16_t getStr()  const noexcept override { return str; }
    std::uint16_t getAgi()  const noexcept override { return agi; }
    std::uint16_t getInt()  const noexcept override { return intl; }
    std::uint16_t getWis()  const noexcept override { return wis; }
    std::uint16_t getDex()  const noexcept override { return dex; }
    std::uint16_t getLevel() const noexcept override { return level; }
    std::uint32_t getLevelExp() const noexcept override { return level_exp; }
    std::uint32_t getExpForNextLevel() const noexcept override { return exp_for_next; }
    std::uint32_t getCurrentHp() const noexcept override { return current_hp; }
    std::uint32_t getMaxHp() const noexcept override { return max_hp; }
    std::uint32_t getCurrentMp() const noexcept override { return current_mp; }
    std::uint32_t getMaxMp() const noexcept override { return max_mp; }
    float getHpFraction() const noexcept override {
        return max_hp == 0 ? 0.0f
            : static_cast<float>(current_hp) / static_cast<float>(max_hp);
    }
    float getMpFraction() const noexcept override {
        return max_mp == 0 ? 0.0f
            : static_cast<float>(current_mp) / static_cast<float>(max_mp);
    }
};

// ===========================================================================
// IInventoryService tests
// ===========================================================================

TEST(IInventoryServiceTest, EmptySlotReturnsNull) {
    MockInventoryService inv;
    EXPECT_EQ(inv.getItem(0), nullptr);
    EXPECT_EQ(inv.occupiedSlotCount(), 0u);
    EXPECT_EQ(inv.totalCapacity(), mxh::game::SLOT_INVENTORY_NUM);
}

TEST(IInventoryServiceTest, OccupiedSlotReturnsItem) {
    MockInventoryService inv;
    inv.inventory[5].dwDBIdx = 42;
    inv.inventory[5].wIconIdx = 1001;
    inv.occupied_count = 1;
    const auto* item = inv.getItem(5);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->dwDBIdx, 42u);
    EXPECT_EQ(item->wIconIdx, static_cast<std::uint16_t>(1001));
}

TEST(IInventoryServiceTest, OutOfRangePositionReturnsNull) {
    MockInventoryService inv;
    EXPECT_EQ(inv.getItem(mxh::game::SLOT_INVENTORY_NUM), nullptr);   // == 80, OOB
    EXPECT_EQ(inv.getItem(1000), nullptr);
}

TEST(IInventoryServiceTest, WearedSlotRoundTrip) {
    MockInventoryService inv;
    inv.weared[mxh::game::WEARED_HAT].dwDBIdx = 7;
    inv.weared[mxh::game::WEARED_HAT].wIconIdx = 2002;
    EXPECT_TRUE(inv.isWearedSlotOccupied(mxh::game::WEARED_HAT));
    const auto* hat = inv.getWearedItem(mxh::game::WEARED_HAT);
    ASSERT_NE(hat, nullptr);
    EXPECT_EQ(hat->wIconIdx, static_cast<std::uint16_t>(2002));
    EXPECT_FALSE(inv.isWearedSlotOccupied(mxh::game::WEARED_WEAPON));
}

TEST(IInventoryServiceTest, FindItemByIconIdx) {
    MockInventoryService inv;
    inv.inventory[10].dwDBIdx = 1; inv.inventory[10].wIconIdx = 555;
    inv.inventory[20].dwDBIdx = 2; inv.inventory[20].wIconIdx = 666;
    inv.inventory[30].dwDBIdx = 3; inv.inventory[30].wIconIdx = 555;  // duplicate
    inv.occupied_count = 3;
    // First match (lowest position) wins.
    auto pos = inv.findItemByIconIdx(555);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, 10u);
    EXPECT_TRUE(inv.hasItem(555));
    EXPECT_FALSE(inv.hasItem(999));
}

// ===========================================================================
// ISkillService tests
// ===========================================================================

TEST(ISkillServiceTest, EmptyServiceHasNoSkills) {
    MockSkillService skills;
    EXPECT_EQ(skills.learnedSkillCount(), 0u);
    EXPECT_FALSE(skills.isLearned(1));
    EXPECT_FALSE(skills.getSkillLevel(1).has_value());
    EXPECT_FALSE(skills.getQuickSlotBinding(1).has_value());
}

TEST(ISkillServiceTest, LearnedSkillsEnumeration) {
    MockSkillService skills;
    skills.learned.push_back({1, 5, 0});
    skills.learned.push_back({2, 3, std::nullopt});
    skills.learned.push_back({3, 12, 9});
    EXPECT_EQ(skills.learnedSkillCount(), 3u);
    EXPECT_EQ(skills.getLearnedSkillAt(0), 1u);
    EXPECT_EQ(skills.getLearnedSkillAt(1), 2u);
    EXPECT_EQ(skills.getLearnedSkillAt(2), 3u);
    EXPECT_TRUE(skills.isLearned(2));
    EXPECT_FALSE(skills.isLearned(99));
    auto lvl = skills.getSkillLevel(2);
    ASSERT_TRUE(lvl.has_value());
    EXPECT_EQ(*lvl, 3u);
    auto qslot = skills.getQuickSlotBinding(3);
    ASSERT_TRUE(qslot.has_value());
    EXPECT_EQ(*qslot, 9u);
    // Skill 2 has no quick-slot binding.
    EXPECT_FALSE(skills.getQuickSlotBinding(2).has_value());
}

// ===========================================================================
// IPlayerStatsService tests
// ===========================================================================

TEST(IPlayerStatsServiceTest, DefaultsAreZero) {
    MockPlayerStatsService stats;
    EXPECT_EQ(stats.getStr(), 0u);
    EXPECT_EQ(stats.getLevel(), 1u);
    // Mock default: current_hp == max_hp == 100, so fraction is 1.0
    // (full health). This is the "fresh spawn" state, not 0.
    EXPECT_FLOAT_EQ(stats.getHpFraction(), 1.0f);
    EXPECT_FLOAT_EQ(stats.getMpFraction(), 1.0f);
}

TEST(IPlayerStatsServiceTest, FractionHandlesMaxZero) {
    MockPlayerStatsService stats;
    stats.max_hp = 0;  // pathological case (player not yet loaded)
    stats.max_mp = 0;
    EXPECT_FLOAT_EQ(stats.getHpFraction(), 0.0f);  // no div-by-zero
    EXPECT_FLOAT_EQ(stats.getMpFraction(), 0.0f);
}

TEST(IPlayerStatsServiceTest, AllFieldsRoundTrip) {
    MockPlayerStatsService stats;
    stats.str = 10; stats.agi = 20; stats.intl = 30; stats.wis = 40; stats.dex = 50;
    stats.level = 25;
    stats.level_exp = 12345;
    stats.exp_for_next = 50000;
    stats.current_hp = 750; stats.max_hp = 1000;
    stats.current_mp = 200; stats.max_mp = 400;
    EXPECT_EQ(stats.getStr(),  10u); EXPECT_EQ(stats.getAgi(),  20u);
    EXPECT_EQ(stats.getInt(),  30u); EXPECT_EQ(stats.getWis(),  40u);
    EXPECT_EQ(stats.getDex(),  50u);
    EXPECT_EQ(stats.getLevel(), 25u);
    EXPECT_EQ(stats.getLevelExp(), 12345u);
    EXPECT_EQ(stats.getExpForNextLevel(), 50000u);
    EXPECT_EQ(stats.getCurrentHp(), 750u); EXPECT_EQ(stats.getMaxHp(), 1000u);
    EXPECT_EQ(stats.getCurrentMp(), 200u); EXPECT_EQ(stats.getMaxMp(), 400u);
    EXPECT_FLOAT_EQ(stats.getHpFraction(), 0.75f);
    EXPECT_FLOAT_EQ(stats.getMpFraction(), 0.50f);
}

// ===========================================================================
// Cross-service: dialog-shaped usage scenario (e.g. CharacterDialog
// refreshing from three services at once).
// ===========================================================================

TEST(ServiceCompositionTest, CharacterDialogShapedRefresh) {
    // A dialog that reads from all three services simultaneously
    // (the CharacterDialog scenario) should be able to do so
    // without coupling between the services. This pins the
    // pattern that future real implementations must support.
    MockInventoryService inv;
    MockSkillService skills;
    MockPlayerStatsService stats;
    inv.weared[mxh::game::WEARED_WEAPON].dwDBIdx = 100;
    inv.weared[mxh::game::WEARED_WEAPON].wIconIdx = 7777;
    skills.learned.push_back({1, 5, 2});
    stats.level = 30; stats.current_hp = 800; stats.max_hp = 1000;

    // The "dialog" body 鈥?this is what CharacterDialog::refresh
    // will eventually look like.
    const auto* weapon = inv.getWearedItem(mxh::game::WEARED_WEAPON);
    ASSERT_NE(weapon, nullptr);
    EXPECT_EQ(weapon->wIconIdx, static_cast<std::uint16_t>(7777));
    EXPECT_TRUE(skills.isLearned(1));
    EXPECT_EQ(skills.getSkillLevel(1).value_or(static_cast<std::uint8_t>(0)),
              static_cast<std::uint8_t>(5));
    EXPECT_FLOAT_EQ(stats.getHpFraction(), 0.8f);
}

}  // namespace mxh::services::test
