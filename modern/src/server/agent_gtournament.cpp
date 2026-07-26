#include "mxh/server/agent_gtournament.hpp"
namespace mxh::server {
// MP_GTOURNAMENTUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4286-4295.
GtournamentAction classify_gtournament_user(const GtournamentRequest& r){
    if(!r.user_found){return {GtournamentActionKind::drop_no_user,r.protocol,r.object_id,0,0};}
    switch(r.protocol){
    case gtournament_movetobattlemap_syn:
        return r.user_map_found?GtournamentAction{GtournamentActionKind::send_movetobattle_to_user_map,gtournament_movetobattlemap_syn,r.object_id,0,0}:GtournamentAction{GtournamentActionKind::send_movetobattle_nack_to_user,gtournament_movetobattlemap_nack,r.object_id,gt_error_code_error,0};
    case gtournament_standinginfo_syn:
        return r.gt_map_found?GtournamentAction{GtournamentActionKind::send_standing_info_to_gt_map,gtournament_standinginfo_syn,r.object_id,0,gt_map_num}:GtournamentAction{GtournamentActionKind::send_standing_info_nack_to_user,gtournament_standinginfo_nack,r.object_id,gt_error_code_error,0};
    case gtournament_battlejoin_syn:case gtournament_observerjoin_syn:
        return r.gt_map_found?GtournamentAction{GtournamentActionKind::send_standing_info_to_gt_map,r.protocol,r.object_id,0,gt_map_num}:GtournamentAction{GtournamentActionKind::send_battlejoin_nack_to_user,gtournament_battlejoin_nack,r.object_id,gt_error_code_error,0};
    case gtournament_leave_syn:return {GtournamentActionKind::send_leave_syn_to_user_map,gtournament_leave_syn,r.object_id,0,0};
    case gtournament_cheat:return r.cheat_data==1?GtournamentAction{GtournamentActionKind::send_cheat_to_user_map,gtournament_cheat,r.object_id,0,0}:(r.gt_map_found?GtournamentAction{GtournamentActionKind::send_cheat_to_gt_map,gtournament_cheat,r.object_id,0,gt_map_num}:GtournamentAction{GtournamentActionKind::drop_no_user,gtournament_cheat,r.object_id,0,0});
    case gtournament_event_start:case gtournament_event_end:
        if(r.user_level>8){return {GtournamentActionKind::drop_no_user,r.protocol,r.object_id,0,0};}
        return r.gt_map_found?GtournamentAction{GtournamentActionKind::send_event_to_gt_map,r.protocol,r.object_id,0,gt_map_num}:GtournamentAction{GtournamentActionKind::drop_no_user,r.protocol,r.object_id,0,0};
    default:return {GtournamentActionKind::forward_to_map_server,r.protocol,r.object_id,0,0};
    }
}
}
[[maybe_unused]] constexpr int agent_gtournament_translation_unit_anchor=0;
