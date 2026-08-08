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
#include <algorithm>

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

std::uint32_t __stdcall CoD3DDeviceDX11::CreateDynamicLight(std::uint32_t dwRS, std::uint32_t dwColor,
                                                             char* /*szFileName*/) {
    if (!m_dev) return 0xffffffff;

    // Find first free slot.
    for (std::uint32_t i = 0; i < MAX_DYNAMIC_LIGHTS; ++i) {
        if (!m_dynamicLights[i].bActive) {
            auto& L = m_dynamicLights[i];
            L.bActive      = true;
            L.bDirectional = (dwRS & LIGHT_FLAG_DIRECTIONAL) != 0;
            L.dwRS         = dwRS;
            L.dwColor      = dwColor;
            L.fAmbient     = 0.25f;
            L.fDiffuse     = 0.95f;
            // Default direction: downward
            L.v3Dir[0] = 0.f; L.v3Dir[1] = -1.f; L.v3Dir[2] = 0.f;
            L.v3Pos[0] = 0.f; L.v3Pos[1] = 0.f; L.v3Pos[2] = 0.f;
            L.fAttenuation0 = 0.f;
            L.fAttenuation1 = 0.05f;
            L.fAttenuation2 = 0.f;
            L.fRange     = 200.f;
            ++m_dynamicLightActiveCount;
            return i;  // index 0-7
        }
    }
    return 0xffffffff; // all slots full
}
BOOL __stdcall CoD3DDeviceDX11::DeleteDynamicLight(std::uint32_t dwIndex) {
    if (dwIndex >= MAX_DYNAMIC_LIGHTS) return FALSE;
    if (!m_dynamicLights[dwIndex].bActive) return FALSE;
    m_dynamicLights[dwIndex].bActive = false;
    if (m_dynamicLightActiveCount > 0) --m_dynamicLightActiveCount;
    return TRUE;
}
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
                                                LIGHT_INDEX_DESC* pDynList, std::uint32_t dwLightNum,
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

    // Light CB: base directional light + fog. Dynamic lights (pDynList) are accumulated
    // into extended slots for future multi-light shader support.
    float ambient[4]   = {0.25f, 0.25f, 0.25f, 1.0f};
    float diffuse[4]   = {0.95f, 0.95f, 0.95f, 1.0f};
    float lightDir[4]  = {0.3f, -0.7f, 0.4f, 0.0f};
    auto camPos = m_dev->cameraPosition();
    float camPos4[4] = {camPos.x, camPos.y, camPos.z, 0.0f};

    LightCB lcb{};
    buildLightCB(lcb, ambient, diffuse, lightDir, camPos4, pDynList, dwLightNum);
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
    // Pick the multi-light PS when there's at least one active dynamic or RT
    // light — it accumulates per-pixel contributions from up to 8 light slots.
    // When no lights are active, fall back to the cheaper single-directional PS.
    const bool needMultiLight = m_meshShaders.psMultiLight &&
                                (m_rtLightActiveCount > 0 ||
                                 (pDynList && dwLightNum > 0));
    ctx->PSSetShader(needMultiLight ? m_meshShaders.psMultiLight.Get()
                                    : m_meshShaders.psLit.Get(),
                     nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_meshShaders.cbWorld.GetAddressOf());
    ctx->VSSetConstantBuffers(1, 1, m_meshShaders.cbViewProj.GetAddressOf());
    ctx->PSSetConstantBuffers(0, 1, m_meshShaders.cbLight.GetAddressOf());
    // Bind the point sampler: the lit PS samples the diffuse texture at
    // t0/s0; without an explicit sampler state the default (possibly
    // undefined) state is used and the sample can produce black.
    ID3D11SamplerState* sampler = m_dev->samplerPointAddress();
    ctx->PSSetSamplers(0, 1, &sampler);

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
BOOL __stdcall CoD3DDeviceDX11::RenderTriIvertex(IVERTEX* piv3Tri, void* pMtlHandle,
                                                std::uint32_t dwFacesNum, std::uint32_t dwFlag) {
    // Convert IVERTEX (pos3+uv+normal) to solid-color debug triangles using the
    // 3D solid shader pair. Texture/material is not honored (this is the legacy
    // "draw tris now" debugging path; production particle effects go through
    // AllocRenderTriBuffer + EnableRenderTriBuffer + a dedicated material).
    if (!m_dev || !m_meshShadersReady || !piv3Tri || dwFacesNum == 0) return FALSE;

    std::uint32_t color = (pMtlHandle != nullptr) ? 0xFFFFFFFFu : 0xFFFFFFFFu;
    if (dwFlag & 0x10) color = 0xFFAAAAAAu;  // translucent hint: lighter shade

    const std::uint32_t vertCount = dwFacesNum * 3;
    struct TriVert { float x, y, z; std::uint32_t rgba; };
    std::vector<TriVert> verts(vertCount);
    for (std::uint32_t i = 0; i < vertCount; ++i) {
        verts[i].x = piv3Tri[i].x;
        verts[i].y = piv3Tri[i].y;
        verts[i].z = piv3Tri[i].z;
        verts[i].rgba = color;
    }
    return drawSolidTrisImpl(verts.data(), verts.size());
}

