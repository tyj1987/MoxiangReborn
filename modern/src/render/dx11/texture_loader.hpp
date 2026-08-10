// mxh/render/dx11/texture_loader.hpp
// Loads textures from raw bytes (TGA / DDS).
// Used by both SpriteObject and Material manager.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::gx::dx11 {

// The legacy renderer compiled authoring-time .tga references to same-stem
// .dds files before opening the resource pack.
std::string compiledTextureName(std::string_view name);

struct LoadedTexture {
    std::uint32_t        width  = 0;
    std::uint32_t        height = 0;
    std::uint32_t        bps    = 32;
    std::vector<std::uint8_t> pixels;  // RGBA8
};

// Decode TGA (type 2 = uncompressed RGB, type 10 = RLE RGB) to RGBA8.
LoadedTexture loadTGA(const std::uint8_t* data, std::uint32_t size);

// Decode legacy DDS textures used by PlayDH (BGRA/RGBA, DXT1 and DXT5)
// to RGBA8. Only the top mip level is returned.
LoadedTexture loadDDS(const std::uint8_t* data, std::uint32_t size);

// Auto-detect format and decode.
LoadedTexture loadTextureFromMemory(const std::uint8_t* data, std::uint32_t size);

// Encode a LoadedTexture to a TGA (type 2, uncompressed 32-bit BGRA, top-down).
// Returns the encoded bytes on success, empty vector on failure.
std::vector<std::uint8_t> saveTGA(const LoadedTexture& tex);

// Encode a LoadedTexture to an uncompressed .DDS file (legacy DDS_HEADER + raw
// BGRA8 pixels). No actual DXT/BC compression — for the Phase 5 in-process
// build-time tool we accept uncompressed rewrap; real BC1/BC7 needs DirectXTex.
// Returns the encoded bytes on success, empty vector on failure.
std::vector<std::uint8_t> saveDDS(const LoadedTexture& tex);

// BC compression format selector for saveDDS_BC.
//
//   Auto  : auto-select BC7 (DX10) when the source has alpha gradient,
//           otherwise BC1 (DXT1) — legacy behavior matching the original
//           ConvertCompressedTexture heuristic. Auto avoids the heavier
//           BC6H/BC7 DX10-header path when BC1/BC3 are sufficient, but
//           falls back to BC7 for any non-trivial texture so the output
//           is always DX10-header-friendly.
//   BC1   : 4×4 block, 8 B (DXT1). RGB only, no alpha.
//   BC3   : 4×4 block, 16 B (DXT5). RGB (BC1 body) + interpolated alpha.
//   BC4   : 4×4 block, 8 B (ATI1). Single-channel (uses tex.pixels R channel).
//           Typical use: grayscale textures, alpha-only maps.
//   BC5   : 4×4 block, 16 B (ATI2). Two-channel (uses tex.pixels R + G).
//           Typical use: tangent-space normal maps (XY in RG, Z reconstructed
//           in shader as sqrt(1 - x² - y²)).
//   BC6H  : 4×4 block, 16 B (DX10 HDR). RGB half-float, no alpha. Requires
//           the DX10 extended header (dwFourCC = 'DX10', DXGI_FORMAT_BC6H_UFLOAT).
//           Quality is intentionally low (mode 1 only, no per-block mode
//           selection) — see KNOWN_BUGS R-11.
//   BC7   : 4×4 block, 16 B (DX10 LDR). RGB or RGBA. Requires the DX10
//           extended header. Quality is intentionally low (mode 6 only,
//           single partition) — see KNOWN_BUGS R-11.
//
// We emit the legacy DDS_HEADER (124 B) with FourCC = DXT1/DXT5/ATI1/ATI2
// for BC1/3/4/5, or the DX10 extended DDS_HEADER_DXT10 (20 B appended) for
// BC6H/BC7.
enum class BCFormat { Auto, BC1, BC3, BC4, BC5, BC6H, BC7 };

// Encode a LoadedTexture to a real BC-compressed .DDS file. With
// `format == BCFormat::Auto`, selects BC7 (DX10) when the source has
// alpha gradient, otherwise BC1 (DXT1). BC6H/BC7 are always emitted
// via the DX10 extended header path.
// Returns the encoded bytes on success, empty vector on failure.
std::vector<std::uint8_t> saveDDS_BC(const LoadedTexture& tex, BCFormat format = BCFormat::Auto);

} // namespace mxh::gx::dx11
