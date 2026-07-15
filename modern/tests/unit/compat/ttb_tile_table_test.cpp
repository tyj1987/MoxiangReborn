// ttb_tile_table_test.cpp - Phase 10.13 .ttb tile-table parser tests
//
// Covers modern/include/mxh/compat/ttb_tile_table.hpp + the matching
// modern/src/ttb_tile_table.cpp skeleton. The .ttb format is parsed
// in two paths (headered and raw u32 grid) so the test covers both.

#include "mxh/compat/ttb_tile_table.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace mxh::compat::test {

namespace {

// Helper: build a headered .ttb buffer (8-byte header + W*H*4 tiles).
std::vector<std::uint8_t> make_headered_ttb(std::uint32_t w, std::uint32_t h,
                                              const std::vector<std::uint32_t>& tiles) {
    std::vector<std::uint8_t> buf;
    buf.resize(8 + w * h * 4);
    std::memcpy(buf.data() + 0, &w, 4);
    std::memcpy(buf.data() + 4, &h, 4);
    if (!tiles.empty()) {
        std::memcpy(buf.data() + 8, tiles.data(), tiles.size() * 4);
    }
    return buf;
}

// Helper: build a raw u32 grid (no header, just N u32 values).
std::vector<std::uint8_t> make_raw_grid_ttb(const std::vector<std::uint32_t>& tiles) {
    std::vector<std::uint8_t> buf(tiles.size() * 4);
    if (!tiles.empty()) {
        std::memcpy(buf.data(), tiles.data(), tiles.size() * 4);
    }
    return buf;
}

}  // namespace

// ===========================================================================
// Default state
// ===========================================================================

TEST(TtbTileTableTest, DefaultConstructionIsEmpty) {
    TtbTileTable t;
    EXPECT_EQ(t.width, 0u);
    EXPECT_EQ(t.height, 0u);
    EXPECT_TRUE(t.tiles.empty());
}

// ===========================================================================
// Headered format (8-byte header + W*H*4 tile indices)
// ===========================================================================

TEST(TtbTileTableTest, ParseHeadered2x2) {
    // 2x2 grid with tile indices {0, 1, 2, 3}.
    auto buf = make_headered_ttb(2, 2, {0, 1, 2, 3});
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 2u);
    EXPECT_EQ(t.height, 2u);
    ASSERT_EQ(t.tiles.size(), 4u);
    EXPECT_EQ(t.tiles[0], 0u);
    EXPECT_EQ(t.tiles[1], 1u);
    EXPECT_EQ(t.tiles[2], 2u);
    EXPECT_EQ(t.tiles[3], 3u);
}

TEST(TtbTileTableTest, ParseHeadered10x10NonZeroTiles) {
    // 10x10 grid filled with a non-trivial pattern. Pin a few
    // specific entries to make sure row-major layout is preserved.
    std::vector<std::uint32_t> tiles(100);
    for (std::uint32_t i = 0; i < 100; ++i) {
        tiles[i] = i * 7 + 13;  // arbitrary but unique per cell
    }
    auto buf = make_headered_ttb(10, 10, tiles);
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 10u);
    EXPECT_EQ(t.height, 10u);
    ASSERT_EQ(t.tiles.size(), 100u);
    for (std::uint32_t i = 0; i < 100; ++i) {
        EXPECT_EQ(t.tiles[i], i * 7 + 13u) << "tile[" << i << "]";
    }
}

TEST(TtbTileTableTest, ParseHeadered1x1) {
    // 1x1 edge case — the smallest valid headered format.
    auto buf = make_headered_ttb(1, 1, {42});
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 1u);
    EXPECT_EQ(t.height, 1u);
    ASSERT_EQ(t.tiles.size(), 1u);
    EXPECT_EQ(t.tiles[0], 42u);
}

TEST(TtbTileTableTest, ParseHeaderedLargeWidth) {
    // 1000x1 (a long thin strip) — verifies the size math for a
    // large but not pathological grid.
    std::vector<std::uint32_t> tiles(1000);
    for (std::uint32_t i = 0; i < 1000; ++i) tiles[i] = i;
    auto buf = make_headered_ttb(1000, 1, tiles);
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 1000u);
    EXPECT_EQ(t.height, 1u);
    ASSERT_EQ(t.tiles.size(), 1000u);
    EXPECT_EQ(t.tiles[999], 999u);
}

