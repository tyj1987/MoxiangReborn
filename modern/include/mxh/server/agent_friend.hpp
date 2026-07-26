#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_FRIEND (MP_FRIEND=33 in MP_CATEGORY).
inline constexpr std::uint8_t friend_category=33;
// Sub-protocols within MP_PROTOCOL_FRIEND (offset 0..34).
inline constexpr std::uint8_t friend_add_syn=0,friend_add_ack=1,friend_add_nack=2;
inline constexpr std::uint8_t friend_add_invite=3;
inline constexpr std::uint8_t friend_add_accept=4,friend_add_accept_ack=5,friend_add_accept_nack=6;
inline constexpr std::uint8_t friend_add_deny=7;
inline constexpr std::uint8_t friend_del_syn=8,friend_del_ack=9,friend_del_nack=10;
inline constexpr std::uint8_t friend_delid_syn=11,friend_delid_ack=12;
inline constexpr std::uint8_t friend_show_list_syn=13,friend_show_list_ack=14,friend_show_list_nack=15;
inline constexpr std::uint8_t friend_login=16,friend_login_notify=17,friend_login_friend=18;
inline constexpr std::uint8_t friend_logout_notify=19;
inline constexpr std::uint8_t friend_logout_notify_to_agent=20;
inline constexpr std::uint8_t friend_logout_notify_to_client=21;
inline constexpr std::uint8_t friend_logout_notify_agent_to_agent=22;
inline constexpr std::uint8_t friend_addid_syn=23,friend_addid_ack=24,friend_addid_nack=25;
inline constexpr std::uint8_t friend_list_syn=26,friend_list_ack=27,friend_list_nack=28;
inline constexpr std::uint8_t friend_add_accept_to_agent=29;
inline constexpr std::uint8_t friend_login_notify_to_agent=30;
inline constexpr std::uint8_t friend_add_invite_to_agent=31;
inline constexpr std::uint8_t friend_add_ack_to_agent=32;
inline constexpr std::uint8_t friend_add_nack_to_agent=33;
inline constexpr std::uint8_t friend_add_accept_nack_to_agent=34;
// Legacy friend error codes embedded in dwData/dwObjectID by ADD_DENY/ADD_INVITE fanout.
inline constexpr std::uint32_t friend_err_add_deny=4;
inline constexpr std::uint32_t friend_err_option_no_friend=5;
enum class FriendActionKind : std::uint8_t { db_notify_login, db_add_syn, db_del_syn, db_del_id_syn, db_add_id_syn_validate, db_get_list, send_to_user, send_to_user_login_notify, send_invite_to_user_or_broadcast_nack, send_add_denied_nack_to_user, send_logout_to_client_if_user_or_broadcast_to_agents, send_logout_to_client_if_user, forward_to_client, drop_no_user, drop_with_invalid_filter };
struct FriendRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_found=true; bool no_friend_option=false; bool from_invalid_char=false; };
struct FriendAction { FriendActionKind kind=FriendActionKind::forward_to_client; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t error_code=0; };
FriendAction classify_friend(const FriendRequest&);
}
