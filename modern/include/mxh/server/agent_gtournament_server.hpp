// agent_gtournament_server.hpp - Phase 6.3 AgentGTournament 1:1 port.
//
// Source-of-truth: legacy [Server]Agent/AgentNetworkMsgParser.cpp lines
// 4295-4497 + 4498-4543 (MP_GTOURNAMENTUserMsgParser +
// MP_GTOURNAMENTServerMsgParser). The legacy handler routes tournament
// state (battle/observer/leave/cheat/event) between clients and the
// dedicated tournament map server. We preserve all 1:1 decisions.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_GTOURNAMENT (MP_GTOURNAMENT=60 in MP_CATEGORY).
inline constexpr std::uint8_t gtoournament_category = 60u;

// Sub-protocols within MP_PROTOCOL_GTOURNAMENT (offset 0..N).
inline constexpr std::uint8_t gtournament_movetobattlemap_syn = 0u;
inline constexpr std::uint8_t gtournament_movetobattlemap_nack = 1u;
inline constexpr std::uint8_t gtournament_standinginfo_syn = 2u;
inline constexpr std::uint8_t gtournament_standinginfo_nack = 3u;
inline constexpr std::uint8_t gtournament_battlejoin_syn = 4u;
inline constexpr std::uint8_t gtournament_observerjoin_syn = 5u;
inline constexpr std::uint8_t gtournament_battlejoin_nack = 6u;
inline constexpr std::uint8_t gtournament_leave_syn = 7u;
inline constexpr std::uint8_t gtournament_cheat = 8u;
inline constexpr std::uint8_t gtournament_event_start = 9u;
inline constexpr std::uint8_t gtournament_event_end = 10u;
inline constexpr std::uint8_t gtournament_standinginfo_registed = 11u;
inline constexpr std::uint8_t gtournament_returntomap = 12u;
inline constexpr std::uint8_t gtournament_notify_winlose = 13u;

// Legacy map num for the tournament map.
inline constexpr std::uint16_t gtournament_map_num = 60u;

// Legacy user levels (reused, see agent_cheat.hpp for canonical defs).
inline constexpr std::uint8_t gtournament_user_level_max_for_event_start = 1u;

// Legacy error codes embedded in dwData.
inline constexpr std::uint32_t gtournament_error = 0u;

struct GTournamentUserRequest {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint16_t gt_map_port = 0u;        // 0 if no port lookup succeeded
    std::uint32_t target_map_port = 0u;    // user's map server conn index
    std::uint8_t user_level = 0u;
    bool user_known_by_charid = true;
    bool user_known_by_conn = true;
};

struct GTournamentServerRequest {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t src_object_id = 0u; // for CHEAT
    std::uint16_t return_map_num = 0u;
    std::uint32_t return_map_port = 0u;
    bool target_user_found = true;
    bool return_port_known = true;
};

enum class GTournamentActionKind : std::uint8_t {
    drop_no_user_by_charid,
    drop_no_user_by_conn,
    drop_event_start_not_gm,
    forward_to_user_map_server,
    forward_to_tournament_map_server,
    send_movetobattlemap_nack_to_user,
    send_standinginfo_nack_to_user,
    send_battlejoin_nack_to_user,
    send_to_user,
    broadcast_to_client,
    set_user_map_state_and_forward,
    default_to_trans_to_user,
};

struct GTournamentUserAction {
    GTournamentActionKind kind = GTournamentActionKind::default_to_trans_to_user;
    std::uint8_t reply_protocol = 0u;
    std::uint16_t gt_map_num = gtournament_map_num;
    bool forward_payload = true;
    bool require_user_by_charid = true;
    bool require_user_by_conn = false;
};

struct GTournamentServerAction {
    GTournamentActionKind kind = GTournamentActionKind::default_to_trans_to_user;
    std::uint8_t reply_protocol = 0u;
    bool forward_payload = true;
    bool update_map_state = false;
    bool broadcast_to_map = false;
};

GTournamentUserAction classify_agent_gtournament_user(const GTournamentUserRequest& r);
GTournamentServerAction classify_agent_gtournament_server(const GTournamentServerRequest& r);

}  // namespace mxh::server