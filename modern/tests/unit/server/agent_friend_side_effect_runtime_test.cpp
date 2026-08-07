// agent_friend_side_effect_runtime_test.cpp
//
// Verifies apply_agent_friend_side_effects() (the runtime orchestrator
// for the legacy agent friend side-effect chains) walks the data-plane
// plan and dispatches each entry: the per-action effect chains in
// legacy order / empty plan on gate failure.

#include <mxh/server/agent_friend_side_effect.hpp>
#include <mxh/server/agent_friend_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AgentFriendAction;
using mxh::server::AgentFriendSideEffectSink;
using mxh::server::AgentFriendValidationInput;
using mxh::server::LEGACY_EFRIEND_ADD_DENY;
using mxh::server::LEGACY_EFRIEND_OPTION_NO_FRIEND;
using mxh::server::LEGACY_FRIEND_ADD_ACCEPT_ACK;
using mxh::server::LEGACY_FRIEND_ADD_ACCEPT_NACK;
using mxh::server::LEGACY_FRIEND_ADD_ACK;
using mxh::server::LEGACY_FRIEND_ADD_INVITE;
using mxh::server::LEGACY_FRIEND_ADD_NACK;
using mxh::server::LEGACY_FRIEND_LOGIN_NOTIFY;
using mxh::server::LEGACY_FRIEND_LOGOUT_NOTIFY_AGENT_TO_AGENT;
using mxh::server::LEGACY_FRIEND_LOGOUT_NOTIFY_TO_CLIENT;
using mxh::server::apply_agent_friend_side_effects;
using mxh::server::agent_friend_side_effect_plan;

class RecordingSink final : public AgentFriendSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_target_id = 0;
    std::uint32_t last_secondary_id = 0;
    std::uint32_t last_error_code = 0;
    std::uint8_t last_protocol = 0;

    void friend_notify_login_to_client(std::uint32_t object_id) override {
        calls.push_back("login");
        last_object_id = object_id;
    }
    void note_is_new_note(std::uint32_t object_id) override {
        calls.push_back("newnote");
        last_object_id = object_id;
    }
    void copy_name_buffer(std::uint32_t object_id,
                          std::uint32_t target_object_id) override {
        calls.push_back("copy");
        last_object_id = object_id;
        last_target_id = target_object_id;
    }
    void filter_check_name(std::uint32_t object_id,
                           std::uint32_t target_object_id) override {
        calls.push_back("filter");
        last_object_id = object_id;
        last_target_id = target_object_id;
    }
    void friend_get_user_idx_by_name(
        std::uint32_t object_id, std::uint32_t target_object_id) override {
        calls.push_back("resolve");
        last_object_id = object_id;
        last_target_id = target_object_id;
    }
    void friend_add_friend(std::uint32_t object_id,
                           std::uint32_t target_object_id) override {
        calls.push_back("addfriend");
        last_object_id = object_id;
        last_target_id = target_object_id;
    }
    void send_add_deny_nack(std::uint32_t object_id,
                            std::uint32_t target_object_id,
                            std::uint32_t error_code,
                            std::uint8_t protocol) override {
        calls.push_back("deny");
        last_object_id = object_id;
        last_target_id = target_object_id;
        last_error_code = error_code;
        last_protocol = protocol;
    }
    void friend_del_friend(std::uint32_t object_id,
                           std::uint32_t target_object_id,
                           std::uint8_t protocol) override {
        calls.push_back("delfriend");
        last_object_id = object_id;
        last_target_id = target_object_id;
        last_protocol = protocol;
    }
    void friend_del_friend_id(
        std::uint32_t object_id, std::uint32_t target_object_id,
        std::uint32_t secondary_object_id, std::uint8_t protocol) override {
        calls.push_back("delfriendid");
        last_object_id = object_id;
        last_target_id = target_object_id;
        last_secondary_id = secondary_object_id;
        last_protocol = protocol;
    }
    void friend_is_valid_target(std::uint32_t object_id,
                                std::uint32_t target_object_id,
                                std::uint8_t protocol) override {
        calls.push_back("validtarget");
        last_object_id = object_id;
        last_target_id = target_object_id;
        last_protocol = protocol;
    }
    void send_logout_to_client(std::uint32_t object_id,
                               std::uint8_t protocol) override {
        calls.push_back("logoutclient");
        last_object_id = object_id;
        last_protocol = protocol;
    }
    void broadcast_logout_to_agents(std::uint32_t object_id,
                                    std::uint8_t protocol) override {
        calls.push_back("logoutagents");
        last_object_id = object_id;
        last_protocol = protocol;
    }
    void friend_get_friend_list(std::uint32_t object_id,
                                std::uint8_t protocol) override {
        calls.push_back("list");
        last_object_id = object_id;
        last_protocol = protocol;
    }
    void send_to_user(std::uint32_t object_id,
                      std::uint8_t protocol) override {
        calls.push_back("user");
        last_object_id = object_id;
        last_protocol = protocol;
    }
    void send_invite_to_user(std::uint32_t object_id,
                             std::uint32_t target_object_id,
                             std::uint8_t protocol) override {
        calls.push_back("invite");
        last_object_id = object_id;
        last_target_id = target_object_id;
        last_protocol = protocol;
    }
    void broadcast_no_friend_nack(std::uint32_t target_object_id,
                                  std::uint32_t object_id,
                                  std::uint32_t error_code,
                                  std::uint8_t protocol) override {
        calls.push_back("nofriend");
        last_target_id = target_object_id;
        last_object_id = object_id;
        last_error_code = error_code;
        last_protocol = protocol;
    }
};

}  // namespace