BOOL __stdcall CoD3DDeviceDX11::RenderTriVector3(VECTOR3* pv3Tri, std::uint32_t dwFacesNum,
                                                 std::uint32_t dwFlag) {
    if (!m_dev || !m_meshShadersReady || !pv3Tri || dwFacesNum == 0) return FALSE;

    const std::uint32_t vertCount = dwFacesNum * 3;
    struct TriVert { float x, y, z; std::uint32_t rgba; };
    std::vector<TriVert> verts(vertCount);
    // dwFlag & 0x01 selects a default tint; production callers in the legacy engine
    // usually pass a material handle — but for RenderTriVector3 there's no material
    // param, so we just use a constant white.
    const std::uint32_t color = (dwFlag & 0x01) ? 0xFFAAAAAAu : 0xFFFFFFFFu;
    for (std::uint32_t i = 0; i < vertCount; ++i) {
        verts[i].x = pv3Tri[i].x;
        verts[i].y = pv3Tri[i].y;
        verts[i].z = pv3Tri[i].z;
        verts[i].rgba = color;
    }
    return drawSolidTrisImpl(verts.data(), verts.size());
}

// Shared path used by RenderTriVector3 / RenderTriIvertex / any future solid-tri
// debug draw. Builds a transient dynamic VB, binds the 3D solid shader pair,
// updates cbViewProj, and issues Draw(vertCount, 0).
BOOL CoD3DDeviceDX11::drawSolidTrisImpl(const void* verts, std::uint32_t vertCount) {
    if (!m_dev || !verts || vertCount == 0) return FALSE;
    auto* device = m_dev->rawDevice();
    auto* ctx    = m_dev->rawContext();
    if (!device || !ctx) return FALSE;

    // Vertex layout: pos(12B) + rgba(4B) = 16B. Pack as 4 floats for direct
    // alignment; use DXGI_FORMAT_R8G8B8A8_UNORM so the byte packing goes in
    // literally as color (alpha in A channel).
    struct TriVert { float x, y, z; std::uint32_t rgba; };
    static_assert(sizeof(TriVert) == 16, "TriVert must be 16 bytes");

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth      = vertCount * sizeof(TriVert);
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = verts;

    Microsoft::WRL::ComPtr<ID3D11Buffer> vb;
    if (FAILED(device->CreateBuffer(&bd, &srd, &vb))) return FALSE;

    constexpr UINT stride = sizeof(TriVert);
    constexpr UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, vb.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(m_meshShaders.il3DSolid.Get());

    ctx->VSSetShader(m_meshShaders.vs3DSolid.Get(), nullptr, 0);
    ctx->PSSetShader(m_meshShaders.ps3DSolid.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_meshShaders.cbViewProj.GetAddressOf());
    ctx->PSSetShader(nullptr, nullptr, 0);  // PS doesn't need a CB

    // Update view-proj (identity world; mesh's viewProj already includes view).
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_meshShaders.cbViewProj.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        std::memcpy(mapped.pData, &m_dev->viewProjMatrix(), sizeof(MATRIX4));
        ctx->Unmap(m_meshShaders.cbViewProj.Get(), 0);
    }

    ctx->Draw(vertCount, 0);
    MLOG_DEBUG("[renderer] RenderTri: drew %u verts (%u tris)",
               vertCount, vertCount / 3);
    return TRUE;
}

