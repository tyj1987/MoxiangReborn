// tests/unit/portal/jwt_token_test_util.cpp
// M5.2: Provides base64url_encode/decode for manual JWT construction in tests.
// hmac_sha256 comes from jwt_token.cpp (linked into the test executable).
// This file only provides base64url helpers which are identical to jwt_token.cpp's
// versions (non-inline → MSVC ICF merges them at link time).
#include "portal/jwt_token.hpp"

#include <cstddef>
#include <cstring>
#include <vector>

namespace mxh::portal {
namespace {

static const char kBase64Url[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

inline int base64url_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

}  // anonymous namespace

// Non-inline (matching jwt_token.hpp declaration) — MSVC ICF will merge with jwt_token.cpp.
inline std::string base64url_encode(const std::uint8_t* data, std::size_t len) {
    std::string out;
    out.reserve((len * 8 + 5) / 6);
    std::size_t i = 0;
    while (i + 2 < len) {
        std::uint32_t triple = (std::uint32_t(data[i]) << 16)
                             | (std::uint32_t(data[i+1]) << 8)
                             | data[i+2];
        out.push_back(kBase64Url[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Url[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Url[(triple >>  6) & 0x3F]);
        out.push_back(kBase64Url[triple & 0x3F]);
        i += 3;
    }
    if (i + 1 < len) {
        std::uint32_t double_word = (std::uint32_t(data[i]) << 16)
                                  | (std::uint32_t(data[i+1]) << 8);
        out.push_back(kBase64Url[(double_word >> 18) & 0x3F]);
        out.push_back(kBase64Url[(double_word >> 12) & 0x3F]);
        out.push_back(kBase64Url[(double_word >>  6) & 0x3F]);
    } else if (i < len) {
        std::uint32_t single = data[i] << 16;
        out.push_back(kBase64Url[(single >> 18) & 0x3F]);
        out.push_back(kBase64Url[(single >> 12) & 0x3F]);
    }
    return out;
}

inline bool base64url_decode(const char* src, std::size_t len,
                      std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(len * 3 / 4);
    std::size_t i = 0;
    while (i < len) {
        int a = i < len ? base64url_decode_char(src[i]) : 0;
        int b = (i + 1 < len) ? base64url_decode_char(src[i+1]) : 0;
        int c = (i + 2 < len) ? base64url_decode_char(src[i+2]) : 0;
        int d = (i + 3 < len) ? base64url_decode_char(src[i+3]) : 0;
        if (a < 0 || b < 0) return false;
        std::uint32_t word = (static_cast<std::uint32_t>(a) << 18)
                           | (static_cast<std::uint32_t>(b) << 12)
                           | (static_cast<std::uint32_t>(c & 0x3F) << 6)
                           |  static_cast<std::uint32_t>(d & 0x3F);
        out.push_back(static_cast<std::uint8_t>((word >> 16) & 0xFF));
        if (i + 2 < len) out.push_back(static_cast<std::uint8_t>((word >> 8) & 0xFF));
        if (i + 3 < len) out.push_back(static_cast<std::uint8_t>(word & 0xFF));
        i += 4;
    }
    return true;
}

// hmac_sha256: comes from jwt_token.cpp (no definition here).
// The test executable links jwt_token.cpp which provides the real implementation.

}  // namespace mxh::portal
