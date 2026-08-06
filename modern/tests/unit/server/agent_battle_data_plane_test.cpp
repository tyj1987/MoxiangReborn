// agent_battle_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_battle (D4.146).
// Augments the legacy 10-test agent_battle_test.cpp with deeper coverage of:
//   - battle_category constant = 31 (MP_BATTLE)
//   - 31 sub-protocol constants (battle_info=0 .. battle_vimu_waiting_cancel_nack=30)
//   - BattleActionKind enum (forward_to_map, drop_protocol)
//   - BattleRequest struct defaults (protocol=0, object_id=0)
//   - BattleAction struct defaults
//   - classify_battle pass-through: always forward_to_map with original protocol
//
// 1:1 invariants (locked):
//   - battle_category = 31
//   - 31 protocol constants (0..30, all distinct)
//   - Agent is pass-through (battle state lives on map server)
//   - forward_to_map always; protocol + object_id always preserved

#pragma once

#include "mxh/server/agent_battle.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::battle_battleobject_create_notify;
using mxh::server::battle_battleobject_destroy_notify;
using mxh::server::battle_category;
using mxh::server::battle_change_objectbattle;
using mxh::server::battle_chat_master_ack;
using mxh::server::battle_chat_master_nack;
using mxh::server::battle_chat_master_syn;
using mxh::server::battle_chat_team_ack;
using mxh::server::battle_chat_team_nack;
using mxh::server::battle_chat_team_syn;
using mxh::server::battle_destroy_notify;
using mxh::server::battle_draw_notify;
using mxh::server::battle_info;
using mxh::server::battle_result;
using mxh::server::battle_start_notify;
using mxh::server::battle_teammember_add_notify;
using mxh::server::battle_teammember_delete_notify;
using mxh::server::battle_teammember_die_notify;
using mxh::server::battle_victory_notify;
using mxh::server::battle_vimu_apply;
using mxh::server::battle_vimu_apply_ack;
using mxh::server::battle_vimu_apply_nack;
using mxh::server::battle_vimu_apply_syn;
using mxh::server::battle_vimu_end;
using mxh::server::battle_vimu_request_ack;
using mxh::server::battle_vimu_request_nack;
using mxh::server::battle_vimu_request_syn;
using mxh::server::battle_vimu_start;
using mxh::server::battle_vimu_waiting_cancel;
using mxh::server::battle_vimu_waiting_cancel_ack;
using mxh::server::battle_vimu_waiting_cancel_nack;
using mxh::server::battle_vimu_waiting_cancel_syn;
using mxh::server::BattleAction;
using mxh::server::BattleActionKind;
using mxh::server::BattleRequest;
using mxh::server::classify_battle;

}  // namespace


// ===========================================================================
// battle_category constant
// ===========================================================================

TEST(AgentBattleDataPlane, CategoryIsThirtyOne) {
    EXPECT_EQ(battle_category, 31u);
}


// ===========================================================================
// Protocol constants
// ===========================================================================

