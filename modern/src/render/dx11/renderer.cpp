// mxh/render/dx11/renderer.cpp
// Implementation of all 75 I4DyuchiGXRenderer methods.
#include "renderer.hpp"
#include "sprite.hpp"
#include "mesh_object.hpp"
#include "font_object.hpp"
#include "effect_shader.hpp"
#include "texture_loader.hpp"

#include <cstdio>
#include <cstring>

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

CoD3DDeviceDX11::CoD3DDeviceDX11() = default;

CoD3DDeviceDX11::~CoD3DDeviceDX11() {
    shutdownMaterials();
    m_primitives.shutdown();
    m_meshShaders.release();
    m_meshShadersReady = false;
    m_dev.reset();
}

STDMETHODIMP CoD3DDeviceDX11::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppv = static_cast<I4DyuchiGXRenderer*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CoD3DDeviceDX11::AddRef() { return ++m_refCount; }
STDMETHODIMP_(ULONG) CoD3DDeviceDX11::Release() {
    ULONG r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// ===== Initialization =====

BOOL __stdcall CoD3DDeviceDX11::Create(HWND hWnd, DISPLAY_INFO* pInfo, I4DyuchiFileStorage* pStorage,
                                       ErrorHandleProc pErr) {
    if (!hWnd || !pInfo) return FALSE;
    m_hwnd = hWnd;
    m_storage = pStorage;
    m_errorHandler = pErr;

    m_dev = std::make_unique<dx11::Device>();
    if (!m_dev->initialize(hWnd, *pInfo)) {
        m_dev.reset();
        return FALSE;
    }

    if (!m_primitives.initialize(m_dev.get())) {
        MLOG_ERROR("[renderer] PrimitiveDrawer init failed");
        m_dev.reset();
        return FALSE;
    }
    if (!m_meshShaders.init(m_dev->rawDevice())) {
        MLOG_ERROR("[renderer] MeshShaders init failed");
        m_primitives.shutdown();
        m_dev.reset();
        return FALSE;
    }
    m_meshShadersReady = true;
    MLOG_INFO("[renderer] CoD3DDeviceDX11 created (hwnd=%p storage=%p)", hWnd, pStorage);
    return TRUE;
}

// ===== Sprite / Mesh / Font / HeightField factories =====

IDISpriteObject* __stdcall CoD3DDeviceDX11::CreateSpriteObject(char* szFileName, std::uint32_t /*dwFlag*/) {
    if (!m_dev) return nullptr;
    return SpriteObject::createFromFile(m_dev.get(), m_storage, szFileName ? szFileName : "", 0);
}

IDISpriteObject* __stdcall CoD3DDeviceDX11::CreateSpriteObject(char* szFileName, std::uint32_t /*dwXPos*/,
                                                              std::uint32_t /*dwYPos*/, std::uint32_t dwWidth,
                                                              std::uint32_t dwHeight, std::uint32_t dwFlag) {
    if (!m_dev) return nullptr;
    auto* sprite = SpriteObject::createFromFile(m_dev.get(), m_storage, szFileName ? szFileName : "", dwFlag);
    if (sprite && dwWidth && dwHeight) sprite->Resize(static_cast<float>(dwWidth), static_cast<float>(dwHeight));
    return sprite;
}

IDISpriteObject* __stdcall CoD3DDeviceDX11::CreateEmptySpriteObject(std::uint32_t dwWidth, std::uint32_t dwHeight,
                                                                   TEXTURE_FORMAT fmt, std::uint32_t /*dwFlag*/) {
    if (!m_dev) return nullptr;
    return SpriteObject::create(m_dev.get(), dwWidth, dwHeight, fmt, nullptr);
}

IDIMeshObject* __stdcall CoD3DDeviceDX11::CreateMeshObject(CMeshFlag flag) {
    if (!m_dev) return nullptr;
    return MeshObject::createEmpty(m_dev.get(), flag);
}
IDIFontObject* __stdcall CoD3DDeviceDX11::CreateFontObject(LOGFONT* pLogFont, std::uint32_t dwFlag) {
    if (!m_dev) return nullptr;
    auto* f = FontObject::create(m_dev.get(), pLogFont, dwFlag);
    if (!f) {
        MLOG_WARN("[renderer] CreateFontObject returned null (LOGFONT or GDI init failed)");
    }
    return f;
}
IDIHeightField* __stdcall CoD3DDeviceDX11::CreateHeightField(std::uint32_t /*dwFlag*/) {
    // HeightField pipeline is non-trivial (LOD + alpha map + chunked VB); defer
    // to a later phase. Callers should null-check the return.
    if (!m_dev) return nullptr;
    return new HeightField(m_dev.get());
}
IDIMeshObject* __stdcall CoD3DDeviceDX11::CreateImmMeshObject(IVERTEX* piv3Tri, std::uint32_t dwTriCount,
                                                             void* /*pMtlHandle*/, std::uint32_t /*dwFlag*/) {
    if (!m_dev || !piv3Tri || dwTriCount == 0) return nullptr;
    auto* m = MeshObject::createEmpty(m_dev.get(), CMeshFlag());
    if (!m) return nullptr;
    // Convert IVERTEX (pos+uv) into the MESH_DESC layout.
    const std::uint32_t n = dwTriCount * 3;
    std::vector<VECTOR3> positions(n);
    std::vector<VECTOR3> normals(n);
    std::vector<TVERTEX> tex(n);
    for (std::uint32_t i = 0; i < n; ++i) {
        positions[i] = { piv3Tri[i].x, piv3Tri[i].y, piv3Tri[i].z };
        normals[i]   = { 0.0f, 1.0f, 0.0f };
        tex[i]       = { piv3Tri[i].u1, piv3Tri[i].v1 };
    }
    MESH_DESC md{};
    md.dwVertexNum     = n;
    md.pv3WorldList    = positions.data();
    md.dwTexVertexNum  = n;
    md.ptvTexCoordList = tex.data();
    md.pv3NormalLocal  = normals.data();
    md.meshFlag        = CMeshFlag();
    if (!m->StartInitialize(&md, nullptr, nullptr)) { m->Release(); return nullptr; }

    std::vector<std::uint16_t> idx(n);
    for (std::uint32_t i = 0; i < n; ++i) idx[i] = static_cast<std::uint16_t>(i);
    FACE_DESC fd{};
    fd.pIndex     = idx.data();
    fd.dwFacesNum = dwTriCount;
    fd.dwMtlIndex = 0;
    if (!m->InsertFaceGroup(&fd)) { m->Release(); return nullptr; }
    m->EndInitialize();
    return m;
}

// ===== Render frame =====

void __stdcall CoD3DDeviceDX11::BeginRender(SHORT_RECT* pRect, std::uint32_t dwColor, std::uint32_t /*dwFlag*/) {
    if (!m_dev) return;
    m_dev->beginFrame(pRect, dwColor, 0);
}

void __stdcall CoD3DDeviceDX11::EndRender() {
    if (!m_dev) return;
    m_dev->endFrame();
}

// ===== Render flags =====

void __stdcall CoD3DDeviceDX11::SetShadowFlag(std::uint32_t dwFlag) { m_shadowFlag = dwFlag; }
std::uint32_t __stdcall CoD3DDeviceDX11::GetShadowFlag() { return m_shadowFlag; }
void __stdcall CoD3DDeviceDX11::SetLightMapFlag(std::uint32_t dwFlag) { m_lightMapFlag = dwFlag; }
std::uint32_t __stdcall CoD3DDeviceDX11::GetLightMapFlag() { return m_lightMapFlag; }
void __stdcall CoD3DDeviceDX11::SetRenderMode(std::uint32_t dwFlag) { m_renderMode = dwFlag; }
std::uint32_t __stdcall CoD3DDeviceDX11::GetRenderMode() { return m_renderMode; }

void __stdcall CoD3DDeviceDX11::EnableFog(float fStart, float fEnd, float fDensity,
                                          std::uint32_t dwColor, std::uint32_t /*dwFlag*/) {
    m_fogEnabled  = true;
    m_fogStart    = fStart;
    m_fogEnd      = fEnd;
    m_fogDensity  = fDensity;
    m_fogColor    = dwColor;
    MLOG_DEBUG("[renderer] Fog enabled (start=%.1f end=%.1f density=%.2f)",
               fStart, fEnd, fDensity);
}
void __stdcall CoD3DDeviceDX11::DisableFog() {
    m_fogEnabled = false;
}

BOOL __stdcall CoD3DDeviceDX11::BeginShadowMap() {
    if (!m_dev) return FALSE;
    return m_dev->beginShadowPass() ? TRUE : FALSE;
}
void __stdcall CoD3DDeviceDX11::EndShadowMap() {
    if (!m_dev) return;
    m_dev->endShadowPass();
}

void __stdcall CoD3DDeviceDX11::GetClientRect(SHORT_RECT* pRect, std::uint16_t* pwWidth, std::uint16_t* pwHeight) {
    if (!m_dev) return;
    RECT rc{};
    ::GetClientRect(m_hwnd, &rc);
    if (pRect) {
        pRect->left   = static_cast<short>(rc.left);
        pRect->top    = static_cast<short>(rc.top);
        pRect->right  = static_cast<short>(rc.right);
        pRect->bottom = static_cast<short>(rc.bottom);
    }
    if (pwWidth)  *pwWidth  = m_dev->width();
    if (pwHeight) *pwHeight = m_dev->height();
}

std::uint32_t __stdcall CoD3DDeviceDX11::CreateDynamicLight(std::uint32_t /*dwRS*/, std::uint32_t /*dwColor*/,
                                                             char* /*szFileName*/) {
    return 0xffffffff;
}
BOOL __stdcall CoD3DDeviceDX11::DeleteDynamicLight(std::uint32_t /*dwIndex*/) { return FALSE; }
BOOL __stdcall CoD3DDeviceDX11::CreateEffectShaderPaletteFromFile(char* szFileName) {
    if (!m_dev) return FALSE;
    if (!m_effectPalette) {
        m_effectPalette = std::make_unique<EffectShaderPalette>(m_dev.get());
    }
    return m_effectPalette->loadFromFile(szFileName) ? TRUE : FALSE;
}
BOOL __stdcall CoD3DDeviceDX11::CreateEffectShaderPalette(CUSTOM_EFFECT_DESC* p, std::uint32_t n) {
    if (!m_dev) return FALSE;
    if (!m_effectPalette) {
        m_effectPalette = std::make_unique<EffectShaderPalette>(m_dev.get());
    }
    return m_effectPalette->buildFromDesc(p, n) ? TRUE : FALSE;
}
void __stdcall CoD3DDeviceDX11::DeleteEffectShaderPalette() {
    m_effectPalette.reset();
}

// ===== Render mesh / sprite / font =====

BOOL __stdcall CoD3DDeviceDX11::RenderMeshObject(IDIMeshObject* pMeshObj, std::uint32_t /*dwRefIndex*/, float /*fDistance*/, std::uint32_t /*dwAlpha*/,
                                                LIGHT_INDEX_DESC* /*pDyn*/, std::uint32_t /*dwLightNum*/,
                                                LIGHT_INDEX_DESC* /*pSpot*/, std::uint32_t /*dwSpotNum*/,
                                                std::uint32_t /*dwMtlSet*/, std::uint32_t dwEffectIndex, std::uint32_t dwFlag) {
    if (!m_dev || !m_meshShadersReady || !pMeshObj) return FALSE;
    auto* mesh = static_cast<MeshObject*>(pMeshObj);
    if (!mesh->vertexBuffer() || mesh->faceGroups().empty()) return FALSE;

    auto* ctx = m_dev->rawContext();

    // Effect shader path: check RENDER_TYPE_USE_EFFECT flag.
    if ((dwFlag & RENDER_TYPE_USE_EFFECT) && m_effectPalette) {
        auto* effect = m_effectPalette->getEffect(dwEffectIndex);
        if (effect && effect->bSuccess) {
            mesh->setEffectPalette(m_effectPalette.get());
            mesh->RenderEffect(m_meshShaders.psEffect.Get(), nullptr, effect, 0);
            return TRUE;
        }
    }

    // Constant buffer: world matrix (identity for now — caller-driven transforms
    // would come via the Executive path in a full port).
    MATRIX4 world = MatrixIdentity();
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_meshShaders.cbWorld.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        std::memcpy(mapped.pData, &world, sizeof(MATRIX4));
        ctx->Unmap(m_meshShaders.cbWorld.Get(), 0);
    }
    if (SUCCEEDED(ctx->Map(m_meshShaders.cbViewProj.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        MATRIX4 vp = m_dev->viewProjMatrix();
        std::memcpy(mapped.pData, &vp, sizeof(MATRIX4));
        ctx->Unmap(m_meshShaders.cbViewProj.Get(), 0);
    }
    // Light CB: ambient + diffuse + lightDir + cameraPos + fogParams + fogColor.
    struct LightCB {
        float ambient[4];
        float diffuse[4];
        float lightDir[4];
        float cameraPos[4];
        float fogParams[4];
        float fogColor[4];
    } lcb{};
    lcb.ambient[0] = lcb.ambient[1] = lcb.ambient[2] = 0.25f;
    lcb.ambient[3] = 1.0f;
    lcb.diffuse[0] = lcb.diffuse[1] = lcb.diffuse[2] = 0.95f;
    lcb.diffuse[3] = 1.0f;
    lcb.lightDir[0] =  0.3f;  // soft top-down light
    lcb.lightDir[1] = -0.7f;
    lcb.lightDir[2] =  0.4f;
    lcb.lightDir[3] =  0.0f;
    auto camPos = m_dev->cameraPosition();
    lcb.cameraPos[0] = camPos.x;
    lcb.cameraPos[1] = camPos.y;
    lcb.cameraPos[2] = camPos.z;
    lcb.cameraPos[3] = 0.0f;
    lcb.fogParams[0] = m_fogEnabled ? 1.0f : 0.0f;
    lcb.fogParams[1] = m_fogStart;
    lcb.fogParams[2] = m_fogEnd;
    lcb.fogParams[3] = m_fogDensity;
    auto fr = static_cast<float>((m_fogColor >> 16) & 0xff) / 255.0f;
    auto fg = static_cast<float>((m_fogColor >>  8) & 0xff) / 255.0f;
    auto fb = static_cast<float>((m_fogColor      ) & 0xff) / 255.0f;
    lcb.fogColor[0] = fr; lcb.fogColor[1] = fg; lcb.fogColor[2] = fb;
    lcb.fogColor[3] = 1.0f;
    if (SUCCEEDED(ctx->Map(m_meshShaders.cbLight.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        std::memcpy(mapped.pData, &lcb, sizeof(LightCB));
        ctx->Unmap(m_meshShaders.cbLight.Get(), 0);
    }

    // Bind pipeline.
    UINT stride = sizeof(MeshObject::Vertex), offset = 0;
    ID3D11Buffer* vb = mesh->vertexBuffer();
    ID3D11Buffer* ib = mesh->indexBuffer();
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetInputLayout(m_meshShaders.ilLit.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(m_meshShaders.vsLit.Get(), nullptr, 0);
    ctx->PSSetShader(m_meshShaders.psLit.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_meshShaders.cbWorld.GetAddressOf());
    ctx->VSSetConstantBuffers(1, 1, m_meshShaders.cbViewProj.GetAddressOf());
    ctx->PSSetConstantBuffers(0, 1, m_meshShaders.cbLight.GetAddressOf());

    for (const auto& fg : mesh->faceGroups()) {
        if (!fg.diffuseSRV) {
            ID3D11ShaderResourceView* nullSRV = nullptr;
            ctx->PSSetShaderResources(0, 1, &nullSRV);
        } else {
            ctx->PSSetShaderResources(0, 1, fg.diffuseSRV.GetAddressOf());
        }
        ctx->DrawIndexed(fg.indexCount, fg.startIndex, 0);
    }
    // Unbind SRV to avoid D3D11 warnings.
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);
    return TRUE;
}

BOOL __stdcall CoD3DDeviceDX11::RenderSprite(IDISpriteObject* pSprite, VECTOR2* pv2Scaling, float fRot,
                                             VECTOR2* pv2Trans, RECT* pRect, std::uint32_t dwColor,
                                             int /*iZOrder*/, std::uint32_t dwFlag) {
    if (!pSprite) return FALSE;
    auto* sprite = dynamic_cast<SpriteObject*>(static_cast<IDISpriteObject*>(pSprite));
    if (!sprite) return FALSE;
    return sprite->Draw(pv2Scaling, fRot, pv2Trans, pRect, dwColor, dwFlag);
}

BOOL __stdcall CoD3DDeviceDX11::RenderFont(IDIFontObject* pFont, TCHAR* str, std::uint32_t dwLen,
                                            RECT* pRect, std::uint32_t dwColor, CHAR_CODE_TYPE type,
                                            int iZOrder, std::uint32_t dwFlag) {
    if (!pFont || !str || dwLen == 0 || !pRect) return FALSE;
    auto* font = dynamic_cast<FontObject*>(static_cast<IDIFontObject*>(pFont));
    if (!font) return FALSE;
    if (iZOrder != 0) {
        // Z-order layered rendering is reserved for a future sprite-batch path.
        MLOG_DEBUG("[renderer] RenderFont zOrder=%d ignored (sprite batching deferred)", iZOrder);
    }
    font->BeginRender();
    BOOL ok = font->DrawText(str, dwLen, pRect, dwColor, type, dwFlag);
    font->EndRender();
    return ok;
}

// ===== Render primitives =====

void __stdcall CoD3DDeviceDX11::RenderBox(VECTOR3* pv3Oct, std::uint32_t dwColor) {
    if (!m_dev || !pv3Oct) return;
    m_primitives.setViewProj(m_dev->viewProjMatrix());
    m_primitives.drawBox(pv3Oct, dwColor);
}
void __stdcall CoD3DDeviceDX11::RenderPoint(VECTOR3* pv3Point, std::uint32_t dwColor) {
    if (!m_dev || !pv3Point) return;
    // Phase 5: same top-down projection convention as RenderBox (X=screen X, Z=screen Y).
    // Full 3D→2D projection is left for later when we have a proper view*proj matrix
    // wired through the camera setup (currently the test/diagnostic scene uses the
    // identity viewport).
    m_primitives.setViewProj(m_dev->viewProjMatrix());
    VECTOR2 screen{ pv3Point->x, pv3Point->z };
    m_primitives.drawPoint(screen, dwColor);
}
void __stdcall CoD3DDeviceDX11::RenderCircle(VECTOR2* pv2Point, float fRs, std::uint32_t dwColor) {
    if (!m_dev || !pv2Point) return;
    m_primitives.setViewProj(m_dev->viewProjMatrix());
    m_primitives.drawCircle(*pv2Point, fRs, dwColor);
}
void __stdcall CoD3DDeviceDX11::RenderLine(VECTOR2* pv2Point0, VECTOR2* pv2Point1, std::uint32_t dwColor) {
    if (!m_dev || !pv2Point0 || !pv2Point1) return;
    m_primitives.setViewProj(m_dev->viewProjMatrix());
    m_primitives.drawLine(*pv2Point0, *pv2Point1, dwColor);
}
void __stdcall CoD3DDeviceDX11::RenderGrid(VECTOR3* pv3Quad, std::uint32_t dwColor) {
    if (!m_dev || !pv3Quad) return;
    m_primitives.setViewProj(m_dev->viewProjMatrix());
    m_primitives.drawGrid(pv3Quad, dwColor);
}
BOOL __stdcall CoD3DDeviceDX11::RenderTriIvertex(IVERTEX* /*p*/, void* /*m*/, std::uint32_t /*n*/, std::uint32_t /*f*/) {
    return FALSE;
}
BOOL __stdcall CoD3DDeviceDX11::RenderTriVector3(VECTOR3* /*p*/, std::uint32_t /*n*/, std::uint32_t /*f*/) {
    return FALSE;
}

void* __stdcall CoD3DDeviceDX11::AllocRenderTriBuffer(IVERTEX** /*p*/, std::uint32_t /*n*/, std::uint32_t /*f*/) {
    return nullptr;
}
void __stdcall CoD3DDeviceDX11::EnableRenderTriBuffer(void* /*h*/, void* /*m*/, std::uint32_t /*n*/) {}
void __stdcall CoD3DDeviceDX11::DisableRenderTriBuffer(void* /*h*/) {}
void __stdcall CoD3DDeviceDX11::FreeRenderTriBuffer(void* /*h*/) {}

// ===== Lighting =====

BOOL __stdcall CoD3DDeviceDX11::SetRTLight(LIGHT_DESC* /*p*/, std::uint32_t /*i*/, std::uint32_t /*f*/) {
    return TRUE;
}
void __stdcall CoD3DDeviceDX11::EnableDirectionalLight(DIRECTIONAL_LIGHT_DESC* /*p*/, std::uint32_t /*f*/) {}
void __stdcall CoD3DDeviceDX11::DisableDirectionalLight() {}
void __stdcall CoD3DDeviceDX11::SetSpotLightDesc(VECTOR3* /*a*/, VECTOR3* /*b*/, VECTOR3* /*c*/, float /*d*/,
                                                  float /*e*/, float /*f*/, float /*g*/, BOOL /*h*/, void* /*i*/,
                                                  std::uint32_t /*j*/, std::uint32_t /*k*/, SPOT_LIGHT_TYPE /*l*/) {}
void __stdcall CoD3DDeviceDX11::SetShadowLightSenderPosition(BOUNDING_SPHERE* pSphere, std::uint32_t /*dwLightIndex*/) {
    if (!m_dev || !pSphere) return;
    // Build a directional shadow light: orthographic projection from the light's
    // position, looking toward the scene (downward by default: light is above).
    VECTOR3 lightPos   = pSphere->v3Point;
    VECTOR3 lightAt    = { lightPos.x, lightPos.y - 1.0f, lightPos.z };  // look downward
    VECTOR3 lightUp    = { 0.0f, 0.0f, 1.0f };   // Z-up world space
    float   halfExtent = pSphere->fRs * 4.0f;     // cover enough scene area

    MATRIX4 matLightView, matLightProj, matShadow;
    MatrixLookAtLH(&matLightView, &lightPos, &lightAt, &lightUp);
    MatrixOrthographicLH(&matLightProj, halfExtent, halfExtent, 1.0f, halfExtent * 4.0f);
    MatrixMultiply2(&matShadow, &matLightView, &matLightProj);
    m_dev->setShadowLightMatrix(matShadow);
    MLOG_DEBUG("[renderer] Shadow light pos=(%.1f %.1f %.1f) radius=%.1f",
                lightPos.x, lightPos.y, lightPos.z, pSphere->fRs);
}

void __stdcall CoD3DDeviceDX11::SetViewFrusturm(VIEW_VOLUME* pViewVolume, CAMERA_DESC* camera,
                                                 MATRIX4* pMatView, MATRIX4* pMatProj, MATRIX4* pMatBillboard) {
    if (!m_dev) return;
    if (pViewVolume && camera && pMatView && pMatProj && pMatBillboard) {
        m_dev->setViewFrustum(*pViewVolume, *camera, *pMatView, *pMatProj, *pMatBillboard);
    }
}

void __stdcall CoD3DDeviceDX11::GetSystemStatus(SYSTEM_STATUS* pStatus) {
    if (!pStatus) return;
    pStatus->dwAvaliableTexMem = 256 * 1024 * 1024;  // 256 MB placeholder
    pStatus->dwTotalTexMem     = 1024 * 1024 * 1024;
    std::strncpy(pStatus->szDeviceType, "DX11", sizeof(pStatus->szDeviceType) - 1);
}

void __stdcall CoD3DDeviceDX11::UpdateWindowSize() {
    if (!m_dev) return;
    m_dev->updateWindowSize();
}
void __stdcall CoD3DDeviceDX11::Present(HWND hWnd) { if (m_dev) m_dev->present(hWnd); }

void __stdcall CoD3DDeviceDX11::SetAmbientColor(std::uint32_t /*dwColor*/) {}
std::uint32_t __stdcall CoD3DDeviceDX11::GetAmbientColor() { return 0xff202020; }
void __stdcall CoD3DDeviceDX11::SetEmissiveColor(std::uint32_t /*dwColor*/) {}
std::uint32_t __stdcall CoD3DDeviceDX11::GetEmissiveColor() { return 0xff000000; }

void __stdcall CoD3DDeviceDX11::BeginPerformanceAnalyze() {}
void __stdcall CoD3DDeviceDX11::EndPerformanceAnalyze() {}

BOOL __stdcall CoD3DDeviceDX11::CaptureScreen(char* szFileName) {
    if (!m_dev || !szFileName) return FALSE;
    auto* swapChain = m_dev->rawSwapChain();
    auto* device   = m_dev->rawDevice();
    auto* ctx      = m_dev->rawContext();
    if (!swapChain || !device || !ctx) return FALSE;

    // Get the back buffer.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer))) {
        MLOG_ERROR("[renderer] CaptureScreen: GetBuffer failed");
        return FALSE;
    }

    // Describe a staging texture matching the back buffer for CPU read.
    D3D11_TEXTURE2D_DESC desc{};
    backBuffer->GetDesc(&desc);
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.BindFlags      = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTex;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, stagingTex.GetAddressOf()))) {
        MLOG_ERROR("[renderer] CaptureScreen: CreateTexture2D(staging) failed");
        return FALSE;
    }

    // Copy the back buffer to the staging texture.
    ctx->CopyResource(stagingTex.Get(), backBuffer.Get());

    // Map for reading.
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        MLOG_ERROR("[renderer] CaptureScreen: Map failed");
        return FALSE;
    }

    // Pack into LoadedTexture (RGBA8).
    LoadedTexture ltex;
    ltex.width  = desc.Width;
    ltex.height = desc.Height;
    ltex.bps    = 32;
    ltex.pixels.resize(static_cast<std::size_t>(desc.Width) * desc.Height * 4);
    const std::uint8_t* src = static_cast<const std::uint8_t*>(mapped.pData);
    const UINT rowPitch = mapped.RowPitch;
    for (UINT y = 0; y < desc.Height; ++y) {
        std::memcpy(ltex.pixels.data() + y * desc.Width * 4,
                    src + y * rowPitch,
                    desc.Width * 4);
    }
    ctx->Unmap(stagingTex.Get(), 0);

    // Encode and write TGA.
    std::vector<std::uint8_t> tga = saveTGA(ltex);
    if (tga.empty()) {
        MLOG_ERROR("[renderer] CaptureScreen: saveTGA failed");
        return FALSE;
    }

    FILE* fp = nullptr;
    if (fopen_s(&fp, szFileName, "wb") != 0 || !fp) {
        MLOG_ERROR("[renderer] CaptureScreen: fopen failed for '%s'", szFileName);
        return FALSE;
    }
    std::fwrite(tga.data(), 1, tga.size(), fp);
    std::fclose(fp);
    MLOG_INFO("[renderer] CaptureScreen: saved %zux%zu to '%s'",
              static_cast<std::size_t>(ltex.width),
              static_cast<std::size_t>(ltex.height), szFileName);
    return TRUE;
}

