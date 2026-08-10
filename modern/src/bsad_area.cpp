// BsadArea.cpp - .bsad skill area parser (MHFile text format).
//
// On-disk layout (see bsad_area.hpp docstring for full description):
//   [12 bytes MHFILE_HEADER {version, type, file_size}]
//   [1 byte CRC1]
//   [file_size bytes encrypted payload]
//
// Payload after MHFile XOR decryption (decrypt_bin_payload) is text:
//     <radius>\r\n
//     <cell00> <cell01> ... <cell0(W-1)>\r\n
//     ...
// where W = H = 2 * radius + 1.

#include "mxh/compat/bsad_area.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <cctype>
#include <cstring>
#include <fstream>
#include <string>

namespace mxh::compat {

namespace {

// Tokenize decrypted text on whitespace (space, tab, \r, \n).
// Returns the next integer value at the cursor position, advancing cursor.
// Returns -1 if no integer can be parsed.
int next_int(const std::uint8_t* data, std::size_t size, std::size_t& cursor) {
    while (cursor < size) {
        const unsigned char c = data[cursor];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++cursor;
            continue;
        }
        if (c < '0' || c > '9') return -1;
        int v = 0;
        while (cursor < size) {
            const unsigned char d = data[cursor];
            if (d < '0' || d > '9') break;
            v = v * 10 + (d - '0');
            ++cursor;
            // Defensive: cap at 3 digits (cells are 0..2).
            if (v > 999) return -1;
        }
        return v;
    }
    return -1;
}

bool is_mhfile_shaped(std::span<const std::uint8_t> bytes) noexcept {
    // Minimum: header(12) + crc1(1) + payload(>=1) = 14 bytes.
    if (bytes.size() < 14) return false;
    MhFileHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    // Real-world bsad files have file_size >= 1, type 1..32, payload fits.
    if (h.file_size == 0) return false;
    if (h.file_size > (1u << 24)) return false;  // 16 MB cap (skill areas are tiny)
    const std::size_t overhead = sizeof(MhFileHeader) + 1;
    if (bytes.size() < overhead + h.file_size) return false;
    return true;
}

}  // namespace

bool BsadArea::is_bsad(std::span<const std::uint8_t> bytes) noexcept {
    return is_mhfile_shaped(bytes);
}

BsadArea BsadArea::parse(std::span<const std::uint8_t> bytes) {
    BsadArea a;
    if (!is_mhfile_shaped(bytes)) return a;

    MhFileHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));

    // Decrypt the payload with the type-aware MHFile algorithm.
    const std::uint8_t* enc = bytes.data() + sizeof(MhFileHeader) + 1;
    auto plain = decrypt_bin_payload({enc, h.file_size}, h.type);

    // Tokenize: first int = radius, then W*H cells where W = H = 2*radius+1.
    std::size_t cursor = 0;
    const int radius = next_int(plain.data(), plain.size(), cursor);
    if (radius < 0 || radius > 32) return a;  // 65x65 max for sane skill areas

    const std::uint32_t dim = static_cast<std::uint32_t>(radius) * 2u + 1u;
    const std::size_t need = static_cast<std::size_t>(dim) * dim;
    a.header.width = static_cast<std::uint16_t>(dim);
    a.header.height = static_cast<std::uint16_t>(dim);
    a.header.reserved = 0;
    a.cells.assign(need, BsadCell::Empty);

    bool any_unparseable = false;
    for (std::size_t i = 0; i < need; ++i) {
        const int v = next_int(plain.data(), plain.size(), cursor);
        if (v < 0 || v > 2) { any_unparseable = true; break; }
        a.cells[i] = static_cast<BsadCell>(v);
    }
    // If tokens ran short, we still return what we have (header reflects radius).
    // any_unparseable currently unused but reserved for stricter validation.
    (void)any_unparseable;
    return a;
}

BsadArea BsadArea::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    if (!f) return {};
    return parse(buf);
}

bool BsadArea::is_hit(std::uint32_t x, std::uint32_t y) const noexcept {
    if (x >= header.width || y >= header.height) return false;
    const auto idx = y * header.width + x;
    if (idx >= cells.size()) return false;
    return cells[idx] == BsadCell::Hit;
}

}  // namespace mxh::compat
