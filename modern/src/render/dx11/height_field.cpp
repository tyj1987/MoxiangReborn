// mxh/render/dx11/height_field.cpp
// IDIHeightField DX11 implementation.
#include "height_field.hpp"
#include "device.hpp"
#include "hfield_object.hpp"
#include "mesh_object.hpp"
#include "primitives.hpp"
#include "texture_loader.hpp"

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
    m_hFieldObjects.clear();
    m_tileTextures.clear();

    // Unmap any still-mapped IBs to keep D3D11 happy at teardown.
    if (m_dev && m_dev->rawContext()) {
        for (auto& kv : m_ibPool) {
            if (kv.second.mapped && kv.second.buffer) {
                m_dev->rawContext()->Unmap(kv.second.buffer.Get(), 0);
                kv.second.mapped = false;
            }
        }
    }
    m_ibPool.clear();
}

std::uint64_t HeightField::makePoolKey(std::uint32_t lod, std::uint32_t posMask) {
    return (static_cast<std::uint64_t>(lod) << 32) | posMask;
}

HeightField::PooledIB* HeightField::getOrCreatePoolEntry(std::uint32_t lod,
                                                        std::uint32_t posMask,
                                                        std::uint32_t capacity) {
    if (!m_dev || posMask >= kMaxPosMasks) return nullptr;
    auto it = m_ibPool.find({ lod, posMask });
    if (it != m_ibPool.end()) {
        if (capacity > it->second.capacity) {
            // Reallocate: mark unmapped + resize the GPU buffer + the shadow.
            if (it->second.mapped && m_dev->rawContext() && it->second.buffer) {
                m_dev->rawContext()->Unmap(it->second.buffer.Get(), 0);
                it->second.mapped = false;
            }
            D3D11_BUFFER_DESC bd{};
            bd.ByteWidth      = capacity * sizeof(std::uint16_t);
            bd.Usage          = D3D11_USAGE_DYNAMIC;
            bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(m_dev->rawDevice()->CreateBuffer(&bd, nullptr,
                                                          it->second.buffer.GetAddressOf()))) {
                m_ibPool.erase(it);
                return nullptr;
            }
            it->second.capacity = capacity;
            it->second.shadow.assign(capacity, 0);
        }
        return &it->second;
    }

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth      = capacity * sizeof(std::uint16_t);
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    PooledIB entry;
    entry.capacity = capacity;
    entry.shadow.assign(capacity, 0);
    if (FAILED(m_dev->rawDevice()->CreateBuffer(&bd, nullptr,
                                                  entry.buffer.GetAddressOf()))) {
        return nullptr;
    }
    auto [insertedIt, _] = m_ibPool.emplace(PoolKey{ lod, posMask }, std::move(entry));
    return &insertedIt->second;
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

BOOL __stdcall HeightField::InitiallizeIndexBufferPool(std::uint32_t dwDetailLevel,
                                                       std::uint32_t dwIndicesNum,
                                                       std::uint32_t dwNum) {
    if (!m_dev || dwDetailLevel >= kMaxLodSlots) return FALSE;
    if (dwIndicesNum == 0 || dwNum == 0) return FALSE;

    // Pre-allocate `dwNum` IBs at posMask 0..dwNum-1 (consecutive posMask range).
    for (std::uint32_t pm = 0; pm < dwNum; ++pm) {
        if (pm >= kMaxPosMasks) break;
        PooledIB* entry = getOrCreatePoolEntry(dwDetailLevel, pm, dwIndicesNum);
        if (!entry) return FALSE;
    }
    MLOG_DEBUG("[heightfield] InitiallizeIndexBufferPool: lod=%u indices=%u num=%u",
               dwDetailLevel, dwIndicesNum, dwNum);
    return TRUE;
}

BOOL __stdcall HeightField::CreateIndexBuffer(std::uint32_t dwIndicesNum,
                                              std::uint32_t dwDetailLevel,
                                              std::uint32_t dwPosMask,
                                              std::uint32_t dwNum) {
    if (!m_dev || dwDetailLevel >= kMaxLodSlots || dwPosMask >= kMaxPosMasks) return FALSE;
    if (dwIndicesNum == 0 || dwNum == 0) return FALSE;
    for (std::uint32_t i = 0; i < dwNum; ++i) {
        std::uint32_t pm = dwPosMask + i;
        if (pm >= kMaxPosMasks) break;
        PooledIB* entry = getOrCreatePoolEntry(dwDetailLevel, pm, dwIndicesNum);
        if (!entry) return FALSE;
    }
    return TRUE;
}

BOOL __stdcall HeightField::LockIndexBufferPtr(std::uint16_t** ppWord,
                                                std::uint32_t dwDetailLevel,
                                                std::uint32_t dwPosMask) {
    if (!ppWord || dwDetailLevel >= kMaxLodSlots || dwPosMask >= kMaxPosMasks) return FALSE;
    auto it = m_ibPool.find({ dwDetailLevel, dwPosMask });
    if (it == m_ibPool.end()) return FALSE;
    PooledIB& entry = it->second;
    if (!entry.buffer || !m_dev) return FALSE;
    auto* ctx = m_dev->rawContext();
    if (!ctx) return FALSE;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(entry.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return FALSE;
    }
    entry.mapped = true;
    // Caller writes into the returned pointer; we mirror to shadow on Unmap.
    *ppWord = static_cast<std::uint16_t*>(mapped.pData);
    return TRUE;
}

