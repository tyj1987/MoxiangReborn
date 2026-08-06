// hero_total_layout_test.cpp
//
// 1:1 lock tests for the wire layout offsets declared in
// mxh/game/hero_total_layout.hpp. These offsets define the byte positions
// inside SEND_HERO_TOTALINFO where each sub-record begins; any silent
// drift here breaks the entire server->client full-state serialization.
//
// 1:1 invariants (locked):
//   - HERO_TOTAL_BASE_OBJECT_OFFSET = 0 (the MSGBASE header is stripped).
//   - HERO_TOTAL_CHARACTER_OFFSET = 35 (1 base + 34 base-info bytes).
//   - HERO_TOTAL_HERO_OFFSET = 147 (character-record end + alignment).
//   - HERO_TOTAL_MOVE_OFFSET = 207.
//   - HERO_TOTAL_UNIQUE_AGENT_OFFSET = 221.
//   - HERO_TOTAL_SHOP_OPTION_OFFSET = 225.
//   - HERO_TOTAL_MUGONG_OFFSET = 345.
//   - HERO_TOTAL_ABILITY_OFFSET = 695.
//   - HERO_TOTAL_ITEM_OFFSET = 1019.
//   - HERO_TOTAL_OPTION_COUNTS_OFFSET = 3747 (= HERO_TOTAL_ITEM_OFFSET + 2728).
//   - HERO_TOTAL_SERVER_TIME_OFFSET = 3757 (= option_counts + 10).
//   - HERO_TOTAL_ADDABLE_INFO_OFFSET = 3773 (= server_time + 16).
//   - HERO_TOTAL_EMPTY_PAYLOAD_SIZE = 3775 (= addable_info + 2).
//
// All offsets must be monotonically non-decreasing.

#pragma once

#include "mxh/game/hero_total_layout.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>

namespace {

using mxh::game::HERO_TOTAL_BASE_OBJECT_OFFSET;
using mxh::game::HERO_TOTAL_CHARACTER_OFFSET;
using mxh::game::HERO_TOTAL_HERO_OFFSET;
using mxh::game::HERO_TOTAL_MOVE_OFFSET;
using mxh::game::HERO_TOTAL_UNIQUE_AGENT_OFFSET;
using mxh::game::HERO_TOTAL_SHOP_OPTION_OFFSET;
using mxh::game::HERO_TOTAL_MUGONG_OFFSET;
using mxh::game::HERO_TOTAL_ABILITY_OFFSET;
using mxh::game::HERO_TOTAL_ITEM_OFFSET;
using mxh::game::HERO_TOTAL_OPTION_COUNTS_OFFSET;
using mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET;
using mxh::game::HERO_TOTAL_ADDABLE_INFO_OFFSET;
using mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE;

}  // namespace


// ===========================================================================
// Per-offset value verification (1:1 with legacy SEND_HERO_TOTALINFO layout)
// ===========================================================================

TEST(HeroTotalLayout, BaseObjectOffsetIsZero) {
    EXPECT_EQ(HERO_TOTAL_BASE_OBJECT_OFFSET, 0u);
}

TEST(HeroTotalLayout, CharacterOffsetIs35) {
    EXPECT_EQ(HERO_TOTAL_CHARACTER_OFFSET, 35u);
}

TEST(HeroTotalLayout, HeroOffsetIs147) {
    EXPECT_EQ(HERO_TOTAL_HERO_OFFSET, 147u);
}

TEST(HeroTotalLayout, MoveOffsetIs207) {
    EXPECT_EQ(HERO_TOTAL_MOVE_OFFSET, 207u);
}

TEST(HeroTotalLayout, UniqueAgentOffsetIs221) {
    EXPECT_EQ(HERO_TOTAL_UNIQUE_AGENT_OFFSET, 221u);
}

TEST(HeroTotalLayout, ShopOptionOffsetIs225) {
    EXPECT_EQ(HERO_TOTAL_SHOP_OPTION_OFFSET, 225u);
}

TEST(HeroTotalLayout, MugongOffsetIs345) {
    EXPECT_EQ(HERO_TOTAL_MUGONG_OFFSET, 345u);
}

TEST(HeroTotalLayout, AbilityOffsetIs695) {
    EXPECT_EQ(HERO_TOTAL_ABILITY_OFFSET, 695u);
}

TEST(HeroTotalLayout, ItemOffsetIs1019) {
    EXPECT_EQ(HERO_TOTAL_ITEM_OFFSET, 1019u);
}

