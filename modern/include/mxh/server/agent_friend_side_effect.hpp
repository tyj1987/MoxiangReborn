#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

inline constexpr std::uint32_t LEGACY_EFRIEND_ADD_DENY = 3u;
inline constexpr std::uint32_t LEGACY_EFRIEND_OPTION_NO_FRIEND = 7u;
inline constexpr std::uint8_t LEGACY_FRIEND_ADD_ACK = 1u;
inline constexpr std::uint8_t LEGACY_FRIEND_ADD_NACK = 2u;
inline constexpr std::uint8_t LEGACY_FRIEND_ADD_INVITE = 3u;
inline constexpr std::uint8_t LEGACY_FRIEND_ADD_ACCEPT_ACK = 5u;
inline constexpr std::uint8_t LEGACY_FRIEND_ADD_ACCEPT_NACK = 6u;
inline constexpr std::uint8_t LEGACY_FRIEND_LOGIN_NOTIFY = 17u;
inline constexpr std::uint8_t LEGACY_FRIEND_LOGOUT_NOTIFY_TO_CLIENT = 21u;
inline constexpr std::uint8_t LEGACY_FRIEND_LOGOUT_NOTIFY_AGENT_TO_AGENT = 22u;

enum class AgentFriendAction : std::uint8_t {
    Login,
    AddSyn,
    AddAccept,
    AddDeny,
    DelSyn,
    DelIdSyn,
    AddIdSyn,
    LogoutNotifyToAgent,
    LogoutNotifyAgentToAgent,
    ListSyn,
    AddAckToAgent,
    AddNackToAgent,
    AddAcceptToAgent,
    AddAcceptNackToAgent,
    LoginNotifyToAgent,
    AddInviteToAgent,
    AddNack,
};

enum class AgentFriendOutcome : std::uint8_t {
    NotedLogin,
    DatabaseAction,
    SentToUser,
    SentAddDenyNack,
    SentLogoutToClient,
    BroadcastedToAgents,
    SentNoFriendNack,
    Filtered,
    NoUser,
};

struct AgentFriendValidationInput final {
    AgentFriendAction action = AgentFriendAction::Login;
    bool user_found = false;
    bool filter_passed = false;
    bool no_friend_option = false;
};

inline AgentFriendOutcome classify_agent_friend_outcome(
    const AgentFriendValidationInput& input) noexcept {
    switch (input.action) {
        case AgentFriendAction::Login:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            return AgentFriendOutcome::NotedLogin;
        case AgentFriendAction::AddSyn:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            if (!input.filter_passed) return AgentFriendOutcome::Filtered;
            return AgentFriendOutcome::DatabaseAction;
        case AgentFriendAction::AddAccept:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            return AgentFriendOutcome::DatabaseAction;
        case AgentFriendAction::DelSyn:
        case AgentFriendAction::DelIdSyn:
        case AgentFriendAction::AddIdSyn:
            return AgentFriendOutcome::DatabaseAction;
        case AgentFriendAction::AddDeny:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            return AgentFriendOutcome::SentAddDenyNack;
        case AgentFriendAction::LogoutNotifyToAgent:
            if (input.user_found) return AgentFriendOutcome::SentLogoutToClient;
            return AgentFriendOutcome::BroadcastedToAgents;
        case AgentFriendAction::LogoutNotifyAgentToAgent:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            return AgentFriendOutcome::SentLogoutToClient;
        case AgentFriendAction::ListSyn:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            return AgentFriendOutcome::DatabaseAction;
        case AgentFriendAction::AddAckToAgent:
        case AgentFriendAction::AddNackToAgent:
        case AgentFriendAction::AddAcceptToAgent:
        case AgentFriendAction::AddAcceptNackToAgent:
        case AgentFriendAction::LoginNotifyToAgent:
        case AgentFriendAction::AddNack:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            return AgentFriendOutcome::SentToUser;
        case AgentFriendAction::AddInviteToAgent:
            if (!input.user_found) return AgentFriendOutcome::NoUser;
            if (input.no_friend_option) return AgentFriendOutcome::SentNoFriendNack;
            return AgentFriendOutcome::SentToUser;
    }
    return AgentFriendOutcome::NoUser;
}

enum class AgentFriendSideEffectKind : std::uint8_t {
    FriendNotifyLogintoClient,
    NoteIsNewNote,
    CopyNameBuffer,
    FilterCheckName,
    FriendGetUserIDXbyName,
    FriendAddFriend,
    SendAddDenyNack,
    FriendDelFriend,
    FriendDelFriendID,
    FriendIsValidTarget,
    SendLogoutToClient,
    BroadcastLogoutToAgents,
    FriendGetFriendList,
    SendToUser,
    SendInviteToUser,
    BroadcastNoFriendNack,
};

struct AgentFriendSideEffect final {
    AgentFriendSideEffectKind kind =
        AgentFriendSideEffectKind::FriendNotifyLogintoClient;
    std::uint32_t object_id = 0;
    std::uint32_t target_object_id = 0;
    std::uint32_t secondary_object_id = 0;
    std::uint32_t error_code = 0;
    std::uint8_t protocol = 0;
};

struct AgentFriendSideEffectPlan final {
    std::vector<AgentFriendSideEffect> effects;
    bool dispatched = false;
    bool send_nack = false;
};

