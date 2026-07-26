#include <gtest/gtest.h>
#include <cstring>
#include "../../../tools/MoxianSideBySide/packet.hpp"
#include "../../../tools/MoxianSideBySide/replay/replay.hpp"
#include "../../../tools/MoxianSideBySide/capture/packet_capture.hpp"
#include "../../../tools/MoxianSideBySide/diff/packet_diff.hpp"

#include <filesystem>

using namespace mxh::tools::sidebyside;

namespace {
Packet make_packet(std::uint32_t objectId = 0x11223344u) {
    Packet p;
    p.checksum = 0xA1;
    p.code = 2;
    p.category = 7;
    p.protocol = 12;
    p.object_id = objectId;
    p.payload = {0x10, 0x20, 0x30};
    p.length = static_cast<std::uint32_t>(p.payload.size());
    p.direction = "s->c";
    p.timestamp_ns = 123456;
    return p;
}

TEST(SideBySidePacket, WireLayoutUsesLegacyLengthAndEightByteHeader) {
    const auto bytes = make_packet().wire_bytes();
    ASSERT_EQ(bytes.size(), 13u);
    EXPECT_EQ(bytes[0], 11u);
    EXPECT_EQ(bytes[1], 0u);
    EXPECT_EQ(bytes[6], 0x44u);
    EXPECT_EQ(bytes[7], 0x33u);
    EXPECT_EQ(bytes[8], 0x22u);
    EXPECT_EQ(bytes[9], 0x11u);
    EXPECT_EQ(bytes[10], 0x10u);
}

TEST(SideBySideCapture, SaveAndLoadRoundTripPreservesPacket) {
    const auto path = std::filesystem::temp_directory_path() / "mxh_sbs_roundtrip.cap";
    const auto original = make_packet();
    ASSERT_TRUE(save_capture({original}, path.string()));
    const auto loaded = load_capture(path.string());
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].wire_bytes(), original.wire_bytes());
    EXPECT_EQ(loaded[0].direction, original.direction);
    EXPECT_EQ(loaded[0].timestamp_ns, original.timestamp_ns);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(SideBySideDiff, IdenticalTracesHaveNoDiff) {
    const auto p = make_packet();
    EXPECT_TRUE(diff_traces({p}, {p}, {}).empty());
}

TEST(SideBySideDiff, ReportsFirstPayloadByteDifference) {
    auto a = make_packet();
    auto b = a;
    b.payload[1] = 0x99;
    const auto diffs = diff_traces({a}, {b}, {});
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].first_diff_offset, 11u);
    EXPECT_EQ(diffs[0].expected_byte, 0x20u);
    EXPECT_EQ(diffs[0].actual_byte, 0x99u);
}

TEST(SideBySideDiff, ReportsLengthDifferenceAtEnd) {
    auto a = make_packet();
    auto b = a;
    b.payload.pop_back();
    b.length = static_cast<std::uint32_t>(b.payload.size());
    const auto diffs = diff_traces({a}, {b}, {});
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].first_diff_offset, 12u);
}

TEST(SideBySideDiff, CanIgnoreObjectIdOnly) {
    auto a = make_packet(1u);
    auto b = make_packet(2u);
    EXPECT_FALSE(diff_traces({a}, {b}, {}).empty());
    DiffOptions options;
    options.ignore_object_id = true;
    EXPECT_TRUE(diff_traces({a}, {b}, options).empty());
}

TEST(SideBySideDiff, ReportsMissingPacketByTraceLength) {
    const auto p = make_packet();
    const auto diffs = diff_traces({p}, {}, {});
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].index, 0u);
}

TEST(SideBySideDiff, IgnoreTraceLengthMismatchSuppressesLengthDiff) {
    Packet a = make_packet();
    Packet b = make_packet();
    DiffOptions opt;
    opt.ignore_trace_length_mismatch = true;
    // a empty, b has 1 packet -> normally triggers a 'length mismatch' diff
    EXPECT_FALSE(diff_traces({}, {b}, {}).empty());
    EXPECT_TRUE(diff_traces({}, {b}, opt).empty());
    EXPECT_TRUE(diff_traces({a}, {}, opt).empty());
}

TEST(SideBySideReplay, LoginScenarioHas38BytePayload) {
    const auto s = login_scenario();
    ASSERT_FALSE(s.client_packets.empty());
    const auto& p = s.client_packets.front();
    EXPECT_EQ(p.category, 7u);  // MP_USERCONN
    EXPECT_EQ(p.protocol, 1u);  // RequestLogin
    EXPECT_EQ(p.payload.size(), 38u);  // [AuthKey:u32][id:17][pw:17]
    std::uint32_t auth = 0;
    std::memcpy(&auth, p.payload.data(), 4);
    EXPECT_EQ(auth, 1000u);
}

TEST(SideBySideReplay, EnterGameScenarioIsTwoStep) {
    const auto s = enter_game_scenario();
    ASSERT_EQ(s.client_packets.size(), 2u);
    EXPECT_EQ(s.client_packets[0].protocol, 16u);  // CharacterSelectSyn
    EXPECT_EQ(s.client_packets[1].protocol, 28u);  // GameInSyn
}

TEST(SideBySideReplay, AttackShopQuestScenariosHaveFixedSizes) {
    EXPECT_EQ(attack_scenario().client_packets.front().payload.size(), 6u);  // skill+u16+target+u32
    EXPECT_EQ(shop_scenario().client_packets.front().payload.size(), 4u);    // item+u16+qty+u16
    EXPECT_EQ(quest_scenario().client_packets.front().payload.size(), 2u);   // quest+u16
}

}

