// math_test.cpp - Phase 10.19 math primitives + matrix helpers test
//
// Covers modern/include/mxh/render/math.hpp — the VECTOR2/3/4
// POD structs, the row-major MATRIX4, and the matrix helpers
// (MatrixIdentity, MatrixMultiply2, MatrixOrthographicLH,
// MatrixLookAtLH, setIdentityMatrix, setMatrixColumn).
//
// What's tested:
//   - VECTOR2/3/4 / MATRIX4 wire-format sizes (1:1 with the
//     original 4DyuchiGRX_common/math.inl from the legacy
//     2003-era engine).
//   - PI / PI_MUL_2 / PI_DIV_2 / PI_DIV_4 / INV_PI /
//     DEFAULT_FOV constants.
//   - MatrixIdentity returns the standard 4x4 identity.
//   - setIdentityMatrix is in-place identity.
//   - MatrixMultiply2 is row-major (verified with a known
//     non-commutative case).
//   - MatrixOrthographicLH produces the standard LH ortho
//     matrix for a 2x2x1..100 frustum.
//   - MatrixLookAtLH produces a valid LH view matrix for a
//     standard eye→target setup; degenerate cases (at==eye)
//     fall back to identity without crashing.
//   - setMatrixColumn writes into column j correctly.
//   - All helpers are null-safe (the legacy code is defensive
//     against null pResult pointers).

#include "mxh/render/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <type_traits>

namespace mxh::gx::test {

// ===========================================================================
// POD struct sizes / layout
// ===========================================================================

TEST(VectorSizeTest, Vector2IsTwoFloats) {
    static_assert(sizeof(VECTOR2) == 2 * sizeof(float),
                  "VECTOR2 must be 8 bytes (two float32s)");
    EXPECT_EQ(sizeof(VECTOR2), 8u);
}

TEST(VectorSizeTest, Vector3IsThreeFloats) {
    static_assert(sizeof(VECTOR3) == 3 * sizeof(float),
                  "VECTOR3 must be 12 bytes (three float32s)");
    EXPECT_EQ(sizeof(VECTOR3), 12u);
}

TEST(VectorSizeTest, Vector4IsFourFloats) {
    static_assert(sizeof(VECTOR4) == 4 * sizeof(float),
                  "VECTOR4 must be 16 bytes (four float32s)");
    EXPECT_EQ(sizeof(VECTOR4), 16u);
}

TEST(MatrixSizeTest, Matrix4IsSixteenFloats) {
    // MATRIX4 is a 4x4 of float32. 16 * 4 = 64 bytes. The
    // union with float m[4][4] guarantees no extra padding.
    static_assert(sizeof(MATRIX4) == 16 * sizeof(float),
                  "MATRIX4 must be 64 bytes (16 float32s)");
    EXPECT_EQ(sizeof(MATRIX4), 64u);
}

TEST(MatrixSizeTest, Matrix4FieldLayoutMatchesArray) {
    // The union means _ij and m[i][j] address the same storage.
    // Pin this so a future "modernization" that adds a different
    // struct field shows up here as a deliberate test update.
    MATRIX4 m = MatrixIdentity();
    EXPECT_EQ(&m._11, &m.m[0][0]);
    EXPECT_EQ(&m._14, &m.m[0][3]);
    EXPECT_EQ(&m._41, &m.m[3][0]);
    EXPECT_EQ(&m._44, &m.m[3][3]);
}

TEST(VectorLayoutTest, Vector3FieldOrder) {
    // The order is x, y, z — pin so any reordering (e.g. SIMD
    // layout) is caught.
    VECTOR3 v{1.0f, 2.0f, 3.0f};
    EXPECT_EQ(v.x, 1.0f);
    EXPECT_EQ(v.y, 2.0f);
    EXPECT_EQ(v.z, 3.0f);
}

// ===========================================================================
// PI constants
// ===========================================================================

TEST(PiConstantsTest, ValuesAreCloseToExpected) {
    // 1.5e-7 is well within float precision for these constants.
    EXPECT_NEAR(PI,       3.14159265f, 1e-6f);
    EXPECT_NEAR(PI_MUL_2, 6.28318530f, 1e-6f);
    EXPECT_NEAR(PI_DIV_2, 1.57079632f, 1e-6f);
    EXPECT_NEAR(PI_DIV_4, 0.78539816f, 1e-6f);
    EXPECT_NEAR(INV_PI,   0.31830988f, 1e-6f);
}

TEST(PiConstantsTest, DefaultFovIs45Degrees) {
    // DEFAULT_FOV = PI / 4.0f = 45 degrees — the legacy engine
    // used 45° as the default render field-of-view.
    EXPECT_NEAR(DEFAULT_FOV, PI / 4.0f, 1e-7f);
    EXPECT_NEAR(DEFAULT_FOV, 0.78539816f, 1e-6f);
}

TEST(PiConstantsTest, DoublesAndHalvesAreConsistent) {
    EXPECT_NEAR(PI_MUL_2, 2.0f * PI, 1e-5f);
    EXPECT_NEAR(PI_DIV_2, PI / 2.0f, 1e-7f);
    EXPECT_NEAR(PI_DIV_4, PI / 4.0f, 1e-7f);
    EXPECT_NEAR(INV_PI,   1.0f / PI, 1e-6f);
}

// ===========================================================================
// MatrixIdentity / setIdentityMatrix
// ===========================================================================

TEST(MatrixIdentityTest, ReturnsStandard4x4Identity) {
    MATRIX4 m = MatrixIdentity();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float expected = (i == j) ? 1.0f : 0.0f;
            EXPECT_EQ(m.m[i][j], expected)
                << "i=" << i << " j=" << j;
        }
    }
}

