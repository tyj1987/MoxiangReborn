// agent_userconn.hpp - Phase 6.3 AgentUserConn 1:1 port.
//
// Source-of-truth: legacy [Server]Agent/AgentNetworkMsgParser.cpp
// MP_USERCONNMsgParser at lines 292-2024 (the login/character/gamein pipeline).
// Pure classifier: routes MP_USERCONN sub-protocols to dispatch intent.
// Legacy quirks preserved:
//   - USERCONN_LOGIN_*: agent logs in after dist-server notifies (line 322+).
//   - USERCONN_CHARACTERLIST_SYN: invalid auth key -> NACK + disconnect (line 588+).
//   - USERCONN_GAMEIN_SYN: forward dwObjectID+auth+channel to map server (line 853+).
//   - USERCONN_GAMEIN_NACK: disconnect user (line 889+).
//   - USERCONN_GAMEIN_ACK: forward totalinfo + broadcast GameInOtherMap (line 899+).
//   - USERCONN_CHANGEMAP_SYN: resolve map port, send CHANGEMAP_ACK/NACK (line 1079+).
//   - USERCONN_DISCONNECT_OVERLAPLOGIN: force disconnect + inform map server (line 461+).
//   - USERCONN_NOTIFY_USERLOGIN_NACK: agent not ready -> nack to dist (line 310+).
//   - USERCONN_FORCE_DISCONNECT_OVERLAPLOGIN_ACK: ack to dist (no source handler in
//     legacy MP_USERCONNMsgParser but exists in MP_PROTOCOL_USERCONN enum).
//   - USERCONN_DISCONNECTED: client-side disconnect ack (no source handler).

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_USERCONN (MP_USERCONN=7 in MP_CATEGORY).
inline constexpr std::uint8_t userconn_category = 7u;

