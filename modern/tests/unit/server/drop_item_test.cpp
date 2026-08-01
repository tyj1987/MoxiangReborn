//
// 1:1 lock tests for mxh::server::DropTableRegistry (Phase D6).
//
// DropTableRegistry implements the legacy [Server]Map/ItemDrop.h
// CItemDrop: a per-(monster_kind, drop_id) weighted list of item_id
// candidates. The roll() picks one entry by rng_value % total_ratio,
// scanning the entries in insertion order until accum exceeds pick.
//
// Coverage:
//   * add() appends to the registry; size() tracks entries
//   * find() returns the first matching (monster_kind, drop_id)
//   * find() returns nullptr for missing (kind, drop_id) combos
//   * roll() returns 0 when no table matches
//   * roll() returns 0 when total ratio is 0 (prevent div-by-zero)
//   * roll() respects per-entry ratios (boundary tests)
//   * roll() handles multiple entries with mixed ratios
//   * roll() returns the last entry when rng_value >= total
//     (1:1 with legacy modulo-arithmetic fallback)
//   * roll() wraps cleanly with rng_value > total (rng % total)
//   * Each (kind, drop_id) pair is independent
//   * Multiple tables per monster_kind are looked up by drop_id
//   * const iterator is safe
//   * Empty registry returns nullptr / 0 for all queries
//   * DropTable.zero-entry (empty entries) gives total=0 -> roll 0
//

#include "mxh/server/drop_item.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::DropTable;
using mxh::server::DropItemEntry;
using mxh::server::DropTableRegistry;

DropTable make_table(std::uint32_t monster_kind, std::uint32_t drop_id,
                     std::initializer_list<std::pair<std::uint32_t, std::uint16_t>> items) {
    DropTable t{};
    t.monster_kind = monster_kind;
    t.drop_id = drop_id;
    for (const auto& [item_id, ratio] : items) {
        DropItemEntry e{};
        e.item_id = item_id;
        e.ratio = ratio;
        t.entries.push_back(e);
    }
    return t;
}

}  // namespace

TEST(DropTableRegistryTest, EmptyRegistryFindReturnsNull) {
    DropTableRegistry reg;
    EXPECT_EQ(reg.size(), 0u);
    EXPECT_EQ(reg.find(0, 0), nullptr);
    EXPECT_EQ(reg.find(1, 1), nullptr);
}

TEST(DropTableRegistryTest, EmptyRegistryRollReturnsZero) {
    DropTableRegistry reg;
    EXPECT_EQ(reg.roll(0, 0, 0), 0u);
    EXPECT_EQ(reg.roll(7, 1, 99), 0u);
}

TEST(DropTableRegistryTest, AddIncrementsSize) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}, {200, 50}}));
    EXPECT_EQ(reg.size(), 1u);
    reg.add(make_table(7, 2, {{300, 100}}));
    EXPECT_EQ(reg.size(), 2u);
}

TEST(DropTableRegistryTest, AddAllowsDuplicateTablesByValue) {
    // 1:1 quirk: legacy stores tables by value in a vector; duplicates
    // are not deduplicated. The first matching (kind, drop_id) is returned.
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}, {200, 50}}));
    reg.add(make_table(7, 1, {{999, 100}}));
    EXPECT_EQ(reg.size(), 2u);
    const auto* f = reg.find(7, 1);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->entries[0].item_id, 100u);  // first match wins
}

TEST(DropTableRegistryTest, FindReturnsNullForKindMismatch) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.find(8, 1), nullptr);
}

TEST(DropTableRegistryTest, FindReturnsNullForDropIdMismatch) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.find(7, 2), nullptr);
}

TEST(DropTableRegistryTest, FindReturnsNullForBothMismatch) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.find(8, 2), nullptr);
}

TEST(DropTableRegistryTest, FindFirstMatchWins) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}, {200, 50}}));
    reg.add(make_table(7, 1, {{999, 100}}));
    const auto* f = reg.find(7, 1);
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(f->entries.size(), 2u);
    EXPECT_EQ(f->entries[0].item_id, 100u);
    EXPECT_EQ(f->entries[1].item_id, 200u);
}

TEST(DropTableRegistryTest, RollSingleEntryWithFullRatio) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.roll(7, 1, 0),   100u);
    EXPECT_EQ(reg.roll(7, 1, 50),  100u);
    EXPECT_EQ(reg.roll(7, 1, 99),  100u);
}

