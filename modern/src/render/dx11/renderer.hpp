// mxh/render/dx11/renderer.hpp
// CoD3DDeviceDX11: implements I4DyuchiGXRenderer with full method signatures
// matching the original DX8 engine. Critical-path methods (Create/Begin/End/
// Present/Sprite/Box/Line/Point/Circle/SetViewFrustum) are real DX11 impls;
// advanced features (HeightField, Effect shader, MeshObject) are stubbed
// pending Phase 5 expansion. The interface contract is preserved.
#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "mxh/render/IRenderer.hpp"
#include "device.hpp"
#include "primitives.hpp"
#include "mesh_shaders.hpp"
#include "font_object.hpp"
#include "height_field.hpp"
#include "effect_shader.hpp"
#include "material.hpp"
#include "dynamic_light.hpp"
#include "tri_buffer.hpp"

namespace mxh::gx::dx11 {

class CoD3DDeviceDX11 : public I4DyuchiGXRenderer {
public:
    CoD3DDeviceDX11();
    ~CoD3DDeviceDX11();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // I4DyuchiGXRenderer - all 75 methods declared here for completeness.
    // Implementation in renderer.cpp.
    BOOL __stdcall Create(HWND hWnd, DISPLAY_INFO* pInfo, I4DyuchiFileStorage* pFileStorage,
                          ErrorHandleProc pErrorHandleFunc) override;

    IDISpriteObject* __stdcall CreateSpriteObject(char* szFileName, std::uint32_t dwFlag) override;
    IDISpriteObject* __stdcall CreateSpriteObject(char* szFileName, std::uint32_t dwXPos, std::uint32_t dwYPos,
                                                   std::uint32_t dwWidth, std::uint32_t dwHeight,
                                                   std::uint32_t dwFlag) override;
    IDISpriteObject* __stdcall CreateEmptySpriteObject(std::uint32_t dwWidth, std::uint32_t dwHeight,
                                                        TEXTURE_FORMAT TexFormat, std::uint32_t dwFlag) override;
    IDIMeshObject* __stdcall CreateMeshObject(CMeshFlag flag) override;
    IDIFontObject* __stdcall CreateFontObject(LOGFONT* pLogFont, std::uint32_t dwFlag) override;
    IDIHeightField* __stdcall CreateHeightField(std::uint32_t dwFlag) override;
    IDIMeshObject* __stdcall CreateImmMeshObject(IVERTEX* piv3Tri, std::uint32_t dwTriCount,
                                                  void* pMtlHandle, std::uint32_t dwFlag) override;

    void __stdcall BeginRender(SHORT_RECT* pRect, std::uint32_t dwColor, std::uint32_t dwFlag) override;
    void __stdcall EndRender() override;
    void __stdcall SetShadowFlag(std::uint32_t dwFlag) override;
    std::uint32_t __stdcall GetShadowFlag() override;
    void __stdcall SetLightMapFlag(std::uint32_t dwFlag) override;
    std::uint32_t __stdcall GetLightMapFlag() override;
    void __stdcall SetRenderMode(std::uint32_t dwFlag) override;
    std::uint32_t __stdcall GetRenderMode() override;
    void __stdcall EnableFog(float fStart, float fEnd, float fDensity, std::uint32_t dwColor,
                              std::uint32_t dwFlag) override;
    void __stdcall DisableFog() override;
    BOOL __stdcall BeginShadowMap() override;
    void __stdcall EndShadowMap() override;
    void __stdcall GetClientRect(SHORT_RECT* pRect, std::uint16_t* pwWidth, std::uint16_t* pwHeight) override;
    std::uint32_t __stdcall CreateDynamicLight(std::uint32_t dwRS, std::uint32_t dwColor,
                                                char* szFileName) override;
    BOOL __stdcall DeleteDynamicLight(std::uint32_t dwIndex) override;
    BOOL __stdcall CreateEffectShaderPaletteFromFile(char* szFileName) override;
    BOOL __stdcall CreateEffectShaderPalette(CUSTOM_EFFECT_DESC* pEffectDescList, std::uint32_t dwNum) override;
    void __stdcall DeleteEffectShaderPalette() override;

