#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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

// 2026-09-07 visual-polish: validate that the procedurally synthesized
// placeholder HFL files emitted by modern/tools/gen_hfl_placeholders.py
// round-trip through the real C++ parser.  Skipped if the PlayDH
// fixture is not present (mirrors the convention of ParsesRealMap12Terrain).
TEST(HflHeightField, PlaceholderFilesParse) {
    namespace fs = std::filesystem;
    fs::path mapDir;
    auto root = fs::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : fs::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "Resource" / "Map";
            if (fs::exists(candidate) && fs::is_directory(candidate)) {
                mapDir = candidate;
                break;
            }
        }
        if (!mapDir.empty()) break;
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    if (mapDir.empty()) GTEST_SKIP() << "PlayDH/Resource/Map fixture not installed";

    // Scan a handful of placeholder HFL files (any non-template map id).
    // The first map id we find that is NOT 10, 21, or 101 is by
    // definition a placeholder; we parse it and verify the parser
    // accepts the procedurally-synthesized header and height grid.
    int placeholders = 0;
    for (const auto& entry : fs::directory_iterator(mapDir)) {
        if (!entry.is_regular_file()) continue;
        const auto& name = entry.path().filename().string();
        if (name == "10.hfl" || name == "21.hfl" || name == "101.hfl") continue;
        if (name.find(".hfl") == std::string::npos) continue;
        const auto bytes = [&] {
            std::ifstream f(entry.path(), std::ios::binary);
            std::ostringstream ss;
            ss << f.rdbuf();
            const std::string& s = ss.str();
            return std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t*>(s.data()),
                                             reinterpret_cast<const std::uint8_t*>(s.data() + s.size()));
        }();
        mxh::compat::HflHeightField terrain;
        std::string error;
        ASSERT_TRUE(mxh::compat::parse_hfl(bytes, terrain, &error))
            << "placeholder " << name << " failed: " << error;
        EXPECT_EQ(terrain.version, 1u);
        EXPECT_GT(terrain.desc.height_count_x, 0u);
        EXPECT_GT(terrain.desc.height_count_z, 0u);
        EXPECT_EQ(terrain.heights.size(),
                  terrain.desc.height_count_x * terrain.desc.height_count_z);
        ++placeholders;
        if (placeholders >= 3) break;
    }
    EXPECT_GE(placeholders, 1) << "no placeholder HFL files were found under " << mapDir;
}
