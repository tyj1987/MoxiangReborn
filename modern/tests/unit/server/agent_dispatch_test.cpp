//
// D4.R1 -- Agent side-effect dispatcher runtime tests.
//
// 1:1 lock the runtime dispatch path for the 23 agent-no-handler
// categories covered by agent_*_side_effect_plan.hpp. Each test exercises
// the modern dispatch_agent_<cat>_plan() function against a mock
// IAgentWireSink and asserts the sink observed the expected
// send2user / drop calls with the correct connection_index, protocol,
// and object_id. The dispatcher is the runtime counterpart of every
// agent_*_side_effect_plan.hpp and the foundation that the future D4
// shop-item runtime orchestrator will extend with DB-touching effects.

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "mxh/server/agent_dispatch.hpp"

using namespace mxh::server;

namespace {

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

// Generic ForwardToUser plan builder (manual effect, no classify_* call).
template <typename Plan, typename Effect>
Plan make_forward_plan(std::uint8_t proto, std::uint32_t conn, std::uint32_t obj_id) {
    using K = decltype(Effect::kind);
    Plan out;
    out.dispatched = true;
    out.drop = false;
    out.effects.reserve(1u);
    Effect e;
    e.kind = static_cast<K>(K::ForwardToUser);
    e.reply_protocol = proto;
    e.connection_index = conn;
    e.object_id = obj_id;
    out.effects.push_back(e);
    return out;
}

// Generic Drop plan builder.
template <typename Plan, typename Effect>
Plan make_drop_plan(std::uint8_t proto, std::uint32_t obj_id) {
    using K = decltype(Effect::kind);
    Plan out;
    out.drop = true;
    out.effects.reserve(1u);
    Effect e;
    e.kind = static_cast<K>(K::Drop);
    e.reply_protocol = proto;
    e.connection_index = 0u;
    e.object_id = obj_id;
    out.effects.push_back(e);
    return out;
}

}  // namespace

// ===== Manual dispatchers: agent_npc =====

TEST(AgentDispatchNpc, ForwardPlanEmitsOneSend2user) {
    AgentNpcRequest r;
    r.protocol = npc_speech_syn;
    r.user_found = true;
    r.object_id = 0xAABBCCDDu;
    const auto plan = agent_npc_side_effect_plan(r, 7u);
    MockSink sink;
    const auto n = dispatch_agent_npc_plan(plan, &sink);
    EXPECT_EQ(n, 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 7u);
    EXPECT_EQ(sink.calls[0].proto, npc_speech_syn);
    EXPECT_EQ(sink.calls[0].obj_id, 0xAABBCCDDu);
}

TEST(AgentDispatchNpc, DropPlanEmitsOneDrop) {
    AgentNpcRequest r;
    r.protocol = npc_speech_nack;
    r.user_found = false;
    r.object_id = 0x11112222u;
    const auto plan = agent_npc_side_effect_plan(r, 9u);
    MockSink sink;
    const auto n = dispatch_agent_npc_plan(plan, &sink);
    EXPECT_EQ(n, 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_TRUE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 0u);
    EXPECT_EQ(sink.calls[0].proto, npc_speech_nack);
    EXPECT_EQ(sink.calls[0].obj_id, 0x11112222u);
}

TEST(AgentDispatchNpc, NullSinkIsSafeNoop) {
    AgentNpcRequest r;
    r.user_found = true;
    const auto plan = agent_npc_side_effect_plan(r, 3u);
    EXPECT_EQ(dispatch_agent_npc_plan(plan, nullptr), 0u);
}

TEST(AgentDispatchNpc, EmptyPlanReturnsZero) {
    AgentNpcSideEffectPlan plan;
    MockSink sink;
    EXPECT_EQ(dispatch_agent_npc_plan(plan, &sink), 0u);
    EXPECT_TRUE(sink.calls.empty());
}

// ===== Manual dispatchers: agent_pk =====