TEST(MatrixIdentityTest, IsInPlaceSafe) {
    // Two consecutive identity calls should be stable.
    MATRIX4 m = MatrixIdentity();
    MATRIX4 m2 = MatrixIdentity();
    EXPECT_EQ(std::memcmp(&m, &m2, sizeof(MATRIX4)), 0);
}

TEST(SetIdentityMatrixTest, OverwritesAllFields) {
    MATRIX4 m{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m.m[i][j] = 99.0f;
    setIdentityMatrix(&m);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float expected = (i == j) ? 1.0f : 0.0f;
            EXPECT_EQ(m.m[i][j], expected);
        }
    }
}

TEST(SetIdentityMatrixTest, NullPointerIsNoOp) {
    // The legacy code is defensive — a null m should be a no-op,
    // not a crash.
    setIdentityMatrix(nullptr);
    SUCCEED();  // did not crash
}

// ===========================================================================
// MatrixMultiply2 — row-major semantics
// ===========================================================================

TEST(MatrixMultiplyTest, IdentityTimesAnyIsAny) {
    MATRIX4 I = MatrixIdentity();
    MATRIX4 A{};
    A._11 = 1.0f; A._12 = 2.0f; A._13 = 3.0f; A._14 = 4.0f;
    A._21 = 5.0f; A._22 = 6.0f; A._23 = 7.0f; A._24 = 8.0f;
    A._31 = 9.0f; A._32 = 10.0f; A._33 = 11.0f; A._34 = 12.0f;
    A._41 = 13.0f; A._42 = 14.0f; A._43 = 15.0f; A._44 = 16.0f;
    MATRIX4 R{};
    MatrixMultiply2(&R, &I, &A);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_EQ(R.m[i][j], A.m[i][j]);
    // And the other side: A * I == A
    MatrixMultiply2(&R, &A, &I);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_EQ(R.m[i][j], A.m[i][j]);
}

