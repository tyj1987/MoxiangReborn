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

// Multi-light pixel shader (Phase 5 deferred / Phase 5.10): accumulates diffuse
// contribution from up to 8 dynamic / real-time lights. Compatible with the same
// register b0 cbuffer used by psLit (LightCB), so the existing 96-byte base
// region + 8 light slots (3 vec4s each) are read. Each slot's pos.w is the
// "enabled" flag (1.0 = active, 0.0 = inactive). Color.rgb is the light color,
// color.a is the range. Attenuation.x/y/z are the standard D3D attenuation
// coefficients. Point-light distance attenuation is applied; for a directional
// light (pos.w == 2.0 — used as a sentinel for "treat pos.xyz as direction"),
// attenuation is skipped.
static const char* kPS_MultiLight = R"(
struct PSInput {
    float4 pos    : SV_Position;
    float2 uv     : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldP : TEXCOORD2;
};
cbuffer CBLight : register(b0) {
    float4 ambient;
    float4 diffuse;
    float4 lightDir;   // base directional, normalized surface→light
    float4 cameraPos;
    float4 fogParams;  // x=enabled(0/1), y=start, z=end, w=density
    float4 fogColor;
    // 8 dynamic / RT light slots (3 vec4s each: pos+enabled-flag, color+range, atten).
    float4 dyn0Pos;  float4 dyn0Color;  float4 dyn0Atten;
    float4 dyn1Pos;  float4 dyn1Color;  float4 dyn1Atten;
    float4 dyn2Pos;  float4 dyn2Color;  float4 dyn2Atten;
    float4 dyn3Pos;  float4 dyn3Color;  float4 dyn3Atten;
    float4 dyn4Pos;  float4 dyn4Color;  float4 dyn4Atten;
    float4 dyn5Pos;  float4 dyn5Color;  float4 dyn5Atten;
    float4 dyn6Pos;  float4 dyn6Color;  float4 dyn6Atten;
    float4 dyn7Pos;  float4 dyn7Color;  float4 dyn7Atten;
};
Texture2D    tex : register(t0);
SamplerState samp : register(s0);

static const float4 dynPos[8]   = { dyn0Pos, dyn1Pos, dyn2Pos, dyn3Pos, dyn4Pos, dyn5Pos, dyn6Pos, dyn7Pos };
static const float4 dynColor[8] = { dyn0Color, dyn1Color, dyn2Color, dyn3Color, dyn4Color, dyn5Color, dyn6Color, dyn7Color };
static const float4 dynAtten[8] = { dyn0Atten, dyn1Atten, dyn2Atten, dyn3Atten, dyn4Atten, dyn5Atten, dyn6Atten, dyn7Atten };

float3 accumulateDynamic(float3 n, float3 worldP) {
    float3 sum = 0;
    [unroll]
    for (int i = 0; i < 8; ++i) {
        float4 p = dynPos[i];
        if (p.w <= 0.5) continue;             // disabled slot
        float4 c = dynColor[i];
        float4 a = dynAtten[i];
        float3 toLight;
        float dist;
        if (p.w > 1.5) {
            // directional sentinel (pos.xyz is normalized surface→light dir)
            toLight = p.xyz;
            dist = 1.0;
        } else {
            toLight = p.xyz - worldP;
            dist = length(toLight);
            if (dist > 0.0001) toLight /= dist;
        }
        float nDotL = saturate(dot(n, toLight));
        float att = 1.0 / max(a.x + a.y * dist + a.z * dist * dist, 0.0001);
        // Range falloff: 0 at far edge, 1 at near.
        float range = c.a;
        if (range > 0.0) att *= saturate(1.0 - dist / range);
        sum += c.rgb * nDotL * att;
    }
    return sum;
}

float4 main(PSInput i) : SV_Target {
    float3 n = normalize(i.normal);
    float nDotL = saturate(dot(n, -lightDir.xyz));
    float4 base = tex.Sample(samp, i.uv);
    // Base directional + accumulated dynamic lights.
    float3 dynamic = accumulateDynamic(n, i.worldP);
    float3 lit = base.rgb * (ambient.rgb + diffuse.rgb * nDotL + dynamic);
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

    // Multi-light pixel shader (Phase 5 deferred). Up to 8 dynamic / RT lights.
    Microsoft::WRL::ComPtr<ID3DBlob> psMultiBlob;
    hr = D3DCompile(kPS_MultiLight, strlen(kPS_MultiLight), nullptr, nullptr, nullptr,
                    "main", "ps_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psMultiBlob, &err);
    if (FAILED(hr)) {
        MLOG_ERROR("[mesh-shader] PS_MultiLight compile failed: %s",
                   err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return false;
    }
    if (FAILED(device->CreatePixelShader(psMultiBlob->GetBufferPointer(),
                                          psMultiBlob->GetBufferSize(),
                                          nullptr, &psMultiLight)))
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
    // LightCB is 480 bytes (96 base + 8 light slots × 48 bytes). psLit only reads
    // the first 96 bytes; psMultiLight reads all 480. Both share register b0.
    if (!makeCB(480, &cbLight))    return false;
    return true;
}

void MeshShaders::release() {
    vsLit.Reset(); psLit.Reset(); psMultiLight.Reset(); psEffect.Reset(); ilLit.Reset();
    vs3DSolid.Reset(); ps3DSolid.Reset(); il3DSolid.Reset();
    cbWorld.Reset(); cbViewProj.Reset(); cbLight.Reset();
}

} // namespace mxh::gx::dx11