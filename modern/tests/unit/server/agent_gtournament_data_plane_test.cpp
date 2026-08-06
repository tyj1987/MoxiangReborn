// agent_gtournament_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_gtournament_user (D4.155).
// Augments the legacy 13-test agent_gtournament_test.cpp with deeper coverage of:
//   - gtournament_category constant = 59 (MP_GTOURNAMENT)
//   - 12 sub-protocol constants (movetobattlemap_syn=7/nack=9, observerjoin_syn=10,
//     battlejoin_syn=13/nack=15, leave_syn=17, standinginfo_syn=18/nack=20,
//     cheat=41, event_start=43, event_end=46)
//   - gt_error_code_error constant = 0
//   - gt_map_num constant = 28 (target_map for gt-map-routed actions)
//   - 11-value GtournamentActionKind enum
//   - struct defaults (GtournamentRequest 12 fields, GtournamentAction 5 fields)
//   - classify_gtournament_user truth table:
//       !user_found wins (any protocol) -> drop_no_user
//       movetobattlemap_syn + user_map_found -> send_movetobattle_to_user_map
//       movetobattlemap_syn + !user_map_found -> send_movetobattle_nack_to_user + nack protocol + error_code
//       standinginfo_syn + gt_map_found -> send_standing_info_to_gt_map + target_map=28
//       standinginfo_syn + !gt_map_found -> send_standing_info_nack_to_user + nack protocol
//       battlejoin_syn/observerjoin_syn + gt_map_found -> send_standing_info_to_gt_map + target_map=28
//       battlejoin_syn/observerjoin_syn + !gt_map_found -> send_battlejoin_nack_to_user
//       leave_syn -> send_leave_syn_to_user_map (unconditional)
//       cheat + cheat_data==1 -> send_cheat_to_user_map
//       cheat + cheat_data!=1 + gt_map_found -> send_cheat_to_gt_map + target_map=28
//       cheat + cheat_data!=1 + !gt_map_found -> drop_no_user
//       event_start/event_end + user_level>8 -> drop_no_user (legacy GM gate)
//       event_start/event_end + user_level<=8 + gt_map_found -> send_event_to_gt_map + target_map=28
//       event_start/event_end + user_level<=8 + !gt_map_found -> drop_no_user
//       default -> forward_to_map_server (protocol preserved)
//
// 1:1 invariants (locked):
//   - gtournament_category = 59
//   - gt_error_code_error = 0
//   - gt_map_num = 28 (target_map for any gt-map-routed action)
//   - !user_found wins over any case
//   - GM gate (user_level > 8) blocks event_start/end
//   - cheat with cheat_data=1 routes to user_map (not gt_map)

#pragma once

#include "mxh/server/agent_gtournament.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_gtournament_user;
using mxh::server::gt_error_code_error;
using mxh::server::gt_map_num;
using mxh::server::gtournament_battlejoin_nack;
using mxh::server::gtournament_battlejoin_syn;
using mxh::server::gtournament_category;
using mxh::server::gtournament_cheat;
using mxh::server::gtournament_event_end;
using mxh::server::gtournament_event_start;
using mxh::server::gtournament_leave_syn;
using mxh::server::gtournament_movetobattlemap_nack;
using mxh::server::gtournament_movetobattlemap_syn;
using mxh::server::gtournament_observerjoin_syn;
using mxh::server::gtournament_standinginfo_nack;
using mxh::server::gtournament_standinginfo_syn;
using mxh::server::GtournamentAction;
using mxh::server::GtournamentActionKind;
using mxh::server::GtournamentRequest;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(GTournamentDataPlane, CategoryIsFiftyNine) {
    EXPECT_EQ(gtournament_category, 59u);
}

TEST(GTournamentDataPlane, ErrorCodeErrorIsZero) {
    EXPECT_EQ(gt_error_code_error, 0u);
}

TEST(GTournamentDataPlane, GtMapNumIsTwentyEight) {
    EXPECT_EQ(gt_map_num, 28u);
}

