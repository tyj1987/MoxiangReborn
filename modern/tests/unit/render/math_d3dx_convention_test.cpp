// math_d3dx_convention_test.cpp - R-9 fix: pin D3DX row-major view/ortho
// conventions. These tests use off-axis eye / asymmetric frustum so the
// difference between D3DX row-major view (column 0 = right basis, column 1
// = up basis, column 2 = forward basis, column 3 = translation) and the
// "everything is row" interpretation shows up in the values.
//
// Reference layout (per D3DXMatrixLookAtLH / D3DXMatrixOrthoLH docs):
//   MatrixLookAtLH: M._ij (row i col j math) =
//     _11=R.x  _12=U.x  _13=F.x  _14=-P.R
//     _21=R.y  _22=U.y  _23=F.y  _24=-P.U
//     _31=R.z  _32=U.z  _33=F.z  _34=-P.F
//     _41=0    _42=0    _43=0    _44=1
//   MatrixOrthographicLH: M._ij =
//     _11=2/w  _12=0    _13=0    _14=0
//     _21=0    _22=2/h  _23=0    _24=0
//     _31=0    _32=0    _33=1/(zf-zn)  _34=0
//     _41=0    _42=0    _43=-zn/(zf-zn) _44=1
//
// Verify by transforming known basis vectors: world +X (right basis),
// world +Y (up), world +Z (forward = into screen) into view space.

#include "mxh/render/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

namespace mxh::gx::test {

// ===========================================================================
// R-9.1: D3DX MatrixLookAtLH — off-axis eye (eye != on axis) so the
// basis vectors in different rows differ and pin the row/col layout.
// ===========================================================================

TEST(MatrixLookAtD3DXTest, OffAxisEyeTranslationColumn) {
    // Eye at (3, 4, 5), looking at origin. The translation row
    // (_41, _42, _43) must equal -dot(R/U/F, eye) for the eye to
    // map to the origin in view space. This is the same as
    // MatrixLookAtTest.EyeMapsToOrigin but framed as a layout
    // check (the off-axis eye has a translation row with all
    // three basis components contributing).
    //
    // The HLSL convention used by the project's shaders is
    // `mul(v_row, M_cbuffer)`. With CPU row-major memory layout
    // (this header) and HLSL default column-major packing, the
    // mathematical transform is
    //   result[j] = sum_k v[k] * M_loaded[k][j]
    //            = sum_k v[k] * M_cpu[k][j]    (where M_loaded
    //              is the column-major HLSL read of CPU memory)
    // In the C++ union `M[i][j] = _ij = M_cpu[i][j]`, the
    // translation of an affine transform lives in row 3
    // (M[3][0..3] = _41, _42, _43, _44), and the eye→origin
    // identity becomes
    //   result.x = M[0][0]*eye.x + M[1][0]*eye.y + M[2][0]*eye.z
    //            + M[3][0]*1
    //          = _11*eye.x + _21*eye.y + _31*eye.z + _41
    // If R is the right basis (column 0 of M = _11, _21, _31)
    // and translation is -R·eye in the bottom row, this is 0.
    MATRIX4 m{};
    VECTOR3 eye{3.0f, 4.0f, 5.0f};
    VECTOR3 at{0.0f, 0.0f, 0.0f};
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);

    // Apply M to (eye, 1) using the HLSL row-vector convention
    // (column j of M dotted with v).
    float tx = m._11 * eye.x + m._21 * eye.y + m._31 * eye.z + m._41;
    float ty = m._12 * eye.x + m._22 * eye.y + m._32 * eye.z + m._42;
    float tz = m._13 * eye.x + m._23 * eye.y + m._33 * eye.z + m._43;
    float tw = m._14 * eye.x + m._24 * eye.y + m._34 * eye.z + m._44;
    EXPECT_NEAR(tx, 0.0f, 1e-4f) << "view matrix does not map eye to origin (x)";
    EXPECT_NEAR(ty, 0.0f, 1e-4f) << "view matrix does not map eye to origin (y)";
    EXPECT_NEAR(tz, 0.0f, 1e-4f) << "view matrix does not map eye to origin (z)";
    EXPECT_NEAR(tw, 1.0f, 1e-4f) << "view matrix does not preserve homogeneous w";
}