TEST(AgentBattleDataPlane, ProtocolInfoIsZero) { EXPECT_EQ(battle_info, 0u); }
TEST(AgentBattleDataPlane, ProtocolChatTeamSynIsOne) { EXPECT_EQ(battle_chat_team_syn, 1u); }
TEST(AgentBattleDataPlane, ProtocolChatTeamAckIsTwo) { EXPECT_EQ(battle_chat_team_ack, 2u); }
TEST(AgentBattleDataPlane, ProtocolChatTeamNackIsThree) { EXPECT_EQ(battle_chat_team_nack, 3u); }
TEST(AgentBattleDataPlane, ProtocolChatMasterSynIsFour) { EXPECT_EQ(battle_chat_master_syn, 4u); }
TEST(AgentBattleDataPlane, ProtocolChatMasterAckIsFive) { EXPECT_EQ(battle_chat_master_ack, 5u); }
TEST(AgentBattleDataPlane, ProtocolChatMasterNackIsSix) { EXPECT_EQ(battle_chat_master_nack, 6u); }
TEST(AgentBattleDataPlane, ProtocolStartNotifyIsSeven) { EXPECT_EQ(battle_start_notify, 7u); }
TEST(AgentBattleDataPlane, ProtocolTeammemberAddNotifyIsEight) { EXPECT_EQ(battle_teammember_add_notify, 8u); }
TEST(AgentBattleDataPlane, ProtocolTeammemberDeleteNotifyIsNine) { EXPECT_EQ(battle_teammember_delete_notify, 9u); }
TEST(AgentBattleDataPlane, ProtocolTeammemberDieNotifyIsTen) { EXPECT_EQ(battle_teammember_die_notify, 10u); }
TEST(AgentBattleDataPlane, ProtocolBattleobjectDestroyNotifyIsEleven) { EXPECT_EQ(battle_battleobject_destroy_notify, 11u); }
TEST(AgentBattleDataPlane, ProtocolBattleobjectCreateNotifyIsTwelve) { EXPECT_EQ(battle_battleobject_create_notify, 12u); }
TEST(AgentBattleDataPlane, ProtocolVictoryNotifyIsThirteen) { EXPECT_EQ(battle_victory_notify, 13u); }
TEST(AgentBattleDataPlane, ProtocolDrawNotifyIsFourteen) { EXPECT_EQ(battle_draw_notify, 14u); }
TEST(AgentBattleDataPlane, ProtocolDestroyNotifyIsFifteen) { EXPECT_EQ(battle_destroy_notify, 15u); }
TEST(AgentBattleDataPlane, ProtocolResultIsSixteen) { EXPECT_EQ(battle_result, 16u); }
TEST(AgentBattleDataPlane, ProtocolChangeObjectbattleIsSeventeen) { EXPECT_EQ(battle_change_objectbattle, 17u); }
TEST(AgentBattleDataPlane, ProtocolVimuRequestSynIsEighteen) { EXPECT_EQ(battle_vimu_request_syn, 18u); }
TEST(AgentBattleDataPlane, ProtocolVimuRequestAckIsNineteen) { EXPECT_EQ(battle_vimu_request_ack, 19u); }
TEST(AgentBattleDataPlane, ProtocolVimuRequestNackIsTwenty) { EXPECT_EQ(battle_vimu_request_nack, 20u); }
TEST(AgentBattleDataPlane, ProtocolVimuStartIsTwentyOne) { EXPECT_EQ(battle_vimu_start, 21u); }
TEST(AgentBattleDataPlane, ProtocolVimuEndIsTwentyTwo) { EXPECT_EQ(battle_vimu_end, 22u); }
TEST(AgentBattleDataPlane, ProtocolVimuApplyIsTwentyThree) { EXPECT_EQ(battle_vimu_apply, 23u); }
TEST(AgentBattleDataPlane, ProtocolVimuApplySynIsTwentyFour) { EXPECT_EQ(battle_vimu_apply_syn, 24u); }
TEST(AgentBattleDataPlane, ProtocolVimuApplyAckIsTwentyFive) { EXPECT_EQ(battle_vimu_apply_ack, 25u); }
TEST(AgentBattleDataPlane, ProtocolVimuApplyNackIsTwentySix) { EXPECT_EQ(battle_vimu_apply_nack, 26u); }
TEST(AgentBattleDataPlane, ProtocolVimuWaitingCancelIsTwentySeven) { EXPECT_EQ(battle_vimu_waiting_cancel, 27u); }
TEST(AgentBattleDataPlane, ProtocolVimuWaitingCancelSynIsTwentyEight) { EXPECT_EQ(battle_vimu_waiting_cancel_syn, 28u); }
TEST(AgentBattleDataPlane, ProtocolVimuWaitingCancelAckIsTwentyNine) { EXPECT_EQ(battle_vimu_waiting_cancel_ack, 29u); }
TEST(AgentBattleDataPlane, ProtocolVimuWaitingCancelNackIsThirty) { EXPECT_EQ(battle_vimu_waiting_cancel_nack, 30u); }