TEST(GTournamentDataPlane, ProtocolMoveBattleSynIsSeven) { EXPECT_EQ(gtournament_movetobattlemap_syn, 7u); }
TEST(GTournamentDataPlane, ProtocolMoveBattleNackIsNine) { EXPECT_EQ(gtournament_movetobattlemap_nack, 9u); }
TEST(GTournamentDataPlane, ProtocolObserverJoinSynIsTen) { EXPECT_EQ(gtournament_observerjoin_syn, 10u); }
TEST(GTournamentDataPlane, ProtocolBattleJoinSynIsThirteen) { EXPECT_EQ(gtournament_battlejoin_syn, 13u); }
TEST(GTournamentDataPlane, ProtocolBattleJoinNackIsFifteen) { EXPECT_EQ(gtournament_battlejoin_nack, 15u); }
TEST(GTournamentDataPlane, ProtocolLeaveSynIsSeventeen) { EXPECT_EQ(gtournament_leave_syn, 17u); }
TEST(GTournamentDataPlane, ProtocolStandingInfoSynIsEighteen) { EXPECT_EQ(gtournament_standinginfo_syn, 18u); }
TEST(GTournamentDataPlane, ProtocolStandingInfoNackIsTwenty) { EXPECT_EQ(gtournament_standinginfo_nack, 20u); }
TEST(GTournamentDataPlane, ProtocolCheatIsFortyOne) { EXPECT_EQ(gtournament_cheat, 41u); }
TEST(GTournamentDataPlane, ProtocolEventStartIsFortyThree) { EXPECT_EQ(gtournament_event_start, 43u); }
TEST(GTournamentDataPlane, ProtocolEventEndIsFortySix) { EXPECT_EQ(gtournament_event_end, 46u); }

TEST(GTournamentDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        gtournament_movetobattlemap_syn, gtournament_movetobattlemap_nack,
        gtournament_observerjoin_syn, gtournament_battlejoin_syn,
        gtournament_battlejoin_nack, gtournament_leave_syn,
        gtournament_standinginfo_syn, gtournament_standinginfo_nack,
        gtournament_cheat, gtournament_event_start, gtournament_event_end,
    };
    EXPECT_EQ(seen.size(), 11u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(GTournamentDataPlane, ActionKindHasElevenValues) {
    auto all = {
        GtournamentActionKind::forward_to_map_server,
        GtournamentActionKind::send_movetobattle_to_user_map,
        GtournamentActionKind::send_standing_info_to_gt_map,
        GtournamentActionKind::send_battlejoin_nack_to_user,
        GtournamentActionKind::send_standing_info_nack_to_user,
        GtournamentActionKind::send_movetobattle_nack_to_user,
        GtournamentActionKind::send_leave_syn_to_user_map,
        GtournamentActionKind::send_cheat_to_user_map,
        GtournamentActionKind::send_cheat_to_gt_map,
        GtournamentActionKind::send_event_to_gt_map,
        GtournamentActionKind::drop_no_user,
    };
    EXPECT_EQ(all.size(), 11u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(GTournamentDataPlane, RequestDefaults) {
    GtournamentRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_found);
    EXPECT_FALSE(r.gt_map_found);
    EXPECT_FALSE(r.user_map_found);
    EXPECT_EQ(r.user_level, 0u);
    EXPECT_EQ(r.guild_idx, 0u);
    EXPECT_EQ(r.battle_idx, 0u);
    EXPECT_EQ(r.return_map_num, 0u);
    EXPECT_EQ(r.cheat_data, 0u);
    EXPECT_EQ(r.wdata, 0u);
    EXPECT_EQ(r.unique_connect_idx, 0u);
}

TEST(GTournamentDataPlane, ActionDefaults) {
    GtournamentAction a{};
    EXPECT_EQ(a.kind, GtournamentActionKind::drop_no_user);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
    EXPECT_EQ(a.target_map, 0u);
}


// ===========================================================================
// !user_found wins
// ===========================================================================

TEST(GTournamentDataPlane, NoUserDrops) {
    GtournamentRequest r;
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_found = false;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}

TEST(GTournamentDataPlane, NoUserWinsOverEventStart) {
    GtournamentRequest r;
    r.protocol = gtournament_event_start;
    r.user_found = false;
    r.user_level = 0;  // would normally pass GM gate
    r.gt_map_found = true;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}


// ===========================================================================
// movetobattlemap_syn
// ===========================================================================

TEST(GTournamentDataPlane, MoveBattleUserMapFoundForwards) {
    GtournamentRequest r;
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_map_found = true;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_movetobattle_to_user_map);
    EXPECT_EQ(a.protocol, gtournament_movetobattlemap_syn);
    EXPECT_EQ(a.error_code, 0u);
    EXPECT_EQ(a.target_map, 0u);
}

TEST(GTournamentDataPlane, MoveBattleUserMapMissingNacks) {
    GtournamentRequest r;
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_map_found = false;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_movetobattle_nack_to_user);
    EXPECT_EQ(a.protocol, gtournament_movetobattlemap_nack);
    EXPECT_EQ(a.error_code, gt_error_code_error);
}


