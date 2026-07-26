
#include "mxh/server/agent_murimnet.hpp"
namespace mxh::server {
MurimNetAction classify_murimnet_user(const MurimNetUserRequest& r){if(r.protocol==murimnet_changetomurimnet_syn){return r.lookup.port.has_value()?MurimNetAction{MurimNetActionKind::forward_to_map,r.protocol,r.character_id}:MurimNetAction{MurimNetActionKind::send_nack,murimnet_changetomurimnet_nack,r.character_id};}if(r.protocol==murimnet_connect_syn||r.protocol==murimnet_reconnect_syn){return {MurimNetActionKind::forward_to_map,r.protocol,r.character_id};}return {MurimNetActionKind::forward_to_map,r.protocol,r.character_id};}
MurimNetAction classify_murimnet_server(const MurimNetServerRequest& r){if(r.protocol==murimnet_changetomurimnet_ack||r.protocol==murimnet_returntomurimnet_ack){return r.lookup.port.has_value()?MurimNetAction{MurimNetActionKind::send_ack,r.protocol,r.character_id}:MurimNetAction{MurimNetActionKind::send_nack,r.protocol==murimnet_changetomurimnet_ack?murimnet_changetomurimnet_nack:murimnet_returntomurimnet_nack,r.character_id};}if(r.protocol==murimnet_pr_start||r.protocol==murimnet_disconnect_ack){return {MurimNetActionKind::forward_to_map,r.protocol,r.character_id};}return {MurimNetActionKind::forward_to_map,r.protocol,r.character_id};}
}
[[maybe_unused]] constexpr int agent_murimnet_translation_unit_anchor=0;