void __stdcall HeightField::UnlcokIndexBufferPtr(std::uint32_t dwDetailLevel,
                                                 std::uint32_t dwPosMask) {
    auto it = m_ibPool.find({ dwDetailLevel, dwPosMask });
    if (it == m_ibPool.end()) return;
    PooledIB& entry = it->second;
    if (!entry.mapped || !entry.buffer || !m_dev) return;
    auto* ctx = m_dev->rawContext();
    if (!ctx) return;
    // Copy current GPU contents into the CPU shadow before unmap (so subsequent
    // CPU queries see consistent state if the caller didn't fill the whole buffer).
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(entry.buffer.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        const std::size_t byteCount = static_cast<std::size_t>(entry.capacity) * sizeof(std::uint16_t);
        if (entry.shadow.size() < entry.capacity) entry.shadow.resize(entry.capacity);
        std::memcpy(entry.shadow.data(), mapped.pData, byteCount);
        ctx->Unmap(entry.buffer.Get(), 0);
        entry.mapped = false;
    }
}

BOOL __stdcall HeightField::LoadTilePalette(TEXTURE_TABLE* pTexTable,
                                            std::uint32_t dwTileTextureNum) {
    if (!m_dev || !pTexTable || dwTileTextureNum == 0) return FALSE;
    m_tileTextures.clear();
    m_tileTextures.reserve(dwTileTextureNum);
    for (std::uint32_t i = 0; i < dwTileTextureNum; ++i) {
        TileTexture tile;
        const char* path = pTexTable[i].szTextureName;     // matches legacy TEXTURE_TABLE::szTextureName
        if (path != nullptr && path[0] != '\0') {
            tile.name = path;
            tile.srv = m_dev->createTextureFromFile(path);
        }
        m_tileTextures.push_back(std::move(tile));
    }
    MLOG_DEBUG("[heightfield] LoadTilePalette: %u tiles (loaded %zu with SRV)",
               dwTileTextureNum,
               std::count_if(m_tileTextures.begin(), m_tileTextures.end(),
                             [](const TileTexture& t) { return t.srv != nullptr; }));
    return TRUE;
}

BOOL __stdcall HeightField::ReplaceTile(char* szFileName, std::uint32_t dwTexIndex) {
    if (!m_dev || !szFileName || dwTexIndex >= m_tileTextures.size()) return FALSE;
    m_tileTextures[dwTexIndex].name = szFileName;
    auto srv = m_dev->createTextureFromFile(szFileName);
    if (!srv) {
        MLOG_WARN("[heightfield] ReplaceTile[%u]: failed to load '%s'", dwTexIndex, szFileName);
        return FALSE;
    }
    m_tileTextures[dwTexIndex].srv = srv;
    return TRUE;
}
BOOL __stdcall HeightField::RenderGrid(VECTOR3* pv3Quad, std::uint32_t dwTexTileIndex,
                                      std::uint32_t dwAlpha) {
    // Phase 5.9c: turn the legacy "render this tile quad" debug path into a
    // real call. We don't have a CPU-side D3D11 context here (RenderGrid is
    // a public COM entry point), so this method can't actually submit draw
    // calls on its own — that's renderChunks()'s job, called by the
    // renderer. What we *can* do reliably on the CPU side is:
    //   1. Reject obviously broken inputs (null quad, post-init check)
    //   2. Record the requested tile + alpha (legacy semantics: "the active
    //      tile is N with alpha A until told otherwise")
    //   3. Walk the chunk grid and re-apply alpha to every loaded chunk
    //      so the next renderChunks() picks up the new value
    //   4. Bump the per-call counter so debug overlays can see the activity
    if (!pv3Quad) {
        MLOG_WARN("[hfield] RenderGrid: null quad rejected");
        return FALSE;
    }
    if (!m_initialized) {
        MLOG_WARN("[hfield] RenderGrid: not initialized");
        return FALSE;
    }
    if (dwTexTileIndex >= m_tileTextures.size() && !m_tileTextures.empty()) {
        MLOG_WARN("[hfield] RenderGrid: tile index %u out of range (palette size=%zu)",
                  dwTexTileIndex, m_tileTextures.size());
        return FALSE;
    }
    m_lastRenderGridTile  = dwTexTileIndex;
    m_lastRenderGridAlpha = dwAlpha;
    ++m_renderGridCount;
    // Walk the chunks and re-issue Render() with the per-call alpha. This
    // is the closest CPU-side analog to "the active tile is N with alpha
    // A" without a real D3D11 device to submit draw calls on. Production
    // callers should still go through the renderer's per-frame path; this
    // method is the legacy "preview one tile" debug entry point.
    for (auto& lodLevel : m_chunks) {
        for (auto& chunk : lodLevel) {
            if (chunk && chunk->meshObject()) {
                // Render() takes a per-call alpha. We pass 0 for ref index,
                // null light lists, no spot lights, no material set, no
                // effect — same shape as renderChunks() uses.
                chunk->meshObject()->Render(0, dwAlpha, nullptr, 0, nullptr, 0,
                                            0, 0, 0);
            }
        }
    }
    MLOG_DEBUG("[hfield] RenderGrid: quad=(%.1f,%.1f,%.1f)..(%.1f,%.1f,%.1f) tile=%u alpha=%u count=%llu",
               pv3Quad[0].x, pv3Quad[0].y, pv3Quad[0].z,
               pv3Quad[3].x, pv3Quad[3].y, pv3Quad[3].z,
               dwTexTileIndex, dwAlpha,
               static_cast<unsigned long long>(m_renderGridCount));
    return TRUE;
}
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
