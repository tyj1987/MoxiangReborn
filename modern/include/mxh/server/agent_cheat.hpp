// agent_cheat.hpp - Phase 6.3 AgentCheat (user-side GM commands) 1:1 port.
//
// Source-of-truth: legacy [Server]Agent/AgentNetworkMsgParser.cpp lines
// 2836-3459 (MP_CHEATUserMsgParser + MP_CHEATServerMsgParser).
//
// The legacy handler enforces GM authorization by checking
// UserLevel in {GM, PROGRAMMER, DEVELOPER} and then routes by sub-protocol.
// For PROGRAMMER/DEVELOPER, every sub-protocol is accepted. For GM, an
// extra GM_INFO pointer must exist (logged in GM_Login) and certain
// advanced sub-protocols like GTOURNAMENT_RESET are blocked.
//
// We preserve the routing decisions; the modern dispatcher is expected to
// call classify_agent_cheat_user / classify_agent_cheat_server and act on
// the returned intent.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_CHEAT (MP_CHEAT=11 in MP_CATEGORY).
inline constexpr std::uint8_t cheat_category = 11u;

// Legacy user levels per [Server]Common/Const.h.
inline constexpr std::uint8_t cheat_user_level_gm = 1u;
inline constexpr std::uint8_t cheat_user_level_programmer = 2u;
inline constexpr std::uint8_t cheat_user_level_developer = 3u;
// Past user-level constants that exist in the legacy enum (kept for 1:1).
inline constexpr std::uint8_t cheat_user_level_monitor = 4u;
inline constexpr std::uint8_t cheat_user_level_superuser = 5u;

// Sub-protocols within MP_PROTOCOL_CHEAT (offset 0..164, see
// [CC]Header/Protocol.h lines 1254-1487). We expose only the major names
// used by AgentCheat routing here; downstream users can extend with the
// full 0..164 range.
inline constexpr std::uint8_t cheat_gm_login_syn = 0u;
inline constexpr std::uint8_t cheat_changemap_syn = 1u;
inline constexpr std::uint8_t cheat_changemap_nack = 2u;
inline constexpr std::uint8_t cheat_changemap_ack = 3u;
inline constexpr std::uint8_t cheat_bancharacter_syn = 4u;
inline constexpr std::uint8_t cheat_bancharacter_nack = 5u;
inline constexpr std::uint8_t cheat_blockcharacter_syn = 6u;
inline constexpr std::uint8_t cheat_whereis_syn = 7u;
inline constexpr std::uint8_t cheat_event_monster_regen = 8u;
inline constexpr std::uint8_t cheat_event_monster_delete = 9u;
inline constexpr std::uint8_t cheat_banmap_syn = 10u;
inline constexpr std::uint8_t cheat_agentcheck_syn = 11u;
inline constexpr std::uint8_t cheat_pkallow_syn = 12u;
inline constexpr std::uint8_t cheat_notice_syn = 13u;
inline constexpr std::uint8_t cheat_abilityexp_syn = 14u;
inline constexpr std::uint8_t cheat_addmugong_syn = 15u;
inline constexpr std::uint8_t cheat_mugongsung_syn = 16u;
inline constexpr std::uint8_t cheat_item_syn = 17u;
inline constexpr std::uint8_t cheat_item_option_syn = 18u;
inline constexpr std::uint8_t cheat_money_syn = 19u;
inline constexpr std::uint8_t cheat_event_syn = 20u;
inline constexpr std::uint8_t cheat_eventnotify_on = 21u;
inline constexpr std::uint8_t cheat_plustime_on = 22u;
inline constexpr std::uint8_t cheat_eventnotify_off = 23u;
inline constexpr std::uint8_t cheat_plustime_alloff = 24u;
inline constexpr std::uint8_t cheat_change_eventmap_syn = 25u;
inline constexpr std::uint8_t cheat_event_start_syn = 26u;
inline constexpr std::uint8_t cheat_event_ready_syn = 27u;
inline constexpr std::uint8_t cheat_pet_stamina = 28u;
inline constexpr std::uint8_t cheat_pet_friendship_syn = 29u;
inline constexpr std::uint8_t cheat_pet_selected_friendship_syn = 30u;
inline constexpr std::uint8_t cheat_guildpoint_syn = 31u;
inline constexpr std::uint8_t cheat_guildhunted_monstercount_syn = 32u;
inline constexpr std::uint8_t cheat_mussang_ready = 33u;
inline constexpr std::uint8_t cheat_jackpot_getprize = 34u;
inline constexpr std::uint8_t cheat_jackpot_moneypermonster = 35u;
inline constexpr std::uint8_t cheat_jackpot_onoff = 36u;
inline constexpr std::uint8_t cheat_jackpot_probability = 37u;
inline constexpr std::uint8_t cheat_jackpot_control = 38u;
inline constexpr std::uint8_t cheat_bobusanginfo_request_syn = 39u;
inline constexpr std::uint8_t cheat_bobusang_leave_syn = 40u;
inline constexpr std::uint8_t cheat_bobusanginfo_change_syn = 41u;
inline constexpr std::uint8_t cheat_itemlimit_syn = 42u;
inline constexpr std::uint8_t cheat_autonote_setting_syn = 43u;
inline constexpr std::uint8_t cheat_bancharacter_ack = 50u;