void* __stdcall CoD3DDeviceDX11::AllocRenderTriBuffer(IVERTEX** ppIVList, std::uint32_t dwFacesNum, std::uint32_t dwFlag) {
    if (!m_dev || !ppIVList || dwFacesNum == 0) return nullptr;

    auto* device = m_dev->rawDevice();
    auto* ctx    = m_dev->rawContext();

    const std::uint32_t vertCount = dwFacesNum * 3;
    const bool isIndexed = (dwFlag & 0x01) != 0;

    // Layout: position (12B) + uv (8B) = 20B per vertex. Pad to 24B for D3D11 alignment.
    struct TriVert { float x, y, z, u, v; };

    std::vector<TriVert> verts;
    verts.reserve(vertCount);
    for (std::uint32_t i = 0; i < vertCount; ++i) {
        const IVERTEX* src = ppIVList[i];
        verts.push_back({src->x, src->y, src->z, src->u1, src->v1});
    }

    // Build vertex buffer.
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth      = static_cast<UINT>(vertCount * sizeof(TriVert));
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = verts.data();

    Microsoft::WRL::ComPtr<ID3D11Buffer> vb;
    if (FAILED(device->CreateBuffer(&bd, &srd, &vb))) return nullptr;

    Microsoft::WRL::ComPtr<ID3D11Buffer> ib;
    if (isIndexed) {
        std::vector<std::uint16_t> indices(vertCount);
        for (std::uint32_t i = 0; i < vertCount; ++i) indices[i] = static_cast<std::uint16_t>(i);
        D3D11_BUFFER_DESC ibd{};
        ibd.ByteWidth      = static_cast<UINT>(vertCount * sizeof(std::uint16_t));
        ibd.Usage          = D3D11_USAGE_IMMUTABLE;
        ibd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA isrd{};
        isrd.pSysMem = indices.data();
        if (FAILED(device->CreateBuffer(&ibd, &isrd, &ib))) return nullptr;
    }

    auto buf = std::make_unique<TriBuffer>();
    buf->vb           = vb;
    buf->ib           = ib;
    buf->vertexCount  = vertCount;
    buf->indexCount   = isIndexed ? vertCount : 0;
    buf->faceCount    = dwFacesNum;
    buf->indexed      = isIndexed;
    buf->mtlHandle    = nullptr;

    m_triBuffers.push_back(std::move(buf));
    return static_cast<void*>(m_triBuffers.back().get());
}

