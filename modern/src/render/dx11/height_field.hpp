// mxh/render/dx11/height_field.hpp
// IDIHeightField DX11 implementation.
//
// Phase 5.9b adds the IB pool: a (lod, posMask) → D3D11 IB dictionary. Lock/Unlock
// return a CPU pointer (D3D11_MAP_WRITE_DISCARD) that callers fill in. The pool
// doubles as the lookup table for the in-flight map when the GPU side reads.
//
// Also adds real tile-palette loading: LoadTilePalette walks TEXTURE_TABLE file
// paths via texture_loader's auto-detect (TGA, etc.) and stores SRVs.
//
// Excluded from Phase 5.9b (deferred):
//   - Streaming hash tree on top of the pool for fast (lod, posMask) lookup
//     across >256 chunks (we use std::map<std::pair<>> which is fine at the
//     sizes the legacy engine actually used).
//
// Original: 墨香【源码】\4DYUCHIGX_RENDER\HFieldManager.h
//
// Design (Phase 5 simplified):
//   - Reads a height map from HFIELD_DESC (float* pyfList).
//   - Builds a tiled terrain grid: tiles × tiles, each tile = kTileSize world units.
//   - Supports 1-3 LOD levels: full-res, half-res, quarter-res chunks.
//   - Index buffer pool: pre-built IBs per LOD, chunk-addressable.
//   - Tile palette: a TEXTURE_TABLE of tile-diffuse textures, blended at tile edges
//     using a 2×2 blend weight approach in the terrain shader.
//   - Terrain chunk (HeightFieldObject) is backed by a MeshObject internally.
//
// Excluded from Phase 5 (deferred):
//   - Per-vertex color from pwTileTable (tile-type → vertex color mapping)
//   - Alpha blending between adjacent LOD chunks (needs seam geometry)
//   - k-dop shadow volumes per chunk
//   - LOD transition morphing (ROAM-style vertex blending)
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/render_typedef.hpp"

#include <map>

namespace mxh::gx::dx11 {

class Device;
class MeshObject;

// One terrain chunk: LOD-adaptive tile mesh backed by MeshObject.
// A HeightField manages a grid of these chunks, one per LOD level.
class HeightFieldChunk {
public:
    HeightFieldChunk(Device* dev, std::uint32_t lodLevel,
                     std::uint32_t tilesX, std::uint32_t tilesZ,
                     float tileWorldSize, float maxHeight);
    ~HeightFieldChunk();

    void bindDevice(Device* dev) { m_dev = dev; }
    MeshObject* meshObject() const { return m_meshObject.get(); }
    std::uint32_t lodLevel() const { return m_lodLevel; }

    // Rebuild vertex positions from a height map (heightMap is an array of
    // (tilesX * kLODStep + 1) × (tilesZ * kLODStep + 1) floats).
    void updateHeights(const float* heightMap, std::uint32_t heightMapSizeX,
                       std::uint32_t heightMapSizeZ, std::uint32_t lodStep);

    bool buildMesh(std::uint32_t tilesX, std::uint32_t tilesZ,
                   float tileWorldSize, float maxHeight,
                   const float* hm, std::uint32_t hmSizeX,
                   std::uint32_t hmSizeZ, std::uint32_t lodStep);

private:

    Device*                               m_dev = nullptr;
    std::unique_ptr<MeshObject>           m_meshObject;
    std::uint32_t                         m_lodLevel = 0;
    std::vector<float>                    m_heights;   // local copy
};

// IDIHeightField factory: builds terrain from HFIELD_DESC and manages chunks.
class HeightField : public IDIHeightField {
public:
    // kTileWorldSize: terrain tile world-unit size (matches original engine).
    static constexpr float kTileWorldSize = 10.0f;
    // kMaxLodLevels: up to 3 LOD levels (0=full, 1=half, 2=quarter).
    // kMaxLodLevels = 3 is enforced by kLODSteps[] in .cpp
    static constexpr std::uint32_t kMaxLodLevels = 3; // DEPRECATED — do not use