TEST(AgentDispatchPk, ForwardPlanEmitsOneSend2user) {
    AgentPkRequest r;
    r.protocol = pk_pkon_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    const auto plan = agent_pk_side_effect_plan(r, 5u);
    MockSink sink;
    const auto n = dispatch_agent_pk_plan(plan, &sink);
    EXPECT_EQ(n, 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 5u);
    EXPECT_EQ(sink.calls[0].proto, pk_pkon_syn);
    EXPECT_EQ(sink.calls[0].obj_id, 0xDEADBEEFu);
}

TEST(AgentDispatchPk, DropPlanEmitsOneDrop) {
    AgentPkRequest r;
    r.protocol = pk_destroy_item;
    r.user_found = false;
    r.object_id = 0xFEEDFACEu;
    const auto plan = agent_pk_side_effect_plan(r, 11u);
    MockSink sink;
    const auto n = dispatch_agent_pk_plan(plan, &sink);
    EXPECT_EQ(n, 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_TRUE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].proto, pk_destroy_item);
    EXPECT_EQ(sink.calls[0].obj_id, 0xFEEDFACEu);
}

TEST(AgentDispatchPk, NullSinkIsSafeNoop) {
    AgentPkRequest r;
    r.user_found = true;
    const auto plan = agent_pk_side_effect_plan(r, 1u);
    EXPECT_EQ(dispatch_agent_pk_plan(plan, nullptr), 0u);
}

// ===== Generic dispatcher template: null-sink safety =====

TEST(AgentDispatchTemplate, NullSinkIsSafeNoop) {
    auto plan = make_forward_plan<AgentJournalSideEffectPlan, AgentJournalSideEffect>(
        journal_category, 1u, 2u);
    const auto n = dispatch_forward_drop_plan<
        AgentJournalSideEffectPlan, AgentJournalSideEffectKind,
        AgentJournalSideEffectKind::Drop, AgentJournalSideEffectKind::ForwardToUser>(
        plan, nullptr, journal_kind_of);
    EXPECT_EQ(n, 0u);
}

// ===== Sweep: all 22 generic categories, forward path =====

TEST(AgentDispatchSweepForward, JournalForwardsViaSink) {
    auto plan = make_forward_plan<AgentJournalSideEffectPlan, AgentJournalSideEffect>(
        journal_category, 11u, 0x01020304u);
    MockSink sink;
    const auto n = dispatch_agent_journal_plan(plan, &sink);
    EXPECT_EQ(n, 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 11u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x01020304u);
}

TEST(AgentDispatchSweepForward, SuryunForwardsViaSink) {
    auto plan = make_forward_plan<AgentSuryunSideEffectPlan, AgentSuryunSideEffect>(
        suryun_category, 12u, 0x02030405u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_suryun_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 12u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x02030405u);
}

TEST(AgentDispatchSweepForward, SocietyActForwardsViaSink) {
    auto plan = make_forward_plan<AgentSocietyActSideEffectPlan, AgentSocietyActSideEffect>(
        societyact_category, 13u, 0x03040506u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_societyact_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 13u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x03040506u);
}

TEST(AgentDispatchSweepForward, PartyWarForwardsViaSink) {
    auto plan = make_forward_plan<AgentPartyWarSideEffectPlan, AgentPartyWarSideEffect>(
        partywar_category, 14u, 0x04050607u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_partywar_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 14u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x04050607u);
}

TEST(AgentDispatchSweepForward, TitanForwardsViaSink) {
    auto plan = make_forward_plan<AgentTitanSideEffectPlan, AgentTitanSideEffect>(
        titan_category, 15u, 0x05060708u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_titan_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 15u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x05060708u);
}

TEST(AgentDispatchSweepForward, ItemExtForwardsViaSink) {
    auto plan = make_forward_plan<AgentItemExtSideEffectPlan, AgentItemExtSideEffect>(
        itemext_category, 16u, 0x06070809u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_itemext_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 16u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x06070809u);
}

