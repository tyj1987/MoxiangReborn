// modern/src/portal/jwt_token.hpp
// Minimal JWT HS256 implementation using Windows BCrypt.
// No external dependencies beyond bcrypt.dll (Windows 8+).

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace mxh::portal {

// Result of JWT verification
struct JwtPayload {
    std::string sub;       // account name
    std::uint32_t user_idx;
    std::string exp;        // ISO8601 expiry
    std::string iat;        // ISO8601 issued at
};

// Create a JWT HS256 token.
// secret: HS256 signing key (at least 32 bytes recommended)
// sub: account name (subject)
// user_idx: numeric user identifier
// expiry_seconds: token lifetime in seconds (default 24h)
std::string create_jwt(std::string_view secret,
                        std::string_view sub,
                        std::uint32_t user_idx,
                        std::uint64_t expiry_seconds = 86400);

// Verify and decode a JWT HS256 token.
// Returns nullopt on success (payload populated).
// On error, returns human-readable error message.
std::optional<std::string> verify_jwt(std::string_view secret,
                                       std::string_view token,
                                       JwtPayload& out);

// ---------------------------------------------------------------------------
// Internal helpers — exposed for unit tests (not part of public API)
// Declared non-inline so jwt_token.cpp and jwt_token_test_util.cpp provide separate
// definitions (same implementation, MSVC ICF merges them).
// ---------------------------------------------------------------------------

// RFC 4648 §5 base64url encoding (no padding).
std::string base64url_encode(const std::uint8_t* data, std::size_t len);

// HMAC-SHA256 via BCrypt.
bool hmac_sha256(const std::uint8_t* key, std::size_t key_len,
                 const std::uint8_t* data, std::size_t data_len,
                 std::uint8_t out[32]);

// Decode base64url (no padding).
bool base64url_decode(const char* src, std::size_t len,
                      std::vector<std::uint8_t>& out);

}  // namespace mxh::portal
