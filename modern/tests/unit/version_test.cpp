// version_test.cpp — Phase 4.3: Protocol version negotiation unit tests.
//
// Tests the version constants, VersionRejectReason, and the version check
// helper logic used by LoginHandler. Pure unit tests — no network I/O.

#include "mxh/proto/protocol.hpp"
#include "mxh/net/net.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace mxh::proto {
namespace {

// ── Version constants ─────────────────────────────────────────────────────

TEST(ProtocolVersion, CurrentVersionIsOne) {
    EXPECT_EQ(kProtocolVersion, 1u);
}

TEST(ProtocolVersion, MinVersionIsZeroForLegacyCompat) {
    EXPECT_EQ(kMinProtocolVersion, 0u);
}

TEST(ProtocolVersion, CurrentIsAtLeastMin) {
    EXPECT_GE(kProtocolVersion, kMinProtocolVersion);
}

// ── VersionRejectReason ───────────────────────────────────────────────────

TEST(VersionRejectReason, ValuesAreDistinct) {
    EXPECT_NE(VersionRejectReason::TooOld, VersionRejectReason::TooNew);
    EXPECT_NE(VersionRejectReason::TooOld, VersionRejectReason::ServerFull);
    EXPECT_NE(VersionRejectReason::TooNew, VersionRejectReason::ServerFull);
}

TEST(VersionRejectReason, TooOldIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(VersionRejectReason::TooOld), 0u);
}

TEST(VersionRejectReason, TooNewIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(VersionRejectReason::TooNew), 1u);
}

TEST(VersionRejectReason, ServerFullIsTwo) {
    EXPECT_EQ(static_cast<std::uint8_t>(VersionRejectReason::ServerFull), 2u);
}

// ── UserConnProtocol enum values ──────────────────────────────────────────

TEST(UserConnProtocol, VersionCheckMessagesExist) {
    // Phase 3.4 added modern version negotiation as three free-standing
    // protocol IDs (kModern* constants), NOT slots in the legacy
    // UserConnProtocol enum (which is locked at 0-95 for 1:1 compat).
    EXPECT_EQ(kModernCheckVersion, 200);
    EXPECT_EQ(kModernNotifyVersionAck, 201);
    EXPECT_EQ(kModernNotifyVersionNack, 202);
}

TEST(UserConnProtocol, LoginMessagesUnchanged) {
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::RequestLogin), 1);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginAck), 2);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginNack), 3);
}

// ── Version compatibility logic (mirrors LoginHandler::handle_version_check) ─

namespace {

// Simulate the version check logic from login_handler.cpp.
// Returns true if version is accepted, false if rejected.
bool check_version_compatible(std::uint16_t client_version,
                              std::uint16_t server_version = kProtocolVersion,
                              std::uint16_t min_version = kMinProtocolVersion) {
    if (client_version < min_version) return false;
    if (client_version > server_version) return false;
    return true;
}

// Build a CheckVersion payload: [client_version: u16] [name_len: u8] [name: bytes]
std::vector<std::uint8_t> make_check_version_payload(std::uint16_t version,
                                                      const std::string& name = "") {
    std::vector<std::uint8_t> payload;
    payload.push_back(static_cast<std::uint8_t>(version & 0xFF));
    payload.push_back(static_cast<std::uint8_t>((version >> 8) & 0xFF));
    payload.push_back(static_cast<std::uint8_t>(name.size()));
    for (char c : name) payload.push_back(static_cast<std::uint8_t>(c));
    return payload;
}

// Parse a version ACK payload: [server_version: u16] [encryption_required: u8]
struct VersionAckInfo {
    std::uint16_t server_version;
    bool encryption_required;
};

VersionAckInfo parse_version_ack(const std::vector<std::uint8_t>& payload) {
    VersionAckInfo info{};
    if (payload.size() >= 3) {
        info.server_version = static_cast<std::uint16_t>(payload[0])
                            | (static_cast<std::uint16_t>(payload[1]) << 8);
        info.encryption_required = payload[2] != 0;
    }
    return info;
}

// Parse a version NACK payload: [server_version: u16] [reason: u8]
struct VersionNackInfo {
    std::uint16_t server_version;
    VersionRejectReason reason;
};

VersionNackInfo parse_version_nack(const std::vector<std::uint8_t>& payload) {
    VersionNackInfo info{};
    if (payload.size() >= 3) {
        info.server_version = static_cast<std::uint16_t>(payload[0])
                            | (static_cast<std::uint16_t>(payload[1]) << 8);
        info.reason = static_cast<VersionRejectReason>(payload[2]);
    }
    return info;
}

}  // namespace

// ── Version compatibility checks ──────────────────────────────────────────

