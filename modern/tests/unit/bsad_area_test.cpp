// Tests for mxh::compat::BsadArea (MHFile text format).

#include "mxh/compat/bsad_area.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using namespace mxh::compat;

namespace {

// Build an MHFile blob from a plaintext payload.
// Header {version=0x0131CA9E (matches real files), type=1, file_size=plain.size()}.
// The "encrypt" step here reuses the MHFile encryption to be byte-identical
// to what the legacy PackingMan tool would have produced.
std::vector<std::uint8_t> make_bsad_blob(const std::string& plain, std::uint32_t type = 1) {
    std::vector<std::uint8_t> payload(plain.begin(), plain.end());
    auto encrypted = encrypt_bin_payload(payload, type);
    MhFileHeader h{};
    h.version = 0x0131CA9E;
    h.type = type;
    h.file_size = static_cast<std::uint32_t>(encrypted.size());

    std::vector<std::uint8_t> out;
    out.resize(sizeof(h) + 1 + encrypted.size());
    std::memcpy(out.data(), &h, sizeof(h));
    out[sizeof(h)] = 0;  // CRC byte (unused)
    std::memcpy(out.data() + sizeof(h) + 1, encrypted.data(), encrypted.size());
    return out;
}

}  // namespace

TEST(BsadArea, Parse3x3AllEmpty) {
    // radius=1 => 3x3 area, all cells empty.
    std::string txt = "1\r\n0 0 0\r\n0 0 0\r\n0 0 0\r\n";
    auto blob = make_bsad_blob(txt);
    auto area = BsadArea::parse(blob);
    ASSERT_EQ(area.header.width, 3u);
    ASSERT_EQ(area.header.height, 3u);
    ASSERT_EQ(area.cells.size(), 9u);
    for (std::uint32_t y = 0; y < 3; ++y)
        for (std::uint32_t x = 0; x < 3; ++x)
            EXPECT_FALSE(area.is_hit(x, y)) << "x=" << x << " y=" << y;
}

TEST(BsadArea, Parse5x5CenterCross) {
    // radius=2 => 5x5 area. Center cross pattern: center + 4 cardinals = Hit.
    std::string txt =
        "2\r\n"
        "0 0 1 0 0\r\n"
        "0 0 1 0 0\r\n"
        "1 1 1 1 1\r\n"
        "0 0 1 0 0\r\n"
        "0 0 1 0 0\r\n";
    auto blob = make_bsad_blob(txt);
    auto area = BsadArea::parse(blob);
    ASSERT_EQ(area.header.width, 5u);
    ASSERT_EQ(area.header.height, 5u);
    EXPECT_TRUE(area.is_hit(2, 2));   // center
    EXPECT_TRUE(area.is_hit(1, 2));   // cardinals
    EXPECT_TRUE(area.is_hit(3, 2));
    EXPECT_TRUE(area.is_hit(2, 1));
    EXPECT_TRUE(area.is_hit(2, 3));
    EXPECT_FALSE(area.is_hit(0, 0));  // corners
    EXPECT_FALSE(area.is_hit(4, 4));
}

