#include "mxh/compat/gx_map_script.hpp"

#include "mxh/log/mlog.hpp"

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace mxh::compat {
namespace {

std::string display_name(std::string_view name) {
    if (name.empty()) return "(unnamed)";
    return std::string(name);
}

std::uint32_t read_le_u32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

bool starts_with_gx(std::span<const std::uint8_t> bytes) noexcept {
    return bytes.size() >= 3 && bytes[0] == static_cast<std::uint8_t>('G') &&
           bytes[1] == static_cast<std::uint8_t>('X') &&
           bytes[2] == static_cast<std::uint8_t>('_');
}

// Original-client .map: size >= 24, prefix == size, and not a plaintext GX_
// script. The 20-byte header and the XOR body are not inspected.
bool is_obfuscated_map(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 24 || bytes.size() > 0xffffffffu) return false;
    if (starts_with_gx(bytes)) return false;
    return read_le_u32(bytes.data()) == static_cast<std::uint32_t>(bytes.size());
}

std::string obfuscated_message(const std::string& name, std::size_t bytes) {
    std::ostringstream out;
    out << "ERROR [map] obfuscated .map not supported: " << name
        << " (bytes=" << bytes << ")";
    return out.str();
}

std::string malformed_message(const std::string& name, std::size_t bytes) {
    std::ostringstream out;
    out << "ERROR [map] malformed .map: " << name << " (bytes=" << bytes << ")";
    return out.str();
}

bool is_space(unsigned char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

std::vector<std::string> tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string current;
    const auto flush = [&] {
        if (!current.empty()) {
            tokens.push_back(std::move(current));
            current.clear();
        }
    };
    for (const unsigned char c : text) {
        if (c == '{' || c == '}') {
            flush();
            tokens.emplace_back(1, static_cast<char>(c));
        } else if (is_space(c) || c == '\0') {
            flush();
        } else {
            current.push_back(static_cast<char>(c));
        }
    }
    flush();
    return tokens;
}

bool parse_float_token(const std::string& token, float& out) noexcept {
    if (token.empty()) return false;
    char* end = nullptr;
    const float value = std::strtof(token.c_str(), &end);
    if (end == token.c_str() || (end && *end != '\0')) return false;
    out = value;
    return true;
}

bool is_section(const std::string& token) noexcept {
    return token == "GX_METADATA" || token == "GX_MAP" || token == "GX_OBJECT" ||
           token == "GX_LIGHT" || token == "GX_TRIGGER";
}

// Plaintext only. Caller has already rejected the obfuscated prefix.
bool parse_plaintext(std::string_view text, GxMapScript& script) {
    const auto tokens = tokenize(text);
    bool saw_section = false;
    int depth = 0;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token == "{") {
            ++depth;
            continue;
        }
        if (token == "}") {
            if (depth == 0) return false;
            --depth;
            continue;
        }
        if (is_section(token)) {
            saw_section = true;
            if (token == "GX_METADATA") script.has_metadata = true;
            continue;
        }
        if (token == "BOX_MAX" || token == "BOX_MIN") {
            if (i + 3 >= tokens.size()) return false;
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (!parse_float_token(tokens[i + 1], x) ||
                !parse_float_token(tokens[i + 2], y) ||
                !parse_float_token(tokens[i + 3], z)) {
                return false;
            }
            float* dst = token == "BOX_MAX" ? script.box_max : script.box_min;
            dst[0] = x;
            dst[1] = y;
            dst[2] = z;
            script.has_metadata = true;
            i += 3;
            continue;
        }
        if (token == "STATIC_MODEL" || token == "HEIGHT_FIELD") {
            if (i + 1 >= tokens.size()) return false;
            const auto& value = tokens[i + 1];
            if (value == "{" || value == "}") return false;
            if (token == "STATIC_MODEL") script.static_model = value;
            else script.height_field = value;
            ++i;
            continue;
        }
    }
    return saw_section && depth == 0;
}

GxMapParseResult fail_result(GxMapReject reject, std::string message) {
    MLOG_ERROR("%s", message.c_str());
    GxMapParseResult result;
    result.ok = false;
    result.reject = reject;
    result.message = std::move(message);
    return result;
}

}  // namespace

GxMapParseResult parse_gx_map_script(std::span<const std::uint8_t> bytes,
                                     std::string_view name) {
    const std::string shown = display_name(name);
    if (is_obfuscated_map(bytes)) {
        // Do not tokenize or inspect the XOR body.
        return fail_result(GxMapReject::Obfuscated,
                           obfuscated_message(shown, bytes.size()));
    }
    const bool plaintext = starts_with_gx(bytes);
    GxMapScript script;
    if (!plaintext ||
        !parse_plaintext(std::string_view(reinterpret_cast<const char*>(bytes.data()),
                                          bytes.size()),
                         script)) {
        return fail_result(GxMapReject::Malformed,
                           malformed_message(shown, bytes.size()));
    }
    GxMapParseResult result;
    result.ok = true;
    result.script = std::move(script);
    return result;
}

}  // namespace mxh::compat
