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

// Effect pixel shader: dot3 lighting + effect texture from slot t2.
// Mirrors original PLMeshObject::RenderEffect (diffDot3Reflect.pso).
// The effect texture (sphere map or wave distortion) modulates the lit color.
static const char* kPS_Effect = R"(
struct PSInput {
    float4 pos    : SV_Position;
    float2 uv     : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldP : TEXCOORD2;
};
cbuffer CBLight : register(b0) {
    float4 ambient;
    float4 diffuse;
    float4 lightDir;
    float4 cameraPos;
    float4 fogParams;
    float4 fogColor;
};
Texture2D    tex      : register(t0);  // base diffuse
Texture2D    effectTex : register(t2); // effect / environment map
SamplerState samp     : register(s0);
float4 main(PSInput i) : SV_Target {
    float3 n = normalize(i.normal);
    float nDotL = saturate(dot(n, -lightDir.xyz));
    float4 base = tex.Sample(samp, i.uv);
    float4 env  = effectTex.Sample(samp, i.uv);
    float3 lit = base.rgb * (ambient.rgb + diffuse.rgb * nDotL);
    // Modulate by effect texture (sphere-map reflection or wave distortion).
    lit *= env.rgb;
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

    // Effect pixel shader (dot3 + effect texture on t2).
    Microsoft::WRL::ComPtr<ID3DBlob> psEffectBlob;
    hr = D3DCompile(kPS_Effect, strlen(kPS_Effect), nullptr, nullptr, nullptr,
                    "main", "ps_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psEffectBlob, &err);
    if (FAILED(hr)) {
        MLOG_ERROR("[mesh-shader] PS_Effect compile failed: %s",
                   err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return false;
    }
    if (FAILED(device->CreatePixelShader(psEffectBlob->GetBufferPointer(),
                                          psEffectBlob->GetBufferSize(),
                                          nullptr, &psEffect)))
        return false;

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

    // 3D solid-color shaders (debug / RenderTri* path). Input = pos3 + packed RGBA.
    static const char* kVS_3DSolid = R"(
struct VSInput {
    float3 pos : POSITION;
    float4 col : COLOR0;
};
struct VSOutput {
    float4 pos : SV_Position;
    float4 col : COLOR0;
};
cbuffer CBViewProj : register(b0) {
    float4x4 viewProj;
};
VSOutput main(VSInput i) {
    VSOutput o;
    o.pos = mul(float4(i.pos, 1.0), viewProj);
    o.col = i.col;
    return o;
}
)";
    static const char* kPS_3DSolid = R"(
struct PSInput {
    float4 pos : SV_Position;
    float4 col : COLOR0;
};
float4 main(PSInput i) : SV_Target {
    return i.col;
}
)";
    Microsoft::WRL::ComPtr<ID3DBlob> vs3DBlob;
    hr = D3DCompile(kVS_3DSolid, strlen(kVS_3DSolid), nullptr, nullptr, nullptr,
                    "main", "vs_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs3DBlob, &err);
    if (FAILED(hr)) {
        MLOG_ERROR("[mesh-shader] VS_3DSolid compile failed: %s",
                   err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return false;
    }
    Microsoft::WRL::ComPtr<ID3DBlob> ps3DBlob;
    hr = D3DCompile(kPS_3DSolid, strlen(kPS_3DSolid), nullptr, nullptr, nullptr,
                    "main", "ps_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps3DBlob, &err);
    if (FAILED(hr)) {
        MLOG_ERROR("[mesh-shader] PS_3DSolid compile failed: %s",
                   err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return false;
    }
    if (FAILED(device->CreateVertexShader(vs3DBlob->GetBufferPointer(), vs3DBlob->GetBufferSize(),
                                          nullptr, &vs3DSolid)))
        return false;
    if (FAILED(device->CreatePixelShader(ps3DBlob->GetBufferPointer(), ps3DBlob->GetBufferSize(),
                                         nullptr, &ps3DSolid)))
        return false;

    D3D11_INPUT_ELEMENT_DESC layout3D[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (FAILED(device->CreateInputLayout(layout3D, 2, vs3DBlob->GetBufferPointer(),
                                          vs3DBlob->GetBufferSize(), &il3DSolid)))
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
    vsLit.Reset(); psLit.Reset(); psEffect.Reset(); ilLit.Reset();
    vs3DSolid.Reset(); ps3DSolid.Reset(); il3DSolid.Reset();
    cbWorld.Reset(); cbViewProj.Reset(); cbLight.Reset();
}

} // namespace mxh::gx::dx11