TEST(DropTableRegistryTest, RollTwoEntriesWithRatioBoundary) {
    // 1:1 with legacy: total = 70 + 30 = 100. pick = rng % 100.
    // entry 0 (ratio 70) covers pick in [0, 70).
    // entry 1 (ratio 30) covers pick in [70, 100).
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 70}, {200, 30}}));
    EXPECT_EQ(reg.roll(7, 1, 0),  100u);
    EXPECT_EQ(reg.roll(7, 1, 69), 100u);
    EXPECT_EQ(reg.roll(7, 1, 70), 200u);
    EXPECT_EQ(reg.roll(7, 1, 99), 200u);
}

TEST(DropTableRegistryTest, RollUsesModuloArithmeticWhenRngExceedsTotal) {
    // 1:1 with legacy: pick = rng_value % total.
    // total = 100, so rng_value 100, 200, 300 all map to pick=0 (entry 0).
    // rng_value 199 maps to pick=99 (entry 1).
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}, {200, 50}}));
    EXPECT_EQ(reg.roll(7, 1, 100), 100u);
    EXPECT_EQ(reg.roll(7, 1, 199), 200u);
    EXPECT_EQ(reg.roll(7, 1, 200), 100u);
    EXPECT_EQ(reg.roll(7, 1, 300), 100u);
}

TEST(DropTableRegistryTest, RollTotalZeroReturnsZero) {
    // All entries have ratio=0 -> total=0 -> fallback 0.
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 0}, {200, 0}}));
    EXPECT_EQ(reg.roll(7, 1, 0), 0u);
    EXPECT_EQ(reg.roll(7, 1, 99), 0u);
}

TEST(DropTableRegistryTest, RollTotalZeroExplicitSingleEntry) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 0}}));
    EXPECT_EQ(reg.roll(7, 1, 5), 0u);
}

TEST(DropTableRegistryTest, RollUsesLastEntryAsFallback) {
    // 1:1 quirk: when rng_value % total == total - 1 (the last valid
    // pick), the loop should land on the last entry. The explicit
    // `return t->entries.back().item_id` is a safety net for pick
    // values that drift beyond the loop's u32 accumulator.
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 1}, {200, 1}, {300, 1}}));  // total=3
    EXPECT_EQ(reg.roll(7, 1, 2),  300u);  // pick=2 -> entry 2
    EXPECT_EQ(reg.roll(7, 1, 5),  300u);  // pick=5%3=2 -> entry 2
    EXPECT_EQ(reg.roll(7, 1, 8),  300u);  // pick=8%3=2 -> entry 2
}

TEST(DropTableRegistryTest, RollThreeEntriesDistribution) {
    // total = 33 + 33 + 34 = 100.
    // entry 0 covers pick in [0, 33).
    // entry 1 covers pick in [33, 66).
    // entry 2 covers pick in [66, 100).
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 33}, {200, 33}, {300, 34}}));
    EXPECT_EQ(reg.roll(7, 1, 0),   100u);
    EXPECT_EQ(reg.roll(7, 1, 32),  100u);
    EXPECT_EQ(reg.roll(7, 1, 33),  200u);
    EXPECT_EQ(reg.roll(7, 1, 65),  200u);
    EXPECT_EQ(reg.roll(7, 1, 66),  300u);
    EXPECT_EQ(reg.roll(7, 1, 99),  300u);
}

TEST(DropTableRegistryTest, RollAsymmetricRatios) {
    // total = 10 + 90 = 100.
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 10}, {200, 90}}));
    EXPECT_EQ(reg.roll(7, 1, 0),  100u);
    EXPECT_EQ(reg.roll(7, 1, 9),  100u);
    EXPECT_EQ(reg.roll(7, 1, 10), 200u);
    EXPECT_EQ(reg.roll(7, 1, 99), 200u);
}

TEST(DropTableRegistryTest, RollSingleEntryZeroRatio) {
    // Single entry with ratio=0 -> total=0 -> fallback 0.
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 0}}));
    EXPECT_EQ(reg.roll(7, 1, 0),  0u);
    EXPECT_EQ(reg.roll(7, 1, 99), 0u);
}

TEST(DropTableRegistryTest, RollIndependentForDifferentKind) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    reg.add(make_table(8, 1, {{200, 100}}));
    EXPECT_EQ(reg.roll(7, 1, 0), 100u);
    EXPECT_EQ(reg.roll(8, 1, 0), 200u);
}

