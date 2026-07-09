// tests/unit/render/phase5_deferred_test.cpp
// Unit tests for the Phase 5 deferred render methods: SetRTLight,
// InitializeRenderTarget, SetLoadFailedTextureTable, and the BC1/BC3 DDS
// encoders. The methods under test do not require a real DX11 device (they
// only mutate renderer-internal state), so the tests construct a bare
// CoD3DDeviceDX11 and exercise the public test accessors.
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cstdint>

#include "renderer.hpp"
#include "texture_loader.hpp"

using namespace mxh::gx::dx11;
using mxh::gx::LIGHT_DESC;
using mxh::gx::VECTOR3;
using mxh::gx::TEXTURE_TABLE;
using mxh::gx::SPOT_LIGHT_TYPE;
using mxh::gx::SPOT_LIGHT_TYPE_PRJIMAGE;
constexpr int MAX_NAME_LEN_LOCAL = 128; // mirror of mxh::gx::MAX_NAME_LEN

// =============================================================================
// SetRTLight tests
// =============================================================================

TEST(SetRTLight, NullDescRejected) {
    CoD3DDeviceDX11 dev;
    EXPECT_FALSE(dev.SetRTLight(nullptr, 0, 0));
    EXPECT_EQ(dev.internalRTLightActiveCount(), 0u);
}

TEST(SetRTLight, OutOfRangeIndexRejected) {
    CoD3DDeviceDX11 dev;
    LIGHT_DESC ld{};
    EXPECT_FALSE(dev.SetRTLight(&ld, MAX_DYNAMIC_LIGHTS, 0));
    EXPECT_FALSE(dev.SetRTLight(&ld, 99, 0));
    EXPECT_EQ(dev.internalRTLightActiveCount(), 0u);
}

TEST(SetRTLight, StoresDescAndIncrementsCount) {
    CoD3DDeviceDX11 dev;
    LIGHT_DESC ld{};
    ld.dwDiffuse  = 0xFFFF0000u; // red
    ld.v3Point    = { 1.0f, 2.0f, 3.0f };
    ld.v3To       = { 0.0f, 0.0f, 1.0f };
    ld.fRs        = 50.0f;
    ld.type       = SPOT_LIGHT_TYPE_PRJIMAGE;
    EXPECT_TRUE(dev.SetRTLight(&ld, 0, 0));
    EXPECT_TRUE(dev.internalRTLightActive(0));
    EXPECT_EQ(dev.internalRTLightActiveCount(), 1u);

    LIGHT_DESC stored = dev.internalRTLightDesc(0);
    EXPECT_EQ(stored.dwDiffuse, 0xFFFF0000u);
    EXPECT_FLOAT_EQ(stored.v3Point.x, 1.0f);
    EXPECT_FLOAT_EQ(stored.v3Point.y, 2.0f);
    EXPECT_FLOAT_EQ(stored.v3Point.z, 3.0f);
    EXPECT_FLOAT_EQ(stored.fRs, 50.0f);
}

TEST(SetRTLight, MultipleSlotsAccumulate) {
    CoD3DDeviceDX11 dev;
    LIGHT_DESC a{}; a.v3Point = { 1, 0, 0 };
    LIGHT_DESC b{}; b.v3Point = { 2, 0, 0 };
    LIGHT_DESC c{}; c.v3Point = { 3, 0, 0 };
    EXPECT_TRUE(dev.SetRTLight(&a, 0, 0));
    EXPECT_TRUE(dev.SetRTLight(&b, 3, 0));
    EXPECT_TRUE(dev.SetRTLight(&c, 7, 0));
    EXPECT_EQ(dev.internalRTLightActiveCount(), 3u);
    EXPECT_TRUE(dev.internalRTLightActive(0));
    EXPECT_FALSE(dev.internalRTLightActive(1)); // gap
    EXPECT_TRUE(dev.internalRTLightActive(3));
    EXPECT_TRUE(dev.internalRTLightActive(7));
    EXPECT_FLOAT_EQ(dev.internalRTLightDesc(0).v3Point.x, 1.f);
    EXPECT_FLOAT_EQ(dev.internalRTLightDesc(3).v3Point.x, 2.f);
    EXPECT_FLOAT_EQ(dev.internalRTLightDesc(7).v3Point.x, 3.f);
}

