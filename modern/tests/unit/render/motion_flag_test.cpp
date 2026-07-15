// motion_flag_test.cpp - Phase 10.20 motion flag bitmask tests
//
// Covers modern/include/mxh/render/motion_flag.hpp — the
// CMotionFlag class and its 3 enum constants (KEYFRAME, VERTEX,
// UV). The wire-format bit positions and mask values are 1:1
// with the original 4DyuchiGRX_common/motion_flag.h from the
// legacy 2003-era engine.
//
// What's tested:
//   - The 3 enum constants keep their bit-mask values exactly
//     as the original code expects.
//   - MASK / MASK_INVERSE constants are correct (XOR equals
//     all-ones for a full 32-bit mask).
//   - CMotionFlag getter/setter round-trip for all 3 fields.
//   - All 3 fields can coexist in the same flag word without
//     overlap.

#include "mxh/render/motion_flag.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::gx::test {

// ===========================================================================
// Enum constant wire-format values
// ===========================================================================

TEST(MotionKeyframeTest, WireFormatValues) {
    // Keyframe animation enable/disable. 1 bit in the low nibble.
    EXPECT_EQ(static_cast<std::uint32_t>(MOTION_TYPE_KEYFRAME_ENABLE),  0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(MOTION_TYPE_KEYFRAME_DISABLE), 0x00000001u);
}

TEST(MotionKeyframeTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(MOTION_TYPE_KEYFRAME_MASK ^ MOTION_TYPE_KEYFRAME_MASK_INVERSE, 0xFFFFFFFFu);
}

TEST(MotionVertexTest, WireFormatValues) {
    // Vertex animation enable/disable. 1 bit in the second nibble.
    EXPECT_EQ(static_cast<std::uint32_t>(MOTION_TYPE_VERTEX_DISABLE), 0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(MOTION_TYPE_VERTEX_ENABLE),  0x00000010u);
}

TEST(MotionVertexTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(MOTION_TYPE_VERTEX_MASK ^ MOTION_TYPE_VERTEX_MASK_INVERSE, 0xFFFFFFFFu);
}

TEST(MotionUvTest, WireFormatValues) {
    // UV animation enable/disable. 1 bit at position 8.
    EXPECT_EQ(static_cast<std::uint32_t>(MOTION_TYPE_UV_DISABLE), 0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(MOTION_TYPE_UV_ENABLE),  0x00000100u);
}

TEST(MotionUvTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(MOTION_TYPE_UV_MASK ^ MOTION_TYPE_UV_MASK_INVERSE, 0xFFFFFFFFu);
}

// ===========================================================================
// CMotionFlag
// ===========================================================================

TEST(CMotionFlagTest, DefaultConstructionIsZero) {
    mxh::gx::CMotionFlag f;
    EXPECT_EQ(f.GetRaw(), 0u);
    EXPECT_EQ(f.GetMotionTypeKeyFrame(), MOTION_TYPE_KEYFRAME_ENABLE);
    EXPECT_EQ(f.GetMotionTypeVertex(),   MOTION_TYPE_VERTEX_DISABLE);
    EXPECT_EQ(f.GetMotionTypeUV(),       MOTION_TYPE_UV_DISABLE);
}

TEST(CMotionFlagTest, KeyframeRoundTrip) {
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME_DISABLE);
    EXPECT_EQ(f.GetMotionTypeKeyFrame(), MOTION_TYPE_KEYFRAME_DISABLE);
    EXPECT_EQ(f.GetRaw(), 0x00000001u);
    f.SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME_ENABLE);
    EXPECT_EQ(f.GetMotionTypeKeyFrame(), MOTION_TYPE_KEYFRAME_ENABLE);
    EXPECT_EQ(f.GetRaw(), 0u);
}

