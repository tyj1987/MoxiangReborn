// Tests for mxh::compat::BsadArea.

#include "mxh/compat/bsad_area.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using namespace mxh::compat;

TEST(BsadArea, Parse3x3CrossShape) {
    // 3x3 area with cross pattern: center + 4 cardinals = hit.
    std::vector<std::uint8_t> blob;
    BsadHeader h{};
    h.width = 3;
    h.height = 3;
    h.reserved = 0;
    blob.resize(sizeof(h) + 9);
    std::memcpy(blob.data(), &h, sizeof(h));
    // Layout (row-major): cells = [
    //   0 (Hit), 1 (Hit), 2 (Hit),
    //   3 (Hit), 4 (Hit - center), 5 (Hit),
    //   6 (Hit), 7 (Hit), 8 (Hit)
    // ]
    std::uint8_t* cells = blob.data() + sizeof(h);
    for (int i = 0; i < 9; ++i) cells[i] = static_cast<std::uint8_t>(BsadCell::Hit);

    auto area = BsadArea::parse(blob);
    ASSERT_EQ(area.header.width, 3u);
    ASSERT_EQ(area.header.height, 3u);
    ASSERT_EQ(area.cells.size(), 9u);
    EXPECT_TRUE(area.is_hit(1, 1));
    EXPECT_TRUE(area.is_hit(0, 0));
    EXPECT_TRUE(area.is_hit(2, 2));
}

TEST(BsadArea, EmptyArea) {
    std::vector<std::uint8_t> blob;
    BsadHeader h{};
    h.width = 5;
    h.height = 5;
    h.reserved = 0;
    blob.resize(sizeof(h) + 25, 0);
    std::memcpy(blob.data(), &h, sizeof(h));
    auto area = BsadArea::parse(blob);
    EXPECT_FALSE(area.is_hit(2, 2));
}

TEST(BsadArea, RejectsTooSmall) {
    std::vector<std::uint8_t> tiny = {0x01, 0x00};
    EXPECT_FALSE(BsadArea::is_bsad(tiny));
}

TEST(BsadArea, RejectsOutOfBoundsQuery) {
    std::vector<std::uint8_t> blob;
    BsadHeader h{};
    h.width = 3;
    h.height = 3;
    blob.resize(sizeof(h) + 9, 0);
    std::memcpy(blob.data(), &h, sizeof(h));
    auto area = BsadArea::parse(blob);
    EXPECT_FALSE(area.is_hit(99, 99));
}