// ===========================================================================
// standinginfo_syn
// ===========================================================================

TEST(GTournamentDataPlane, StandingInfoGtMapFoundForwards) {
    GtournamentRequest r;
    r.protocol = gtournament_standinginfo_syn;
    r.gt_map_found = true;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_standing_info_to_gt_map);
    EXPECT_EQ(a.protocol, gtournament_standinginfo_syn);
    EXPECT_EQ(a.target_map, gt_map_num);
}

TEST(GTournamentDataPlane, StandingInfoGtMapMissingNacks) {
    GtournamentRequest r;
    r.protocol = gtournament_standinginfo_syn;
    r.gt_map_found = false;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_standing_info_nack_to_user);
    EXPECT_EQ(a.protocol, gtournament_standinginfo_nack);
    EXPECT_EQ(a.error_code, gt_error_code_error);
}


// ===========================================================================
// battlejoin_syn / observerjoin_syn
// ===========================================================================

TEST(GTournamentDataPlane, BattleJoinGtMapFoundForwards) {
    GtournamentRequest r;
    r.protocol = gtournament_battlejoin_syn;
    r.gt_map_found = true;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_standing_info_to_gt_map);
    EXPECT_EQ(a.target_map, gt_map_num);
}

TEST(GTournamentDataPlane, BattleJoinGtMapMissingNacks) {
    GtournamentRequest r;
    r.protocol = gtournament_battlejoin_syn;
    r.gt_map_found = false;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_battlejoin_nack_to_user);
    EXPECT_EQ(a.protocol, gtournament_battlejoin_nack);
}

TEST(GTournamentDataPlane, ObserverJoinGtMapFoundForwards) {
    GtournamentRequest r;
    r.protocol = gtournament_observerjoin_syn;
    r.gt_map_found = true;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_standing_info_to_gt_map);
    EXPECT_EQ(a.target_map, gt_map_num);
}

TEST(GTournamentDataPlane, ObserverJoinGtMapMissingNacks) {
    GtournamentRequest r;
    r.protocol = gtournament_observerjoin_syn;
    r.gt_map_found = false;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_battlejoin_nack_to_user);
}


// ===========================================================================
// leave_syn -- unconditional
// ===========================================================================

TEST(GTournamentDataPlane, LeaveSynSendsToUserMap) {
    GtournamentRequest r;
    r.protocol = gtournament_leave_syn;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::send_leave_syn_to_user_map);
}

TEST(GTournamentDataPlane, LeaveSynIgnoresAllFlags) {
    GtournamentRequest r;
    r.protocol = gtournament_leave_syn;
    r.user_map_found = false;
    r.gt_map_found = false;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::send_leave_syn_to_user_map);
}


