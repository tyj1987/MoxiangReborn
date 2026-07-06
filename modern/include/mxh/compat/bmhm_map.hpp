// BmhmMap.hpp - Modern .bmhm/.mhm map format parser (skeleton).
//
// Original: 墨香【源码】\4DyuchiGXMapEditor\TileSet.cpp + 4DyuchiFilePack
// Magic header (8 bytes): 7E CB 31 01 2A 00 00 00
//
// This file is a SKELETON: it detects the format and exposes the header,
// but full tile/HField/trigger parsing is left for Phase 1.2.

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mxh::compat {

#pragma pack(push, 1)
struct BmhmHeader {
    std::uint8_t  magic[8];  // {0x7E,0xCB,0x31,0x01,0x2A,0x00,0x00,0x00}
    std::uint32_t version;
    std::uint32_t width;        // tile count X
    std::uint32_t height;       // tile count Z
    std::uint32_t tile_size;    // world units per tile (50)
    std::uint32_t hfield_offset; // offset to height field
    std::uint32_t tile_offset;  // offset to tile table
    std::uint32_t trigger_offset; // offset to NPC/trigger list
    std::uint32_t reserved[4];
};
#pragma pack(pop)

struct BmhmMap {
    BmhmHeader header{};
    std::vector<float> heights;      // height field (width*height)
    std::vector<std::uint8_t> tile_data;  // tile table bytes
    // std::vector<Trigger> triggers;  // TODO(Phase 1.2)

    [[nodiscard]] static bool is_bmhm(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] static BmhmMap parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] static BmhmMap load(const std::filesystem::path& path);
};

}  // namespace mxh::compat