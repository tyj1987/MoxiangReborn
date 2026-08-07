//
// 1:1 lock the implicit default-branch dispatch for MP_JOURNAL in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_journal.hpp"
#include "mxh/server/agent_journal_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentJournalPlan, UserFoundEmitsForwardEffect) {
    AgentJournalRequest r;
    r.protocol = journal_getlist_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_journal_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentJournalSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, journal_getlist_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(journal_effect_targets_user(plan.effects[0]));
}

TEST(AgentJournalPlan, UserMissingEmitsDropEffect) {
    AgentJournalRequest r;
    r.protocol = journal_getlist_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_journal_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentJournalSideEffectKind::Drop);
    EXPECT_FALSE(journal_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentJournalPlan, ForwardPlanPreservesObjectId) {
    AgentJournalRequest r;
    r.protocol = journal_getlist_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_journal_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentJournalPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        journal_getlist_syn,
        journal_getlist_ack,
        journal_getlist_nack,
        journal_add,
        journal_update,
        journal_delete,
        journal_levelup
    };
    for (std::uint8_t p : all) {
        AgentJournalRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_journal_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentJournalPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        journal_getlist_syn,
        journal_getlist_ack,
        journal_getlist_nack,
        journal_add,
        journal_update,
        journal_delete,
        journal_levelup
    };
    for (std::uint8_t p : all) {
        AgentJournalRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_journal_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentJournalSideEffectKind::Drop);
    }
}

TEST(AgentJournalPlan, DefaultPlanStructFieldsAreStable) {
    AgentJournalSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentJournalPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentJournalSideEffect forward{};
    forward.kind = AgentJournalSideEffectKind::ForwardToUser;
    AgentJournalSideEffect drop{};
    drop.kind = AgentJournalSideEffectKind::Drop;
    EXPECT_TRUE(journal_effect_targets_user(forward));
    EXPECT_FALSE(journal_effect_targets_user(drop));
}

TEST(AgentJournalPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentJournalRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_journal_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentJournalPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentJournalRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_journal_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