    HeightField(Device* dev);
    ~HeightField();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDIHeightField
    BOOL __stdcall StartInitialize(HFIELD_DESC* pDesc) override;
    void __stdcall EndInitialize() override;
    IDIMeshObject* __stdcall CreateHeightFieldObject(HFIELD_OBJECT_DESC* pDesc) override;
    BOOL __stdcall InitiallizeIndexBufferPool(std::uint32_t dwDetailLevel,
                                              std::uint32_t dwIndicesNum,
                                              std::uint32_t dwNum) override;
    BOOL __stdcall LoadTilePalette(TEXTURE_TABLE* pTexTable,
                                   std::uint32_t dwTileTextureNum) override;
    BOOL __stdcall ReplaceTile(char* szFileName, std::uint32_t dwTexIndex) override;
    BOOL __stdcall CreateIndexBuffer(std::uint32_t dwIndicesNum,
                                    std::uint32_t dwDetailLevel,
                                    std::uint32_t dwPositionMask,
                                    std::uint32_t dwNum) override;
    BOOL __stdcall LockIndexBufferPtr(std::uint16_t** ppWord,
                                     std::uint32_t dwDetailLevel,
                                     std::uint32_t dwPositionMask) override;
    void __stdcall UnlcokIndexBufferPtr(std::uint32_t dwDetailLevel,
                                        std::uint32_t dwPositionMask) override;
    BOOL __stdcall RenderGrid(VECTOR3* pv3Quad, std::uint32_t dwTexTileIndex,
                               std::uint32_t dwAlpha) override;
    void __stdcall SetHFieldTileBlend(BOOL bSwitch) override;
    BOOL __stdcall IsEnableHFieldTileBlend() override;

    // Helpers for renderer.
    void setViewProj(const MATRIX4& vp) { m_viewProj = vp; }
    void renderChunks(const MATRIX4& worldMatrix);

    // Phase 5.9b pool caps, public so tests + callers can size their inputs.
    static constexpr std::uint32_t kMaxPosMasks = 16;        // legacy m_IndexTable[lod][16]
    static constexpr std::uint32_t kMaxLodSlots = 8;         // legacy detail-level cap

private:
    Device*                           m_dev = nullptr;
    HFIELD_DESC                       m_desc{};
    std::vector<float>                m_heightMap;  // local copy
    std::uint32_t                     m_refCount = 1;

    // Per-LOD chunk grids. Each entry is a vector of chunks (one per row).
    std::vector<std::vector<std::unique_ptr<HeightFieldChunk>>> m_chunks;

    // Phase 5.9a: HFieldObjects created via CreateHeightFieldObject. Kept alive
    // past the returned IDIMeshObject* handle so per-chunk CPU state (colors,
    // Y-axis, alpha-map flag) survives as long as the manager does.
    std::vector<std::unique_ptr<class HFieldObject>> m_hFieldObjects;

    // Tile palette: tile textures (SRVs).
    struct TileTexture {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        std::string name;
    };
    std::vector<TileTexture>          m_tileTextures;
    bool                             m_tileBlendEnabled = false;

    // ----- Phase 5.9b: IB pool + mapped pointers -----
    //
    // (lodLevel, posMask) → one PooledIB. Each is a D3D11_USAGE_DYNAMIC IB; Lock
    // returns the CPU pointer for in-place writes, Unmap persists a CPU shadow
    // so the pool stays queryable after the GPU has read the buffer.
    struct PooledIB {
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        std::vector<std::uint16_t>          shadow;        // last CPU-known contents
        std::uint32_t                        capacity = 0;
        bool                                mapped = false;
    };

    using PoolKey = std::pair<std::uint32_t, std::uint32_t>;  // (lod, posMask)
    std::map<PoolKey, PooledIB>   m_ibPool;

    // Helper: get or create a pool entry at (lod, posMask) with given capacity.
    // If it already exists, capacity is increased only if requested > current.
    PooledIB* getOrCreatePoolEntry(std::uint32_t lod, std::uint32_t posMask,
                                    std::uint32_t capacity);
    static std::uint64_t makePoolKey(std::uint32_t lod, std::uint32_t posMask);

    MATRIX4                           m_viewProj = {};
    bool                             m_initialized = false;
};

// Helper: compute height at (worldX, worldZ) from the height map.
float heightAt(const std::vector<float>& hm, std::uint32_t hmSizeX,
               std::uint32_t hmSizeZ, float worldX, float worldZ,
               float worldWidth, float worldDepth, float tileSize);

} // namespace mxh::gx::dx11
