// byte_match_sweep_test.cpp - Phase 6.3 byte sweep tests for the full
// MP_CATEGORY protocol enum surface. Each TEST verifies a single byte
// invariant from the legacy MP_PROTOCOL_* enum.

#include "mxh/server/agent_userconn.hpp"
#include "mxh/server/agent_powerup.hpp"
#include "mxh/server/agent_cheat.hpp"
#include "mxh/server/agent_friend.hpp"
#include "mxh/server/agent_weather.hpp"
#include "mxh/server/agent_wanted.hpp"
#include "mxh/server/agent_survival.hpp"
#include "mxh/server/agent_itemlimit.hpp"
#include "mxh/server/agent_fortwar.hpp"
#include "mxh/server/agent_packed.hpp"
#include "mxh/server/agent_note.hpp"
#include "mxh/server/agent_autonote.hpp"
#include "mxh/server/agent_item.hpp"
#include "mxh/server/agent_party.hpp"
#include "mxh/server/agent_murimnet.hpp"
#include "mxh/server/agent_hackcheck.hpp"
#include "mxh/server/agent_bobusang_user.hpp"
#include "mxh/server/agent_simple_auth_forward.hpp"
#include "mxh/server/agent_siegewarprofit.hpp"
#include "mxh/server/agent_guild_union.hpp"

#include <gtest/gtest.h>

