// mxh/render/dx11/primitives.cpp
// DX11 primitive shaders and draw helpers.
#include "primitives.hpp"
#include "device.hpp"

#include <d3dcompiler.h>
#include <vector>

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

// Shader sources: minimal solid + textured 2D pipeline. Position-only vs +
// solid color ps; textured uses vs + sampling ps.
static const char* kVS_Solid = R"(
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

static const char* kPS_Solid = R"(
struct PSInput {
    float4 pos : SV_Position;
    float4 col : COLOR0;
};
float4 main(PSInput i) : SV_Target {
    return i.col;
}
)";

static const char* kVS_Textured = R"(
struct VSInput {
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};
struct VSOutput {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};
cbuffer CBuf : register(b0) {
    float4x4 viewProj;
};
VSOutput main(VSInput i) {
    VSOutput o;
    o.pos = mul(float4(i.pos, 0.0, 1.0), viewProj);
    o.uv = i.uv;
    o.col = i.col;
    return o;
}
)";

static const char* kPS_Textured = R"(
struct PSInput {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};
Texture2D    tex : register(t0);
SamplerState samp : register(s0);
float4 main(PSInput i) : SV_Target {
    float4 sampled = tex.Sample(samp, i.uv);
    return sampled * i.col;
}
)";

bool PrimitiveShaders::init(ID3D11Device* device) {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob;

    auto compile = [](const char* src, const char* entry, const char* target, ID3DBlob** out) -> bool {
        Microsoft::WRL::ComPtr<ID3DBlob> err;
        HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                                 entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, out, &err);
        if (FAILED(hr)) {
            MLOG_ERROR("[dx11] shader compile failed (%s): %s", entry,
                       err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
            return false;
        }
        return true;
    };

    if (!compile(kVS_Solid, "main", "vs_4_0", &vsBlob)) return false;
    if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vsSolid)))
        return false;

    {
        Microsoft::WRL::ComPtr<ID3DBlob> ps;
        if (!compile(kPS_Solid, "main", "ps_4_0", &ps)) return false;
        if (FAILED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &psSolid)))
            return false;
    }

    // Input layout: pos(2f) + col(4f) interleaved.
    D3D11_INPUT_ELEMENT_DESC layoutSolid[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (FAILED(device->CreateInputLayout(layoutSolid, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &ilSolid)))
        return false;

    // Textured shaders.
    Microsoft::WRL::ComPtr<ID3DBlob> vsTBlob;
    if (!compile(kVS_Textured, "main", "vs_4_0", &vsTBlob)) return false;
    if (FAILED(device->CreateVertexShader(vsTBlob->GetBufferPointer(), vsTBlob->GetBufferSize(), nullptr, &vsTextured)))
        return false;

    {
        Microsoft::WRL::ComPtr<ID3DBlob> ps;
        if (!compile(kPS_Textured, "main", "ps_4_0", &ps)) return false;
        if (FAILED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &psTextured)))
            return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutTex[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (FAILED(device->CreateInputLayout(layoutTex, 3, vsTBlob->GetBufferPointer(), vsTBlob->GetBufferSize(), &ilTextured)))
        return false;

    // Dynamic vertex buffer (capacity 4096 verts).
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage          = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth      = sizeof(float) * 32 * 1024;  // 32k floats = enough for 1024 textured quads
    vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&vbd, nullptr, &vbSolid))) return false;

    // Constant buffer: 64-byte mat4.
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth      = 64;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &cbViewProj))) return false;

    return true;
}

void PrimitiveShaders::release() {
    vsSolid.Reset(); psSolid.Reset(); ilSolid.Reset();
    vsTextured.Reset(); psTextured.Reset(); ilTextured.Reset();
    vbSolid.Reset(); cbViewProj.Reset();
}

bool PrimitiveDrawer::initialize(Device* dev) {
    m_dev = dev;
    if (!m_shaders.init(dev->rawDevice())) return false;
    m_cbViewProj = m_shaders.cbViewProj;
    return true;
}

