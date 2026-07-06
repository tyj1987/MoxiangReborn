// mxh/render/dx11/mesh_shaders.cpp
// HLSL shaders for IDIMeshObject rendering.
#include "mesh_shaders.hpp"

#include <d3dcompiler.h>

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

static const char* kVS_Lit = R"(
struct VSInput {
    float3 pos    : POSITION;
    float2 uv     : TEXCOORD0;
    float3 normal : NORMAL;
};
struct VSOutput {
    float4 pos    : SV_Position;
    float2 uv     : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldP : TEXCOORD2;
};
cbuffer CBWorld : register(b0) {
    float4x4 world;
};
cbuffer CBViewProj : register(b1) {
    float4x4 viewProj;
};
VSOutput main(VSInput i) {
    VSOutput o;
    float4 wp = mul(float4(i.pos, 1.0), world);
    o.worldP = wp.xyz;
    o.pos = mul(wp, viewProj);
    o.uv = i.uv;
    o.normal = mul(i.normal, (float3x3)world);
    return o;
}
)";

static const char* kPS_Lit = R"(
struct PSInput {
    float4 pos    : SV_Position;
    float2 uv     : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldP : TEXCOORD2;
};
cbuffer CBLight : register(b0) {
    float4 ambient;
    float4 diffuse;
    float4 lightDir;   // already normalized, points from surface to light
    float4 cameraPos;  // xyz = camera world position
    float4 fogParams;  // x = enabled(0/1), y = start, z = end, w = density
    float4 fogColor;   // rgb = fog color
};
Texture2D    tex : register(t0);
SamplerState samp : register(s0);
float4 main(PSInput i) : SV_Target {
    float3 n = normalize(i.normal);
    float nDotL = saturate(dot(n, -lightDir.xyz));
    float4 base = tex.Sample(samp, i.uv);
    float3 lit = base.rgb * (ambient.rgb + diffuse.rgb * nDotL);
    if (fogParams.x > 0.5) {
        float dist = length(i.worldP - cameraPos.xyz);
        float fog  = saturate((dist - fogParams.y) / max(fogParams.z - fogParams.y, 0.0001));
        lit = lerp(lit, fogColor.rgb, fog);
    }
    return float4(lit, base.a);
}
)";

bool MeshShaders::init(ID3D11Device* device) {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, err;

    HRESULT hr = D3DCompile(kVS_Lit, strlen(kVS_Lit), nullptr, nullptr, nullptr,
                            "main", "vs_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsBlob, &err);
    if (FAILED(hr)) {
        MLOG_ERROR("[mesh-shader] VS compile failed: %s",
                   err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return false;
    }
    hr = D3DCompile(kPS_Lit, strlen(kPS_Lit), nullptr, nullptr, nullptr,
                    "main", "ps_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psBlob, &err);
    if (FAILED(hr)) {
        MLOG_ERROR("[mesh-shader] PS compile failed: %s",
                   err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return false;
    }

    if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vsLit)))
        return false;
    if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &psLit)))
        return false;

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (FAILED(device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &ilLit)))
        return false;

    // Constant buffers.
    auto makeCB = [&](UINT byteWidth, ID3D11Buffer** out) {
        D3D11_BUFFER_DESC cbd{};
        cbd.Usage          = D3D11_USAGE_DYNAMIC;
        cbd.ByteWidth      = byteWidth;
        cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        return SUCCEEDED(device->CreateBuffer(&cbd, nullptr, out));
    };
    if (!makeCB(64,  &cbWorld))    return false;
    if (!makeCB(64,  &cbViewProj)) return false;
    if (!makeCB(96,  &cbLight))    return false;
    return true;
}

void MeshShaders::release() {
    vsLit.Reset(); psLit.Reset(); ilLit.Reset();
    cbWorld.Reset(); cbViewProj.Reset(); cbLight.Reset();
}

} // namespace mxh::gx::dx11