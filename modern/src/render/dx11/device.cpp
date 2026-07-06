// mxh/render/dx11/device.cpp
// DX11 Device / SwapChain / RenderTarget setup.
#include "device.hpp"

#include <d3dcompiler.h>

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

Device::~Device() { shutdown(); }

bool Device::initialize(HWND hWnd, const DISPLAY_INFO& info) {
    if (!createSwapChain(hWnd, info)) {
        return false;
    }
    if (!createRenderTargets()) {
        return false;
    }

    // Default depth-stencil state: write depth, test less.
    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable    = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc      = D3D11_COMPARISON_LESS;
    dsd.StencilEnable  = FALSE;
    if (FAILED(m_device->CreateDepthStencilState(&dsd, &m_depthStateDefault))) {
        MLOG_ERROR("[dx11] CreateDepthStencilState failed");
    }
    m_context->OMSetDepthStencilState(m_depthStateDefault.Get(), 0);

    // Default rasterizer: back-face culling, fill solid, no scissor.
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_BACK;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = TRUE;
    if (FAILED(m_device->CreateRasterizerState(&rd, &m_rasterizerDefault))) {
        MLOG_ERROR("[dx11] CreateRasterizerState failed");
    }
    m_context->RSSetState(m_rasterizerDefault.Get());

    // Default blend: alpha blend.
    D3D11_BLEND_DESC bd{};
    bd.AlphaToCoverageEnable = FALSE;
    bd.IndependentBlendEnable = FALSE;
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp   = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(m_device->CreateBlendState(&bd, &m_blendDefault))) {
        MLOG_ERROR("[dx11] CreateBlendState failed");
    }
    m_context->OMSetBlendState(m_blendDefault.Get(), nullptr, 0xffffffff);

    // Sampler states.
    D3D11_SAMPLER_DESC sd{};
    sd.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    if (FAILED(m_device->CreateSamplerState(&sd, &m_samplerPoint))) {
        MLOG_ERROR("[dx11] CreateSamplerState(point) failed");
    }
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    if (FAILED(m_device->CreateSamplerState(&sd, &m_samplerLinear))) {
        MLOG_ERROR("[dx11] CreateSamplerState(linear) failed");
    }

    MLOG_INFO("[dx11] Device initialized %ux%u (bps=%u, refresh=%u)",
              info.dwWidth, info.dwHeight, info.dwBPS, info.dwRefreshRate);
    return true;
}

void Device::shutdown() {
    releaseRenderTargets();
    if (m_swapChain) {
        m_swapChain->SetFullscreenState(FALSE, nullptr);
    }
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
    m_depthStateDefault.Reset();
    m_rasterizerDefault.Reset();
    m_blendDefault.Reset();
    m_samplerPoint.Reset();
    m_samplerLinear.Reset();
}

bool Device::createSwapChain(HWND hWnd, const DISPLAY_INFO& info) {
    m_displayType = info.dispType;
    m_width  = static_cast<std::uint16_t>(info.dwWidth);
    m_height = static_cast<std::uint16_t>(info.dwHeight);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = info.dwWidth;
    sd.BufferDesc.Height                  = info.dwHeight;
    sd.BufferDesc.RefreshRate.Numerator   = info.dwRefreshRate > 0 ? info.dwRefreshRate : 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = (info.dispType == WINDOW_WITH_BLT);
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL createdFeature = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                       // default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,                             // no debug flags (we can enable D3D11_CREATE_DEVICE_DEBUG if needed)
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &sd,
        &m_swapChain,
        &m_device,
        &createdFeature,
        &m_context);
    if (FAILED(hr)) {
        MLOG_ERROR("[dx11] D3D11CreateDeviceAndSwapChain failed: 0x%08x", hr);
        return false;
    }
    MLOG_INFO("[dx11] Created feature level 0x%x", static_cast<unsigned>(createdFeature));
    return true;
}

bool Device::createRenderTargets() {
    releaseRenderTargets();

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer))) {
        MLOG_ERROR("[dx11] swapchain GetBuffer failed");
        return false;
    }
    if (FAILED(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_backBufferRTV))) {
        MLOG_ERROR("[dx11] CreateRenderTargetView failed");
        return false;
    }

    // Depth buffer matching back buffer size.
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width     = m_width;
    depthDesc.Height    = m_height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage          = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    if (FAILED(m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthBuffer))) {
        MLOG_ERROR("[dx11] CreateTexture2D(depth) failed");
        return false;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
    dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsvd, &m_dsv))) {
        MLOG_ERROR("[dx11] CreateDepthStencilView failed");
        return false;
    }

    m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_dsv.Get());

    // Default viewport = full client area.
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width    = static_cast<float>(m_width);
    vp.Height   = static_cast<float>(m_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);
    return true;
}

void Device::releaseRenderTargets() {
    if (m_context) m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_dsv.Reset();
    m_depthBuffer.Reset();
    m_backBufferRTV.Reset();
}

void Device::beginFrame(const SHORT_RECT* pRect, std::uint32_t dwClearColor, std::uint32_t /*dwFlag*/) {
    // Reset to back buffer (in case a render target was bound).
    m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_dsv.Get());

    // Sub-rect (currently we just clip via viewport; full clear if no rect).
    D3D11_VIEWPORT vp{};
    if (pRect) {
        vp.TopLeftX = static_cast<float>(pRect->left);
        vp.TopLeftY = static_cast<float>(pRect->top);
        vp.Width    = static_cast<float>(pRect->right - pRect->left);
        vp.Height   = static_cast<float>(pRect->bottom - pRect->top);
    } else {
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        vp.Width    = static_cast<float>(m_width);
        vp.Height   = static_cast<float>(m_height);
    }
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    // DXGI A8R8G8B8 -> float4 [0,1].
    float r = ((dwClearColor >> 16) & 0xff) / 255.0f;
    float g = ((dwClearColor >>  8) & 0xff) / 255.0f;
    float b = ((dwClearColor      ) & 0xff) / 255.0f;
    float a = ((dwClearColor >> 24) & 0xff) / 255.0f;
    const FLOAT clearColor[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_backBufferRTV.Get(), clearColor);
    m_context->ClearDepthStencilView(m_dsv.Get(),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Device::endFrame() {
    // No-op for DX11 (D3DX had Scene/Begin/End).
}

void Device::present(HWND /*hWnd*/) {
    if (m_swapChain) {
        // VSync: 1 = wait for vsync, 0 = immediate.
        m_swapChain->Present(1, 0);
    }
}

void Device::setViewFrustum(const VIEW_VOLUME& /*vv*/, const CAMERA_DESC& cam,
                            const MATRIX4& matView, const MATRIX4& matProj, const MATRIX4& matBillboard) {
    m_matView      = matView;
    m_matProj      = matProj;
    m_matBillboard = matBillboard;

    // Compose view*proj for convenience (column-major XMMATRIX multiplication
    // would be done here in a real engine; for now we store the parts separately
    // so callers can decide which form to use).
    // Pre-multiply column-major matrices:
    MATRIX4 out{};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += matView.m[i][k] * matProj.m[k][j];
            }
            out.m[i][j] = sum;
        }
    }
    m_matViewProj = out;
}

} // namespace mxh::gx::dx11
