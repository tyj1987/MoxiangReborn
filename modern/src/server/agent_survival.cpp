#include "mxh/server/agent_survival.hpp"
namespace mxh::server {
// MP_SURVIVALUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5094-5105.
SurvivalUserAction classify_survival_user(const SurvivalUserRequest& r){
    if(!r.user_found){return {SurvivalUserActionKind::default_forward_to_map,r.protocol,r.object_id,0,0,0};}
    if(r.protocol==survival_leave_syn){return {SurvivalUserActionKind::send_leave_syn_to_map,survival_leave_syn,r.object_id,r.unique_connect_idx,r.user_level,r.channel};}
    if(r.protocol==survival_ready_syn||r.protocol==survival_stop_syn||r.protocol==survival_mapoff_syn||r.protocol==survival_itemusingcount_set){return {SurvivalUserActionKind::gm_protected_forward_to_map,r.protocol,r.object_id,0,0,0};}
    return {SurvivalUserActionKind::default_forward_to_map,r.protocol,r.object_id,0,0,0};
}
// MP_SURVIVALServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5105-5158.
SurvivalServerAction classify_survival_server(const SurvivalServerRequest& r){
    if(r.protocol==survival_returntomap&&r.user_found&&r.target_map_port_found){return {SurvivalServerActionKind::update_user_map_and_forward_to_client,survival_returntomap,r.object_id,r.target_map,true};}
    if(r.protocol==survival_returntomap){return {SurvivalServerActionKind::default_forward_to_client,survival_returntomap,r.object_id,0,false};}
    return {SurvivalServerActionKind::default_forward_to_client,r.protocol,r.object_id,0,false};
}
}
[[maybe_unused]] constexpr int agent_survival_translation_unit_anchor=0;