// Sub-protocols within MP_PROTOCOL_USERCONN (offset 0..114, see [CC]Header/Protocol.h).
inline constexpr std::uint8_t userconn_dist_connectsuccess = 0u;
inline constexpr std::uint8_t userconn_login_syn = 1u;
inline constexpr std::uint8_t userconn_login_ack = 2u;
inline constexpr std::uint8_t userconn_login_nack = 3u;
inline constexpr std::uint8_t userconn_notify_userlogin_syn = 4u;
inline constexpr std::uint8_t userconn_notify_userlogin_ack = 5u;
inline constexpr std::uint8_t userconn_notify_userlogin_nack = 6u;
inline constexpr std::uint8_t userconn_notify_overlappedlogin = 7u;
inline constexpr std::uint8_t userconn_agent_connectsuccess = 8u;
inline constexpr std::uint8_t userconn_characterlist_syn = 9u;
inline constexpr std::uint8_t userconn_directcharacterlist_syn = 10u;
inline constexpr std::uint8_t userconn_characterlist_nack = 11u;
inline constexpr std::uint8_t userconn_characterlist_ack = 12u;
inline constexpr std::uint8_t userconn_disconnect_syn = 13u;
inline constexpr std::uint8_t userconn_disconnect_ack = 14u;
inline constexpr std::uint8_t userconn_disconnect_nack = 15u;
inline constexpr std::uint8_t userconn_characterselect_syn = 16u;
inline constexpr std::uint8_t userconn_characterselect_ack = 17u;
inline constexpr std::uint8_t userconn_characterselect_nack = 18u;
inline constexpr std::uint8_t userconn_character_namecheck_syn = 19u;
inline constexpr std::uint8_t userconn_character_namecheck_ack = 20u;
inline constexpr std::uint8_t userconn_character_namecheck_nack = 21u;
inline constexpr std::uint8_t userconn_character_make_syn = 22u;
inline constexpr std::uint8_t userconn_character_make_ack = 23u;
inline constexpr std::uint8_t userconn_character_make_nack = 24u;
inline constexpr std::uint8_t userconn_character_info_syn = 25u;
inline constexpr std::uint8_t userconn_character_info_ack = 26u;
inline constexpr std::uint8_t userconn_character_info_nack = 27u;
inline constexpr std::uint8_t userconn_gamein_syn = 28u;
inline constexpr std::uint8_t userconn_gamein_ack = 29u;
inline constexpr std::uint8_t userconn_gamein_nack = 30u;
inline constexpr std::uint8_t userconn_gameout_syn = 31u;
inline constexpr std::uint8_t userconn_gameout_ack = 32u;
inline constexpr std::uint8_t userconn_gameout_nack = 33u;
inline constexpr std::uint8_t userconn_disconnected = 34u;
inline constexpr std::uint8_t userconn_character_add = 35u;
inline constexpr std::uint8_t userconn_pet_add = 36u;
inline constexpr std::uint8_t userconn_monster_add = 37u;
inline constexpr std::uint8_t userconn_bossmonster_add = 38u;
inline constexpr std::uint8_t userconn_npc_add = 39u;
inline constexpr std::uint8_t userconn_object_remove = 40u;
inline constexpr std::uint8_t userconn_character_die = 41u;
inline constexpr std::uint8_t userconn_monster_die = 42u;
inline constexpr std::uint8_t userconn_pet_die = 43u;
inline constexpr std::uint8_t userconn_character_revive = 44u;
inline constexpr std::uint8_t userconn_character_remove_syn = 45u;
inline constexpr std::uint8_t userconn_character_remove_ack = 46u;
inline constexpr std::uint8_t userconn_character_remove_nack = 47u;
inline constexpr std::uint8_t userconn_changemap_syn = 48u;
inline constexpr std::uint8_t userconn_changemap_ack = 49u;
inline constexpr std::uint8_t userconn_changemap_nack = 50u;
inline constexpr std::uint8_t userconn_map_out = 51u;
inline constexpr std::uint8_t userconn_map_out_withmapnum = 52u;
inline constexpr std::uint8_t userconn_character_totalinfo = 53u;
inline constexpr std::uint8_t userconn_savepoint_syn = 54u;
inline constexpr std::uint8_t userconn_savepoint_ack = 55u;
inline constexpr std::uint8_t userconn_savepoint_nack = 56u;
inline constexpr std::uint8_t userconn_backtocharsel_syn = 57u;
inline constexpr std::uint8_t userconn_backtocharsel_ack = 58u;
inline constexpr std::uint8_t userconn_backtocharsel_nack = 59u;
inline constexpr std::uint8_t userconn_gridinit = 60u;
inline constexpr std::uint8_t userconn_setvisible = 61u;
inline constexpr std::uint8_t userconn_otheruser_connecttry_notify = 62u;
inline constexpr std::uint8_t userconn_connection_check = 63u;
inline constexpr std::uint8_t userconn_connection_check_ok = 64u;
inline constexpr std::uint8_t userconn_checksumerror = 65u;
inline constexpr std::uint8_t userconn_force_disconnect_overlaplogin = 66u;
inline constexpr std::uint8_t userconn_disconnected_by_overlaplogin = 67u;
inline constexpr std::uint8_t userconn_channelinfo_syn = 68u;
inline constexpr std::uint8_t userconn_channelinfo_ack = 69u;
inline constexpr std::uint8_t userconn_channelinfo_nack = 70u;
inline constexpr std::uint8_t userconn_notifytoagent_alreadyout = 71u;
inline constexpr std::uint8_t userconn_request_distout = 72u;
inline constexpr std::uint8_t userconn_disconnected_on_login = 73u;
inline constexpr std::uint8_t userconn_server_notready = 74u;
inline constexpr std::uint8_t userconn_mapdesc = 75u;
inline constexpr std::uint8_t userconn_character_revive_nack = 76u;
inline constexpr std::uint8_t userconn_ready_to_revive = 77u;
inline constexpr std::uint8_t userconn_cheat_using = 78u;
inline constexpr std::uint8_t userconn_cheat_changemap_ack = 79u;
inline constexpr std::uint8_t userconn_use_dynamic_syn = 80u;
inline constexpr std::uint8_t userconn_use_dynamic_ack = 81u;
inline constexpr std::uint8_t userconn_use_dynamic_nack = 82u;
inline constexpr std::uint8_t userconn_login_dynamic_syn = 83u;
inline constexpr std::uint8_t userconn_login_dynamic_ack = 84u;
inline constexpr std::uint8_t userconn_login_dynamic_nack = 85u;
inline constexpr std::uint8_t userconn_logincheck_delete = 86u;
inline constexpr std::uint8_t userconn_force_disconnect_overlaplogin_ack = 87u;
inline constexpr std::uint8_t userconn_map_out_to_eventmap = 88u;
inline constexpr std::uint8_t userconn_map_out_to_eventbeforemap = 89u;
inline constexpr std::uint8_t userconn_enter_eventmap_syn = 90u;
inline constexpr std::uint8_t userconn_event_ready = 91u;
inline constexpr std::uint8_t userconn_event_start = 92u;
inline constexpr std::uint8_t userconn_event_end = 93u;
inline constexpr std::uint8_t userconn_eventitem_use = 94u;
inline constexpr std::uint8_t userconn_eventitem_use2 = 95u;
inline constexpr std::uint8_t userconn_gameinpos_syn = 96u;
inline constexpr std::uint8_t userconn_gameinpos_ack = 97u;
inline constexpr std::uint8_t userconn_gameinpos_nack = 98u;
inline constexpr std::uint8_t userconn_remaintime_notify = 99u;
inline constexpr std::uint8_t userconn_backtobeforemap_touser = 100u;
inline constexpr std::uint8_t userconn_backtobeforemap_syn = 101u;
inline constexpr std::uint8_t userconn_backtobeforemap_ack = 102u;
inline constexpr std::uint8_t userconn_backtobeforemap_nack = 103u;
inline constexpr std::uint8_t userconn_enter_gtournament_syn = 104u;
inline constexpr std::uint8_t userconn_characterslot = 105u;
inline constexpr std::uint8_t userconn_castlegate_add = 106u;
inline constexpr std::uint8_t userconn_gamein_othermap_syn = 107u;
inline constexpr std::uint8_t userconn_nowaitexitplayer = 108u;
inline constexpr std::uint8_t userconn_flagnpc_onoff = 109u;
inline constexpr std::uint8_t userconn_login_syn_buddy = 110u;
inline constexpr std::uint8_t userconn_changemap_channelinfo_syn = 111u;
inline constexpr std::uint8_t userconn_changemap_channelinfo_ack = 112u;
inline constexpr std::uint8_t userconn_changemap_channelinfo_nack = 113u;
inline constexpr std::uint8_t userconn_currentmap_channelinfo = 114u;

