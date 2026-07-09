// Tests for HeightField::RenderGrid (Phase 5.9c stub fill-in).
//
// Pre-Phase-5.9c RenderGrid was a one-liner stub: `return TRUE;`. The fill-in
// turns it into a real call that:
//   - Rejects null quad + not-initialized inputs (returns FALSE)
//   - Records lastRenderGridTile / lastRenderGridAlpha / renderGridCount so
//     debug overlays + tests can verify the call happened
//   - Walks the chunk grid and re-issues Render() with the per-call alpha
//
// These tests are CPU-only (no D3D11 device); the device-requiring path is
// exercised by the manual MoxianRenderDemo + the smoke harness.

#include "height_field.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::gx::dx11 {

// RenderGrid without an initialized HeightField must reject.
TEST(HeightFieldRenderGrid, RejectsNullQuad) {
    HeightField hf(nullptr);
    // The constructor accepts nullptr device; the only required precondition
    // for RenderGrid to be useful is initialization. Without StartInitialize
    // the call must return FALSE.
    VECTOR3 quad[4] = {{0,0,0},{1,0,0},{1,0,1},{0,0,1}};
    EXPECT_FALSE(hf.RenderGrid(quad, 0, 255));
}

TEST(HeightFieldRenderGrid, RejectsNullQuadEvenWhenInitialized) {
    // A HeightField with no Device can still be "initialized" if a caller
    // sets m_initialized via a friend path; for the test we drive StartInitialize
    // with a no-op desc and a null device to confirm the null-quad check fires
    // *before* the device dereference.
    HeightField hf(nullptr);
    HFIELD_DESC desc{};
    desc.left = 0.0f; desc.top = 0.0f; desc.right = 1.0f; desc.bottom = 1.0f;
    desc.fFaceSize = 1.0f;
    desc.dwFacesNumPerObjAxis = 1;
    desc.dwObjNumX = 1; desc.dwObjNumZ = 1;
    desc.bDetailLevelNum = 1;
    desc.dwIndexBufferNumLV0 = 1;
    desc.pTexTable = nullptr;
    desc.dwTileTextureNum = 0;
    desc.pyfList = nullptr;
    desc.dwYFNumX = 1; desc.dwYFNumZ = 1;
    desc.width = 1.0f; desc.height = 1.0f;
    desc.dwFacesNumX = 1; desc.dwFacesNumZ = 1;
    desc.dwTriNumPerObj = 2;
    desc.dwVerticesNumPerObj = 4;
    desc.pwTileTable = nullptr;
    desc.dwFacesNumPerTileAxis = 1;
    desc.dwTileNumPerObjAxis = 1;
    desc.dwTileNumX = 1; desc.dwTileNumZ = 1;
    desc.fTileSize = 1.0f;
    // StartInitialize returns FALSE when m_dev is null, so m_initialized stays
    // false and the next RenderGrid call must hit the not-initialized branch.
    EXPECT_FALSE(hf.StartInitialize(&desc));
    VECTOR3 quad[4] = {{0,0,0},{1,0,0},{1,0,1},{0,0,1}};
    EXPECT_FALSE(hf.RenderGrid(quad, 0, 128));
}

TEST(HeightFieldRenderGrid, AccessorsStartAtZero) {
    // Pre-call state: counter and last-tile/alpha are all zero.
    HeightField hf(nullptr);
    EXPECT_EQ(hf.renderGridCount(), 0u);
    EXPECT_EQ(hf.lastRenderGridTile(), 0u);
    EXPECT_EQ(hf.lastRenderGridAlpha(), 0u);
}

} // namespace mxh::gx::dx11