TEST(BsadArea, Parse13x13SpikewallShape) {
    // Reproduce the structure of 13x13_Spikewall.bsad:
    // rows 0..2 empty, rows 3..4 hit ring, rows 5..7 hit + block core,
    // rows 8..9 hit ring, rows 10..12 empty.
    std::string txt =
        "6\r\n"
        "0 0 0 0 0 0 0 0 0 0 0 0 0\r\n"
        "0 0 0 0 0 0 0 0 0 0 0 0 0\r\n"
        "0 0 0 0 0 0 0 0 0 0 0 0 0\r\n"
        "1 1 1 1 1 1 1 1 1 1 1 1 1\r\n"
        "1 1 1 1 1 1 1 1 1 1 1 1 1\r\n"
        "1 1 2 2 2 2 2 2 2 2 2 1 1\r\n"
        "1 1 2 2 2 2 2 2 2 2 2 1 1\r\n"
        "1 1 2 2 2 2 2 2 2 2 2 1 1\r\n"
        "1 1 1 1 1 1 1 1 1 1 1 1 1\r\n"
        "1 1 1 1 1 1 1 1 1 1 1 1 1\r\n"
        "0 0 0 0 0 0 0 0 0 0 0 0 0\r\n"
        "0 0 0 0 0 0 0 0 0 0 0 0 0\r\n"
        "0 0 0 0 0 0 0 0 0 0 0 0 0\r\n";
    auto blob = make_bsad_blob(txt);
    auto area = BsadArea::parse(blob);
    ASSERT_EQ(area.header.width, 13u);
    ASSERT_EQ(area.header.height, 13u);
    ASSERT_EQ(area.cells.size(), 169u);
    EXPECT_EQ(area.cells[0], BsadCell::Empty);
    EXPECT_EQ(area.cells[13 * 3], BsadCell::Hit);
    EXPECT_EQ(area.cells[13 * 5 + 2], BsadCell::Block);
    EXPECT_EQ(area.cells[13 * 5 + 0], BsadCell::Hit);
}

TEST(BsadArea, RejectsTooSmall) {
    std::vector<std::uint8_t> tiny = {0x01, 0x00, 0x02};
    EXPECT_FALSE(BsadArea::is_bsad(tiny));
}

TEST(BsadArea, RejectsMissingHeader) {
    // 13 bytes - one byte short of header+crc1.
    std::vector<std::uint8_t> short_(13, 0);
    EXPECT_FALSE(BsadArea::is_bsad(short_));
}

TEST(BsadArea, RejectsOutOfBoundsQuery) {
    std::string txt = "1\r\n0 0 0\r\n0 0 0\r\n0 0 0\r\n";
    auto area = BsadArea::parse(make_bsad_blob(txt));
    EXPECT_FALSE(area.is_hit(99, 99));
    EXPECT_FALSE(area.is_hit(5, 0));   // x past width
    EXPECT_FALSE(area.is_hit(0, 5));   // y past height
}

TEST(BsadArea, ParsesAllRealPlayDhFiles) {
    // Regression: every .bsad file in the real PlayDH Resource/SkillArea dir
    // must parse without throwing or producing an empty area. This is the
    // coverage that gates the M4 resource-coverage acceptance.
    const std::vector<std::string> kRealFiles = {
        "3x3_Blank.bsad", "5x5_Blank.bsad", "7x7_Blank.bsad", "9x9_Blank.bsad",
        "11x11_Blank.bsad", "11x11_wall.bsad", "13x13_Blank.bsad",
        "13x13_Spikewall.bsad", "15x15_Blank.bsad", "17X17_Blank.bsad",
        "17X17_lineAttack.bsad",
    };
    for (const auto& name : kRealFiles) {
        const std::string path = std::string("C:/moxiang/modern/scratch/2026-08-10-resource-coverage/playdh_link_for_audit/Resource/SkillArea/") + name;
        auto area = BsadArea::load(path);
        EXPECT_TRUE(area.cells.size() > 0) << name << " parsed empty";
        EXPECT_EQ(area.header.width, area.header.height) << name << " non-square";
        EXPECT_GT(area.header.width, 0u) << name << " zero width";
    }
}

TEST(BsadArea, DifferentTypeStillDecodes) {
    // Some real .bsad files use type=0 (no extra XOR step). Verify the parser
    // handles type=0 correctly.
    std::string txt = "1\r\n0 0 0\r\n0 0 0\r\n0 0 0\r\n";
    auto blob = make_bsad_blob(txt, /*type=*/0);
    auto area = BsadArea::parse(blob);
    ASSERT_EQ(area.header.width, 3u);
    ASSERT_EQ(area.cells.size(), 9u);
}
