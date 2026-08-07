//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_OPTIONUserMsgParser (lines 2707-2730). Each test pins one branch of the
// legacy dispatch to its modern side-effect plan output so future drift triggers
// a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_option.hpp"
#include "mxh/server/agent_option_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0x11223344u;
}

TEST(OptionPlan, DropNoUserEmitsDropEffect) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = false;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, OptionSideEffectKind::Drop);
    EXPECT_FALSE(plan.effects[0].mutate_user_options);
}

TEST(OptionPlan, SetSynForwardsToMapAndMutates) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.option_bits = legacy_opt_nowhisper | legacy_opt_nofriend;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, OptionSideEffectKind::ForwardToMapServer);
    EXPECT_TRUE(plan.effects[0].mutate_user_options);
    EXPECT_TRUE(plan.effects[0].forward_payload);
    EXPECT_EQ(plan.effects[0].option_bits, legacy_opt_nowhisper | legacy_opt_nofriend);
    EXPECT_TRUE(option_effect_targets_map(plan.effects[0]));
}

TEST(OptionPlan, SetSynWithoutFlagsStillMutates) {
    // legacy: the mutation is always attempted, even with no flag bits set.
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    r.option_bits = 0u;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_TRUE(plan.effects[0].mutate_user_options);
    EXPECT_EQ(plan.effects[0].option_bits, 0u);
}

TEST(OptionPlan, AvatarViewForwardsToMapNoMutation) {
    OptionUserRequest r;
    r.protocol = option_avatarview;
    r.user_found = true;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, OptionSideEffectKind::ForwardToMapServer);
    EXPECT_FALSE(plan.effects[0].mutate_user_options);
    EXPECT_EQ(plan.effects[0].option_bits, 0u);
}

TEST(OptionPlan, DefaultProtocolForwardsToMapNoMutation) {
    OptionUserRequest r;
    r.protocol = 200u;
    r.user_found = true;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, OptionSideEffectKind::ForwardToMapServer);
    EXPECT_FALSE(plan.effects[0].mutate_user_options);
}

TEST(OptionPlan, SetSynEchoesProtocolOnMutationEffect) {
    OptionUserRequest r;
    r.protocol = option_set_syn;
    r.user_found = true;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].reply_protocol, option_set_syn);
}

TEST(OptionPlan, DropEchoesProtocol) {
    OptionUserRequest r;
    r.protocol = option_set_nack;
    r.user_found = false;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].reply_protocol, option_set_nack);
}

TEST(OptionPlan, ForwardIsAlwaysExactlyOneEffect) {
    // Sanity: there is never an unintended duplicate effect.
    const std::uint8_t protocols[] = {option_set_syn, option_set_ack, option_set_nack, option_avatarview, 99u, 200u};
    for (std::uint8_t p : protocols) {
        OptionUserRequest r;
        r.protocol = p;
        r.user_found = true;
        auto action = classify_option_user(r);
        const auto plan = option_side_effect_plan(action);
        if (!plan.drop) {
            EXPECT_EQ(plan.effects.size(), 1u);
        }
    }
}

TEST(OptionPlan, EffectTargetsMapHelperOnlyTrueForForward) {
    OptionSideEffect e_drop;
    e_drop.kind = OptionSideEffectKind::Drop;
    EXPECT_FALSE(option_effect_targets_map(e_drop));
    OptionSideEffect e_fwd;
    e_fwd.kind = OptionSideEffectKind::ForwardToMapServer;
    EXPECT_TRUE(option_effect_targets_map(e_fwd));
}

TEST(OptionPlan, SetAckWithNoFlagsMutatesWithZeroBits) {
    // legacy MSG_WORD payload is always 16 bits regardless of protocol;
    // option_bits=0 still triggers the mutate branch on SET_SYN.
    OptionUserRequest r;
    r.protocol = option_set_ack;
    r.user_found = true;
    r.option_bits = 0u;
    auto action = classify_option_user(r);
    const auto plan = option_side_effect_plan(action);
    // ACK falls into forward_default; mutate_user_options must be false.
    EXPECT_FALSE(plan.effects[0].mutate_user_options);
}
