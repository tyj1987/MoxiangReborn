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
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vsSolid;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  psSolid;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  ilSolid;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       vbSolid;     // dynamic vertex buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbViewProj;  // view-proj constant buffer

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

    // Render a wireframe axis-aligned box (8 corners 鈫?12 edges).
    void drawBox(const VECTOR3* oct, std::uint32_t color);

    // Render a 2D line in screen space (orthographic). color = ARGB.
    void drawLine(const VECTOR2& a, const VECTOR2& b, std::uint32_t color);

    // Render a 2D point.
    void drawPoint(const VECTOR2& p, std::uint32_t color);

    // Render a 2D filled circle.
    void drawCircle(const VECTOR2& center, float radius, std::uint32_t color);

    // Render a screen-space textured quad (used by SpriteObject).
    void drawTexturedQuad(ID3D11ShaderResourceView* srv,
                          float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          std::uint32_t color, float rotation, int zOrder);

private:
    Device*                                   m_dev = nullptr;
    PrimitiveShaders                          m_shaders;
    Microsoft::WRL::ComPtr<ID3D11Buffer>      m_cbViewProj;

    // helper: pack color (ARGB) into float4
    static void unpackColor(std::uint32_t argb, float out[4]);
    void updateViewProj();
    MATRIX4 m_viewProj{};
};

} // namespace mxh::gx::dx11
