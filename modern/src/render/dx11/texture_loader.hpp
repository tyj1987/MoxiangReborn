// mxh/render/dx11/texture_loader.hpp
// Loads textures from raw bytes (TGA / BMP / bmhm).
// Used by both SpriteObject and Material manager.
#pragma once

#include <cstdint>
#include <vector>

namespace mxh::gx::dx11 {

struct LoadedTexture {
    std::uint32_t        width  = 0;
    std::uint32_t        height = 0;
    std::uint32_t        bps    = 32;
    std::vector<std::uint8_t> pixels;  // RGBA8
};

// Decode TGA (type 2 = uncompressed RGB, type 10 = RLE RGB) to RGBA8.
LoadedTexture loadTGA(const std::uint8_t* data, std::uint32_t size);

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
//   Auto  : auto-select BC3 (DXT5) when the source has alpha gradient,
//           otherwise BC1 (DXT1) — legacy behavior matching the original
//           ConvertCompressedTexture heuristic.
//   BC1   : 4×4 block, 8 B (DXT1). RGB only, no alpha.
//   BC3   : 4×4 block, 16 B (DXT5). RGB (BC1 body) + interpolated alpha.
//   BC4   : 4×4 block, 8 B (ATI1). Single-channel (uses tex.pixels R channel).
//           Typical use: grayscale textures, alpha-only maps.
//   BC5   : 4×4 block, 16 B (ATI2). Two-channel (uses tex.pixels R + G).
//           Typical use: tangent-space normal maps (XY in RG, Z reconstructed
//           in shader as sqrt(1 - x² - y²)).
//
// We emit the legacy DDS_HEADER (124 B) with FourCC = DXT1/DXT5/ATI1/ATI2 so
// the file is readable by every D3D11-era loader that does not require the
// DX10 extension header.
enum class BCFormat { Auto, BC1, BC3, BC4, BC5 };

// Encode a LoadedTexture to a real BC-compressed .DDS file (Phase 5 deferred /
// ConvertCompressedTexture). With `format == BCFormat::Auto`, selects BC3
// (DXT5) when the source has alpha gradient, otherwise BC1 (DXT1). The BC4
// and BC5 variants are useful for normal-map / grayscale texture pipelines.
// All encoders are hand-written 4×4 block compressors with min/max anchor
// endpoints and nearest-palette quantization.
// Returns the encoded bytes on success, empty vector on failure.
std::vector<std::uint8_t> saveDDS_BC(const LoadedTexture& tex, BCFormat format = BCFormat::Auto);

} // namespace mxh::gx::dx11