    BOOL __stdcall RenderMeshObject(IDIMeshObject* pMeshObj, std::uint32_t dwRefIndex, float fDistance,
                                     std::uint32_t dwAlpha,
                                     LIGHT_INDEX_DESC* pDynamicLightIndexList, std::uint32_t dwLightNum,
                                     LIGHT_INDEX_DESC* pSpotLightIndexList, std::uint32_t dwSpotLightNum,
                                     std::uint32_t dwMtlSetIndex, std::uint32_t dwEffectIndex,
                                     std::uint32_t dwFlag) override;
    BOOL __stdcall RenderSprite(IDISpriteObject* pSprite, VECTOR2* pv2Scaling, float fRot,
                                 VECTOR2* pv2Trans, RECT* pRect, std::uint32_t dwColor, int iZOrder,
                                 std::uint32_t dwFlag) override;
    BOOL __stdcall RenderFont(IDIFontObject* pFont, TCHAR* str, std::uint32_t dwLen, RECT* pRect,
                                std::uint32_t dwColor, CHAR_CODE_TYPE type, int iZOrder,
                                std::uint32_t dwFlag) override;
    void __stdcall RenderBox(VECTOR3* pv3Oct, std::uint32_t dwColor) override;
    void __stdcall RenderPoint(VECTOR3* pv3Point, std::uint32_t dwColor) override;
    void __stdcall RenderCircle(VECTOR2* pv2Point, float fRs, std::uint32_t dwColor) override;
    void __stdcall RenderLine(VECTOR2* pv2Point0, VECTOR2* pv2Point1, std::uint32_t dwColor) override;
    void __stdcall RenderGrid(VECTOR3* pv3Quad, std::uint32_t dwColor) override;
    BOOL __stdcall RenderTriIvertex(IVERTEX* piv3Tri, void* pMtlHandle, std::uint32_t dwFacesNum,
                                     std::uint32_t dwFlag) override;
    BOOL __stdcall RenderTriVector3(VECTOR3* pv3Tri, std::uint32_t dwFacesNum, std::uint32_t dwFlag) override;
    void*  __stdcall AllocRenderTriBuffer(IVERTEX** ppIVList, std::uint32_t dwFacesNum,
                                            std::uint32_t dwRenderFlag) override;
    void __stdcall EnableRenderTriBuffer(void* pTriBufferHandle, void* pMtlHandle,
                                           std::uint32_t dwRenderFacesNum) override;
    void __stdcall DisableRenderTriBuffer(void* pTriBufferHandle) override;
    void __stdcall FreeRenderTriBuffer(void* pTriBufferHandle) override;

    BOOL __stdcall SetRTLight(LIGHT_DESC* pLightDesc, std::uint32_t dwLightIndex, std::uint32_t dwFlag) override;
    void __stdcall EnableDirectionalLight(DIRECTIONAL_LIGHT_DESC* pLightDesc, std::uint32_t dwFlag) override;
    void __stdcall DisableDirectionalLight() override;
    void __stdcall SetSpotLightDesc(VECTOR3* pv3From, VECTOR3* pv3To, VECTOR3* pv3Up, float fFov,
                                     float fNear, float fFar, float fWidth, BOOL bOrtho, void* pMtlHandle,
                                     std::uint32_t dwColorOP, std::uint32_t dwLightIndex,
                                     SPOT_LIGHT_TYPE type) override;
    void __stdcall SetShadowLightSenderPosition(BOUNDING_SPHERE* pSphere, std::uint32_t dwLightIndex) override;
    void __stdcall SetViewFrusturm(VIEW_VOLUME* pViewVolume, CAMERA_DESC* camera, MATRIX4* pMatView,
                                    MATRIX4* pMatProj, MATRIX4* pMatForBilboard) override;
    void __stdcall GetSystemStatus(SYSTEM_STATUS* pStatus) override;
    void __stdcall UpdateWindowSize() override;
    void __stdcall Present(HWND hWnd) override;
    void __stdcall SetAmbientColor(std::uint32_t dwColor) override;
    std::uint32_t __stdcall GetAmbientColor() override;
    void __stdcall SetEmissiveColor(std::uint32_t dwColor) override;
    std::uint32_t __stdcall GetEmissiveColor() override;
    void __stdcall BeginPerformanceAnalyze() override;
    void __stdcall EndPerformanceAnalyze() override;
    BOOL __stdcall CaptureScreen(char* szFileName) override;

    std::uint32_t __stdcall CreateMaterialSet(MATERIAL_TABLE* pMtlEntry, std::uint32_t dwNum) override;
    void __stdcall DeleteMaterialSet(std::uint32_t dwMtlSetIndex) override;
    void* __stdcall CreateMaterial(MATERIAL* pMaterial, std::uint32_t* pdwWidth, std::uint32_t* pdwHeight,
                                     std::uint32_t dwFlag) override;
    void __stdcall SetMaterialTextureBorder(void* pMtlHandle, std::uint32_t dwColor) override;
    void __stdcall DeleteMaterial(void* pMtlHandle) override;
    void __stdcall SetAttentuation0(float att) override;
    float __stdcall GetAttentuation0() override;
    BOOL __stdcall ConvertCompressedTexture(char* szFileName, std::uint32_t dwFlag) override;
    void __stdcall EnableSpecular(float fVal) override;
    void __stdcall DisableSpecular() override;
    void __stdcall SetVerticalSync(BOOL bSwitch) override;
    BOOL __stdcall IsSetVerticalSync() override;
    void __stdcall ResetDevice(BOOL bTest) override;
    void __stdcall SetFreeVBCacheRate(float fVal) override;
    float __stdcall GetFreeVBCacheRate() override;
    std::uint32_t __stdcall ClearVBCacheWithIDIMeshObject(IDIMeshObject* pObject) override;
    std::uint32_t __stdcall ClearCacheWithMotionUID(void* pMotionUID) override;
    void __stdcall SetTickCount(std::uint32_t dwTickCount, BOOL bGameFrame) override;
    BOOL __stdcall GetD3DDevice(REFIID refiid, void** ppVoid) override;
    BOOL __stdcall InitializeRenderTarget(std::uint32_t dwTexelSize, std::uint32_t dwMaxTexNum) override;
    void __stdcall SetRenderTextureMustUpdate(BOOL bMustUpdate) override;
    void __stdcall SetAlphaRefValue(std::uint32_t dwRefVaule) override;

