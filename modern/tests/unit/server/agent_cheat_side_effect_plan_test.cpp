#include <array>
#include <cstddef>

#include <mxh/server/agent_cheat.hpp>
#include <mxh/server/agent_cheat_side_effect_plan.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

CheatUserRequest make_cheat_user(std::uint8_t protocol, std::uint8_t level) {
    CheatUserRequest request{};
    request.protocol = protocol;
    request.user_level = level;
    request.user_known = true;
    request.gm_info_present = true;
    request.target_map_port_known = true;
    request.target_char_in_user_table = true;
    request.admin_target_user = 123u;
    return request;
}

CheatUserRequest unauthorized() {
    CheatUserRequest request{};
    request.protocol = cheat_gm_login_syn;
    request.user_level = 0u;
    request.user_known = true;
    return request;
}

CheatUserRequest gm_no_login() {
    CheatUserRequest request{};
    request.protocol = cheat_abilityexp_syn;
    request.user_level = cheat_user_level_gm;
    request.user_known = true;
    request.gm_info_present = false;
    return request;
}

CheatServerRequest cheat_server(std::uint8_t protocol) {
    CheatServerRequest request{};
    request.protocol = protocol;
    request.target_found = true;
    request.whereis_target_known = true;
    return request;
}

TEST(CheatUserPlan, UnauthorizedUserDropsWithoutSideEffects) {
    const auto action = classify_agent_cheat_user(unauthorized());
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Drop);
}

TEST(CheatUserPlan, GmWithoutInfoDropsForAdvancedCheat) {
    const auto action = classify_agent_cheat_user(gm_no_login());
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Drop);
}

