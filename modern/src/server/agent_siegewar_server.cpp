#include "mxh/server/agent_siegewar_server.hpp"
namespace mxh::server {
// MP_SIEGEWARServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4787-4922.
SiegeWarServerAction classify_siegewar_server(const SiegeWarServerRequest& r){
    switch(r.protocol){
    case siegewar_taxrate:return {SiegeWarServerActionKind::broadcast_taxrate_to_affected_maps,siegewar_taxrate,0,0};
    case siegewar_returntomap:
        if(!r.user_found){return {SiegeWarServerActionKind::drop_no_user,siegewar_returntomap,r.object_id,0};}
        return r.target_map_found?SiegeWarServerAction{SiegeWarServerActionKind::update_user_map_and_forward_to_client,siegewar_returntomap,r.object_id,r.target_map}:SiegeWarServerAction{SiegeWarServerActionKind::default_forward_to_client,siegewar_returntomap,r.object_id,0};
    case siegewar_flagchange:return {SiegeWarServerActionKind::broadcast_to_all_users,siegewar_flagchange,0,0};
    default:return {SiegeWarServerActionKind::default_forward_to_client,r.protocol,0,0};
    }
}
}
[[maybe_unused]] constexpr int agent_siegewar_server_translation_unit_anchor=0;
