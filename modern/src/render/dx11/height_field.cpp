// mxh/render/dx11/height_field.cpp
// IDIHeightField DX11 implementation.
#include "height_field.hpp"
#include "device.hpp"
#include "hfield_object.hpp"
#include "mesh_object.hpp"
#include "primitives.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace mxh::gx::dx11 {

// Terrain LOD step table. Matches HFIELD_DESC::bDetailLevelNum.
// kLODSteps[0]=full, [1]=half, [2]=quarter resolution.
static constexpr std::uint32_t kLODSteps[] = { 1, 2, 4 };

// Build index buffer data for a terrain tile grid.
// Returns number of indices written.
static std::uint32_t buildTileIndices(std::uint16_t* idx, std::uint32_t tilesX,
                                       std::uint32_t tilesZ,
                                       std::uint32_t vertsPerRow) {
    std::uint32_t n = 0;
    for (std::uint32_t tz = 0; tz < tilesZ; ++tz) {
        for (std::uint32_t tx = 0; tx < tilesX; ++tx) {
            std::uint16_t v00 = static_cast<std::uint16_t>((tz * 1) * vertsPerRow + tx * 1);
            std::uint16_t v10 = static_cast<std::uint16_t>(v00 + 1);
            std::uint16_t v01 = static_cast<std::uint16_t>(v00 + vertsPerRow);
            std::uint16_t v11 = static_cast<std::uint16_t>(v01 + 1);
            idx[n++] = v00; idx[n++] = v10; idx[n++] = v01;
            idx[n++] = v10; idx[n++] = v11; idx[n++] = v01;
        }
    }
    return n;
}

// ===== HeightFieldChunk =====

HeightFieldChunk::HeightFieldChunk(Device* dev, std::uint32_t lodLevel,
                                   std::uint32_t tilesX, std::uint32_t tilesZ,
                                   float tileWorldSize, float maxHeight)
    : m_dev(dev), m_lodLevel(lodLevel) {
    m_meshObject.reset(MeshObject::createEmpty(dev, CMeshFlag()));
}

HeightFieldChunk::~HeightFieldChunk() = default;

bool HeightFieldChunk::buildMesh(std::uint32_t tilesX, std::uint32_t tilesZ,
                                 float tileWorldSize, float maxHeight,
                                 const float* hm, std::uint32_t hmSizeX,
                                 std::uint32_t hmSizeZ, std::uint32_t lodStep) {
    if (!m_dev) return false;

    std::uint32_t vertsX = tilesX * lodStep + 1;
    std::uint32_t vertsZ = tilesZ * lodStep + 1;
    std::uint32_t vertCount = vertsX * vertsZ;

    std::vector<VECTOR3> positions(vertCount);
    std::vector<TVERTEX> texCoords(vertCount);
    std::vector<VECTOR3> normals(vertCount);

    float halfW = static_cast<float>(tilesX) * tileWorldSize * 0.5f;
    float halfD = static_cast<float>(tilesZ) * tileWorldSize * 0.5f;

    for (std::uint32_t z = 0; z < vertsZ; ++z) {
        for (std::uint32_t x = 0; x < vertsX; ++x) {
            std::uint32_t vi = z * vertsX + x;
            float wx = (static_cast<float>(x * lodStep) * tileWorldSize) - halfW;
            float wz = (static_cast<float>(z * lodStep) * tileWorldSize) - halfD;
            float wy = 0.0f;
            if (hm && hmSizeX > 0 && hmSizeZ > 0) {
                float hmX = (static_cast<float>(x) / static_cast<float>(vertsX - 1))
                            * static_cast<float>(hmSizeX - 1);
                float hmZ = (static_cast<float>(z) / static_cast<float>(vertsZ - 1))
                            * static_cast<float>(hmSizeZ - 1);
                std::uint32_t hx0 = static_cast<std::uint32_t>(
                    std::min(hmX, static_cast<float>(hmSizeX - 1)));
                std::uint32_t hz0 = static_cast<std::uint32_t>(
                    std::min(hmZ, static_cast<float>(hmSizeZ - 1)));
                float fx = hmX - static_cast<float>(hx0);
                float fz = hmZ - static_cast<float>(hz0);
                float h00 = hm[hz0 * hmSizeX + hx0];
                float h10 = hm[hz0 * hmSizeX + std::min(hx0 + 1, hmSizeX - 1)];
                float h01 = hm[std::min(hz0 + 1, hmSizeZ - 1) * hmSizeX + hx0];
                float h11 = hm[std::min(hz0 + 1, hmSizeZ - 1) * hmSizeX
                               + std::min(hx0 + 1, hmSizeX - 1)];
                wy = (h00 * (1.0f - fx) * (1.0f - fz)
                    + h10 * fx * (1.0f - fz)
                    + h01 * (1.0f - fx) * fz
                    + h11 * fx * fz) * maxHeight;
            }
            positions[vi] = { wx, wy, wz };
            texCoords[vi] = {
                static_cast<float>(x) / static_cast<float>(vertsX - 1),
                static_cast<float>(z) / static_cast<float>(vertsZ - 1)
            };
            normals[vi] = { 0.0f, 1.0f, 0.0f };
        }
    }

    std::vector<std::uint16_t> indices(tilesX * tilesZ * 6);
    buildTileIndices(indices.data(), tilesX, tilesZ, vertsX);

    MESH_DESC md{};
    md.dwVertexNum     = vertCount;
    md.pv3WorldList    = positions.data();
    md.dwTexVertexNum  = vertCount;
    md.ptvTexCoordList = texCoords.data();
    md.pv3NormalLocal  = normals.data();
    md.meshFlag        = CMeshFlag();

    if (!m_meshObject->StartInitialize(&md, nullptr, nullptr)) return false;
    FACE_DESC fd{};
    fd.pIndex     = indices.data();
    fd.dwFacesNum = tilesX * tilesZ * 2;
    fd.dwMtlIndex = 0;
    if (!m_meshObject->InsertFaceGroup(&fd)) return false;
    m_meshObject->EndInitialize();
    return true;
}