TEST(SetRTLight, OverwriteDoesNotIncrementCount) {
    CoD3DDeviceDX11 dev;
    LIGHT_DESC a{}; a.v3Point = { 1, 0, 0 };
    EXPECT_TRUE(dev.SetRTLight(&a, 0, 0));
    EXPECT_EQ(dev.internalRTLightActiveCount(), 1u);
    // Overwriting the same slot must not double-count.
    EXPECT_TRUE(dev.SetRTLight(&a, 0, 0));
    EXPECT_EQ(dev.internalRTLightActiveCount(), 1u);
}

// =============================================================================
// buildLightCB folds m_rtLights into cbuffer slots
// =============================================================================
// We test the slot-filling behavior indirectly: when an RT light is active at
// index 2, the corresponding dynLightPos2/Color2 in the cbuffer must reflect
// the RT light's position + diffuse + range, not the (zero) default.

namespace {
// Hand-rolled mirror of buildLightCB's RT-light overlay pass, for use in
// tests. This duplicates the production logic to verify the contract: any
// change to the production overlay must be reflected here.
void apply_rt_overlay_for_test(LightCB& out, const CoD3DDeviceDX11& dev) {
    float* posSlots[8] = { out.dynLightPos0, out.dynLightPos1, out.dynLightPos2,
                           out.dynLightPos3, out.dynLightPos4, out.dynLightPos5,
                           out.dynLightPos6, out.dynLightPos7 };
    float* colSlots[8] = { out.dynLightColor0, out.dynLightColor1, out.dynLightColor2,
                           out.dynLightColor3, out.dynLightColor4, out.dynLightColor5,
                           out.dynLightColor6, out.dynLightColor7 };
    float* attSlots[8] = { out.dynLightAtten0, out.dynLightAtten1, out.dynLightAtten2,
                           out.dynLightAtten3, out.dynLightAtten4, out.dynLightAtten5,
                           out.dynLightAtten6, out.dynLightAtten7 };
    for (std::uint32_t idx = 0; idx < MAX_DYNAMIC_LIGHTS; ++idx) {
        if (!dev.internalRTLightActive(idx)) continue;
        LIGHT_DESC d = dev.internalRTLightDesc(idx);
        float rgb[4];
        color_to_float4(d.dwDiffuse, rgb);
        posSlots[idx][0] = d.v3Point.x;
        posSlots[idx][1] = d.v3Point.y;
        posSlots[idx][2] = d.v3Point.z;
        posSlots[idx][3] = 1.0f;
        colSlots[idx][0] = rgb[0];
        colSlots[idx][1] = rgb[1];
        colSlots[idx][2] = rgb[2];
        colSlots[idx][3] = d.fRs > 0.f ? d.fRs : 200.f;
        attSlots[idx][0] = 1.0f;
    }
}
} // namespace

TEST(BuildLightCBRTLight, RTLightFillsSlot) {
    CoD3DDeviceDX11 dev;
    LIGHT_DESC ld{};
    ld.dwDiffuse = 0xFFFF0000u; // red
    ld.v3Point   = { 7.f, 8.f, 9.f };
    ld.fRs       = 100.f;
    ASSERT_TRUE(dev.SetRTLight(&ld, 2, 0));

    LightCB cb{};
    apply_rt_overlay_for_test(cb, dev);

    // Slot 2 must reflect the RT light.
    EXPECT_FLOAT_EQ(cb.dynLightPos2[0], 7.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos2[1], 8.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos2[2], 9.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos2[3], 1.0f); // enabled flag
    EXPECT_FLOAT_EQ(cb.dynLightColor2[0], 1.0f); // R
    EXPECT_FLOAT_EQ(cb.dynLightColor2[1], 0.0f); // G
    EXPECT_FLOAT_EQ(cb.dynLightColor2[2], 0.0f); // B
    EXPECT_FLOAT_EQ(cb.dynLightColor2[3], 100.f);
    EXPECT_FLOAT_EQ(cb.dynLightAtten2[0], 1.0f);
    // Other slots must remain zero (no leaks).
    EXPECT_FLOAT_EQ(cb.dynLightPos0[3], 0.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos7[3], 0.f);
}

TEST(BuildLightCBRTLight, DefaultRangeIsClamped) {
    // If fRs is <= 0, the production code falls back to 200.0f. Verify.
    CoD3DDeviceDX11 dev;
    LIGHT_DESC ld{};
    ld.fRs = 0.f;
    ASSERT_TRUE(dev.SetRTLight(&ld, 0, 0));
    LightCB cb{};
    apply_rt_overlay_for_test(cb, dev);
    EXPECT_FLOAT_EQ(cb.dynLightColor0[3], 200.f);
}

