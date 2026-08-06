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

GuildUnionServerAction classify_guild_union_server(const GuildUnionServerRequest& r){
    switch(r.protocol){
    case guild_union_create_notify_to_map:
    case guild_union_destroy_notify_to_map:
    case guild_union_invite_accept_notify_to_map:
    case guild_union_add_notify_to_map:
    case guild_union_remove_notify_to_map:
    case guild_union_secede_notify_to_map:
    case guild_union_mark_regist_notify_to_map:
        return {GuildUnionServerActionKind::broadcast_to_other_maps,r.protocol};
    default:
        return {GuildUnionServerActionKind::drop_unknown,r.protocol};
    }
}
}
[[maybe_unused]] constexpr int agent_guild_union_translation_unit_anchor=0;
