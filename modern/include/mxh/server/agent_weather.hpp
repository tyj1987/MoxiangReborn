#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_WEATHER (MP_WEATHER=64).
inline constexpr std::uint8_t weather_category=64;
// Sub-protocols within MP_PROTOCOL_WEATHER (offset 0..3).
inline constexpr std::uint8_t weather_set=0,weather_exe=1,weather_return=2,weather_state=3;
// GM levels permitted to drive weather (PROGRAMMER and DEVELOPER bypass GM-power sub-check).
inline constexpr std::uint8_t user_level_gm_weather=8,user_level_programmer=9,user_level_developer=10;
enum class WeatherActionKind : std::uint8_t { forward_to_map_by_data, drop_no_user, drop_wrong_user_level };
struct WeatherRequest { std::uint8_t protocol=0; std::uint32_t connection_index=0; bool user_found=true; std::uint8_t user_level=0; std::uint16_t data_field=0; bool is_gm_master_or_eventer=false; std::uint32_t map_num=0; };
struct WeatherAction { WeatherActionKind kind=WeatherActionKind::drop_no_user; std::uint8_t protocol=0; std::uint32_t connection_index=0; std::uint16_t target_map=0; };
WeatherAction classify_weather(const WeatherRequest&);
}
