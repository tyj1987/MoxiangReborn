// mesh_flag_test.cpp - Phase 10.18 render flag bitmask tests
//
// Covers modern/include/mxh/render/mesh_flag.hpp — the 3 bitmask
// flag classes (CMeshFlag, CLightFlag, CCameraFlag) and their
// associated enum constants. The wire-format bit positions and
// mask values are 1:1 with the original 4DyuchiGRX_common/mesh_flag.h
// from the legacy 2003-era engine.
//
// What's tested:
//   - The 5 enum constants (SHADE_TYPE, TRANSFORM_TYPE, RIGID_TYPE,
//     PICK_ENABLE_TYPE, DYNAMIC_LIGHT_APPLY_TYPE) keep their
//     bit-mask values exactly as the original code expects.
//   - The MASK / MASK_INVERSE constants are correct (XOR equals
//     all-ones for a full 32-bit mask).
//   - CMeshFlag getter/setter round-trip for all 5 fields.
//   - CLightFlag dynamic-light setter round-trip.
//   - CCameraFlag is a 4-byte class with no public API (data type
//     only); just pin its size.
//
// What's NOT tested (and is covered by other test files):
//   - The actual DirectX 11 render path. CMeshFlag is consumed by
//     mxh::render::dx11::mesh_shaders.cpp etc. and exercised by
//     the render tests in tests/unit/render/.
//
// Collateral finding (no source change):
//   - GetRenderZPriorityValue / SetRenderZPriorityValue are
//     declared in mesh_flag.hpp but no .cpp file implements them.
//     Calling them would be a link error. The test does NOT
//     exercise them; if a future change adds the implementation
//     the test will need to cover that pair.

#include "mxh/render/mesh_flag.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace mxh::gx::test {

// ===========================================================================
// Enum constant wire-format values
// ===========================================================================

TEST(ShadeTypeTest, WireFormatValues) {
    // The original 4DyuchiGRX_common/mesh_flag.h uses these exact
    // values. A future change that re-orders or re-numbers the
    // shade types would break every legacy mesh file.
    EXPECT_EQ(static_cast<std::uint32_t>(SHADE_TYPE_VERTEX_LIGHT_IM), 0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(SHADE_TYPE_VERTEX_LIGHT_RT), 0x00000001u);
    EXPECT_EQ(static_cast<std::uint32_t>(SHADE_TYPE_LIGHT_MAP),       0x00000003u);
}

TEST(ShadeTypeTest, MaskAndInverseCoverAll32Bits) {
    // A mask + its inverse must XOR to all-ones (every bit is
    // covered by exactly one of the two).
    EXPECT_EQ(SHADE_TYPE_MASK ^ SHADE_TYPE_MASK_INVERSE, 0xFFFFFFFFu);
}

TEST(TransformTypeTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(TRANSFORM_TYPE_SOLID),       0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(TRANSFORM_TYPE_NOT_SOLID),   0x00000010u);
    EXPECT_EQ(static_cast<std::uint32_t>(TRANSFORM_TYPE_ALIGN_VIEW),  0x00000030u);
    EXPECT_EQ(static_cast<std::uint32_t>(TRANSFORM_TYPE_ILLUSION),    0x00000050u);
}

TEST(TransformTypeTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(TRANSFORM_TYPE_MASK ^ TRANSFORM_TYPE_MASK_INVERSE, 0xFFFFFFFFu);
}

TEST(RigidTypeTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(RIGID_TYPE_NOT_RIGID), 0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(RIGID_TYPE_RIGID),     0x00000100u);
}

TEST(RigidTypeTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(RIGID_TYPE_MASK ^ RIGID_TYPE_MASK_INVERSE, 0xFFFFFFFFu);
}

TEST(PickEnableTypeTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(PICK_ENABLE),  0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(PICK_DISABLE), 0x00000200u);
}

TEST(PickEnableTypeTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(PICK_ENABLE_TYPE_MASK ^ PICK_ENABLE_TYPE_MASK_INVERSE, 0xFFFFFFFFu);
}

TEST(DynamicLightApplyTypeTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(DYNAMIC_LIGHT_APPLY_TYPE_DISABLE),          0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(DYNAMIC_LIGHT_APPLY_TYPE_CHARACTER_ENABLE), 0x00000001u);
    // Wire-format quirk: the original enum says MAP_ENABLE = 0x2 even
    // though its mask is 0x0000000f (4 bits). Pin both values
    // explicitly so a future "fix" that re-numbers the bits shows
    // up here as a deliberate test update rather than a silent
    // change.
    EXPECT_EQ(static_cast<std::uint32_t>(DYNAMIC_LIGHT_APPLY_TYPE_MAP_ENABLE),       0x00000002u);
    EXPECT_EQ(static_cast<std::uint32_t>(DYNAMIC_LIGHT_APPLY_TYPE_BOTH_ENABLE),      0x00000003u);
}

