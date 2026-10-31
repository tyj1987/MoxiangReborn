#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/pack_file.hpp"

#include "../support/handwritten_fixture.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

mxh::test::HandwrittenHfl hfl_spec(std::uint32_t height_x, std::uint32_t height_z,
                                  std::uint32_t tile_x, std::uint32_t tile_z,
                                  float width, float tile_size,
                                  std::uint16_t texture_index, std::string texture_name,
                                  std::vector<float> heights,
                                  std::vector<std::uint16_t> tiles) {
    mxh::test::HandwrittenHfl spec;
    spec.height_x = height_x;
    spec.height_z = height_z;
    spec.tile_x = tile_x;
    spec.tile_z = tile_z;
    spec.width = width;
    spec.tile_size = tile_size;
    spec.texture_index = texture_index;
    spec.texture_name = std::move(texture_name);
    spec.heights = std::move(heights);
    spec.tiles = std::move(tiles);
    return spec;
}

void expect_hfl(const mxh::compat::HflHeightField& terrain,
                const mxh::test::HandwrittenHfl& spec) {
    EXPECT_EQ(terrain.version, 1u);
    EXPECT_EQ(terrain.desc.height_count_x, spec.height_x);
    EXPECT_EQ(terrain.desc.height_count_z, spec.height_z);
    EXPECT_EQ(terrain.heights, spec.heights);
    EXPECT_EQ(terrain.desc.tile_count_x, spec.tile_x);
    EXPECT_EQ(terrain.desc.tile_count_z, spec.tile_z);
    EXPECT_EQ(terrain.tiles, spec.tiles);
    ASSERT_EQ(terrain.textures.size(), 1u);
    EXPECT_EQ(terrain.textures[0].index, spec.texture_index);
    EXPECT_EQ(terrain.textures[0].name, spec.texture_name);
    EXPECT_FLOAT_EQ(terrain.desc.width, spec.width);
    EXPECT_FLOAT_EQ(terrain.desc.tile_size, spec.tile_size);
}

}  // namespace

// Real map 12 terrain. Counts and the height span are properties of that
// file. MXH_PLAYDH_ROOT is the only location.
TEST(HflHeightField, ParsesRealMap12Terrain) {
    const auto root = mxh::test::require_playdh_root();
    const auto path = root / "Map.pak";
    ASSERT_TRUE(std::filesystem::is_regular_file(path))
        << mxh::test::kPlayDhRootEnv << " is set but Map.pak is missing";
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

// Recovered loose map 17 terrain. Same rule: the env root or a failure.
TEST(HflHeightField, ParsesRecoveredMap17LooseTerrain) {
    const auto root = mxh::test::require_playdh_root();
    const auto path = root / "Resource" / "Map" / "17.hfl";
    ASSERT_TRUE(std::filesystem::is_regular_file(path))
        << mxh::test::kPlayDhRootEnv << " is set but Resource/Map/17.hfl is missing";
    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input);
    std::vector<std::uint8_t> bytes;
    for (char value = 0; input.get(value);)
        bytes.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(value)));
    ASSERT_EQ(bytes.size(), 1192972u);
    mxh::compat::HflHeightField terrain;
    std::string error;
    ASSERT_TRUE(mxh::compat::parse_hfl(bytes, terrain, &error)) << error;
    EXPECT_EQ(terrain.version, 1u);
    EXPECT_EQ(terrain.desc.height_count_x, 513u);
    EXPECT_EQ(terrain.desc.height_count_z, 513u);
    EXPECT_EQ(terrain.desc.tile_count_x, 256u);
    EXPECT_EQ(terrain.desc.tile_count_z, 256u);
    EXPECT_EQ(terrain.textures.size(), 70u);
    EXPECT_FLOAT_EQ(terrain.desc.width, 51200.0f);
    EXPECT_FLOAT_EQ(terrain.desc.tile_size, 200.0f);
}

TEST(HflHeightField, RejectsTruncatedData) {
    const std::uint8_t data[8]{};
    mxh::compat::HflHeightField terrain;
    EXPECT_FALSE(mxh::compat::parse_hfl(data, terrain));
}

// Three hand-written .hfl files in a temp directory. The parser must accept
// each header, height grid, texture name, and tile table.
TEST(HflHeightField, PlaceholderFilesParse) {
    const mxh::test::HandwrittenHfl specs[] = {
        hfl_spec(2, 2, 1, 1, 400.0f, 50.0f, 7, "syn_a",
                 {-3.5f, 0.0f, 12.0f, 640.0f}, {4}),
        hfl_spec(3, 1, 1, 2, 90.0f, 30.0f, 2, "syn_b",
                 {1.0f, 2.0f, 3.0f}, {8, 9}),
        hfl_spec(1, 4, 2, 2, 16.0f, 4.0f, 11, "syn_c",
                 {5.0f, 6.0f, 7.0f, 8.0f}, {1, 2, 3, 4}),
    };
    const char* names[] = {"syn_a.hfl", "syn_b.hfl", "syn_c.hfl"};
    mxh::test::HandwrittenFixtureDir dir;
    ASSERT_TRUE(dir.created());
    for (std::size_t i = 0; i < 3; ++i) {
        const auto bytes = mxh::test::build_hfl(specs[i]);
        ASSERT_FALSE(bytes.empty()) << names[i];
        const auto path = dir.write_bytes(names[i], bytes);
        ASSERT_FALSE(path.empty()) << names[i];
        const auto read = read_file(path);
        ASSERT_EQ(read, bytes) << names[i];
        mxh::compat::HflHeightField terrain;
        std::string error;
        ASSERT_TRUE(mxh::compat::parse_hfl(read, terrain, &error)) << names[i] << ": " << error;
        expect_hfl(terrain, specs[i]);
    }
}

// Pak plus .hfl. The archive is built in the temp directory at test time.
TEST(HflHeightField, ParsesSyntheticPackedTerrain) {
    const auto spec = hfl_spec(2, 3, 2, 2, 120.0f, 10.0f, 3, "syn_pack",
                               {-8.0f, 1.0f, 2.0f, 3.0f, 4.0f, 700.0f},
                               {11, 12, 13, 14});
    const auto hfl = mxh::test::build_hfl(spec);
    ASSERT_FALSE(hfl.empty());
    const auto pak = mxh::test::build_original96_pak({{"grid.hfl", hfl}});
    ASSERT_FALSE(pak.empty());
    mxh::test::HandwrittenFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const auto path = dir.write_bytes("syn.pak", pak);
    ASSERT_FALSE(path.empty());
    std::string error;
    const auto pack = mxh::compat::PackFile::open(path, &error);
    ASSERT_NE(pack, nullptr) << error;
    const auto bytes = pack->read("grid.hfl");
    ASSERT_EQ(bytes, hfl);
    mxh::compat::HflHeightField terrain;
    ASSERT_TRUE(mxh::compat::parse_hfl(bytes, terrain, &error)) << error;
    expect_hfl(terrain, spec);
    const auto [low, high] = std::minmax_element(terrain.heights.begin(), terrain.heights.end());
    EXPECT_FLOAT_EQ(*low, -8.0f);
    EXPECT_FLOAT_EQ(*high, 700.0f);
}
