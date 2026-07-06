// mxh/render/IRenderer.hpp
// 1:1 port of original 4DyuchiGRX_common/IRenderer.h.
// All methods preserve original signatures (stdcall, parameter order, return type)
// so existing 4Dyuchi client code can be linked against our DX11 implementation
// with only the .lib / .dll swap.
#pragma once

#include <objbase.h>

#include "math.hpp"
#include "render_typedef.hpp"
#include "mesh_flag.hpp"
#include "motion_flag.hpp"
#include "IFileStorage.hpp"

namespace mxh::gx {

interface IGeometryController;
interface IGeometryControllerStatic;

interface IDIMeshObject : public IUnknown {
    virtual BOOL __stdcall StartInitialize(MESH_DESC* pDesc, IGeometryController* pControl,
                                            IGeometryControllerStatic* pControlStatic) = 0;
    virtual void __stdcall EndInitialize() = 0;
    virtual BOOL __stdcall InsertFaceGroup(FACE_DESC* pDesc) = 0;
    virtual BOOL __stdcall Render(std::uint32_t dwRefIndex, std::uint32_t dwAlpha,
                                   LIGHT_INDEX_DESC* pDynamicLightIndexList, std::uint32_t dwLightNum,
                                   LIGHT_INDEX_DESC* pSpotLightIndexList, std::uint32_t dwSpotLightNum,
                                   std::uint32_t dwMtlSetIndex, std::uint32_t dwEffectIndex,
                                   std::uint32_t dwFlag) = 0;
    virtual BOOL __stdcall RenderProjection(std::uint32_t dwRefIndex, std::uint32_t dwAlpha,
                                             std::uint8_t* pSpotLightIndex, std::uint32_t dwViewNum,
                                             std::uint32_t dwFlag) = 0;
    virtual BOOL __stdcall Update(std::uint32_t dwFlag) = 0;
    virtual void __stdcall DisableUpdate() = 0;
};

interface IDIHFieldObject : public IUnknown {
    virtual BOOL __stdcall Create(std::uint32_t dwPosX, std::uint32_t dwPosZ, std::uint32_t dwDetailLevel,
                                   std::uint32_t dwFacesNumPerX, std::uint32_t dwFacesNumPerZ,
                                   VECTOR3* pv3Rect, HFIELD_DESC* pHFDesc) = 0;
    virtual BOOL __stdcall SetYFactor(std::uint32_t dwDestPitch, HFIELD_DESC* pHFDesc) = 0;
    virtual BOOL __stdcall SetVertexColor(std::uint32_t* pdwColor, std::uint32_t dwVerticesNum) = 0;
    virtual BOOL __stdcall SetVertexColorAll(std::uint32_t dwColor) = 0;
    virtual BOOL __stdcall SetDetailLevel(std::uint32_t dwDetailLevel) = 0;
    virtual void __stdcall SetDistanceFromViewPoint(float fDistance) = 0;
    virtual void __stdcall SetPositionMask(std::uint32_t dwPosMask) = 0;
    virtual BOOL __stdcall ReBuildMesh(std::uint32_t dwDestPitch, HFIELD_DESC* pHFDesc, std::uint32_t* pdwColor) = 0;
    virtual void __stdcall SetMustUpdate() = 0;
    virtual BOOL __stdcall UpdateAlphaMap(TILE_BUFFER_DESC* pTileBufferDesc) = 0;
    virtual void __stdcall CleanupAlphaMap() = 0;
};

interface IDIImmMeshObject : public IUnknown {
    virtual BOOL __stdcall GetTriBufferPtr(std::uint8_t** ppDest, std::uint32_t* pdwSize) = 0;
    virtual BOOL __stdcall Update(std::uint32_t dwFlag) = 0;
};

interface IDIFontObject : public IUnknown {
    virtual void __stdcall BeginRender() = 0;
    virtual void __stdcall EndRender() = 0;
    virtual BOOL __stdcall DrawText(TCHAR* str, std::uint32_t dwLen, RECT* pRect, std::uint32_t dwColor,
                                     CHAR_CODE_TYPE type, std::uint32_t dwFlag) = 0;
};

interface IDISpriteObject : public IUnknown {
    virtual BOOL __stdcall Draw(VECTOR2* pv2Scaling, float fRot, VECTOR2* pv2Trans, RECT* pRect,
                                 std::uint32_t dwColor, std::uint32_t dwFlag) = 0;
    virtual BOOL __stdcall Resize(float fWidth, float fHeight) = 0;
    virtual BOOL __stdcall GetImageHeader(IMAGE_HEADER* pImgHeader, std::uint32_t dwFrameIndex) = 0;
    virtual BOOL __stdcall LockRect(LOCKED_RECT* pOutLockedRect, RECT* pRect, TEXTURE_FORMAT TexFormat) = 0;
    virtual BOOL __stdcall UnlockRect() = 0;
};

interface IDIHeightField : public IUnknown {
    virtual BOOL __stdcall StartInitialize(HFIELD_DESC* pDesc) = 0;
    virtual void __stdcall EndInitialize() = 0;
    virtual IDIMeshObject* __stdcall CreateHeightFieldObject(HFIELD_OBJECT_DESC* pDesc) = 0;
    virtual BOOL __stdcall InitiallizeIndexBufferPool(std::uint32_t dwDetailLevel, std::uint32_t dwIndicesNum,
                                                     std::uint32_t dwNum) = 0;
    virtual BOOL __stdcall LoadTilePalette(TEXTURE_TABLE* pTexTable, std::uint32_t dwTileTextureNum) = 0;
    virtual BOOL __stdcall ReplaceTile(char* szFileName, std::uint32_t dwTexIndex) = 0;
    virtual BOOL __stdcall CreateIndexBuffer(std::uint32_t dwIndicesNum, std::uint32_t dwDetailLevel,
                                             std::uint32_t dwPositionMask, std::uint32_t dwNum) = 0;
    virtual BOOL __stdcall LockIndexBufferPtr(std::uint16_t** ppWord, std::uint32_t dwDetailLevel,
                                              std::uint32_t dwPositionMask) = 0;
    virtual void __stdcall UnlcokIndexBufferPtr(std::uint32_t dwDetailLevel, std::uint32_t dwPositionMask) = 0;
    virtual BOOL __stdcall RenderGrid(VECTOR3* pv3Quad, std::uint32_t dwTexTileIndex, std::uint32_t dwAlpha) = 0;
    virtual void __stdcall SetHFieldTileBlend(BOOL bSwitch) = 0;
    virtual BOOL __stdcall IsEnableHFieldTileBlend() = 0;
};

interface I4DyuchiGXRenderer : public IUnknown {
    // Original GUID from 4DyuchiGRX_common/IRenderer_GUID.h
    // {E2BEB0CE-C837-4ef1-B5E4-6B3D375A83F4}
    virtual BOOL __stdcall Create(HWND hWnd, DISPLAY_INFO* pInfo, I4DyuchiFileStorage* pFileStorage,
                                   ErrorHandleProc pErrorHandleFunc) = 0;

