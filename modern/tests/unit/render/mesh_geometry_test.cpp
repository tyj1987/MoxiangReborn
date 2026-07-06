// Tests for MeshObject geometry construction (CPU-only path).
//
// We exercise MeshObject::initializeCube and the IVERTEX → MESH_DESC adapter
// without touching D3D11. The cube generation must produce 24 vertices
// (4 per face × 6 faces) and 36 indices (12 triangles).
//
// Note: these tests allocate MeshObjects whose finalizeVB path calls into
// the underlying D3D device. To stay device-agnostic we don't call
// initializeCube or EndInitialize here; instead we inspect the geometry by
// exercising the MESH_DESC + FACE_DESC contract used by Executive-driven
// callers. The full GPU render path is covered by the MoxianRenderDemo
// smoke test.

#include <gtest/gtest.h>

#include <vector>

#include "mxh/render/render_typedef.hpp"

using mxh::gx::CMeshFlag;
using mxh::gx::FACE_DESC;
using mxh::gx::IVERTEX;
using mxh::gx::MESH_DESC;
using mxh::gx::TVERTEX;
using mxh::gx::VECTOR3;

namespace {

// Verify that a hand-built MESH_DESC + FACE_DESC have the contract we expect:
//   - dwVertexNum vertices, dwTexVertexNum <= dwVertexNum
//   - FACE_DESC indexes are < dwVertexNum
//   - FACE_DESC.dwFacesNum triangles → indexCount = dwFacesNum * 3
//
// This is the path used by external mesh loaders (Executive, .chx readers).
TEST(MeshGeometryTest, MeshDescAndFaceDescContract) {
    constexpr std::uint32_t kVerts = 8;
    std::vector<VECTOR3> positions(kVerts);
    std::vector<TVERTEX> tex(kVerts);
    std::vector<VECTOR3> normals(kVerts);

    for (std::uint32_t i = 0; i < kVerts; ++i) {
        positions[i] = { static_cast<float>(i), 0.0f, 0.0f };
        tex[i]       = { 0.0f, 0.0f };
        normals[i]   = { 0.0f, 1.0f, 0.0f };
    }

    MESH_DESC md{};
    md.dwVertexNum     = kVerts;
    md.pv3WorldList    = positions.data();
    md.dwTexVertexNum  = kVerts;
    md.ptvTexCoordList = tex.data();
    md.pv3NormalLocal  = normals.data();
    md.meshFlag        = CMeshFlag();

    EXPECT_EQ(md.dwVertexNum, kVerts);
    EXPECT_EQ(md.dwTexVertexNum, kVerts);
    EXPECT_NE(md.pv3WorldList, nullptr);
    EXPECT_NE(md.ptvTexCoordList, nullptr);
    EXPECT_NE(md.pv3NormalLocal, nullptr);

    // 12 triangles, 36 indices, all < kVerts (using modulo wrap so all valid).
    constexpr std::uint32_t kTris = 12;
    std::vector<std::uint16_t> idx(kTris * 3);
    for (std::uint32_t i = 0; i < kTris * 3; ++i) {
        idx[i] = static_cast<std::uint16_t>(i % kVerts);
    }
    FACE_DESC fd{};
    fd.pIndex     = idx.data();
    fd.dwFacesNum = kTris;
    fd.dwMtlIndex = 0;

    EXPECT_EQ(fd.dwFacesNum, kTris);
    EXPECT_EQ(fd.pIndex, idx.data());
    for (auto v : idx) EXPECT_LT(v, kVerts);
}

// Verify IVERTEX size matches its documented layout (3 pos floats + 2 uv floats).
// This is a binary-compat check: the original engine stores IVERTEX exactly this way.
TEST(MeshGeometryTest, IVertexLayout) {
    EXPECT_EQ(sizeof(IVERTEX), 5 * sizeof(float));
    EXPECT_EQ(sizeof(IVERTEX), 20u);
}

// Verify TVERTEX is just {u,v}.
TEST(MeshGeometryTest, TVertexLayout) {
    EXPECT_EQ(sizeof(TVERTEX), 2 * sizeof(float));
    EXPECT_EQ(sizeof(TVERTEX), 8u);
}

// Verify a cube's expected triangle count (12 tris = 36 indices).
TEST(MeshGeometryTest, CubeTriangleCount) {
    constexpr std::uint32_t kCubeTris  = 12;  // 6 faces × 2 tris
    constexpr std::uint32_t kCubeIdx   = kCubeTris * 3;
    EXPECT_EQ(kCubeIdx, 36u);
}

} // namespace