TEST(DropTableRegistryTest, RollIndependentForDifferentDropId) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    reg.add(make_table(7, 2, {{200, 100}}));
    EXPECT_EQ(reg.roll(7, 1, 0), 100u);
    EXPECT_EQ(reg.roll(7, 2, 0), 200u);
}

TEST(DropTableRegistryTest, RollPreservesInsertionOrderAcrossTables) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    reg.add(make_table(7, 2, {{200, 100}}));
    reg.add(make_table(7, 3, {{300, 100}}));
    EXPECT_EQ(reg.roll(7, 1, 0), 100u);
    EXPECT_EQ(reg.roll(7, 2, 0), 200u);
    EXPECT_EQ(reg.roll(7, 3, 0), 300u);
}

TEST(DropTableRegistryTest, RollReturnsZeroForMissingKind) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.roll(99, 1, 0), 0u);
}

TEST(DropTableRegistryTest, RollReturnsZeroForMissingDropId) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.roll(7, 99, 0), 0u);
}

TEST(DropTableRegistryTest, RollReturnsZeroForMissingBoth) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 100}}));
    EXPECT_EQ(reg.roll(99, 99, 0), 0u);
}

TEST(DropTableRegistryTest, LargeRngValueModuloTotal) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}, {200, 50}}));  // total=100
    // RNG values up to 4 billion should still work via uint32_t modulo.
    const std::uint32_t big = 0xFFFFFFFFu;
    EXPECT_EQ(reg.roll(7, 1, big), 200u);  // big % 100 = 95 -> entry 2
    const std::uint32_t near = 0xFFFFFF00u;
    EXPECT_EQ(reg.roll(7, 1, near), 100u);  // near % 100 = 0 -> entry 1
}

TEST(DropTableRegistryTest, LargeRatiosRespectBoundary) {
    // 1:1 with legacy MAX_DROP_ITEM_PERCENT = 10000 (basis points).
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 5000}, {200, 5000}}));  // total=10000
    EXPECT_EQ(reg.roll(7, 1, 0),    100u);
    EXPECT_EQ(reg.roll(7, 1, 4999), 100u);
    EXPECT_EQ(reg.roll(7, 1, 5000), 200u);
    EXPECT_EQ(reg.roll(7, 1, 9999), 200u);
}

TEST(DropTableRegistryTest, FindReturnsConstPointer) {
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}, {200, 50}}));
    const auto& cref = reg;
    const auto* f = cref.find(7, 1);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->entries.size(), 2u);
}

TEST(DropTableRegistryTest, RollIsIdempotent) {
    // Same inputs -> same output (pure function, no side effects).
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 70}, {200, 30}}));
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(reg.roll(7, 1, 42), reg.roll(7, 1, 42));
    }
}

TEST(DropTableRegistryTest, DropTableDefaultsZeroEntry) {
    // 1:1 with legacy CItemDrop default-ctor: monster_kind=0, drop_id=0,
    // entries empty. Verify the struct layout.
    DropTable t{};
    EXPECT_EQ(t.monster_kind, 0u);
    EXPECT_EQ(t.drop_id, 0u);
    EXPECT_TRUE(t.entries.empty());
}

TEST(DropTableRegistryTest, DropItemEntryDefaultsAreOne) {
    // 1:1 with legacy CItemDrop::stDrop default: count_min=1, count_max=1.
    DropItemEntry e{};
    EXPECT_EQ(e.item_id, 0u);
    EXPECT_EQ(e.ratio, 0u);
    EXPECT_EQ(e.count_min, 1u);
    EXPECT_EQ(e.count_max, 1u);
}

TEST(DropTableRegistryTest, MaxDropPerMonsterConstantIsLegacyTen) {
    // 1:1 with legacy CItemDrop::MAX_DROP_PER_MONSTER = 10.
    EXPECT_EQ(mxh::server::MAX_DROP_PER_MONSTER, 10);
}

TEST(DropTableRegistryTest, AddDoesNotDeduplicateAcrossKind) {
    // Same drop_id with different monster_kind should not be touched.
    DropTableRegistry reg;
    reg.add(make_table(7, 1, {{100, 50}}));
    reg.add(make_table(8, 1, {{200, 50}}));
    EXPECT_EQ(reg.size(), 2u);
    EXPECT_EQ(reg.find(7, 1)->entries[0].item_id, 100u);
    EXPECT_EQ(reg.find(8, 1)->entries[0].item_id, 200u);
}
