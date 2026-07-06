// BsadArea.cpp - .bsad skill area parser.

#include "mxh/compat/bsad_area.hpp"

#include <cstring>
#include <fstream>

namespace mxh::compat {

bool BsadArea::is_bsad(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < sizeof(BsadHeader) + 1) return false;
    BsadHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    return h.width > 0 && h.width <= 32
        && h.height > 0 && h.height <= 32
        && bytes.size() >= sizeof(BsadHeader) + static_cast<std::size_t>(h.width) * h.height;
}

BsadArea BsadArea::parse(std::span<const std::uint8_t> bytes) {
    BsadArea a;
    if (!is_bsad(bytes)) return a;
    std::memcpy(&a.header, bytes.data(), sizeof(BsadHeader));
    const auto n = static_cast<std::size_t>(a.header.width) * a.header.height;
    a.cells.resize(n);
    std::memcpy(a.cells.data(),
                bytes.data() + sizeof(BsadHeader),
                n * sizeof(BsadCell));
    return a;
}

BsadArea BsadArea::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return parse(buf);
}

bool BsadArea::is_hit(std::uint32_t x, std::uint32_t y) const noexcept {
    if (x >= header.width || y >= header.height) return false;
    const auto idx = y * header.width + x;
    if (idx >= cells.size()) return false;
    return cells[idx] == BsadCell::Hit;
}

}  // namespace mxh::compat