TEST(DynamicLightApplyTypeTest, MaskAndInverseCoverAll32Bits) {
    EXPECT_EQ(DYNAMIC_LIGHT_APPLY_TYPE_MASK ^ DYNAMIC_LIGHT_APPLY_TYPE_MASK_INVERSE, 0xFFFFFFFFu);
}

// ===========================================================================
// Z-priority + write-Z-buffer global constants
// ===========================================================================

TEST(RenderZPriorityTest, ConstantsArePinned) {
    // The Z-priority is encoded in the upper bits of the flag word
    // (bits 24..30, with the sign bit at 31 reserved for Z-buffer
    // write disable). A future change to these constants would
    // break every depth-sorted render call.
    EXPECT_EQ(RENDER_ZPRIORITY_DEFAULT, 0);
    EXPECT_FLOAT_EQ(RENDER_ZPRIORITY_UNIT, -10.0f);
    EXPECT_EQ(RENDER_ZPRIORITY_MASK,         0x7f000000u);
    EXPECT_EQ(RENDER_ZPRIORITY_MASK_INVERSE, 0x80ffffffu);
    EXPECT_EQ(WRITE_ZBUFFER_MASK,             0x80000000u);
    EXPECT_EQ(WRITE_ZBUFFER_MASK_INVERSE,    0x7fffffffu);
}

TEST(RenderZPriorityTest, ZPriorityAndZWriteUseDistinctBits) {
    // RENDER_ZPRIORITY_MASK = 0x7F000000 uses bits 24..30 (the
    // high 7 bits of byte 3). WRITE_ZBUFFER_MASK = 0x80000000
    // uses bit 31 only. They do NOT overlap — the Z-priority
    // value and the Z-write-disable flag can be set
    // independently.
    EXPECT_EQ((RENDER_ZPRIORITY_MASK & WRITE_ZBUFFER_MASK), 0u);
    // The Z-priority field's value range is 0x00..0x7F (7 bits
    // = 0x7F000000 >> 24). The sign bit of the upper byte is the
    // Z-write flag, not a sign extension of the priority.
    EXPECT_EQ((RENDER_ZPRIORITY_MASK >> 24), 0x7Fu);
}

TEST(RenderZPriorityTest, ZPriorityMaskInverseOverlapsBit31) {
    // Wire-format quirk: RENDER_ZPRIORITY_MASK = 0x7F000000
    // (bits 24..30 set, bit 31 clear). Its INVERSE is 0x80FFFFFF
    // — bit 31 IS set in the inverse. That bit is the
    // write-Z-buffer flag (WRITE_ZBUFFER_MASK = 0x80000000).
    // So `RENDER_ZPRIORITY_MASK_INVERSE & WRITE_ZBUFFER_MASK ==
    // 0x80000000` (non-zero). When a caller does
    // `flag = (flag & RENDER_ZPRIORITY_MASK_INVERSE) | new_prio`,
    // the Z-write-disable bit (bit 31) is preserved unchanged.
    // This is a legacy design quirk — pinning it so a future
    // "fix" that tightens the inverse mask shows up here as a
    // deliberate test update rather than a silent behaviour
    // change.
    EXPECT_EQ(RENDER_ZPRIORITY_MASK_INVERSE, 0x80FFFFFFu);
    EXPECT_EQ((RENDER_ZPRIORITY_MASK_INVERSE & WRITE_ZBUFFER_MASK),
              0x80000000u);
}

TEST(RenderZPriorityTest, MasksAndInversesCoverAll32Bits) {
    EXPECT_EQ(RENDER_ZPRIORITY_MASK ^ RENDER_ZPRIORITY_MASK_INVERSE, 0xFFFFFFFFu);
    EXPECT_EQ(WRITE_ZBUFFER_MASK ^ WRITE_ZBUFFER_MASK_INVERSE,         0xFFFFFFFFu);
}

// ===========================================================================
// CMeshFlag
// ===========================================================================

