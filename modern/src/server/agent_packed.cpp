#include "mxh/server/agent_packed.hpp"
namespace mxh::server {
// MP_PACKEDMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 2074-2100.
PackedAction classify_packed_user(const PackedRequest& r){
    switch(r.protocol){
    case packed_normal:return {PackedActionKind::fanout_to_users,packed_normal,0,r.receivers_present.size(),r.data_size};
    case packed_to_mapserver:return r.target_map_port_found?PackedAction{PackedActionKind::send_to_map_server_by_port,packed_to_mapserver,0,r.receiver_count,r.data_size}:PackedAction{};
    case packed_to_broad_mapserver:return {PackedActionKind::broadcast_to_other_maps,packed_to_broad_mapserver,0,r.receiver_count,r.data_size};
    default:return {};
    }
}
}
[[maybe_unused]] constexpr int agent_packed_translation_unit_anchor=0;
