// material.hpp — Material set and material data for CoD3DDeviceDX11.
//
// Each MATERIAL in the original engine describes one sub-material: diffuse /
// reflect / bump texture file names plus lighting properties (diffuse color,
// ambient, specular, transparency, shininess). A MATERIAL_TABLE is a flat array
// of pointers indexed by sub-material ID.
//
// The DX11 implementation loads each named texture as an SRV (via
// Device::createTextureFromFile) and stores them alongside the color/opacity
// data.  The caller receives an opaque void* handle which is simply a pointer
// to the in-process MaterialData; callers must not free it.
#pragma once

// winsock2.h must come before <d3d11.h> / <windows.h>.
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

class Device;

// ---------------------------------------------------------------------------
// Loaded texture + metadata kept by one sub-material.
// ---------------------------------------------------------------------------
struct MaterialTexture {
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    std::uint32_t width  = 0;
    std::uint32_t height = 0;
    bool         loaded  = false;
};

// ---------------------------------------------------------------------------
// One sub-material (one entry in a MATERIAL_TABLE).
// ---------------------------------------------------------------------------
struct MaterialData {
    // Material properties (mirror the INPUT MATERIAL struct so callers can
    // still read dwDiffuse / dwAmbient / dwSpecular / fTransparency / etc.).
    std::uint32_t dwDiffuse       = 0;
    std::uint32_t dwAmbient       = 0;
    std::uint32_t dwSpecular      = 0;
    float         fTransparency   = 0.0f;
    float         fShine          = 0.0f;
    float         fShineStrength  = 0.0f;
    std::uint32_t dwFlag          = 0;

    // Loaded textures.
    MaterialTexture diffuse;   // szDiffuseTexmapFileName
    MaterialTexture reflect;   // szReflectTexmapFileName
    MaterialTexture bump;      // szBumpTexmapFileName

    // Border color for the diffuse texture (set by SetMaterialTextureBorder).
    std::uint32_t borderColor = 0;
};

// ---------------------------------------------------------------------------
// A material set — one CreateMaterialSet call produces one of these.
// dwMtlSetIndex is the user-visible handle returned to callers.
// ---------------------------------------------------------------------------
struct MaterialSet {
    // Sub-materials indexed by dwMtlIndex from each MATERIAL_TABLE entry.
    // Index 0 is reserved (means "no material").
    std::vector<std::unique_ptr<MaterialData>> entries;
};

}  // namespace mxh::gx::dx11
