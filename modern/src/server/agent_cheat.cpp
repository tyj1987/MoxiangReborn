// agent_cheat.cpp - Phase 6.3 AgentCheat 1:1 port.

#include "mxh/server/agent_cheat.hpp"

namespace mxh::server {

namespace {

bool is_authorized_cheat_user(std::uint8_t level) {
    return level == cheat_user_level_gm ||
           level == cheat_user_level_programmer ||
           level == cheat_user_level_developer;
}

}  // namespace

// Pure classifier mirroring legacy MP_CHEATUserMsgParser at lines 2836-3459.
CheatUserAction classify_agent_cheat_user(const CheatUserRequest& r) {
    using K = CheatUserActionKind;

    if (!r.user_known) {
        return {K::drop_no_user, 0u, false, false, false, false};
    }
    if (!is_authorized_cheat_user(r.user_level)) {
        return {K::drop_unauthorized, 0u, false, false, false, false};
    }

    // GM_INFO gate: programmer/developer bypass; GM must have logged in.
    if (r.user_level == cheat_user_level_gm && !r.gm_info_present) {
        return {K::drop_gm_not_logged_in, 0u, false, false, false, false};
    }

    switch (r.protocol) {
    // GM_LOGIN_SYN: legacy hands off to GM_Login with (id, ip); no GM_INFO yet.
    case cheat_gm_login_syn:
        return {K::gm_login, cheat_gm_login_syn, true, false, false, false};

    // CHANGEMAP_SYN: resolve target map port; if 0 -> NACK; else forward to map.
    case cheat_changemap_syn:
        if (!r.target_map_port_known) {
            return {K::send_changemap_nack_to_user, cheat_changemap_nack, false, false, false, false};
        }
        return {K::trans_to_map_server, cheat_changemap_syn, true, false, false, false};

    // BANCHARACTER / BANMAP / BLOCKCHARACTER: broadcast to all map servers.
    case cheat_bancharacter_syn:
        if (r.admin_target_user == 0u) {
            return {K::drop_gm_blocked_subprotocol, 0u, false, false, false, false};
        }
        return {K::send_bancharacter_to_map_server, cheat_bancharacter_syn, true, false, true, false};
    case cheat_blockcharacter_syn:
        return {K::send_blockcharacter_syn, cheat_blockcharacter_syn, true, false, true, false};
    case cheat_banmap_syn:
        return {K::trans_to_map_server, cheat_banmap_syn, true, false, true, false};

    // WHEREIS: per legacy, GM-only unless source-target known.
    case cheat_whereis_syn:
        if (!r.target_char_in_user_table) {
            return {K::drop_no_user, 0u, false, false, false, false};
        }
        return {K::trans_to_map_server, cheat_whereis_syn, true, false, false, false};

    // Monster regen/delete and event map flags: broadcast to all maps.
    case cheat_event_monster_regen:
        return {K::send_event_monster_regen, cheat_event_monster_regen, true, false, true, false};
    case cheat_event_monster_delete:
        return {K::send_event_monster_delete, cheat_event_monster_delete, true, false, true, false};
    case cheat_event_start_syn:
    case cheat_event_ready_syn:
        return {K::trans_to_map_server, r.protocol, true, false, true, false};

    case cheat_change_eventmap_syn:
        return {K::trans_to_map_server, cheat_change_eventmap_syn, true, false, false, false};

    // Notice: just broadcast to map servers.
    case cheat_notice_syn:
        return {K::trans_to_map_server, cheat_notice_syn, true, false, true, false};

    // PKALLOW_SYN toggles a global PK flag; forwarded to maps.
    case cheat_pkallow_syn:
        return {K::send_pkallow_syn, cheat_pkallow_syn, true, false, true, false};

    case cheat_agentcheck_syn:
        return {K::trans_to_map_server, cheat_agentcheck_syn, true, false, false, false};

    // Stat/item/money cheats -> pass to map with logged GM usage.
    case cheat_abilityexp_syn:
    case cheat_addmugong_syn:
    case cheat_mugongsung_syn:
    case cheat_item_syn:
    case cheat_item_option_syn:
    case cheat_money_syn:
        return {K::log_gm_tool_use, r.protocol, true, false, false, false};

    case cheat_event_syn:
        return {K::log_gm_tool_use, cheat_event_syn, true, false, false, false};

    case cheat_eventnotify_on:
    case cheat_plustime_on:
        return {K::log_gm_tool_use, r.protocol, true, false, true, false};
    case cheat_eventnotify_off:
    case cheat_plustime_alloff:
        return {K::log_gm_tool_use, r.protocol, true, false, true, false};

    case cheat_pet_stamina:
    case cheat_pet_friendship_syn:
    case cheat_pet_selected_friendship_syn:
    case cheat_guildpoint_syn:
    case cheat_guildhunted_monstercount_syn:
    case cheat_mussang_ready:
        return {K::log_gm_tool_use, r.protocol, true, false, false, false};

    case cheat_jackpot_getprize:
    case cheat_jackpot_moneypermonster:
    case cheat_jackpot_onoff:
    case cheat_jackpot_probability:
    case cheat_jackpot_control:
        return {K::log_gm_tool_use, r.protocol, true, false, false, false};

    // Bobusang / ItemLimit / Autonote cheats -> forward to relevant module.
    case cheat_bobusanginfo_request_syn:
    case cheat_bobusang_leave_syn:
    case cheat_bobusanginfo_change_syn:
        return {K::log_gm_tool_use, r.protocol, true, false, false, false};
    case cheat_itemlimit_syn:
        return {K::log_gm_tool_use, cheat_itemlimit_syn, true, false, false, false};
    case cheat_autonote_setting_syn:
        return {K::log_gm_tool_use, cheat_autonote_setting_syn, true, false, false, false};

    case cheat_damage_syn:
        return {K::trans_to_map_server, cheat_damage_syn, true, false, false, false};
    case cheat_damage_ack:
        return {K::forward_to_user, cheat_damage_ack, true, false, false, false};
    case cheat_damage_nack:
        return {K::forward_to_user, cheat_damage_nack, true, false, false, false};

    case cheat_map_condition:
    case cheat_agent_condition:
        return {K::trans_to_map_server, r.protocol, true, false, false, false};

    case cheat_titan_fuel_spell_max_syn:
        return {K::trans_to_map_server, cheat_titan_fuel_spell_max_syn, true, false, false, false};

    default:
        return {K::trans_to_map_server, r.protocol, true, false, false, false};
    }
}

// Server-side inbound (MP_CHEATServerMsgParser at lines ~3460-3540).
CheatServerAction classify_agent_cheat_server(const CheatServerRequest& r) {
    using K = CheatServerActionKind;
    switch (r.protocol) {
    case cheat_bancharacter_syn:
        if (!r.target_found) {
            return {K::drop_unknown_char, 0u, false, false};
        }
        return {K::send_bancharacter_to_user, cheat_bancharacter_syn, true, false};
    case cheat_bancharacter_ack:
        return {K::send_bancharacter_ack_to_user, cheat_bancharacter_ack, true, false};
    case cheat_bancharacter_nack:
        return {K::send_bancharacter_ack_broadcast, cheat_bancharacter_nack, true, true};
    case cheat_whereis_syn:
        if (!r.whereis_target_known) {
            return {K::send_whereis_nack_to_user, cheat_whereis_syn, true, false};
        }
        return {K::send_whereis_ack_to_user, cheat_whereis_syn, true, false};
    case cheat_whereis_ack:
        return {K::send_whereis_ack_broadcast, cheat_whereis_ack, true, true};
    default:
        return {K::trans_to_map_server, r.protocol, true, false};
    }
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int agent_cheat_translation_unit_anchor = 0;
}