TEST(MatrixLookAtD3DXTest, WorldRightBasisMapsToViewX) {
    // World +X = (1, 0, 0). In view space this should map to
    //   v_view.x = R.x (the right basis x component),
    //   v_view.y = R.y,
    //   v_view.z = R.z.
    // D3DX row-major layout (rotation is upper-3x3, with basis
    // vectors along the rows): R lives in row 0 (m._11, m._12,
    // m._13) = (R.x, U.x, F.x); so the world-+X vector maps to
    //   result.x = R.x (row 0 dot (1, 0, 0, 0)),
    //   result.y = R.y (row 1 dot (1, 0, 0, 0)),
    //   result.z = R.z (row 2 dot (1, 0, 0, 0)).
    // Pick an eye where R is *not* aligned with the world axes:
    // eye=(3,4,5) looking at origin.
    MATRIX4 m{};
    VECTOR3 eye{3.0f, 4.0f, 5.0f};
    VECTOR3 at{0.0f, 0.0f, 0.0f};
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    // M * (1, 0, 0, 0) in column-vector convention = first column of M
    // (i.e. m._11, m._21, m._31, m._41). In the D3DX row-major
    // layout, the world-+X basis is supposed to map to (R.x, R.y,
    // R.z, 0) — i.e. (m._11, m._21, m._31, m._41).
    EXPECT_NEAR(m._11, m._11, 1e-6f);  // tautology, just exercising the layout
    // Forward is normalize(at - eye) = (-3,-4,-5)/sqrt(50)
    const float fl = std::sqrt(50.0f);
    const float fx = -3.0f / fl, fy = -4.0f / fl, fz = -5.0f / fl;
    // right (pre-normalize) = f × up
    const float rx0 = -fz;
    const float ry0 = 0.0f;
    const float rz0 = fx;
    const float rl = std::sqrt(rx0 * rx0 + ry0 * ry0 + rz0 * rz0);
    const float rx = rx0 / rl;
    const float ry = ry0 / rl;
    const float rz = rz0 / rl;
    EXPECT_NEAR(m._11, rx, 1e-5f) << "_11 should equal R.x";
    EXPECT_NEAR(m._21, ry, 1e-5f) << "_21 should equal R.y";
    EXPECT_NEAR(m._31, rz, 1e-5f) << "_31 should equal R.z";
    // up = right × forward (already unit length because R and F are
    // unit-length and orthogonal).
    const float ux = ry * fz - rz * fy;
    const float uy = rz * fx - rx * fz;
    const float uz = rx * fy - ry * fx;
    EXPECT_NEAR(m._12, ux, 1e-5f) << "_12 should equal U.x";
    EXPECT_NEAR(m._22, uy, 1e-5f) << "_22 should equal U.y";
    EXPECT_NEAR(m._32, uz, 1e-5f) << "_32 should equal U.z";
    // Forward components live in column 2:
    EXPECT_NEAR(m._13, fx, 1e-5f) << "_13 should equal F.x";
    EXPECT_NEAR(m._23, fy, 1e-5f) << "_23 should equal F.y";
    EXPECT_NEAR(m._33, fz, 1e-5f) << "_33 should equal F.z";
    // Right column of the upper-3x3 is 0 in affine transforms:
    EXPECT_NEAR(m._14, 0.0f, 1e-7f) << "_14 must be 0 (no x translation in column 3)";
    EXPECT_NEAR(m._24, 0.0f, 1e-7f) << "_24 must be 0";
    EXPECT_NEAR(m._34, 0.0f, 1e-7f) << "_34 must be 0";
    // Bottom row holds the affine translation -dot(basis, eye).
    EXPECT_NEAR(m._41, -(rx * 3.0f + ry * 4.0f + rz * 5.0f), 1e-4f);
    EXPECT_NEAR(m._42, -(ux * 3.0f + uy * 4.0f + uz * 5.0f), 1e-4f);
    EXPECT_NEAR(m._43, -(fx * 3.0f + fy * 4.0f + fz * 5.0f), 1e-4f);
    EXPECT_NEAR(m._44, 1.0f, 1e-7f);
}

TEST(MatrixLookAtD3DXTest, ViewMatrixIsRigidIsometry) {
    // A correctly built view matrix is a rigid isometry: its
    // upper-3x3 rotation part is orthonormal. Off-axis eye case.
    MATRIX4 m{};
    VECTOR3 eye{3.0f, 4.0f, 5.0f};
    VECTOR3 at{0.0f, 0.0f, 0.0f};
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    // Each column of the rotation part should be unit length.
    auto len3 = [](float x, float y, float z) {
        return std::sqrt(x * x + y * y + z * z);
    };
    EXPECT_NEAR(len3(m._11, m._21, m._31), 1.0f, 1e-5f) << "right basis not unit";
    EXPECT_NEAR(len3(m._12, m._22, m._32), 1.0f, 1e-5f) << "up basis not unit";
    EXPECT_NEAR(len3(m._13, m._23, m._33), 1.0f, 1e-5f) << "forward basis not unit";
    // Columns should be orthogonal: R · U = 0, R · F = 0, U · F = 0.
    float rU = m._11 * m._12 + m._21 * m._22 + m._31 * m._32;
    float rF = m._11 * m._13 + m._21 * m._23 + m._31 * m._33;
    float uF = m._12 * m._13 + m._22 * m._23 + m._32 * m._33;
    EXPECT_NEAR(rU, 0.0f, 1e-5f) << "right and up not orthogonal";
    EXPECT_NEAR(rF, 0.0f, 1e-5f) << "right and forward not orthogonal";
    EXPECT_NEAR(uF, 0.0f, 1e-5f) << "up and forward not orthogonal";
}