TEST(MatrixMultiplyTest, NonCommutative) {
    // A pair of clearly non-commutative 2x2-in-top-corner
    // matrices, embedded in 4x4. The implementation uses the
    // standard math convention result[i][j] = sum_k A[i][k] * B[k][j]
    // (row-major multiply, row-major storage). Verify both
    // products and confirm they differ.
    MATRIX4 A{};
    MATRIX4 B{};
    // A in the top-left corner:
    A._11 = 1.0f; A._12 = 2.0f;
    A._21 = 3.0f; A._22 = 4.0f;
    A._33 = 1.0f; A._44 = 1.0f;
    // B in the top-left corner:
    B._11 = 5.0f; B._12 = 6.0f;
    B._21 = 7.0f; B._22 = 8.0f;
    B._33 = 1.0f; B._44 = 1.0f;
    MATRIX4 AB{}, BA{};
    MatrixMultiply2(&AB, &A, &B);
    MatrixMultiply2(&BA, &B, &A);
    // A*B top-left = A as 2x2 times B as 2x2:
    //   [1 2] [5 6]   [1*5+2*7  1*6+2*8]   [19 22]
    //   [3 4] [7 8] = [3*5+4*7  3*6+4*8] = [43 50]
    EXPECT_EQ(AB._11, 19.0f);
    EXPECT_EQ(AB._12, 22.0f);
    EXPECT_EQ(AB._21, 43.0f);
    EXPECT_EQ(AB._22, 50.0f);
    // B*A top-left:
    //   [5 6] [1 2]   [5*1+6*3  5*2+6*4]   [23 34]
    //   [7 8] [3 4] = [7*1+8*3  7*2+8*4] = [31 46]
    EXPECT_EQ(BA._11, 23.0f);
    EXPECT_EQ(BA._12, 34.0f);
    EXPECT_EQ(BA._21, 31.0f);
    EXPECT_EQ(BA._22, 46.0f);
    // A*B != B*A
    EXPECT_NE(AB._11, BA._11);
}

TEST(MatrixMultiplyTest, NullPointerIsNoOp) {
    MATRIX4 a = MatrixIdentity();
    MATRIX4 b = MatrixIdentity();
    MATRIX4 r = MatrixIdentity();
    MatrixMultiply2(nullptr, &a, &b);  // null result
    MatrixMultiply2(&r, nullptr, &b);  // null A
    MatrixMultiply2(&r, &a, nullptr);  // null B
    // r should be unchanged (still identity from before).
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_EQ(r.m[i][j], (i == j) ? 1.0f : 0.0f);
}

// ===========================================================================
// MatrixOrthographicLH — maps scene volume to NDC depth [0,1]
// ===========================================================================

TEST(MatrixOrthoTest, TwoByTwoUnitBox) {
    // w=2, h=2, zn=1, zf=3:
    //   _11 = 2/w = 1, _22 = 2/h = 1, _33 = 1/(zf-zn) = 0.5,
    //   _43 = -zn/(zf-zn) = -0.5, _44 = 1
    MATRIX4 m{};
    MatrixOrthographicLH(&m, 2.0f, 2.0f, 1.0f, 3.0f);
    EXPECT_EQ(m._11, 1.0f);
    EXPECT_EQ(m._22, 1.0f);
    EXPECT_EQ(m._33, 0.5f);
    EXPECT_EQ(m._43, -0.5f);
    EXPECT_EQ(m._44, 1.0f);
    // All other cells should be 0.
    EXPECT_EQ(m._12, 0.0f); EXPECT_EQ(m._13, 0.0f); EXPECT_EQ(m._14, 0.0f);
    EXPECT_EQ(m._21, 0.0f); EXPECT_EQ(m._23, 0.0f); EXPECT_EQ(m._24, 0.0f);
    EXPECT_EQ(m._31, 0.0f); EXPECT_EQ(m._32, 0.0f); EXPECT_EQ(m._34, 0.0f);
    EXPECT_EQ(m._41, 0.0f); EXPECT_EQ(m._42, 0.0f);
}

TEST(MatrixOrthoTest, WiderBoxScalesX) {
    // w=4, h=2: _11 = 0.5, _22 = 1
    MATRIX4 m{};
    MatrixOrthographicLH(&m, 4.0f, 2.0f, 0.0f, 10.0f);
    EXPECT_EQ(m._11, 0.5f);
    EXPECT_EQ(m._22, 1.0f);
    EXPECT_EQ(m._33, 0.1f);   // 1/(10-0)
    EXPECT_EQ(m._43, 0.0f);   // -0/10
}

TEST(MatrixOrthoTest, NullPointerIsNoOp) {
    MatrixOrthographicLH(nullptr, 1.0f, 1.0f, 0.0f, 1.0f);
    SUCCEED();
}

// ===========================================================================
// MatrixLookAtLH
// ===========================================================================

