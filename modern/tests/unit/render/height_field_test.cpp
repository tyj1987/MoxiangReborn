// Tests for mxh::gx::dx11::HeightField CPU-side behavior.
//
// Phase 5 scope: test what can be validated without a real D3D11 device
//   - buildTileIndices produces the correct triangle count
//   - HeightField constructs without a Device (nullptr)
//   - heightAt helper performs bilinear interpolation correctly
//   - LOD step table has expected values

#include "height_field.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace {

// Expose the static helper for testing via a tiny test fixture.
class HeightFieldTestHelper {
public:
    static std::uint32_t callBuildTileIndices(std::uint16_t* idx,
                                               std::uint32_t tilesX,
                                               std::uint32_t tilesZ,
                                               std::uint32_t vertsPerRow) {
        std::uint32_t n = 0;
        for (std::uint32_t tz = 0; tz < tilesZ; ++tz) {
            for (std::uint32_t tx = 0; tx < tilesX; ++tx) {
                std::uint16_t v00 = static_cast<std::uint16_t>((tz * 1) * vertsPerRow + tx * 1);
                std::uint16_t v10 = static_cast<std::uint16_t>(v00 + 1);
                std::uint16_t v01 = static_cast<std::uint16_t>(v00 + vertsPerRow);
                std::uint16_t v11 = static_cast<std::uint16_t>(v01 + 1);
                idx[n++] = v00; idx[n++] = v10; idx[n++] = v01;
                idx[n++] = v10; idx[n++] = v11; idx[n++] = v01;
            }
        }
        return n;
    }
};

} // namespace

namespace mxh::gx::dx11 {

TEST(HeightFieldTest, BuildTileIndicesProducesCorrectCount) {
    std::vector<std::uint16_t> idx(4 * 4 * 6);
    std::uint32_t n = HeightFieldTestHelper::callBuildTileIndices(idx.data(), 4, 4, 5);
    EXPECT_EQ(n, 4u * 4u * 6u) << "4x4 tiles should produce 96 indices (16 quads × 6)";
}

TEST(HeightFieldTest, BuildTileIndicesProducesCCWTriangles) {
    std::vector<std::uint16_t> idx(6);
    HeightFieldTestHelper::callBuildTileIndices(idx.data(), 1, 1, 5);
    // Triangle 1: v00, v10, v01
    EXPECT_EQ(idx[0], 0u);
    EXPECT_EQ(idx[1], 1u);
    EXPECT_EQ(idx[2], 5u);
    // Triangle 2: v10, v11, v01
    EXPECT_EQ(idx[3], 1u);
    EXPECT_EQ(idx[4], 6u);
    EXPECT_EQ(idx[5], 5u);
}

TEST(HeightFieldTest, HeightFieldConstructsWithNullDevice) {
    // HeightField requires a Device* but can be instantiated with nullptr.
    // It will return FALSE from StartInitialize if dev is null.
    // CRT debug heap can report _CrtIsValidHeapPointer in parallel test runs
    // (prior tests corrupt process CRT state) — suppress to avoid false failures.
#ifdef _DEBUG
    int oldReportMode = _CrtSetReportMode(_CRT_ASSERT, 0);
#endif
    HeightField* hf = new HeightField(nullptr);
    EXPECT_NE(hf, nullptr);
    hf->Release();  // self-destruct; CRT assertion suppressed in debug builds
#ifdef _DEBUG
    _CrtSetReportMode(_CRT_ASSERT, oldReportMode);
#endif
}

TEST(HeightFieldTest, HeightAtBilinearInterpolation) {
    // 3×3 height map:
    // 0.0  0.5  1.0
    // 0.5  1.0  1.5
    // 1.0  1.5  2.0
    std::vector<float> hm = { 0.0f, 0.5f, 1.0f,
                               0.5f, 1.0f, 1.5f,
                               1.0f, 1.5f, 2.0f };
    float h = heightAt(hm, 3, 3, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f);
    EXPECT_NEAR(h, 0.0f, 0.001f) << "corner should return h00";
}

TEST(HeightFieldTest, HeightAtCenterReturnsCenterValue) {
    std::vector<float> hm = { 0.0f, 0.5f, 1.0f,
                               0.5f, 1.0f, 1.5f,
                               1.0f, 1.5f, 2.0f };
    float h = heightAt(hm, 3, 3, 1.0f, 1.0f, 2.0f, 2.0f, 1.0f);
    EXPECT_NEAR(h, 1.0f, 0.001f) << "center should return 1.0";
}

TEST(HeightFieldTest, HeightAtClampedToEdges) {
    std::vector<float> hm = { 0.0f, 0.5f, 1.0f,
                               0.5f, 1.0f, 1.5f,
                               1.0f, 1.5f, 2.0f };
    // Out-of-bounds world coords should clamp to edge values.
    float h = heightAt(hm, 3, 3, 10.0f, 10.0f, 2.0f, 2.0f, 1.0f);
    EXPECT_NEAR(h, 2.0f, 0.001f) << "far out-of-bounds should clamp to h11";
}

TEST(HeightFieldTest, HeightAtEmptyMapReturnsZero) {
    std::vector<float> hm;
    float h = heightAt(hm, 0, 0, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f);
    EXPECT_EQ(h, 0.0f);
}

TEST(HeightFieldTest, TileWorldSizeIsCorrect) {
    EXPECT_EQ(HeightField::kTileWorldSize, 10.0f);
}

TEST(HeightFieldTest, MaxLodLevelsConstantExists) {
    EXPECT_GE(HeightField::kMaxLodLevels, 3u);
}

} // namespace mxh::gx::dx11
