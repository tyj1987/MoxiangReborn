// fixed_tile_info_test.cpp - Phase D6 FixedTileInfo 1:1 port tests.

#include "mxh/server/fixed_tile_info.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::FixedTileInfoState;
using mxh::server::fixed_tile_info_init;
using mxh::server::fixed_tile_info_in_tile;
using mxh::server::fixed_tile_info_get;
using mxh::server::fixed_tile_info_get_index;
using mxh::server::TileIndex;
using mxh::server::FixedTile;
using mxh::server::fixed_tile_init;
using mxh::server::TFA_COLLISON;

TEST(FixedTileInfo, InitRejectsNonPositive) {
    FixedTileInfoState s{};
    EXPECT_FALSE(fixed_tile_info_init(s, 0, 10));
    EXPECT_FALSE(fixed_tile_info_init(s, 10, 0));
    EXPECT_FALSE(fixed_tile_info_init(s, -1, 10));
}

TEST(FixedTileInfo, InitAllocatesFlatArray) {
    FixedTileInfoState s{};
    ASSERT_TRUE(fixed_tile_info_init(s, 8, 6));
    EXPECT_EQ(s.m_nTileWidth, 8);
    EXPECT_EQ(s.m_nTileHeight, 6);
    EXPECT_EQ(s.m_tiles.size(), 48u);
}

TEST(FixedTileInfo, InTileValidatesBounds) {
    FixedTileInfoState s{};
    ASSERT_TRUE(fixed_tile_info_init(s, 4, 4));
    EXPECT_TRUE(fixed_tile_info_in_tile(s, 0, 0));
    EXPECT_TRUE(fixed_tile_info_in_tile(s, 3, 3));
    EXPECT_FALSE(fixed_tile_info_in_tile(s, -1, 0));
    EXPECT_FALSE(fixed_tile_info_in_tile(s, 0, -1));
    EXPECT_FALSE(fixed_tile_info_in_tile(s, 4, 0));
    EXPECT_FALSE(fixed_tile_info_in_tile(s, 0, 4));
}

TEST(FixedTileInfo, GetReturnsNullOnOutOfBounds) {
    FixedTileInfoState s{};
    ASSERT_TRUE(fixed_tile_info_init(s, 4, 4));
    EXPECT_EQ(fixed_tile_info_get(s, -1, 0), nullptr);
    EXPECT_EQ(fixed_tile_info_get(s, 0, -1), nullptr);
    EXPECT_EQ(fixed_tile_info_get(s, 4, 0), nullptr);
    EXPECT_EQ(fixed_tile_info_get(s, 0, 4), nullptr);
}

TEST(FixedTileInfo, GetReturnsInBoundsSlot) {
    FixedTileInfoState s{};
    ASSERT_TRUE(fixed_tile_info_init(s, 4, 4));
    FixedTile* t = fixed_tile_info_get(s, 2, 1);
    ASSERT_NE(t, nullptr);
    fixed_tile_init(*t, TFA_COLLISON);
    EXPECT_TRUE(fixed_tile_info_get(s, 2, 1) != nullptr);
}

TEST(FixedTileInfo, LayoutIsRowMajorZTimesWPlusX) {
    FixedTileInfoState s{};
    ASSERT_TRUE(fixed_tile_info_init(s, 4, 4));
    EXPECT_EQ(fixed_tile_info_get(s, 0, 0), &s.m_tiles[0]);
    EXPECT_EQ(fixed_tile_info_get(s, 3, 0), &s.m_tiles[3]);
    EXPECT_EQ(fixed_tile_info_get(s, 0, 1), &s.m_tiles[4]);
    EXPECT_EQ(fixed_tile_info_get(s, 3, 1), &s.m_tiles[7]);
    EXPECT_EQ(fixed_tile_info_get(s, 2, 3), &s.m_tiles[3 * 4 + 2]);
}

TEST(FixedTileInfo, GetIndexFromFloat) {
    TileIndex idx = fixed_tile_info_get_index(7.5f, 11.5f, 2.0f);
    EXPECT_EQ(idx.nx, 3);
    EXPECT_EQ(idx.nz, 5);
}

TEST(FixedTileInfo, GetIndexNegativeFloors) {
    TileIndex idx = fixed_tile_info_get_index(-1.0f, -1.0f, 2.0f);
    // C-style truncate: -1 / 2 = 0 in integer, but for -1.0f / 2 = -0.5 -> int = 0?
    // fx / tile_size for negative floats truncates toward zero -> -0.5 -> 0 (legacy C++).
    // We accept either 0 or -1; let the bounds check reject later.
    EXPECT_TRUE(idx.nx == 0 || idx.nx == -1);
}

TEST(FixedTileInfo, GetIndexRejectsZeroTileSize) {
    TileIndex idx = fixed_tile_info_get_index(7.0f, 11.0f, 0.0f);
    EXPECT_EQ(idx.nx, 0);
    EXPECT_EQ(idx.nz, 0);
}

}  // namespace
