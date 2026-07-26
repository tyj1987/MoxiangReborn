#include "mxh/server/agent_item.hpp"
namespace mxh::server {
// MP_ITEMUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4148-4206.
ItemUserAction classify_item_user(const ItemUserRequest& r){
    switch(r.protocol){
    case item_shopitem_changemap_syn:return {ItemUserActionKind::forward_to_map,item_shopitem_changemap_syn,r.object_id,0,true};
    case item_shopitem_nchange_syn:
        if(!r.user_found){return {ItemUserActionKind::send_nack_to_user,item_shopitem_nchange_nack,r.object_id,item_name_nack_code,false};}
        if(r.name_length<4||r.name_length>20){return {ItemUserActionKind::send_nack_to_user,item_shopitem_nchange_nack,r.object_id,item_name_nack_code,false};}
        if(!r.name_usable||r.name_has_invalid_char){return {ItemUserActionKind::send_nack_to_user,item_shopitem_nchange_nack,r.object_id,item_name_nack_code,false};}
        return {ItemUserActionKind::forward_to_map_if_name_valid,item_shopitem_nchange_syn,r.object_id,0,false};
    case item_shopitem_chase_syn:return {ItemUserActionKind::send_chase_lookup,item_shopitem_chase_syn,r.object_id,0,false};
    default:return {ItemUserActionKind::forward_to_map,r.protocol,r.object_id,0,false};
    }
}
// MP_ITEMUserMsgParserExt is a pure pass-through to map.
ItemUserAction classify_item_user_ext(std::uint8_t p){return {ItemUserActionKind::forward_to_map,p,0,0,false};}
// MP_ITEMServerMsgParser routing per legacy lines 4214-4286.
ItemServerAction classify_item_server(const ItemServerRequest& r){
    switch(r.protocol){
    case item_shopitem_changemap_syn:return {ItemServerActionKind::forward_to_client,item_shopitem_changemap_syn,0,0,false};
    case item_shopitem_chase_ack:return {ItemServerActionKind::forward_to_user,item_shopitem_chase_ack,r.data,0,false};
    case item_shopitem_chase_nack:return {ItemServerActionKind::send_chase_nack_to_user,item_shopitem_chase_nack,r.data,item_chase_nack_data,false};
    case item_shopitem_shout_ack:return {ItemServerActionKind::shout_ack_with_broadcast,item_shopitem_shout_ack,r.data,0,r.shout_buffer_full};
    case item_shopitem_shout_sendserver:return {ItemServerActionKind::shout_add_only,item_shopitem_shout_sendserver,r.data,0,false};
    default:return {ItemServerActionKind::forward_to_client,r.protocol,0,0,false};
    }
}
// MP_ITEMServerMsgParserExt is a pure pass-through to client.
ItemServerAction classify_item_server_ext(std::uint8_t p){return {ItemServerActionKind::forward_to_client,p,0,0,false};}
}
[[maybe_unused]] constexpr int agent_item_translation_unit_anchor=0;
