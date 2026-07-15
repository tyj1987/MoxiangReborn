// chr_motion_test.cpp - Phase 10.14 .chr motion parser tests
//
// Covers modern/include/mxh/compat/chr_motion.hpp + src/chr_motion.cpp.
// The .chr format holds per-frame bone animation data. The current
// implementation is a skeleton: it validates the 32-byte packed
// header and copies the raw bytes, but does not yet decode the bone
// tracks (TODO in Phase 1.3). The test pins the validation logic
// so a future bone-track decode lands on a verified foundation.

#include "mxh/compat/chr_motion.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace mxh::compat::test {

namespace {

// Helper: build a 32-byte ChrHeader (packed, no padding) with the
// given values. Matches the layout in chr_motion.hpp:
//
//   u32 magic  u32 version  u32 frame_count  u32 bone_count
//   u32 fps    u32 reserved[3]
constexpr std::size_t kChrHeaderSize = 32;

std::vector<std::uint8_t> make_chr_bytes(std::uint32_t magic,
                                          std::uint32_t version,
                                          std::uint32_t frame_count,
                                          std::uint32_t bone_count,
                                          std::uint32_t fps) {
    std::vector<std::uint8_t> buf(kChrHeaderSize, 0);
    std::memcpy(buf.data() + 0,  &magic,       4);
    std::memcpy(buf.data() + 4,  &version,     4);
    std::memcpy(buf.data() + 8,  &frame_count, 4);
    std::memcpy(buf.data() + 12, &bone_count,  4);
    std::memcpy(buf.data() + 16, &fps,         4);
    // reserved[3] stays zero
    return buf;
}

}  // namespace

// ===========================================================================
// ChrHeader wire format
// ===========================================================================

TEST(ChrHeaderTest, WireFormatSizeIs32Bytes) {
    // 4+4+4+4+4+12 = 32 bytes. The struct uses #pragma pack(push, 1)
    // so no padding. Pinning here catches a future change that drops
    // the pragma or adds a field without bumping the size check.
    EXPECT_EQ(sizeof(ChrHeader), 32u);
    EXPECT_EQ(sizeof(ChrHeader), kChrHeaderSize);
}

// ===========================================================================
// is_chr() — header validation
// ===========================================================================

TEST(IsChrTest, RejectsBufferShorterThanHeader) {
    // Any buffer smaller than 32 bytes cannot be a valid .chr
    // regardless of content. The check fires before version/fps
    // are even read.
    EXPECT_FALSE(ChrMotion::is_chr(std::span<const std::uint8_t>{}));
    // 31 bytes — one short of the 32-byte header size.
    std::vector<std::uint8_t> short_buf = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30
    };
    EXPECT_FALSE(ChrMotion::is_chr(short_buf));
}

TEST(IsChrTest, AcceptsValidHeader) {
    auto buf = make_chr_bytes(/*magic=*/0x41434852,  // 'ACHR' LE
                              /*version=*/3,
                              /*frame_count=*/60,
                              /*bone_count=*/20,
                              /*fps=*/30);
    EXPECT_TRUE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, RejectsVersionZero) {
    // version must be >= 1.
    auto buf = make_chr_bytes(0, 0, 60, 20, 30);
    EXPECT_FALSE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, RejectsVersionAboveTen) {
    // The current parser only accepts versions 1..10. Pin that
    // boundary so a future bump to "accept version 11+" shows up
    // here.
    auto buf = make_chr_bytes(0, 11, 60, 20, 30);
    EXPECT_FALSE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, AcceptsVersionBoundaries) {
    EXPECT_TRUE(ChrMotion::is_chr(make_chr_bytes(0, 1, 60, 20, 30)));
    EXPECT_TRUE(ChrMotion::is_chr(make_chr_bytes(0, 10, 60, 20, 30)));
}

TEST(IsChrTest, RejectsZeroFrameCount) {
    auto buf = make_chr_bytes(0, 3, /*frame_count=*/0, 20, 30);
    EXPECT_FALSE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, RejectsHugeFrameCount) {
    // frame_count must be < 1'000'000.
    auto buf = make_chr_bytes(0, 3, /*frame_count=*/1'000'000, 20, 30);
    EXPECT_FALSE(ChrMotion::is_chr(buf));
    auto buf2 = make_chr_bytes(0, 3, /*frame_count=*/2'000'000, 20, 30);
    EXPECT_FALSE(ChrMotion::is_chr(buf2));
}

