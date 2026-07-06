// ChrMotion.hpp - .chr motion/animation parser (skeleton).

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mxh::compat {

#pragma pack(push, 1)
struct ChrHeader {
    std::uint32_t magic;          // 'CHRA' or similar
    std::uint32_t version;
    std::uint32_t frame_count;
    std::uint32_t bone_count;
    std::uint32_t fps;
    std::uint32_t reserved[3];
};
#pragma pack(pop)

struct ChrMotion {
    ChrHeader header{};
    std::vector<std::uint8_t> raw;

    [[nodiscard]] static bool is_chr(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] static ChrMotion parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] static ChrMotion load(const std::filesystem::path& path);
};

}  // namespace mxh::compat