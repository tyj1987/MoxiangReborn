// D4.158 AgentOption data plane tests.
//
// 1:1 port of MP_OPTIONUserMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 2707-2730.

#include <mxh/server/agent_option.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0x12345678u;
}

TEST(AgentOptionClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(option_category, 36u);
}

TEST(AgentOptionClassify, ProtocolConstantsAreContiguousFromZero) {
    EXPECT_EQ(option_set_syn, 0u);
    EXPECT_EQ(option_set_ack, 1u);
    EXPECT_EQ(option_set_nack, 2u);
    EXPECT_EQ(option_avatarview, 3u);
}

TEST(AgentOptionClassify, SetSynWithoutUserDrops) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.object_id = kObjectId;
    r.user_found = false;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, option_set_syn);
    EXPECT_EQ(a.object_id, kObjectId);
    EXPECT_FALSE(a.forward_to_map);
}

TEST(AgentOptionClassify, SetSynWithNoFlagsForwardsFlat) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    r.option_bits = 0u;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::forward_set_syn_to_map);
    EXPECT_EQ(a.reply_protocol, option_set_syn);
    EXPECT_FALSE(a.no_whisper);
    EXPECT_FALSE(a.no_friend);
    EXPECT_TRUE(a.forward_to_map);
    EXPECT_EQ(a.option_bits, 0u);
}

TEST(AgentOptionClassify, SetSynWithNoWhisperOnlySetsWhisperFlag) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.option_bits = legacy_opt_nowhisper;
    auto a = classify_option_user(r);
    EXPECT_TRUE(a.no_whisper);
    EXPECT_FALSE(a.no_friend);
    EXPECT_EQ(a.option_bits, legacy_opt_nowhisper);
}

TEST(AgentOptionClassify, SetSynWithNoFriendOnlySetsFriendFlag) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.option_bits = legacy_opt_nofriend;
    auto a = classify_option_user(r);
    EXPECT_FALSE(a.no_whisper);
    EXPECT_TRUE(a.no_friend);
}

TEST(AgentOptionClassify, SetSynWithBothFlagsSetsBoth) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.option_bits = static_cast<std::uint16_t>(legacy_opt_nowhisper | legacy_opt_nofriend);
    auto a = classify_option_user(r);
    EXPECT_TRUE(a.no_whisper);
    EXPECT_TRUE(a.no_friend);
}

TEST(AgentOptionClassify, SetSynPreservesObjectId) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.object_id = kObjectId;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.object_id, kObjectId);
}

TEST(AgentOptionClassify, AvatarViewForwardsToMap) {
    OptionUserRequest r;
    r.protocol = option_avatarview;
    r.user_found = true;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::forward_avatarview_to_map);
    EXPECT_EQ(a.reply_protocol, option_avatarview);
    EXPECT_TRUE(a.forward_to_map);
}

TEST(AgentOptionClassify, UnknownProtocolForwardsByDefault) {
    OptionUserRequest r;
    r.protocol = 200u;
    r.user_found = true;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::forward_default);
    EXPECT_EQ(a.reply_protocol, 200u);
    EXPECT_TRUE(a.forward_to_map);
}

TEST(AgentOptionClassify, UnknownProtocolWithoutUserStillDrops) {
    OptionUserRequest r;
    r.protocol = 200u;
    r.user_found = false;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::drop_no_user);
    EXPECT_FALSE(a.forward_to_map);
}

TEST(AgentOptionClassify, AvatarViewWithoutUserDrops) {
    OptionUserRequest r;
    r.protocol = option_avatarview;
    r.user_found = false;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::drop_no_user);
    EXPECT_FALSE(a.forward_to_map);
}

TEST(AgentOptionClassify, OptBitConstantsAreDistinct) {
    EXPECT_NE(legacy_opt_nowhisper, legacy_opt_nofriend);
    EXPECT_EQ(legacy_opt_nowhisper & legacy_opt_nofriend, 0u);
}

TEST(AgentOptionClassify, SetSynWithUnknownBitsDoesNotSetLegacyFlags) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.option_bits = 0xFFFCu;  // all bits except the two legacy OPT flags
    auto a = classify_option_user(r);
    EXPECT_FALSE(a.no_whisper);
    EXPECT_FALSE(a.no_friend);
    EXPECT_EQ(a.option_bits, 0xFFFCu);
}

TEST(AgentOptionClassify, ReplyProtocolEchoesOnDrop) {
    OptionUserRequest r;
    r.protocol = option_set_nack;
    r.user_found = false;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, option_set_nack);
}

TEST(AgentOptionClassify, SetSynWithZeroObjectIdStillForwards) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.object_id = 0u;
    r.option_bits = legacy_opt_nowhisper;
    auto a = classify_option_user(r);
    EXPECT_EQ(a.kind, OptionUserActionKind::forward_set_syn_to_map);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_TRUE(a.no_whisper);
    EXPECT_TRUE(a.forward_to_map);
}
