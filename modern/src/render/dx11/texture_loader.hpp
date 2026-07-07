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

} // namespace mxh::gx::dx11
