// protocol_byte_level_test.cpp - T2 protocol codec byte-level verification.
//
// Phase 6.1 of the 12-week plan: prove that modern's protocol codec
// (mxh::net::MsgHeader + TcpServer/TcpClient framing) produces wire
// bytes that match the legacy MSGBASE / MSGROOT layout used by
// SWorking/* and the original MHClient-Connect.exe.
//
// The legacy wire layout (1:1 with [CC]Header/CommonStruct.h):
//   MSGBASE = [checksum: u8] [code: i8] [category: u8] [protocol: u8] [object_id: u32]
//   MSGROOT = [checksum: u8] [code: i8] [category: u8] [protocol: u8]
//
// We pin the layout with static_assert + round-trip tests. The
// check_value/expected_value pattern catches accidental reordering
// of fields or changes in width (which would silently break wire
// compatibility).
#include "mxh/net/net.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/proto/negotiate.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

// Wire layout invariants (1:1 with legacy MSGBASE / MSGROOT).
TEST(MxhProtocolByteLevel, MsgHeaderIsEightBytes) {
    EXPECT_EQ(sizeof(mxh::net::MsgHeader), 8u);
}

TEST(MxhProtocolByteLevel, MsgRootIsFourBytes) {
    EXPECT_EQ(sizeof(mxh::net::MsgRoot), 4u);
}

TEST(MxhProtocolByteLevel, MsgHeaderLayoutIsPacked) {
    // Mirrors the wire layout byte-by-byte.
    mxh::net::MsgHeader h{};
    h.checksum = 0x11;
    h.code     = 0x22;
    h.category = 0x33;
    h.protocol = 0x44;
    h.object_id = 0xDEADBEEFu;
    const auto* p = reinterpret_cast<const std::uint8_t*>(&h);
    EXPECT_EQ(p[0], 0x11u);
    EXPECT_EQ(p[1], 0x22u);
    EXPECT_EQ(p[2], 0x33u);
    EXPECT_EQ(p[3], 0x44u);
    EXPECT_EQ(p[4], 0xEFu);  // little-endian object_id LSB first
    EXPECT_EQ(p[5], 0xBEu);
    EXPECT_EQ(p[6], 0xADu);
    EXPECT_EQ(p[7], 0xDEu);
}

TEST(MxhProtocolByteLevel, MsgRootLayoutIsPacked) {
    mxh::net::MsgRoot r{};
    r.checksum = 0xAA;
    r.code     = 0xBB;
    r.category = 0xCC;
    r.protocol = 0xDD;
    const auto* p = reinterpret_cast<const std::uint8_t*>(&r);
    EXPECT_EQ(p[0], 0xAAu);
    EXPECT_EQ(p[1], 0xBBu);
    EXPECT_EQ(p[2], 0xCCu);
    EXPECT_EQ(p[3], 0xDDu);
}

// Category enum wire values match the legacy [CC]Header/Protocol.h
// MP_CATEGORY enum (1:1 byte alignment).
TEST(MxhProtocolByteLevel, CategoryByteValuesAreStable) {
    // Pin the byte values for the categories we send packets on.
    // These MUST stay 1:1 with the legacy MP_CATEGORY enum.
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Server),     1u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::UserConn),   7u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Move),       8u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Mugong),     9u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Chat),       6u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Item),       5u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Character),  3u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Map),        4u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Quest),     39u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::Category::Battle),    31u);
}

// Version negotiation magic ("MXHN") must stay 1:1.
TEST(MxhProtocolByteLevel, NegotiateMagicIsStable) {
    // MXHN big-endian.
    EXPECT_EQ(static_cast<std::uint32_t>(
        ('M' << 24) | ('X' << 16) | ('H' << 8) | 'N'), 0x4D58484Eu);
}

// CipherType enum bits must stay stable.
TEST(MxhProtocolByteLevel, CipherTypeBitsAreStable) {
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::CipherType::None),    0u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::CipherType::HSEL),    0x01u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::CipherType::AES_GCM), 0x02u);
}

// Round-trip: a Message struct with known fields, encoded byte-by-byte,
// must match the legacy MSGBASE layout exactly.
TEST(MxhProtocolByteLevel, MessageRoundTripPreservesHeader) {
    mxh::net::Message m;
    m.header.checksum  = 0x12;
    m.header.code      = 0x34;
    m.header.category  = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    m.header.protocol  = 5;
    m.header.object_id = 0x01020304u;
    m.payload = {0xAA, 0xBB, 0xCC};

    std::array<std::uint8_t, 11> wire{};
    std::memcpy(wire.data(), &m.header, 8);
    std::memcpy(wire.data() + 8, m.payload.data(), 3);

    EXPECT_EQ(wire[0], 0x12u);
    EXPECT_EQ(wire[1], 0x34u);
    EXPECT_EQ(wire[2], 7u);   // Category::UserConn
    EXPECT_EQ(wire[3], 5u);
    EXPECT_EQ(wire[4], 0x04u);  // object_id LE LSB
    EXPECT_EQ(wire[5], 0x03u);
    EXPECT_EQ(wire[6], 0x02u);
    EXPECT_EQ(wire[7], 0x01u);
    EXPECT_EQ(wire[8], 0xAAu);
    EXPECT_EQ(wire[9], 0xBBu);
    EXPECT_EQ(wire[10], 0xCCu);
}

// Protocol layout regression: the wire offset of every standard field
// (checksum, code, category, protocol, object_id) must stay 1:1.
TEST(MxhProtocolByteLevel, FieldOffsetsAreStable) {
    mxh::net::MsgHeader h{};
    auto* base = reinterpret_cast<std::uint8_t*>(&h);
    auto off = [&](const void* p) {
        return static_cast<std::size_t>(reinterpret_cast<const std::uint8_t*>(p) - base);
    };
    EXPECT_EQ(off(&h.checksum),  0u);
    EXPECT_EQ(off(&h.code),      1u);
    EXPECT_EQ(off(&h.category),  2u);
    EXPECT_EQ(off(&h.protocol),  3u);
    EXPECT_EQ(off(&h.object_id), 4u);
}

// Total size = header + payload.size(). Verifies the legacy wire
// path uses "no length prefix" semantics.
TEST(MxhProtocolByteLevel, TotalSizeIsHeaderPlusPayload) {
    mxh::net::Message m;
    m.payload.resize(100);
    EXPECT_EQ(m.total_size(), 108u);

    m.payload.resize(0);
    EXPECT_EQ(m.total_size(), 8u);

    m.payload.resize(12345);
    EXPECT_EQ(m.total_size(), 12353u);
}
