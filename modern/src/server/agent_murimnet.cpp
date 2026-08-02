
#include "mxh/server/agent_murimnet.hpp"
namespace mxh::server {

// classify_murimnet_user: routes MP_MURIMNET_* user-side messages arriving at
// the AgentServer from the MapServer.
//
// 1:1 with legacy [Server]Agent/MNNetworkMsgParser.cpp routing decisions
// (each protocol is forwarded as-is to the MapServer unless it must be
// refused here at the agent boundary):
//   - ChangetoMurimNet_Syn (0) requires a port (where to send the user). If
//     the legacy MHVerInfo lookup failed to provide one we send Nack.
//   - Connect_Syn (6) and Reconnect_Syn (9) are unconditionally forwarded;
//     the MapServer's MNPlayerManager + PlayRoomManager handle them.
//   - Pr_TeamChange_Syn (21), Chnl_ModeChange (32), Chat_All (48),
//     NotifyToMn_PlayerLogout (58) are forwarded as-is. These are the
//     runtime/room-side commands the modern MurimNetRuntime handles.
//   - All other protocol bytes are dropped (legacy "unknown protocol"
//     behavior; a real runtime would never produce them from the client).
//
// The protocol byte is preserved verbatim in MurimNetAction::protocol so
// the MapServer-side MurimNetRuntime dispatches on the same enumerator.
MurimNetAction classify_murimnet_user(const MurimNetUserRequest& r) {
    if (r.protocol == murimnet_changetomurimnet_syn) {
        return r.lookup.port.has_value()
            ? MurimNetAction{MurimNetActionKind::forward_to_map, r.protocol, r.character_id}
            : MurimNetAction{MurimNetActionKind::send_nack,    murimnet_changetomurimnet_nack, r.character_id};
    }
    if (r.protocol == murimnet_connect_syn ||
        r.protocol == murimnet_reconnect_syn ||
        r.protocol == murimnet_pr_teamchange_syn ||
        r.protocol == murimnet_chnl_modechange ||
        r.protocol == murimnet_chat_all ||
        r.protocol == murimnet_notifytomn_player_logout) {
        return MurimNetAction{MurimNetActionKind::forward_to_map, r.protocol, r.character_id};
    }
    return MurimNetAction{MurimNetActionKind::drop_unknown, r.protocol, r.character_id};
}

// classify_murimnet_server: routes MP_MURIMNET_* server-side messages
// arriving at the AgentServer from the MapServer (the MapServer is the
// authoritative MurimNet endpoint for user routing; the AgentServer only
// relays ACK/NACK and certain broadcast frames).
//
//   - ChangetoMurimNet_Ack (1) and ReturntoMurimNet_Ack (4) require a port
//     to address the user; missing port yields Nack.
//   - Pr_Start (24) and Disconnect_Ack (13) are forwarded.
//   - Pr_TeamChange_Ack (22), Pr_TeamChange_Nack (23), Pr_Start_Ack (26),
//     Pr_Start_Nack (27), Chat_All (48) acks, NotifyToMn_GameEnd (59)
//     are forwarded as-is.
//   - All other protocol bytes are dropped.
MurimNetAction classify_murimnet_server(const MurimNetServerRequest& r) {
    if (r.protocol == murimnet_changetomurimnet_ack ||
        r.protocol == murimnet_returntomurimnet_ack) {
        return r.lookup.port.has_value()
            ? MurimNetAction{MurimNetActionKind::send_ack,  r.protocol, r.character_id}
            : MurimNetAction{MurimNetActionKind::send_nack,
                              r.protocol == murimnet_changetomurimnet_ack
                                  ? murimnet_changetomurimnet_nack
                                  : murimnet_returntomurimnet_nack,
                              r.character_id};
    }
    if (r.protocol == murimnet_pr_start ||
        r.protocol == murimnet_disconnect_ack ||
        r.protocol == murimnet_pr_start_syn ||
        r.protocol == murimnet_pr_start_ack ||
        r.protocol == static_cast<std::uint8_t>(MurimNetProtocol::Pr_Start_Nack) ||
        r.protocol == static_cast<std::uint8_t>(MurimNetProtocol::Pr_TeamChange_Ack) ||
        r.protocol == static_cast<std::uint8_t>(MurimNetProtocol::Pr_TeamChange_Nack) ||
        r.protocol == murimnet_chat_all ||
        r.protocol == murimnet_notifytomn_gameend) {
        return MurimNetAction{MurimNetActionKind::forward_to_map, r.protocol, r.character_id};
    }
    return MurimNetAction{MurimNetActionKind::drop_unknown, r.protocol, r.character_id};
}

}
[[maybe_unused]] constexpr int agent_murimnet_translation_unit_anchor=0;