// ===== Material =====

std::uint32_t __stdcall CoD3DDeviceDX11::CreateMaterialSet(MATERIAL_TABLE* pMtlEntry, std::uint32_t dwNum) {
    if (!pMtlEntry || dwNum == 0) return 0;
    if (!m_dev) return 0;

    auto set = std::make_unique<MaterialSet>();
    set->entries.reserve(dwNum);

    for (std::uint32_t i = 0; i < dwNum; ++i) {
        MATERIAL_TABLE& entry = pMtlEntry[i];
        if (!entry.pMtl) {
            set->entries.push_back(nullptr);  // null entry for this slot
            continue;
        }

        auto mat = std::make_unique<MaterialData>();
        MATERIAL& src = *entry.pMtl;
        mat->dwDiffuse       = src.dwDiffuse;
        mat->dwAmbient       = src.dwAmbient;
        mat->dwSpecular      = src.dwSpecular;
        mat->fTransparency   = src.fTransparency;
        mat->fShine          = src.fShine;
        mat->fShineStrength  = src.fShineStrength;
        mat->dwFlag          = src.dwFlag;

        // Load diffuse texture.
        const char* diffuseName = src.GetDiffuseTexmapName();
        if (diffuseName && diffuseName[0] != '\0') {
            mat->diffuse = loadMaterialTexture(diffuseName);
        }

        // Load reflect texture (environment map).
        const char* reflectName = src.GetReflectTexmapName();
        if (reflectName && reflectName[0] != '\0') {
            mat->reflect = loadMaterialTexture(reflectName);
        }

        // Load bump/normal map.
        const char* bumpName = src.GetBumpTexmapName();
        if (bumpName && bumpName[0] != '\0') {
            mat->bump = loadMaterialTexture(bumpName);
        }

        // Transfer ownership to the material set (not m_materials).
        set->entries.push_back(std::move(mat));
    }

    std::uint32_t handle = static_cast<std::uint32_t>(m_materialSets.size()) + 1;
    m_materialSets.push_back(std::move(set));
    return handle;  // 0 means "invalid", so caller uses handle directly
}

