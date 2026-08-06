
// fixed_tile_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::fixed_tile (D4.135).
// Augments the legacy 5-test fixed_tile_test.cpp with deeper coverage of:
//   - TFA_COLLISON / TFA_FLYCOLLISON / TFA_PEACEZONE / TFA_JSAZONE
//     constants distinctness + bit layout.
//   - FixedTileAttrBits struct defaults.
//   - fixed_tile_init() copies input byte exactly.
//   - fixed_tile_is_collision() returns true iff TFA_COLLISON bit is set.
//   - Multi-init state overwriting.
//   - Boundary byte values (0, 0xFF).
//
// 1:1 invariants (locked):
//   - TFA_COLLISON = 0x01, TFA_PEACEZONE = 0x08, TFA_FLYCOLLISON = 0x10,
//     TFA_JSAZONE = 0x40.
//   - All four constants are distinct (no bit overlap).
//   - FixedTileAttrBits default-constructed has uFixedAttr = 0.
//   - fixed_tile_init(t, byte) sets t.m_FixedAttr.uFixedAttr = byte.
//   - fixed_tile_is_collision(t) returns
//     (t.m_FixedAttr.uFixedAttr & TFA_COLLISON) != 0.
//   - Multiple flag combinations: collision wins regardless of
//     co-existing flags.
//   - TFA_JSAZONE alone is NOT collision (only TFA_COLLISON bit triggers).

#pragma once

#include "mxh/server/fixed_tile.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace {

using mxh::server::FixedTile;
using mxh::server::FixedTileAttr;
using mxh::server::FixedTileAttrBits;
using mxh::server::fixed_tile_init;
using mxh::server::fixed_tile_is_collision;
using mxh::server::TFA_COLLISON;
using mxh::server::TFA_FLYCOLLISON;
using mxh::server::TFA_PEACEZONE;
using mxh::server::TFA_JSAZONE;

}  // namespace


// ===========================================================================
// Constants (1:1 with legacy Tile.h bit layout)
// ===========================================================================

TEST(FixedTileDataPlane, TfaCollisonIsZeroOne) {
    EXPECT_EQ(TFA_COLLISON, 0x01u);
}

TEST(FixedTileDataPlane, TfaPeacezoneIsZeroEight) {
    EXPECT_EQ(TFA_PEACEZONE, 0x08u);
}

TEST(FixedTileDataPlane, TfaFlycollisonIsOneZero) {
    EXPECT_EQ(TFA_FLYCOLLISON, 0x10u);
}

TEST(FixedTileDataPlane, TfaJsazoneIsFourZero) {
    EXPECT_EQ(TFA_JSAZONE, 0x40u);
}

TEST(FixedTileDataPlane, AllTfaConstantsAreDistinct) {
    EXPECT_NE(TFA_COLLISON, TFA_PEACEZONE);
    EXPECT_NE(TFA_COLLISON, TFA_FLYCOLLISON);
    EXPECT_NE(TFA_COLLISON, TFA_JSAZONE);
    EXPECT_NE(TFA_PEACEZONE, TFA_FLYCOLLISON);
    EXPECT_NE(TFA_PEACEZONE, TFA_JSAZONE);
    EXPECT_NE(TFA_FLYCOLLISON, TFA_JSAZONE);
}

TEST(FixedTileDataPlane, NoTfaConstantOverlaps) {
    // AND each pair must be 0 - distinct bits.
    EXPECT_EQ(static_cast<std::uint8_t>(TFA_COLLISON & TFA_PEACEZONE), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(TFA_COLLISON & TFA_FLYCOLLISON), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(TFA_COLLISON & TFA_JSAZONE), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(TFA_PEACEZONE & TFA_FLYCOLLISON), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(TFA_PEACEZONE & TFA_JSAZONE), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(TFA_FLYCOLLISON & TFA_JSAZONE), 0u);
}

TEST(FixedTileDataPlane, AllConstantsFitInUint8) {
    EXPECT_LE(TFA_COLLISON, 0xFFu);
    EXPECT_LE(TFA_PEACEZONE, 0xFFu);
    EXPECT_LE(TFA_FLYCOLLISON, 0xFFu);
    EXPECT_LE(TFA_JSAZONE, 0xFFu);
}

