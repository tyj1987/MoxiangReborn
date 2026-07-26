#include "mxh/server/agent_siegewar.hpp"
namespace mxh::server {
// MP_SIEGEWARUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4694-4787.
SiegeWarUserAction classify_siegewar_user(const SiegeWarUserRequest& r){
    if(r.protocol==siegewar_cheat){return {SiegeWarUserActionKind::cheat_fanout_to_map_servers,siegewar_cheat,r.object_id,0,0,0};}
    if(!r.user_found){return {SiegeWarUserActionKind::drop_no_user,r.protocol,r.object_id,0,0,0};}
    switch(r.protocol){
    case siegewar_movein_syn:return {SiegeWarUserActionKind::movein_to_user_map,siegewar_movein_syn,r.object_id,r.unique_connect_idx,0,0};
    case siegewar_battlejoin_syn:case siegewar_observerjoin_syn:
        return r.target_map_found?SiegeWarUserAction{SiegeWarUserActionKind::battlejoin_to_target_map_or_nack,r.protocol,r.object_id,r.unique_connect_idx,0,0}:SiegeWarUserAction{SiegeWarUserActionKind::battlejoin_to_target_map_or_nack,siegewar_battlejoin_nack,r.object_id,0,0,0};
    case siegewar_leave_syn:return {SiegeWarUserActionKind::leave_syn_to_user_map,siegewar_leave_syn,r.object_id,r.unique_connect_idx,0,0};
    default:return {SiegeWarUserActionKind::default_forward_to_map,r.protocol,r.object_id,0,0,0};
    }
}
}
[[maybe_unused]] constexpr int agent_siegewar_translation_unit_anchor=0;
