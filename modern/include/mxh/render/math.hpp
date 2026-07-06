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
    return r;
}

} // namespace mxh::gx