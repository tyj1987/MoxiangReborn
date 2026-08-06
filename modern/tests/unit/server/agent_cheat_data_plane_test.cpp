#include "mxh/server/agent_cheat.hpp"
#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <set>
#include <type_traits>

namespace {
using namespace mxh::server;

TEST(CheatDataPlane, CategoryAndProtocolConstantsAreStable) {
    EXPECT_EQ(cheat_category, 11u);
    const std::array<std::uint8_t, 43> sequential = {
        cheat_gm_login_syn, cheat_changemap_syn, cheat_changemap_nack, cheat_changemap_ack,
        cheat_bancharacter_syn, cheat_bancharacter_nack, cheat_blockcharacter_syn, cheat_whereis_syn,
        cheat_event_monster_regen, cheat_event_monster_delete, cheat_banmap_syn, cheat_agentcheck_syn,
        cheat_pkallow_syn, cheat_notice_syn, cheat_abilityexp_syn, cheat_addmugong_syn,
        cheat_mugongsung_syn, cheat_item_syn, cheat_item_option_syn, cheat_money_syn, cheat_event_syn,
        cheat_eventnotify_on, cheat_plustime_on, cheat_eventnotify_off, cheat_plustime_alloff,
        cheat_change_eventmap_syn, cheat_event_start_syn, cheat_event_ready_syn, cheat_pet_stamina,
        cheat_pet_friendship_syn, cheat_pet_selected_friendship_syn, cheat_guildpoint_syn,
        cheat_guildhunted_monstercount_syn, cheat_mussang_ready, cheat_jackpot_getprize,
        cheat_jackpot_moneypermonster, cheat_jackpot_onoff, cheat_jackpot_probability, cheat_jackpot_control,
        cheat_bobusanginfo_request_syn, cheat_bobusang_leave_syn, cheat_bobusanginfo_change_syn,
        cheat_itemlimit_syn};
    std::set<std::uint8_t> unique(sequential.begin(), sequential.end());
    EXPECT_EQ(unique.size(), sequential.size());
    EXPECT_EQ(cheat_autonote_setting_syn, 43u);
    EXPECT_EQ(cheat_damage_syn, 44u);
    EXPECT_EQ(cheat_damage_ack, 45u);
    EXPECT_EQ(cheat_damage_nack, 46u);
    EXPECT_EQ(cheat_titan_fuel_spell_max_syn, 49u);
    EXPECT_EQ(cheat_bancharacter_ack, 50u);
    EXPECT_EQ(cheat_whereis_ack, 52u);
}

TEST(CheatDataPlane, RequestAndActionDefaultsAreStable) {
    CheatUserRequest user;
    EXPECT_EQ(user.protocol, 0u); EXPECT_EQ(user.user_level, 0u);
    EXPECT_TRUE(user.user_known); EXPECT_TRUE(user.gm_info_present);
    EXPECT_TRUE(user.target_map_port_known); EXPECT_TRUE(user.target_char_in_user_table);
    EXPECT_EQ(user.admin_target_user, 0u);
    CheatServerRequest server;
    EXPECT_EQ(server.protocol, 0u); EXPECT_TRUE(server.target_found); EXPECT_TRUE(server.whereis_target_known);
    CheatUserAction ua;
    EXPECT_EQ(ua.kind, CheatUserActionKind::trans_to_map_server);
    EXPECT_EQ(ua.reply_protocol, 0u); EXPECT_TRUE(ua.forward_payload);
    EXPECT_FALSE(ua.require_login); EXPECT_FALSE(ua.broadcast_all_maps); EXPECT_FALSE(ua.broadcast_except_one);
    CheatServerAction sa;
    EXPECT_EQ(sa.kind, CheatServerActionKind::trans_to_map_server);
    EXPECT_EQ(sa.reply_protocol, 0u); EXPECT_FALSE(sa.broadcast_all_maps); EXPECT_TRUE(sa.forward_payload);
}

TEST(CheatDataPlane, UserGatesHaveLegacyPriority) {
    CheatUserRequest r; r.protocol = cheat_gm_login_syn;
    r.user_known = false; r.user_level = cheat_user_level_gm; r.gm_info_present = false;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::drop_no_user);
    r.user_known = true; r.user_level = 0u;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::drop_unauthorized);
    r.user_level = cheat_user_level_gm; r.gm_info_present = false;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::drop_gm_not_logged_in);
    r.user_level = cheat_user_level_programmer;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::gm_login);
}

TEST(CheatDataPlane, UserLoginAndChangeMapBranches) {
    CheatUserRequest r; r.user_level = cheat_user_level_gm; r.protocol = cheat_gm_login_syn;
    auto login = classify_agent_cheat_user(r);
    EXPECT_EQ(login.kind, CheatUserActionKind::gm_login); EXPECT_EQ(login.reply_protocol, cheat_gm_login_syn);
    r.protocol = cheat_changemap_syn; r.target_map_port_known = false;
    auto nack = classify_agent_cheat_user(r);
    EXPECT_EQ(nack.kind, CheatUserActionKind::send_changemap_nack_to_user); EXPECT_EQ(nack.reply_protocol, cheat_changemap_nack);
    r.target_map_port_known = true;
    auto forward = classify_agent_cheat_user(r);
    EXPECT_EQ(forward.kind, CheatUserActionKind::trans_to_map_server); EXPECT_TRUE(forward.forward_payload);
}

