#include "mxh/server/agent_guild_union.hpp"
namespace mxh::server {
// MP_GUILD_UNIONUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp.
GuildUnionAction classify_guild_union_user(const GuildUnionRequest& r){
    if(!r.user_found){return {GuildUnionActionKind::drop_no_user,r.protocol,r.object_id,0};}
    if(r.protocol==guild_union_create_syn){
        if(!r.name_usable||r.has_invalid_char){return {GuildUnionActionKind::send_create_nack_to_user,guild_union_create_nack,r.object_id,guild_union_err_not_valid_name};}
        return {GuildUnionActionKind::forward_to_map,guild_union_create_syn,r.object_id,0};
    }
    return {GuildUnionActionKind::forward_to_map,r.protocol,r.object_id,0};
}
}
[[maybe_unused]] constexpr int agent_guild_union_translation_unit_anchor=0;