TEST(HeroTotalLayout, OptionCountsOffsetIs3747) {
    EXPECT_EQ(HERO_TOTAL_OPTION_COUNTS_OFFSET, 3747u);
}

TEST(HeroTotalLayout, ServerTimeOffsetIs3757) {
    EXPECT_EQ(HERO_TOTAL_SERVER_TIME_OFFSET, 3757u);
}

TEST(HeroTotalLayout, AddableInfoOffsetIs3773) {
    EXPECT_EQ(HERO_TOTAL_ADDABLE_INFO_OFFSET, 3773u);
}

TEST(HeroTotalLayout, EmptyPayloadSizeIs3775) {
    EXPECT_EQ(HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 3775u);
}


// ===========================================================================
// Inter-offset relations (1:1 invariant formulas from header)
// ===========================================================================

TEST(HeroTotalLayout, CharacterHerosAfterBaseObject) {
    EXPECT_GT(HERO_TOTAL_CHARACTER_OFFSET, HERO_TOTAL_BASE_OBJECT_OFFSET);
    EXPECT_EQ(HERO_TOTAL_CHARACTER_OFFSET - HERO_TOTAL_BASE_OBJECT_OFFSET, 35u);
}

TEST(HeroTotalLayout, HeroAfterCharacter) {
    EXPECT_GT(HERO_TOTAL_HERO_OFFSET, HERO_TOTAL_CHARACTER_OFFSET);
    EXPECT_EQ(HERO_TOTAL_HERO_OFFSET - HERO_TOTAL_CHARACTER_OFFSET, 112u);
}

TEST(HeroTotalLayout, MoveAfterHero) {
    EXPECT_GT(HERO_TOTAL_MOVE_OFFSET, HERO_TOTAL_HERO_OFFSET);
    EXPECT_EQ(HERO_TOTAL_MOVE_OFFSET - HERO_TOTAL_HERO_OFFSET, 60u);
}

TEST(HeroTotalLayout, UniqueAgentAfterMove) {
    EXPECT_GT(HERO_TOTAL_UNIQUE_AGENT_OFFSET, HERO_TOTAL_MOVE_OFFSET);
    EXPECT_EQ(HERO_TOTAL_UNIQUE_AGENT_OFFSET - HERO_TOTAL_MOVE_OFFSET, 14u);
}

TEST(HeroTotalLayout, ShopOptionAfterUniqueAgent) {
    EXPECT_GT(HERO_TOTAL_SHOP_OPTION_OFFSET, HERO_TOTAL_UNIQUE_AGENT_OFFSET);
    EXPECT_EQ(HERO_TOTAL_SHOP_OPTION_OFFSET - HERO_TOTAL_UNIQUE_AGENT_OFFSET, 4u);
}

TEST(HeroTotalLayout, MugongAfterShopOption) {
    EXPECT_GT(HERO_TOTAL_MUGONG_OFFSET, HERO_TOTAL_SHOP_OPTION_OFFSET);
    EXPECT_EQ(HERO_TOTAL_MUGONG_OFFSET - HERO_TOTAL_SHOP_OPTION_OFFSET, 120u);
}

TEST(HeroTotalLayout, AbilityAfterMugong) {
    EXPECT_GT(HERO_TOTAL_ABILITY_OFFSET, HERO_TOTAL_MUGONG_OFFSET);
    EXPECT_EQ(HERO_TOTAL_ABILITY_OFFSET - HERO_TOTAL_MUGONG_OFFSET, 350u);
}

TEST(HeroTotalLayout, ItemAfterAbility) {
    EXPECT_GT(HERO_TOTAL_ITEM_OFFSET, HERO_TOTAL_ABILITY_OFFSET);
    EXPECT_EQ(HERO_TOTAL_ITEM_OFFSET - HERO_TOTAL_ABILITY_OFFSET, 324u);
}

TEST(HeroTotalLayout, OptionCountsAfterItemPlus2728) {
    // 2728 = sizeof(ItemTotalInfo) (legacy wire size).
    EXPECT_EQ(HERO_TOTAL_OPTION_COUNTS_OFFSET,
              HERO_TOTAL_ITEM_OFFSET + 2728u);
}

TEST(HeroTotalLayout, ServerTimeAfterOptionCountsPlus10) {
    EXPECT_EQ(HERO_TOTAL_SERVER_TIME_OFFSET,
              HERO_TOTAL_OPTION_COUNTS_OFFSET + 10u);
}

TEST(HeroTotalLayout, AddableInfoAfterServerTimePlus16) {
    EXPECT_EQ(HERO_TOTAL_ADDABLE_INFO_OFFSET,
              HERO_TOTAL_SERVER_TIME_OFFSET + 16u);
}