TEST(ApplyAgentFriendSideEffects, LoginEmitsNotifyThenNewNoteCheck) {
    AgentFriendValidationInput in;
    in.action = AgentFriendAction::Login;
    in.user_found = true;
    auto plan = agent_friend_side_effect_plan(
        in, /*object_id=*/0x00280029u, 0, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 2u);

    RecordingSink sink;
    auto out = apply_agent_friend_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.login_notifies, 1u);
    EXPECT_EQ(out.new_note_checks, 1u);
    EXPECT_TRUE(out.dispatched_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"login", "newnote"}));
    EXPECT_EQ(sink.last_object_id, 0x00280029u);
}

TEST(ApplyAgentFriendSideEffects, AddSynEmitsCopyFilterResolveInOrder) {
    AgentFriendValidationInput in;
    in.action = AgentFriendAction::AddSyn;
    in.user_found = true;
    in.filter_passed = true;
    auto plan = agent_friend_side_effect_plan(
        in, /*object_id=*/7, /*target=*/8, 0);
    ASSERT_EQ(plan.effects.size(), 3u);

    RecordingSink sink;
    auto out = apply_agent_friend_side_effects(plan, sink);
    EXPECT_EQ(out.copies, 1u);
    EXPECT_EQ(out.filters, 1u);
    EXPECT_EQ(out.name_resolves, 1u);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"copy", "filter", "resolve"}));
    EXPECT_EQ(sink.last_target_id, 8u);
}

TEST(ApplyAgentFriendSideEffects, AddAcceptAndAddDenyBranches) {
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddAccept;
        in.user_found = true;
        auto plan = agent_friend_side_effect_plan(in, 7, 9, 0);
        RecordingSink sink;
        auto out = apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(out.add_friends, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"addfriend"}));
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddDeny;
        in.user_found = true;
        auto plan = agent_friend_side_effect_plan(in, 7, 9, 0);
        EXPECT_TRUE(plan.send_nack);
        RecordingSink sink;
        auto out = apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(out.deny_nacks, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"deny"}));
        EXPECT_EQ(sink.last_error_code, LEGACY_EFRIEND_ADD_DENY);
        EXPECT_EQ(sink.last_protocol, LEGACY_FRIEND_ADD_NACK);
    }
}

TEST(ApplyAgentFriendSideEffects, DeleteVariantsUseLegacyProtocols) {
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::DelSyn;
        auto plan = agent_friend_side_effect_plan(in, 1, 2, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"delfriend"}));
        EXPECT_EQ(sink.last_protocol, 8u);
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::DelIdSyn;
        auto plan = agent_friend_side_effect_plan(in, 1, 2, 3);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"delfriendid"}));
        EXPECT_EQ(sink.last_secondary_id, 3u);
        EXPECT_EQ(sink.last_protocol, 11u);
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddIdSyn;
        auto plan = agent_friend_side_effect_plan(in, 1, 2, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"validtarget"}));
        EXPECT_EQ(sink.last_protocol, 23u);
    }
}

TEST(ApplyAgentFriendSideEffects, LogoutRoutes) {
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::LogoutNotifyToAgent;
        in.user_found = true;
        auto plan = agent_friend_side_effect_plan(in, 1, 0, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"logoutclient"}));
        EXPECT_EQ(sink.last_protocol,
                  LEGACY_FRIEND_LOGOUT_NOTIFY_TO_CLIENT);
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::LogoutNotifyToAgent;
        in.user_found = false;
        auto plan = agent_friend_side_effect_plan(in, 1, 0, 0);
        RecordingSink sink;
        auto out = apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(out.logout_broadcasts, 1u);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"logoutagents"}));
        EXPECT_EQ(sink.last_protocol,
                  LEGACY_FRIEND_LOGOUT_NOTIFY_AGENT_TO_AGENT);
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::LogoutNotifyAgentToAgent;
        in.user_found = true;
        auto plan = agent_friend_side_effect_plan(in, 1, 0, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"logoutclient"}));
    }
}

