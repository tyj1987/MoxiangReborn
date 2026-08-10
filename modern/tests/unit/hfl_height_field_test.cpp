#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>

namespace {
std::filesystem::path findMapPack() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "Map.pak";
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}
}

TEST(HflHeightField, ParsesRealMap12Terrain) {
    const auto path = findMapPack();
    if (path.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto bytes = pack->read("12.hfl");
    ASSERT_EQ(bytes.size(), 1200122u);
    mxh::compat::HflHeightField terrain;
    std::string error;
    ASSERT_TRUE(mxh::compat::parse_hfl(bytes, terrain, &error)) << error;
    EXPECT_EQ(terrain.version, 1u);
    EXPECT_EQ(terrain.desc.height_count_x, 513u);
    EXPECT_EQ(terrain.desc.height_count_z, 513u);
    EXPECT_EQ(terrain.heights.size(), 513u * 513u);
    EXPECT_EQ(terrain.desc.tile_count_x, 256u);
    EXPECT_EQ(terrain.desc.tile_count_z, 256u);
    EXPECT_EQ(terrain.tiles.size(), 256u * 256u);
    EXPECT_EQ(terrain.textures.size(), 125u);
    EXPECT_FLOAT_EQ(terrain.desc.width, 51200.0f);
    EXPECT_FLOAT_EQ(terrain.desc.tile_size, 200.0f);
    const auto [low, high] = std::minmax_element(terrain.heights.begin(), terrain.heights.end());
    EXPECT_LT(*low, 0.0f);
    EXPECT_GT(*high, 500.0f);
}

TEST(HflHeightField, RejectsTruncatedData) {
    const std::uint8_t data[8]{};
    mxh::compat::HflHeightField terrain;
    EXPECT_FALSE(mxh::compat::parse_hfl(data, terrain));
}
