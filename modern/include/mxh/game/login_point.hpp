#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "mxh/compat/detail/text_parse.hpp"

namespace mxh::game {
struct LoginPoint {
    float x = 0;
    float z = 0;
};
// One row per map. Revive uses the first shipped point; Map10 has exactly one.
using LoginPointTable = std::map<std::uint16_t, std::vector<LoginPoint>>;
enum class LoginPointProfile { PlayDhCurrent, LegacyMhFile };

inline std::optional<LoginPoint> login_revive_point(const LoginPointTable& table,
                                                    std::uint16_t map) {
    const auto found = table.find(map);
    if (found == table.end() || found->second.empty()) return std::nullopt;
    return found->second.front();
}

inline std::optional<LoginPointTable> parse_login_point_text(std::string_view text) {
    std::istringstream input{std::string(text)};
    LoginPointTable table;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream row{line};
        std::vector<std::string> tokens;
        for (std::string token; row >> token; ) tokens.push_back(std::move(token));
        if (tokens.empty()) continue;
        auto integer = [](const std::string& token, auto& value) {
            const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
            return result.ec == std::errc{} && result.ptr == token.data() + token.size();
        };
        unsigned long kind = 0;
        unsigned long map = 0;
        unsigned long count = 0;
        unsigned long chx = 0;
        if (tokens.size() < 7 || tokens[1].empty() || !integer(tokens[0], kind) || kind == 0 ||
            !integer(tokens[2], map) || map == 0 || map > 65535 || !integer(tokens[3], count) ||
            count < 1 || count > 10 || tokens.size() != 5 + 2 * count || !integer(tokens.back(), chx))
            return std::nullopt;
        std::vector<LoginPoint> points;
        points.reserve(static_cast<std::size_t>(count));
        for (unsigned long i = 0; i < count; ++i) {
            float x = 0, z = 0;
            const auto& x_token = tokens[4 + 2 * i];
            const auto& z_token = tokens[5 + 2 * i];
            const auto x_parsed = std::from_chars(x_token.data(), x_token.data() + x_token.size(), x);
            const auto z_parsed = std::from_chars(z_token.data(), z_token.data() + z_token.size(), z);
            if (x_parsed.ec != std::errc{} || x_parsed.ptr != x_token.data() + x_token.size() ||
                z_parsed.ec != std::errc{} || z_parsed.ptr != z_token.data() + z_token.size() ||
                !std::isfinite(x) || !std::isfinite(z) || x < 0 || z < 0 || x >= 65536 || z >= 65536)
                return std::nullopt;
            points.push_back(LoginPoint{x, z});
        }
        if (!table.emplace(static_cast<std::uint16_t>(map), std::move(points)).second)
            return std::nullopt;
    }
    if (table.empty()) return std::nullopt;
    return table;
}

// PlayDH body transform: unique per-lane maximum of the digit/tab/newline
// alphabet at offset 24. Decoded Map10/Map12 coordinates match the legacy MHFile.
inline std::optional<LoginPointTable> decode_login_points(std::span<const std::uint8_t> raw,
                                                          LoginPointProfile profile) {
    if (raw.size() < 14 || raw.size() > 65536) return std::nullopt;
    const auto u32 = [&](std::size_t at) {
        return std::uint32_t(raw[at]) | (std::uint32_t(raw[at + 1]) << 8) |
            (std::uint32_t(raw[at + 2]) << 16) | (std::uint32_t(raw[at + 3]) << 24);
    };
    std::vector<std::uint8_t> decoded;
    if (profile == LoginPointProfile::PlayDhCurrent) {
        if (raw.size() < 25 || u32(0) != raw.size()) return std::nullopt;
        constexpr std::uint8_t key[]{176, 34, 211, 18, 242, 2, 21, 114};
        decoded.assign(raw.begin() + 24, raw.end());
        for (std::size_t i = 0; i < decoded.size(); ++i) decoded[i] ^= key[i % 8];
    } else if (profile == LoginPointProfile::LegacyMhFile) {
        if (u32(4) == 0 || u32(8) != raw.size() - 14) return std::nullopt;
        decoded.assign(raw.begin() + 13, raw.end() - 1);
        const auto crc = mxh::compat::detail::decode_mhfile_text_payload(u32(4), decoded);
        if (crc != raw[12] || crc != raw.back()) return std::nullopt;
    } else return std::nullopt;
    return parse_login_point_text(std::string_view(
        reinterpret_cast<const char*>(decoded.data()), decoded.size()));
}
}  // namespace mxh::game
