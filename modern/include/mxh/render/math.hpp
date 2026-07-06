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

} // namespace mxh::gx