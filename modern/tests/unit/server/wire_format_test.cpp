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
#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace fs = std::filesystem;

fs::path find_golden_dir() {
    fs::path cwd;
    try { cwd = fs::current_path(); } catch (...) { return {}; }
    const std::vector<std::wstring> sub_paths = {
        L"server/golden",
        L"server\\golden",
        L"../server/golden",
        L"modern/tests/unit/server/golden",
        L"tests/unit/server/golden",
    };
    for (fs::path base = cwd; !base.empty(); base = base.parent_path()) {
        for (const auto& sub : sub_paths) {
            std::error_code ec;
            fs::path candidate = base / sub;
            if (fs::is_directory(candidate, ec)) return candidate;
        }
        if (base == base.root_path()) break;
    }
    return {};
}

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
TEST(WireFormatGolden, RoundTrip_auction_request) {
    static const char* kName = "auction_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "auction_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "auction_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "auction_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_auctionboard_request) {
    static const char* kName = "auctionboard_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "auctionboard_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "auctionboard_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "auctionboard_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_autonote_v2_request) {
    static const char* kName = "autonote_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "autonote_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "autonote_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "autonote_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_autopatch_request) {
    static const char* kName = "autopatch_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "autopatch_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "autopatch_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "autopatch_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_battle_request) {
    static const char* kName = "battle_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "battle_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "battle_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "battle_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_bobusang_v2_request) {
    static const char* kName = "bobusang_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "bobusang_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "bobusang_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "bobusang_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_bossmonster_request) {
    static const char* kName = "bossmonster_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "bossmonster_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "bossmonster_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "bossmonster_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_char_request) {
    static const char* kName = "char_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "char_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "char_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "char_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_char_revive_request) {
    static const char* kName = "char_revive_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "char_revive_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "char_revive_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "char_revive_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_cheat_request) {
    static const char* kName = "cheat_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "cheat_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "cheat_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "cheat_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_debug_request) {
    static const char* kName = "debug_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "debug_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "debug_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "debug_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_dist_connect_success) {
    static const char* kName = "dist_connect_success.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "dist_connect_success.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "dist_connect_success.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "dist_connect_success.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_exchange_request) {
    static const char* kName = "exchange_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "exchange_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "exchange_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "exchange_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_fortwar_v2_request) {
    static const char* kName = "fortwar_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "fortwar_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "fortwar_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "fortwar_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_friend_request) {
    static const char* kName = "friend_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "friend_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "friend_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "friend_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_gtournament_v2_request) {
    static const char* kName = "gtournament_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "gtournament_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "gtournament_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "gtournament_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_guild_fieldwar_request) {
    static const char* kName = "guild_fieldwar_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "guild_fieldwar_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "guild_fieldwar_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "guild_fieldwar_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_guild_request) {
    static const char* kName = "guild_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "guild_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "guild_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "guild_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_guild_v2_request) {
    static const char* kName = "guild_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "guild_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "guild_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "guild_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_guildunion_v2_request) {
    static const char* kName = "guildunion_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "guildunion_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "guildunion_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "guildunion_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_hackcheck_request) {
    static const char* kName = "hackcheck_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "hackcheck_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "hackcheck_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "hackcheck_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_hackshield_request) {
    static const char* kName = "hackshield_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "hackshield_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "hackshield_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "hackshield_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_item_request) {
    static const char* kName = "item_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "item_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "item_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "item_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_itemext_request) {
    static const char* kName = "itemext_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "itemext_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "itemext_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "itemext_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_itemlimit_v2_request) {
    static const char* kName = "itemlimit_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "itemlimit_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "itemlimit_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "itemlimit_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_jackpot_request) {
    static const char* kName = "jackpot_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "jackpot_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "jackpot_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "jackpot_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_journal_request) {
    static const char* kName = "journal_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "journal_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "journal_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "journal_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_kyunggong_request) {
    static const char* kName = "kyunggong_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "kyunggong_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "kyunggong_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "kyunggong_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_large_payload_cat8_request) {
    static const char* kName = "large_payload_cat8_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "large_payload_cat8_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "large_payload_cat8_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "large_payload_cat8_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_login_ack) {
    static const char* kName = "login_ack.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "login_ack.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "login_ack.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "login_ack.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_login_ack_enc) {
    static const char* kName = "login_ack_enc.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "login_ack_enc.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "login_ack_enc.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "login_ack_enc.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_login_nack) {
    static const char* kName = "login_nack.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "login_nack.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "login_nack.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "login_nack.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_login_request) {
    static const char* kName = "login_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "login_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "login_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "login_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_monster_request) {
    static const char* kName = "monster_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "monster_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "monster_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "monster_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_mornitormapserver_request) {
    static const char* kName = "mornitormapserver_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "mornitormapserver_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "mornitormapserver_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "mornitormapserver_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_mornitorserver_request) {
    static const char* kName = "mornitorserver_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "mornitorserver_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "mornitorserver_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "mornitorserver_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_mornitortool_request) {
    static const char* kName = "mornitortool_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "mornitortool_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "mornitortool_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "mornitortool_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_mugong_request) {
    static const char* kName = "mugong_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "mugong_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "mugong_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "mugong_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_munpa_request) {
    static const char* kName = "munpa_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "munpa_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "munpa_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "munpa_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_murimnet_request) {
    static const char* kName = "murimnet_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "murimnet_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "murimnet_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "murimnet_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_note_request) {
    static const char* kName = "note_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "note_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "note_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "note_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_npc_request) {
    static const char* kName = "npc_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "npc_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "npc_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "npc_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_option_request) {
    static const char* kName = "option_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "option_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "option_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "option_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_packeddata_request) {
    static const char* kName = "packeddata_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "packeddata_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "packeddata_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "packeddata_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_party_request) {
    static const char* kName = "party_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "party_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "party_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "party_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_partywar_request) {
    static const char* kName = "partywar_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "partywar_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "partywar_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "partywar_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_partywar_v2_request) {
    static const char* kName = "partywar_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "partywar_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "partywar_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "partywar_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_peacewarmode_request) {
    static const char* kName = "peacewarmode_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "peacewarmode_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "peacewarmode_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "peacewarmode_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_pet_request) {
    static const char* kName = "pet_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "pet_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "pet_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "pet_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_pet_v2_request) {
    static const char* kName = "pet_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "pet_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "pet_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "pet_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_pk_request) {
    static const char* kName = "pk_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "pk_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "pk_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "pk_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_powerup_request) {
    static const char* kName = "powerup_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "powerup_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "powerup_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "powerup_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_pyoguk_request) {
    static const char* kName = "pyoguk_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "pyoguk_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "pyoguk_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "pyoguk_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_quest_request) {
    static const char* kName = "quest_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "quest_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "quest_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "quest_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_quick_request) {
    static const char* kName = "quick_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "quick_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "quick_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "quick_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_admin_request) {
    static const char* kName = "rmtool_admin_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_admin_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_admin_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_admin_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_character_request) {
    static const char* kName = "rmtool_character_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_character_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_character_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_character_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_connect_request) {
    static const char* kName = "rmtool_connect_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_connect_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_connect_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_connect_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_gamelog_request) {
    static const char* kName = "rmtool_gamelog_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_gamelog_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_gamelog_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_gamelog_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_item_request) {
    static const char* kName = "rmtool_item_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_item_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_item_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_item_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_munpa_request) {
    static const char* kName = "rmtool_munpa_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_munpa_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_munpa_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_munpa_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_operlog_request) {
    static const char* kName = "rmtool_operlog_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_operlog_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_operlog_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_operlog_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_pet_request) {
    static const char* kName = "rmtool_pet_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_pet_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_pet_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_pet_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_statistics_request) {
    static const char* kName = "rmtool_statistics_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_statistics_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_statistics_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_statistics_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_rmtool_user_request) {
    static const char* kName = "rmtool_user_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "rmtool_user_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "rmtool_user_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "rmtool_user_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_server_request) {
    static const char* kName = "server_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "server_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "server_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "server_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_siegewar_castle_request) {
    static const char* kName = "siegewar_castle_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "siegewar_castle_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "siegewar_castle_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "siegewar_castle_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_siegewar_profit_request) {
    static const char* kName = "siegewar_profit_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "siegewar_profit_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "siegewar_profit_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "siegewar_profit_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_siegewar_request) {
    static const char* kName = "siegewar_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "siegewar_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "siegewar_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "siegewar_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_signal_request) {
    static const char* kName = "signal_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "signal_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "signal_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "signal_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_sim_bub_request) {
    static const char* kName = "sim_bub_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "sim_bub_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "sim_bub_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "sim_bub_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_skill_request) {
    static const char* kName = "skill_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "skill_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "skill_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "skill_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_societyact_request) {
    static const char* kName = "societyact_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "societyact_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "societyact_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "societyact_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_titan_request) {
    // D4.R2 gap-fill: titan (cat=72) had a side-effect plan and data
    // plane but no wire-format golden. Locks the 18-byte legacy frame
    // shape (2B length + 8B header + 8B payload) so any drift in the
    // modern Packet::wire_bytes() encoder is caught by round-trip.
    static const char* kName = "titan_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "titan_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "titan_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "titan_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
    EXPECT_EQ(p_in.category, 72u);
}

