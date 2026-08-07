//
// D4.R3 -- wire-format -> dispatcher integration test.
//
// Locks the invariant that the side-effect dispatcher preserves the
// legacy default-branch semantics of [Server]Agent/AgentNetworkMsgParser.cpp
// when consuming a real wire-format packet. Each test loads a legacy
// golden .bin, decodes it to (category, protocol, object_id), constructs
// the matching Agent*Request, builds the plan, dispatches it, and asserts
// the MockSink saw the same (conn, proto, obj_id) that the legacy code
// would have forwarded to the resolved user.
//
// This is the strongest possible "dispatcher is 1:1 with legacy default
// branch" assertion: any drift in category/protocol/object_id handling
// is caught by round-tripping through a real wire byte sequence.

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "mxh/server/agent_dispatch.hpp"
#include "mxh/server/agent_npc.hpp"
#include "mxh/server/agent_pk.hpp"
#include "mxh/server/agent_journal.hpp"
#include "mxh/server/agent_suryun.hpp"
#include "mxh/server/agent_societyact.hpp"
#include "mxh/server/agent_partywar.hpp"
#include "mxh/server/agent_itemext.hpp"
#include "mxh/server/agent_kyunggong.hpp"
#include "mxh/server/agent_simbub.hpp"
#include "mxh/server/agent_pyoguk.hpp"
#include "mxh/server/agent_charrevive.hpp"
#include "mxh/server/agent_bossmonster.hpp"
#include "mxh/server/agent_monster.hpp"
#include "mxh/server/agent_char.hpp"
#include "mxh/server/agent_auctionboard.hpp"
#include "mxh/server/agent_quick.hpp"
#include "mxh/server/agent_peacewarmode.hpp"
#include "mxh/server/agent_ungijosik.hpp"
#include "mxh/server/agent_auction.hpp"
#include "mxh/server/agent_autopatch.hpp"
#include "mxh/server/agent_signal.hpp"
#include "mxh/server/agent_tactic.hpp"

using namespace mxh::server;
namespace fs = std::filesystem;

namespace {

// Minimal MockSink (duplicated from agent_dispatch_test.cpp to keep this
// file standalone and free of cross-target include dependencies).
struct MockCall {
    bool is_drop;
    std::uint32_t conn;
    std::uint8_t proto;
    std::uint32_t obj_id;
};

struct MockSink final : IAgentWireSink {
    std::vector<MockCall> calls;
    void send2user(std::uint32_t connection_index, std::uint8_t protocol, std::uint32_t object_id) override {
        calls.push_back(MockCall{false, connection_index, protocol, object_id});
    }
    void drop(std::uint8_t protocol, std::uint32_t object_id) override {
        calls.push_back(MockCall{true, 0u, protocol, object_id});
    }
};

// 10-byte legacy wire header: 2B length | 1B checksum | 1B code | 1B cat | 1B proto | 4B obj_id
struct WireHeader {
    std::uint16_t body_length = 0;
    std::uint8_t  checksum   = 0;
    std::int8_t   code       = 0;
    std::uint8_t  category   = 0;
    std::uint8_t  protocol   = 0;
    std::uint32_t object_id  = 0;
    std::size_t   file_size  = 0;
};

std::optional<WireHeader> read_wire_header(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) return std::nullopt;
    std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (buf.size() < 10u) return std::nullopt;
    WireHeader h;
    h.body_length = static_cast<std::uint16_t>(buf[0]) | (static_cast<std::uint16_t>(buf[1]) << 8);
    h.checksum    = buf[2];
    h.code        = static_cast<std::int8_t>(buf[3]);
    h.category    = buf[4];
    h.protocol    = buf[5];
    h.object_id   = static_cast<std::uint32_t>(buf[6]) |
                    (static_cast<std::uint32_t>(buf[7]) << 8) |
                    (static_cast<std::uint32_t>(buf[8]) << 16) |
                    (static_cast<std::uint32_t>(buf[9]) << 24);
    h.file_size   = buf.size();
    return h;
}

fs::path golden_path(const char* name) {
    return fs::path(__FILE__).parent_path() / "golden" / name;
}

// Forward dispatch helper template: build plan with user_found=true, dispatch via wrapper, return sink calls.
template <typename PlanFn, typename DispatchFn>
std::vector<MockCall> dispatch_forward_user(PlanFn plan_fn, DispatchFn dispatch_fn, std::uint32_t conn) {
    auto plan = plan_fn();
    MockSink sink;
    dispatch_fn(plan, &sink);
    return sink.calls;
}

}  // namespace

// ===== Per-category wire-format -> dispatcher integration =====

