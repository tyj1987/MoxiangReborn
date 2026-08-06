// D4.106 -- AgentAutonote side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_AUTONOTEUserMsgParser (lines 5236-5252) and MP_AUTONOTEServerMsgParser
// (lines 5252-5290). Each test pins one branch of the legacy dispatch to its modern
// side-effect plan output so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_autonote.hpp"
#include "mxh/server/agent_autonote_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// USER side-effects
// ---------------------------------------------------------------------

TEST(AutonoteUserPlan, DropNoUserEmitsDropEffect) {
    AutonoteUserAction a{};
    a.kind = AutonoteUserActionKind::drop_no_user;
    a.connection_index = 5u;
    const auto plan = autonote_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::Drop);
}

TEST(AutonoteUserPlan, SendPunishEmitsPunishEffect) {
    AutonoteUserAction a{};
    a.kind = AutonoteUserActionKind::send_punish_to_user;
    a.protocol = autonote_punish;
    a.connection_index = 7u;
    a.punish_seconds = 60u;
    const auto plan = autonote_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::SendPunishToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, autonote_punish);
    EXPECT_EQ(plan.effects[0].connection_index, 7u);
    EXPECT_EQ(plan.effects[0].punish_seconds, 60u);
    EXPECT_TRUE(autonote_user_effect_targets_user(plan.effects[0]));
}

TEST(AutonoteUserPlan, ForwardToMapEmitsRawForward) {
    AutonoteUserAction a{};
    a.kind = AutonoteUserActionKind::forward_to_map;
    a.protocol = autonote_asktoauto_syn;
    a.connection_index = 7u;
    const auto plan = autonote_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(autonote_user_effect_targets_map(plan.effects[0]));
}

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(AutonoteServerPlan, AskToAutoAckEmitsAskEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::asktoauto_ack_send_and_punish;
    a.protocol = autonote_asktoauto_ack;
    a.object_id = 11u;
    a.user_id = 22u;
    a.punish_seconds = autonote_punish_seconds_ask;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::SendAskToAutoAckAndPunish);
    EXPECT_EQ(plan.effects[0].punish_seconds, autonote_punish_seconds_ask);
    EXPECT_TRUE(autonote_server_effect_targets_user(plan.effects[0]));
}

TEST(AutonoteServerPlan, NotAutoEmitsPunishAndSendEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::notauto_punish_and_send_to_user_if_character;
    a.protocol = autonote_notauto;
    a.punish_seconds = 300u;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::PunishOtherAndSendNotAuto);
    EXPECT_EQ(plan.effects[0].punish_seconds, 300u);
}

TEST(AutonoteServerPlan, AnswerAckEmitsPunishOtherEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::answer_ack_punish_other;
    a.protocol = autonote_answer_ack;
    a.punish_seconds = 180u;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::PunishOther);
    EXPECT_EQ(plan.effects[0].punish_seconds, 180u);
    EXPECT_FALSE(autonote_server_effect_targets_user(plan.effects[0]));
}

TEST(AutonoteServerPlan, AnswerFailEmitsIncrementCountEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::answer_fail_punish_count;
    a.protocol = autonote_answer_fail;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::IncrementPunishCount);
    EXPECT_FALSE(autonote_server_effect_targets_user(plan.effects[0]));
}

TEST(AutonoteServerPlan, AnswerLogoutEmitsIncrementCountEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::answer_logout_punish_count;
    a.protocol = autonote_answer_logout;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::IncrementPunishCount);
}

TEST(AutonoteServerPlan, AnswerTimeoutWithUserEmitsIncrementAndSendEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::answer_timeout_punish_count_and_send;
    a.protocol = autonote_answer_timeout;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::IncrementPunishCountAndSendTimeout);
    EXPECT_TRUE(autonote_server_effect_targets_user(plan.effects[0]));
}

TEST(AutonoteServerPlan, KillAutoEmitsSendIfCharacterEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::killauto_send_if_character;
    a.protocol = autonote_killauto;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::SendKillAutoIfCharacter);
    EXPECT_TRUE(autonote_server_effect_targets_user(plan.effects[0]));
}

TEST(AutonoteServerPlan, DisconnectEmitsDisconnectEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::disconnect_if_user;
    a.protocol = autonote_disconnect;
    a.disconnect = true;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::DisconnectUser);
    EXPECT_TRUE(plan.effects[0].disconnect);
}

TEST(AutonoteServerPlan, ForwardToUserEmitsRawForward) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::forward_to_user_if_found;
    a.protocol = autonote_list_all;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::ForwardRawToUser);
}

TEST(AutonoteServerPlan, DropNoUserEmitsDropEffect) {
    AutonoteServerAction a{};
    a.kind = AutonoteServerActionKind::drop_no_user;
    a.protocol = autonote_asktoauto_ack;
    const auto plan = autonote_server_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::Drop);
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests
// ---------------------------------------------------------------------

TEST(AutonoteUserClassifierPlan, AskToAutoWithPunishEmitsPunishPlan) {
    AutonoteUserRequest req{};
    req.protocol = autonote_asktoauto_syn;
    req.user_found = true;
    req.user_level = 0u;  // not GM
    req.punish_remaining_seconds = 30u;
    const auto action = classify_autonote_user(req);
    EXPECT_EQ(action.kind, AutonoteUserActionKind::send_punish_to_user);
    EXPECT_EQ(action.punish_seconds, 30u);
    const auto plan = autonote_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::SendPunishToUser);
    EXPECT_EQ(plan.effects[0].punish_seconds, 30u);
}