void __stdcall CoD3DDeviceDX11::EnableRenderTriBuffer(void* h, void* mtlHandle, std::uint32_t) {
    if (!m_dev || !h) return;
    auto* buf = static_cast<TriBuffer*>(h);
    if (buf->magic != TRI_BUFFER_MAGIC) return;

    auto* ctx = m_dev->rawContext();

    // Set vertex buffer.
    constexpr UINT stride = sizeof(float) * 5; // x,y,z,u,v
    constexpr UINT offset  = 0;
    ctx->IASetVertexBuffers(0, 1, buf->vb.GetAddressOf(), &stride, &offset);

    if (buf->indexed && buf->ib) {
        ctx->IASetIndexBuffer(buf->ib.Get(), DXGI_FORMAT_R16_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    } else {
        ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    ctx->IASetInputLayout(nullptr); // TriBuffer uses raw pos+uv, no IA layout needed for manual draw

    // Bind mesh shaders if ready.
    if (m_meshShadersReady) {
        ctx->VSSetShader(m_meshShaders.vsLit.Get(), nullptr, 0);
        ctx->PSSetShader(m_meshShaders.psLit.Get(), nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, m_meshShaders.cbWorld.GetAddressOf());
        ctx->VSSetConstantBuffers(1, 1, m_meshShaders.cbViewProj.GetAddressOf());
        ctx->PSSetConstantBuffers(0, 1, m_meshShaders.cbLight.GetAddressOf());
    }

    buf->mtlHandle = mtlHandle;
    m_activeTriBuffer = buf;
}

void __stdcall CoD3DDeviceDX11::DisableRenderTriBuffer(void* /*h*/) {
    if (!m_dev) return;
    m_activeTriBuffer = nullptr;
    // Restore IA state: rebind mesh object pipeline.
    // The next RenderMeshObject call will set up its own state.
}

void __stdcall CoD3DDeviceDX11::FreeRenderTriBuffer(void* h) {
    if (!h) return;
    auto* buf = static_cast<TriBuffer*>(h);
    if (buf->magic != TRI_BUFFER_MAGIC) return;
    if (m_activeTriBuffer == buf) m_activeTriBuffer = nullptr;

    for (auto it = m_triBuffers.begin(); it != m_triBuffers.end(); ++it) {
        if (it->get() == buf) {
            m_triBuffers.erase(it);
            break;
        }
    }
}

// ===== Light helper =====

void CoD3DDeviceDX11::buildLightCB(LightCB& out,
                                    const float ambient[4], const float diffuse[4],
                                    const float lightDir[4], const float cameraPos4[4],
                                    LIGHT_INDEX_DESC* pDynList, std::uint32_t dwLightNum) const {
    // Base directional light (matches cbLight 96B format).
    for (int i = 0; i < 4; ++i) {
        out.ambient[i]   = ambient[i];
        out.diffuse[i]   = diffuse[i];
        out.lightDir[i]  = lightDir[i];
        out.cameraPos[i] = cameraPos4[i];
    }
    out.fogParams[0] = m_fogEnabled ? 1.0f : 0.0f;
    out.fogParams[1] = m_fogStart;
    out.fogParams[2] = m_fogEnd;
    out.fogParams[3] = m_fogDensity;
    out.fogColor[0] = static_cast<float>((m_fogColor >> 16) & 0xff) / 255.f;
    out.fogColor[1] = static_cast<float>((m_fogColor >>  8) & 0xff) / 255.f;
    out.fogColor[2] = static_cast<float>((m_fogColor      ) & 0xff) / 255.f;
    out.fogColor[3] = 1.0f;

    // Zero-init extended dynamic-light slots.
    std::memset(out.dynLightPos0, 0, sizeof(out.dynLightPos0));
    std::memset(out.dynLightColor0, 0, sizeof(out.dynLightColor0));
    std::memset(out.dynLightAtten0, 0, sizeof(out.dynLightAtten0));
    // ... slots 1-7 are zeroed by the struct default-init or explicit below:
    std::memset(out.dynLightPos1, 0, sizeof(out.dynLightPos1));
    std::memset(out.dynLightColor1, 0, sizeof(out.dynLightColor1));
    std::memset(out.dynLightAtten1, 0, sizeof(out.dynLightAtten1));
    std::memset(out.dynLightPos2, 0, sizeof(out.dynLightPos2));
    std::memset(out.dynLightColor2, 0, sizeof(out.dynLightColor2));
    std::memset(out.dynLightAtten2, 0, sizeof(out.dynLightAtten2));
    std::memset(out.dynLightPos3, 0, sizeof(out.dynLightPos3));
    std::memset(out.dynLightColor3, 0, sizeof(out.dynLightColor3));
    std::memset(out.dynLightAtten3, 0, sizeof(out.dynLightAtten3));
    std::memset(out.dynLightPos4, 0, sizeof(out.dynLightPos4));
    std::memset(out.dynLightColor4, 0, sizeof(out.dynLightColor4));
    std::memset(out.dynLightAtten4, 0, sizeof(out.dynLightAtten4));
    std::memset(out.dynLightPos5, 0, sizeof(out.dynLightPos5));
    std::memset(out.dynLightColor5, 0, sizeof(out.dynLightColor5));
    std::memset(out.dynLightAtten5, 0, sizeof(out.dynLightAtten5));
    std::memset(out.dynLightPos6, 0, sizeof(out.dynLightPos6));
    std::memset(out.dynLightColor6, 0, sizeof(out.dynLightColor6));
    std::memset(out.dynLightAtten6, 0, sizeof(out.dynLightAtten6));
    std::memset(out.dynLightPos7, 0, sizeof(out.dynLightPos7));
    std::memset(out.dynLightColor7, 0, sizeof(out.dynLightColor7));
    std::memset(out.dynLightAtten7, 0, sizeof(out.dynLightAtten7));

    // Fill extended slots from pDynList.
    // Each LIGHT_INDEX_DESC maps a face group's material handle to a dynamic light index.
    // Extended slots 0-7 are written when the caller provides pDynList.
    float* posSlots[8]   = {out.dynLightPos0, out.dynLightPos1, out.dynLightPos2,
                             out.dynLightPos3, out.dynLightPos4, out.dynLightPos5,
                             out.dynLightPos6, out.dynLightPos7};
    float* colSlots[8]   = {out.dynLightColor0, out.dynLightColor1, out.dynLightColor2,
                             out.dynLightColor3, out.dynLightColor4, out.dynLightColor5,
                             out.dynLightColor6, out.dynLightColor7};
    float* attSlots[8]   = {out.dynLightAtten0, out.dynLightAtten1, out.dynLightAtten2,
                             out.dynLightAtten3, out.dynLightAtten4, out.dynLightAtten5,
                             out.dynLightAtten6, out.dynLightAtten7};

    if (pDynList) {
        for (std::uint32_t li = 0; li < dwLightNum && li < MAX_DYNAMIC_LIGHTS; ++li) {
            std::uint8_t idx = pDynList[li].bLightIndex;
            if (idx >= MAX_DYNAMIC_LIGHTS) continue;
            const auto& L = m_dynamicLights[idx];
            if (!L.bActive) continue;

            float rgb[4];
            color_to_float4(L.dwColor, rgb);

            posSlots[idx][0] = L.v3Pos[0];
            posSlots[idx][1] = L.v3Pos[1];
            posSlots[idx][2] = L.v3Pos[2];
            posSlots[idx][3] = L.bActive ? 1.0f : 0.0f; // enabled flag in w

            colSlots[idx][0] = rgb[0];
            colSlots[idx][1] = rgb[1];
            colSlots[idx][2] = rgb[2];
            colSlots[idx][3] = L.fRange;

            attSlots[idx][0] = L.fAttenuation0;
            attSlots[idx][1] = L.fAttenuation1;
            attSlots[idx][2] = L.fAttenuation2;
            attSlots[idx][3] = 0.0f;
        }
    }

    // Overlay RT lights (SetRTLight): takes priority over CreateDynamicLight
    // entries at the same index. RT lights are point/spot lights — their
    // v3Point is the world position, v3To is the aim point (used by future
    // spot-cone work). For now we always treat RT lights as point lights with
    // standard 1/d² attenuation and a range falloff. SetRTLight's pos.w is
    // set to 1.0f so the multi-light PS activates the slot.
    for (std::uint32_t idx = 0; idx < MAX_DYNAMIC_LIGHTS; ++idx) {
        const auto& R = m_rtLights[idx];
        if (!R.bActive) continue;
        const LIGHT_DESC& D = R.desc;
        float rgb[4];
        color_to_float4(D.dwDiffuse, rgb);
        posSlots[idx][0] = D.v3Point.x;
        posSlots[idx][1] = D.v3Point.y;
        posSlots[idx][2] = D.v3Point.z;
        posSlots[idx][3] = 1.0f; // enabled flag for the multi-light PS
        colSlots[idx][0] = rgb[0];
        colSlots[idx][1] = rgb[1];
        colSlots[idx][2] = rgb[2];
        colSlots[idx][3] = D.fRs > 0.f ? D.fRs : 200.f;
        attSlots[idx][0] = 1.0f;
        attSlots[idx][1] = 0.0f;
        attSlots[idx][2] = 0.0f;
        attSlots[idx][3] = 0.0f;
    }
}

// ===== Lighting =====

BOOL __stdcall CoD3DDeviceDX11::SetRTLight(LIGHT_DESC* pLightDesc, std::uint32_t dwLightIndex, std::uint32_t /*dwFlag*/) {
    if (!pLightDesc || dwLightIndex >= MAX_DYNAMIC_LIGHTS) {
        MLOG_WARN("[renderer] SetRTLight: invalid arg (p=%p, idx=%u)",
                  static_cast<const void*>(pLightDesc), dwLightIndex);
        return FALSE;
    }
    // Store the descriptor; mark the slot active. Recompute the active count
    // so RenderMeshObject can decide whether the multi-light PS is needed.
    bool wasActive = m_rtLights[dwLightIndex].bActive;
    m_rtLights[dwLightIndex].desc   = *pLightDesc;
    m_rtLights[dwLightIndex].bActive = true;
    if (!wasActive) ++m_rtLightActiveCount;
    MLOG_DEBUG("[renderer] SetRTLight[%u] active (count=%u, range=%.1f)",
               dwLightIndex, m_rtLightActiveCount, pLightDesc->fRs);
    return TRUE;
}
void __stdcall CoD3DDeviceDX11::EnableDirectionalLight(DIRECTIONAL_LIGHT_DESC* pDesc, std::uint32_t) {
    if (!pDesc) return;
    m_directionalLightEnabled = (pDesc->bEnable == TRUE);
    if (m_directionalLightEnabled) {
        m_ambientColor  = pDesc->dwAmbient;
        m_emissiveColor = pDesc->dwSpecular;
    }
}
void __stdcall CoD3DDeviceDX11::DisableDirectionalLight() {
    m_directionalLightEnabled = false;
}
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

void __stdcall CoD3DDeviceDX11::SetAmbientColor(std::uint32_t dwColor) { m_ambientColor = dwColor; }
std::uint32_t __stdcall CoD3DDeviceDX11::GetAmbientColor() { return 0xff202020; }
void __stdcall CoD3DDeviceDX11::SetEmissiveColor(std::uint32_t dwColor) { m_emissiveColor = dwColor; }
std::uint32_t __stdcall CoD3DDeviceDX11::GetEmissiveColor() { return 0xff000000; }

void __stdcall CoD3DDeviceDX11::BeginPerformanceAnalyze() {
    m_inPerfAnalyze = true;
    MLOG_DEBUG("[renderer] BeginPerformanceAnalyze: started");
}
void __stdcall CoD3DDeviceDX11::EndPerformanceAnalyze() {
    m_inPerfAnalyze = false;
    MLOG_DEBUG("[renderer] EndPerformanceAnalyze: ended");
}

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
BOOL __stdcall CoD3DDeviceDX11::ConvertCompressedTexture(char* szFileName, std::uint32_t /*dwFlag*/) {
    if (!szFileName || szFileName[0] == '\0') return FALSE;

    // Slurp the file into memory.
    FILE* fp = nullptr;
    if (fopen_s(&fp, szFileName, "rb") != 0 || !fp) {
        MLOG_WARN("[renderer] ConvertCompressedTexture: cannot open '%s'", szFileName);
        return FALSE;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(fp); return FALSE; }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(sz));
    size_t got = fread(data.data(), 1, data.size(), fp);
    std::fclose(fp);
    if (got != data.size()) {
        MLOG_WARN("[renderer] ConvertCompressedTexture: short read '%s'", szFileName);
        return FALSE;
    }

    // DDS fast path: passthrough any file that already starts with "DDS ".
    // Re-encoding an already-compressed BC file would be a quality regression.
    // TGA/BMP and other non-DDS inputs go through the BC encoder below.
    if (data.size() >= 4 &&
        data[0] == 'D' && data[1] == 'D' && data[2] == 'S' && data[3] == ' ') {
        MLOG_INFO("[renderer] ConvertCompressedTexture: '%s' is already DDS, "
                  "passthrough (no re-encode to avoid quality regression)",
                  szFileName);
        return TRUE;
    }

    // TGA/BMP/etc. → load → emit BC-compressed DDS beside the source.
    LoadedTexture tex = loadTextureFromMemory(data.data(), data.size());
    if (tex.pixels.empty() || tex.width == 0 || tex.height == 0) {
        MLOG_WARN("[renderer] ConvertCompressedTexture: decoder produced empty image for '%s'",
                  szFileName);
        return FALSE;
    }
    std::vector<std::uint8_t> dds = saveDDS_BC(tex);
    if (dds.empty()) {
        MLOG_WARN("[renderer] ConvertCompressedTexture: saveDDS failed");
        return FALSE;
    }

    // Replace the extension with .dds.
    char outPath[_MAX_PATH]{};
    std::strncpy(outPath, szFileName, _MAX_PATH - 1);
    outPath[_MAX_PATH - 1] = '\0';
    char* dot = std::strrchr(outPath, '.');
    if (!dot) dot = outPath + std::strlen(outPath);
    std::strcpy(dot, ".dds");

    FILE* out = nullptr;
    if (fopen_s(&out, outPath, "wb") != 0 || !out) {
        MLOG_WARN("[renderer] ConvertCompressedTexture: cannot write '%s'", outPath);
        return FALSE;
    }
    size_t wrote = fwrite(dds.data(), 1, dds.size(), out);
    std::fclose(out);
    if (wrote != dds.size()) {
        MLOG_WARN("[renderer] ConvertCompressedTexture: short write to '%s'", outPath);
        return FALSE;
    }
    MLOG_INFO("[renderer] ConvertCompressedTexture: %zux%zu → '%s' (%zu bytes)",
              static_cast<std::size_t>(tex.width), static_cast<std::size_t>(tex.height),
              outPath, dds.size());
    return TRUE;
}
void __stdcall CoD3DDeviceDX11::EnableSpecular(float fVal) {
    m_specularEnabled = true;
    m_specularShininess = fVal;
    MLOG_DEBUG("[renderer] EnableSpecular: enabled, shininess=%.1f", fVal);
}
void __stdcall CoD3DDeviceDX11::DisableSpecular() {
    m_specularEnabled = false;
    MLOG_DEBUG("[renderer] DisableSpecular: disabled");
}
void __stdcall CoD3DDeviceDX11::SetVerticalSync(BOOL bSwitch) {
    m_vsync = bSwitch;
    if (m_dev) m_dev->setVSync(bSwitch);
}
BOOL __stdcall CoD3DDeviceDX11::IsSetVerticalSync() { return m_vsync; }
void __stdcall CoD3DDeviceDX11::ResetDevice(BOOL bTest) {
    if (!m_dev) return;
    if (!bTest) {
        // Real reset: destroy and recreate device + swap chain + render targets.
        // The caller wants a clean state, so release everything and let Create() be
        // called again by the application.
        m_dev->release();
        // Drop all light state — slots/active counts are bound to the device.
        for (auto& R : m_rtLights) R.bActive = false;
        m_rtLightActiveCount = 0;
        for (auto& L : m_dynamicLights) L.bActive = false;
        m_dynamicLightActiveCount = 0;
        MLOG_INFO("[renderer] ResetDevice: device released (application must call Create again)");
    } else {
        // Test mode: just validate device is alive.
        MLOG_DEBUG("[renderer] ResetDevice test: device=%p, alive=%s",
                  m_dev.get(), m_dev->rawDevice() ? "yes" : "no");
    }
}
void __stdcall CoD3DDeviceDX11::SetFreeVBCacheRate(float fVal) { m_freeVBCacheRate = fVal; }
float __stdcall CoD3DDeviceDX11::GetFreeVBCacheRate() { return m_freeVBCacheRate; }
std::uint32_t __stdcall CoD3DDeviceDX11::ClearVBCacheWithIDIMeshObject(IDIMeshObject* pMeshObj) {
    if (!pMeshObj) return 0;
    auto* mesh = dynamic_cast<MeshObject*>(static_cast<IDIMeshObject*>(pMeshObj));
    if (!mesh) return 0;
    mesh->releaseBuffers();
    return 1; // 1 buffer set cleared (vb + ib in one call)
}
std::uint32_t __stdcall CoD3DDeviceDX11::ClearCacheWithMotionUID(void* pMotionUID) {
    // Motion-UID cache (Phase 5.13). Three semantics:
    //   pMotionUID == nullptr → clear ALL entries, return total removed
    //   pMotionUID found      → erase that entry, return 1
    //   pMotionUID not found  → no-op, return 0
    if (!pMotionUID) {
        const std::uint32_t total = static_cast<std::uint32_t>(m_motionCache.size());
        m_motionCache.clear();
        MLOG_DEBUG("[renderer] ClearCacheWithMotionUID(null): cleared %u motion entr%s",
                   total, total == 1 ? "y" : "ies");
        return total;
    }
    auto it = m_motionCache.find(pMotionUID);
    if (it == m_motionCache.end()) {
        MLOG_DEBUG("[renderer] ClearCacheWithMotionUID(%p): not in cache", pMotionUID);
        return 0;
    }
    m_motionCache.erase(it);
    MLOG_DEBUG("[renderer] ClearCacheWithMotionUID(%p): removed (cache now %zu entr%s)",
               pMotionUID, m_motionCache.size(),
               m_motionCache.size() == 1 ? "y" : "ies");
    return 1;
}

// -------------------------------------------------------------------------
// Motion cache registration / lookup (Phase 5.13)
// -------------------------------------------------------------------------
// These are renderer-internal: the IRenderer surface only exposes the clear
// path. Mesh system code calls RegisterMotionUID when a motion's GPU buffers
// are first uploaded, LookupMotionUID when it wants to share those buffers
// with another mesh playing the same motion, and UnregisterMotionUID when
// a mesh drops its reference. The same UID can be registered by N mesh
// objects (shared motion) and the cache evicts only when refcount hits 0.

void CoD3DDeviceDX11::RegisterMotionUID(void* motionUID, void* vb, void* ib,
                                        std::uint32_t vertexCount, std::uint32_t indexCount) {
    if (!motionUID) {
        MLOG_WARN("[renderer] RegisterMotionUID: rejected null UID");
        return;
    }
    auto& e = m_motionCache[motionUID];
    // First registration: fill in the buffers + counts. Subsequent
    // registrations (shared motion across mesh objects) only bump the
    // refcount; we don't overwrite vb/ib because they should be identical
    // for a given motion and the first registration is the canonical one.
    if (e.refCount == 0) {
        e.vb          = vb;
        e.ib          = ib;
        e.vertexCount = vertexCount;
        e.indexCount  = indexCount;
    } else if (e.vb != vb || e.ib != ib) {
        MLOG_WARN("[renderer] RegisterMotionUID(%p): re-registration with different "
                  "buffers (cached vb=%p ib=%p, new vb=%p ib=%p) — keeping cached",
                  motionUID, e.vb, e.ib, vb, ib);
    }
    ++e.refCount;
    MLOG_DEBUG("[renderer] RegisterMotionUID(%p) refCount=%u vb=%p ib=%p v=%u i=%u",
               motionUID, e.refCount, vb, ib, vertexCount, indexCount);
}

void CoD3DDeviceDX11::UnregisterMotionUID(void* motionUID) {
    if (!motionUID) return;
    auto it = m_motionCache.find(motionUID);
    if (it == m_motionCache.end()) {
        MLOG_DEBUG("[renderer] UnregisterMotionUID(%p): not in cache", motionUID);
        return;
    }
    if (it->second.refCount > 0) --it->second.refCount;
    if (it->second.refCount == 0) {
        MLOG_DEBUG("[renderer] UnregisterMotionUID(%p): refCount=0, evicting", motionUID);
        m_motionCache.erase(it);
    } else {
        MLOG_DEBUG("[renderer] UnregisterMotionUID(%p): refCount=%u (still alive)",
                   motionUID, it->second.refCount);
    }
}

bool CoD3DDeviceDX11::LookupMotionUID(void* motionUID, void** outVB, void** outIB,
                                      std::uint32_t* outVertexCount,
                                      std::uint32_t* outIndexCount) const {
    if (!motionUID) return false;
    auto it = m_motionCache.find(motionUID);
    if (it == m_motionCache.end()) return false;
    if (outVB)          *outVB          = it->second.vb;
    if (outIB)          *outIB          = it->second.ib;
    if (outVertexCount) *outVertexCount = it->second.vertexCount;
    if (outIndexCount)  *outIndexCount  = it->second.indexCount;
    return true;
}

void* CoD3DDeviceDX11::internalMotionCacheGetVB(void* motionUID) const {
    auto it = m_motionCache.find(motionUID);
    return it != m_motionCache.end() ? it->second.vb : nullptr;
}

void* CoD3DDeviceDX11::internalMotionCacheGetIB(void* motionUID) const {
    auto it = m_motionCache.find(motionUID);
    return it != m_motionCache.end() ? it->second.ib : nullptr;
}

std::uint32_t CoD3DDeviceDX11::internalMotionCacheVertexCount(void* motionUID) const {
    auto it = m_motionCache.find(motionUID);
    return it != m_motionCache.end() ? it->second.vertexCount : 0u;
}

std::uint32_t CoD3DDeviceDX11::internalMotionCacheIndexCount(void* motionUID) const {
    auto it = m_motionCache.find(motionUID);
    return it != m_motionCache.end() ? it->second.indexCount : 0u;
}

std::uint32_t CoD3DDeviceDX11::internalMotionCacheRefCount(void* motionUID) const {
    auto it = m_motionCache.find(motionUID);
    return it != m_motionCache.end() ? it->second.refCount : 0u;
}
void __stdcall CoD3DDeviceDX11::SetTickCount(std::uint32_t t, BOOL /*g*/) {
    if (m_effectPalette) m_effectPalette->setTickCount(t);
}

BOOL __stdcall CoD3DDeviceDX11::GetD3DDevice(REFIID refiid, void** ppVoid) {
    if (!m_dev || !ppVoid) return FALSE;
    // Phase 5.8: extend legacy IID support to direct DX11 IIDs so callers that
    // reach into the underlying device via __uuidof(ID3D11Device) /
    // __uuidof(ID3D11DeviceContext) can get the real interface back. Anything
    // else still falls through to FALSE (preserves the "only IUnknown" contract
    // documented in the plan; non-DX11 GUIDs aren't supported).
    if (refiid == __uuidof(IUnknown)) {
        *ppVoid = m_dev->rawDevice();
        return TRUE;
    }
    if (refiid == __uuidof(ID3D11Device)) {
        *ppVoid = m_dev->rawDevice();
        return TRUE;
    }
    if (refiid == __uuidof(ID3D11DeviceContext)) {
        *ppVoid = m_dev->rawContext();
        return TRUE;
    }
    return FALSE;
}

BOOL __stdcall CoD3DDeviceDX11::InitializeRenderTarget(std::uint32_t dwTexelSize, std::uint32_t dwMaxTexNum) {
    if (dwTexelSize == 0 || dwMaxTexNum == 0) {
        MLOG_WARN("[renderer] InitializeRenderTarget: invalid size=%u, num=%u",
                  dwTexelSize, dwMaxTexNum);
        return FALSE;
    }
    // Clamp to a reasonable upper bound to keep the descriptor footprint small.
    constexpr std::uint32_t kMaxAllowed = 64;
    if (dwMaxTexNum > kMaxAllowed) {
        MLOG_WARN("[renderer] InitializeRenderTarget: clamping %u -> %u", dwMaxTexNum, kMaxAllowed);
        dwMaxTexNum = kMaxAllowed;
    }
    m_rtTexelSize = dwTexelSize;
    m_rtMaxTexNum = dwMaxTexNum;
    m_rtNextSlot  = 0;
    MLOG_INFO("[renderer] InitializeRenderTarget: texelSize=%u maxNum=%u (descriptors only; "
              "GPU resources lazy-alloc on first use)", dwTexelSize, dwMaxTexNum);
    return TRUE;
}
void __stdcall CoD3DDeviceDX11::SetRenderTextureMustUpdate(BOOL b) {
    m_renderTextureMustUpdate = (b != FALSE);
    MLOG_DEBUG("[renderer] SetRenderTextureMustUpdate: %s", m_renderTextureMustUpdate ? "ON" : "OFF");
}
void __stdcall CoD3DDeviceDX11::SetAlphaRefValue(std::uint32_t v) { m_alphaRefValue = v; }

BOOL __stdcall CoD3DDeviceDX11::SetLoadFailedTextureTable(TEXTURE_TABLE* pLoadFailedTextureTable,
                                                            std::uint32_t dwLoadFailedTextureTableSize) {
    // Caller retains ownership. We just snapshot the pointer + size so
    // GetLoadFailedTextureTable can return the same view later. A null table
    // is allowed (used to clear the prior table).
    m_loadFailedTable      = pLoadFailedTextureTable;
    m_loadFailedTableSize  = dwLoadFailedTextureTableSize;
    MLOG_DEBUG("[renderer] SetLoadFailedTextureTable: %p, size=%u",
               static_cast<const void*>(pLoadFailedTextureTable),
               dwLoadFailedTextureTableSize);
    return TRUE;
}
void __stdcall CoD3DDeviceDX11::GetLoadFailedTextureTable(TEXTURE_TABLE** ppoutLoadFailedTextureTable,
                                                             std::uint32_t* poutdwLoadFailedTextureTableSize,
                                                             std::uint32_t* poutdwFailedTextureCount) {
    if (ppoutLoadFailedTextureTable) *ppoutLoadFailedTextureTable = m_loadFailedTable;
    if (poutdwLoadFailedTextureTableSize) *poutdwLoadFailedTextureTableSize = m_loadFailedTableSize;
    // Failed count is the number of distinct entries in the table — we don't
    // track per-load outcomes, so report the table size as the worst case.
    if (poutdwFailedTextureCount) *poutdwFailedTextureCount = m_loadFailedTableSize;
}

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
