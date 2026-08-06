#include <array>
#include <cstddef>

#include <mxh/server/agent_userconn.hpp>
#include <mxh/server/agent_userconn_side_effect_plan.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

UserConnRequest make_userconn(std::uint8_t protocol) {
    UserConnRequest request{};
    request.protocol = protocol;
    request.user_id = 42u;
    request.auth_key = 7u;
    request.dist_auth_key = 7u;
    request.character_id = 0u;
    request.map_server_conn = 0u;
    request.target_map_num = 0u;
    request.channel = 0u;
    request.user_level = 0u;
    request.agent_ready = true;
    request.user_found_by_userid = true;
    request.user_found_by_conn = true;
    request.user_found_by_charid = true;
    request.auth_keys_match = true;
    request.map_port_known = true;
    request.event_blocked = false;
    request.connection_check_failed = false;
    return request;
}

TEST(UserConnPlan, NotifyUserLoginWithoutAgentSendsDistNack) {
    UserConnRequest request = make_userconn(userconn_notify_userlogin_syn);
    request.agent_ready = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.send_to_user);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::SendNackToDist);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_notify_userlogin_nack);
    EXPECT_EQ(plan.effects[0].error_code, userconn_login_err_no_agent_server);
}

TEST(UserConnPlan, NotifyUserLoginWithAgentForwardsToMap) {
    const auto action = classify_userconn(make_userconn(userconn_notify_userlogin_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_FALSE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::ForwardToMapServer);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(UserConnPlan, NotifyUserLoginAckSendsToUser) {
    const auto action = classify_userconn(make_userconn(userconn_notify_userlogin_ack));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_notify_userlogin_ack);
}

TEST(UserConnPlan, OverlappedLoginMissingUserDrops) {
    UserConnRequest request = make_userconn(userconn_notify_overlappedlogin);
    request.user_found_by_userid = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::Drop);
}

TEST(UserConnPlan, OverlappedLoginNotifiesOtherUser) {
    const auto action = classify_userconn(make_userconn(userconn_notify_overlappedlogin));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_otheruser_connecttry_notify);
}

TEST(UserConnPlan, ForceDisconnectOverlapSendsNoWaitExit) {
    UserConnRequest request = make_userconn(userconn_force_disconnect_overlaplogin);
    request.character_id = 10u;
    request.map_server_conn = 99u;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_nowaitexitplayer);
}

TEST(UserConnPlan, ForceDisconnectOverlapWithoutMapSendsDisconnect) {
    const auto action = classify_userconn(make_userconn(userconn_force_disconnect_overlaplogin));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_disconnected_by_overlaplogin);
}

TEST(UserConnPlan, ForceDisconnectMissingUserDrops) {
    UserConnRequest request = make_userconn(userconn_force_disconnect_overlaplogin);
    request.user_found_by_userid = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(UserConnPlan, CharacterListWithoutAuthDisconnects) {
    UserConnRequest request = make_userconn(userconn_characterlist_syn);
    request.auth_keys_match = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.disconnect);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_characterlist_nack);
    EXPECT_EQ(plan.effects.back().kind, UserConnSideEffectKind::DisconnectUser);
}

TEST(UserConnPlan, CharacterListWithAuthForwardsToMap) {
    const auto action = classify_userconn(make_userconn(userconn_characterlist_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_characterlist_ack);
}

TEST(UserConnPlan, GameInNackDisconnectsUser) {
    const auto action = classify_userconn(make_userconn(userconn_gamein_nack));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.disconnect);
    EXPECT_EQ(plan.effects.back().kind, UserConnSideEffectKind::DisconnectUser);
}

TEST(UserConnPlan, GameInAckSendsToUser) {
    const auto action = classify_userconn(make_userconn(userconn_gamein_ack));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_gamein_ack);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(UserConnPlan, GameInOtherMapBroadcastsExcept) {
    const auto action = classify_userconn(make_userconn(userconn_gamein_othermap_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.broadcast_except);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::BroadcastExcept2MapServers);
}

