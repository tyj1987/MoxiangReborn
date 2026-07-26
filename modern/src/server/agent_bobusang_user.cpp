#include "mxh/server/agent_bobusang_user.hpp"
namespace mxh::server {
// MP_BOBUSANGUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5159-5190.
BobusangUserAction classify_bobusang_user(const BobusangUserRequest& r){
    if(!r.user_found){return {BobusangUserActionKind::drop_no_user,r.protocol,r.object_id};}
    if(r.is_gm&&!r.gm_master_or_below){return {BobusangUserActionKind::drop_wrong_gm_power,r.protocol,r.object_id};}
    return {BobusangUserActionKind::forward_to_map,r.protocol,r.object_id};
}
}
[[maybe_unused]] constexpr int agent_bobusang_user_translation_unit_anchor=0;
