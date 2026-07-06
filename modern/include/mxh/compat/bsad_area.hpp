// BsadArea.hpp - .bsad battle skill area descriptor parser.
//
// Original: 墨香【源码】\[CC]Skill\SkillArea*.cpp
// Used to describe skill hit zones (e.g. "9x9_Blank", "13x13_Spikewall").
//
// Format (reverse-engineered from skill area files):
//   [u16 width]
//   [u16 height]
//   [u32 reserved]
//   [u8 cells[width*height]]   // cell type: 0=empty, 1=hit, 2=block

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