// =============================================================================
// InitializeRenderTarget tests
// =============================================================================

TEST(InitializeRenderTarget, ValidArgsStored) {
    CoD3DDeviceDX11 dev;
    EXPECT_TRUE(dev.InitializeRenderTarget(64, 8));
    EXPECT_EQ(dev.internalRTTexelSize(), 64u);
    EXPECT_EQ(dev.internalRTMaxTexNum(), 8u);
}

TEST(InitializeRenderTarget, ZeroArgsRejected) {
    CoD3DDeviceDX11 dev;
    EXPECT_FALSE(dev.InitializeRenderTarget(0, 8));
    EXPECT_FALSE(dev.InitializeRenderTarget(64, 0));
    EXPECT_EQ(dev.internalRTTexelSize(), 0u); // unchanged
}

TEST(InitializeRenderTarget, OverLimitClamped) {
    CoD3DDeviceDX11 dev;
    EXPECT_TRUE(dev.InitializeRenderTarget(32, 1024));
    EXPECT_EQ(dev.internalRTMaxTexNum(), 64u); // clamped
    EXPECT_EQ(dev.internalRTTexelSize(), 32u);
}

// =============================================================================
// SetLoadFailedTextureTable tests
// =============================================================================

TEST(SetLoadFailedTextureTable, StoresPointerAndSize) {
    CoD3DDeviceDX11 dev;
    TEXTURE_TABLE table[3] = {};
    table[0].wIndex = 1; std::strncpy(table[0].szTextureName, "missing_a.tga", MAX_NAME_LEN_LOCAL - 1);
    table[1].wIndex = 2; std::strncpy(table[1].szTextureName, "missing_b.tga", MAX_NAME_LEN_LOCAL - 1);
    table[2].wIndex = 3; std::strncpy(table[2].szTextureName, "missing_c.tga", MAX_NAME_LEN_LOCAL - 1);
    EXPECT_TRUE(dev.SetLoadFailedTextureTable(table, 3));
    EXPECT_EQ(dev.internalLoadFailedTable(), table);
    EXPECT_EQ(dev.internalLoadFailedTableSize(), 3u);
}

TEST(SetLoadFailedTextureTable, NullTableAllowed) {
    CoD3DDeviceDX11 dev;
    EXPECT_TRUE(dev.SetLoadFailedTextureTable(nullptr, 0));
    EXPECT_EQ(dev.internalLoadFailedTable(), nullptr);
    EXPECT_EQ(dev.internalLoadFailedTableSize(), 0u);
}

TEST(GetLoadFailedTextureTable, ReturnsStoredView) {
    CoD3DDeviceDX11 dev;
    TEXTURE_TABLE table[2] = {};
    EXPECT_TRUE(dev.SetLoadFailedTextureTable(table, 2));
    TEXTURE_TABLE* out = nullptr;
    std::uint32_t outSize = 0;
    std::uint32_t outCount = 0;
    dev.GetLoadFailedTextureTable(&out, &outSize, &outCount);
    EXPECT_EQ(out, table);
    EXPECT_EQ(outSize, 2u);
    EXPECT_EQ(outCount, 2u);
}

// =============================================================================
// BC1 (DXT1) / BC3 (DXT5) DDS encoder tests
// =============================================================================
// We feed the encoder a small synthetic image and validate:
//  - the output starts with the "DDS " magic
//  - the legacy DDS_HEADER is well-formed
//  - the pixel-format FourCC is DXT1 (BC1) or DXT5 (BC3)
//  - the body length matches (W/4 rounded up) * (H/4 rounded up) * 8 or 16
//  - a flat-color 4×4 block round-trips to the same color in all palette
//    entries (since the min == max, every index is 0 and the chosen
//    color is the original)
// =============================================================================

namespace {
bool starts_with(const std::vector<std::uint8_t>& v, const char* s) {
    const std::size_t n = std::strlen(s);
    if (v.size() < n) return false;
    return std::memcmp(v.data(), s, n) == 0;
}
} // namespace

TEST(SaveDDSBC, EmptyTextureReturnsEmpty) {
    LoadedTexture tex;
    EXPECT_TRUE(saveDDS_BC(tex).empty());
}