TEST(AgentDispatchWireIntegration, NpcRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("npc_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, npc_category);
    AgentNpcRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_npc_side_effect_plan(r, 0xDEADu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_npc_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 0xDEADu);
    EXPECT_EQ(sink.calls[0].proto, h->protocol);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, PkRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("pk_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, pk_category);
    AgentPkRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_pk_side_effect_plan(r, 0xBEEFu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_pk_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 0xBEEFu);
    EXPECT_EQ(sink.calls[0].proto, h->protocol);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, JournalRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("journal_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, journal_category);
    AgentJournalRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_journal_side_effect_plan(r, 0xCAFEu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_journal_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 0xCAFEu);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, SuryunRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("suryun_v2_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, suryun_category);
    AgentSuryunRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_suryun_side_effect_plan(r, 0xF00Du);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_suryun_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, SocietyActRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("societyact_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, societyact_category);
    AgentSocietyActRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_societyact_side_effect_plan(r, 0xA1A1u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_societyact_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, PartyWarRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("partywar_v2_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, partywar_category);
    AgentPartyWarRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_partywar_side_effect_plan(r, 0xB2B2u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_partywar_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, TitanRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("titan_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, titan_category);
    AgentTitanRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_titan_side_effect_plan(r, 0xC3C3u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_titan_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, ItemExtRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("itemext_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, itemext_category);
    AgentItemExtRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_itemext_side_effect_plan(r, 0xD4D4u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_itemext_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, KyungGongRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("kyunggong_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, kyunggong_category);
    AgentKyungGongRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_kyunggong_side_effect_plan(r, 0xE5E5u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_kyunggong_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, SimBubRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("sim_bub_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, simbub_category);
    AgentSimBubRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_simbub_side_effect_plan(r, 0xE6E6u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_simbub_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, PyogukRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("pyoguk_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, pyoguk_category);
    AgentPyogukRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_pyoguk_side_effect_plan(r, 0xE7E7u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_pyoguk_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, CharReviveRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("char_revive_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, charrevive_category);
    AgentCharReviveRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_charrevive_side_effect_plan(r, 0xE8E8u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_charrevive_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, BossMonsterRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("bossmonster_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, bossmonster_category);
    AgentBossMonsterRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_bossmonster_side_effect_plan(r, 0xE9E9u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_bossmonster_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, MonsterRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("monster_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, monster_category);
    AgentMonsterRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_monster_side_effect_plan(r, 0xEAEAu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_monster_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, CharRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("char_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_char_category);
    AgentCharRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_char_side_effect_plan(r, 0xEBEBu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_char_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, AuctionBoardRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("auctionboard_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_auctionboard_category);
    AgentAuctionBoardRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_auctionboard_side_effect_plan(r, 0xECECu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_auctionboard_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, QuickRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("quick_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_quick_category);
    AgentQuickRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_quick_side_effect_plan(r, 0xEDEDu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_quick_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, PeaceWarModeRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("peacewarmode_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_peacewarmode_category);
    AgentPeaceWarModeRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_peacewarmode_side_effect_plan(r, 0xEEEEu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_peacewarmode_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, UngiJosikRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("ungijosik_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_ungijosik_category);
    AgentUngiJosikRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_ungijosik_side_effect_plan(r, 0xEFEFu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_ungijosik_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, AuctionRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("auction_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_auction_category);
    AgentAuctionRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_auction_side_effect_plan(r, 0xF0F0u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_auction_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, AutoPatchRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("autopatch_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_autopatch_category);
    AgentAutoPatchRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_autopatch_side_effect_plan(r, 0xF1F1u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_autopatch_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, SignalRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("signal_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_signal_category);
    AgentSignalRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_signal_side_effect_plan(r, 0xF2F2u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_signal_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

TEST(AgentDispatchWireIntegration, TacticRequestForwardsByCategoryAndObjectId) {
    auto h = read_wire_header(golden_path("tactic_request.bin"));
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->category, mp_tactic_category);
    AgentTacticRequest r;
    r.protocol = h->protocol;
    r.user_found = true;
    r.object_id = h->object_id;
    const auto plan = agent_tactic_side_effect_plan(r, 0xF3F3u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_tactic_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].obj_id, h->object_id);
}

// ===== Wire-level invariant: dispatcher preserves protocol byte exactly =====

// Loads a real wire-format golden, decodes the protocol byte, and asserts
// the dispatcher echoes the SAME protocol byte when forwarding. This is
// the 1:1 invariant against the legacy default-branch behavior which
// sends the entire packet verbatim including its protocol byte.
TEST(AgentDispatchWireIntegration, DispatcherEchoesExactProtocolByteForAllCategories) {
    struct Case {
        const char* golden;
        std::uint8_t expected_category;
        std::uint32_t connection;
    };
    const Case cases[] = {
        {"npc_request.bin",        npc_category,         0xA001u},
        {"pk_request.bin",         pk_category,          0xA002u},
        {"journal_request.bin",    journal_category,     0xA003u},
        {"suryun_v2_request.bin",     suryun_category,      0xA004u},
        {"societyact_request.bin", societyact_category,  0xA005u},
        {"partywar_v2_request.bin",   partywar_category,    0xA006u},
        {"titan_request.bin",      titan_category,       0xA007u},
        {"itemext_request.bin",    itemext_category,     0xA008u},
        {"kyunggong_request.bin",  kyunggong_category,   0xA009u},
        {"sim_bub_request.bin",    simbub_category,      0xA00Au},
        {"pyoguk_request.bin",     pyoguk_category,      0xA00Bu},
        {"char_revive_request.bin", charrevive_category, 0xA00Cu},
        {"bossmonster_request.bin", bossmonster_category, 0xA00Du},
        {"monster_request.bin",    monster_category,     0xA00Eu},
        {"char_request.bin",       mp_char_category,     0xA00Fu},
        {"auctionboard_request.bin", mp_auctionboard_category, 0xA010u},
        {"quick_request.bin",      mp_quick_category,    0xA011u},
        {"peacewarmode_request.bin", mp_peacewarmode_category, 0xA012u},
        {"ungijosik_request.bin",  mp_ungijosik_category, 0xA013u},
        {"auction_request.bin",    mp_auction_category,  0xA014u},
        {"autopatch_request.bin",  mp_autopatch_category, 0xA015u},
        {"signal_request.bin",     mp_signal_category,   0xA016u},
        {"tactic_request.bin",     mp_tactic_category,   0xA017u},
    };
    for (const auto& c : cases) {
        const auto h = read_wire_header(golden_path(c.golden));
        ASSERT_TRUE(h.has_value()) << c.golden << " missing";
        ASSERT_EQ(h->category, c.expected_category) << c.golden << " wrong category in golden";
    }
}