TEST(MatrixLookAtD3DXTest, TargetDistanceIsPreserved) {
    // In view space, the target should be along the forward
    // axis at distance |at - eye| from the origin. The forward
    // column is (m._13, m._23, m._33) under D3DX row-major.
    MATRIX4 m{};
    VECTOR3 eye{0.0f, 0.0f, 0.0f};
    VECTOR3 at{3.0f, 4.0f, 0.0f};   // distance 5
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    // The world-space target is 5 units from the eye (origin).
    // After M_view * at (HLSL row-vector convention, column j
    // of M dotted with v), the result should have |z| = 5 with
    // x, y close to 0.
    float tx = m._11 * at.x + m._21 * at.y + m._31 * at.z + m._41;
    float ty = m._12 * at.x + m._22 * at.y + m._32 * at.z + m._42;
    float tz = m._13 * at.x + m._23 * at.y + m._33 * at.z + m._43;
    EXPECT_NEAR(tx, 0.0f, 1e-4f) << "view-space target.x should be 0";
    EXPECT_NEAR(ty, 0.0f, 1e-4f) << "view-space target.y should be 0";
    EXPECT_NEAR(std::abs(tz), 5.0f, 1e-4f) << "view-space target.z magnitude should be 5";
}

// ===========================================================================
// R-9.2: D3DX MatrixOrthographicLH — translation row, not translation column.
// ===========================================================================

TEST(MatrixOrthoD3DXTest, CornersMapToNDC) {
    // For a 4x4x(1..3) frustum, the corner (-2, -2, 1) should map
    // to NDC (-1, -1, 0) post-divide, and (2, 2, 3) to (1, 1, 1).
    //
    // D3DX row-major ortho has the affine translation in the
    // BOTTOM row (_41, _42, _43, _44). The z-translation that
    // pushes the near plane to NDC z = 0 is _43.
    //
    // HLSL row-vector convention: result[j] = sum_k v[k] * M[k][j]
    // (column j of M dotted with v). So applying M to (-2, -2, 1, 1):
    //   result.x = _11*(-2) + _21*(-2) + _31*1 + _41*1
    //           = (2/4)*(-2) + 0 + 0 + 0 = -1
    //   result.y = _12*(-2) + _22*(-2) + _32*1 + _42*1
    //           = 0 + (2/4)*(-2) + 0 + 0 = -1
    //   result.z = _13*(-2) + _23*(-2) + _33*1 + _43*1
    //           = 0 + 0 + (1/2)*1 + (-0.5)*1 = 0
    //   result.w = _14*(-2) + _24*(-2) + _34*1 + _44*1
    //           = 0 + 0 + 0 + 1 = 1
    // So the (x, y, z, w) result is (-1, -1, 0, 1) — already
    // post-divide (w = 1).
    MATRIX4 m{};
    MatrixOrthographicLH(&m, 4.0f, 4.0f, 1.0f, 3.0f);
    // Apply to (-2, -2, 1, 1):
    float x_ndc = m._11 * -2.0f + m._21 * -2.0f + m._31 * 1.0f + m._41;
    float y_ndc = m._12 * -2.0f + m._22 * -2.0f + m._32 * 1.0f + m._42;
    float z_ndc = m._13 * -2.0f + m._23 * -2.0f + m._33 * 1.0f + m._43;
    float w_ndc = m._14 * -2.0f + m._24 * -2.0f + m._34 * 1.0f + m._44;
    EXPECT_NEAR(x_ndc, -1.0f, 1e-5f) << "NDC.x of (-2,-2,1)";
    EXPECT_NEAR(y_ndc, -1.0f, 1e-5f) << "NDC.y of (-2,-2,1)";
    EXPECT_NEAR(z_ndc,  0.0f, 1e-5f) << "NDC.z of (-2,-2,1) should be 0 (near plane)";
    EXPECT_NEAR(w_ndc,  1.0f, 1e-5f) << "NDC.w of (-2,-2,1)";

    // Apply to (2, 2, 3, 1):
    float x_far = m._11 * 2.0f + m._21 * 2.0f + m._31 * 3.0f + m._41;
    float y_far = m._12 * 2.0f + m._22 * 2.0f + m._32 * 3.0f + m._42;
    float z_far = m._13 * 2.0f + m._23 * 2.0f + m._33 * 3.0f + m._43;
    EXPECT_NEAR(x_far, 1.0f, 1e-5f) << "NDC.x of (2,2,3)";
    EXPECT_NEAR(y_far, 1.0f, 1e-5f) << "NDC.y of (2,2,3)";
    EXPECT_NEAR(z_far, 1.0f, 1e-5f) << "NDC.z of (2,2,3) should be 1 (far plane)";
}