TEST(CMeshFlagTest, DefaultConstructionIsZero) {
    mxh::gx::CMeshFlag f;
    EXPECT_EQ(f.GetRaw(), 0u);
    EXPECT_EQ(static_cast<std::uint32_t>(f.GetShadeType()),       0u);
    EXPECT_EQ(static_cast<std::uint32_t>(f.GetTransformType()),   0u);
    EXPECT_EQ(static_cast<std::uint32_t>(f.GetRigidType()),       0u);
    EXPECT_EQ(static_cast<std::uint32_t>(f.GetPickEnable()),     0u);
    EXPECT_FALSE(f.IsDisableZBubfferWrite());
}

TEST(CMeshFlagTest, ConstructionWithRawValue) {
    mxh::gx::CMeshFlag f(0xCAFEBABEu);
    EXPECT_EQ(f.GetRaw(), 0xCAFEBABEu);
}

TEST(CMeshFlagTest, SetRawReplacesAllFields) {
    mxh::gx::CMeshFlag f;
    f.SetShadeType(SHADE_TYPE_LIGHT_MAP);
    f.SetTransformType(TRANSFORM_TYPE_ILLUSION);
    f.SetRaw(0xDEADBEEFu);
    EXPECT_EQ(f.GetRaw(), 0xDEADBEEFu);
    // GetShadeType / GetTransformType return the raw bits AND-ed
    // with the field mask, so the expected values are the raw
    // mask values, not the enum constants. The enum constants
    // like TRANSFORM_TYPE_NOT_SOLID = 0x10 are values that fit
    // in the bit field; the mask is 0xF0.
    EXPECT_EQ(static_cast<std::uint32_t>(f.GetShadeType()),     0x0Fu);  // 0xDEADBEEF & SHADE_TYPE_MASK(0x0F) = 0xEF & 0x0F = 0x0F
    EXPECT_EQ(static_cast<std::uint32_t>(f.GetTransformType()), 0xE0u);  // 0xDEADBEEF & TRANSFORM_TYPE_MASK(0xF0) = 0xBE & 0xF0 = 0xB0... actually let me recompute: 0xDEADBEEF byte 0 = 0xEF, 0xEF & 0x0F = 0x0F (low nibble); byte 0 (high nibble) = 0xE, so 0xE << 4 = 0xE0
}

TEST(CMeshFlagTest, ShadeTypeRoundTrip) {
    mxh::gx::CMeshFlag f;
    f.SetShadeType(SHADE_TYPE_VERTEX_LIGHT_IM);
    EXPECT_EQ(f.GetShadeType(), SHADE_TYPE_VERTEX_LIGHT_IM);
    f.SetShadeType(SHADE_TYPE_VERTEX_LIGHT_RT);
    EXPECT_EQ(f.GetShadeType(), SHADE_TYPE_VERTEX_LIGHT_RT);
    f.SetShadeType(SHADE_TYPE_LIGHT_MAP);
    EXPECT_EQ(f.GetShadeType(), SHADE_TYPE_LIGHT_MAP);
}

TEST(CMeshFlagTest, TransformTypeRoundTrip) {
    mxh::gx::CMeshFlag f;
    for (auto t : {TRANSFORM_TYPE_SOLID, TRANSFORM_TYPE_NOT_SOLID,
                   TRANSFORM_TYPE_ALIGN_VIEW, TRANSFORM_TYPE_ILLUSION}) {
        f.SetTransformType(t);
        EXPECT_EQ(f.GetTransformType(), t);
    }
}

TEST(CMeshFlagTest, RigidTypeRoundTrip) {
    mxh::gx::CMeshFlag f;
    f.SetRigidType(RIGID_TYPE_NOT_RIGID);
    EXPECT_EQ(f.GetRigidType(), RIGID_TYPE_NOT_RIGID);
    f.SetRigidType(RIGID_TYPE_RIGID);
    EXPECT_EQ(f.GetRigidType(), RIGID_TYPE_RIGID);
}

TEST(CMeshFlagTest, PickEnableRoundTrip) {
    mxh::gx::CMeshFlag f;
    f.SetPickEnable(PICK_ENABLE);
    EXPECT_EQ(f.GetPickEnable(), PICK_ENABLE);
    f.SetPickEnable(PICK_DISABLE);
    EXPECT_EQ(f.GetPickEnable(), PICK_DISABLE);
}

TEST(CMeshFlagTest, ZBufferWriteDefaultIsEnabled) {
    mxh::gx::CMeshFlag f;
    EXPECT_FALSE(f.IsDisableZBubfferWrite());
    EXPECT_EQ(f.GetRaw(), 0u);  // Z write enabled = bit 31 clear = 0
}