    BOOL __stdcall SetLoadFailedTextureTable(TEXTURE_TABLE* pLoadFailedTextureTable,
                                               std::uint32_t dwLoadFailedTextureTableSize) override;
    void __stdcall GetLoadFailedTextureTable(TEXTURE_TABLE** ppoutLoadFailedTextureTable,
                                               std::uint32_t* poutdwLoadFailedTextureTableSize,
                                               std::uint32_t* poutdwFailedTextureCount) override;
    void __stdcall SetRenderWireSolidBothMode(BOOL bMode) override;
    BOOL __stdcall GetRenderWireSolidBothMode() override;

    // Internal accessors for testing.
    dx11::Device*          internalDevice()     { return m_dev.get(); }
    PrimitiveDrawer*       internalPrimitives() { return &m_primitives; }
    I4DyuchiFileStorage*   internalStorage()    { return m_storage; }
    EffectShaderPalette*   internalEffectPalette() { return m_effectPalette.get(); }

    // Effect palette helpers (mirrors INL_GetVLMeshEffect in original).
    EffectEntry* INL_GetVLMeshEffect(std::uint32_t index) {
        return m_effectPalette ? m_effectPalette->getEffect(index) : nullptr;
    }
    void SetSphereMapMatrix(MATRIX4* pResultMat, const MATRIX4* pMatWorld, const MATRIX4* pMatView) {
        if (m_effectPalette) m_effectPalette->setSphereMapMatrix(pResultMat, pMatWorld, pMatView);
    }
    void SetWaveTexMatrix(MATRIX4* pMatResult) {
        if (m_effectPalette) m_effectPalette->setWaveTexMatrix(pMatResult);
    }

private:
    std::unique_ptr<dx11::Device>          m_dev;
    std::unique_ptr<EffectShaderPalette>   m_effectPalette;
    PrimitiveDrawer                        m_primitives;
    MeshShaders                            m_meshShaders;
    bool                                   m_meshShadersReady = false;
    I4DyuchiFileStorage*                   m_storage = nullptr;
    ErrorHandleProc                        m_errorHandler = nullptr;
    HWND                                   m_hwnd = nullptr;
    std::uint32_t                          m_refCount = 1;
    std::uint32_t                          m_shadowFlag = 0;
    std::uint32_t                          m_lightMapFlag = 0;
    std::uint32_t                          m_renderMode = RENDER_MODE_SOLID;
    std::uint32_t                          m_alphaRefValue = DEFAULT_ALPHA_REF_VALUE;
    float                                  m_attenuation0 = 1.0f;
    float                                  m_freeVBCacheRate = DEFAULT_FREE_VBCACHE_RATE;
    BOOL                                   m_vsync = TRUE;
    BOOL                                   m_wireSolidBothMode = FALSE;

    // Fog state. The mesh PS applies linear fog when m_fogEnabled is true.
    bool                                   m_fogEnabled = false;
    float                                  m_fogStart  = 0.0f;
    float                                  m_fogEnd    = 0.0f;
    float                                  m_fogDensity = 1.0f;
    std::uint32_t                          m_fogColor  = 0xff000000;

    // -------------------------------------------------------------------------
    // Material management.
    // -------------------------------------------------------------------------
    // Material sets: handle = dwMtlSetIndex + 1 (0 means "invalid set").
    std::vector<std::unique_ptr<MaterialSet>> m_materialSets;
    // All materials across all sets, keyed by raw MaterialData* handle.
    // (The handle is simply a pointer to the MaterialData stored here.)
    std::unordered_map<MaterialData*, std::unique_ptr<MaterialData>> m_materials;

    // Helper: load a texture from the file system into an SRV + dimensions.
    MaterialTexture loadMaterialTexture(const char* fileName);

    // Helper: shut down all material sets (called from destructor / ResetDevice).
    void shutdownMaterials();

    // -------------------------------------------------------------------------
    // Dynamic Light management.
    // -------------------------------------------------------------------------
    std::array<DynamicLight, MAX_DYNAMIC_LIGHTS> m_dynamicLights{};

    // -------------------------------------------------------------------------
    // TriBuffer management.
    // -------------------------------------------------------------------------
    // Active TriBuffer set. Handles are pointers into this map.
    // A TriBuffer handle is the pointer value itself.
    std::vector<std::unique_ptr<TriBuffer>> m_triBuffers;

    // Currently enabled TriBuffer (for DisableRenderTriBuffer).
    TriBuffer* m_activeTriBuffer = nullptr;

    // Helper: build LightCB from active dynamic lights + fog state.
    void buildLightCB(LightCB& out, const float ambient[4], const float diffuse[4],
                      const float lightDir[4], const float cameraPos4[4],
                      LIGHT_INDEX_DESC* pDynList, std::uint32_t dwLightNum) const;
};

} // namespace mxh::gx::dx11
