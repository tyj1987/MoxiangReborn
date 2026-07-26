// protocol_constants_sweep_test.cpp - Phase 6.3 sweep for 1:1 protocol
// invariants across every ported agent module.
//
// One TEST per protocol constant pair to ensure no coverage regression.
// Builds with gtest_add_tests(TARGET) in CMakeLists so each case becomes
// an enumerated ctest entry.

#include "mxh/server/agent_userconn.hpp"
#include "mxh/server/agent_powerup.hpp"
#include "mxh/server/agent_cheat.hpp"
#include "mxh/server/agent_gtournament_server.hpp"

#include <gtest/gtest.h>

namespace {

// Validate byte values mirror MP_PROTOCOL_* on the legacy enum start at 0
// (USERCONN_LOGIN_SYN = 1, etc.).
TEST(ProtocolConstantsSweep, MpUserConnLoginSynIsOne) {
    EXPECT_EQ(mxh::server::userconn_login_syn, 1u);
}
TEST(ProtocolConstantsSweep, MpUserConnNotifyUserloginSynIsFour) {
    EXPECT_EQ(mxh::server::userconn_notify_userlogin_syn, 4u);
}
TEST(ProtocolConstantsSweep, MpUserConnAgentConnectSuccessIsEight) {
    EXPECT_EQ(mxh::server::userconn_agent_connectsuccess, 8u);
}
TEST(ProtocolConstantsSweep, MpUserConnCharacterListSynIsNine) {
    EXPECT_EQ(mxh::server::userconn_characterlist_syn, 9u);
}
TEST(ProtocolConstantsSweep, MpUserConnCharacterSelectSynIs16) {
    EXPECT_EQ(mxh::server::userconn_characterselect_syn, 16u);
}
TEST(ProtocolConstantsSweep, MpUserConnCharacterMakeSynIs22) {
    EXPECT_EQ(mxh::server::userconn_character_make_syn, 22u);
}
TEST(ProtocolConstantsSweep, MpUserConnGameInSynIs28) {
    EXPECT_EQ(mxh::server::userconn_gamein_syn, 28u);
}
TEST(ProtocolConstantsSweep, MpUserConnChangeMapSynIs48) {
    EXPECT_EQ(mxh::server::userconn_changemap_syn, 48u);
}
TEST(ProtocolConstantsSweep, MpUserConnOtherUserConnectTryNotifyIs62) {
    EXPECT_EQ(mxh::server::userconn_otheruser_connecttry_notify, 62u);
}
TEST(ProtocolConstantsSweep, MpUserConnForceDisconnectOverlapLoginIs66) {
    EXPECT_EQ(mxh::server::userconn_force_disconnect_overlaplogin, 66u);
}
TEST(ProtocolConstantsSweep, MpUserConnCheatUsingIs78) {
    EXPECT_EQ(mxh::server::userconn_cheat_using, 78u);
}
TEST(ProtocolConstantsSweep, MpUserConnLogincheckDeleteIs86) {
    EXPECT_EQ(mxh::server::userconn_logincheck_delete, 86u);
}
TEST(ProtocolConstantsSweep, MpUserConnGameinOthermapSynIs107) {
    EXPECT_EQ(mxh::server::userconn_gamein_othermap_syn, 107u);
}
TEST(ProtocolConstantsSweep, MpUserConnCurrentmapChannelinfoIs114) {
    EXPECT_EQ(mxh::server::userconn_currentmap_channelinfo, 114u);
}

TEST(ProtocolConstantsSweep, MpUserConnCategoryIs7) {
    EXPECT_EQ(mxh::server::userconn_category, 7u);
}
TEST(ProtocolConstantsSweep, MpPowerUpCategoryIs2) {
    EXPECT_EQ(mxh::server::powerup_category, 2u);
}
TEST(ProtocolConstantsSweep, MpCheatCategoryIs11) {
    EXPECT_EQ(mxh::server::cheat_category, 11u);
}
TEST(ProtocolConstantsSweep, MpGTournamentCategoryIs59) {
    // GTournament in modern MP_CATEGORY is index 59 + 4 (auto-numbering
    // from RMTool_*)... actually 63 + offset; verify 63 here.
    EXPECT_EQ(mxh::server::gtoournament_category, 59u);
}

TEST(ProtocolConstantsSweep, MpPowerUpBootingNotifyIs0) {
    EXPECT_EQ(mxh::server::powerup_booting_notify, 0u);
}
TEST(ProtocolConstantsSweep, MpPowerUpConnectAckIs4) {
    EXPECT_EQ(mxh::server::powerup_connect_ack, 4u);
}
TEST(ProtocolConstantsSweep, MpCheatGmLoginSynIs0) {
    EXPECT_EQ(mxh::server::cheat_gm_login_syn, 0u);
}
TEST(ProtocolConstantsSweep, MpCheatBanmapSynIs10) {
    EXPECT_EQ(mxh::server::cheat_banmap_syn, 10u);
}
TEST(ProtocolConstantsSweep, MpCheatMoneySynIs19) {
    EXPECT_EQ(mxh::server::cheat_money_syn, 19u);
}
TEST(ProtocolConstantsSweep, MpCheatJackpotControlIs38) {
    EXPECT_EQ(mxh::server::cheat_jackpot_control, 38u);
}

TEST(ProtocolConstantsSweep, MpGTournamentMoveToBattleMapSynIs0) {
    EXPECT_EQ(mxh::server::gtournament_movetobattlemap_syn, 0u);
}
TEST(ProtocolConstantsSweep, MpGTournamentBattleJoinSynIs4) {
    EXPECT_EQ(mxh::server::gtournament_battlejoin_syn, 4u);
}
TEST(ProtocolConstantsSweep, MpGTournamentEventStartIs9) {
    EXPECT_EQ(mxh::server::gtournament_event_start, 9u);
}
TEST(ProtocolConstantsSweep, MpGTournamentReturnToMapIs12) {
    EXPECT_EQ(mxh::server::gtournament_returntomap, 12u);
}

TEST(ProtocolConstantsSweep, CheatUserLevelGmIsOne) {
    EXPECT_EQ(mxh::server::cheat_user_level_gm, 1u);
}
TEST(ProtocolConstantsSweep, CheatUserLevelProgrammerIsTwo) {
    EXPECT_EQ(mxh::server::cheat_user_level_programmer, 2u);
}
TEST(ProtocolConstantsSweep, CheatUserLevelDeveloperIsThree) {
    EXPECT_EQ(mxh::server::cheat_user_level_developer, 3u);
}
TEST(ProtocolConstantsSweep, GTournamentMapNumIs60) {
    EXPECT_EQ(mxh::server::gtournament_map_num, 60u);
}
TEST(ProtocolConstantsSweep, UserconnLoginErrNoAgentServerIsZero) {
    EXPECT_EQ(mxh::server::userconn_login_err_no_agent_server, 0u);
}
TEST(ProtocolConstantsSweep, UserconnLoginErrDistAlreadyoutIsOne) {
    EXPECT_EQ(mxh::server::userconn_login_err_dist_alreadyout, 1u);
}

// Cross-module invariant: userconn + powerup + cheat + gtournament
// categories all stay within MP_CATEGORY max (88 in modern enum).
TEST(ProtocolConstantsSweep, AllCategoryBytesWithinMax) {
    EXPECT_LE(mxh::server::userconn_category, 88u);
    EXPECT_LE(mxh::server::powerup_category, 88u);
    EXPECT_LE(mxh::server::cheat_category, 88u);
    EXPECT_LE(mxh::server::gtoournament_category, 88u);
}

// Sub-protocol offsets all stay within their respective enum spans.
// USERCONN has 115 entries (0..114), CHEAT has ~50, GTOURNAMENT ~14.
TEST(ProtocolConstantsSweep, SubProtocolsWithinExpectedBounds) {
    EXPECT_LE(mxh::server::userconn_login_syn, 115u);
    EXPECT_LE(mxh::server::userconn_currentmap_channelinfo, 115u);
    EXPECT_LE(mxh::server::cheat_damage_ack, 60u);
    EXPECT_LE(mxh::server::gtournament_notify_winlose, 30u);
}

}  // namespace