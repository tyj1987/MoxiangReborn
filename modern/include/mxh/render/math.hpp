// mxh/render/math.hpp
// 1:1 with original 4DyuchiGRX_common/math.inl (DirectX 8-era fixed-point math types).
// All types kept as plain structs so binary layouts match the original COM interfaces.
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
inline void MatrixOrthographicLH(MATRIX4* pOut, float w, float h, float zn, float zf) {
    if (!pOut) return;
    *pOut = MatrixIdentity();
    pOut->_11 = 2.0f / w;
    pOut->_22 = 2.0f / h;
    pOut->_33 = 1.0f / (zf - zn);
    pOut->_43 = -zn / (zf - zn);
    pOut->_44 = 1.0f;
}

// Row-major look-at (view) matrix — eye looking at target with up vector.
// Result transforms world space so that looking down -Z (LH convention).
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
    // Row-major view matrix:
    // R·R  R·U  R·F  -dot(R,eye)
    // U·R  U·U  U·F  -dot(U,eye)
    // F·R  F·U  F·F  -dot(F,eye)
    //   0    0    0        1
    pOut->_11 = r.x; pOut->_12 = r.y; pOut->_13 = r.z; pOut->_14 = -(r.x*pEye->x + r.y*pEye->y + r.z*pEye->z);
    pOut->_21 = u.x; pOut->_22 = u.y; pOut->_23 = u.z; pOut->_24 = -(u.x*pEye->x + u.y*pEye->y + u.z*pEye->z);
    pOut->_31 = f.x; pOut->_32 = f.y; pOut->_33 = f.z; pOut->_34 = -(f.x*pEye->x + f.y*pEye->y + f.z*pEye->z);
    pOut->_41 = 0.0f; pOut->_42 = 0.0f; pOut->_43 = 0.0f; pOut->_44 = 1.0f;
}

// Set column j (j=0..3) of a row-major matrix to a 4-element vector.
inline void setMatrixColumn(MATRIX4* m, int j, float x, float y, float z, float w) {
    if (!m || j < 0 || j > 3) return;
    m->m[0][j] = x; m->m[1][j] = y; m->m[2][j] = z; m->m[3][j] = w;
}

} // namespace mxh::gx