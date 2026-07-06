// mxh/render/dx11/device.hpp
// Internal DX11 Device wrapper used by CoD3DDeviceDX11.
#pragma once

#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <cstdint>
#include <string>

#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

class EffectShaderPalette;

// Forward-declare MeshShaders to expose the effect pixel shader.
struct MeshShaders;

class Device {
public:
    Device() = default;
    ~Device();

    // Initialize DX11 device + swap chain. Returns false if the GPU/Driver
    // rejects the requested format (e.g. user requests 32-bit color on a 16-bit
    // desktop). Caller can retry with a different DISPLAY_INFO.
    bool initialize(HWND hWnd, const DISPLAY_INFO& info);

    void shutdown();

    // Begin/End render frame. Clear color/depth.
    void beginFrame(const SHORT_RECT* pRect, std::uint32_t dwClearColor, std::uint32_t dwFlag);
    void endFrame();

    void present(HWND hWnd);

    // Viewport + matrices for 3D rendering.
    void setViewFrustum(const VIEW_VOLUME& vv, const CAMERA_DESC& cam,
                        const MATRIX4& matView, const MATRIX4& matProj, const MATRIX4& matBillboard);

    // Client window size accessor.
    std::uint16_t width()  const { return m_width; }
    std::uint16_t height() const { return m_height; }

    // Public DX11 access (for advanced callers needing raw ID3D11Device).
    ID3D11Device*           rawDevice()  const { return m_device.Get(); }
    ID3D11DeviceContext*    rawContext() const { return m_context.Get(); }
    IDXGISwapChain*         rawSwapChain() const { return m_swapChain.Get(); }
    ID3D11RenderTargetView* backBufferRTV() const { return m_backBufferRTV.Get(); }
    ID3D11DepthStencilView* depthStencilView() const { return m_dsv.Get(); }

    // State exposed to other modules (renderer/sprite/font/texture).
    const MATRIX4& viewMatrix()        const { return m_matView; }
    const MATRIX4& projMatrix()        const { return m_matProj; }
    const MATRIX4& viewProjMatrix()    const { return m_matViewProj; }
    const MATRIX4& billboardMatrix()   const { return m_matBillboard; }

    std::uint32_t ambientColor()       const { return m_ambientColor; }
    std::uint32_t emissiveColor()      const { return m_emissiveColor; }

    const VECTOR3& cameraPosition()   const { return m_cameraPos; }

    // Effect shader support.
    void setEffectPalette(EffectShaderPalette* palette);
    EffectShaderPalette* effectPalette() const { return m_effectPalette; }

    // Tick count for wave animation.
    void setTickCount(std::uint32_t tick);
    std::uint32_t tickCount() const { return m_tickCount; }

    // Texture creation (for effect shader palette).
    ID3D11ShaderResourceView* createTextureFromFile(const char* fileName);

    // File storage accessor.
    I4DyuchiFileStorage* fileStorage() const { return m_storage; }
    void setStorage(I4DyuchiFileStorage* s) { m_storage = s; }

private:
    bool createSwapChain(HWND hWnd, const DISPLAY_INFO& info);
    bool createRenderTargets();
    void releaseRenderTargets();

    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_backBufferRTV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_depthBuffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStateDefault;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterizerDefault;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        m_blendDefault;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>      m_samplerPoint;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>      m_samplerLinear;

    DISPLAY_TYPE m_displayType = WINDOW_WITH_BLT;
    std::uint16_t m_width  = 0;
    std::uint16_t m_height = 0;

    MATRIX4 m_matView{};
    MATRIX4 m_matProj{};
    MATRIX4 m_matViewProj{};
    MATRIX4 m_matBillboard{};

    std::uint32_t                          m_ambientColor  = DEFAULT_AMBIENT_COLOR;
    std::uint32_t                          m_emissiveColor = 0xff000000;
    VECTOR3                               m_cameraPos{};
    std::uint32_t                          m_tickCount = 0;

    EffectShaderPalette*                  m_effectPalette = nullptr;

    interface I4DyuchiFileStorage*          m_storage = nullptr;
};

} // namespace mxh::gx::dx11
