#include "mxh/server/agent_simple_auth_forward.hpp"
namespace mxh::server {
// MP_STREETSTALLUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5034-5082.
StreetStallUserAction classify_streetstall_user(const StreetStallUserRequest& r){
    if(!r.user_found){return {StreetStallUserActionKind::drop_no_user,0,r.connection_index};}
    if(r.character_id!=r.object_id){return {StreetStallUserActionKind::drop_object_mismatch,0,r.connection_index};}
    return {StreetStallUserActionKind::forward_to_map,0,r.connection_index};
}
// MP_EXCHANGEUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5082-5094.
ExchangeUserAction classify_exchange_user(const ExchangeUserRequest& r){
    if(!r.user_found){return {ExchangeUserActionKind::drop_no_user,0,r.connection_index};}
    if(r.character_id!=r.object_id){return {ExchangeUserActionKind::drop_object_mismatch,0,r.connection_index};}
    return {ExchangeUserActionKind::forward_to_map,0,r.connection_index};
}
}
[[maybe_unused]] constexpr int agent_simple_auth_forward_translation_unit_anchor=0;
