#include "mxh/server/agent_party.hpp"
namespace mxh::server {
// MP_PARTYUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 2034-2068.
PartyUserAction classify_party_user(const PartyUserRequest& r){
    if(r.protocol==party_master_to_request_syn){
        return r.master_resolved?PartyUserAction{PartyUserActionKind::forward_to_map,party_master_to_request_syn,r.master_object_id,0}:PartyUserAction{PartyUserActionKind::send_to_map_with_not_master_error,party_error,r.object_id,party_err_request_not_master};
    }
    return {PartyUserActionKind::forward_to_map,r.protocol,r.object_id,0};
}
// MP_PARTYServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 1886-2033.
PartyServerAction classify_party_server(const PartyServerRequest& r){
    switch(r.protocol){
    case party_notify_add_to_mapserver:case party_notify_delete_to_mapserver:case party_notify_changemaster_to_mapserver:case party_notify_breakup_to_mapserver:case party_notify_ban_to_mapserver:case party_notify_member_login_to_mapserver:case party_notify_member_logout_to_mapserver:case party_notify_member_loginmsg:case party_notify_create_to_mapserver:case party_notify_member_level:return {PartyServerActionKind::broadcast_to_other_maps,r.protocol,0,0};
    case party_request_consent_ack:return {PartyServerActionKind::forward_to_object_map,r.protocol,r.object_id,r.object_id2};
    case party_request_refusal_ack:return {PartyServerActionKind::forward_to_object_user,r.protocol,r.object_id,r.object_id2};
    case party_request_consent_nack:return {PartyServerActionKind::send_consent_nack,r.protocol,r.object_id2,0};
    case party_request_refusal_nack:return {PartyServerActionKind::send_refusal_nack,r.protocol,r.object_id2,0};
    case party_error:return {PartyServerActionKind::forward_to_object_user,r.protocol,r.object_id,0};
    default:return {PartyServerActionKind::default_to_client,r.protocol,0,0};
    }
}
}
[[maybe_unused]] constexpr int agent_party_translation_unit_anchor=0;
