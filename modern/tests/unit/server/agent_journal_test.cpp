// D4 AgentJournal data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_JOURNAL.

#include <mxh/server/agent_journal.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentJournalClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(journal_category, 53u);
}

TEST(AgentJournalClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        journal_getlist_syn,
        journal_getlist_ack,
        journal_getlist_nack,
        journal_add,
        journal_update,
        journal_delete,
        journal_levelup
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 7u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentJournalClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(journal_getlist_syn, 0u);
    EXPECT_EQ(journal_levelup, 6u);
}

TEST(AgentJournalClassify, UserFoundForwards) {
    AgentJournalRequest r;
    r.protocol = journal_getlist_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::ForwardToUser);
}

TEST(AgentJournalClassify, UserNotFoundDrops) {
    AgentJournalRequest r;
    r.protocol = journal_getlist_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::DropNoUser);
}

TEST(AgentJournalClassify, EverySubProtocolForwardsWhenUserFound) {
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
        EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentJournalClassify, EverySubProtocolDropsWhenUserMissing) {
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
        EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentJournalClassify, ObjectIdIgnoredInClassification) {
    AgentJournalRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentJournalRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_journal(a), classify_agent_journal(b));
}

TEST(AgentJournalClassify, OutcomeIsDeterministic) {
    AgentJournalRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::ForwardToUser);
}

TEST(AgentJournalClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentJournalRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_journal(r), AgentJournalOutcome::ForwardToUser);
}
