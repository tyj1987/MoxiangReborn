#include <array>
#include <cstddef>

#include <mxh/server/agent_chat.hpp>
#include <mxh/server/agent_chat_side_effect_plan.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ChatRequest make_chat(std::uint8_t protocol) {
    ChatRequest request{};
    request.protocol = protocol;
    request.receiver_found = true;
    request.receiver_blocks_whisper = false;
    request.sender_found = true;
    request.msg_table_insert_ok = true;
    request.target_name_too_short = false;
    return request;
}

TEST(ChatPlan, BroadcastChatForwardsToClient) {
    const std::array protocols = {chat_all, chat_smallshout,
        chat_gm_smallshout, chat_monster_speech};
    for (const auto protocol : protocols) {
        const auto plan = chat_side_effect_plan(make_chat(protocol));
        EXPECT_TRUE(plan.dispatched);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ChatSideEffectKind::ForwardToClient);
    }
}

TEST(ChatPlan, WhisperSynMissingReceiverDrops) {
    ChatRequest request = make_chat(chat_whisper_syn);
    request.receiver_found = false;
    const auto plan = chat_side_effect_plan(request);
    EXPECT_TRUE(plan.drop);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(ChatPlan, WhisperSynBlockedOptionSendsNack) {
    ChatRequest request = make_chat(chat_whisper_syn);
    request.receiver_blocks_whisper = true;
    const auto plan = chat_side_effect_plan(request);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ChatSideEffectKind::SendWhisperNackToServer);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);
}

TEST(ChatPlan, WhisperSynSuccessEmitsAckAndReceiverPacket) {
    const auto plan = chat_side_effect_plan(make_chat(chat_whisper_syn));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ChatSideEffectKind::SendWhisperAckToServer);
    EXPECT_EQ(plan.effects[1].kind,
              ChatSideEffectKind::SendWhisperToReceiverUser);
    EXPECT_TRUE(plan.effects[0].forward_payload);
    EXPECT_TRUE(plan.effects[1].forward_payload);
}

TEST(ChatPlan, WhisperGmSynMissingReceiverDrops) {
    ChatRequest request = make_chat(chat_whisper_gm_syn);
    request.receiver_found = false;
    const auto plan = chat_side_effect_plan(request);
    EXPECT_TRUE(plan.drop);
}

TEST(ChatPlan, WhisperGmSynSendsAckAndGmReceiverPacket) {
    const auto plan = chat_side_effect_plan(make_chat(chat_whisper_gm_syn));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ChatSideEffectKind::SendWhisperAckToServer);
    EXPECT_EQ(plan.effects[1].kind,
              ChatSideEffectKind::SendWhisperGmToReceiverUser);
}

TEST(ChatPlan, WhisperAckForwardsToUser) {
    const auto plan = chat_side_effect_plan(make_chat(chat_whisper_ack));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ChatSideEffectKind::ForwardAckToUser);
}

TEST(ChatPlan, WhisperNackForwardsToUser) {
    const auto plan = chat_side_effect_plan(make_chat(chat_whisper_nack));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ChatSideEffectKind::ForwardNackToUser);
}

TEST(ChatPlan, PartyChatWithoutSenderDrops) {
    ChatRequest request = make_chat(chat_party);
    request.sender_found = false;
    const auto plan = chat_side_effect_plan(request);
    EXPECT_TRUE(plan.drop);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(ChatPlan, PartyChatEmitsMemberAndAgentBroadcast) {
    const auto plan = chat_side_effect_plan(make_chat(chat_party));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ChatSideEffectKind::SendPartyChatToMember);
    EXPECT_EQ(plan.effects[1].kind,
              ChatSideEffectKind::BroadcastPartyChatToOtherAgents);
}

TEST(ChatPlan, GuildChatWithoutSenderDrops) {
    ChatRequest request = make_chat(chat_guild);
    request.sender_found = false;
    const auto plan = chat_side_effect_plan(request);
    EXPECT_TRUE(plan.drop);
}

TEST(ChatPlan, GuildChatBroadcastsToAllMaps) {
    const auto plan = chat_side_effect_plan(make_chat(chat_guild));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ChatSideEffectKind::BroadcastGuildChatToAllMaps);
}

TEST(ChatPlan, GuildUnionChatWithoutSenderDrops) {
    ChatRequest request = make_chat(chat_guild_union);
    request.sender_found = false;
    const auto plan = chat_side_effect_plan(request);
    EXPECT_TRUE(plan.drop);
}

TEST(ChatPlan, GuildUnionChatBroadcastsToAllMaps) {
    const auto plan = chat_side_effect_plan(make_chat(chat_guild_union));
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ChatSideEffectKind::BroadcastGuildUnionChatToAllMaps);
}

TEST(ChatPlan, PlanIsIdempotent) {
    const auto plan1 = chat_side_effect_plan(make_chat(chat_whisper_syn));
    const auto plan2 = chat_side_effect_plan(make_chat(chat_whisper_syn));
    EXPECT_EQ(plan1.dispatched, plan2.dispatched);
    EXPECT_EQ(plan1.drop, plan2.drop);
    ASSERT_EQ(plan1.effects.size(), plan2.effects.size());
    for (std::size_t index = 0; index < plan1.effects.size(); ++index) {
        EXPECT_EQ(plan1.effects[index].kind, plan2.effects[index].kind);
        EXPECT_EQ(plan1.effects[index].nack_code, plan2.effects[index].nack_code);
        EXPECT_EQ(plan1.effects[index].forward_payload, plan2.effects[index].forward_payload);
    }
}
