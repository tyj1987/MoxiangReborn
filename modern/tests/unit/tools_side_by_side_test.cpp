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

TEST(SideBySideDiff, ReportsLengthPrefixDifferenceBeforePayload) {
    auto a = make_packet();
    auto b = a;
    b.payload.pop_back();
    b.length = static_cast<std::uint32_t>(b.payload.size());
    const auto diffs = diff_traces({a}, {b}, {});
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].first_diff_offset, 0u);
}

TEST(SideBySideDiff, CanIgnoreLengthPrefixForDiagnosticMode) {
    auto a = make_packet();
    auto b = a;
    b.payload.pop_back();
    DiffOptions options;
    options.ignore_length_prefix = true;
    const auto diffs = diff_traces({a}, {b}, options);
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

TEST(SideBySideDiff, ObjectIdMaskDoesNotHideCategoryOrProtocol) {
    auto a = make_packet(1u);
    auto b = make_packet(2u);
    b.category = a.category + 1;
    DiffOptions options;
    options.ignore_object_id = true;
    const auto diffs = diff_traces({a}, {b}, options);
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].first_diff_offset, 4u);

    b = a;
    b.protocol = a.protocol + 1;
    const auto protocolDiffs = diff_traces({a}, {b}, options);
    ASSERT_EQ(protocolDiffs.size(), 1u);
    EXPECT_EQ(protocolDiffs[0].first_diff_offset, 5u);
}

TEST(SideBySideDiff, PayloadIgnoreOffsetsStartAfterTenWireBytes) {
    auto a = make_packet();
    auto b = a;
    b.payload[0] = 0x99;
    DiffOptions options;
    options.ignore_payload_offsets = {0};
    EXPECT_TRUE(diff_traces({a}, {b}, options).empty());
}

TEST(SideBySideDiff, LengthPrefixIsComparedByDefault) {
    auto a = make_packet();
    auto b = a;
    b.payload.push_back(0x40);
    const auto diffs = diff_traces({a}, {b}, {});
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].first_diff_offset, 0u);
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
    EXPECT_EQ(s.endpoint, ReplayEndpoint::Agent);
    EXPECT_EQ(s.client_packets[0].protocol, 16u);  // CharacterSelectSyn
    EXPECT_EQ(s.client_packets[1].protocol, 28u);  // GameInSyn
}

TEST(SideBySideReplay, ScenariosRouteToExpectedServer) {
    EXPECT_EQ(login_scenario().endpoint, ReplayEndpoint::Login);
    EXPECT_EQ(attack_scenario().endpoint, ReplayEndpoint::Map);
    EXPECT_EQ(shop_scenario().endpoint, ReplayEndpoint::Map);
    EXPECT_EQ(quest_scenario().endpoint, ReplayEndpoint::Map);
    EXPECT_EQ(chat_scenario().endpoint, ReplayEndpoint::Map);
    EXPECT_STREQ(endpoint_name(ReplayEndpoint::Agent), "agent");
}

TEST(SideBySideReplay, AttackShopQuestChatScenariosHaveFixedSizes) {
    // attack: cat=Skill(22), proto=StartSyn(0),
    //   payload=[skill_idx:u32][main_target:u32][target_x:f32][target_z:f32] = 16B.
    EXPECT_EQ(attack_scenario().client_packets.front().category, 22u);
    EXPECT_EQ(attack_scenario().client_packets.front().protocol, 0u);
    EXPECT_EQ(attack_scenario().client_packets.front().payload.size(), 16u);
    // shop: cat=Item(5), proto=BuySyn(22), payload=[item:u16][qty:u16] = 4B.
    EXPECT_EQ(shop_scenario().client_packets.front().category, 5u);
    EXPECT_EQ(shop_scenario().client_packets.front().protocol, 22u);
    EXPECT_EQ(shop_scenario().client_packets.front().payload.size(), 4u);
    // quest: cat=Quest(39), proto=StartSyn(9), payload=[quest:u16] = 2B.
    EXPECT_EQ(quest_scenario().client_packets.front().category, 39u);
    EXPECT_EQ(quest_scenario().client_packets.front().protocol, 9u);
    EXPECT_EQ(quest_scenario().client_packets.front().payload.size(), 2u);
    // chat: cat=Chat(6), proto=All(0), payload="hello" = 5B.
    EXPECT_EQ(chat_scenario().client_packets.front().category, 6u);
    EXPECT_EQ(chat_scenario().client_packets.front().protocol, 0u);
    EXPECT_EQ(chat_scenario().client_packets.front().payload.size(), 5u);
    // Wire-bytes check: "hello" as ASCII (0x68 0x65 0x6c 0x6c 0x6f).
    const auto chatPayload = chat_scenario().client_packets.front().payload;
    ASSERT_EQ(chatPayload.size(), 5u);
    EXPECT_EQ(chatPayload[0], 0x68u);
    EXPECT_EQ(chatPayload[1], 0x65u);
    EXPECT_EQ(chatPayload[2], 0x6cu);
    EXPECT_EQ(chatPayload[3], 0x6cu);
    EXPECT_EQ(chatPayload[4], 0x6fu);
}

