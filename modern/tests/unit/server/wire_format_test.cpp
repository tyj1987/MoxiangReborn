// wire_format_test.cpp
//
// Phase 6.4 unblock: regression coverage for tools/MoxianSideBySide/packet.cpp
// Packet::wire_bytes() (locked against the 4.5 wire-format mismatch fix in
// commit 60110a7). The wire format MUST be byte-identical to the legacy
// [Server]*/4DyuchiNET_Latest/ send path:
//
//   [ 2B length LE = uint16_t(8 + payload.size()) ]
//   [ 8B MSGBASE   = checksum:1 | code:1 | category:1 | protocol:1 | object_id:4 LE ]
//   [ payload bytes                                                ]
//
// Any drift here breaks Phase B 7.11 (side-by-side login/enter_game diff=0).

#include "packet.hpp"

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

namespace {
using mxh::tools::sidebyside::Packet;

// Helper: build a deterministic Packet for round-trip tests.
Packet MakePacket(std::uint8_t cat, std::uint8_t proto,
                   std::uint32_t object_id, std::uint8_t checksum,
                   std::int8_t code, std::vector<std::uint8_t> payload) {
    Packet p;
    p.checksum = checksum;
    p.code = code;
    p.category = cat;
    p.protocol = proto;
    p.object_id = object_id;
    p.payload = std::move(payload);
    p.direction = "s->c";
    return p;
}
}

// =====================================================================
// Layout invariants
// =====================================================================

TEST(WireFormat, EmptyPayloadHasEightByteBody) {
    auto bytes = MakePacket(7, 3, 0, 0, 0, {}).wire_bytes();
    ASSERT_EQ(bytes.size(), 10u);
    // 2-byte length LE = 8 (just MSGBASE, no payload).
    EXPECT_EQ(bytes[0], 0x08);
    EXPECT_EQ(bytes[1], 0x00);
}

TEST(WireFormat, LengthPrefixMatchesEightPlusPayload) {
    for (std::size_t n : {0u, 1u, 4u, 23u, 100u, 247u}) {
        std::vector<std::uint8_t> payload(n);
        for (std::size_t i = 0; i < n; ++i) payload[i] = static_cast<std::uint8_t>(i + 1);
        auto bytes = MakePacket(7, 3, 0, 0, 0, payload).wire_bytes();
        std::uint16_t len = static_cast<std::uint16_t>(bytes[0]) |
                              (static_cast<std::uint16_t>(bytes[1]) << 8);
        EXPECT_EQ(len, static_cast<std::uint16_t>(8 + n)) << "payload size " << n;
        EXPECT_EQ(bytes.size(), 2u + 8u + n);
    }
}

TEST(WireFormat, MsgBaseFieldByteOrder) {
    auto bytes = MakePacket(7, 3, 0x12345678u, 0xAB, -1, {}).wire_bytes();
    // bytes 2..9 = MSGBASE = [checksum][code][category][protocol][object_id LE]
    EXPECT_EQ(bytes[2], 0xAB);  // checksum (u8)
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 0xFF);  // code (i8 = -1)
    EXPECT_EQ(bytes[4], 7);     // category
    EXPECT_EQ(bytes[5], 3);     // protocol
    // object_id LE: 0x12345678
    EXPECT_EQ(bytes[6], 0x78);
    EXPECT_EQ(bytes[7], 0x56);
    EXPECT_EQ(bytes[8], 0x34);
    EXPECT_EQ(bytes[9], 0x12);
}

TEST(WireFormat, ProtocolFieldIsUnsignedByte) {
    // protocol=200 must encode as 200 (0xC8), NOT sign-extended.
    auto bytes = MakePacket(7, 200, 0, 0, 0, {}).wire_bytes();
    EXPECT_EQ(bytes[5], 200);
}

TEST(WireFormat, ObjectIdZeroEncodesAsAllZeros) {
    auto bytes = MakePacket(7, 0, 0, 0, 0, {}).wire_bytes();
    EXPECT_EQ(bytes[6], 0);
    EXPECT_EQ(bytes[7], 0);
    EXPECT_EQ(bytes[8], 0);
    EXPECT_EQ(bytes[9], 0);
}

TEST(WireFormat, ObjectIdMaxUint32EncodesAllOnes) {
    auto bytes = MakePacket(7, 0, 0xFFFFFFFFu, 0, 0, {}).wire_bytes();
    EXPECT_EQ(bytes[6], 0xFF);
    EXPECT_EQ(bytes[7], 0xFF);
    EXPECT_EQ(bytes[8], 0xFF);
    EXPECT_EQ(bytes[9], 0xFF);
}

TEST(WireFormat, PayloadBytesAppendedVerbatim) {
    std::vector<std::uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    auto bytes = MakePacket(7, 3, 0, 0, 0, payload).wire_bytes();
    ASSERT_EQ(bytes.size(), 14u);
    EXPECT_EQ(bytes[10], 0xDE);
    EXPECT_EQ(bytes[11], 0xAD);
    EXPECT_EQ(bytes[12], 0xBE);
    EXPECT_EQ(bytes[13], 0xEF);
}

// =====================================================================
// Round-trip: encode then manually decode, expect identical fields
// (this is what tools/MoxianSideBySide/capture/packet_capture.cpp does)
// =====================================================================