// ===========================================================================
// Raw grid format (no header — fallback path)
// ===========================================================================

TEST(TtbTileTableTest, ParseRawGridFourTiles) {
    // 4 u32 values, no header. Falls into the raw-grid path. The
    // best-effort width/height is set to (4, 1).
    auto buf = make_raw_grid_ttb({10, 20, 30, 40});
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 4u);
    EXPECT_EQ(t.height, 1u);
    ASSERT_EQ(t.tiles.size(), 4u);
    EXPECT_EQ(t.tiles[0], 10u);
    EXPECT_EQ(t.tiles[1], 20u);
    EXPECT_EQ(t.tiles[2], 30u);
    EXPECT_EQ(t.tiles[3], 40u);
}

TEST(TtbTileTableTest, ParseRawGridSizeNotMultipleOf4Fails) {
    // Raw grid path requires size % 4 == 0. Sending 7 bytes is
    // neither a valid header nor a valid raw grid → empty result.
    std::vector<std::uint8_t> buf = {1, 2, 3, 4, 5, 6, 7};
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 0u);
    EXPECT_EQ(t.height, 0u);
    EXPECT_TRUE(t.tiles.empty());
}

// ===========================================================================
// Invalid input
// ===========================================================================

TEST(TtbTileTableTest, ParseEmptyBufferReturnsEmpty) {
    TtbTileTable t = TtbTileTable::parse(std::span<const std::uint8_t>{});
    EXPECT_EQ(t.width, 0u);
    EXPECT_EQ(t.height, 0u);
    EXPECT_TRUE(t.tiles.empty());
}

TEST(TtbTileTableTest, ParseTooShortForHeaderReturnsEmpty) {
    // Less than 8 bytes — the parser's first guard is
    // `if (bytes.size() < 8) return t;`, so anything shorter returns
    // empty regardless of content. (A 4-byte input COULD theoretically
    // be one u32 in the raw-grid path, but the parser bails out
    // early without ever checking the raw-grid branch.) The test
    // pins this behavior so a future refactor that promotes the
    // raw-grid path to a try-before-the-8-byte-guard flow would
    // surface as a deliberate test change.
    std::vector<std::uint8_t> buf = {1, 2, 3, 4};
    TtbTileTable t = TtbTileTable::parse(buf);
    EXPECT_EQ(t.width, 0u);
    EXPECT_EQ(t.height, 0u);
    EXPECT_TRUE(t.tiles.empty());
}

TEST(TtbTileTableTest, ParseHeaderedZeroWidthFails) {
    // Header with width=0 fails the (w > 0 && w < 10000) check,
    // and 8 bytes is not a valid raw grid (8 % 4 == 0 but the
    // raw-grid path then needs 2 u32 entries which would be
    // width=2, height=1). So we get a 2-tile raw grid.
    std::uint32_t w = 0, h = 0;
    std::vector<std::uint8_t> buf(8);
    std::memcpy(buf.data() + 0, &w, 4);
    std::memcpy(buf.data() + 4, &h, 4);
    TtbTileTable t = TtbTileTable::parse(buf);
    // Headered path fails (w==0); raw-grid path runs, produces 2 tiles.
    EXPECT_EQ(t.width, 2u);
    EXPECT_EQ(t.height, 1u);
    EXPECT_EQ(t.tiles.size(), 2u);
}

TEST(TtbTileTableTest, ParseHeaderedMismatchedSizeFails) {
    // Header says 2x2 (needs 8 + 4*4 = 24 bytes) but we provide
    // only 16 bytes. Headered path fails, 16 bytes is not a
    // multiple of 4 (16 is a multiple actually, 16/4=4), so raw
    // grid path picks it up.
    auto buf = make_headered_ttb(2, 2, {0, 1, 2, 3});
    buf.resize(16);  // truncate to 16 bytes
    TtbTileTable t = TtbTileTable::parse(buf);
    // Falls through to raw grid (4 u32 values).
    EXPECT_EQ(t.width, 4u);
    EXPECT_EQ(t.height, 1u);
    EXPECT_EQ(t.tiles.size(), 4u);
}

}  // namespace mxh::compat::test