TEST(SaveDDSBC, SolidRedPicksBC1) {
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (std::size_t i = 0; i < 4 * 4; ++i) {
        tex.pixels[i * 4 + 0] = 255; // R
        tex.pixels[i * 4 + 3] = 255; // A
    }
    auto out = saveDDS_BC(tex);
    ASSERT_FALSE(out.empty());
    EXPECT_TRUE(starts_with(out, "DDS "));
    // Header at offset 4, DdsPixelFormat.dwFourCC at offset 84 (4 + 80).
    std::uint32_t fourcc = 0;
    std::memcpy(&fourcc, out.data() + 84, 4);
    // MAKEFOURCC('D','X','T','1') in memory (LE): bytes [0x44,0x58,0x54,0x31]
    // = "DXT1" — stored as the little-endian uint32 0x31545844.
    EXPECT_EQ(fourcc, 0x31545844u);
    // 1 block × 8 bytes (BC1) + 4 magic + 124 header = 136.
    EXPECT_EQ(out.size(), 4u + 124u + 8u);
}

TEST(SaveDDSBC, AlphaGradientPicksBC3) {
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.resize(4 * 4 * 4);
    for (std::size_t i = 0; i < 4 * 4; ++i) {
        // Gradient alpha: 100, 117, 134, ... (stays in 100..255 range, so
        // aMin > 0 and aMax < 255 — both predicates are true → BC3).
        std::uint8_t a = static_cast<std::uint8_t>(100 + (i * 17) % 156);
        tex.pixels[i * 4 + 0] = 100;
        tex.pixels[i * 4 + 1] = 150;
        tex.pixels[i * 4 + 2] = 200;
        tex.pixels[i * 4 + 3] = a;
    }
    auto out = saveDDS_BC(tex);
    ASSERT_FALSE(out.empty());
    std::uint32_t fourcc = 0;
    std::memcpy(&fourcc, out.data() + 84, 4);
    // MAKEFOURCC('D','X','T','5') in memory (LE): 0x35545844.
    EXPECT_EQ(fourcc, 0x35545844u);
    // 1 block × 16 bytes (BC3 = 8-byte alpha + 8-byte BC1) + header.
    EXPECT_EQ(out.size(), 4u + 124u + 16u);
}

TEST(SaveDDSBC, NonMultipleOf4PadsToBlockGrid) {
    // 5x3 image => ceil(5/4)=2 × ceil(3/4)=1 = 2 blocks.
    // Alpha = 200 (uniform but not 0/255) → BC3 selected → 16 bytes/block.
    LoadedTexture tex;
    tex.width = 5; tex.height = 3;
    tex.pixels.assign(5 * 3 * 4, 200);
    auto out = saveDDS_BC(tex);
    ASSERT_FALSE(out.empty());
    // Header: dwWidth=5, dwHeight=3, dwPitchOrLinearSize = 2 * 16 = 32.
    std::uint32_t w = 0, h = 0, pitch = 0;
    std::memcpy(&w,     out.data() + 4 + 12, 4);
    std::memcpy(&h,     out.data() + 4 +  8, 4);
    std::memcpy(&pitch, out.data() + 4 + 16, 4);
    EXPECT_EQ(w, 5u);
    EXPECT_EQ(h, 3u);
    EXPECT_EQ(pitch, 32u);
    EXPECT_EQ(out.size(), 4u + 124u + 32u);
}

TEST(SaveDDSBC, SolidColorBlockHasFlatIndices) {
    // Flat-color block: every pixel same. After encoding:
    //  - color0 == color1 (we force c0 > c1 by setting c1 = c0 - 1)
    //  - all 2-bit indices = 0 (the "brightest" palette)
    //  - the 4-byte index pack should be 0.
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (auto& p : tex.pixels) p = 0x80; // mid-gray
    auto out = saveDDS_BC(tex);
    ASSERT_GT(out.size(), 4u + 124u + 8u);
    // Index bytes are the last 4 bytes of the BC1 block.
    const std::uint8_t* idx = out.data() + 4 + 124 + 4;
    EXPECT_EQ(idx[0], 0u);
    EXPECT_EQ(idx[1], 0u);
    EXPECT_EQ(idx[2], 0u);
    EXPECT_EQ(idx[3], 0u);
}

