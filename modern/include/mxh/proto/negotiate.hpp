// negotiate.hpp — Phase 3.4: Cipher negotiation protocol.
//
// Implements the client → server handshake that selects which encryption
// layer to use for the session.
//
// Wire protocol (all fields big-endian / network byte order):
//
// Client → Server (handshake, unencrypted, 12 bytes):
//   [magic:     4B]  "MXHN" = {0x4D, 0x58, 0x48, 0x4E}
//   [version:   4B]  client protocol version (0 = legacy)
//   [cipher:    1B]  cipher capability bitmask (see CipherType)
//   [reserved:  3B]  must be 0; non-zero = malformed
//   [xor_sum:   1B]  XOR of first 11 bytes
//
// Server → Client (response, unencrypted, 7 bytes + key_data):
//   [ack:       1B]  0x01 = success | 0xFF = unsupported cipher
//   [cipher:    1B]  chosen CipherType
//   [key_len:   1B]  length of key_data in bytes
//   [reserved:  1B]  must be 0
//   [xor_sum:   1B]  XOR of first 4 bytes + key_data
//   [key_data: N B]  cipher-specific key material (N = key_len)
//
// For AES (chosen_cipher = AES_GCM):
//   key_data = 32-byte random session key (server-generated)
//
// After handshake the connection switches to encrypted mode using the
// chosen cipher. All subsequent packets go through the IEncryptor.

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::proto {

// ── Cipher types ───────────────────────────────────────────────────────────

enum class CipherType : std::uint8_t {
    None    = 0,
    HSEL    = 1 << 0,  // Legacy HSEL stream cipher (no auth)
    AES_GCM = 1 << 1,  // AES-256-GCM via Windows CNG (authenticated)
};

constexpr CipherType operator|(CipherType a, CipherType b) {
    return static_cast<CipherType>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}
constexpr bool has_flag(CipherType mask, CipherType flag) {
    return (static_cast<std::uint8_t>(mask) & static_cast<std::uint8_t>(flag)) != 0;
}

// ── Protocol constants ─────────────────────────────────────────────────────

inline constexpr std::size_t kHandshakeRequestSize = 12;
inline constexpr std::size_t kHandshakeResponseHeaderSize = 5;
inline constexpr std::uint32_t kProtocolVersionCurrent = 1;
inline constexpr std::uint32_t kProtocolVersionLegacy  = 0;
inline constexpr std::uint8_t  kMagic[4] = {0x4D, 0x58, 0x48, 0x4E};  // "MXHN"
inline constexpr std::uint8_t  kAckSuccess = 0x01;
inline constexpr std::uint8_t  kAckFail    = 0xFF;

// ── Wire structs ──────────────────────────────────────────────────────────

// Wire structs — packed to guarantee exact byte layout.
//
// HandshakeRequest wire format (13 bytes on wire, checksum at byte 11):
//   [0..3]   magic[4]    = "MXHN" (0x4D 0x58 0x48 0x4E)
//   [4..7]   version     = big-endian uint32
//   [8]      cipher_mask = CipherType bitmask
//   [9..10]  reserved[2] = must be 0
//   [11]     xor_sum     = XOR of bytes 0..10
//
// We store the 12-byte fixed header as individual fields with #pragma pack(1)
// to eliminate any padding.  Version is stored big-endian (network byte order).
//
// Wire: magic(4) + version(4) + cipher_mask(1) + reserved(2) + xor_sum(1) = 12 bytes
// The xor_sum byte IS part of the struct (stored at offset 11).
#pragma pack(push, 1)
struct HandshakeRequest {
    std::uint8_t  magic[4];
    std::uint32_t version;       // stored big-endian
    std::uint8_t  cipher_mask;
    std::uint8_t  reserved[2];
    std::uint8_t  xor_sum;
};
#pragma pack(pop)
static_assert(sizeof(HandshakeRequest) == 12,
              "HandshakeRequest must be exactly 12 bytes with pack(1)");

// HandshakeResponse header (5 bytes, followed by key_data N bytes):
//   [0] ack
//   [1] chosen cipher
//   [2] key_len
//   [3] reserved (must be 0)
//   [4] xor checksum
#pragma pack(push, 1)
struct HandshakeResponse {
    std::uint8_t  ack;
    std::uint8_t  cipher;
    std::uint8_t  key_len;
    std::uint8_t  reserved;
    std::uint8_t  xor_sum;
    // key_data follows immediately after in the wire buffer
};
#pragma pack(pop)
static_assert(sizeof(HandshakeResponse) == kHandshakeResponseHeaderSize,
              "HandshakeResponse header must be exactly 5 bytes with pack(1)");

// ── Negotiation result ──────────────────────────────────────────────────────

struct NegotiationResult {
    bool        ok = false;
    CipherType  chosen = CipherType::None;
    std::string error;          // human-readable error
    std::vector<std::uint8_t> key_data;  // cipher-specific key material
};

// ── HandshakeRequest parsing ───────────────────────────────────────────────

// Returns nullopt if the buffer is too short.
// On success, fills result and returns the number of bytes consumed (always 12).
[[nodiscard]]
inline std::optional<HandshakeRequest> parse_handshake_request(
    std::span<const std::uint8_t> data) noexcept
{
    if (data.size() < kHandshakeRequestSize) return std::nullopt;

    HandshakeRequest req{};
    std::memcpy(&req, data.data(), sizeof(req));

    // Verify magic.
    if (std::memcmp(req.magic, kMagic, 4) != 0) return std::nullopt;

    // Version is big-endian on wire; convert to host byte order.
    // Read directly from the wire buffer (bytes 4..7).
    req.version = static_cast<std::uint32_t>(
        (static_cast<std::uint32_t>(data[4]) << 24) |
        (static_cast<std::uint32_t>(data[5]) << 16) |
        (static_cast<std::uint32_t>(data[6]) <<  8) |
        (static_cast<std::uint32_t>(data[7]) <<  0));

    // Verify reserved bytes are zero.
    if (req.reserved[0] != 0 || req.reserved[1] != 0) {
        return std::nullopt;
    }

    // Verify XOR checksum: XOR of first 11 bytes should equal xor_sum at byte 11.
    std::uint8_t computed = 0;
    for (std::size_t i = 0; i < 11; ++i) {
        computed ^= data[i];
    }
    if (computed != req.xor_sum) return std::nullopt;

    return req;
}

// ── HandshakeResponse building ─────────────────────────────────────────────

// Builds a handshake response into the provided buffer.
// Returns the number of bytes written (header + key_data), or 0 on error.
[[nodiscard]]
inline std::size_t build_handshake_response(
    std::span<std::uint8_t> buffer,
    CipherType chosen,
    std::span<const std::uint8_t> key_data) noexcept
{
    const std::size_t total = kHandshakeResponseHeaderSize + key_data.size();
    if (buffer.size() < total) return 0;

    buffer[0] = kAckSuccess;
    buffer[1] = static_cast<std::uint8_t>(chosen);
    buffer[2] = static_cast<std::uint8_t>(key_data.size());
    buffer[3] = 0;  // reserved

    // XOR checksum: first 4 header bytes XORed with all key_data bytes.
    std::uint8_t cs = buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3];
    for (std::uint8_t b : key_data) cs ^= b;
    buffer[4] = cs;

    if (!key_data.empty()) {
        std::memcpy(buffer.data() + kHandshakeResponseHeaderSize,
                    key_data.data(), key_data.size());
    }

    return total;
}

