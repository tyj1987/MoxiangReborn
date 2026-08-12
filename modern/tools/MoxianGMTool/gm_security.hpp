#pragma once

#include <cctype>
#include <map>
#include <string>
#include <string_view>

namespace mxh::gm {

inline bool constant_time_equal(std::string_view lhs, std::string_view rhs) {
    std::size_t difference = lhs.size() ^ rhs.size();
    const std::size_t count = lhs.size() > rhs.size() ? lhs.size() : rhs.size();
    for (std::size_t i = 0; i < count; ++i) {
        const unsigned char a = i < lhs.size() ? static_cast<unsigned char>(lhs[i]) : 0;
        const unsigned char b = i < rhs.size() ? static_cast<unsigned char>(rhs[i]) : 0;
        difference |= a ^ b;
    }
    return difference == 0;
}

inline std::string lower_ascii(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

inline bool authorize_bearer(const std::map<std::string, std::string>& headers,
                             std::string_view expected_token) {
    if (expected_token.empty()) return false;
    for (const auto& [name, value] : headers) {
        if (lower_ascii(name) != "authorization") continue;
        constexpr std::string_view prefix = "Bearer ";
        if (value.size() <= prefix.size() || value.compare(0, prefix.size(), prefix) != 0)
            return false;
        return constant_time_equal(std::string_view(value).substr(prefix.size()), expected_token);
    }
    return false;
}

} // namespace mxh::gm