TEST(UserConnPlan, CharacterAddBroadcastsToMaps) {
    const std::array protocols = {userconn_character_add, userconn_pet_add,
        userconn_monster_add, userconn_bossmonster_add, userconn_npc_add,
        userconn_object_remove, userconn_character_die, userconn_monster_die,
        userconn_pet_die, userconn_character_revive, userconn_character_revive_nack,
        userconn_ready_to_revive};
    for (const auto protocol : protocols) {
        const auto action = classify_userconn(make_userconn(protocol));
        const auto plan = userconn_side_effect_plan(action);
        EXPECT_TRUE(plan.broadcast);
        EXPECT_EQ(plan.effects[0].reply_protocol, protocol);
    }
}

TEST(UserConnPlan, ChangeMapWithoutPortSendsNack) {
    UserConnRequest request = make_userconn(userconn_changemap_syn);
    request.map_port_known = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_changemap_nack);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(UserConnPlan, ChangeMapWithEventBlockSendsNack) {
    UserConnRequest request = make_userconn(userconn_changemap_syn);
    request.event_blocked = true;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_changemap_nack);
}

TEST(UserConnPlan, ChangeMapForwardsPacket) {
    const auto action = classify_userconn(make_userconn(userconn_changemap_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_changemap_syn);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(UserConnPlan, MapOutWithoutPortDrops) {
    UserConnRequest request = make_userconn(userconn_map_out);
    request.map_port_known = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(UserConnPlan, MapOutForwardsToMap) {
    const auto action = classify_userconn(make_userconn(userconn_map_out));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_map_out);
}

TEST(UserConnPlan, BackToCharSelWithoutUserDrops) {
    UserConnRequest request = make_userconn(userconn_backtocharsel_syn);
    request.user_found_by_conn = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(UserConnPlan, BackToCharSelForwardsToMap) {
    const auto action = classify_userconn(make_userconn(userconn_backtocharsel_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_backtocharsel_syn);
}

TEST(UserConnPlan, BackToCharSelAckSendsCharacterListAck) {
    const auto action = classify_userconn(make_userconn(userconn_backtocharsel_ack));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_characterlist_ack);
}

TEST(UserConnPlan, BackToBeforeMapSynForwardsToMap) {
    const auto action = classify_userconn(make_userconn(userconn_backtobeforemap_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_backtobeforemap_syn);
}

TEST(UserConnPlan, BackToBeforeMapAckSendsChangeMapAck) {
    const auto action = classify_userconn(make_userconn(userconn_backtobeforemap_ack));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_changemap_ack);
}

TEST(UserConnPlan, LoginCheckDeleteReplies) {
    const auto action = classify_userconn(make_userconn(userconn_logincheck_delete));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.reply_logincheck_delete);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::ReplyLogincheckDelete);
}

TEST(UserConnPlan, CheatUseLogsButNoPacket) {
    const auto action = classify_userconn(make_userconn(userconn_cheat_using));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.log_cheat);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::LogCheatUse);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_cheat_using);
}

TEST(UserConnPlan, NotifyToAgentAlreadyOutDropsWhenAuthMissing) {
    UserConnRequest request = make_userconn(userconn_notifytoagent_alreadyout);
    request.auth_keys_match = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(UserConnPlan, NotifyToAgentAlreadyOutSendsLoginNack) {
    const auto action = classify_userconn(make_userconn(userconn_notifytoagent_alreadyout));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_login_nack);
    EXPECT_EQ(plan.effects[0].error_code, userconn_login_err_dist_alreadyout);
}

TEST(UserConnPlan, RequestDistOutDisconnects) {
    const auto action = classify_userconn(make_userconn(userconn_request_distout));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.disconnect);
    EXPECT_FALSE(plan.send_to_user);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::DisconnectUser);
}