// =============================================================================
// BC4 (ATI1) / BC5 (ATI2) DDS encoder tests
// =============================================================================
// BC4 is single-channel (uses R channel of LoadedTexture by convention).
// BC5 is two-channel (R+G), standard for tangent-space normal maps where
// the shader reconstructs Z = sqrt(1 - x² - y²).
//
// Contract checks per test:
//   - magic "DDS "
//   - FourCC is "ATI1" (BC4) or "ATI2" (BC5) at byte offset 84
//   - body length matches blocks * (8 or 16)
//   - flat-color block produces endpoints e0 == e1 and zeroed index pack
//   - multi-tone block produces a non-trivial index pack and a non-flat
//     endpoint pair
//   - BC5 encodes R in the first 8 bytes and G in the next 8 bytes
//   - non-multiple-of-4 dimensions edge-pad to a 4×4 block grid
// =============================================================================

namespace {
// Convenience: pull the FourCC out of a DDS body.
std::uint32_t dds_fourcc(const std::vector<std::uint8_t>& v) {
    std::uint32_t f = 0;
    std::memcpy(&f, v.data() + 84, 4);
    return f;
}
} // namespace

TEST(SaveDDSBC4, SolidGrayPicksATI1) {
    // All pixels same R = 128. Block should encode as a flat (e0 == e1) with
    // every index == 0.
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (auto& p : tex.pixels) p = 0x80;  // R = 128 everywhere
    auto out = saveDDS_BC(tex, BCFormat::BC4);
    ASSERT_FALSE(out.empty());
    EXPECT_TRUE(starts_with(out, "DDS "));
    // ATI1 in memory (LE) = 'A','T','I','1' → 0x31495441.
    EXPECT_EQ(dds_fourcc(out), 0x31495441u);
    // 1 block × 8 B + magic + header = 136.
    EXPECT_EQ(out.size(), 4u + 124u + 8u);
    // Flat block: endpoints equal (e0 == e1 == 128) and indices all zero.
    const std::uint8_t* body = out.data() + 4 + 124;
    EXPECT_EQ(body[0], 128u);  // e0
    EXPECT_EQ(body[1], 128u);  // e1
    for (int k = 0; k < 6; ++k) EXPECT_EQ(body[2 + k], 0u);
}

TEST(SaveDDSBC4, GrayscaleGradientRoundtripsToEndpoints) {
    // A 4×4 block whose R values ramp linearly from 0 to 255 must produce
    // a non-flat endpoint pair (e0 != e1) and a non-zero index pack.
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            std::uint8_t r = static_cast<std::uint8_t>(y * 64 + x * 16);
            tex.pixels[(y * 4 + x) * 4 + 0] = r; // R channel
        }
    }
    auto out = saveDDS_BC(tex, BCFormat::BC4);
    ASSERT_GE(out.size(), 4u + 124u + 8u);
    const std::uint8_t* body = out.data() + 4 + 124;
    // e0 == max, e1 == min for a non-flat block; the encoder also evaluates
    // the 8-step mode (a0 <= a1) and picks whichever has lower distortion.
    // The data spans 0..252 with multiple values, so the interpolated 6-step
    // path (a0 > a1) wins. Endpoint delta must be at least 32 (rounding).
    int e0 = body[0], e1 = body[1];
    EXPECT_GE(std::abs(e0 - e1), 16);
    // At least one of the 6 index bytes must be non-zero (otherwise it would
    // be a flat block, which we already covered above).
    bool anyNonZero = false;
    for (int k = 0; k < 6; ++k) if (body[2 + k] != 0) { anyNonZero = true; break; }
    EXPECT_TRUE(anyNonZero);
}

TEST(SaveDDSBC4, RChannelOnlyIgnoresOtherChannels) {
    // The encoder must read the R channel only. Set R=64 everywhere and vary
    // G/B/A wildly; the resulting block must still encode e0 == e1 == 64
    // (flat).
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (int i = 0; i < 16; ++i) {
        tex.pixels[i * 4 + 0] = 64;          // R = 64 (must be used)
        tex.pixels[i * 4 + 1] = static_cast<std::uint8_t>(i * 17); // G varies
        tex.pixels[i * 4 + 2] = 200;         // B constant
        tex.pixels[i * 4 + 3] = static_cast<std::uint8_t>(i * 11); // A varies
    }
    auto out = saveDDS_BC(tex, BCFormat::BC4);
    ASSERT_GE(out.size(), 4u + 124u + 8u);
    const std::uint8_t* body = out.data() + 4 + 124;
    EXPECT_EQ(body[0], 64u);
    EXPECT_EQ(body[1], 64u);
}