void __stdcall CoD3DDeviceDX11::DeleteMaterialSet(std::uint32_t dwMtlSetIndex) {
    if (dwMtlSetIndex == 0 || dwMtlSetIndex > m_materialSets.size()) return;
    std::uint32_t idx = dwMtlSetIndex - 1;
    m_materialSets[idx].reset();  // unique_ptr destructor handles entries
}

void* __stdcall CoD3DDeviceDX11::CreateMaterial(MATERIAL* pMaterial,
                                                std::uint32_t* pdwWidth,
                                                std::uint32_t* pdwHeight,
                                                std::uint32_t /*dwFlag*/) {
    if (!pMaterial) return nullptr;

    auto mat = std::make_unique<MaterialData>();
    mat->dwDiffuse       = pMaterial->dwDiffuse;
    mat->dwAmbient       = pMaterial->dwAmbient;
    mat->dwSpecular      = pMaterial->dwSpecular;
    mat->fTransparency   = pMaterial->fTransparency;
    mat->fShine          = pMaterial->fShine;
    mat->fShineStrength  = pMaterial->fShineStrength;
    mat->dwFlag          = pMaterial->dwFlag;

    // Load diffuse texture.
    const char* diffuseName = pMaterial->GetDiffuseTexmapName();
    if (diffuseName && diffuseName[0] != '\0') {
        mat->diffuse = loadMaterialTexture(diffuseName);
    }

    if (pdwWidth)  *pdwWidth  = mat->diffuse.width;
    if (pdwHeight) *pdwHeight = mat->diffuse.height;

    // Load reflect / bump textures.
    const char* reflectName = pMaterial->GetReflectTexmapName();
    if (reflectName && reflectName[0] != '\0') {
        mat->reflect = loadMaterialTexture(reflectName);
    }
    const char* bumpName = pMaterial->GetBumpTexmapName();
    if (bumpName && bumpName[0] != '\0') {
        mat->bump = loadMaterialTexture(bumpName);
    }

    MaterialData* raw = mat.get();
    m_materials[raw] = std::move(mat);
    return static_cast<void*>(raw);
}

