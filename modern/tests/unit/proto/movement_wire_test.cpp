#include "mxh/proto/movement_wire.hpp"
#include "mxh/game/movement_timeline.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <limits>

namespace wire = mxh::proto::movement;
static_assert(wire::max_points == mxh::game::MovementTimeline::max_route_points);

namespace {
wire::Command command_fixture() {
    wire::Command c;
    c.epoch=0x0807060504030201ULL; c.sequence=0x1817161514131211ULL;
    c.count=1; c.points[0]={0x1234,0x5678}; return c;
}
wire::State state_fixture() {
    wire::State s;
    s.kind=wire::StateKind::Started; s.epoch=1; s.command_sequence=2;
    s.state_sequence=3; s.server_time_ms=4; s.x=1000; s.z=1000; s.speed=400;
    s.count=1; s.points[0]={1400,1000}; return s;
}
}

TEST(MovementWire, CommandGoldenPinsLittleEndianLayoutWithoutNativeStructPacking) {
    const auto bytes=wire::encode(command_fixture()); ASSERT_TRUE(bytes);
    const std::vector<std::uint8_t> expected{
        'M','X','M','C',1,1,1,0,
        1,2,3,4,5,6,7,8, 0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
        0x34,0x12,0x78,0x56};
    EXPECT_EQ(*bytes,expected);
    const auto decoded=wire::decode_command(expected); ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->epoch,0x0807060504030201ULL);
    EXPECT_EQ(decoded->sequence,0x1817161514131211ULL);
    EXPECT_EQ(decoded->points[0].x,0x1234); EXPECT_EQ(decoded->points[0].z,0x5678);
}

TEST(MovementWire, StateGoldenPinsAuthorityClockSequenceAndIeeeFloatFields) {
    const std::vector<std::uint8_t> expected{
        'M','X','M','S',1,1,1,0,
        1,0,0,0,0,0,0,0, 2,0,0,0,0,0,0,0,
        3,0,0,0,0,0,0,0, 4,0,0,0,0,0,0,0,
        0,0,0x7a,0x44, 0,0,0x7a,0x44, 0,0,0xc8,0x43,
        0x78,0x05,0xe8,0x03};
    auto bytes=wire::encode(state_fixture()); ASSERT_TRUE(bytes); EXPECT_EQ(*bytes,expected);
    auto s=wire::decode_state(expected); ASSERT_TRUE(s);
    EXPECT_FLOAT_EQ(s->x,1000); EXPECT_FLOAT_EQ(s->speed,400);
    EXPECT_EQ(s->server_time_ms,4u); EXPECT_EQ(s->state_sequence,3u);
    EXPECT_EQ(s->points[0].x,1400);
}

TEST(MovementWire, RejectsEveryTruncationTrailingBytesWrongTypeVersionAndReservedBits) {
    const auto command=*wire::encode(command_fixture());
    const auto state=*wire::encode(state_fixture());
    for (std::size_t n=0;n<command.size();++n)
        EXPECT_FALSE(wire::decode_command(std::span<const std::uint8_t>(command.data(),n)));
    for (std::size_t n=0;n<state.size();++n)
        EXPECT_FALSE(wire::decode_state(std::span<const std::uint8_t>(state.data(),n)));
    auto extra=command; extra.push_back(0); EXPECT_FALSE(wire::decode_command(extra));
    extra=state; extra.push_back(0); EXPECT_FALSE(wire::decode_state(extra));
    EXPECT_FALSE(wire::decode_command(state)); EXPECT_FALSE(wire::decode_state(command));
    for (const auto offset : {0u,3u,4u,5u,6u,7u}) {
        auto bad=command; bad[offset]=255; EXPECT_FALSE(wire::decode_command(bad));
        bad=state; bad[offset]=255; EXPECT_FALSE(wire::decode_state(bad));
    }
    EXPECT_FALSE(wire::decode_command(std::array<std::uint8_t,4>{1,0,2,0}));
}

TEST(MovementWire, RouteAndStopCountsBoundsAndIdentityAreStrict) {
    auto c=command_fixture(); c.count=15; c.points.fill({51199,51199});
    auto bytes=wire::encode(c); ASSERT_TRUE(bytes); EXPECT_EQ(bytes->size(),84u);
    auto parsed=wire::decode_command(*bytes); ASSERT_TRUE(parsed); EXPECT_EQ(parsed->count,15);
    c.count=16; EXPECT_FALSE(wire::encode(c));
    c.count=0; EXPECT_FALSE(wire::encode(c));
    c.count=1; c.points[0].x=51200; EXPECT_FALSE(wire::encode(c));
    c=command_fixture(); c.kind=wire::CommandKind::Stop;
    bytes=wire::encode(c); ASSERT_TRUE(bytes);
    EXPECT_EQ(wire::decode_command(*bytes)->kind,wire::CommandKind::Stop);
    c.count=2; EXPECT_FALSE(wire::encode(c));
    c=command_fixture(); c.epoch=0; EXPECT_FALSE(wire::encode(c));
    c=command_fixture(); c.sequence=0; EXPECT_FALSE(wire::encode(c));
    c=command_fixture(); c.epoch=c.sequence=std::numeric_limits<std::uint64_t>::max();
    bytes=wire::encode(c); ASSERT_TRUE(bytes);
    EXPECT_EQ(wire::decode_command(*bytes)->sequence,std::numeric_limits<std::uint64_t>::max());
    // Decoder must enforce semantics independently of the encoder.
    auto bad=*bytes; std::fill(bad.begin()+8,bad.begin()+16,0);
    EXPECT_FALSE(wire::decode_command(bad));
    bad=*bytes; bad[24]=0; bad[25]=200; // x=51200
    EXPECT_FALSE(wire::decode_command(bad));
}

