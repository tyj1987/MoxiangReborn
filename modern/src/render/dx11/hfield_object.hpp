// mxh/render/dx11/hfield_object.hpp
// IDIHFieldObject DX11 implementation.
//
// Phase 5.9a scope:
//   - Per-chunk height-field object: stores positions/UVs/normals/colors and
//     delegates GPU rendering to a MeshObject.
//   - All 11 IDIHFieldObject methods wired: Create, SetYFactor, SetVertexColor,
//     SetVertexColorAll, SetDetailLevel, SetDistanceFromViewPoint,
//     SetPositionMask, ReBuildMesh, SetMustUpdate, UpdateAlphaMap,
//     CleanupAlphaMap.
//   - Heights and vertex colors are kept CPU-side and pushed back into the
//     MeshObject on each ReBuildMesh (CPU rebuild is fine for Phase 5.9a; a
//     D3D11 buffer-update path is deferred until profiling shows it's a hot
//     spot).
//
// Deferred to a later phase:
//   - Lock/Unlock ID3D11 vertex buffer for true partial updates (needs a new
//     DYNAMIC USAGE VB on the MeshObject).
//   - 4-way tile texture blend in the PS (alpha map sampling).
#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <memory>
#include <vector>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

class Device;
class MeshObject;

// Per-chunk height-field mesh: owns CPU-side vertex data + a backing
// MeshObject for GPU rendering.
class HFieldObject : public IDIHFieldObject {
public:
    HFieldObject(Device* dev);
    ~HFieldObject();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDIHFieldObject (Phase 5.9a: all 11 methods wired)
    BOOL __stdcall Create(std::uint32_t dwPosX, std::uint32_t dwPosZ,
                          std::uint32_t dwDetailLevel,
                          std::uint32_t dwFacesNumPerX, std::uint32_t dwFacesNumPerZ,
                          VECTOR3* pv3Rect, HFIELD_DESC* pHFDesc) override;
    BOOL __stdcall SetYFactor(std::uint32_t dwDestPitch, HFIELD_DESC* pHFDesc) override;
    BOOL __stdcall SetVertexColor(std::uint32_t* pdwColor, std::uint32_t dwVerticesNum) override;
    BOOL __stdcall SetVertexColorAll(std::uint32_t dwColor) override;
    BOOL __stdcall SetDetailLevel(std::uint32_t dwDetailLevel) override;
    void __stdcall SetDistanceFromViewPoint(float fDistance) override;
    void __stdcall SetPositionMask(std::uint32_t dwPosMask) override;
    BOOL __stdcall ReBuildMesh(std::uint32_t dwDestPitch, HFIELD_DESC* pHFDesc,
                               std::uint32_t* pdwColor) override;
    void __stdcall SetMustUpdate() override;
    BOOL __stdcall UpdateAlphaMap(TILE_BUFFER_DESC* pTileBufferDesc) override;
    void __stdcall CleanupAlphaMap() override;

    // Accessors (used by HeightField manager + tests).
    MeshObject*             meshObject() const { return m_mesh.get(); }
    std::uint32_t           vertexCount() const { return m_vertCount; }
    std::uint32_t           indexCount()  const { return m_indexCount; }
    std::uint32_t           lodLevel()    const { return m_lodLevel; }
    std::uint32_t           posMask()     const { return m_posMask; }
    bool                    hasAlphaMap() const { return m_hasAlphaMap; }
    const std::vector<std::uint32_t>& colors() const { return m_colors; }
    const std::vector<std::uint8_t>&  alphaMap() const { return m_alphaMap; }

private:
    // Build the underlying MeshObject from current m_positions / m_texCoords /
    // m_normals + m_indices. Returns true on success.
    bool rebuildMeshObjectFromCPU();

    // Update vertex Y values by sampling pHFDesc->pyfList at the chunk's
    // (dwPosX, dwPosZ) offset. dwDestPitch is in floats (row stride into the
    // full terrain height buffer).
    bool applyYFactor(std::uint32_t dwDestPitch, const HFIELD_DESC* pHFDesc);

    // Allocate / fill the index buffer for the current face grid. The legacy
    // API takes the IB pool (per (detailLevel, posMask)) but for Phase 5.9a
    // we own the IB locally — that's enough to drive SetVertexColor /
    // ReBuildMesh tests without dragging in the manager's pool.
    void rebuildIndexBuffer(std::uint32_t facesPerX, std::uint32_t facesPerZ);

    Device*                 m_dev = nullptr;
    std::unique_ptr<MeshObject> m_mesh;

    HFIELD_DESC             m_desc{};
    std::uint32_t           m_vertCount   = 0;
    std::uint32_t           m_indexCount  = 0;
    std::uint32_t           m_lodLevel    = 0;
    std::uint32_t           m_posMask     = 0;
    std::uint32_t           m_posX        = 0;
    std::uint32_t           m_posZ        = 0;
    float                   m_distanceFromView = 0.0f;
    bool                    m_mustUpdate  = true;
    bool                    m_hasAlphaMap = false;

    // CPU-side vertex layouts (matches MeshObject::Vertex stride: pos3 + uv2 + normal3 = 32B).
    std::vector<VECTOR3>         m_positions;
    std::vector<VECTOR2>         m_uvs;
    std::vector<VECTOR3>         m_normals;
    std::vector<std::uint16_t>   m_indices;

    // Per-vertex ARGB color (overrides material diffuse if non-zero).
    std::vector<std::uint32_t>   m_colors;

    // 4-way tile blend weights (1 byte each per vertex); alphaMap.size() == 4*vertCount.
    std::vector<std::uint8_t>    m_alphaMap;

    std::uint32_t                m_refCount = 1;
};

} // namespace mxh::gx::dx11
