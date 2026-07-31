// agent_constants_sweep_test.cpp - Phase 6.3 byte sweep across all agent_*
// sub-protocol byte invariants. Each TEST is enumerated via add_test.
#include <gtest/gtest.h>

#include "mxh/server/agent_autonote.hpp"
#include "mxh/server/agent_bobusang_user.hpp"
#include "mxh/server/agent_chat.hpp"
#include "mxh/server/agent_db_msg_parser.hpp"
#include "mxh/server/agent_fortwar.hpp"
#include "mxh/server/agent_friend.hpp"
#include "mxh/server/agent_guild.hpp"
#include "mxh/server/agent_guild_union.hpp"
#include "mxh/server/agent_hackcheck.hpp"
#include "mxh/server/agent_item.hpp"
#include "mxh/server/agent_itemlimit.hpp"
#include "mxh/server/agent_murimnet.hpp"
#include "mxh/server/agent_network_msg_parser.hpp"
#include "mxh/server/agent_note.hpp"
#include "mxh/server/agent_packed.hpp"
#include "mxh/server/agent_party.hpp"
#include "mxh/server/agent_siegewar.hpp"
#include "mxh/server/agent_siegewarprofit.hpp"
#include "mxh/server/agent_siegewar_server.hpp"
#include "mxh/server/agent_simple_auth_forward.hpp"
#include "mxh/server/agent_skill.hpp"
#include "mxh/server/agent_survival.hpp"
#include "mxh/server/agent_wanted.hpp"
#include "mxh/server/agent_weather.hpp"

