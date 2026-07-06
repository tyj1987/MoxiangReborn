// ChrMotion.cpp - Skeleton .chr parser.

#include "mxh/compat/chr_motion.hpp"

#include <cstring>
#include <fstream>

namespace mxh::compat {

bool ChrMotion::is_chr(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < sizeof(ChrHeader)) return false;
    ChrHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    return h.version >= 1 && h.version <= 10
        && h.frame_count > 0 && h.frame_count < 1'000'000
        && h.fps > 0 && h.fps < 240;
}

ChrMotion ChrMotion::parse(std::span<const std::uint8_t> bytes) {
    ChrMotion m;
    if (!is_chr(bytes)) return m;
    m.raw.assign(bytes.begin(), bytes.end());
    std::memcpy(&m.header, bytes.data(), sizeof(ChrHeader));
    // TODO(Phase 1.3): decode bone tracks.
    return m;
}

ChrMotion ChrMotion::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return parse(buf);
}

}  // namespace mxh::compat