TEST(WireFormat, RoundTripManualDecodePreservesFields) {
    std::vector<std::uint8_t> payload = {
        0x01, 0x00, 0x07, 0x00, 0xE8, 0x03, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D
    };
    auto bytes = MakePacket(7, 0, 8, 0x08, 0, payload).wire_bytes();
    ASSERT_GE(bytes.size(), 10u);

    // 1. Length prefix
    std::uint16_t bodySize = static_cast<std::uint16_t>(bytes[0]) |
                            (static_cast<std::uint16_t>(bytes[1]) << 8);
    EXPECT_EQ(bodySize, static_cast<std::uint16_t>(bytes.size() - 2));

    // 2. MSGBASE = 8 bytes
    EXPECT_EQ(bytes[2], 0x08);                  // checksum
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 0);  // code (i8 = 0)
    EXPECT_EQ(bytes[4], 7);                     // category
    EXPECT_EQ(bytes[5], 0);                     // protocol
    std::uint32_t object_id = static_cast<std::uint32_t>(bytes[6]) |
                              (static_cast<std::uint32_t>(bytes[7]) << 8) |
                              (static_cast<std::uint32_t>(bytes[8]) << 16) |
                              (static_cast<std::uint32_t>(bytes[9]) << 24);
    EXPECT_EQ(object_id, 8u);

    // 3. Payload bytes verbatim
    std::vector<std::uint8_t> decoded_payload(bytes.begin() + 10, bytes.end());
    EXPECT_EQ(decoded_payload, payload);
}

// =====================================================================
// Golden bytes: lock the byte-level invariant that the sbs harness
// sends. Captured from modern/scratch/sbs_modern_smoke/modern_login.cap
// line 2 (character list ack payload: 0x1F000000 07020000 0000 3132372e302e302e31...)
// =====================================================================

TEST(WireFormat, GoldenBytesIpPortStylePayload) {
    // Mirrors the AgentConnectSuccess / agent-addr payload pattern:
    //   0x1F000000 = object_id=31 (LE)
    //   07020000   = category=7 protocol=2
    //   00 00      = checksum=0 code=0
    //   payload: ASCII IP "127.0.0.1" + padding + port bytes
    std::vector<std::uint8_t> payload = {
        '1', '2', '7', '.', '0', '.', '0', '.', '1',
        0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 0x42, 0x01, 0x00, 0x00, 0x02
    };
    auto bytes = MakePacket(7, 2, 0x1F, 0x00, 0x00, payload).wire_bytes();
    // Total expected wire: 2 (length) + 8 (MSGBASE) + 20 (payload) = 30 bytes
    ASSERT_EQ(bytes.size(), 30u);
    EXPECT_EQ(bytes[0], 0x1C);  // length=28 (8 + 20) low byte
    EXPECT_EQ(bytes[1], 0x00);
    EXPECT_EQ(bytes[2], 0x00);  // checksum
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 0x00);  // code (i8)
    EXPECT_EQ(bytes[4], 7);     // category
    EXPECT_EQ(bytes[5], 2);     // protocol
    EXPECT_EQ(bytes[6], 0x1F);  // object_id LE byte 0
    EXPECT_EQ(bytes[7], 0x00);
    EXPECT_EQ(bytes[8], 0x00);
    EXPECT_EQ(bytes[9], 0x00);
    EXPECT_EQ(bytes[10], '1');  // payload ASCII "127.0.0.1" verbatim
    EXPECT_EQ(bytes[29], 0x02);  // payload last byte
}

TEST(WireFormat, LengthPrefixIsLittleEndian) {
    // 256-byte payload -> bodySize = 8 + 256 = 264 = 0x0108
    // LE encoding: low byte first -> [0x08, 0x01]
    std::vector<std::uint8_t> payload(256, 0xAA);
    auto bytes = MakePacket(7, 0, 0, 0, 0, payload).wire_bytes();
    EXPECT_EQ(bytes[0], 0x08);
    EXPECT_EQ(bytes[1], 0x01);
    EXPECT_EQ(bytes.size(), 266u);
}

TEST(WireFormat, WireFormatIsConsistentWithNetLegacyHeader) {
    // The net.cpp legacy recv path expects:
    //   - 2B length LE
    //   - 8B MSGBASE = [checksum:1][code:1][category:1][protocol:1][object_id:4 LE]
    //   - payload of (length - 8) bytes
    // This test pins the wire_bytes() output to that exact shape, so
    // if either side silently drifts the gate 7.11 fails again.
    std::vector<std::uint8_t> payload = {0x11, 0x22, 0x33};
    auto bytes = MakePacket(31, 200, 0xCAFEBABEu, 0x42, -7, payload).wire_bytes();
    ASSERT_EQ(bytes.size(), 13u);  // 2 + 8 + 3
    // length = 8 + 3 = 11 = 0x0B
    EXPECT_EQ(bytes[0], 0x0B);
    EXPECT_EQ(bytes[1], 0x00);
    // MSGBASE
    EXPECT_EQ(bytes[2], 0x42);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 0xF9);  // -7 in i8 = 0xF9
    EXPECT_EQ(bytes[4], 31);
    EXPECT_EQ(bytes[5], 200);
    // object_id 0xCAFEBABE LE
    EXPECT_EQ(bytes[6], 0xBE);
    EXPECT_EQ(bytes[7], 0xBA);
    EXPECT_EQ(bytes[8], 0xFE);
    EXPECT_EQ(bytes[9], 0xCA);
    // payload
    EXPECT_EQ(bytes[10], 0x11);
    EXPECT_EQ(bytes[11], 0x22);
    EXPECT_EQ(bytes[12], 0x33);
}
