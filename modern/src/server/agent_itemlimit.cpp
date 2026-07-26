#include "mxh/server/agent_itemlimit.hpp"
namespace mxh::server {
// MP_ITEMLIMITServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5211-5235.
ItemLimitAction classify_itemlimit(const ItemLimitRequest& r){
    if(r.protocol==itemlimit_addcount_to_map){return {ItemLimitActionKind::broadcast_to_other_maps,itemlimit_addcount_to_map};}
    return {ItemLimitActionKind::forward_to_client,r.protocol};
}
}
[[maybe_unused]] constexpr int agent_itemlimit_translation_unit_anchor=0;
