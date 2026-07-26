#include "mxh/server/agent_friend.hpp"
namespace mxh::server {
// MP_FRIENDMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 2086-2137.
FriendAction classify_friend(const FriendRequest& r){
    if(!r.user_found){return {FriendActionKind::drop_no_user,r.protocol,r.object_id,0};}
    switch(r.protocol){
    case friend_login:return {FriendActionKind::db_notify_login,friend_login,r.object_id,0};
    case friend_add_syn:
        if(r.from_invalid_char){return {FriendActionKind::drop_with_invalid_filter,friend_add_syn,r.object_id,0};}
        return {FriendActionKind::db_add_syn,friend_add_syn,r.object_id,0};
    case friend_add_accept:return {FriendActionKind::db_add_syn,friend_add_accept,r.object_id,0};
    case friend_add_deny:return {FriendActionKind::send_add_denied_nack_to_user,friend_add_nack,r.object_id,friend_err_add_deny};
    case friend_del_syn:return {FriendActionKind::db_del_syn,friend_del_syn,r.object_id,0};
    case friend_delid_syn:return {FriendActionKind::db_del_id_syn,friend_delid_syn,r.object_id,0};
    case friend_addid_syn:return {FriendActionKind::db_add_id_syn_validate,friend_addid_syn,r.object_id,0};
    case friend_logout_notify_to_agent:return {FriendActionKind::send_logout_to_client_if_user_or_broadcast_to_agents,friend_logout_notify_to_client,r.object_id,0};
    case friend_logout_notify_agent_to_agent:return {FriendActionKind::send_logout_to_client_if_user,friend_logout_notify_to_client,r.object_id,0};
    case friend_list_syn:return {FriendActionKind::db_get_list,friend_list_syn,r.object_id,0};
    case friend_add_ack_to_agent:return {FriendActionKind::send_to_user,friend_add_ack,r.object_id,0};
    case friend_add_nack_to_agent:return {FriendActionKind::send_to_user,friend_add_nack,r.object_id,0};
    case friend_add_accept_to_agent:return {FriendActionKind::send_to_user,friend_add_accept_ack,r.object_id,0};
    case friend_add_accept_nack_to_agent:return {FriendActionKind::send_to_user,friend_add_accept_nack,r.object_id,0};
    case friend_login_notify_to_agent:return {FriendActionKind::send_to_user_login_notify,friend_login_notify,r.object_id,0};
    case friend_add_invite_to_agent:return r.no_friend_option?FriendAction{FriendActionKind::send_invite_to_user_or_broadcast_nack,friend_add_nack,r.object_id,friend_err_option_no_friend}:FriendAction{FriendActionKind::send_invite_to_user_or_broadcast_nack,friend_add_invite,r.object_id,0};
    case friend_add_nack:return {FriendActionKind::send_to_user,friend_add_nack,r.object_id,0};
    default:return {FriendActionKind::forward_to_client,r.protocol,r.object_id,0};
    }
}
}
[[maybe_unused]] constexpr int agent_friend_translation_unit_anchor=0;