    virtual IDISpriteObject* __stdcall CreateSpriteObject(char* szFileName, std::uint32_t dwFlag) = 0;
    virtual IDISpriteObject* __stdcall CreateSpriteObject(char* szFileName, std::uint32_t dwXPos,
                                                           std::uint32_t dwYPos, std::uint32_t dwWidth,
                                                           std::uint32_t dwHeight, std::uint32_t dwFlag) = 0;
    virtual IDISpriteObject* __stdcall CreateEmptySpriteObject(std::uint32_t dwWidth, std::uint32_t dwHeight,
                                                                  TEXTURE_FORMAT TexFormat, std::uint32_t dwFlag) = 0;
    virtual IDIMeshObject* __stdcall CreateMeshObject(CMeshFlag flag) = 0;
    virtual IDIFontObject* __stdcall CreateFontObject(LOGFONT* pLogFont, std::uint32_t dwFlag) = 0;
    virtual IDIHeightField* __stdcall CreateHeightField(std::uint32_t dwFlag) = 0;
    virtual IDIMeshObject* __stdcall CreateImmMeshObject(IVERTEX* piv3Tri, std::uint32_t dwTriCount,
                                                          void* pMtlHandle, std::uint32_t dwFlag) = 0;

    virtual void __stdcall BeginRender(SHORT_RECT* pRect, std::uint32_t dwColor, std::uint32_t dwFlag) = 0;
    virtual void __stdcall EndRender() = 0;

    virtual void __stdcall SetShadowFlag(std::uint32_t dwFlag) = 0;
    virtual std::uint32_t __stdcall GetShadowFlag() = 0;
    virtual void __stdcall SetLightMapFlag(std::uint32_t dwFlag) = 0;
    virtual std::uint32_t __stdcall GetLightMapFlag() = 0;
    virtual void __stdcall SetRenderMode(std::uint32_t dwFlag) = 0;
    virtual std::uint32_t __stdcall GetRenderMode() = 0;