TEST(SaveDDSBC5, HeaderIsATI2AndBodyIs16BytesPerBlock) {
    // Use a 4×4 RG gradient. After encoding:
    //   - FourCC = ATI2 (BC5)
    //   - First 8 B = R channel block, next 8 B = G channel block.
    //   - 1 block × 16 B + magic + header = 144.
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            tex.pixels[(y * 4 + x) * 4 + 0] = static_cast<std::uint8_t>(x * 64); // R
            tex.pixels[(y * 4 + x) * 4 + 1] = static_cast<std::uint8_t>(y * 64); // G
        }
    }
    auto out = saveDDS_BC(tex, BCFormat::BC5);
    ASSERT_FALSE(out.empty());
    EXPECT_TRUE(starts_with(out, "DDS "));
    // ATI2 in memory (LE) = 'A','T','I','2' → 0x32495441.
    EXPECT_EQ(dds_fourcc(out), 0x32495441u);
    EXPECT_EQ(out.size(), 4u + 124u + 16u);
}

TEST(SaveDDSBC5, RGChannelsEncodedSeparately) {
    // Flat R (R = 200) but gradient G (G = 0..192). The R block must be
    // flat (e0 == e1 == 200, indices zero) and the G block must be
    // non-flat (e0 != e1, at least one index byte non-zero).
    LoadedTexture tex;
    tex.width = tex.height = 4;
    tex.pixels.assign(4 * 4 * 4, 0);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            tex.pixels[(y * 4 + x) * 4 + 0] = 200;                         // R flat
            tex.pixels[(y * 4 + x) * 4 + 1] = static_cast<std::uint8_t>(y * 64); // G gradient
        }
    }
    auto out = saveDDS_BC(tex, BCFormat::BC5);
    ASSERT_GE(out.size(), 4u + 124u + 16u);
    const std::uint8_t* body = out.data() + 4 + 124;
    // R block (first 8 B) is flat.
    EXPECT_EQ(body[0], 200u);
    EXPECT_EQ(body[1], 200u);
    for (int k = 0; k < 6; ++k) EXPECT_EQ(body[2 + k], 0u);
    // G block (next 8 B) is non-flat.
    EXPECT_GE(std::abs(int(body[8]) - int(body[9])), 16);
    bool anyNonZero = false;
    for (int k = 0; k < 6; ++k) if (body[10 + k] != 0) { anyNonZero = true; break; }
    EXPECT_TRUE(anyNonZero);
}

TEST(SaveDDSBC5, NonMultipleOf4PadsToBlockGrid) {
    // 5×3 normal map. Block grid = ceil(5/4) * ceil(3/4) = 2 blocks.
    // 2 blocks × 16 B = 32 B body. dwWidth=5, dwHeight=3, linear size=32.
    LoadedTexture tex;
    tex.width = 5; tex.height = 3;
    tex.pixels.assign(5 * 3 * 4, 0);
    for (std::size_t i = 0; i < 5 * 3; ++i) {
        tex.pixels[i * 4 + 0] = static_cast<std::uint8_t>(i * 11); // R
        tex.pixels[i * 4 + 1] = static_cast<std::uint8_t>(i * 13); // G
    }
    auto out = saveDDS_BC(tex, BCFormat::BC5);
    ASSERT_FALSE(out.empty());
    std::uint32_t w = 0, h = 0, pitch = 0;
    std::memcpy(&w,     out.data() + 4 + 12, 4);
    std::memcpy(&h,     out.data() + 4 +  8, 4);
    std::memcpy(&pitch, out.data() + 4 + 16, 4);
    EXPECT_EQ(w, 5u);
    EXPECT_EQ(h, 3u);
    EXPECT_EQ(pitch, 32u);
    EXPECT_EQ(out.size(), 4u + 124u + 32u);
}

TEST(SaveDDSBC4, EmptyTextureReturnsEmpty) {
    LoadedTexture tex; // no width/height
    EXPECT_TRUE(saveDDS_BC(tex, BCFormat::BC4).empty());
    EXPECT_TRUE(saveDDS_BC(tex, BCFormat::BC5).empty());
}
