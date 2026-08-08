// mxh/render/dx11/mesh_object.hpp
// IDIMeshObject DX11 implementation. Loads simple static-mesh data into
// vertex/index buffers and a per-face-group material table.
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

#include "mxh/render/IRenderer.hpp"

namespace mxh::gx::dx11 {

class Device;

// Per-face-group material binding (subset of mesh rendered with one texture).
struct FaceGroup {
    std::uint32_t               startIndex;
    std::uint32_t               indexCount;
    std::uint32_t               mtlIndex;     // index into the parent's material table
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuseSRV;
};

class MeshObject : public IDIMeshObject {
public:
    static MeshObject* createEmpty(Device* dev, CMeshFlag flag);
    static MeshObject* createFromFile(Device* dev, I4DyuchiFileStorage* storage, const char* path);

    // Vertex layout: pos(3f) + uv(2f) + normal(3f) — matches the lit shader's
    // input element layout. Public so the renderer can compute stride.
    struct Vertex {
        float x, y, z;
        float u, v;
        float nx, ny, nz;
    };

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDIMeshObject
    BOOL __stdcall StartInitialize(MESH_DESC* pDesc, IGeometryController* pControl,
                                    IGeometryControllerStatic* pControlStatic) override;
    void __stdcall EndInitialize() override;
    BOOL __stdcall InsertFaceGroup(FACE_DESC* pDesc) override;
    void __stdcall SetFaceGroupDiffuseSRV(std::uint32_t groupIndex,
                                          void* srv) override {
        setDiffuseSRV(groupIndex, static_cast<ID3D11ShaderResourceView*>(srv));
    }
    BOOL __stdcall Render(std::uint32_t dwRefIndex, std::uint32_t dwAlpha,
                           LIGHT_INDEX_DESC* pDynamicLightIndexList, std::uint32_t dwLightNum,
                           LIGHT_INDEX_DESC* pSpotLightIndexList, std::uint32_t dwSpotLightNum,
                           std::uint32_t dwMtlSetIndex, std::uint32_t dwEffectIndex,
                           std::uint32_t dwFlag) override;
    BOOL __stdcall RenderProjection(std::uint32_t dwRefIndex, std::uint32_t dwAlpha,
                                     std::uint8_t* pSpotLightIndex, std::uint32_t dwViewNum,
                                     std::uint32_t dwFlag) override;
    BOOL __stdcall Update(std::uint32_t dwFlag) override;
    void __stdcall DisableUpdate() override;

    // Convenience for tests/demo: build a unit cube with given diffuse SRV.
    bool initializeCube(Device* dev, ID3D11ShaderResourceView* diffuseSRV);

    // Set a diffuse SRV on a specific face group (defaults to group 0).
    void setDiffuseSRV(std::uint32_t groupIndex, ID3D11ShaderResourceView* srv);

    // Effect shader support: wire the effect palette and renderer for
    // RENDER_TYPE_USE_EFFECT path (RenderEffect).
    void setRenderer(Device* dev);
    void setEffectPalette(class EffectShaderPalette* palette);
    void releaseBuffers();

    // Render using an effect entry (sphere-map / wave). Called by the renderer's
    // RenderMeshObject when dwFlag & RENDER_TYPE_USE_EFFECT.
    // matWorld: object's world matrix (used for sphere-map texgen).
    // pEffect:  the EffectEntry from the palette (pre-fetched by index).
    // alpha:    alpha modulation (0-255).
    void RenderEffect(ID3D11PixelShader* psEffect, const MATRIX4* matWorld,
                      struct EffectEntry* pEffect, std::uint32_t alpha);

    Device*                device()       const { return m_dev; }
    ID3D11Buffer*          vertexBuffer() const { return m_vertexBuffer.Get(); }
    ID3D11Buffer*          indexBuffer()  const { return m_indexBuffer.Get(); }
    const std::vector<FaceGroup>& faceGroups() const { return m_faceGroups; }

private:
    MeshObject() = default;
public:
    ~MeshObject();

    bool finalizeVB();

    Device*                                m_dev = nullptr;
    class EffectShaderPalette*             m_effectPalette = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Buffer>   m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>   m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>   m_texMatrixBuffer;
    std::vector<FaceGroup>                 m_faceGroups;

    std::vector<Vertex>                    m_vertices;
    std::vector<std::uint16_t>             m_indices;
    std::uint32_t                          m_vertexCount = 0;
    std::uint32_t                          m_indexCount  = 0;
    std::uint32_t                          m_refCount    = 1;
};

} // namespace mxh::gx::dx11
