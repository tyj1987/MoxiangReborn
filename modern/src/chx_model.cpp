// ChxModel.cpp - Skeleton .chx parser.

#include "mxh/compat/chx_model.hpp"

#include <cstring>
#include <fstream>

namespace mxh::compat {

bool ChxModel::is_chx(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < sizeof(ChxHeader)) return false;
    ChxHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    // Loose magic check: many region-specific versions exist.
    // Accept if version is plausible (1-10) and counts are non-zero & sane.
    return h.version >= 1 && h.version <= 10
        && h.vertex_count > 0 && h.vertex_count < 10'000'000
        && h.index_count  > 0 && h.index_count  < 50'000'000;
}

ChxModel ChxModel::parse(std::span<const std::uint8_t> bytes) {
    ChxModel m;
    if (!is_chx(bytes)) return m;
    m.raw.assign(bytes.begin(), bytes.end());
    std::memcpy(&m.header, bytes.data(), sizeof(ChxHeader));
    // TODO(Phase 1.3): decode vertex/index/mesh/bone tables.
    return m;
}

ChxModel ChxModel::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return parse(buf);
}

}  // namespace mxh::compat