inline AgentFriendSideEffectPlan agent_friend_side_effect_plan(
    const AgentFriendValidationInput& input,
    std::uint32_t object_id,
    std::uint32_t target_object_id,
    std::uint32_t secondary_object_id) {
    AgentFriendSideEffectPlan plan;
    const AgentFriendOutcome outcome = classify_agent_friend_outcome(input);

    switch (input.action) {
        case AgentFriendAction::Login:
            if (outcome != AgentFriendOutcome::NotedLogin) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::FriendNotifyLogintoClient, object_id, 0u, 0u, 0u, 0u});
            plan.effects.push_back({AgentFriendSideEffectKind::NoteIsNewNote, object_id, 0u, 0u, 0u, 0u});
            return plan;
        case AgentFriendAction::AddSyn:
            if (outcome != AgentFriendOutcome::DatabaseAction) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::CopyNameBuffer, object_id, target_object_id, 0u, 0u, 0u});
            plan.effects.push_back({AgentFriendSideEffectKind::FilterCheckName, object_id, target_object_id, 0u, 0u, 0u});
            plan.effects.push_back({AgentFriendSideEffectKind::FriendGetUserIDXbyName, object_id, target_object_id, 0u, 0u, 0u});
            return plan;
        case AgentFriendAction::AddAccept:
            if (outcome != AgentFriendOutcome::DatabaseAction) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::FriendAddFriend, object_id, target_object_id, 0u, 0u, 0u});
            return plan;
        case AgentFriendAction::AddDeny:
            if (outcome != AgentFriendOutcome::SentAddDenyNack) return plan;
            plan.dispatched = true;
            plan.send_nack = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendAddDenyNack, object_id, target_object_id, 0u, LEGACY_EFRIEND_ADD_DENY, LEGACY_FRIEND_ADD_NACK});
            return plan;
        case AgentFriendAction::DelSyn:
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::FriendDelFriend, object_id, target_object_id, 0u, 0u, 8u});
            return plan;
        case AgentFriendAction::DelIdSyn:
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::FriendDelFriendID, object_id, target_object_id, secondary_object_id, 0u, 11u});
            return plan;
        case AgentFriendAction::AddIdSyn:
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::FriendIsValidTarget, object_id, target_object_id, 0u, 0u, 23u});
            return plan;
        case AgentFriendAction::LogoutNotifyToAgent:
            if (outcome == AgentFriendOutcome::SentLogoutToClient) {
                plan.dispatched = true;
                plan.effects.push_back({AgentFriendSideEffectKind::SendLogoutToClient, object_id, 0u, 0u, 0u, LEGACY_FRIEND_LOGOUT_NOTIFY_TO_CLIENT});
            } else if (outcome == AgentFriendOutcome::BroadcastedToAgents) {
                plan.dispatched = true;
                plan.effects.push_back({AgentFriendSideEffectKind::BroadcastLogoutToAgents, object_id, 0u, 0u, 0u, LEGACY_FRIEND_LOGOUT_NOTIFY_AGENT_TO_AGENT});
            }
            return plan;
        case AgentFriendAction::LogoutNotifyAgentToAgent:
            if (outcome != AgentFriendOutcome::SentLogoutToClient) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendLogoutToClient, object_id, 0u, 0u, 0u, LEGACY_FRIEND_LOGOUT_NOTIFY_TO_CLIENT});
            return plan;
        case AgentFriendAction::ListSyn:
            if (outcome != AgentFriendOutcome::DatabaseAction) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::FriendGetFriendList, object_id, 0u, 0u, 0u, 26u});
            return plan;
        case AgentFriendAction::AddAckToAgent:
            if (outcome != AgentFriendOutcome::SentToUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendToUser, object_id, 0u, 0u, 0u, LEGACY_FRIEND_ADD_ACK});
            return plan;
        case AgentFriendAction::AddNackToAgent:
            if (outcome != AgentFriendOutcome::SentToUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendToUser, object_id, 0u, 0u, 0u, LEGACY_FRIEND_ADD_NACK});
            return plan;
        case AgentFriendAction::AddAcceptToAgent:
            if (outcome != AgentFriendOutcome::SentToUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendToUser, object_id, 0u, 0u, 0u, LEGACY_FRIEND_ADD_ACCEPT_ACK});
            return plan;
        case AgentFriendAction::AddAcceptNackToAgent:
            if (outcome != AgentFriendOutcome::SentToUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendToUser, object_id, 0u, 0u, 0u, LEGACY_FRIEND_ADD_ACCEPT_NACK});
            return plan;
        case AgentFriendAction::LoginNotifyToAgent:
            if (outcome != AgentFriendOutcome::SentToUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendToUser, object_id, 0u, 0u, 0u, LEGACY_FRIEND_LOGIN_NOTIFY});
            return plan;
        case AgentFriendAction::AddInviteToAgent:
            if (outcome == AgentFriendOutcome::SentToUser) {
                plan.dispatched = true;
                plan.effects.push_back({AgentFriendSideEffectKind::SendInviteToUser, object_id, target_object_id, 0u, 0u, LEGACY_FRIEND_ADD_INVITE});
            } else if (outcome == AgentFriendOutcome::SentNoFriendNack) {
                plan.dispatched = true;
                plan.send_nack = true;
                plan.effects.push_back({AgentFriendSideEffectKind::BroadcastNoFriendNack, target_object_id, object_id, 0u, LEGACY_EFRIEND_OPTION_NO_FRIEND, LEGACY_FRIEND_ADD_NACK});
            }
            return plan;
        case AgentFriendAction::AddNack:
            if (outcome != AgentFriendOutcome::SentToUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentFriendSideEffectKind::SendToUser, object_id, 0u, 0u, 0u, LEGACY_FRIEND_ADD_NACK});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