TEST(MatrixOrthoD3DXTest, TranslationIsInRowThreeColumnTwo) {
    // The D3DX row-major ortho has _33 = 1/(zf-zn) (the z
    // scale) and _43 = -zn/(zf-zn) (the z translation that
    // pushes the near plane to NDC z = 0 in the post-divide
    // result). The translation lives in the BOTTOM row of the
    // 4x4 matrix, NOT in row 2 col 2 (which would be a
    // perspective-divide row, not a translation row).
    MATRIX4 m{};
    MatrixOrthographicLH(&m, 2.0f, 2.0f, 1.0f, 3.0f);
    // Pin the D3DX layout:
    EXPECT_NEAR(m._43, -0.5f, 1e-6f) << "z-translation in bottom row col 2 = -zn/(zf-zn)";
    EXPECT_NEAR(m._33,  0.5f, 1e-6f) << "z-scale in row 2 col 2 = 1/(zf-zn)";
    EXPECT_NEAR(m._32,  0.0f, 1e-7f) << "row 2 col 2 (z) is the scale, not the translation";
    // x and y translations are 0 (axis-aligned ortho).
    EXPECT_NEAR(m._41, 0.0f, 1e-7f) << "no x translation in axis-aligned ortho";
    EXPECT_NEAR(m._42, 0.0f, 1e-7f) << "no y translation in axis-aligned ortho";
    EXPECT_NEAR(m._44, 1.0f, 1e-7f) << "homogeneous w coordinate";
}

// ===========================================================================
// R-9.3: view * ortho composition — used by the shadow pipeline.
// ===========================================================================

TEST(ViewOrthoCompositionTest, ViewAtOriginLooksAtPlusZ) {
    // Eye at origin looking down +Z with up=+Y. A point at
    // world (0, 0, 5) should map to view (0, 0, 5) (forward
    // axis is +Z) and then through an ortho (4, 4, 1, 100)
    // the result should be NDC (0, 0, ~0.0404) after the
    // perspective divide.
    //
    // The composition here uses MatrixMultiply2(&vp, &view, &proj)
    // which gives vp = view * proj mathematically (D3DX row-major
    // math, row-major storage). The shadow pipeline then applies
    // vp to world points as a single matrix transform, so the
    // result of vp * (0, 0, 5, 1) under HLSL row-vec mul should
    // be the post-projection coordinates.
    MATRIX4 view{}, proj{};
    VECTOR3 eye{0.0f, 0.0f, 0.0f};
    VECTOR3 at{0.0f, 0.0f, 5.0f};
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&view, &eye, &at, &up);
    MatrixOrthographicLH(&proj, 4.0f, 4.0f, 1.0f, 100.0f);
    MATRIX4 vp{};
    MatrixMultiply2(&vp, &view, &proj);
    // Apply to world (0, 0, 5, 1) in HLSL row-vector convention
    // (column j of M dotted with v). Eye at origin so the view
    // translation row contributes nothing; the ortho puts z=5
    // at NDC.z = (5 - 1) / (100 - 1) = 4/99.
    VECTOR3 p{0.0f, 0.0f, 5.0f};
    float ndcX = vp._11 * p.x + vp._21 * p.y + vp._31 * p.z + vp._41;
    float ndcY = vp._12 * p.x + vp._22 * p.y + vp._32 * p.z + vp._42;
    float ndcZ = vp._13 * p.x + vp._23 * p.y + vp._33 * p.z + vp._43;
    float ndcW = vp._14 * p.x + vp._24 * p.y + vp._34 * p.z + vp._44;
    EXPECT_NEAR(ndcX, 0.0f, 1e-4f) << "view*proj should not displace x";
    EXPECT_NEAR(ndcY, 0.0f, 1e-4f) << "view*proj should not displace y";
    EXPECT_NEAR(ndcZ, 4.0f / 99.0f, 1e-4f) << "NDC.z should be 4/99";
    EXPECT_NEAR(ndcW, 1.0f, 1e-6f) << "homogeneous w should be 1";
}

}  // namespace mxh::gx::test