TEST(IsChrTest, AcceptsLargeFrameCount) {
    // Just under the 1M cap should still pass.
    auto buf = make_chr_bytes(0, 3, /*frame_count=*/999'999, 20, 30);
    EXPECT_TRUE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, RejectsZeroFps) {
    auto buf = make_chr_bytes(0, 3, 60, 20, /*fps=*/0);
    EXPECT_FALSE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, RejectsFpsAbove240) {
    auto buf = make_chr_bytes(0, 3, 60, 20, /*fps=*/241);
    EXPECT_FALSE(ChrMotion::is_chr(buf));
}

TEST(IsChrTest, AcceptsFpsBoundaries) {
    // Pin the inclusive/exclusive boundaries of the fps check.
    // The cpp uses `h.fps < 240` (strict less-than), so fps=239
    // passes and fps=240 fails. fps=1 also passes.
    EXPECT_TRUE(ChrMotion::is_chr(make_chr_bytes(0, 3, 60, 20, /*fps=*/1)));
    EXPECT_TRUE(ChrMotion::is_chr(make_chr_bytes(0, 3, 60, 20, /*fps=*/239)));
}

TEST(IsChrTest, BoneCountIsNotValidated) {
    // The current parser only checks version + frame_count + fps.
    // bone_count can be 0 (no bones is technically valid for a
    // static model), or absurdly large. Pin the current behaviour
    // so a future change that adds bone_count validation lands
    // here as a deliberate test update.
    EXPECT_TRUE(ChrMotion::is_chr(make_chr_bytes(0, 3, 60, /*bone_count=*/0, 30)));
    EXPECT_TRUE(ChrMotion::is_chr(make_chr_bytes(0, 3, 60, /*bone_count=*/99999, 30)));
}

// ===========================================================================
// parse() — full pipeline
// ===========================================================================

TEST(ChrParseTest, ReturnsEmptyForInvalidHeader) {
    // Invalid header → empty ChrMotion (no raw bytes, no header).
    auto buf = make_chr_bytes(0, 0, 60, 20, 30);  // version 0 → invalid
    ChrMotion m = ChrMotion::parse(buf);
    EXPECT_TRUE(m.raw.empty());
    // Default-constructed header is all-zero; we don't compare magic
    // because the field is 0 by default and the test value was 0.
    EXPECT_EQ(m.header.frame_count, 0u);
}

TEST(ChrParseTest, PreservesHeaderOnValidInput) {
    auto buf = make_chr_bytes(/*magic=*/0xDEADBEEF, /*version=*/5,
                              /*frame_count=*/120, /*bone_count=*/42,
                              /*fps=*/60);
    ChrMotion m = ChrMotion::parse(buf);
    EXPECT_EQ(m.header.magic, 0xDEADBEEFu);
    EXPECT_EQ(m.header.version, 5u);
    EXPECT_EQ(m.header.frame_count, 120u);
    EXPECT_EQ(m.header.bone_count, 42u);
    EXPECT_EQ(m.header.fps, 60u);
    EXPECT_EQ(m.header.reserved[0], 0u);
    EXPECT_EQ(m.header.reserved[1], 0u);
    EXPECT_EQ(m.header.reserved[2], 0u);
}

TEST(ChrParseTest, PreservesRawBytesOnValidInput) {
    // The current skeleton stores the entire input in .raw, so the
    // header is at offset 0..31 of raw. Pin that so a future
    // compression or framing change surfaces here.
    auto buf = make_chr_bytes(0xCAFEBABE, 3, 60, 20, 30);
    // Add some payload bytes after the header (placeholder for the
    // bone-track data that the skeleton doesn't decode yet).
    for (int i = 0; i < 100; ++i) {
        buf.push_back(static_cast<std::uint8_t>(i & 0xFF));
    }
    ChrMotion m = ChrMotion::parse(buf);
    ASSERT_EQ(m.raw.size(), buf.size());
    EXPECT_EQ(m.raw.size(), kChrHeaderSize + 100u);
    // First 32 bytes are the header verbatim
    for (std::size_t i = 0; i < kChrHeaderSize; ++i) {
        EXPECT_EQ(m.raw[i], buf[i]) << "raw[" << i << "]";
    }
    // Payload bytes are preserved as-is
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(m.raw[kChrHeaderSize + i], static_cast<std::uint8_t>(i & 0xFF));
    }
}

TEST(ChrParseTest, ReturnsEmptyForBufferShorterThanHeader) {
    // Less than 32 bytes → is_chr fails → parse returns empty.
    std::vector<std::uint8_t> buf(16, 0);
    ChrMotion m = ChrMotion::parse(buf);
    EXPECT_TRUE(m.raw.empty());
    EXPECT_EQ(m.header.frame_count, 0u);
}

TEST(ChrParseTest, ReturnsEmptyForEmptyBuffer) {
    ChrMotion m = ChrMotion::parse(std::span<const std::uint8_t>{});
    EXPECT_TRUE(m.raw.empty());
}

}  // namespace mxh::compat::test
