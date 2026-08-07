// D4 AgentItemExt data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_ITEMEXT.

#include <mxh/server/agent_itemext.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentItemExtClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(itemext_category, 73u);
}

TEST(AgentItemExtClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        itemext_shopitem_curse_cancellation_additem_syn,
        itemext_shopitem_curse_cancellation_additem_ack,
        itemext_shopitem_curse_cancellation_additem_nack,
        itemext_shopitem_curse_cancellation_release,
        itemext_shopitem_curse_cancellation_deleteitem,
        itemext_shopitem_curse_cancellation_syn,
        itemext_shopitem_curse_cancellation_ack,
        itemext_shopitem_curse_cancellation_nack,
        itemext_uniqueitem_mix_additem_syn,
        itemext_uniqueitem_mix_additem_ack,
        itemext_uniqueitem_mix_additem_nack,
        itemext_uniqueitem_mix_release,
        itemext_uniqueitem_mix_deleteitem,
        itemext_uniqueitem_mix_syn,
        itemext_uniqueitem_mix_ack,
        itemext_uniqueitem_mix_nack,
        itemext_shopitem_decoration_on,
        itemext_skinitem_select_syn,
        itemext_skinitem_select_ack,
        itemext_skinitem_select_nack,
        itemext_skinitem_discard_ack
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 21u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentItemExtClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(itemext_shopitem_curse_cancellation_additem_syn, 0u);
    EXPECT_EQ(itemext_skinitem_discard_ack, 20u);
}

TEST(AgentItemExtClassify, UserFoundForwards) {
    AgentItemExtRequest r;
    r.protocol = itemext_shopitem_curse_cancellation_additem_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::ForwardToUser);
}

TEST(AgentItemExtClassify, UserNotFoundDrops) {
    AgentItemExtRequest r;
    r.protocol = itemext_shopitem_curse_cancellation_additem_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::DropNoUser);
}

TEST(AgentItemExtClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        itemext_shopitem_curse_cancellation_additem_syn,
        itemext_shopitem_curse_cancellation_additem_ack,
        itemext_shopitem_curse_cancellation_additem_nack,
        itemext_shopitem_curse_cancellation_release,
        itemext_shopitem_curse_cancellation_deleteitem,
        itemext_shopitem_curse_cancellation_syn,
        itemext_shopitem_curse_cancellation_ack,
        itemext_shopitem_curse_cancellation_nack,
        itemext_uniqueitem_mix_additem_syn,
        itemext_uniqueitem_mix_additem_ack,
        itemext_uniqueitem_mix_additem_nack,
        itemext_uniqueitem_mix_release,
        itemext_uniqueitem_mix_deleteitem,
        itemext_uniqueitem_mix_syn,
        itemext_uniqueitem_mix_ack,
        itemext_uniqueitem_mix_nack,
        itemext_shopitem_decoration_on,
        itemext_skinitem_select_syn,
        itemext_skinitem_select_ack,
        itemext_skinitem_select_nack,
        itemext_skinitem_discard_ack
    };
    for (std::uint8_t p : all) {
        AgentItemExtRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentItemExtClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        itemext_shopitem_curse_cancellation_additem_syn,
        itemext_shopitem_curse_cancellation_additem_ack,
        itemext_shopitem_curse_cancellation_additem_nack,
        itemext_shopitem_curse_cancellation_release,
        itemext_shopitem_curse_cancellation_deleteitem,
        itemext_shopitem_curse_cancellation_syn,
        itemext_shopitem_curse_cancellation_ack,
        itemext_shopitem_curse_cancellation_nack,
        itemext_uniqueitem_mix_additem_syn,
        itemext_uniqueitem_mix_additem_ack,
        itemext_uniqueitem_mix_additem_nack,
        itemext_uniqueitem_mix_release,
        itemext_uniqueitem_mix_deleteitem,
        itemext_uniqueitem_mix_syn,
        itemext_uniqueitem_mix_ack,
        itemext_uniqueitem_mix_nack,
        itemext_shopitem_decoration_on,
        itemext_skinitem_select_syn,
        itemext_skinitem_select_ack,
        itemext_skinitem_select_nack,
        itemext_skinitem_discard_ack
    };
    for (std::uint8_t p : all) {
        AgentItemExtRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentItemExtClassify, ObjectIdIgnoredInClassification) {
    AgentItemExtRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentItemExtRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_itemext(a), classify_agent_itemext(b));
}

TEST(AgentItemExtClassify, OutcomeIsDeterministic) {
    AgentItemExtRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::ForwardToUser);
}

TEST(AgentItemExtClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentItemExtRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_itemext(r), AgentItemExtOutcome::ForwardToUser);
}
