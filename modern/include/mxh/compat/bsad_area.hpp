// BsadArea.hpp - .bsad battle skill area descriptor parser.
//
// Original: [CC]Skill/SkillArea*.cpp
// Used to describe skill hit zones (e.g. "9x9_Blank", "13x13_Spikewall").
//
// ACTUAL on-disk format (reverse-engineered from SkillAreaManager.cpp + real
// files): the .bsad file is a standard Moxiang MHFile .bin (12-byte header +
// 1-byte CRC + encrypted payload). After the standard MHFile XOR decryption
// (decrypt_bin_payload), the payload is a TEXT payload laid out as:
//
//     <radius>\r\n
//     <cell00> <cell01> ... <cell0(W-1)>\r\n
//     ...
//     <cell(H-1)0> ... <cell(H-1)(W-1)>\r\n
//
// where W = H = 2 * radius + 1, and each <cellXY> is "0"|"1"|"2"
// (consumed via legacy pFile->GetByte() -> atoi(GetString())). Cell types:
//   0 = Empty, 1 = Hit, 2 = Block.
//
// We retain a 4-byte legacy BsadHeader (width/height/reserved) on the parsed
// in-memory struct so downstream consumers (game logic, debug viz) can still
// use header.width/height, but on-disk it's purely MHFile.

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mxh::compat {

#pragma pack(push, 1)
struct BsadHeader {
    std::uint16_t width;
    std::uint16_t height;
    std::uint32_t reserved;
};
#pragma pack(pop)

enum class BsadCell : std::uint8_t {
    Empty = 0,
    Hit   = 1,
    Block = 2,
};

struct BsadArea {
    BsadHeader header{};
    std::vector<BsadCell> cells;  // row-major: width * height

    [[nodiscard]] static bool is_bsad(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] static BsadArea parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] static BsadArea load(const std::filesystem::path& path);

    [[nodiscard]] bool is_hit(std::uint32_t x, std::uint32_t y) const noexcept;
};

}  // namespace mxh::compat