TEST(VersionCheck, CurrentVersionAccepted) {
    EXPECT_TRUE(check_version_compatible(kProtocolVersion));
}

TEST(VersionCheck, LegacyVersionAcceptedWhenMinIsZero) {
    EXPECT_TRUE(check_version_compatible(0));  // legacy = 0, min = 0
}

TEST(VersionCheck, OlderThanMinRejected) {
    // If server raises min_version to 1, legacy client (v0) should be rejected.
    EXPECT_FALSE(check_version_compatible(0, /*server=*/1, /*min=*/1));
}

TEST(VersionCheck, NewerThanServerRejected) {
    // Client claims v99 but server is v1.
    EXPECT_FALSE(check_version_compatible(99));
}

TEST(VersionCheck, OneBelowMinRejected) {
    // kMinProtocolVersion is 0 (uint16_t); subtracting 1 wraps to 0xFFFF.
    // Use a server with min_version=1 to test rejection of v0.
    EXPECT_FALSE(check_version_compatible(0, /*server=*/1, /*min=*/1));
}

TEST(VersionCheck, ExactlyMinAccepted) {
    EXPECT_TRUE(check_version_compatible(kMinProtocolVersion));
}

TEST(VersionCheck, ExactlyServerMaxAccepted) {
    EXPECT_TRUE(check_version_compatible(kProtocolVersion));
}

// ── Payload encoding/decoding ─────────────────────────────────────────────

TEST(VersionPayload, CheckVersionEncodesVersionCorrectly) {
    auto payload = make_check_version_payload(42);
    ASSERT_EQ(payload.size(), 3u);
    EXPECT_EQ(payload[0], 42 & 0xFF);        // low byte
    EXPECT_EQ(payload[1], (42 >> 8) & 0xFF); // high byte
    EXPECT_EQ(payload[2], 0);                 // empty name
}

TEST(VersionPayload, CheckVersionEncodesNameCorrectly) {
    auto payload = make_check_version_payload(1, "MoxianClient");
    // "MoxianClient" = 12 chars; total = 3 (header) + 12 = 15
    ASSERT_EQ(payload.size(), 3u + 12u);
    EXPECT_EQ(payload[2], 12);  // name length
    EXPECT_EQ(payload[3], 'M');
    EXPECT_EQ(payload[14], 't');
}

TEST(VersionPayload, ParseVersionAckRoundTrip) {
    // Simulate server ACK: version=1, encryption_required=true
    std::vector<std::uint8_t> ack_payload = {0x01, 0x00, 0x01};
    auto info = parse_version_ack(ack_payload);
    EXPECT_EQ(info.server_version, 1);
    EXPECT_TRUE(info.encryption_required);
}

TEST(VersionPayload, ParseVersionAckNoEncryption) {
    std::vector<std::uint8_t> ack_payload = {0x01, 0x00, 0x00};
    auto info = parse_version_ack(ack_payload);
    EXPECT_EQ(info.server_version, 1);
    EXPECT_FALSE(info.encryption_required);
}

TEST(VersionPayload, ParseVersionNackTooOld) {
    std::vector<std::uint8_t> nack_payload = {0x01, 0x00, 0x00};
    auto info = parse_version_nack(nack_payload);
    EXPECT_EQ(info.server_version, 1);
    EXPECT_EQ(info.reason, VersionRejectReason::TooOld);
}

TEST(VersionPayload, ParseVersionNackTooNew) {
    std::vector<std::uint8_t> nack_payload = {0x01, 0x00, 0x01};
    auto info = parse_version_nack(nack_payload);
    EXPECT_EQ(info.server_version, 1);
    EXPECT_EQ(info.reason, VersionRejectReason::TooNew);
}

TEST(VersionPayload, ParseVersionNackServerFull) {
    std::vector<std::uint8_t> nack_payload = {0x01, 0x00, 0x02};
    auto info = parse_version_nack(nack_payload);
    EXPECT_EQ(info.reason, VersionRejectReason::ServerFull);
}

TEST(VersionPayload, EmptyPayloadParsedAsZero) {
    std::vector<std::uint8_t> empty;
    auto info = parse_version_ack(empty);
    EXPECT_EQ(info.server_version, 0);
    EXPECT_FALSE(info.encryption_required);
}

// ── MsgHeader still 8 bytes (version doesn't change wire format) ──────────

TEST(MsgHeaderLayout, StillEightBytes) {
    EXPECT_EQ(sizeof(mxh::net::MsgHeader), 8u);
}

TEST(MsgRootLayout, StillFourBytes) {
    EXPECT_EQ(sizeof(mxh::net::MsgRoot), 4u);
}

}  // namespace
}  // namespace mxh::proto
