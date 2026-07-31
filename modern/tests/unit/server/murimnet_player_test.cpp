#include "mxh/server/murimnet_player.hpp"
#include "mxh/server/murimnet_play_room_manager.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
static MurimNetPlayerInfo player(std::uint32_t id){MurimNetPlayerInfo i;i.player_id=id;i.agent_num=3;i.unique_id_in_agent=44;i.back_map_num=7;i.name="hero";i.level=25;return i;}
TEST(MurimNetPlayer, InitStoresLegacyFields){MurimNetPlayer p;ASSERT_TRUE(p.init(player(10)));EXPECT_EQ(p.id(),10u);EXPECT_EQ(p.info().agent_num,3u);EXPECT_EQ(p.info().unique_id_in_agent,44u);EXPECT_EQ(p.info().back_map_num,7u);EXPECT_EQ(p.info().name,"hero");EXPECT_EQ(p.info().level,25u);EXPECT_EQ(p.room_state().id,10u);}
TEST(MurimNetPlayer, RejectsZeroObjectId){MurimNetPlayer p;EXPECT_FALSE(p.init(player(0)));}
TEST(MurimNetPlayerManager, CapacityDuplicateFindDelete){MurimNetPlayerManager m;ASSERT_TRUE(m.init(2));auto* a=m.add_player(player(1));ASSERT_NE(a,nullptr);EXPECT_EQ(m.add_player(player(1)),nullptr);EXPECT_NE(m.add_player(player(2)),nullptr);EXPECT_EQ(m.add_player(player(3)),nullptr);EXPECT_EQ(m.find_player(1),a);EXPECT_TRUE(m.delete_player(1));EXPECT_EQ(m.find_player(1),nullptr);EXPECT_FALSE(m.delete_player(1));}
TEST(MurimNetPlayerManager, ReleaseResetsManager){MurimNetPlayerManager m;ASSERT_TRUE(m.init(2));ASSERT_NE(m.add_player(player(1)),nullptr);m.release();EXPECT_EQ(m.player_count(),0u);EXPECT_EQ(m.max_players(),0u);EXPECT_EQ(m.add_player(player(2)),nullptr);}
TEST(MurimNetPlayerManager, RoomStateIntegratesWithPlayRoomManager){MurimNetPlayerManager players;ASSERT_TRUE(players.init(2));auto* p=players.add_player(player(1));ASSERT_NE(p,nullptr);MurimNetPlayRoomManager rooms;ASSERT_TRUE(rooms.init(2));ASSERT_TRUE(rooms.enter_room(p->room_state(),1));EXPECT_EQ(p->room_state().location,MnPlayerLocation::PlayRoom);EXPECT_EQ(p->room_state().location_index,1u);EXPECT_TRUE(rooms.exit_room(p->room_state()));}
