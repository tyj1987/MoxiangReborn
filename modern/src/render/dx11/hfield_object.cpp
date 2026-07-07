// mxh/render/dx11/hfield_object.cpp
// IDIHFieldObject DX11 implementation. See hfield_object.hpp for design notes.
#include "hfield_object.hpp"

#include "device.hpp"
#include "mesh_object.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace mxh::gx::dx11 {

namespace {

// Build index buffer data for a (facesX × facesZ) grid of CW triangles with
// (facesX+1) × (facesZ+1) verts arranged row-by-row.
std::uint32_t buildGridIndicesCCW(std::vector<std::uint16_t>& out,
                                  std::uint32_t facesX, std::uint32_t facesZ,
                                  std::uint32_t vertsPerRow) {
    out.resize(static_cast<std::size_t>(facesX) * facesZ * 6);
    std::uint32_t n = 0;
    for (std::uint32_t tz = 0; tz < facesZ; ++tz) {
        for (std::uint32_t tx = 0; tx < facesX; ++tx) {
            std::uint16_t v00 = static_cast<std::uint16_t>(tz * vertsPerRow + tx);
            std::uint16_t v10 = static_cast<std::uint16_t>(v00 + 1);
            std::uint16_t v01 = static_cast<std::uint16_t>(v00 + vertsPerRow);
            std::uint16_t v11 = static_cast<std::uint16_t>(v01 + 1);
            // CCW from above (Y up): v00 → v10 → v01; v10 → v11 → v01.
            out[n++] = v00; out[n++] = v10; out[n++] = v01;
            out[n++] = v10; out[n++] = v11; out[n++] = v01;
        }
    }
    return n;
}

} // namespace

HFieldObject::HFieldObject(Device* dev) : m_dev(dev) {}

HFieldObject::~HFieldObject() = default;

