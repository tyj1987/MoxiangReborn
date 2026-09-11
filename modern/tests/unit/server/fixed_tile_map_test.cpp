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

namespace {
// Independent reference path builder from D:/MX TileManager.cpp:557-699.
// Build the visited cells without consulting the production map or its helpers;
// collision is resolved separately against the fixture WORD array.
std::vector<std::pair<int, int>> legacy_path(int x, int z, int tx, int tz) {
    std::vector<std::pair<int, int>> path;
    const bool major_x = std::abs(tx-x) >= std::abs(tz-z);
    int major = major_x ? x : z, minor = major_x ? z : x;
    const int target_major = major_x ? tx : tz, target_minor = major_x ? tz : tx;
    const int distance_major = std::abs(target_major-major), distance_minor = std::abs(target_minor-minor);
    const int major_step = target_major >= major ? 1 : -1, minor_step = target_minor >= minor ? 1 : -1;
    if (!distance_major) return {{x,z}};
    int decision = 4 * distance_minor - distance_major; // first continue advances the loop tail
    major += major_step;
    for (;;) {
        path.emplace_back(major_x ? major : minor, major_x ? minor : major);
        if (major == target_major) break;
        if (decision > 0) { minor += minor_step; decision -= 2 * distance_major; }
        major += major_step;
        decision += 2 * distance_minor;
    }
    return path;
}
}

TEST(FixedTileMap, LegacyReferencePathsLockBiasAndStrictTieRule) {
    EXPECT_EQ(legacy_path(0,0,1,1), (std::vector<std::pair<int,int>>{{1,0}}));
    EXPECT_EQ(legacy_path(1,1,0,0), (std::vector<std::pair<int,int>>{{0,1}}));
    EXPECT_EQ(legacy_path(0,0,4,1), (std::vector<std::pair<int,int>>{{1,0},{2,0},{3,1},{4,1}}));
    EXPECT_EQ(legacy_path(0,0,1,4), (std::vector<std::pair<int,int>>{{0,1},{0,2},{1,3},{1,4}}));
}

TEST(FixedTileMap, AllDirectionsMatchIndependentLegacyPathOracle) {
    constexpr int side = 7;
    // Every single blocked cell, all clear, checkerboard and all blocked.
    for (int pattern = -3; pattern < side * side; ++pattern) {
        auto b = grid(side,side);
        std::vector<bool> blocked(side*side);
        for (int i = 0; i < side*side; ++i) {
            blocked[i] = pattern == i || pattern == -3 || (pattern == -2 && ((i/side+i%side)%2));
            b[8+2*i] = blocked[i] ? 1 : 0;
        }
        std::string error; auto map = FixedTileMap::decode(b,error); ASSERT_TRUE(map);
        for (int start = 0; start < side*side; ++start)
        for (int end = 0; end < side*side; ++end) {
            int last_x = start%side, last_z = start/side;
            bool collision = false;
            for (auto [x,z] : legacy_path(last_x,last_z,end%side,end/side)) {
                if (x<0 || z<0 || x>=side || z>=side || blocked[z*side+x]) { collision=true; break; }
                last_x=x; last_z=z;
            }
            const float offset = (start+end)%2 ? 49.999f : 0.001f;
            auto actual = map->trace((start%side)*50+offset,(start/side)*50+offset,
                                     (end%side)*50+offset,(end/side)*50+offset);
            ASSERT_EQ(actual.collision, collision) << pattern << ':' << start << ':' << end;
            if (collision) {
                ASSERT_FLOAT_EQ(actual.last_x,last_x*50) << pattern << ':' << start << ':' << end;
                ASSERT_FLOAT_EQ(actual.last_z,last_z*50) << pattern << ':' << start << ':' << end;
            }
        }
    }
}