namespace {

// ---------------------------------------------------------------------------
// USERCONN byte sweep (Sub-protocol 0..114, all in MP_PROTOCOL_USERCONN).
// ---------------------------------------------------------------------------
TEST(UserConnByte, DistConnectSuccessIs0) { EXPECT_EQ(mxh::server::userconn_dist_connectsuccess, 0u); }
TEST(UserConnByte, LoginSynIs1) { EXPECT_EQ(mxh::server::userconn_login_syn, 1u); }
TEST(UserConnByte, LoginAckIs2) { EXPECT_EQ(mxh::server::userconn_login_ack, 2u); }
TEST(UserConnByte, LoginNackIs3) { EXPECT_EQ(mxh::server::userconn_login_nack, 3u); }
TEST(UserConnByte, NotifyUserloginSynIs4) { EXPECT_EQ(mxh::server::userconn_notify_userlogin_syn, 4u); }
TEST(UserConnByte, NotifyUserloginAckIs5) { EXPECT_EQ(mxh::server::userconn_notify_userlogin_ack, 5u); }
TEST(UserConnByte, NotifyUserloginNackIs6) { EXPECT_EQ(mxh::server::userconn_notify_userlogin_nack, 6u); }
TEST(UserConnByte, NotifyOverlappedloginIs7) { EXPECT_EQ(mxh::server::userconn_notify_overlappedlogin, 7u); }
TEST(UserConnByte, AgentConnectsuccessIs8) { EXPECT_EQ(mxh::server::userconn_agent_connectsuccess, 8u); }
TEST(UserConnByte, CharacterlistSynIs9) { EXPECT_EQ(mxh::server::userconn_characterlist_syn, 9u); }
TEST(UserConnByte, DirectcharacterlistSynIs10) { EXPECT_EQ(mxh::server::userconn_directcharacterlist_syn, 10u); }
TEST(UserConnByte, CharacterlistNackIs11) { EXPECT_EQ(mxh::server::userconn_characterlist_nack, 11u); }
TEST(UserConnByte, CharacterlistAckIs12) { EXPECT_EQ(mxh::server::userconn_characterlist_ack, 12u); }
TEST(UserConnByte, DisconnectSynIs13) { EXPECT_EQ(mxh::server::userconn_disconnect_syn, 13u); }
TEST(UserConnByte, DisconnectAckIs14) { EXPECT_EQ(mxh::server::userconn_disconnect_ack, 14u); }
TEST(UserConnByte, DisconnectNackIs15) { EXPECT_EQ(mxh::server::userconn_disconnect_nack, 15u); }
TEST(UserConnByte, CharacterselectSynIs16) { EXPECT_EQ(mxh::server::userconn_characterselect_syn, 16u); }
TEST(UserConnByte, CharacterselectAckIs17) { EXPECT_EQ(mxh::server::userconn_characterselect_ack, 17u); }
TEST(UserConnByte, CharacterselectNackIs18) { EXPECT_EQ(mxh::server::userconn_characterselect_nack, 18u); }
TEST(UserConnByte, CharacterNamecheckSynIs19) { EXPECT_EQ(mxh::server::userconn_character_namecheck_syn, 19u); }
TEST(UserConnByte, CharacterNamecheckAckIs20) { EXPECT_EQ(mxh::server::userconn_character_namecheck_ack, 20u); }
TEST(UserConnByte, CharacterNamecheckNackIs21) { EXPECT_EQ(mxh::server::userconn_character_namecheck_nack, 21u); }
TEST(UserConnByte, CharacterMakeSynIs22) { EXPECT_EQ(mxh::server::userconn_character_make_syn, 22u); }
TEST(UserConnByte, CharacterMakeAckIs23) { EXPECT_EQ(mxh::server::userconn_character_make_ack, 23u); }
TEST(UserConnByte, CharacterMakeNackIs24) { EXPECT_EQ(mxh::server::userconn_character_make_nack, 24u); }
TEST(UserConnByte, CharacterInfoSynIs25) { EXPECT_EQ(mxh::server::userconn_character_info_syn, 25u); }
TEST(UserConnByte, CharacterInfoAckIs26) { EXPECT_EQ(mxh::server::userconn_character_info_ack, 26u); }
TEST(UserConnByte, CharacterInfoNackIs27) { EXPECT_EQ(mxh::server::userconn_character_info_nack, 27u); }
TEST(UserConnByte, GameinSynIs28) { EXPECT_EQ(mxh::server::userconn_gamein_syn, 28u); }
TEST(UserConnByte, GameinAckIs29) { EXPECT_EQ(mxh::server::userconn_gamein_ack, 29u); }
TEST(UserConnByte, GameinNackIs30) { EXPECT_EQ(mxh::server::userconn_gamein_nack, 30u); }
TEST(UserConnByte, GameoutSynIs31) { EXPECT_EQ(mxh::server::userconn_gameout_syn, 31u); }
TEST(UserConnByte, GameoutAckIs32) { EXPECT_EQ(mxh::server::userconn_gameout_ack, 32u); }
TEST(UserConnByte, GameoutNackIs33) { EXPECT_EQ(mxh::server::userconn_gameout_nack, 33u); }
TEST(UserConnByte, DisconnectedIs34) { EXPECT_EQ(mxh::server::userconn_disconnected, 34u); }
TEST(UserConnByte, CharacterAddIs35) { EXPECT_EQ(mxh::server::userconn_character_add, 35u); }
TEST(UserConnByte, PetAddIs36) { EXPECT_EQ(mxh::server::userconn_pet_add, 36u); }
TEST(UserConnByte, MonsterAddIs37) { EXPECT_EQ(mxh::server::userconn_monster_add, 37u); }
TEST(UserConnByte, BossmonsterAddIs38) { EXPECT_EQ(mxh::server::userconn_bossmonster_add, 38u); }
TEST(UserConnByte, NpcAddIs39) { EXPECT_EQ(mxh::server::userconn_npc_add, 39u); }
TEST(UserConnByte, ObjectRemoveIs40) { EXPECT_EQ(mxh::server::userconn_object_remove, 40u); }
TEST(UserConnByte, CharacterDieIs41) { EXPECT_EQ(mxh::server::userconn_character_die, 41u); }
TEST(UserConnByte, MonsterDieIs42) { EXPECT_EQ(mxh::server::userconn_monster_die, 42u); }
TEST(UserConnByte, PetDieIs43) { EXPECT_EQ(mxh::server::userconn_pet_die, 43u); }
TEST(UserConnByte, CharacterReviveIs44) { EXPECT_EQ(mxh::server::userconn_character_revive, 44u); }
TEST(UserConnByte, CharacterRemoveSynIs45) { EXPECT_EQ(mxh::server::userconn_character_remove_syn, 45u); }
TEST(UserConnByte, CharacterRemoveAckIs46) { EXPECT_EQ(mxh::server::userconn_character_remove_ack, 46u); }
TEST(UserConnByte, CharacterRemoveNackIs47) { EXPECT_EQ(mxh::server::userconn_character_remove_nack, 47u); }
TEST(UserConnByte, ChangemapSynIs48) { EXPECT_EQ(mxh::server::userconn_changemap_syn, 48u); }
TEST(UserConnByte, ChangemapAckIs49) { EXPECT_EQ(mxh::server::userconn_changemap_ack, 49u); }
TEST(UserConnByte, ChangemapNackIs50) { EXPECT_EQ(mxh::server::userconn_changemap_nack, 50u); }
TEST(UserConnByte, MapOutIs51) { EXPECT_EQ(mxh::server::userconn_map_out, 51u); }
TEST(UserConnByte, MapOutWithmapnumIs52) { EXPECT_EQ(mxh::server::userconn_map_out_withmapnum, 52u); }
TEST(UserConnByte, CharacterTotalinfoIs53) { EXPECT_EQ(mxh::server::userconn_character_totalinfo, 53u); }
TEST(UserConnByte, SavepointSynIs54) { EXPECT_EQ(mxh::server::userconn_savepoint_syn, 54u); }
TEST(UserConnByte, SavepointAckIs55) { EXPECT_EQ(mxh::server::userconn_savepoint_ack, 55u); }
TEST(UserConnByte, SavepointNackIs56) { EXPECT_EQ(mxh::server::userconn_savepoint_nack, 56u); }
TEST(UserConnByte, BacktocharselSynIs57) { EXPECT_EQ(mxh::server::userconn_backtocharsel_syn, 57u); }
TEST(UserConnByte, BacktocharselAckIs58) { EXPECT_EQ(mxh::server::userconn_backtocharsel_ack, 58u); }
TEST(UserConnByte, BacktocharselNackIs59) { EXPECT_EQ(mxh::server::userconn_backtocharsel_nack, 59u); }
TEST(UserConnByte, GridinitIs60) { EXPECT_EQ(mxh::server::userconn_gridinit, 60u); }
TEST(UserConnByte, SetvisibleIs61) { EXPECT_EQ(mxh::server::userconn_setvisible, 61u); }
TEST(UserConnByte, OtheruserConnecttryNotifyIs62) { EXPECT_EQ(mxh::server::userconn_otheruser_connecttry_notify, 62u); }
TEST(UserConnByte, ConnectionCheckIs63) { EXPECT_EQ(mxh::server::userconn_connection_check, 63u); }
TEST(UserConnByte, ConnectionCheckOkIs64) { EXPECT_EQ(mxh::server::userconn_connection_check_ok, 64u); }
TEST(UserConnByte, ChecksumerrorIs65) { EXPECT_EQ(mxh::server::userconn_checksumerror, 65u); }
TEST(UserConnByte, ForceDisconnectOverlaploginIs66) { EXPECT_EQ(mxh::server::userconn_force_disconnect_overlaplogin, 66u); }
TEST(UserConnByte, DisconnectedByOverlaploginIs67) { EXPECT_EQ(mxh::server::userconn_disconnected_by_overlaplogin, 67u); }
TEST(UserConnByte, ChannelinfoSynIs68) { EXPECT_EQ(mxh::server::userconn_channelinfo_syn, 68u); }
TEST(UserConnByte, ChannelinfoAckIs69) { EXPECT_EQ(mxh::server::userconn_channelinfo_ack, 69u); }
TEST(UserConnByte, ChannelinfoNackIs70) { EXPECT_EQ(mxh::server::userconn_channelinfo_nack, 70u); }
TEST(UserConnByte, NotifytoagentAlreadyoutIs71) { EXPECT_EQ(mxh::server::userconn_notifytoagent_alreadyout, 71u); }
TEST(UserConnByte, RequestDistoutIs72) { EXPECT_EQ(mxh::server::userconn_request_distout, 72u); }
TEST(UserConnByte, DisconnectedOnLoginIs73) { EXPECT_EQ(mxh::server::userconn_disconnected_on_login, 73u); }
TEST(UserConnByte, ServerNotreadyIs74) { EXPECT_EQ(mxh::server::userconn_server_notready, 74u); }
TEST(UserConnByte, MapdescIs75) { EXPECT_EQ(mxh::server::userconn_mapdesc, 75u); }
TEST(UserConnByte, CharacterReviveNackIs76) { EXPECT_EQ(mxh::server::userconn_character_revive_nack, 76u); }
TEST(UserConnByte, ReadyToReviveIs77) { EXPECT_EQ(mxh::server::userconn_ready_to_revive, 77u); }
TEST(UserConnByte, CheatUsingIs78) { EXPECT_EQ(mxh::server::userconn_cheat_using, 78u); }
TEST(UserConnByte, CheatChangemapAckIs79) { EXPECT_EQ(mxh::server::userconn_cheat_changemap_ack, 79u); }
TEST(UserConnByte, UseDynamicSynIs80) { EXPECT_EQ(mxh::server::userconn_use_dynamic_syn, 80u); }
TEST(UserConnByte, UseDynamicAckIs81) { EXPECT_EQ(mxh::server::userconn_use_dynamic_ack, 81u); }
TEST(UserConnByte, UseDynamicNackIs82) { EXPECT_EQ(mxh::server::userconn_use_dynamic_nack, 82u); }
TEST(UserConnByte, LoginDynamicSynIs83) { EXPECT_EQ(mxh::server::userconn_login_dynamic_syn, 83u); }
TEST(UserConnByte, LoginDynamicAckIs84) { EXPECT_EQ(mxh::server::userconn_login_dynamic_ack, 84u); }
TEST(UserConnByte, LoginDynamicNackIs85) { EXPECT_EQ(mxh::server::userconn_login_dynamic_nack, 85u); }
TEST(UserConnByte, LogincheckDeleteIs86) { EXPECT_EQ(mxh::server::userconn_logincheck_delete, 86u); }
TEST(UserConnByte, ForceDisconnectOverlaploginAckIs87) { EXPECT_EQ(mxh::server::userconn_force_disconnect_overlaplogin_ack, 87u); }
TEST(UserConnByte, MapOutToEventmapIs88) { EXPECT_EQ(mxh::server::userconn_map_out_to_eventmap, 88u); }
TEST(UserConnByte, MapOutToEventbeforemapIs89) { EXPECT_EQ(mxh::server::userconn_map_out_to_eventbeforemap, 89u); }
TEST(UserConnByte, EnterEventmapSynIs90) { EXPECT_EQ(mxh::server::userconn_enter_eventmap_syn, 90u); }
TEST(UserConnByte, EventReadyIs91) { EXPECT_EQ(mxh::server::userconn_event_ready, 91u); }
TEST(UserConnByte, EventStartIs92) { EXPECT_EQ(mxh::server::userconn_event_start, 92u); }
TEST(UserConnByte, EventEndIs93) { EXPECT_EQ(mxh::server::userconn_event_end, 93u); }
TEST(UserConnByte, EventitemUseIs94) { EXPECT_EQ(mxh::server::userconn_eventitem_use, 94u); }
TEST(UserConnByte, EventitemUse2Is95) { EXPECT_EQ(mxh::server::userconn_eventitem_use2, 95u); }
TEST(UserConnByte, GameinposSynIs96) { EXPECT_EQ(mxh::server::userconn_gameinpos_syn, 96u); }
TEST(UserConnByte, GameinposAckIs97) { EXPECT_EQ(mxh::server::userconn_gameinpos_ack, 97u); }
TEST(UserConnByte, GameinposNackIs98) { EXPECT_EQ(mxh::server::userconn_gameinpos_nack, 98u); }
TEST(UserConnByte, RemaintimeNotifyIs99) { EXPECT_EQ(mxh::server::userconn_remaintime_notify, 99u); }
TEST(UserConnByte, BacktobeforemapTouserIs100) { EXPECT_EQ(mxh::server::userconn_backtobeforemap_touser, 100u); }
TEST(UserConnByte, BacktobeforemapSynIs101) { EXPECT_EQ(mxh::server::userconn_backtobeforemap_syn, 101u); }
TEST(UserConnByte, BacktobeforemapAckIs102) { EXPECT_EQ(mxh::server::userconn_backtobeforemap_ack, 102u); }
TEST(UserConnByte, BacktobeforemapNackIs103) { EXPECT_EQ(mxh::server::userconn_backtobeforemap_nack, 103u); }
TEST(UserConnByte, EnterGtournamentSynIs104) { EXPECT_EQ(mxh::server::userconn_enter_gtournament_syn, 104u); }
TEST(UserConnByte, CharacterslotIs105) { EXPECT_EQ(mxh::server::userconn_characterslot, 105u); }
TEST(UserConnByte, CastlegateAddIs106) { EXPECT_EQ(mxh::server::userconn_castlegate_add, 106u); }
TEST(UserConnByte, GameinOthermapSynIs107) { EXPECT_EQ(mxh::server::userconn_gamein_othermap_syn, 107u); }
TEST(UserConnByte, NowaitexitplayerIs108) { EXPECT_EQ(mxh::server::userconn_nowaitexitplayer, 108u); }
TEST(UserConnByte, FlagnpcOnoffIs109) { EXPECT_EQ(mxh::server::userconn_flagnpc_onoff, 109u); }
TEST(UserConnByte, LoginSynBuddyIs110) { EXPECT_EQ(mxh::server::userconn_login_syn_buddy, 110u); }
TEST(UserConnByte, ChangemapChannelinfoSynIs111) { EXPECT_EQ(mxh::server::userconn_changemap_channelinfo_syn, 111u); }
TEST(UserConnByte, ChangemapChannelinfoAckIs112) { EXPECT_EQ(mxh::server::userconn_changemap_channelinfo_ack, 112u); }
TEST(UserConnByte, ChangemapChannelinfoNackIs113) { EXPECT_EQ(mxh::server::userconn_changemap_channelinfo_nack, 113u); }
TEST(UserConnByte, CurrentmapChannelinfoIs114) { EXPECT_EQ(mxh::server::userconn_currentmap_channelinfo, 114u); }

}  // namespace