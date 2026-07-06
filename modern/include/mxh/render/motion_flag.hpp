// mxh/render/motion_flag.hpp
// 1:1 with original 4DyuchiGRX_common/motion_flag.h.
#pragma once

#include <cstdint>

namespace mxh::gx {

enum MOTION_TYPE_KEYFRAME : std::uint32_t {
    MOTION_TYPE_KEYFRAME_ENABLE  = 0x00000000,
    MOTION_TYPE_KEYFRAME_DISABLE = 0x00000001,
};
constexpr std::uint32_t MOTION_TYPE_KEYFRAME_MASK         = 0x0000000f;
constexpr std::uint32_t MOTION_TYPE_KEYFRAME_MASK_INVERSE = 0xfffffff0;

enum MOTION_TYPE_VERTEX : std::uint32_t {
    MOTION_TYPE_VERTEX_DISABLE = 0x00000000,
    MOTION_TYPE_VERTEX_ENABLE  = 0x00000010,
};
constexpr std::uint32_t MOTION_TYPE_VERTEX_MASK         = 0x000000f0;
constexpr std::uint32_t MOTION_TYPE_VERTEX_MASK_INVERSE = 0xffffff0f;

enum MOTION_TYPE_UV : std::uint32_t {
    MOTION_TYPE_UV_DISABLE = 0x00000000,
    MOTION_TYPE_UV_ENABLE  = 0x00000100,
};
constexpr std::uint32_t MOTION_TYPE_UV_MASK         = 0x00000f00;
constexpr std::uint32_t MOTION_TYPE_UV_MASK_INVERSE = 0xfffff0ff;

class CMotionFlag {
    std::uint32_t m_dwFlag = 0;
public:
    MOTION_TYPE_KEYFRAME GetMotionTypeKeyFrame() const         { return static_cast<MOTION_TYPE_KEYFRAME>(m_dwFlag & MOTION_TYPE_KEYFRAME_MASK); }
    void                 SetMotionTypeKeyFrame(MOTION_TYPE_KEYFRAME t) { m_dwFlag = (m_dwFlag & MOTION_TYPE_KEYFRAME_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    MOTION_TYPE_VERTEX   GetMotionTypeVertex() const           { return static_cast<MOTION_TYPE_VERTEX>(m_dwFlag & MOTION_TYPE_VERTEX_MASK); }
    void                 SetMotionTypeVertex(MOTION_TYPE_VERTEX t)     { m_dwFlag = (m_dwFlag & MOTION_TYPE_VERTEX_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    MOTION_TYPE_UV       GetMotionTypeUV() const               { return static_cast<MOTION_TYPE_UV>(m_dwFlag & MOTION_TYPE_UV_MASK); }
    void                 SetMotionTypeUV(MOTION_TYPE_UV t)     { m_dwFlag = (m_dwFlag & MOTION_TYPE_UV_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    std::uint32_t        GetRaw() const { return m_dwFlag; }
    CMotionFlag() : m_dwFlag(0) {}
};

} // namespace mxh::gx