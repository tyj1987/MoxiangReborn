#include "mxh/server/agent_siegewarprofit.hpp"
namespace mxh::server {
// MP_SIEGEWARPROFITUserMsgParser routing is unconditional forward-to-map.
SiegeWarProfitUserAction classify_siegewarprofit_user(){return {SiegeWarProfitUserActionKind::forward_to_map};}
// MP_SIEGEWARPROFITServerMsgParser per legacy [Server]Agent/AgentNetworkMsgParser.cpp.
SiegeWarProfitServerAction classify_siegewarprofit_server(const SiegeWarProfitRequest& r){
    if(r.protocol==siegewarprofit_change_texrate_notify_to_map||r.protocol==siegewarprofit_change_guild_notify_to_map){return {SiegeWarProfitServerActionKind::broadcast_to_other_maps,r.protocol};}
    return {SiegeWarProfitServerActionKind::forward_to_client,r.protocol};
}
}
[[maybe_unused]] constexpr int agent_siegewarprofit_translation_unit_anchor=0;
