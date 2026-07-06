// Tests for mxh::gx::dx11::EffectShaderPalette CPU-side behavior.
//
// Phase 5 scope: test what can be validated without a real D3D11 device
//   - EffectEntry default values
//   - Palette construction / destruction
//   - buildFromDesc with empty/null input
//   - getEffect index bounds
//   - setSphereMapMatrix output properties
//   - setWaveTexMatrix output properties (values in [-1,1])

#include "effect_shader.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace mxh::gx::dx11 {

TEST(EffectShaderTest, EffectEntryDefaultValues) {
    EffectEntry e;
    EXPECT_EQ(e.bDisableSrcTex, FALSE);
    EXPECT_EQ(e.method, TEXGEN_METHOD_REFLECT_SPHEREMAP);
    EXPECT_FALSE(e.bSuccess);
    EXPECT_EQ(e.dwFlag, 0u);
    EXPECT_EQ(e.srv.Get(), nullptr);
}

TEST(EffectShaderTest, PaletteConstructsWithNullDevice) {
    EffectShaderPalette palette(nullptr);
    EXPECT_EQ(palette.effectCount(), 0u);
    EXPECT_EQ(palette.getEffect(0), nullptr);
}

TEST(EffectShaderTest, PaletteBuildFromNullDescReturnsFalse) {
    EffectShaderPalette palette(nullptr);
    EXPECT_FALSE(palette.buildFromDesc(nullptr, 0));
    EXPECT_FALSE(palette.buildFromDesc(nullptr, 5));
}

TEST(EffectShaderTest, PaletteBuildFromDescWithEmptyTexName) {
    EffectShaderPalette palette(nullptr);
    CUSTOM_EFFECT_DESC desc{};
    desc.szTexName[0] = '\0';  // no texture
    desc.method = TEXGEN_METHOD_WAVE;
    bool ok = palette.buildFromDesc(&desc, 1);
    EXPECT_TRUE(ok);
    EXPECT_EQ(palette.effectCount(), 1u);
    auto* e = palette.getEffect(0);
    ASSERT_NE(e, nullptr);
    EXPECT_FALSE(e->bSuccess);
    EXPECT_EQ(e->method, TEXGEN_METHOD_WAVE);
}

TEST(EffectShaderTest, PaletteBuildFromDescMultipleEntries) {
    EffectShaderPalette palette(nullptr);
    std::vector<CUSTOM_EFFECT_DESC> descs(3);
    descs[0].method = TEXGEN_METHOD_REFLECT_SPHEREMAP;
    descs[1].method = TEXGEN_METHOD_WAVE;
    descs[2].method = TEXGEN_METHOD_REFLECT_SPHEREMAP;
    bool ok = palette.buildFromDesc(descs.data(), 3);
    EXPECT_TRUE(ok);
    EXPECT_EQ(palette.effectCount(), 3u);
    EXPECT_EQ(palette.getEffect(0)->method, TEXGEN_METHOD_REFLECT_SPHEREMAP);
    EXPECT_EQ(palette.getEffect(1)->method, TEXGEN_METHOD_WAVE);
    EXPECT_EQ(palette.getEffect(2)->method, TEXGEN_METHOD_REFLECT_SPHEREMAP);
}

TEST(EffectShaderTest, GetEffectOutOfRangeReturnsNull) {
    EffectShaderPalette palette(nullptr);
    EXPECT_EQ(palette.getEffect(99), nullptr);
    EXPECT_EQ(palette.getEffect(0), nullptr);  // empty palette
}

TEST(EffectShaderTest, ClearEmptiesPalette) {
    EffectShaderPalette palette(nullptr);
    CUSTOM_EFFECT_DESC desc{};
    palette.buildFromDesc(&desc, 1);
    ASSERT_EQ(palette.effectCount(), 1u);
    palette.clear();
    EXPECT_EQ(palette.effectCount(), 0u);
}

TEST(EffectShaderTest, SetSphereMapMatrixOutputProperties) {
    EffectShaderPalette palette(nullptr);
    MATRIX4 world = MatrixIdentity();
    MATRIX4 view  = MatrixIdentity();
    MATRIX4 result{};
    palette.setSphereMapMatrix(&result, &world, &view);
    // After identity × identity: scaled ±0.5 and translated +0.5
    // Row 1: _11=_21=_31 = 1*0.5 = 0.5
    EXPECT_NEAR(result._11, 0.5f, 0.001f);
    EXPECT_NEAR(result._21, 0.5f, 0.001f);
    EXPECT_NEAR(result._31, 0.5f, 0.001f);
    // Row 2: _12=_22=_32 = -1*0.5 = -0.5
    EXPECT_NEAR(result._12, -0.5f, 0.001f);
    EXPECT_NEAR(result._22, -0.5f, 0.001f);
    EXPECT_NEAR(result._32, -0.5f, 0.001f);
    // Translation row: _41=0.5, _42=0.5
    EXPECT_NEAR(result._41, 0.5f, 0.001f);
    EXPECT_NEAR(result._42, 0.5f, 0.001f);
    // Translation on z is 0
    EXPECT_NEAR(result._43, 0.0f, 0.001f);
    // Translation on w is 1
    EXPECT_NEAR(result._44, 1.0f, 0.001f);
}

TEST(EffectShaderTest, SetSphereMapMatrixNullPtrNoCrash) {
    EffectShaderPalette palette(nullptr);
    palette.setSphereMapMatrix(nullptr, nullptr, nullptr);
    // No crash = pass
}

TEST(EffectShaderTest, SetWaveTexMatrixOutputProperties) {
    EffectShaderPalette palette(nullptr);
    palette.setTickCount(0);  // fXTrans = sin(0) = 0, fYTrans = sin(0) = 0
    MATRIX4 result{};
    palette.setWaveTexMatrix(&result);
    // Should be identity with animated offsets
    EXPECT_NEAR(result._11, 1.0f, 0.001f);
    EXPECT_NEAR(result._22, 1.0f, 0.001f);
    EXPECT_NEAR(result._33, 1.0f, 0.001f);
    EXPECT_NEAR(result._44, 1.0f, 0.001f);
    EXPECT_NEAR(result._31, 0.0f, 0.001f);  // sin(0) = 0
    EXPECT_NEAR(result._32, 0.0f, 0.001f);  // sin(0) = 0
}

TEST(EffectShaderTest, SetWaveTexMatrixNullPtrNoCrash) {
    EffectShaderPalette palette(nullptr);
    palette.setWaveTexMatrix(nullptr);
    // No crash = pass
}

TEST(EffectShaderTest, SetWaveTexMatrixAnimatedValuesInRange) {
    EffectShaderPalette palette(nullptr);
    // tick=1000: fXTrans = sin(1.0) ≈ 0.84, fYTrans = sin(0.8) ≈ 0.717
    palette.setTickCount(1000);
    MATRIX4 result{};
    palette.setWaveTexMatrix(&result);
    EXPECT_GE(result._31, -1.0f);
    EXPECT_LE(result._31, 1.0f);
    EXPECT_GE(result._32, -1.0f);
    EXPECT_LE(result._32, 1.0f);
}

} // namespace mxh::gx::dx11