// Legacy login error codes (used in notify_userlogin_nack).
inline constexpr std::uint32_t userconn_login_err_no_agent_server = 0u;
inline constexpr std::uint32_t userconn_login_err_dist_alreadyout = 1u;
inline constexpr std::uint32_t userconn_login_err_overlap = 2u;
inline constexpr std::uint32_t userconn_login_err_no_user = 3u;

// Input contract for classify_userconn.
struct UserConnRequest {
    std::uint8_t protocol = 0u;
    std::uint32_t user_id = 0u;
    std::uint32_t auth_key = 0u;
    std::uint32_t dist_auth_key = 0u;
    std::uint32_t character_id = 0u;
    std::uint32_t map_server_conn = 0u;
    std::uint32_t target_map_num = 0u;
    std::uint16_t channel = 0u;
    std::uint8_t user_level = 0u;
    bool agent_ready = true;
    bool user_found_by_userid = true;
    bool user_found_by_conn = true;
    bool user_found_by_charid = true;
    bool auth_keys_match = true;
    bool map_port_known = true;
    bool event_blocked = false;
    bool connection_check_failed = false;
};

// Dispatch intent for MP_USERCONN sub-protocols.
enum class UserConnActionKind : std::uint8_t {
    drop_no_user,
    drop_unauthenticated,
    send_nack_to_dist_no_agent,
    send_notify_userlogin_ack,
    send_otheruser_connecttry_notify,
    send_disconnected_by_overlap_to_client,
    send_nowaitexit_to_map_server,
    disconnect_user,
    forward_to_map_server,
    send_to_user,
    broadcast_to_map_servers,
    broadcast_except_to_map_servers,
    reply_logincheck_delete,
    log_cheat_use,
};

// Resulting action: protocol to reply with (0 = no reply), error/scratch data.
struct UserConnAction {
    UserConnActionKind kind = UserConnActionKind::forward_to_map_server;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t error_code = 0u;
    bool forward_payload = false;
    bool disable_failure_flag = false;
};

UserConnAction classify_userconn(const UserConnRequest& r);

}  // namespace mxh::server