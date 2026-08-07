// agent_friend_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// agent_friend_side_effect_plan(). The data plane returns an empty
// plan (no user / filtered) or the action's effect chain (1-3
// entries); this header walks the plan and dispatches each entry to a
// virtual AgentFriendSideEffectSink.
//
// 1:1 invariants (1:1 with legacy agent friend handlers):
//   - Login: FriendNotifyLogintoClient -> NoteIsNewNote.
//   - AddSyn: CopyNameBuffer -> FilterCheckName ->
//     FriendGetUserIDXbyName.
//   - AddAccept: FriendAddFriend; AddDeny: SendAddDenyNack(3, NACK).
//   - DelSyn/DelIdSyn/AddIdSyn: FriendDelFriend(8) / FriendDelFriendID
//     (11) / FriendIsValidTarget(23).
//   - Logout: online -> client(21); offline -> agents(22).
//   - ListSyn: FriendGetFriendList(26).
//   - Agent responses: ADD_ACK(1) / ADD_NACK(2) / ACCEPT_ACK(5) /
//     ACCEPT_NACK(6) / LOGIN_NOTIFY(17).
//   - AddInviteToAgent: SendInviteToUser(3) / no-friend NACK(7, 2).
//
// Pattern mirrors agent_note_side_effect_runtime.hpp (D4.93) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_friend_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AgentFriend side-effect chain.
class AgentFriendSideEffectSink {
public:
    virtual ~AgentFriendSideEffectSink() = default;

    virtual void friend_notify_login_to_client(std::uint32_t object_id) = 0;
    virtual void note_is_new_note(std::uint32_t object_id) = 0;
    virtual void copy_name_buffer(std::uint32_t object_id,
                                  std::uint32_t target_object_id) = 0;
    virtual void filter_check_name(std::uint32_t object_id,
                                   std::uint32_t target_object_id) = 0;
    virtual void friend_get_user_idx_by_name(
        std::uint32_t object_id, std::uint32_t target_object_id) = 0;
    virtual void friend_add_friend(std::uint32_t object_id,
                                   std::uint32_t target_object_id) = 0;
    virtual void send_add_deny_nack(
        std::uint32_t object_id, std::uint32_t target_object_id,
        std::uint32_t error_code, std::uint8_t protocol) = 0;
    virtual void friend_del_friend(std::uint32_t object_id,
                                   std::uint32_t target_object_id,
                                   std::uint8_t protocol) = 0;
    virtual void friend_del_friend_id(
        std::uint32_t object_id, std::uint32_t target_object_id,
        std::uint32_t secondary_object_id, std::uint8_t protocol) = 0;
    virtual void friend_is_valid_target(
        std::uint32_t object_id, std::uint32_t target_object_id,
        std::uint8_t protocol) = 0;
    virtual void send_logout_to_client(std::uint32_t object_id,
                                       std::uint8_t protocol) = 0;
    virtual void broadcast_logout_to_agents(std::uint32_t object_id,
                                            std::uint8_t protocol) = 0;
    virtual void friend_get_friend_list(std::uint32_t object_id,
                                        std::uint8_t protocol) = 0;
    virtual void send_to_user(std::uint32_t object_id,
                              std::uint8_t protocol) = 0;
    virtual void send_invite_to_user(
        std::uint32_t object_id, std::uint32_t target_object_id,
        std::uint8_t protocol) = 0;
    virtual void broadcast_no_friend_nack(
        std::uint32_t target_object_id, std::uint32_t object_id,
        std::uint32_t error_code, std::uint8_t protocol) = 0;
};

struct AgentFriendRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t login_notifies  = 0;
    std::size_t new_note_checks = 0;
    std::size_t copies          = 0;
    std::size_t filters         = 0;
    std::size_t name_resolves   = 0;
    std::size_t add_friends     = 0;
    std::size_t deny_nacks      = 0;
    std::size_t del_friends     = 0;
    std::size_t del_friend_ids  = 0;
    std::size_t valid_targets   = 0;
    std::size_t logout_sends    = 0;
    std::size_t logout_broadcasts = 0;
    std::size_t list_queries    = 0;
    std::size_t user_sends      = 0;
    std::size_t invites         = 0;
    std::size_t no_friend_nacks = 0;
    bool dispatched_flag_consumed = false;
    bool nack_flag_consumed       = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AgentFriendRuntimeOutcome apply_agent_friend_side_effects(
    const AgentFriendSideEffectPlan& plan,
    AgentFriendSideEffectSink& sink) {
    AgentFriendRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AgentFriendSideEffectKind::FriendNotifyLogintoClient:
            sink.friend_notify_login_to_client(effect.object_id);
            ++out.login_notifies;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::NoteIsNewNote:
            sink.note_is_new_note(effect.object_id);
            ++out.new_note_checks;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::CopyNameBuffer:
            sink.copy_name_buffer(effect.object_id,
                                  effect.target_object_id);
            ++out.copies;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FilterCheckName:
            sink.filter_check_name(effect.object_id,
                                   effect.target_object_id);
            ++out.filters;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FriendGetUserIDXbyName:
            sink.friend_get_user_idx_by_name(
                effect.object_id, effect.target_object_id);
            ++out.name_resolves;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FriendAddFriend:
            sink.friend_add_friend(effect.object_id,
                                   effect.target_object_id);
            ++out.add_friends;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::SendAddDenyNack:
            sink.send_add_deny_nack(
                effect.object_id, effect.target_object_id,
                effect.error_code, effect.protocol);
            ++out.deny_nacks;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FriendDelFriend:
            sink.friend_del_friend(effect.object_id,
                                   effect.target_object_id,
                                   effect.protocol);
            ++out.del_friends;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FriendDelFriendID:
            sink.friend_del_friend_id(
                effect.object_id, effect.target_object_id,
                effect.secondary_object_id, effect.protocol);
            ++out.del_friend_ids;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FriendIsValidTarget:
            sink.friend_is_valid_target(
                effect.object_id, effect.target_object_id,
                effect.protocol);
            ++out.valid_targets;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::SendLogoutToClient:
            sink.send_logout_to_client(effect.object_id, effect.protocol);
            ++out.logout_sends;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::BroadcastLogoutToAgents:
            sink.broadcast_logout_to_agents(effect.object_id,
                                            effect.protocol);
            ++out.logout_broadcasts;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::FriendGetFriendList:
            sink.friend_get_friend_list(effect.object_id,
                                        effect.protocol);
            ++out.list_queries;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::SendToUser:
            sink.send_to_user(effect.object_id, effect.protocol);
            ++out.user_sends;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::SendInviteToUser:
            sink.send_invite_to_user(
                effect.object_id, effect.target_object_id,
                effect.protocol);
            ++out.invites;
            ++out.effects_applied;
            break;
        case AgentFriendSideEffectKind::BroadcastNoFriendNack:
            sink.broadcast_no_friend_nack(
                effect.object_id, effect.target_object_id,
                effect.error_code, effect.protocol);
            ++out.no_friend_nacks;
            ++out.effects_applied;
            break;
        }
    }
    out.dispatched_flag_consumed = plan.dispatched;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
