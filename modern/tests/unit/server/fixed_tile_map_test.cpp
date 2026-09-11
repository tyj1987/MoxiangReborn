#include "mxh/server/fixed_tile_map.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <limits>

using mxh::server::FixedTileMap;
namespace {
std::vector<std::uint8_t> grid(std::uint32_t w, std::uint32_t h) {
    std::vector<std::uint8_t> b(8 + w * h * 2);
    for (int i = 0; i < 4; ++i) { b[i] = (w >> (i * 8)) & 255; b[4+i] = (h >> (i * 8)) & 255; }
    return b;
}
}
TEST(FixedTileMap, RejectsMalformedDimensionsLengthsAndMissingFiles) {
    std::string error;
    EXPECT_FALSE(FixedTileMap::decode({}, error));
    EXPECT_FALSE(error.empty());
    auto b = grid(2, 2);
    b.pop_back(); EXPECT_FALSE(FixedTileMap::decode(b, error));
    b = grid(2, 2); b.push_back(0); EXPECT_FALSE(FixedTileMap::decode(b, error));
    b = grid(0, 2); EXPECT_FALSE(FixedTileMap::decode(b, error));
    b = grid(2, 2); b[3] = 128; EXPECT_FALSE(FixedTileMap::decode(b, error));
    EXPECT_FALSE(FixedTileMap::load("nonexistent-fixed-tile-fixture.ttb", error));
}
TEST(FixedTileMap, PreservesWordAttributesAndOnlyBitZeroBlocksGround) {
    auto b = grid(2, 1); b[8] = 0x10; b[9] = 0x11; b[10] = 1;
    std::string error; auto map = FixedTileMap::decode(b, error); ASSERT_TRUE(map);
    EXPECT_EQ(map->attributes()[0], 0x1110);
    EXPECT_FALSE(map->blocked(0, 0)); EXPECT_TRUE(map->blocked(50, 0));
    EXPECT_TRUE(map->blocked(-0.001f, 0)); EXPECT_TRUE(map->blocked(100, 0));
    EXPECT_TRUE(map->blocked(std::numeric_limits<float>::quiet_NaN(), 0));
    EXPECT_TRUE(map->trace(0, 0, 100, 50).collision);
}
TEST(FixedTileMap, OriginalDiagonalScanRequiresSeparateEndpointCheck) {
    auto b = grid(2, 2); b[14] = 1;
    std::string error; auto map = FixedTileMap::decode(b, error); ASSERT_TRUE(map);
    EXPECT_FALSE(map->trace(1, 1, 51, 51).collision);
    EXPECT_TRUE(map->blocked(51, 51));
    EXPECT_TRUE(map->trace(51, 51, 51, 51).collision);
}
TEST(FixedTileMap, FirstCellSkipAndLastPassedOriginMatchLegacy) {
    auto b = grid(3, 1); b[8] = 1; b[12] = 1;
    std::string error; auto map = FixedTileMap::decode(b, error); ASSERT_TRUE(map);
    EXPECT_FALSE(map->trace(1, 1, 51, 1).collision);
    const auto hit = map->trace(1, 1, 101, 1);
    EXPECT_TRUE(hit.collision); EXPECT_FLOAT_EQ(hit.last_x, 50); EXPECT_FLOAT_EQ(hit.last_z, 0);
}
TEST(FixedTileMap, CanonicalMap10HasOriginalDimensionsAndPopulation) {
    std::string error;
    auto map = FixedTileMap::load(std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/Map/10.ttb", error);
    ASSERT_TRUE(map) << error;
    EXPECT_EQ(map->width(), 1024); EXPECT_EQ(map->height(), 1024);
    EXPECT_EQ(std::count(map->attributes().begin(), map->attributes().end(), 0), 451667);
    EXPECT_EQ(std::count(map->attributes().begin(), map->attributes().end(), 1), 596909);
}