TEST(Autonote, AutonoteCategoryIs75) { EXPECT_EQ(mxh::server::autonote_category, 75u); }
TEST(Autonote, AutonoteAsktoautoSynIs0) { EXPECT_EQ(mxh::server::autonote_asktoauto_syn, 0u); }
TEST(Autonote, AutonoteAsktoautoIs3) { EXPECT_EQ(mxh::server::autonote_asktoauto, 3u); }
TEST(Autonote, AutonoteAnswerSynIs4) { EXPECT_EQ(mxh::server::autonote_answer_syn, 4u); }
TEST(Autonote, AutonoteAnswerFailIs7) { EXPECT_EQ(mxh::server::autonote_answer_fail, 7u); }
TEST(Autonote, AutonoteNotautoIs10) { EXPECT_EQ(mxh::server::autonote_notauto, 10u); }
TEST(Autonote, AutonoteKillautoIs11) { EXPECT_EQ(mxh::server::autonote_killauto, 11u); }
TEST(Autonote, AutonoteListAddIs13) { EXPECT_EQ(mxh::server::autonote_list_add, 13u); }
TEST(Autonote, AutonotePunishIs15) { EXPECT_EQ(mxh::server::autonote_punish, 15u); }
TEST(Autonote, AutonoteAsktoautoImageIs16) { EXPECT_EQ(mxh::server::autonote_asktoauto_image, 16u); }
TEST(Autonote, UserLevelGmIs8) { EXPECT_EQ(mxh::server::user_level_gm, 8u); }
TEST(Bobusang_user, BobusangUserCategoryIs73) { EXPECT_EQ(mxh::server::bobusang_user_category, 73u); }
TEST(Chat, ChatAllIs0) { EXPECT_EQ(mxh::server::chat_all, 0u); }
TEST(Fortwar, FortwarCategoryIs76) { EXPECT_EQ(mxh::server::fortwar_category, 76u); }
TEST(Fortwar, FortwarInfoIs0) { EXPECT_EQ(mxh::server::fortwar_info, 0u); }
TEST(Fortwar, FortwarStartBefore10minIs1) { EXPECT_EQ(mxh::server::fortwar_start_before10min, 1u); }
TEST(Fortwar, FortwarStartBefore10minToMapIs5) { EXPECT_EQ(mxh::server::fortwar_start_before10min_to_map, 5u); }
TEST(Friend, FriendCategoryIs33) { EXPECT_EQ(mxh::server::friend_category, 33u); }
TEST(Friend, FriendAddSynIs0) { EXPECT_EQ(mxh::server::friend_add_syn, 0u); }
TEST(Friend, FriendAddInviteIs3) { EXPECT_EQ(mxh::server::friend_add_invite, 3u); }
TEST(Friend, FriendAddAcceptIs4) { EXPECT_EQ(mxh::server::friend_add_accept, 4u); }
TEST(Friend, FriendAddDenyIs7) { EXPECT_EQ(mxh::server::friend_add_deny, 7u); }
TEST(Friend, FriendDelSynIs8) { EXPECT_EQ(mxh::server::friend_del_syn, 8u); }
TEST(Friend, FriendDelidSynIs11) { EXPECT_EQ(mxh::server::friend_delid_syn, 11u); }
TEST(Friend, FriendShowListSynIs13) { EXPECT_EQ(mxh::server::friend_show_list_syn, 13u); }
TEST(Friend, FriendLoginIs16) { EXPECT_EQ(mxh::server::friend_login, 16u); }
TEST(Friend, FriendLogoutNotifyIs19) { EXPECT_EQ(mxh::server::friend_logout_notify, 19u); }
TEST(Friend, FriendLogoutNotifyToAgentIs20) { EXPECT_EQ(mxh::server::friend_logout_notify_to_agent, 20u); }
TEST(Friend, FriendLogoutNotifyToClientIs21) { EXPECT_EQ(mxh::server::friend_logout_notify_to_client, 21u); }
TEST(Friend, FriendLogoutNotifyAgentToAgentIs22) { EXPECT_EQ(mxh::server::friend_logout_notify_agent_to_agent, 22u); }
TEST(Friend, FriendAddidSynIs23) { EXPECT_EQ(mxh::server::friend_addid_syn, 23u); }
TEST(Friend, FriendListSynIs26) { EXPECT_EQ(mxh::server::friend_list_syn, 26u); }
TEST(Friend, FriendAddAcceptToAgentIs29) { EXPECT_EQ(mxh::server::friend_add_accept_to_agent, 29u); }
TEST(Friend, FriendLoginNotifyToAgentIs30) { EXPECT_EQ(mxh::server::friend_login_notify_to_agent, 30u); }
TEST(Friend, FriendAddInviteToAgentIs31) { EXPECT_EQ(mxh::server::friend_add_invite_to_agent, 31u); }
TEST(Friend, FriendAddAckToAgentIs32) { EXPECT_EQ(mxh::server::friend_add_ack_to_agent, 32u); }
TEST(Friend, FriendAddNackToAgentIs33) { EXPECT_EQ(mxh::server::friend_add_nack_to_agent, 33u); }
TEST(Friend, FriendAddAcceptNackToAgentIs34) { EXPECT_EQ(mxh::server::friend_add_accept_nack_to_agent, 34u); }
TEST(Guild, GuildCategoryIs63) { EXPECT_EQ(mxh::server::guild_category, 63u); }
TEST(Guild, GuildCreateSynIs2) { EXPECT_EQ(mxh::server::guild_create_syn, 2u); }
TEST(Guild, GuildGivenicknameNackIs21) { EXPECT_EQ(mxh::server::guild_givenickname_nack, 21u); }
TEST(Guild, GuildErrCreateNameIs4) { EXPECT_EQ(mxh::server::guild_err_create_name, 4u); }
TEST(Guild_union, GuildUnionCategoryIs61) { EXPECT_EQ(mxh::server::guild_union_category, 61u); }
TEST(Guild_union, GuildUnionCreateSynIs2) { EXPECT_EQ(mxh::server::guild_union_create_syn, 2u); }
TEST(Hackcheck, HackcheckCategoryIs41) { EXPECT_EQ(mxh::server::hackcheck_category, 41u); }
TEST(Hackcheck, HackcheckSpeedhackIs0) { EXPECT_EQ(mxh::server::hackcheck_speedhack, 0u); }
TEST(Item, ItemCategoryIs5) { EXPECT_EQ(mxh::server::item_category, 5u); }
TEST(Item, ItemShopitemChangemapSynIs116) { EXPECT_EQ(mxh::server::item_shopitem_changemap_syn, 116u); }
TEST(Item, ItemShopitemChaseSynIs154) { EXPECT_EQ(mxh::server::item_shopitem_chase_syn, 154u); }
TEST(Item, ItemShopitemNchangeSynIs161) { EXPECT_EQ(mxh::server::item_shopitem_nchange_syn, 161u); }
TEST(Item, ItemShopitemShoutAckIs189) { EXPECT_EQ(mxh::server::item_shopitem_shout_ack, 189u); }
TEST(Item, ItemShopitemShoutSendserverIs191) { EXPECT_EQ(mxh::server::item_shopitem_shout_sendserver, 191u); }
TEST(Itemlimit, ItemlimitCategoryIs74) { EXPECT_EQ(mxh::server::itemlimit_category, 74u); }
TEST(Itemlimit, ItemlimitAddcountToMapIs0) { EXPECT_EQ(mxh::server::itemlimit_addcount_to_map, 0u); }
TEST(Itemlimit, ItemlimitFullToClientIs1) { EXPECT_EQ(mxh::server::itemlimit_full_to_client, 1u); }
TEST(Murimnet, MurimnetCategoryIs38) { EXPECT_EQ(mxh::server::murimnet_category, 38u); }
TEST(Murimnet, MurimnetChangetomurimnetSynIs0) { EXPECT_EQ(mxh::server::murimnet_changetomurimnet_syn, 0u); }
TEST(Murimnet, MurimnetChangetomurimnetAckIs1) { EXPECT_EQ(mxh::server::murimnet_changetomurimnet_ack, 1u); }
TEST(Note, NoteCategoryIs58) { EXPECT_EQ(mxh::server::note_category, 58u); }
TEST(Note, NoteSendnoteSynIs0) { EXPECT_EQ(mxh::server::note_sendnote_syn, 0u); }
TEST(Note, NoteSendnoteidSynIs3) { EXPECT_EQ(mxh::server::note_sendnoteid_syn, 3u); }
TEST(Note, NoteReceivenoteIs4) { EXPECT_EQ(mxh::server::note_receivenote, 4u); }
TEST(Note, NoteDelnoteSynIs5) { EXPECT_EQ(mxh::server::note_delnote_syn, 5u); }
TEST(Note, NoteDelallnoteSynIs8) { EXPECT_EQ(mxh::server::note_delallnote_syn, 8u); }
TEST(Note, NoteNotelistSynIs11) { EXPECT_EQ(mxh::server::note_notelist_syn, 11u); }
TEST(Note, NoteReadnoteSynIs14) { EXPECT_EQ(mxh::server::note_readnote_syn, 14u); }
TEST(Note, NoteNewNoteIs17) { EXPECT_EQ(mxh::server::note_new_note, 17u); }
TEST(Packed, PackedCategoryIs13) { EXPECT_EQ(mxh::server::packed_category, 13u); }
TEST(Packed, PackedNormalIs0) { EXPECT_EQ(mxh::server::packed_normal, 0u); }
TEST(Packed, PackedToMapserverIs1) { EXPECT_EQ(mxh::server::packed_to_mapserver, 1u); }
TEST(Packed, PackedToBroadMapserverIs2) { EXPECT_EQ(mxh::server::packed_to_broad_mapserver, 2u); }
TEST(Party, PartyCategoryIs14) { EXPECT_EQ(mxh::server::party_category, 14u); }
TEST(Party, PartyInfoIs0) { EXPECT_EQ(mxh::server::party_info, 0u); }
TEST(Party, PartyCreateSynIs1) { EXPECT_EQ(mxh::server::party_create_syn, 1u); }
TEST(Party, PartyAddSynIs4) { EXPECT_EQ(mxh::server::party_add_syn, 4u); }
TEST(Party, PartyAddInviteIs7) { EXPECT_EQ(mxh::server::party_add_invite, 7u); }
TEST(Party, PartyInviteAcceptSynIs8) { EXPECT_EQ(mxh::server::party_invite_accept_syn, 8u); }
TEST(Party, PartyInviteDenySynIs11) { EXPECT_EQ(mxh::server::party_invite_deny_syn, 11u); }
TEST(Party, PartyNotifyAddToMapserverIs14) { EXPECT_EQ(mxh::server::party_notify_add_to_mapserver, 14u); }
TEST(Party, PartyDelSynIs15) { EXPECT_EQ(mxh::server::party_del_syn, 15u); }
TEST(Party, PartyNotifyDeleteToMapserverIs18) { EXPECT_EQ(mxh::server::party_notify_delete_to_mapserver, 18u); }
TEST(Party, PartyBanSynIs20) { EXPECT_EQ(mxh::server::party_ban_syn, 20u); }
TEST(Party, PartyNotifyBanToMapserverIs23) { EXPECT_EQ(mxh::server::party_notify_ban_to_mapserver, 23u); }
TEST(Party, PartyChangemasterSynIs24) { EXPECT_EQ(mxh::server::party_changemaster_syn, 24u); }
TEST(Party, PartyNotifyChangemasterToMapserverIs27) { EXPECT_EQ(mxh::server::party_notify_changemaster_to_mapserver, 27u); }
TEST(Party, PartyBreakupSynIs28) { EXPECT_EQ(mxh::server::party_breakup_syn, 28u); }
TEST(Party, PartyNotifyBreakupToMapserverIs31) { EXPECT_EQ(mxh::server::party_notify_breakup_to_mapserver, 31u); }
TEST(Party, PartyMemberLoginIs32) { EXPECT_EQ(mxh::server::party_member_login, 32u); }
TEST(Party, PartyNotifyMemberLoginToMapserverIs33) { EXPECT_EQ(mxh::server::party_notify_member_login_to_mapserver, 33u); }
TEST(Party, PartyMemberLogoutIs34) { EXPECT_EQ(mxh::server::party_member_logout, 34u); }
TEST(Party, PartyNotifyMemberLogoutToMapserverIs35) { EXPECT_EQ(mxh::server::party_notify_member_logout_to_mapserver, 35u); }
TEST(Party, PartyMemberlifeIs36) { EXPECT_EQ(mxh::server::party_memberlife, 36u); }
TEST(Party, PartyMemberlevelIs39) { EXPECT_EQ(mxh::server::party_memberlevel, 39u); }
TEST(Party, PartyNotifyChangesToMapserverIs42) { EXPECT_EQ(mxh::server::party_notify_changes_to_mapserver, 42u); }
TEST(Party, PartyClearIs43) { EXPECT_EQ(mxh::server::party_clear, 43u); }
TEST(Party, PartyNotifyCreateToMapserverIs44) { EXPECT_EQ(mxh::server::party_notify_create_to_mapserver, 44u); }
TEST(Party, PartyMemberLoginmsgIs45) { EXPECT_EQ(mxh::server::party_member_loginmsg, 45u); }
TEST(Party, PartyNotifyMemberLoginmsgIs46) { EXPECT_EQ(mxh::server::party_notify_member_loginmsg, 46u); }
TEST(Party, PartyNotifyMemberLevelIs47) { EXPECT_EQ(mxh::server::party_notify_member_level, 47u); }
TEST(Party, PartyMonsterObtainNotifyIs48) { EXPECT_EQ(mxh::server::party_monster_obtain_notify, 48u); }
TEST(Party, PartyErrorIs50) { EXPECT_EQ(mxh::server::party_error, 50u); }
TEST(Party, PartyMasterToRequestSynIs51) { EXPECT_EQ(mxh::server::party_master_to_request_syn, 51u); }
TEST(Party, PartyRequestConsentSynIs53) { EXPECT_EQ(mxh::server::party_request_consent_syn, 53u); }
TEST(Party, PartyRequestRefusalSynIs56) { EXPECT_EQ(mxh::server::party_request_refusal_syn, 56u); }
TEST(Party, PartyNotifyInfoIs59) { EXPECT_EQ(mxh::server::party_notify_info, 59u); }
TEST(Siegewar, SiegewarCategoryIs62) { EXPECT_EQ(mxh::server::siegewar_category, 62u); }
TEST(Siegewar, SiegewarMoveinSynIs1) { EXPECT_EQ(mxh::server::siegewar_movein_syn, 1u); }
TEST(Siegewar, SiegewarBattlejoinSynIs7) { EXPECT_EQ(mxh::server::siegewar_battlejoin_syn, 7u); }
TEST(Siegewar, SiegewarLeaveSynIs12) { EXPECT_EQ(mxh::server::siegewar_leave_syn, 12u); }
TEST(Siegewar, SiegewarCheatIs61) { EXPECT_EQ(mxh::server::siegewar_cheat, 61u); }
TEST(Siegewarprofit, SiegewarprofitCategoryIs63) { EXPECT_EQ(mxh::server::siegewarprofit_category, 63u); }
TEST(Siegewarprofit, SiegewarprofitChangeTexrateNotifyToMapIs7) { EXPECT_EQ(mxh::server::siegewarprofit_change_texrate_notify_to_map, 7u); }
TEST(Siegewarprofit, SiegewarprofitChangeGuildNotifyToMapIs11) { EXPECT_EQ(mxh::server::siegewarprofit_change_guild_notify_to_map, 11u); }
TEST(Siegewar_server, SiegewarTaxrateIs60) { EXPECT_EQ(mxh::server::siegewar_taxrate, 60u); }
TEST(Siegewar_server, SiegewarReturntomapIs50) { EXPECT_EQ(mxh::server::siegewar_returntomap, 50u); }
TEST(Siegewar_server, SiegewarFlagchangeIs62) { EXPECT_EQ(mxh::server::siegewar_flagchange, 62u); }
TEST(Simple_auth_forward, StreetstallCategoryIs29) { EXPECT_EQ(mxh::server::streetstall_category, 29u); }
TEST(Simple_auth_forward, ExchangeCategoryIs28) { EXPECT_EQ(mxh::server::exchange_category, 28u); }
TEST(Survival, SurvivalCategoryIs70) { EXPECT_EQ(mxh::server::survival_category, 70u); }
TEST(Survival, SurvivalInfoIs0) { EXPECT_EQ(mxh::server::survival_info, 0u); }
TEST(Survival, SurvivalReadySynIs7) { EXPECT_EQ(mxh::server::survival_ready_syn, 7u); }
TEST(Wanted, WantedCategoryIs51) { EXPECT_EQ(mxh::server::wanted_category, 51u); }
TEST(Wanted, WantedNotifyDeleteToMapIs9) { EXPECT_EQ(mxh::server::wanted_notify_delete_to_map, 9u); }
TEST(Wanted, WantedNotifyNotcompleteToMapIs18) { EXPECT_EQ(mxh::server::wanted_notify_notcomplete_to_map, 18u); }
TEST(Wanted, WantedNotcompleteToAgentIs23) { EXPECT_EQ(mxh::server::wanted_notcomplete_to_agent, 23u); }
TEST(Weather, WeatherCategoryIs64) { EXPECT_EQ(mxh::server::weather_category, 64u); }
TEST(Weather, WeatherSetIs0) { EXPECT_EQ(mxh::server::weather_set, 0u); }
TEST(Weather, UserLevelGmWeatherIs8) { EXPECT_EQ(mxh::server::user_level_gm_weather, 8u); }