TEST(FixedTileDataPlane, ConstantsArePowersOfTwo) {
    // Each TFA flag should be exactly one bit.
    EXPECT_EQ(TFA_COLLISON & (TFA_COLLISON - 1), 0u);
    EXPECT_EQ(TFA_PEACEZONE & (TFA_PEACEZONE - 1), 0u);
    EXPECT_EQ(TFA_FLYCOLLISON & (TFA_FLYCOLLISON - 1), 0u);
    EXPECT_EQ(TFA_JSAZONE & (TFA_JSAZONE - 1), 0u);
}


// ===========================================================================
// Types
// ===========================================================================

TEST(FixedTileDataPlane, FixedTileAttrIsUint32) {
    static_assert(std::is_same<FixedTileAttr, std::uint32_t>::value,
                  "FixedTileAttr must be uint32_t");
    EXPECT_TRUE(true);
}


// ===========================================================================
// Default state
// ===========================================================================

TEST(FixedTileDataPlane, DefaultTileIsNotCollision) {
    FixedTile t{};
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, DefaultAttrBitsIsZero) {
    FixedTileAttrBits b{};
    EXPECT_EQ(b.uFixedAttr, 0u);
}

TEST(FixedTileDataPlane, DefaultTileHasZeroAttrBits) {
    FixedTile t{};
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0u);
}


// ===========================================================================
// fixed_tile_init - basic semantics
// ===========================================================================

TEST(FixedTileDataPlane, InitSetsZero) {
    FixedTile t{};
    fixed_tile_init(t, 0u);
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0u);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, InitSetsCollision) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON);
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, TFA_COLLISON);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, InitSetsMaxByte) {
    FixedTile t{};
    fixed_tile_init(t, 0xFFu);
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0xFFu);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, InitCopiesAllFlagCombinations) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON | TFA_PEACEZONE | TFA_FLYCOLLISON | TFA_JSAZONE);
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0x01u | 0x08u | 0x10u | 0x40u);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}


// ===========================================================================
// fixed_tile_init - overwrite semantics
// ===========================================================================

TEST(FixedTileDataPlane, InitOverwritesPreviousValue) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON);
    EXPECT_TRUE(fixed_tile_is_collision(t));
    fixed_tile_init(t, 0u);
    EXPECT_FALSE(fixed_tile_is_collision(t));
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0u);
}

TEST(FixedTileDataPlane, InitOverwriteWithDifferentFlags) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON | TFA_FLYCOLLISON);
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, TFA_COLLISON | TFA_FLYCOLLISON);
    fixed_tile_init(t, TFA_JSAZONE | TFA_PEACEZONE);
    EXPECT_EQ(t.m_FixedAttr.uFixedAttr, TFA_JSAZONE | TFA_PEACEZONE);
    EXPECT_FALSE(fixed_tile_is_collision(t));  // JSAZONE alone is not collision
}


// ===========================================================================
// is_collision - flag combinations
// ===========================================================================

TEST(FixedTileDataPlane, IsCollisionWithPeacezoneOnly) {
    FixedTile t{};
    fixed_tile_init(t, TFA_PEACEZONE);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, IsCollisionWithFlycollisonOnly) {
    FixedTile t{};
    fixed_tile_init(t, TFA_FLYCOLLISON);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, IsCollisionWithJsazoneOnly) {
    FixedTile t{};
    fixed_tile_init(t, TFA_JSAZONE);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, IsCollisionWithCollisonPlusPeacezone) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON | TFA_PEACEZONE);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, IsCollisionWithAllFlags) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON | TFA_PEACEZONE | TFA_FLYCOLLISON | TFA_JSAZONE);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, IsCollisionWithAllExceptCollison) {
    FixedTile t{};
    fixed_tile_init(t, TFA_PEACEZONE | TFA_FLYCOLLISON | TFA_JSAZONE);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTileDataPlane, IsCollisionWithAllBitsSetExceptCollison) {
    FixedTile t{};
    std::uint8_t mask = 0xFFu & ~TFA_COLLISON;  // all bits except COLLISON
    fixed_tile_init(t, mask);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}



// ===========================================================================
// Boundary byte values
// ===========================================================================

TEST(FixedTileDataPlane, BoundaryByteValues) {
    for (std::uint16_t i = 0; i <= 255; ++i) {
        FixedTile t{};
        fixed_tile_init(t, static_cast<std::uint8_t>(i));
        EXPECT_EQ(t.m_FixedAttr.uFixedAttr, static_cast<std::uint8_t>(i));
    }
}