STDMETHODIMP HFieldObject::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IDIHFieldObject*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) HFieldObject::AddRef() { return ++m_refCount; }
STDMETHODIMP_(ULONG) HFieldObject::Release() {
    ULONG r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

BOOL __stdcall HFieldObject::Create(std::uint32_t dwPosX, std::uint32_t dwPosZ,
                                    std::uint32_t dwDetailLevel,
                                    std::uint32_t dwFacesNumPerX,
                                    std::uint32_t dwFacesNumPerZ,
                                    VECTOR3* pv3Rect, HFIELD_DESC* pHFDesc) {
    if (!m_dev || !pHFDesc) return FALSE;
    if (dwFacesNumPerX == 0 || dwFacesNumPerZ == 0) return FALSE;

    m_desc = *pHFDesc;
    m_posX  = dwPosX;
    m_posZ  = dwPosZ;
    m_lodLevel = dwDetailLevel;
    m_posMask  = 0;

    const std::uint32_t vertsX = dwFacesNumPerX + 1;
    const std::uint32_t vertsZ = dwFacesNumPerZ + 1;
    m_vertCount  = vertsX * vertsZ;
    m_indexCount = dwFacesNumPerX * dwFacesNumPerZ * 6;

    // World XZ bounds come from pv3Rect (legacy passes 2 plane corners).
    // Index 0/1 = bottom-left X/Z, Index 2/3 = top-right X/Z.
    float x0 = (pv3Rect != nullptr) ? pv3Rect[0].x : 0.0f;
    float z0 = (pv3Rect != nullptr) ? pv3Rect[0].z : 0.0f;
    float x1 = (pv3Rect != nullptr) ? pv3Rect[1].x : static_cast<float>(dwFacesNumPerX);
    float z1 = (pv3Rect != nullptr) ? pv3Rect[1].z : static_cast<float>(dwFacesNumPerZ);
    if (x1 <= x0) x1 = x0 + static_cast<float>(dwFacesNumPerX);
    if (z1 <= z0) z1 = z0 + static_cast<float>(dwFacesNumPerZ);

    const float stepX = (x1 - x0) / static_cast<float>(dwFacesNumPerX);
    const float stepZ = (z1 - z0) / static_cast<float>(dwFacesNumPerZ);

    m_positions.assign(m_vertCount, { 0.0f, 0.0f, 0.0f });
    m_uvs.assign(m_vertCount, { 0.0f, 0.0f });
    m_normals.assign(m_vertCount, { 0.0f, 1.0f, 0.0f });
    m_colors.assign(m_vertCount, 0);
    m_alphaMap.assign(m_vertCount * 4, 0);  // 4 tile weights, default 0 (use tile 0)
    for (std::uint32_t i = 0; i < m_vertCount; ++i) {
        m_alphaMap[i * 4 + 0] = 255;       // tile 0 fully opaque by default
    }

    for (std::uint32_t z = 0; z < vertsZ; ++z) {
        for (std::uint32_t x = 0; x < vertsX; ++x) {
            const std::uint32_t vi = z * vertsX + x;
            m_positions[vi].x = x0 + static_cast<float>(x) * stepX;
            m_positions[vi].y = 0.0f;       // Y is filled by applyYFactor below
            m_positions[vi].z = z0 + static_cast<float>(z) * stepZ;
            m_uvs[vi].x = static_cast<float>(x) / static_cast<float>(vertsX - 1);
            m_uvs[vi].y = static_cast<float>(z) / static_cast<float>(vertsZ - 1);
        }
    }

    rebuildIndexBuffer(dwFacesNumPerX, dwFacesNumPerZ);

    if (!applyYFactor(pHFDesc->dwYFNumX, pHFDesc)) {
        MLOG_WARN("[hfield] Create: YFactor apply failed for chunk (%u,%u)", dwPosX, dwPosZ);
    }
    if (!rebuildMeshObjectFromCPU()) {
        MLOG_ERROR("[hfield] Create: mesh upload failed for chunk (%u,%u)", dwPosX, dwPosZ);
        return FALSE;
    }
    MLOG_DEBUG("[hfield] chunk (%u,%u) LOD%u: %u verts, %u indices",
               dwPosX, dwPosZ, dwDetailLevel, m_vertCount, m_indexCount);
    return TRUE;
}

BOOL __stdcall HFieldObject::SetYFactor(std::uint32_t dwDestPitch, HFIELD_DESC* pHFDesc) {
    if (!pHFDesc || !applyYFactor(dwDestPitch, pHFDesc)) return FALSE;
    m_mustUpdate = true;
    return rebuildMeshObjectFromCPU();
}

BOOL __stdcall HFieldObject::SetVertexColor(std::uint32_t* pdwColor, std::uint32_t dwVerticesNum) {
    if (!pdwColor) return FALSE;
    const std::uint32_t n = std::min<std::uint32_t>(dwVerticesNum, m_vertCount);
    std::memcpy(m_colors.data(), pdwColor, n * sizeof(std::uint32_t));
    if (n < m_vertCount) {
        std::fill(m_colors.begin() + n, m_colors.end(), 0u);
    }
    return TRUE;
}

BOOL __stdcall HFieldObject::SetVertexColorAll(std::uint32_t dwColor) {
    std::fill(m_colors.begin(), m_colors.end(), dwColor);
    return TRUE;
}

BOOL __stdcall HFieldObject::SetDetailLevel(std::uint32_t dwDetailLevel) {
    m_lodLevel = dwDetailLevel;
    m_mustUpdate = true;
    return TRUE;
}

void __stdcall HFieldObject::SetDistanceFromViewPoint(float fDistance) {
    m_distanceFromView = fDistance;
}

void __stdcall HFieldObject::SetPositionMask(std::uint32_t dwPosMask) {
    m_posMask = dwPosMask;
}

BOOL __stdcall HFieldObject::ReBuildMesh(std::uint32_t dwDestPitch, HFIELD_DESC* pHFDesc,
                                         std::uint32_t* pdwColor) {
    if (!pHFDesc) return FALSE;
    if (!applyYFactor(dwDestPitch, pHFDesc)) return FALSE;
    if (pdwColor) {
        const std::uint32_t n = std::min<std::uint32_t>(m_vertCount, m_vertCount);
        std::memcpy(m_colors.data(), pdwColor, n * sizeof(std::uint32_t));
    }
    m_mustUpdate = true;
    return rebuildMeshObjectFromCPU();
}

void __stdcall HFieldObject::SetMustUpdate() { m_mustUpdate = true; }

BOOL __stdcall HFieldObject::UpdateAlphaMap(TILE_BUFFER_DESC* pTileBufferDesc) {
    if (!pTileBufferDesc) return FALSE;
    m_hasAlphaMap = true;
    MLOG_DEBUG("[hfield] UpdateAlphaMap tile=%u integrated=%u (Phase 5.9a: weights are placeholders)",
               pTileBufferDesc->wTileIndexIntegrated,
               pTileBufferDesc->wTileIndexIntegrated);
    return TRUE;
}

void __stdcall HFieldObject::CleanupAlphaMap() {
    m_hasAlphaMap = false;
    m_alphaMap.assign(m_alphaMap.size(), 0);
    if (!m_alphaMap.empty()) m_alphaMap[0] = 255;
}

void HFieldObject::rebuildIndexBuffer(std::uint32_t facesPerX, std::uint32_t facesPerZ) {
    const std::uint32_t vertsPerRow = facesPerX + 1;
    buildGridIndicesCCW(m_indices, facesPerX, facesPerZ, vertsPerRow);
    m_indexCount = static_cast<std::uint32_t>(m_indices.size());
}

bool HFieldObject::applyYFactor(std::uint32_t dwDestPitch, const HFIELD_DESC* pHFDesc) {
    if (!pHFDesc) return false;
    if (!pHFDesc->pyfList || pHFDesc->dwYFNumX == 0 || pHFDesc->dwYFNumZ == 0) {
        return true;  // No height data → leave Y at 0 (flat plane).
    }
    const std::uint32_t yfW = pHFDesc->dwYFNumX;
    const std::uint32_t yfH = pHFDesc->dwYFNumZ;
    const std::uint32_t pitch = (dwDestPitch != 0) ? dwDestPitch : yfW;

    const std::uint32_t vertsX = m_positions.empty() ? 0u
        : static_cast<std::uint32_t>(std::sqrt(static_cast<double>(m_positions.size())));

    // We assume positions are row-major square (vertsX == vertsZ). If dwPosX/
    // dwPosZ are given, we sample the corresponding tile from the height map.
    const float heightScale = pHFDesc->height;

    for (std::uint32_t i = 0; i < m_vertCount; ++i) {
        // Map chunk-local vertex (i) to a sample in the global height map.
        // Stratified sample: walk the chunk row/col indices.
        const std::uint32_t localX = i % vertsX;
        const std::uint32_t localZ = i / vertsX;

        // The chunk occupies a sub-region of the height map. Without explicit
        // grid-per-chunk we sample on a regular grid relative to (dwPosX, dwPosZ).
        std::uint32_t gx = (m_posX + localX);
        std::uint32_t gz = (m_posZ + localZ);
        if (gx >= yfW) gx = yfW - 1;
        if (gz >= yfH) gz = yfH - 1;

        const std::uint32_t sampleIdx = gz * pitch + gx;
        if (sampleIdx >= static_cast<std::uint32_t>(yfW) * yfH) {
            m_positions[i].y = 0.0f;
            continue;
        }
        const float h = pHFDesc->pyfList[sampleIdx];
        m_positions[i].y = h * heightScale;
    }
    return true;
}

bool HFieldObject::rebuildMeshObjectFromCPU() {
    if (m_vertCount == 0 || m_positions.empty()) return false;

    if (!m_mesh) {
        m_mesh.reset(MeshObject::createEmpty(m_dev, CMeshFlag()));
    }
    if (!m_mesh) return false;

    // Convert our internal layouts (VECTOR3 + VECTOR2 + VECTOR3) into
    // MeshObject::Vertex-stride arrays. The MESH_DESC pointer fields expect
    // three separate arrays (positions, uvs, normals) so we copy into fresh
    // std::vectors and pass their .data() pointers.
    struct TriVert { float x, y, z, u, v, nx, ny, nz; };
    static_assert(sizeof(TriVert) == 32, "TriVert stride must match MeshObject::Vertex (32B)");

    MESH_DESC md{};
    md.dwVertexNum    = m_vertCount;

    std::vector<VECTOR3> pos(m_vertCount), nrm(m_vertCount);
    std::vector<TVERTEX> uv(m_vertCount);
    for (std::uint32_t i = 0; i < m_vertCount; ++i) {
        pos[i]   = m_positions[i];
        uv[i].u  = m_uvs[i].x;
        uv[i].v  = m_uvs[i].y;
        nrm[i]   = m_normals[i];
    }
    md.pv3WorldList   = pos.data();
    md.dwTexVertexNum = m_vertCount;
    md.ptvTexCoordList = uv.data();
    md.pv3NormalLocal = nrm.data();

    if (!m_mesh->StartInitialize(&md, nullptr, nullptr)) {
        MLOG_ERROR("[hfield] mesh StartInitialize failed");
        return false;
    }

    FACE_DESC fd{};
    fd.pIndex     = m_indices.empty() ? nullptr : m_indices.data();
    fd.dwFacesNum = (m_indexCount > 0) ? (m_indexCount / 3) : 0;
    fd.dwMtlIndex = 0;
    if (!m_mesh->InsertFaceGroup(&fd)) {
        MLOG_ERROR("[hfield] mesh InsertFaceGroup failed");
        return false;
    }
    m_mesh->EndInitialize();
    m_mustUpdate = false;
    return true;
}

} // namespace mxh::gx::dx11