void __stdcall CoD3DDeviceDX11::SetMaterialTextureBorder(void* pMtlHandle, std::uint32_t dwColor) {
    if (!pMtlHandle) return;
    auto it = m_materials.find(static_cast<MaterialData*>(pMtlHandle));
    if (it != m_materials.end()) {
        it->second->borderColor = dwColor;
    }
}

void __stdcall CoD3DDeviceDX11::DeleteMaterial(void* pMtlHandle) {
    if (!pMtlHandle) return;
    m_materials.erase(static_cast<MaterialData*>(pMtlHandle));
}

// ---------------------------------------------------------------------------
// Material helpers
// ---------------------------------------------------------------------------
void CoD3DDeviceDX11::shutdownMaterials() {
    m_materials.clear();
    m_materialSets.clear();
}

MaterialTexture CoD3DDeviceDX11::loadMaterialTexture(const char* fileName) {
    MaterialTexture tex;
    if (!m_dev || !fileName || fileName[0] == '\0') return tex;
    tex.srv = m_dev->createTextureFromFile(fileName);
    if (tex.srv) {
        tex.loaded = true;
        // Query the underlying texture to get width/height.
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        tex.srv->GetResource(resource.GetAddressOf());
        if (resource) {
            D3D11_RESOURCE_DIMENSION dim{};
            resource->GetType(&dim);
            if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
                Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
                resource.As(&tex2D);
                if (tex2D) {
                    D3D11_TEXTURE2D_DESC desc{};
                    tex2D->GetDesc(&desc);
                    tex.width  = desc.Width;
                    tex.height = desc.Height;
                }
            }
        }
    }
    return tex;
}