TEST(MatrixLookAtTest, StandardForwardView) {
    // Eye at (0, 0, -5) looking at origin with up (0, 1, 0).
    // forward = (at - eye)/|at - eye| = (0, 0, 5)/5 = (0, 0, 1)
    // right = forward × up = (0, 0, 1) × (0, 1, 0)
    //       = (0*0 - 1*1, 1*0 - 0*0, 0*1 - 0*0) = (-1, 0, 0)
    // up = right × forward = (-1, 0, 0) × (0, 0, 1)
    //    = (0*1 - 0*0, 0*0 - (-1)*1, -1*0 - 0*0) = (0, 1, 0)
    // The view matrix rows are R, U, F, (0,0,0,1), with
    // translation column = -dot(R/U/F, eye).
    MATRIX4 m{};
    VECTOR3 eye{0.0f, 0.0f, -5.0f};
    VECTOR3 at{0.0f, 0.0f, 0.0f};
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    // Right axis: (-1, 0, 0) — the cross-product f×up convention
    // produces a left-handed view (the world's +X appears on the
    // left of the screen). This is the original engine's quirk.
    EXPECT_NEAR(m._11, -1.0f, 1e-5f);
    EXPECT_NEAR(m._12,  0.0f, 1e-5f);
    EXPECT_NEAR(m._13,  0.0f, 1e-5f);
    // Up axis: (0, 1, 0)
    EXPECT_NEAR(m._21, 0.0f, 1e-5f);
    EXPECT_NEAR(m._22, 1.0f, 1e-5f);
    EXPECT_NEAR(m._23, 0.0f, 1e-5f);
    // Forward axis: (0, 0, 1)
    EXPECT_NEAR(m._31, 0.0f, 1e-5f);
    EXPECT_NEAR(m._32, 0.0f, 1e-5f);
    EXPECT_NEAR(m._33, 1.0f, 1e-5f);
    // Bottom row: (0, 0, 0, 1)
    EXPECT_NEAR(m._41, 0.0f, 1e-5f);
    EXPECT_NEAR(m._42, 0.0f, 1e-5f);
    EXPECT_NEAR(m._43, 0.0f, 1e-5f);
    EXPECT_NEAR(m._44, 1.0f, 1e-5f);
    // Translation: -dot(R, eye) = 0, -dot(U, eye) = 0,
    // -dot(F, eye) = -(0+0+1*(-5)) = 5.
    EXPECT_NEAR(m._14, 0.0f, 1e-5f);
    EXPECT_NEAR(m._24, 0.0f, 1e-5f);
    EXPECT_NEAR(m._34, 5.0f, 1e-5f);
}

TEST(MatrixLookAtTest, EyeMapsToOrigin) {
    // A view matrix should transform the eye position to the
    // origin (the camera is at the origin in its own view
    // space), regardless of eye orientation. Test this with a
    // non-axis-aligned setup.
    MATRIX4 m{};
    VECTOR3 eye{3.0f, 4.0f, 5.0f};
    VECTOR3 at{0.0f, 0.0f, 0.0f};
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    // (m * eye).x = R.x * eye.x + R.y * eye.y + R.z * eye.z + (-dot(R, eye))
    //             = 0 by construction
    // (m * eye).y = 0 by construction
    // (m * eye).z = F.x * eye.x + F.y * eye.y + F.z * eye.z + (-dot(F, eye))
    //             = 0 by construction
    float tx = m._11 * eye.x + m._12 * eye.y + m._13 * eye.z + m._14;
    float ty = m._21 * eye.x + m._22 * eye.y + m._23 * eye.z + m._24;
    float tz = m._31 * eye.x + m._32 * eye.y + m._33 * eye.z + m._34;
    EXPECT_NEAR(tx, 0.0f, 1e-4f);
    EXPECT_NEAR(ty, 0.0f, 1e-4f);
    EXPECT_NEAR(tz, 0.0f, 1e-4f);
}

TEST(MatrixLookAtTest, TargetIsAlongForwardAxis) {
    // The target should be along the forward axis at the
    // distance |at - eye|. In this implementation the
    // forward axis row is (m._31, m._32, m._33), and the
    // target maps to (0, 0, |at - eye|) in view space (the
    // camera looks toward +Z in this convention, not -Z as
    // the hpp comment claims — pinned so a future
    // "left-hand-ify" change is caught).
    MATRIX4 m{};
    VECTOR3 eye{0.0f, 0.0f, 0.0f};
    VECTOR3 at{0.0f, 0.0f, 10.0f};  // 10 units along +Z
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    float tx = m._11 * at.x + m._12 * at.y + m._13 * at.z + m._14;
    float ty = m._21 * at.x + m._22 * at.y + m._23 * at.z + m._24;
    float tz = m._31 * at.x + m._32 * at.y + m._33 * at.z + m._34;
    EXPECT_NEAR(tx, 0.0f, 1e-4f);
    EXPECT_NEAR(ty, 0.0f, 1e-4f);
    EXPECT_NEAR(tz, 10.0f, 1e-4f);
}