TEST(AutonoteUserClassifierPlan, AskToAutoNoPunishEmitsForwardPlan) {
    AutonoteUserRequest req{};
    req.protocol = autonote_asktoauto_syn;
    req.user_found = true;
    req.user_level = 0u;
    req.punish_remaining_seconds = 0u;
    const auto action = classify_autonote_user(req);
    EXPECT_EQ(action.kind, AutonoteUserActionKind::forward_to_map);
    const auto plan = autonote_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::ForwardRawToMap);
}

TEST(AutonoteUserClassifierPlan, AskToAutoGmWithPunishEmitsPunishPlan) {
    // Legacy: user_level<=user_level_gm AND punish>0 sends PUNISH (GM included).
    AutonoteUserRequest req{};
    req.protocol = autonote_asktoauto_syn;
    req.user_found = true;
    req.user_level = user_level_gm;
    req.punish_remaining_seconds = 30u;
    const auto action = classify_autonote_user(req);
    EXPECT_EQ(action.kind, AutonoteUserActionKind::send_punish_to_user);
    const auto plan = autonote_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::SendPunishToUser);
}

TEST(AutonoteUserClassifierPlan, AnswerSynEmitsForwardPlan) {
    AutonoteUserRequest req{};
    req.protocol = autonote_answer_syn;
    req.user_found = true;
    const auto action = classify_autonote_user(req);
    EXPECT_EQ(action.kind, AutonoteUserActionKind::forward_to_map);
    const auto plan = autonote_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteUserSideEffectKind::ForwardRawToMap);
}

TEST(AutonoteUserClassifierPlan, NoUserEmitsDropPlan) {
    AutonoteUserRequest req{};
    req.protocol = autonote_asktoauto_syn;
    req.user_found = false;
    const auto action = classify_autonote_user(req);
    EXPECT_EQ(action.kind, AutonoteUserActionKind::drop_no_user);
    const auto plan = autonote_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(AutonoteServerClassifierPlan, AskToAutoAckWithUserEmitsAskEffect) {
    AutonoteServerRequest req{};
    req.protocol = autonote_asktoauto_ack;
    req.user_object_found = true;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::asktoauto_ack_send_and_punish);
    EXPECT_EQ(action.punish_seconds, autonote_punish_seconds_ask);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::SendAskToAutoAckAndPunish);
}

TEST(AutonoteServerClassifierPlan, AskToAutoAckNoUserEmitsDrop) {
    AutonoteServerRequest req{};
    req.protocol = autonote_asktoauto_ack;
    req.user_object_found = false;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::drop_no_user);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(AutonoteServerClassifierPlan, NotAutoWithUserIdEmitsNotAutoEffect) {
    AutonoteServerRequest req{};
    req.protocol = autonote_notauto;
    req.user_id_found = true;
    req.auto_note_use_minutes = 5u;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::notauto_punish_and_send_to_user_if_character);
    EXPECT_EQ(action.punish_seconds, 300u);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::PunishOtherAndSendNotAuto);
}

TEST(AutonoteServerClassifierPlan, NotAutoNoUserIdEmitsDrop) {
    AutonoteServerRequest req{};
    req.protocol = autonote_notauto;
    req.user_id_found = false;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::drop_no_user);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(AutonoteServerClassifierPlan, AnswerAckEmitsPunishOtherEffect) {
    AutonoteServerRequest req{};
    req.protocol = autonote_answer_ack;
    req.auto_note_use_minutes = 3u;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::answer_ack_punish_other);
    EXPECT_EQ(action.punish_seconds, 180u);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::PunishOther);
}

TEST(AutonoteServerClassifierPlan, AnswerFailEmitsIncrementCount) {
    AutonoteServerRequest req{};
    req.protocol = autonote_answer_fail;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::answer_fail_punish_count);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::IncrementPunishCount);
}

TEST(AutonoteServerClassifierPlan, AnswerTimeoutWithUserEmitsIncrementAndSend) {
    AutonoteServerRequest req{};
    req.protocol = autonote_answer_timeout;
    req.user_object_found = true;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::answer_timeout_punish_count_and_send);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::IncrementPunishCountAndSendTimeout);
}

TEST(AutonoteServerClassifierPlan, AnswerTimeoutNoUserEmitsIncrementCount) {
    AutonoteServerRequest req{};
    req.protocol = autonote_answer_timeout;
    req.user_object_found = false;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::answer_fail_punish_count);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::IncrementPunishCount);
}

TEST(AutonoteServerClassifierPlan, KillAutoNoUserIdEmitsDrop) {
    AutonoteServerRequest req{};
    req.protocol = autonote_killauto;
    req.user_id_found = false;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::drop_no_user);
}

TEST(AutonoteServerClassifierPlan, DisconnectEmitsDisconnectPlan) {
    AutonoteServerRequest req{};
    req.protocol = autonote_disconnect;
    req.user_id_found = true;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::disconnect_if_user);
    EXPECT_TRUE(action.disconnect);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::DisconnectUser);
    EXPECT_TRUE(plan.effects[0].disconnect);
}

TEST(AutonoteServerClassifierPlan, UnknownProtocolWithUserEmitsForwardPlan) {
    AutonoteServerRequest req{};
    req.protocol = 99u;
    req.user_object_found = true;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::forward_to_user_if_found);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, AutonoteServerSideEffectKind::ForwardRawToUser);
}

TEST(AutonoteServerClassifierPlan, UnknownProtocolNoUserEmitsDropPlan) {
    AutonoteServerRequest req{};
    req.protocol = 99u;
    req.user_object_found = false;
    const auto action = classify_autonote_server(req);
    EXPECT_EQ(action.kind, AutonoteServerActionKind::drop_no_user);
    const auto plan = autonote_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}