// ===== Misc =====

void __stdcall CoD3DDeviceDX11::SetAttentuation0(float att) { m_attenuation0 = att; }
float __stdcall CoD3DDeviceDX11::GetAttentuation0() { return m_attenuation0; }
BOOL __stdcall CoD3DDeviceDX11::ConvertCompressedTexture(char* /*f*/, std::uint32_t /*g*/) { return FALSE; }
void __stdcall CoD3DDeviceDX11::EnableSpecular(float /*f*/) {}
void __stdcall CoD3DDeviceDX11::DisableSpecular() {}
void __stdcall CoD3DDeviceDX11::SetVerticalSync(BOOL bSwitch) { m_vsync = bSwitch; }
BOOL __stdcall CoD3DDeviceDX11::IsSetVerticalSync() { return m_vsync; }
void __stdcall CoD3DDeviceDX11::ResetDevice(BOOL /*bTest*/) { /* Phase 5 stub */ }
void __stdcall CoD3DDeviceDX11::SetFreeVBCacheRate(float fVal) { m_freeVBCacheRate = fVal; }
float __stdcall CoD3DDeviceDX11::GetFreeVBCacheRate() { return m_freeVBCacheRate; }
std::uint32_t __stdcall CoD3DDeviceDX11::ClearVBCacheWithIDIMeshObject(IDIMeshObject* /*p*/) { return 0; }
std::uint32_t __stdcall CoD3DDeviceDX11::ClearCacheWithMotionUID(void* /*p*/) { return 0; }
void __stdcall CoD3DDeviceDX11::SetTickCount(std::uint32_t t, BOOL /*g*/) {
    if (m_effectPalette) m_effectPalette->setTickCount(t);
}