TEST(CMeshFlagTest, DisableZBufferWriteSetsBit31) {
    mxh::gx::CMeshFlag f;
    f.DisableZBufferWrite();
    EXPECT_TRUE(f.IsDisableZBubfferWrite());
    EXPECT_EQ(f.GetRaw(), 0x80000000u);
}

TEST(CMeshFlagTest, EnableZBufferWriteClearsBit31) {
    mxh::gx::CMeshFlag f(0xFFFFFFFFu);
    f.EnableZBufferWrite();
    EXPECT_FALSE(f.IsDisableZBubfferWrite());
    EXPECT_EQ(f.GetRaw(), 0x7FFFFFFFu);
}

TEST(CMeshFlagTest, ZBufferWriteDoesNotCorruptShadeType) {
    // Setting DisableZBufferWrite should not affect the shade
    // type (they live in different bit ranges).
    mxh::gx::CMeshFlag f;
    f.SetShadeType(SHADE_TYPE_LIGHT_MAP);
    f.DisableZBufferWrite();
    EXPECT_EQ(f.GetShadeType(), SHADE_TYPE_LIGHT_MAP);
    EXPECT_TRUE(f.IsDisableZBubfferWrite());
}

TEST(CMeshFlagTest, ShadeTypeDoesNotCorruptZBufferWrite) {
    // Symmetric to the above — setting the shade type must not
    // flip the Z-buffer write bit.
    mxh::gx::CMeshFlag f(0x80000000u);  // start with Z write disabled
    f.SetShadeType(SHADE_TYPE_LIGHT_MAP);
    EXPECT_TRUE(f.IsDisableZBubfferWrite());
    EXPECT_EQ(f.GetShadeType(), SHADE_TYPE_LIGHT_MAP);
}

TEST(CMeshFlagTest, AllFieldsCanCoexist) {
    // Set every field to a non-default value, verify the bits
    // compose without overlap.
    mxh::gx::CMeshFlag f;
    f.SetShadeType(SHADE_TYPE_LIGHT_MAP);           // bits 0..3 = 0x3
    f.SetTransformType(TRANSFORM_TYPE_ILLUSION);    // bits 4..7 = 0x5
    f.SetRigidType(RIGID_TYPE_RIGID);                // bit  8    = 0x1
    f.SetPickEnable(PICK_DISABLE);                  // bit  9    = 0x1
    f.DisableZBufferWrite();                        // bit 31    = 0x1
    EXPECT_EQ(f.GetRaw(),
              0x00000003u | 0x00000050u | 0x00000100u | 0x00000200u | 0x80000000u);
    EXPECT_EQ(f.GetShadeType(),     SHADE_TYPE_LIGHT_MAP);
    EXPECT_EQ(f.GetTransformType(), TRANSFORM_TYPE_ILLUSION);
    EXPECT_EQ(f.GetRigidType(),     RIGID_TYPE_RIGID);
    EXPECT_EQ(f.GetPickEnable(),   PICK_DISABLE);
    EXPECT_TRUE(f.IsDisableZBubfferWrite());
}

// ===========================================================================
// CLightFlag
// ===========================================================================

TEST(CLightFlagTest, DefaultConstructionIsZero) {
    mxh::gx::CLightFlag f;
    EXPECT_EQ(f.GetDynamicLightType(), DYNAMIC_LIGHT_APPLY_TYPE_DISABLE);
}

TEST(CLightFlagTest, DynamicLightTypeRoundTrip) {
    mxh::gx::CLightFlag f;
    for (auto t : {DYNAMIC_LIGHT_APPLY_TYPE_DISABLE,
                   DYNAMIC_LIGHT_APPLY_TYPE_CHARACTER_ENABLE,
                   DYNAMIC_LIGHT_APPLY_TYPE_MAP_ENABLE,
                   DYNAMIC_LIGHT_APPLY_TYPE_BOTH_ENABLE}) {
        f.SetDynamicLightType(t);
        EXPECT_EQ(f.GetDynamicLightType(), t);
    }
}

// ===========================================================================
// CCameraFlag
// ===========================================================================

TEST(CCameraFlagTest, IsUint32Sized) {
    // CCameraFlag wraps a single uint32_t m_dwFlag. Pin its size
    // so a future field addition (e.g. an enable-bool) shows up
    // here as a deliberate test update.
    static_assert(sizeof(mxh::gx::CCameraFlag) == 4,
                  "CCameraFlag should be exactly 4 bytes (one uint32_t)");
    EXPECT_EQ(sizeof(mxh::gx::CCameraFlag), sizeof(std::uint32_t));
}

}  // namespace mxh::gx::test
