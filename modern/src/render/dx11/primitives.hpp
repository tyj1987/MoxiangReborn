// mxh/render/dx11/sprite.hpp
// 2D textured-quad primitives and debug-draw helpers (RenderBox/Line/Point/Circle).
#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

class Device;

// One-time GPU resource init shared by all primitives.
struct PrimitiveShaders {
    // 2D solid pipeline: float2 pos + RGBA. Used by drawLine /
    // drawPoint / drawCircle / drawGrid (host-supplied screen-space
    // coordinates).
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vsSolid;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  psSolid;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  ilSolid;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       vbSolid;     // dynamic vertex buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbViewProj;  // view-proj constant buffer

    // 3D solid pipeline: float3 pos + RGBA. Used by drawBox (the
    // 8 corners of a real 3D box — X/Y/Z all meaningful). The 3D
    // VS multiplies by the same viewProj constant buffer; the only
    // difference is the input layout (3 floats per position vs 2)
    // and the vertex struct size. R-9.x.
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vsSolid3D;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  ilSolid3D;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vsTextured;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  psTextured;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  ilTextured;

    bool init(ID3D11Device* device);
    void release();
};

// Color primitive helpers. These are called by CoD3DDeviceDX11 for
// RenderBox / RenderLine / RenderPoint / RenderCircle (debug visualization).
class PrimitiveDrawer {
public:
    PrimitiveDrawer() = default;

    bool initialize(Device* dev);
    void shutdown();

    void setViewProj(const MATRIX4& vp);

    // Render a wireframe axis-aligned box (8 corners → 12 edges) in
    // 3D world space. Each VECTOR3's x, y, z are real 3D coordinates;
    // the GPU multiplies by m_viewProj to project to screen space.
    // R-9.x: previously this method degraded to 2D by using only
    // oct[i].x and oct[i].z (ignoring y). The 3D upgrade restores
    // full 3D meaning — a box at (0,1,0)..(1,2,1) is now drawn as
    // a 1×1×1 cube centered at (0.5, 1.5, 0.5), not flattened to
    // the XZ plane.
    void drawBox(const VECTOR3* oct, std::uint32_t color);

    // Render a 2D line in screen space (orthographic). color = ARGB.
    void drawLine(const VECTOR2& a, const VECTOR2& b, std::uint32_t color);

    // Render a 2D point.
    void drawPoint(const VECTOR2& p, std::uint32_t color);

    // Render a 2D filled circle.
    void drawCircle(const VECTOR2& center, float radius, std::uint32_t color);

    // Render a wireframe quad in world space (4 corners connected edge to edge).
    // Top-down projection: X=screen X, Z=screen Y (same as drawBox).
    void drawGrid(const VECTOR3* quad, std::uint32_t color);

    // Render a screen-space textured quad (used by SpriteObject).
    void drawTexturedQuad(ID3D11ShaderResourceView* srv,
                          float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          std::uint32_t color, float rotation, int zOrder);

private:
    Device*                                   m_dev = nullptr;
    PrimitiveShaders                          m_shaders;
    Microsoft::WRL::ComPtr<ID3D11Buffer>      m_cbViewProj;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rsCullNone;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_dsNoDepth;

    // helper: pack color (ARGB) into float4
    static void unpackColor(std::uint32_t argb, float out[4]);
    void updateViewProj();
    MATRIX4 m_viewProj{};
};

} // namespace mxh::gx::dx11