TEST(AgentDispatchSweepForward, KyungGongForwardsViaSink) {
    auto plan = make_forward_plan<AgentKyungGongSideEffectPlan, AgentKyungGongSideEffect>(
        kyunggong_category, 17u, 0x0708090Au);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_kyunggong_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 17u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0708090Au);
}

TEST(AgentDispatchSweepForward, SimBubForwardsViaSink) {
    auto plan = make_forward_plan<AgentSimBubSideEffectPlan, AgentSimBubSideEffect>(
        simbub_category, 18u, 0x08090A0Bu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_simbub_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 18u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x08090A0Bu);
}

TEST(AgentDispatchSweepForward, PyogukForwardsViaSink) {
    auto plan = make_forward_plan<AgentPyogukSideEffectPlan, AgentPyogukSideEffect>(
        pyoguk_category, 19u, 0x090A0B0Cu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_pyoguk_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 19u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x090A0B0Cu);
}

TEST(AgentDispatchSweepForward, CharReviveForwardsViaSink) {
    auto plan = make_forward_plan<AgentCharReviveSideEffectPlan, AgentCharReviveSideEffect>(
        charrevive_category, 20u, 0x0A0B0C0Du);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_charrevive_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 20u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0A0B0C0Du);
}

TEST(AgentDispatchSweepForward, BossMonsterForwardsViaSink) {
    auto plan = make_forward_plan<AgentBossMonsterSideEffectPlan, AgentBossMonsterSideEffect>(
        bossmonster_category, 21u, 0x0B0C0D0Eu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_bossmonster_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 21u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0B0C0D0Eu);
}

TEST(AgentDispatchSweepForward, MonsterForwardsViaSink) {
    auto plan = make_forward_plan<AgentMonsterSideEffectPlan, AgentMonsterSideEffect>(
        monster_category, 22u, 0x0C0D0E0Fu);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_monster_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 22u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0C0D0E0Fu);
}

TEST(AgentDispatchSweepForward, CharForwardsViaSink) {
    auto plan = make_forward_plan<AgentCharSideEffectPlan, AgentCharSideEffect>(
        mp_char_category, 23u, 0x0D0E0F10u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_char_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 23u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0D0E0F10u);
}

TEST(AgentDispatchSweepForward, AuctionBoardForwardsViaSink) {
    auto plan = make_forward_plan<AgentAuctionBoardSideEffectPlan, AgentAuctionBoardSideEffect>(
        mp_auctionboard_category, 24u, 0x0E0F1011u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_auctionboard_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 24u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0E0F1011u);
}

TEST(AgentDispatchSweepForward, QuickForwardsViaSink) {
    auto plan = make_forward_plan<AgentQuickSideEffectPlan, AgentQuickSideEffect>(
        mp_quick_category, 25u, 0x0F101112u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_quick_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 25u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x0F101112u);
}

TEST(AgentDispatchSweepForward, PeaceWarModeForwardsViaSink) {
    auto plan = make_forward_plan<AgentPeaceWarModeSideEffectPlan, AgentPeaceWarModeSideEffect>(
        mp_peacewarmode_category, 26u, 0x10111213u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_peacewarmode_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 26u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x10111213u);
}

TEST(AgentDispatchSweepForward, UngiJosikForwardsViaSink) {
    auto plan = make_forward_plan<AgentUngiJosikSideEffectPlan, AgentUngiJosikSideEffect>(
        mp_ungijosik_category, 27u, 0x11121314u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_ungijosik_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 27u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x11121314u);
}

TEST(AgentDispatchSweepForward, AuctionForwardsViaSink) {
    auto plan = make_forward_plan<AgentAuctionSideEffectPlan, AgentAuctionSideEffect>(
        mp_auction_category, 28u, 0x12131415u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_auction_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 28u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x12131415u);
}

