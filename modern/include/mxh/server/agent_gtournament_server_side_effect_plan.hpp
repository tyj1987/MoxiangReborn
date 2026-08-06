
//
// D4.103 -- AgentGTournament side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GTOURNAMENTUserMsgParser (lines 4295-4497) and MP_GTOURNAMENTServerMsgParser
// (lines 4498-4543). The data plane (classify_agent_gtournament_user +
// classify_agent_gtournament_server) decides which action to take; this header
// captures the ordered side-effect list the orchestrator must execute.
//
// USER side-effect kinds:
//   - drop_no_user_by_charid: user not in objectid table; drop silently.
//   - drop_no_user_by_conn: user not in connection table; drop silently.
//   - drop_event_start_not_gm: event_start/end non-GM; drop silently.
//   - forward_to_user_map_server: raw forward to user->dwMapServerConnectionIndex.
//   - forward_to_tournament_map_server: raw forward to gtmap server connection.
//   - send_*_nack_to_user: build MSG_DWORD (Category=GTOURNAMENT, Protocol=NACK,
//     dwData=gtournament_error) and Send2User(user->dwConnectionIndex, sizeof(MSG_DWORD)).
//   - default_to_trans_to_user: TransToClientMsgParser fallback.
//
// SERVER side-effect kinds:
//   - send_to_user: MSG_DWORD send to user->dwConnectionIndex.
//   - broadcast_to_client: Broadcast2MapServer.
//   - set_user_map_state_and_forward: update wUserMapNum+dwMapServerConnectionIndex
//     to return_map_num/return_map_port, then raw forward MSG_DWORD to user.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_gtournament_server.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class GTournamentUserSideEffectKind : std::uint8_t {
    Drop,                                  // no_user or non-gm
    ForwardRawToUserMap,                   // raw to user->dwMapServerConnectionIndex
    ForwardRawToTournamentMap,             // raw to gtmap server connection
    SendMoveToBattleMapNackToUser,         // MSG_DWORD(Protocol=MOVETOBATTLEMAP_NACK, dwData=0)
    SendStandingInfoNackToUser,            // MSG_DWORD(Protocol=STANDINGINFO_NACK, dwData=0)
    SendBattleJoinNackToUser,              // MSG_DWORD(Protocol=BATTLEJOIN_NACK, dwData=0)
    DefaultTransToUser,                    // TransToClientMsgParser fallback
};

struct GTournamentUserSideEffect final {
    GTournamentUserSideEffectKind kind = GTournamentUserSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint16_t gt_map_num = 0u;
    bool forward_payload = true;
};

struct GTournamentUserSideEffectPlan final {
    std::vector<GTournamentUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool gtournament_user_effect_targets_map(const GTournamentUserSideEffect& e) noexcept {
    return e.kind == GTournamentUserSideEffectKind::ForwardRawToUserMap ||
           e.kind == GTournamentUserSideEffectKind::ForwardRawToTournamentMap;
}

inline bool gtournament_user_effect_targets_user(const GTournamentUserSideEffect& e) noexcept {
    return e.kind == GTournamentUserSideEffectKind::SendMoveToBattleMapNackToUser ||
           e.kind == GTournamentUserSideEffectKind::SendStandingInfoNackToUser ||
           e.kind == GTournamentUserSideEffectKind::SendBattleJoinNackToUser ||
           e.kind == GTournamentUserSideEffectKind::DefaultTransToUser;
}

inline GTournamentUserSideEffectPlan gtournament_user_side_effect_plan(const GTournamentUserAction& a) {
    GTournamentUserSideEffectPlan plan;
    using K = GTournamentUserSideEffectKind;
    using A = GTournamentActionKind;
    switch (a.kind) {
        case A::drop_no_user_by_charid:
        case A::drop_no_user_by_conn:
        case A::drop_event_start_not_gm:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
        case A::forward_to_user_map_server:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToUserMap, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
        case A::forward_to_tournament_map_server:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToTournamentMap, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
        case A::send_movetobattlemap_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendMoveToBattleMapNackToUser, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
        case A::send_standinginfo_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendStandingInfoNackToUser, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
        case A::send_battlejoin_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendBattleJoinNackToUser, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
        case A::send_to_user:
        case A::broadcast_to_client:
        case A::set_user_map_state_and_forward:
        case A::default_to_trans_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::DefaultTransToUser, a.reply_protocol, a.gt_map_num, a.forward_payload});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class GTournamentServerSideEffectKind : std::uint8_t {
    Drop,                                  // no_user_by_charid on server
    SendRawToUser,                         // MSG_DWORD to user->dwConnectionIndex
    BroadcastToMapServers,                 // Broadcast2MapServer raw forward
    SetUserMapStateAndForwardToUser,       // update map state then forward raw to user
};

struct GTournamentServerSideEffect final {
    GTournamentServerSideEffectKind kind = GTournamentServerSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    bool forward_payload = true;
    bool update_map_state = false;
};

struct GTournamentServerSideEffectPlan final {
    std::vector<GTournamentServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool gtournament_server_effect_targets_user(const GTournamentServerSideEffect& e) noexcept {
    return e.kind == GTournamentServerSideEffectKind::SendRawToUser ||
           e.kind == GTournamentServerSideEffectKind::SetUserMapStateAndForwardToUser;
}

inline bool gtournament_server_effect_targets_map(const GTournamentServerSideEffect& e) noexcept {
    return e.kind == GTournamentServerSideEffectKind::BroadcastToMapServers;
}

inline GTournamentServerSideEffectPlan gtournament_server_side_effect_plan(const GTournamentServerAction& a) {
    GTournamentServerSideEffectPlan plan;
    using K = GTournamentServerSideEffectKind;
    using A = GTournamentActionKind;
    switch (a.kind) {
        case A::drop_no_user_by_charid:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.reply_protocol, a.forward_payload, a.update_map_state});
            return plan;
        case A::send_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendRawToUser, a.reply_protocol, a.forward_payload, false});
            return plan;
        case A::broadcast_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToMapServers, a.reply_protocol, a.forward_payload, false});
            return plan;
        case A::set_user_map_state_and_forward:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SetUserMapStateAndForwardToUser, a.reply_protocol, a.forward_payload, true});
            return plan;
        case A::drop_no_user_by_conn:
        case A::drop_event_start_not_gm:
        case A::forward_to_user_map_server:
        case A::forward_to_tournament_map_server:
        case A::send_movetobattlemap_nack_to_user:
        case A::send_standinginfo_nack_to_user:
        case A::send_battlejoin_nack_to_user:
        case A::default_to_trans_to_user:
            // server-side parser should never produce these kinds; treat as drop.
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.reply_protocol, a.forward_payload, a.update_map_state});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server