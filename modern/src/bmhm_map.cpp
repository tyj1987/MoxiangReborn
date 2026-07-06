// BmhmMap.cpp - Skeleton .bmhm/.mhm parser.

#include "mxh/compat/bmhm_map.hpp"

#include <cstring>
#include <fstream>

namespace mxh::compat {

bool BmhmMap::is_bmhm(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < sizeof(BmhmHeader)) return false;
    static constexpr std::uint8_t kMagic[8] = {0x7E, 0xCB, 0x31, 0x01, 0x2A, 0x00, 0x00, 0x00};
    return std::memcmp(bytes.data(), kMagic, 8) == 0;
}

BmhmMap BmhmMap::parse(std::span<const std::uint8_t> bytes) {
    BmhmMap m;
    if (!is_bmhm(bytes)) return m;
    std::memcpy(&m.header, bytes.data(), sizeof(BmhmHeader));

    // Parse height field (interleaved float32 grid).
    if (m.header.hfield_offset != 0
        && m.header.hfield_offset + sizeof(float) * m.header.width * m.header.height <= bytes.size()) {
        const auto* base = reinterpret_cast<const float*>(
            bytes.data() + m.header.hfield_offset);
        m.heights.assign(base, base + static_cast<std::size_t>(m.header.width) * m.header.height);
    }

    // Parse tile table bytes (TODO: structured decode in Phase 1.2).
    if (m.header.tile_offset != 0
        && m.header.tile_offset < bytes.size()) {
        const auto end = std::min<std::size_t>(
            bytes.size(),
            m.header.tile_offset + 4u * m.header.width * m.header.height);
        m.tile_data.assign(bytes.begin() + m.header.tile_offset, bytes.begin() + end);
    }

    // Trigger/NPC list parsing: TODO(Phase 1.2).
    return m;
}

BmhmMap BmhmMap::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return parse(buf);
}

}  // namespace mxh::compat