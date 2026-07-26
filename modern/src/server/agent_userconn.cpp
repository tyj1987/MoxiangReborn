// agent_userconn.cpp - Phase 6.3 AgentUserConn 1:1 port.

#include "mxh/server/agent_userconn.hpp"

namespace mxh::server {

// Pure classifier returning one routing intent per MP_USERCONN sub-protocol.
// Mirrors the legacy switch in [Server]Agent/AgentNetworkMsgParser.cpp lines
// 292-2024. We deliberately keep the order and explicit drop cases so the
// 1:1 mapping to legacy code remains easy to audit.
UserConnAction classify_userconn(const UserConnRequest& r) {
    using K = UserConnActionKind;

    switch (r.protocol) {
    // Login notify coming from the Distribute server. If the agent is not ready
    // we must NACK back to Dist with userconn_login_err_no_agent_server (line 310+).
    case userconn_notify_userlogin_syn: {
        if (!r.agent_ready) {
            return {K::send_nack_to_dist_no_agent, userconn_notify_userlogin_nack,
                    userconn_login_err_no_agent_server, false, false};
        }
        // legacy hands off to UserIDXSendAndCharacterBaseInfo which is a DB read
        // scaffolded at agent_db_msg_parser.cpp; we mark it as a forward that
        // does not need an immediate reply packet.
        return {K::forward_to_map_server, 0u, 0u, true, false};
    }
    case userconn_notify_userlogin_ack:
        return {K::send_notify_userlogin_ack, userconn_notify_userlogin_ack, 0u, true, false};
    case userconn_notify_userlogin_nack:
        return {K::send_to_user, userconn_notify_userlogin_nack, 0u /*error_code*/, true, false};

    // Overlapped login notify from Dist -> forward OTHERUSER_CONNECTTRY_NOTIFY to
    // the existing in-agent user (line 420+). If user is missing there is no
    // listener, drop.
    case userconn_notify_overlappedlogin:
        if (!r.user_found_by_userid) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::send_otheruser_connecttry_notify, userconn_otheruser_connecttry_notify, 0u, false, false};

    // Force disconnect from Dist on overlap. Inform map server so it does not
    // wait, then disconnect user. If the user never had a connection index
    // we fall back to LoginCheckDelete (line 461+).
    case userconn_force_disconnect_overlaplogin:
        if (!r.user_found_by_userid) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        if (r.character_id != 0u && r.map_server_conn != 0u) {
            return {K::send_nowaitexit_to_map_server, userconn_nowaitexitplayer, 0u, false, false};
        }
        return {K::send_disconnected_by_overlap_to_client, userconn_disconnected_by_overlaplogin, 0u, false, false};
    case userconn_force_disconnect_overlaplogin_ack:
        return {K::send_to_user, userconn_force_disconnect_overlaplogin_ack, 0u, true, false};
    case userconn_disconnected_by_overlaplogin:
        return {K::send_to_user, userconn_disconnected_by_overlaplogin, 0u, true, false};

    // Connection-check beats (line 565+).
    case userconn_connection_check:
        return {K::send_to_user, userconn_connection_check, 0u, true, false};
    case userconn_connection_check_ok:
        if (!r.user_found_by_conn) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, 0u, 0u, true, true}; // reset bConnectionCheckFailed
    case userconn_checksumerror:
        return {K::log_cheat_use, userconn_checksumerror, 0u, true, false};

    // CHARACTERLIST_SYN: invalid auth -> NACK + disconnect (line 573+).
    case userconn_characterlist_syn:
        if (!r.user_found_by_userid || !r.auth_keys_match) {
            return {K::disconnect_user, userconn_characterlist_nack, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_characterlist_ack, 0u, true, false};
    case userconn_directcharacterlist_syn:
        if (!r.user_found_by_conn) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_characterlist_ack, 0u, true, false};
    case userconn_characterlist_ack:
        return {K::send_to_user, userconn_characterlist_ack, 0u, true, false};
    case userconn_characterlist_nack:
        return {K::send_to_user, userconn_characterlist_nack, 0u, true, false};

    // Disconnect request from client (line 550+).
    case userconn_disconnect_syn:
        return {K::disconnect_user, userconn_disconnect_syn, 0u, false, false};
    case userconn_disconnect_ack:
        return {K::disconnect_user, userconn_disconnect_ack, 0u, false, false};
    case userconn_disconnect_nack:
        return {K::send_to_user, userconn_disconnect_nack, 0u, true, false};

