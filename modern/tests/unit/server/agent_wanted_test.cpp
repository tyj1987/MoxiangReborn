#include "mxh/server/agent_wanted.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(Wanted, NotifyDeleteToMapBroadcasts){WantedRequest r;r.protocol=wanted_notify_delete_to_map;EXPECT_EQ(classify_wanted(r).kind,WantedServerActionKind::broadcast_to_other_maps);}
TEST(Wanted, NotifyRegistToMapBroadcasts){WantedRequest r;r.protocol=wanted_notify_regist_to_map;EXPECT_EQ(classify_wanted(r).kind,WantedServerActionKind::broadcast_to_other_maps);}
TEST(Wanted, NotifyNotcompleteToMapBroadcasts){WantedRequest r;r.protocol=wanted_notify_notcomplete_to_map;EXPECT_EQ(classify_wanted(r).kind,WantedServerActionKind::broadcast_to_other_maps);}
TEST(Wanted, DestroyedToMapBroadcasts){WantedRequest r;r.protocol=wanted_destroyed_to_map;EXPECT_EQ(classify_wanted(r).kind,WantedServerActionKind::broadcast_to_other_maps);}
TEST(Wanted, NotcompleteToAgentUserMissingDrops){WantedRequest r;r.protocol=wanted_notcomplete_to_agent;r.user_found=false;EXPECT_EQ(classify_wanted(r).kind,WantedServerActionKind::drop_no_user);}
TEST(Wanted, NotcompleteToAgentUserFoundCompletes){WantedRequest r;r.protocol=wanted_notcomplete_to_agent;r.user_found=true;auto a=classify_wanted(r);EXPECT_EQ(a.kind,WantedServerActionKind::complete_notcomplete_send_to_map);EXPECT_EQ(a.protocol,wanted_notcomplete_by_delchr);}
TEST(Wanted, DefaultForwardsToClient){WantedRequest r;r.protocol=99;EXPECT_EQ(classify_wanted(r).kind,WantedServerActionKind::default_forward_to_client);}