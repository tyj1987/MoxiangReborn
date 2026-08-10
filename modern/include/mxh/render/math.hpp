// mxh/render/math.hpp
// 1:1 with original 4DyuchiGRX_common/math.inl (DirectX 8-era fixed-point math types).
// All types kept as plain structs so binary layouts match the original COM interfaces.
//
// Convention: D3DX-style row-major naming on a row-major physical memory layout.
//   - `M._ij` is a mathematical row-i, column-j element (the standard DirectX
//     naming convention since D3DX 8.x). The union with `m[4][4]` puts _ij at the
//     same memory location as `m[i][j]`, so the physical memory layout is
//     `m[0][0] m[0][1] m[0][2] m[0][3] m[1][0] ...` — i.e. row 0 floats are
//     contiguous, then row 1, etc.
//   - HLSL `float4x4` defaults to column-major physical packing. When CPU code
//     `memcpy`s a `MATRIX4` straight into a `cbuffer float4x4` slot, HLSL
//     interprets the bytes as `M[0][0] m[0][1] m[0][2] m[0][3] m[1][0] ...`
//     which is the same memory order (and equivalent to the transpose of the
//     math matrix that HLSL uses internally). The project's HLSL shaders
//     therefore use `mul(v_row, M)` to stay consistent with the CPU-side
//     D3DX-style layout.
//   - MatrixMultiply2 follows the D3DX matrix multiplication formula:
//     `result._ij = sum_k a._ik * b._kj` (row-major math, row-major storage).
//   - MatrixLookAtLH / MatrixOrthographicLH produce D3DX-compatible outputs:
//       MatrixLookAtLH:        MatrixOrthographicLH (rows = function of basis/extent):
//         _11=R.x _12=U.x _13=F.x _14=-P.R    _11=2/w _12=0   _13=0       _14=0
//         _21=R.y _22=U.y _23=F.y _24=-P.U    _21=0   _22=2/h _23=0       _24=0
//         _31=R.z _32=U.z _33=F.z _34=-P.F    _31=0   _32=0   _33=1/(zf-zn) _34=0
//         _41=0   _42=0   _43=0   _44=1        _41=0   _42=0   _43=-zn/(zf-zn) _44=1
//     Translation lives in column 3 (the basis dot products with -P), and
//     z-translation in row 3 col 2 for the ortho (pre-perspective-divide row).
//   - R-9 (Phase 12.x) audit: the previous implementation wrote column-major
//     data into the row-major storage (e.g. _43 = -zn/(zf-zn) instead of
//     _32, and the view basis as the first three rows instead of the first
//     three columns). It happened to work for axis-aligned camera setups
//     (R.x=1, U.y=1, F.z=1) but produced wrong rotation/translation for
//     any off-axis eye. This file now uses the D3DX-correct layout and the
//     `math_d3dx_convention_test.cpp` file pins the convention with
//     off-axis tests.
#pragma once

#include <cmath>

namespace mxh::gx {

struct VECTOR2 {
    float x;
    float y;
};

struct VECTOR3 {
    float x;
    float y;
    float z;
};

struct VECTOR4 {
    float x;
    float y;
    float z;
    float w;
};

// Row-major 4x4 matrix to match the original's direct layout (no transpose).
// Original matrices are documented as column-major in DirectX but the project's
// matrix ops (math.inl) treat them as row-major. We keep the same convention
// to preserve 1:1 numerical behavior with the original engine.
struct MATRIX4 {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

constexpr float PI        = 3.14159265358979323846f;
constexpr float PI_MUL_2  = 6.28318530717958623200f;
constexpr float PI_DIV_2  = 1.57079632679489655800f;
constexpr float PI_DIV_4  = 0.78539816339744827900f;
constexpr float INV_PI    = 0.31830988618379069122f;
constexpr float DEFAULT_FOV = PI / 4.0f;

inline MATRIX4 MatrixIdentity() {
    MATRIX4 r{};
    r._11 = r._22 = r._33 = r._44 = 1.0f;
    // Zero the off-diagonal elements (r{} only zero-initializes the struct,
    // which we then partially overwrite with diagonal 1s — explicit is safer).
    r._12 = r._13 = r._14 = 0.0f;
    r._21 = r._23 = r._24 = 0.0f;
    r._31 = r._32 = r._34 = 0.0f;
    r._41 = r._42 = r._43 = 0.0f;
    return r;
}

// Set identity matrix in-place.
inline void setIdentityMatrix(MATRIX4* m) {
    if (!m) return;
    m->_11 = m->_22 = m->_33 = m->_44 = 1.0f;
    m->_12 = m->_13 = m->_14 = m->_21 = m->_23 = m->_24 = 0.0f;
    m->_31 = m->_32 = m->_34 = m->_41 = m->_42 = m->_43 = 0.0f;
}

// Row-major matrix multiply: result = a × b.
inline void MatrixMultiply2(MATRIX4* pResult, const MATRIX4* pA, const MATRIX4* pB) {
    if (!pResult || !pA || !pB) return;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += pA->m[i][k] * pB->m[k][j];
            }
            pResult->m[i][j] = sum;
        }
    }
}