inline constexpr std::uint8_t cheat_whereis_ack = 52u;
inline constexpr std::uint8_t cheat_damage_syn = 44u;
inline constexpr std::uint8_t cheat_damage_ack = 45u;
inline constexpr std::uint8_t cheat_damage_nack = 46u;
inline constexpr std::uint8_t cheat_map_condition = 47u;
inline constexpr std::uint8_t cheat_agent_condition = 48u;
inline constexpr std::uint8_t cheat_titan_fuel_spell_max_syn = 49u;
// 50..164 reserved (see source for full enumeration).

// Input contract for the cheat_user classifier.
struct CheatUserRequest {
    std::uint8_t protocol = 0u;
    std::uint8_t user_level = 0u;
    bool user_known = true;
    bool gm_info_present = true;
    bool target_map_port_known = true;
    bool target_char_in_user_table = true;
    std::uint32_t admin_target_user = 0u;
};

// Server-side request contract (map server -> agent).
struct CheatServerRequest {
    std::uint8_t protocol = 0u;
    bool target_found = true;
    bool whereis_target_known = true;
};

// Cheat user-side dispatch intent.
enum class CheatUserActionKind : std::uint8_t {
    drop_unauthorized,                  // not GM/PROGRAMMER/DEVELOPER
    drop_no_user,                       // connection has no user
    drop_gm_not_logged_in,              // GM but no GM_INFO
    drop_gm_blocked_subprotocol,        // GM trying advanced cheat
    gm_login,                           // MP_CHEAT_GM_LOGIN_SYN
    trans_to_map_server,                // forward -> map server
    log_gm_tool_use,                    // legacy LogGMToolUse invocation
    send_bancharacter_to_map_server,    // MP_CHEAT_BANCHARACTER_SYN -> all maps
    send_bancharacter_nack_to_user,     // MP_CHEAT_BANCHARACTER_NACK
    send_blockcharacter_syn,            // MP_CHEAT_BLOCKCHARACTER_SYN -> all maps
    send_event_monster_regen,           // MP_CHEAT_EVENT_MONSTER_REGEN -> all maps
    send_event_monster_delete,          // MP_CHEAT_EVENT_MONSTER_DELETE -> all maps
    send_pkallow_syn,                   // MP_CHEAT_PKALLOW_SYN -> all maps
    send_changemap_nack_to_user,        // unknown map port => NACK
    forward_to_user,                    // default user ack
};

// Cheat server-side dispatch intent.
enum class CheatServerActionKind : std::uint8_t {
    drop_unknown_char,
    send_bancharacter_to_user,
    send_bancharacter_ack_to_user,
    send_bancharacter_ack_broadcast,
    send_whereis_ack_to_user,
    send_whereis_nack_to_user,
    send_whereis_ack_broadcast,
    trans_to_map_server,
};

struct CheatUserAction {
    CheatUserActionKind kind = CheatUserActionKind::trans_to_map_server;
    std::uint8_t reply_protocol = 0u;
    bool forward_payload = true;
    bool require_login = false;          // legacy GM_Login requirement
    bool broadcast_all_maps = false;     // Broadcast2MapServer
    bool broadcast_except_one = false;   // Broadcast2MapServerExceptOne
};

struct CheatServerAction {
    CheatServerActionKind kind = CheatServerActionKind::trans_to_map_server;
    std::uint8_t reply_protocol = 0u;
    bool broadcast_all_maps = false;
    bool forward_payload = true;
};

CheatUserAction classify_agent_cheat_user(const CheatUserRequest& r);
CheatServerAction classify_agent_cheat_server(const CheatServerRequest& r);

}  // namespace mxh::server