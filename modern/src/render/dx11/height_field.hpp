// mxh/render/dx11/height_field.hpp
// IDIHeightField DX11 implementation.
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

private:
    Device*                           m_dev = nullptr;
    HFIELD_DESC                       m_desc{};
    std::vector<float>                m_heightMap;  // local copy
    std::uint32_t                     m_refCount = 1;

    // Per-LOD chunk grids. Each entry is a vector of chunks (one per row).
    std::vector<std::vector<std::unique_ptr<HeightFieldChunk>>> m_chunks;

    // Tile palette: tile textures (SRVs).
    struct TileTexture {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        std::string name;
    };
    std::vector<TileTexture>          m_tileTextures;
    bool                             m_tileBlendEnabled = false;

    MATRIX4                           m_viewProj = {};
    bool                             m_initialized = false;
};

// Helper: compute height at (worldX, worldZ) from the height map.
float heightAt(const std::vector<float>& hm, std::uint32_t hmSizeX,
               std::uint32_t hmSizeZ, float worldX, float worldZ,
               float worldWidth, float worldDepth, float tileSize);

} // namespace mxh::gx::dx11
