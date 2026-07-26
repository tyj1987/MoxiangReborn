#include "mxh/server/agent_hackcheck.hpp"
namespace mxh::server {
// MP_HACKCHECKMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 3683-3740.
HackCheckAction classify_hackcheck(const HackCheckRequest& r){
    switch(r.protocol){
    case hackcheck_speedhack:
        if(!r.user_found){return {HackCheckActionKind::drop_no_user,hackcheck_speedhack,r.object_id,0};}
        return (r.server_time>=r.client_time&&(r.server_time-r.client_time)<(speedhack_checktime-speedhack_tolerance_ms))?HackCheckAction{HackCheckActionKind::detect_speedhack_and_ban,hackcheck_ban_user,r.object_id,r.server_time-r.client_time}:HackCheckAction{HackCheckActionKind::ignore,hackcheck_speedhack,r.object_id,0};
    case hackcheck_ban_user_toagent:
        if(!r.user_found){return {HackCheckActionKind::drop_no_user,hackcheck_ban_user_toagent,r.object_id,0};}
        return {HackCheckActionKind::ban_user_to_agent_always,hackcheck_ban_user,r.object_id,0};
    default:return {HackCheckActionKind::ignore,r.protocol,r.object_id,0};
    }
}
}
[[maybe_unused]] constexpr int agent_hackcheck_translation_unit_anchor=0;
