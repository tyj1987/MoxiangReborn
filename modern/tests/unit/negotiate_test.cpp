// negotiate_test.cpp — Phase 3.4: Cipher negotiation wire protocol tests.
//
// Tests the handshake packet parsing, building, and negotiation logic.
// Does NOT require network I/O — pure wire-format unit tests.

#include "mxh/proto/negotiate.hpp"

#include <array>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace mxh::proto {
namespace {

// ── Helper: build a valid HandshakeRequest wire buffer ────────────────────

static std::vector<std::uint8_t> make_valid_request(
    std::uint32_t version,
    std::uint8_t  cipher_mask)
{
    std::array<std::uint8_t, kHandshakeRequestSize> buf{};
    std::memcpy(buf.data(), kMagic, 4);

    buf[4] = static_cast<std::uint8_t>((version >> 24) & 0xFF);
    buf[5] = static_cast<std::uint8_t>((version >> 16) & 0xFF);
    buf[6] = static_cast<std::uint8_t>((version >>  8) & 0xFF);
    buf[7] = static_cast<std::uint8_t>((version >>  0) & 0xFF);

    buf[8]  = cipher_mask;
    buf[9]  = 0;
    buf[10] = 0;
    buf[11] = 0;

    // Compute XOR checksum over first 11 bytes.
    std::uint8_t cs = 0;
    for (std::size_t i = 0; i < kHandshakeRequestSize - 1; ++i) cs ^= buf[i];
    buf[11] = cs;

    return {buf.begin(), buf.end()};
}

// ── Tests: HandshakeRequest parsing ───────────────────────────────────────

TEST(NegotiateParseRequest, ValidCurrentVersion) {
    auto buf = make_valid_request(kProtocolVersionCurrent,
                                  static_cast<std::uint8_t>(CipherType::AES_GCM));
    auto req = parse_handshake_request(buf);

    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->version, kProtocolVersionCurrent);
    EXPECT_EQ(req->cipher_mask,
              static_cast<std::uint8_t>(CipherType::AES_GCM));
}

TEST(NegotiateParseRequest, ValidLegacyVersion) {
    auto buf = make_valid_request(kProtocolVersionLegacy,
                                  static_cast<std::uint8_t>(CipherType::HSEL));
    auto req = parse_handshake_request(buf);

    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->version, kProtocolVersionLegacy);
}

TEST(NegotiateParseRequest, ValidBothCiphers) {
    auto buf = make_valid_request(
        kProtocolVersionCurrent,
        static_cast<std::uint8_t>(CipherType::AES_GCM | CipherType::HSEL));
    auto req = parse_handshake_request(buf);

    ASSERT_TRUE(req.has_value());
    EXPECT_TRUE(has_flag(static_cast<CipherType>(req->cipher_mask), CipherType::AES_GCM));
    EXPECT_TRUE(has_flag(static_cast<CipherType>(req->cipher_mask), CipherType::HSEL));
}

TEST(NegotiateParseRequest, TooShortReturnsNullopt) {
    std::vector<std::uint8_t> short_buf(11, 0x00);
    EXPECT_FALSE(parse_handshake_request(short_buf).has_value());
}

TEST(NegotiateParseRequest, BadMagicReturnsNullopt) {
    auto buf = make_valid_request(kProtocolVersionCurrent, 0);
    buf[0] = 0x00;  // corrupt magic
    EXPECT_FALSE(parse_handshake_request(buf).has_value());
}

TEST(NegotiateParseRequest, NonZeroReservedReturnsNullopt) {
    auto buf = make_valid_request(kProtocolVersionCurrent, 0);
    buf[9] = 0x01;  // corrupt reserved byte
    EXPECT_FALSE(parse_handshake_request(buf).has_value());
}

TEST(NegotiateParseRequest, BadChecksumReturnsNullopt) {
    auto buf = make_valid_request(kProtocolVersionCurrent, 0);
    buf[11] ^= 0x01;  // flip one bit in checksum
    EXPECT_FALSE(parse_handshake_request(buf).has_value());
}

// ── Tests: HandshakeResponse building ─────────────────────────────────────

