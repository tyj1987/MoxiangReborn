// cheat_byte_sweep_test.cpp - Phase 6.3 byte sweep across MP_CHEAT
// sub-protocol byte invariants. Each TEST is enumerated via add_test.

#include "mxh/server/agent_cheat.hpp"
#include "mxh/server/agent_gtournament_server.hpp"
#include "mxh/server/agent_powerup.hpp"
#include "mxh/server/agent_userconn.hpp"

#include <gtest/gtest.h>

namespace {

TEST(CheatByte, GmLoginSynIs0) { EXPECT_EQ(mxh::server::cheat_gm_login_syn, 0u); }
TEST(CheatByte, ChangemapSynIs1) { EXPECT_EQ(mxh::server::cheat_changemap_syn, 1u); }
TEST(CheatByte, ChangemapNackIs2) { EXPECT_EQ(mxh::server::cheat_changemap_nack, 2u); }
TEST(CheatByte, ChangemapAckIs3) { EXPECT_EQ(mxh::server::cheat_changemap_ack, 3u); }
TEST(CheatByte, BancharacterSynIs4) { EXPECT_EQ(mxh::server::cheat_bancharacter_syn, 4u); }
TEST(CheatByte, BancharacterNackIs5) { EXPECT_EQ(mxh::server::cheat_bancharacter_nack, 5u); }
TEST(CheatByte, BlockcharacterSynIs6) { EXPECT_EQ(mxh::server::cheat_blockcharacter_syn, 6u); }
TEST(CheatByte, WhereisSynIs7) { EXPECT_EQ(mxh::server::cheat_whereis_syn, 7u); }
TEST(CheatByte, EventMonsterRegenIs8) { EXPECT_EQ(mxh::server::cheat_event_monster_regen, 8u); }
TEST(CheatByte, EventMonsterDeleteIs9) { EXPECT_EQ(mxh::server::cheat_event_monster_delete, 9u); }
TEST(CheatByte, BanmapSynIs10) { EXPECT_EQ(mxh::server::cheat_banmap_syn, 10u); }
TEST(CheatByte, AgentcheckSynIs11) { EXPECT_EQ(mxh::server::cheat_agentcheck_syn, 11u); }
TEST(CheatByte, PkallowSynIs12) { EXPECT_EQ(mxh::server::cheat_pkallow_syn, 12u); }
TEST(CheatByte, NoticeSynIs13) { EXPECT_EQ(mxh::server::cheat_notice_syn, 13u); }
TEST(CheatByte, AbilityexpSynIs14) { EXPECT_EQ(mxh::server::cheat_abilityexp_syn, 14u); }
TEST(CheatByte, AddmugongSynIs15) { EXPECT_EQ(mxh::server::cheat_addmugong_syn, 15u); }
TEST(CheatByte, MugongsungSynIs16) { EXPECT_EQ(mxh::server::cheat_mugongsung_syn, 16u); }
TEST(CheatByte, ItemSynIs17) { EXPECT_EQ(mxh::server::cheat_item_syn, 17u); }
TEST(CheatByte, ItemOptionSynIs18) { EXPECT_EQ(mxh::server::cheat_item_option_syn, 18u); }
TEST(CheatByte, MoneySynIs19) { EXPECT_EQ(mxh::server::cheat_money_syn, 19u); }
TEST(CheatByte, EventSynIs20) { EXPECT_EQ(mxh::server::cheat_event_syn, 20u); }
TEST(CheatByte, EventnotifyOnIs21) { EXPECT_EQ(mxh::server::cheat_eventnotify_on, 21u); }
TEST(CheatByte, PlustimeOnIs22) { EXPECT_EQ(mxh::server::cheat_plustime_on, 22u); }
TEST(CheatByte, EventnotifyOffIs23) { EXPECT_EQ(mxh::server::cheat_eventnotify_off, 23u); }
TEST(CheatByte, PlustimeAlloffIs24) { EXPECT_EQ(mxh::server::cheat_plustime_alloff, 24u); }
TEST(CheatByte, ChangeEventmapSynIs25) { EXPECT_EQ(mxh::server::cheat_change_eventmap_syn, 25u); }
TEST(CheatByte, EventStartSynIs26) { EXPECT_EQ(mxh::server::cheat_event_start_syn, 26u); }
TEST(CheatByte, EventReadySynIs27) { EXPECT_EQ(mxh::server::cheat_event_ready_syn, 27u); }
TEST(CheatByte, PetStaminaIs28) { EXPECT_EQ(mxh::server::cheat_pet_stamina, 28u); }
TEST(CheatByte, PetFriendshipSynIs29) { EXPECT_EQ(mxh::server::cheat_pet_friendship_syn, 29u); }
TEST(CheatByte, PetSelectedFriendshipSynIs30) { EXPECT_EQ(mxh::server::cheat_pet_selected_friendship_syn, 30u); }
TEST(CheatByte, GuildpointSynIs31) { EXPECT_EQ(mxh::server::cheat_guildpoint_syn, 31u); }
TEST(CheatByte, GuildhuntedMonstercountSynIs32) { EXPECT_EQ(mxh::server::cheat_guildhunted_monstercount_syn, 32u); }
TEST(CheatByte, MussangReadyIs33) { EXPECT_EQ(mxh::server::cheat_mussang_ready, 33u); }
TEST(CheatByte, JackpotGetprizeIs34) { EXPECT_EQ(mxh::server::cheat_jackpot_getprize, 34u); }
TEST(CheatByte, JackpotMoneypermonsterIs35) { EXPECT_EQ(mxh::server::cheat_jackpot_moneypermonster, 35u); }
TEST(CheatByte, JackpotOnoffIs36) { EXPECT_EQ(mxh::server::cheat_jackpot_onoff, 36u); }
TEST(CheatByte, JackpotProbabilityIs37) { EXPECT_EQ(mxh::server::cheat_jackpot_probability, 37u); }
TEST(CheatByte, JackpotControlIs38) { EXPECT_EQ(mxh::server::cheat_jackpot_control, 38u); }
TEST(CheatByte, BobusanginfoRequestSynIs39) { EXPECT_EQ(mxh::server::cheat_bobusanginfo_request_syn, 39u); }
TEST(CheatByte, BobusangLeaveSynIs40) { EXPECT_EQ(mxh::server::cheat_bobusang_leave_syn, 40u); }
TEST(CheatByte, BobusanginfoChangeSynIs41) { EXPECT_EQ(mxh::server::cheat_bobusanginfo_change_syn, 41u); }
TEST(CheatByte, ItemlimitSynIs42) { EXPECT_EQ(mxh::server::cheat_itemlimit_syn, 42u); }
TEST(CheatByte, AutonoteSettingSynIs43) { EXPECT_EQ(mxh::server::cheat_autonote_setting_syn, 43u); }
TEST(CheatByte, DamageSynIs44) { EXPECT_EQ(mxh::server::cheat_damage_syn, 44u); }
TEST(CheatByte, DamageAckIs45) { EXPECT_EQ(mxh::server::cheat_damage_ack, 45u); }
TEST(CheatByte, DamageNackIs46) { EXPECT_EQ(mxh::server::cheat_damage_nack, 46u); }
TEST(CheatByte, MapConditionIs47) { EXPECT_EQ(mxh::server::cheat_map_condition, 47u); }
TEST(CheatByte, AgentConditionIs48) { EXPECT_EQ(mxh::server::cheat_agent_condition, 48u); }
TEST(CheatByte, TitanFuelSpellMaxSynIs49) { EXPECT_EQ(mxh::server::cheat_titan_fuel_spell_max_syn, 49u); }
TEST(CheatByte, BancharacterAckIs50) { EXPECT_EQ(mxh::server::cheat_bancharacter_ack, 50u); }
TEST(CheatByte, WhereisAckIs52) { EXPECT_EQ(mxh::server::cheat_whereis_ack, 52u); }

TEST(PowerUpByte, BootingNotifyIs0) { EXPECT_EQ(mxh::server::powerup_booting_notify, 0u); }
TEST(PowerUpByte, BootlistSynIs1) { EXPECT_EQ(mxh::server::powerup_bootlist_syn, 1u); }
TEST(PowerUpByte, BootlistAckIs2) { EXPECT_EQ(mxh::server::powerup_bootlist_ack, 2u); }
TEST(PowerUpByte, ConnectSynIs3) { EXPECT_EQ(mxh::server::powerup_connect_syn, 3u); }
TEST(PowerUpByte, ConnectAckIs4) { EXPECT_EQ(mxh::server::powerup_connect_ack, 4u); }

TEST(GTournamentByte, MovetobattlemapSynIs0) { EXPECT_EQ(mxh::server::gtournament_movetobattlemap_syn, 0u); }
TEST(GTournamentByte, MovetobattlemapNackIs1) { EXPECT_EQ(mxh::server::gtournament_movetobattlemap_nack, 1u); }
TEST(GTournamentByte, StandinginfoSynIs2) { EXPECT_EQ(mxh::server::gtournament_standinginfo_syn, 2u); }
TEST(GTournamentByte, StandinginfoNackIs3) { EXPECT_EQ(mxh::server::gtournament_standinginfo_nack, 3u); }
TEST(GTournamentByte, BattlejoinSynIs4) { EXPECT_EQ(mxh::server::gtournament_battlejoin_syn, 4u); }
TEST(GTournamentByte, ObserverjoinSynIs5) { EXPECT_EQ(mxh::server::gtournament_observerjoin_syn, 5u); }
TEST(GTournamentByte, BattlejoinNackIs6) { EXPECT_EQ(mxh::server::gtournament_battlejoin_nack, 6u); }
TEST(GTournamentByte, LeaveSynIs7) { EXPECT_EQ(mxh::server::gtournament_leave_syn, 7u); }
TEST(GTournamentByte, CheatIs8) { EXPECT_EQ(mxh::server::gtournament_cheat, 8u); }
TEST(GTournamentByte, EventStartIs9) { EXPECT_EQ(mxh::server::gtournament_event_start, 9u); }
TEST(GTournamentByte, EventEndIs10) { EXPECT_EQ(mxh::server::gtournament_event_end, 10u); }
TEST(GTournamentByte, StandinginfoRegistedIs11) { EXPECT_EQ(mxh::server::gtournament_standinginfo_registed, 11u); }
TEST(GTournamentByte, ReturntomapIs12) { EXPECT_EQ(mxh::server::gtournament_returntomap, 12u); }
TEST(GTournamentByte, NotifyWinloseIs13) { EXPECT_EQ(mxh::server::gtournament_notify_winlose, 13u); }

TEST(CheatConstants, UserLevelGmIs1) { EXPECT_EQ(mxh::server::cheat_user_level_gm, 1u); }
TEST(CheatConstants, UserLevelProgrammerIs2) { EXPECT_EQ(mxh::server::cheat_user_level_programmer, 2u); }
TEST(CheatConstants, UserLevelDeveloperIs3) { EXPECT_EQ(mxh::server::cheat_user_level_developer, 3u); }

TEST(UserConnConstants, LoginErrNoAgentServerIs0) { EXPECT_EQ(mxh::server::userconn_login_err_no_agent_server, 0u); }
TEST(UserConnConstants, LoginErrDistAlreadyoutIs1) { EXPECT_EQ(mxh::server::userconn_login_err_dist_alreadyout, 1u); }

TEST(CategoryInvariant, UserConnIs7) { EXPECT_EQ(mxh::server::userconn_category, 7u); }
TEST(CategoryInvariant, PowerUpIs2) { EXPECT_EQ(mxh::server::powerup_category, 2u); }
TEST(CategoryInvariant, CheatIs11) { EXPECT_EQ(mxh::server::cheat_category, 11u); }
TEST(CategoryInvariant, GTournamentIs59) { EXPECT_EQ(mxh::server::gtoournament_category, 59u); }
TEST(MapConstants, GTournamentMapNumIs60) { EXPECT_EQ(mxh::server::gtournament_map_num, 60u); }
TEST(PowerUpConstants, MaxAgentServersAtLeast50) { EXPECT_GE(mxh::server::powerup_max_agent_servers, 50u); }

}  // namespace