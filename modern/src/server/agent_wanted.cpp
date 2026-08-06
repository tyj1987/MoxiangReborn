#include "mxh/server/agent_wanted.hpp"
namespace mxh::server {
// MP_WANTEDServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp (around line 2397).
WantedAction classify_wanted(const WantedRequest& r){
    switch(r.protocol){
    case wanted_notify_delete_to_map:case wanted_notify_regist_to_map:case wanted_notify_notcomplete_to_map:case wanted_destroyed_to_map:
        return {WantedServerActionKind::broadcast_to_other_maps,r.protocol,0};
    case wanted_notcomplete_to_agent:
        if(!r.user_found){return {WantedServerActionKind::drop_no_user,wanted_notcomplete_to_agent,r.object_id};}
        return {WantedServerActionKind::complete_notcomplete_send_to_map,wanted_notcomplete_by_delchr,r.object_id,r.target_map_connection_index};
    default:
        return {WantedServerActionKind::default_forward_to_client,r.protocol,0};
    }
}
}
[[maybe_unused]] constexpr int agent_wanted_translation_unit_anchor=0;
