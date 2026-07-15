// text_parse.hpp - Shared inline text-parsing helpers.
//
// Houses the trim() / tokenize() / BOM-stripping primitives used by
// multiple .mhm / .chr / .chx plain-text parsers. Defined `inline` in
// the header so each TU can include this without ODR violation. Keep
// the bodies tiny — anything non-trivial belongs in a .cpp.

#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::compat::detail {

// Strip UTF-8 BOM (3 bytes: EF BB BF) if present, then trim leading
// and trailing ASCII whitespace.
[[nodiscard]] inline std::string_view trim(std::string_view s) noexcept {
    if (s.size() >= 3
        && static_cast<std::uint8_t>(s[0]) == 0xEF
        && static_cast<std::uint8_t>(s[1]) == 0xBB
        && static_cast<std::uint8_t>(s[2]) == 0xBF) {
        s.remove_prefix(3);
    }
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

// Split a line by ASCII whitespace into tokens, preserving tokens and
// ignoring empty runs. Whitespace here is ' ' and '\t' only (no
// newline — caller splits lines first).
[[nodiscard]] inline std::vector<std::string_view> tokenize(
    std::string_view line) noexcept {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    const std::size_t n = line.size();
    while (i < n) {
        while (i < n && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= n) break;
        std::size_t start = i;
        while (i < n && line[i] != ' ' && line[i] != '\t') ++i;
        out.emplace_back(line.substr(start, i - start));
    }
    return out;
}

}  // namespace mxh::compat::detail
