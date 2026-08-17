// tests/unit/compat/image_path_table_test.cpp
// 1:1 verification of legacy image path table parsing. Reads the
// actual PlayDH/Image/image_*.bin files and verifies the parser
// produces the expected number of entries + spot-checks key records.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "mxh/compat/image_path_table.hpp"

namespace fs = std::filesystem;

namespace {

fs::path locate_playdh() {
    fs::path candidates[] = {
        "modern/data/PlayDH",
        "../data/PlayDH",
        "../../data/PlayDH",
        "../../../data/PlayDH",
        "C:/moxiang/modern/data/PlayDH",
        "C:/moxiang/墨香【源码配套资源】/PlayDH",
    };
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c / "Image", ec)) return c;
    }
    return {};
}

}  // namespace

TEST(ImagePathTable, ParseSyntheticInput) {
    // Synthetic 6-tuple ASCII payload (index idx left top right bottom).
    std::vector<std::uint8_t> p = {
        '0', ' ', '0', ' ', '1', ' ', '2', ' ', '3', ' ', '4', ' ',
        '\n',
        '1', ' ', '1', ' ', '5', ' ', '6', ' ', '7', ' ', '8', '\n',
    };
    auto v = mxh::compat::parse_image_path_table(p);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].index, 0);
    EXPECT_EQ(v[0].idx, 0);
    EXPECT_EQ(v[0].left, 1);
    EXPECT_EQ(v[0].top, 2);
    EXPECT_EQ(v[0].right, 3);
    EXPECT_EQ(v[0].bottom, 4);
    EXPECT_EQ(v[1].index, 1);
    EXPECT_EQ(v[1].idx, 1);
    EXPECT_EQ(v[1].left, 5);
    EXPECT_EQ(v[1].top, 6);
    EXPECT_EQ(v[1].right, 7);
    EXPECT_EQ(v[1].bottom, 8);
}

TEST(ImagePathTable, MugongPathHasExpectedEntries) {
    auto playdh = locate_playdh();
    ASSERT_FALSE(playdh.empty());
    auto v = mxh::compat::read_image_path_table(
        playdh / "Image" / "image_mugong_path.bin");
    // Legacy has ~134 mugong icons. Allow some slack for region
    // variants but require >= 100.
    EXPECT_GE(v.size(), 100u);
    // Spot-check first entry (idx=0 mugong icon position).
    ASSERT_FALSE(v.empty());
    EXPECT_EQ(v[0].index, 0);
    // Skill icon sprites are 40x40.
    EXPECT_EQ(v[0].right - v[0].left, 40);
    EXPECT_EQ(v[0].bottom - v[0].top, 40);
}

TEST(ImagePathTable, ItemPathHasExpectedEntries) {
    auto playdh = locate_playdh();
    ASSERT_FALSE(playdh.empty());
    auto v = mxh::compat::read_image_path_table(
        playdh / "Image" / "image_item_path.bin");
    // Legacy has thousands of item icons.
    EXPECT_GE(v.size(), 1000u);
    ASSERT_FALSE(v.empty());
    EXPECT_EQ(v[0].index, 0);
}

TEST(ImagePathTable, HardPathHasExpectedEntries) {
    auto playdh = locate_playdh();
    ASSERT_FALSE(playdh.empty());
    auto v = mxh::compat::read_image_path_table(
        playdh / "Image" / "image_hard_path.bin");
    EXPECT_GE(v.size(), 50u);
}

TEST(ImagePathTable, EmptyPayloadReturnsEmpty) {
    std::vector<std::uint8_t> p;
    auto v = mxh::compat::parse_image_path_table(p);
    EXPECT_TRUE(v.empty());
}

TEST(ImagePathTable, NonNumericTokensYieldZeros) {
    // Mirrors legacy atoi() behavior: non-digit tokens yield 0.
    std::vector<std::uint8_t> p = {'X', ' ', 'Y', ' ', 'Z', ' ', 'A', ' ', 'B'};
    auto v = mxh::compat::parse_image_path_table(p);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0].idx, 0);
    EXPECT_EQ(v[0].left, 0);
    EXPECT_EQ(v[0].top, 0);
    EXPECT_EQ(v[0].right, 0);
    EXPECT_EQ(v[0].bottom, 0);
}