// Modern-only golden captures live in modern/tests/fixtures/sbs_captures_modern/.
// They are produced by running mxh_side_by_side --modern-only --start and
// verified here so the modern MapServer protocol coverage does not regress.
TEST(SideBySideModernGolden, ChatScenarioNameIsChat) {
    EXPECT_STREQ(chat_scenario().name.c_str(), "chat");
}

TEST(SideBySideModernGolden, AllFiveScenariosHaveFixtures) {
    const std::filesystem::path dir =
        "C:/moxiang/modern/tests/fixtures/sbs_captures_modern";
    for (const char* name : {"modern_login.cap", "modern_enter_game.cap",
                             "modern_attack.cap", "modern_shop.cap",
                             "modern_quest.cap"}) {
        const auto p = dir / name;
        ASSERT_TRUE(std::filesystem::exists(p)) << "missing fixture " << name;
        const auto trace = load_capture(p.string());
        ASSERT_FALSE(trace.empty()) << "empty fixture " << name;
    }
}

TEST(SideBySideModernGolden, LoginTraceIsDistSuccessThenAck) {
    const auto trace = load_capture(
        "C:/moxiang/modern/tests/fixtures/sbs_captures_modern/modern_login.cap");
    ASSERT_EQ(trace.size(), 2u);
    EXPECT_EQ(trace[0].category, 7u);   // UserConn
    EXPECT_EQ(trace[0].protocol, 0u);   // DistConnectSuccess
    EXPECT_EQ(trace[0].object_id, 1000u);
    EXPECT_EQ(trace[1].category, 7u);   // UserConn
    EXPECT_EQ(trace[1].protocol, 2u);   // NotifyUserLoginAck
}

TEST(SideBySideModernGolden, EnterGameTraceIsAgentConnectThenNack) {
    const auto trace = load_capture(
        "C:/moxiang/modern/tests/fixtures/sbs_captures_modern/modern_enter_game.cap");
    ASSERT_EQ(trace.size(), 2u);
    EXPECT_EQ(trace[0].category, 7u);   // UserConn
    EXPECT_EQ(trace[0].protocol, 8u);   // AgentConnectSuccess
    EXPECT_EQ(trace[1].category, 7u);
    EXPECT_EQ(trace[1].protocol, 18u);  // GameInNack (no character selected)
}

// M3 D-stage Ack upgrade: the modern MapServer is started with
// --dev-stub-caster (side-by-side harness only), so handle_skill()
// injects a minimal PlayerInfo when the StartSyn caster_id is not in
// state and continues to the real StartAck + SkillObjectAdd +
// SkillObjectRemove path.  The capture is now 3 frames instead of
// 1 Nack.  See docs/SIDE_BY_SIDE_T3.md for the M3 D-stage rationale.
TEST(SideBySideModernGolden, AttackTraceIsSkillStartAck) {
    const auto trace = load_capture(
        "C:/moxiang/modern/tests/fixtures/sbs_captures_modern/modern_attack.cap");
    ASSERT_EQ(trace.size(), 3u);
    // Frame 0: Skill.StartAck (cat=22, proto=1) for caster_id=1001.
    EXPECT_EQ(trace[0].category, 22u);
    EXPECT_EQ(trace[0].protocol, 1u);
    EXPECT_EQ(trace[0].object_id, 1001u);
    // payload: [skill_idx:u32=1][skill_obj_id:u32]
    ASSERT_EQ(trace[0].payload.size(), 8u);
    EXPECT_EQ(trace[0].payload[0], 1u);  // skill_idx low byte
    // Frame 1: SkillObjectAdd (cat=22, proto=3), 22-byte payload.
    EXPECT_EQ(trace[1].category, 22u);
    EXPECT_EQ(trace[1].protocol, 3u);
    // Frame 2: SkillObjectRemove (cat=22, proto=4), empty payload.
    EXPECT_EQ(trace[2].category, 22u);
    EXPECT_EQ(trace[2].protocol, 4u);
    EXPECT_EQ(trace[2].payload.size(), 0u);
}

TEST(SideBySideModernGolden, ShopTraceIsItemBuyNack) {
    const auto trace = load_capture(
        "C:/moxiang/modern/tests/fixtures/sbs_captures_modern/modern_shop.cap");
    ASSERT_EQ(trace.size(), 1u);
    EXPECT_EQ(trace[0].category, 5u);   // Item
    EXPECT_EQ(trace[0].protocol, 24u);  // BuyNack
    // Payload echoes the request [item:u16][qty:u16].
    EXPECT_EQ(trace[0].payload.size(), 4u);
}

TEST(SideBySideModernGolden, QuestTraceIsQuestStartNack) {
    const auto trace = load_capture(
        "C:/moxiang/modern/tests/fixtures/sbs_captures_modern/modern_quest.cap");
    ASSERT_EQ(trace.size(), 1u);
    EXPECT_EQ(trace[0].category, 39u);  // Quest
    EXPECT_EQ(trace[0].protocol, 11u);  // StartNack
    // Payload echoes quest_id (u16).
    EXPECT_EQ(trace[0].payload.size(), 2u);
}

}

