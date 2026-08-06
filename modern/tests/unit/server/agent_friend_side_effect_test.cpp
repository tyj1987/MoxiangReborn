#include <array>
#include <cstddef>

#include <mxh/server/agent_friend_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AgentFriendValidationInput action_input(AgentFriendAction action) {
    AgentFriendValidationInput input{};
    input.action = action;
    return input;
}

AgentFriendValidationInput user_action(AgentFriendAction action) {
    auto input = action_input(action);
    input.user_found = true;
    return input;
}

AgentFriendValidationInput add_syn_ok() {
    auto input = user_action(AgentFriendAction::AddSyn);
    input.filter_passed = true;
    return input;
}

TEST(AgentFriendOutcome, LoginRequiresUserAndNotesLogin) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::Login)),
              AgentFriendOutcome::NotedLogin);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::Login)),
              AgentFriendOutcome::NoUser);
}

TEST(AgentFriendOutcome, AddSynRequiresUserAndFilter) {
    EXPECT_EQ(classify_agent_friend_outcome(add_syn_ok()),
              AgentFriendOutcome::DatabaseAction);
    auto no_user = add_syn_ok();
    no_user.user_found = false;
    EXPECT_EQ(classify_agent_friend_outcome(no_user), AgentFriendOutcome::NoUser);
    auto filtered = add_syn_ok();
    filtered.filter_passed = false;
    EXPECT_EQ(classify_agent_friend_outcome(filtered), AgentFriendOutcome::Filtered);
}

TEST(AgentFriendOutcome, AddAcceptRequiresUser) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::AddAccept)),
              AgentFriendOutcome::DatabaseAction);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::AddAccept)),
              AgentFriendOutcome::NoUser);
}

TEST(AgentFriendOutcome, AddDenyRequiresRecipientAndSendsNack) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::AddDeny)),
              AgentFriendOutcome::SentAddDenyNack);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::AddDeny)),
              AgentFriendOutcome::NoUser);
}

TEST(AgentFriendOutcome, DatabaseActionsWithoutLookupGateDispatch) {
    const std::array actions = {
        AgentFriendAction::DelSyn,
        AgentFriendAction::DelIdSyn,
        AgentFriendAction::AddIdSyn,
    };
    for (const auto action : actions) {
        EXPECT_EQ(classify_agent_friend_outcome(action_input(action)),
                  AgentFriendOutcome::DatabaseAction);
    }
}

TEST(AgentFriendOutcome, LogoutToAgentUsesDirectOrAgentBroadcast) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::LogoutNotifyToAgent)),
              AgentFriendOutcome::SentLogoutToClient);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::LogoutNotifyToAgent)),
              AgentFriendOutcome::BroadcastedToAgents);
}

TEST(AgentFriendOutcome, LogoutAgentToAgentDropsWhenRecipientMissing) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::LogoutNotifyAgentToAgent)),
              AgentFriendOutcome::SentLogoutToClient);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::LogoutNotifyAgentToAgent)),
              AgentFriendOutcome::NoUser);
}

TEST(AgentFriendOutcome, ListRequiresUser) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::ListSyn)),
              AgentFriendOutcome::DatabaseAction);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::ListSyn)),
              AgentFriendOutcome::NoUser);
}

TEST(AgentFriendOutcome, AgentResponsesRequireRecipient) {
    const std::array actions = {
        AgentFriendAction::AddAckToAgent,
        AgentFriendAction::AddNackToAgent,
        AgentFriendAction::AddAcceptToAgent,
        AgentFriendAction::AddAcceptNackToAgent,
        AgentFriendAction::LoginNotifyToAgent,
        AgentFriendAction::AddNack,
    };
    for (const auto action : actions) {
        EXPECT_EQ(classify_agent_friend_outcome(user_action(action)),
                  AgentFriendOutcome::SentToUser);
        EXPECT_EQ(classify_agent_friend_outcome(action_input(action)),
                  AgentFriendOutcome::NoUser);
    }
}