TEST(ApplyAgentFriendSideEffects, AgentResponsesUseLegacyProtocols) {
    struct Case {
        AgentFriendAction action;
        std::uint8_t expected_protocol;
    };
    const Case cases[] = {
        {AgentFriendAction::AddAckToAgent, LEGACY_FRIEND_ADD_ACK},
        {AgentFriendAction::AddNackToAgent, LEGACY_FRIEND_ADD_NACK},
        {AgentFriendAction::AddAcceptToAgent, LEGACY_FRIEND_ADD_ACCEPT_ACK},
        {AgentFriendAction::AddAcceptNackToAgent, LEGACY_FRIEND_ADD_ACCEPT_NACK},
        {AgentFriendAction::LoginNotifyToAgent, LEGACY_FRIEND_LOGIN_NOTIFY},
        {AgentFriendAction::AddNack, LEGACY_FRIEND_ADD_NACK},
    };
    for (const auto& c : cases) {
        AgentFriendValidationInput in;
        in.action = c.action;
        in.user_found = true;
        auto plan = agent_friend_side_effect_plan(in, 7, 0, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"user"}));
        EXPECT_EQ(sink.last_protocol, c.expected_protocol);
    }
}

TEST(ApplyAgentFriendSideEffects, InviteAndListBranches) {
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddInviteToAgent;
        in.user_found = true;
        in.no_friend_option = false;
        auto plan = agent_friend_side_effect_plan(in, 7, 9, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"invite"}));
        EXPECT_EQ(sink.last_target_id, 9u);
        EXPECT_EQ(sink.last_protocol, LEGACY_FRIEND_ADD_INVITE);
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddInviteToAgent;
        in.user_found = true;
        in.no_friend_option = true;
        auto plan = agent_friend_side_effect_plan(in, 7, 9, 0);
        EXPECT_TRUE(plan.send_nack);
        RecordingSink sink;
        auto out = apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(out.no_friend_nacks, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"nofriend"}));
        EXPECT_EQ(sink.last_target_id, 9u);
        EXPECT_EQ(sink.last_object_id, 7u);
        EXPECT_EQ(sink.last_error_code, LEGACY_EFRIEND_OPTION_NO_FRIEND);
        EXPECT_EQ(sink.last_protocol, LEGACY_FRIEND_ADD_NACK);
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::ListSyn;
        in.user_found = true;
        auto plan = agent_friend_side_effect_plan(in, 7, 0, 0);
        RecordingSink sink;
        (void)apply_agent_friend_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"list"}));
        EXPECT_EQ(sink.last_protocol, 26u);
    }
}

TEST(ApplyAgentFriendSideEffects, NoUserAndFilteredEmitEmptyPlans) {
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::Login;
        in.user_found = false;
        auto plan = agent_friend_side_effect_plan(in, 1, 0, 0);
        EXPECT_FALSE(plan.dispatched);
        EXPECT_TRUE(plan.effects.empty());
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddSyn;
        in.user_found = true;
        in.filter_passed = false;
        auto plan = agent_friend_side_effect_plan(in, 1, 0, 0);
        EXPECT_TRUE(plan.effects.empty());
    }
    {
        AgentFriendValidationInput in;
        in.action = AgentFriendAction::AddAcceptToAgent;
        in.user_found = false;
        auto plan = agent_friend_side_effect_plan(in, 1, 0, 0);
        EXPECT_TRUE(plan.effects.empty());
    }
}

TEST(ApplyAgentFriendSideEffects, EmptyPlanIsNoOp) {
    mxh::server::AgentFriendSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_agent_friend_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.login_notifies, 0u);
    EXPECT_EQ(out.new_note_checks, 0u);
    EXPECT_EQ(out.copies, 0u);
    EXPECT_EQ(out.filters, 0u);
    EXPECT_EQ(out.name_resolves, 0u);
    EXPECT_EQ(out.add_friends, 0u);
    EXPECT_EQ(out.deny_nacks, 0u);
    EXPECT_EQ(out.del_friends, 0u);
    EXPECT_EQ(out.del_friend_ids, 0u);
    EXPECT_EQ(out.valid_targets, 0u);
    EXPECT_EQ(out.logout_sends, 0u);
    EXPECT_EQ(out.logout_broadcasts, 0u);
    EXPECT_EQ(out.list_queries, 0u);
    EXPECT_EQ(out.user_sends, 0u);
    EXPECT_EQ(out.invites, 0u);
    EXPECT_EQ(out.no_friend_nacks, 0u);
    EXPECT_FALSE(out.dispatched_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