// Build a failure response (no cipher selected).
[[nodiscard]]
inline std::size_t build_handshake_nack(
    std::span<std::uint8_t> buffer,
    std::string_view reason) noexcept
{
    (void)reason;  // logged client-side, not transmitted
    if (buffer.size() < kHandshakeResponseHeaderSize) return 0;

    buffer[0] = kAckFail;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3];  // = 0xFF
    return kHandshakeResponseHeaderSize;
}

// ── HandshakeResponse parsing ───────────────────────────────────────────────

[[nodiscard]]
inline bool parse_handshake_response(
    std::span<const std::uint8_t> data,
    NegotiationResult& out) noexcept
{
    if (data.size() < kHandshakeResponseHeaderSize) {
        out.ok = false;
        out.error = "response too short";
        return false;
    }

    std::uint8_t ack = data[0];
    if (ack == kAckFail) {
        out.ok = false;
        out.error = "server rejected: unsupported cipher";
        return false;
    }
    if (ack != kAckSuccess) {
        out.ok = false;
        out.error = "unknown ack byte";
        return false;
    }

    std::uint8_t cipher  = data[1];
    std::uint8_t key_len = data[2];
    std::uint8_t cs      = data[4];

    // Verify checksum.
    std::uint8_t computed = data[0] ^ data[1] ^ data[2] ^ data[3];
    for (std::size_t i = kHandshakeResponseHeaderSize;
         i < kHandshakeResponseHeaderSize + key_len; ++i) {
        if (i >= data.size()) {
            out.ok = false;
            out.error = "response truncated: missing key_data";
            return false;
        }
        computed ^= data[i];
    }
    if (computed != cs) {
        out.ok = false;
        out.error = "checksum mismatch";
        return false;
    }

    out.ok   = true;
    out.chosen = static_cast<CipherType>(cipher);
    out.key_data.assign(
        data.begin() + kHandshakeResponseHeaderSize,
        data.begin() + kHandshakeResponseHeaderSize + key_len);
    return true;
}

// ── Server-side negotiation logic ──────────────────────────────────────────

// Given client capabilities and server preferences, pick the best cipher.
[[nodiscard]]
inline CipherType select_cipher(
    std::uint8_t client_cipher_mask,
    bool server_supports_aes = true,
    bool server_supports_hsel = true) noexcept
{
    std::uint8_t server_mask = 0;
    if (server_supports_aes)  server_mask |= static_cast<std::uint8_t>(CipherType::AES_GCM);
    if (server_supports_hsel) server_mask |= static_cast<std::uint8_t>(CipherType::HSEL);

    std::uint8_t intersection = client_cipher_mask & server_mask;

    // Prefer AES-GCM if both client and server support it.
    if (intersection & static_cast<std::uint8_t>(CipherType::AES_GCM)) {
        return CipherType::AES_GCM;
    }
    // Fall back to HSEL (legacy compatibility).
    if (intersection & static_cast<std::uint8_t>(CipherType::HSEL)) {
        return CipherType::HSEL;
    }
    return CipherType::None;
}

// ── Name helpers ───────────────────────────────────────────────────────────

[[nodiscard]]
inline const char* cipher_type_name(CipherType ct) noexcept {
    switch (ct) {
        case CipherType::None:    return "None";
        case CipherType::HSEL:    return "HSEL";
        case CipherType::AES_GCM: return "AES-256-GCM";
        default:                  return "Unknown";
    }
}

}  // namespace mxh::proto