// ===========================================================================
// cheat -- 3-way branch
// ===========================================================================

TEST(GTournamentDataPlane, CheatData1SendsToUserMap) {
    GtournamentRequest r;
    r.protocol = gtournament_cheat;
    r.cheat_data = 1;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::send_cheat_to_user_map);
}

TEST(GTournamentDataPlane, CheatData1IgnoresGtMapFound) {
    GtournamentRequest r;
    r.protocol = gtournament_cheat;
    r.cheat_data = 1;
    r.gt_map_found = true;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::send_cheat_to_user_map);
}

TEST(GTournamentDataPlane, CheatData2GtMapFoundSendsToGt) {
    GtournamentRequest r;
    r.protocol = gtournament_cheat;
    r.cheat_data = 2;
    r.gt_map_found = true;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_cheat_to_gt_map);
    EXPECT_EQ(a.target_map, gt_map_num);
}

TEST(GTournamentDataPlane, CheatData2GtMapMissingDrops) {
    GtournamentRequest r;
    r.protocol = gtournament_cheat;
    r.cheat_data = 2;
    r.gt_map_found = false;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}

TEST(GTournamentDataPlane, CheatDataZeroGtMapMissingDrops) {
    GtournamentRequest r;
    r.protocol = gtournament_cheat;
    r.cheat_data = 0;
    r.gt_map_found = false;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}


// ===========================================================================
// event_start / event_end -- GM gate
// ===========================================================================

TEST(GTournamentDataPlane, EventStartGmBlockedByLevel) {
    GtournamentRequest r;
    r.protocol = gtournament_event_start;
    r.user_level = 10;
    r.gt_map_found = true;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}

TEST(GTournamentDataPlane, EventStartGmLevelSends) {
    GtournamentRequest r;
    r.protocol = gtournament_event_start;
    r.user_level = 8;
    r.gt_map_found = true;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::send_event_to_gt_map);
    EXPECT_EQ(a.target_map, gt_map_num);
}

TEST(GTournamentDataPlane, EventStartGmLevelZeroSends) {
    GtournamentRequest r;
    r.protocol = gtournament_event_start;
    r.user_level = 0;
    r.gt_map_found = true;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::send_event_to_gt_map);
}

TEST(GTournamentDataPlane, EventEndGmLevelSends) {
    GtournamentRequest r;
    r.protocol = gtournament_event_end;
    r.user_level = 8;
    r.gt_map_found = true;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::send_event_to_gt_map);
}

TEST(GTournamentDataPlane, EventEndGmBlockedByLevel) {
    GtournamentRequest r;
    r.protocol = gtournament_event_end;
    r.user_level = 9;
    r.gt_map_found = true;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}

TEST(GTournamentDataPlane, EventStartGmLevelGtMapMissingDrops) {
    GtournamentRequest r;
    r.protocol = gtournament_event_start;
    r.user_level = 8;
    r.gt_map_found = false;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::drop_no_user);
}


// ===========================================================================
// default -- forward_to_map_server
// ===========================================================================

TEST(GTournamentDataPlane, UnknownProtocolForwardsToMapServer) {
    GtournamentRequest r;
    r.protocol = 0;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::forward_to_map_server);
}

TEST(GTournamentDataPlane, UnknownProtocolPreservesProtocol) {
    GtournamentRequest r;
    r.protocol = 99;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(GTournamentDataPlane, Protocol255ForwardsToMapServer) {
    GtournamentRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_gtournament_user(r).kind, GtournamentActionKind::forward_to_map_server);
}


// ===========================================================================
// object_id preservation
// ===========================================================================

TEST(GTournamentDataPlane, PreservesObjectIdMaxUint32) {
    GtournamentRequest r;
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_map_found = true;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(GTournamentDataPlane, PreservesObjectIdOnDrop) {
    GtournamentRequest r;
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    auto a = classify_gtournament_user(r);
    EXPECT_EQ(a.object_id, 0xCAFEBABEu);
}
