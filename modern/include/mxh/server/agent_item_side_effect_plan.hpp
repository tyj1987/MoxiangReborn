
//
// D4.111 -- AgentItem side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_ITEMUserMsgParser (lines 4148-4206) and MP_ITEMUserMsgParserExt (pass-through),
// MP_ITEMServerMsgParser (lines 4214-4286) and MP_ITEMServerMsgParserExt (pass-through).
// The data plane (classify_item_user + classify_item_server) decides which action
// to take; this header captures the ordered side-effect list the orchestrator must execute.
//
// USER side-effects:
//   - forward_to_map: unconditional raw forwarding to map (default + Ext pass-through).
//   - forward_to_map_if_name_valid: raw forward after name validation passes for NCHANGE_SYN.
//   - send_nack_to_user: build MSG_DWORD (Protocol=NCHANGE_NACK, dwData=item_name_nack_code)
//     to user->dwConnectionIndex (NACK on missing user / bad name length / bad char / invalid).
//   - send_chase_lookup: build CHASE_SYN lookup (passes through to map after lookup).
//
// SERVER side-effects:
//   - forward_to_user: raw forward MSG_DWORD to user->dwConnectionIndex (CHASE_ACK).
//   - send_chase_nack_to_user: rewrite dwData to item_chase_nack_data and forward to user.
//   - shout_ack_with_broadcast: if shout_buffer_full, raw forward MSG_DWORD to client;
//     otherwise store in shout buffer and ACK.
//   - shout_add_only: add to shout buffer only (SHOUT_SENDSERVER).
//   - forward_to_client: TransToClientMsgParser fallback (default + Ext pass-through).
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_item.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class ItemUserSideEffectKind : std::uint8_t {
    ForwardRawToMap,                       // unconditional raw forward (default + Ext)
    ForwardNChangeSynToMap,                // raw forward NCHANGE_SYN after name validation
    SendNackToUser,                        // MSG_DWORD(NCHANGE_NACK, dwData=item_name_nack_code)
    SendChaseLookup,                       // CHASE_SYN lookup pass-through to map
};

struct ItemUserSideEffect final {
    ItemUserSideEffectKind kind = ItemUserSideEffectKind::ForwardRawToMap;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t error_code = 0u;
    bool drop_payload = false;
};

struct ItemUserSideEffectPlan final {
    std::vector<ItemUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool item_user_effect_targets_map(const ItemUserSideEffect& e) noexcept {
    return e.kind == ItemUserSideEffectKind::ForwardRawToMap ||
           e.kind == ItemUserSideEffectKind::ForwardNChangeSynToMap ||
           e.kind == ItemUserSideEffectKind::SendChaseLookup;
}

inline bool item_user_effect_targets_user(const ItemUserSideEffect& e) noexcept {
    return e.kind == ItemUserSideEffectKind::SendNackToUser;
}

inline ItemUserSideEffectPlan item_user_side_effect_plan(const ItemUserAction& a) {
    ItemUserSideEffectPlan plan;
    using K = ItemUserSideEffectKind;
    using A = ItemUserActionKind;
    switch (a.kind) {
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id, 0u, a.drop_payload});
            return plan;
        case A::forward_to_map_if_name_valid:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardNChangeSynToMap, a.protocol, a.object_id, 0u, a.drop_payload});
            return plan;
        case A::send_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendNackToUser, a.protocol, a.object_id, a.error_code, false});
            return plan;
        case A::send_chase_lookup:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendChaseLookup, a.protocol, a.object_id, 0u, a.drop_payload});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class ItemServerSideEffectKind : std::uint8_t {
    ForwardRawToUser,                      // CHASE_ACK raw forward to user
    SendChaseNackToUser,                   // CHASE_NACK with dwData rewritten to item_chase_nack_data
    ShoutAckWithBroadcast,                 // SHOUT_ACK: ack + broadcast (or ack-only if buffer full)
    ShoutAddOnly,                          // SHOUT_SENDSERVER: add to shout buffer
    ForwardRawToClient,                    // TransToClientMsgParser fallback (default + Ext)
};

struct ItemServerSideEffect final {
    ItemServerSideEffectKind kind = ItemServerSideEffectKind::ForwardRawToClient;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t alternate_data = 0u;
    bool broadcast_shout = false;
};

struct ItemServerSideEffectPlan final {
    std::vector<ItemServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool item_server_effect_targets_user(const ItemServerSideEffect& e) noexcept {
    return e.kind == ItemServerSideEffectKind::ForwardRawToUser ||
           e.kind == ItemServerSideEffectKind::SendChaseNackToUser;
}

inline bool item_server_effect_targets_client(const ItemServerSideEffect& e) noexcept {
    return e.kind == ItemServerSideEffectKind::ForwardRawToClient ||
           e.kind == ItemServerSideEffectKind::ShoutAckWithBroadcast;
}

inline ItemServerSideEffectPlan item_server_side_effect_plan(const ItemServerAction& a) {
    ItemServerSideEffectPlan plan;
    using K = ItemServerSideEffectKind;
    using A = ItemServerActionKind;
    switch (a.kind) {
        case A::forward_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToUser, a.protocol, a.object_id, a.alternate_data, false});
            return plan;
        case A::send_chase_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendChaseNackToUser, a.protocol, a.object_id, a.alternate_data, false});
            return plan;
        case A::shout_ack_with_broadcast:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ShoutAckWithBroadcast, a.protocol, a.object_id, 0u, a.broadcast_shout});
            return plan;
        case A::shout_add_only:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ShoutAddOnly, a.protocol, a.object_id, 0u, false});
            return plan;
        case A::forward_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, a.protocol, a.object_id, 0u, false});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server