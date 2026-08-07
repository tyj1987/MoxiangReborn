// agent_chat_dispatch_test.cpp
//
// Verifies that dispatch_agent_chat_plan walks each ChatSideEffectKind
// and routes to the matching IChatWireSink method. Locks the dispatch
// table against accidental reordering / skipped kinds.
//
// Pattern mirrors mxh_agent_dispatch_tests (D4.R1) but covers the
// 12-kind chat surface instead of the simple ForwardToUser/Drop pair.

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <mxh/server/agent_chat.hpp>
#include <mxh/server/agent_chat_side_effect_plan.hpp>
#include <mxh/server/agent_chat_dispatch.hpp>

namespace {

using mxh::server::ChatRequest;
using mxh::server::ChatSideEffect;
using mxh::server::ChatSideEffectKind;
using mxh::server::ChatSideEffectPlan;
using mxh::server::dispatch_agent_chat_plan;
using mxh::server::IChatWireSink;
using mxh::server::chat_all;
using mxh::server::chat_smallshout;
using mxh::server::chat_gm_smallshout;
using mxh::server::chat_monster_speech;
using mxh::server::chat_whisper_syn;
using mxh::server::chat_whisper_gm_syn;
using mxh::server::chat_whisper_ack;
using mxh::server::chat_whisper_nack;
using mxh::server::chat_party;
using mxh::server::chat_guild;
using mxh::server::chat_guild_union;
using mxh::server::chat_shout_send_server;
using mxh::server::chat_fastchat;

struct RecordingChatSink final : IChatWireSink {
    struct Call {
        std::string tag;
        std::uint32_t receiver_id = 0u;
        std::uint8_t nack_code = 0u;
        bool forward_payload = false;
        bool gm_only = false;
    };
    std::vector<Call> calls;

    void drop(std::uint8_t p, std::uint32_t obj) override {
        Call c;
        c.tag = "drop";
        c.nack_code = p;
        (void)obj;
        calls.push_back(c);
    }
    void send2user(std::uint32_t r, std::uint8_t p, std::uint32_t obj, bool fp) override {
        (void)p; (void)obj;
        Call c;
        c.tag = "send2user";
        c.receiver_id = r;
        c.forward_payload = fp;
        calls.push_back(c);
    }
    void send_whisper_ack_to_server(std::uint8_t p, std::uint32_t obj, bool fp) override {
        (void)obj;
        Call c;
        c.tag = "send_whisper_ack_to_server";
        c.nack_code = p;
        c.forward_payload = fp;
        calls.push_back(c);
    }
    void send_whisper_to_user(std::uint32_t r, std::uint8_t p, std::uint32_t obj, bool fp, bool gm) override {
        (void)p; (void)obj;
        Call c;
        c.tag = "send_whisper_to_user";
        c.receiver_id = r;
        c.forward_payload = fp;
        c.gm_only = gm;
        calls.push_back(c);
    }
    void send_party_chat_to_member(std::uint32_t r, std::uint8_t p, std::uint32_t obj, bool fp) override {
        (void)p; (void)obj;
        Call c;
        c.tag = "send_party_chat_to_member";
        c.receiver_id = r;
        c.forward_payload = fp;
        calls.push_back(c);
    }
    void broadcast_party_chat_to_other_agents(std::uint8_t p, std::uint32_t obj, bool fp) override {
        (void)p; (void)obj;
        Call c;
        c.tag = "broadcast_party_chat_to_other_agents";
        c.forward_payload = fp;
        calls.push_back(c);
    }
    void broadcast_guild_chat_to_all_maps(std::uint8_t p, std::uint32_t obj, bool fp) override {
        (void)p; (void)obj;
        Call c;
        c.tag = "broadcast_guild_chat_to_all_maps";
        c.forward_payload = fp;
        calls.push_back(c);
    }
    void broadcast_guild_union_chat_to_all_maps(std::uint8_t p, std::uint32_t obj, bool fp) override {
        (void)p; (void)obj;
        Call c;
        c.tag = "broadcast_guild_union_chat_to_all_maps";
        c.forward_payload = fp;
        calls.push_back(c);
    }
};

}  // namespace

TEST(AgentChatDispatch, NullSinkReturnsZero) {
    ChatSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({ChatSideEffectKind::ForwardToClient, 0u, 0u, true});
    EXPECT_EQ(dispatch_agent_chat_plan(plan, nullptr), 0u);
}