TEST(CMotionFlagTest, VertexRoundTrip) {
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_ENABLE);
    EXPECT_EQ(f.GetMotionTypeVertex(), MOTION_TYPE_VERTEX_ENABLE);
    EXPECT_EQ(f.GetRaw(), 0x00000010u);
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_DISABLE);
    EXPECT_EQ(f.GetMotionTypeVertex(), MOTION_TYPE_VERTEX_DISABLE);
    EXPECT_EQ(f.GetRaw(), 0u);
}

TEST(CMotionFlagTest, UvRoundTrip) {
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeUV(MOTION_TYPE_UV_ENABLE);
    EXPECT_EQ(f.GetMotionTypeUV(), MOTION_TYPE_UV_ENABLE);
    EXPECT_EQ(f.GetRaw(), 0x00000100u);
    f.SetMotionTypeUV(MOTION_TYPE_UV_DISABLE);
    EXPECT_EQ(f.GetMotionTypeUV(), MOTION_TYPE_UV_DISABLE);
    EXPECT_EQ(f.GetRaw(), 0u);
}

TEST(CMotionFlagTest, AllFieldsCanCoexist) {
    // Set all 3 fields to non-default values and verify they
    // compose without overlap.
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME_DISABLE);  // bit 0
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_ENABLE);        // bit 4
    f.SetMotionTypeUV(MOTION_TYPE_UV_ENABLE);                // bit 8
    EXPECT_EQ(f.GetRaw(), 0x00000111u);
    EXPECT_EQ(f.GetMotionTypeKeyFrame(), MOTION_TYPE_KEYFRAME_DISABLE);
    EXPECT_EQ(f.GetMotionTypeVertex(),   MOTION_TYPE_VERTEX_ENABLE);
    EXPECT_EQ(f.GetMotionTypeUV(),       MOTION_TYPE_UV_ENABLE);
}

TEST(CMotionFlagTest, SettingKeyframeDoesNotCorruptVertex) {
    // Setting a low-bit field must not touch the higher bits.
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_ENABLE);
    f.SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME_DISABLE);
    EXPECT_EQ(f.GetMotionTypeVertex(),   MOTION_TYPE_VERTEX_ENABLE);
    EXPECT_EQ(f.GetMotionTypeKeyFrame(), MOTION_TYPE_KEYFRAME_DISABLE);
}

TEST(CMotionFlagTest, SettingUvDoesNotCorruptKeyframe) {
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME_DISABLE);
    f.SetMotionTypeUV(MOTION_TYPE_UV_ENABLE);
    EXPECT_EQ(f.GetMotionTypeKeyFrame(), MOTION_TYPE_KEYFRAME_DISABLE);
    EXPECT_EQ(f.GetMotionTypeUV(),       MOTION_TYPE_UV_ENABLE);
}

TEST(CMotionFlagTest, ReEnableVertexAfterDisableClearsBit) {
    // The setter uses (m_dwFlag & MASK_INVERSE) | t, so
    // setting back to DISABLE (0) must clear the bit, not
    // leave a stale value.
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_ENABLE);
    EXPECT_EQ(f.GetRaw(), 0x00000010u);
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_DISABLE);
    EXPECT_EQ(f.GetRaw(), 0u);
    EXPECT_EQ(f.GetMotionTypeVertex(), MOTION_TYPE_VERTEX_DISABLE);
}

TEST(CMotionFlagTest, GetRawMatchesEnumBitwiseOr) {
    // Sanity: GetRaw() should equal the bitwise OR of the
    // raw enum values of the three fields.
    mxh::gx::CMotionFlag f;
    f.SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME_DISABLE);
    f.SetMotionTypeVertex(MOTION_TYPE_VERTEX_ENABLE);
    f.SetMotionTypeUV(MOTION_TYPE_UV_ENABLE);
    std::uint32_t expected =
        static_cast<std::uint32_t>(MOTION_TYPE_KEYFRAME_DISABLE) |
        static_cast<std::uint32_t>(MOTION_TYPE_VERTEX_ENABLE) |
        static_cast<std::uint32_t>(MOTION_TYPE_UV_ENABLE);
    EXPECT_EQ(f.GetRaw(), expected);
}

}  // namespace mxh::gx::test
