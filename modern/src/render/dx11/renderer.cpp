// mxh/render/dx11/renderer.cpp
// Implementation of all 75 I4DyuchiGXRenderer methods.
#include "renderer.hpp"
#include "sprite.hpp"

#include <cstring>

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

CoD3DDeviceDX11::CoD3DDeviceDX11() = default;

CoD3DDeviceDX11::~CoD3DDeviceDX11() {
    m_primitives.shutdown();
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

IDIMeshObject* __stdcall CoD3DDeviceDX11::CreateMeshObject(CMeshFlag /*flag*/) {
    MLOG_WARN("[renderer] CreateMeshObject stub (Phase 5 advanced)");
    return nullptr;
}
IDIFontObject* __stdcall CoD3DDeviceDX11::CreateFontObject(LOGFONT* /*pLogFont*/, std::uint32_t /*dwFlag*/) {
    MLOG_WARN("[renderer] CreateFontObject stub (Phase 5 advanced)");
    return nullptr;
}
IDIHeightField* __stdcall CoD3DDeviceDX11::CreateHeightField(std::uint32_t /*dwFlag*/) {
    MLOG_WARN("[renderer] CreateHeightField stub (Phase 5 advanced)");
    return nullptr;
}
IDIMeshObject* __stdcall CoD3DDeviceDX11::CreateImmMeshObject(IVERTEX* /*piv3Tri*/, std::uint32_t /*dwTriCount*/,
                                                             void* /*pMtlHandle*/, std::uint32_t /*dwFlag*/) {
    MLOG_WARN("[renderer] CreateImmMeshObject stub (Phase 5 advanced)");
    return nullptr;
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

void __stdcall CoD3DDeviceDX11::EnableFog(float /*fStart*/, float /*fEnd*/, float /*fDensity*/,
                                          std::uint32_t /*dwColor*/, std::uint32_t /*dwFlag*/) {
    MLOG_DEBUG("[renderer] EnableFog stub");
}
void __stdcall CoD3DDeviceDX11::DisableFog() {}

BOOL __stdcall CoD3DDeviceDX11::BeginShadowMap() { return FALSE; }
void __stdcall CoD3DDeviceDX11::EndShadowMap() {}

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
BOOL __stdcall CoD3DDeviceDX11::CreateEffectShaderPaletteFromFile(char* /*szFileName*/) { return FALSE; }
BOOL __stdcall CoD3DDeviceDX11::CreateEffectShaderPalette(CUSTOM_EFFECT_DESC* /*p*/, std::uint32_t /*n*/) { return FALSE; }
void __stdcall CoD3DDeviceDX11::DeleteEffectShaderPalette() {}

// ===== Render mesh / sprite / font =====

BOOL __stdcall CoD3DDeviceDX11::RenderMeshObject(IDIMeshObject* /*pMeshObj*/, std::uint32_t /*dwRefIndex*/, float /*fDistance*/, std::uint32_t /*dwAlpha*/,
                                                LIGHT_INDEX_DESC* /*pDyn*/, std::uint32_t /*dwLightNum*/,
                                                LIGHT_INDEX_DESC* /*pSpot*/, std::uint32_t /*dwSpotNum*/,
                                                std::uint32_t /*dwMtlSet*/, std::uint32_t /*dwEffect*/, std::uint32_t /*dwFlag*/) {
    return FALSE;
}

BOOL __stdcall CoD3DDeviceDX11::RenderSprite(IDISpriteObject* pSprite, VECTOR2* pv2Scaling, float fRot,
                                             VECTOR2* pv2Trans, RECT* pRect, std::uint32_t dwColor,
                                             int /*iZOrder*/, std::uint32_t dwFlag) {
    if (!pSprite) return FALSE;
    auto* sprite = dynamic_cast<SpriteObject*>(static_cast<IDISpriteObject*>(pSprite));
    if (!sprite) return FALSE;
    return sprite->Draw(pv2Scaling, fRot, pv2Trans, pRect, dwColor, dwFlag);
}

BOOL __stdcall CoD3DDeviceDX11::RenderFont(IDIFontObject* /*pFont*/, TCHAR* /*str*/, std::uint32_t /*dwLen*/,
                                            RECT* /*pRect*/, std::uint32_t /*dwColor*/, CHAR_CODE_TYPE /*type*/,
                                            int /*iZOrder*/, std::uint32_t /*dwFlag*/) {
    return FALSE;
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
void __stdcall CoD3DDeviceDX11::SetShadowLightSenderPosition(BOUNDING_SPHERE* /*p*/, std::uint32_t /*i*/) {}

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
    // Phase 5 stub: re-create swap chain at new size.
}
void __stdcall CoD3DDeviceDX11::Present(HWND hWnd) { if (m_dev) m_dev->present(hWnd); }

void __stdcall CoD3DDeviceDX11::SetAmbientColor(std::uint32_t /*dwColor*/) {}
std::uint32_t __stdcall CoD3DDeviceDX11::GetAmbientColor() { return 0xff202020; }
void __stdcall CoD3DDeviceDX11::SetEmissiveColor(std::uint32_t /*dwColor*/) {}
std::uint32_t __stdcall CoD3DDeviceDX11::GetEmissiveColor() { return 0xff000000; }

void __stdcall CoD3DDeviceDX11::BeginPerformanceAnalyze() {}
void __stdcall CoD3DDeviceDX11::EndPerformanceAnalyze() {}
BOOL __stdcall CoD3DDeviceDX11::CaptureScreen(char* /*szFileName*/) { return FALSE; }

// ===== Material =====

std::uint32_t __stdcall CoD3DDeviceDX11::CreateMaterialSet(MATERIAL_TABLE* /*p*/, std::uint32_t /*n*/) { return 0; }
void __stdcall CoD3DDeviceDX11::DeleteMaterialSet(std::uint32_t /*i*/) {}
void* __stdcall CoD3DDeviceDX11::CreateMaterial(MATERIAL* /*p*/, std::uint32_t* /*w*/, std::uint32_t* /*h*/,
                                                  std::uint32_t /*f*/) { return nullptr; }
void __stdcall CoD3DDeviceDX11::SetMaterialTextureBorder(void* /*m*/, std::uint32_t /*c*/) {}
void __stdcall CoD3DDeviceDX11::DeleteMaterial(void* /*m*/) {}

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
void __stdcall CoD3DDeviceDX11::SetTickCount(std::uint32_t /*t*/, BOOL /*g*/) {}

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
