// mxh/render/mesh_flag.hpp
// 1:1 with original 4DyuchiGRX_common/mesh_flag.h (CMeshFlag, CLightFlag, CCameraFlag).
#pragma once

#include <cstdint>

namespace mxh::gx {

enum SHADE_TYPE : std::uint32_t {
    SHADE_TYPE_VERTEX_LIGHT_IM  = 0x00000000,
    SHADE_TYPE_VERTEX_LIGHT_RT  = 0x00000001,
    SHADE_TYPE_LIGHT_MAP        = 0x00000003,
};
constexpr std::uint32_t SHADE_TYPE_MASK          = 0x0000000f;
constexpr std::uint32_t SHADE_TYPE_MASK_INVERSE  = 0xfffffff0;

enum TRANSFORM_TYPE : std::uint32_t {
    TRANSFORM_TYPE_SOLID      = 0x00000000,
    TRANSFORM_TYPE_NOT_SOLID  = 0x00000010,
    TRANSFORM_TYPE_ALIGN_VIEW = 0x00000030,
    TRANSFORM_TYPE_ILLUSION   = 0x00000050,
};
constexpr std::uint32_t TRANSFORM_TYPE_MASK         = 0x000000f0;
constexpr std::uint32_t TRANSFORM_TYPE_MASK_INVERSE = 0xffffff0f;

enum RIGID_TYPE : std::uint32_t {
    RIGID_TYPE_NOT_RIGID = 0x00000000,
    RIGID_TYPE_RIGID     = 0x00000100,
};
constexpr std::uint32_t RIGID_TYPE_MASK         = 0x00000100;
constexpr std::uint32_t RIGID_TYPE_MASK_INVERSE = ~RIGID_TYPE_MASK;

enum PICK_ENABLE_TYPE : std::uint32_t {
    PICK_ENABLE  = 0x00000000,
    PICK_DISABLE = 0x00000200,
};
constexpr std::uint32_t PICK_ENABLE_TYPE_MASK         = 0x00000200;
constexpr std::uint32_t PICK_ENABLE_TYPE_MASK_INVERSE = ~PICK_ENABLE_TYPE_MASK;

constexpr int      RENDER_ZPRIORITY_DEFAULT      = 0;
constexpr float    RENDER_ZPRIORITY_UNIT         = -10.0f;
constexpr std::uint32_t RENDER_ZPRIORITY_MASK         = 0x7f000000;
constexpr std::uint32_t RENDER_ZPRIORITY_MASK_INVERSE = 0x80ffffff;
constexpr std::uint32_t WRITE_ZBUFFER_MASK            = 0x80000000;
constexpr std::uint32_t WRITE_ZBUFFER_MASK_INVERSE    = 0x7fffffff;

class CMeshFlag {
    std::uint32_t m_dwFlag = 0;
public:
    bool              IsDisableZBubfferWrite() const { return (m_dwFlag & WRITE_ZBUFFER_MASK) != 0; }
    void              DisableZBufferWrite()         { m_dwFlag |= WRITE_ZBUFFER_MASK; }
    void              EnableZBufferWrite()          { m_dwFlag &= WRITE_ZBUFFER_MASK_INVERSE; }

    int               GetRenderZPriorityValue() const;
    void              SetRenderZPriorityValue(int iZOrder);

    SHADE_TYPE        GetShadeType() const          { return static_cast<SHADE_TYPE>(m_dwFlag & SHADE_TYPE_MASK); }
    void              SetShadeType(SHADE_TYPE t)   { m_dwFlag = (m_dwFlag & SHADE_TYPE_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    TRANSFORM_TYPE    GetTransformType() const      { return static_cast<TRANSFORM_TYPE>(m_dwFlag & TRANSFORM_TYPE_MASK); }
    void              SetTransformType(TRANSFORM_TYPE t) { m_dwFlag = (m_dwFlag & TRANSFORM_TYPE_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    RIGID_TYPE        GetRigidType() const          { return static_cast<RIGID_TYPE>(m_dwFlag & RIGID_TYPE_MASK); }
    void              SetRigidType(RIGID_TYPE t)   { m_dwFlag = (m_dwFlag & RIGID_TYPE_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    PICK_ENABLE_TYPE  GetPickEnable() const         { return static_cast<PICK_ENABLE_TYPE>(m_dwFlag & PICK_ENABLE_TYPE_MASK); }
    void              SetPickEnable(PICK_ENABLE_TYPE t) { m_dwFlag = (m_dwFlag & PICK_ENABLE_TYPE_MASK_INVERSE) | static_cast<std::uint32_t>(t); }

    std::uint32_t     GetRaw() const                { return m_dwFlag; }
    void              SetRaw(std::uint32_t f)       { m_dwFlag = f; }

    CMeshFlag()                                  : m_dwFlag(0) {}
    explicit CMeshFlag(std::uint32_t f)          : m_dwFlag(f) {}
};

enum DYNAMIC_LIGHT_APPLY_TYPE : std::uint32_t {
    DYNAMIC_LIGHT_APPLY_TYPE_DISABLE             = 0x00000000,
    DYNAMIC_LIGHT_APPLY_TYPE_CHARACTER_ENABLE    = 0x00000001,
    DYNAMIC_LIGHT_APPLY_TYPE_MAP_ENABLE          = 0x00000002,
    DYNAMIC_LIGHT_APPLY_TYPE_BOTH_ENABLE         = 0x00000003,
};
constexpr std::uint32_t DYNAMIC_LIGHT_APPLY_TYPE_MASK         = 0x0000000f;
constexpr std::uint32_t DYNAMIC_LIGHT_APPLY_TYPE_MASK_INVERSE = 0xfffffff0;

class CLightFlag {
    std::uint32_t m_dwFlag = 0;
public:
    DYNAMIC_LIGHT_APPLY_TYPE GetDynamicLightType() const         { return static_cast<DYNAMIC_LIGHT_APPLY_TYPE>(m_dwFlag & DYNAMIC_LIGHT_APPLY_TYPE_MASK); }
    void                     SetDynamicLightType(DYNAMIC_LIGHT_APPLY_TYPE t) { m_dwFlag = (m_dwFlag & DYNAMIC_LIGHT_APPLY_TYPE_MASK_INVERSE) | static_cast<std::uint32_t>(t); }
    CLightFlag() : m_dwFlag(0) {}
};

class CCameraFlag {
    std::uint32_t m_dwFlag = 0;
};

} // namespace mxh::gx