// mxh/render/dx11/dynamic_light.hpp
// Dynamic Light management: up to 8 simultaneous lights (CreateDynamicLight / DeleteDynamicLight).
// Each light can be directional or point-light. LIGHT_INDEX_DESC maps light indices to material
// handles for per-face-group lighting in RenderMeshObject.
#pragma once

#include <cstdint>
#include <cstring>
#include <array>

#include <d3d11.h>
#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

// Maximum simultaneous dynamic lights (matches DX8 LIGHT_STAGE limit).
constexpr std::uint32_t MAX_DYNAMIC_LIGHTS = 8;

// TriBuffer magic handle constant (used to validate TriBuffer handles).
inline constexpr std::uint64_t TRI_BUFFER_MAGIC = 0x54524942u; // "TRIB"

// Light flags (mirrors original RS_LIGHT_* render states).
constexpr std::uint32_t LIGHT_FLAG_ENABLE       = 0x00000001;
constexpr std::uint32_t LIGHT_FLAG_DIRECTIONAL  = 0x00000002;
constexpr std::uint32_t LIGHT_FLAG_POINT         = 0x00000004;
constexpr std::uint32_t LIGHT_FLAG_SPOT          = 0x00000008;

// Dynamic light entry. Stored in m_dynamicLights[8] array.
struct DynamicLight {
    bool         bActive      = false;
    bool         bDirectional = true;   // true=directional, false=point
    std::uint32_t dwRS        = 0;     // render-state flags
    std::uint32_t dwColor     = 0xffc8c8c8; // 0xAABBGGRR default light-gray

    // Directional light
    float        v3Dir[3]     = {0.f, -1.f, 0.f}; // normalised
    float        fAmbient     = 0.25f;
    float        fDiffuse     = 0.95f;

    // Point/spot light
    float        v3Pos[3]     = {0.f, 0.f, 0.f};
    float        fAttenuation0 = 0.f;  // constant
    float        fAttenuation1 = 0.05f; // linear
    float        fAttenuation2 = 0.f;  // quadratic
    float        fRange        = 200.f;
};

// Full light constant buffer (expanded from 96B to 312B to hold up to 8 dynamic lights).
// The first 96 bytes (ambient/diffuse/lightDir/cameraPos/fogParams/fogColor) are
// compatible with the current vsLit/psLit shaders. Extended slots are reserved
// for a future multi-light pixel shader.
struct LightCB {
    // Base directional light (96 bytes / 24 floats) — matches cbLight in mesh_shaders.
    float ambient[4];     // {r,g,b,a}
    float diffuse[4];    // {r,g,b,a}
    float lightDir[4];   // {x,y,z,w}
    float cameraPos[4];
    float fogParams[4]; // {enabled, start, end, density}
    float fogColor[4];

    // Extended: dynamic light slots 0-7 (3 vec4s each: pos+enabled, color+range, attenuation+pad)
    // Written when pDynList is non-null in RenderMeshObject.
    float dynLightPos0[4];    float dynLightColor0[4];    float dynLightAtten0[4];
    float dynLightPos1[4];    float dynLightColor1[4];    float dynLightAtten1[4];
    float dynLightPos2[4];    float dynLightColor2[4];    float dynLightAtten2[4];
    float dynLightPos3[4];    float dynLightColor3[4];    float dynLightAtten3[4];
    float dynLightPos4[4];    float dynLightColor4[4];    float dynLightAtten4[4];
    float dynLightPos5[4];    float dynLightColor5[4];    float dynLightAtten5[4];
    float dynLightPos6[4];    float dynLightColor6[4];    float dynLightAtten6[4];
    float dynLightPos7[4];    float dynLightColor7[4];    float dynLightAtten7[4];
};

// Convert 0xAABBGGRR engine color to float4 {r,g,b,a}.
inline void color_to_float4(std::uint32_t c, float out4[4]) {
    out4[0] = static_cast<float>((c >> 16) & 0xff) / 255.f; // R
    out4[1] = static_cast<float>((c >>  8) & 0xff) / 255.f; // G
    out4[2] = static_cast<float>((c >>  0) & 0xff) / 255.f; // B
    out4[3] = static_cast<float>((c >> 24) & 0xff) / 255.f; // A
}

// Zero-init an entire LightCB.
inline void init_light_cb(LightCB& cb) {
    std::memset(&cb, 0, sizeof(LightCB));
    cb.ambient[0] = cb.ambient[1] = cb.ambient[2] = 0.25f; cb.ambient[3] = 1.f;
    cb.diffuse[0] = cb.diffuse[1] = cb.diffuse[2] = 0.95f;  cb.diffuse[3] = 1.f;
    cb.lightDir[0] =  0.3f; cb.lightDir[1] = -0.7f; cb.lightDir[2] =  0.4f; cb.lightDir[3] = 0.f;
    cb.fogParams[0] = -1.f; // disabled by default (enabled=-1 in fog register)
}

} // namespace mxh::gx::dx11