TEST(HeroTotalLayout, EmptyPayloadSizeAfterAddableInfoPlus2) {
    EXPECT_EQ(HERO_TOTAL_EMPTY_PAYLOAD_SIZE,
              HERO_TOTAL_ADDABLE_INFO_OFFSET + 2u);
}


// ===========================================================================
// Monotonic ordering invariant (offsets must be non-decreasing; otherwise
// the wire reader would corrupt the next sub-record).
// ===========================================================================

TEST(HeroTotalLayout, AllOffsetsAreNonDecreasing) {
    constexpr std::array<std::size_t, 13> offsets = {
        HERO_TOTAL_BASE_OBJECT_OFFSET,
        HERO_TOTAL_CHARACTER_OFFSET,
        HERO_TOTAL_HERO_OFFSET,
        HERO_TOTAL_MOVE_OFFSET,
        HERO_TOTAL_UNIQUE_AGENT_OFFSET,
        HERO_TOTAL_SHOP_OPTION_OFFSET,
        HERO_TOTAL_MUGONG_OFFSET,
        HERO_TOTAL_ABILITY_OFFSET,
        HERO_TOTAL_ITEM_OFFSET,
        HERO_TOTAL_OPTION_COUNTS_OFFSET,
        HERO_TOTAL_SERVER_TIME_OFFSET,
        HERO_TOTAL_ADDABLE_INFO_OFFSET,
        HERO_TOTAL_EMPTY_PAYLOAD_SIZE,
    };
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        EXPECT_GE(offsets[i], offsets[i - 1])
            << "HeroTotal offset " << i << " (" << offsets[i]
            << ") < previous (" << offsets[i - 1] << ")";
    }
}

TEST(HeroTotalLayout, AllOffsetsArePositive) {
    EXPECT_GT(HERO_TOTAL_CHARACTER_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_HERO_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_MOVE_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_UNIQUE_AGENT_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_SHOP_OPTION_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_MUGONG_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_ABILITY_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_ITEM_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_OPTION_COUNTS_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_SERVER_TIME_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_ADDABLE_INFO_OFFSET, 0u);
    EXPECT_GT(HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0u);
}

// ===========================================================================
// Static-assertion mirror: the header's static_assert lines should also
// hold as runtime checks so any future regression is caught even if a
// contributor disables static_asserts locally.
// ===========================================================================

TEST(HeroTotalLayout, HeaderStaticAssertsHoldAtRuntime) {
    static_assert(HERO_TOTAL_OPTION_COUNTS_OFFSET == 3747,
                  "Header invariant drifted from legacy 3747");
    static_assert(HERO_TOTAL_SERVER_TIME_OFFSET == 3757,
                  "Header invariant drifted from legacy 3757");
    static_assert(HERO_TOTAL_ADDABLE_INFO_OFFSET == 3773,
                  "Header invariant drifted from legacy 3773");
    static_assert(HERO_TOTAL_EMPTY_PAYLOAD_SIZE == 3775,
                  "Header invariant drifted from legacy 3775");
    SUCCEED();
}

// ===========================================================================
// Cross-validation: 13 offsets + 12 gaps = 12 gaps must sum to the
// total wire payload (the offset-deltas must add up to EMPTY_PAYLOAD).
// ===========================================================================

TEST(HeroTotalLayout, SumOfGapsEqualsEmptyPayload) {
    const std::array<std::size_t, 13> offsets = {
        HERO_TOTAL_BASE_OBJECT_OFFSET,
        HERO_TOTAL_CHARACTER_OFFSET,
        HERO_TOTAL_HERO_OFFSET,
        HERO_TOTAL_MOVE_OFFSET,
        HERO_TOTAL_UNIQUE_AGENT_OFFSET,
        HERO_TOTAL_SHOP_OPTION_OFFSET,
        HERO_TOTAL_MUGONG_OFFSET,
        HERO_TOTAL_ABILITY_OFFSET,
        HERO_TOTAL_ITEM_OFFSET,
        HERO_TOTAL_OPTION_COUNTS_OFFSET,
        HERO_TOTAL_SERVER_TIME_OFFSET,
        HERO_TOTAL_ADDABLE_INFO_OFFSET,
        HERO_TOTAL_EMPTY_PAYLOAD_SIZE,
    };
    std::size_t total = 0;
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        total += offsets[i] - offsets[i - 1];
    }
    EXPECT_EQ(total, HERO_TOTAL_EMPTY_PAYLOAD_SIZE);
}