TEST(AgentChatDispatch, EmptyPlanProducesNoCalls) {
    RecordingChatSink sink;
    ChatSideEffectPlan plan;
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(AgentChatDispatch, BroadcastProtocolsCallSend2User) {
    for (std::uint8_t proto : {chat_all, chat_smallshout, chat_gm_smallshout, chat_monster_speech}) {
        RecordingChatSink sink;
        ChatRequest req; req.protocol = proto;
        const auto plan = mxh::server::chat_side_effect_plan(req);
        EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 1u);
        ASSERT_EQ(sink.calls.size(), 1u);
        EXPECT_EQ(sink.calls[0].tag, "send2user");
        EXPECT_TRUE(sink.calls[0].forward_payload);
    }
}

TEST(AgentChatDispatch, WhisperHappyPathAcksAndDelivers) {
    RecordingChatSink sink;
    ChatRequest req;
    req.protocol = chat_whisper_syn;
    req.receiver_found = true;
    req.receiver_blocks_whisper = false;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 2u);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0].tag, "send_whisper_ack_to_server");
    EXPECT_TRUE(sink.calls[0].forward_payload);
    EXPECT_EQ(sink.calls[1].tag, "send_whisper_to_user");
    EXPECT_FALSE(sink.calls[1].gm_only);
}

TEST(AgentChatDispatch, WhisperBlockedSendsNackCode2) {
    RecordingChatSink sink;
    ChatRequest req;
    req.protocol = chat_whisper_syn;
    req.receiver_found = true;
    req.receiver_blocks_whisper = true;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].tag, "send_whisper_ack_to_server");
    EXPECT_EQ(sink.calls[0].nack_code, 2u);
    EXPECT_FALSE(sink.calls[0].forward_payload);
}

TEST(AgentChatDispatch, WhisperGmAcksAndDeliversWithGmFlag) {
    RecordingChatSink sink;
    ChatRequest req;
    req.protocol = chat_whisper_gm_syn;
    req.receiver_found = true;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 2u);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0].tag, "send_whisper_ack_to_server");
    EXPECT_TRUE(sink.calls[1].gm_only);
}

TEST(AgentChatDispatch, WhisperAckAndNackForwardToUser) {
    {
        RecordingChatSink sink;
        ChatRequest req; req.protocol = chat_whisper_ack;
        const auto plan = mxh::server::chat_side_effect_plan(req);
        EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 1u);
        ASSERT_EQ(sink.calls.size(), 1u);
        EXPECT_EQ(sink.calls[0].tag, "send2user");
        EXPECT_TRUE(sink.calls[0].forward_payload);
    }
    {
        RecordingChatSink sink;
        ChatRequest req; req.protocol = chat_whisper_nack;
        const auto plan = mxh::server::chat_side_effect_plan(req);
        EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 1u);
        ASSERT_EQ(sink.calls.size(), 1u);
        EXPECT_EQ(sink.calls[0].tag, "send2user");
        EXPECT_TRUE(sink.calls[0].forward_payload);
    }
}

TEST(AgentChatDispatch, PartyChatSendsAndBroadcasts) {
    RecordingChatSink sink;
    ChatRequest req; req.protocol = chat_party; req.sender_found = true;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 2u);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0].tag, "send_party_chat_to_member");
    EXPECT_EQ(sink.calls[1].tag, "broadcast_party_chat_to_other_agents");
}

TEST(AgentChatDispatch, GuildChatBroadcastsToAllMaps) {
    RecordingChatSink sink;
    ChatRequest req; req.protocol = chat_guild; req.sender_found = true;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].tag, "broadcast_guild_chat_to_all_maps");
}

TEST(AgentChatDispatch, GuildUnionChatBroadcastsToAllMaps) {
    RecordingChatSink sink;
    ChatRequest req; req.protocol = chat_guild_union; req.sender_found = true;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 1u);
    ASSERT_EQ(sink.calls.size(), 1u);
    EXPECT_EQ(sink.calls[0].tag, "broadcast_guild_union_chat_to_all_maps");
}

TEST(AgentChatDispatch, UnknownProtocolProducesNoCalls) {
    RecordingChatSink sink;
    ChatRequest req; req.protocol = 255u;
    const auto plan = mxh::server::chat_side_effect_plan(req);
    EXPECT_EQ(dispatch_agent_chat_plan(plan, &sink), 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(AgentChatDispatch, UnhandledProtocolsProduceEmptyPlan) {
    for (std::uint8_t proto : {chat_shout_send_server, chat_fastchat}) {
        ChatRequest req; req.protocol = proto;
        const auto plan = mxh::server::chat_side_effect_plan(req);
        EXPECT_FALSE(plan.dispatched);
        EXPECT_TRUE(plan.effects.empty());
    }
}