TEST(UserConnPlan, DisconnectedOnLoginDropsWithoutUserOrAuth) {
    UserConnRequest request = make_userconn(userconn_disconnected_on_login);
    request.user_found_by_userid = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(UserConnPlan, DisconnectedOnLoginDisconnects) {
    const auto action = classify_userconn(make_userconn(userconn_disconnected_on_login));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.disconnect);
    EXPECT_EQ(plan.effects[0].kind, UserConnSideEffectKind::DisconnectUser);
}

TEST(UserConnPlan, EventMapLifecycleForwardsToMap) {
    const std::array protocols = {userconn_event_ready, userconn_event_start,
        userconn_event_end, userconn_eventitem_use, userconn_eventitem_use2};
    for (const auto protocol : protocols) {
        const auto action = classify_userconn(make_userconn(protocol));
        const auto plan = userconn_side_effect_plan(action);
        EXPECT_TRUE(plan.forward_to_map);
        EXPECT_EQ(plan.effects[0].reply_protocol, protocol);
    }
}

TEST(UserConnPlan, EnterEventMapWithoutUserDrops) {
    UserConnRequest request = make_userconn(userconn_enter_eventmap_syn);
    request.user_found_by_conn = false;
    const auto action = classify_userconn(request);
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(UserConnPlan, GridinitBroadcastsToMaps) {
    const std::array protocols = {userconn_gridinit, userconn_setvisible,
        userconn_characterslot, userconn_castlegate_add, userconn_flagnpc_onoff,
        userconn_remaintime_notify};
    for (const auto protocol : protocols) {
        const auto action = classify_userconn(make_userconn(protocol));
        const auto plan = userconn_side_effect_plan(action);
        EXPECT_TRUE(plan.broadcast);
        EXPECT_EQ(plan.effects[0].reply_protocol, protocol);
    }
}

TEST(UserConnPlan, ChannelInfoSynForwardsToMap) {
    const auto action = classify_userconn(make_userconn(userconn_channelinfo_syn));
    const auto plan = userconn_side_effect_plan(action);
    EXPECT_TRUE(plan.forward_to_map);
    EXPECT_EQ(plan.effects[0].reply_protocol, userconn_channelinfo_syn);
}

TEST(UserConnPlan, LoginVariantsForwardToMap) {
    const std::array protocols = {userconn_login_syn, userconn_login_ack,
        userconn_login_nack, userconn_use_dynamic_syn, userconn_use_dynamic_ack,
        userconn_use_dynamic_nack, userconn_login_dynamic_syn,
        userconn_login_dynamic_ack, userconn_login_dynamic_nack};
    for (const auto protocol : protocols) {
        const auto action = classify_userconn(make_userconn(protocol));
        const auto plan = userconn_side_effect_plan(action);
        EXPECT_TRUE(plan.forward_to_map);
        EXPECT_EQ(plan.effects[0].reply_protocol, protocol);
    }
}

TEST(UserConnPlan, PlanIsIdempotent) {
    const auto action = classify_userconn(make_userconn(userconn_gamein_nack));
    const auto first = userconn_side_effect_plan(action);
    const auto second = userconn_side_effect_plan(action);
    EXPECT_EQ(first.dispatched, second.dispatched);
    EXPECT_EQ(first.disconnect, second.disconnect);
    EXPECT_EQ(first.send_to_user, second.send_to_user);
    EXPECT_EQ(first.drop, second.drop);
    ASSERT_EQ(first.effects.size(), second.effects.size());
    for (std::size_t index = 0; index < first.effects.size(); ++index) {
        EXPECT_EQ(first.effects[index].kind, second.effects[index].kind);
        EXPECT_EQ(first.effects[index].reply_protocol, second.effects[index].reply_protocol);
        EXPECT_EQ(first.effects[index].forward_payload, second.effects[index].forward_payload);
    }
}