TEST(NegotiateBuildResponse, SuccessWithAESKey) {
    std::array<std::uint8_t, 64> buffer{};
    std::array<std::uint8_t, 32> session_key{};
    for (std::size_t i = 0; i < session_key.size(); ++i) session_key[i] = static_cast<std::uint8_t>(i);

    std::size_t written = build_handshake_response(
        buffer, CipherType::AES_GCM, session_key);

    EXPECT_EQ(written, kHandshakeResponseHeaderSize + 32);
    EXPECT_EQ(buffer[0], kAckSuccess);
    EXPECT_EQ(buffer[1], static_cast<std::uint8_t>(CipherType::AES_GCM));
    EXPECT_EQ(buffer[2], 32);
    EXPECT_EQ(buffer[3], 0);  // reserved

    // Checksum = ack ^ cipher ^ key_len ^ reserved ^ key_data
    std::uint8_t expected_cs = 0x01 ^ 0x02 ^ 32 ^ 0;
    for (std::uint8_t b : session_key) expected_cs ^= b;
    EXPECT_EQ(buffer[4], expected_cs);

    // Key data is at offset 5.
    for (std::size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(buffer[kHandshakeResponseHeaderSize + i], session_key[i]);
    }
}

TEST(NegotiateBuildResponse, SuccessWithHSEL) {
    std::array<std::uint8_t, 16> buffer{};
    // HSEL doesn't use key exchange (shared key); 0 bytes of key_data.
    std::size_t written = build_handshake_response(buffer, CipherType::HSEL, {});

    EXPECT_EQ(written, kHandshakeResponseHeaderSize);
    EXPECT_EQ(buffer[0], kAckSuccess);
    EXPECT_EQ(buffer[1], static_cast<std::uint8_t>(CipherType::HSEL));
    EXPECT_EQ(buffer[2], 0);   // no key_data
    EXPECT_EQ(buffer[3], 0);   // reserved
    EXPECT_EQ(buffer[4], 0x01 ^ 0x01 ^ 0 ^ 0);  // checksum
}

TEST(NegotiateBuildResponse, BufferTooSmall) {
    std::array<std::uint8_t, 4> tiny{};
    std::array<std::uint8_t, 1> key{};
    EXPECT_EQ(build_handshake_response(tiny, CipherType::AES_GCM, key), 0u);
}

TEST(NegotiateBuildNack, BuildsCorrectPacket) {
    std::array<std::uint8_t, 8> buffer{};
    std::size_t written = build_handshake_nack(buffer, "no cipher");

    EXPECT_EQ(written, kHandshakeResponseHeaderSize);
    EXPECT_EQ(buffer[0], kAckFail);
    EXPECT_EQ(buffer[1], 0);
    EXPECT_EQ(buffer[2], 0);
    EXPECT_EQ(buffer[3], 0);
    EXPECT_EQ(buffer[4], 0xFF);  // checksum
}

// ── Tests: HandshakeResponse parsing ──────────────────────────────────────

TEST(NegotiateParseResponse, ValidAESResponse) {
    // Build what server would send: ack + cipher + key_len + cs + 16-byte key
    std::vector<std::uint8_t> key(16, 0xAB);
    std::array<std::uint8_t, 64> wire{};
    wire[0] = kAckSuccess;
    wire[1] = static_cast<std::uint8_t>(CipherType::AES_GCM);
    wire[2] = 16;
    wire[3] = 0;
    std::uint8_t cs = wire[0] ^ wire[1] ^ wire[2] ^ wire[3];
    for (std::uint8_t b : key) cs ^= b;
    wire[4] = cs;
    std::memcpy(wire.data() + kHandshakeResponseHeaderSize, key.data(), 16);

    NegotiationResult result{};
    bool ok = parse_handshake_response(wire, result);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.chosen, CipherType::AES_GCM);
    EXPECT_EQ(result.key_data.size(), 16u);
}

TEST(NegotiateParseResponse, ValidHSELResponse) {
    std::array<std::uint8_t, kHandshakeResponseHeaderSize> wire{};
    wire[0] = kAckSuccess;
    wire[1] = static_cast<std::uint8_t>(CipherType::HSEL);
    wire[2] = 0;   // no key data
    wire[3] = 0;
    wire[4] = wire[0] ^ wire[1] ^ wire[2] ^ wire[3];  // checksum

    NegotiationResult result{};
    EXPECT_TRUE(parse_handshake_response(wire, result));
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.chosen, CipherType::HSEL);
    EXPECT_TRUE(result.key_data.empty());
}

