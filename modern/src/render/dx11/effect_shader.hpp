// mxh/render/dx11/effect_shader.hpp
// Effect Shader Palette — DX11 implementation.
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include <vector>

#include "mxh/render/render_typedef.hpp"
#include "mxh/render/math.hpp"

namespace mxh::gx::dx11 {

class Device;
class MeshObject;

// One entry in the effect shader palette. Mirrors VLMESH_EFFECT_DESC.
struct EffectEntry {
    BOOL                                                    bDisableSrcTex = FALSE;
    TEXGEN_METHOD                                           method         = TEXGEN_METHOD_REFLECT_SPHEREMAP;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>        srv;
    BOOL                                                    bSuccess = FALSE;
    std::uint32_t                                           dwFlag   = 0;
};

// Manages the global effect shader palette.
class EffectShaderPalette {
public:
    EffectShaderPalette(Device* dev);
    ~EffectShaderPalette();

    // Build palette from an array of CUSTOM_EFFECT_DESC (loads textures).
    bool buildFromDesc(const CUSTOM_EFFECT_DESC* descs, std::uint32_t count);

    // Load palette from a binary file: [DWORD version][DWORD count][CUSTOM_EFFECT_DESC[count]].
    bool loadFromFile(const char* fileName);

    // Free all entries.
    void clear();

    // Access by effect index.
    EffectEntry* getEffect(std::uint32_t index);
    std::uint32_t effectCount() const { return static_cast<std::uint32_t>(m_entries.size()); }

    // Update tick count (for wave animation).
    void setTickCount(std::uint32_t tick) { m_tickCount = tick; }

    // Compute texture matrices (mirrors CoD3DDevice implementation).
    // Sphere-map: matResult = matWorld × matView, scaled ±0.5, translated +0.5.
    void setSphereMapMatrix(MATRIX4* pResultMat, const MATRIX4* pMatWorld, const MATRIX4* pMatView) const;
    // Wave: identity matrix with animated sin UV offsets.
    void setWaveTexMatrix(MATRIX4* pMatResult) const;

private:
    Device*                  m_dev = nullptr;
    std::vector<EffectEntry> m_entries;
    std::uint32_t            m_tickCount = 0;
};

} // namespace mxh::gx::dx11