    // Character create / select paths (lines 679-1049) - mostly forward to map server.
    case userconn_characterselect_syn:
        if (!r.user_found_by_userid) {
            return {K::drop_unauthenticated, userconn_characterselect_nack, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_characterselect_syn, 0u, true, false};
    case userconn_characterselect_ack:
        return {K::send_to_user, userconn_characterselect_ack, 0u, true, false};
    case userconn_characterselect_nack:
        return {K::send_to_user, userconn_characterselect_nack, 0u, true, false};
    case userconn_character_make_syn:
        return {K::forward_to_map_server, userconn_character_make_syn, 0u, true, false};
    case userconn_character_make_ack:
        return {K::send_to_user, userconn_character_make_ack, 0u, true, false};
    case userconn_character_make_nack:
        return {K::send_to_user, userconn_character_make_nack, 0u, true, false};
    case userconn_character_namecheck_syn:
    case userconn_character_namecheck_ack:
    case userconn_character_namecheck_nack:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    case userconn_character_info_syn:
    case userconn_character_info_ack:
    case userconn_character_info_nack:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};

    // GAMEIN_SYN (line 853+): forward channel+auth to map server that hosts
    // the user character.
    case userconn_gamein_syn:
        if (!r.user_found_by_conn) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_gamein_syn, 0u, true, false};
    case userconn_gamein_ack:
        return {K::send_to_user, userconn_gamein_ack, 0u, true, false};
    case userconn_gamein_nack:
        if (!r.user_found_by_charid) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::disconnect_user, 0u, 0u, false, false};
    case userconn_gamein_othermap_syn:
        return {K::broadcast_except_to_map_servers, userconn_gamein_othermap_syn, 0u, true, false};
    case userconn_gameinpos_syn:
        return {K::forward_to_map_server, userconn_gameinpos_syn, 0u, true, false};
    case userconn_gameinpos_ack:
        return {K::send_to_user, userconn_gameinpos_ack, 0u, true, false};
    case userconn_gameinpos_nack:
        return {K::send_to_user, userconn_gameinpos_nack, 0u, true, false};
    case userconn_gameout_syn:
    case userconn_gameout_ack:
    case userconn_gameout_nack:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    case userconn_disconnected:
        return {K::log_cheat_use, userconn_disconnected, 0u, true, false};

    // Add/Remove object broadcasts from map server (line 942+).
    case userconn_character_add:
    case userconn_pet_add:
    case userconn_monster_add:
    case userconn_bossmonster_add:
    case userconn_npc_add:
    case userconn_object_remove:
    case userconn_character_die:
    case userconn_monster_die:
    case userconn_pet_die:
    case userconn_character_revive:
    case userconn_character_revive_nack:
    case userconn_ready_to_revive:
        return {K::broadcast_to_map_servers, r.protocol, 0u, true, false};

    // Remove character (line 1073+) -> forward and clear tables.
    case userconn_character_remove_syn:
        return {K::forward_to_map_server, userconn_character_remove_syn, 0u, true, false};
    case userconn_character_remove_ack:
        return {K::send_to_user, userconn_character_remove_ack, 0u, true, false};
    case userconn_character_remove_nack:
        return {K::send_to_user, userconn_character_remove_nack, 0u, true, false};

    // CHANGEMAP_SYN (line 1079+): resolve target port; if 0 -> NACK, else forward.
    case userconn_changemap_syn:
        if (!r.map_port_known || r.event_blocked) {
            return {K::send_to_user, userconn_changemap_nack, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_changemap_syn, 0u, true, false};
    case userconn_changemap_ack:
        return {K::send_to_user, userconn_changemap_ack, 0u, true, false};
    case userconn_changemap_nack:
        return {K::send_to_user, userconn_changemap_nack, 0u, true, false};
    case userconn_map_out:
    case userconn_map_out_withmapnum:
        if (!r.map_port_known) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    case userconn_character_totalinfo:
        return {K::forward_to_map_server, userconn_character_totalinfo, 0u, true, false};
    case userconn_savepoint_syn:
    case userconn_savepoint_ack:
    case userconn_savepoint_nack:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    case userconn_backtocharsel_syn:
        if (!r.user_found_by_conn) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_backtocharsel_syn, 0u, true, false};
    case userconn_backtocharsel_ack:
        if (!r.user_found_by_charid) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::send_to_user, userconn_characterlist_ack, 0u, true, false};
    case userconn_backtocharsel_nack:
        return {K::send_to_user, userconn_backtocharsel_nack, 0u, true, false};
    case userconn_backtobeforemap_syn:
        if (!r.map_port_known) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_backtobeforemap_syn, 0u, true, false};
    case userconn_backtobeforemap_ack:
        if (!r.map_port_known) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::send_to_user, userconn_changemap_ack, 0u, true, false};
    case userconn_backtobeforemap_nack:
        return {K::send_to_user, userconn_backtobeforemap_nack, 0u, true, false};
    case userconn_backtobeforemap_touser:
        return {K::send_to_user, userconn_backtobeforemap_touser, 0u, true, false};

    // Login cleanup from client (line 1309+).
    case userconn_logincheck_delete:
        return {K::reply_logincheck_delete, 0u, 0u, false, false};
    case userconn_cheat_using:
        return {K::log_cheat_use, userconn_cheat_using, 0u, true, false};
    case userconn_cheat_changemap_ack:
        return {K::send_to_user, userconn_cheat_changemap_ack, 0u, true, false};

    // Dist -> agent "alreadyout" and request_distout (line 622+).
    case userconn_notifytoagent_alreadyout:
        if (!r.auth_keys_match) {
            return {K::drop_unauthenticated, 0u, 0u, false, false};
        }
        if (!r.user_found_by_userid) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::send_to_user, userconn_login_nack, userconn_login_err_dist_alreadyout, true, false};
    case userconn_request_distout:
        if (!r.user_found_by_userid) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::disconnect_user, 0u, 0u, false, false};
    case userconn_disconnected_on_login:
        if (!r.user_found_by_userid || !r.auth_keys_match) {
            return {K::drop_unauthenticated, 0u, 0u, false, false};
        }
        return {K::disconnect_user, 0u, 0u, false, false};

    // Server-notready / mapdesc / login variants.
    case userconn_server_notready:
        return {K::send_to_user, userconn_server_notready, 0u, true, false};
    case userconn_mapdesc:
        return {K::send_to_user, userconn_mapdesc, 0u, true, false};
    case userconn_use_dynamic_syn:
    case userconn_use_dynamic_ack:
    case userconn_use_dynamic_nack:
    case userconn_login_dynamic_syn:
    case userconn_login_dynamic_ack:
    case userconn_login_dynamic_nack:
    case userconn_login_syn:
    case userconn_login_ack:
    case userconn_login_nack:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    case userconn_login_syn_buddy:
        return {K::forward_to_map_server, userconn_login_syn_buddy, 0u, true, false};

    // Channel info (line 784+).
    case userconn_channelinfo_syn:
        return {K::forward_to_map_server, userconn_channelinfo_syn, 0u, true, false};
    case userconn_channelinfo_ack:
        return {K::send_to_user, userconn_channelinfo_ack, 0u, true, false};
    case userconn_channelinfo_nack:
        return {K::send_to_user, userconn_channelinfo_nack, 0u, true, false};
    case userconn_changemap_channelinfo_syn:
    case userconn_changemap_channelinfo_ack:
    case userconn_changemap_channelinfo_nack:
    case userconn_currentmap_channelinfo:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};

    // Event map lifecycle (line 1422+).
    case userconn_map_out_to_eventmap:
        if (!r.map_port_known) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::send_to_user, userconn_map_out_to_eventmap, 0u, true, false};
    case userconn_map_out_to_eventbeforemap:
        return {K::forward_to_map_server, userconn_map_out_to_eventbeforemap, 0u, true, false};
    case userconn_enter_eventmap_syn:
        if (!r.user_found_by_conn) {
            return {K::drop_no_user, 0u, 0u, false, false};
        }
        return {K::forward_to_map_server, userconn_enter_eventmap_syn, 0u, true, false};
    case userconn_event_ready:
    case userconn_event_start:
    case userconn_event_end:
    case userconn_eventitem_use:
    case userconn_eventitem_use2:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    case userconn_enter_gtournament_syn:
        return {K::forward_to_map_server, userconn_enter_gtournament_syn, 0u, true, false};

    // Misc broadcast/notification (line 472+).
    case userconn_gridinit:
    case userconn_setvisible:
    case userconn_characterslot:
    case userconn_castlegate_add:
    case userconn_flagnpc_onoff:
    case userconn_remaintime_notify:
        return {K::broadcast_to_map_servers, r.protocol, 0u, true, false};

    // Dist / agent connectsuccess and "I am a user" packets.
    case userconn_dist_connectsuccess:
    case userconn_agent_connectsuccess:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};

    default:
        return {K::forward_to_map_server, r.protocol, 0u, true, false};
    }
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int agent_userconn_translation_unit_anchor = 0;
}