TEST(AgentFriendOutcome, InviteChoosesNormalOrNoFriendNack) {
    EXPECT_EQ(classify_agent_friend_outcome(
                  user_action(AgentFriendAction::AddInviteToAgent)),
              AgentFriendOutcome::SentToUser);
    auto blocked = user_action(AgentFriendAction::AddInviteToAgent);
    blocked.no_friend_option = true;
    EXPECT_EQ(classify_agent_friend_outcome(blocked),
              AgentFriendOutcome::SentNoFriendNack);
    EXPECT_EQ(classify_agent_friend_outcome(
                  action_input(AgentFriendAction::AddInviteToAgent)),
              AgentFriendOutcome::NoUser);
}

TEST(AgentFriendPlan, LoginEmitsLoginNotificationThenNewNoteCheck) {
    const auto plan = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::Login), 10u, 0u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentFriendSideEffectKind::FriendNotifyLogintoClient);
    EXPECT_EQ(plan.effects[1].kind, AgentFriendSideEffectKind::NoteIsNewNote);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
}

TEST(AgentFriendPlan, AddSynCopiesFiltersThenResolvesName) {
    const auto plan = agent_friend_side_effect_plan(add_syn_ok(), 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::CopyNameBuffer);
    EXPECT_EQ(plan.effects[1].kind, AgentFriendSideEffectKind::FilterCheckName);
    EXPECT_EQ(plan.effects[2].kind, AgentFriendSideEffectKind::FriendGetUserIDXbyName);
    EXPECT_EQ(plan.effects[2].object_id, 10u);
    EXPECT_EQ(plan.effects[2].target_object_id, 20u);
}

TEST(AgentFriendPlan, AddSynRejectedByFilterHasNoEffects) {
    auto input = add_syn_ok();
    input.filter_passed = false;
    const auto plan = agent_friend_side_effect_plan(input, 10u, 20u, 0u);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(AgentFriendPlan, AddAcceptEmitsDatabaseInsert) {
    const auto plan = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::AddAccept), 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::FriendAddFriend);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].target_object_id, 20u);
}

TEST(AgentFriendPlan, AddDenyEmitsLegacyNack) {
    const auto plan = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::AddDeny), 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::SendAddDenyNack);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EFRIEND_ADD_DENY);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_FRIEND_ADD_NACK);
}

TEST(AgentFriendPlan, DeleteByNameEmitsDatabaseDelete) {
    const auto plan = agent_friend_side_effect_plan(
        action_input(AgentFriendAction::DelSyn), 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::FriendDelFriend);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
}

TEST(AgentFriendPlan, DeleteByIdCarriesBothIds) {
    const auto plan = agent_friend_side_effect_plan(
        action_input(AgentFriendAction::DelIdSyn), 10u, 20u, 30u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::FriendDelFriendID);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].target_object_id, 20u);
    EXPECT_EQ(plan.effects[0].secondary_object_id, 30u);
}

TEST(AgentFriendPlan, AddIdValidatesBothIds) {
    const auto plan = agent_friend_side_effect_plan(
        action_input(AgentFriendAction::AddIdSyn), 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::FriendIsValidTarget);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].target_object_id, 20u);
}

TEST(AgentFriendPlan, LogoutToAgentSendsToClientWhenOnline) {
    const auto plan = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::LogoutNotifyToAgent), 10u, 0u, 0u);
    ASSERT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::SendLogoutToClient);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_FRIEND_LOGOUT_NOTIFY_TO_CLIENT);
}

TEST(AgentFriendPlan, LogoutToAgentBroadcastsWhenOffline) {
    const auto plan = agent_friend_side_effect_plan(
        action_input(AgentFriendAction::LogoutNotifyToAgent), 10u, 0u, 0u);
    ASSERT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::BroadcastLogoutToAgents);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_FRIEND_LOGOUT_NOTIFY_AGENT_TO_AGENT);
}

TEST(AgentFriendPlan, AgentLogoutOnlySendsWhenRecipientExists) {
    const auto sent = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::LogoutNotifyAgentToAgent), 10u, 0u, 0u);
    ASSERT_TRUE(sent.dispatched);
    EXPECT_EQ(sent.effects[0].kind, AgentFriendSideEffectKind::SendLogoutToClient);
    const auto dropped = agent_friend_side_effect_plan(
        action_input(AgentFriendAction::LogoutNotifyAgentToAgent), 10u, 0u, 0u);
    EXPECT_FALSE(dropped.dispatched);
    EXPECT_TRUE(dropped.effects.empty());
}

