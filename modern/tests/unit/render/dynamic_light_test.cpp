// tests/unit/render/dynamic_light_test.cpp
// Unit tests for Dynamic Light management: DynamicLight struct, LightCB, color_to_float4.
#include <gtest/gtest.h>
#include "dynamic_light.hpp"

using namespace mxh::gx::dx11;

// =============================================================================
// DynamicLight struct tests
// =============================================================================

TEST(DynamicLightDefaults, fieldsAreDefault) {
    DynamicLight L;
    EXPECT_FALSE(L.bActive);
    EXPECT_TRUE(L.bDirectional);
    EXPECT_EQ(L.dwRS, 0u);
    EXPECT_EQ(L.dwColor, 0xffc8c8c8u);
    EXPECT_FLOAT_EQ(L.fAmbient, 0.25f);
    EXPECT_FLOAT_EQ(L.fDiffuse, 0.95f);
    EXPECT_FLOAT_EQ(L.v3Dir[0], 0.f);
    EXPECT_FLOAT_EQ(L.v3Dir[1], -1.f);
    EXPECT_FLOAT_EQ(L.v3Dir[2], 0.f);
    EXPECT_FLOAT_EQ(L.fRange, 200.f);
}

TEST(DynamicLightDefaults, pointLightFields) {
    DynamicLight L;
    EXPECT_FLOAT_EQ(L.v3Pos[0], 0.f);
    EXPECT_FLOAT_EQ(L.v3Pos[1], 0.f);
    EXPECT_FLOAT_EQ(L.v3Pos[2], 0.f);
    EXPECT_FLOAT_EQ(L.fAttenuation0, 0.f);
    EXPECT_FLOAT_EQ(L.fAttenuation1, 0.05f);
    EXPECT_FLOAT_EQ(L.fAttenuation2, 0.f);
}

TEST(DynamicLightDefaults, flagsValues) {
    EXPECT_EQ(LIGHT_FLAG_ENABLE, 0x00000001u);
    EXPECT_EQ(LIGHT_FLAG_DIRECTIONAL, 0x00000002u);
    EXPECT_EQ(LIGHT_FLAG_POINT, 0x00000004u);
    EXPECT_EQ(LIGHT_FLAG_SPOT, 0x00000008u);
}

// =============================================================================
// LightCB init tests
// =============================================================================

TEST(LightCBInit, baseDirectionalLightDefaults) {
    LightCB cb{};
    init_light_cb(cb);
    EXPECT_FLOAT_EQ(cb.ambient[0], 0.25f);
    EXPECT_FLOAT_EQ(cb.ambient[1], 0.25f);
    EXPECT_FLOAT_EQ(cb.ambient[2], 0.25f);
    EXPECT_FLOAT_EQ(cb.ambient[3], 1.0f);
    EXPECT_FLOAT_EQ(cb.diffuse[0], 0.95f);
    EXPECT_FLOAT_EQ(cb.diffuse[1], 0.95f);
    EXPECT_FLOAT_EQ(cb.diffuse[2], 0.95f);
    EXPECT_FLOAT_EQ(cb.diffuse[3], 1.0f);
    EXPECT_FLOAT_EQ(cb.lightDir[0],  0.3f);
    EXPECT_FLOAT_EQ(cb.lightDir[1], -0.7f);
    EXPECT_FLOAT_EQ(cb.lightDir[2],  0.4f);
    EXPECT_FLOAT_EQ(cb.lightDir[3],  0.0f);
    // Fog disabled by default
    EXPECT_FLOAT_EQ(cb.fogParams[0], -1.f);
}

TEST(LightCBInit, extendedSlotsZeroed) {
    LightCB cb{};
    init_light_cb(cb);
    // Check slot 0 pos is zero
    EXPECT_FLOAT_EQ(cb.dynLightPos0[0], 0.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos0[1], 0.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos0[2], 0.f);
    EXPECT_FLOAT_EQ(cb.dynLightPos0[3], 0.f);
    // Check slot 7 color is zero
    EXPECT_FLOAT_EQ(cb.dynLightColor7[0], 0.f);
    EXPECT_FLOAT_EQ(cb.dynLightAtten7[1], 0.f);
}

// =============================================================================
// color_to_float4 tests
// =============================================================================

TEST(ColorConversion, opaqueWhite) {
    float f[4];
    color_to_float4(0xFFFFFFFFu, f);
    EXPECT_FLOAT_EQ(f[0], 1.0f); // R
    EXPECT_FLOAT_EQ(f[1], 1.0f); // G
    EXPECT_FLOAT_EQ(f[2], 1.0f); // B
    EXPECT_FLOAT_EQ(f[3], 1.0f); // A
}

TEST(ColorConversion, opaqueRed) {
    float f[4];
    // 0xAABBGGRR: A=FF, R=FF, G=00, B=00 => {R=1.0, G=0.0, B=0.0, A=1.0}
    color_to_float4(0xFFFF0000u, f);
    EXPECT_FLOAT_EQ(f[0], 1.0f); // R
    EXPECT_FLOAT_EQ(f[1], 0.0f); // G
    EXPECT_FLOAT_EQ(f[2], 0.0f); // B
    EXPECT_FLOAT_EQ(f[3], 1.0f); // A
}

TEST(ColorConversion, semiTransparentYellow) {
    float f[4];
    // 0xAABBGGRR: A=80, R=FF, G=FF, B=00 => {R=1.0, G=1.0, B=0.0, A=0.502}
    color_to_float4(0x80FFFF00u, f);
    EXPECT_FLOAT_EQ(f[0], 1.0f);  // R
    EXPECT_FLOAT_EQ(f[1], 1.0f);  // G
    EXPECT_FLOAT_EQ(f[2], 0.0f);  // B
    EXPECT_FLOAT_EQ(f[3], 128.f/255.f); // A ~0.502
}

TEST(ColorConversion, zeroAlpha) {
    float f[4];
    color_to_float4(0x00FFFFFFu, f);
    EXPECT_FLOAT_EQ(f[0], 1.0f);
    EXPECT_FLOAT_EQ(f[1], 1.0f);
    EXPECT_FLOAT_EQ(f[2], 1.0f);
    EXPECT_FLOAT_EQ(f[3], 0.0f);
}

// =============================================================================
// LIGHT_INDEX_DESC tests
// =============================================================================

TEST(LightIndexDesc, structLayout) {
    mxh::gx::LIGHT_INDEX_DESC desc{};
    desc.pMtlHandle  = reinterpret_cast<void*>(0x12345678);
    desc.bLightIndex = 3;
    desc.bTexOP      = 5;
    EXPECT_EQ(desc.pMtlHandle, reinterpret_cast<void*>(0x12345678));
    EXPECT_EQ(desc.bLightIndex, 3u);
    EXPECT_EQ(desc.bTexOP, 5u);
    EXPECT_EQ(desc.bReserved2, 0u);
    EXPECT_EQ(desc.bReserved3, 0u);
}

// =============================================================================
// MAX_DYNAMIC_LIGHTS constant
// =============================================================================

TEST(DynamicLightConstants, maxLightsIs8) {
    EXPECT_EQ(MAX_DYNAMIC_LIGHTS, 8u);
}

// =============================================================================
// TriBuffer magic constant
// =============================================================================

TEST(TriBufferMagic, magicValueIsTRIB) {
    EXPECT_EQ(mxh::gx::dx11::TRI_BUFFER_MAGIC, 0x54524942u);
}