BOOL __stdcall CoD3DDeviceDX11::GetD3DDevice(REFIID refiid, void** ppVoid) {
    if (!m_dev || !ppVoid) return FALSE;
    // Phase 5: we return our DX11 device under the legacy IID. Real callers
    // should use internalDevice() instead.
    if (refiid == __uuidof(IUnknown)) {
        *ppVoid = m_dev->rawDevice();
        return TRUE;
    }
    return FALSE;
}

BOOL __stdcall CoD3DDeviceDX11::InitializeRenderTarget(std::uint32_t /*s*/, std::uint32_t /*n*/) { return TRUE; }
void __stdcall CoD3DDeviceDX11::SetRenderTextureMustUpdate(BOOL /*b*/) {}
void __stdcall CoD3DDeviceDX11::SetAlphaRefValue(std::uint32_t v) { m_alphaRefValue = v; }

BOOL __stdcall CoD3DDeviceDX11::SetLoadFailedTextureTable(TEXTURE_TABLE* /*p*/, std::uint32_t /*n*/) { return TRUE; }
void __stdcall CoD3DDeviceDX11::GetLoadFailedTextureTable(TEXTURE_TABLE** /*p*/, std::uint32_t* /*a*/, std::uint32_t* /*b*/) {}

void __stdcall CoD3DDeviceDX11::SetRenderWireSolidBothMode(BOOL b) { m_wireSolidBothMode = b; }
BOOL __stdcall CoD3DDeviceDX11::GetRenderWireSolidBothMode() { return m_wireSolidBothMode; }

} // namespace mxh::gx::dx11

// ===== Factory =====

extern "C" HRESULT __stdcall CreateGXRendererInstance(void** ppv) {
    if (!ppv) return E_POINTER;
    auto* renderer = new mxh::gx::dx11::CoD3DDeviceDX11();
    *ppv = renderer;
    return S_OK;
}