void PrimitiveDrawer::shutdown() {
    m_shaders.release();
    m_cbViewProj.Reset();
    m_dev = nullptr;
}

void PrimitiveDrawer::setViewProj(const MATRIX4& vp) {
    m_viewProj = vp;
}

void PrimitiveDrawer::updateViewProj() {
    if (!m_dev) return;
    auto* ctx = m_dev->rawContext();
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_cbViewProj.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        std::memcpy(mapped.pData, &m_viewProj, sizeof(MATRIX4));
        ctx->Unmap(m_cbViewProj.Get(), 0);
    }
}

void PrimitiveDrawer::unpackColor(std::uint32_t argb, float out[4]) {
    out[0] = ((argb >> 16) & 0xff) / 255.0f;
    out[1] = ((argb >>  8) & 0xff) / 255.0f;
    out[2] = ((argb      ) & 0xff) / 255.0f;
    out[3] = ((argb >> 24) & 0xff) / 255.0f;
}

void PrimitiveDrawer::drawBox(const VECTOR3* oct, std::uint32_t color) {
    if (!m_dev) return;
    auto* ctx = m_dev->rawContext();
    updateViewProj();

    // 12 edges 鈫?24 vertices.
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},  // bottom
        {4,5},{5,6},{6,7},{7,4},  // top
        {0,4},{1,5},{2,6},{3,7},  // verticals
    };

    struct V { float x, y; std::uint32_t c; };
    std::vector<V> verts(24);
    for (int i = 0; i < 12; ++i) {
        V v{};
        v.x = oct[edges[i][0]].x;  // simple: use x,y for now (proper ortho projection is TODO)
        v.y = oct[edges[i][0]].z;
        v.c = color;
        verts[i*2] = v;
        v.x = oct[edges[i][1]].x;
        v.y = oct[edges[i][1]].z;
        verts[i*2 + 1] = v;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(m_shaders.vbSolid.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    std::memcpy(mapped.pData, verts.data(), verts.size() * sizeof(V));
    ctx->Unmap(m_shaders.vbSolid.Get(), 0);

    UINT stride = sizeof(V), offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_shaders.vbSolid.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(m_shaders.ilSolid.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->VSSetShader(m_shaders.vsSolid.Get(), nullptr, 0);
    ctx->PSSetShader(m_shaders.psSolid.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_cbViewProj.GetAddressOf());
    ctx->Draw(24, 0);
}

void PrimitiveDrawer::drawLine(const VECTOR2& a, const VECTOR2& b, std::uint32_t color) {
    if (!m_dev) return;
    auto* ctx = m_dev->rawContext();
    updateViewProj();

    struct V { float x, y; std::uint32_t c; };
    V verts[2] = { { a.x, a.y, color }, { b.x, b.y, color } };

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(m_shaders.vbSolid.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    std::memcpy(mapped.pData, verts, sizeof(verts));
    ctx->Unmap(m_shaders.vbSolid.Get(), 0);

    UINT stride = sizeof(V), offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_shaders.vbSolid.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(m_shaders.ilSolid.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->VSSetShader(m_shaders.vsSolid.Get(), nullptr, 0);
    ctx->PSSetShader(m_shaders.psSolid.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_cbViewProj.GetAddressOf());
    ctx->Draw(2, 0);
}

void PrimitiveDrawer::drawPoint(const VECTOR2& p, std::uint32_t color) {
    drawLine({ p.x - 1.0f, p.y }, { p.x + 1.0f, p.y }, color);
    drawLine({ p.x, p.y - 1.0f }, { p.x, p.y + 1.0f }, color);
}

void PrimitiveDrawer::drawCircle(const VECTOR2& center, float radius, std::uint32_t color) {
    if (!m_dev) return;
    auto* ctx = m_dev->rawContext();
    updateViewProj();

    constexpr int N = DEFULAT_CIRCLE_PIECES_NUM;
    struct V { float x, y; std::uint32_t c; };
    V verts[N * 2];

    for (int i = 0; i < N; ++i) {
        float a1 = (PI_MUL_2 * i)     / N;
        float a2 = (PI_MUL_2 * (i+1)) / N;
        verts[i*2]     = { center.x + radius * std::cos(a1), center.y + radius * std::sin(a1), color };
        verts[i*2 + 1] = { center.x + radius * std::cos(a2), center.y + radius * std::sin(a2), color };
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(m_shaders.vbSolid.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    std::memcpy(mapped.pData, verts, sizeof(verts));
    ctx->Unmap(m_shaders.vbSolid.Get(), 0);

    UINT stride = sizeof(V), offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_shaders.vbSolid.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(m_shaders.ilSolid.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->VSSetShader(m_shaders.vsSolid.Get(), nullptr, 0);
    ctx->PSSetShader(m_shaders.psSolid.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_cbViewProj.GetAddressOf());
    ctx->Draw(N * 2, 0);
}

void PrimitiveDrawer::drawGrid(const VECTOR3* quad, std::uint32_t color) {
    if (!m_dev || !quad) return;
    auto* ctx = m_dev->rawContext();
    updateViewProj();

    // 4 corners 鈫?4 edges as 8 vertices (LINELIST).
    struct V { float x, y; std::uint32_t c; };
    V verts[8];
    auto push = [&](int idx, const VECTOR3& p) {
        verts[idx] = { p.x, p.z, color };
    };
    push(0, quad[0]); push(1, quad[1]);
    push(2, quad[1]); push(3, quad[2]);
    push(4, quad[2]); push(5, quad[3]);
    push(6, quad[3]); push(7, quad[0]);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(m_shaders.vbSolid.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    std::memcpy(mapped.pData, verts, sizeof(verts));
    ctx->Unmap(m_shaders.vbSolid.Get(), 0);

    UINT stride = sizeof(V), offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_shaders.vbSolid.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(m_shaders.ilSolid.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->VSSetShader(m_shaders.vsSolid.Get(), nullptr, 0);
    ctx->PSSetShader(m_shaders.psSolid.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_cbViewProj.GetAddressOf());
    ctx->Draw(8, 0);
}

void PrimitiveDrawer::drawTexturedQuad(ID3D11ShaderResourceView* srv,
                                       float x, float y, float w, float h,
                                       float u0, float v0, float u1, float v1,
                                       std::uint32_t color, float rotation, int /*zOrder*/) {
    if (!m_dev || !srv) return;
    auto* ctx = m_dev->rawContext();
    updateViewProj();

    // Build quad vertices with optional rotation around center.
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float cosA = std::cos(rotation);
    float sinA = std::sin(rotation);

    auto rot = [&](float px, float py) -> std::pair<float, float> {
        float dx = px - cx;
        float dy = py - cy;
        return { cx + dx * cosA - dy * sinA, cy + dx * sinA + dy * cosA };
    };

    auto [x0, y0] = rot(x,       y);
    auto [x1, y1] = rot(x + w,   y);
    auto [x2, y2] = rot(x + w,   y + h);
    auto [x3, y3] = rot(x,       y + h);

    struct V { float x, y, u, v; std::uint32_t c; };
    V verts[6] = {
        { x0, y0, u0, v0, color },
        { x1, y1, u1, v0, color },
        { x2, y2, u1, v1, color },
        { x0, y0, u0, v0, color },
        { x2, y2, u1, v1, color },
        { x3, y3, u0, v1, color },
    };

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(m_shaders.vbSolid.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    std::memcpy(mapped.pData, verts, sizeof(verts));
    ctx->Unmap(m_shaders.vbSolid.Get(), 0);

    UINT stride = sizeof(V), offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_shaders.vbSolid.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(m_shaders.ilTextured.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(m_shaders.vsTextured.Get(), nullptr, 0);
    ctx->PSSetShader(m_shaders.psTextured.Get(), nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &srv);
    ctx->VSSetConstantBuffers(0, 1, m_cbViewProj.GetAddressOf());
    ctx->Draw(6, 0);

    // Unbind SRV to avoid D3D11 warnings.
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);
}

} // namespace mxh::gx::dx11
