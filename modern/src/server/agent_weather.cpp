#include "mxh/server/agent_weather.hpp"
namespace mxh::server {
// MP_WEATHERUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5016-5034.
WeatherAction classify_weather(const WeatherRequest& r){
    if(!r.user_found){return {WeatherActionKind::drop_no_user,r.protocol,r.connection_index,0};}
    const bool gm_class=r.user_level==user_level_gm_weather;
    const bool programmer=r.user_level==user_level_programmer;
    const bool developer=r.user_level==user_level_developer;
    if(!(gm_class||programmer||developer)){return {WeatherActionKind::drop_wrong_user_level,r.protocol,r.connection_index,0};}
    if(gm_class&&!r.is_gm_master_or_eventer){return {WeatherActionKind::drop_wrong_user_level,r.protocol,r.connection_index,0};}
    std::uint16_t wServer=0;
    switch(r.protocol){
    case weather_set:wServer=static_cast<std::uint16_t>(r.map_num&0xFFFFu);break;
    case weather_exe:wServer=r.data_field;break;
    case weather_return:wServer=r.data_field;break;
    default:return {WeatherActionKind::forward_to_map_by_data,r.protocol,r.connection_index,0};
    }
    return {WeatherActionKind::forward_to_map_by_data,r.protocol,r.connection_index,wServer};
}
}
[[maybe_unused]] constexpr int agent_weather_translation_unit_anchor=0;