// Row-major orthographic projection (maps scene volume to NDC depth [0,1]).
// Width/height are the half-extents; near/far are positive distances.
// D3DX row-major layout: the basis vectors live in row 0/1/2 columns 0..2,
// and the affine translation lives in the BOTTOM row (_41, _42, _43, _44).
// The z-translation that pushes the near plane to NDC z = 0 lives in _43
// (bottom row col 2) — pre-perspective-divide.
inline void MatrixOrthographicLH(MATRIX4* pOut, float w, float h, float zn, float zf) {
    if (!pOut) return;
    *pOut = MatrixIdentity();
    pOut->_11 = 2.0f / w;
    pOut->_22 = 2.0f / h;
    pOut->_33 = 1.0f / (zf - zn);
    pOut->_43 = -zn / (zf - zn);   // R-9 fix: was _32 (placed the offset
                                   // in row 2 col 2, producing a translation
                                   // that would re-appear in the
                                   // perspective-divide row).
    pOut->_44 = 1.0f;
}

// Row-major look-at (view) matrix — eye looking at target with up vector.
// D3DX-compatible row-major layout: the basis vectors (R, U, F) live along
// the columns of the upper-3x3 submatrix (row 0 = (R.x, U.x, F.x, 0), etc.),
// and the affine translation lives in the BOTTOM row (_41, _42, _43, _44).
// A row-major `mul(v_row, M)` from HLSL gives the same world-to-view
// transform that `D3DXVec3TransformCoord` did in the original engine when
// the CPU matrix is uploaded as-is (the HLSL column-major packing then
// acts as the mathematical transpose of the row-major CPU layout, which
// matches the D3DX row-major transform convention).
inline void MatrixLookAtLH(MATRIX4* pOut, const VECTOR3* pEye,
                           const VECTOR3* pAt, const VECTOR3* pUp) {
    if (!pOut || !pEye || !pAt || !pUp) return;
    VECTOR3 f{}, r{}, u{};
    // forward = normalize(at - eye)
    float fx = pAt->x - pEye->x, fy = pAt->y - pEye->y, fz = pAt->z - pEye->z;
    float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-6f) { *pOut = MatrixIdentity(); return; }
    f = { fx/fl, fy/fl, fz/fl };
    // right = normalize(forward x up)
    float rx = f.y*pUp->z - f.z*pUp->y;
    float ry = f.z*pUp->x - f.x*pUp->z;
    float rz = f.x*pUp->y - f.y*pUp->x;
    float rl = std::sqrt(rx*rx + ry*ry + rz*rz);
    if (rl < 1e-6f) { *pOut = MatrixIdentity(); return; }
    r = { rx/rl, ry/rl, rz/rl };
    // up = right x forward (LH)
    u = { r.y*f.z - r.z*f.y, r.z*f.x - r.x*f.z, r.x*f.y - r.y*f.x };
    // D3DX row-major view matrix:
    //   _11=R.x _12=U.x _13=F.x _14=0
    //   _21=R.y _22=U.y _23=F.y _24=0
    //   _31=R.z _32=U.z _33=F.z _34=0
    //   _41=-P.R _42=-P.U _43=-P.F _44=1
    pOut->_11 = r.x; pOut->_12 = u.x; pOut->_13 = f.x; pOut->_14 = 0.0f;
    pOut->_21 = r.y; pOut->_22 = u.y; pOut->_23 = f.y; pOut->_24 = 0.0f;
    pOut->_31 = r.z; pOut->_32 = u.z; pOut->_33 = f.z; pOut->_34 = 0.0f;
    pOut->_41 = -(r.x*pEye->x + r.y*pEye->y + r.z*pEye->z);
    pOut->_42 = -(u.x*pEye->x + u.y*pEye->y + u.z*pEye->z);
    pOut->_43 = -(f.x*pEye->x + f.y*pEye->y + f.z*pEye->z);
    pOut->_44 = 1.0f;
}

// Row-major screen-space orthographic projection: maps pixel coordinates
// (x in [0, width], y in [0, height]) to NDC ([-1, 1] x [1, -1]). The matrix
// uses the same row-major layout as MatrixOrthographicLH: diagonal scale on
// rows 0 and 1, translation on the BOTTOM row (_14, _24, _44=1). Combined
// with the HLSL primitive VS (mul(row, M) with default column-major packing
// of row-major cbuffer data), this turns into the desired -1/+1 pixel-to-NDC
// translation. width and height are clamped to >= 1 to avoid divide-by-zero
// when a window has not yet been sized.
inline void MatrixScreenOrtho(MATRIX4* pOut, float width, float height) {
    if (!pOut) return;
    if (width  < 1.0f) width  = 1.0f;
    if (height < 1.0f) height = 1.0f;
    *pOut = MatrixIdentity();
    pOut->_11 =  2.0f / width;
    pOut->_22 = -2.0f / height;
    pOut->_14 = -1.0f;
    pOut->_24 =  1.0f;
    pOut->_44 =  1.0f;
}

// Set column j (j=0..3) of a row-major matrix to a 4-element vector.
inline void setMatrixColumn(MATRIX4* m, int j, float x, float y, float z, float w) {
    if (!m || j < 0 || j > 3) return;
    m->m[0][j] = x; m->m[1][j] = y; m->m[2][j] = z; m->m[3][j] = w;
}

} // namespace mxh::gx