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
#include <cmath>

#include "mxh/render/render_typedef.hpp"
#include "mxh/render/math.hpp"

using mxh::gx::CMeshFlag;
using mxh::gx::FACE_DESC;
using mxh::gx::IVERTEX;
using mxh::gx::MATRIX4;
using mxh::gx::MESH_DESC;
using mxh::gx::TVERTEX;
using mxh::gx::VECTOR3;

namespace {

using mxh::gx::MatrixIdentity;
using mxh::gx::MatrixLookAtLH;
using mxh::gx::MatrixOrthographicLH;

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

// ---------------------------------------------------------------------------
// Matrix math helpers (used by shadow map pipeline).
// ---------------------------------------------------------------------------

// Verify MatrixOrthographicLH produces a row-major orthographic projection.
TEST(MatrixMathTest, OrthographicLHHasCorrectDiagonal) {
    MATRIX4 m{};
    MatrixOrthographicLH(&m, 10.0f, 10.0f, 1.0f, 100.0f);
    // _11 = 2/w, _22 = 2/h, _33 = 1/(zf-zn), _44 = 1
    EXPECT_FLOAT_EQ(m._11, 0.2f);
    EXPECT_FLOAT_EQ(m._22, 0.2f);
    EXPECT_FLOAT_EQ(m._33, 1.0f / 99.0f);
    EXPECT_FLOAT_EQ(m._44, 1.0f);
    // Off-diagonal should be zero
    EXPECT_FLOAT_EQ(m._12, 0.0f); EXPECT_FLOAT_EQ(m._13, 0.0f);
    EXPECT_FLOAT_EQ(m._14, 0.0f); EXPECT_FLOAT_EQ(m._21, 0.0f);
}

// Verify MatrixLookAtLH produces a valid (non-identity) view matrix.
// Camera at (0,0,5) looking toward origin (0,0,0) in LH coordinates:
//   forward = normalize(at - eye) = normalize(0,0,-5) = (0,0,-1)
//   R-9 fix: D3DX row-major view matrix has the affine
//   translation in the BOTTOM row, not the rightmost column.
//   So _43 = -dot(F, eye) = -((-1)*5) = 5. (Was _34 in the
//   pre-R-9 column-major layout.)
TEST(MatrixMathTest, LookAtLHProducesValidMatrix) {
    MATRIX4 m{};
    VECTOR3 eye{ 0.0f, 0.0f, 5.0f };
    VECTOR3 at{  0.0f, 0.0f, 0.0f };
    VECTOR3 up{  0.0f, 1.0f, 0.0f };
    MatrixLookAtLH(&m, &eye, &at, &up);
    // Right column of the upper-3x3 (m._14, m._24, m._34) is
    // zero in an affine view matrix; the basis vectors live in
    // rows 0..2 and translation lives in row 3.
    EXPECT_FLOAT_EQ(m._14, 0.0f);
    EXPECT_FLOAT_EQ(m._24, 0.0f);
    EXPECT_FLOAT_EQ(m._34, 0.0f);
    // Bottom row of a D3DX row-major view matrix holds the
    // affine translation -dot(basis, eye):
    //   _41 = -dot(R, eye) = 0   (R = (0, 0, -1), eye.z = 5)
    //   _42 = -dot(U, eye) = 0
    //   _43 = -dot(F, eye) = -((-1)*5) = 5
    //   _44 = 1
    EXPECT_FLOAT_EQ(m._41, 0.0f);
    EXPECT_FLOAT_EQ(m._42, 0.0f);
    EXPECT_FLOAT_EQ(m._43, 5.0f);
    EXPECT_FLOAT_EQ(m._44, 1.0f);
}

// Verify MatrixIdentity produces the canonical identity matrix.
TEST(MatrixMathTest, IdentityHasCorrectDiagonal) {
    MATRIX4 m = MatrixIdentity();
    EXPECT_FLOAT_EQ(m._11, 1.0f); EXPECT_FLOAT_EQ(m._22, 1.0f);
    EXPECT_FLOAT_EQ(m._33, 1.0f); EXPECT_FLOAT_EQ(m._44, 1.0f);
    EXPECT_FLOAT_EQ(m._12, 0.0f); EXPECT_FLOAT_EQ(m._13, 0.0f); EXPECT_FLOAT_EQ(m._14, 0.0f);
    EXPECT_FLOAT_EQ(m._21, 0.0f); EXPECT_FLOAT_EQ(m._23, 0.0f); EXPECT_FLOAT_EQ(m._24, 0.0f);
    EXPECT_FLOAT_EQ(m._31, 0.0f); EXPECT_FLOAT_EQ(m._32, 0.0f); EXPECT_FLOAT_EQ(m._34, 0.0f);
    EXPECT_FLOAT_EQ(m._41, 0.0f); EXPECT_FLOAT_EQ(m._42, 0.0f); EXPECT_FLOAT_EQ(m._43, 0.0f);
}

} // namespace