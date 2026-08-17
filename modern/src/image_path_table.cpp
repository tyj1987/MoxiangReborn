// mxh/compat/image_path_table.cpp
// Implementation of the legacy image path table parser.

#include "mxh/compat/image_path_table.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace mxh::compat {

std::vector<ImagePathEntry> parse_image_path_table(
    std::span<const std::uint8_t> payload) {
    std::vector<ImagePathEntry> out;
    if (payload.empty()) return out;

    // The decrypted payload is ASCII text: whitespace-separated integers
    // in groups of 5 (idx left top right bottom). The legacy MHFile::GetInt()
    // uses atoi() which silently returns 0 for non-numeric tokens — mirror
    // that behavior here via strtol.
    std::string text(reinterpret_cast<const char*>(payload.data()),
                     payload.size());

    auto parse_int = [&](const char*& p) -> std::int32_t {
        // Skip whitespace.
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
        if (!*p) {
            return 0;  // end-of-buffer: legacy atoi("") == 0
        }
        char* end = nullptr;
        long v = std::strtol(p, &end, 10);
        if (end == p) {
            // No digits consumed — legacy returns 0.
            // But we still need to advance past the non-digit so we don't
            // loop forever on the same character.
            ++p;
            return 0;
        }
        p = end;
        return static_cast<std::int32_t>(v);
    };

    const char* p = text.data();
    const char* end = text.data() + text.size();
    while (p < end) {
        ImagePathEntry e;
        e.index   = parse_int(p);  // hash key (e.g. m_ImageHardPath index)
        e.idx     = parse_int(p);  // sprite index
        e.left    = parse_int(p);
        e.top     = parse_int(p);
        e.right   = parse_int(p);
        e.bottom  = parse_int(p);
        // Stop on the first all-zero sentinel record (legacy tables often
        // terminate with a 0,0,0,0,0,0 row).
        if (e.index == 0 && e.idx == 0 && e.left == 0 && e.top == 0 &&
            e.right == 0 && e.bottom == 0) {
            if (!out.empty()) break;
        }
        out.push_back(e);
        if (p >= end) break;
    }
    return out;
}

std::vector<ImagePathEntry> read_image_path_table(
    const std::filesystem::path& path) {
    auto read = read_mh_bin(path);
    if (!read.ok()) return {};
    return parse_image_path_table(read.value.data);
}

}  // namespace mxh::compat