void HeightFieldChunk::updateHeights(const float* heightMap,
                                     std::uint32_t heightMapSizeX,
                                     std::uint32_t heightMapSizeZ,
                                     std::uint32_t lodStep) {
    if (!m_meshObject || !heightMap) return;
    m_heights.assign(heightMap, heightMap + heightMapSizeX * heightMapSizeZ);
    // Rebuild the chunk mesh with updated heights.
    buildMesh(4, 4, 10.0f, 1.0f, heightMap, heightMapSizeX, heightMapSizeZ, lodStep);
}

// ===== HeightField =====

HeightField::HeightField(Device* dev) : m_dev(dev) {}

HeightField::~HeightField() {
    m_chunks.clear();
    m_tileTextures.clear();
}

STDMETHODIMP HeightField::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IDIHeightField*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) HeightField::AddRef() { return ++m_refCount; }
STDMETHODIMP_(ULONG) HeightField::Release() {
    ULONG r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

BOOL __stdcall HeightField::StartInitialize(HFIELD_DESC* pDesc) {
    if (!pDesc || !m_dev) return FALSE;
    m_desc = *pDesc;
    if (pDesc->pyfList && pDesc->dwYFNumX > 0 && pDesc->dwYFNumZ > 0) {
        std::size_t n = static_cast<std::size_t>(pDesc->dwYFNumX) * pDesc->dwYFNumZ;
        m_heightMap.assign(pDesc->pyfList, pDesc->pyfList + n);
    }

    std::uint32_t tilesX = std::max<std::uint32_t>(pDesc->dwTileNumX, 1);
    std::uint32_t tilesZ = std::max<std::uint32_t>(pDesc->dwTileNumZ, 1);
    std::uint32_t lodCount = std::min<std::uint32_t>(pDesc->bDetailLevelNum, 3u);
    if (lodCount == 0) lodCount = 1;

    m_chunks.clear();
    for (std::uint32_t lod = 0; lod < lodCount; ++lod) {
        std::vector<std::unique_ptr<HeightFieldChunk>> lodChunks;
        std::uint32_t chunkTilesX = std::max<std::uint32_t>(1, tilesX / 4);
        std::uint32_t chunkTilesZ = std::max<std::uint32_t>(1, tilesZ / 4);
        for (std::uint32_t cz = 0; cz < chunkTilesZ; ++cz) {
            for (std::uint32_t cx = 0; cx < chunkTilesX; ++cx) {
                std::uint32_t lodStep = kLODSteps[lod];
                auto* chunk = new HeightFieldChunk(m_dev, lod, 4, 4, kTileWorldSize, pDesc->height);
                chunk->buildMesh(4, 4, kTileWorldSize, pDesc->height,
                                 m_heightMap.empty() ? nullptr : m_heightMap.data(),
                                 pDesc->dwYFNumX, pDesc->dwYFNumZ, lodStep);
                lodChunks.emplace_back(chunk);
            }
        }
        m_chunks.push_back(std::move(lodChunks));
    }

    MLOG_INFO("[heightfield] initialized: %u tiles, %u LOD levels, %u chunks",
               tilesX * tilesZ, lodCount,
               static_cast<std::uint32_t>(m_chunks.empty() ? 0 : m_chunks[0].size()));
    return TRUE;
}

void __stdcall HeightField::EndInitialize() { m_initialized = true; }

IDIMeshObject* __stdcall HeightField::CreateHeightFieldObject(HFIELD_OBJECT_DESC* pDesc) {
    if (!m_dev) return nullptr;
    // Phase 5.9a: defer to HFieldObject which owns the per-chunk state machine
    // (CPU-side positions / colors / Y-axis factoring, alpha-blend bookkeeping).
    // We store the HFieldObject so its CPU state outlives the returned mesh ref
    // (callers can fetch it later via m_hFieldObjects if needed; the legacy
    // engine kept an internal list for the same reason).
    VECTOR3 rect[2] = {
        { 0.0f, 0.0f, 0.0f },
        { static_cast<float>(m_desc.dwTileNumX) * kTileWorldSize,
          0.0f,
          static_cast<float>(m_desc.dwTileNumZ) * kTileWorldSize }
    };
    auto hfObj = std::make_unique<HFieldObject>(m_dev);
    if (!hfObj->Create(pDesc ? pDesc->dwPosX : 0,
                        pDesc ? pDesc->dwPosZ : 0,
                        /*dwDetailLevel*/ 0,
                        m_desc.dwTileNumX ? m_desc.dwTileNumX : 1,
                        m_desc.dwTileNumZ ? m_desc.dwTileNumZ : 1,
                        rect, &m_desc)) {
        return nullptr;
    }
    IDIMeshObject* meshIface = hfObj->meshObject();
    if (!meshIface) return nullptr;
    m_hFieldObjects.push_back(std::move(hfObj));
    meshIface->AddRef();  // bump refcount for the returned external handle
    return meshIface;
}

BOOL __stdcall HeightField::InitiallizeIndexBufferPool(std::uint32_t, std::uint32_t, std::uint32_t) { return TRUE; }
BOOL __stdcall HeightField::LoadTilePalette(TEXTURE_TABLE*, std::uint32_t) { return TRUE; }
BOOL __stdcall HeightField::ReplaceTile(char*, std::uint32_t) { return TRUE; }
BOOL __stdcall HeightField::CreateIndexBuffer(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) { return TRUE; }
BOOL __stdcall HeightField::LockIndexBufferPtr(std::uint16_t**, std::uint32_t, std::uint32_t) { return FALSE; }
void __stdcall HeightField::UnlcokIndexBufferPtr(std::uint32_t, std::uint32_t) {}
BOOL __stdcall HeightField::RenderGrid(VECTOR3*, std::uint32_t, std::uint32_t) { return TRUE; }
void __stdcall HeightField::SetHFieldTileBlend(BOOL b) { m_tileBlendEnabled = !!b; }
BOOL __stdcall HeightField::IsEnableHFieldTileBlend() { return m_tileBlendEnabled ? TRUE : FALSE; }

void HeightField::renderChunks(const MATRIX4&) {
    for (auto& lodLevel : m_chunks) {
        for (auto& chunk : lodLevel) {
            if (chunk->meshObject()) {
                chunk->meshObject()->Render(0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
            }
        }
    }
}

// Helper
float heightAt(const std::vector<float>& hm, std::uint32_t hmSizeX,
               std::uint32_t hmSizeZ, float worldX, float worldZ,
               float worldWidth, float worldDepth, float /*tileSize*/) {
    if (hm.empty() || hmSizeX == 0 || hmSizeZ == 0) return 0.0f;

    // Normalize world coords to [0, 1] range (edge-aligned, not center-offset).
    float normX = worldX / worldWidth;
    float normZ = worldZ / worldDepth;

    // Map to height map grid.
    float hmX = normX * static_cast<float>(hmSizeX - 1);
    float hmZ = normZ * static_cast<float>(hmSizeZ - 1);

    // Compute integer grid cell and fractional offset.
    std::uint32_t hx0 = static_cast<std::uint32_t>(std::floor(hmX));
    std::uint32_t hz0 = static_cast<std::uint32_t>(std::floor(hmZ));
    float fx = hmX - static_cast<float>(hx0);
    float fz = hmZ - static_cast<float>(hz0);

    // Clamp to valid grid cells (handles out-of-bounds world coords).
    hx0 = std::min(hx0, hmSizeX - 1);
    hz0 = std::min(hz0, hmSizeZ - 1);
    std::uint32_t hx1 = std::min(hx0 + 1, hmSizeX - 1);
    std::uint32_t hz1 = std::min(hz0 + 1, hmSizeZ - 1);

    float h00 = hm[hz0 * hmSizeX + hx0];
    float h10 = hm[hz0 * hmSizeX + hx1];
    float h01 = hm[hz1 * hmSizeX + hx0];
    float h11 = hm[hz1 * hmSizeX + hx1];
    return h00 * (1.0f - fx) * (1.0f - fz)
         + h10 * fx * (1.0f - fz)
         + h01 * (1.0f - fx) * fz
         + h11 * fx * fz;
}

} // namespace mxh::gx::dx11