TEST(AgentDispatchSweepForward, AutoPatchForwardsViaSink) {
    auto plan = make_forward_plan<AgentAutoPatchSideEffectPlan, AgentAutoPatchSideEffect>(
        mp_autopatch_category, 29u, 0x13141516u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_autopatch_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 29u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x13141516u);
}

TEST(AgentDispatchSweepForward, SignalForwardsViaSink) {
    auto plan = make_forward_plan<AgentSignalSideEffectPlan, AgentSignalSideEffect>(
        mp_signal_category, 30u, 0x14151617u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_signal_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 30u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x14151617u);
}

TEST(AgentDispatchSweepForward, TacticForwardsViaSink) {
    auto plan = make_forward_plan<AgentTacticSideEffectPlan, AgentTacticSideEffect>(
        mp_tactic_category, 31u, 0x15161718u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_tactic_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 31u);
    EXPECT_EQ(sink.calls[0].obj_id, 0x15161718u);
}

// ===== Sweep: representative drop path =====

TEST(AgentDispatchSweepDrop, JournalDropsViaSink) {
    auto plan = make_drop_plan<AgentJournalSideEffectPlan, AgentJournalSideEffect>(
        journal_category, 0xA1A2A3A4u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_journal_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_TRUE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].proto, journal_category);
    EXPECT_EQ(sink.calls[0].obj_id, 0xA1A2A3A4u);
}

TEST(AgentDispatchSweepDrop, AuctionDropsViaSink) {
    auto plan = make_drop_plan<AgentAuctionSideEffectPlan, AgentAuctionSideEffect>(
        mp_auction_category, 0xB1B2B3B4u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_auction_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_TRUE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].proto, mp_auction_category);
    EXPECT_EQ(sink.calls[0].obj_id, 0xB1B2B3B4u);
}

TEST(AgentDispatchSweepDrop, TacticDropsViaSink) {
    auto plan = make_drop_plan<AgentTacticSideEffectPlan, AgentTacticSideEffect>(
        mp_tactic_category, 0xC1C2C3C4u);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_tactic_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_TRUE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].proto, mp_tactic_category);
    EXPECT_EQ(sink.calls[0].obj_id, 0xC1C2C3C4u);
}

// ===== Multi-effect plan preserves order =====

TEST(AgentDispatchOrder, MultipleEffectsAreDispatchedInPlanOrder) {
    AgentTacticSideEffectPlan plan;
    plan.effects.reserve(3u);
    AgentTacticSideEffect f;
    f.kind = AgentTacticSideEffectKind::ForwardToUser;
    f.reply_protocol = mp_tactic_category;
    f.connection_index = 100u;
    f.object_id = 0xF0u;
    plan.effects.push_back(f);
    AgentTacticSideEffect d;
    d.kind = AgentTacticSideEffectKind::Drop;
    d.reply_protocol = mp_tactic_category;
    d.connection_index = 0u;
    d.object_id = 0xF1u;
    plan.effects.push_back(d);
    AgentTacticSideEffect f2;
    f2.kind = AgentTacticSideEffectKind::ForwardToUser;
    f2.reply_protocol = mp_tactic_category;
    f2.connection_index = 200u;
    f2.object_id = 0xF2u;
    plan.effects.push_back(f2);
    MockSink sink;
    EXPECT_EQ(dispatch_agent_tactic_plan(plan, &sink), 3u);
    ASSERT_EQ(sink.calls.size(), 3u);
    EXPECT_FALSE(sink.calls[0].is_drop);
    EXPECT_EQ(sink.calls[0].conn, 100u);
    EXPECT_EQ(sink.calls[0].obj_id, 0xF0u);
    EXPECT_TRUE(sink.calls[1].is_drop);
    EXPECT_EQ(sink.calls[1].obj_id, 0xF1u);
    EXPECT_FALSE(sink.calls[2].is_drop);
    EXPECT_EQ(sink.calls[2].conn, 200u);
    EXPECT_EQ(sink.calls[2].obj_id, 0xF2u);
}
