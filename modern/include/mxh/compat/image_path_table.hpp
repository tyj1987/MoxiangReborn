// mxh/compat/image_path_table.hpp
// 1:1 parser for the legacy image path tables (image_*.bin). The legacy
// client loads six of these:
//   image_hard_path.bin   (PFT_HARDPATH  — hardcoded UI sprites)
//   image_item_path.bin   (PFT_ITEMPATH  — item icons)
//   image_mugong_path.bin (PFT_MUGONGPATH — skill icons)
//   image_ability_path.bin(PFT_ABILITYPATH — ability icons)
//   image_buff_path.bin   (PFT_BUFFPATH — buff icons)
//   image_jackpot_path.bin(PFT_JACKPOTPATH — jackpot icons)
//   image_minimap_path.bin (PFT_MINIMAPPATH — minimap icons)
//
// All share the same layout: each entry is 5 ints (idx left top right
// bottom), space-separated, one record per line. The on-disk format
// is the same MHFile (.bin) encryption used elsewhere — see mh_file_ex.
//
// Source: 墨香【源码】\[Client]MH\interface\cScriptManager.cpp
//   sIMAGHARDPATH { int idx; LONG left, top, right, bottom; }

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mxh::compat {

struct ImagePathEntry {
    std::int32_t index = 0;     // hash key (legacy m_ImageHardPath etc.)
    std::int32_t idx = 0;       // sprite idx (legacy pPath->idx)
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

// Parse the decrypted payload of an image path .bin. The payload is
// whitespace-separated 6-tuples (index idx left top right bottom).
// The first int is the hash key, the second is the sprite idx,
// followed by the source rect (l t r b). Returns an empty vector
// on malformed input.
std::vector<ImagePathEntry> parse_image_path_table(
    std::span<const std::uint8_t> payload);

// Convenience: read + parse a .bin file from disk.
std::vector<ImagePathEntry> read_image_path_table(
    const std::filesystem::path& path);

}  // namespace mxh::compat
