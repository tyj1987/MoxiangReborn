// agent_gtournament_server.cpp - Phase 6.3 AgentGTournament 1:1 port.

#include "mxh/server/agent_gtournament_server.hpp"

namespace mxh::server {

GTournamentUserAction classify_agent_gtournament_user(const GTournamentUserRequest& r) {
    using K = GTournamentActionKind;
    switch (r.protocol) {
    case gtournament_movetobattlemap_syn:
        if (!r.user_known_by_charid) {
            return {K::drop_no_user_by_charid, 0u, 0u, false, true, false};
        }
        if (r.gt_map_port == 0u) {
            return {K::send_movetobattlemap_nack_to_user, gtournament_movetobattlemap_nack, gtournament_map_num, false, true, false};
        }
        return {K::forward_to_tournament_map_server, gtournament_movetobattlemap_syn, gtournament_map_num, true, true, false};

    case gtournament_standinginfo_syn:
        if (!r.user_known_by_charid) {
            return {K::drop_no_user_by_charid, 0u, 0u, false, true, false};
        }
        if (r.gt_map_port == 0u) {
            return {K::send_standinginfo_nack_to_user, gtournament_standinginfo_nack, gtournament_map_num, false, true, false};
        }
        return {K::forward_to_tournament_map_server, gtournament_standinginfo_syn, gtournament_map_num, true, true, false};

    case gtournament_battlejoin_syn:
    case gtournament_observerjoin_syn:
        if (!r.user_known_by_charid) {
            return {K::drop_no_user_by_charid, 0u, 0u, false, true, false};
        }
        if (r.gt_map_port == 0u) {
            return {K::send_battlejoin_nack_to_user, gtournament_battlejoin_nack, gtournament_map_num, false, true, false};
        }
        return {K::forward_to_tournament_map_server, r.protocol, gtournament_map_num, true, true, false};

    case gtournament_leave_syn:
        if (!r.user_known_by_charid) {
            return {K::drop_no_user_by_charid, 0u, 0u, false, true, false};
        }
        return {K::forward_to_user_map_server, gtournament_leave_syn, 0u, true, true, false};

    case gtournament_cheat: {
        if (!r.user_known_by_charid) {
            return {K::drop_no_user_by_charid, 0u, 0u, false, true, false};
        }
        if (r.target_map_port != 0u) {
            return {K::forward_to_user_map_server, gtournament_cheat, 0u, true, true, false};
        }
        if (r.gt_map_port != 0u) {
            return {K::forward_to_tournament_map_server, gtournament_cheat, gtournament_map_num, true, true, false};
        }
        return {K::drop_no_user_by_charid, 0u, 0u, false, true, false};
    }

    case gtournament_event_start:
    case gtournament_event_end:
        if (!r.user_known_by_conn) {
            return {K::drop_no_user_by_conn, 0u, 0u, false, false, true};
        }
        // Legacy checks `if (UserLevel > eUSERLEVEL_GM) return;`. GM=1.
        if (r.user_level > gtournament_user_level_max_for_event_start) {
            return {K::drop_event_start_not_gm, 0u, 0u, false, false, true};
        }
        if (r.gt_map_port != 0u) {
            return {K::forward_to_tournament_map_server, r.protocol, gtournament_map_num, true, false, true};
        }
        return {K::drop_no_user_by_conn, 0u, 0u, false, false, true};

    default:
        return {K::default_to_trans_to_user, r.protocol, 0u, true, true, false};
    }
}

GTournamentServerAction classify_agent_gtournament_server(const GTournamentServerRequest& r) {
    using K = GTournamentActionKind;
    switch (r.protocol) {
    case gtournament_cheat:
        if (!r.target_user_found) {
            return {K::drop_no_user_by_charid, 0u, false, false, false};
        }
        return {K::send_to_user, gtournament_cheat, true, false, false};

    case gtournament_standinginfo_registed:
        if (!r.target_user_found) {
            return {K::drop_no_user_by_charid, 0u, false, false, false};
        }
        return {K::send_to_user, gtournament_standinginfo_registed, true, false, false};

    case gtournament_returntomap:
        if (!r.target_user_found) {
            return {K::drop_no_user_by_charid, 0u, false, false, false};
        }
        if (r.return_port_known) {
            GTournamentServerAction a{K::set_user_map_state_and_forward, gtournament_returntomap, true, true, false};
            a.update_map_state = true;
            return a;
        }
        return {K::send_to_user, gtournament_returntomap, true, false, false};

    case gtournament_notify_winlose:
        if (!r.target_user_found) {
            return {K::drop_no_user_by_charid, 0u, false, false, false};
        }
        return {K::send_to_user, gtournament_notify_winlose, true, false, false};

    default:
        return {K::broadcast_to_client, r.protocol, true, false, false};
    }
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int agent_gtournament_server_translation_unit_anchor = 0;
}