TEST(CheatUserPlan, GmLoginRecordsAndAcknowledges) {
    const auto request = make_cheat_user(cheat_gm_login_syn, cheat_user_level_gm);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.logged);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::LogGMRecord);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_gm_login_syn);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(CheatUserPlan, ChangeMapNackSends2UserOnly) {
    CheatUserRequest request = make_cheat_user(cheat_changemap_syn,
                                               cheat_user_level_programmer);
    request.target_map_port_known = false;
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.send_to_user);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Send2User);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_changemap_nack);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(CheatUserPlan, ChangeMapForwardsPacketToMapServer) {
    const auto request = make_cheat_user(cheat_changemap_syn,
                                         cheat_user_level_programmer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_FALSE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Trans2MapServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_changemap_syn);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(CheatUserPlan, BanCharacterWithZeroTargetDrops) {
    CheatUserRequest request = make_cheat_user(cheat_bancharacter_syn,
                                               cheat_user_level_developer);
    request.admin_target_user = 0u;
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
}

TEST(CheatUserPlan, BanCharacterBroadcastsToAllMaps) {
    const auto request = make_cheat_user(cheat_bancharacter_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Send2User);
    EXPECT_EQ(plan.effects[1].kind, CheatUserSideEffectKind::Broadcast2MapServer);
    EXPECT_TRUE(plan.effects[1].broadcast_all_maps);
    EXPECT_EQ(plan.effects[1].reply_protocol, cheat_bancharacter_syn);
}

TEST(CheatUserPlan, BlockCharacterBroadcastsToAllMaps) {
    const auto request = make_cheat_user(cheat_blockcharacter_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_FALSE(plan.forward_to_map);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Broadcast2MapServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_blockcharacter_syn);
}

TEST(CheatUserPlan, EventMonsterRegenBroadcastsToAllMaps) {
    const auto request = make_cheat_user(cheat_event_monster_regen,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_event_monster_regen);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(CheatUserPlan, EventMonsterDeleteBroadcastsToAllMaps) {
    const auto request = make_cheat_user(cheat_event_monster_delete,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_event_monster_delete);
}

TEST(CheatUserPlan, PkAllowBroadcastsToAllMaps) {
    const auto request = make_cheat_user(cheat_pkallow_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_pkallow_syn);
}

TEST(CheatUserPlan, LogGmToolUseCheatsForwardToMapServer) {
    const std::array protocols = {cheat_abilityexp_syn, cheat_addmugong_syn,
        cheat_mugongsung_syn, cheat_item_syn, cheat_item_option_syn,
        cheat_money_syn, cheat_event_syn, cheat_mussang_ready,
        cheat_pet_stamina, cheat_pet_friendship_syn,
        cheat_pet_selected_friendship_syn, cheat_guildpoint_syn,
        cheat_guildhunted_monstercount_syn, cheat_jackpot_getprize,
        cheat_jackpot_moneypermonster, cheat_jackpot_onoff,
        cheat_jackpot_probability, cheat_jackpot_control,
        cheat_bobusanginfo_request_syn, cheat_bobusang_leave_syn,
        cheat_bobusanginfo_change_syn, cheat_itemlimit_syn,
        cheat_autonote_setting_syn};
    for (const auto protocol : protocols) {
        const auto request = make_cheat_user(protocol, cheat_user_level_developer);
        const auto action = classify_agent_cheat_user(request);
        const auto plan = cheat_user_side_effect_plan(action);
        EXPECT_TRUE(plan.dispatched);
        EXPECT_TRUE(plan.logged);
        EXPECT_TRUE(plan.forward_to_map);
        EXPECT_FALSE(plan.broadcast);
        ASSERT_EQ(plan.effects.size(), 2u);
        EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::LogGMToolUse);
        EXPECT_EQ(plan.effects[1].kind, CheatUserSideEffectKind::Trans2MapServer);
        EXPECT_EQ(plan.effects[1].reply_protocol, protocol);
    }
}

TEST(CheatUserPlan, PlusTimeFlagsBroadcastAndLog) {
    const std::array protocols = {cheat_eventnotify_on, cheat_plustime_on,
        cheat_eventnotify_off, cheat_plustime_alloff};
    for (const auto protocol : protocols) {
        const auto request = make_cheat_user(protocol, cheat_user_level_developer);
        const auto action = classify_agent_cheat_user(request);
        const auto plan = cheat_user_side_effect_plan(action);
        EXPECT_TRUE(plan.broadcast);
        EXPECT_TRUE(plan.logged);
        EXPECT_FALSE(plan.forward_to_map);
        ASSERT_EQ(plan.effects.size(), 2u);
        EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::LogGMToolUse);
        EXPECT_EQ(plan.effects[1].kind, CheatUserSideEffectKind::Broadcast2MapServer);
        EXPECT_EQ(plan.effects[1].reply_protocol, protocol);
        EXPECT_TRUE(plan.effects[1].broadcast_all_maps);
    }
}

TEST(CheatUserPlan, ForwardToUserCheatsSendReply) {
    const std::array protocols = {cheat_damage_ack, cheat_damage_nack};
    for (const auto protocol : protocols) {
        const auto request = make_cheat_user(protocol, cheat_user_level_developer);
        const auto action = classify_agent_cheat_user(request);
        const auto plan = cheat_user_side_effect_plan(action);
        EXPECT_TRUE(plan.send_to_user);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Send2User);
        EXPECT_EQ(plan.effects[0].reply_protocol, protocol);
    }
}