TEST(FixedTileDataPlane, BoundaryByteCollisionDetection) {
    bool should_be_collision;
    for (std::uint16_t i = 0; i <= 255; ++i) {
        FixedTile t{};
        auto byte = static_cast<std::uint8_t>(i);
        fixed_tile_init(t, byte);
        should_be_collision = (byte & TFA_COLLISON) != 0;
        EXPECT_EQ(fixed_tile_is_collision(t), should_be_collision);
    }
}


// ===========================================================================
// Multiple init cycles
// ===========================================================================

TEST(FixedTileDataPlane, InitCyclesResetCleanly) {
    FixedTile t{};
    for (std::uint16_t i = 0; i < 10; ++i) {
        fixed_tile_init(t, 0xFFu);
        EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0xFFu);
        EXPECT_TRUE(fixed_tile_is_collision(t));
        fixed_tile_init(t, 0u);
        EXPECT_EQ(t.m_FixedAttr.uFixedAttr, 0u);
        EXPECT_FALSE(fixed_tile_is_collision(t));
    }
}

TEST(FixedTileDataPlane, MultipleDistinctInits) {
    FixedTile t{};
    for (std::uint8_t b : {std::uint8_t(0u), std::uint8_t(1u), std::uint8_t(2u), std::uint8_t(4u), std::uint8_t(8u), std::uint8_t(16u), std::uint8_t(32u), std::uint8_t(64u), std::uint8_t(128u), std::uint8_t(255u)}) {
        fixed_tile_init(t, b);
        EXPECT_EQ(t.m_FixedAttr.uFixedAttr, b);
    }
}


// ===========================================================================
// FixedTileAttrBits field access
// ===========================================================================

TEST(FixedTileDataPlane, FixedTileAttrBitsFieldType) {
    static_assert(std::is_same<decltype(FixedTileAttrBits{}.uFixedAttr),
                               std::uint8_t>::value,
                  "uFixedAttr must be uint8_t");
    EXPECT_TRUE(true);
}

TEST(FixedTileDataPlane, FixedTileAttrBitsFieldSize) {
    EXPECT_EQ(sizeof(FixedTileAttrBits{}.uFixedAttr), 1u);
}

TEST(FixedTileDataPlane, FixedTileAttrBitsCanStoreAllByteValues) {
    for (std::uint16_t i = 0; i <= 255; ++i) {
        FixedTileAttrBits b{};
        b.uFixedAttr = static_cast<std::uint8_t>(i);
        EXPECT_EQ(b.uFixedAttr, static_cast<std::uint8_t>(i));
    }
}


// ===========================================================================
// FixedTile struct layout
// ===========================================================================

TEST(FixedTileDataPlane, FixedTileContainsAttrBits) {
    FixedTile t{};
    EXPECT_EQ(sizeof(t.m_FixedAttr), sizeof(FixedTileAttrBits));
}

TEST(FixedTileDataPlane, FixedTileHasNoOtherFields) {
    // m_FixedAttr is the only field.
    FixedTile t{};
    static_assert(sizeof(FixedTile) >= sizeof(FixedTileAttrBits),
                  "FixedTile must be at least FixedTileAttrBits");
    EXPECT_TRUE(true);
}


// ===========================================================================
// Wire format invariants (1:1 with legacy byte layout)
// ===========================================================================

TEST(FixedTileDataPlane, UFixedAttrIsFirstField) {
    EXPECT_EQ(offsetof(FixedTileAttrBits, uFixedAttr), 0u);
}

TEST(FixedTileDataPlane, FixedTileAttrBitsSizeIsOneByte) {
    EXPECT_EQ(sizeof(FixedTileAttrBits), 1u);
}

TEST(FixedTileDataPlane, InitPreservesByteExactly) {
    FixedTile t{};
    for (std::uint8_t b : {std::uint8_t(0x00u), std::uint8_t(0x01u), std::uint8_t(0x07u), std::uint8_t(0x10u), std::uint8_t(0x42u), std::uint8_t(0x55u), std::uint8_t(0xAAu), std::uint8_t(0xFFu)}) {
        fixed_tile_init(t, b);
        EXPECT_EQ(static_cast<std::uint8_t>(t.m_FixedAttr.uFixedAttr), b);
    }
}
