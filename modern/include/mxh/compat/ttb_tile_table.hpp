// TtbTileTable.hpp - .ttb tile table parser (skeleton).
//
// Original: 墨香【源码】\MHMap.cpp + 4DyuchiGXMapEditor\TileView.cpp
// Used alongside .bmhm to look up tile texture indices per cell.

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mxh::compat {

struct TtbTileTable {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint32_t> tiles;  // tile index per cell (row-major)

    [[nodiscard]] static TtbTileTable parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] static TtbTileTable load(const std::filesystem::path& path);
};

}  // namespace mxh::compat