TEST(CheatUserPlan, WhereisWithoutTargetCharDrops) {
    CheatUserRequest request = make_cheat_user(cheat_whereis_syn,
                                               cheat_user_level_developer);
    request.target_char_in_user_table = false;
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(CheatUserPlan, WhereisWithTargetForwardsToMapServer) {
    const auto request = make_cheat_user(cheat_whereis_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_whereis_syn);
}

TEST(CheatUserPlan, DamageSynForwardsToMapServer) {
    const auto request = make_cheat_user(cheat_damage_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_damage_syn);
}

TEST(CheatUserPlan, NoticeBroadcastsToAllMaps) {
    const auto request = make_cheat_user(cheat_notice_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_notice_syn);
}

TEST(CheatUserPlan, AgentCheckForwardsToMapServer) {
    const auto request = make_cheat_user(cheat_agentcheck_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_agentcheck_syn);
}

TEST(CheatUserPlan, BanMapForwardsToMapServer) {
    const auto request = make_cheat_user(cheat_banmap_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto plan = cheat_user_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_banmap_syn);
}

TEST(CheatUserPlan, PlanIsIdempotent) {
    const auto request = make_cheat_user(cheat_bancharacter_syn,
                                         cheat_user_level_developer);
    const auto action = classify_agent_cheat_user(request);
    const auto first = cheat_user_side_effect_plan(action);
    const auto second = cheat_user_side_effect_plan(action);
    EXPECT_EQ(first.dispatched, second.dispatched);
    EXPECT_EQ(first.broadcast, second.broadcast);
    EXPECT_EQ(first.forward_to_map, second.forward_to_map);
    EXPECT_EQ(first.send_to_user, second.send_to_user);
    EXPECT_EQ(first.drop, second.drop);
    ASSERT_EQ(first.effects.size(), second.effects.size());
    for (std::size_t index = 0; index < first.effects.size(); ++index) {
        EXPECT_EQ(first.effects[index].kind, second.effects[index].kind);
        EXPECT_EQ(first.effects[index].reply_protocol, second.effects[index].reply_protocol);
        EXPECT_EQ(first.effects[index].forward_payload, second.effects[index].forward_payload);
    }
}

TEST(CheatServerPlan, BanCharacterWithoutTargetDrops) {
    CheatServerRequest request = cheat_server(cheat_bancharacter_syn);
    request.target_found = false;
    const auto action = classify_agent_cheat_server(request);
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
}

TEST(CheatServerPlan, BanCharacterForwardsToUser) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_bancharacter_syn));
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.send_to_user);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, CheatUserSideEffectKind::Send2User);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_bancharacter_syn);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(CheatServerPlan, BanCharacterAckSendsToUser) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_bancharacter_ack));
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_bancharacter_ack);
}

TEST(CheatServerPlan, BanCharacterNackBroadcastsToMaps) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_bancharacter_nack));
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_bancharacter_nack);
    EXPECT_TRUE(plan.effects[0].broadcast_all_maps);
}

TEST(CheatServerPlan, WhereisWithoutTargetSendsNack) {
    CheatServerRequest request = cheat_server(cheat_whereis_syn);
    request.whereis_target_known = false;
    const auto action = classify_agent_cheat_server(request);
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_whereis_syn);
}

TEST(CheatServerPlan, WhereisAckSendsToUser) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_whereis_syn));
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_whereis_syn);
}

TEST(CheatServerPlan, WhereisAckBroadcasts) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_whereis_ack));
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_whereis_ack);
    EXPECT_TRUE(plan.effects[0].broadcast_all_maps);
}

TEST(CheatServerPlan, DefaultProtocolForwardsToMapServer) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_damage_syn));
    const auto plan = cheat_server_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_EQ(plan.effects[0].reply_protocol, cheat_damage_syn);
    EXPECT_TRUE(plan.effects[0].broadcast_all_maps);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(CheatServerPlan, PlanIsIdempotent) {
    const auto action = classify_agent_cheat_server(cheat_server(cheat_bancharacter_ack));
    const auto first = cheat_server_side_effect_plan(action);
    const auto second = cheat_server_side_effect_plan(action);
    EXPECT_EQ(first.dispatched, second.dispatched);
    EXPECT_EQ(first.broadcast, second.broadcast);
    EXPECT_EQ(first.forward_to_map, second.forward_to_map);
    EXPECT_EQ(first.send_to_user, second.send_to_user);
    EXPECT_EQ(first.drop, second.drop);
    ASSERT_EQ(first.effects.size(), second.effects.size());
    for (std::size_t index = 0; index < first.effects.size(); ++index) {
        EXPECT_EQ(first.effects[index].kind, second.effects[index].kind);
        EXPECT_EQ(first.effects[index].reply_protocol, second.effects[index].reply_protocol);
    }
}
