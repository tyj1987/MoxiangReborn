// primitives_shader_source.hpp - R-9.x shader source constants.
//
// Extracted from primitives.cpp so that the shader sources can be
// unit-tested (D3DCompile the source, verify the input signature
// contains the expected elements, verify the cbuffer binding). The
// shader sources are HLSL text; they need a D3D compiler to be
// fully validated, but a sanity test can confirm the source is
// syntactically well-formed and contains the expected structure.
//
// R-9.x: the 3D solid VS is the new addition. It is identical in
// structure to the 2D solid VS except for the input position
// (float3 instead of float2). The 2D VS is preserved unchanged
// for the 2D draw methods (drawLine / drawPoint / drawCircle /
// drawGrid / drawTexturedQuad).
#pragma once

namespace mxh::gx::dx11 {

// 2D solid VS source (legacy, used by drawLine / drawPoint /
// drawCircle / drawGrid — host supplies screen-space coordinates).
inline constexpr const char* kVS_Solid2D = R"(
struct VSInput {
    float2 pos : POSITION;
    float4 col : COLOR0;
};
struct VSOutput {
    float4 pos : SV_Position;
    float4 col : COLOR0;
};
cbuffer CBuf : register(b0) {
    float4x4 viewProj;
};
VSOutput main(VSInput i) {
    VSOutput o;
    o.pos = mul(float4(i.pos, 0.0, 1.0), viewProj);
    o.col = i.col;
    return o;
}
)";

// 3D solid VS source (R-9.x, used by drawBox — host supplies 3D
// world coordinates, GPU multiplies by viewProj). The only
// differences from kVS_Solid2D: float2 → float3, w component
// hardcoded to 1.0 instead of 0.0 (so the position is in world
// space, not the XY plane).
inline constexpr const char* kVS_Solid3D = R"(
struct VSInput {
    float3 pos : POSITION;
    float4 col : COLOR0;
};
struct VSOutput {
    float4 pos : SV_Position;
    float4 col : COLOR0;
};
cbuffer CBuf : register(b0) {
    float4x4 viewProj;
};
VSOutput main(VSInput i) {
    VSOutput o;
    o.pos = mul(float4(i.pos, 1.0), viewProj);
    o.col = i.col;
    return o;
}
)";

// Solid PS source (shared by both 2D and 3D VS — they have
// identical outputs).
inline constexpr const char* kPS_Solid = R"(
struct PSInput {
    float4 pos : SV_Position;
    float4 col : COLOR0;
};
float4 main(PSInput i) : SV_Target {
    return i.col;
}
)";

} // namespace mxh::gx::dx11
