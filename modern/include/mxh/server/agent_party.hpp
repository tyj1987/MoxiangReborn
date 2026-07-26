#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_PARTY (1-based value from MP_SERVER=1, position 14).
inline constexpr std::uint8_t party_category=14;
// Sub-protocols within MP_PROTOCOL_PARTY (MP_PARTY_INFO=0 ... MP_PARTY_MATCHING_INFO=60).
// Values below are 1:1 with [CC]Header/Protocol.h, offset from MP_PARTY_INFO=0.
inline constexpr std::uint8_t party_info=0;
inline constexpr std::uint8_t party_create_syn=1,party_create_ack=2,party_create_nack=3;
inline constexpr std::uint8_t party_add_syn=4,party_add_ack=5,party_add_nack=6;
inline constexpr std::uint8_t party_add_invite=7;
inline constexpr std::uint8_t party_invite_accept_syn=8,party_invite_accept_ack=9,party_invite_accept_nack=10;
inline constexpr std::uint8_t party_invite_deny_syn=11,party_invite_deny_ack=12,party_invite_deny_nack=13;
inline constexpr std::uint8_t party_notify_add_to_mapserver=14;
inline constexpr std::uint8_t party_del_syn=15,party_del_ack=16,party_del_nack=17;
inline constexpr std::uint8_t party_notify_delete_to_mapserver=18,party_syndelete_to_mapserver=19;
inline constexpr std::uint8_t party_ban_syn=20,party_ban_ack=21,party_ban_nack=22;
inline constexpr std::uint8_t party_notify_ban_to_mapserver=23;
inline constexpr std::uint8_t party_changemaster_syn=24,party_changemaster_ack=25,party_changemaster_nack=26;
inline constexpr std::uint8_t party_notify_changemaster_to_mapserver=27;
inline constexpr std::uint8_t party_breakup_syn=28,party_breakup_ack=29,party_breakup_nack=30;
inline constexpr std::uint8_t party_notify_breakup_to_mapserver=31;
inline constexpr std::uint8_t party_member_login=32;
inline constexpr std::uint8_t party_notify_member_login_to_mapserver=33;
inline constexpr std::uint8_t party_member_logout=34;
inline constexpr std::uint8_t party_notify_member_logout_to_mapserver=35;
inline constexpr std::uint8_t party_memberlife=36,party_membershipield=37,party_membernaeryuk=38;
inline constexpr std::uint8_t party_memberlevel=39,party_sendpos=40,party_revivepos=41;
inline constexpr std::uint8_t party_notify_changes_to_mapserver=42;
inline constexpr std::uint8_t party_clear=43;
inline constexpr std::uint8_t party_notify_create_to_mapserver=44;
inline constexpr std::uint8_t party_member_loginmsg=45;
inline constexpr std::uint8_t party_notify_member_loginmsg=46;
inline constexpr std::uint8_t party_notify_member_level=47;
inline constexpr std::uint8_t party_monster_obtain_notify=48,party_obtain_money_to_party=49;
inline constexpr std::uint8_t party_error=50;
inline constexpr std::uint8_t party_master_to_request_syn=51,party_master_to_request_ack=52;
inline constexpr std::uint8_t party_request_consent_syn=53,party_request_consent_ack=54,party_request_consent_nack=55;
inline constexpr std::uint8_t party_request_refusal_syn=56,party_request_refusal_ack=57,party_request_refusal_nack=58;
inline constexpr std::uint8_t party_notify_info=59,party_matching_info=60;
// Legacy error code for master-to-request when target user is not the party master.
inline constexpr std::uint32_t party_err_request_not_master=1;
enum class PartyUserActionKind : std::uint8_t { forward_to_map, send_to_map_with_not_master_error };
enum class PartyServerActionKind : std::uint8_t { broadcast_to_other_maps, forward_to_object_map, forward_to_object_user, send_consent_nack, send_refusal_nack, default_to_client };
struct PartyUserRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t master_object_id=0; bool master_resolved=false; };
struct PartyServerRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t object_id2=0; };
struct PartyUserAction { PartyUserActionKind kind=PartyUserActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t error_code=0; };
struct PartyServerAction { PartyServerActionKind kind=PartyServerActionKind::default_to_client; std::uint8_t protocol=0; std::uint32_t target_object_id=0; std::uint32_t alternate_object_id=0; };
PartyUserAction classify_party_user(const PartyUserRequest&);
PartyServerAction classify_party_server(const PartyServerRequest&);
}