TEST(NegotiateParseResponse, NackResponse) {
    std::array<std::uint8_t, kHandshakeResponseHeaderSize> wire{};
    wire[0] = kAckFail;
    wire[1] = 0;
    wire[2] = 0;
    wire[3] = 0;
    wire[4] = 0xFF;

    NegotiationResult result{};
    EXPECT_FALSE(parse_handshake_response(wire, result));
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "server rejected: unsupported cipher");
}

TEST(NegotiateParseResponse, UnknownAckByte) {
    std::array<std::uint8_t, kHandshakeResponseHeaderSize> wire{};
    wire[0] = 0x99;
    wire[1] = 0;
    wire[2] = 0;
    wire[3] = 0;
    wire[4] = 0x99;

    NegotiationResult result{};
    EXPECT_FALSE(parse_handshake_response(wire, result));
    EXPECT_EQ(result.error, "unknown ack byte");
}

TEST(NegotiateParseResponse, BadChecksum) {
    std::array<std::uint8_t, 8> wire{};
    wire[0] = kAckSuccess;
    wire[1] = 0;
    wire[2] = 3;
    wire[3] = 0;
    wire[4] = 0;  // wrong checksum
    wire[5] = 1;
    wire[6] = 2;
    wire[7] = 3;

    NegotiationResult result{};
    EXPECT_FALSE(parse_handshake_response(wire, result));
    EXPECT_EQ(result.error, "checksum mismatch");
}

TEST(NegotiateParseResponse, TruncatedKeyData) {
    std::array<std::uint8_t, 7> wire{};
    wire[0] = kAckSuccess;
    wire[1] = 0;
    wire[2] = 10;  // claims 10 bytes of key
    wire[3] = 0;
    wire[4] = wire[0] ^ wire[1] ^ wire[2] ^ wire[3];
    // only 2 bytes present but header says 10

    NegotiationResult result{};
    EXPECT_FALSE(parse_handshake_response(wire, result));
    EXPECT_EQ(result.error, "response truncated: missing key_data");
}

TEST(NegotiateParseResponse, TooShortHeader) {
    std::vector<std::uint8_t> tiny(3, 0);
    NegotiationResult result{};
    EXPECT_FALSE(parse_handshake_response(tiny, result));
    EXPECT_EQ(result.error, "response too short");
}

// ── Tests: Cipher selection logic ─────────────────────────────────────────

TEST(NegotiateSelectCipher, ServerAndClientBothSupportAES) {
    auto chosen = select_cipher(
        static_cast<std::uint8_t>(CipherType::AES_GCM | CipherType::HSEL));
    EXPECT_EQ(chosen, CipherType::AES_GCM);
}

TEST(NegotiateSelectCipher, ClientOnlyHSEL_ServerBoth) {
    auto chosen = select_cipher(
        static_cast<std::uint8_t>(CipherType::HSEL));
    EXPECT_EQ(chosen, CipherType::HSEL);
}

TEST(NegotiateSelectCipher, ClientOnlyAES_ServerBoth) {
    auto chosen = select_cipher(
        static_cast<std::uint8_t>(CipherType::AES_GCM));
    EXPECT_EQ(chosen, CipherType::AES_GCM);
}

TEST(NegotiateSelectCipher, NoIntersection_ReturnsNone) {
    // Client: AES only; Server: HSEL only → no common cipher.
    auto chosen = select_cipher(
        static_cast<std::uint8_t>(CipherType::AES_GCM),
        /*server_supports_aes=*/false,
        /*server_supports_hsel=*/true);
    EXPECT_EQ(chosen, CipherType::None);
}

TEST(NegotiateSelectCipher, ServerDisablesAES_ClientPrefersAES) {
    auto chosen = select_cipher(
        static_cast<std::uint8_t>(CipherType::AES_GCM | CipherType::HSEL),
        /*server_supports_aes=*/false,
        /*server_supports_hsel=*/true);
    EXPECT_EQ(chosen, CipherType::HSEL);  // graceful fallback
}

TEST(NegotiateSelectCipher, ServerDisablesBoth_ReturnsNone) {
    auto chosen = select_cipher(
        static_cast<std::uint8_t>(CipherType::AES_GCM | CipherType::HSEL),
        /*server_supports_aes=*/false,
        /*server_supports_hsel=*/false);
    EXPECT_EQ(chosen, CipherType::None);
}

// ── Tests: Constants ───────────────────────────────────────────────────────