TEST(MovementWire, StateRejectsImpossibleMotionAndNonfiniteAuthority) {
    for (float value : {-1.0f,51101.0f,std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
        auto s=state_fixture(); s.x=value; EXPECT_FALSE(wire::encode(s));
        s=state_fixture(); s.z=value; EXPECT_FALSE(wire::encode(s));
    }
    for (float speed : {0.0f,-1.0f,std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
        auto s=state_fixture(); s.speed=speed; EXPECT_FALSE(wire::encode(s));
    }
    auto s=state_fixture(); s.count=0; EXPECT_FALSE(wire::encode(s));
    s=state_fixture(); s.kind=wire::StateKind::Stopped; EXPECT_FALSE(wire::encode(s));
    s.count=0; s.speed=0; ASSERT_TRUE(wire::encode(s));
    s.kind=wire::StateKind::Corrected; ASSERT_TRUE(wire::encode(s));
    s.command_sequence=0; EXPECT_FALSE(wire::encode(s));
    s.kind=wire::StateKind::Snapshot; ASSERT_TRUE(wire::encode(s));
    s.state_sequence=0; EXPECT_FALSE(wire::encode(s));
    s=state_fixture(); s.epoch=0; EXPECT_FALSE(wire::encode(s));
    const auto bytes=*wire::encode(state_fixture());
    for (const auto offset : {40u,44u,48u}) {
        auto bad=bytes; bad[offset]=0; bad[offset+1]=0;
        bad[offset+2]=0x80; bad[offset+3]=0x7f;
        EXPECT_FALSE(wire::decode_state(bad));
    }
}

TEST(MovementWire, FullRouteSnapshotPreservesLargeClocksAndFractionalPosition) {
    auto s=state_fixture(); s.kind=wire::StateKind::Snapshot;
    s.server_time_ms=0x12345678ffffffffULL; s.state_sequence=0x1234567800000001ULL;
    s.x=1234.25f; s.z=4321.5f; s.count=15;
    for (std::size_t i=0;i<15;++i) s.points[i]={static_cast<std::uint16_t>(1000+i),2000};
    const auto bytes=wire::encode(s); ASSERT_TRUE(bytes); EXPECT_EQ(bytes->size(),112u);
    const auto parsed=wire::decode_state(*bytes); ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->server_time_ms,s.server_time_ms); EXPECT_EQ(parsed->state_sequence,s.state_sequence);
    EXPECT_FLOAT_EQ(parsed->x,s.x); EXPECT_FLOAT_EQ(parsed->z,s.z);
    for (std::size_t i=0;i<15;++i) EXPECT_EQ(parsed->points[i].x,1000+i);
}

TEST(MovementWire, OneTargetCommandKindIsRoundTripSafeAndCountStrict) {
    auto c=command_fixture();
    c.kind=wire::CommandKind::OneTarget;
    c.count=1; c.points[0]={0xabcd,0x1234};
    auto bytes=wire::encode(c); ASSERT_TRUE(bytes);
    EXPECT_EQ(bytes->size(),wire::command_header_size+4u);
    EXPECT_EQ(bytes->at(5),static_cast<std::uint8_t>(wire::CommandKind::OneTarget));
    auto parsed=wire::decode_command(*bytes); ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->kind,wire::CommandKind::OneTarget);
    EXPECT_EQ(parsed->points[0].x,0xabcd); EXPECT_EQ(parsed->points[0].z,0x1234);

    // OneTarget must carry exactly one point; multi-point routes are reserved
    // for Route. Two-point input must round-trip through a parse failure.
    c.count=2; c.points[1]={0x1111,0x2222};
    EXPECT_FALSE(wire::encode(c));
}

TEST(MovementWire, HelloPayloadIsLiteralEightBytesAndVersionPinned) {
    // Hello payload bytes are a public constexpr; any drift in their values
    // breaks the on-wire handshake. Pin the exact layout.
    const std::array<std::uint8_t,8> expected{
        'M','X','M','H', wire::version, 0, 0, 0};
    EXPECT_EQ(wire::hello_payload, expected);
    EXPECT_EQ(wire::hello_payload.size(),8u);
    // Re-decode the hello header through the command codec with count=0 to
    // confirm it parses cleanly as a four-byte-MXMH prefix even though it is
    // not a valid Command.
    auto header_only=std::vector<std::uint8_t>(wire::hello_payload.begin(),
                                               wire::hello_payload.begin()+8);
    EXPECT_FALSE(wire::decode_command(header_only));
    // But the protocol discriminators must remain distinct.
    EXPECT_NE(wire::hello_protocol, wire::command_protocol);
    EXPECT_NE(wire::command_protocol, wire::owner_state_protocol);
    EXPECT_NE(wire::owner_state_protocol, wire::observer_state_protocol);
    EXPECT_GE(wire::hello_protocol,128u);
    EXPECT_LE(wire::observer_state_protocol,255u);
}

TEST(MovementWire, AllThreeCommandKindsRejectCountZeroAndRejectInvalidKind) {
    for (auto kind : {wire::CommandKind::Route,
                      wire::CommandKind::Stop,
                      wire::CommandKind::OneTarget}) {
        auto c=command_fixture();
        c.kind=kind;
        c.count=0;
        EXPECT_FALSE(wire::encode(c));
    }
    auto c=command_fixture();
    c.kind=static_cast<wire::CommandKind>(99);
    EXPECT_FALSE(wire::encode(c));
}