TEST(CheatDataPlane, UserTargetValidationAndBroadcasts) {
    CheatUserRequest r; r.user_level = cheat_user_level_developer;
    r.protocol = cheat_bancharacter_syn; r.admin_target_user = 0u;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::drop_gm_blocked_subprotocol);
    r.admin_target_user = 9u;
    auto ban = classify_agent_cheat_user(r);
    EXPECT_EQ(ban.kind, CheatUserActionKind::send_bancharacter_to_map_server); EXPECT_TRUE(ban.broadcast_all_maps);
    r.protocol = cheat_whereis_syn; r.target_char_in_user_table = false;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::drop_no_user);
    r.target_char_in_user_table = true;
    EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::trans_to_map_server);
    r.protocol = cheat_event_monster_regen;
    auto event = classify_agent_cheat_user(r);
    EXPECT_EQ(event.kind, CheatUserActionKind::send_event_monster_regen); EXPECT_TRUE(event.broadcast_all_maps);
}

TEST(CheatDataPlane, UserGroupedProtocolsPreserveProtocolAndFlags) {
    CheatUserRequest r; r.user_level = cheat_user_level_gm;
    const std::array<std::uint8_t, 16> logged = {cheat_abilityexp_syn, cheat_addmugong_syn, cheat_mugongsung_syn, cheat_item_syn, cheat_item_option_syn, cheat_money_syn, cheat_event_syn, cheat_pet_stamina, cheat_guildpoint_syn, cheat_jackpot_control, cheat_bobusang_leave_syn, cheat_itemlimit_syn, cheat_autonote_setting_syn, cheat_eventnotify_on, cheat_plustime_on, cheat_eventnotify_off};
    for (auto protocol : logged) { r.protocol = protocol; auto a = classify_agent_cheat_user(r); EXPECT_EQ(a.kind, CheatUserActionKind::log_gm_tool_use); EXPECT_EQ(a.reply_protocol, protocol); EXPECT_TRUE(a.forward_payload); }
    const std::array<std::uint8_t, 5> broadcast = {cheat_blockcharacter_syn, cheat_banmap_syn, cheat_notice_syn, cheat_pkallow_syn, cheat_event_ready_syn};
    for (auto protocol : broadcast) { r.protocol = protocol; auto a = classify_agent_cheat_user(r); EXPECT_TRUE(a.broadcast_all_maps); EXPECT_EQ(a.reply_protocol, protocol); }
}

TEST(CheatDataPlane, DamageConditionAndUnknownProtocolsForward) {
    CheatUserRequest r; r.user_level = cheat_user_level_gm;
    for (auto protocol : {cheat_damage_syn, cheat_map_condition, cheat_agent_condition, cheat_titan_fuel_spell_max_syn}) { r.protocol = protocol; auto a = classify_agent_cheat_user(r); EXPECT_EQ(a.kind, CheatUserActionKind::trans_to_map_server); EXPECT_EQ(a.reply_protocol, protocol); EXPECT_TRUE(a.forward_payload); }
    r.protocol = cheat_damage_ack; EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::forward_to_user);
    r.protocol = cheat_damage_nack; EXPECT_EQ(classify_agent_cheat_user(r).kind, CheatUserActionKind::forward_to_user);
    r.protocol = 200u; auto unknown = classify_agent_cheat_user(r); EXPECT_EQ(unknown.kind, CheatUserActionKind::trans_to_map_server); EXPECT_EQ(unknown.reply_protocol, 200u);
}

TEST(CheatDataPlane, ServerBranchesPreserveWireIntent) {
    CheatServerRequest r; r.protocol = cheat_bancharacter_syn; r.target_found = false;
    EXPECT_EQ(classify_agent_cheat_server(r).kind, CheatServerActionKind::drop_unknown_char);
    r.target_found = true; auto ban = classify_agent_cheat_server(r); EXPECT_EQ(ban.kind, CheatServerActionKind::send_bancharacter_to_user); EXPECT_TRUE(ban.broadcast_all_maps);
    r.protocol = cheat_bancharacter_ack; EXPECT_EQ(classify_agent_cheat_server(r).kind, CheatServerActionKind::send_bancharacter_ack_to_user);
    r.protocol = cheat_bancharacter_nack; auto nack = classify_agent_cheat_server(r); EXPECT_EQ(nack.kind, CheatServerActionKind::send_bancharacter_ack_broadcast); EXPECT_TRUE(nack.broadcast_all_maps);
    r.protocol = cheat_whereis_syn; r.whereis_target_known = false; EXPECT_EQ(classify_agent_cheat_server(r).kind, CheatServerActionKind::send_whereis_nack_to_user);
    r.whereis_target_known = true; EXPECT_EQ(classify_agent_cheat_server(r).kind, CheatServerActionKind::send_whereis_ack_to_user);
    r.protocol = cheat_whereis_ack; auto ack = classify_agent_cheat_server(r); EXPECT_EQ(ack.kind, CheatServerActionKind::send_whereis_ack_broadcast); EXPECT_TRUE(ack.broadcast_all_maps);
    r.protocol = 251u; auto fallback = classify_agent_cheat_server(r); EXPECT_EQ(fallback.kind, CheatServerActionKind::trans_to_map_server); EXPECT_EQ(fallback.reply_protocol, 251u);
}

TEST(CheatDataPlane, AuthorizedLevelBoundarySweep) {
    CheatUserRequest r; r.protocol = cheat_gm_login_syn; r.gm_info_present = true;
    for (std::uint8_t level = 0u; level <= 5u; ++level) { r.user_level = level; auto a = classify_agent_cheat_user(r); const bool allowed = level == cheat_user_level_gm || level == cheat_user_level_programmer || level == cheat_user_level_developer; EXPECT_EQ(a.kind == CheatUserActionKind::gm_login, allowed); }
}

}