TEST(NegotiateConstants, MagicBytesCorrect) {
    EXPECT_EQ(kMagic[0], 0x4D);  // 'M'
    EXPECT_EQ(kMagic[1], 0x58);  // 'X'
    EXPECT_EQ(kMagic[2], 0x48);  // 'H'
    EXPECT_EQ(kMagic[3], 0x4E);  // 'N'
}

TEST(NegotiateConstants, RequestSizeIs12) {
    EXPECT_EQ(kHandshakeRequestSize, 12u);
}

TEST(NegotiateConstants, ResponseHeaderSizeIs5) {
    EXPECT_EQ(kHandshakeResponseHeaderSize, 5u);
}

TEST(NegotiateConstants, CipherTypeValuesCorrect) {
    EXPECT_EQ(static_cast<std::uint8_t>(CipherType::HSEL),    0x01);
    EXPECT_EQ(static_cast<std::uint8_t>(CipherType::AES_GCM), 0x02);
}

TEST(NegotiateConstants, CipherTypeHasFlagWorks) {
    auto both = CipherType::AES_GCM | CipherType::HSEL;
    EXPECT_TRUE(has_flag(both, CipherType::AES_GCM));
    EXPECT_TRUE(has_flag(both, CipherType::HSEL));
    EXPECT_FALSE(has_flag(CipherType::HSEL, CipherType::AES_GCM));
}

TEST(NegotiateConstants, CipherTypeNames) {
    EXPECT_STREQ(cipher_type_name(CipherType::None),    "None");
    EXPECT_STREQ(cipher_type_name(CipherType::HSEL),    "HSEL");
    EXPECT_STREQ(cipher_type_name(CipherType::AES_GCM), "AES-256-GCM");
}

// ── Tests: Round-trip request → response (integration-style) ─────────────

TEST(NegotiateRoundTrip, FullClientServerFlow_AES) {
    // 1. Client builds request.
    std::vector<std::uint8_t> client_buf = make_valid_request(
        kProtocolVersionCurrent,
        static_cast<std::uint8_t>(CipherType::AES_GCM | CipherType::HSEL));

    // 2. Server parses request.
    auto req = parse_handshake_request(client_buf);
    ASSERT_TRUE(req.has_value());

    // 3. Server selects cipher.
    CipherType chosen = select_cipher(req->cipher_mask);
    EXPECT_EQ(chosen, CipherType::AES_GCM);

    // 4. Server builds 32-byte session key.
    std::array<std::uint8_t, 32> session_key{};
    for (std::size_t i = 0; i < session_key.size(); ++i)
        session_key[i] = static_cast<std::uint8_t>(i * 7 + 3);

    // 5. Server builds response.
    std::array<std::uint8_t, 64> server_buf{};
    std::size_t written = build_handshake_response(
        server_buf, chosen, session_key);
    ASSERT_GT(written, 0);

    // 6. Client parses response.
    NegotiationResult result{};
    std::span<const std::uint8_t> resp_span(server_buf.data(), written);
    bool ok = parse_handshake_response(resp_span, result);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.chosen, CipherType::AES_GCM);
    ASSERT_EQ(result.key_data.size(), 32u);

    // 7. Verify session key matches.
    for (std::size_t i = 0; i < 32; ++i)
        EXPECT_EQ(result.key_data[i], session_key[i]);
}

TEST(NegotiateRoundTrip, FullClientServerFlow_HSELLegacyFallback) {
    // Client only supports HSEL (legacy client).
    std::vector<std::uint8_t> client_buf = make_valid_request(
        kProtocolVersionLegacy,
        static_cast<std::uint8_t>(CipherType::HSEL));

    auto req = parse_handshake_request(client_buf);
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->version, kProtocolVersionLegacy);

    CipherType chosen = select_cipher(req->cipher_mask);
    EXPECT_EQ(chosen, CipherType::HSEL);

    // Server builds response with no key_data.
    std::array<std::uint8_t, 8> server_buf{};
    std::size_t written = build_handshake_response(server_buf, chosen, {});
    ASSERT_GT(written, 0);

    NegotiationResult result{};
    std::span<const std::uint8_t> resp_span(server_buf.data(), written);
    ASSERT_TRUE(parse_handshake_response(resp_span, result));
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.chosen, CipherType::HSEL);
    EXPECT_TRUE(result.key_data.empty());
}

}  // namespace
}  // namespace mxh::proto