TEST(WireFormatGolden, RoundTrip_streetstall_request) {
    static const char* kName = "streetstall_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "streetstall_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "streetstall_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "streetstall_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_suryun_request) {
    static const char* kName = "suryun_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "suryun_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "suryun_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "suryun_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_suryun_v2_request) {
    static const char* kName = "suryun_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "suryun_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "suryun_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "suryun_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_tactic_request) {
    static const char* kName = "tactic_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "tactic_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "tactic_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "tactic_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_ungijosik_request) {
    static const char* kName = "ungijosik_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "ungijosik_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "ungijosik_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "ungijosik_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_unknown_category_cat4_request) {
    static const char* kName = "unknown_category_cat4_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "unknown_category_cat4_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "unknown_category_cat4_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "unknown_category_cat4_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_unknown_category_cat6_request) {
    static const char* kName = "unknown_category_cat6_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "unknown_category_cat6_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "unknown_category_cat6_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "unknown_category_cat6_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_unknown_category_request) {
    static const char* kName = "unknown_category_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "unknown_category_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "unknown_category_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "unknown_category_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_wanted_request) {
    static const char* kName = "wanted_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "wanted_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "wanted_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "wanted_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_wanted_v2_request) {
    static const char* kName = "wanted_v2_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "wanted_v2_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "wanted_v2_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "wanted_v2_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

TEST(WireFormatGolden, RoundTrip_weather_request) {
    static const char* kName = "weather_request.bin";
    const auto dir = find_golden_dir();
    if (dir.empty()) GTEST_SKIP() << "weather_request.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    std::ifstream in(p, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), 10u) << "weather_request.bin too small";
    Packet p_in;
    p_in.checksum  = bytes[2];
    p_in.code      = static_cast<std::int8_t>(bytes[3]);
    p_in.category  = bytes[4];
    p_in.protocol  = bytes[5];
    p_in.object_id = static_cast<std::uint32_t>(bytes[6]) | (static_cast<std::uint32_t>(bytes[7]) << 8) | (static_cast<std::uint32_t>(bytes[8]) << 16) | (static_cast<std::uint32_t>(bytes[9]) << 24);
    p_in.payload.assign(bytes.begin() + 10, bytes.end());
    auto reencoded = p_in.wire_bytes();
    EXPECT_EQ(reencoded, bytes) << "weather_request.bin round-trip failed";
    EXPECT_EQ(reencoded.size(), bytes.size());
    EXPECT_EQ(p_in.payload.size() + 10u, reencoded.size());
}

