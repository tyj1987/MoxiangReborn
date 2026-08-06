
//
// D4.114 -- AgentWeather side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_WEATHERUserMsgParser (lines 5016-5034). The data plane (classify_weather) decides
// which action to take; this header captures the ordered side-effect list the
// orchestrator must execute.
//
// Legacy branches:
//   - user not found -> drop_no_user.
//   - user_level not in {GM_CLASS(8), PROGRAMMER(9), DEVELOPER(10)} -> drop_wrong_user_level.
//   - user_level==GM_CLASS but not GM_MASTER/EVENTER -> drop_wrong_user_level.
//   - weather_set: target_map = map_num (low 16 bits).
//   - weather_exe: target_map = data_field.
//   - weather_return: target_map = data_field.
//   - default: forward_to_map_by_data with target_map=0 (fallback).
//
// Side effects:
//   - forward_to_map_by_data: Send2Server(GetServerPort(target_map)->conn_index, pMsg, dwLength).
//   - drop_no_user / drop_wrong_user_level: silent drop.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_weather.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class WeatherSideEffectKind : std::uint8_t {
    Drop,                                  // no_user or wrong_user_level
    ForwardToMapByData,                    // Send2Server via GetServerPort(target_map)->conn_index
};

struct WeatherSideEffect final {
    WeatherSideEffectKind kind = WeatherSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint16_t target_map = 0u;
};

struct WeatherSideEffectPlan final {
    std::vector<WeatherSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool weather_effect_targets_map(const WeatherSideEffect& e) noexcept {
    return e.kind == WeatherSideEffectKind::ForwardToMapByData;
}

inline WeatherSideEffectPlan weather_side_effect_plan(const WeatherAction& a) {
    WeatherSideEffectPlan plan;
    using K = WeatherSideEffectKind;
    using A = WeatherActionKind;
    switch (a.kind) {
        case A::drop_no_user:
        case A::drop_wrong_user_level:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.connection_index, 0u});
            return plan;
        case A::forward_to_map_by_data:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardToMapByData, a.protocol, a.connection_index, a.target_map});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server