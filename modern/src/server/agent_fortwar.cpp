#include "mxh/server/agent_fortwar.hpp"
namespace mxh::server {
// MP_FORTWARServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5290-5452.
FortWarAction classify_fortwar(const FortWarRequest& r){
    switch(r.protocol){
    case fortwar_start_before10min:case fortwar_start:case fortwar_end:
        return {FortWarActionKind::broadcast_to_all_users,r.protocol};
    case fortwar_start_before10min_to_map:case fortwar_start_to_map:case fortwar_ing_to_map:case fortwar_end_to_map:
        return {FortWarActionKind::broadcast_to_other_maps,r.protocol};
    case fortwar_info:case fortwar_ing:default:
        return r.user_object_found?FortWarAction{FortWarActionKind::forward_to_user_if_found,r.protocol}:FortWarAction{FortWarActionKind::drop_no_user,r.protocol};
    }
}
}
[[maybe_unused]] constexpr int agent_fortwar_translation_unit_anchor=0;