TEST(AgentFriendPlan, ListEmitsFriendListLookup) {
    const auto plan = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::ListSyn), 10u, 0u, 0u);
    ASSERT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::FriendGetFriendList);
}

TEST(AgentFriendPlan, AgentResponsesUseLegacyProtocols) {
    const std::array cases = {
        std::pair{AgentFriendAction::AddAckToAgent, LEGACY_FRIEND_ADD_ACK},
        std::pair{AgentFriendAction::AddNackToAgent, LEGACY_FRIEND_ADD_NACK},
        std::pair{AgentFriendAction::AddAcceptToAgent, LEGACY_FRIEND_ADD_ACCEPT_ACK},
        std::pair{AgentFriendAction::AddAcceptNackToAgent, LEGACY_FRIEND_ADD_ACCEPT_NACK},
        std::pair{AgentFriendAction::LoginNotifyToAgent, LEGACY_FRIEND_LOGIN_NOTIFY},
        std::pair{AgentFriendAction::AddNack, LEGACY_FRIEND_ADD_NACK},
    };
    for (const auto [action, protocol] : cases) {
        const auto plan = agent_friend_side_effect_plan(
            user_action(action), 10u, 0u, 0u);
        ASSERT_TRUE(plan.dispatched);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::SendToUser);
        EXPECT_EQ(plan.effects[0].protocol, protocol);
    }
}

TEST(AgentFriendPlan, InviteSendsToOnlineRecipient) {
    const auto plan = agent_friend_side_effect_plan(
        user_action(AgentFriendAction::AddInviteToAgent), 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::SendInviteToUser);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_FRIEND_ADD_INVITE);
}

TEST(AgentFriendPlan, InviteNoFriendOptionBroadcastsNack) {
    auto input = user_action(AgentFriendAction::AddInviteToAgent);
    input.no_friend_option = true;
    const auto plan = agent_friend_side_effect_plan(input, 10u, 20u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.effects[0].kind, AgentFriendSideEffectKind::BroadcastNoFriendNack);
    EXPECT_EQ(plan.effects[0].object_id, 20u);
    EXPECT_EQ(plan.effects[0].target_object_id, 10u);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EFRIEND_OPTION_NO_FRIEND);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_FRIEND_ADD_NACK);
}

TEST(AgentFriendPlan, MissingUserLeavesGatedActionsEmpty) {
    const std::array actions = {
        AgentFriendAction::Login,
        AgentFriendAction::AddSyn,
        AgentFriendAction::AddAccept,
        AgentFriendAction::AddDeny,
        AgentFriendAction::ListSyn,
        AgentFriendAction::AddAckToAgent,
        AgentFriendAction::AddInviteToAgent,
        AgentFriendAction::AddNack,
        AgentFriendAction::AddAcceptNackToAgent,
        AgentFriendAction::LoginNotifyToAgent,
        AgentFriendAction::AddNackToAgent,
        AgentFriendAction::LogoutNotifyAgentToAgent,
    };
    for (const auto action : actions) {
        const auto plan = agent_friend_side_effect_plan(
            action_input(action), 10u, 20u, 30u);
        EXPECT_FALSE(plan.dispatched);
        EXPECT_TRUE(plan.effects.empty());
    }
}

TEST(AgentFriendPlan, PlanIsIdempotent) {
    const auto input = add_syn_ok();
    const auto first = agent_friend_side_effect_plan(input, 10u, 20u, 30u);
    const auto second = agent_friend_side_effect_plan(input, 10u, 20u, 30u);
    EXPECT_EQ(first.dispatched, second.dispatched);
    EXPECT_EQ(first.send_nack, second.send_nack);
    ASSERT_EQ(first.effects.size(), second.effects.size());
    for (std::size_t index = 0; index < first.effects.size(); ++index) {
        EXPECT_EQ(first.effects[index].kind, second.effects[index].kind);
        EXPECT_EQ(first.effects[index].object_id, second.effects[index].object_id);
        EXPECT_EQ(first.effects[index].target_object_id, second.effects[index].target_object_id);
        EXPECT_EQ(first.effects[index].secondary_object_id, second.effects[index].secondary_object_id);
        EXPECT_EQ(first.effects[index].error_code, second.effects[index].error_code);
        EXPECT_EQ(first.effects[index].protocol, second.effects[index].protocol);
    }
}