    virtual void __stdcall EnableFog(float fStart, float fEnd, float fDensity, std::uint32_t dwColor,
                                     std::uint32_t dwFlag) = 0;
    virtual void __stdcall DisableFog() = 0;

    virtual BOOL __stdcall BeginShadowMap() = 0;
    virtual void __stdcall EndShadowMap() = 0;

    virtual void __stdcall GetClientRect(SHORT_RECT* pRect, std::uint16_t* pwWidth, std::uint16_t* pwHeight) = 0;
    virtual std::uint32_t __stdcall CreateDynamicLight(std::uint32_t dwRS, std::uint32_t dwColor,
                                                        char* szFileName) = 0;
    virtual BOOL __stdcall DeleteDynamicLight(std::uint32_t dwIndex) = 0;

    virtual BOOL __stdcall CreateEffectShaderPaletteFromFile(char* szFileName) = 0;
    virtual BOOL __stdcall CreateEffectShaderPalette(CUSTOM_EFFECT_DESC* pEffectDescList, std::uint32_t dwNum) = 0;
    virtual void __stdcall DeleteEffectShaderPalette() = 0;

    virtual BOOL __stdcall RenderMeshObject(IDIMeshObject* pMeshObj, std::uint32_t dwRefIndex, float fDistance,
                                            std::uint32_t dwAlpha,
                                            LIGHT_INDEX_DESC* pDynamicLightIndexList, std::uint32_t dwLightNum,
                                            LIGHT_INDEX_DESC* pSpotLightIndexList, std::uint32_t dwSpotLightNum,
                                            std::uint32_t dwMtlSetIndex, std::uint32_t dwEffectIndex,
                                            std::uint32_t dwFlag) = 0;
    virtual BOOL __stdcall RenderSprite(IDISpriteObject* pSprite, VECTOR2* pv2Scaling, float fRot,
                                        VECTOR2* pv2Trans, RECT* pRect, std::uint32_t dwColor, int iZOrder,
                                        std::uint32_t dwFlag) = 0;
    virtual BOOL __stdcall RenderFont(IDIFontObject* pFont, TCHAR* str, std::uint32_t dwLen, RECT* pRect,
                                       std::uint32_t dwColor, CHAR_CODE_TYPE type, int iZOrder,
                                       std::uint32_t dwFlag) = 0;
    virtual void __stdcall RenderBox(VECTOR3* pv3Oct, std::uint32_t dwColor) = 0;
    virtual void __stdcall RenderPoint(VECTOR3* pv3Point, std::uint32_t dwColor) = 0;
    virtual void __stdcall RenderCircle(VECTOR2* pv2Point, float fRs, std::uint32_t dwColor) = 0;
    virtual void __stdcall RenderLine(VECTOR2* pv2Point0, VECTOR2* pv2Point1, std::uint32_t dwColor) = 0;
    virtual void __stdcall RenderGrid(VECTOR3* pv3Quad, std::uint32_t dwColor) = 0;
    virtual BOOL __stdcall RenderTriIvertex(IVERTEX* piv3Tri, void* pMtlHandle, std::uint32_t dwFacesNum,
                                            std::uint32_t dwFlag) = 0;
    virtual BOOL __stdcall RenderTriVector3(VECTOR3* pv3Tri, std::uint32_t dwFacesNum, std::uint32_t dwFlag) = 0;

    virtual void*  __stdcall AllocRenderTriBuffer(IVERTEX** ppIVList, std::uint32_t dwFacesNum,
                                                   std::uint32_t dwRenderFlag) = 0;
    virtual void __stdcall EnableRenderTriBuffer(void* pTriBufferHandle, void* pMtlHandle,
                                                 std::uint32_t dwRenderFacesNum) = 0;
    virtual void __stdcall DisableRenderTriBuffer(void* pTriBufferHandle) = 0;
    virtual void __stdcall FreeRenderTriBuffer(void* pTriBufferHandle) = 0;

    virtual BOOL __stdcall SetRTLight(LIGHT_DESC* pLightDesc, std::uint32_t dwLightIndex, std::uint32_t dwFlag) = 0;
    virtual void __stdcall EnableDirectionalLight(DIRECTIONAL_LIGHT_DESC* pLightDesc, std::uint32_t dwFlag) = 0;
    virtual void __stdcall DisableDirectionalLight() = 0;