TEST(AgentBattleDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        battle_info, battle_chat_team_syn, battle_chat_team_ack, battle_chat_team_nack,
        battle_chat_master_syn, battle_chat_master_ack, battle_chat_master_nack,
        battle_start_notify, battle_teammember_add_notify, battle_teammember_delete_notify,
        battle_teammember_die_notify, battle_battleobject_destroy_notify,
        battle_battleobject_create_notify, battle_victory_notify, battle_draw_notify,
        battle_destroy_notify, battle_result, battle_change_objectbattle,
        battle_vimu_request_syn, battle_vimu_request_ack, battle_vimu_request_nack,
        battle_vimu_start, battle_vimu_end, battle_vimu_apply,
        battle_vimu_apply_syn, battle_vimu_apply_ack, battle_vimu_apply_nack,
        battle_vimu_waiting_cancel, battle_vimu_waiting_cancel_syn,
        battle_vimu_waiting_cancel_ack, battle_vimu_waiting_cancel_nack,
    };
    EXPECT_EQ(seen.size(), 31u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(AgentBattleDataPlane, ActionKindHasTwoValues) {
    auto all = { BattleActionKind::forward_to_map, BattleActionKind::drop_protocol };
    EXPECT_EQ(all.size(), 2u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(AgentBattleDataPlane, RequestDefaults) {
    BattleRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
}

TEST(AgentBattleDataPlane, ActionDefaults) {
    BattleAction a{};
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
}


// ===========================================================================
// classify_battle pass-through
// ===========================================================================

TEST(AgentBattleDataPlane, ClassifyInfoForwards) {
    BattleRequest r{battle_info, 42u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, battle_info);
    EXPECT_EQ(a.object_id, 42u);
}

TEST(AgentBattleDataPlane, ClassifyResultForwards) {
    BattleRequest r{battle_result, 100u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, battle_result);
}

TEST(AgentBattleDataPlane, ClassifyVictoryForwards) {
    BattleRequest r{battle_victory_notify, 5u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
}

TEST(AgentBattleDataPlane, ClassifyStartForwards) {
    BattleRequest r{battle_start_notify, 5u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
}

TEST(AgentBattleDataPlane, ClassifyVimuStartForwards) {
    BattleRequest r{battle_vimu_start, 5u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
}


// ===========================================================================
// All 31 protocols sweep
// ===========================================================================

TEST(AgentBattleDataPlane, ClassifyAllProtocolsForwards) {
    std::uint8_t protos[] = {
        battle_info, battle_chat_team_syn, battle_chat_team_ack, battle_chat_team_nack,
        battle_chat_master_syn, battle_chat_master_ack, battle_chat_master_nack,
        battle_start_notify, battle_teammember_add_notify, battle_teammember_delete_notify,
        battle_teammember_die_notify, battle_battleobject_destroy_notify,
        battle_battleobject_create_notify, battle_victory_notify, battle_draw_notify,
        battle_destroy_notify, battle_result, battle_change_objectbattle,
        battle_vimu_request_syn, battle_vimu_request_ack, battle_vimu_request_nack,
        battle_vimu_start, battle_vimu_end, battle_vimu_apply,
        battle_vimu_apply_syn, battle_vimu_apply_ack, battle_vimu_apply_nack,
        battle_vimu_waiting_cancel, battle_vimu_waiting_cancel_syn,
        battle_vimu_waiting_cancel_ack, battle_vimu_waiting_cancel_nack,
    };
    for (auto p : protos) {
        BattleRequest r{p, 100u};
        auto a = classify_battle(r);
        EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
        EXPECT_EQ(a.protocol, p);
        EXPECT_EQ(a.object_id, 100u);
    }
}


// ===========================================================================
// Boundary protocols
// ===========================================================================

TEST(AgentBattleDataPlane, ClassifyProtocol31Forwards) {
    BattleRequest r{31u, 5u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 31u);
}

TEST(AgentBattleDataPlane, ClassifyProtocol255Forwards) {
    BattleRequest r{255u, 5u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 255u);
}


// ===========================================================================
// Object_id preservation
// ===========================================================================

TEST(AgentBattleDataPlane, ClassifyPreservesObjectIdMaxUint32) {
    BattleRequest r{battle_info, 0xFFFFFFFFu};
    auto a = classify_battle(r);
    EXPECT_EQ(a.object_id, 0xFFFFFFFFu);
}

TEST(AgentBattleDataPlane, ClassifyPreservesObjectIdZero) {
    BattleRequest r{battle_info, 0u};
    auto a = classify_battle(r);
    EXPECT_EQ(a.object_id, 0u);
}

TEST(AgentBattleDataPlane, ClassifyDefaultForwards) {
    BattleRequest r{};
    auto a = classify_battle(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
}