TEST(MatrixLookAtTest, DegenerateEyeEqualsAtFallsBackToIdentity) {
    // When eye == at the forward vector is zero, can't
    // normalize. The original code falls back to identity so
    // the caller doesn't crash. Pin the fallback behavior.
    MATRIX4 m{};
    VECTOR3 eye{1.0f, 2.0f, 3.0f};
    VECTOR3 at{1.0f, 2.0f, 3.0f};   // == eye
    VECTOR3 up{0.0f, 1.0f, 0.0f};
    MatrixLookAtLH(&m, &eye, &at, &up);
    MATRIX4 I = MatrixIdentity();
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_EQ(m.m[i][j], I.m[i][j]);
}

TEST(MatrixLookAtTest, NullPointerIsNoOp) {
    VECTOR3 eye{0,0,0}, at{0,0,1}, up{0,1,0};
    MatrixLookAtLH(nullptr, &eye, &at, &up);
    MatrixLookAtLH(nullptr, nullptr, &at, &up);
    MatrixLookAtLH(nullptr, &eye, nullptr, &up);
    MatrixLookAtLH(nullptr, &eye, &at, nullptr);
    SUCCEED();
}

// ===========================================================================
// setMatrixColumn
// ===========================================================================

TEST(SetMatrixColumnTest, ColumnZero) {
    MATRIX4 m{};
    setMatrixColumn(&m, 0, 1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(m.m[0][0], 1.0f);
    EXPECT_EQ(m.m[1][0], 2.0f);
    EXPECT_EQ(m.m[2][0], 3.0f);
    EXPECT_EQ(m.m[3][0], 4.0f);
    // Other columns untouched (still 0)
    for (int j = 1; j < 4; ++j)
        for (int i = 0; i < 4; ++i)
            EXPECT_EQ(m.m[i][j], 0.0f) << "i=" << i << " j=" << j;
}

TEST(SetMatrixColumnTest, AllFourColumns) {
    MATRIX4 m{};
    setMatrixColumn(&m, 0, 1.0f, 2.0f, 3.0f, 4.0f);
    setMatrixColumn(&m, 1, 5.0f, 6.0f, 7.0f, 8.0f);
    setMatrixColumn(&m, 2, 9.0f, 10.0f, 11.0f, 12.0f);
    setMatrixColumn(&m, 3, 13.0f, 14.0f, 15.0f, 16.0f);
    // setMatrixColumn writes column j = (x, y, z, w) into
    // m[0][j] = x, m[1][j] = y, m[2][j] = z, m[3][j] = w. So
    // after 4 setMatrixColumn calls the matrix holds 1..16 in
    // column-major order:
    //   m[0] = (1, 5, 9, 13)
    //   m[1] = (2, 6, 10, 14)
    //   m[2] = (3, 7, 11, 15)
    //   m[3] = (4, 8, 12, 16)
    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i)
            EXPECT_EQ(m.m[i][j], static_cast<float>(j * 4 + i + 1))
                << "i=" << i << " j=" << j;
}

TEST(SetMatrixColumnTest, InvalidColumnIndexIsNoOp) {
    MATRIX4 m = MatrixIdentity();
    setMatrixColumn(&m, -1, 9.0f, 9.0f, 9.0f, 9.0f);
    setMatrixColumn(&m, 4,  9.0f, 9.0f, 9.0f, 9.0f);
    // m is still identity.
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_EQ(m.m[i][j], (i == j) ? 1.0f : 0.0f);
}

TEST(SetMatrixColumnTest, NullPointerIsNoOp) {
    setMatrixColumn(nullptr, 0, 0.0f, 0.0f, 0.0f, 0.0f);
    SUCCEED();
}

}  // namespace mxh::gx::test