    virtual void __stdcall SetSpotLightDesc(VECTOR3* pv3From, VECTOR3* pv3To, VECTOR3* pv3Up, float fFov,
                                            float fNear, float fFar, float fWidth, BOOL bOrtho, void* pMtlHandle,
                                            std::uint32_t dwColorOP, std::uint32_t dwLightIndex,
                                            SPOT_LIGHT_TYPE type) = 0;
    virtual void __stdcall SetShadowLightSenderPosition(BOUNDING_SPHERE* pSphere, std::uint32_t dwLightIndex) = 0;
    virtual void __stdcall SetViewFrusturm(VIEW_VOLUME* pViewVolume, CAMERA_DESC* camera, MATRIX4* pMatView,
                                           MATRIX4* pMatProj, MATRIX4* pMatForBilboard) = 0;
    virtual void __stdcall GetSystemStatus(SYSTEM_STATUS* pStatus) = 0;
    virtual void __stdcall UpdateWindowSize() = 0;
    virtual void __stdcall Present(HWND hWnd) = 0;
    virtual void __stdcall SetAmbientColor(std::uint32_t dwColor) = 0;
    virtual std::uint32_t __stdcall GetAmbientColor() = 0;
    virtual void __stdcall SetEmissiveColor(std::uint32_t dwColor) = 0;
    virtual std::uint32_t __stdcall GetEmissiveColor() = 0;
    virtual void __stdcall BeginPerformanceAnalyze() = 0;
    virtual void __stdcall EndPerformanceAnalyze() = 0;
    virtual BOOL __stdcall CaptureScreen(char* szFileName) = 0;

    virtual std::uint32_t __stdcall CreateMaterialSet(MATERIAL_TABLE* pMtlEntry, std::uint32_t dwNum) = 0;
    virtual void __stdcall DeleteMaterialSet(std::uint32_t dwMtlSetIndex) = 0;
    virtual void* __stdcall CreateMaterial(MATERIAL* pMaterial, std::uint32_t* pdwWidth, std::uint32_t* pdwHeight,
                                            std::uint32_t dwFlag) = 0;
    virtual void __stdcall SetMaterialTextureBorder(void* pMtlHandle, std::uint32_t dwColor) = 0;
    virtual void __stdcall DeleteMaterial(void* pMtlHandle) = 0;

    virtual void __stdcall SetAttentuation0(float att) = 0;
    virtual float __stdcall GetAttentuation0() = 0;
    virtual BOOL __stdcall ConvertCompressedTexture(char* szFileName, std::uint32_t dwFlag) = 0;
    virtual void __stdcall EnableSpecular(float fVal) = 0;
    virtual void __stdcall DisableSpecular() = 0;
    virtual void __stdcall SetVerticalSync(BOOL bSwitch) = 0;
    virtual BOOL __stdcall IsSetVerticalSync() = 0;
    virtual void __stdcall ResetDevice(BOOL bTest) = 0;
    virtual void __stdcall SetFreeVBCacheRate(float fVal) = 0;
    virtual float __stdcall GetFreeVBCacheRate() = 0;
    virtual std::uint32_t __stdcall ClearVBCacheWithIDIMeshObject(IDIMeshObject* pObject) = 0;
    virtual std::uint32_t __stdcall ClearCacheWithMotionUID(void* pMotionUID) = 0;
    virtual void __stdcall SetTickCount(std::uint32_t dwTickCount, BOOL bGameFrame) = 0;
    virtual BOOL __stdcall GetD3DDevice(REFIID refiid, void** ppVoid) = 0;
    virtual BOOL __stdcall InitializeRenderTarget(std::uint32_t dwTexelSize, std::uint32_t dwMaxTexNum) = 0;
    virtual void __stdcall SetRenderTextureMustUpdate(BOOL bMustUpdate) = 0;
    virtual void __stdcall SetAlphaRefValue(std::uint32_t dwRefVaule) = 0;

    virtual BOOL __stdcall SetLoadFailedTextureTable(TEXTURE_TABLE* pLoadFailedTextureTable,
                                                      std::uint32_t dwLoadFailedTextureTableSize) = 0;
    virtual void __stdcall GetLoadFailedTextureTable(TEXTURE_TABLE** ppoutLoadFailedTextureTable,
                                                      std::uint32_t* poutdwLoadFailedTextureTableSize,
                                                      std::uint32_t* poutdwFailedTextureCount) = 0;
    virtual void __stdcall SetRenderWireSolidBothMode(BOOL bMode) = 0;
    virtual BOOL __stdcall GetRenderWireSolidBothMode() = 0;
};

// Factory function (matches CoD3DDevice::CreateInstance).
extern "C" HRESULT __stdcall CreateGXRendererInstance(void** ppv);

} // namespace mxh::gx