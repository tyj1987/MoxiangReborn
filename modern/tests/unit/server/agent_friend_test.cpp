#include "mxh/server/agent_friend.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(Friend, NoUserAlwaysDrops){FriendRequest r;r.protocol=friend_login;r.user_found=false;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::drop_no_user);}
TEST(Friend, LoginNotifiesLoginAndNotes){FriendRequest r;r.protocol=friend_login;r.user_found=true;auto a=classify_friend(r);EXPECT_EQ(a.kind,FriendActionKind::db_notify_login);EXPECT_EQ(a.protocol,friend_login);}
TEST(Friend, AddSynNoFilterCallsAddFriend){FriendRequest r;r.protocol=friend_add_syn;r.user_found=true;r.from_invalid_char=false;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::db_add_syn);}
TEST(Friend, AddSynInvalidCharDrops){FriendRequest r;r.protocol=friend_add_syn;r.user_found=true;r.from_invalid_char=true;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::drop_with_invalid_filter);}
TEST(Friend, AddAcceptCommitsAdd){FriendRequest r;r.protocol=friend_add_accept;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::db_add_syn);}
TEST(Friend, AddDenySendsNackWithAddDenyCode){FriendRequest r;r.protocol=friend_add_deny;r.user_found=true;auto a=classify_friend(r);EXPECT_EQ(a.kind,FriendActionKind::send_add_denied_nack_to_user);EXPECT_EQ(a.protocol,friend_add_nack);EXPECT_EQ(a.error_code,friend_err_add_deny);}
TEST(Friend, DelSynDeletesFriend){FriendRequest r;r.protocol=friend_del_syn;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::db_del_syn);}
TEST(Friend, DelIdSynDeletesById){FriendRequest r;r.protocol=friend_delid_syn;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::db_del_id_syn);}
TEST(Friend, AddIdSynValidatesTarget){FriendRequest r;r.protocol=friend_addid_syn;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::db_add_id_syn_validate);}
TEST(Friend, LogoutNotifyToAgentRoutesToClientOrAgents){FriendRequest r;r.protocol=friend_logout_notify_to_agent;auto a=classify_friend(r);EXPECT_EQ(a.kind,FriendActionKind::send_logout_to_client_if_user_or_broadcast_to_agents);EXPECT_EQ(a.protocol,friend_logout_notify_to_client);}
TEST(Friend, LogoutNotifyAgentToAgentForwardsClientOnly){FriendRequest r;r.protocol=friend_logout_notify_agent_to_agent;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::send_logout_to_client_if_user);}
TEST(Friend, ListSynFetchesFriendList){FriendRequest r;r.protocol=friend_list_syn;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::db_get_list);}
TEST(Friend, AddAckToAgentSendsToUser){FriendRequest r;r.protocol=friend_add_ack_to_agent;auto a=classify_friend(r);EXPECT_EQ(a.kind,FriendActionKind::send_to_user);EXPECT_EQ(a.protocol,friend_add_ack);}
TEST(Friend, AddNackToAgentSendsToUser){FriendRequest r;r.protocol=friend_add_nack_to_agent;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::send_to_user);}
TEST(Friend, AddInviteNoFriendOptionSendsNoFriendNack){FriendRequest r;r.protocol=friend_add_invite_to_agent;r.no_friend_option=true;auto a=classify_friend(r);EXPECT_EQ(a.kind,FriendActionKind::send_invite_to_user_or_broadcast_nack);EXPECT_EQ(a.protocol,friend_add_nack);EXPECT_EQ(a.error_code,friend_err_option_no_friend);}
TEST(Friend, AddInviteNormalSendsInvite){FriendRequest r;r.protocol=friend_add_invite_to_agent;r.no_friend_option=false;auto a=classify_friend(r);EXPECT_EQ(a.kind,FriendActionKind::send_invite_to_user_or_broadcast_nack);EXPECT_EQ(a.protocol,friend_add_invite);}
TEST(Friend, LoginNotifyToAgentSendsLoginNotifyToUser){FriendRequest r;r.protocol=friend_login_notify_to_agent;auto a=classify_friend(r);EXPECT_EQ(a.protocol,friend_login_notify);EXPECT_EQ(a.kind,FriendActionKind::send_to_user_login_notify);}
TEST(Friend, AddNackSendsToUser){FriendRequest r;r.protocol=friend_add_nack;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::send_to_user);}
TEST(Friend, UnknownProtocolForwardsToClient){FriendRequest r;r.protocol=99;EXPECT_EQ(classify_friend(r).kind,FriendActionKind::forward_to_client);}